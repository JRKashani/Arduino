#include "ImuCapsule.h"
#include "DEFINE.h"

namespace {
const ADXL335Calibration weights = {
  X_ZERO_RATIO, Y_ZERO_RATIO, Z_ZERO_RATIO,
  X_RATIO_PER_G, Y_RATIO_PER_G, Z_RATIO_PER_G
};
ADXL335_IMU imu(ACCEL_X_PIN, ACCEL_Y_PIN, ACCEL_Z_PIN,
               ACCEL_REF_3V3_PIN, weights);
ADXL335Reading latest = {};
bool ready = false;
unsigned long lastPrintMs = 0;
}

bool setupImuCapsule() {
  ready = false;
  latest = ADXL335Reading{};
  if (!imu.begin()) {
    Serial.println(F("IMU: invalid calibration weights in DEFINE.h."));
    return false;
  }
  Serial.println(F("IMU: keep robot flat and still; calibrating baseline."));
  ready = imu.calibrateBaseline(ACCEL_CALIBRATION_SAMPLES,
                                ACCEL_CALIBRATION_SAMPLE_DELAY_MS);
  if (!ready) {
    Serial.println(F("IMU: baseline failed. Check A5/3.3V and sensor wiring; reset to retry."));
    return false;
  }
  Serial.print(F("IMU baseline roll="));
  Serial.print(imu.baselineRollDeg(), 2);
  Serial.print(F(" pitch="));
  Serial.println(imu.baselinePitchDeg(), 2);
  lastPrintMs = millis();
  return true;
}

void runImuCapsule() {
  if (!ready) return;
  latest = imu.read();
  if (!IMU_PRINT_ENABLED) return;
  const unsigned long now = millis();
  if (now - lastPrintMs < IMU_PRINT_INTERVAL_MS) return;
  lastPrintMs = now;

  Serial.print(F("ADC X/Y/Z/REF="));
  Serial.print(latest.rawX);
  Serial.print('/');
  Serial.print(latest.rawY);
  Serial.print('/');
  Serial.print(latest.rawZ);
  Serial.print('/');
  Serial.print(latest.raw3V3);
  if (!latest.valid) {
    Serial.println(F(" | IMU invalid reading"));
    return;
  }
  Serial.print(F(" | g X/Y/Z="));
  Serial.print(latest.ax, 3);
  Serial.print('/');
  Serial.print(latest.ay, 3);
  Serial.print('/');
  Serial.print(latest.az, 3);
  Serial.print(F(" | magnitude="));
  Serial.print(latest.magnitude, 3);
  Serial.print(F(" | robot roll="));
  Serial.print(latest.robotRollDeg, 2);
  Serial.print(F(" pitch="));
  Serial.println(latest.robotPitchDeg, 2);
}

const ADXL335Reading &latestImuReading() {
  return latest;
}
