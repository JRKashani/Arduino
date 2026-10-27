// Standalone acquisition only. No navigation, baseline changes or motor movement.
#include <Arduino.h>
#include <Wire.h>
#include <VL53L0X.h>
#include "../DEFINE.h"
#include <stdlib.h>
#include <string.h>

namespace {
const uint8_t IR_PIN = A6;
const unsigned long ECHO_TIMEOUT_US = 30000UL; // 2 m needs about 11600 us.
const unsigned long MIN_PERIOD_MS = 65UL;
const unsigned long LIVE_PERIOD_MS = 500UL;
const uint32_t LASER_BUDGET_US = 33000UL;
VL53L0X rightLaser;
bool laserReady = false, active = false, overflowing = false, liveEnabled = true;
char sensor = 'U', command[96];
uint8_t commandLength = 0;
unsigned long captureId = 0, nextMs = 0, lastPing = 0, periodMs = 100;
unsigned long nextLiveMs = 0;
unsigned int requested = 0, sequence = 0;

void holdMotorsOff() {
  const uint8_t pins[] = {LEFT_PWM, RIGHT_PWM, LEFT_DIR_1, LEFT_DIR_2,
                          RIGHT_DIR_1, RIGHT_DIR_2};
  for (uint8_t i = 0; i < sizeof(pins); ++i) {
    digitalWrite(pins[i], LOW);
    pinMode(pins[i], OUTPUT);
  }
  analogWrite(LEFT_PWM, 0);
  analogWrite(RIGHT_PWM, 0);
}

void shutdownLasers() {
  digitalWrite(XSHUT_LEFT_PIN, LOW);
  digitalWrite(XSHUT_RIGHT_PIN, LOW);
  pinMode(XSHUT_LEFT_PIN, OUTPUT);
  pinMode(XSHUT_RIGHT_PIN, OUTPUT);
  laserReady = false;
}

bool prepareLaser() {
  if (laserReady) return true;
  shutdownLasers();
  delay(10);
  // Release XSHUT to the breakout's pull-up; never drive this pin to 5 V.
  pinMode(XSHUT_RIGHT_PIN, INPUT);
  delay(10);
  rightLaser.setAddress(0x29); // software address after hardware reset
  rightLaser.setTimeout(500);
  if (!rightLaser.init()) return false;
  rightLaser.setAddress(LASER_RIGHT_ADDRESS);
  // Pololu's long-range profile: improve sensitivity for the 200 cm test.
  if (!rightLaser.setSignalRateLimit(0.1f) ||
      !rightLaser.setVcselPulsePeriod(VL53L0X::VcselPeriodPreRange, 18) ||
      !rightLaser.setVcselPulsePeriod(VL53L0X::VcselPeriodFinalRange, 14)) return false;
  if (!rightLaser.setMeasurementTimingBudget(LASER_BUDGET_US)) return false;
  laserReady = true;
  return true;
}

void info() {
  Serial.println(F("DISTANCE,1,1.2.0"));
  Serial.println(F("CAPABILITY,LIVE_500MS"));
  Serial.print(F("CONFIG,US_SCALE,")); Serial.println(ULTRASONIC_DISTANCE_SCALE, 6);
  Serial.print(F("CONFIG,US_OFFSET_CM,")); Serial.println(ULTRASONIC_DISTANCE_OFFSET_CM, 6);
  Serial.print(F("CONFIG,US_TIMEOUT_US,")); Serial.println(ECHO_TIMEOUT_US);
  Serial.println(F("CONFIG,US_CALIBRATION_RANGE_CM,15:150"));
  Serial.println(F("CONFIG,IR_PIN,A6"));
  Serial.println(F("CONFIG,IR_MODEL,GP2Y0A21YK0F"));
  Serial.println(F("CONFIG,ADC_REFERENCE,AVCC"));
  Serial.println(F("CONFIG,LASER_PROFILE,long_range_33ms_single_shot"));
  Serial.println(F("CONFIG,DISTANCE_ORIGIN,sensor_face"));
  Serial.println(F("READY"));
}

void finish(const char *reason) {
  Serial.print(F("END,")); Serial.print(captureId); Serial.print(',');
  Serial.print(sequence); Serial.print(','); Serial.println(reason);
  active = false;
  nextLiveMs = millis() + LIVE_PERIOD_MS;
}

bool number(const char *text, unsigned long lo, unsigned long hi, unsigned long &value) {
  if (!text || !*text) return false;
  for (const char *p = text; *p; ++p) if (*p < '0' || *p > '9') return false;
  if (strlen(text) > 9) return false;
  value = strtoul(text, nullptr, 10);
  return value >= lo && value <= hi;
}

void handleCommand() {
  if (!strcmp(command, "INFO")) { info(); return; }
  if (!strcmp(command, "ABORT")) {
    if (active) finish("aborted"); else Serial.println(F("IDLE"));
    return;
  }
  if (active) { Serial.println(F("ERROR,busy")); return; }
  if (!strcmp(command, "MONITOR,OFF")) {
    liveEnabled = false; Serial.println(F("MONITOR,OFF")); return;
  }
  if (!strncmp(command, "MONITOR,", 8) && strlen(command) == 9 && strchr("ULI", command[8])) {
    const char selected = command[8];
    if (selected == 'L') {
      if (!prepareLaser()) { shutdownLasers(); liveEnabled = false; Serial.println(F("ERROR,right_laser_init")); return; }
    } else shutdownLasers();
    sensor = selected; liveEnabled = true; nextLiveMs = millis() + LIVE_PERIOD_MS;
    Serial.print(F("MONITOR,")); Serial.print(sensor); Serial.println(F(",500"));
    return;
  }
  char *parts[7]; uint8_t count = 0;
  char *token = strtok(command, ",");
  while (token && count < 7) { parts[count++] = token; token = strtok(nullptr, ","); }
  unsigned long id, n, period, settle;
  if (count != 6 || token || strcmp(parts[0], "START") ||
      strlen(parts[2]) != 1 || !strchr("ULI", parts[2][0]) ||
      !number(parts[1], 1, 999999999UL, id) || !number(parts[3], 1, 100, n) ||
      !number(parts[4], MIN_PERIOD_MS, 2000, period) || !number(parts[5], 0, 10000, settle)) {
    Serial.println(F("ERROR,expected_START_id_U-or-L-or-I_count_period-ms_settle-ms"));
    return;
  }
  sensor = parts[2][0];
  if (sensor == 'L') {
    if (!prepareLaser()) { shutdownLasers(); liveEnabled = false; Serial.println(F("ERROR,right_laser_init")); return; }
  } else shutdownLasers();
  captureId = id; requested = (unsigned int)n; sequence = 0; periodMs = period;
  nextMs = millis() + settle; active = true;
  Serial.print(F("BEGIN,")); Serial.print(captureId); Serial.print(','); Serial.print(sensor);
  Serial.print(','); Serial.println(requested);
}

void sample(bool recording) {
  unsigned long raw = 0;
  int detail = -1;
  bool valid = true;
  const char *status = "OK";
  const unsigned long timestamp = millis();
  if (sensor == 'U') {
    lastPing = timestamp;
    digitalWrite(ULTRASONIC_FRONT_TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(ULTRASONIC_FRONT_TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(ULTRASONIC_FRONT_TRIG_PIN, LOW);
    raw = pulseIn(ULTRASONIC_FRONT_ECHO_PIN, HIGH, ECHO_TIMEOUT_US);
    valid = raw >= 116 && raw <= 23200; // nominal 2..400 cm; preserve all raw values
    if (!valid) status = raw ? "OUT_OF_RANGE" : "NO_ECHO";
  } else if (sensor == 'L') {
    raw = rightLaser.readRangeSingleMillimeters();
    bool timeout = rightLaser.timeoutOccurred();
    uint8_t busStatus = rightLaser.last_status;
    detail = (rightLaser.readReg(VL53L0X::RESULT_RANGE_STATUS) & 0x78) >> 3;
    valid = !timeout && !busStatus && !rightLaser.last_status && raw > 0 && raw < 8190 && detail == 11;
    if (timeout) status = "TIMEOUT";
    else if (busStatus || rightLaser.last_status) status = "I2C_ERROR";
    else if (!valid) status = "RANGE_STATUS";
    // Status 11 is device-level range complete. Full ST host sigma checks are
    // not implemented by the existing Pololu library; retain the status code.
  } else {
    analogRead(IR_PIN); // discard the first ADC conversion after switching
    raw = analogRead(IR_PIN);
    if (raw == 0 || raw == 1023) status = "ADC_RAIL";
    // Rails are still actual ADC observations; don't erase the IR characteristic.
  }
  if (liveEnabled) {
    Serial.print(F("LIVE,")); Serial.print(sensor); Serial.print(',');
    Serial.print(timestamp); Serial.print(','); Serial.print(raw); Serial.print(',');
    Serial.print(valid ? 1 : 0); Serial.print(','); Serial.print(status); Serial.print(',');
    Serial.print(detail);
    if (valid && sensor == 'U') {
      Serial.print(F(",cm=")); Serial.print(raw / 58.0f, 2);
      Serial.print(F(",corrected_cm="));
      Serial.print(raw / 58.0f * ULTRASONIC_DISTANCE_SCALE + ULTRASONIC_DISTANCE_OFFSET_CM, 2);
    } else if (valid && sensor == 'L') {
      Serial.print(F(",cm=")); Serial.print(raw / 10.0f, 2);
    } else if (valid && sensor == 'I') {
      Serial.print(F(",V_nominal5=")); Serial.print(raw * 5.0f / 1023.0f, 3);
    }
    Serial.println(F(""));
  }
  if (recording) {
    Serial.print(F("DATA,")); Serial.print(captureId); Serial.print(',');
    Serial.print(sequence++); Serial.print(','); Serial.print(timestamp); Serial.print(',');
    Serial.print(raw); Serial.print(','); Serial.print(valid ? 1 : 0); Serial.print(',');
    Serial.print(status); Serial.print(','); Serial.println(detail);
    if (sequence >= requested) finish("complete");
  }
}
} // namespace

void setup() {
  holdMotorsOff();
  pinMode(ULTRASONIC_FRONT_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_RIGHT_45_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_FRONT_ECHO_PIN, INPUT);
  digitalWrite(ULTRASONIC_FRONT_TRIG_PIN, LOW);
  digitalWrite(ULTRASONIC_RIGHT_45_TRIG_PIN, LOW);
  pinMode(IR_PIN, INPUT);
  analogReference(DEFAULT);
  shutdownLasers();
  Wire.begin();
#if defined(WIRE_HAS_TIMEOUT)
  Wire.setWireTimeout(25000, true);
#endif
  Serial.begin(115200);
  Serial.println(F("BOOT,DISTANCE,1"));
  info();
  nextLiveMs = millis() + LIVE_PERIOD_MS;
}

void loop() {
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      if (overflowing) Serial.println(F("ERROR,command_too_long"));
      else if (commandLength) { command[commandLength] = 0; handleCommand(); }
      commandLength = 0; overflowing = false;
    } else if (!overflowing) {
      if (commandLength < sizeof(command) - 1) command[commandLength++] = c;
      else overflowing = true;
    }
  }
  const unsigned long now = millis();
  if (active && (long)(now - nextMs) >= 0 &&
      (sensor != 'U' || now - lastPing >= MIN_PERIOD_MS)) {
    nextMs = now + periodMs;
    sample(true);
  } else if (!active && liveEnabled && (long)(now - nextLiveMs) >= 0 &&
             (sensor != 'U' || now - lastPing >= MIN_PERIOD_MS)) {
    nextLiveMs = now + LIVE_PERIOD_MS;
    sample(false);
  }
}
