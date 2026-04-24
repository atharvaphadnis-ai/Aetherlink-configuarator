#pragma once

#include <Arduino.h>

enum class FlightMode : uint8_t {
  ACRO = 0,
  ANGLE = 1
};

enum class ReceiverProtocol : uint8_t {
  SIM = 0,
  SBUS = 1,
  CRSF = 2,
  IBUS = 3
};

struct PidAxis {
  float kp = 0.4f;
  float ki = 0.12f;
  float kd = 0.015f;
  float i_limit = 200.0f;
};

struct FilterConfig {
  float gyro_pt1_rc = 0.010f;
  float dterm_pt1_rc = 0.015f;
  bool kalman_enabled = true;
  float q_angle = 0.001f;
  float q_bias = 0.003f;
  float r_measure = 0.030f;
  float complementary_alpha = 0.98f;
};

struct BatteryConfig {
  uint8_t cells = 4;
  float low_v_per_cell = 3.50f;
  float critical_v_per_cell = 3.30f;
};

struct FailsafeConfig {
  uint16_t hold_ms = 500;
  uint16_t drop_ms = 300;
  uint16_t cut_ms = 1500;
};

struct ControllerConfig {
  PidAxis roll;
  PidAxis pitch;
  PidAxis yaw;
  FilterConfig filter;
  BatteryConfig battery;
  FailsafeConfig failsafe;
  FlightMode mode = FlightMode::ACRO;
  uint16_t pwm_min = 900;
  uint16_t pwm_max = 2100;
  uint16_t pwm_idle = 1050;
  uint16_t pwm_freq_hz = 490;
  bool soft_start = true;
  uint16_t soft_start_ms = 500;
  bool feature_magnetometer = true;
  bool feature_battery = true;
  bool feature_gps = false;
  bool feature_blackbox = true;
  ReceiverProtocol rx_protocol = ReceiverProtocol::SIM;
};
