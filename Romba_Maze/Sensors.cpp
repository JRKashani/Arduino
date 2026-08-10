#include "Sensors.h"

#if defined(__has_include)
    #if __has_include(<Wire.h>)
        #include <Wire.h>
    #else
        #include <stdint.h>
        #include <stddef.h>

        class TwoWireStub
        {
        public:
            void begin() {}
            uint8_t beginTransmission(uint8_t) { return 0; }
            uint8_t endTransmission() { return 0; }
        };

        static TwoWireStub Wire;
    #endif
#else
    #include <Wire.h>
#endif

#include <math.h>


// ============================================================
// SETTINGS
// ============================================================

static const uint8_t DISTANCE_AVERAGE_COUNT = 3;
static const unsigned long ANALOG_WINDOW_US = 10000UL;
static const uint16_t LASER_TIMEOUT_MS = 50;

static const uint8_t LASER_FRONT_ADDRESS = 0x30;
static const uint8_t LASER_RIGHT_ADDRESS = 0x31;


// ============================================================
// SIMPLE 3-SAMPLE ROLLING AVERAGE
// ============================================================

struct RollingAverage3
{
    float values[DISTANCE_AVERAGE_COUNT];
    uint8_t count;
    uint8_t next;
};

static RollingAverage3 US60Filter  = {{0}, 0, 0};
static RollingAverage3 US120Filter = {{0}, 0, 0};
static RollingAverage3 US300Filter = {{0}, 0, 0};

static RollingAverage3 laserFrontFilter = {{0}, 0, 0};
static RollingAverage3 laserRightFilter = {{0}, 0, 0};


static void addReading(RollingAverage3 &filter, float value)
{
    filter.values[filter.next] = value;

    filter.next++;
    if (filter.next >= DISTANCE_AVERAGE_COUNT)
        filter.next = 0;

    if (filter.count < DISTANCE_AVERAGE_COUNT)
        filter.count++;
}


static float getAverage(const RollingAverage3 &filter)
{
    if (filter.count == 0)
        return NAN;

    float sum = 0.0f;

    for (uint8_t i = 0; i < filter.count; i++)
        sum += filter.values[i];

    return sum / (float)filter.count;
}


// ============================================================
// HC-SR04
// ============================================================

static float readUltrasonic(
    Ultrasonic &sensor,
    RollingAverage3 &filter,
    bool &valid)
{
    const long distance_cm = sensor.ranging(CM);

    // In the supplied Ultrasonic library, zero means timeout /
    // no valid echo. Do not insert it into the filter.
    if (distance_cm <= 0)
    {
        valid = false;
        return NAN;
    }

    valid = true;
    addReading(filter, (float)distance_cm);
    return getAverage(filter);
}


void readUltrasonics(SensorReading &data)
{
    // Sequential reads avoid intentionally firing the ultrasonic
    // sensors at the same time.
    data.US60_cm = readUltrasonic(
        US60, US60Filter, data.US60_valid);

    data.US120_cm = readUltrasonic(
        US120, US120Filter, data.US120_valid);

    data.US300_cm = readUltrasonic(
        US300, US300Filter, data.US300_valid);
}


// ============================================================
// VL53L0X
// ============================================================

static void holdLaserInShutdown(uint8_t xshutPin)
{
    pinMode(xshutPin, OUTPUT);
    digitalWrite(xshutPin, LOW);
}


static void releaseLaserFromShutdown(uint8_t xshutPin)
{
    // Pololu #2490 XSHUT is pulled up on the carrier and is not
    // 5 V tolerant. On a Mega, release it to high impedance
    // instead of actively driving it HIGH.
    pinMode(xshutPin, INPUT);
}
/*
static void scanI2C(const char* message)
{
    Serial.println();
    Serial.println(message);

    bool foundAny = false;

    for (uint8_t address = 1; address < 127; address++)
    {
        Wire.beginTransmission(address);
        uint8_t error = Wire.endTransmission();

        if (error == 0)
        {
            Serial.print("  Found I2C device at 0x");

            if (address < 16)
                Serial.print('0');

            Serial.println(address, HEX);
            foundAny = true;
        }
    }

    if (!foundAny)
        Serial.println("  No I2C devices found");

    Serial.println();
}
*/
/*bool setupSensors()
{
    
    Wire.begin();

    // Both sensors start at 0x29, so keep both disabled first.
    holdLaserInShutdown(LASER_FRONT_XSHUT_PIN);
    holdLaserInShutdown(LASER_RIGHT_XSHUT_PIN);
    delay(10);

    // Front laser: enable alone, initialize, assign unique address.
    releaseLaserFromShutdown(LASER_FRONT_XSHUT_PIN);
    delay(10);

    laserFront.setTimeout(LASER_TIMEOUT_MS);
    if (!laserFront.init())
        return false;

    laserFront.setAddress(LASER_FRONT_ADDRESS);

    // Right laser: now enable it and assign another unique address.
    releaseLaserFromShutdown(LASER_RIGHT_XSHUT_PIN);
    delay(10);

    laserRight.setTimeout(LASER_TIMEOUT_MS);
    if (!laserRight.init())
        return false;

    laserRight.setAddress(LASER_RIGHT_ADDRESS);

    // Continuous ranging lets loop() read the latest measurement
    // without starting a new single-shot measurement each time.
    laserFront.startContinuous();
    laserRight.startContinuous();

    return true;
    */
   bool setupSensors()
{
    //Serial.println("=== VL53L0X DEBUG ===");

    Wire.begin();
    delay(50);

    // ========================================================
    // TEST 0: before touching XSHUT
    // ========================================================

    //scanI2C("Before touching XSHUT:");

    // ========================================================
    // TEST 1: shut down BOTH
    // ========================================================

    //Serial.println("Driving BOTH XSHUT pins LOW");

    holdLaserInShutdown(LASER_FRONT_XSHUT_PIN);
    holdLaserInShutdown(LASER_RIGHT_XSHUT_PIN);

    delay(50);

    //scanI2C("Both sensors should now be OFF:");

    // ========================================================
    // TEST 2: release FRONT only
    // ========================================================

    //Serial.println("Releasing FRONT only");

    releaseLaserFromShutdown(LASER_FRONT_XSHUT_PIN);

    delay(100);

    //scanI2C("Only FRONT should now appear at 0x29:");

    // ========================================================
    // TEST 3: initialize FRONT
    // ========================================================

    //Serial.println("Calling laserFront.init()");

    if (!laserFront.init())
    {
        Serial.println("ERROR: FRONT init failed");
        return false;
    }

    //Serial.println("FRONT init SUCCESS");

    laserFront.setTimeout(LASER_TIMEOUT_MS);

    // ========================================================
    // TEST 4: change FRONT address
    // ========================================================

    //Serial.println("Moving FRONT from 0x29 to 0x30");

    laserFront.setAddress(LASER_FRONT_ADDRESS);

    delay(50);

    //scanI2C("FRONT should now appear at 0x30:");

    // ========================================================
    // TEST 5: release RIGHT
    // ========================================================

    //Serial.println("Releasing RIGHT");

    releaseLaserFromShutdown(LASER_RIGHT_XSHUT_PIN);

    delay(100);

    //scanI2C("Expected: RIGHT=0x29, FRONT=0x30:");

    // ========================================================
    // TEST 6: initialize RIGHT
    // ========================================================

    //Serial.println("Calling laserRight.init()");

    if (!laserRight.init())
    {
        Serial.println("ERROR: RIGHT init failed");
        return false;
    }

    //Serial.println("RIGHT init SUCCESS");

    laserRight.setTimeout(LASER_TIMEOUT_MS);

    laserRight.setAddress(LASER_RIGHT_ADDRESS);

    delay(50);

    //scanI2C("Final expected addresses: 0x30 and 0x31:");

    // ========================================================

    laserFront.startContinuous();
    laserRight.startContinuous();

    //Serial.println("=== BOTH LASERS READY ===");

    return true;
}
//}


static float readLaser(
    VL53L0X &sensor,
    RollingAverage3 &filter,
    bool &valid)
{
    const uint16_t distance_mm =
        sensor.readRangeContinuousMillimeters();

    const bool timedOut = sensor.timeoutOccurred();

    if (timedOut || distance_mm == 65535U)
    {
        valid = false;
        return NAN;
    }

    valid = true;
    addReading(filter, (float)distance_mm);
    return getAverage(filter);
}


void readLasers(SensorReading &data)
{
    data.laserFront_mm = readLaser(
        laserFront,
        laserFrontFilter,
        data.laserFront_valid);

    data.laserRight_mm = readLaser(
        laserRight,
        laserRightFilter,
        data.laserRight_valid);
}


// ============================================================
// SHARP GP2Y0A21YK IR - NOT CURRENTLY USED
// ============================================================

/*
static float sharpDistanceCm(float adc)
{
    float voltage = adc * 5.0f / 1023.0f;

    if (voltage <= 0.05f)
        return NAN;

    float distance_cm = 27.0f / voltage;

    if (distance_cm < 10.0f || distance_cm > 80.0f)
        return NAN;

    return distance_cm;
}

static void readInfraredSensors(SensorReading &data)
{
    // This block is intentionally disabled for the current build.
    // Re-enable only after the IR hardware returns to the robot.
}
*/


// ============================================================
// ADXL335
// ============================================================

// These are the provisional values already used in the project.
// They are NOT final calibration values.
static const float ADXL_ZERO_X = 337.5f;
static const float ADXL_ZERO_Y = 337.5f;
static const float ADXL_ZERO_Z = 337.5f;
static const float ADXL_COUNTS_PER_G = 67.5f;


static float adxlToG(float adc, float zero)
{
    return (adc - zero) / ADXL_COUNTS_PER_G;
}


void readAccelerometer(SensorReading &data)
{
    uint32_t sumX = 0;
    uint32_t sumY = 0;
    uint32_t sumZ = 0;
    uint16_t samples = 0;

    const unsigned long start_us = micros();

    do
    {
        sumX += analogRead(ACCEL_X_PIN);
        sumY += analogRead(ACCEL_Y_PIN);
        sumZ += analogRead(ACCEL_Z_PIN);
        samples++;
    }
    while ((micros() - start_us) < ANALOG_WINDOW_US);

    const float xAverage = (float)sumX / (float)samples;
    const float yAverage = (float)sumY / (float)samples;
    const float zAverage = (float)sumZ / (float)samples;

    data.accelX_g = adxlToG(xAverage, ADXL_ZERO_X);
    data.accelY_g = adxlToG(yAverage, ADXL_ZERO_Y);
    data.accelZ_g = adxlToG(zAverage, ADXL_ZERO_Z);
}


// ============================================================
// LDR
// ============================================================

void readLightSensors(SensorReading &data)
{
    uint32_t sumLeft = 0;
    uint32_t sumRight = 0;
    uint16_t samples = 0;

    const unsigned long start_us = micros();

    do
    {
        sumLeft += analogRead(LDR_LEFT_PIN);
        sumRight += analogRead(LDR_RIGHT_PIN);
        samples++;
    }
    while ((micros() - start_us) < ANALOG_WINDOW_US);

    data.photoLeft_adc = (float)sumLeft / (float)samples;
    data.photoRight_adc = (float)sumRight / (float)samples;
}


// ============================================================
// CLEAR DATA BEFORE EACH ACQUISITION PASS
// ============================================================

void clearSensorReading(SensorReading &data)
{
    data.timestamp = millis();

    data.US60_cm = NAN;
    data.US120_cm = NAN;
    data.US300_cm = NAN;

    data.US60_valid = false;
    data.US120_valid = false;
    data.US300_valid = false;

    data.laserFront_mm = NAN;
    data.laserRight_mm = NAN;

    data.laserFront_valid = false;
    data.laserRight_valid = false;

    /*
    data.IRLeft_adc = NAN;
    data.IRRight_adc = NAN;
    data.IRLeft_cm = NAN;
    data.IRRight_cm = NAN;
    data.IRLeft_valid = false;
    data.IRRight_valid = false;
    */

    data.photoLeft_adc = NAN;
    data.photoRight_adc = NAN;

    data.accelX_g = NAN;
    data.accelY_g = NAN;
    data.accelZ_g = NAN;
}


// ============================================================
// MAIN SENSOR ENTRY POINT
// ============================================================

const SensorReading& readSensors(uint8_t stage)
{
    clearSensorReading(currentData);

    switch (stage)
    {
        case STAGE_OPEN_AREA:
        case STAGE_FUNNEL:
        case STAGE_WALL_FOLLOWING:
            // Starter acquisition set only. No navigation logic here.
            readUltrasonics(currentData);
            readLasers(currentData);
            break;

        case STAGE_WADI:
            readLasers(currentData);
            readAccelerometer(currentData);
            break;

        case STAGE_LIGHT:
            readLasers(currentData);
            readLightSensors(currentData);
            break;

        default:
            // Unknown stage: data remains invalid/NAN.
            break;
    }

    currentData.timestamp = millis();
    return currentData;
}