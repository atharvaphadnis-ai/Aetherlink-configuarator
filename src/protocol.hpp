#pragma once

#include <Arduino.h>
#include <ArduinoJson.h>
#include "cli.hpp"
#include "flight_controller.hpp"

class Protocol {
 public:
  explicit Protocol(Stream& io) : io_(io) {}

  void begin() {
    rx_.reserve(256);
  }

  void pump(FlightController& fc) {
    while (io_.available()) {
      const char c = static_cast<char>(io_.read());
      if (c == '\n') {
        handleLine(fc, rx_);
        rx_.clear();
      } else if (c != '\r') {
        if (rx_.length() < 512) rx_ += c;
      }
    }
  }

  void sendTelemetry(const TelemetryFrame& t) {
    StaticJsonDocument<512> doc;
    doc["type"] = "telemetry";
    doc["roll"] = t.roll_deg;
    doc["pitch"] = t.pitch_deg;
    doc["yaw"] = t.yaw_deg;
    doc["gx"] = t.gyro_x_dps;
    doc["gy"] = t.gyro_y_dps;
    doc["gz"] = t.gyro_z_dps;
    doc["bv"] = t.battery_v;
    doc["m1"] = t.m1;
    doc["m2"] = t.m2;
    doc["m3"] = t.m3;
    doc["m4"] = t.m4;
    doc["dt"] = t.loop_dt_us;
    serializeJson(doc, io_);
    io_.println();
  }

 private:
  void handleLine(FlightController& fc, const String& line) {
    if (line.length() == 0) return;

    StaticJsonDocument<512> in;
    const auto err = deserializeJson(in, line);
    if (err) {
      sendError("invalid_json");
      return;
    }

    const char* cmd = in["cmd"] | "";
    if (strcmp(cmd, "status") == 0) {
      sendStatus(fc);
    } else if (strcmp(cmd, "get_config") == 0) {
      sendConfig(fc.config());
    } else if (strcmp(cmd, "arm") == 0) {
      fc.arm();
      sendOk("arm");
    } else if (strcmp(cmd, "disarm") == 0) {
      fc.disarm();
      sendOk("disarm");
    } else if (strcmp(cmd, "set_mode") == 0) {
      const char* mode = in["mode"] | "acro";
      fc.config().mode = (strcmp(mode, "angle") == 0) ? FlightMode::ANGLE : FlightMode::ACRO;
      sendOk("set_mode");
    } else if (strcmp(cmd, "set_pid") == 0) {
      const char* axis = in["axis"] | "";
      const float kp = in["kp"] | 0.0f;
      const float ki = in["ki"] | 0.0f;
      const float kd = in["kd"] | 0.0f;
      setPid(fc, axis, kp, ki, kd);
    } else if (strcmp(cmd, "set_battery") == 0) {
      fc.config().battery.cells = constrain(in["cells"] | 4, 1, 12);
      fc.config().battery.low_v_per_cell = in["low"] | 3.5f;
      fc.config().battery.critical_v_per_cell = in["critical"] | 3.3f;
      sendOk("set_battery");
    } else if (strcmp(cmd, "set_rx_protocol") == 0) {
      const char* p = in["protocol"] | "sim";
      if (strcmp(p, "sbus") == 0) fc.config().rx_protocol = ReceiverProtocol::SBUS;
      else if (strcmp(p, "crsf") == 0) fc.config().rx_protocol = ReceiverProtocol::CRSF;
      else if (strcmp(p, "ibus") == 0) fc.config().rx_protocol = ReceiverProtocol::IBUS;
      else fc.config().rx_protocol = ReceiverProtocol::SIM;
      sendOk("set_rx_protocol");
    } else if (strcmp(cmd, "save_config") == 0) {
      if (!fc.saveConfig()) {
        sendError("save_failed");
        return;
      }
      sendOk("save_config");
    } else if (strcmp(cmd, "load_config") == 0) {
      if (!fc.loadConfig()) {
        sendError("load_failed");
        return;
      }
      sendOk("load_config");
    } else if (strcmp(cmd, "cli") == 0) {
      const char* line = in["line"] | "";
      runCli(fc, line);
    } else if (strcmp(cmd, "blackbox_count") == 0) {
      sendBlackboxCount(fc);
    } else if (strcmp(cmd, "blackbox_get") == 0) {
      sendBlackboxRecord(fc, in["index"] | 0);
    } else {
      sendError("unknown_command");
    }
  }

  void setPid(FlightController& fc, const char* axis, float kp, float ki, float kd) {
    PidAxis* target = nullptr;
    if (strcmp(axis, "roll") == 0) target = &fc.config().roll;
    if (strcmp(axis, "pitch") == 0) target = &fc.config().pitch;
    if (strcmp(axis, "yaw") == 0) target = &fc.config().yaw;
    if (!target) {
      sendError("invalid_axis");
      return;
    }

    target->kp = constrain(kp, 0.0f, 4.0f);
    target->ki = constrain(ki, 0.0f, 2.0f);
    target->kd = constrain(kd, 0.0f, 1.0f);
    sendOk("set_pid");
  }

  void sendStatus(const FlightController& fc) {
    StaticJsonDocument<256> out;
    out["type"] = "status";
    out["armed"] = fc.armed();
    out["mode"] = fc.config().mode == FlightMode::ANGLE ? "angle" : "acro";
    out["rx"] = rxProtocolString(fc.config().rx_protocol);
    out["safety"] = static_cast<uint8_t>(fc.safetyState());
    out["uptime_ms"] = millis();
    serializeJson(out, io_);
    io_.println();
  }

  void sendConfig(const ControllerConfig& cfg) {
    StaticJsonDocument<512> out;
    out["type"] = "config";
    JsonObject roll = out["roll"].to<JsonObject>();
    roll["kp"] = cfg.roll.kp;
    roll["ki"] = cfg.roll.ki;
    roll["kd"] = cfg.roll.kd;
    JsonObject pitch = out["pitch"].to<JsonObject>();
    pitch["kp"] = cfg.pitch.kp;
    pitch["ki"] = cfg.pitch.ki;
    pitch["kd"] = cfg.pitch.kd;
    JsonObject yaw = out["yaw"].to<JsonObject>();
    yaw["kp"] = cfg.yaw.kp;
    yaw["ki"] = cfg.yaw.ki;
    yaw["kd"] = cfg.yaw.kd;
    out["cells"] = cfg.battery.cells;
    out["mode"] = cfg.mode == FlightMode::ANGLE ? "angle" : "acro";
    out["rx"] = rxProtocolString(cfg.rx_protocol);
    serializeJson(out, io_);
    io_.println();
  }

  void sendBlackboxCount(const FlightController& fc) {
    StaticJsonDocument<128> out;
    out["type"] = "blackbox_count";
    out["count"] = fc.blackbox().count();
    serializeJson(out, io_);
    io_.println();
  }

  void sendBlackboxRecord(const FlightController& fc, size_t index) {
    BlackboxRecord rec;
    if (!fc.blackbox().getByIndex(index, rec)) {
      sendError("blackbox_index_oob");
      return;
    }
    StaticJsonDocument<384> out;
    out["type"] = "blackbox_record";
    out["i"] = index;
    out["t"] = rec.t_ms;
    out["roll"] = rec.est.roll_deg;
    out["pitch"] = rec.est.pitch_deg;
    out["yaw"] = rec.est.yaw_deg;
    out["gx"] = rec.imu.gx_dps;
    out["gy"] = rec.imu.gy_dps;
    out["gz"] = rec.imu.gz_dps;
    out["m1"] = rec.motor.m1;
    out["m2"] = rec.motor.m2;
    out["m3"] = rec.motor.m3;
    out["m4"] = rec.motor.m4;
    out["bv"] = rec.battery_v;
    serializeJson(out, io_);
    io_.println();
  }

  void sendOk(const char* action) {
    StaticJsonDocument<128> out;
    out["type"] = "ok";
    out["action"] = action;
    serializeJson(out, io_);
    io_.println();
  }

  void sendError(const char* reason) {
    StaticJsonDocument<128> out;
    out["type"] = "error";
    out["reason"] = reason;
    serializeJson(out, io_);
    io_.println();
  }

  void runCli(FlightController& fc, const char* line) {
    CliCommand cmd;
    if (!cli_.parse(String(line), cmd)) {
      sendError("cli_parse_error");
      return;
    }

    if (cmd.tokens[0] == "status") {
      sendStatus(fc);
      return;
    }
    if (cmd.tokens[0] == "arm") {
      fc.arm();
      sendOk("cli_arm");
      return;
    }
    if (cmd.tokens[0] == "disarm") {
      fc.disarm();
      sendOk("cli_disarm");
      return;
    }
    if (cmd.tokens[0] == "help") {
      StaticJsonDocument<256> out;
      out["type"] = "cli_help";
      out["commands"] = "status arm disarm pid calibrate save defaults set";
      serializeJson(out, io_);
      io_.println();
      return;
    }
    if (cmd.tokens[0] == "save") {
      if (!fc.saveConfig()) {
        sendError("cli_save_failed");
        return;
      }
      sendOk("cli_save");
      return;
    }
    if (cmd.tokens[0] == "defaults") {
      fc.config() = ControllerConfig{};
      (void)fc.saveConfig();
      sendOk("cli_defaults");
      return;
    }
    if (cmd.tokens[0] == "calibrate") {
      sendOk("cli_calibrate");
      return;
    }
    if (cmd.tokens[0] == "pid" && cmd.count == 1) {
      sendConfig(fc.config());
      return;
    }
    if (cmd.tokens[0] == "set" && cmd.count == 3) {
      if (cmd.tokens[1] == "rkp") {
        fc.config().roll.kp = cmd.tokens[2].toFloat();
        sendOk("cli_set_rkp");
        return;
      }
      if (cmd.tokens[1] == "glpf") {
        fc.config().filter.gyro_pt1_rc = cmd.tokens[2].toFloat();
        sendOk("cli_set_glpf");
        return;
      }
    }

    sendError("cli_unknown_command");
  }

  Stream& io_;
  String rx_;
  CliParser cli_{};

  static const char* rxProtocolString(ReceiverProtocol p) {
    switch (p) {
      case ReceiverProtocol::SBUS: return "sbus";
      case ReceiverProtocol::CRSF: return "crsf";
      case ReceiverProtocol::IBUS: return "ibus";
      case ReceiverProtocol::SIM:
      default: return "sim";
    }
  }
};
