#ifndef DEFINE_H
#define DEFINE_H

#include <Arduino.h>

// Motor pins
const uint8_t LEFT_DIR_1 = 46;
const uint8_t LEFT_DIR_2 = 48;
const uint8_t LEFT_PWM = 44;
const uint8_t RIGHT_DIR_1 = 49;
const uint8_t RIGHT_DIR_2 = 47;
const uint8_t RIGHT_PWM = 45;

// Motor balance, calibration tables and the active motion profile.
#include "MotorCalibration.h"

// VL53L0X pins, addresses and validation
const uint8_t XSHUT_RIGHT_PIN = 22;
const uint8_t XSHUT_LEFT_PIN = 24;
const uint8_t LASER_RIGHT_ADDRESS = 0x30;
const uint8_t LASER_LEFT_ADDRESS = 0x31;
const int LASER_MAX_VALID_DISTANCE_MM = 2000;
const int LASER_WALL_GONE_THRESHOLD_MM = 700;

// HC-SR04 pins, timing and calibration
const uint8_t ULTRASONIC_FRONT_TRIG_PIN = 43;
const uint8_t ULTRASONIC_FRONT_ECHO_PIN = 42;
const uint8_t ULTRASONIC_RIGHT_45_TRIG_PIN = 51;
const uint8_t ULTRASONIC_RIGHT_45_ECHO_PIN = 50;
const unsigned long ULTRASONIC_ECHO_TIMEOUT_US = 10000UL;
const unsigned long ULTRASONIC_MIN_INTERVAL_MS = 60;
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
// Anti-chatter (robot physics, not track geometry):
// Lateral speed can never exceed forward speed, so larger dErr values are
// artefacts (e.g. the beam sweeping across a corner vertex while rotating).
// Cap = forward speed at DRIVE_SPEED from the calibration fit (~124 mm/s).
const float WALL_FOLLOW_DERR_MAX_MMPS =
    10.0f * (MOTOR_SPEED_SCALE_CM_PER_SECOND_PER_PWM * DRIVE_SPEED +
             MOTOR_SPEED_OFFSET_CM_PER_SECOND);
const float WALL_FOLLOW_STEER_SLEW_PER_S = 150.0f; // max steer change per second
// Steering dead zone: |steer| below ~5 barely turns the robot (friction/caster),
// seen as parallel travel at +-4 cm offset. Added only when correcting distance.
const int STEER_DEADZONE_COMP = 4;
// Outside corner / wall lost: arc right (toward the lost wall) until it returns.
const uint8_t WALL_LOST_CONFIRM_COUNT = 3;        // consecutive gone readings before searching
const int WALL_REACQUIRE_MM = 600;               // reacquire only at/below this (hysteresis vs gone threshold)
const int WALL_SEARCH_SPEED = 40;                 // base speed while searching
const int WALL_SEARCH_STEER = 12;                 // steer magnitude toward the wall while searching
const unsigned long WALL_SEARCH_TIMEOUT_MS = 8000UL; // arc ~18 deg/s at 40/12 => 90 deg ~5 s; 8 s ~ 145 deg, then stop
// Inside corner anticipation: steer away as a wall closes in ahead.
// front: center-relative nose distance. r45: raw slant of the right-45 sensor;
// on a straight wall at 30 cm it reads ~(30-5.5)*sqrt(2) = ~35 cm.
const float CORNER_FRONT_START_CM = 60.0f; // start steering away below this
const float CORNER_FRONT_FULL_CM = 30.0f;  // full corner steer at/below this
const float CORNER_R45_START_CM = 28.0f;
const float CORNER_R45_FULL_CM = 15.0f;
const int CORNER_MAX_STEER = 25;           // away-steer magnitude at full

// Rate limit for stage decision logging (0 = log every loop iteration).
const unsigned long STAGE_LOG_INTERVAL_MS = 250UL;

// Safety stop (front obstacle) while a stage runs
const float FRONT_EMERGENCY_STOP_CM = 20.0;
const float FRONT_EMERGENCY_CLEAR_CM = 30.0;

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
