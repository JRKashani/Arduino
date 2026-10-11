On 2026-10-11 the user requested applying the new six-face calibration and
repeating the five-slope mission. The selected candidate comes from
`imu_tilt_sessions/20261010T230920_901007Z_six_face/calibration-result.json`.
The saved result is `candidate_passed`: maximum training vector error 0.08044 g,
maximum final validation vector error 0.09552 g, with 12 validation retest rounds.
All failures and replaced measurements remain in the source logs. The final pass
includes retained measurements and replacements; it is not a fresh six-position
validation collected in one round. The five-slope run is a further experiment.

| Parameter | Previous | Selected candidate (firmware decimal) |
| --- | --- | --- |
| X zero | 0.5143 | 0.50290918 |
| Y zero | 0.5070 | 0.49537615 |
| Z zero | 0.5068 | 0.50094098 |
| X ratio/g | 0.1050 | 0.10327006 |
| Y ratio/g | 0.1020 | 0.10375069 |
| Z ratio/g | 0.0988 | 0.09805550 |

These constants are in `src/DEFINE.h`, shared by the normal robot firmware and
the independent `imu_tilt` build. The shared ADXL335 library was not modified.
Only the stationary `imu_tilt` firmware is uploaded for this experiment. It keeps
motors off and applies no startup flat zero. Every new recorder boot logs the
actual firmware constants, preserving the distinction from previous sessions.

Start a new session rather than resuming a session recorded with old constants:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7 --surfaces 5 --settle 5 --duration 10
```

Allow 60 seconds after connecting before taking the initial flat capture. Use the
same marked flat position and robot direction for start, intermediate and final
checks. Take intermediate flat checks when prompted. Each slope has eight robot
placements: 0/90/180/270 degrees, twice. Phone readings are entered at the explicit
phone prompts, as two signed numbers per orientation; placement prompts take
Enter only. Different household slopes need not have exact preset angles.
