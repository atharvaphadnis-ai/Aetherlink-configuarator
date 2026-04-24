#pragma once

#include <Arduino.h>
#include "blackbox.hpp"
#include "config.hpp"
#include "control.hpp"
#include "fusion.hpp"
#include "motor_output.hpp"
#include "receiver.hpp"
#include "safety.hpp"
#include "sensors.hpp"
#include "storage.hpp"

struct TelemetryFrame {
  float roll_deg = 0.0f;
  float pitch_deg = 0.0f;
  float yaw_deg = 0.0f;
  float gyro_x_dps = 0.0f;
  float gyro_y_dps = 0.0f;
  float gyro_z_dps = 0.0f;
  float battery_v = 0.0f;
  uint16_t m1 = 1000;
  uint16_t m2 = 1000;
  uint16_t m3 = 1000;
  uint16_t m4 = 1000;
  uint32_t loop_dt_us = 0;
};

class FlightController {
 public:
  void begin() {
    last_us_ = micros();
    cfg_ = ControllerConfig{};
    storage_.begin();
    (void)storage_.load(cfg_);
    sensors_.begin();
    receiver_.begin();
    safety_.begin();
    motor_output_.begin(cfg_.pwm_freq_hz);
  }

  void update() {
    const uint32_t now = micros();
    frame_.loop_dt_us = max<uint32_t>(1, now - last_us_);
    last_us_ = now;
    const float dt_s = frame_.loop_dt_us * 1e-6f;

    sensors_.readImu(imu_);
    sensors_.readMag(mag_);
    sensors_.readBaro(baro_);
    sensors_.readGps(gps_);
    fusion_.update(cfg_, imu_, dt_s, est_);

    RcInput rc{};
    rc_link_healthy_ = receiver_.read(cfg_.rx_protocol, rc);

    safety_.update(cfg_.failsafe, rc_link_healthy_);
    if (safety_.shouldCutMotors()) {
      motor_ = {cfg_.pwm_min, cfg_.pwm_min, cfg_.pwm_min, cfg_.pwm_min};
    } else {
      control_.update(cfg_, est_, rc, dt_s, safety_.isArmed(), motor_);
    }
    if (USE_REAL_MOTOR_OUTPUT) {
      motor_output_.write(motor_, cfg_.pwm_min, cfg_.pwm_max);
    }

    frame_.roll_deg = est_.roll_deg;
    frame_.pitch_deg = est_.pitch_deg;
    frame_.yaw_deg = est_.yaw_deg;
    frame_.gyro_x_dps = imu_.gx_dps;
    frame_.gyro_y_dps = imu_.gy_dps;
    frame_.gyro_z_dps = imu_.gz_dps;
    frame_.battery_v = batteryVoltageEstimate();
    frame_.m1 = motor_.m1;
    frame_.m2 = motor_.m2;
    frame_.m3 = motor_.m3;
    frame_.m4 = motor_.m4;

    if (cfg_.feature_blackbox) {
      blackbox_.push({millis(), imu_, est_, motor_, frame_.battery_v});
    }
  }

  ControllerConfig& config() { return cfg_; }
  const ControllerConfig& config() const { return cfg_; }
  const TelemetryFrame& telemetry() const { return frame_; }
  void arm() { safety_.arm(); }
  void disarm() { safety_.disarm(); }
  bool saveConfig() { return storage_.save(cfg_); }
  bool loadConfig() { return storage_.load(cfg_); }
  bool armed() const { return safety_.isArmed(); }
  SafetyState safetyState() const { return safety_.state(); }
  Blackbox& blackbox() { return blackbox_; }
  const Blackbox& blackbox() const { return blackbox_; }
  const MagSample& mag() const { return mag_; }
  const BaroSample& baro() const { return baro_; }
  const GpsSample& gps() const { return gps_; }

 private:
  float batteryVoltageEstimate() const {
    const float cells = static_cast<float>(cfg_.battery.cells);
    return cells * 3.9f + 0.08f * sinf(millis() * 0.0002f);
  }

  ControllerConfig cfg_{};
  TelemetryFrame frame_{};
  uint32_t last_us_ = 0;
  SensorSuite sensors_{};
  ImuSample imu_{};
  MagSample mag_{};
  BaroSample baro_{};
  GpsSample gps_{};
  AttitudeEstimate est_{};
  Controller control_{};
  SafetyManager safety_{};
  MotorOutputs motor_{};
  Blackbox blackbox_{};
  MotorOutputBackend motor_output_{};
  ReceiverInput receiver_{};
  ConfigStorage storage_{};
  bool rc_link_healthy_ = false;
};
