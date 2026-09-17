#include "limb_controller.h"

namespace robot::motion {

LimbController::LimbController(actuators::ServoBus& servoBus)
    : servoBus_(servoBus) {}

void LimbController::begin() {}

void LimbController::stand() {
  // 等确认舵机零位、关节方向和 URT-2 协议后实现站立姿态。
}

void LimbController::stop() { servoBus_.stopAll(); }

}  // namespace robot::motion
