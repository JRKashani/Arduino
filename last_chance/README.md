# last_chance

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
