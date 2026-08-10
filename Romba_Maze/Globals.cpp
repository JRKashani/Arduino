#include "Globals.h"

// ============================================================
// MOTOR PINS
// ============================================================

const uint8_t LEFT_DIR_1  = 46;
const uint8_t LEFT_DIR_2  = 48;
const uint8_t LEFT_PWM    = 44;

const uint8_t RIGHT_DIR_1 = 49;
const uint8_t RIGHT_DIR_2 = 47;
const uint8_t RIGHT_PWM   = 45;


// ============================================================
// HC-SR04 ULTRASONIC SENSORS
// ============================================================

const uint8_t US60_TRIG_PIN  = 2;
const uint8_t US60_ECHO_PIN  = 3;

const uint8_t US120_TRIG_PIN = 4;
const uint8_t US120_ECHO_PIN = 5;

const uint8_t US300_TRIG_PIN = 6;
const uint8_t US300_ECHO_PIN = 7;

Ultrasonic US60(US60_TRIG_PIN, US60_ECHO_PIN);
Ultrasonic US120(US120_TRIG_PIN, US120_ECHO_PIN);
Ultrasonic US300(US300_TRIG_PIN, US300_ECHO_PIN);

// ============================================================
// VL53L0X
// Mega hardware I2C: SDA = 20, SCL = 21
// ============================================================

const uint8_t LASER_FRONT_XSHUT_PIN = 33;
const uint8_t LASER_RIGHT_XSHUT_PIN = 31;
const uint8_t LASER_LEFT_XSHUT_PIN = 30;

VL53L0X laserFront;
VL53L0X laserRight;
VL53L0X laserLeft;

float laserLeftDistance = 0;
float laserRightDistance = 0;
float laserFrontDistance = 0;


// ============================================================
// SHARP IR - NOT CURRENTLY USED
// ============================================================

/*
const uint8_t IR_LEFT_PIN  = A3;
const uint8_t IR_RIGHT_PIN = A4;
*/


// ============================================================
// LDR
// ============================================================

const uint8_t LDR_LEFT_PIN  = A7;
const uint8_t LDR_RIGHT_PIN = A6;


// ============================================================
// ADXL335
// ============================================================

const uint8_t ACCEL_X_PIN = A0;
const uint8_t ACCEL_Y_PIN = A1;
const uint8_t ACCEL_Z_PIN = A2;


// ============================================================
// OTHER PROJECT GLOBALS
// ============================================================

const int whiteLedPin = 12;
const int redLedPin   = 13;
const int greenLedPin  = 10;

int right_or_left = 0;   // -1 for left, 1 for right

SteeringDualH car;

int slow     = 50;
int fast     = 80;
int veryFast = 150;

int tolerance_US            = 30;
int tolerance_PhotoResistor = 80;
float K = 1.0f;

const int targetDistanceWall  = 30;
const int targetDistanceLight = 12;

SensorReading currentData;