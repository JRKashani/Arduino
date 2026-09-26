#include "DEFINE.h"
#include "ImuCapsule.h"
#include "MotionCapsule.h"
#include "MotionTest.h"

void setup() {
  Serial.begin(115200);
  setupMotionCapsule(); // Stop motors before the stationary IMU calibration.
  setupImuCapsule();
  printMotionTestMenu();
}

void loop() {
  runMotionTest();
  runImuCapsule();
  runMotionCapsule();
  // Add each mission stage as its own function when we develop it.
}
