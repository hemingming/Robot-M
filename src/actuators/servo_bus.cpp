#include "servo_bus.h"

#include "../robot_config.h"

namespace robot::actuators {

void ServoBus::begin() {
  // 只启动 UART，不发送动作，避免上电时让未知 ID 的舵机突然运动。
  serial_.begin(robot::kServoUartBaud, SERIAL_8N1, robot::kServoUartRxPin,
                robot::kServoUartTxPin);
  servo_.pSerial = &serial_;
  servo_.IOTimeOut = 100;
  healthy_ = true;
  Serial.printf("Servo bus ready: TX=GPIO%u, RX=GPIO%u, baud=%lu\n",
                robot::kServoUartTxPin, robot::kServoUartRxPin,
                static_cast<unsigned long>(robot::kServoUartBaud));
}

void ServoBus::stopAll() {
  if (!healthy_) {
    return;
  }
  // SCS 广播 ID 为 0xFE；关闭扭力不会改变位置，只停止保持力矩。
  servo_.EnableTorque(0xFE, 0);
}

bool ServoBus::sendTarget(const ServoTarget& target) {
  if (!healthy_ || target.id == 0 || target.id == 0xFE) {
    return false;
  }
  // SCS0017 属于 SCS 系列，位置范围为 0..1023；拒绝越界值，避免撞限位。
  if (target.position < 0 || target.position > 1023) {
    return false;
  }
  servo_.EnableTorque(target.id, 1);
  return servo_.WritePosEx(target.id, target.position, target.speed,
                           static_cast<uint8_t>(min<uint16_t>(target.acceleration, 255))) > 0;
}

bool ServoBus::ping(uint8_t id) {
  return healthy_ && id != 0 && id != 0xFE && servo_.Ping(id) > 0;
}

bool ServoBus::setId(uint8_t currentId, uint8_t newId) {
  if (!healthy_ || currentId == 0 || currentId == 0xFE || newId == 0 ||
      newId == 0xFE || currentId == newId) {
    return false;
  }
  servo_.EnableTorque(currentId, 0);
  if (servo_.unLockEprom(currentId) <= 0) {
    return false;
  }
  const bool written = servo_.writeByte(currentId, SCSCL_ID, newId) > 0;
  servo_.LockEprom(currentId);
  return written;
}

bool ServoBus::healthy() const { return healthy_; }

}  // namespace robot::actuators
