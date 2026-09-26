#ifndef MOTOR_CALIBRATION_H
#define MOTOR_CALIBRATION_H

// Measured motor calibration (2026-09-22) and the motion profile derived
// from it. Kept as a record of the experiments: some table rows and
// metadata are not referenced by code but document how constants were
// chosen. Included by DEFINE.h; edit the active profile here.

#include <Arduino.h>

const int MOTOR_BALANCE_BIAS = -8;

// Motor calibration data (2026-09-22).
// Distance rows use MEAN time; turn rows use MEDIAN time, separately per side.
// All times include the operator's manual stopping response, not the countdown.
// Do not combine rows with different bias, PWM, harshness, or measured angle.
// Superseded turn results (before the requested reset) and bias +10 are excluded.
const int MOTOR_CALIBRATION_BIAS_UNKNOWN = 32767; // Metadata only; never apply.

struct MotorDistanceCalibration {
  uint8_t pwm;
  int bias;
  float distanceCm;
  float meanTimeMs;
  uint8_t samples;
  constexpr float msPerCm() const { return meanTimeMs / distanceCm; }
  constexpr float cmPerSecond() const { return 1000.0f * distanceCm / meanTimeMs; }
};

// Initial 120 cm experiment: bias was not recorded in the supplied data.
// Each row is one measurement. Conversion: time_ms = distance_cm * msPerCm().
constexpr MotorDistanceCalibration MOTOR_DISTANCE_120CM[] = {
  // PWM, bias, distance_cm, mean_time_ms, samples
  { 30, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f, 18660.0f, 1},
  { 40, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f, 12550.0f, 1},
  { 50, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  9440.0f, 1},
  { 60, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  7810.0f, 1},
  { 65, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  7130.0f, 1},
  { 70, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  6520.0f, 1},
  { 80, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  5750.0f, 1},
  { 85, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  5400.0f, 1},
  { 90, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  5200.0f, 1},
  {100, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  4490.0f, 1},
  {125, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  3620.0f, 1},
  {150, MOTOR_CALIBRATION_BIAS_UNKNOWN, 120.0f,  3260.0f, 1}
};
const uint8_t MOTOR_DISTANCE_120CM_COUNT =
    sizeof(MOTOR_DISTANCE_120CM) / sizeof(MOTOR_DISTANCE_120CM[0]);

// Later 60 cm trials, grouped only by identical PWM AND bias.
constexpr MotorDistanceCalibration MOTOR_DISTANCE_60CM[] = {
  // PWM, bias, distance_cm, mean_time_ms, samples
  { 60, -8, 60.0f, 3883.0f,         4},
  { 70, -8, 60.0f, 9701.0f / 3.0f, 3},
  { 80,  1, 60.0f, 2830.0f,         1},
  { 80, -1, 60.0f, 2757.5f,         2},
  { 80, -2, 60.0f, 2792.0f,         1},
  { 80, -3, 60.0f, 2721.0f,         1},
  { 80, -4, 60.0f, 2804.0f,         1},
  { 90, -4, 60.0f, 2433.0f,         5},
  {100, -4, 60.0f, 2178.0f,         3},
  {100, -5, 60.0f, 2188.0f,         1},
  {100, -6, 60.0f, 2177.0f,         1},
  {100, -7, 60.0f, 2239.0f,         1},
  {100, -8, 60.0f, 2204.5f,         2}
};
const uint8_t MOTOR_DISTANCE_60CM_COUNT =
    sizeof(MOTOR_DISTANCE_60CM) / sizeof(MOTOR_DISTANCE_60CM[0]);

struct MotorTurnCalibration {
  uint8_t pwm;
  int bias;
  uint16_t angleDeg; // Measured heading change, not wheel rotation.
  int leftHarshness;
  int rightHarshness;
  float leftMedianMs;
  float rightMedianMs;
  uint8_t leftSamples;
  uint8_t rightSamples;
  constexpr float leftMsPerDegree() const { return leftMedianMs / angleDeg; }
  constexpr float rightMsPerDegree() const { return rightMedianMs / angleDeg; }
};

// Time/angle ratios are empirical averages for the measured angle.
// Scaling to smaller angles is an unverified estimate (startup/coasting effects).
// Harshness alone does not define deg/s or radius; PWM and bias also matter.
constexpr MotorTurnCalibration MOTOR_TURN_90DEG[] = {
  // PWM, bias, angle, left_h, right_h, left_ms, right_ms, left_n, right_n
  { 40, 0, 90, 10,   -10, 4071.0f, 4816.0f, 4, 3},
  { 45, 0, 90, 10,   -10, 3851.0f, 4569.0f, 3, 3},
  { 50, 0, 90, 10,   -10, 3900.0f, 4924.0f, 3, 3},
  { 50, 0, 90, 15,   -15, 2638.0f, 2996.0f, 3, 3},
  { 50, 0, 90, 20,   -20, 2151.0f, 2317.0f, 3, 3},
  { 60, 0, 90, 50,   -50,  852.0f,  903.0f, 4, 3},
  {100, 0, 90, 70,   -70,  651.0f,  814.0f, 5, 4}
};
const uint8_t MOTOR_TURN_90DEG_COUNT =
    sizeof(MOTOR_TURN_90DEG) / sizeof(MOTOR_TURN_90DEG[0]);

constexpr MotorTurnCalibration MOTOR_TURN_180DEG[] = {
  {70, 0, 180, 50, -50, 1648.5f, 1828.0f, 4, 3},
  {75, 0, 180, 50, -50, 1635.5f, 1841.5f, 6, 6}
};
const uint8_t MOTOR_TURN_180DEG_COUNT =
    sizeof(MOTOR_TURN_180DEG) / sizeof(MOTOR_TURN_180DEG[0]);

// Accepted quarter-circle approximation: radius about 60 cm left, 55-57 cm right.
// Both represent 90 degrees, NOT full circles. Bias -8 is required for this row.
constexpr MotorTurnCalibration MOTOR_ARC_60CM_RADIUS[] = {
  {60, -8, 90, 10, -10, 5856.0f, 5343.0f, 3, 3}
};
const float MOTOR_ARC_TARGET_RADIUS_CM = 60.0f;
const float MOTOR_ARC_RIGHT_RADIUS_MIN_CM = 55.0f;
const float MOTOR_ARC_RIGHT_RADIUS_MAX_CM = 57.0f;

// Exploratory bias-0 quarter-turns retained for reference only, NOT radius
// calibration: the right -6 trial made a much wider arc than the target.
constexpr MotorTurnCalibration MOTOR_ARC_EXPLORATORY[] = {
  {60, 0, 90, 6, -6, 5836.0f, 14521.0f, 1, 1}
};

// Active motion profile: use the matching bias/PWM distance and arc rows.
constexpr uint8_t MOTION_DISTANCE_CALIBRATION_INDEX = 0;
constexpr uint8_t MOTION_ARC_CALIBRATION_INDEX = 0;
constexpr int MOTION_PWM = MOTOR_DISTANCE_60CM[MOTION_DISTANCE_CALIBRATION_INDEX].pwm;
const float MOTION_FORWARD_CM_PER_SECOND =
    MOTOR_DISTANCE_60CM[MOTION_DISTANCE_CALIBRATION_INDEX].cmPerSecond();
const float MOTION_TEST_DISTANCE_CM = 60.0f;
const float MOTION_MAX_DISTANCE_CM = 120.0f;
const unsigned long MOTION_COUNTDOWN_MS = 2000UL;
// Trial trims from automatic runs: straight drifts left; right overshoots ~10 cm.
// Keep measured calibration tables intact. Turns retain their calibrated bias.
const int MOTION_STRAIGHT_BIAS_TRIM = -2;
const int MOTION_STRAIGHT_BIAS = MOTOR_BALANCE_BIAS + MOTION_STRAIGHT_BIAS_TRIM;
const long MOTION_RIGHT_TIME_TRIM_MS = -650L;
const int MOTION_LEFT_HARSHNESS = MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].leftHarshness;
const int MOTION_RIGHT_HARSHNESS = MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].rightHarshness;
const unsigned long MOTION_LEFT_QUARTER_MS = static_cast<unsigned long>(
    MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].leftMedianMs + 0.5f);
const unsigned long MOTION_RIGHT_QUARTER_MS = static_cast<unsigned long>(
    MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].rightMedianMs +
    MOTION_RIGHT_TIME_TRIM_MS + 0.5f);
static_assert(MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].rightMedianMs +
              MOTION_RIGHT_TIME_TRIM_MS >= 1.0f, "Right turn duration must be positive");
static_assert(MOTION_PWM == MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].pwm,
              "Motion distance and arc PWM must match");
static_assert(MOTOR_BALANCE_BIAS == MOTOR_DISTANCE_60CM[MOTION_DISTANCE_CALIBRATION_INDEX].bias &&
              MOTOR_BALANCE_BIAS == MOTOR_ARC_60CM_RADIUS[MOTION_ARC_CALIBRATION_INDEX].bias,
              "Motion bias must match both calibration rows");
// Nominal radius 60 cm; the measured right arc was about 3-5 cm tighter.

// Forward travel speed calibration fitted over PWM 30-100.
// speedCmPerSecond = MOTOR_SPEED_SCALE_CM_PER_SECOND_PER_PWM * pwm
//                  + MOTOR_SPEED_OFFSET_CM_PER_SECOND;
const float MOTOR_SPEED_SCALE_CM_PER_SECOND_PER_PWM = 0.28198f;
const float MOTOR_SPEED_OFFSET_CM_PER_SECOND = -1.67262f;
const uint8_t MOTOR_SPEED_FIT_MIN_PWM = 30;
const uint8_t MOTOR_SPEED_FIT_MAX_PWM = 100;
// This fit belongs to MOTOR_DISTANCE_120CM (unknown bias); active motion uses
// the measured PWM-60/bias--8 row instead. Do not extrapolate the fit to PWM 0.

#endif
