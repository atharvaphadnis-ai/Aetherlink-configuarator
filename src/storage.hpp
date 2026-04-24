#pragma once

#include <Arduino.h>
#include <EEPROM.h>
#include "config.hpp"

class ConfigStorage {
 public:
  void begin() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    EEPROM.begin(kStorageBytes);
#endif
  }

  bool save(const ControllerConfig& cfg) {
    Record rec{};
    rec.magic = kMagic;
    rec.version = kVersion;
    rec.seq = ++seq_;
    rec.payload = cfg;
    rec.crc = crc32(reinterpret_cast<const uint8_t*>(&rec.payload), sizeof(rec.payload));
    writeRecord(kPrimaryAddr, rec);
    writeRecord(kBackupAddr, rec);
    commit();
    return true;
  }

  bool load(ControllerConfig& cfg) {
    Record primary{};
    Record backup{};
    const bool pOk = readRecord(kPrimaryAddr, primary) && valid(primary);
    const bool bOk = readRecord(kBackupAddr, backup) && valid(backup);
    if (!pOk && !bOk) return false;
    const Record& chosen = (pOk && (!bOk || primary.seq >= backup.seq)) ? primary : backup;
    cfg = chosen.payload;
    seq_ = chosen.seq;
    return true;
  }

 private:
  struct Record {
    uint32_t magic = 0;
    uint16_t version = 0;
    uint16_t reserved = 0;
    uint32_t seq = 0;
    ControllerConfig payload{};
    uint32_t crc = 0;
  };

  static constexpr uint32_t kMagic = 0x494E4156;
  static constexpr uint16_t kVersion = 2;
  static constexpr size_t kPrimaryAddr = 0;
  static constexpr size_t kBackupAddr = 512;
  static constexpr size_t kStorageBytes = 2048;
  uint32_t seq_ = 0;

  static bool valid(const Record& rec) {
    if (rec.magic != kMagic || rec.version != kVersion) return false;
    const uint32_t calc = crc32(reinterpret_cast<const uint8_t*>(&rec.payload), sizeof(rec.payload));
    return calc == rec.crc;
  }

  static uint32_t crc32(const uint8_t* data, size_t len) {
    uint32_t crc = 0xFFFFFFFFu;
    while (len--) {
      crc ^= *data++;
      for (uint8_t i = 0; i < 8; ++i) {
        const uint32_t mask = static_cast<uint32_t>(-(static_cast<int32_t>(crc & 1)));
        crc = (crc >> 1) ^ (0xEDB88320u & mask);
      }
    }
    return ~crc;
  }

  static void writeRecord(size_t addr, const Record& rec) {
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&rec);
    for (size_t i = 0; i < sizeof(rec); ++i) EEPROM.write(addr + i, p[i]);
  }

  static bool readRecord(size_t addr, Record& rec) {
    if (addr + sizeof(rec) > kStorageBytes) return false;
    uint8_t* p = reinterpret_cast<uint8_t*>(&rec);
    for (size_t i = 0; i < sizeof(rec); ++i) p[i] = EEPROM.read(addr + i);
    return true;
  }

  static void commit() {
#if defined(ARDUINO_ARCH_ESP32) || defined(ARDUINO_ARCH_RP2040)
    EEPROM.commit();
#endif
  }
};
