#include "limb_controller.h"

namespace robot_m {

namespace {
constexpr uint8_t kLegServoIds[6] = {1, 2, 3, 4, 5, 6};
constexpr int16_t kCenterPosition = 512;
constexpr int16_t kStridePosition = 45;
constexpr uint64_t kStepIntervalUs = 350000;  // 与 PlatformIO 固件保持一致的 350ms 步频。
}  // namespace

LimbController::LimbController(ScsServoBus& servo_bus) : servo_bus_(servo_bus) {
    esp_timer_create_args_t timer_args = {};
    timer_args.callback = &LimbController::TimerCallback;
    timer_args.arg = this;
    timer_args.name = "limb_gait";
    esp_timer_create(&timer_args, &timer_);
}

LimbController::~LimbController() {
    if (timer_ != nullptr) {
        esp_timer_stop(timer_);
        esp_timer_delete(timer_);
    }
}

void LimbController::Stand() {
    std::lock_guard<std::mutex> lock(mutex_);
    action_ = Action::kNone;
    esp_timer_stop(timer_);
    // 未完成机械校准前的中位基准姿态；确认零位后再调整这组位置值。
    for (uint8_t id : kLegServoIds) {
        servo_bus_.WritePosition(id, kCenterPosition, 100);
    }
}

void LimbController::Stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    action_ = Action::kNone;
    esp_timer_stop(timer_);
    servo_bus_.StopAll();
}

void LimbController::StartForward() {
    std::lock_guard<std::mutex> lock(mutex_);
    action_ = Action::kForward;
    phase_ = false;
    ApplyPhase();
    esp_timer_stop(timer_);
    esp_timer_start_periodic(timer_, kStepIntervalUs);
}

void LimbController::StartTurnLeft() {
    std::lock_guard<std::mutex> lock(mutex_);
    action_ = Action::kTurnLeft;
    phase_ = false;
    ApplyPhase();
    esp_timer_stop(timer_);
    esp_timer_start_periodic(timer_, kStepIntervalUs);
}

void LimbController::StartTurnRight() {
    std::lock_guard<std::mutex> lock(mutex_);
    action_ = Action::kTurnRight;
    phase_ = false;
    ApplyPhase();
    esp_timer_stop(timer_);
    esp_timer_start_periodic(timer_, kStepIntervalUs);
}

void LimbController::ApplyPhase() {
    const int16_t forward_offset = phase_ ? kStridePosition : static_cast<int16_t>(-kStridePosition);

    for (size_t index = 0; index < 6; ++index) {
        int16_t offset = forward_offset;
        if (action_ == Action::kTurnLeft) {
            offset = index < 3 ? static_cast<int16_t>(-forward_offset) : forward_offset;
        } else if (action_ == Action::kTurnRight) {
            offset = index < 3 ? forward_offset : static_cast<int16_t>(-forward_offset);
        }
        servo_bus_.WritePosition(kLegServoIds[index], static_cast<int16_t>(kCenterPosition + offset), 60);
    }
}

void LimbController::TimerCallback(void* arg) {
    auto* controller = static_cast<LimbController*>(arg);
    std::lock_guard<std::mutex> lock(controller->mutex_);
    if (controller->action_ == Action::kNone) {
        return;
    }
    controller->phase_ = !controller->phase_;
    controller->ApplyPhase();
}

}  // namespace robot_m
