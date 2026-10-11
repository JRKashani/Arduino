# Measured accelerometer tables

Angles are phone-estimated slope magnitudes, signed by robot heading. These are measured household slopes, not exact prescribed angles. The phone convention is assumed to be axis-elevation; sensor columns retain the original sensor X/Y/Z signs, with no body-axis remapping.

Each slope row averages the two loop placement means equally. Each flat group first averages retained captures equally; the zero row then averages the six accepted flat placement groups equally. The zero label identifies the flat reference, not a phone-verified exact zero.

The g values are measured acceleration including gravity, using the replacement-sensor six-face constants active during acquisition. The phone-fitted candidate has NOT been applied. The g values are not theoretical sin/cos values, and raw ADC counts are not voltages.

Original samples and references remain preserved, including the user-confirmed S5 270-degree minus-sign correction.

## Raw ADC readings: X-tilt

| Angle (deg) | X (ADC counts) | Y (ADC counts) | Z (ADC counts) |
| ---: | ---: | ---: | ---: |
| -34.3 | 307.4 | 354.7 | 406.3 |
| -20.0 | 323.0 | 356.4 | 413.9 |
| -14.0 | 330.7 | 356.8 | 416.4 |
| -8.8 | 337.0 | 356.8 | 417.6 |
| -4.3 | 342.4 | 356.0 | 417.6 |
| 0 (flat reference) | 348.0 | 356.7 | 418.0 |
| +4.3 | 352.9 | 356.7 | 417.7 |
| +8.8 | 358.8 | 356.9 | 417.3 |
| +14.0 | 365.8 | 356.7 | 415.5 |
| +20.0 | 372.3 | 356.1 | 413.3 |
| +34.3 | 388.0 | 358.2 | 404.3 |

## Measured acceleration: X-tilt

| Angle (deg) | X (g) | Y (g) | Z (g) |
| ---: | ---: | ---: | ---: |
| -34.3 | -0.552 | 0.044 | 0.841 |
| -20.0 | -0.334 | 0.069 | 0.949 |
| -14.0 | -0.229 | 0.070 | 0.979 |
| -8.8 | -0.141 | 0.070 | 0.995 |
| -4.3 | -0.060 | 0.065 | 1.003 |
| 0 (flat reference) | 0.015 | 0.071 | 1.004 |
| +4.3 | 0.084 | 0.072 | 1.002 |
| +8.8 | 0.163 | 0.071 | 0.991 |
| +14.0 | 0.263 | 0.072 | 0.969 |
| +20.0 | 0.354 | 0.063 | 0.938 |
| +34.3 | 0.574 | 0.093 | 0.815 |
