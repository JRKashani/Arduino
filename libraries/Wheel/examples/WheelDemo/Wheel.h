#ifndef WHEEL
#define WHEEL

class Wheel
{
  public:
  // constructor
  Wheel(){};
  Wheel(int pwmPin, int dirPin);
  
  // set functions
  void setPwmPin(int  pwmPin);
  void setDirPin(int  dirPin);
  void setForward(bool state); 
  void setVelocity(int   vel); // negative for backwards (-255 : 255)
  void stop();

  // get functions
  int  pwmPin();
  int  dirPin();
  bool forward();
  int  velocity();

  private:
  int  _pwmPin;
  int  _dirPin;
  bool _forward;
  int  _velocity;
};


#endif
