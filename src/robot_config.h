#pragma once

#include <Arduino.h>

namespace robot {

// Power and basic IO
constexpr uint8_t kBatteryVoltagePin = 4;
constexpr uint8_t kPowerEnablePin = 5;

// I2C bus
constexpr uint8_t kI2cSdaPin = 38;
constexpr uint8_t kI2cSclPin = 39;
constexpr uint32_t kI2cClockSpeed = 400000;

// User-defined sensor / peripheral addresses
constexpr uint8_t kImuAddress = 0x68;
constexpr uint8_t kTofAddress = 0x29;
constexpr uint8_t kTouchScreenCsPin = 34;
constexpr uint8_t kTouchScreenDcPin = 33;
constexpr uint8_t kTouchScreenRstPin = 21;

// System behavior
constexpr uint32_t kStatusUpdateMs = 200;

}  // namespace robot
