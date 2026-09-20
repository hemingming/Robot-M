#pragma once

#include <Arduino.h>

namespace robot::config {

// 主循环中状态、诊断和麦克风电平的采样周期。
constexpr uint32_t kStatusUpdateMs = 200;

// 电池电压经电阻分压后才接入 ADC：电池实际电压 = ADC 引脚电压 × 分压比，
// 其中分压比 = (R上 + R下) / R下。2.0 对应等值分压（如 R上=R下=100k），
// 仅为占位值，必须按原理图电阻或万用表实测校准后再依赖低电保护阈值。
constexpr float kBatteryAdcDividerRatio = 2.0f;

// 电池监控总开关。GPIO4 分压网络未接入时设为 false，跳过 ADC 读取与
// 低电保护，避免悬空引脚被误读为低电压导致状态机锁入 EmergencyStop。
// 接好分压网络后改回 true，并用 battery cal 命令校准 kBatteryAdcDividerRatio。
constexpr bool kBatteryMonitoringEnabled = false;
// 功能总开关：关闭后对应模块仍可编译，但启动时不会访问硬件。
constexpr bool kDisplayEnabled = true;
constexpr bool kSensorsEnabled = true;
constexpr bool kMicrophoneEnabled = true;
constexpr bool kSpeakerEnabled = false;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;

}  // namespace robot::config
