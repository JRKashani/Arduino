# last_chance

Standalone stationary IMU experiment: see
[the tilt mission instructions](docs/imu-tilt-mission.md). Build/upload with
`pio run -e imu_tilt -t upload`, then use the guided recorder in
`tools/imu_tilt/tilt_mission.py`. The normal robot firmware is the default build.

The replacement ADXL335 is the current robot sensor. Its active six-face
calibration and wiring are documented in
[the sensor record](docs/imu-calibration-replacement-2026-10-11.md).
The [five-slope results](docs/imu-results-2026-10-11/comparison-review.md) and
[raw ADC / g tables](docs/imu-results-2026-10-11/tables/measured-tables.md)
use that calibration; the phone-derived proposal has not been applied.

Robot firmware for the Braude mechatronics final project (Arduino Mega 2560),
built with PlatformIO. **Read [HANDOFF.md](HANDOFF.md)** for the full project
state, hardware map, Stage 3 design, tuning guide and next steps.

Quick start (from this folder):

| What | Command |
|---|---|
| Build | `pio run` |
| Upload to the Mega | `pio run -t upload` |
| Serial console (115200) | `pio device monitor` |
| Host unit tests | `pio test -e native` |

Keep the robot flat and still at reset (IMU calibration). In the console:
`T` telemetry, `3` run Stage 3 (right-wall follow), `X` stop, `H` help.

All pins, calibration and tuning constants: `src/DEFINE.h`
(motor calibration tables: `src/MotorCalibration.h`).
