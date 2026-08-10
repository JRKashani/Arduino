void openAreaNavigation(const SensorReading &sensors)
{
    const unsigned long OPEN_AREA_DRIVE_TIME_MS = 5000;  // CALIBRATE
    const int FRONT_STOP_DISTANCE_MM = 120;              // safety only

    unsigned long startTime = millis();

    // Robot is assumed to already point toward the center of the funnel.
    car.straight(veryFast);

    while (millis() - startTime < OPEN_AREA_DRIVE_TIME_MS)
    {
        readSensors(STAGE_OPEN_AREA);

        laserFrontDistance = sensors.laserFront_mm;

        // Emergency protection only.
        if (laserFrontDistance > 0 &&
            laserFrontDistance < FRONT_STOP_DISTANCE_MM)
        {

            car.stop();
            laserRightDistance = sensors.laserRight_mm;
            laserLeftDistance  = sensors.laserLeft_mm;
            right_or_left = (laserRightDistance > laserLeftDistance) ? 1 : -1;
            car.turn(fast * right_or_left);  // turn away from obstacle
            delay(1000);

            //Serial.println("OPEN AREA: obstacle detected ahead");
            return;
        }
    }

    car.stop();

    for (int i = 0; i < 3; i++)
    {
        digitalWrite(whiteLedPin, HIGH);
        delay(250);

        digitalWrite(whiteLedPin, LOW);
        delay(250);
    }

    //Serial.println("OPEN AREA COMPLETE");
}