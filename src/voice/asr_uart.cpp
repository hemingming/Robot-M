#include "asr_uart.h"

#include "../robot_config.h"

namespace robot::voice {

void AsrUart::begin() {
  serial_.begin(115200, SERIAL_8N1, robot::pins::kAsrUartRx,
                robot::pins::kAsrUartTx);
  lineBuffer_.reserve(32);
  Serial.printf("ASR UART ready: TX=GPIO%u, RX=GPIO%u\n",
                robot::pins::kAsrUartTx, robot::pins::kAsrUartRx);
}

void AsrUart::update() {
  while (serial_.available()) {
    const char character = static_cast<char>(serial_.read());
    if (character == '\n' || character == '\r') {
      if (!lineBuffer_.isEmpty()) {
        handleLine(lineBuffer_);
        lineBuffer_.clear();
      }
    } else if (lineBuffer_.length() < 31) {
      lineBuffer_ += character;
    } else {
      lineBuffer_.clear();
    }
  }
}

void AsrUart::onAction(ActionCallback callback) {
  actionCallback_ = std::move(callback);
}

void AsrUart::handleLine(const String& line) {
  String command = line;
  command.trim();
  command.toUpperCase();

  AsrAction action;
  if (command == "WAKE" || command == "唤醒" || command == "你好大头") {
    action = AsrAction::Wake;
  } else if (command == "FORWARD" || command == "直走") {
    action = AsrAction::Forward;
  } else if (command == "LEFT" || command == "TURN_LEFT" || command == "左转") {
    action = AsrAction::TurnLeft;
  } else if (command == "RIGHT" || command == "TURN_RIGHT" || command == "右转") {
    action = AsrAction::TurnRight;
  } else if (command == "STOP" || command == "停止") {
    action = AsrAction::Stop;
  } else if (command == "STAND" || command == "站立") {
    action = AsrAction::Stand;
  } else {
    return;
  }

  Serial.printf("ASR action: %s\n", command.c_str());
  if (actionCallback_) {
    actionCallback_(action);
  }
}

}  // namespace robot::voice
