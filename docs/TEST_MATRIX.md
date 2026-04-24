# Cross-Platform Test Matrix

## Targets

- ESP32 dev board
- STM32 Nucleo F446RE (reference STM32)
- Raspberry Pi Pico (RP2040)
- Teensy 4.1

## Host Platforms

- Windows 10/11
- macOS 13+
- Ubuntu 22.04+
- Android (PWA in Chromium)

## Required Test Categories

## 1. Build and Flash
- Compile each board target.
- Flash each board.
- Confirm boot banner and telemetry output.

## 2. Serial Link
- Port enumeration.
- Connect/disconnect cycles (100x).
- Command round-trip latency under 100 ms.
- Telemetry continuity with no parser crashes.

## 3. Protocol and CLI
- `status`, `get_config`, `set_pid`, `set_mode`, `set_battery`, `arm`, `disarm`.
- `cli` token parsing to 16 tokens.
- Unknown/invalid JSON command error handling.

## 4. Fusion and Control
- Complementary mode outputs stable on static bench.
- Kalman mode outputs stable on static bench.
- PID and mixer output bounds always inside `[pwm_min, pwm_max]`.
- Yaw wrapping stays in `[-180, 180]`.

## 5. Safety
- Arming/disarming logic.
- Failsafe transitions `ARMED -> HOLD -> DROP -> CUT`.
- Motor cut behavior on failsafe cut state.

## 6. Blackbox
- Ring buffer inserts under full telemetry rate.
- `blackbox_count` and `blackbox_get` correctness.
- CSV export script produces parsable output.

## 7. Frontend PWA
- Connect/disconnect via Web Serial.
- Real-time chart renders 60s window.
- PID editor writes values and receives `ok`.
- CLI console sends commands and shows responses.
- Manifest loads and standalone install prompt appears.

## 8. Long-Run Stability
- 2h continuous loop test with telemetry streaming.
- 24h idle + reconnect stress.
- Memory use and loop timing trend checks.

## Exit Criteria (Minimum)

- All critical tests pass on all target boards.
- No unresolved crash, lockup, or motor-cut safety bug.
- No command parser corruption under malformed input.
- Bench and tether tests pass before untethered flight.
# Cross-Platform Test Matrix

## Boards

- ESP32 (ESP32-WROOM devkit)
- STM32 (F4/F7 reference: Nucleo F446RE)
- RP2040 (Raspberry Pi Pico)
- Teensy 4.1

## Host Platforms

- Windows 10/11
- macOS 12+
- Ubuntu 22.04+
- Android (PWA in Chromium-based browser)

## Required Test Cases

1. Serial connect/disconnect and auto-recovery
2. CLI command parsing and error handling
3. Parameter write/read round-trip (`set_pid`, `get_config`)
4. Arming/disarming state transitions
5. Failsafe phases and motor cut behavior
6. Telemetry latency (<100 ms target on USB serial)
7. Blackbox count/download/export consistency
8. Frontend graph rendering under 60 s rolling buffer
9. PWA install and offline shell availability
10. Brownout/reboot recovery behavior

## Pass Criteria

- No unhandled exceptions in host UI during 30 minute session
- No firmware hard faults/resets under 30 minute telemetry run
- Failsafe transitions match configured timeout windows (+/- 20 ms)
- Blackbox exported row count equals firmware-reported count
- CPU/loop timing jitter within board-specific budget

## Validation Stages

- Stage 1: Unit/static checks
- Stage 2: Bench test (props off)
- Stage 3: Tethered hover
- Stage 4: Controlled outdoor envelope expansion

Production release requires all stages completed and signed off.
