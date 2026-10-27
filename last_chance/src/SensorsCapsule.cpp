#include "SensorsCapsule.h"
#include "DEFINE.h"
#include <Wire.h>
#include <VL53L0X.h>

namespace {

// --- Small median filter (capacity 5, matches the *_FILTER_SIZE defaults) ---
class MedianFilter {
public:
  void reset() { _count = 0; _head = 0; }
  void push(float value) {
    _buf[_head] = value;
    _head = (_head + 1) % CAP;
    if (_count < CAP) _count++;
  }
  bool ready() const { return _count > 0; }
  float median() const {
    float tmp[CAP];
    for (uint8_t i = 0; i < _count; i++) tmp[i] = _buf[i];
    // insertion sort (tiny N)
    for (uint8_t i = 1; i < _count; i++) {
      float key = tmp[i];
      int j = i - 1;
      while (j >= 0 && tmp[j] > key) { tmp[j + 1] = tmp[j]; j--; }
      tmp[j + 1] = key;
    }
    return tmp[_count / 2];
  }
private:
  static const uint8_t CAP = 5;
  float _buf[CAP];
  uint8_t _count = 0;
  uint8_t _head = 0;
};

// --- Devices ---
VL53L0X laserLeft;
VL53L0X laserRight;
bool leftLaserOk = false;
bool rightLaserOk = false;

MedianFilter leftLaserFilter, rightLaserFilter;
MedianFilter frontUsFilter, rightDiagUsFilter;

SensorData data = {};

// Ultrasonic round-robin scheduling.
unsigned long lastPingMs = 0;
uint8_t nextUltrasonic = 0; // 0 = front, 1 = right-45

// Telemetry.
bool telemetryOn = false;
bool stageSensorLogging = false;
unsigned long lastTelemetryMs = 0;

// Enable a VL53L0X: hold XSHUT low to shut down, then release to high-Z so the
// breakout's pull-up raises it to the sensor's 2.8 V logic (never drive it HIGH
// from a 5 V pin). Init at the default 0x29, then move it to a unique address.
// Same ranging profile as the calibration experiment (see DEFINE.h).
bool applyLaserProfile(VL53L0X &dev) {
  return dev.setSignalRateLimit(LASER_SIGNAL_RATE_LIMIT_MCPS) &&
         dev.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, LASER_VCSEL_PRE_RANGE_PCLKS) &&
         dev.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, LASER_VCSEL_FINAL_RANGE_PCLKS) &&
         dev.setMeasurementTimingBudget(LASER_TIMING_BUDGET_US);
}

bool bringUpLaser(VL53L0X &dev, uint8_t xshutPin, uint8_t address) {
  pinMode(xshutPin, INPUT); // release -> enabled
  delay(10);
  dev.setTimeout(500);
  if (!dev.init()) return false;
  dev.setAddress(address);
  if (!applyLaserProfile(dev)) return false;
  dev.startContinuous();
  return true;
}

// Calibrated sensor-face cm, then shifted to the robot center.
float laserCenterCm(float reportedMm) {
  const float faceCm = LASER_DISTANCE_SCALE * (reportedMm / 10.0f) + LASER_DISTANCE_OFFSET_CM;
  return faceCm + LASER_LATERAL_OFFSET_CM;
}

float readUltrasonicCm(uint8_t trigPin, uint8_t echoPin, bool &valid) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  unsigned long duration = pulseIn(echoPin, HIGH, ULTRASONIC_ECHO_TIMEOUT_US);
  if (duration == 0) { valid = false; return 0.0f; } // no echo within timeout
  float rawCm = duration / 58.0f; // ~29 us per cm, round trip
  valid = (rawCm >= 2.0f && rawCm <= 400.0f);
  return rawCm * ULTRASONIC_DISTANCE_SCALE + ULTRASONIC_DISTANCE_OFFSET_CM;
}

void readLaser(VL53L0X &dev, bool ok, MedianFilter &filter,
               float &outCm, bool &outValid, int &outRawMm) {
  if (!ok) { outValid = false; outRawMm = -1; return; }
  uint16_t mm = dev.readRangeContinuousMillimeters();
  outRawMm = mm;
  bool valid = !dev.timeoutOccurred() && mm > 0 && mm <= LASER_MAX_VALID_DISTANCE_MM;
  if (valid) filter.push((float)mm);
  outValid = valid && filter.ready();
  // Filtered raw mm -> calibrated, center-relative cm (laser sits toward the wall).
  outCm = outValid ? laserCenterCm(filter.median()) : 0.0f;
}

void printField(const char *label, float value, bool valid, const char *unit) {
  Serial.print(label);
  if (valid) { Serial.print(value, 1); Serial.print(unit); }
  else Serial.print(F("--"));
  Serial.print(' ');
}

void printTelemetry(bool frontFresh, bool rightDiagFresh) {
  Serial.print(F("SENS t=")); Serial.print(data.timestampMs);
  Serial.print(F("ms "));
  printField("Lwall=", data.leftWallCm, data.leftWallValid, "cm");
  printField("Rwall=", data.rightWallCm, data.rightWallValid, "cm");
  Serial.print(F("(raw L/R mm="));
  Serial.print(data.leftRawMm); Serial.print('/'); Serial.print(data.rightRawMm);
  Serial.print(F(") "));
  printField("Front=", data.frontCm, data.frontValid, "cm");
  printField("R45=", data.rightDiagCm, data.rightDiagValid, "cm");
  printField("FrontSample=", data.frontRawCm, data.frontValid, "cm");
  printField("R45Sample=", data.rightDiagRawCm, data.rightDiagValid, "cm");
  Serial.print(F("USnew="));
  Serial.print(frontFresh ? F("front") : (rightDiagFresh ? F("R45") : F("none")));
  Serial.print(' ');
  Serial.print(F("LDR L/R/diff="));
  Serial.print(data.ldrLeft); Serial.print('/');
  Serial.print(data.ldrRight); Serial.print('/');
  Serial.println(data.ldrDiff);
}

} // namespace

bool setupSensors() {
  // Ultrasonic pins.
  pinMode(ULTRASONIC_FRONT_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_FRONT_ECHO_PIN, INPUT);
  pinMode(ULTRASONIC_RIGHT_45_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_RIGHT_45_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_FRONT_TRIG_PIN, LOW);
  digitalWrite(ULTRASONIC_RIGHT_45_TRIG_PIN, LOW);

  // Shut down both lasers before bringing them up one at a time.
  pinMode(XSHUT_RIGHT_PIN, OUTPUT);
  pinMode(XSHUT_LEFT_PIN, OUTPUT);
  digitalWrite(XSHUT_RIGHT_PIN, LOW);
  digitalWrite(XSHUT_LEFT_PIN, LOW);
  delay(10);

  Wire.begin();

  rightLaserOk = bringUpLaser(laserRight, XSHUT_RIGHT_PIN, LASER_RIGHT_ADDRESS);
  if (!rightLaserOk)
    Serial.println(F("SENS: right VL53L0X init failed (check wiring/XSHUT 22, I2C pull-ups)."));
  leftLaserOk = bringUpLaser(laserLeft, XSHUT_LEFT_PIN, LASER_LEFT_ADDRESS);
  if (!leftLaserOk)
    Serial.println(F("SENS: left VL53L0X init failed (check wiring/XSHUT 24, I2C pull-ups)."));

  lastPingMs = millis();
  return rightLaserOk && leftLaserOk;
}

void updateSensors() {
  const unsigned long now = millis();
  bool &frontFresh = data.frontFresh, &rightDiagFresh = data.rightDiagFresh;
  frontFresh = rightDiagFresh = false;
  data.timestampMs = now;

  // Lasers (read every call; continuous mode). NOTE: readRange* blocks until a
  // fresh sample (~30 ms each) - fine for bring-up; switch to data-ready
  // polling later if the loop rate needs to be higher.
  readLaser(laserLeft, leftLaserOk, leftLaserFilter,
            data.leftWallCm, data.leftWallValid, data.leftRawMm);
  readLaser(laserRight, rightLaserOk, rightLaserFilter,
            data.rightWallCm, data.rightWallValid, data.rightRawMm);

  // LDRs (fast analog reads every call).
  data.ldrLeft = analogRead(LDR_LEFT_PIN);
  data.ldrRight = analogRead(LDR_RIGHT_PIN);
  data.ldrDiff = data.ldrLeft - data.ldrRight;

  // Ultrasonics: ping one sensor per scheduled slot to bound blocking time.
  if (now - lastPingMs >= ULTRASONIC_MIN_INTERVAL_MS) {
    lastPingMs = now;
    bool valid = false;
    if (nextUltrasonic == 0) {
      frontFresh = true;
      float rawCm = readUltrasonicCm(ULTRASONIC_FRONT_TRIG_PIN,
                                     ULTRASONIC_FRONT_ECHO_PIN, valid);
      if (valid) frontUsFilter.push(rawCm);
      data.frontRawCm = rawCm;
      data.frontValid = valid && frontUsFilter.ready();
      data.frontCm = data.frontValid
                         ? frontUsFilter.median() + ULTRASONIC_FRONT_FORWARD_OFFSET_CM
                         : 0.0f;
    } else {
      rightDiagFresh = true;
      float rawCm = readUltrasonicCm(ULTRASONIC_RIGHT_45_TRIG_PIN,
                                     ULTRASONIC_RIGHT_45_ECHO_PIN, valid);
      if (valid) rightDiagUsFilter.push(rawCm);
      data.rightDiagRawCm = rawCm;
      data.rightDiagValid = valid && rightDiagUsFilter.ready();
      data.rightDiagCm = data.rightDiagValid ? rightDiagUsFilter.median() : 0.0f;
    }
    nextUltrasonic ^= 1;
  }

  if (stageSensorLogging ||
      (telemetryOn && now - lastTelemetryMs >= SENSOR_TELEMETRY_INTERVAL_MS)) {
    lastTelemetryMs = now;
    printTelemetry(frontFresh, rightDiagFresh);
  }
}

const SensorData &latestSensorData() { return data; }

void resetDistanceFilters() {
  leftLaserFilter.reset();
  rightLaserFilter.reset();
  frontUsFilter.reset();
  rightDiagUsFilter.reset();
  data.leftWallValid = data.rightWallValid = false;
  data.frontValid = data.rightDiagValid = false;
}

void toggleSensorTelemetry() {
  telemetryOn = !telemetryOn;
  Serial.print(F("Sensor telemetry "));
  Serial.println(telemetryOn ? F("ON") : F("OFF"));
}

void setStageSensorLogging(bool enabled) { stageSensorLogging = enabled; }

