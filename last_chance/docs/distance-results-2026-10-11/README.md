# Distance-sensor results - 2026-10-11

Archived real measurements for the front HC-SR04, right VL53L0X and Sharp GP2Y0A21YK0F on A6. Includes completed runs, interrupted/setup attempts, all raw DATA samples, rejected attempts, live serial output and generated Excel/CSV/plot reports. No simulated data is included.

Each session retains its original directory structure. `session.json` records settings; append-only `events-*.jsonl` are the source measurement records. `serial-*.log` includes live preview output in addition to explicit captures. Only explicit DATA samples count in tables. Analysis CSVs retain inclusion flags; failed, rejected and incomplete data must not be silently combined with accepted measurements. Plot caches are omitted.

| Session | Sensor | Last event | Raw DATA samples | Included samples | Completed positions |
|---|---|---|---:|---:|---:|
| [20261011T134101_190940Z_us](20261011T134101_190940Z_us/analysis/20261011T141951_366159Z/report.md) | us | interrupted | 20 | 10 | 0 |
| [20261011T142003_780535Z_us](20261011T142003_780535Z_us/analysis/20261011T143903_532281Z/report.md) | us | run_complete | 590 | 570 | 19 |
| [20261011T143949_487903Z_us](20261011T143949_487903Z_us/session.json) | us | interrupted | 0 | 0 | 0 |
| [20261011T144040_475227Z_laser](20261011T144040_475227Z_laser/session.json) | laser | interrupted | 0 | 0 | 0 |
| [20261011T144802_531804Z_laser](20261011T144802_531804Z_laser/analysis/20261011T150552_051989Z/report.md) | laser | run_complete | 480 | 480 | 16 |
| [20261011T150603_410906Z_ir](20261011T150603_410906Z_ir/session.json) | ir | error | 0 | 0 | 0 |
| [20261011T151242_664623Z_ir](20261011T151242_664623Z_ir/analysis/20261011T151449_507560Z/report.md) | ir | interrupted | 60 | 60 | 2 |
| [20261011T152053_543501Z_laser](20261011T152053_543501Z_laser/analysis/20261011T152435_927208Z/report.md) | laser | run_complete | 120 | 120 | 4 |
| [20261011T152446_019469Z_ir](20261011T152446_019469Z_ir/analysis/20261011T153412_479901Z/report.md) | ir | run_complete | 450 | 450 | 15 |

Total archived raw DATA samples: **1720**. `manifest-sha256.json` lists SHA-256 hashes of every copied file; all copies were verified byte-for-byte against the original sessions. Archive files are excluded from Git text normalization to preserve these hashes.

## Findings and limitations

- [Ultrasonic review](20261011T142003_780535Z_us/analysis/20261011T143903_532281Z/review.md): 19 distances, 570 included samples, existing calibration mean absolute error 0.51 cm against entered distances. Two rejected captures remain archived.
- [Laser alignment note](20261011T144802_531804Z_laser/setup-note.md): mounting was straightened around 60 cm; exact transition is uncertain. The subsequent separate session repeats 50, 60, 70 and 91 cm.
- [IR and laser follow-up review](20261011T152446_019469Z_ir/analysis/20261011T153412_479901Z/review.md): the three matching laser positions agree within 0.29 cm. IR has 450 samples over 10-80 cm, but output becomes ambiguous at farther distances, so no ADC-to-distance calibration was applied.
- Early/incomplete sessions are retained for provenance and do not establish successful coverage. Each session records its own firmware profile and ADC reference. Do not merge different profiles or mounting conditions without accounting for them.

## Recreate analysis

From the `last_chance` folder, with the dependencies in `tools/distance_test/requirements.txt` installed:

```powershell
.\.venv\Scripts\python.exe tools\distance_test\distance_mission.py analyze docs\distance-results-2026-10-1161011T152446_019469Z_ir
```

This adds a new timestamped analysis directory and preserves source records. [Experiment instructions](../distance-test.md).
