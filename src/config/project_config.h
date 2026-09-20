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

// I2C 总线总开关。BMI323（0x68/0x69）和 VL53L1X（0x29）未接入时设为 false，
// 跳过 I2C 初始化、设备探测和周期读数，避免 Wire 库持续打印 Error 263 刷屏。
// 关闭后 imuHealthy 视为 true，防止状态机在 Walking 时误触发 Recovery。
// 接好 IMU/ToF 后改回 true。
constexpr bool kI2cEnabled = false;
constexpr uint32_t kWifiConnectTimeoutMs = 15000;

// 舵机 ID 分配（2026-09-20 实物确认）：1-3 左腿，4-6 右腿，
// 7-8 左臂，9-10 右臂，11 屏幕左右摇头，12 屏幕上下点头。
constexpr uint8_t kServoIdLeftLegFirst = 1;
constexpr uint8_t kServoIdLeftLegLast = 3;
constexpr uint8_t kServoIdRightLegFirst = 4;
constexpr uint8_t kServoIdRightLegLast = 6;
constexpr uint8_t kServoIdLeftArmFirst = 7;
constexpr uint8_t kServoIdLeftArmLast = 8;
constexpr uint8_t kServoIdRightArmFirst = 9;
constexpr uint8_t kServoIdRightArmLast = 10;
constexpr uint8_t kServoIdHeadYaw = 11;
constexpr uint8_t kServoIdHeadPitch = 12;

}  // namespace robot::config
