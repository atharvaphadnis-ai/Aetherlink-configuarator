#include <Arduino.h>
#include "flight_controller.hpp"
#include "protocol.hpp"

namespace {
FlightController g_fc;
Protocol* g_protocol = nullptr;
uint32_t g_lastTelemetryMs = 0;
constexpr uint16_t kTelemetryPeriodMs = static_cast<uint16_t>(1000 / TELEMETRY_HZ);
}  // namespace

void setup() {
  Serial.begin(115200);
  while (!Serial && millis() < 3000) {
    delay(10);
  }

  g_fc.begin();
  static Protocol protocol(Serial);
  g_protocol = &protocol;
  g_protocol->begin();

  Serial.println("{\"type\":\"boot\",\"name\":\"inav-multiboard-fw\",\"ver\":\"0.1.0\"}");
}

void loop() {
  g_fc.update();
  g_protocol->pump(g_fc);

  const uint32_t nowMs = millis();
  if (nowMs - g_lastTelemetryMs >= kTelemetryPeriodMs) {
    g_lastTelemetryMs = nowMs;
    g_protocol->sendTelemetry(g_fc.telemetry());
  }
}
