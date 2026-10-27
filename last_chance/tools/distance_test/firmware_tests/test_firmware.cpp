#include <cassert>
#include <iostream>
#include <vector>
#include "Arduino.h"
#include "Wire.h"

FakeSerial Serial;
FakeWire Wire;
uint16_t laserValue=2000;
uint8_t laserStatus=11;
bool laserInitOk=true;
unsigned long fakeNow=0, echoValue=11600, usedTimeout=0;
std::vector<unsigned long> pings;
bool isMotor(uint8_t pin) { return pin==44 || pin==45 || pin==46 || pin==47 || pin==48 || pin==49; }
unsigned long millis() { return fakeNow; }
void delay(unsigned long ms) { fakeNow += ms; }
void delayMicroseconds(unsigned int) {}
void digitalWrite(uint8_t pin, int value) { if(isMotor(pin)) assert(value==0); }
void analogWrite(uint8_t pin, int value) { assert(isMotor(pin) && value==0); }
void pinMode(uint8_t, uint8_t) {}
void analogReference(uint8_t ref) { assert(ref==DEFAULT); }
int analogRead(uint8_t pin) { assert(pin==A6); return 400; }
unsigned long pulseIn(uint8_t pin, uint8_t level, unsigned long timeout) {
  assert(pin==42 && level==HIGH); usedTimeout=timeout; pings.push_back(fakeNow); return echoValue;
}
#include "../../../src/distance_test/main.cpp"

void send(const std::string &commandText, unsigned long runMs=500) {
  Serial.outgoing.clear();
  Serial.incoming=commandText+"\n";
  for(unsigned long elapsed=0; elapsed<=runMs; elapsed+=10) { loop(); fakeNow+=10; }
}
bool contains(const char *text) { return Serial.outgoing.find(text)!=std::string::npos; }
int main() {
  setup();
  assert(contains("BOOT,DISTANCE,1") && contains("CONFIG,US_TIMEOUT_US,30000"));
  assert(contains("CONFIG,LASER_PROFILE,long_range_33ms_single_shot"));
  send("INFO", 510);
  assert(contains("LIVE,U,") && contains("corrected_cm=") && !contains("DATA,"));
  assert(pings.size()==1);
  pings.clear();
  send("START,1,U,3,100,0");
  assert(contains(",11600,1,OK,-1") && contains("END,1,3,complete"));
  assert(usedTimeout==30000 && pings.size()==3);
  assert(pings[1]-pings[0]>=65 && pings[2]-pings[1]>=65);
  echoValue=0;
  send("START,2,U,1,100,0");
  assert(contains(",0,0,NO_ECHO,-1"));
  send("START,3,U,1,1,0");
  assert(contains("ERROR,") && !contains("BEGIN,"));
  send("START,4,L,1,100,0");
  assert(contains(",2000,1,OK,11"));
  laserStatus=4; laserValue=8190;
  send("START,5,L,1,100,0");
  assert(contains(",8190,0,RANGE_STATUS,4"));
  send("START,6,I,1,100,0");
  assert(contains(",400,1,OK,-1") && !laserReady);
  send("START,7,U,3,100,1000\nABORT");
  assert(contains("END,7,0,aborted") && !contains("DATA,"));
  echoValue=11600;
  send("MONITOR,U", 0);
  assert(contains("MONITOR,U,500"));
  pings.clear();
  const unsigned long startTime=fakeNow;
  send("START,9,U,10,500,500", 5000);
  assert(contains("END,9,10,complete") && pings.size()==10);
  assert(pings.front()-startTime==500 && pings.back()-startTime==5000);
  for(size_t i=1; i<pings.size(); ++i) assert(pings[i]-pings[i-1]==500);
  send(std::string(120,'x'));
  assert(contains("ERROR,command_too_long"));
  laserInitOk=false;
  send("START,8,L,1,100,0");
  assert(contains("ERROR,right_laser_init") && !active && !liveEnabled);
  std::cout << "12 firmware scenarios passed; every motor output remained zero.\n";
}
