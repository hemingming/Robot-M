#include <Arduino.h>
#include <Wire.h>
#include <cmath>

#include "drivers/audio_i2s.h"
#include "drivers/display_st7796.h"
#include "actuators/servo_bus.h"
#include "hal/i2c_bus.h"
#include "manager/robot_runtime.h"
#include "motion/limb_controller.h"
#include "network/wifi_handler.h"
#include "voice/asr_uart.h"
#include "robot_config.h"

#if __has_include(<SparkFun_VL53L1X.h>)
#include <SparkFun_VL53L1X.h>
// ToF 对象只在对应库存在时编译，便于在尚未安装传感器库时先运行显示和音频功能。
static SFEVL53L1X g_tofSensor;
static bool g_tofReady = false;
#endif

namespace {

// 运行时状态机实例。硬件采集结果通过 SensorSnapshot 传入状态机。
robot::RobotRuntime g_runtime;
robot::actuators::ServoBus g_servoBus;
robot::motion::LimbController g_limbController(g_servoBus);
robot::voice::AsrUart g_asrUart;

#if __has_include(<SparkFun_VL53L1X.h>)
uint16_t g_lastTofDistanceMm = 0;
uint8_t g_lastTofStatus = 255;
#endif

// ADC 读到的是分压后的引脚电压，必须乘回分压比才是电池真实电压。
// 所有电池电压读数统一走这里，避免换算逻辑散落在状态采样和诊断中。
// 监控被禁用时返回一个安全电压（8.0V，远高于 6.6V 阈值），
// 避免悬空引脚被误读为低电压导致状态机锁入 EmergencyStop。
float readBatteryVoltage() {
  if (!robot::config::kBatteryMonitoringEnabled) {
    return 8.0f;
  }
  return analogReadMilliVolts(robot::kBatteryVoltagePin) / 1000.0f *
         robot::config::kBatteryAdcDividerRatio;
}

void printBatteryDiagnostic() {
  if (!robot::config::kBatteryMonitoringEnabled) {
    Serial.println("Battery monitoring disabled (kBatteryMonitoringEnabled=false).");
    return;
  }
  // 同时打印原始 ADC 值、Arduino 校准后的毫伏值和换算后的电池电压，便于校准分压比例。
  const int rawValue = analogRead(robot::kBatteryVoltagePin);
  const uint32_t millivolts = analogReadMilliVolts(robot::kBatteryVoltagePin);
  Serial.printf("Battery ADC GPIO %u: raw=%d, calibrated=%lu mV, battery=%.2f V\n",
                robot::kBatteryVoltagePin, rawValue,
                static_cast<unsigned long>(millivolts), readBatteryVoltage());
}

void handleServoConsole() {
  if (!Serial.available()) {
    return;
  }

  const String command = Serial.readStringUntil('\n');
  int id = 0;
  int position = 0;
  int speed = 0;
  int acceleration = 0;

  if (command.startsWith("servo ping ")) {
    id = command.substring(11).toInt();
    Serial.printf("Servo %d ping: %s\n", id,
                  g_servoBus.ping(static_cast<uint8_t>(id)) ? "OK" : "FAIL");
  } else if (command.startsWith("robot stand")) {
    g_limbController.stand();
    g_runtime.requestMode(robot::RobotMode::Standby);
    Serial.println("Robot stand command sent.");
  } else if (command.startsWith("robot forward")) {
    g_limbController.startForward();
    g_runtime.requestMode(robot::RobotMode::Walking);
    Serial.println("Robot forward started.");
  } else if (command.startsWith("robot turn left")) {
    g_limbController.startTurnLeft();
    g_runtime.requestMode(robot::RobotMode::Walking);
    Serial.println("Robot turn left started.");
  } else if (command.startsWith("robot turn right")) {
    g_limbController.startTurnRight();
    g_runtime.requestMode(robot::RobotMode::Walking);
    Serial.println("Robot turn right started.");
  } else if (command.startsWith("robot stop")) {
    g_limbController.stop();
    g_runtime.requestMode(robot::RobotMode::Standby);
    Serial.println("Robot stop sent, mode set to standby.");
  } else if (command.startsWith("i2c scan")) {
    if (!robot::config::kI2cEnabled) {
      Serial.println("I2C disabled (kI2cEnabled=false).");
      return;
    }
    robot::hal::i2cScan();
  } else if (command.startsWith("battery cal ")) {
    if (!robot::config::kBatteryMonitoringEnabled) {
      Serial.println("Battery monitoring disabled; cannot calibrate.");
      Serial.println("Enable kBatteryMonitoringEnabled and wire GPIO4 to the divider first.");
    } else {
    const float realVoltage = command.substring(12).toFloat();
    if (realVoltage <= 0.1f) {
      Serial.println("Usage: battery cal <real_voltage_from_multimeter>");
      Serial.println("Example: battery cal 7.80");
    } else {
      // 多次采样取平均，降低 ADC 单次抖动对校准结果的影响。
      uint32_t sumMv = 0;
      const int samples = 8;
      for (int i = 0; i < samples; ++i) {
        sumMv += analogReadMilliVolts(robot::kBatteryVoltagePin);
        delay(5);
      }
      const float adcPinV =
          (sumMv / static_cast<float>(samples)) / 1000.0f;
      const float recommended =
          (adcPinV > 0.001f) ? (realVoltage / adcPinV) : 0.0f;
      Serial.printf("ADC pin (avg of %d): %.3f V\n", samples, adcPinV);
      Serial.printf("Real battery (multimeter): %.2f V\n", realVoltage);
      Serial.printf("-> Recommended kBatteryAdcDividerRatio = %.4ff\n",
                    recommended);
      Serial.printf("Current ratio = %.4ff -> battery = %.2f V\n",
                    robot::config::kBatteryAdcDividerRatio,
                    readBatteryVoltage());
      Serial.println("Edit src/config/project_config.h and rebuild to apply.");
    }
    }
  } else if (command.startsWith("battery")) {
    printBatteryDiagnostic();
  } else if (command.startsWith("imu read")) {
    if (!robot::config::kI2cEnabled) {
      Serial.println("I2C disabled (kI2cEnabled=false).");
      return;
    }
    robot::hal::Bmi323RawData data;
    if (robot::hal::readBmi323Raw(data)) {
      Serial.printf("BMI323 chip=0x%04X accel=(%d,%d,%d) gyro=(%d,%d,%d)\n",
                    data.chipId, data.accelX, data.accelY, data.accelZ,
                    data.gyroX, data.gyroY, data.gyroZ);
      Serial.printf("BMI323 units accel=(%.3f,%.3f,%.3f) g gyro=(%.2f,%.2f,%.2f) deg/s\n",
                    data.accelX / 8192.0f, data.accelY / 8192.0f,
                    data.accelZ / 8192.0f, data.gyroX / 65.536f,
                    data.gyroY / 65.536f, data.gyroZ / 65.536f);
    } else {
      Serial.println("BMI323 read failed.");
    }
  } else if (command.startsWith("servo scan")) {
    int firstId = 1;
    int lastId = 20;
    sscanf(command.c_str() + 10, "%d %d", &firstId, &lastId);
    firstId = constrain(firstId, 1, 253);
    lastId = constrain(lastId, firstId, 253);
    Serial.printf("Scanning servo IDs %d..%d...\n", firstId, lastId);
    int found = 0;
    for (int scanId = firstId; scanId <= lastId; ++scanId) {
      if (g_servoBus.ping(static_cast<uint8_t>(scanId))) {
        Serial.printf("Servo %d: OK\n", scanId);
        ++found;
      }
    }
    Serial.printf("Scan complete: %d servo(s) found.\n", found);
  } else if (command.startsWith("servo setid ")) {
    int currentId = 0;
    int newId = 0;
    if (sscanf(command.c_str() + 12, "%d %d", &currentId, &newId) == 2) {
      Serial.printf("Servo set ID %d -> %d: %s; power-cycle before testing.\n",
                    currentId, newId,
                    g_servoBus.setId(static_cast<uint8_t>(currentId),
                                     static_cast<uint8_t>(newId))
                        ? "OK"
                        : "FAIL");
    } else {
      Serial.println("Usage: servo setid <current_id> <new_id>");
    }
  } else if (command.startsWith("servo move ") &&
             sscanf(command.c_str() + 11, "%d %d %d %d", &id, &position,
                    &speed, &acceleration) == 4) {
    robot::actuators::ServoTarget target;
    target.id = static_cast<uint8_t>(id);
    target.position = static_cast<int16_t>(position);
    target.speed = static_cast<uint16_t>(speed);
    target.acceleration = static_cast<uint16_t>(acceleration);
    Serial.printf("Servo %d move: %s\n", id,
                  g_servoBus.sendTarget(target) ? "OK" : "FAIL");
  } else if (command.startsWith("servo center ")) {
    id = command.substring(13).toInt();
    robot::actuators::ServoTarget target;
    target.id = static_cast<uint8_t>(id);
    target.position = 512;
    target.speed = 100;
    target.acceleration = 2;
    Serial.printf("Servo %d center: %s\n", id,
                  g_servoBus.sendTarget(target) ? "OK" : "FAIL");
  } else if (command.startsWith("servo sweep ")) {
    int fromPosition = 0;
    int toPosition = 0;
    if (sscanf(command.c_str() + 12, "%d %d %d", &id, &fromPosition,
               &toPosition) == 3 &&
        id > 0 && id != 0xFE) {
      const int step = (toPosition >= fromPosition) ? 10 : -10;
      bool failed = false;
      for (int pos = fromPosition;
           !failed && (step > 0 ? pos <= toPosition : pos >= toPosition);
           pos += step) {
        robot::actuators::ServoTarget target;
        target.id = static_cast<uint8_t>(id);
        target.position = static_cast<int16_t>(constrain(pos, 0, 1023));
        target.speed = 300;
        target.acceleration = 2;
        if (!g_servoBus.sendTarget(target)) {
          Serial.printf("Servo %d sweep failed at %d\n", id, pos);
          failed = true;
        }
        delay(80);
      }
      if (!failed) {
        Serial.printf("Servo %d sweep %d -> %d done.\n", id, fromPosition,
                      toPosition);
      }
    } else {
      Serial.println("Usage: servo sweep <id> <from> <to>  (0..1023)");
    }
  } else if (command.startsWith("servo stop")) {
    g_servoBus.stopAll();
    Serial.println("Servo stop sent.");
  } else {
    Serial.println("Commands: battery; battery cal <real_voltage>; i2c scan; imu read; robot stand; robot forward; robot turn left; robot turn right; robot stop; servo ping <id>; servo scan [first] [last]; servo setid <old> <new>; servo move <id> <position> <speed> <acc>; servo center <id>; servo sweep <id> <from> <to>; servo stop");
  }
}

void initSerial() {
  // 固件诊断统一使用 115200 波特率。
  Serial.begin(115200);
  delay(100);
  Serial.println("Robot-M booting...");
}

void initPower() {
  // 先打开外设供电，随后再初始化 I2C、显示屏和传感器。
  pinMode(robot::kPowerEnablePin, OUTPUT);
  digitalWrite(robot::kPowerEnablePin, HIGH);
}

void initAsr() {
  g_asrUart.onAction([](robot::voice::AsrAction action) {
    // 唤醒词和停止指令始终生效；其余动作指令必须在唤醒/行走窗口内才会被执行，
    // 避免 ASR 模块把环境噪音误识别成动作指令时机器人擅自动作。
    if (action == robot::voice::AsrAction::Wake) {
      g_runtime.noteWakeWord(millis());
      robot::drivers::playWakeChirp();
      Serial.println("ASR wake word detected, listening window opened.");
      return;
    }
    if (action == robot::voice::AsrAction::Stop) {
      g_limbController.stop();
      g_runtime.requestMode(robot::RobotMode::Standby);
      return;
    }
    if (!g_runtime.isAwake()) {
      Serial.println("ASR action ignored: robot not woken up yet.");
      return;
    }
    switch (action) {
      case robot::voice::AsrAction::Forward:
        g_limbController.startForward();
        g_runtime.requestMode(robot::RobotMode::Walking);
        break;
      case robot::voice::AsrAction::TurnLeft:
        g_limbController.startTurnLeft();
        g_runtime.requestMode(robot::RobotMode::Walking);
        break;
      case robot::voice::AsrAction::TurnRight:
        g_limbController.startTurnRight();
        g_runtime.requestMode(robot::RobotMode::Walking);
        break;
      case robot::voice::AsrAction::Stand:
        g_limbController.stand();
        g_runtime.requestMode(robot::RobotMode::Standby);
        break;
      case robot::voice::AsrAction::Wake:
      case robot::voice::AsrAction::Stop:
        break;
    }
  });
  g_asrUart.begin();
}

void initSensors() {
  // TCRT5000 使用比较器数字输出，GPIO 电平会随反射强弱变化。
  pinMode(robot::kTcrtLeftDigitalPin, INPUT);
  pinMode(robot::kTcrtRightDigitalPin, INPUT);

  // ToF 挂在 Wire 总线上，I2C 未启用时同样不能访问，否则 begin 必然失败刷日志。
  if (!robot::kSensorsEnabled || !robot::config::kI2cEnabled) {
    Serial.println("Sensors disabled.");
    return;
  }
#if __has_include(<SparkFun_VL53L1X.h>)
  // VL53L1X 使用已初始化的 Wire 总线；这里暂时只启动测距，不把数据写入状态机。
  // ToF 传感器与其他 I2C 外设共用 Wire 总线。
  g_tofReady = g_tofSensor.begin(Wire);
  Serial.printf("VL53L1X begin: %s\n", g_tofReady ? "OK" : "FAIL");
  if (g_tofReady) {
    g_tofSensor.startRanging();
  }
#endif
}

void updateRuntime(uint32_t nowMs) {
  // 硬件层先收集一份普通数据快照，再交给状态机统一决定模式。
  // 这样 RobotRuntime 不需要依赖 ADC、I2C 或具体传感器驱动。
  robot::SensorSnapshot sensors;
  sensors.batteryVoltage = readBatteryVoltage();
  sensors.servoBusHealthy = g_servoBus.healthy();

  if (robot::config::kI2cEnabled) {
    robot::hal::Bmi323RawData imuData;
    if (robot::hal::readBmi323Raw(imuData)) {
      const float accelX = imuData.accelX / 8192.0f;
      const float accelY = imuData.accelY / 8192.0f;
      const float accelZ = imuData.accelZ / 8192.0f;
      sensors.rollDegrees = atan2f(accelY, accelZ) * 57.29578f;
      sensors.pitchDegrees =
          atan2f(-accelX, sqrtf(accelY * accelY + accelZ * accelZ)) * 57.29578f;
      sensors.yawRateDegreesPerSecond = imuData.gyroZ / 65.536f;
      sensors.imuHealthy = imuData.chipId == 0x0043;
    }
  } else {
    // I2C 未启用时视 IMU 为健康，避免 Walking 状态被误判为姿态异常。
    sensors.imuHealthy = true;
  }

#if __has_include(<SparkFun_VL53L1X.h>)
  sensors.obstacleDetected = g_lastTofStatus == 0 &&
                             g_lastTofDistanceMm > 0 &&
                             g_lastTofDistanceMm < 200;
#endif

  g_runtime.update(nowMs, sensors);

  if (g_runtime.modeChanged()) {
    Serial.printf("Mode: %s\n", robot::robotModeName(g_runtime.mode()));
    if (g_runtime.mode() == robot::RobotMode::Recovery ||
        g_runtime.mode() == robot::RobotMode::EmergencyStop) {
      g_limbController.stop();
    }
  }
  if (g_runtime.mode() == robot::RobotMode::Walking) {
    g_limbController.update(nowMs);
  }
}

void printStatus() {
  // 周期性输出最小诊断信息，避免影响主循环时序。
  const float batteryVoltage = readBatteryVoltage();
  Serial.printf("Battery: %.2f V\n", batteryVoltage);
  static int lastLeftTcrt = -1;
  static int lastRightTcrt = -1;
  const int leftTcrt = digitalRead(robot::kTcrtLeftDigitalPin);
  const int rightTcrt = digitalRead(robot::kTcrtRightDigitalPin);
  if (leftTcrt != lastLeftTcrt || rightTcrt != lastRightTcrt) {
    lastLeftTcrt = leftTcrt;
    lastRightTcrt = rightTcrt;
    Serial.printf("TCRT5000: left=%d right=%d\n", leftTcrt, rightTcrt);
  }

#if __has_include(<SparkFun_VL53L1X.h>)
  // 传感器未启用时直接返回，避免在调试模式下读取尚未初始化的设备。
  if (!robot::kSensorsEnabled) {
    return;
  }
  static uint32_t tofNotReadyCount = 0;
  if (g_tofReady && g_tofSensor.checkForDataReady()) {
    g_lastTofDistanceMm = g_tofSensor.getDistance();
    g_lastTofStatus = g_tofSensor.getRangeStatus();
    Serial.printf("Distance: %u mm, Range status: %u\n", g_lastTofDistanceMm,
                  g_lastTofStatus);
    g_tofSensor.clearInterrupt();
  } else if (g_tofReady && ++tofNotReadyCount % 25 == 0) {
    Serial.println("VL53L1X data not ready");
  }
#endif
}

}  // namespace

void setup() {
  // 初始化顺序：串口 -> 供电 -> 总线 -> 外设 -> 运行时状态机。
  initSerial();
  // Wi-Fi 在启动早期初始化；失败不会阻止本地显示、音频和状态机继续工作。
  robot::network::initWifi();
  initPower();
  g_servoBus.begin();
  g_limbController.begin();
  initAsr();
  if (robot::config::kI2cEnabled) {
    robot::hal::initI2cBus();
    robot::hal::printI2cDeviceDiagnostics();
  } else {
    Serial.println("I2C disabled (kI2cEnabled=false).");
  }
  printBatteryDiagnostic();
  robot::drivers::initDisplay();
  initSensors();
  robot::drivers::initAudio();
  robot::drivers::playAudioTestTone();
  g_runtime.begin(millis());

  Serial.println("Robot-M initialization complete.");
}

void loop() {
  // 使用差值判断处理 millis() 溢出，状态更新周期由配置文件统一控制。
  static uint32_t lastStatusMs = 0;
  const uint32_t now = millis();
  handleServoConsole();
  g_asrUart.update();

  // 使用非阻塞周期任务，主循环仍保持短周期运行，避免影响音频采样和其他实时工作。
  if (now - lastStatusMs >= robot::kStatusUpdateMs) {
    lastStatusMs = now;
    updateRuntime(now);
    printStatus();
    const uint32_t microphoneLevel = robot::drivers::readMicrophoneLevel();
    Serial.printf("Microphone level: %lu\n",
            static_cast<unsigned long>(microphoneLevel));
    robot::drivers::updateMicrophoneDisplay(microphoneLevel);
  }

  delay(10);
}
