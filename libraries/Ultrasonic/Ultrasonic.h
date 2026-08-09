/*
  Ultrasonic.h - Library for HR-SC04 Ultrasonic Ranging Module.
  Created by ITead studio. Alex, Apr 20, 2010.
  iteadstudio.com
*/


#ifndef ULTRASONIC_H
#define ULTRASONIC_H


#define CM  1
#define INC 0

class Ultrasonic
{
  public:
    Ultrasonic(int trigPin, int echoPin);
    long timing();
    long ranging(int units);

    private:
    int _trig;
    int _echo;
};

#endif