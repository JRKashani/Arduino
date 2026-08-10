#pragma once

#include "Globals.h"

// Initialize I2C and the two VL53L0X sensors.
// Returns false if either VL53L0X fails to initialize.
bool setupSensors();

// Main acquisition entry point.
// It clears currentData, samples the sensors selected for the
// supplied stage, stores the results in currentData, and returns
// a const reference to currentData.
const SensorReading& readSensors(uint8_t stage);

// Lower-level acquisition functions are exposed so you can later
// build a different sampling schedule without rewriting drivers.
void clearSensorReading(SensorReading &data);
void readUltrasonics(SensorReading &data);
void readLasers(SensorReading &data);
void readAccelerometer(SensorReading &data);
void readLightSensors(SensorReading &data);