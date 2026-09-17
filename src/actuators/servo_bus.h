#pragma once

#include <Arduino.h>

namespace robot::actuators {

struct ServoTarget {
  uint8_t id = 0;
  int16_t position = 0;
  uint16_t speed = 0;
  uint16_t acceleration = 0;
};

class ServoBus {
 public:
  void begin();
  void stopAll();
  bool sendTarget(const ServoTarget& target);
  bool healthy() const;

 private:
  bool healthy_ = false;
};

}  // namespace robot::actuators
