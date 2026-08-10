void funnelNavigation(const SensorReading &sensors)
{
    const float DIST_TOLERANCE   = 900.0f;
    const float FUNNEL_TOLERANCE = 100.0f;
    const float LOW_SUM_LIMIT    = 300.0f;
    const float EXIT_DELTA       = 1000.0f;

    const unsigned long MAX_OPENING_TURN_MS = 2000UL;

    bool flag_low_sum = false;

    float delta, sum, high_dist;

    // State for handling a large opening on one side
    bool correctingLargeOpening = false;
    unsigned long correctionStartTime = 0;

    int correctionDirection = 0;   // -1 for left, 1 for right

    while (!flag_low_sum || abs(laserRightDistance - laserLeftDistance) <= EXIT_DELTA)
    {
        // ====================================================
        // GET FRESH SENSOR DATA
        // ====================================================

        readSensors(STAGE_FUNNEL);

        laserRightDistance = sensors.laserRight_mm;
        laserLeftDistance  = sensors.laserLeft_mm;
        laserFrontDistance = sensors.laserFront_mm;


        // ====================================================
        // VALIDITY CHECK
        // ====================================================

        if (!(sensors.laserLeft_valid &&
              sensors.laserRight_valid &&
              sensors.laserFront_valid))
        {
            // Do not continue driving blindly with bad data.
            car.straight(slow);
            continue;
        }


        // ====================================================
        // CALCULATE FUNNEL GEOMETRY
        // ====================================================

        delta =
            laserRightDistance - laserLeftDistance;

        sum =
            laserRightDistance + laserLeftDistance;

        high_dist =
            max(laserRightDistance, laserLeftDistance);

        // Positive delta:
        // right side is farther away -> more room on right.
        right_or_left =
            (delta > 0.0f) ? 1 : -1;


        // ====================================================
        // STAGE EXIT DETECTION
        // ====================================================

        if (sum < LOW_SUM_LIMIT)
        {
            flag_low_sum = true;
        }

        // We first passed through a narrow section,
        // and now one side suddenly opens significantly.
        if (flag_low_sum &&
            abs(delta) > EXIT_DELTA)
        {
            break;
        }


        // ====================================================
        // CURRENTLY CORRECTING A LARGE OPENING
        // ====================================================

        if (correctingLargeOpening)
        {
            digitalWrite(redLedPin, HIGH);
            bool openingClosed =
                high_dist < DIST_TOLERANCE;

            bool correctionTimedOut =
                millis() - correctionStartTime >=
                MAX_OPENING_TURN_MS;

            if (openingClosed || correctionTimedOut)
            {
                // Finished the special correction.
                correctingLargeOpening = false;

                car.straight(fast);
            }
            else
            {
                // Keep turning in the SAME direction selected
                // when the large opening was first detected.
                car.turn(correctionDirection * slow);
            }

            // Don't also execute normal funnel correction during
            // this iteration.
            continue;
        }


        // ====================================================
        // DETECT A LARGE OPENING
        // ====================================================

        if (high_dist > DIST_TOLERANCE)
        {
            correctingLargeOpening = true;

            correctionStartTime = millis();

            // Remember which side was open when correction began.
            correctionDirection = right_or_left;

            car.turn(correctionDirection * slow);

            continue;
        }


        // ====================================================
        // NORMAL FUNNEL CENTERING
        // ====================================================

        if (abs(delta) > FUNNEL_TOLERANCE)
        {
            // Turn toward the side with more available space.
            car.turn(right_or_left * delta/10.0f);
        }
        else
        {
            // Approximately equal distance from both walls.
            car.straight(fast);
        }
    }


    // ========================================================
    // FUNNEL FINISHED
    // ========================================================

    car.stop();
    // Blink white LED 3 times to indicate stage transition.
    for (int i = 0; i < 3; i++)
    {
        digitalWrite(LED_BUILTIN, HIGH);
        delay(250);

        digitalWrite(LED_BUILTIN, LOW);
        delay(250);
    }
}