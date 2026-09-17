#include "display_st7796.h"

#include <SPI.h>

#include "../robot_config.h"

namespace robot::drivers {

namespace {

bool displayReady = false;

// 发送 ST7796S 命令：DC 为低表示后续 SPI 字节是寄存器命令。
void writeCommand(uint8_t command) {
  digitalWrite(robot::kDisplayDcPin, LOW);
  digitalWrite(robot::kDisplayCsPin, LOW);
  SPI.transfer(command);
  digitalWrite(robot::kDisplayCsPin, HIGH);
}

// 发送 ST7796S 参数数据：DC 为高表示后续 SPI 字节属于当前命令的数据。
void writeData(const uint8_t* data, size_t length) {
  digitalWrite(robot::kDisplayDcPin, HIGH);
  digitalWrite(robot::kDisplayCsPin, LOW);
  SPI.transferBytes(data, nullptr, length);
  digitalWrite(robot::kDisplayCsPin, HIGH);
}

// 设置一个包含端点的 RGB565 矩形。
// ST7796S 的列/行窗口命令使用 16 位坐标，像素数据随后通过 RAMWR 连续写入。
void fillRect(uint16_t xStart, uint16_t yStart, uint16_t xEnd,
              uint16_t yEnd, uint16_t color) {
  const uint8_t column[] = {static_cast<uint8_t>(xStart >> 8),
                            static_cast<uint8_t>(xStart),
                            static_cast<uint8_t>(xEnd >> 8),
                            static_cast<uint8_t>(xEnd)};
  const uint8_t row[] = {static_cast<uint8_t>(yStart >> 8),
                         static_cast<uint8_t>(yStart),
                         static_cast<uint8_t>(yEnd >> 8),
                         static_cast<uint8_t>(yEnd)};
  const uint8_t colorBytes[] = {static_cast<uint8_t>(color >> 8),
                                static_cast<uint8_t>(color)};
  writeCommand(0x2A);
  writeData(column, sizeof(column));
  writeCommand(0x2B);
  writeData(row, sizeof(row));
  writeCommand(0x2C);
  digitalWrite(robot::kDisplayDcPin, HIGH);
  digitalWrite(robot::kDisplayCsPin, LOW);
  const uint32_t pixelCount = static_cast<uint32_t>(xEnd - xStart + 1) *
                              (yEnd - yStart + 1);
  for (uint32_t pixel = 0; pixel < pixelCount; ++pixel) {
    SPI.transferBytes(colorBytes, nullptr, sizeof(colorBytes));
  }
  digitalWrite(robot::kDisplayCsPin, HIGH);
}

}  // namespace

void initDisplay() {
  if (!robot::kDisplayEnabled) {
    Serial.println("Display disabled.");
    return;
  }
  Serial.println("Initializing TFT display...");

  pinMode(robot::kDisplayCsPin, OUTPUT);
  pinMode(robot::kDisplayDcPin, OUTPUT);
  pinMode(robot::kDisplayRstPin, OUTPUT);
  digitalWrite(robot::kDisplayCsPin, HIGH);
  digitalWrite(robot::kDisplayDcPin, HIGH);
  digitalWrite(robot::kDisplayRstPin, HIGH);
  // 使用硬件 SPI，并显式指定引脚，避免依赖开发板默认 SPI 映射。
  SPI.begin(robot::kDisplaySclkPin, robot::kDisplayMisoPin,
            robot::kDisplayMosiPin, robot::kDisplayCsPin);

  // ST7796S 硬复位后需要等待内部电源和寄存器恢复稳定。
  digitalWrite(robot::kDisplayRstPin, LOW);
  delay(20);
  digitalWrite(robot::kDisplayRstPin, HIGH);
  delay(150);

  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  writeCommand(0x01);
  delay(120);
  writeCommand(0x11);
  delay(120);
  const uint8_t pixelFormat = 0x55;
  writeCommand(0x3A);
  writeData(&pixelFormat, 1);
  // 0x08 配合 320x480 窗口使用竖屏方向；颜色反转由小智工程单独配置。
  const uint8_t memoryAccess = 0x08;
  writeCommand(0x36);
  writeData(&memoryAccess, 1);
  writeCommand(0x21);
  writeCommand(0x29);
  SPI.endTransaction();

  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  // 三段纯色是最小硬件自检画面，可快速判断坐标方向和整屏刷新是否正常。
  fillRect(0, 0, 319, 159, 0xF800);
  fillRect(0, 160, 319, 319, 0x07E0);
  fillRect(0, 320, 319, 479, 0x001F);
  SPI.endTransaction();
  displayReady = true;
  Serial.println("TFT display initialized.");
}

void updateMicrophoneDisplay(uint32_t level) {
  if (!displayReady) {
    return;
  }
  // 将原始电平压缩到 280 像素宽，避免过大采样值造成坐标越界。
  const uint32_t barWidth = min<uint32_t>(level / 500, 280);
  SPI.beginTransaction(SPISettings(20000000, MSBFIRST, SPI_MODE0));
  fillRect(20, 445, 299, 469, 0x0000);
  if (barWidth > 0) {
    fillRect(20, 445, 19 + barWidth, 469, 0x07E0);
  }
  SPI.endTransaction();
}

}  // namespace robot::drivers
