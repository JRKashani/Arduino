#ifndef ADXL335_IMU_H
#define ADXL335_IMU_H

#include <Arduino.h>

// Axis voltage / measured 3.3 V rail. Supply all six measured weights.
struct ADXL335Calibration {
  float xZeroRatio;
  float yZeroRatio;
  float zZeroRatio;
  float xRatioPerG;
  float yRatioPerG;
  float zRatioPerG;
};

struct ADXL335Reading {
  bool valid;
  int rawX, rawY, rawZ, raw3V3;
  float ax, ay, az;  // Acceleration in g, including gravity.
  float magnitude;
  float rawRollDeg, rawPitchDeg;
  float correctedRollDeg, correctedPitchDeg;
  float robotRollDeg, robotPitchDeg;
};

class ADXL335_IMU {
public:
  ADXL335_IMU(uint8_t xPin, uint8_t yPin, uint8_t zPin,
              uint8_t referencePin, const ADXL335Calibration &calibration);

  bool begin();
  ADXL335Reading read();
  // Blocks while sampling. Keep the robot flat and still.
  // Invalid samples are skipped; failure preserves the previous baseline.
  bool calibrateBaseline(uint16_t samples, unsigned long sampleDelayMs);
  bool setCalibration(const ADXL335Calibration &calibration);
  void clearBaseline();
  bool hasBaseline() const { return _hasBaseline; }
  float baselineRollDeg() const { return _baselineRollDeg; }
  float baselinePitchDeg() const { return _baselinePitchDeg; }

private:
  uint8_t _xPin, _yPin, _zPin, _referencePin;
  ADXL335Calibration _calibration;
  float _baselineRollDeg, _baselinePitchDeg;
  bool _hasBaseline;

  static bool validCalibration(const ADXL335Calibration &calibration);
  static int readSettledADC(uint8_t pin);
  static float wrapAngle180(float angle);
};

#endif
