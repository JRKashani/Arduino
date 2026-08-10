#include <Wire.h>
#include <VL53L0X.h>

// ============================================================
// TEST MODE
// 0 = original single-sensor continuous test
// 1 = three sensors, XSHUT round-robin
// ============================================================

#define THREE_LASER_ROUND_ROBIN 1


VL53L0X sensor;


// ============================================================
// THREE-SENSOR CONFIGURATION
// ============================================================

#if THREE_LASER_ROUND_ROBIN

const uint8_t LASER_COUNT = 3;

const uint8_t xshutPins[LASER_COUNT] =
{
    30,     // Laser 0 - Left
    31,     // Laser 1 - Right
    33      // Laser 2 - front
};

uint8_t currentLaser = 0;


// Shut down one VL53L0X.
//
// IMPORTANT:
// XSHUT is NOT 5 V tolerant on the Pololu #2490.
// Driving LOW is safe.
void laserOff(uint8_t pin)
{
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
}


// Wake one VL53L0X.
//
// Do NOT digitalWrite(pin, HIGH) on a Mega.
// INPUT makes the pin high-impedance, allowing the
// VL53L0X board's internal 2.8 V pull-up to enable it.
void laserOn(uint8_t pin)
{
    pinMode(pin, INPUT);

    // Give the sensor a little time to boot.
    delay(5);
}


void allLasersOff()
{
    for (uint8_t i = 0; i < LASER_COUNT; i++)
    {
        laserOff(xshutPins[i]);
    }
}


// Reads ONE sensor.
// All other sensors must already be shut down.
uint16_t readLaser(uint8_t index)
{
    laserOn(xshutPins[index]);

    // XSHUT resets the VL53L0X, so initialization
    // is required every time it is awakened.
    if (!sensor.init())
    {
        Serial.print("Laser ");
        Serial.print(index);
        Serial.println(": INIT FAILED");

        laserOff(xshutPins[index]);

        return 0xFFFF;
    }

    sensor.setTimeout(500);

    uint16_t distance =
        sensor.readRangeSingleMillimeters();

    if (sensor.timeoutOccurred())
    {
        Serial.print("Laser ");
        Serial.print(index);
        Serial.println(": TIMEOUT");

        distance = 0xFFFF;
    }

    laserOff(xshutPins[index]);

    return distance;
}

#endif


// ============================================================
// SETUP
// ============================================================

void setup()
{
    Serial.begin(9600);
    delay(1000);

    Wire.begin();

    Serial.println("Starting VL53L0X test...");


#if THREE_LASER_ROUND_ROBIN

    allLasersOff();

    delay(10);

    Serial.println("3-laser XSHUT round-robin mode.");
    Serial.println("XSHUT pins: 22, 23, 24");

#else

    sensor.setTimeout(500);

    Serial.println("Calling sensor.init()...");

    if (!sensor.init())
    {
        Serial.println("INIT FAILED");
    }
    else
    {
        Serial.println("INIT SUCCESS");

        sensor.startContinuous(100);
    }

#endif
}


// ============================================================
// LOOP
// ============================================================

void loop()
{

#if THREE_LASER_ROUND_ROBIN

    uint16_t distance = readLaser(currentLaser);

    Serial.print("Laser ");
    Serial.print(currentLaser);
    Serial.print(": ");

    if (distance == 0xFFFF)
    {
        Serial.println("INVALID");
    }
    else
    {
        Serial.print(distance);
        Serial.println(" mm");
    }

    // Move to next sensor:
    // 0 -> 1 -> 2 -> 0 -> ...
    currentLaser++;

    if (currentLaser >= LASER_COUNT)
    {
        currentLaser = 0;

        Serial.println("----------------");
    }

    delay(20);


#else

    uint16_t distance =
        sensor.readRangeContinuousMillimeters();

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    if (sensor.timeoutOccurred())
    {
        Serial.println("TIMEOUT");
    }

    delay(100);

#endif
}