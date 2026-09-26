#ifndef DEFINE_H
#define DEFINE_H

#include <Arduino.h>

// Motor pins and balance
const uint8_t LEFT_DIR_1 = 46;
const uint8_t LEFT_DIR_2 = 48;
const uint8_t LEFT_PWM = 44;
const uint8_t RIGHT_DIR_1 = 49;
const uint8_t RIGHT_DIR_2 = 47;
const uint8_t RIGHT_PWM = 45;
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

// Motor calibration mode and reusable experiment settings
#define MOTOR_CALIBRATION_MODE 0
const int CALIBRATION_BIAS_CANDIDATES[] = {-30, -20, -13, -5, 0, 5, 13, 20, 30};
const uint8_t CALIBRATION_BIAS_CANDIDATE_COUNT =
	sizeof(CALIBRATION_BIAS_CANDIDATES) / sizeof(CALIBRATION_BIAS_CANDIDATES[0]);
const int CALIBRATION_BIAS_TEST_SPEED = 40;
const unsigned long CALIBRATION_BIAS_TEST_DURATION_MS = 5000UL;
const uint8_t CALIBRATION_BIAS_REPEATS = 1;
const int CALIBRATION_SPEED_COMMANDS[] = {30, 40, 50, 60, 80};
const uint8_t CALIBRATION_SPEED_COMMAND_COUNT =
	sizeof(CALIBRATION_SPEED_COMMANDS) / sizeof(CALIBRATION_SPEED_COMMANDS[0]);
const unsigned long CALIBRATION_SPEED_TEST_DURATION_MS = 3000UL;
const float CALIBRATION_MIN_FRONT_CLEARANCE_CM = 30.0f;
const int CALIBRATION_TURN_BASE_SPEEDS[] = {30, 40, 50};
const uint8_t CALIBRATION_TURN_BASE_SPEED_COUNT =
	sizeof(CALIBRATION_TURN_BASE_SPEEDS) / sizeof(CALIBRATION_TURN_BASE_SPEEDS[0]);
const int CALIBRATION_TURN_COMMANDS[] = {-20, -10, 10, 20};
const uint8_t CALIBRATION_TURN_COMMAND_COUNT =
	sizeof(CALIBRATION_TURN_COMMANDS) / sizeof(CALIBRATION_TURN_COMMANDS[0]);
const unsigned long CALIBRATION_TURN_TEST_DURATION_MS = 2000UL;
const int CALIBRATION_ROTATION_COMMANDS[] = {-30, 30};
const uint8_t CALIBRATION_ROTATION_COMMAND_COUNT =
	sizeof(CALIBRATION_ROTATION_COMMANDS) / sizeof(CALIBRATION_ROTATION_COMMANDS[0]);
const unsigned long CALIBRATION_ROTATION_TEST_DURATION_MS = 2000UL;
const unsigned long CALIBRATION_COUNTDOWN_MS = 3000UL;
const unsigned long CALIBRATION_ACCEL_LOG_INTERVAL_MS = 100UL;
// Set to a measured wheel-center spacing before using optional theory.
const float TRACK_WIDTH_MM = 0.0f;

// VL53L0X pins, addresses, validation, and filters
const uint8_t XSHUT_RIGHT_PIN = 22;
const uint8_t XSHUT_LEFT_PIN = 24;
const uint8_t LASER_RIGHT_ADDRESS = 0x30;
const uint8_t LASER_LEFT_ADDRESS = 0x31;
const int LASER_MAX_VALID_DISTANCE_MM = 2000;
const int LASER_WALL_GONE_THRESHOLD_MM = 700;
const uint8_t RIGHT_TOF_FILTER_SIZE = 5;
const uint8_t LEFT_TOF_FILTER_SIZE = 5;
const uint8_t RIGHT_WALL_HISTORY_SIZE = 5;
const uint8_t RIGHT_WALL_MAJORITY_NO_WALL_COUNT = 4;

// HC-SR04 pins, timing, and filters
const uint8_t ULTRASONIC_FRONT_TRIG_PIN = 43;
const uint8_t ULTRASONIC_FRONT_ECHO_PIN = 42;
const uint8_t ULTRASONIC_RIGHT_45_TRIG_PIN = 51;
const uint8_t ULTRASONIC_RIGHT_45_ECHO_PIN = 50;
const unsigned long ULTRASONIC_ECHO_TIMEOUT_US = 10000UL;
const unsigned long ULTRASONIC_MIN_INTERVAL_MS = 60;
const unsigned long ULTRASONIC_INTER_SENSOR_GAP_MS = 30;
const uint8_t ULTRASONIC_FILTER_SIZE = 5;
const uint8_t ULTRASONIC_CLOSE_CONFIRMATION_COUNT = 2;
const float ULTRASONIC_IMMEDIATE_STOP_CM = 15.0;
const bool ULTRASONIC_DEBUG = false;
// Calibration from measurements spanning 15-150 cm:
// corrected distance = measured distance * scale + offset.
const float ULTRASONIC_DISTANCE_SCALE = 1.01535f;
const float ULTRASONIC_DISTANCE_OFFSET_CM = 0.70910f;

// Sensor mounting offsets from the robot center (= IMU location), in cm.
// The sensor layer applies these so downstream stage code works in
// center-relative distances; that is why wall/light targets can be expressed
// straight from the robot center (e.g. RIGHT_WALL_FOLLOW_TARGET_MM = 300 mm).
const float LASER_LATERAL_OFFSET_CM = 1.2f;             // each side laser sits 1.2 cm toward its wall
const float ULTRASONIC_FRONT_FORWARD_OFFSET_CM = 10.0f; // nose sensor is 10 cm ahead of center
// Telemetry (live sensor dashboard) print cadence.
const unsigned long SENSOR_TELEMETRY_INTERVAL_MS = 300UL;

// LDR pins and calibration offset
const uint8_t LDR_RIGHT_PIN = A3;
const uint8_t LDR_LEFT_PIN = A4;
const int LIGHT_SENSOR_OFFSET = 157;

// ADXL335 pins and empirical calibration weights passed to ADXL335_IMU.
// Each zero ratio is the axis voltage / 3.3 V rail at zero g.
// Each ratio-per-g is the change in that normalized ratio for one g.
const uint8_t ACCEL_X_PIN = A2;
const uint8_t ACCEL_Y_PIN = A1;
const uint8_t ACCEL_Z_PIN = A0;
const uint8_t ACCEL_REF_3V3_PIN = A5;
const float X_ZERO_RATIO = 0.5143f;
const float Y_ZERO_RATIO = 0.5070f;
const float Z_ZERO_RATIO = 0.5068f;
const float X_RATIO_PER_G = 0.1050f;
const float Y_RATIO_PER_G = 0.1020f;
const float Z_RATIO_PER_G = 0.0988f;
const int ACCEL_CALIBRATION_SAMPLES = 100;
const unsigned long ACCEL_CALIBRATION_SAMPLE_DELAY_MS = 10;
const unsigned long IMU_PRINT_INTERVAL_MS = 200UL;
const bool IMU_PRINT_ENABLED = false; // Keep sampling; quiet during motion tests.

// Status LEDs and driving speeds
const uint8_t LED_RED_PIN = 8;
const uint8_t LED_GREEN_PIN = 9;
const uint8_t LED_WHITE_PIN = 12;
const int DRIVE_SPEED = 50;

// Stage 1 and Stage 2
const int WALL_DETECTED_THRESHOLD_MM = 500;
const int LEFT_WALL_GONE_THRESHOLD_MM = 1000;
const float STEER_GAIN = 0.05;
const int CENTER_TOLERANCE_MM = 100;
const float STAGE2_SEARCH_FRONT_SAFETY_CM = 50.0;
const int RIGHT_WALL_SEARCH_STEP_INCREMENT = 2;
const int RIGHT_WALL_SEARCH_STEP_MAX = 15;

// Stage 3
const int RIGHT_WALL_FOLLOW_TARGET_MM = 300; // 30 cm from robot center
const float WALL_FOLLOW_GAIN = 0.10;         // Kp: steer per mm of distance error
const int WALL_FOLLOW_MAX_STEER = 25;        // clamp on the total (P+D) steer = max turn sharpness
const float WALL_FOLLOW_DEADBAND_MM = 5.0;   // no P correction within this band (D still acts)
// Derivative term: damps the weave and acts as a heading proxy (approach rate).
const float WALL_FOLLOW_DERIV_GAIN = 0.30;   // Kd: steer per (mm/s); ~critical damping for Kp 0.10 (model: zeta~0.9)
const unsigned long WALL_FOLLOW_DERIV_INTERVAL_MS = 100UL; // derivative recompute cadence (stable dt)
const float WALL_FOLLOW_DERIV_LPF_ALPHA = 0.4f; // dError low-pass (0..1, higher = less smoothing)
// Speed scheduling: DRIVE_SPEED when steering straight, down to this at max steer.
const int WALL_FOLLOW_MIN_SPEED = 30;
// Outside corner / wall lost: arc right (toward the lost wall) until it returns.
const uint8_t WALL_LOST_CONFIRM_COUNT = 3;        // consecutive gone readings before searching
const int WALL_SEARCH_SPEED = 40;                 // base speed while searching
const int WALL_SEARCH_STEER = 12;                 // steer magnitude toward the wall while searching
const unsigned long WALL_SEARCH_TIMEOUT_MS = 5000UL; // ~half circle; then stop (end of wall)

// Rate limit for stage decision logging (0 = log every loop iteration).
const unsigned long STAGE_LOG_INTERVAL_MS = 250UL;

// Emergency recovery
const float FRONT_EMERGENCY_STOP_CM = 20.0;
const int EMERGENCY_TURN_AMOUNT = 10;
const int EMERGENCY_ROTATE_SPEED = 30;
const float FRONT_EMERGENCY_CLEAR_CM = 30.0;
const uint8_t EMERGENCY_CLEAR_CONFIRMATION_COUNT = 2;
const uint8_t EMERGENCY_WALL_CONFIRMATION_COUNT = 2;

// Stage 4
const float TILT_TOLERANCE_DEG = 5.0;
const float ROLL_CORRECTION_GAIN = 2.0;
const int STAGE4_CLIMB_SPEED = 60;
const unsigned long FLAT_DURATION_REQUIRED_MS = 1000;

// Stage 5
const int LIGHT_DIFFERENCE_TOLERANCE = 30;
const float LIGHT_STOP_DISTANCE_CM = 12.0;
const int LIGHT_STEER_AMOUNT = 10;

#endif
