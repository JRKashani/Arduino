#pragma once

#include <Arduino.h>
#include <Ultrasonic.h>
#include <SteeringDualH.h>
#include <VL53L0X.h>

// ============================================================
// SENSOR DATA
// ============================================================

struct SensorReading
{
    uint32_t timestamp;

    // HC-SR04: centimetres
    float US60_cm;
    float US120_cm;
    float US300_cm;

    bool US60_valid;
    bool US120_valid;
    bool US300_valid;

    // VL53L0X: millimetres
    float laserFront_mm;
    float laserRight_mm;
    float laserLeft_mm;

    bool laserFront_valid;
    bool laserRight_valid;
    bool laserLeft_valid;

    // --------------------------------------------------------
    // Sharp GP2Y0A21YK IR sensors - NOT CURRENTLY USED.
    // Kept here commented out so they can be restored later.
    // --------------------------------------------------------
    /*
    float IRLeft_adc;
    float IRRight_adc;

    float IRLeft_cm;
    float IRRight_cm;

    bool IRLeft_valid;
    bool IRRight_valid;
    */

    // LDR: raw ADC
    float photoLeft_adc;
    float photoRight_adc;

    // ADXL335: acceleration in g
    // Conversion currently uses provisional calibration values
    // in Sensors.cpp. Recalibrate on the actual robot later.
    float accelX_g;
    float accelY_g;
    float accelZ_g;
};

extern SensorReading currentData;


// ============================================================
// STAGES
// ============================================================

enum RobotStage : uint8_t
{
    STAGE_OPEN_AREA = 1,
    STAGE_FUNNEL = 2,
    STAGE_WALL_FOLLOWING = 3,
    STAGE_WADI = 4,
    STAGE_LIGHT = 5
};


// ============================================================
// MOTOR PINS
// ============================================================

extern const uint8_t LEFT_DIR_1;
extern const uint8_t LEFT_DIR_2;
extern const uint8_t LEFT_PWM;

extern const uint8_t RIGHT_DIR_1;
extern const uint8_t RIGHT_DIR_2;
extern const uint8_t RIGHT_PWM;


// ============================================================
// HC-SR04 ULTRASONIC SENSORS
// ============================================================

extern const uint8_t US60_TRIG_PIN;
extern const uint8_t US60_ECHO_PIN;

extern const uint8_t US120_TRIG_PIN;
extern const uint8_t US120_ECHO_PIN;

extern const uint8_t US300_TRIG_PIN;
extern const uint8_t US300_ECHO_PIN;

extern Ultrasonic US60;
extern Ultrasonic US120;
extern Ultrasonic US300;


// ============================================================
// VL53L0X
// Mega hardware I2C: SDA = 20, SCL = 21
// ============================================================

extern const uint8_t LASER_FRONT_XSHUT_PIN;
extern const uint8_t LASER_RIGHT_XSHUT_PIN;
extern const uint8_t LASER_LEFT_XSHUT_PIN;

extern VL53L0X laserFront;
extern VL53L0X laserRight;
extern VL53L0X laserLeft;

extern float laserLeftDistance;
extern float laserRightDistance;
extern float laserFrontDistance;


// ============================================================
// SHARP IR - NOT CURRENTLY USED
// ============================================================

/*
extern const uint8_t IR_LEFT_PIN;
extern const uint8_t IR_RIGHT_PIN;
*/


// ============================================================
// LDR
// ============================================================

extern const uint8_t LDR_LEFT_PIN;
extern const uint8_t LDR_RIGHT_PIN;


// ============================================================
// ADXL335
// ============================================================

extern const uint8_t ACCEL_X_PIN;
extern const uint8_t ACCEL_Y_PIN;
extern const uint8_t ACCEL_Z_PIN;


// ============================================================
// OTHER PROJECT GLOBALS
// ============================================================

extern const int whiteLedPin;
extern const int redLedPin;
extern const int greenLedPin;

extern int right_or_left;   // -1 for left, 1 for right

extern SteeringDualH car;

extern int slow;
extern int fast;
extern int veryFast;

extern int tolerance_US;
extern int tolerance_PhotoResistor;
extern float K;

extern const int targetDistanceWall;
extern const int targetDistanceLight;