#ifndef _ROBOT_M_BOARD_CONFIG_H_
#define _ROBOT_M_BOARD_CONFIG_H_

#include <driver/gpio.h>
#include <driver/spi_master.h>
#include <driver/uart.h>

// 音频硬件：麦克风和功放使用独立的 I2S 时钟，避免两个控制器争用同一组 BCLK/WS。
#define AUDIO_INPUT_SAMPLE_RATE 16000
#define AUDIO_OUTPUT_SAMPLE_RATE 24000
#define AUDIO_I2S_SPK_GPIO_BCLK GPIO_NUM_17
#define AUDIO_I2S_SPK_GPIO_LRCK GPIO_NUM_18
#define AUDIO_I2S_SPK_GPIO_DOUT GPIO_NUM_15
#define AUDIO_I2S_MIC_GPIO_SCK GPIO_NUM_1
#define AUDIO_I2S_MIC_GPIO_WS GPIO_NUM_2
#define AUDIO_I2S_MIC_GPIO_DIN GPIO_NUM_16

// ESP32-S3 标准 BOOT 键；音量键未接入，因此使用 GPIO_NC。
#define BOOT_BUTTON_GPIO GPIO_NUM_0
#define VOLUME_UP_BUTTON_GPIO GPIO_NUM_NC
#define VOLUME_DOWN_BUTTON_GPIO GPIO_NUM_NC

// 板载 RGB 状态灯。2026-09-20 用探针固件实测确认接在 GPIO38（官方 v1.1 接法），
// 此前"灯不亮"是因为小智待机状态本就熄灯且默认亮度仅 4~16/255。
// 注意：GPIO38 同时在下方触摸预留里定义为 I2C_SDA_PIN，且当前外接 HUB 的 SDA 也接在此引脚；
// I2C 尚未启用所以暂不冲突，未来启用 I2C（触摸/传感器）前必须把 HUB 挪到其他引脚并同步改配置。
#define BUILTIN_LED_GPIO GPIO_NUM_38
// 本板 RGB 的 R/G 通道与标准 WS2812 对调（探针实测发红显示绿），因此用 RGB 字节排列。
#define BOARD_LED_COLOR_COMPONENT_FORMAT LED_STRIP_COLOR_COMPONENT_FMT_RGB
// 默认亮度 4~16/255 肉眼难辨，调高到正常可辨水平。
#define BOARD_LED_BRIGHTNESS_DEFAULT 32
#define BOARD_LED_BRIGHTNESS_LOW 24
#define BOARD_LED_BRIGHTNESS_HIGH 96

// 4.0 英寸 ST7796S SPI 屏。物理面板为 480x320，应用坐标按竖屏 320x480 使用。
#define DISPLAY_SPI_MODE 0
#define DISPLAY_SPI_HOST SPI2_HOST
#define DISPLAY_SPI_SCLK_HZ (20 * 1000 * 1000)
#define DISPLAY_SPI_SCLK_PIN GPIO_NUM_12
#define DISPLAY_SPI_MOSI_PIN GPIO_NUM_11
#define DISPLAY_SPI_MISO_PIN GPIO_NUM_13
#define DISPLAY_CS_PIN GPIO_NUM_10
#define DISPLAY_DC_PIN GPIO_NUM_9
#define DISPLAY_RST_PIN GPIO_NUM_8
#define DISPLAY_BACKLIGHT_PIN GPIO_NUM_NC
#define DISPLAY_BACKLIGHT_OUTPUT_INVERT false
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 480
#define DISPLAY_MIRROR_X false
// Y 轴翻转用于修正面板安装方向，使界面文字上下方向正确。
#define DISPLAY_MIRROR_Y true
#define DISPLAY_SWAP_XY false
#define DISPLAY_RGB_ORDER LCD_RGB_ELEMENT_ORDER_BGR
#define DISPLAY_INVERT_COLOR false
#define DISPLAY_OFFSET_X 0
#define DISPLAY_OFFSET_Y 0

// 触摸控制器共用 I2C 总线；当前小智板卡只预留引脚，触摸逻辑尚未接入。
#define I2C_SDA_PIN GPIO_NUM_38
#define I2C_SCL_PIN GPIO_NUM_39
#define TOUCH_INT_PIN GPIO_NUM_7
#define TOUCH_RST_PIN GPIO_NUM_6

// URT-2 舵机总线：与 PlatformIO 固件的 src/config/pin_def.h 保持一致的 GPIO 和波特率。
#define SERVO_UART_PORT UART_NUM_1
#define SERVO_UART_TX_PIN GPIO_NUM_19
#define SERVO_UART_RX_PIN GPIO_NUM_20
#define SERVO_UART_BAUD_RATE 1000000

// 外设供电使能（稳压板/舵机动力），与 Arduino 固件 pin_def.h 的 kPowerEnable=5 一致。
// 不拉高时外设电源板不工作：舵机无供电、电源指示灯不亮。
#define PERIPHERAL_POWER_ENABLE_GPIO GPIO_NUM_5

#endif  // _ROBOT_M_BOARD_CONFIG_H_
