
#pragma once

#include <Arduino.h>
#include <Ultrasonic.h>
#include <SteeringDualH.h>

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

extern SteeringDualH car;

extern int slow;
extern int fast;
extern int veryFast;

extern int tolerance_US;
extern int tolerance_PhotoResistor;
extern float K;

struct SensorReading
{
    uint32_t timestamp;

    // Distances in cm
    uint8_t US60;
    uint8_t US120;
    uint8_t US240;
    uint8_t US300;
/*
    // Distances in mm
    uint8_t laserLeft;
    uint8_t laserRight;

    // Raw ADC values: 0-1023
    uint16_t photoLeft;
    uint16_t photoRight;

    // Distances in mm
    uint16_t IR1;
    uint16_t IR2;
    uint16_t IR3;

    // IMU
    float accelX;
    float accelY;
    float accelZ;

    float gyroX;
    float gyroY;
    float gyroZ;
*/
    // Motor commands
    int8_t motorLeftDirection;
    uint8_t motorLeftSpeed;

    int8_t motorRightDirection;
    uint8_t motorRightSpeed;
};

extern SensorReading currentData;