#include "servo_bus.h"

namespace robot::actuators {

void ServoBus::begin() {
  // URT-2 串口和波特率待根据实物接线确认后接入。
  healthy_ = false;
  Serial.println("Servo bus not configured.");
}

void ServoBus::stopAll() {
  // 预留总线舵机急停入口。
}

bool ServoBus::sendTarget(const ServoTarget& target) {
  (void)target;
  return false;
}

bool ServoBus::healthy() const { return healthy_; }

}  // namespace robot::actuators
