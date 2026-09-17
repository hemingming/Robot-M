#include <Arduino.h>
#include <Wire.h>

#include "drivers/audio_i2s.h"
#include "drivers/display_st7796.h"
#include "hal/i2c_bus.h"
#include "manager/robot_runtime.h"
#include "network/wifi_handler.h"
#include "robot_config.h"

#if __has_include(<SparkFun_VL53L1X.h>)
#include <SparkFun_VL53L1X.h>
// ToF 对象只在对应库存在时编译，便于在尚未安装传感器库时先运行显示和音频功能。
static SFEVL53L1X g_tofSensor;
#endif

namespace {

// 运行时状态机实例。硬件采集结果通过 SensorSnapshot 传入状态机。
robot::RobotRuntime g_runtime;

void printBatteryDiagnostic() {
  // 同时打印原始 ADC 值和 Arduino 校准后的毫伏值，便于校准分压比例。
  const int rawValue = analogRead(robot::kBatteryVoltagePin);
  const uint32_t millivolts = analogReadMilliVolts(robot::kBatteryVoltagePin);
  Serial.printf("Battery ADC GPIO %u: raw=%d, calibrated=%lu mV\n",
                robot::kBatteryVoltagePin, rawValue,
                static_cast<unsigned long>(millivolts));
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

void initSensors() {
  // 传感器开关用于显示屏/音频调试阶段，关闭后不访问 I2C 传感器。
  if (!robot::kSensorsEnabled) {
    Serial.println("Sensors disabled for display test.");
    return;
  }
#if __has_include(<SparkFun_VL53L1X.h>)
  // VL53L1X 使用已初始化的 Wire 总线；这里暂时只启动测距，不把数据写入状态机。
  // ToF 传感器与其他 I2C 外设共用 Wire 总线。
  g_tofSensor.begin(Wire);
  g_tofSensor.startRanging();
#endif
}

void updateRuntime(uint32_t nowMs) {
  // 硬件层先收集一份普通数据快照，再交给状态机统一决定模式。
  // 这样 RobotRuntime 不需要依赖 ADC、I2C 或具体传感器驱动。
  // 目前先接入电池数据；IMU、舵机和障碍物数据后续填充到同一快照。
  robot::SensorSnapshot sensors;
  sensors.batteryVoltage =
      analogReadMilliVolts(robot::kBatteryVoltagePin) / 1000.0f;
  g_runtime.update(nowMs, sensors);

  if (g_runtime.modeChanged()) {
    Serial.printf("Mode: %s\n", robot::robotModeName(g_runtime.mode()));
  }
}

void printStatus() {
  // 周期性输出最小诊断信息，避免影响主循环时序。
  const float batteryVoltage = analogReadMilliVolts(robot::kBatteryVoltagePin) / 1000.0f;
  Serial.printf("Battery: %.2f V\n", batteryVoltage);

#if __has_include(<SparkFun_VL53L1X.h>)
  // 传感器未启用时直接返回，避免在调试模式下读取尚未初始化的设备。
  if (!robot::kSensorsEnabled) {
    return;
  }
  if (g_tofSensor.checkForDataReady()) {
    const uint16_t distanceMm = g_tofSensor.getDistance();
    Serial.printf("Distance: %u mm\n", distanceMm);
    g_tofSensor.clearInterrupt();
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
  robot::hal::initI2cBus();
  robot::hal::printI2cDeviceDiagnostics();
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
