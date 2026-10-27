# Stationary distance-sensor experiment

Test one sensor at a time: front HC-SR04 (`us`), right VL53L0X (`laser`), then
Sharp GP2Y0A21YK0F (`ir`, marked 2Y0A21). The dedicated `distance_test` firmware
holds all motor outputs off. Reposition the robot or target manually.

Enter the actual tape measurement from the selected **sensor face** to the target
in cm. Choose as many positions as you want; after each position the recorder
offers to add another measured distance or finish. Face that sensor squarely
toward a large, flat wall/board. For the right
laser, the robot's right side faces the target. Use the same target, lighting,
power setup and reference face throughout a sensor run. Keep the tape out of
the beam. A suggested 200 cm position can be entered as 198.7 cm if that is the
actual measurement; plots use 198.7.

## Wiring

[Wiring diagram (PNG)](diagrams/distance-sensors-wiring.png) ·
[SVG](diagrams/distance-sensors-wiring.svg) · [PDF](diagrams/distance-sensors-wiring.pdf).
The diagram shows **one Arduino Mega 2560** connected to all three sensors,
with a shared 5 V rail above and common ground below. The laser module uses the
user-confirmed 5 V supply; power-pin names follow the module labels. The IMU's
separate 3.3 V supply is not shown.

| Sensor | Existing / new connection |
|---|---|
| Front HC-SR04 | TRIG 43, ECHO 42; existing 5 V and GND |
| Right VL53L0X | SDA 20, SCL 21, XSHUT 22, address 0x30; module VIN/VCC to confirmed 5 V, GND common |
| Sharp analog IR | **Vo/output to A6**, Vcc to **5 V**, GND to Arduino GND |

A0-A5 are already assigned to the IMU, its supply monitor and the LDRs. A6 is
reserved by this test program for the new IR output. Follow the sensor's pin
labels/datasheet, rather than assuming wire colors or a connector orientation.
Sharp specifies 4.5-5.5 V supply and recommends at least 10 uF bypass capacitance
between Vcc and GND near the sensor (observe polarity for an electrolytic).
Connect with power removed. Keep the IR emitter covered or disconnected during
the laser run so it does not illuminate the target; record the setup used.
The left laser stays in shutdown and the second ultrasonic is never triggered.

## Ranges and sampling positions

| Sensor | Datasheet / operating context | Test plan |
|---|---|---|
| HC-SR04 | Nominal 2-400 cm; echo us / 58 gives cm | 200 down to 10 cm |
| VL53L0X | Up to 2 m under suitable conditions; default profile can have a shorter reliable range | 200 down to 10 cm, preserving failures |
| GP2Y0A21YK0F | Specified **10-80 cm**, nonlinear analog output | 200 down to 10 cm; >80 cm explicitly outside specification |

Start around 200 cm and work toward 10 cm at distances you choose. Use wider
spacing far away and closer spacing near the IR sensor's nonlinear near response.
The recorder imposes no list of distances or total number of positions. Practical
spacing informed by the range and response is not a measurement of intrinsic
sensor resolution. A 10-200 cm experiment characterizes only this portion of
the US range.

At each distance, make at least **three separate captures**. The default is three
captures of ten samples, one sample every **0.5 seconds**. Each repeat spans
**5 seconds** after Enter (first sample at 0.5 s, last at 5 s), giving **15 seconds
of recording per distance**, plus time spent confirming repeats. The US timeout
is 30 ms, allowing the roughly 11.6 ms round-trip at 2 m;
the navigation firmware's 10 ms timeout would cut off before that endpoint.
Laser single-shot readings use the long-range profile in firmware 1.2.0 with a
33 ms timing budget. Actual sample timestamps are saved if a slow read overruns
the requested interval. No median filter or previous-position data are reused.

The selected sensor streams continuously every 0.5 seconds, including while you
type distances or move the target. The recorder terminal displays these readings
and retains partially typed input. Preview readings are kept in the serial log;
only captures explicitly triggered with Enter enter the measurement tables.
US output includes raw echo time, uncorrected cm and calibrated cm; laser output
includes mm and cm; IR output includes ADC counts and voltage.

The recorder terminal acts as the live serial monitor during the experiment.
Close Arduino IDE / PlatformIO's separate serial monitor before recording, since
only one program can open COM7. For a standalone serial monitor at 115200 baud,
the firmware starts streaming the front US automatically. Send `MONITOR,L` for
the right laser, `MONITOR,I` for IR, `MONITOR,U` for US, or `MONITOR,OFF` to stop
preview. The IR voltage printed directly by firmware assumes nominal 5 V; the
recorder uses your entered reference voltage. Motor outputs remain off.

## Run from PowerShell

From `C:\Users\Efi\Documents\Arduino\last_chance`, close any serial monitor.
Dependencies are installed in this project's `.venv`:

```powershell
& .\.venv\Scripts\python.exe -m pip install -r tools\distance_test\requirements.txt
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e distance_test -t upload --upload-port COM7
```

Start the front ultrasonic directly through Python. This works even when
PowerShell's execution policy disables `.ps1` scripts:

```powershell
& .\.venv\Scripts\python.exe -u tools\distance_test\distance_mission.py record --sensor us --port COM7
```

After completing it, start a new session for each next sensor (same firmware):

```powershell
& .\.venv\Scripts\python.exe -u tools\distance_test\distance_mission.py record --sensor laser --port COM7
& .\.venv\Scripts\python.exe -u tools\distance_test\distance_mission.py record --sensor ir --port COM7
```

Each command is interactive; run the next after the previous has finished.
Use `--port COM8` if the port changes. List ports with:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\distance_mission.py ports
```

The terminal sequence is:

```text
Measurement position 1.
ACTUAL measured distance (cm): 198.7
  Repeat 1/3 at measured 198.7 cm.
  Press Enter when still to capture (no distance here):
  ... automatic sensor readings ...
  Accept [a/Enter], redo this repeat [r], or skip repeat [s]:
  ... complete the other two captures ...
Add another measured distance? [Y/n]: y
Measurement position 2.
ACTUAL measured distance (cm): 172.4
```

The same measured distance applies to all three repeats. If the target moves,
quit and resume to remeasure the unfinished point. Redo excludes that attempt
from the curves while keeping its samples. Accepting a capture with no usable
readings records a failure, not a zero distance. A point with fewer than three
usable accepted captures is flagged as failing the repetition requirement.

`q` at a prompt or Ctrl+C saves and stops. Resume with:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\distance_mission.py record --port COM7 --resume "distance_sessions\YOUR_SESSION"
```

Completed/skipped points are preserved. An unfinished point starts again with
a new tape measurement and three captures; earlier partial attempts remain in
the logs and are excluded. The original sensor and repetition count are used on
resume. The current 0.5 s / 5 s timing is appended to the log, and each capture
records its timing so earlier measurements retain their original settings.
Older sessions with a fixed distance plan also resume in open-ended mode;
their existing metadata and measurements remain preserved. A changed firmware
configuration requires a new session.

## Calibration and outputs

The firmware reports the active shared ultrasonic coefficients from `src/DEFINE.h`.
Currently `raw_cm = echo_us / 58`, then
`calibrated_cm = raw_cm * 1.01535 + 0.70910`. Their original stated measurement
interval was 15-150 cm; outside it, the report flags extrapolation. No 10 cm
robot-center offset is added. Laser mm are divided by 10 for cm, with no 1.2 cm
mounting offset and no extra empirical laser calibration.

Distance-test firmware 1.2.0 uses the VL53L0X long-range profile for testing up to
200 cm: signal-rate limit 0.1 MCPS, pre-range VCSEL period 18 PCLKs, final-range
period 14 PCLKs, and a 33 ms timing budget. These are the
[Pololu long-range settings](https://github.com/pololu/vl53l0x-arduino/blob/master/examples/Single/Single.ino).
Long-range mode increases sensitivity and can also increase unwanted-reflection
errors. Target reflectivity and ambient lighting affect whether 200 cm works;
invalid readings remain flagged. Live output stays at 500 ms and captures at
five seconds per repeat. The profile is saved in session metadata. Start a new
session after changing firmware profiles so older measurements remain separate.

There is no measured calibration for this IR unit yet. Preserve its ADC response;
do not apply an arbitrary internet inverse-distance formula. Optional measured
Arduino 5 V rail voltage improves the approximate ADC-to-volts conversion:
`V = ADC * reference_v / 1023`. Enter leaves a clearly labeled nominal 5.0 V.
This is a recorded reference assumption, not a live measurement of AVCC.

The completed IR run used a measured reference of **4.71 V** and has 450 samples
at 15 distances from 10 to 80 cm. Its ADC response flattens and reverses at farther
distances; about 172 counts occurs at both 40 and 80 cm. A single ADC-to-distance
lookup would be ambiguous. Live output therefore remains ADC/voltage, with no
automatic cm conversion. Use this run as a measured characteristic, not a
validated distance calibration.

The optional **graph-only laser correction** is
`corrected_cm = 0.969241451 * reported_cm - 0.699266228`.
It was fitted to 16 position means over 20-170 cm. The original laser mount was
straightened around 60 cm; the exact transition is uncertain. The separate
50/60/70/91 cm follow-up was not used to fit coefficients: its mean absolute error
decreases from 2.446 to 0.904 cm. This is a limited consistency check with the
same tape reference. The correction is not applied to firmware, recorder output,
or the raw laser graphs. Retain the mount-change note and avoid treating training
residuals as independent accuracy validation.

The session directory contains `session.json`, append-only `events-*.jsonl` and
`serial-*.log`. Source data are flushed after each received sample. Each export
goes into a new `analysis/TIMESTAMP` directory:

| File | Contents |
|---|---|
| `measurements.xlsx` | Experiment metadata, every sample, capture summaries, point summaries and two three-curve charts |
| `samples.csv` | Every attempt, raw output, validity, status, timestamp, conversions and inclusion flag |
| `captures.csv` | Every capture's mean, min, max, sample standard deviation and valid count |
| `summary.csv` | Actual distance and min/mean/max of accepted capture means at each point |
| `characteristic.png` / `.svg` | Minimum, mean and maximum curves, plus converted-output panel |
| `report.md`, `sources.json` | Procedure, limitations, coverage and source hashes |

The three assignment curves are taken across the three repeated **capture means**,
with equal weight per capture. The ten individual samples quantify variability
within each capture and remain available separately. Missing values
stay blank in Excel/CSV and create gaps in plots. IR out-of-range raw values remain
visible and the >80 cm interval is shaded. No new calibration is automatically applied.

Regenerate an export with:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\distance_mission.py analyze "distance_sessions\YOUR_SESSION"
```

## Completed results and presentation graphs

Start with the [results index](distance-results-2026-10-11/README.md), which links
the retained source sessions, workbooks, CSVs and reviews. The principal graph
data are 19 US positions, 16 laser positions and 15 IR positions: **1,500 accepted
samples**. Rejected attempts and the separate laser follow-up remain outside
these curves.

| Deliverable | Units and treatment |
|---|---|
| [Combined graph with experimental laser correction](distance-results-2026-10-11/comparison-calibrated-laser/README.md) | IR ADC on left y-axis; calibrated US cm and experimentally corrected laser cm on right |
| [Combined graph without laser correction](distance-results-2026-10-11/comparison/README.md) | Same layout; laser is reported mm divided by 10 |
| [Three individual raw graphs and PDF](distance-results-2026-10-11/raw-sensor-graphs/README.md) | US echo µs, laser reported mm, IR ADC; US right y-axis shows `(echo_us / 58) * 1.01535 + 0.70910` cm |
| [Single-Arduino wiring diagram](diagrams/distance-sensors-wiring.png) | Front US, right laser and IR on one Mega; SVG/PDF also available |

For these presentation graphs, each point is the mean of three accepted repeat
means. **Vertical bars span the minimum and maximum individual accepted samples**;
the separate raw graphs also show those samples as faint dots. Horizontal bars
are **±1 cm**, representing the assumed tape uncertainty (2 cm total width).
They are not confidence intervals. Corrected laser sample extrema use the same
scale and offset as their means; fit-coefficient uncertainty is not included.
The combined graph's independent y-scales mean intersections do not indicate
equal numerical readings. The recorder's original three assignment curves use
the range of repeat means instead; do not confuse the two definitions.

Rebuild graphs and wiring from the archived records without opening COM7:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\plot_comparison.py
& .\.venv\Scripts\python.exe tools\distance_test\plot_comparison.py --calibrated-laser
& .\.venv\Scripts\python.exe tools\distance_test\plot_raw_sensors.py
& .\.venv\Scripts\python.exe docs\diagrams\generate_distance_wiring.py
```

These commands create PNG/SVG/PDF figures; the graph directories also contain
plotted-data CSVs and source hashes. The corrected graph adds a calibration JSON
with coefficients and residuals. Source session records remain unchanged.

## Sources

- [Sharp GP2Y0A21YK0F datasheet](https://global.sharp/products/device/lineup/data/pdf/datasheet/gp2y0a21yk_e.pdf)
- [HC-SR04 module datasheet (ElecFreaks, hosted by SparkFun)](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf)
- [ST VL53L0X datasheet](https://www.st.com/resource/en/datasheet/vl53l0x.pdf)
- [Pololu single-ranging example and profile options](https://github.com/pololu/vl53l0x-arduino/blob/master/examples/Single/Single.ino)

The laser accepts device status 11 and rejects timeouts, I2C errors and invalid
range statuses. The existing Pololu library does not implement the full ST host
sigma checks, so device-level validity is not a guarantee of absolute accuracy.
Keep target/lighting notes with the data and examine failures near 2 m.
