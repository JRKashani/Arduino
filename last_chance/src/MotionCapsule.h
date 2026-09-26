#ifndef MOTION_CAPSULE_H
#define MOTION_CAPSULE_H

#include <Arduino.h>

enum class TurnDirection { Left, Right };

void setupMotionCapsule();
// Call frequently from loop(), including during countdown and movement.
void runMotionCapsule();
// Return false for invalid requests or if another movement is pending/running.
// Accepted commands start after MOTION_COUNTDOWN_MS. No movement queue.
bool driveCm(float distanceCm);
bool turnQuarterCircle(TurnDirection direction);
bool motionBusy();
void stopMotion(); // Cancels countdown too.

// Continuous (closed-loop) drive for the mission stages. Applies immediately:
// straight base speed plus a live steering harshness (positive = left,
// negative = right), re-issued every loop with fresh sensor feedback.
// speed and steer are clamped by the motor library to [-255, 255].
void driveContinuous(int speed, int steer);

#endif
