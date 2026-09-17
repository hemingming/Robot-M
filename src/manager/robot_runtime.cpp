#include "robot_runtime.h"

#include <cmath>

namespace robot {

namespace {

constexpr float kMinimumBatteryVoltage = 6.6f;
constexpr float kRecoveryTiltDegrees = 18.0f;

// 步行状态下只要横滚或俯仰超过阈值，就认为姿态不安全并进入 Recovery。
bool isTilted(const SensorSnapshot& sensors) {
  return fabsf(sensors.rollDegrees) > kRecoveryTiltDegrees ||
         fabsf(sensors.pitchDegrees) > kRecoveryTiltDegrees;
}

}  // namespace

void RobotRuntime::begin(uint32_t nowMs) {
  // 启动时记录时间基准，并从 Boot 进入受控的待机状态。
  lastUpdateMs_ = nowMs;
  setMode(RobotMode::Standby);
}

void RobotRuntime::update(uint32_t nowMs, const SensorSnapshot& sensors) {
  // 每次更新都先保存最新快照；后续所有安全判断只读取这份快照。
  lastUpdateMs_ = nowMs;
  sensors_ = sensors;

  // 低电压优先级最高，进入 EmergencyStop 后不会被普通模式请求覆盖。
  if (sensors_.batteryVoltage > 0.0f &&
      sensors_.batteryVoltage < kMinimumBatteryVoltage) {
    setMode(RobotMode::EmergencyStop);
    return;
  }

  if (mode_ == RobotMode::EmergencyStop) {
    return;
  }

  // 行走时 IMU 不健康或机身倾斜，立即切换 Recovery，等待姿态恢复。
  if (mode_ == RobotMode::Walking &&
      (!sensors_.imuHealthy || isTilted(sensors_))) {
    setMode(RobotMode::Recovery);
    return;
  }

  if (mode_ == RobotMode::Recovery) {
    if (sensors_.imuHealthy && !isTilted(sensors_)) {
      setMode(RobotMode::Standby);
    }
    return;
  }

  setMode(requestedMode_);
}

void RobotRuntime::requestMode(RobotMode requestedMode) {
  // 外部只能请求普通模式，EmergencyStop 必须由安全条件触发，不能被业务代码伪造覆盖。
  if (requestedMode != RobotMode::EmergencyStop) {
    requestedMode_ = requestedMode;
  }
}

RobotMode RobotRuntime::mode() const { return mode_; }

const SensorSnapshot& RobotRuntime::sensors() const { return sensors_; }

bool RobotRuntime::modeChanged() {
  // 这是一次性事件读取：调用者读到变化后，下一次调用会返回 false。
  const bool changed = modeChanged_;
  modeChanged_ = false;
  return changed;
}

void RobotRuntime::setMode(RobotMode nextMode) {
  if (mode_ == nextMode) {
    return;
  }
  mode_ = nextMode;
  modeChanged_ = true;
}

const char* robotModeName(RobotMode mode) {
  switch (mode) {
    case RobotMode::Boot:
      return "BOOT";
    case RobotMode::Standby:
      return "STANDBY";
    case RobotMode::Listening:
      return "LISTENING";
    case RobotMode::Walking:
      return "WALKING";
    case RobotMode::Recovery:
      return "RECOVERY";
    case RobotMode::EmergencyStop:
      return "EMERGENCY_STOP";
  }
  return "UNKNOWN";
}

}  // namespace robot
