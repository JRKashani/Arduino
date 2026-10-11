The replacement ADXL335 is the robot's current sensor for future work, as confirmed
on 2026-10-11. Keep its calibration paired with this sensor and mount; replacing
either requires a new calibration record. Wiring: X to A2, Y to A1, Z to A0,
VCC to Arduino 3.3 V with A5 monitoring the same rail, and GND directly to Arduino
GND. See the [wiring diagram](diagrams/imu-wiring.svg).

The user requested using the replacement sensor's six-face calibration immediately,
skipping six-face validation and assessing it using five household slopes with a
phone as an external estimator. Accuracy is secondary to completing data collection.

The selected constants are computed from the latest accepted round-1 poses in
`imu_tilt_sessions/20261011T085956_819855Z_six_face`. The final Z-down retake passed
the existing training check (maximum vector residual 0.09319 g). Training passing
is not independent validation. No second calibration round is required for this
user-authorized provisional run. Earlier failures and the board reset remain in
the original logs; no source measurements were overwritten.

Full constants and previous values are in `imu-calibration-replacement-2026-10-11.json`.
Firmware uses rounded decimal values in `src/DEFINE.h`, shared by normal and tilt
builds. Only stationary `imu_tilt` firmware is uploaded. The five-slope launcher
embeds the provenance in each new session and checks it against the board constants.

Run `tools/imu_tilt/run-five-slopes.ps1` from PowerShell, or:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7 --surfaces 5 --settle 5 --duration 10 --calibration-info docs\imu-calibration-replacement-2026-10-11.json
```

This starts a new session for the replacement sensor. Keep the direct ground
connection, USB connection, switch positions and phone app zero fixed throughout.
Wait 60 seconds after connection before the first flat capture. At position prompts
press Enter only; phone prompts take two signed readings. Rotate physically through
0/90/180/270 with screen orientation locked and use the same labeled readouts.
The main slope component should exchange axes approximately at 90 degrees.
Keep phone readings before/after each surface, repeat loops and intermediate flat
checks so repeatability and changing references can be assessed after collection.

Warnings do not block accepting measurements; raw data remain available even when
a placement is excluded from conservative calibration fitting. The phone remains
an estimator. Its signs and angle convention must be established before using
numeric reference errors or proposed fit constants. The analysis can preserve raw
data before those definitions are known. The 3-degree comparison target remains
an evaluation metric, not a prerequisite to collecting this experiment.

The completed [five-slope review](imu-results-2026-10-11/comparison-review.md)
and [measured-angle tables](imu-results-2026-10-11/tables/measured-tables.md)
are archived in this repository. All 40 planned placements were accepted; all
20 loop pairs agreed within 3 degrees per axis. The phone-derived fit remains an
unapplied proposal with inferred axis mapping and phone convention. The active
constants above remain unchanged. Full acquisition logs stay locally under
`imu_tilt_sessions/`; the archive contains derived measurements and source hashes.
