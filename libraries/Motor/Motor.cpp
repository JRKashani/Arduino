  #include <Arduino.h>
  #include "Motor.h"

  // constructor
  Motor::Motor()
  {
    setPins(7, 8, 9);
    stop();
  }

  Motor::Motor(int dirPin1, int dirPin2, int pwmPin)
  {
    setPins( dirPin1, dirPin2, pwmPin);
    stop();
  }

  // set functions
  void Motor::setPins(int dirPin1, int dirPin2, int pwmPin)
  {
    _pwmPin  = pwmPin;
    _dirPin1 = dirPin1;
    _dirPin2 = dirPin2;
    pinMode(_pwmPin,  OUTPUT);
    pinMode(_dirPin1, OUTPUT);
    pinMode(_dirPin2, OUTPUT);
  }

  void Motor::flip()
  {
    int temp = _dirPin1;
    _dirPin1 = _dirPin2;
    _dirPin2 = temp;
  }

  void Motor::setVelocity(int vel)
  {
    
    _velocity = constrain(vel, -255, 255);
    if ( _velocity < 0 ) {           //Go Reverse
      // Serial.print("Motor ");
      // Serial.print(_pwmPin);
      // Serial.print(" Driving Backwards ");
      // Serial.println(_velocity);
      digitalWrite(_dirPin1, HIGH);
      digitalWrite(_dirPin2, LOW);
      analogWrite(_pwmPin, -_velocity);
    } else {                         //Go Forward       
      // Serial.print("Motor ");
      // Serial.print(_pwmPin);
      // Serial.print(" Driving Forward");
      // Serial.println(_velocity);
      digitalWrite(_dirPin1, LOW);
      digitalWrite(_dirPin2, HIGH);
      analogWrite(_pwmPin, _velocity);
    }
  }

  void Motor::stop()
  {
      _velocity = 0;
      digitalWrite(_dirPin1, LOW);
      digitalWrite(_dirPin2, LOW);
      analogWrite(_pwmPin, _velocity);
  }

  // get function
  int  Motor::velocity()
  {
    return _velocity;
  }
  