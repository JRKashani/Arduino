#include "ImuTiltMission.h"
#include "../DEFINE.h"
#include <stdlib.h>
#include <string.h>

namespace {
const ADXL335Calibration weights = {
  X_ZERO_RATIO, Y_ZERO_RATIO, Z_ZERO_RATIO,
  X_RATIO_PER_G, Y_RATIO_PER_G, Z_RATIO_PER_G
};

bool number(const char *text, unsigned long &value) {
  if (!text || !*text) return false;
  // Bound before conversion, including overflow on AVR's 32-bit unsigned long.
  value = 0;
  for (const char *p = text; *p; ++p) {
    if (*p < '0' || *p > '9') return false;
    const unsigned long digit = *p - '0';
    if (value > (0xFFFFFFFFUL - digit) / 10UL) return false;
    value = value * 10UL + digit;
  }
  return true;
}
}

ImuTiltMission::ImuTiltMission()
    : imu_(ACCEL_X_PIN, ACCEL_Y_PIN, ACCEL_Z_PIN, ACCEL_REF_3V3_PIN, weights) {}

void ImuTiltMission::stopMotors() {
  const uint8_t pins[] = {LEFT_PWM, RIGHT_PWM, LEFT_DIR_1, LEFT_DIR_2,
                          RIGHT_DIR_1, RIGHT_DIR_2};
  for (uint8_t pin : pins) {
    digitalWrite(pin, LOW);
    pinMode(pin, OUTPUT);
  }
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}

bool ImuTiltMission::begin() {
  stopMotors();
  state_ = State::Idle;
  ready_ = imu_.begin(); // Clears baseline; never zero an unknown inclined pose.
  info();
  Serial.println(ready_ ? F("READY") : F("ERROR,IMU_INIT"));
  return ready_;
}

void ImuTiltMission::info() {
  Serial.println(F("IMUTILT,1,1.0.0"));
  Serial.print(F("CONFIG,"));
  Serial.print(weights.xZeroRatio, 7); Serial.print(',');
  Serial.print(weights.yZeroRatio, 7); Serial.print(',');
  Serial.print(weights.zZeroRatio, 7); Serial.print(',');
  Serial.print(weights.xRatioPerG, 7); Serial.print(',');
  Serial.print(weights.yRatioPerG, 7); Serial.print(',');
  Serial.println(weights.zRatioPerG, 7);
  Serial.println(F("BASELINE,NONE,0,0"));
}

bool ImuTiltMission::startTrial(unsigned long id, unsigned long settleMs,
                                unsigned long durationMs, unsigned long periodUs) {
  if (!ready_ || state_ == State::Settle || state_ == State::Capture ||
      id == 0 || settleMs > 60000UL || durationMs < 1000UL ||
      durationMs > 60000UL || periodUs < 20000UL || periodUs > 1000000UL)
    return false;
  stopMotors();
  id_ = id;
  settleMs_ = settleMs;
  durationUs_ = durationMs * 1000UL;
  periodUs_ = periodUs;
  sequence_ = samples_ = missed_ = 0;
  settleStartMs_ = millis();
  state_ = State::Settle;
  Serial.print(F("SETTLE,")); Serial.println(id_);
  return true;
}

void ImuTiltMission::finish(bool aborted) {
  const unsigned long elapsed = state_ == State::Capture
      ? micros() - captureStartUs_ : 0;
  Serial.print(F("END,")); Serial.print(id_);
  Serial.print(aborted ? F(",ABORT,") : F(",DONE,"));
  Serial.print(samples_); Serial.print(',');
  Serial.print(missed_); Serial.print(','); Serial.println(elapsed);
  state_ = State::Review;
  stopMotors();
}

void ImuTiltMission::abort() {
  if (state_ == State::Settle || state_ == State::Capture) finish(true);
  else { state_ = State::Idle; Serial.println(F("IDLE")); }
  stopMotors();
}

void ImuTiltMission::command() {
  if (!strcmp(input_, "ABORT")) { abort(); return; }
  if (!strcmp(input_, "INFO")) { info(); Serial.println(F("READY")); return; }
  if (!strcmp(input_, "STATUS")) {
    Serial.print(F("STATE,")); Serial.println(static_cast<int>(state_)); return;
  }
  // strsep-style split that preserves empty fields (strtok would skip them).
  char *fields[5];
  char *cursor = input_;
  bool good = true;
  for (uint8_t i = 0; i < 5; ++i) {
    fields[i] = cursor;
    char *comma = strchr(cursor, ',');
    if (i < 4) {
      if (!comma) { good = false; break; }
      *comma = '\0'; cursor = comma + 1;
    } else if (comma) good = false;
  }
  unsigned long values[4];
  if (good && !strcmp(fields[0], "START")) {
    for (uint8_t i = 0; i < 4; ++i)
      if (!number(fields[i + 1], values[i])) good = false;
    if (good && startTrial(values[0], values[1], values[2], values[3])) return;
  }
  Serial.println(F("ERROR,COMMAND_OR_STATE"));
}

void ImuTiltMission::sample(unsigned long elapsedUs) {
  const ADXL335Reading r = imu_.read();
  Serial.print(F("DATA,")); Serial.print(id_); Serial.print(',');
  Serial.print(sequence_++); Serial.print(','); Serial.print(elapsedUs);
  Serial.print(','); Serial.print(r.valid ? 1 : 0);
  Serial.print(','); Serial.print(r.rawX);
  Serial.print(','); Serial.print(r.rawY);
  Serial.print(','); Serial.print(r.rawZ);
  Serial.print(','); Serial.print(r.raw3V3);
  Serial.print(','); Serial.print(r.ax, 6);
  Serial.print(','); Serial.print(r.ay, 6);
  Serial.print(','); Serial.print(r.az, 6);
  Serial.print(','); Serial.print(r.magnitude, 6);
  Serial.print(','); Serial.print(r.rawRollDeg, 5);
  Serial.print(','); Serial.print(r.rawPitchDeg, 5);
  Serial.print(','); Serial.print(r.robotRollDeg, 5);
  Serial.print(','); Serial.println(r.robotPitchDeg, 5);
  ++samples_;
}

void ImuTiltMission::update() {
  stopMotors();
  // Bounded work per update; a serial flood cannot starve the capture scheduler.
  for (uint8_t budget = 0; budget < 96 && Serial.available(); ++budget) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflow_) Serial.println(F("ERROR,LINE_TOO_LONG"));
      else if (inputLength_) { input_[inputLength_] = '\0'; command(); }
      inputLength_ = 0;
      overflow_ = false;
    } else if (!overflow_) {
      if (inputLength_ < sizeof(input_) - 1 && c >= 32 && c <= 126)
        input_[inputLength_++] = c;
      else overflow_ = true; // Discard the whole line, including its tail.
    }
  }
  if (state_ == State::Settle && millis() - settleStartMs_ >= settleMs_) {
    state_ = State::Capture;
    Serial.print(F("CAPTURE,")); Serial.println(id_);
    captureStartUs_ = micros(); nextUs_ = 0;
  }
  if (state_ != State::Capture) return;
  const unsigned long elapsed = micros() - captureStartUs_;
  if (elapsed >= durationUs_) {
    // Count any final scheduled slots lost to an overlong output/iteration.
    if (nextUs_ < durationUs_)
      missed_ += (durationUs_ - 1UL - nextUs_) / periodUs_ + 1UL;
    finish(false);
    return;
  }
  if (elapsed < nextUs_) return;
  const unsigned long skipped = (elapsed - nextUs_) / periodUs_;
  missed_ += skipped; sequence_ += skipped;
  nextUs_ += (skipped + 1UL) * periodUs_;
  sample(elapsed);
}
