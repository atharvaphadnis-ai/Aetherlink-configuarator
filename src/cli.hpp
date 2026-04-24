#pragma once

#include <Arduino.h>

struct CliCommand {
  String tokens[CLI_MAX_TOKENS];
  size_t count = 0;
};

class CliParser {
 public:
  bool parse(const String& line, CliCommand& out) const {
    out.count = 0;
    String cur;
    for (size_t i = 0; i < line.length(); ++i) {
      const char c = line[i];
      if (c == ' ' || c == '\t') {
        if (cur.length()) {
          if (out.count >= CLI_MAX_TOKENS) return false;
          out.tokens[out.count++] = cur;
          cur = "";
        }
      } else {
        cur += c;
      }
    }
    if (cur.length()) {
      if (out.count >= CLI_MAX_TOKENS) return false;
      out.tokens[out.count++] = cur;
    }
    return out.count > 0;
  }
};
