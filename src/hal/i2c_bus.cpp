#include "i2c_bus.h"

#include <Wire.h>

#include "../robot_config.h"

namespace robot::hal {

namespace {

void probeDevice(uint8_t address, const char* name) {
  Wire.beginTransmission(address);
  const uint8_t result = Wire.endTransmission();
  Serial.printf("I2C %-8s 0x%02X: %s (code=%u)\n", name, address,
                result == 0 ? "found" : "not responding", result);
}

}  // namespace

void initI2cBus() {
  Wire.begin(robot::kI2cSdaPin, robot::kI2cSclPin, robot::kI2cClockSpeed);
  Wire.setTimeOut(50);
}

void printI2cDeviceDiagnostics() {
  Serial.printf("I2C bus: SDA=GPIO%u, SCL=GPIO%u\n", robot::kI2cSdaPin,
                robot::kI2cSclPin);
  probeDevice(robot::kTofAddress, "ToF");
  probeDevice(robot::kImuAddress, "BMI323");
}

}  // namespace robot::hal
