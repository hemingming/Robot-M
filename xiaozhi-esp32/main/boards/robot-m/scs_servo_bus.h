#ifndef _ROBOT_M_SCS_SERVO_BUS_H_
#define _ROBOT_M_SCS_SERVO_BUS_H_

#include <cstdint>

#include <driver/gpio.h>
#include <driver/uart.h>

namespace robot_m {

// URT-2 总线舵机（SCS0017，SCSCL 协议）的最小 ESP-IDF UART 驱动。
// 只实现语音动作需要的子集：Ping、写目标位置、开关扭力。
class ScsServoBus {
public:
    void Init(uart_port_t port, gpio_num_t tx_pin, gpio_num_t rx_pin, int baud_rate);
    bool Ping(uint8_t id);
    bool WritePosition(uint8_t id, int16_t position, uint16_t speed);
    bool SetTorque(uint8_t id, bool enable);
    // SCS 广播 ID（0xFE）关闭扭力，停止所有舵机的保持力矩。
    void StopAll();

private:
    void WriteBuffer(uint8_t id, uint8_t instruction, const uint8_t* params, uint8_t param_len);
    bool ReadAck(uint8_t id);
    void FlushInput();

    uart_port_t port_ = UART_NUM_1;
    bool initialized_ = false;
};

}  // namespace robot_m

#endif  // _ROBOT_M_SCS_SERVO_BUS_H_
