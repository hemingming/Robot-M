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
  explicit LimbController(actuators::ServoBus& servoBus);

  void begin();
  void stand();
  void stop();

 private:
  actuators::ServoBus& servoBus_;
};

}  // namespace robot::motion
