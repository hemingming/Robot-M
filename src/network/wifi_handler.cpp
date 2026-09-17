#include "wifi_handler.h"

#include <Arduino.h>
#include <WiFi.h>

#include "../config/project_config.h"

#if __has_include("../wifi_secrets.h")
#include "../wifi_secrets.h"
#else
namespace wifi_secrets {
constexpr char kSsid[] = "";
constexpr char kPassword[] = "";
}  // namespace wifi_secrets
#endif

namespace robot::network {

void initWifi() {
  if (wifi_secrets::kSsid[0] == '\0') {
    Serial.println("Wi-Fi not configured. Add src/wifi_secrets.h.");
    return;
  }

  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.begin(wifi_secrets::kSsid, wifi_secrets::kPassword);
  Serial.printf("Wi-Fi connecting to %s", wifi_secrets::kSsid);

  const uint32_t startMs = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startMs < robot::config::kWifiConnectTimeoutMs) {
    delay(250);
    Serial.print('.');
  }
  Serial.println();

  if (WiFi.status() == WL_CONNECTED) {
    Serial.printf("Wi-Fi connected: %s, RSSI=%d dBm\n",
                  WiFi.localIP().toString().c_str(), WiFi.RSSI());
  } else {
    Serial.printf("Wi-Fi connection timeout, status=%d\n", WiFi.status());
  }
}

}  // namespace robot::network
