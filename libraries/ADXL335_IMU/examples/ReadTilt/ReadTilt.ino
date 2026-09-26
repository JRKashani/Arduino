#include <ADXL335_IMU.h>

// Established weights; change these for a different sensor calibration.
const ADXL335Calibration weights = {
  0.5143f, 0.5070f, 0.5068f,
  0.1050f, 0.1020f, 0.0988f
};
ADXL335_IMU imu(A2, A1, A0, A5, weights);
bool ready = false;

void setup() {
  Serial.begin(115200);
  Serial.println(F("Keep robot flat and still for baseline calibration."));
  ready = imu.begin() && imu.calibrateBaseline(100, 10);
  if (!ready) Serial.println(F("IMU setup failed; check wiring and weights."));
}

void loop() {
  if (!ready) return;
  const ADXL335Reading r = imu.read();
  if (r.valid) {
    Serial.print(F("Roll: "));
    Serial.print(r.robotRollDeg, 2);
    Serial.print(F(" Pitch: "));
    Serial.println(r.robotPitchDeg, 2);
  } else {
    Serial.println(F("Invalid IMU reading"));
  }
  delay(200);
}
