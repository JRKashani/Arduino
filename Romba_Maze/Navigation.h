#pragma once

#include <Arduino.h>
#include "Globals.h"
#include "Sensors.h"

struct LaserMinimum
{
    uint16_t distance_mm;
    unsigned long timestamp_ms;
};

struct LaserScanResult
{
    LaserMinimum left;
    LaserMinimum front;
    LaserMinimum right;
};

LaserScanResult scanWithRobot();