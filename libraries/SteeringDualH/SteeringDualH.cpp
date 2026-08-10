#include "SteeringDualH.h"
#include <Arduino.h>

SteeringDualH::SteeringDualH()
: _vel(0), _bias(0), _biasFactor(1.0)
{
  attach(46, 48, 44, 49, 47, 45);
}

void SteeringDualH::attach(int leftDirPin1,  int leftDirPin2,  int leftPwmPin,  
                           int rightDirPin1, int rightDirPin2, int rightPwmPin)
{
  _left.setPins( leftDirPin1,  leftDirPin2, leftPwmPin);
  _right.setPins(rightDirPin1, rightDirPin2, rightPwmPin);
}

void SteeringDualH::flipLeft()
{
  _left.flip();
}

void SteeringDualH::flipRight()
{
  _right.flip();
}

void SteeringDualH::straight(int velocity)
{
  _vel = limit(velocity);
  int rVel = limit(_vel * _biasFactor);
  int lVel = limit(_vel / _biasFactor);
  _right.setVelocity(rVel);
  _left.setVelocity(lVel);
  /*Serial.print("biasFactor: ");
  Serial.println(_biasFactor);*/

}

void SteeringDualH::stop()
{
  straight(0);
}

void SteeringDualH::turn(int harshenss)
{
  int rVel = limit( (_vel + harshenss) * _biasFactor);
  int lVel = limit( (_vel - harshenss) / _biasFactor);
  _right.setVelocity(rVel);
  _left.setVelocity(lVel);
  /*Serial.print("rVel: ");
  Serial.println(rVel);
  Serial.print("lVel: ");
  Serial.println(lVel);
  Serial.print("rVel - lVel: ");
  Serial.println(rVel - lVel);*/
}

int SteeringDualH::bias()
{
  return _bias;
}

void SteeringDualH::setBias(int bias)
{
  _bias = limit(bias);
  _biasFactor = 1.0 + _bias / 255.0;
}

int SteeringDualH::limit(int value)
{
  return constrain(value, -255, 255);
}