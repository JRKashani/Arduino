#ifndef MOTION_CAPSULE_H
#define MOTION_CAPSULE_H

#include <Arduino.h>
#include "MotorCalibration.h"

enum class TurnDirection { Left, Right };

void setupMotionCapsule();
// Call frequently from loop(), including during countdown and movement.
void runMotionCapsule();
// Return false for invalid requests or if another movement is pending/running.
// Accepted commands start after MOTION_COUNTDOWN_MS. No movement queue.
bool driveCm(float distanceCm);
// Immediate stage move at the distance row's measured PWM and bias.
bool driveStageCm(float distanceCm);
bool turnQuarterCircle(TurnDirection direction);
// Immediate timed 90-degree turn using an exact MOTOR_TURN_90DEG row.
// No test countdown; preserves that row's PWM, bias, harshness and duration.
bool turnCalibrated90(TurnDirection direction, uint8_t calibrationIndex);
// Smaller angles scale the measured time; accuracy must be checked on hardware.
bool turnCalibrated(TurnDirection direction, float angleDeg, uint8_t calibrationIndex);
bool motionBusy();
void stopMotion(); // Cancels countdown too.

// Continuous (closed-loop) drive for the mission stages. Applies immediately:
// straight base speed plus a live steering harshness (positive = left,
// negative = right), re-issued every loop with fresh sensor feedback.
// speed and steer are clamped by the motor library to [-255, 255].
void driveContinuous(int speed, int steer, int bias = MOTOR_BALANCE_BIAS);

#endif
