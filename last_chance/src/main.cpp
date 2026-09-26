#include <Arduino.h>
#include "DEFINE.h"
#include "ImuCapsule.h"
#include "MotionCapsule.h"
#include "MotionTest.h"
#include "SensorsCapsule.h"
#include "StageManager.h"

void setup() {
  Serial.begin(115200);
  setupMotionCapsule(); // Stop motors before the stationary IMU calibration.
  setupSensors();       // I2C + VL53L0X addressing, ultrasonic and LDR pins.
  setupImuCapsule();
  printMotionTestMenu();
}

void loop() {
  updateSensors();    // refresh the filtered sensor snapshot every loop.
  runMotionTest();    // serial console: timed test moves + stage start/stop.
  runStageManager();  // drives the motors when a stage is active (else no-op).
  runImuCapsule();
  // Add each mission stage as its own function when we develop it.
}
