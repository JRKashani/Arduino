
#pragma once

#if defined(__has_include)
#  if __has_include(<Arduino.h>)
#    include <Arduino.h>
#    include <Sensors.h>
#    include "Globals.h"
#  else
#    include <cstdint>
#    include <cstddef>
#    include <cstdio>

using byte = uint8_t;
using boolean = bool;

struct SerialClass
{
    void begin(unsigned long) {}
    void print(const char *) {}
    void print(char) {}
    void print(int) {}
    void print(unsigned int) {}
    void print(long) {}
    void print(unsigned long) {}
    void print(float) {}
    void println(const char *) {}
    void println(char) {}
    void println(int) {}
    void println(unsigned int) {}
    void println(long) {}
    void println(unsigned long) {}
    void println(float) {}
    void println() {}
};

inline SerialClass Serial;

inline void pinMode(int, int) {}
inline void digitalWrite(int, int) {}
inline void delay(unsigned long) {}
inline unsigned long millis() { return 0; }

#ifndef LED_BUILTIN
#define LED_BUILTIN 13
#endif

#ifndef OUTPUT
#define OUTPUT 1
#endif

#ifndef HIGH
#define HIGH 1
#endif

#ifndef LOW
#define LOW 0
#endif

struct SensorReading
{
    bool US60_valid = false;
    float US60_cm = 0.0f;
    bool laserFront_valid = false;
    float laserFront_mm = 0.0f;
};

enum RobotStage
{
    STAGE_OPEN_AREA,
    STAGE_FUNNEL,
    STAGE_WALL_FOLLOWING,
    STAGE_WADI,
    STAGE_LIGHT
};

struct FakeCar
{
    void setBias(int) {}
    template <typename... Args>
    void attach(Args...) {}
    void flipRight() {}
    void stop() {}
};

inline FakeCar car;

static constexpr int whiteLedPin = 13;
static constexpr int LEFT_DIR_1 = 2;
static constexpr int LEFT_DIR_2 = 3;
static constexpr int LEFT_PWM = 4;
static constexpr int RIGHT_DIR_1 = 5;
static constexpr int RIGHT_DIR_2 = 6;
static constexpr int RIGHT_PWM = 7;

bool setupSensors() { return true; }
const SensorReading &readSensors(uint8_t) { static SensorReading sensors; return sensors; }

void openAreaNavigation(const SensorReading &) {}
void funnelNavigation(const SensorReading &) {}
void wallFollowingNavigation(const SensorReading &) {}
void wadiNavigation(const SensorReading &) {}
void lightNavigation(const SensorReading &) {}
#  endif
#else
#  include <Arduino.h>
#  include <Sensors.h>
#  include "Globals.h"
#endif

RobotStage currentStage = STAGE_OPEN_AREA;

void setup()
{
    Serial.begin(9600);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(whiteLedPin, OUTPUT);
    digitalWrite(whiteLedPin, LOW);

    car.setBias(1);
    car.attach(
        LEFT_DIR_1, LEFT_DIR_2, LEFT_PWM,
        RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_PWM);
    car.flipRight();
    car.stop();

    if (!setupSensors())
    {
        Serial.println("VL53L0X initialization failed");
        digitalWrite(LED_BUILTIN, HIGH);
        car.stop();

        while (true)
        {
            // Do not start autonomous movement with failed laser setup.

        }
    }

    Serial.println("Sensors ready");
}


void loop()
{
    // One acquisition pass for the sensors relevant to the
    // current stage.
    const SensorReading &sensors = readSensors((uint8_t)currentStage);
    
    switch (currentStage)
    {
        case STAGE_OPEN_AREA:
            // Example of changing which sensor set will be acquired:
            openAreaNavigation(sensors);
            currentStage = STAGE_FUNNEL;
            break;

        case STAGE_FUNNEL:
            funnelNavigation(sensors);
            currentStage = STAGE_WALL_FOLLOWING;
            break;

        case STAGE_WALL_FOLLOWING:
            wallFollowingNavigation(sensors);
            currentStage = STAGE_WADI;
            break;

        case STAGE_WADI:
            wadiNavigation(sensors);
            currentStage = STAGE_LIGHT;
            break;

        case STAGE_LIGHT:
            lightNavigation(sensors);
            car.stop();
            delay(120000);
            break;
    }

    // --------------------------------------------------------
    // YOUR IMPLEMENTATION GOES HERE.
    // Examples below only demonstrate how to access data.
    // They are not navigation logic.
    // --------------------------------------------------------

    /*if (sensors.US60_valid)
    {
        Serial.print("US60 [cm]: ");
        Serial.println(sensors.US60_cm);
    }*/

    /*if (sensors.laserFront_valid)
    {
        Serial.print("Front laser [mm]: ");
        Serial.println(sensors.laserFront_mm);
    }*/

    // Example of changing which sensor set will be acquired:
    // currentStage = STAGE_WADI;
    // currentStage = STAGE_LIGHT;
}