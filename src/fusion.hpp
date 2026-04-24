#pragma once

#include <Arduino.h>
#include "config.hpp"
#include "sensors.hpp"

struct AttitudeEstimate {
  float roll_deg = 0.0f;
  float pitch_deg = 0.0f;
  float yaw_deg = 0.0f;
};

class ComplementaryEstimator {
 public:
  void update(const ImuSample& imu, float dt_s, float alpha, AttitudeEstimate& est) {
    const float accelRoll = atan2f(imu.ay_g, imu.az_g) * 57.29578f;
    const float accelPitch = atan2f(-imu.ax_g, sqrtf(imu.ay_g * imu.ay_g + imu.az_g * imu.az_g)) * 57.29578f;
    est.roll_deg = alpha * (est.roll_deg + imu.gx_dps * dt_s) + (1.0f - alpha) * accelRoll;
    est.pitch_deg = alpha * (est.pitch_deg + imu.gy_dps * dt_s) + (1.0f - alpha) * accelPitch;
    est.yaw_deg += imu.gz_dps * dt_s;
  }
};

class Kalman1D {
 public:
  void update(float newAngle, float newRate, float dt, float qAngle, float qBias, float rMeasure,
              float& angleOut) {
    angle_ += dt * (newRate - bias_);
    p00_ += dt * (dt * p11_ - p01_ - p10_ + qAngle);
    p01_ -= dt * p11_;
    p10_ -= dt * p11_;
    p11_ += qBias * dt;

    const float s = p00_ + rMeasure;
    const float k0 = p00_ / s;
    const float k1 = p10_ / s;
    const float y = newAngle - angle_;
    angle_ += k0 * y;
    bias_ += k1 * y;

    const float p00Tmp = p00_;
    const float p01Tmp = p01_;
    p00_ -= k0 * p00Tmp;
    p01_ -= k0 * p01Tmp;
    p10_ -= k1 * p00Tmp;
    p11_ -= k1 * p01Tmp;

    angleOut = angle_;
  }

 private:
  float angle_ = 0.0f;
  float bias_ = 0.0f;
  float p00_ = 1.0f, p01_ = 0.0f, p10_ = 0.0f, p11_ = 1.0f;
};

class SensorFusion {
 public:
  void update(const ControllerConfig& cfg, const ImuSample& imu, float dt_s, AttitudeEstimate& est) {
    const float accelRoll = atan2f(imu.ay_g, imu.az_g) * 57.29578f;
    const float accelPitch = atan2f(-imu.ax_g, sqrtf(imu.ay_g * imu.ay_g + imu.az_g * imu.az_g)) * 57.29578f;
    if (cfg.filter.kalman_enabled) {
      rollKal_.update(accelRoll, imu.gx_dps, dt_s, cfg.filter.q_angle, cfg.filter.q_bias,
                      cfg.filter.r_measure, est.roll_deg);
      pitchKal_.update(accelPitch, imu.gy_dps, dt_s, cfg.filter.q_angle, cfg.filter.q_bias,
                       cfg.filter.r_measure, est.pitch_deg);
      est.yaw_deg += imu.gz_dps * dt_s;
    } else {
      comp_.update(imu, dt_s, cfg.filter.complementary_alpha, est);
    }

    if (est.yaw_deg > 180.0f) est.yaw_deg -= 360.0f;
    if (est.yaw_deg < -180.0f) est.yaw_deg += 360.0f;
  }

 private:
  ComplementaryEstimator comp_{};
  Kalman1D rollKal_{};
  Kalman1D pitchKal_{};
};
