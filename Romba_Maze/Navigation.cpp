#pragma once

#include "Navigation.h"

void updateMinimum(
    LaserMinimum &minimum,
    uint16_t distance,
    unsigned long timestamp)
{
    const uint16_t MIN_VALID_DISTANCE = 50;
    const uint16_t MAX_VALID_DISTANCE = 4000;

    if (distance < MIN_VALID_DISTANCE ||
        distance > MAX_VALID_DISTANCE)
        return;

    if (distance < minimum.distance_mm)
    {
        minimum.distance_mm = distance;
        minimum.timestamp_ms = timestamp;
    }
}

LaserScanResult scanWithRobot()
{
    const unsigned long SWEEP_TIME = 1000;
    const unsigned long SAMPLE_INTERVAL = 20;

    const uint16_t MIN_VALID_DISTANCE = 50;
    const uint16_t MAX_VALID_DISTANCE = 4000;

    LaserScanResult result;

    // Initialize minima to impossible/high values
    result.left.distance_mm  = UINT16_MAX;
    result.front.distance_mm = UINT16_MAX;
    result.right.distance_mm = UINT16_MAX;

    result.left.timestamp_ms  = 0;
    result.front.timestamp_ms = 0;
    result.right.timestamp_ms = 0;

    unsigned long scanStart = millis();
    unsigned long lastSample = 0;

    // ------------------------------------------------
    // Sweep 1: turn on the spot in one direction
    // ------------------------------------------------

    car.turn(fast * (1));

    unsigned long sweepStart = millis();

    while (millis() - sweepStart < SWEEP_TIME)
    {
        if (millis() - lastSample < SAMPLE_INTERVAL)
            continue;

        lastSample = millis();

        readSensors(STAGE_OPEN_AREA);

        unsigned long t = millis() - scanStart;

        // LEFT LASER
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserLeft_mm < result.left.distance_mm)
        {
            result.left.distance_mm = currentData.laserLeft_mm;
            result.left.timestamp_ms = t;
        }

        // FRONT LASER
        if (currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm < result.front.distance_mm)
        {
            result.front.distance_mm = currentData.laserFront_mm;
            result.front.timestamp_ms = t;
        }

        // RIGHT LASER
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm < result.right.distance_mm)
        {
            result.right.distance_mm = currentData.laserRight_mm;
            result.right.timestamp_ms = t;
        }
    }

    car.stop();

    delay(100);


    // ------------------------------------------------
    // Sweep 2: turn back in the opposite direction
    // ------------------------------------------------

    car.turn(fast * (-1));  // Replace with your actual car function

    sweepStart = millis();

    // Twice as long:
    // from LEFT extreme -> through original direction -> RIGHT extreme
    while (millis() - sweepStart < 2 * SWEEP_TIME)
    {
        if (millis() - lastSample < SAMPLE_INTERVAL)
            continue;

        lastSample = millis();

        readSensors(STAGE_OPEN_AREA);

        unsigned long t = millis() - scanStart;

        // LEFT LASER
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserLeft_mm < result.left.distance_mm)
        {
            result.left.distance_mm = currentData.laserLeft_mm;
            result.left.timestamp_ms = t;
        }

        // FRONT LASER
        if (currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm < result.front.distance_mm)
        {
            result.front.distance_mm = currentData.laserFront_mm;
            result.front.timestamp_ms = t;
        }

        // RIGHT LASER
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm < result.right.distance_mm)
        {
            result.right.distance_mm = currentData.laserRight_mm;
            result.right.timestamp_ms = t;
        }
    }

    car.stop();

    car.turn(fast * (1));

    delay(SWEEP_TIME);

    car.stop();

    return result;
}