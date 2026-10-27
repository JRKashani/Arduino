// Manual drive logger: the PC steers the robot (tools/manual_drive) while every
// sensor update is streamed as an S record. Uses the navigation sensor layer
// unchanged, so the recorded values are exactly what the stages will see.
#include <Arduino.h>
#include <ManualCommand.h>
#include "../DEFINE.h"
#include "../ImuCapsule.h"
#include "../MotionCapsule.h"
#include "../SensorsCapsule.h"
#include "ManualDrive.h"
#include "ManualLink.h"
#include "SampleLog.h"

namespace {
void printConfig(const __FlashStringHelper *name, float value, uint8_t digits) {
  Serial.print(F("CONFIG,")); Serial.print(name); Serial.print(',');
  Serial.println(value, digits);
}

void printInfo() {
  Serial.println(F("MANUAL,1,1.0.0"));
  printConfig(F("LASER_SCALE"), LASER_DISTANCE_SCALE, 9);
  printConfig(F("LASER_OFFSET_CM"), LASER_DISTANCE_OFFSET_CM, 9);
  printConfig(F("LASER_LATERAL_OFFSET_CM"), LASER_LATERAL_OFFSET_CM, 2);
  printConfig(F("US_SCALE"), ULTRASONIC_DISTANCE_SCALE, 6);
  printConfig(F("US_OFFSET_CM"), ULTRASONIC_DISTANCE_OFFSET_CM, 6);
  printConfig(F("US_TIMEOUT_US"), ULTRASONIC_ECHO_TIMEOUT_US, 0);
  printConfig(F("US_FRONT_FORWARD_OFFSET_CM"), ULTRASONIC_FRONT_FORWARD_OFFSET_CM, 2);
  printConfig(F("FRONT_STOP_CM"), FRONT_EMERGENCY_STOP_CM, 1);
  printConfig(F("WATCHDOG_MS"), MANUAL_WATCHDOG_MS, 0);
  printConfig(F("MAX_SPEED"), MANUAL_MAX_SPEED, 0);
  printConfig(F("MAX_STEER"), MANUAL_MAX_STEER, 0);
  Serial.print(F("FIELDS,")); Serial.println(SAMPLE_FIELDS);
  Serial.println(F("READY"));
}

void onLine(const char *line) {
  const ManualCommand cmd = parseManualCommand(line, MANUAL_MAX_SPEED, MANUAL_MAX_STEER);
  if (cmd.type == ManualCommandType::Info) printInfo();
  else if (cmd.type == ManualCommandType::Invalid) Serial.println(F("ERROR,COMMAND"));
  else handleManualCommand(cmd);
}
} // namespace

void setup() {
  Serial.begin(115200);
  setupMotionCapsule(); // motors stopped before anything else
  setupManualDrive();
  Serial.println(F("BOOT,MANUAL,1"));
  setupSensors();
  setupImuCapsule();    // robot flat and still: zeroes roll/pitch
  printInfo();
}

void loop() {
  pollManualLink(onLine);
  updateSensors();
  updateManualDrive(latestSensorData());
  runImuCapsule();
  printSample(latestSensorData(), latestImuReading(), manualDriveStatus());
}
