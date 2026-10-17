# Individual sensor raw-output graphs

The [combined graph with experimental laser calibration](../comparison-calibrated-laser/three-sensor-characteristic.png) is preserved unchanged. These additional figures show raw outputs from the same principal sessions.

[All three graphs as a PDF](raw-sensor-graphs.pdf) | [Plotted data](plotted-data.csv)

- [Ultrasonic PNG](us-raw.png) / [PDF](us-raw.pdf): echo time in microseconds on the left; calculated cm using the existing ultrasonic calibration on the right. This is one curve with two equivalent scales.
- [Laser PNG](laser-raw.png) / [PDF](laser-raw.pdf): uncorrected device-reported millimetres.
- [IR PNG](ir-raw.png) / [PDF](ir-raw.pdf): ADC counts.

Points are equally weighted means of three accepted repeats. Individual accepted samples are also shown as faint dots. Vertical bars span the extrema of individual accepted samples, not confidence intervals or the range of repeat means. Horizontal bars are ±1 cm (2 cm total width). Rejected, invalid and incomplete data are excluded. The laser alignment caveat remains noted on its figure. There are 19 ultrasonic, 16 laser and 15 IR positions, with 1,500 accepted samples in total.

Reproduce from last_chance:

```powershell
.\.venv\Scripts\python.exe tools\distance_test\plot_raw_sensors.py
```
