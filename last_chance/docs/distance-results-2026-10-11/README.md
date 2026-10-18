# Distance-sensor results - 2026-10-11

Archived real measurements for the front HC-SR04, right VL53L0X and Sharp GP2Y0A21YK0F on A6. Includes completed runs, interrupted/setup attempts, all raw DATA samples, rejected attempts, live serial output and generated Excel/CSV/plot reports. No simulated data is included.

Each session retains its original directory structure. `session.json` records settings; append-only `events-*.jsonl` are the source measurement records. `serial-*.log` includes live preview output in addition to explicit captures. Only explicit DATA samples count in tables. Analysis CSVs retain inclusion flags; failed, rejected and incomplete data must not be silently combined with accepted measurements. Plot caches are omitted.

| Session | Sensor | Last event | Raw DATA samples | Included samples | Completed positions |
|---|---|---|---:|---:|---:|
| [20261011T134101_190940Z_us](20261011T134101_190940Z_us/analysis/20261011T141951_366159Z/report.md) | us | interrupted | 20 | 10 | 0 |
| [20261011T142003_780535Z_us](20261011T142003_780535Z_us/analysis/20261011T143903_532281Z/report.md) | us | run_complete | 590 | 570 | 19 |
| [20261011T144802_531804Z_laser](20261011T144802_531804Z_laser/analysis/20261011T150552_051989Z/report.md) | laser | run_complete | 480 | 480 | 16 |
| [20261011T152053_543501Z_laser](20261011T152053_543501Z_laser/analysis/20261011T152435_927208Z/report.md) | laser | run_complete | 120 | 120 | 4 |
| [20261011T152446_019469Z_ir](20261011T152446_019469Z_ir/analysis/20261011T153412_479901Z/report.md) | ir | run_complete | 450 | 450 | 15 |

The retained sessions in this checkout contain **1,660 raw DATA samples**. The original archive snapshot contained **1,720 samples across nine sessions**; its `manifest-sha256.json` is preserved unchanged. Four session directories listed in that snapshot are absent from this checkout:

- `20261011T143949_487903Z_us`
- `20261011T144040_475227Z_laser`
- `20261011T150603_410906Z_ir`
- `20261011T151242_664623Z_ir`

Of these, `20261011T151242_664623Z_ir` contained 60 samples at 170/180 cm, outside the IR specification; the others had no DATA samples. All 63 manifest entries still present were verified against their SHA-256 hashes during the 2026-10-17 documentation update. The original manifest therefore cannot certify a complete checkout. It does not cover later presentation figures, which have separate `sources-sha256.json` files. Raw records and the original manifest have not been modified.

## Findings and limitations

- [Ultrasonic review](20261011T142003_780535Z_us/analysis/20261011T143903_532281Z/review.md): 19 distances, 570 included samples, existing calibration mean absolute error 0.51 cm against entered distances. Two rejected captures remain archived.
- [Laser alignment note](20261011T144802_531804Z_laser/setup-note.md): mounting was straightened around 60 cm; exact transition is uncertain. The subsequent separate session repeats 50, 60, 70 and 91 cm.
- [IR and laser follow-up review](20261011T152446_019469Z_ir/analysis/20261011T153412_479901Z/review.md): the three matching laser positions agree within 0.29 cm. IR has 450 samples over 10-80 cm, but output becomes ambiguous at farther distances, so no ADC-to-distance calibration was applied.
- Early/incomplete sessions are retained for provenance and do not establish successful coverage. Each session records its own firmware profile and ADC reference. Do not merge different profiles or mounting conditions without accounting for them.

## Presentation graphs and wiring

- [Combined graph with experimental laser correction](comparison-calibrated-laser/README.md): IR ADC on the left, US and laser cm on the right. The fitted laser correction is graph-only and is not active in firmware.
- [Original combined graph](comparison/README.md): laser without the empirical correction.
- [Three raw-output figures](raw-sensor-graphs/README.md), also available as [one PDF](raw-sensor-graphs/raw-sensor-graphs.pdf): ultrasonic echo time with calculated cm on the second y-axis, laser mm and IR ADC counts.
- [One-Mega wiring diagram](../diagrams/distance-sensors-wiring.png), with [SVG](../diagrams/distance-sensors-wiring.svg) and [PDF](../diagrams/distance-sensors-wiring.pdf).

The principal curves use 1,500 accepted samples across 50 positions (19 US, 16 laser, 15 IR). Points average three accepted repeat means. Vertical bars span individual sample minima/maxima, and horizontal bars are **±1 cm** (2 cm total width). These differ from the original assignment curves' min/mean/max across repeat means. Separate laser follow-up measurements are not merged into the principal curve; the mounting adjustment around 60 cm remains a limitation.

## Recreate analysis

From the `last_chance` folder, with the dependencies in `tools/distance_test/requirements.txt` installed:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\distance_mission.py analyze "docs/distance-results-2026-10-11/20261011T152446_019469Z_ir"
```

This adds a new timestamped analysis directory and preserves source records. [Experiment instructions](../distance-test.md).

To reproduce the presentation figures and wiring, run:

```powershell
& .\.venv\Scripts\python.exe tools\distance_test\plot_comparison.py --calibrated-laser
& .\.venv\Scripts\python.exe tools\distance_test\plot_raw_sensors.py
& .\.venv\Scripts\python.exe docs\diagrams\generate_distance_wiring.py
```
