# last_chance

Archived measurements, raw records and reviews: [distance results](docs/distance-results-2026-10-11/README.md).

Distance-sensor assignment: [guided US / right-laser / analog-IR test](docs/distance-test.md).
Build/upload `distance_test`, then run the recorder directly from PowerShell:
`& .\.venv\Scripts\python.exe -u tools\distance_test\distance_mission.py record --sensor us --port COM7`.

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
The [full sample CSV](docs/imu-results-2026-10-11/samples.csv) contains all
25,500 samples, including raw ADC readings and calibrated acceleration in g.

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
`T` telemetry, `1` run Stage 1 (open area, then Stage 2), `2` run Stage 2
(funnel, starting already aligned), `3` run Stage 3
(right-wall follow), `X` stop, `H` help.

Stage 1 starts within 1.5 m of the funnel entrance, facing either wall. It drives
straight until the front wall is 30 cm from the robot center, stops, compares the
left/right lasers, and turns 90 degrees toward the closer side. It then uses the
side laser facing the original forward wall (right after a left turn, left after
a right turn), driving until front and that side agree within 2 cm or cross.
If front is already shorter at the first comparison, it stops for repositioning.
At equality, Stage 1 ends and automatically hands off to Stage 2 (funnel).
Stage 2 turns back 45 degrees, then drives forward while comparing both sides.
When they differ by more than 2 cm, it stops, turns 45 degrees toward the farther
wall, drives 5 cm, turns back 45 degrees, and measures again. Filters are cleared
and refilled for 700 ms after maneuvers. Stage 2 has no end condition yet;
`S`/`X` stops, and the front emergency stop cancels either stage.
To test only the funnel, place the robot already facing along the funnel and
send `2`; this direct start skips the 45-degree entry turn.

Change `STAGE1_WALL_TARGET_CM`, `STAGE1_EQUAL_TOLERANCE_CM`,
`STAGE2_EQUAL_TOLERANCE_CM`, `STAGE2_CORRECTION_DISTANCE_CM`, and
`ENTRY_MEASURE_SETTLE_MS` in `src/DEFINE.h`
to tune this sequence. Invalid readings pause measurement-based forward motion;
missing or equal initial side readings keep it stopped until comparison is possible.
Turns use the measured PWM 60, bias 0, harshness +50/-50 row (852 ms left,
903 ms right for 90 degrees), without countdown. The 45-degree times are scaled
estimates (426 ms left, 452 ms right), not separately measured calibration.
The 5 cm move uses the PWM 60 / bias -8 distance row (~324 ms). Short moves and
the full sequence need verification on hardware.

All pins, calibration and tuning constants: `src/DEFINE.h`
(motor calibration tables: `src/MotorCalibration.h`).

For the physical funnel test, upload with `pio run -e megaatmega2560 -t upload`,
then open `pio device monitor` at 115200 baud. Stage 1 only starts when you send
`1`, and continues automatically into Stage 2. Send `2` for a direct funnel test.
Full logging starts automatically: every sensor update prints a timestamp,
filtered distances, raw laser values, calibrated ultrasonic samples and LDRs.
`--` marks invalid distances. `USnew=front/R45/none` identifies which ultrasonic
was sampled in that update; the other ultrasonic reading is retained from its
previous ping. Every Stage 1/2 decision prints its stage, state, action and reason,
including waiting on invalid readings, comparisons, timed maneuvers and safety
stops. Turn and 5 cm drive commands report PWM, bias and duration. No `T` command
is needed. `S`/`X` stops the stage and its automatic logging; `T` independently
toggles the slower sensor dashboard before or after a run.
