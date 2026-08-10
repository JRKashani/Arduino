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

extern const int trigPin60;
extern const int echoPin60;

extern const int trigPin120;
extern const int echoPin120;

extern const int trigPin240;
extern const int echoPin240;

extern const int trigPin300;
extern const int echoPin300;

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