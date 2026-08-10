#include <Arduino.h>
#include "Sensors.h"

RobotStage currentStage = STAGE_OPEN_AREA;

void setup()
{
    Serial.begin(9600);

    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(whiteLedPin, OUTPUT);
    pinMode(redLedPin, OUTPUT);
    pinMode(greenLedPin, OUTPUT);

    digitalWrite(whiteLedPin, LOW);
    digitalWrite(redLedPin, LOW);
    digitalWrite(greenLedPin, LOW);

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

    Serial.println("2nd breakpoint");
    Serial.println(currentStage);
    delay(1000);
    
    switch (currentStage)
    {
        case STAGE_OPEN_AREA:
            // Example of changing which sensor set will be acquired:
            openAreaNavigation(sensors);
            currentStage = STAGE_FUNNEL;
            Serial.println("3rd breakpoint");
            Serial.println(currentStage);
            //delay(500);
            car.stop();
            break;

        case STAGE_FUNNEL:
            funnelNavigation(sensors);
            //currentStage = STAGE_WALL_FOLLOWING;
            break;
/*
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
            break;*/
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