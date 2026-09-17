#pragma once

#include <Arduino.h>

namespace robot::drivers {

void initDisplay();
void updateMicrophoneDisplay(uint32_t level);

}  // namespace robot::drivers
