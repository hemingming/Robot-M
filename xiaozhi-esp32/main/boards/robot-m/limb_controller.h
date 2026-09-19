#ifndef _ROBOT_M_LIMB_CONTROLLER_H_
#define _ROBOT_M_LIMB_CONTROLLER_H_

#include <esp_timer.h>
#include <mutex>

#include "scs_servo_bus.h"

namespace robot_m {

// 语音指令驱动的四肢步态控制器，移植自 PlatformIO 固件的 motion/limb_controller，
// 使用 esp_timer 周期性切换步态相位，替代 Arduino loop() 里的节流判断。
class LimbController {
public:
    enum class Action { kNone, kForward, kTurnLeft, kTurnRight };

    explicit LimbController(ScsServoBus& servo_bus);
    ~LimbController();

    void Stand();
    void Stop();
    void StartForward();
    void StartTurnLeft();
    void StartTurnRight();

private:
    void ApplyPhase();
    static void TimerCallback(void* arg);

    ScsServoBus& servo_bus_;
    std::mutex mutex_;
    esp_timer_handle_t timer_ = nullptr;
    Action action_ = Action::kNone;
    bool phase_ = false;
};

}  // namespace robot_m

#endif  // _ROBOT_M_LIMB_CONTROLLER_H_
