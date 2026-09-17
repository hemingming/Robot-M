#include "robot_runtime.h"

namespace robot {

namespace {

constexpr float kMinimumBatteryVoltage = 6.6f;
constexpr float kRecoveryTiltDegrees = 18.0f;

bool isTilted(const SensorSnapshot& sensors) {
  return fabsf(sensors.rollDegrees) > kRecoveryTiltDegrees ||
         fabsf(sensors.pitchDegrees) > kRecoveryTiltDegrees;
}

}  // namespace

void RobotRuntime::begin(uint32_t nowMs) {
  lastUpdateMs_ = nowMs;
  setMode(RobotMode::Standby);
}

void RobotRuntime::update(uint32_t nowMs, const SensorSnapshot& sensors) {
  lastUpdateMs_ = nowMs;
  sensors_ = sensors;

  if (sensors_.batteryVoltage > 0.0f &&
      sensors_.batteryVoltage < kMinimumBatteryVoltage) {
    setMode(RobotMode::EmergencyStop);
    return;
  }

  if (mode_ == RobotMode::EmergencyStop) {
    return;
  }

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
  if (requestedMode != RobotMode::EmergencyStop) {
    requestedMode_ = requestedMode;
  }
}

RobotMode RobotRuntime::mode() const {
  return mode_;
}

const SensorSnapshot& RobotRuntime::sensors() const {
  return sensors_;
}

bool RobotRuntime::modeChanged() {
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