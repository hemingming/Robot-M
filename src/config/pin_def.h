#pragma once

#include <Arduino.h>

namespace robot::pins {

// 电源与电池采样。电池电压必须经过硬件分压后再接 ADC。
constexpr uint8_t kBatteryVoltage = 4;
constexpr uint8_t kPowerEnable = 5;

// I2C 总线：GPIO38 为 SDA，GPIO39 为 SCL，供传感器和触摸控制器共用。
constexpr uint8_t kI2cSda = 38;
constexpr uint8_t kI2cScl = 39;
constexpr uint32_t kI2cClockSpeed = 400000;

// ST7796S SPI 显示屏控制线；SD 卡片选暂未接入，因此不在这里定义。
constexpr uint8_t kDisplayMosi = 11;
constexpr uint8_t kDisplayMiso = 13;
constexpr uint8_t kDisplaySclk = 12;
constexpr uint8_t kDisplayCs = 10;
constexpr uint8_t kDisplayDc = 9;
constexpr uint8_t kDisplayRst = 8;
// 电容触摸控制器中断和复位线，当前仅保留硬件定义。
constexpr uint8_t kTouchInt = 7;
constexpr uint8_t kTouchRst = 6;

// INMP441 麦克风和 MAX98357A 功放共用 BCLK/WS 时钟，分别使用独立数据线。
constexpr uint8_t kAudioBclk = 17;
constexpr uint8_t kAudioLrclk = 18;
constexpr uint8_t kMicrophoneData = 16;
constexpr uint8_t kSpeakerData = 15;

constexpr uint8_t kTcrtLeftDigital = 14;
constexpr uint8_t kTcrtRightDigital = 21;

}  // namespace robot::pins
