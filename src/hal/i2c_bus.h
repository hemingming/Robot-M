#pragma once

#include <Arduino.h>

namespace robot::hal {

void initI2cBus();
void printI2cDeviceDiagnostics();

}  // namespace robot::hal
