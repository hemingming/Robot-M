#include "audio_i2s.h"

#include <cmath>
#include <driver/i2s.h>

#include "../robot_config.h"

namespace robot::drivers {

void initAudio() {
  // 麦克风和功放共用 I2S 外设；两者都关闭时不申请 DMA 和 I2S 资源。
  if (!robot::kMicrophoneEnabled && !robot::kSpeakerEnabled) {
    return;
  }

  // INMP441 输出 32 位 I2S 帧，当前只采集左声道；MAX98357A 也使用同一时钟。
  const i2s_config_t config = {
      .mode = static_cast<i2s_mode_t>(
          I2S_MODE_MASTER |
          (robot::kSpeakerEnabled ? I2S_MODE_TX : static_cast<i2s_mode_t>(0)) |
          (robot::kMicrophoneEnabled ? I2S_MODE_RX : static_cast<i2s_mode_t>(0))),
      .sample_rate = 16000,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = I2S_CHANNEL_FMT_ONLY_LEFT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 4,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = true,
      .fixed_mclk = 0,
  };
  // 未启用的方向使用 I2S_PIN_NO_CHANGE，避免改写另一侧的无效引脚。
  const i2s_pin_config_t pins = {
      .bck_io_num = robot::kAudioBclkPin,
      .ws_io_num = robot::kAudioLrclkPin,
      .data_out_num = robot::kSpeakerEnabled ? robot::kSpeakerDataPin : I2S_PIN_NO_CHANGE,
      .data_in_num = robot::kMicrophoneEnabled ? robot::kMicrophoneDataPin : I2S_PIN_NO_CHANGE,
  };

  // 先安装驱动，再绑定 GPIO；任一步失败都立即退出并打印 ESP-IDF 错误名。
  const esp_err_t installResult = i2s_driver_install(I2S_NUM_0, &config, 0, nullptr);
  if (installResult != ESP_OK) {
    Serial.printf("I2S driver install failed: %s\n", esp_err_to_name(installResult));
    return;
  }
  const esp_err_t pinResult = i2s_set_pin(I2S_NUM_0, &pins);
  if (pinResult != ESP_OK) {
    Serial.printf("I2S pin setup failed: %s\n", esp_err_to_name(pinResult));
    i2s_driver_uninstall(I2S_NUM_0);
    return;
  }
  i2s_zero_dma_buffer(I2S_NUM_0);
  Serial.printf("I2S audio ready: BCLK=GPIO%u, WS=GPIO%u, mic=GPIO%u, speaker=GPIO%u\n",
                robot::kAudioBclkPin, robot::kAudioLrclkPin,
                robot::kMicrophoneDataPin, robot::kSpeakerDataPin);
}

void playAudioTestTone() {
  if (!robot::kSpeakerEnabled) {
    return;
  }

  // 用短促 440 Hz 正弦波验证功放、I2S 时钟和数据线，不作为持续播放接口。
  constexpr uint32_t sampleRate = 16000;
  constexpr uint32_t sampleCount = sampleRate * 300 / 1000;
  constexpr float frequencyHz = 440.0f;
  constexpr float amplitude = 0.18f;
  int32_t samples[256];
  size_t writtenBytes = 0;

  for (uint32_t offset = 0; offset < sampleCount; offset += 256) {
    const uint32_t blockLength = min<uint32_t>(256, sampleCount - offset);
    for (uint32_t index = 0; index < blockLength; ++index) {
      const float phase = 2.0f * PI * frequencyHz * (offset + index) / sampleRate;
      samples[index] = static_cast<int32_t>(sinf(phase) * amplitude * 2147483647.0f);
    }
    i2s_write(I2S_NUM_0, samples, blockLength * sizeof(samples[0]),
              &writtenBytes, portMAX_DELAY);
  }
  Serial.println("I2S speaker test tone sent.");
}

uint32_t readMicrophoneLevel() {
  if (!robot::kMicrophoneEnabled) {
    return 0;
  }

  // 非阻塞读取一小块样本，避免状态循环因等待 DMA 数据而卡住。
  int32_t samples[128];
  size_t bytesRead = 0;
  const esp_err_t result = i2s_read(I2S_NUM_0, samples, sizeof(samples),
                                    &bytesRead, 0);
  if (result != ESP_OK || bytesRead == 0) {
    Serial.printf("I2S microphone read failed: %s\n", esp_err_to_name(result));
    return 0;
  }

  const size_t sampleCount = bytesRead / sizeof(samples[0]);
  // 对绝对幅值求平均，得到适合显示条使用的粗略音量指标，而不是音频 RMS。
  uint64_t sum = 0;
  for (size_t index = 0; index < sampleCount; ++index) {
    sum += static_cast<uint64_t>(abs(samples[index] >> 14));
  }
  return sum / sampleCount;
}

}  // namespace robot::drivers
