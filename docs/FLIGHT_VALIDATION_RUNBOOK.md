# Flight Validation Runbook

## Phase 0: Bench-Only

1. Power from bench supply with current limit.
2. Confirm sensor values are sane at rest.
3. Verify arming cannot happen with invalid preconditions.
4. Verify failsafe cut always forces minimum motor outputs.
5. Confirm blackbox logs all required channels.

## Phase 1: Motor Bench

1. Remove props.
2. Arm/disarm 50 cycles.
3. Sweep throttle and verify motor ordering/direction.
4. Trigger RC-loss simulation and verify staged failsafe.
5. Check for thermal, brownout, and timing anomalies.

## Phase 2: Tethered Hover

1. Install props and secure tether.
2. Hover in Angle mode at low altitude.
3. Validate attitude control response and drift limits.
4. Trigger controlled RC-loss and verify expected behavior.
5. Review blackbox for oscillation/saturation.

## Phase 3: Controlled Free Flight

1. Open field, no people nearby.
2. Start with conservative PID and low rates.
3. Validate Acro/Angle transitions.
4. Validate battery threshold warnings and landing behavior.
5. Collect and review post-flight logs.

## Hard Stop Conditions

- Unexpected arm/disarm transitions
- Any motor runaway event
- Loop timing spikes above safe control envelope
- Sensor estimator divergence or frequent reset
- Failsafe state machine inconsistencies

## Sign-Off Requirements

- Test records archived for each board target.
- Firmware and UI commit hashes documented per test run.
- Known limitations documented before release tagging.
# Flight Validation Runbook

## Pre-Flight Bench Checklist (Props Off)

- Verify target board and firmware checksum
- Verify IMU/mag/baro/GPS health flags
- Run `status`, `get_config`, `blackbox_count`
- Arm/disarm test via CLI and UI buttons
- Trigger RC link loss simulation and confirm failsafe cut
- Confirm motor outputs clamp to safe minimum when disarmed

## Tethered Hover Checklist

- New battery and known-good props/ESCs
- Begin blackbox logging
- Hover 30-60 seconds in angle mode
- Observe vibration and attitude drift
- Confirm failsafe behavior with controlled link drop test

## Envelope Expansion

- Acro mode response checks (roll/pitch/yaw)
- Step-input oscillation observations
- Battery sag vs throttle response
- GPS fix and position-hold behavior (if enabled)

## Abort Conditions

- Unexpected arming while disarmed command active
- Sensor health loss during attitude hold
- Persistent oscillation > 5 degrees peak-to-peak
- Brownout/reset in flight

## Post-Flight

- Export blackbox via `tools/blackbox_export.py`
- Review PID error signatures and saturation events
- Record firmware hash, board, battery, weather, and pilot notes
