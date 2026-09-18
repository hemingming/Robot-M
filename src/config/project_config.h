#pragma once

#include <Arduino.h>

namespace robot::config {

// 主循环中状态、诊断和麦克风电平的采样周期。
constexpr uint32_t kStatusUpdateMs = 200;
// 功能总开关：关闭后对应模块仍可编译，但启动时不会访问硬件。
constexpr bool kDisplayEnabled = true;
constexpr bool kSensorsEnabled = true;
constexpr bool kMicrophoneEnabled = true;
constexpr bool kSpeakerEnabled = false;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;

}  // namespace robot::config
