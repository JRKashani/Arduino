# Running the stationary IMU experiment

The standalone Mega program and computer recorder implement the
[experiment plan](imu-tilt-mission-plan.md). Five arbitrary slopes are labeled
S1–S5; no exact nominal angle is required. The recorder guides eight stationary
placements per slope and separate start/end flat checks. Default timing is
3 seconds settling, then 10 seconds recording at 50 Hz.

The dedicated firmware holds both motor PWM and all four direction outputs low.
It uses only the ADXL335 and serial console. Startup does not zero the IMU;
baseline is explicitly `NONE`. Raw ADC counts and supply readings are preserved
even when the calculated acceleration is invalid.

## Quick start on this computer

Run these commands in PowerShell from the `last_chance` folder. Dependencies
are already installed in this project's `.venv`. The Mega was detected on
COM7 during preparation; check again if it changes. Close other serial monitors.

Upload the standalone firmware:

```powershell
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e imu_tilt -t upload --upload-port COM7
```

List ports if necessary, then start the guided experiment:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py ports
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7
```

The recorder prints the session directory and creates it under
`imu_tilt_sessions/`. Keep that directory. You enter the phone's two signed
numbers at each of its four orientations, confirm the entered values, then
position the robot as prompted. Each trial ends with accept, repeat, or skip.
Rejected and aborted attempts remain in the source logs. Optional phone checks
after each surface and optional flat checks between slopes are prompted.

When you choose repeat (`r`), the recorder asks:

```text
Keep previous capture(s) together with the new one [k], or discard previous capture(s) [d]?
```

`k` retains valid completed captures in the current placement's repeat group.
After accepting the final capture, analysis combines their means with equal
weight per capture and gives that placement one calibration weight. Different
planned loops remain separate repetitions. `d` excludes all earlier captures
in that repeat group, including captures you previously kept. Their raw logs
remain available. Skipping the placement excludes its repeat group. An aborted
or empty capture cannot be retained. An interrupted group awaiting a final
accepted repeat is marked pending and excluded from combined results; resume
takes a fresh reference for the unfinished surface and starts a new group.

The recorder also compares flat checks. A change over 3 degrees in roll/pitch
or 0.1 g in calculated magnitude produces a diagnostic warning. These checks
assume the SAME flat surface and robot direction with unchanged calibration.
Absolute angles remain visible, with no automatic zero correction. A large,
constant mounting offset and a signal that changes between checks need different
investigations. Do not conceal a changing signal by repeatedly re-zeroing it.

At each new slope, mark board direction U approximately uphill and direction R
90 degrees clockwise from U. Phone 0 means its top edge faces U; robot 0 means
its nose faces U. Both rotate clockwise **in the board plane**, viewed from
above. Keep the center position, wheel contact, support and tether repeatable.
At 0 degrees the robot faces roughly uphill, at 90 across the slope, at 180
downhill, and at 270 across the other way. The robot is always stationary when
recording. Rotate manually; the program never commands movement.

Use signed **degrees**, not percent grade. Preserve each displayed phone number
exactly; use the same phone seating and app zero throughout. Record the publisher
and version because multiple apps are called Bubble Level. The program does not
assume the phone's display convention during acquisition.

Press Ctrl+C during a capture to abort that attempt and keep its partial data.
Then repeat or skip it. Ctrl+C at a prompt ends the recorder. To resume later:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7 --resume "imu_tilt_sessions\YOUR_SESSION"
```

Resume skips already accepted slope placements. It takes fresh flat checks and
fresh phone readings for any unfinished surface, so changed supports do not
silently inherit an old reference. Every recorder connection has a separate boot
segment. A reset/disconnection during a capture stops the recorder with the data
retained; use resume after reconnecting. Never reuse a session for a different
robot or a changed IMU mount: start a new session and document the change.

If a one-surface pilot should become the full experiment, extend its saved plan
to five surfaces while resuming:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7 --resume "imu_tilt_sessions\YOUR_SESSION" --extend-surfaces 5
```

This retains accepted S1 placements and adds S2–S5. The extension is appended to
the session log, and future resumes/reports remember it. `--surfaces` controls
new sessions; `--extend-surfaces` controls an existing session's total count.

For a shorter pilot, use one surface and shorter captures:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py record --port COM7 --surfaces 1 --duration 3
```

`--settle`, `--duration`, and `--hz` are configurable. The firmware accepts
0–60 seconds settling, 1–60 seconds capture, and 1–50 Hz. `--diagonal` adds one
45-degree placement per surface. Start a new five-surface session after the pilot.

## Reports and phone conventions

Generate noise, repeatability, flat-drift reports and plots immediately, before
the phone/robot axis conventions are verified:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py analyze "imu_tilt_sessions\YOUR_SESSION"
```

For a comparison and candidate calibration, first verify the display definitions
and physical signs. Raising the phone's top edge should correspond to positive
up after applying `--up-sign`; raising its right edge should correspond to positive
right after applying `--right-sign`. Both options accept `1` or `-1`. Lock screen
orientation if possible. Check the app documentation or its publisher for which
of these definitions it uses; if unknown, retain the default `unverified`:

| Option | Definition of the two signed phone values |
| --- | --- |
| `axis-elevation` | Separate top/right axis elevations: gx=sin(up), gy=sin(right), gz=positive sqrt(1-gx²-gy²) |
| `euler` | Up is pitch and right is roll: gx=sin(up), gy=cos(up)sin(right), gz=cos(up)cos(right) |
| `gradient` | Two projected angles: up=atan(gx/gz), right=atan(gy/gz) |
| `unverified` | Keep original numbers; do not convert them or fit calibration |

The analyzer rotates each phone vector into board coordinates, combines the
four orientations, and reports their disagreement. It also checks changes
between before/after reference blocks. Do not average the displayed signed
angles directly.

Verify sensor axes on the robot with known nose-up and right-side-up poses.
`--sensor-map` lists sensor axes for robot nose/right/up. The default is `x,y,z`.
For example, if sensor Y becomes negative when the robot's right side is raised,
use `--sensor-map "x,-y,z"`, **after physically checking it**. Existing firmware
robot roll is atan2(ay,az); robot pitch is atan2(ax,sqrt(ay²+az²)). These software
labels alone do not prove the physical mounting signs.

Once checked, an example invocation is:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py analyze "imu_tilt_sessions\YOUR_SESSION" --phone-convention axis-elevation --axes-verified --sensor-map "x,y,z"
```

Change that example to the **verified** convention/map/signs. Explicit mounting
corrections are available as `--mount-roll`, `--mount-pitch`, `--mount-yaw` in
degrees: body vector = Rz(yaw) Ry(pitch) Rx(roll) times the signed mapped sensor
vector. Default corrections are zero. The analyzer does not infer yaw or a
mounting rotation from a flat capture. Chassis geometry or unknown mounting
offsets can be absorbed into fitted offsets; any proposed constants describe
the installed system under those assumptions.

Each analysis creates a new timestamped directory without changing source data:

- `samples.csv`: every sample, including rejected/aborted attempts, linked to
  surface, heading, reference and boot.
- `trials.csv`: validity, ADC/g/angle and ratio statistics, actual rate, missing
  samples, first/second-half drift, reference errors and quality flags.
- `placements.csv`: combined results for accepted placements, retained attempt
  IDs, capture counts and repeat disagreement. Each source capture stays in
  `trials.csv`; pending/rejected captures are explicitly excluded.
- `phone-readings.csv`, `references.csv`: original phone entries (including
  retries), selected-reference labels and converted reference disagreement.
- `repeatability.csv`: paired loop differences on the same reference block.
- `report.txt`, `report.json`: coverage, level drift, limitations, source hashes,
  analysis options and fitting/validation results.
- `calibration-proposal.json`: identifiable axis constants and held-out results;
  no active robot constants are changed.
- `measurements.png/.svg`, `repeatability.png/.svg`: measured/reference angles,
  residuals, magnitude, phone disagreement and repeatability.
- `flat-diagnostics.json`, `flat-checks.png/.svg`: successive same-pose flat
  comparisons, axis/reference ADC changes and the flat-check timeline.

Fitting uses accepted loop-1 placements only, with equal weight per placement.
Loop 2 checks the frozen proposal. References with more than 3 degrees disagreement
or before/after movement, angle std/drift above 3 degrees, ADC saturation,
incomplete captures or fewer than ten valid samples are excluded from fitting.
All data remains in the reports. These are exploratory quality limits;
`--max-std`, `--max-drift`, `--max-reference-disagreement` make them explicit and
are saved with each analysis.

Retained captures that disagree by more than 3 degrees or 0.1 g are flagged and
excluded from fitting as a group. Slope captures between two substantially
changed flat checks are also flagged and excluded from calibration fitting until
the cause is investigated; their accepted history and raw logs remain intact.
This comparison uses consecutive flat checks so an intervening large change
cannot disappear behind a small first-to-last difference.

Each axis needs at least 0.2 g reference span and a usable fit condition by
default. Weakly excited axes keep their existing constants. In particular,
0–30 degree slopes give only about 0.134 g vertical range, so Z may remain
unchanged. `--minimum-span` is an explicit override, not additional evidence
of calibration quality. A candidate must improve both RMS and maximum gravity
direction error on at least four validation placements to pass the improvement
check. Treat it as provisional and verify on another slope before adoption.

Absolute phone accuracy is unknown; four orientations measure consistency.
Loop 2 often shares phone references with loop 1. Reports do not claim that
repeatability establishes absolute accuracy or fill the assignment's exact angle
grid with invented measurements.

## Installation, demonstration, and verification

For another computer with Python 3.9+ and PlatformIO, create the environment:

```powershell
python -m venv .venv
& .\.venv\Scripts\python.exe -m pip install -r tools\imu_tilt\requirements.txt
```

The core analyzer requires only the Python standard library. `--no-plots` writes
CSV/JSON without matplotlib. Recording requires pyserial. Full requirements
install both the recorder and plotting dependencies.

To try the full pipeline without hardware, choose a new demo directory:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py demo --output build\my-tilt-demo
& .\.venv\Scripts\python.exe tools\imu_tilt\tilt_mission.py analyze build\my-tilt-demo --phone-convention axis-elevation --axes-verified
```

Demo data is explicitly marked simulated and cannot be resumed as a real session.
The demo includes 40 accepted slope placements, one rejected attempt and two flat
checks. All source records are appended/flushed in `events-BOOT.jsonl`, with a
separate exact serial text log per connection. Analysis never edits these files.

Automated checks:

```powershell
& .\.venv\Scripts\python.exe -m unittest discover -s tools\imu_tilt -p 'test_*.py' -v
& tools\imu_tilt\firmware_tests\run-msvc.cmd
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e imu_tilt
& "$env:USERPROFILE\.platformio\penv\Scripts\pio.exe" run -e megaatmega2560
```

The firmware tests compile the actual mission and shared ADXL335 implementation
against a fake Arduino. The MSVC runner uses the installed Visual Studio 2022
Build Tools; another host compiler can compile those same three C++ files with
the mock/library/mission include directories. Keep `unsigned long` at 32 bits
for the micros-wrap simulation (as on AVR and MSVC).

The normal robot firmware remains the default PlatformIO environment. To restore
it after the experiment, close the recorder and upload `megaatmega2560` instead
of `imu_tilt`.

Builds, simulated firmware scenarios and computer-side tests have been run.
Actual serial throughput, phone seating, motor behavior and repeatability still
need the short hardware pilot before collecting all five surfaces.
# Three-degree experiment target

For exploratory comparisons before physical axis verification, use
`--axes-inferred` with an explicit phone convention and sensor map. This labels
the assumptions as inferred, permits conditional comparisons and provisional
offset/scale fitting, and never recommends applying a candidate. It does not
change active calibration. `--axes-verified` retains its original meaning of
physical verification; the options are mutually exclusive.

The requested target is now +/-3 degrees on each of roll and pitch. Recorder
angle summaries display whole degrees; raw data and offline calculations retain
their original precision. Default capture variability, drift, flat-change and
repeat-disagreement angular limits are 3 degrees. The analyzer defaults phone
reference disagreement to 3 degrees too; its quality limits can still be overridden
with CLI options. Magnitude and ADC checks remain separate from angle tolerance.

For verified phone references, `placements.csv` includes `within_3deg_per_axis`
and the report counts how many placements meet the target. Unverified references
remain unassessed. These comparisons do not guarantee absolute accuracy, and
standard deviation is a variability measure rather than a maximum-error bound.
A stable 4-degree flat offset still needs an independently justified mounting
correction if corrected measurements are to meet the 3-degree target; the recorder
does not silently subtract it. Six-face calibration validation limits are unchanged.

If recording is already running, it keeps its loaded display/warning settings.
The collected raw data remain suitable for the updated offline analysis. To use
the new display during recording, stop between captures and resume that session.
