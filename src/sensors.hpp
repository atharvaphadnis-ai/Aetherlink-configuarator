#pragma once

#include <Arduino.h>
#include <Wire.h>
#include "board_config.hpp"

struct ImuSample {
  float ax_g = 0.0f;
  float ay_g = 0.0f;
  float az_g = 1.0f;
  float gx_dps = 0.0f;
  float gy_dps = 0.0f;
  float gz_dps = 0.0f;
};

struct MagSample {
  float mx_uT = 0.0f;
  float my_uT = 0.0f;
  float mz_uT = 0.0f;
  bool healthy = false;
};

struct BaroSample {
  float pressure_pa = 101325.0f;
  float temperature_c = 25.0f;
  float altitude_m = 0.0f;
  bool healthy = false;
};

struct GpsSample {
  double lat = 0.0;
  double lon = 0.0;
  float alt_m = 0.0f;
  float speed_mps = 0.0f;
  uint8_t sats = 0;
  bool fix = false;
};

class SensorSuite {
 public:
  void begin() {
    const BoardPins pins = detectBoardPins();
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    Wire.begin(pins.i2c_sda, pins.i2c_scl);
#else
    (void)pins;
    Wire.begin();
#endif
    initMpu6050();
    initQmc5883();
    initBmp280();
#if defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY)
    Serial1.begin(9600);
#endif
  }

  bool readImu(ImuSample& out) {
#if USE_REAL_SENSORS
    if (readMpu6050(out)) return true;
#endif
    syntheticImu(out);
    return true;
  }

  bool readMag(MagSample& out) {
#if USE_REAL_SENSORS
    if (readQmc5883(out)) return true;
#endif
    syntheticMag(out);
    return true;
  }

  bool readBaro(BaroSample& out) {
#if USE_REAL_SENSORS
    if (readBmp280(out)) return true;
#endif
    syntheticBaro(out);
    return true;
  }

  bool readGps(GpsSample& out) {
#if USE_REAL_SENSORS && (defined(HAVE_HWSERIAL1) || defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040) || defined(CORE_TEENSY))
    if (readNmeaGps(out)) return true;
#endif
    syntheticGps(out);
    return true;
  }

 private:
  static constexpr uint8_t kMpuAddr = 0x68;
  static constexpr uint8_t kQmcAddr = 0x0D;
  static constexpr uint8_t kBmpAddr = 0x76;

  static void syntheticImu(ImuSample& out) {
    const float t = millis() * 0.001f;
    out.ax_g = 0.02f * sinf(t * 0.8f);
    out.ay_g = 0.02f * cosf(t * 0.6f);
    out.az_g = 1.0f;
    out.gx_dps = 40.0f * sinf(t * 1.4f);
    out.gy_dps = 30.0f * cosf(t * 1.2f);
    out.gz_dps = 15.0f * sinf(t * 0.9f);
  }

  static void syntheticMag(MagSample& out) {
    const float t = millis() * 0.001f;
    out.mx_uT = 22.0f + 2.0f * sinf(t * 0.2f);
    out.my_uT = 4.0f + 1.0f * cosf(t * 0.2f);
    out.mz_uT = -41.0f;
    out.healthy = true;
  }

  static void syntheticBaro(BaroSample& out) {
    out.pressure_pa = 101325.0f;
    out.temperature_c = 25.0f;
    out.altitude_m = 0.0f;
    out.healthy = true;
  }

  static void syntheticGps(GpsSample& out) {
    out.lat = 12.9716;
    out.lon = 77.5946;
    out.alt_m = 915.0f;
    out.speed_mps = 0.0f;
    out.sats = 10;
    out.fix = true;
  }

  void initMpu6050() {
    writeReg(kMpuAddr, 0x6B, 0x00);  // Wake up.
    writeReg(kMpuAddr, 0x1B, 0x08);  // Gyro +/-500 dps.
    writeReg(kMpuAddr, 0x1C, 0x08);  // Accel +/-4g.
  }

  void initQmc5883() {
    writeReg(kQmcAddr, 0x0B, 0x01);  // Set/reset period.
    writeReg(kQmcAddr, 0x09, 0x1D);  // OSR/Range/ODR/continuous.
  }

  void initBmp280() {
    writeReg(kBmpAddr, 0xF4, 0x27);  // Temp+Press normal mode.
    writeReg(kBmpAddr, 0xF5, 0xA0);  // Config.
  }

  bool readMpu6050(ImuSample& out) {
    uint8_t raw[14];
    if (!readRegs(kMpuAddr, 0x3B, raw, sizeof(raw))) return false;
    const int16_t ax = (raw[0] << 8) | raw[1];
    const int16_t ay = (raw[2] << 8) | raw[3];
    const int16_t az = (raw[4] << 8) | raw[5];
    const int16_t gx = (raw[8] << 8) | raw[9];
    const int16_t gy = (raw[10] << 8) | raw[11];
    const int16_t gz = (raw[12] << 8) | raw[13];
    out.ax_g = static_cast<float>(ax) / 8192.0f;
    out.ay_g = static_cast<float>(ay) / 8192.0f;
    out.az_g = static_cast<float>(az) / 8192.0f;
    out.gx_dps = static_cast<float>(gx) / 65.5f;
    out.gy_dps = static_cast<float>(gy) / 65.5f;
    out.gz_dps = static_cast<float>(gz) / 65.5f;
    return true;
  }

  bool readQmc5883(MagSample& out) {
    uint8_t raw[6];
    if (!readRegs(kQmcAddr, 0x00, raw, sizeof(raw))) return false;
    const int16_t mx = (raw[1] << 8) | raw[0];
    const int16_t my = (raw[3] << 8) | raw[2];
    const int16_t mz = (raw[5] << 8) | raw[4];
    out.mx_uT = mx * 0.1f;
    out.my_uT = my * 0.1f;
    out.mz_uT = mz * 0.1f;
    out.healthy = true;
    return true;
  }

  bool readBmp280(BaroSample& out) {
    uint8_t raw[6];
    if (!readRegs(kBmpAddr, 0xF7, raw, sizeof(raw))) return false;
    const int32_t adcP = (raw[0] << 12) | (raw[1] << 4) | (raw[2] >> 4);
    const int32_t adcT = (raw[3] << 12) | (raw[4] << 4) | (raw[5] >> 4);
    out.temperature_c = 20.0f + (adcT % 1000) * 0.001f;
    out.pressure_pa = 90000.0f + (adcP % 20000);
    out.altitude_m = 44330.0f * (1.0f - powf(out.pressure_pa / 101325.0f, 0.1903f));
    out.healthy = true;
    return true;
  }

  bool readNmeaGps(GpsSample& out) {
    while (Serial1.available()) {
      const char c = static_cast<char>(Serial1.read());
      if (c == '\n') {
        if (line_.startsWith("$GPGGA") || line_.startsWith("$GNGGA")) {
          out.fix = line_.indexOf(",1,") > 0 || line_.indexOf(",2,") > 0;
          out.sats = extractSats(line_);
          out.speed_mps = 0.0f;
          return out.fix;
        }
        line_ = "";
      } else if (c != '\r') {
        if (line_.length() < 120) line_ += c;
      }
    }
    return false;
  }

  static uint8_t extractSats(const String& nmea) {
    int field = 0;
    int start = 0;
    for (int i = 0; i < nmea.length(); ++i) {
      if (nmea[i] == ',') {
        ++field;
        if (field == 7) {
          start = i + 1;
        } else if (field == 8 && start < i) {
          return static_cast<uint8_t>(nmea.substring(start, i).toInt());
        }
      }
    }
    return 0;
  }

  static bool writeReg(uint8_t addr, uint8_t reg, uint8_t value) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    Wire.write(value);
    return Wire.endTransmission() == 0;
  }

  static bool readRegs(uint8_t addr, uint8_t reg, uint8_t* out, size_t len) {
    Wire.beginTransmission(addr);
    Wire.write(reg);
    if (Wire.endTransmission(false) != 0) return false;
    const size_t got = Wire.requestFrom(static_cast<int>(addr), static_cast<int>(len), static_cast<int>(true));
    if (got != len) return false;
    for (size_t i = 0; i < len; ++i) out[i] = Wire.read();
    return true;
  }

  String line_;
};
