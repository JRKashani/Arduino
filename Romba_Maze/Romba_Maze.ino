#include "Sensors.h"

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

    // --------------------------------------------------------
    // YOUR IMPLEMENTATION GOES HERE.
    // Examples below only demonstrate how to access data.
    // They are not navigation logic.
    // --------------------------------------------------------

    if (sensors.US60_valid)
    {
        Serial.print("US60 [cm]: ");
        Serial.println(sensors.US60_cm);
    }

    if (sensors.laserFront_valid)
    {
        Serial.print("Front laser [mm]: ");
        Serial.println(sensors.laserFront_mm);
    }

    // Example of changing which sensor set will be acquired:
    // currentStage = STAGE_WADI;
    // currentStage = STAGE_LIGHT;
}