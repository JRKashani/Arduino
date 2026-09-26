# Manual motor and angle calibration

Upload SerialDebugMotorsDriver.ino to Mega 2560. Open Serial Monitor at
115200 baud, with any line-ending setting. Send one command at a time.
This restores the earlier manual controls and adds timed steering trials.
The sketch does not use ultrasonic readings or CalibrationMath.h.
Existing geometry helper and verification files belong to the previous
ultrasonic version and are not tests of this manual version.

Wiring: left direction 46/48, PWM 44; right direction 49/47, PWM 45.
The right motor is reversed using flipRight(), as in the main project.
Defaults match the earlier manual version: PWM 40, bias 0.

| Command | Action |
| --- | --- |
| 1 / 2 / 3 | Left wheel forward / stop / reverse |
| 4 / 5 / 6 | Right wheel forward / stop / reverse |
| F / B | Both wheels forward / backward until stopped |
| S | Stop both; cancel a countdown or trial |
| + / - | Change PWM by 5, between 0 and 150 |
| ] / [ | Change bias by 1, between -100 and 100 |
| T | Five-second straight trial |
| L / R | Timed left / right steering trial |
| J / K | Stopwatch left / right turn until S; save elapsed time and settings |
| V | Stopwatch forward with selected PWM and bias; S at 60 cm saves result |
| C / E | Stopwatch left / right circle at fixed PWM 60 and selected bias |
| G | Print the last 20 saved stopwatch turns as CSV |
| > / < | Change turn amount by 5, between 1 and 100 |
| } / { | Change turn amount by 1 for fine circle adjustment |
| . / , | Change turn duration by 250 ms, between 250 and 5000 ms |
| P | Print settings |
| H / ? | Help |

Changing settings stops the motors. T/L/R/J/K/V/C/E use COUNTDOWN_MS (currently
2000 ms). The countdown is excluded from the saved time.
S remains responsive during both countdown and movement. Any other non-whitespace
command during a trial also cancels it and is consumed; resend after stopping.
Manual wheel and F/B commands continue until stopped. There is no obstacle stop.

## TIC / turn / TOC at 90 degrees

Set velocity with + / -, harshness with > / <, and bias with ] / [.
Mark the starting heading and the desired 90-degree heading. Send J for left or
K for right. After the countdown, timing starts as motor commands
are applied. The turn continues until you send S (lowercase s also works).
The duration setting for L/R does not limit J/K.

At 90 degrees, send S: motors stop before printing, and the sketch saves elapsed
milliseconds, commanded velocity (PWM, not cm/s), signed harshness and bias.
Positive harshness means left; negative means right. Example output:

```text
time_ms,velocity_pwm,harshness,bias,trial
2450,40,10,-13,TURN_MANUAL
```

G reprints saved trials in chronological order. The newest 20 are retained in RAM;
additional results replace the oldest. Copy the CSV from Serial Monitor before
reset or power-off. There is no automatic disk or EEPROM saving.
S during the countdown cancels without saving. Any other non-whitespace command
during movement cancels without saving; this can discard an unsuccessful trial.
Timing ends when the Arduino reads S, so reaction time, serial delivery, and
coasting affect the measured 90-degree result. Repeat each setting several times.

## Measure turn angles manually

1. Lift the wheels and use 1 and 4 to verify physical forward direction; S stops.
2. On the floor, establish straight travel with T. Adjust bias using [ / ].
   If forward travel curves right, increase bias; if left, decrease it.
   If retaining the established -13 bias, send [ thirteen times before testing.
3. Mark the robot's starting heading on the floor. Use a straight reference along
   its chassis, and measure the change in that direction with a protractor.
4. Start with PWM 40, turn amount 10, duration 1000 ms. Send L.
5. After the robot stops, measure its heading change (not the angle of its path).
   Record PWM, bias, signed correction, elapsed time printed by the sketch, and
   measured angle. Left is positive; right is negative.
6. Reposition to the same start and repeat at least three times, then test R.
   Use > / < to change correction, and . / , to change the duration.

The turn command is straight(PWM), then turn(signed correction). Both wheels move
forward; turn amount must be smaller than PWM. These are steering arcs, not
in-place rotations. Bias uses the same formula as SteeringDualH.

Average turn rate in degrees/second = measured angle / (elapsed milliseconds / 1000).
For example, 30 degrees in 1000 ms gives 30 degrees/second. Starting and coasting
affect short runs; keep conditions and measurement procedure consistent.
Keep separate results for each base PWM, bias, correction, and direction.
Settings reset on restart. Only J/K/V/C/E trials completed with S are retained in the
RAM results table; record any other measurements yourself.

## Forward TIC / TOC over 60 cm

Mark two lines 60 cm apart. Use the same reference point on the robot at both.
Set PWM with + / - and bias with ] / [. P shows the selected values.
Send V, wait for the countdown, then send S as the reference reaches 60 cm.
The robot stops and records elapsed time, selected PWM, zero harshness, and bias:

```text
time_ms,velocity_pwm,harshness,bias,trial
6000,40,0,0,STRAIGHT_60CM
```

This label assumes you stopped at 60 cm; the sketch does not measure distance.
Repeat each setting at least three times. Average speed in cm/s is 60000/time_ms.
V ignores turnAmount and the fixed-duration setting and keeps moving until S.

## Circle radius calibration at PWM 60

The target is a 60 cm RADIUS circle traced by the robot center (diameter 120 cm).
Mark the center point between the drive wheels on the robot and use its path as
the reference, not the outer wheel or chassis edge.

Choose the bias and start at harshness 10. Send C for a left circle or E for a
right circle. Both use PWM 60 regardless of the selected forward-test PWM.
They keep the current bias and harshness, and run until S. For comparable times,
stop after one complete revolution. After stopping, record the observed radius
alongside the saved CSV row. CIRCLE_TARGET_RADIUS_60CM denotes the intended
target, not a measured or confirmed radius.

If the circle is too large, increase harshness (> by 5 or } by 1). If too small,
decrease it (< by 5 or { by 1). Reposition and repeat. Keep bias fixed throughout;
test left and right separately. Circle commands require harshness < 60, keeping
both commanded wheels forward. Adjust harshness between runs; input during a
run other than S cancels without saving. The forward-test PWM is unchanged by C/E.

Existing angle/time measurements do not determine circle diameter: the center's
travel during a turn is not yet measured. This experiment finds the harshness
empirically; it does not guarantee a perfect circle. Log diameter variation if
the path is not circular. Results remain in RAM only; use G and copy before reset.
