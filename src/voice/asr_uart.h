#pragma once

#include <Arduino.h>
#include <functional>

namespace robot::voice {

enum class AsrAction : uint8_t {
  Wake,
  Forward,
  TurnLeft,
  TurnRight,
  Stop,
  Stand,
};

class AsrUart {
 public:
  using ActionCallback = std::function<void(AsrAction action)>;

  void begin();
  void update();
  void onAction(ActionCallback callback);

 private:
  void handleLine(const String& line);

  HardwareSerial serial_{2};
  String lineBuffer_;
  ActionCallback actionCallback_;
};

}  // namespace robot::voice
