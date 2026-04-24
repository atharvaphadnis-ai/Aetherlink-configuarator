#pragma once

#include <Arduino.h>
#include "config.hpp"
#include "fusion.hpp"

struct RcInput {
  int16_t roll = 0;
  int16_t pitch = 0;
  int16_t yaw = 0;
  uint16_t throttle = 1000;
};

struct MotorOutputs {
  uint16_t m1 = 1000;
  uint16_t m2 = 1000;
  uint16_t m3 = 1000;
  uint16_t m4 = 1000;
};

class PID {
 public:
  void reset() { i_ = 0.0f; prevError_ = 0.0f; }

  float update(float sp, float pv, const PidAxis& cfg, float dt) {
    const float error = sp - pv;
    i_ += error * dt;
    i_ = constrain(i_, -cfg.i_limit, cfg.i_limit);
    const float d = (error - prevError_) / max(dt, 1e-4f);
    prevError_ = error;
    return (cfg.kp * error) + (cfg.ki * i_) + (cfg.kd * d);
  }

 private:
  float i_ = 0.0f;
  float prevError_ = 0.0f;
};

class Controller {
 public:
  void update(const ControllerConfig& cfg, const AttitudeEstimate& est, const RcInput& rc, float dt,
              bool armed, MotorOutputs& out) {
    const float rollSp = static_cast<float>(rc.roll) * 0.1f;
    const float pitchSp = static_cast<float>(rc.pitch) * 0.1f;
    const float yawSp = static_cast<float>(rc.yaw) * 0.1f;

    const float uRoll = rollPid_.update(rollSp, est.roll_deg, cfg.roll, dt);
    const float uPitch = pitchPid_.update(pitchSp, est.pitch_deg, cfg.pitch, dt);
    const float uYaw = yawPid_.update(yawSp, est.yaw_deg, cfg.yaw, dt);

    const uint16_t base = constrain(rc.throttle, cfg.pwm_min, cfg.pwm_max);
    if (!armed) {
      out = {cfg.pwm_min, cfg.pwm_min, cfg.pwm_min, cfg.pwm_min};
      return;
    }

    // X-quad mixer
    out.m1 = sat(base + uRoll - uPitch + uYaw, cfg.pwm_min, cfg.pwm_max);
    out.m2 = sat(base - uRoll - uPitch - uYaw, cfg.pwm_min, cfg.pwm_max);
    out.m3 = sat(base - uRoll + uPitch + uYaw, cfg.pwm_min, cfg.pwm_max);
    out.m4 = sat(base + uRoll + uPitch - uYaw, cfg.pwm_min, cfg.pwm_max);
  }

 private:
  static uint16_t sat(float v, uint16_t lo, uint16_t hi) {
    return static_cast<uint16_t>(constrain(v, static_cast<float>(lo), static_cast<float>(hi)));
  }

  PID rollPid_{};
  PID pitchPid_{};
  PID yawPid_{};
};
