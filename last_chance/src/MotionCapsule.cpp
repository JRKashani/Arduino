#include "MotionCapsule.h"
#include "DEFINE.h"
#include <SteeringDualH.h>
#include <math.h>

namespace {
SteeringDualH motors;
// Manual = a stage is driving continuously via driveContinuous(); the timed
// state machine in runMotionCapsule() stays out of the way in that state.
enum class State { Idle, Countdown, Moving, Manual };
State state = State::Idle;
bool initialized = false;
unsigned long phaseStartMs = 0, durationMs = 0;
int correction = 0;
int timedPwm = MOTION_PWM;

bool scheduleMotion(unsigned long duration, int turn) {
  const int bias = turn == 0 ? MOTION_STRAIGHT_BIAS : MOTOR_BALANCE_BIAS;
  if (!initialized || state != State::Idle || duration == 0 ||
      MOTION_PWM <= 0 || MOTION_PWM > 255 || abs(turn) >= MOTION_PWM ||
      bias <= -255 || bias > 255) return false;
  durationMs = duration;
  timedPwm = MOTION_PWM;
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

bool driveStageCm(float distanceCm) {
  if (!initialized || state != State::Idle || !isfinite(distanceCm) ||
      distanceCm <= 0 || distanceCm > MOTION_MAX_DISTANCE_CM) return false;
  const MotorDistanceCalibration &cal = MOTOR_DISTANCE_60CM[MOTION_DISTANCE_CALIBRATION_INDEX];
  const float duration = distanceCm * cal.msPerCm();
  if (!isfinite(duration) || duration < 1.0f || duration > 30000.0f) return false;
  durationMs = static_cast<unsigned long>(duration + 0.5f);
  timedPwm = cal.pwm;
  correction = 0;
  Serial.print(F("MOTION stage drive distance_cm=")); Serial.print(distanceCm, 1);
  Serial.print(F(" PWM=")); Serial.print(timedPwm);
  Serial.print(F(" bias=")); Serial.print(cal.bias);
  Serial.print(F(" duration_ms=")); Serial.println(durationMs);
  motors.setBias(cal.bias);
  motors.straight(timedPwm);
  phaseStartMs = millis();
  state = State::Moving;
  return true;
}

void driveContinuous(int speed, int steer, int bias) {
  if (!initialized) return;
  state = State::Manual;
  motors.setBias(bias);
  motors.straight(speed);
  if (steer != 0) motors.turn(steer); // positive = left, negative = right
}

bool turnQuarterCircle(TurnDirection direction) {
  if (direction == TurnDirection::Left)
    return scheduleMotion(MOTION_LEFT_QUARTER_MS, MOTION_LEFT_HARSHNESS);
  if (direction == TurnDirection::Right)
    return scheduleMotion(MOTION_RIGHT_QUARTER_MS, MOTION_RIGHT_HARSHNESS);
  return false;
}

bool turnCalibrated90(TurnDirection direction, uint8_t calibrationIndex) {
  return turnCalibrated(direction, 90.0f, calibrationIndex);
}

bool turnCalibrated(TurnDirection direction, float angleDeg, uint8_t calibrationIndex) {
  if (!initialized || state != State::Idle ||
      calibrationIndex >= MOTOR_TURN_90DEG_COUNT || !isfinite(angleDeg) ||
      angleDeg <= 0 || angleDeg > 90.0f) return false;
  const MotorTurnCalibration &cal = MOTOR_TURN_90DEG[calibrationIndex];
  if (direction != TurnDirection::Left && direction != TurnDirection::Right)
    return false;
  const bool left = direction == TurnDirection::Left;
  const float duration = angleDeg * (left ? cal.leftMsPerDegree() : cal.rightMsPerDegree());
  if (duration < 1.0f) return false;
  durationMs = static_cast<unsigned long>(duration + 0.5f);
  correction = left ? cal.leftHarshness : cal.rightHarshness;
  timedPwm = cal.pwm;
  Serial.print(F("MOTION turn=")); Serial.print(left ? F("LEFT") : F("RIGHT"));
  Serial.print(F(" angle_deg=")); Serial.print(angleDeg, 1);
  Serial.print(F(" PWM=")); Serial.print(timedPwm);
  Serial.print(F(" bias=")); Serial.print(cal.bias);
  Serial.print(F(" harshness=")); Serial.print(correction);
  Serial.print(F(" duration_ms=")); Serial.println(durationMs);
  motors.setBias(cal.bias);
  motors.straight(timedPwm);
  motors.turn(correction);
  phaseStartMs = millis();
  state = State::Moving;
  return true;
}

void runMotionCapsule() {
  const unsigned long now = millis();
  if (state == State::Countdown && now - phaseStartMs >= MOTION_COUNTDOWN_MS) {
    Serial.println(F("MOVING"));
    phaseStartMs = millis();
    motors.straight(timedPwm);
    if (correction != 0) motors.turn(correction);
    state = State::Moving;
  } else if (state == State::Moving && now - phaseStartMs >= durationMs) {
    stopMotion();
    Serial.println(F("DONE: timed motion stopped."));
  }
}
