// Builds the ACTUAL mission and ADXL335 library against a simulated Arduino.
#include "Arduino.h"
#include "ImuTiltMission.h"
#include <cassert>
#include <iostream>

FakeSerial Serial;
uint32_t nowUs = 0;
int adcReads = 0, pinModes[80] = {}, pinValues[80] = {};
unsigned long millis() { return nowUs / 1000UL; }
unsigned long micros() { return nowUs; }
int analogRead(uint8_t pin) {
  ++adcReads;
  if (pin == A5) return 675;
  if (pin == A2) return 365; // Deliberately inclined; must NOT become zero.
  if (pin == A1) return 342;
  return 409;
}
void digitalWrite(uint8_t pin, int value) { assert(value == 0); pinValues[pin] = value; }
void analogWrite(uint8_t pin, int value) { assert(value == 0); pinValues[pin] = value; }
void pinMode(uint8_t pin, uint8_t mode) { pinModes[pin] = mode; }
void delay(unsigned long ms) { nowUs += ms*1000UL; }
void delayMicroseconds(unsigned int us) { nowUs += us; }

void command(ImuTiltMission &mission, const std::string &line) {
  Serial.incoming += line + "\n";
  while (Serial.available()) mission.update();
}

int count(const std::string &text, const std::string &needle) {
  int result = 0;
  for (size_t p = 0; (p = text.find(needle, p)) != std::string::npos; p += needle.size()) ++result;
  return result;
}

void reset() { Serial = FakeSerial{}; nowUs = 0; adcReads = 0; }

void test_boot_no_baseline_and_no_motor_enable() {
  reset(); ImuTiltMission mission;
  assert(mission.begin());
  assert(adcReads == 0);
  assert(Serial.outgoing.find("BASELINE,NONE,0,0") != std::string::npos);
  for (int pin : {44, 45, 46, 48, 49, 47}) {
    assert(pinModes[pin] == OUTPUT); assert(pinValues[pin] == 0);
  }
  command(mission, "F"); command(mission, "L"); command(mission, "R");
  assert(mission.status() == ImuTiltMission::State::Idle);
  assert(adcReads == 0);
}

void test_complete_capture_timing_and_fixed_metadata() {
  reset(); ImuTiltMission mission; mission.begin();
  assert(mission.startTrial(7, 0, 1000, 20000));
  mission.update(); // slot 0
  for (uint32_t i = 1; i <= 50; ++i) { nowUs = i*20000; mission.update(); }
  assert(mission.status() == ImuTiltMission::State::Review);
  assert(count(Serial.outgoing, "DATA,7,") == 50);
  assert(Serial.outgoing.find("END,7,DONE,50,0,1000000") != std::string::npos);
  assert(Serial.outgoing.find("DATA,7,0,0,1,365,342,409,675,") != std::string::npos);
}

void test_malformed_and_overlong_commands_do_not_start() {
  reset(); ImuTiltMission mission; mission.begin();
  for (const char *bad : {"START,,0,1000,20000", "START,-1,0,1000,20000",
       "START,4294967296,0,1000,20000", "START,1,0,1000,20000,extra",
       "START,1,0,999,20000", "START,1,0,1000,19999", "START,1,60001,1000,20000"}) {
    command(mission, bad);
    assert(mission.status() == ImuTiltMission::State::Idle);
  }
  command(mission, std::string(120, 'a')+"START,1,0,1000,20000");
  assert(mission.status() == ImuTiltMission::State::Idle);
  assert(Serial.outgoing.find("ERROR,LINE_TOO_LONG") != std::string::npos);
  command(mission, "START,8,3000,1000,20000");
  assert(mission.status() == ImuTiltMission::State::Settle);
  command(mission, "ABORT");
  assert(mission.status() == ImuTiltMission::State::Review);
  assert(adcReads == 0);
  assert(Serial.outgoing.find("END,8,ABORT,0,0,0") != std::string::npos);
}

void test_skipped_slots_and_abort_during_capture() {
  reset(); ImuTiltMission mission; mission.begin();
  command(mission, "START,3,0,1000,20000");
  assert(!mission.startTrial(4, 0, 1000, 20000)); // no queue/overlap
  nowUs = 100000; mission.update(); // slot 5; slots 1-4 missed
  nowUs = 1000000; mission.update();
  assert(Serial.outgoing.find("DATA,3,5,100000,") != std::string::npos);
  assert(Serial.outgoing.find("END,3,DONE,2,48,1000000") != std::string::npos);
  command(mission, "START,4,0,1000,20000");
  command(mission, "ABORT");
  assert(Serial.outgoing.find("END,4,ABORT,1,0,") != std::string::npos);
}

void test_capture_across_micros_wrap() {
  reset(); ImuTiltMission mission; mission.begin();
  nowUs = 0xFFFFFF00UL;
  command(mission, "START,5,0,1000,20000");
  const uint32_t start = 0xFFFFFF00UL;
  for (uint32_t i = 1; i <= 50; ++i) { nowUs = start+i*20000UL; mission.update(); }
  assert(Serial.outgoing.find("END,5,DONE,50,0,1000000") != std::string::npos);
}

int main() {
  test_boot_no_baseline_and_no_motor_enable();
  test_complete_capture_timing_and_fixed_metadata();
  test_malformed_and_overlong_commands_do_not_start();
  test_skipped_slots_and_abort_during_capture();
  test_capture_across_micros_wrap();
  std::cout << "5 firmware scenarios passed; every motor write remained zero.\n";
}
