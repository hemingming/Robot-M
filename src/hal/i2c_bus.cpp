#include "i2c_bus.h"

#include <Wire.h>
#include <bmi323.h>

#include "../robot_config.h"

namespace robot::hal {

namespace {

uint8_t g_bmi323Address = robot::kImuAddress;
bmi3_dev g_bmi323Device = {};
bool g_bmi323Initialized = false;

int8_t bmi323Read(uint8_t regAddress, uint8_t* data, uint32_t length,
                  void*) {
  Wire.beginTransmission(g_bmi323Address);
  Wire.write(regAddress);
  if (Wire.endTransmission(false) != 0 ||
      Wire.requestFrom(static_cast<int>(g_bmi323Address),
                       static_cast<int>(length)) != length) {
    return -1;
  }
  for (uint32_t index = 0; index < 2; ++index) {
    data[index] = Wire.read();
  }
  for (uint32_t index = 0; index < length - 2; ++index) {
    data[index + 2] = Wire.read();
  }
  return 0;
}

int8_t bmi323Write(uint8_t regAddress, const uint8_t* data, uint32_t length,
                   void*) {
  Wire.beginTransmission(g_bmi323Address);
  Wire.write(regAddress);
  for (uint32_t index = 0; index < length; ++index) {
    Wire.write(data[index]);
  }
  return Wire.endTransmission() == 0 ? 0 : -1;
}

void bmi323DelayUs(uint32_t period, void*) {
  delayMicroseconds(period);
}

void probeDevice(uint8_t address, const char* name) {
  Wire.beginTransmission(address);
  const uint8_t result = Wire.endTransmission();
  Serial.printf("I2C %-8s 0x%02X: %s (code=%u)\n", name, address,
                result == 0 ? "found" : "not responding", result);
}

bool readRegisters(uint8_t address, uint8_t startRegister, uint8_t* buffer,
                   size_t length) {
  Wire.beginTransmission(address);
  Wire.write(startRegister);
  if (Wire.endTransmission(true) != 0) {
    return false;
  }
  if (Wire.requestFrom(static_cast<int>(address), static_cast<int>(length)) !=
      length) {
    return false;
  }
  for (size_t index = 0; index < length; ++index) {
    buffer[index] = Wire.read();
  }
  return true;
}

int16_t decodeWord(const uint8_t* bytes) {
  return static_cast<int16_t>(static_cast<uint16_t>(bytes[0]) |
                              (static_cast<uint16_t>(bytes[1]) << 8));
}

}  // namespace

void initI2cBus() {
  Wire.begin(robot::kI2cSdaPin, robot::kI2cSclPin, 100000);
  Wire.setTimeOut(50);
}

void printI2cDeviceDiagnostics() {
  Serial.printf("I2C bus: SDA=GPIO%u, SCL=GPIO%u\n", robot::kI2cSdaPin,
                robot::kI2cSclPin);
  probeDevice(robot::kTofAddress, "ToF");
  probeDevice(0x68, "BMI323");
  probeDevice(0x69, "BMI323");
  uint8_t chipId[2] = {};
  if (readRegisters(0x68, 0x00, chipId, sizeof(chipId)) &&
      (chipId[0] != 0 || chipId[1] != 0)) {
    g_bmi323Address = 0x68;
  } else if (readRegisters(0x69, 0x00, chipId, sizeof(chipId)) &&
             (chipId[0] != 0 || chipId[1] != 0)) {
    g_bmi323Address = 0x69;
  }
  Serial.printf("BMI323 selected address: 0x%02X\n", g_bmi323Address);

  g_bmi323Device.intf = BMI3_I2C_INTF;
  g_bmi323Device.intf_ptr = nullptr;
  g_bmi323Device.read = bmi323Read;
  g_bmi323Device.write = bmi323Write;
  g_bmi323Device.delay_us = bmi323DelayUs;
  const int8_t initResult = bmi323_init(&g_bmi323Device);
  if (initResult != BMI323_OK) {
    Serial.printf("BMI323 official API init failed: %d, chip=0x%02X\n",
                  initResult, g_bmi323Device.chip_id);
    return;
  }

  bmi3_sens_config sensorConfig[2] = {};
  sensorConfig[0].type = BMI323_ACCEL;
  sensorConfig[0].cfg.acc.odr = BMI3_ACC_ODR_100HZ;
  sensorConfig[0].cfg.acc.bwp = BMI3_ACC_BW_ODR_HALF;
  sensorConfig[0].cfg.acc.acc_mode = BMI3_ACC_MODE_NORMAL;
  sensorConfig[0].cfg.acc.range = BMI3_ACC_RANGE_4G;
  sensorConfig[0].cfg.acc.avg_num = BMI3_ACC_AVG2;
  sensorConfig[1].type = BMI323_GYRO;
  sensorConfig[1].cfg.gyr.odr = BMI3_GYR_ODR_100HZ;
  sensorConfig[1].cfg.gyr.bwp = BMI3_GYR_BW_ODR_HALF;
  sensorConfig[1].cfg.gyr.gyr_mode = BMI3_GYR_MODE_NORMAL;
  sensorConfig[1].cfg.gyr.range = BMI3_GYR_RANGE_500DPS;
  sensorConfig[1].cfg.gyr.avg_num = BMI3_GYR_AVG2;
  const int8_t configResult =
      bmi323_set_sensor_config(sensorConfig, 2, &g_bmi323Device);
  if (configResult != BMI323_OK) {
    Serial.printf("BMI323 official API config failed: %d\n", configResult);
    return;
  }
  g_bmi323Initialized = true;
  Serial.printf("BMI323 official API ready: chip=0x%02X\n",
                g_bmi323Device.chip_id);
}

bool readBmi323Raw(Bmi323RawData& data) {
  if (!g_bmi323Initialized) {
    return false;
  }

  delay(12);
  bmi3_sensor_data sensorData[2] = {};
  sensorData[0].type = BMI323_ACCEL;
  sensorData[1].type = BMI323_GYRO;
  if (bmi323_get_sensor_data(sensorData, 2, &g_bmi323Device) != BMI323_OK) {
    return false;
  }

  data.chipId = g_bmi323Device.chip_id;
  data.accelX = sensorData[0].sens_data.acc.x;
  data.accelY = sensorData[0].sens_data.acc.y;
  data.accelZ = sensorData[0].sens_data.acc.z;
  data.gyroX = sensorData[1].sens_data.gyr.x;
  data.gyroY = sensorData[1].sens_data.gyr.y;
  data.gyroZ = sensorData[1].sens_data.gyr.z;
  return true;
}

uint8_t bmi323Address() { return g_bmi323Address; }

}  // namespace robot::hal
