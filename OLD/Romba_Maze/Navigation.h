#pragma once

#include <Arduino.h>
#include "Globals.h"
#include "Sensors.h"

struct LaserMinimum
{
    uint16_t distance_mm;
    unsigned long timestamp_ms;
};

struct LaserBalanceMinimum
{
    uint16_t difference_mm;
    uint16_t left_mm;
    uint16_t right_mm;
    unsigned long timestamp_ms;
};

struct LaserScanResult
{
    LaserMinimum left;
    LaserMinimum front;
    LaserMinimum right;

    LaserMinimum leftRight;
    LaserMinimum leftFront;
    LaserMinimum rightFront;

    LaserBalanceMinimum leftRightBalance;

    bool emergencyEscapePerformed;
};

void wallFollowingNavigation();
void resetWallFollowingNavigation();

LaserScanResult scanWithRobot();

void funnelNavigation(const SensorReading &sensors);



