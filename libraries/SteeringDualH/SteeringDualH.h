#ifndef STEERINGDUALH_H
#define STEERINGDUALH_H

#include <Motor.h>

class SteeringDualH
{
public:
  SteeringDualH();

// we do not define a constructor and use default constructor instead

  // setup
  void attach(int leftDirPin1,  int leftDirPin2, int leftPwmPin,  
              int rightDirPin1, int rightDirPin2, int rightPwmPin);
  
  // flip the definition of forward
  void flipLeft();
  void flipRight();

  // steering control
  void straight(int velocity); // negative for backwards
  void stop();                 // stop both motors
  void turn(int harshenss);    // negative for right turn
  void fastLeft();
  void fastRight();

  // compansate for the difference between the wheels
  int  bias();
  void setBias(int bias);

private:
  Motor  _left;
  Motor  _right;
  int 	 _vel;
  int 	 _bias;
  float  _biasFactor;

  // private method to limit the value between [-255 , 255] 
  int limit(int value);
};

#endif

