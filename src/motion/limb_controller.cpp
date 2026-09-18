#include "limb_controller.h"

namespace robot::motion {

LimbController::LimbController(actuators::ServoBus& servoBus)
    : servoBus_(servoBus) {}

void LimbController::begin() {}

void LimbController::stand() {
  // 这是未完成机械校准前的中位基准姿态：腿部六个关节先回到约 512。
  // 左右镜像、舵机安装偏移和实际站立角度确认后，只需调整这组位置值。
  action_ = Action::None;
  constexpr uint8_t kLegServoIds[] = {1, 2, 3, 4, 5, 6};
  constexpr int16_t kStandPositions[] = {512, 512, 512, 512, 512, 512};

  for (size_t index = 0; index < 6; ++index) {
    actuators::ServoTarget target;
    target.id = kLegServoIds[index];
    target.position = kStandPositions[index];
    target.speed = 100;
    target.acceleration = 2;
    servoBus_.sendTarget(target);
  }
}

void LimbController::stop() {
  action_ = Action::None;
  servoBus_.stopAll();
}

void LimbController::startForward() {
  action_ = Action::Forward;
  phase_ = false;
  applyPhase();
}

void LimbController::startTurnLeft() {
  action_ = Action::TurnLeft;
  phase_ = false;
  applyPhase();
}

void LimbController::startTurnRight() {
  action_ = Action::TurnRight;
  phase_ = false;
  applyPhase();
}

void LimbController::update(uint32_t nowMs) {
  if (action_ == Action::None || nowMs - lastStepMs_ < 350) {
    return;
  }
  lastStepMs_ = nowMs;
  phase_ = !phase_;
  applyPhase();
}

void LimbController::applyPhase() {
  constexpr uint8_t kLegServoIds[] = {1, 2, 3, 4, 5, 6};
  constexpr int16_t kCenter = 512;
  constexpr int16_t kStride = 45;
  const int16_t forwardOffset = phase_ ? kStride : -kStride;

  for (size_t index = 0; index < 6; ++index) {
    int16_t offset = forwardOffset;
    if (action_ == Action::TurnLeft) {
      offset = index < 3 ? -forwardOffset : forwardOffset;
    } else if (action_ == Action::TurnRight) {
      offset = index < 3 ? forwardOffset : -forwardOffset;
    }

    actuators::ServoTarget target;
    target.id = kLegServoIds[index];
    target.position = kCenter + offset;
    target.speed = 60;
    target.acceleration = 2;
    servoBus_.sendTarget(target);
  }
}

}  // namespace robot::motion
