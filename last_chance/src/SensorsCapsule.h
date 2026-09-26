#ifndef SENSORS_CAPSULE_H
#define SENSORS_CAPSULE_H

#include <Arduino.h>

// Unified, filtered sensor readings for the last_chance robot.
//
// Distances are CENTER-RELATIVE where the geometry is a simple offset:
//   - side lasers report distance from the robot center to the wall
//     (raw laser reading + LASER_LATERAL_OFFSET_CM),
//   - the nose ultrasonic reports distance from the center to the obstacle
//     ahead (raw reading + ULTRASONIC_FRONT_FORWARD_OFFSET_CM).
// The right-45 degree ultrasonic is reported as its raw slant distance: it is
// a forward-right corner/obstacle detector, not a clean perpendicular range,
// so no center conversion is applied here.
//
// Each subsystem carries a validity flag; check it before trusting a value.
struct SensorData {
  unsigned long timestampMs;

  // VL53L0X side lasers (cm, center-relative to the wall).
  float leftWallCm;
  float rightWallCm;
  bool  leftWallValid;
  bool  rightWallValid;
  int   leftRawMm;   // raw sensor value for diagnostics
  int   rightRawMm;

  // HC-SR04 ultrasonics.
  float frontCm;         // center-relative distance ahead
  float rightDiagCm;     // raw slant distance from the 45 deg sensor
  bool  frontValid;
  bool  rightDiagValid;
  float frontRawCm;      // raw (uncorrected-for-offset) readings for diagnostics
  float rightDiagRawCm;

  // LDR light sensors (raw ADC 0..1023) and left-minus-right difference.
  int ldrLeft;
  int ldrRight;
  int ldrDiff;
};

// Bring up I2C, sequence the two VL53L0X through XSHUT and assign their
// addresses, and configure ultrasonic/LDR pins. Returns false if either laser
// failed to initialise (diagnostics are printed to Serial); the capsule still
// runs and simply marks the failed sensor invalid.
bool setupSensors();

// Call frequently from loop(). Reads the lasers and LDRs each call and pings
// one ultrasonic per scheduled slot (round-robin, honouring the DEFINE.h
// timing), updating the filtered SensorData.
void updateSensors();

const SensorData &latestSensorData();

// Live serial dashboard, off by default. Toggle with toggleSensorTelemetry();
// when enabled, updateSensors() prints a line every SENSOR_TELEMETRY_INTERVAL_MS.
void toggleSensorTelemetry();
bool sensorTelemetryEnabled();

#endif
