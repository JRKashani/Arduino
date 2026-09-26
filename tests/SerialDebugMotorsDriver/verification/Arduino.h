#pragma once
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <string>
#include <sstream>
#include <iomanip>

#define HIGH 1
#define LOW 0
#define INPUT 0
#define OUTPUT 1
class __FlashStringHelper;
#define F(s) reinterpret_cast<const __FlashStringHelper *>(s)
template<class T> T constrain(T x, T lo, T hi) { return x < lo ? lo : (x > hi ? hi : x); }

struct FakeSerial {
  std::string input, output;
  void begin(unsigned long) {}
  int available() const { return static_cast<int>(input.size()); }
  int read() { const char c = input.front(); input.erase(0, 1); return c; }
  void print(const __FlashStringHelper *s) { output += reinterpret_cast<const char *>(s); }
  template<class T> void print(T x) { std::ostringstream s; s << x; output += s.str(); }
  void print(float x, int precision) {
    std::ostringstream s; s << std::fixed << std::setprecision(precision) << x; output += s.str();
  }
  void println() { output += '\n'; }
  template<class T> void println(T x) { print(x); println(); }
  void println(float x, int precision) { print(x, precision); println(); }
};
extern FakeSerial Serial;
unsigned long millis();
void pinMode(int, int);
void digitalWrite(int, int);
void analogWrite(int, int);
void delayMicroseconds(unsigned int);
unsigned long pulseIn(int, int, unsigned long);
