#include "wifi_board.h"
#include "codecs/no_audio_codec.h"
#include "display/lcd_display.h"
#include "application.h"
#include "button.h"
#include "config.h"
#include "mcp_server.h"
#include "limb_controller.h"
#include "scs_servo_bus.h"

#include <sstream>

#include <esp_log.h>
#include <esp_lcd_panel_io.h>
#include <esp_lcd_panel_ops.h>
#include <esp_lcd_panel_vendor.h>
#include <esp_lcd_st7796.h>
#include <driver/spi_common.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#define TAG "ROBOT_M"

// Robot-M 是小智工程的板级适配层：负责把通用语音应用连接到本板的屏幕、按键和音频引脚。
class RobotMBoard : public WifiBoard {
private:
    Button boot_button_;
    LcdDisplay* display_ = nullptr;
    robot_m::ScsServoBus servo_bus_;
    robot_m::LimbController limb_controller_{servo_bus_};
    enum class ServoPingState { kIdle, kRunning, kResponded, kNoResponse };
    ServoPingState servo_ping_state_ = ServoPingState::kIdle;
    uint8_t servo_ping_id_ = 0;

    void InitializeSpi() {
        // 初始化屏幕专用 SPI 总线；最大传输长度按整屏 RGB565 数据预留。
        spi_bus_config_t bus_config = {};
        bus_config.mosi_io_num = DISPLAY_SPI_MOSI_PIN;
        bus_config.miso_io_num = DISPLAY_SPI_MISO_PIN;
        bus_config.sclk_io_num = DISPLAY_SPI_SCLK_PIN;
        bus_config.quadwp_io_num = GPIO_NUM_NC;
        bus_config.quadhd_io_num = GPIO_NUM_NC;
        bus_config.max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * sizeof(uint16_t);
        ESP_ERROR_CHECK(spi_bus_initialize(DISPLAY_SPI_HOST, &bus_config, SPI_DMA_CH_AUTO));
    }

    void InitializeDisplay() {
        // esp_lcd 负责命令/数据时序，SpiLcdDisplay 再把面板包装成小智 UI 使用的 Display。
        esp_lcd_panel_io_handle_t panel_io = nullptr;
        esp_lcd_panel_handle_t panel = nullptr;

        // 面板 IO 配置描述 CS/DC、SPI 模式、时钟频率以及命令/参数位宽。
        esp_lcd_panel_io_spi_config_t io_config = {};
        io_config.cs_gpio_num = DISPLAY_CS_PIN;
        io_config.dc_gpio_num = DISPLAY_DC_PIN;
        io_config.spi_mode = DISPLAY_SPI_MODE;
        io_config.pclk_hz = DISPLAY_SPI_SCLK_HZ;
        io_config.trans_queue_depth = 10;
        io_config.lcd_cmd_bits = 8;
        io_config.lcd_param_bits = 8;
        ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi(DISPLAY_SPI_HOST, &io_config, &panel_io));

        // 面板配置描述复位线、RGB 顺序和像素位深；方向参数在初始化后单独设置。
        esp_lcd_panel_dev_config_t panel_config = {};
        panel_config.reset_gpio_num = DISPLAY_RST_PIN;
        panel_config.rgb_ele_order = DISPLAY_RGB_ORDER;
        panel_config.bits_per_pixel = 16;
        ESP_ERROR_CHECK(esp_lcd_new_panel_st7796(panel_io, &panel_config, &panel));
        ESP_ERROR_CHECK(esp_lcd_panel_reset(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_init(panel));
        ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel, DISPLAY_INVERT_COLOR));
        ESP_ERROR_CHECK(esp_lcd_panel_swap_xy(panel, DISPLAY_SWAP_XY));
        // 先完成硬件方向修正，再打开显示，避免 UI 在初始化中间出现短暂乱码。
        ESP_ERROR_CHECK(esp_lcd_panel_mirror(panel, DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y));
        ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel, true));

        display_ = new SpiLcdDisplay(panel_io, panel, DISPLAY_WIDTH, DISPLAY_HEIGHT,
                                      DISPLAY_OFFSET_X, DISPLAY_OFFSET_Y,
                                      DISPLAY_MIRROR_X, DISPLAY_MIRROR_Y, DISPLAY_SWAP_XY);
        ESP_LOGI(TAG, "ST7796S display initialized");
    }

    void InitializeButtons() {
        // 启动阶段按 BOOT 进入配网；正常运行时同一个按键切换语音对话状态。
        boot_button_.OnClick([this]() {
            auto& app = Application::GetInstance();
            if (app.GetDeviceState() == kDeviceStateStarting) {
                EnterWifiConfigMode();
                return;
            }
            app.ToggleChatState();
        });
    }

    void InitializeServoBus() {
        servo_bus_.Init(SERVO_UART_PORT, SERVO_UART_TX_PIN, SERVO_UART_RX_PIN, SERVO_UART_BAUD_RATE);
    }

    void HandleServoConsoleLine(const char* line) {
        std::istringstream input(line);
        std::string command;
        std::string action;
        int first_id = 1;
        int last_id = 20;
        bool valid = false;
        if (input >> command >> action && command == "servo") {
            if (action == "ping") {
                valid = static_cast<bool>(input >> first_id);
                last_id = first_id;
            } else if (action == "scan") {
                input >> std::ws;
                valid = input.eof() || static_cast<bool>(input >> first_id >> last_id);
            }
        }
        if (valid) {
            input >> std::ws;
            valid = input.eof() && first_id >= 1 && last_id <= 253 && first_id <= last_id;
        }
        if (!valid) {
            ESP_LOGW(TAG, "Usage: servo ping <id> | servo scan [first last]; IDs 1..253; read-only");
            return;
        }

        ESP_LOGI(TAG, "Servo query started: IDs %d..%d", first_id, last_id);
        int found = 0;
        for (int servo_id = first_id; servo_id <= last_id; ++servo_id) {
            const bool responded = servo_bus_.Ping(static_cast<uint8_t>(servo_id));
            ESP_LOGI(TAG, "Servo %d: %s", servo_id, responded ? "valid Ping reply" : "no valid Ping reply");
            if (responded) {
                ++found;
            }
            vTaskDelay(1);
        }
        ESP_LOGI(TAG, "Servo query complete: %d responding ID(s); no valid reply does not mean damaged", found);
    }

    void InitializeServoConsole() {
#if CONFIG_ESP_CONSOLE_UART
        constexpr auto console_port = static_cast<uart_port_t>(CONFIG_ESP_CONSOLE_UART_NUM);
        if (console_port == SERVO_UART_PORT || uart_is_driver_installed(console_port)) {
            ESP_LOGW(TAG, "Servo console unavailable: UART already in use");
            return;
        }
        const esp_err_t result = uart_driver_install(console_port, 512, 0, 0, nullptr, 0);
        if (result != ESP_OK) {
            ESP_LOGE(TAG, "Servo console UART initialization failed: %s", esp_err_to_name(result));
            return;
        }
        const BaseType_t created = xTaskCreate(
            [](void* context) {
                auto* board = static_cast<RobotMBoard*>(context);
                char line[64] = {};
                size_t length = 0;
                bool overflow = false;
                while (true) {
                    char character = 0;
                    if (uart_read_bytes(console_port, &character, 1, portMAX_DELAY) != 1) {
                        vTaskDelay(1);
                        continue;
                    }
                    if (character == '\r' || character == '\n') {
                        if (overflow) {
                            ESP_LOGW(TAG, "Servo console line too long; discarded");
                        } else if (length > 0) {
                            line[length] = '\0';
                            board->HandleServoConsoleLine(line);
                        }
                        length = 0;
                        overflow = false;
                    } else if (!overflow && (character == '\b' || character == 127)) {
                        if (length > 0) {
                            --length;
                        }
                    } else if (!overflow && (character == '\t' || (character >= 32 && character < 127))) {
                        if (length < sizeof(line) - 1) {
                            line[length++] = character;
                        } else {
                            overflow = true;
                        }
                    } else if (!overflow) {
                        overflow = true;
                    }
                }
            },
            "servo_console", 4096, this, 1, nullptr);
        if (created != pdPASS) {
            uart_driver_delete(console_port);
            ESP_LOGE(TAG, "Could not start servo console task");
            return;
        }
        ESP_LOGI(TAG, "Read-only servo console ready: UART%d, %d baud; servo ping <id> | servo scan [first last]",
                 console_port, CONFIG_ESP_CONSOLE_UART_BAUDRATE);
#else
        ESP_LOGW(TAG, "Servo console requires a UART console; native USB shares the servo pins");
#endif
    }

    void RegisterMotionMcpTools() {
        // 语音指令入口：直接映射到 PlatformIO 固件的前进/转向/站立/急停语义，
        // 保持两套固件的动作命名和舵机 ID 分配一致。
        auto& mcp_server = McpServer::GetInstance();

        mcp_server.AddTool(
            "self.robot.servo_ping",
            "查询指定舵机 ID 是否响应，只发 Ping，不移动、不改变扭力或 ID，也不读取角度。"
            "首次调用启动异步查询；返回查询中时，用相同 id 再调用以取得结果。"
            "一次只查询一个 ID，不自动扫描。未收到应答不代表舵机损坏。",
            PropertyList({Property("id", kPropertyTypeInteger, 1, 253)}),
            [this](const PropertyList& properties) -> ToolResult {
                const auto id = static_cast<uint8_t>(properties["id"].value<int>());
                if (servo_ping_state_ != ServoPingState::kIdle) {
                    if (id != servo_ping_id_) {
                        return std::unexpected("请先用 id=" + std::to_string(servo_ping_id_) +
                                               " 获取上一条查询结果，再查询其他 ID。");
                    }
                    if (servo_ping_state_ == ServoPingState::kRunning) {
                        return ReturnValue(std::string("查询中，请用相同 id 再调用获取结果。"));
                    }
                    const bool responded = servo_ping_state_ == ServoPingState::kResponded;
                    servo_ping_state_ = ServoPingState::kIdle;
                    return ReturnValue("舵机 ID " + std::to_string(id) +
                                       (responded ? "：收到有效 Ping 应答；不代表无故障或已校准。"
                                                  : "：未收到有效 Ping 应答，请检查供电、接线、波特率和 ID；不能据此判断损坏。"));
                }

                servo_ping_id_ = id;
                servo_ping_state_ = ServoPingState::kRunning;
                const BaseType_t created = xTaskCreate(
                    [](void* arg) {
                        auto* board = static_cast<RobotMBoard*>(arg);
                        const bool responded = board->servo_bus_.Ping(board->servo_ping_id_);
                        Application::GetInstance().Schedule([board, responded]() {
                            board->servo_ping_state_ = responded ? ServoPingState::kResponded
                                                                : ServoPingState::kNoResponse;
                        });
                        vTaskDelete(nullptr);
                    },
                    "servo_ping", 4096, this, 1, nullptr);
                if (created != pdPASS) {
                    servo_ping_state_ = ServoPingState::kIdle;
                    return std::unexpected("无法启动舵机查询任务，请稍后重试。");
                }
                return ReturnValue(std::string("已启动查询，请用相同 id 再调用获取结果。"));
            });

        mcp_server.AddTool("self.robot.walk_forward", "让机器人持续向前走，直到调用 self.robot.stop",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               limb_controller_.StartForward();
                               return true;
                           });

        mcp_server.AddTool("self.robot.turn_left", "让机器人持续原地左转，直到调用 self.robot.stop",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               limb_controller_.StartTurnLeft();
                               return true;
                           });

        mcp_server.AddTool("self.robot.turn_right", "让机器人持续原地右转，直到调用 self.robot.stop",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               limb_controller_.StartTurnRight();
                               return true;
                           });

        mcp_server.AddTool("self.robot.stand", "让机器人回到站立中位姿态并停止当前动作",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               limb_controller_.Stand();
                               return true;
                           });

        mcp_server.AddTool("self.robot.stop", "立即停止机器人动作并关闭舵机扭力",
                           PropertyList(),
                           [this](const PropertyList&) -> ReturnValue {
                               limb_controller_.Stop();
                               return true;
                           });
    }

public:
    RobotMBoard() : boot_button_(BOOT_BUTTON_GPIO) {
        // 构造板卡对象时完成底层外设初始化，随后由框架取得音频和显示对象。
        InitializeSpi();
        InitializeDisplay();
        InitializeButtons();
        InitializeServoBus();
        RegisterMotionMcpTools();
        InitializeServoConsole();
    }

    AudioCodec* GetAudioCodec() override {
        // 使用无编解码器的 I2S 适配，直接把 INMP441/MAX98357A 接入小智音频管线。
        static NoAudioCodecSimplex audio_codec(
            AUDIO_INPUT_SAMPLE_RATE, AUDIO_OUTPUT_SAMPLE_RATE,
            AUDIO_I2S_SPK_GPIO_BCLK, AUDIO_I2S_SPK_GPIO_LRCK,
            AUDIO_I2S_SPK_GPIO_DOUT, AUDIO_I2S_MIC_GPIO_SCK,
            AUDIO_I2S_MIC_GPIO_WS, AUDIO_I2S_MIC_GPIO_DIN);
        return &audio_codec;
    }

    void StopMotion() override { limb_controller_.Stop(); }

    Display* GetDisplay() override { return display_; }

    Backlight* GetBacklight() override {
        return nullptr;
    }
};

DECLARE_BOARD(RobotMBoard);
