#pragma once

#include <Arduino.h>
#include "control.hpp"
#include "fusion.hpp"
#include "sensors.hpp"

struct BlackboxRecord {
  uint32_t t_ms;
  ImuSample imu;
  AttitudeEstimate est;
  MotorOutputs motor;
  float battery_v;
};

class Blackbox {
 public:
  static constexpr size_t kMaxRecords = 6000;

  void push(const BlackboxRecord& r) {
    records_[head_] = r;
    head_ = (head_ + 1) % kMaxRecords;
    if (count_ < kMaxRecords) ++count_;
  }

  size_t count() const { return count_; }

  bool getByIndex(size_t idx, BlackboxRecord& out) const {
    if (idx >= count_) return false;
    const size_t start = (head_ + kMaxRecords - count_) % kMaxRecords;
    const size_t real = (start + idx) % kMaxRecords;
    out = records_[real];
    return true;
  }

 private:
  BlackboxRecord records_[kMaxRecords]{};
  size_t head_ = 0;
  size_t count_ = 0;
};
