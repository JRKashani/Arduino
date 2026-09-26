# last_chance — Handoff Document

_Written 2026-09-26. Branch: `platformio-last-chance` on `https://github.com/JRKashani/Arduino`._

This document describes where the robot firmware stands, how to build/test it,
how Stage 3 (wall following) works and how to tune it, what is still untested,
and what to do next. Read sections 1–4 before touching the robot.

---

## 0. TL;DR

- `last_chance/` is now a **PlatformIO** project (not an Arduino-IDE sketch).
  Open **the `last_chance` folder** in VS Code with the PlatformIO extension.
- Working today on hardware: motors, all sensors (+ live telemetry), and
  **Stage 3: follow the right wall at 30 cm**, including 45° outside and inside
  corners. Stages 1, 2, 4, 5 are **not implemented**.
- The **last commit is untested on hardware** (anti-chatter, dead-zone
  compensation, 8 s search timeout). First job: test it (section 7.1).
- **90° corners have not been tested.** Outside 90° corners are the biggest
  known risk (section 8).
- Every tuning knob is in `src/DEFINE.h`. Change **one** value at a time,
  re-flash, read the log.

---

## 1. Project and competition context

Braude "Introduction to Mechatronic Systems" final project. The track has 5
segments, in order:

| # | Segment | Requirement |
|---|---|---|
| 1 | Open space | Drive from the start line to the center of the funnel entrance |
| 2 | Funnel | Travel along the funnel's center axis (equal distance to both walls) |
| 3 | Winding wall | One wall is cut off at the funnel exit; follow the remaining wall at **30 cm** |
| 4 | Wadi | Narrow path with sloped sides; gap = robot width + 1 cm; stay centered |
| 5 | Light | Drive to a light source (phone flash) and stop **12 cm** from the pole |

All distances are measured **from the robot center**. The track shape is not
fixed, so solutions must be general (do not tune to one practice layout).

Grade: **70% documentation** (electronic schematic, flowcharts — overview +
per-stage — and documented code, plus a presentation) and **30% performance**.
The schematic and flowcharts **do not exist yet** (section 9).

---

## 2. Hardware (as encoded in `src/DEFINE.h`)

Board: **Arduino Mega 2560**. No wheel encoders. No gyro (the IMU is an
accelerometer only — it measures tilt, not heading).

### 2.1 Pin map

| Subsystem | Signal | Pin |
|---|---|---|
| Left motor | DIR1 / DIR2 / PWM | 46 / 48 / 44 |
| Right motor | DIR1 / DIR2 / PWM | 49 / 47 / 45 |
| VL53L0X laser, right | XSHUT (I²C addr 0x30) | 22 |
| VL53L0X laser, left | XSHUT (I²C addr 0x31) | 24 |
| I²C bus (both lasers) | SDA / SCL | 20 / 21 |
| HC-SR04 front (nose) | TRIG / ECHO | 43 / 42 |
| HC-SR04 right-45° | TRIG / ECHO | 51 / 50 |
| LDR right / left | analog | A3 / A4 |
| ADXL335 accel | X / Y / Z / 3.3 V reference | A2 / A1 / A0 / A5 |
| LEDs | red / green / white | 8 / 9 / 12 (not used by code yet) |

### 2.2 Geometry (measured by the team; center = IMU position)

| Item | Forward of center | Sideways | Aim |
|---|---|---|---|
| Side lasers (L and R) | +2 cm | 1.2 cm toward each side | 90° out |
| Front ultrasonic | +10 cm | 0 | straight ahead |
| Right-45° ultrasonic | +8.5 cm | 5.5 cm right | ~45° forward-right |
| LDRs | at the nose | 3.2 cm each side | ±45°, no shrouding |

Robot 16.5 cm wide (wheels end to end), 19.5 cm long. Wheels 2.5 cm wide,
~1–1.5 cm ahead of center. Lasers and IMU are mounted high; walls in stages
1–3 are taller than the lasers. **Wadi side walls are only ~10 cm tall (40 cm
long at 15°) — the lasers cannot see them**; Stage 4 must use IMU roll.

### 2.3 Hardware lessons learned

- **A wire in front of a laser** caused phantom short readings earlier. Keep all
  beam paths clear.
- **IMU baseline moved between runs** (roll −3.2 / pitch 8.8 in one run,
  roll 5.0 / pitch 0.0 in another). If the robot was on the same flat spot both
  times, **the IMU mount may be loose**. Check before Stage 4.
- The front ultrasonic reports `--` (no echo) for anything farther than ~170 cm
  and sometimes for surfaces hit at a steep angle. This is normal.

---

## 3. Software setup

### 3.1 Tools

- VS Code + **PlatformIO IDE** extension (or PlatformIO Core CLI `pio`).
- Open folder: `JRKashani-Arduino/last_chance` (the folder with `platformio.ini`).
- Host unit tests need a C/C++ compiler on the PC. On Windows install
  **WinLibs GCC**: `winget install BrechtSanders.WinLibs.POSIX.UCRT`, then open a
  new terminal. Only needed for `pio test`, not for flashing the robot.

### 3.2 Everyday commands (run inside `last_chance/`)

| What | Command |
|---|---|
| Build | `pio run` |
| Build + upload to the Mega | `pio run -t upload` |
| Serial monitor (115200) | `pio device monitor` (quit with Ctrl+C) |
| Host unit tests (no board) | `pio test -e native` |

Close the serial monitor before uploading (they share the COM port).

### 3.3 Git basics

```bash
git pull                       # get latest
git status                     # what changed
git add last_chance            # stage changes
git commit -m "what and why"   # save a checkpoint
git push                       # upload to GitHub
git revert <commit-id>         # undo a commit safely (keeps history)
```

Commit only after a change was tested on the robot, and say in the message what
was tested.

### 3.4 Folder layout

```
last_chance/
├── platformio.ini          env:megaatmega2560 (robot) and env:native (host tests)
├── HANDOFF.md              this document
├── README.md               OLD (still describes the Arduino-IDE workflow) — needs updating
├── src/
│   ├── main.cpp            setup()/loop(): wires all capsules together
│   ├── DEFINE.h            ALL pins, calibration and tuning constants
│   ├── SensorsCapsule.*    lasers, ultrasonics, LDRs; filtering; telemetry
│   ├── ImuCapsule.*        ADXL335 tilt (roll/pitch) with flat-baseline calibration
│   ├── MotionCapsule.*     motors: timed moves (F/L/R) + driveContinuous()
│   ├── MotionTest.*        serial console (commands in section 4)
│   ├── StageManager.*      stage dispatch, safety stop, Stage 3 logic
│   └── Log.h               rate-limited logging macros
├── lib/Control/            pure math (PD, ramps, slew…) — unit-tested on the PC
└── test/test_native/       unit tests for lib/Control
```

Shared libraries (`Motor`, `SteeringDualH`, `VL53L0X`, `ADXL335_IMU`,
`Ultrasonic`) live in the repo's top-level `libraries/` folder and are found via
`lib_extra_dirs = ../libraries`.

### 3.5 Main loop order (`main.cpp`)

```
setup: Serial 115200 → motors stopped → sensors (laser addressing) → IMU flat calibration → menu
loop:  updateSensors() → runMotionTest() (serial console) → runStageManager() → runImuCapsule()
```

**Keep the robot flat and still at power-up/reset** — the IMU calibrates its
baseline then.

---

## 4. Serial console (115200 baud)

| Key | Action |
|---|---|
| `T` | Toggle live sensor telemetry (`SENS …` lines) |
| `3` | Start Stage 3 (right-wall follow) |
| `1`,`2`,`4`,`5` | Prints "not implemented yet" |
| `X` or `S` | Stop the running stage / stop motors |
| `F` | Timed forward 60 cm (test move) |
| `L` / `R` | Timed quarter circle left / right (test move; overshoots — known) |
| `H` / `?` | Menu |

Safety: while a stage runs, if the **front** distance ≤ 20 cm the robot stops
("SAFETY STOP") and resumes when it is ≥ 30 cm. Still keep a hand ready and
press `X`.

### 4.1 Telemetry line (`T`)

```
SENS Lwall=31.2cm Rwall=29.8cm (raw L/R mm=300/286) Front=85.0cm R45=36.1cm LDR L/R/diff=512/498/14
```

- `Lwall`/`Rwall`: distance from **robot center** to the wall (raw + 1.2 cm).
- `Front`: from **robot center** to the obstacle ahead (raw + 10 cm).
- `R45`: raw slant distance of the 45° sensor (no conversion). On a straight
  wall with the robot at 30 cm it reads ≈ 35 cm.
- `--` means invalid / no reading.

---

## 5. Sensor layer (`SensorsCapsule`)

- Both VL53L0X are powered up one at a time using XSHUT and moved to addresses
  0x30/0x31. If one fails, a message prints and that sensor reads invalid.
- 5-sample median filter on each distance. Ultrasonic calibration
  (`ULTRASONIC_DISTANCE_SCALE/OFFSET`) applied. Ultrasonics are pinged
  alternately every 60 ms.
- Laser reads block ~30 ms each, so the loop runs at ~12–16 Hz. Fine for now.

---

## 6. Stage 3 — how it works

Goal: keep the robot center 30 cm from the wall on its **right**. The team
decided to always follow the right wall.

### 6.1 Steering sign (important)

`driveContinuous(speed, steer)`: **positive steer = turn left = away from the
right wall; negative = toward it.** Verified on hardware (an earlier wrong sign
drove the robot into the wall). The log labels it `away` / `toward` / `hold`.

### 6.2 Control pipeline (every loop)

1. **Wall present?** Wall is "lost" when the right laser is invalid or beyond
   70 cm for 3 loops in a row → **SEARCH** (6.4).
2. **PD controller** on `error = rWall − 300 mm`:
   - P: `Kp × error` (ignored within ±5 mm).
   - D: `Kd × dErr`, where `dErr` is the approach rate (mm/s) computed every
     100 ms and smoothed. Negative `dErr` = closing on the wall. D acts like a
     heading sensor and is what stops the weaving.
   - `dErr` is capped at ±124 mm/s (the robot can't move sideways faster than
     it drives; bigger values are sensor artifacts at corners).
3. **Dead-zone compensation:** small steer values don't turn the robot
   (friction/caster), so ±4 is added while correcting distance.
4. **Inside-corner term** (`crn`): if the front sees something within 60 cm, or
   the right-45° within 28 cm, add away-steer (0 → 25, growing as it gets
   closer). While `crn > 0` the command is the **more-away** of PD and corner, so
   the D term can't cancel the turn.
5. **Clamp** to ±25, then **slew limit** (max 150 steer units per second) so
   steering can't flip ±25 instantly.
6. **Speed scaling:** speed 50 when straight, down to 30 at full steer (slower =
   tighter correction).

### 6.3 Reading the Stage 3 log

One line every 250 ms (per message type):

```
WALL3 rWall=32.6cm err=26mm dErr=13mm/s steer=-6 toward spd=45 front=65.8cm r45=30.8cm tgt=-6 pd=-6 crn=0
```

| Field | Meaning |
|---|---|
| `rWall`, `err` | distance to right wall from center; error vs 30 cm (+ = too far) |
| `dErr` | approach rate; − = closing on the wall |
| `steer` | command actually sent (after slew) |
| `tgt` | command before slew; if ≠ `steer`, the slew limiter is working |
| `pd` / `crn` | PD part / inside-corner part |
| `CLAMPED` | hit the ±25 limit |

Event lines (printed once): `wall lost (...) -> SEARCH right`,
`wall reacquired at ... -> FOLLOW`, `search timeout (end of wall?) -> STOP`,
`inside corner ahead ...`, `corner cleared -> PD only`, `SAFETY STOP: front ...`.

### 6.4 Wall lost / outside corner

When the wall is lost, the robot arcs right (speed 40, steer −12, ≈30 cm
radius, ≈18°/s) until the laser sees a wall ≤ 60 cm, then resumes following.
Lost at >70 cm but reacquired only at ≤60 cm (hysteresis stops flapping). After
8 s without a wall it stops (end of wall). A 45° outside corner doesn't trigger
this at all — the laser keeps seeing the new segment and PD handles it.

### 6.5 Tuning guide (all in `DEFINE.h`, Stage 3 section)

| Symptom in log / on floor | Knob | Direction |
|---|---|---|
| Weaves back and forth across 30 cm | `WALL_FOLLOW_DERIV_GAIN` (0.30) | up (e.g. 0.35) |
| Twitchy steering while distance is steady | `WALL_FOLLOW_DERIV_LPF_ALPHA` (0.4) or Kd | alpha down to 0.25 / Kd down |
| Slow to return to 30 cm | `WALL_FOLLOW_GAIN` (0.10) | up a little (0.11–0.12) |
| Parks a few cm off 30 cm, driving parallel | `STEER_DEADZONE_COMP` (4) | up to 5–6 |
| Wobbles right around 30 cm | `STEER_DEADZONE_COMP` | down |
| Turns too violently | `WALL_FOLLOW_MAX_STEER` (25) | down (18–20) |
| Can't recover from a steep inward start | `WALL_FOLLOW_MIN_SPEED` (30) | down to 25 (watch for stalling) |
| Inside corner: reacts too late | `CORNER_FRONT_START_CM` (60) | up (70–75) |
| Reacts to far objects on straights | `CORNER_FRONT_START_CM` | down |
| Outside corner: reacquires too far out | `WALL_SEARCH_STEER` (12) | up (tighter arc) |
| Outside corner: swings in too close | `WALL_SEARCH_STEER` | down |
| Log too noisy / too sparse | `STAGE_LOG_INTERVAL_MS` (250) | up / down |

Rule: tune only from physics or several different layouts, never to one
specific practice wall.

---

## 7. Test status

### 7.1 FIRST: test the untested last commit

The newest code commit `bff3bda` (anti-chatter + dead-zone + 8 s timeout) compiles and passes
all 18 host tests but has **not run on the robot**. Test on the 3×45 cm practice
wall (outside 45° corner, then inside 45° corner):

- At the outside corner, `steer` should no longer flip between +25 and −25 on
  consecutive lines.
- On straight segments it should sit closer to 30 cm (previously 26 or 33–34).
- If it's worse, revert just that commit: `git revert <id>` (see `git log`).

### 7.2 What has been verified on hardware

| Item | Result |
|---|---|
| All sensors + telemetry | OK |
| Stage 3 on a straight wall | converges without overshoot, holds 29.5–30.4 cm |
| Hard inward start | closest ~19 cm from center (~11 cm side clearance) |
| 45° outside corner | handled by PD; brief steering chatter at the corner point |
| 45° inside corner | closest 26 cm (was 14 cm before corner logic) |
| Wall end | `wall lost → SEARCH` works; hysteresis prevents flapping |

### 7.3 Not yet tested

- **90° outside corner** (biggest risk), **90° inside corner**.
- Curved walls; different corner orders; long straights.
- Anything related to Stages 1, 2, 4, 5 and stage transitions.

---

## 8. Known gaps and risks

1. **90° outside corners.** Relies on the search arc (radius ≈30 cm by
   calculation, not measured). The laser's ~25° field of view may catch the
   corner edge and flip follow/search a few times at the vertex. Test it; tune
   `WALL_SEARCH_STEER`.
2. **90° inside corners.** Geometry says there is room (turn radius at full
   steer ≈8–15 cm), but the gradual corner ramp may start the turn late at
   speed 50. Tune `CORNER_FRONT_START_CM`.
3. **Timed moves overshoot** (`L`/`R` quarter circles). There's no heading
   sensor, so they will never be precise. Stages must steer from sensors, not
   from timed turns.
4. **Stage transitions** (detecting "Stage 3 is over, now wadi") are not
   implemented. After the wall ends the robot currently searches, then stops.
5. **Stage 5 12 cm stop:** 12 cm from center means the nose ultrasonic is only
   ~2 cm from the pole — below its reliable minimum range, and a thin pole is a
   weak target. Bench-test the HC-SR04 at 2–5 cm; alternatives are stopping at a
   larger reading and coasting, or using LDR brightness.
6. **Motor library quirk:** `Motor`'s default constructor briefly sets pins
   7/8/9 as outputs at boot; 8 and 9 are the red/green LEDs. Handle when LEDs
   are used.
7. `README.md` still describes the old Arduino-IDE workflow — update it.
8. Laser reads block ~30 ms each; switch to non-blocking polling if a higher
   loop rate is ever needed.

---

## 9. Next steps (suggested order)

1. **Test the last commit** on the 45° wall (7.1). Commit a note of the result.
2. **Rebuild the practice wall with 90° corners** (outside and inside, also
   swap their order). Tune with the knobs in 6.5 — the **same values must work
   on all layouts**.
3. **Stage 4 — wadi.** Plan: steer on **IMU roll**. If the robot drifts off
   center, one wheel climbs a slope and the body rolls; the roll sign says which
   way. Use roll ≈ 0 as the target (PD, small deadband), drive slowly (high IMU
   = motion noise), keep pitch separate (pitch = climbing). Existing constants:
   `TILT_TOLERANCE_DEG`, `ROLL_CORRECTION_GAIN`, `STAGE4_CLIMB_SPEED` (60 is
   probably too fast). Needed first: wadi dimensions, how the robot enters it,
   and an IMU mount check (2.3).
4. **Stage 2 — funnel.** Same PD controller; error = `Lwall − Rwall` (the laser
   offsets cancel), target 0.
5. **Stage 5 — light.** Two fixed LDRs, steer on `ldrDiff`; stop with the front
   ultrasonic (see risk 5). Room lights off + phone flash = good conditions.
6. **Stage 1 — open area** to the funnel entrance.
7. **Stage manager transitions** 1→2→3→4→5 and one "run whole track" command.
8. **Documentation (70% of the grade):** electronic schematic (use the pin
   map in 2.1), overview flowchart + one flowchart per stage (section 6.2 is
   the Stage 3 flowchart in words), code comments, presentation.

---

## 10. Decisions already made (don't re-litigate without new data)

| Decision | Why |
|---|---|
| Closed-loop steering from sensors, not timed moves | no encoders or gyro; timed turns drift |
| Always follow the **right** wall in Stage 3 | team choice; simpler |
| Front HC-SR04 for the 12 cm light stop | already wired (but see risk 5) |
| Keep lasers mounted high | walls 1–3 are taller than the lasers; the wadi uses the IMU instead |
| No servo "home LIDAR" as main sensing | ~3–4 s per scan, ~25° beam; at most a one-shot scan for Stage 1 |
| No servo scan for light | LDRs are slow; two fixed LDRs are enough |
| No integral term (for now) | the offset was a steering dead zone, now compensated directly |
| PlatformIO instead of Arduino IDE | allowed by the teacher; enables host unit tests |

---

## 11. Commit history on this branch

```
bff3bda  Stage 3: anti-chatter, dead-zone compensation, 8 s search timeout — UNTESTED ON HARDWARE
6339587  Stage 3: inside-corner anticipation; fix toward-steer bug; search hysteresis
7ad8d6c  Stage 3: handle a lost wall (outside corner / wall end) with a search arc
ed5de6b  Stage 3: slow down while steering hard (speed scheduling)
c2bfcae  Stage 3: PD wall following, per-site rate-limited logging, tuned gains
02923fa  Fix Stage 3 wall-follow steering sign (later reverted in c2bfcae)
d84ee4e  Add closed-loop control infrastructure and Stage 3 wall following
446987e  Add unified sensor capsule with live telemetry to last_chance
93d9002  Convert last_chance to a PlatformIO project
```

---

## 12. Working tips

- Charge the batteries before every test session; low battery changes motor
  behavior (and every calibration).
- Test one change at a time; paste the serial log (20–60 lines) when asking for
  help. The logs show exactly why the robot steered each way.
- Start each test from a known position (e.g. ~30 cm from the wall, parallel),
  then deliberately try bad starts (too close, angled in, angled out).
- If the robot does something strange, check the sensors first with `T`
  (obstructions, loose wires) before changing gains.
