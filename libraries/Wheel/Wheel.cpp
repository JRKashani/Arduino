  #include <Arduino.h>
  #include "Wheel.h"

  // constructors
Wheel::Wheel()
{

}

Wheel::Wheel(int pwmPin, int dirPin1, int dirPin2, int encoderPin, float diameter);
{
 setPins(pwmPin);
 setDirPin(dirPin);
 setVelocity(0);
 setForward(LOW);
}

  // set functions
void Wheel::setPins(int pwmPin, int dirPin1, int dirPin2, int encoderPin)
{
 _pwmPin=pwmPin;
 pinMode(pwmPin, OUTPUT);
}

void Wheel::setDiameter(int diameter)
{

}

void Wheel::setPwmLevel(int vel);
{
 _velocity=vel;
    if ( _velocity < 0 )            //Go Reverse
    {                 
      digitalWrite(_dirPin, !_forward);
      analogWrite(_pwmPin, -_velocity);
    }
     if ( _velocity >= 0 )           //Go Forward
     {
       digitalWrite(_dirPin, _forward);
       analogWrite(_pwmPin, min(255, _velocity));
     }
  }

  void Wheel::stop()
  {
   setVelocity(0);        

 }

 void Wheel::setDebounceTime(unsigned long t)
 {


 }

 void Wheel::resetDistance()
 {

 }

  // get functions
 int Wheel::pwmPin()
 {
   return _pwmPin;
 }

 int Wheel::dirPin()
 {
   return _dirPin;
 }

 bool Wheel::forward()
 {
   return _forward;
 }

 int Wheel::velocity()
 {
   return _velocity;
 }
