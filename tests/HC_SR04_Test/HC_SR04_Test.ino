/*
  HC_SR04_Test.ino - both robot ultrasonic sensors.
  Serial Monitor: 9600 baud. Outputs uncalibrated distance in cm.

  Both sensors: VCC -> Mega 5V, GND -> common GND.
  Front:    Trig -> 43, Echo -> 42.
  Right 45: Trig -> 51, Echo -> 50.

  Sensors are triggered separately, with a 60 ms quiet gap after each
  measurement to reduce interference. A missing echo prints NO ECHO.
*/

#include <Arduino.h>

// Same wiring as last_chance/DEFINE.h.
const uint8_t FRONT_TRIG_PIN = 43;
const uint8_t FRONT_ECHO_PIN = 42;
const uint8_t RIGHT_45_TRIG_PIN = 51;
const uint8_t RIGHT_45_ECHO_PIN = 50;
const unsigned long ECHO_TIMEOUT_US = 15000UL;
const unsigned long INTER_SENSOR_GAP_MS = 60UL;

void setupSensor(uint8_t trigPin, uint8_t echoPin) {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  digitalWrite(trigPin, LOW);
}

void setup() {
  Serial.begin(9600);
  setupSensor(FRONT_TRIG_PIN, FRONT_ECHO_PIN);
  setupSensor(RIGHT_45_TRIG_PIN, RIGHT_45_ECHO_PIN);
  Serial.println(F("Both HC-SR04 sensors ready. Uncalibrated distances in cm."));
}

unsigned long readEcho(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  return pulseIn(echoPin, HIGH, ECHO_TIMEOUT_US);
}

void printDistance(unsigned long duration) {
  if (duration == 0) {
    Serial.print(F("NO ECHO"));
  } else {
    Serial.print(duration / 58.0f, 1);
    Serial.print(F(" cm"));
  }
}

void loop() {
  const unsigned long frontEcho = readEcho(FRONT_TRIG_PIN, FRONT_ECHO_PIN);
  delay(INTER_SENSOR_GAP_MS);
  const unsigned long rightEcho = readEcho(RIGHT_45_TRIG_PIN, RIGHT_45_ECHO_PIN);

  Serial.print(F("Front: "));
  printDistance(frontEcho);
  Serial.print(F(" | Right 45: "));
  printDistance(rightEcho);
  Serial.println();

  delay(INTER_SENSOR_GAP_MS);
}
