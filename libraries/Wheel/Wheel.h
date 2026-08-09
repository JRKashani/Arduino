#ifndef WHEEL_H
#define WHEEL_H

class Wheel
{
public:
    // constructor
    Wheel(int pwmPin, int dirPin1, int dirPin2,
          int encoderPin);

    // set functions
    void setDiameter(int diameterMM);
    void setResolution(int ticksPerRound);
    void setDebounceTime(int debounceTime);
    void setSpeedSampleTime(int speedSampleTime);
    void flip();
    void drive(int pulseWidth);
    void coast();
    void brake();
    void resetDistance();

    // get functions
    long distance();        // in mm
    int  angle();           // in degrees
    int  velocity();        // in mm/s

private:
    static void tick0();
    static void tick1();

    void handleInterrupt();
    void calculateRatio();

    // static pointers that can be assigned to the object
    // depending on the interrupt pin _encPin it uses
    static Wheel* _instance0;
    static Wheel* _instance1;

    // member variables`
    int  _pwmPin;
    int  _dirPin1;
    int  _dirPin2;
    int  _encPin;
    int  _pwmLevel;
    volatile long _ticks;
    volatile long _ticksSinceLastSample;
    int  _ticksPerSec;
    int  _debounceTime;             // in microseconds
    int  _speedSampleTime;          // in milliseconds
    unsigned long _lastTickTime;
    unsigned long _lastSpeedSampleTime;
    int  _diameter;
    int  _ticksPerRound;
    int  _direction;
    int  _mmPerTick;
};


#endif
