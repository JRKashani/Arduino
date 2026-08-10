void funnelNavigation(const SensorReading &sensors)
{
    const float DIST_TOLERANCE       = 900.0f;
    const float FUNNEL_TOLERANCE     = 100.0f;

    const float LOW_SUM_LIMIT        = 300.0f;
    const float EXIT_DELTA           = 1000.0f;

    const float SIDE_SEEN_MM         = 1200.0f;

    const float FRONT_CAUTION_MM     = 250.0f;
    const float FRONT_DANGER_MM      = 100.0f;

    const unsigned long MAX_OPENING_TURN_MS = 2000UL;

    bool flag_low_sum = false;

    bool correctingLargeOpening = false;
    unsigned long correctionStartTime = 0;

    int correctionDirection = 0;
    int lastTurnDirection = 1;

    bool funnelComplete = false;

    while (!funnelComplete)
    {
        readSensors(STAGE_FUNNEL);

        laserRightDistance = sensors.laserRight_mm;
        laserLeftDistance  = sensors.laserLeft_mm;
        laserFrontDistance = sensors.laserFront_mm;


        // ----------------------------------------------------
        // FRONT SENSOR IS REQUIRED
        // ----------------------------------------------------

        if (!sensors.laserFront_valid)
        {
            car.stop();
            continue;
        }


        bool leftSeen =
            sensors.laserLeft_valid &&
            laserLeftDistance < SIDE_SEEN_MM;

        bool rightSeen =
            sensors.laserRight_valid &&
            laserRightDistance < SIDE_SEEN_MM;


        // ----------------------------------------------------
        // CHOOSE TURN DIRECTION
        // ----------------------------------------------------

        int turnDirection = lastTurnDirection;

        if (leftSeen && rightSeen)
        {
            turnDirection =
                (laserRightDistance > laserLeftDistance)
                ? 1
                : -1;
        }
        else if (leftSeen && !rightSeen)
        {
            turnDirection = 1;
        }
        else if (!leftSeen && rightSeen)
        {
            turnDirection = -1;
        }

        lastTurnDirection = turnDirection;


        // ====================================================
        // PRIORITY 1: FRONT COLLISION DANGER
        // ====================================================

        if (laserFrontDistance < FRONT_DANGER_MM)
        {
            correctingLargeOpening = false;

            car.stop();

            car.turn(-turnDirection * fast);

            delay(1000);

            continue;
        }


        // ====================================================
        // PRIORITY 2: FRONT OBJECT APPROACHING
        // ====================================================

        if (laserFrontDistance < FRONT_CAUTION_MM)
        {
            correctingLargeOpening = false;

            car.turn(-turnDirection * slow);

            delay(500);

            continue;
        }


        // ----------------------------------------------------
        // SIDE GEOMETRY REQUIRES BOTH SIDE READINGS
        // ----------------------------------------------------

        if (!(leftSeen && rightSeen))
        {
            // Front is clear, but we cannot reliably center.
            car.straight(slow);
            continue;
        }


        float delta =
            laserRightDistance - laserLeftDistance;

        float sum =
            laserRightDistance + laserLeftDistance;

        float high_dist =
            max(laserRightDistance, laserLeftDistance);

        int right_or_left =
            (delta > 0.0f) ? 1 : -1;


        // ====================================================
        // FUNNEL EXIT DETECTION
        // ====================================================

        if (sum < LOW_SUM_LIMIT)
        {
            flag_low_sum = true;
        }

        if (flag_low_sum &&
            abs(delta) > EXIT_DELTA)
        {
            funnelComplete = true;
            break;
        }


        // ====================================================
        // LARGE-OPENING CORRECTION ALREADY ACTIVE
        // ====================================================

        if (correctingLargeOpening)
        {
            bool openingClosed =
                high_dist < DIST_TOLERANCE;

            bool timedOut =
                millis() - correctionStartTime >=
                MAX_OPENING_TURN_MS;

            if (openingClosed || timedOut)
            {
                correctingLargeOpening = false;

                car.straight(fast);
            }
            else
            {
                car.turn(correctionDirection * slow);
                delay(50);
            }

            continue;
        }


        // ====================================================
        // NEW LARGE OPENING
        // ====================================================

        if (high_dist > DIST_TOLERANCE)
        {
            correctingLargeOpening = true;

            correctionStartTime = millis();

            correctionDirection = right_or_left;

            car.turn(correctionDirection * slow);
            delay(50);

            continue;
        }


        // ====================================================
        // NORMAL FUNNEL CENTERING
        // ====================================================

        if (abs(delta) > FUNNEL_TOLERANCE)
        {
            car.turn(right_or_left * abs(delta) / 10.0f);
        }
        else
        {
            car.straight(fast);
        }
    }

    car.stop();
}