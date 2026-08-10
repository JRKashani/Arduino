#include "Sensors.h"
#include <math.h>


// ============================================================
// FILTER SETTINGS
// ============================================================

const uint8_t DISTANCE_AVERAGE_COUNT = 3;

const unsigned long ANALOG_WINDOW_US = 10000UL;


// ============================================================
// SIMPLE 3-SAMPLE ROLLING AVERAGE
// Used for HC-SR04 and VL53L0X.
// ============================================================

struct RollingAverage3
{
    float values[DISTANCE_AVERAGE_COUNT];
    uint8_t count;
    uint8_t next;
};


RollingAverage3 US60Filter  = {{0}, 0, 0};
RollingAverage3 US120Filter = {{0}, 0, 0};
RollingAverage3 US240Filter = {{0}, 0, 0};
RollingAverage3 US300Filter = {{0}, 0, 0};

RollingAverage3 laserLeftFilter  = {{0}, 0, 0};
RollingAverage3 laserRightFilter = {{0}, 0, 0};


// ============================================================
// ROLLING-AVERAGE HELPERS
// ============================================================

void addReading(RollingAverage3 &filter, float value)
{
    filter.values[filter.next] = value;

    filter.next++;

    if (filter.next >= DISTANCE_AVERAGE_COUNT)
        filter.next = 0;

    if (filter.count < DISTANCE_AVERAGE_COUNT)
        filter.count++;
}


float getAverage(const RollingAverage3 &filter)
{
    if (filter.count == 0)
        return NAN;

    float sum = 0.0f;

    for (uint8_t i = 0; i < filter.count; i++)
        sum += filter.values[i];

    return sum / filter.count;
}


// ============================================================
// HC-SR04
// ============================================================

float readUltrasonic(
    Ultrasonic &sensor,
    RollingAverage3 &filter,
    bool &valid)
{
    long distance_cm = sensor.ranging(CM);

    // Library returns 0 when pulseIn() times out.
    if (distance_cm <= 0)
    {
        valid = false;

        // Do NOT insert zero into the moving average.
        return NAN;
    }

    valid = true;

    addReading(filter, (float)distance_cm);

    return getAverage(filter);
}


// ============================================================
// VL53L0X
// ============================================================

float readLaser(
    VL53L0X &sensor,
    RollingAverage3 &filter,
    bool &valid)
{
    uint16_t distance_mm =
        sensor.readRangeContinuousMillimeters();

    bool timedOut = sensor.timeoutOccurred();

    if (timedOut || distance_mm == 65535)
    {
        valid = false;

        // Again: do not contaminate the average with the
        // timeout value.
        return NAN;
    }

    valid = true;

    addReading(filter, (float)distance_mm);

    return getAverage(filter);
}


// ============================================================
// SHARP GP2Y0A21YK
//
// This conversion is only an approximation.
// We should replace it with calibration data later.
// ============================================================

float sharpDistanceCm(float adc)
{
    float voltage = adc * 5.0f / 1023.0f;

    if (voltage <= 0.05f)
        return NAN;

    float distance_cm = 27.0f / voltage;

    // Nominal useful range of the GP2Y0A21YK.
    if (distance_cm < 10.0f || distance_cm > 80.0f)
        return NAN;

    return distance_cm;
}


// ============================================================
// ADXL335
//
// Initial nominal calibration.
// Assumptions:
//   ADXL335 powered at 3.3 V
//   Mega ADC reference = 5 V
//
// These MUST eventually be replaced with measured calibration.
// ============================================================

const float ADXL_ZERO_X = 337.5f;
const float ADXL_ZERO_Y = 337.5f;
const float ADXL_ZERO_Z = 337.5f;

const float ADXL_COUNTS_PER_G = 67.5f;


float adxlToG(float adc, float zero)
{
    return (adc - zero) / ADXL_COUNTS_PER_G;
}


// ============================================================
// 10 ms WADI ANALOG WINDOW
//
// Samples:
//   IR left
//   IR right
//   accel X
//   accel Y
//   accel Z
//
// repeatedly for approximately 10 ms.
// ============================================================

void readWadiAnalog(SensorReading &data)
{
    uint32_t sumIRLeft  = 0;
    uint32_t sumIRRight = 0;

    uint32_t sumX = 0;
    uint32_t sumY = 0;
    uint32_t sumZ = 0;

    uint16_t samples = 0;

    unsigned long start_us = micros();

    do
    {
        sumIRLeft  += analogRead(IRLeftPin);
        sumIRRight += analogRead(IRRightPin);

        sumX += analogRead(accelXPin);
        sumY += analogRead(accelYPin);
        sumZ += analogRead(accelZPin);

        samples++;
    }
    while ((micros() - start_us) < ANALOG_WINDOW_US);


    float irLeftAverage =
        (float)sumIRLeft / samples;

    float irRightAverage =
        (float)sumIRRight / samples;

    float xAverage =
        (float)sumX / samples;

    float yAverage =
        (float)sumY / samples;

    float zAverage =
        (float)sumZ / samples;


    // Raw IR averages
    data.IRLeft_adc  = irLeftAverage;
    data.IRRight_adc = irRightAverage;


    // Approximate converted distances
    data.IRLeft_cm  =
        sharpDistanceCm(irLeftAverage);

    data.IRRight_cm =
        sharpDistanceCm(irRightAverage);


    data.IRLeft_valid =
        !isnan(data.IRLeft_cm);

    data.IRRight_valid =
        !isnan(data.IRRight_cm);


    // Accelerations
    data.accelX_g =
        adxlToG(xAverage, ADXL_ZERO_X);

    data.accelY_g =
        adxlToG(yAverage, ADXL_ZERO_Y);

    data.accelZ_g =
        adxlToG(zAverage, ADXL_ZERO_Z);
}


// ============================================================
// 10 ms LIGHT-SENSOR WINDOW
// ============================================================

void readLightSensors(SensorReading &data)
{
    uint32_t sumLeft  = 0;
    uint32_t sumRight = 0;

    uint16_t samples = 0;

    unsigned long start_us = micros();

    do
    {
        sumLeft += analogRead(photoLeftPin);
        sumRight += analogRead(photoRightPin);

        samples++;
    }
    while ((micros() - start_us) < ANALOG_WINDOW_US);


    data.photoLeft_adc =
        (float)sumLeft / samples;

    data.photoRight_adc =
        (float)sumRight / samples;
}


// ============================================================
// CLEAR UNUSED VALUES
//
// This is important.
//
// If we move from stage 2 to stage 4, we do not want an old
// ultrasonic value sitting in currentData and looking current.
// ============================================================

void clearSensorReading(SensorReading &data)
{
    data.timestamp = millis();

    data.US60_cm  = NAN;
    data.US120_cm = NAN;
    data.US240_cm = NAN;
    data.US300_cm = NAN;

    data.US60_valid  = false;
    data.US120_valid = false;
    data.US240_valid = false;
    data.US300_valid = false;


    data.laserFront_mm  = NAN;
    data.laserRight_mm = NAN;

    data.laserFront_valid  = false;
    data.laserRight_valid = false;


    data.IRLeft_adc  = NAN;
    data.IRRight_adc = NAN;

    data.IRLeft_cm  = NAN;
    data.IRRight_cm = NAN;

    data.IRLeft_valid  = false;
    data.IRRight_valid = false;


    data.photoLeft_adc  = NAN;
    data.photoRight_adc = NAN;


    data.accelX_g = NAN;
    data.accelY_g = NAN;
    data.accelZ_g = NAN;
}


// ============================================================
// MAIN SENSOR FUNCTION
// ============================================================

const SensorReading& readSensors(uint8_t stage)
{
    clearSensorReading(currentData);


    switch (stage)
    {
        // ----------------------------------------------------
        // STAGES 1-3
        //
        // For now acquire all ultrasonic sensors.
        // We can reduce the set later once the exact control
        // logic for each stage is fixed.
        // ----------------------------------------------------

        case 1:
        case 2:
        case 3:

            currentData.US60_cm =
                readUltrasonic(
                    US60_cm,
                    US60Filter,
                    currentData.US60_valid);

            currentData.US120_cm =
                readUltrasonic(
                    US120_cm,
                    US120Filter,
                    currentData.US120_valid);

            currentData.US240_cm =
                readUltrasonic(
                    US240_cm,
                    US240Filter,
                    currentData.US240_valid);

            currentData.US300_cm =
                readUltrasonic(
                    US300_cm,
                    US300Filter,
                    currentData.US300_valid);

            break;


        // ----------------------------------------------------
        // STAGE 4 - WADI
        //
        // 3 accelerometer axes
        // 2 Sharp IR sensors
        // 2 VL53L0X sensors
        // ----------------------------------------------------

        case 4:

            readWadiAnalog(currentData);

            currentData.laserFront_mm =
                readLaser(
                    laserFront_mm,
                    laserFrontFilter,
                    currentData.laserFront_valid);

            currentData.laserRight_mm =
                readLaser(
                    laserRight_mm,
                    laserRightFilter,
                    currentData.laserRight_valid);

            break;


        // ----------------------------------------------------
        // STAGE 5 - LIGHT
        //
        // LDR pair for steering toward light.
        // VL53L0X pair for final stopping distance.
        // ----------------------------------------------------

        case 5:

            readLightSensors(currentData);

            currentData.laserFront_mm =
                readLaser(
                    laserFront_mm,
                    laserFrontFilter,
                    currentData.laserFront_valid);

            currentData.laserRight_mm =
                readLaser(
                    laserRight_mm,
                    laserRightFilter,
                    currentData.laserRight_valid);

            break;


        default:

            // Invalid stage.
            // currentData remains entirely invalid/NAN.
            break;
    }


    currentData.timestamp = millis();

    return currentData;
}