#include <Arduino.h>
#include "Wheel.h"

// constructor
Wheel::Wheel(int pwmPin, int dirPin1, int dirPin2,
             int encoderPin)
    : _pwmPin(pwmPin)
    , _dirPin1(dirPin1)
    , _dirPin2(dirPin2)
    , _encPin(encoderPin)
    , _ticks(0)
    , _ticksPerSec(0)
    , _ticksSinceLastSample(0)
    , _debounceTime(2500)
    , _speedSampleTime(10)
    , _lastTickTime(0)
    , _lastSpeedSampleTime(0)
    , _diameter(65)
    , _ticksPerRound(40)
    , _direction(1)
{
    calculateRatio();
    pinMode(_pwmPin,  OUTPUT);
    pinMode(_dirPin1, OUTPUT);
    pinMode(_dirPin2, OUTPUT);
    pinMode(_encPin,  INPUT);
    switch(_encPin)
    {
    case 2:
        attachInterrupt(digitalPinToInterrupt(_encPin), tick0, CHANGE);
        _instance0 = this;
        break;
    case 3:
        attachInterrupt(digitalPinToInterrupt(_encPin), tick1, CHANGE);
        _instance1 = this;
        break;
    }
}

void Wheel::setDiameter(int diameterMM)
{
    _diameter = diameterMM;
    calculateRatio();
}

void Wheel::setResolution(int ticksPerRound)
{
    _ticksPerRound = _ticksPerRound;
    calculateRatio();
}

void Wheel::setDebounceTime(int debounceTime)
{
    _debounceTime = debounceTime;
}

void Wheel::setSpeedSampleTime(int speedSampleTime)
{
    _speedSampleTime = speedSampleTime;
}

void Wheel::flip()
{
    int temp   = _dirPin1;
    _dirPin1   = _dirPin2;
    _dirPin2   = temp;
    _direction = -_direction;
    _pwmLevel  = -_pwmLevel;
}

void Wheel::drive(int pulseWidth)
{
    _pwmLevel = constrain(pulseWidth, -255, 255);
    if(_pwmLevel >= 0) {
        digitalWrite(_dirPin1, HIGH);
        digitalWrite(_dirPin2, LOW);
        analogWrite(_pwmPin, _pwmLevel);
        _direction = 1;
   } else {
        digitalWrite(_dirPin1, LOW);
        digitalWrite(_dirPin2, HIGH);
        analogWrite(_pwmPin, -_pwmLevel);
        _direction = -1;
    }
}

void Wheel::coast()
{
    digitalWrite(_dirPin1, LOW);
    digitalWrite(_dirPin2, LOW);
    digitalWrite(_pwmPin,  HIGH);
}

void Wheel::brake()
{
    digitalWrite(_dirPin1, HIGH);
    digitalWrite(_dirPin2, HIGH);
}

void Wheel::resetDistance()
{
    _ticks = 0;
    _ticksSinceLastSample = 0;
}

long Wheel::distance()
{
    return _ticks * _mmPerTick;
}

int Wheel::angle()
{
    return (360 * _ticks) / _ticksPerRound;
}

// TODO: doesn't properly handle negative velocity
int Wheel::velocity()
{
    if(millis() - _lastSpeedSampleTime > _speedSampleTime)
    {
        _ticksPerSec = (_ticksSinceLastSample * 1000) /
        (millis() - _lastSpeedSampleTime);
        _ticksSinceLastSample = 0;
        _lastSpeedSampleTime = millis();
    }
    return _ticksPerSec * _mmPerTick;
}

// ISR glue routines
void Wheel::tick0()
{
    _instance0->handleInterrupt();
}

void Wheel::tick1()
{
    _instance1->handleInterrupt();
}

// for use by ISR glue routines
Wheel* Wheel::_instance0;
Wheel* Wheel::_instance1;

// class instance to handle an interrupt
void Wheel::handleInterrupt()
{
    if(micros() - _lastTickTime > _debounceTime) {
        _ticks += _direction;
        _ticksSinceLastSample += _direction;
        _lastTickTime = micros();
        if(millis() - _lastSpeedSampleTime > _speedSampleTime) {
            _ticksPerSec = (_ticksSinceLastSample * 1000) /
                           (millis() - _lastSpeedSampleTime);
            _ticksSinceLastSample = 0;
            _lastSpeedSampleTime = millis();
        }
    }
}

void Wheel::calculateRatio()
{
    _mmPerTick = (377 * _diameter) / (120 * _ticksPerRound); // pi is 377/120
}
