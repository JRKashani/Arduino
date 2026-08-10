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

const uint8_t US60_TRIG_PIN  = 22;
const uint8_t US60_ECHO_PIN  = 23;

const uint8_t US120_TRIG_PIN = 24;
const uint8_t US120_ECHO_PIN = 25;

const uint8_t US240_TRIG_PIN = 26;
const uint8_t US240_ECHO_PIN = 27;

const uint8_t US300_TRIG_PIN = 28;
const uint8_t US300_ECHO_PIN = 29;


// ============================================================
// VL53L0X
// Mega hardware I2C:
// SDA = 20
// SCL = 21
// ============================================================

const uint8_t LASER_LEFT_XSHUT_PIN  = 30;
const uint8_t LASER_RIGHT_XSHUT_PIN = 31;


// ============================================================
// SHARP IR
// ============================================================

const uint8_t IR_LEFT_PIN  = A0;
const uint8_t IR_RIGHT_PIN = A1;


// ============================================================
// LDR
// ============================================================

const uint8_t LDR_LEFT_PIN  = A2;
const uint8_t LDR_RIGHT_PIN = A3;


// ============================================================
// ADXL335
// ============================================================

const uint8_t ACCEL_X_PIN = A4;
const uint8_t ACCEL_Y_PIN = A5;
const uint8_t ACCEL_Z_PIN = A6;


// ============================================================
// BUTTON
// ============================================================

const uint8_t BUTTON_PIN = 2;

  const int redLedPin   =  11;
  const int greenLedPin =  10;
/*
  const int trigPin60   =  13;
  const int echoPin60   =  12;

  const int trigPin120  =  6;
  const int echoPin120  =  7;

  const int trigPin240  =  3;
  const int echoPin240  =  2;

  const int trigPin300  =  8;
  const int echoPin300  =  9;
*/

  Ultrasonic US60(US60_TRIG_PIN, US60_ECHO_PIN);
  Ultrasonic US120(US120_TRIG_PIN, US120_ECHO_PIN);
  Ultrasonic US240(US240_TRIG_PIN, US240_ECHO_PIN);
  Ultrasonic US300(US300_TRIG_PIN, US300_ECHO_PIN);

  SteeringDualH car;

  VL53L0X laserLeft;
  VL53L0X laserRight;

  int slow       =  50;
  int fast       =  80;
  int veryFast   = 150;

  int tolerance_US            = 30;
  int tolerance_PhotoResistor = 80;
  float K = 1;

  SensorReading currentData;