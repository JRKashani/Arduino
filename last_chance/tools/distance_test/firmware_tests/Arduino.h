#pragma once
#include <stdint.h>
#include <iomanip>
#include <sstream>
#include <string>
#define F(text) text
#define LOW 0
#define HIGH 1
#define INPUT 0
#define OUTPUT 1
#define DEFAULT 1
const uint8_t A0=54, A1=55, A2=56, A3=57, A4=58, A5=59, A6=60;
struct FakeSerial {
  std::string incoming, outgoing;
  void begin(unsigned long) {}
  int available() { return static_cast<int>(incoming.size()); }
  char read() { char c=incoming[0]; incoming.erase(0,1); return c; }
  void print(const char *v) { outgoing += v; }
  void print(char v) { outgoing += v; }
  template<class T> void print(T v) { outgoing += std::to_string(v); }
  void print(float v, int precision) {
    std::ostringstream s; s << std::fixed << std::setprecision(precision) << v; outgoing += s.str();
  }
  template<class T> void println(T v) { print(v); outgoing += '\n'; }
  void println(float v, int precision) { print(v, precision); outgoing += '\n'; }
};
extern FakeSerial Serial;
unsigned long millis();
void delay(unsigned long);
void delayMicroseconds(unsigned int);
void digitalWrite(uint8_t, int);
void analogWrite(uint8_t, int);
void pinMode(uint8_t, uint8_t);
void analogReference(uint8_t);
int analogRead(uint8_t);
unsigned long pulseIn(uint8_t, uint8_t, unsigned long);
