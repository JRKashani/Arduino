# ADXL335_IMU

ADXL335 acceleration and tilt library extracted from the robot's established
implementation. It uses only Arduino core functions. This accelerometer provides
pitch and roll estimates from gravity, with no yaw measurement. Motion acceleration
affects the tilt estimates.

Pass the X, Y, Z and measured supply pins plus an `ADXL335Calibration` to the
constructor. The robot wiring is X=A2, Y=A1, Z=A0, 3.3 V rail=A5. A5 is an analog
measurement input, not AREF. The library leaves the ADC reference unchanged.

The established calibration is:

| Axis | Zero ratio | Ratio per g |
| --- | --- | --- |
| X | 0.5143 | 0.1050 |
| Y | 0.5070 | 0.1020 |
| Z | 0.5068 | 0.0988 |

Conversion: `axis_g = (rawAxis / raw3V3 - zeroRatio) / ratioPerG`.
The library has no hardcoded weights; the caller supplies all six values.
In `last_chance`, edit these in `DEFINE.h`. `setCalibration(weights)` also allows
runtime changes; success clears the baseline, so calibrate again afterward.
Invalid weights are rejected without replacing the previous calibration.

Call `begin()`, then `calibrateBaseline(samples, sampleDelayMs)` with the robot
flat and still. Baseline calibration blocks (about one second for 100 samples
at 10 ms) and averages roll circularly. Invalid readings are skipped; at least
one valid sample is required. Failure preserves the previous baseline.
Baseline angles are held in RAM and recalibrated after reset, not saved to EEPROM.

`read()` returns raw ADC counts, acceleration in g including gravity, magnitude,
raw angles, baseline-corrected angles, and robot angles. Robot roll equals corrected
roll; robot pitch is the negative of corrected pitch, matching the previous robot
code. Baseline subtraction changes angles only, preserving the gravity vector.

Always check `reading.valid` before consuming calculated fields. A nonpositive
supply reading, invalid weights, or zero/nonfinite acceleration magnitude returns
an invalid reading. This is not a full disconnected-wire or motion detector.
`hasBaseline()` is separate: valid raw readings can exist before calibration.

Open `examples/ReadTilt/ReadTilt.ino` for a standalone test at 115200 baud.

Host regression tests use a simulated ADC and do not require hardware. With a
C++ compiler, from this library folder:

```sh
c++ -std=c++11 -Itests -I. ADXL335_IMU.cpp tests/test_imu.cpp -o test_imu
./test_imu
```
