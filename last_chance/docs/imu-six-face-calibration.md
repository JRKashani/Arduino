Run from the project root with the existing `imu_tilt` firmware uploaded:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\calibrate.py --port COM7
```

Close other serial monitors first. Identify the sensor's positive X/Y/Z directions
from the module markings. Each axis is pointed vertically up and down, with the
other axes horizontal. Support the whole robot securely and keep the sensor mount
unchanged. Wait 60 seconds after connecting before the first capture. No phone
readings are required. The wizard takes six calibration captures followed by six
new validation captures, each with five seconds of settling and ten seconds of
recording. Accept with `a`, or repeat with `r`. Rejected attempts remain in the log.

The offset is the midpoint of each axis's positive/negative mean supply ratio;
the sensitivity is half their difference. This is a diagonal offset/scale model,
not a mounting alignment or cross-axis calibration. Its assumptions and procedure
follow [Analog Devices AN-1057](https://www.analog.com/en/resources/app-notes/2020/07/22/16/49/an-1057.html).

Raw sample and serial logs go in a new `imu_tilt_sessions/*_six_face` directory.
The wizard rejects invalid/saturated readings and captures with supply-ratio
standard deviation or half-window drift above 0.003. The six-pose training check
and independent validation each require vector error at most 0.1 g in every pose.
These are provisional experiment limits, not sensor accuracy specifications.
Old angle/magnitude estimates do not determine capture quality.

`calibration-result.json` records the candidate and validation. A passing candidate
also produces `candidate-constants.txt` for review. The wizard never edits
`src/DEFINE.h`, writes EEPROM, uploads firmware, or changes the active constants.
If interrupted, source logs remain; start a fresh calibration to obtain a complete
pair of rounds. Passing these checks still requires a separate slope/flat test to
confirm useful tilt accuracy.

If the training geometry check fails, the wizard prints all six corrected vectors
and flags positions whose error exceeds 0.1 g. Choose `r` to replace flagged
positions, `f` to retake all six, or `q` to stop. Every replacement is logged and
the complete training check is recomputed without relaxing its threshold.
`training-diagnostics.json` saves the latest check; the event log keeps all checks.

A completed six-pose training session can be reused after stopping:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\calibrate.py --port COM7 --retry-session "imu_tilt_sessions\YOUR_SESSION_six_face"
```

This creates a new session with links to the original captures and requires
unchanged active constants. Use it only with the same sensor/wiring/mount setup;
otherwise start fresh. All six validation captures must still be acquired anew.

Validation failures now offer `r` to replace only failed validation positions,
`f` to retake all six validation positions, or `q` to stop. The candidate remains
fixed throughout validation. Passing positions are retained when choosing `r`.
Every check and superseded attempt remains in the logs; the result records the
number of validation retest rounds. A pass after repeats is not a fresh complete
independent validation run; use `f` for that check, and investigate persistent
failures rather than repeatedly accepting a favorable measurement.

To retry validation from a completed session without repeating training:

```powershell
& .\.venv\Scripts\python.exe tools\imu_tilt\calibrate.py --port COM7 --retry-validation "imu_tilt_sessions\YOUR_SESSION_six_face"
```

This creates a new session preserving links to the original training and
validation captures. The saved candidate is reused without refitting. The source
must have passed training and contain six accepted validation positions. Active
constants must match the source, and the physical setup must be unchanged.
