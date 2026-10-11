# Standalone stationary tilt mission — plan

Status: implemented as standalone firmware and computer recorder/analyzer. See
[running instructions](imu-tilt-mission.md). Active calibration constants are unchanged.

Confirmed experiment: five improvised household slopes, eight stationary robot
placements per slope, manually rotated by 90 degrees between placements. Four
phone placements per slope, each providing two signed readings. Exact nominal
angles are not required. Use actual measured references and identify surfaces
as S1–S5. Add a separate level reference before and after the session.

1. **Purpose and deliverables**

   Build a standalone `ImuTiltMission` module, with its own PlatformIO firmware
   environment and a small computer-side serial recorder/analyzer. Reuse the
   existing `ADXL335_IMU` library and pin/calibration definitions. The dedicated
   entry point initializes the motors stopped and runs only this experiment.
   It does not initialize the lasers or call the navigation/motion-test loop.

   Deliver an immutable sample log, phone reference log, per-trial summary,
   comparison plots, and a proposed calibration report. Calibration proposals
   remain separate from the active values in `src/DEFINE.h`.

2. **Prepare the fixture and coordinate marks**

   Use a rigid, supported board, wide enough for all wheels/caster at all four
   headings. Avoid flexible cardboard, soft cushions, or any support that changes
   under the robot's weight. Mark one center and four perpendicular heading
   lines on the board. Keep the IMU above approximately the same center each time.

   Mark board direction U approximately uphill and direction R 90 degrees
   clockwise from U when looking down at the board. Headings are measured in the
   board plane, not as compass bearings. Mark the robot nose and phone top edge.
   Exact uphill alignment is unnecessary: both phone components capture residual
   cross-slope. Keep U/R unchanged for each surface's complete measurement block.

   Secure the IMU mount and wires. Support the tether so it does not pull, lift,
   or twist the chassis. If a restraint is needed to stop sliding, it must not
   change wheel contact or chassis attitude. Keep motors off throughout.

   The reference surface is the wheel-contact plane. Robot chassis pitch caused
   by wheel/caster geometry is an installed-system offset, not necessarily an
   accelerometer fault. Record the placement and use a flat check to characterize
   it. A reference pad parallel to the intended chassis datum is useful if one
   can be established without disturbing the robot.

3. **Measure each phone reference**

   Record Samsung S21 FE, the exact Bubble Level app publisher/version, displayed
   units, phone support/case arrangement, and any app zero/calibration setting.
   Several apps share that name; do not assume their angle definitions match.
   Lock display orientation if possible and verify which physical phone edge
   each number describes. Use degrees, preserve signs, and do not re-zero on
   each slope.

   Use a repeatable flat support that avoids camera-bump rocking. Measure at the
   robot's footprint on the same board. Ensure removing/adding the robot does
   not flex the board; check a nearby reference spot with the robot loaded if
   necessary.

   At phone orientations 0, 90, 180, 270 degrees clockwise from U, wait for
   settling and record both displayed numbers exactly, plus which edges are
   high if the display sign is ambiguous. Briefly repeat a reading after lifting
   and reseating the phone to assess placement sensitivity. A useful input form:

   | Surface | Phone orientation | Displayed up component (deg) | Displayed right component (deg) |
   | --- | --- | --- | --- |
   | S1 | 0 | | |
   | S1 | 90 | | |
   | S1 | 180 | | |
   | S1 | 270 | | |

   Preserve original entries. Before combining, verify signs by raising one
   marked edge, determine the app's angle convention, convert the pair to a
   gravity/normal vector in phone coordinates, then rotate into common board
   coordinates. Fit/average there and retain disagreement between readings.
   Do not average the eight signed numbers directly. If the app provides
   independent axis elevations instead of Euler roll/pitch, use that definition
   in the conversion; the formulas are not interchangeable.

   Opposite orientations help diagnose fixed phone bias, but four placements
   do not establish absolute accuracy. Carry reference spread and display
   resolution into the report. Recheck the four references after a block if
   the board shifts; preferably repeat them after every block. A changed setup
   gets a new reference/block ID rather than silently sharing the old one.

4. **Acquire 40 robot trials**

   Each slope has this sequence, with a separate capture at each position:

   | Trial | Repeat loop | Heading clockwise from U |
   | --- | --- | --- |
   | 1 | 1 | 0 degrees |
   | 2 | 1 | 90 degrees |
   | 3 | 1 | 180 degrees |
   | 4 | 1 | 270 degrees |
   | 5 | 2 | 0 degrees |
   | 6 | 2 | 90 degrees |
   | 7 | 2 | 180 degrees |
   | 8 | 2 | 270 degrees |

   This gives two independent placements at each heading. Manually lift/reseat
   or reposition between trials and use the marks rather than commanded motor
   turns. On a predominantly one-direction slope the opposite headings excite
   opposite pitch/roll signs. The heading labels themselves are operator inputs;
   an accelerometer cannot measure yaw about gravity.

   Start with a provisional 3-second settling interval and 10-second capture
   at 50 samples/second (about 500 samples per trial). Verify achievable timing
   in the dedicated firmware and record actual timestamps. These are experiment
   settings, not guaranteed accuracy specifications. Extend settling or repeat
   when a window contains motion, rocking, or drift. Preserve rejected attempts
   with a reason and a new attempt ID for the replacement.

   Take level checks before and after the session, preferably also between
   slopes. Preserve raw absolute readings. If a baseline is used, acquire it
   explicitly once on the reference surface, log its values, and keep it fixed.
   Do not automatically zero the device when starting on an unknown slope.
   After reset, create a new boot/session segment; never silently treat a newly
   zeroed inclined pose as level.

5. **Standalone program behavior and records**

   Proposed state flow: `IDLE -> POSITION -> SETTLE -> CAPTURE -> REVIEW`.
   In REVIEW the operator accepts, repeats, or flags the trial. Abort is available
   during settling/capture. Only explicit operator input advances positioning.
   Firmware streams samples; the computer stores full data and calculates
   statistics. The Mega needs only bounded serial buffers and running statistics.

   Proposed module interface: `begin()`, `startTrial(metadata)`, `update()`,
   `abort()`, and `status()`. The dedicated build selects a separate entry point;
   the normal robot build excludes it. Use a line-based metadata protocol rather
   than the existing single-character motion command parser.

   Save these linked records:

   - Session: schema/firmware version, session and boot IDs, timestamp, board and
     IMU identity, mounting notes, current six conversion constants, baseline
     angles and whether enabled, phone details, sampling/settling settings.
   - Phone reference: surface/reference ID, orientation, both original signed
     values, units, app zero state, before/after label, repeat/seating notes.
   - Trial: surface/reference ID, heading, loop number, trial/attempt ID, fixture
     notes, accepted/rejected status and reason, start/end times.
   - Sample: sequence number, elapsed microseconds, raw X/Y/Z ADC counts and
     raw A5 supply measurement, validity, converted ax/ay/az in g, magnitude,
     pre-baseline roll/pitch and existing robot roll/pitch.
   - Summary: valid/invalid counts, actual duration/rate, missing/late samples,
     mean, standard deviation, min/max and first-half versus second-half drift
     for ADC ratios and calculated outputs; matched reference and angle errors.

   A5 measures the accelerometer supply; it is not the ADC AREF connection.
   Preserve it because current conversion uses `rawAxis / raw3V3`.
   Firmware validity alone is insufficient: flag saturation, implausible supply,
   changing readings, and unusual gravity magnitude separately. Do not delete
   stable readings solely because the old calibration makes magnitude differ
   from 1 g. Choose numerical quality thresholds after a short pilot recording.

6. **Use the results for calibration**

   First verify sensor-to-robot axis mapping and signs with known nose-up and
   right-side-up placements. The current code sets robot pitch to negative
   baseline-corrected sensor pitch. Preserve that convention explicitly when
   comparing with phone references.

   Transform each board reference to the robot's recorded heading. Compare the
   measured gravity vector and angles with the expected values. Report current
   performance before proposing changes: mean angular error, RMS error, maximum
   error, within-trial noise, and disagreement between the two placements at
   each heading. Hundreds of samples in one placement are not hundreds of
   independent repetitions, and shared phone uncertainty does not average away.

   If identifiable from the data, fit normalized ADC offset/sensitivity and
   separately account for rigid sensor mounting orientation. Prefer the simplest
   model supported by the coverage. Check fit conditioning before proposing all
   six constants or a cross-axis matrix. Use loop 1 to fit and loop 2 to validate
   with coefficients frozen; this tests repeatability on the same reference
   surfaces, not independent absolute accuracy. An extra unfit slope is a better
   generalization check. Refit accepted data only after reporting validation.

   Near-level slopes give weak Z calibration coverage: for an ideally aligned
   unit, slopes from 0 to 30 degrees change vertical gravity only from 1 to
   about 0.866 g. Treat these trials primarily as installed tilt calibration and
   validation. If practical with a rigid fixture, add six stationary orientations
   placing each sensor axis at +1 g and -1 g to estimate axis offsets/scales more
   directly. Preserve the sensor mounting when doing this.

   Gravity-based inclination assumes no additional sustained acceleration;
   static captures suit that assumption. Offset and sensitivity correction
   should precede angle calculation. The six-orientation method is described in
   [Analog Devices AN-1057](https://www.analog.com/en/resources/app-notes/2020/07/22/16/49/an-1057.html).
   The ADXL335's supply-dependent offset and sensitivity are documented in its
   [datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL335.pdf).

7. **Report and implementation checks**

   Produce measured-versus-reference and residual plots for roll/pitch, gravity
   magnitude versus slope, and paired loop comparisons. Include phone-reference
   disagreement and flat-check drift. Report X/Y/Z ADC and g channels alongside
   angles so the assignment's axis readings are available. Z acceleration is
   not yaw; if three axis-inclination angles are required, explicitly define
   each angle relative to horizontal or vertical instead of inventing a heading.

   The supplied assignment asks for -30, -20, -10, -5, 0, 5, 10, 20, 30 degrees.
   Household slopes need not match these exactly. Report actual measurements;
   any values predicted at the requested grid are model estimates and must be
   labeled, with no extrapolation presented as a measured result. Opposite robot
   orientations supply both signs when the corresponding nonzero slope is tested.

   Before collecting the full dataset, build both firmware environments, check
   the serial recorder on one level and one tilted capture, and confirm motors
   stay stopped on startup, abort, malformed input, and reset. Test reference
   transforms against synthetic cardinal orientations and cross-slope cases;
   test recording/retry linkage and preservation of raw data. Validate sample
   timing and repeatability on hardware. Automated implementation checks and
   commands are documented in the running instructions.

   Suggested additions in priority order: repeatable phone seating; start/end
   flat checks; a repeat phone reference after each slope; broader slope coverage
   where the fixture is stable; one optional diagonal robot heading for a
   combined roll/pitch check; six-axis gravity calibration if practical.
