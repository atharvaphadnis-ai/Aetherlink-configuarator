#pragma once

#include <Arduino.h>
#include "board_config.hpp"
#include "control.hpp"

class MotorOutputBackend {
 public:
  void begin(uint16_t pwm_hz) {
    (void)pwm_hz;
    const BoardPins pins = detectBoardPins();
    p1_ = pins.motor1;
    p2_ = pins.motor2;
    p3_ = pins.motor3;
    p4_ = pins.motor4;
    pinMode(p1_, OUTPUT);
    pinMode(p2_, OUTPUT);
    pinMode(p3_, OUTPUT);
    pinMode(p4_, OUTPUT);
  }

  void write(const MotorOutputs& m, uint16_t pwm_min, uint16_t pwm_max) {
#if USE_DSHOT
    writeDshot(m, pwm_min, pwm_max);
#else
    writePwm(m, pwm_min, pwm_max);
#endif
  }

 private:
  void writePwm(const MotorOutputs& m, uint16_t pwm_min, uint16_t pwm_max) {
    analogWrite(p1_, toDuty(m.m1, pwm_min, pwm_max));
    analogWrite(p2_, toDuty(m.m2, pwm_min, pwm_max));
    analogWrite(p3_, toDuty(m.m3, pwm_min, pwm_max));
    analogWrite(p4_, toDuty(m.m4, pwm_min, pwm_max));
  }

  void writeDshot(const MotorOutputs& m, uint16_t pwm_min, uint16_t pwm_max) {
    const uint16_t t1 = throttleToDshot(m.m1, pwm_min, pwm_max);
    const uint16_t t2 = throttleToDshot(m.m2, pwm_min, pwm_max);
    const uint16_t t3 = throttleToDshot(m.m3, pwm_min, pwm_max);
    const uint16_t t4 = throttleToDshot(m.m4, pwm_min, pwm_max);
    const uint16_t f1 = dshotFrame(t1, false);
    const uint16_t f2 = dshotFrame(t2, false);
    const uint16_t f3 = dshotFrame(t3, false);
    const uint16_t f4 = dshotFrame(t4, false);

#if defined(ARDUINO_ARCH_ESP32)
    sendEsp32Rmt(p1_, f1); sendEsp32Rmt(p2_, f2); sendEsp32Rmt(p3_, f3); sendEsp32Rmt(p4_, f4);
#elif defined(ARDUINO_ARCH_RP2040)
    sendRp2040Pio(p1_, f1); sendRp2040Pio(p2_, f2); sendRp2040Pio(p3_, f3); sendRp2040Pio(p4_, f4);
#elif defined(ARDUINO_ARCH_STM32)
    sendStm32TimerDma(p1_, f1); sendStm32TimerDma(p2_, f2); sendStm32TimerDma(p3_, f3); sendStm32TimerDma(p4_, f4);
#elif defined(CORE_TEENSY)
    sendTeensyTimer(p1_, f1); sendTeensyTimer(p2_, f2); sendTeensyTimer(p3_, f3); sendTeensyTimer(p4_, f4);
#else
    // Fallback for unsupported boards.
    writePwm(m, pwm_min, pwm_max);
#endif
  }

  static uint16_t throttleToDshot(uint16_t us, uint16_t min_us, uint16_t max_us) {
    const long clamped = constrain(static_cast<long>(us), static_cast<long>(min_us), static_cast<long>(max_us));
    return static_cast<uint16_t>(map(clamped, min_us, max_us, 48, 2047));
  }

  static uint16_t dshotFrame(uint16_t throttle, bool telemetry) {
    uint16_t frame = static_cast<uint16_t>((throttle << 1) | (telemetry ? 1 : 0));
    uint16_t csum = 0;
    uint16_t csum_data = frame;
    for (int i = 0; i < 3; i++) {
      csum ^= csum_data;
      csum_data >>= 4;
    }
    csum &= 0xF;
    return static_cast<uint16_t>((frame << 4) | csum);
  }

  static uint8_t toDuty(uint16_t us, uint16_t min_us, uint16_t max_us) {
    const long clamped = constrain(static_cast<long>(us), static_cast<long>(min_us), static_cast<long>(max_us));
    return static_cast<uint8_t>(map(clamped, min_us, max_us, 0, 255));
  }

  // The following methods are integration points for board-native DSHOT drivers.
  // They currently emit bit patterns via GPIO for bring-up; replace with DMA/PIO/RMT for production timing margins.
  static void bitbangDshot(uint8_t pin, uint16_t frame) {
    for (int i = 15; i >= 0; --i) {
      const bool bit = (frame >> i) & 0x1;
      digitalWrite(pin, HIGH);
      delayMicroseconds(bit ? 2 : 1);
      digitalWrite(pin, LOW);
      delayMicroseconds(bit ? 1 : 2);
    }
  }

  static void sendEsp32Rmt(uint8_t pin, uint16_t frame) { bitbangDshot(pin, frame); }
  static void sendRp2040Pio(uint8_t pin, uint16_t frame) { bitbangDshot(pin, frame); }
  static void sendStm32TimerDma(uint8_t pin, uint16_t frame) { bitbangDshot(pin, frame); }
  static void sendTeensyTimer(uint8_t pin, uint16_t frame) { bitbangDshot(pin, frame); }

  uint8_t p1_ = 3;
  uint8_t p2_ = 5;
  uint8_t p3_ = 6;
  uint8_t p4_ = 9;
};
