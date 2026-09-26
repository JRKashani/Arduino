#include "MotionCapsule.h"
#include "DEFINE.h"
#include <SteeringDualH.h>
#include <math.h>

namespace {
SteeringDualH motors;
enum class State { Idle, Countdown, Moving };
State state = State::Idle;
bool initialized = false;
unsigned long phaseStartMs = 0, durationMs = 0;
int correction = 0;

bool scheduleMotion(unsigned long duration, int turn) {
  const int bias = turn == 0 ? MOTION_STRAIGHT_BIAS : MOTOR_BALANCE_BIAS;
  if (!initialized || state != State::Idle || duration == 0 ||
      MOTION_PWM <= 0 || MOTION_PWM > 255 || abs(turn) >= MOTION_PWM ||
      bias <= -255 || bias > 255) return false;
  durationMs = duration;
  correction = turn;
  motors.stop();
  motors.setBias(bias); // Apply per movement, including after a straight/turn switch.
  Serial.print(F("Motion queued: PWM=")); Serial.print(MOTION_PWM);
  Serial.print(F(" bias=")); Serial.print(bias);
  Serial.print(F(" harshness=")); Serial.print(correction);
  Serial.print(F(" duration_ms=")); Serial.println(durationMs);
  Serial.print(F("Countdown ms=")); Serial.print(MOTION_COUNTDOWN_MS);
  Serial.println(F("; S cancels."));
  phaseStartMs = millis();
  state = State::Countdown;
  return true;
}
}

void setupMotionCapsule() {
  motors.attach(LEFT_DIR_1, LEFT_DIR_2, LEFT_PWM,
                RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_PWM);
  motors.flipRight(); // attach resets pin order, so repeated setup is safe.
  motors.stop();
  motors.setBias(MOTOR_BALANCE_BIAS);
  state = State::Idle;
  initialized = true;
}

bool motionBusy() { return state != State::Idle; }

void stopMotion() {
  if (initialized) motors.stop();
  state = State::Idle;
}

bool driveCm(float distanceCm) {
  if (!isfinite(distanceCm) || distanceCm <= 0 ||
      distanceCm > MOTION_MAX_DISTANCE_CM ||
      !isfinite(MOTION_FORWARD_CM_PER_SECOND) ||
      MOTION_FORWARD_CM_PER_SECOND <= 0) return false;
  const float duration = 1000.0f * distanceCm / MOTION_FORWARD_CM_PER_SECOND;
  if (!isfinite(duration) || duration < 1.0f || duration > 30000.0f) return false;
  return scheduleMotion(static_cast<unsigned long>(duration + 0.5f), 0);
}

bool turnQuarterCircle(TurnDirection direction) {
  if (direction == TurnDirection::Left)
    return scheduleMotion(MOTION_LEFT_QUARTER_MS, MOTION_LEFT_HARSHNESS);
  if (direction == TurnDirection::Right)
    return scheduleMotion(MOTION_RIGHT_QUARTER_MS, MOTION_RIGHT_HARSHNESS);
  return false;
}

void runMotionCapsule() {
  const unsigned long now = millis();
  if (state == State::Countdown && now - phaseStartMs >= MOTION_COUNTDOWN_MS) {
    Serial.println(F("MOVING"));
    phaseStartMs = millis();
    motors.straight(MOTION_PWM);
    if (correction != 0) motors.turn(correction);
    state = State::Moving;
  } else if (state == State::Moving && now - phaseStartMs >= durationMs) {
    stopMotion();
    Serial.println(F("DONE: motors stopped. Reposition before next test."));
  }
}
