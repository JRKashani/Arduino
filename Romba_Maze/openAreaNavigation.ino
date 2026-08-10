void openAreaNavigation(const SensorReading &sensors)
{
    bool isLeftFrontClose = false;
    bool isRightFrontClose = false;0;   // -1 for left, 1 for right

    while (!isLeftFrontClose && !isRightFrontClose)
    {
        /*delay(1000);
        digitalWrite(LED_BUILTIN, LOW);
        digitalWrite(whiteLedPin, LOW);
        digitalWrite(redLedPin, LOW);
        //digitalWrite(greenLedPin, LOW);
        digitalWrite(greenLedPin, HIGH);*/

        // Refresh sensor data.
        // readSensors() updates currentData, which sensors references.
        readSensors(STAGE_OPEN_AREA);

        laserLeftDistance  = sensors.laserLeft_mm;
        laserRightDistance = sensors.laserRight_mm;
        laserFrontDistance = sensors.laserFront_mm;

        Serial.print("laserLeftDistance: ");
        Serial.println(laserLeftDistance);

        Serial.print("laserRightDistance: ");
        Serial.println(laserRightDistance);

        Serial.print("laserFrontDistance: ");
        Serial.println(laserFrontDistance);

        car.straight(fast);
        //digitalWrite(LED_BUILTIN, HIGH);

        isLeftFrontClose =
            sensors.laserLeft_valid &&
            laserLeftDistance < 500.0f;

        isRightFrontClose =
            sensors.laserRight_valid &&
            laserRightDistance < 500.0f;

        if ((isLeftFrontClose || isRightFrontClose) &&
            sensors.laserFront_valid &&
            laserFrontDistance < 300.0f)
        {
            //digitalWrite(LED_BUILTIN, LOW);

            if (isLeftFrontClose && !isRightFrontClose)
            {
                // Obstacle only on left -> go right
                right_or_left = -1;
            }
            else if (isRightFrontClose && !isLeftFrontClose)
            {
                // Obstacle only on right -> go left
                right_or_left = 1;
            }
            else
            {
                // Both sides are close.
                // Turn toward the side with MORE free space.
                right_or_left =
                    (laserLeftDistance < laserRightDistance)
                    ? 1     // left closer -> turn right
                    : -1;   // right closer -> turn left
            }
            
            car.stop();
            car.turn(right_or_left * fast);

            /*if (right_or_left == 1)
            {
                digitalWrite(greenLedPin, HIGH);
            }
            else
            {
                digitalWrite(redLedPin, HIGH);
            }*/

            delay(500);
        }
    }

    car.stop();
    //digitalWrite(LED_BUILTIN, LOW);

    // Blink white LED 3 times to indicate stage transition.
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(whiteLedPin, HIGH);
        delay(250);

        digitalWrite(whiteLedPin, LOW);
        delay(250);
    }
}