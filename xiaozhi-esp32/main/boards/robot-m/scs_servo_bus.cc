#include "scs_servo_bus.h"

#include <cstring>

#include <esp_log.h>

#define TAG "ScsServoBus"

namespace robot_m {

namespace {
constexpr uint8_t kInstPing = 0x01;
constexpr uint8_t kInstWrite = 0x03;
constexpr uint8_t kBroadcastId = 0xFE;
constexpr uint8_t kRegTorqueEnable = 40;
constexpr uint8_t kRegGoalPositionL = 42;
constexpr int kAckTimeoutMs = 30;
}  // namespace

void ScsServoBus::Init(uart_port_t port, gpio_num_t tx_pin, gpio_num_t rx_pin, int baud_rate) {
    port_ = port;
    uart_config_t config = {};
    config.baud_rate = baud_rate;
    config.data_bits = UART_DATA_8_BITS;
    config.parity = UART_PARITY_DISABLE;
    config.stop_bits = UART_STOP_BITS_1;
    config.flow_ctrl = UART_HW_FLOWCTRL_DISABLE;
    config.source_clk = UART_SCLK_DEFAULT;

    ESP_ERROR_CHECK(uart_driver_install(port_, 256, 256, 0, nullptr, 0));
    ESP_ERROR_CHECK(uart_param_config(port_, &config));
    ESP_ERROR_CHECK(uart_set_pin(port_, tx_pin, rx_pin, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    initialized_ = true;
    ESP_LOGI(TAG, "Servo bus ready: TX=GPIO%d, RX=GPIO%d, baud=%d", tx_pin, rx_pin, baud_rate);
}

void ScsServoBus::FlushInput() {
    uart_flush_input(port_);
}

void ScsServoBus::WriteBuffer(uint8_t id, uint8_t instruction, const uint8_t* params, uint8_t param_len) {
    const uint8_t length = static_cast<uint8_t>(2 + param_len);
    uint8_t checksum = static_cast<uint8_t>(id + length + instruction);
    for (uint8_t i = 0; i < param_len; ++i) {
        checksum = static_cast<uint8_t>(checksum + params[i]);
    }
    checksum = static_cast<uint8_t>(~checksum);

    uint8_t header[5] = {0xFF, 0xFF, id, length, instruction};
    uart_write_bytes(port_, reinterpret_cast<const char*>(header), sizeof(header));
    if (param_len > 0) {
        uart_write_bytes(port_, reinterpret_cast<const char*>(params), param_len);
    }
    uart_write_bytes(port_, reinterpret_cast<const char*>(&checksum), 1);
}

bool ScsServoBus::ReadAck(uint8_t id) {
    // 广播命令没有应答，直接视为成功。
    if (id == kBroadcastId) {
        return true;
    }

    uint8_t previous = 0;
    uint8_t current = 0;
    bool header_found = false;
    for (int attempt = 0; attempt < 16; ++attempt) {
        if (uart_read_bytes(port_, &current, 1, pdMS_TO_TICKS(kAckTimeoutMs)) != 1) {
            return false;
        }
        if (previous == 0xFF && current == 0xFF) {
            header_found = true;
            break;
        }
        previous = current;
    }
    if (!header_found) {
        return false;
    }

    uint8_t body[4] = {0};
    if (uart_read_bytes(port_, body, sizeof(body), pdMS_TO_TICKS(kAckTimeoutMs)) != sizeof(body)) {
        return false;
    }
    if (body[0] != id || body[1] != 2) {
        return false;
    }
    const uint8_t expectedChecksum = static_cast<uint8_t>(~(body[0] + body[1] + body[2]));
    return expectedChecksum == body[3];
}

bool ScsServoBus::Ping(uint8_t id) {
    if (!initialized_ || id == 0 || id == kBroadcastId) {
        return false;
    }
    FlushInput();
    WriteBuffer(id, kInstPing, nullptr, 0);
    return ReadAck(id);
}

bool ScsServoBus::WritePosition(uint8_t id, int16_t position, uint16_t speed) {
    if (!initialized_ || id == 0 || id == kBroadcastId) {
        return false;
    }
    // SCS0017 位置范围为 0..1023，越界会撞机械限位。
    if (position < 0 || position > 1023) {
        return false;
    }
    // 先开扭力再下发目标位置，避免舵机处于自由转动状态时收到位置指令。
    SetTorque(id, true);

    uint8_t params[7];
    params[0] = kRegGoalPositionL;
    params[1] = static_cast<uint8_t>(position & 0xFF);
    params[2] = static_cast<uint8_t>((position >> 8) & 0xFF);
    params[3] = 0;  // Time low：不使用定时位置，交由 Speed 控制运动时间。
    params[4] = 0;  // Time high
    params[5] = static_cast<uint8_t>(speed & 0xFF);
    params[6] = static_cast<uint8_t>((speed >> 8) & 0xFF);

    FlushInput();
    WriteBuffer(id, kInstWrite, params, sizeof(params));
    return ReadAck(id);
}

bool ScsServoBus::SetTorque(uint8_t id, bool enable) {
    if (!initialized_ || id == 0) {
        return false;
    }
    uint8_t params[2] = {kRegTorqueEnable, static_cast<uint8_t>(enable ? 1 : 0)};
    FlushInput();
    WriteBuffer(id, kInstWrite, params, sizeof(params));
    return ReadAck(id);
}

void ScsServoBus::StopAll() {
    if (!initialized_) {
        return;
    }
    SetTorque(kBroadcastId, false);
}

}  // namespace robot_m
