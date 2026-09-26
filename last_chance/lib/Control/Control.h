#ifndef CONTROL_H
#define CONTROL_H

// Pure control helpers. Deliberately Arduino-free so they can be unit-tested
// on the host with `pio test -e native`.

// Proportional steering with a deadband and a symmetric clamp.
// `error` and `gain` are unit-agnostic (the stages use error in millimetres).
// Returns 0 while |error| <= deadband; otherwise gain*error rounded to the
// nearest integer and clamped to [-maxOutput, maxOutput].
int steerFromError(float error, float gain, int maxOutput, float deadband);

// Proportional-derivative steering. The P term is suppressed within +-deadband
// (no fidgeting near target), but the D term (kd applied to dError, e.g. the
// approach rate in mm/s) ALWAYS acts, providing damping even inside the band.
// Output = P + kd*dError, rounded and clamped to [-maxOutput, maxOutput].
int steerFromPD(float error, float dError, float kp, float kd,
                int maxOutput, float deadband);

#endif
