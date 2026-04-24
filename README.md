# INAV Multi-Board Configurator (Firmware + PWA)

Cross-platform drone configurator stack with:
- Arduino/PlatformIO firmware core for `ESP32`, `STM32`, `RP2040 (Pico)`, `Teensy 4.1`
- React + Tailwind + Web Serial PWA frontend
- Serial JSON command protocol
- Sensor fusion/control/failsafe/blackbox architecture

## Safety and Production Reality

This repository now includes substantial implementation, but **no one can honestly guarantee zero minor/major errors or guaranteed stable flight without full hardware validation**.

For real production flight, complete the validation program in `docs/TEST_MATRIX.md` and run staged bench/tether/field tests on each board and sensor set.

---

## Implemented Firmware Modules (`src/`)

- `config.hpp`  
  Runtime config (PID/filter/battery/failsafe/platform flags).

- `sensors.hpp`  
  Sensor suite with real-device read paths for MPU6050/QMC5883/BMP280/basic NMEA GPS plus synthetic fallback.

- `fusion.hpp`  
  Complementary and Kalman attitude estimation paths with runtime toggle.

- `control.hpp`  
  PID loops + X-quad mixer + motor output saturation.

- `safety.hpp`  
  Arming/disarming and staged failsafe state machine (`HOLD`, `DROP`, `CUT`).

- `blackbox.hpp`  
  In-memory ring-buffer blackbox recorder.

- `cli.hpp`  
  16-token CLI parser.

- `protocol.hpp`  
  JSON serial protocol commands:
  - `status`
  - `get_config`
  - `set_mode`
  - `set_pid`
  - `set_battery`
  - `set_rx_protocol` (`sim`/`sbus`/`crsf`/`ibus`)
  - `save_config`
  - `load_config`
  - `arm`
  - `disarm`
  - `cli` (line parser)
  - `blackbox_count`
  - `blackbox_get`

- `main.cpp`  
  Control loop and telemetry scheduler.

- `board_config.hpp`  
  Per-board pin mapping defaults for I2C and motor outputs.

- `motor_output.hpp`  
  Real motor backend with PWM writes and DSHOT integration point.

- `receiver.hpp`  
  RC receiver layer with SBUS/CRSF/IBUS parser paths plus protocol fallback.

- `storage.hpp`  
  EEPROM-backed versioned configuration store with CRC32 and primary/backup recovery.

---

## Implemented Frontend (`frontend/`)

- Vite + React + TypeScript + Tailwind setup
- Web Serial client
- Live telemetry chart (Recharts)
- PID editor panel
- CLI console panel
- Arm/disarm controls
- PWA manifest

> Browser: Web Serial works in Chromium-based browsers.

---

## Build and Run

### Firmware

Install PlatformIO Core and run:

```bash
pio run -e esp32dev
pio run -e stm32
pio run -e rpipico
pio run -e teensy41
```

Build flags in `platformio.ini`:
- `USE_REAL_SENSORS=1`
- `USE_REAL_MOTOR_OUTPUT=1`
- `USE_DSHOT=0` (set to `1` to enable DSHOT transport adapter path)

Upload (example):

```bash
pio run -e esp32dev -t upload
```

### Frontend

```bash
cd frontend
npm install
npm run dev
```

Open the printed localhost URL, click **Connect**, choose the serial port, then use status/PID/CLI flows.

---

## Blackbox Export Tool

`tools/blackbox_export.py` exports firmware blackbox frames to CSV:

```bash
python tools/blackbox_export.py --port COM5 --baud 115200 --out blackbox.csv
```

Requires:

```bash
pip install pyserial
```

---

## What Still Needs Hardware-Specific Completion

These are scaffolded and partially implemented, but still require production hardening:

- Sensor drivers are integrated for common chips; calibration and per-board validation are still required
- Real ESC output stack still needs full DSHOT/OneShot board-specific timing backends
- Persistent storage needs schema migration/versioning and corruption recovery
- Deterministic scheduling and interrupt strategy tuning per MCU
- Brownout/reboot recovery and watchdog integration per target
- Full HIL/SITL replay and long-run soak testing

---

## Validation and Flight Readiness

Use:
- `docs/TEST_MATRIX.md`
- `docs/FLIGHT_VALIDATION_RUNBOOK.md`

Do not fly untethered until all required checks pass.
