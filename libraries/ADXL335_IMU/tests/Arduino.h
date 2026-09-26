#ifndef TEST_ARDUINO_H
#define TEST_ARDUINO_H
#include <stdint.h>
#define INPUT 0
#define PI 3.14159265358979323846
int analogRead(uint8_t pin);
void pinMode(uint8_t pin, uint8_t mode);
void delay(unsigned long ms);
void delayMicroseconds(unsigned int us);
#endif
