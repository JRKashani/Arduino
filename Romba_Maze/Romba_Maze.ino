
#include "Sensors.h"


bool right_or_left = false;

void setup()
{
    Serial.begin(9600);
    pinMode(LED_BUILTIN, OUTPUT);
    pinMode(whiteLedPin, OUTPUT);
    digitalWrite(whiteLedPin, LOW);

    car.setBias(1);
    car.attach(46, 48, 44, 49, 47, 45);
    car.flipRight();
}

void loop()
{   
    while(!stage_1to2)
    {        
        currentData.US60_cm = readSensors(US60_cm, US60Filter, currentData.US60_valid);
        currentData.US300_cm = readSensors(US300_cm, US300Filter, currentData.US300_valid);
        currentData.LaserFront_mm = readSensors(laserFront_mm, laserFrontFilter, currentData.LaserFront_valid);

        if (currentData.US60_cm < 50 && currentData.US300_cm < 50)
        {
            stage_1to2 = true;
            digitalWrite(whiteLedPin, HIGH);
        }
        else if (10*currentData.LaserFront_mm < 10)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            if (currentData.US60_cm < currentData.US300_cm)
            {
                right_or_left = true;
            }
            else
            {
                right_or_left = false;
            }
            car.stop();
            car.turn(20*right_or_left);
        }
        else 
        {
            stage_1to2 = false;
            car.straight(slow);
        }        
    } 
    while(!stage_2to3)
    {
        currentData.US60_cm = readSensors(US60_cm, US60Filter, currentData.US60_valid);
        currentData.US300_cm = readSensors(US300_cm, US300Filter, currentData.US300_valid);
        currentData.LaserFront_mm = readSensors(laserFront_mm, laserFrontFilter, currentData.LaserFront_valid);

        if (currentData.US60_cm < 20 && currentData.US300_cm < 20)
        {
            stage_2to3 = true;
        }
        else if (10*currentData.LaserFront_mm < 10)
        {
            if (currentData.US60_cm < currentData.US300_cm)
            {
                right_or_left = 1;
            }
            else
            {
                right_or_left = -1;
            }
            car.stop();
            car.turn(20*right_or_left);
        }
        else 
        {
            stage_2to3 = false;
            car.straight(slow);
        }        
    }


    switch (getStage())
    {
        case STAGE_OPEN_AREA:
            car.straight(slow);
            break;
        case STAGE_FUNNEL:
            /* code */
            break;
        case STAGE_WALL_FOLLOWING:
            /* code */
            break;
        case STAGE_WADI:
            /* code */
            break;
        case STAGE_LIGHT:
            /* code */
            break;
        default:
            break;
    }
    
}