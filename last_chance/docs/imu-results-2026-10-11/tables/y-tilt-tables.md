# Measured accelerometer tables

Angles are phone-estimated slope magnitudes, signed by robot heading. These are measured household slopes, not exact prescribed angles. The phone convention is assumed to be axis-elevation; sensor columns retain the original sensor X/Y/Z signs, with no body-axis remapping.

Each slope row averages the two loop placement means equally. Each flat group first averages retained captures equally; the zero row then averages the six accepted flat placement groups equally. The zero label identifies the flat reference, not a phone-verified exact zero.

The g values are measured acceleration including gravity, using the replacement-sensor six-face constants active during acquisition. The phone-fitted candidate has NOT been applied. The g values are not theoretical sin/cos values, and raw ADC counts are not voltages.

Original samples and references remain preserved, including the user-confirmed S5 270-degree minus-sign correction.

Y-tilt uses headings 90 and 270 degrees. The signed angle is the slope magnitude, not sensor Y sign: heading 90 is negative and heading 270 positive, consistent with recorded sensor roll.

## Raw ADC readings

| Angle (deg) | X (ADC counts) | Y (ADC counts) | Z (ADC counts) |
| ---: | ---: | ---: | ---: |
| -34.3 | 346.7 | 316.4 | 409.3 |
| -20.0 | 347.3 | 331.9 | 415.8 |
| -14.0 | 347.8 | 339.6 | 417.5 |
| -8.8 | 347.5 | 346.3 | 418.3 |
| -4.3 | 347.5 | 351.5 | 418.2 |
| 0 (flat reference) | 348.0 | 356.7 | 418.0 |
| +4.3 | 347.5 | 361.8 | 417.5 |
| +8.8 | 347.9 | 367.4 | 416.8 |
| +14.0 | 348.0 | 373.8 | 414.7 |
| +20.0 | 346.9 | 381.0 | 411.7 |
| +34.3 | 347.9 | 395.8 | 402.7 |

## Measured acceleration

| Angle (deg) | X (g) | Y (g) | Z (g) |
| ---: | ---: | ---: | ---: |
| -34.3 | -0.004 | -0.487 | 0.884 |
| -20.0 | 0.003 | -0.273 | 0.972 |
| -14.0 | 0.010 | -0.167 | 0.995 |
| -8.8 | 0.005 | -0.075 | 1.006 |
| -4.3 | 0.009 | 0.001 | 1.008 |
| 0 (flat reference) | 0.015 | 0.071 | 1.004 |
| +4.3 | 0.008 | 0.142 | 0.998 |
| +8.8 | 0.010 | 0.217 | 0.983 |
| +14.0 | 0.015 | 0.308 | 0.958 |
| +20.0 | 0.001 | 0.409 | 0.918 |
| +34.3 | 0.013 | 0.613 | 0.791 |
