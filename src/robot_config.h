#pragma once

#include "config/pin_def.h"
#include "config/project_config.h"

namespace robot {

// 这一层把 pins/config 中的实现细节导出为运行时代码使用的统一配置接口。
// 驱动只依赖 kDisplayCsPin 等语义化名称，不直接散落 GPIO 数字。
using namespace pins;
using namespace config;

constexpr uint8_t kBatteryVoltagePin = pins::kBatteryVoltage;
constexpr uint8_t kPowerEnablePin = pins::kPowerEnable;
constexpr uint8_t kI2cSdaPin = pins::kI2cSda;
constexpr uint8_t kI2cSclPin = pins::kI2cScl;
constexpr uint32_t kI2cClockSpeed = pins::kI2cClockSpeed;
constexpr uint8_t kDisplayMosiPin = pins::kDisplayMosi;
constexpr uint8_t kDisplayMisoPin = pins::kDisplayMiso;
constexpr uint8_t kDisplaySclkPin = pins::kDisplaySclk;
constexpr uint8_t kDisplayCsPin = pins::kDisplayCs;
constexpr uint8_t kDisplayDcPin = pins::kDisplayDc;
constexpr uint8_t kDisplayRstPin = pins::kDisplayRst;
constexpr uint8_t kTouchScreenIntPin = pins::kTouchInt;
constexpr uint8_t kTouchScreenRstPin = pins::kTouchRst;
constexpr uint8_t kAudioBclkPin = pins::kAudioBclk;
constexpr uint8_t kAudioLrclkPin = pins::kAudioLrclk;
constexpr uint8_t kMicrophoneDataPin = pins::kMicrophoneData;
constexpr uint8_t kSpeakerDataPin = pins::kSpeakerData;
constexpr uint8_t kTcrtLeftDigitalPin = pins::kTcrtLeftDigital;
constexpr uint8_t kTcrtRightDigitalPin = pins::kTcrtRightDigital;

// I2C 传感器地址集中维护，后续接入驱动时不要在业务代码中重复写数字。
constexpr uint8_t kImuAddress = 0x68;
constexpr uint8_t kTofAddress = 0x29;

}  // namespace robot
