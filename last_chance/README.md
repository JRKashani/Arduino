# last_chance

Open `last_chance.ino` in this folder and select Arduino Mega 2560.
The initially empty nested sketch was relocated here beside `DEFINE.h`.
The `ADXL335_IMU` library lives in `../libraries/ADXL335_IMU`, alongside the
steering, ultrasonic and ToF libraries.

The first capsule is `ImuCapsule.cpp`:

- `setupImuCapsule()` initializes the sensor and establishes the flat baseline.
- `runImuCapsule()` refreshes readings and prints diagnostics every 200 ms.
- `latestImuReading()` exposes the latest sample for future stage functions;
  check its `valid` flag before using it.

Edit the six empirical calibration weights, wiring, baseline sampling and print
interval in `DEFINE.h`. Values from the previous project have been preserved.
Open Serial Monitor at **115200 baud**, place the robot flat and still, and reset.
After calibration, robot roll and pitch should be near zero. Tilt each axis and
check its sign. Diagnostics include raw X/Y/Z/reference counts and acceleration.
If baseline calibration fails, correct the wiring and reset to retry.

IMU sampling continues during motion, but periodic printing defaults to off.
Set IMU_PRINT_ENABLED in DEFINE.h to true to see IMU diagnostics again.

## Motion capsule tests

Upload last_chance.ino to Mega 2560, then open Serial Monitor at 115200 baud.
Keep the robot flat and still until startup IMU calibration finishes and the
motion menu appears. The motors start only after a command. Any line ending works.

| Command | Action |
| --- | --- |
| F | Drive forward 60 cm using the timed speed estimate |
| L | Left quarter-circle, nominal radius 60 cm |
| R | Right quarter-circle, approximately 3-5 cm tighter radius |
| S | Stop or cancel the countdown; lowercase s also works |
| H / ? | Menu |

Each movement starts after a two-second countdown and stops automatically.
Send one command at a time and reposition by hand between tests. Any non-whitespace
command during an active test cancels it; send the next command again afterward.
S discards input already queued. These tests have no ultrasonic obstacle stop.

The capsule uses PWM 60, bias -8, forward speed 60/3.883 cm/s, left harshness +10
for 5856 ms, and right harshness -10 for 5343 ms. Parameters are in DEFINE.h.
The older MOTOR_SPEED_SCALE/OFFSET formula is retained for reference; motion uses
the newer fixed-PWM measurement. Times exclude the countdown. The circle times
were measured with a manual stop, so verify actual angles with automatic stopping.
Distance/heading are timed estimates, not encoder or yaw feedback.

API for future independent stages:

```cpp
driveCm(60.0f);
// Or, when idle:
turnQuarterCircle(TurnDirection::Right);
```

Both return true if accepted, false if busy or invalid. driveCm accepts positive
forward distances up to MOTION_MAX_DISTANCE_CM (120 cm by default), with durations
between 1 and 30000 ms. Short distances need validation for startup/coasting effects.
Call runMotionCapsule() frequently in loop(); motionBusy() includes the countdown.
stopMotion() stops/cancels immediately. There is no movement queue. Replace the
Serial test driver with individual mission-stage functions when ready.

## Motor calibration tables in DEFINE.h

- MOTOR_DISTANCE_120CM: all 12 initial PWM/time measurements; bias was not recorded.
- MOTOR_DISTANCE_60CM: 13 PWM/bias groups, mean times and sample counts. Bias +10
  is excluded as requested. Different biases are never pooled.
- MOTOR_TURN_90DEG and MOTOR_TURN_180DEG: per-direction median times, signed
  harshness, PWM, bias and sample counts, using only the replacement turn dataset.
- MOTOR_ARC_60CM_RADIUS: accepted 90-degree arc profile at PWM 60 / bias -8.
- MOTOR_ARC_EXPLORATORY: the two initial bias-0 arc trials for reference only;
  their target radius was not achieved consistently, so they are not active.

Distance rows expose msPerCm() and cmPerSecond(). For a matching PWM and bias,
estimated time_ms = requested_distance_cm * row.msPerCm(). Turn rows expose
leftMsPerDegree() and rightMsPerDegree(); multiplying by a requested angle gives
an unverified scaled estimate. Prefer the directly measured 90/180-degree times.
There is no single verified PWM/harshness-to-angle formula or general curve model.

The existing 0.28198 * PWM - 1.67262 cm/s fit remains available with its explicit
PWM 30-100 bounds. It was fitted to the initial 120 cm data, not the later bias -8
trials. Active motion parameters now derive directly from the selected distance
and arc table rows. Compile-time checks reject mismatched PWM/bias profiles.
The measured baseline F/L/R times are 3883/5856/5343 ms at PWM 60, bias -8.

### Current automatic-test adjustments

After straight travel drifted left and right turns overshot by about 10 cm:
- F uses MOTION_STRAIGHT_BIAS_TRIM=-2, giving effective bias -10; time stays 3883 ms.
- L retains bias -8, harshness +10 and 5856 ms.
- R retains bias -8 and harshness -10, with MOTION_RIGHT_TIME_TRIM_MS=-650,
  giving 4693 ms. The reduction approximates 10 cm / 15.45 cm/s; curved-path speed
  may differ, so this is a trial adjustment, not a new measured calibration.

The library bias is applied separately for each movement so the straight trim
does not alter either turn. Original measured tables remain unchanged. A fixed
bias can compensate a consistent tendency, but cannot remove run-to-run drift.
