#pragma once

#include <Arduino.h>

namespace robot {

enum class RobotMode : uint8_t {
  Boot,
  Standby,
  Listening,
  Walking,
  Recovery,
  EmergencyStop,
};

struct SensorSnapshot {
  float batteryVoltage = 0.0f;
  float rollDegrees = 0.0f;
  float pitchDegrees = 0.0f;
  float yawRateDegreesPerSecond = 0.0f;
  bool imuHealthy = false;
  bool servoBusHealthy = false;
  bool obstacleDetected = false;
};

class RobotRuntime {
 public:
  void begin(uint32_t nowMs);
  void update(uint32_t nowMs, const SensorSnapshot& sensors);
  void requestMode(RobotMode requestedMode);
  RobotMode mode() const;
  const SensorSnapshot& sensors() const;
  bool modeChanged();

 private:
  void setMode(RobotMode nextMode);

  RobotMode mode_ = RobotMode::Boot;
  RobotMode requestedMode_ = RobotMode::Standby;
  SensorSnapshot sensors_{};
  uint32_t lastUpdateMs_ = 0;
  bool modeChanged_ = false;
};

const char* robotModeName(RobotMode mode);

}  // namespace robot
