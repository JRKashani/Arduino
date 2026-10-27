# Manual drive logger

Drive the robot through the track from the PC keyboard while the robot streams
every sensor update. Each sample is labelled with the run number and the track
segment you say you are in. Several runs then give the **expected sensor values
per segment**, which the stage logic can be built and checked against.

The firmware uses the same sensor layer as navigation (same filters, same
laser and ultrasonic calibration, same robot-center offsets). The recorded
values are therefore exactly what the stages will see.

## 1. Upload (once)

From the `last_chance` folder, with no serial monitor open:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e manual_drive -t upload --upload-port COM7
```

Re-upload the normal navigation firmware (`-e megaatmega2560`) when you are done.

## 2. Record

1. Charge the batteries. Place the robot **flat and still**: connecting resets
   the board and zeroes the IMU at that pose.
2. Start the recorder (PowerShell, `last_chance` folder):

   ```powershell
   & .\.venv\Scripts\python.exe -u tools\manual_drive\manual_drive.py record --port COM7 --note "track layout A"
   ```

3. Put the robot at the start line, press **1**, and drive.
   Press the segment number each time you **enter** a new segment.
4. At the end of a run press **Space**, carry the robot back, press **N** (new
   run), then **1** at the start line again. Repeat a few times.
5. Press **Q** to finish. Everything is already on disk.

| Key | Action |
|---|---|
| W / Up | faster (0 → 40, then +10, max 100) |
| S / Down | slower; below 0 reverses (−40, −50 …) |
| A / Left | steer more left (+5, max 60) |
| D / Right | steer more right (−5) |
| C | straighten steering |
| Space | **stop** (speed and steering 0) |
| 1 … 5 | segment you are in now (1 open area, 2 funnel, 3 wall, 4 wadi, 5 light); 0 = none |
| M | mark this moment (e.g. "corner", "robot touched wall") |
| N | new run (stops the robot first) |
| G | toggle the front safety stop |
| H / ? | help |
| Q / Esc | stop and finish |

Commands are latched: one W press keeps the speed until you change it. Speeds
between 1 and 39 are skipped because the motors stall there.

The status line shows the command and the live L / R / F / D distances (cm from
the robot center; D is the raw right-45 slant), roll and pitch.

### Safety

- **Watchdog:** the PC repeats the command every 0.15 s. If the robot hears
  nothing for 0.6 s (cable pulled, program closed) it stops by itself.
- **Front stop:** while ON (default), forward motion is held when the front
  reads ≤ 20 cm from the center (≈10 cm from the nose). Turning in place and
  reversing still work. Press G to turn it off, e.g. for the light pole.
- Keep a hand ready. Space always stops.
- A board reset mid-run (loose USB, power dip) is reported; the robot stops and
  the IMU is re-zeroed at whatever pose it was in, so note it.

## 3. Files

Sessions go to `manual_sessions/<UTC time>/` (ignored by git; copy a session
into `docs/` if it should be kept):

| File | Content |
|---|---|
| `samples.csv` | one row per sensor update: `run, segment, pc_s` + firmware fields |
| `events.jsonl` | keys, segment changes, marks, new runs, firmware events (safety, watchdog, reset) |
| `serial.log` | every line sent and received, with UTC time |
| `session.json` | port, note, firmware version, calibration constants, IMU baseline line |

Sample fields (all distances in cm; `*_ok` = 1 when valid):

| Field | Meaning |
|---|---|
| `t_ms` | robot clock |
| `speed`, `steer`, `blocked` | command actually applied (`blocked` = front stop holding forward) |
| `l_cm`, `r_cm` | side lasers, calibrated, robot center → wall (median of 5) |
| `l_raw_mm`, `r_raw_mm` | last raw laser reading, before calibration and filtering |
| `f_cm` | front ultrasonic, robot center → obstacle (median of 5) |
| `d_cm` | right-45 ultrasonic, raw slant from the sensor (median of 5) |
| `f_sample_cm`, `d_sample_cm` | latest calibrated single ping, sensor face |
| `us_new` | which ultrasonic was pinged in this update: `F`, `D` or `N` (none) |
| `ldr_l`, `ldr_r` | light sensors, ADC 0-1023 |
| `roll_deg`, `pitch_deg` | tilt relative to the pose at connection |
| `ax_g`, `ay_g`, `az_g` | absolute acceleration (g), independent of that zeroing |

## 4. Analyse

Combine any number of sessions:

```powershell
& .\.venv\Scripts\python.exe tools\manual_drive\manual_drive.py summarize manual_sessions\<s1> manual_sessions\<s2>
& .\.venv\Scripts\python.exe tools\manual_drive\manual_drive.py plot manual_sessions\<s1> manual_sessions\<s2>
```

- `summarize` prints and writes `by-segment.csv` (all runs pooled) and
  `by-run.csv`. Each sensor gets the percentage of valid readings, mean, std,
  min, 10th percentile, median, 90th percentile and max, using valid readings only.
  The 10th to 90th percentile band is the "expected" range; min and max show outliers.
- `plot` writes one PNG per run: distances, roll/pitch and the command over
  time, with the labelled segments shaded.

Output goes to `<first session>/analysis/<UTC time>/` unless `--output` is given.

## 5. Notes

- The robot is tethered by USB, so walk behind it with the laptop.
- About 30 samples per second. Each ultrasonic is pinged about every 120 ms; a
  sample with `us_new = N` repeats the previous ultrasonic values.
- Segment labels are only as accurate as the key presses. Press the number as
  the robot center crosses into the segment, and use M for anything unusual.
- The side-laser correction was measured on the right laser only (20-170 cm);
  the left laser is assumed to behave the same.

## Tests

```powershell
cd tools\manual_drive; ..\..\.venv\Scripts\python.exe -m unittest -q; cd ..\..
tools\manual_drive\firmware_tests\run-msvc.cmd
```

The second command tests the firmware command parser, safety stop and watchdog
on the PC (needs Visual Studio 2022 Build Tools).
