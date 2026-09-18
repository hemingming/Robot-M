#pragma once

#include <Arduino.h>

namespace robot::hal {

struct Bmi323RawData {
	uint16_t chipId = 0;
	int16_t accelX = 0;
	int16_t accelY = 0;
	int16_t accelZ = 0;
	int16_t gyroX = 0;
	int16_t gyroY = 0;
	int16_t gyroZ = 0;
};

void initI2cBus();
void printI2cDeviceDiagnostics();
bool readBmi323Raw(Bmi323RawData& data);
uint8_t bmi323Address();

}  // namespace robot::hal
