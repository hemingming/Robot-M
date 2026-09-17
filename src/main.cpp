#include <Arduino.h>
#include <Wire.h>

#include "robot_config.h"

#if __has_include(<TFT_eSPI.h>)
#include <TFT_eSPI.h>
static TFT_eSPI g_tft = TFT_eSPI();
#endif

#if __has_include(<SparkFun_VL53L1X.h>)
#include <SparkFun_VL53L1X.h>
static SFEVL53L1X g_tofSensor;
#endif

namespace {

void initI2c() {
  Wire.begin(robot::kI2cSdaPin, robot::kI2cSclPin, robot::kI2cClockSpeed);
}

void initSerial() {
  Serial.begin(115200);
  delay(100);
  Serial.println("Robot-M booting...");
}

void initPower() {
  pinMode(robot::kPowerEnablePin, OUTPUT);
  digitalWrite(robot::kPowerEnablePin, HIGH);
}

void initDisplay() {
#if __has_include(<TFT_eSPI.h>)
  g_tft.init();
  g_tft.setRotation(1);
  g_tft.fillScreen(TFT_BLACK);
  g_tft.setTextColor(TFT_WHITE, TFT_BLACK);
  g_tft.setTextSize(2);
  g_tft.drawString("Robot-M", 20, 20);
#endif
}

void initSensors() {
#if __has_include(<SparkFun_VL53L1X.h>)
  g_tofSensor.begin(Wire);
  g_tofSensor.startRanging();
#endif
}

void printStatus() {
  const float batteryVoltage = analogReadMilliVolts(robot::kBatteryVoltagePin) / 1000.0f;
  Serial.printf("Battery: %.2f V\n", batteryVoltage);

#if __has_include(<SparkFun_VL53L1X.h>)
  if (g_tofSensor.checkForDataReady()) {
    const uint16_t distanceMm = g_tofSensor.getDistance();
    Serial.printf("Distance: %u mm\n", distanceMm);
    g_tofSensor.clearInterrupt();
  }
#endif
}

}  // namespace

void setup() {
  initSerial();
  initPower();
  initI2c();
  initDisplay();
  initSensors();

  Serial.println("Robot-M initialization complete.");
}

void loop() {
  static uint32_t lastStatusMs = 0;
  const uint32_t now = millis();

  if (now - lastStatusMs >= robot::kStatusUpdateMs) {
    lastStatusMs = now;
    printStatus();
  }

  delay(10);
}
