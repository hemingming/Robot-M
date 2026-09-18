#pragma once

#include <Arduino.h>
#include <SCSCL.h>

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
  bool ping(uint8_t id);
  bool setId(uint8_t currentId, uint8_t newId);
  bool healthy() const;

 private:
  HardwareSerial serial_{1};
  SCSCL servo_{};
  bool healthy_ = false;
};

}  // namespace robot::actuators
