#ifndef MOTOR_H
#define MOTOR_H

class Motor
{
  public:
  // constructors
  Motor();
  Motor(int dirPin1, int dirPin2, int pwmPin);
  
  // set functions
  void setPins(int dirPin1, int dirPin2, int pwmPin);
  void flip(); 				       // alternates between CW and CCW as the positive direction
  void setVelocity(int vel); // negative for backwards (-255 : 255)
  void stop();

  // get function
  int  velocity();

  private:
  int  _dirPin1;
  int  _dirPin2;
  int  _pwmPin;
  int  _velocity;
};


#endif
