#pragma once

#include <Arduino.h>
#include "config.hpp"
#include "control.hpp"

class ReceiverInput {
 public:
  void begin() {
#if defined(HAVE_HWSERIAL2) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY)
    Serial2.begin(420000);
#endif
  }

  bool read(ReceiverProtocol protocol, RcInput& out) {
    bool ok = false;
    switch (protocol) {
      case ReceiverProtocol::SBUS: ok = readSbus(out); break;
      case ReceiverProtocol::CRSF: ok = readCrsf(out); break;
      case ReceiverProtocol::IBUS: ok = readIbus(out); break;
      case ReceiverProtocol::SIM:
      default: break;
    }
    if (!ok) synthetic(out);
    return ok;
  }

 private:
  static void synthetic(RcInput& out) {
    out.throttle = 1500;
    out.roll = static_cast<int16_t>(120.0f * sinf(millis() * 0.001f));
    out.pitch = static_cast<int16_t>(90.0f * cosf(millis() * 0.001f));
    out.yaw = 0;
  }

  static uint8_t crc8DvbS2(const uint8_t* ptr, uint8_t len) {
    uint8_t crc = 0;
    while (len--) {
      crc ^= *ptr++;
      for (uint8_t i = 0; i < 8; i++) {
        crc = (crc & 0x80) ? static_cast<uint8_t>((crc << 1) ^ 0xD5) : static_cast<uint8_t>(crc << 1);
      }
    }
    return crc;
  }

  bool readSbus(RcInput& out) {
#if defined(HAVE_HWSERIAL2) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY)
    while (Serial2.available()) {
      const uint8_t b = static_cast<uint8_t>(Serial2.read());
      if (sbus_idx_ == 0 && b != 0x0F) continue;
      sbus_[sbus_idx_++] = b;
      if (sbus_idx_ < 25) continue;
      sbus_idx_ = 0;
      if (sbus_[24] != 0x00 && sbus_[24] != 0x04) return false;
      const uint16_t c1 = ((sbus_[1] | sbus_[2] << 8) & 0x07FF);
      const uint16_t c2 = ((sbus_[2] >> 3 | sbus_[3] << 5) & 0x07FF);
      const uint16_t c3 = ((sbus_[3] >> 6 | sbus_[4] << 2 | sbus_[5] << 10) & 0x07FF);
      const uint16_t c4 = ((sbus_[5] >> 1 | sbus_[6] << 7) & 0x07FF);
      out.roll = map(c1, 172, 1811, -500, 500);
      out.pitch = map(c2, 172, 1811, -500, 500);
      out.throttle = static_cast<uint16_t>(map(c3, 172, 1811, 900, 2100));
      out.yaw = map(c4, 172, 1811, -500, 500);
      return true;
    }
#endif
    return false;
  }

  bool readCrsf(RcInput& out) {
#if defined(HAVE_HWSERIAL2) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY)
    while (Serial2.available()) {
      const uint8_t b = static_cast<uint8_t>(Serial2.read());
      if (crsf_idx_ == 0) {
        crsf_[crsf_idx_++] = b;
        continue;
      }
      if (crsf_idx_ == 1) {
        crsf_len_ = b;
        if (crsf_len_ < 2 || crsf_len_ > 62) {
          crsf_idx_ = 0;
          continue;
        }
        crsf_[crsf_idx_++] = b;
        continue;
      }
      crsf_[crsf_idx_++] = b;
      if (crsf_idx_ < static_cast<uint8_t>(crsf_len_ + 2)) continue;
      const uint8_t type = crsf_[2];
      const uint8_t payload_len = crsf_len_ - 2;
      const uint8_t crc = crsf_[crsf_len_ + 1];
      if (crc8DvbS2(&crsf_[2], payload_len + 1) != crc) {
        crsf_idx_ = 0;
        continue;
      }
      if (type != 0x16 || payload_len < 22) {
        crsf_idx_ = 0;
        continue;
      }
      const uint8_t* p = &crsf_[3];
      const uint16_t c1 = (p[0] | (p[1] << 8)) & 0x07FF;
      const uint16_t c2 = ((p[1] >> 3) | (p[2] << 5)) & 0x07FF;
      const uint16_t c3 = ((p[2] >> 6) | (p[3] << 2) | (p[4] << 10)) & 0x07FF;
      const uint16_t c4 = ((p[4] >> 1) | (p[5] << 7)) & 0x07FF;
      out.roll = map(c1, 172, 1811, -500, 500);
      out.pitch = map(c2, 172, 1811, -500, 500);
      out.throttle = static_cast<uint16_t>(map(c3, 172, 1811, 900, 2100));
      out.yaw = map(c4, 172, 1811, -500, 500);
      crsf_idx_ = 0;
      return true;
    }
#endif
    return false;
  }

  bool readIbus(RcInput& out) {
#if defined(HAVE_HWSERIAL2) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY)
    while (Serial2.available()) {
      const uint8_t b = static_cast<uint8_t>(Serial2.read());
      if (ibus_idx_ == 0 && b != 0x20) continue;
      ibus_[ibus_idx_++] = b;
      if (ibus_idx_ < 32) continue;
      ibus_idx_ = 0;
      uint16_t sum = 0xFFFF;
      for (int i = 0; i < 30; ++i) sum -= ibus_[i];
      const uint16_t rxCrc = static_cast<uint16_t>(ibus_[30] | (ibus_[31] << 8));
      if (sum != rxCrc) continue;
      const uint16_t c1 = static_cast<uint16_t>(ibus_[2] | (ibus_[3] << 8));
      const uint16_t c2 = static_cast<uint16_t>(ibus_[4] | (ibus_[5] << 8));
      const uint16_t c3 = static_cast<uint16_t>(ibus_[6] | (ibus_[7] << 8));
      const uint16_t c4 = static_cast<uint16_t>(ibus_[8] | (ibus_[9] << 8));
      out.roll = map(c1, 1000, 2000, -500, 500);
      out.pitch = map(c2, 1000, 2000, -500, 500);
      out.throttle = static_cast<uint16_t>(map(c3, 1000, 2000, 900, 2100));
      out.yaw = map(c4, 1000, 2000, -500, 500);
      return true;
    }
#endif
    return false;
  }

  uint8_t sbus_[25]{};
  uint8_t sbus_idx_ = 0;
  uint8_t crsf_[64]{};
  uint8_t crsf_idx_ = 0;
  uint8_t crsf_len_ = 0;
  uint8_t ibus_[32]{};
  uint8_t ibus_idx_ = 0;
};
