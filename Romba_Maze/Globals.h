#pragma once

#include <Arduino.h>
#include <Ultrasonic.h>
#include <SteeringDualH.h>
#include <VL53L0X.h>

struct SensorReading
{
    uint32_t timestamp;

    // HC-SR04: centimetres
    float US60_cm;
    float US120_cm;
    float US240_cm;
    float US300_cm;

    bool US60_valid;
    bool US120_valid;
    bool US240_valid;
    bool US300_valid;

    // VL53L0X: millimetres
    float laserLeft_mm;
    float laserRight_mm;

    bool laserLeft_valid;
    bool laserRight_valid;

    // Sharp GP2Y0A21YK
    // Keep both raw ADC and approximate distance.
    float IRLeft_adc;
    float IRRight_adc;

    float IRLeft_cm;
    float IRRight_cm;

    bool IRLeft_valid;
    bool IRRight_valid;

    // LDR: raw ADC only
    float photoLeft_adc;
    float photoRight_adc;

    // ADXL335: acceleration in g
    float accelX_g;
    float accelY_g;
    float accelZ_g;
};

extern SensorReading currentData;

extern Ultrasonic US60;
extern Ultrasonic US120;
extern Ultrasonic US240;
extern Ultrasonic US300;

extern const int redLedPin;
extern const int greenLedPin;

/*
extern const int trigPin60;
extern const int echoPin60;

extern const int trigPin120;
extern const int echoPin120;

extern const int trigPin240;
extern const int echoPin240;

extern const int trigPin300;
extern const int echoPin300;
*/

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

extern const uint8_t US240_TRIG_PIN;
extern const uint8_t US240_ECHO_PIN;

extern const uint8_t US300_TRIG_PIN;
extern const uint8_t US300_ECHO_PIN;


// ============================================================
// VL53L0X
// Mega hardware I2C:
// SDA = 20
// SCL = 21
// ============================================================

extern const uint8_t LASER_LEFT_XSHUT_PIN;
extern const uint8_t LASER_RIGHT_XSHUT_PIN;


// ============================================================
// SHARP IR
// ============================================================

extern const uint8_t IR_LEFT_PIN;
extern const uint8_t IR_RIGHT_PIN;


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
// BUTTON
// ============================================================

extern const uint8_t BUTTON_PIN;

extern VL53L0X laserLeft;
extern VL53L0X laserRight;

extern const uint8_t IRLeftPin;
extern const uint8_t IRRightPin;

extern const uint8_t photoLeftPin;
extern const uint8_t photoRightPin;

extern const uint8_t accelXPin;
extern const uint8_t accelYPin;
extern const uint8_t accelZPin;

extern SteeringDualH car;

extern int slow;
extern int fast;
extern int veryFast;

extern int tolerance_US;
extern int tolerance_PhotoResistor;
extern float K;