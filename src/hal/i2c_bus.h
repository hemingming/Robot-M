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
// 扫描 0x01~0x7F 全部 I2C 地址，列出所有应答的从机地址。
// 用于在 BMI323/VL53L1X 未接时定位 HUB 等任意 I2C 设备的真实地址。
void i2cScan();
bool readBmi323Raw(Bmi323RawData& data);
uint8_t bmi323Address();

}  // namespace robot::hal
