#pragma once

#include <Arduino.h>

#include "../actuators/servo_bus.h"

namespace robot::motion {

enum class Limb : uint8_t {
  FrontLeft,
  FrontRight,
  RearLeft,
  RearRight,
};

class LimbController {
 public:
  enum class Action : uint8_t { None, Forward, TurnLeft, TurnRight };

  explicit LimbController(actuators::ServoBus& servoBus);

  void begin();
  void stand();
  void stop();
  void startForward();
  void startTurnLeft();
  void startTurnRight();
  void update(uint32_t nowMs);

 private:
  void applyPhase();

  actuators::ServoBus& servoBus_;
  Action action_ = Action::None;
  uint32_t lastStepMs_ = 0;
  bool phase_ = false;
};

}  // namespace robot::motion
