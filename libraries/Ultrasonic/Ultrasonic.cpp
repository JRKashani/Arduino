/*
  Ultrasonic.cpp - Library for HC-SR04 Ultrasonic Ranging Module.library

  Created by ITead studio. Apr 20, 2010.
  iteadstudio.com
*/

#include <Arduino.h>
#include "Ultrasonic.h"

Ultrasonic::Ultrasonic(int trigPin, int echoPin)
{
   pinMode(trigPin, OUTPUT);
   pinMode(echoPin, INPUT);
   _trig = trigPin;
   _echo = echoPin;
}

long Ultrasonic::timing()
{
  digitalWrite(_trig, LOW);
  delayMicroseconds(2);
  digitalWrite(_trig, HIGH);
  delayMicroseconds(10);
  digitalWrite(_trig, LOW);
  return pulseIn(_echo, HIGH, 15000UL);
}

long Ultrasonic::ranging(int units)
{
  long duration = timing();
  if (units) {
    return duration / 59; // cm
  } else {
    return duration / 149; // inch
  }
}