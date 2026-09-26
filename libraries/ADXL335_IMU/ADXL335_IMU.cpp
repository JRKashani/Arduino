#include "ADXL335_IMU.h"
#include <math.h>

ADXL335_IMU::ADXL335_IMU(uint8_t xPin, uint8_t yPin, uint8_t zPin,
                         uint8_t referencePin,
                         const ADXL335Calibration &calibration)
    : _xPin(xPin), _yPin(yPin), _zPin(zPin), _referencePin(referencePin),
      _calibration(calibration), _baselineRollDeg(0.0f),
      _baselinePitchDeg(0.0f), _hasBaseline(false) {}

bool ADXL335_IMU::validCalibration(const ADXL335Calibration &c) {
  return isfinite(c.xZeroRatio) && isfinite(c.yZeroRatio) &&
         isfinite(c.zZeroRatio) && isfinite(c.xRatioPerG) &&
         isfinite(c.yRatioPerG) && isfinite(c.zRatioPerG) &&
         c.xRatioPerG > 0.0f && c.yRatioPerG > 0.0f && c.zRatioPerG > 0.0f;
}

bool ADXL335_IMU::begin() {
  pinMode(_xPin, INPUT);
  pinMode(_yPin, INPUT);
  pinMode(_zPin, INPUT);
  pinMode(_referencePin, INPUT);
  clearBaseline();
  return validCalibration(_calibration);
}

bool ADXL335_IMU::setCalibration(const ADXL335Calibration &calibration) {
  if (!validCalibration(calibration)) {
    return false;
  }
  _calibration = calibration;
  clearBaseline();  // The previous baseline used different conversion weights.
  return true;
}

void ADXL335_IMU::clearBaseline() {
  _baselineRollDeg = 0.0f;
  _baselinePitchDeg = 0.0f;
  _hasBaseline = false;
}

int ADXL335_IMU::readSettledADC(uint8_t pin) {
  analogRead(pin);  // Discard the first conversion after switching channels.
  delayMicroseconds(20);
  return analogRead(pin);
}

float ADXL335_IMU::wrapAngle180(float angle) {
  while (angle > 180.0f) angle -= 360.0f;
  while (angle < -180.0f) angle += 360.0f;
  return angle;
}

ADXL335Reading ADXL335_IMU::read() {
  ADXL335Reading r = {};
  if (!validCalibration(_calibration)) return r;

  r.rawX = readSettledADC(_xPin);
  r.rawY = readSettledADC(_yPin);
  r.rawZ = readSettledADC(_zPin);
  r.raw3V3 = readSettledADC(_referencePin);
  if (r.raw3V3 <= 0) return r;

  r.ax = (float(r.rawX) / r.raw3V3 - _calibration.xZeroRatio) /
         _calibration.xRatioPerG;
  r.ay = (float(r.rawY) / r.raw3V3 - _calibration.yZeroRatio) /
         _calibration.yRatioPerG;
  r.az = (float(r.rawZ) / r.raw3V3 - _calibration.zZeroRatio) /
         _calibration.zRatioPerG;
  r.magnitude = sqrt(r.ax * r.ax + r.ay * r.ay + r.az * r.az);
  if (!isfinite(r.magnitude) || r.magnitude <= 0.0f) return r;

  r.rawRollDeg = atan2(r.ay, r.az) * 180.0f / PI;
  r.rawPitchDeg = atan2(-r.ax, sqrt(r.ay * r.ay + r.az * r.az)) * 180.0f / PI;
  r.correctedRollDeg = wrapAngle180(r.rawRollDeg - _baselineRollDeg);
  r.correctedPitchDeg = r.rawPitchDeg - _baselinePitchDeg;
  // Preserve the established robot mounting convention.
  r.robotRollDeg = r.correctedRollDeg;
  r.robotPitchDeg = -r.correctedPitchDeg;
  r.valid = true;
  return r;
}

bool ADXL335_IMU::calibrateBaseline(uint16_t samples,
                                    unsigned long sampleDelayMs) {
  if (samples == 0 || !validCalibration(_calibration)) return false;
  float rollSinSum = 0.0f;
  float rollCosSum = 0.0f;
  float pitchSum = 0.0f;
  uint16_t validSamples = 0;

  for (uint16_t i = 0; i < samples; ++i) {
    const ADXL335Reading r = read();
    if (r.valid) {
      const float rollRad = r.rawRollDeg * PI / 180.0f;
      rollSinSum += sin(rollRad);
      rollCosSum += cos(rollRad);
      pitchSum += r.rawPitchDeg;
      ++validSamples;
    }
    delay(sampleDelayMs);
  }
  if (validSamples == 0) return false;

  // Circular averaging handles roll near the -180/+180 degree boundary.
  _baselineRollDeg = atan2(rollSinSum, rollCosSum) * 180.0f / PI;
  _baselinePitchDeg = pitchSum / validSamples;
  _hasBaseline = true;
  return true;
}
