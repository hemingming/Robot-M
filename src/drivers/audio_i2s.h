#pragma once

#include <Arduino.h>

namespace robot::drivers {

void initAudio();
void playAudioTestTone();
uint32_t readMicrophoneLevel();

}  // namespace robot::drivers
