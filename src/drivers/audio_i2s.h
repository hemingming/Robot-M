#pragma once

#include <Arduino.h>

namespace robot::drivers {

void initAudio();
void playAudioTestTone();
// 唤醒词命中后的短促反馈音，提示机器人已进入监听窗口。
void playWakeChirp();
uint32_t readMicrophoneLevel();

}  // namespace robot::drivers
