#pragma once

#include <Arduino.h>

struct BoardPins {
  uint8_t i2c_sda;
  uint8_t i2c_scl;
  uint8_t motor1;
  uint8_t motor2;
  uint8_t motor3;
  uint8_t motor4;
};

inline BoardPins detectBoardPins() {
#if defined(ARDUINO_ARCH_ESP32)
  return {21, 22, 25, 26, 27, 14};
#elif defined(ARDUINO_ARCH_STM32)
  return {PB9, PB8, PA8, PA9, PA10, PB3};
#elif defined(ARDUINO_ARCH_RP2040)
  return {4, 5, 6, 7, 8, 9};
#elif defined(CORE_TEENSY)
  return {18, 19, 2, 3, 4, 5};
#else
  return {A4, A5, 3, 5, 6, 9};
#endif
}
