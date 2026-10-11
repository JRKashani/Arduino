#ifndef IMU_TILT_TEST_ARDUINO_H
#define IMU_TILT_TEST_ARDUINO_H
#include <stdint.h>
#include <iomanip>
#include <sstream>
#include <string>

#define F(text) text
#define LOW 0
#define OUTPUT 1
#define INPUT 0
#define PI 3.14159265358979323846
const uint8_t A0 = 54, A1 = 55, A2 = 56, A3 = 57, A4 = 58, A5 = 59;

struct FakeSerial {
  std::string incoming, outgoing;
  int available() { return static_cast<int>(incoming.size()); }
  char read() { char c = incoming[0]; incoming.erase(0, 1); return c; }
  void print(const char *v) { outgoing += v; }
  void print(char v) { outgoing += v; }
  template <class T> void print(T v) { outgoing += std::to_string(v); }
  void print(float v, int precision) {
    std::ostringstream stream;
    stream << std::fixed << std::setprecision(precision) << v;
    outgoing += stream.str();
  }
  template <class T> void println(T v) { print(v); outgoing += '\n'; }
  void println(float v, int precision) { print(v, precision); outgoing += '\n'; }
};
extern FakeSerial Serial;
unsigned long millis();
unsigned long micros();
int analogRead(uint8_t pin);
void digitalWrite(uint8_t pin, int value);
void analogWrite(uint8_t pin, int value);
void pinMode(uint8_t pin, uint8_t mode);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
#endif
