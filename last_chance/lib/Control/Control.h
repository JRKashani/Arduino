#ifndef CONTROL_H
#define CONTROL_H

// Pure control helpers. Deliberately Arduino-free so they can be unit-tested
// on the host with `pio test -e native`.

// Proportional-derivative steering. The P term is suppressed within +-deadband
// (no fidgeting near target), but the D term (kd applied to dError, e.g. the
// approach rate in mm/s) ALWAYS acts, providing damping even inside the band.
// Output = P + kd*dError, rounded and clamped to [-maxOutput, maxOutput].
int steerFromPD(float error, float dError, float kp, float kd,
                int maxOutput, float deadband);

// Speed scheduling: slow down while steering hard, so a large correction turns
// the heading around in less sideways travel. Linear from baseSpeed at steer 0
// down to minSpeed at |steer| >= maxSteer.
int scaleSpeedBySteer(int baseSpeed, int minSpeed, int steer, int maxSteer);

// Proximity ramp: 0 when distance >= startDist, maxOutput when distance <=
// fullDist, linear in between (rounded). Used for "obstacle ahead" steering.
int rampByProximity(float distance, float startDist, float fullDist, int maxOutput);

// Slew-rate limiter: move `current` toward `target` by at most
// maxRatePerS * dtS (rounded, at least 1 when rate > 0 and dt > 0).
int slewLimit(int current, int target, float maxRatePerS, float dtS);

// Actuator dead-zone compensation: small commands that would not overcome
// static friction get `comp` added in their direction; 0 stays 0.
int compensateDeadzone(int cmd, int comp);

#endif
