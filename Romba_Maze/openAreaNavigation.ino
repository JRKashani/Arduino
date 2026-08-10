void openAreaNavigation(const SensorReading &sensors)
{
    bool isLeftFrontClose = false;
    bool isRightFrontClose = false;

    int right_or_left = 0;   // -1 for left, 1 for right

    while (!isLeftFrontClose && !isRightFrontClose)
    {
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

        isLeftFrontClose =
            sensors.laserLeft_valid &&
            laserLeftDistance < 500.0f;

        isRightFrontClose =
            sensors.laserRight_valid &&
            laserRightDistance < 500.0f;

        if ((isLeftFrontClose || isRightFrontClose) &&
            sensors.laserFront_valid &&
            laserFrontDistance < 100.0f)
        {
            car.stop();

            right_or_left =
                (laserLeftDistance < laserRightDistance)
                ? 1     // obstacle closer on left -> turn right
                : -1;   // obstacle closer on right -> turn left

            car.turn(right_or_left * 20);

            delay(100);
        }
    }

    car.stop();

    // Blink white LED 3 times to indicate stage transition.
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(whiteLedPin, HIGH);
        delay(125);

        digitalWrite(whiteLedPin, LOW);
        delay(125);
    }
}