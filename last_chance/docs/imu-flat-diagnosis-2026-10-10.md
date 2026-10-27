# Flat-reference change found in the first real session

Source: `imu_tilt_sessions/20261010T155251_711723Z`, original sample and event logs.
The operator confirmed the last check used the same flat setup, with no known
wiring or mount change. No raw data or active calibration constants were edited.

The absolute angles use the existing ADXL335 conversion constants and no
baseline subtraction. A nonzero flat reading alone can reflect a mounting or
calibration offset. These records also show changing readings in the same pose.

| Accepted flat check | Trial | Roll (deg) | Pitch (deg) | Calculated magnitude (g) | Reference ADC |
| --- | --- | --- | --- | --- | --- |
| Initial LEVEL_START | 2 | 9.155 | 5.518 | 1.176 | 737.45 |
| LEVEL_END after S1 | 12 | 0.725 | -3.990 | 1.002 | 736.05 |
| LEVEL_START on resume | 13 | 0.659 | -3.990 | 1.003 | 736.07 |
| LEVEL_BETWEEN_S2 | 24 | 11.628 | 7.296 | 1.236 | 736.26 |

From trial 13 to trial 24, roll changed by about +10.97 degrees and pitch by
+11.29 degrees. Raw X/Y/Z means changed from approximately 373.17/374.06/445.80
to 390.80/391.85/460.50 ADC counts. The supply-reference reading barely changed.
This is visible in the raw analog readings as well as calculated angles.

The first change occurred during S1: early captures had magnitude near 1.17 g,
while the later ones were near 0.99 g. A second change occurred during S2's last
orientation: rejected trial 22 had roll std about 11.0 degrees and pitch std
about 7.3 degrees; accepted replacement trial 23 was quieter but stayed in the
shifted state, with calculated magnitude about 1.27 g. Repeating reduced the
within-capture variation without restoring the earlier state.

These observations do not identify a unique hardware cause. The coordinated
axis-output change with a nearly unchanged A5 reference makes the common
electrical connections a useful first check: sensor ground to Mega ground,
module supply at the module itself, A5 connected to that same supply node,
and the three signal connections. Measure supply between the module's own VS
and GND; an A5 ADC count alone is not a voltmeter reading or proof of the supply
actually reaching the sensor. Also inspect the sensor's mechanical attachment
and the complete wheel-contact/reference arrangement.

Keep the robot in one known flat pose and repeat captures, comparing the raw
counts, normalized ratios, magnitude and angles. Let the assembly settle after
handling. Establish reproducible readings before collecting a new calibration
dataset. A constant residual offset can then be investigated with known poses;
changing constants to force this last pose to zero would not explain the two
observed state changes.

The updated analyzer writes consecutive flat comparisons and excludes slope
placements bracketed by substantially changed checks from calibration fitting.
It preserves the original accepted/rejected decisions and raw logs. With the
present checkpoints, S1 and S2 both need investigation before being used to fit
one shared calibration model.

The ADXL335 has analog outputs with supply-dependent offset and sensitivity;
see the [Analog Devices datasheet](https://www.analog.com/media/en/technical-documentation/data-sheets/ADXL335.pdf).
Gravity-based tilt assumes gravity is the only sustained acceleration stimulus;
see [AN-1057](https://www.analog.com/en/resources/app-notes/2020/07/22/16/49/an-1057.html).
