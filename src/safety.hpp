#pragma once

#include <Arduino.h>
#include "config.hpp"

enum class SafetyState : uint8_t {
  DISARMED = 0,
  ARMED = 1,
  FAILSAFE_HOLD = 2,
  FAILSAFE_DROP = 3,
  FAILSAFE_CUT = 4
};

class SafetyManager {
 public:
  void begin() { state_ = SafetyState::DISARMED; }

  void arm() {
    if (state_ == SafetyState::DISARMED) state_ = SafetyState::ARMED;
  }

  void disarm() { state_ = SafetyState::DISARMED; }

  void update(const FailsafeConfig& cfg, bool rcHealthy) {
    const uint32_t now = millis();
    if (rcHealthy) {
      lastHealthyMs_ = now;
      if (state_ != SafetyState::DISARMED) state_ = SafetyState::ARMED;
      return;
    }

    const uint32_t lostMs = now - lastHealthyMs_;
    if (state_ == SafetyState::DISARMED) return;

    if (lostMs < cfg.hold_ms) {
      state_ = SafetyState::FAILSAFE_HOLD;
    } else if (lostMs < (cfg.hold_ms + cfg.drop_ms)) {
      state_ = SafetyState::FAILSAFE_DROP;
    } else if (lostMs >= cfg.cut_ms) {
      state_ = SafetyState::FAILSAFE_CUT;
    }
  }

  bool isArmed() const { return state_ == SafetyState::ARMED; }
  bool shouldCutMotors() const { return state_ == SafetyState::FAILSAFE_CUT; }
  SafetyState state() const { return state_; }

 private:
  SafetyState state_ = SafetyState::DISARMED;
  uint32_t lastHealthyMs_ = 0;
};
