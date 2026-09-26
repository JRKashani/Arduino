#include "ADXL335_IMU.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>

namespace {
int adc[4] = {500, 500, 600, 1000};
bool alternateRoll = false;
unsigned int referenceReads = 0;
void near(float actual, float expected, float tolerance = 0.001f) {
  assert(fabs(actual - expected) < tolerance);
}
}

int analogRead(uint8_t pin) {
  if (pin == 3) ++referenceReads;
  if (pin == 1 && alternateRoll) {
    return 500 + ((referenceReads / 2) % 2 == 0 ? 1 : -1);
  }
  return adc[pin];
}
void pinMode(uint8_t, uint8_t) {}
void delay(unsigned long) {}
void delayMicroseconds(unsigned int) {}

int main() {
  const ADXL335Calibration weights = {0.5f, 0.5f, 0.5f, 0.1f, 0.1f, 0.1f};
  ADXL335_IMU imu(0, 1, 2, 3, weights);
  assert(imu.begin());
  assert(!imu.hasBaseline());
  ADXL335Reading r = imu.read();
  assert(r.valid);
  near(r.ax, 0);
  near(r.az, 1);
  near(r.robotRollDeg, 0);
  near(r.robotPitchDeg, 0);

  // Supply changes cancel out because both axis and rail are measured.
  adc[0] = 250; adc[1] = 250; adc[2] = 300; adc[3] = 500;
  near(imu.read().az, 1);

  // 45-degree tilt, mounting sign, and angle-only baseline correction.
  adc[0] = 550; adc[1] = 500; adc[2] = 550; adc[3] = 1000;
  near(imu.read().robotPitchDeg, 45);
  assert(imu.calibrateBaseline(10, 0));
  assert(imu.hasBaseline());
  r = imu.read();
  near(r.robotPitchDeg, 0);
  near(r.ax, 0.5f); // Gravity vector is not subtracted.
  near(r.az, 0.5f);

  // Invalid reference and empty calibrations preserve the last baseline.
  adc[3] = 0;
  assert(!imu.read().valid);
  assert(!imu.calibrateBaseline(10, 0));
  assert(!imu.calibrateBaseline(0, 0));
  near(imu.baselinePitchDeg(), -45);
  assert(imu.hasBaseline());

  ADXL335Calibration invalid = weights;
  invalid.xRatioPerG = 0;
  assert(!imu.setCalibration(invalid));
  assert(imu.hasBaseline());
  invalid.xRatioPerG = NAN;
  assert(!imu.setCalibration(invalid));
  ADXL335_IMU bad(0, 1, 2, 3, invalid);
  assert(!bad.begin());
  assert(!bad.read().valid);
  assert(imu.setCalibration(weights));
  assert(!imu.hasBaseline());

  // Roll averaging must remain near 180 degrees across the wrap boundary.
  adc[0] = 500; adc[1] = 500; adc[2] = 400; adc[3] = 1000;
  referenceReads = 0;
  alternateRoll = true;
  assert(imu.calibrateBaseline(10, 0));
  near(fabs(imu.baselineRollDeg()), 180);
  assert(fabs(imu.read().robotRollDeg) < 1);
  alternateRoll = false;

  adc[2] = 500; // No gravity vector: orientation is undefined.
  assert(!imu.read().valid);

  // Established empirical weights with realistic quantized Mega ADC values.
  const ADXL335Calibration established = {
    0.5143f, 0.5070f, 0.5068f, 0.1050f, 0.1020f, 0.0988f
  };
  assert(imu.setCalibration(established));
  adc[0] = 360; adc[1] = 355; adc[2] = 424; adc[3] = 700;
  r = imu.read();
  assert(r.valid);
  near(r.ax, 0, 0.01f);
  near(r.ay, 0, 0.01f);
  near(r.az, 1, 0.01f);
  puts("PASS: conversion, supply normalization, tilt, baseline, wrap, invalid inputs, established weights");
}
