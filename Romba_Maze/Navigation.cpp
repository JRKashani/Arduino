#pragma once

#include "Navigation.h"

void updateMinimum(
    LaserMinimum &minimum,
    uint16_t distance,
    unsigned long timestamp)
{
    const uint16_t MIN_VALID_DISTANCE = 50;
    const uint16_t MAX_VALID_DISTANCE = 4000;

    if (distance < MIN_VALID_DISTANCE ||
        distance > MAX_VALID_DISTANCE)
        return;

    if (distance < minimum.distance_mm)
    {
        minimum.distance_mm = distance;
        minimum.timestamp_ms = timestamp;
    }
}

static void updateLeftRightBalance(
    LaserScanResult &result,
    unsigned long timestamp_ms)
{
    const uint16_t MAX_FUNNEL_PAIR_SUM_MM = 1600;

    if (!currentData.laserLeft_valid ||
        !currentData.laserRight_valid)
        return;

    uint16_t leftDistance =
        (uint16_t)currentData.laserLeft_mm;

    uint16_t rightDistance =
        (uint16_t)currentData.laserRight_mm;

    uint16_t pairSum =
        leftDistance + rightDistance;

    // Ignore "balanced" readings that are obviously open space
    // rather than the funnel walls.
    if (pairSum > MAX_FUNNEL_PAIR_SUM_MM)
        return;

    uint16_t difference =
        (leftDistance > rightDistance)
            ? leftDistance - rightDistance
            : rightDistance - leftDistance;

    if (difference < result.leftRightBalance.difference_mm)
    {
        result.leftRightBalance.difference_mm = difference;
        result.leftRightBalance.left_mm = leftDistance;
        result.leftRightBalance.right_mm = rightDistance;
        result.leftRightBalance.timestamp_ms = timestamp_ms;
    }
}

LaserScanResult scanWithRobot()
{
    const unsigned long SWEEP_TIME = 1000;
    const unsigned long SAMPLE_INTERVAL = 20;

    const uint16_t MIN_VALID_DISTANCE = 50;
    const uint16_t MAX_VALID_DISTANCE = 4000;

    LaserScanResult result;

    // Initialize minima to impossible/high values
    result.left.distance_mm       = UINT16_MAX;
    result.front.distance_mm      = UINT16_MAX;
    result.right.distance_mm      = UINT16_MAX;

    result.leftRight.distance_mm  = UINT16_MAX;
    result.leftFront.distance_mm  = UINT16_MAX;
    result.rightFront.distance_mm = UINT16_MAX;

    result.leftRightBalance.difference_mm = UINT16_MAX;

    result.left.timestamp_ms       = 0;
    result.front.timestamp_ms      = 0;
    result.right.timestamp_ms      = 0;

    result.leftRight.timestamp_ms  = 0;
    result.leftFront.timestamp_ms  = 0;
    result.rightFront.timestamp_ms = 0;

    unsigned long scanStart = millis();
    unsigned long lastSample = 0;

    // ------------------------------------------------
    // Sweep 1: turn on the spot in one direction
    // ------------------------------------------------

    car.turn(fast * (1));

    unsigned long sweepStart = millis();

    while (millis() - sweepStart < SWEEP_TIME)
    {
        if (millis() - lastSample < SAMPLE_INTERVAL)
            continue;

        lastSample = millis();

        readSensors(STAGE_OPEN_AREA);

        unsigned long t = millis() - scanStart;
        
        updateLeftRightBalance(result, t);

        // LEFT LASER
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserLeft_mm < result.left.distance_mm)
        {
            result.left.distance_mm = currentData.laserLeft_mm;
            result.left.timestamp_ms = t;
        }

        // FRONT LASER
        if (currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm < result.front.distance_mm)
        {
            result.front.distance_mm = currentData.laserFront_mm;
            result.front.timestamp_ms = t;
        }

        // RIGHT LASER
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm < result.right.distance_mm)
        {
            result.right.distance_mm = currentData.laserRight_mm;
            result.right.timestamp_ms = t;
        }
        // ------------------------------------------------
        // COMBINED LASER MINIMA
        // ------------------------------------------------

        // LEFT + RIGHT
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserLeft_mm +
                        currentData.laserRight_mm;

            if (sum < result.leftRight.distance_mm)
            {
                result.leftRight.distance_mm = sum;
                result.leftRight.timestamp_ms = t;
            }
        }


        // LEFT + FRONT
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserLeft_mm +
                        currentData.laserFront_mm;

            if (sum < result.leftFront.distance_mm)
            {
                result.leftFront.distance_mm = sum;
                result.leftFront.timestamp_ms = t;
            }
        }


        // RIGHT + FRONT
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserRight_mm +
                        currentData.laserFront_mm;

            if (sum < result.rightFront.distance_mm)
            {
                result.rightFront.distance_mm = sum;
                result.rightFront.timestamp_ms = t;
            }
        }
        // ------------------------------------------------
        // BEST LEFT / RIGHT BALANCE
        // ------------------------------------------------

        if (currentData.laserLeft_valid &&
            currentData.laserRight_valid)
        {
            uint16_t leftDistance =
                (uint16_t)currentData.laserLeft_mm;

            uint16_t rightDistance =
                (uint16_t)currentData.laserRight_mm;

            uint16_t difference =
                (leftDistance > rightDistance)
                    ? leftDistance - rightDistance
                    : rightDistance - leftDistance;

            if (difference < result.leftRightBalance.difference_mm)
            {
                result.leftRightBalance.difference_mm = difference;
                result.leftRightBalance.left_mm = leftDistance;
                result.leftRightBalance.right_mm = rightDistance;
                result.leftRightBalance.timestamp_ms = t;
            }
        }
    }

    car.stop();

    delay(100);


    // ------------------------------------------------
    // Sweep 2: turn back in the opposite direction
    // ------------------------------------------------

    car.turn(fast * (-1));  // Replace with your actual car function

    sweepStart = millis();

    // Twice as long:
    // from LEFT extreme -> through original direction -> RIGHT extreme
    while (millis() - sweepStart < 2 * SWEEP_TIME)
    {
        if (millis() - lastSample < SAMPLE_INTERVAL)
            continue;

        lastSample = millis();

        readSensors(STAGE_OPEN_AREA);

        unsigned long t = millis() - scanStart;

        updateLeftRightBalance(result, t);

        // LEFT LASER
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserLeft_mm < result.left.distance_mm)
        {
            result.left.distance_mm = currentData.laserLeft_mm;
            result.left.timestamp_ms = t;
        }

        // FRONT LASER
        if (currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm < result.front.distance_mm)
        {
            result.front.distance_mm = currentData.laserFront_mm;
            result.front.timestamp_ms = t;
        }

        // RIGHT LASER
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm < result.right.distance_mm)
        {
            result.right.distance_mm = currentData.laserRight_mm;
            result.right.timestamp_ms = t;
        }
        
        // ------------------------------------------------
        // COMBINED LASER MINIMA
        // ------------------------------------------------

        // LEFT + RIGHT
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserLeft_mm +
                        currentData.laserRight_mm;

            if (sum < result.leftRight.distance_mm)
            {
                result.leftRight.distance_mm = sum;
                result.leftRight.timestamp_ms = t;
            }
        }


        // LEFT + FRONT
        if (currentData.laserLeft_mm >= MIN_VALID_DISTANCE &&
            currentData.laserLeft_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserLeft_mm +
                        currentData.laserFront_mm;

            if (sum < result.leftFront.distance_mm)
            {
                result.leftFront.distance_mm = sum;
                result.leftFront.timestamp_ms = t;
            }
        }


        // RIGHT + FRONT
        if (currentData.laserRight_mm >= MIN_VALID_DISTANCE &&
            currentData.laserRight_mm <= MAX_VALID_DISTANCE &&
            currentData.laserFront_mm >= MIN_VALID_DISTANCE &&
            currentData.laserFront_mm <= MAX_VALID_DISTANCE)
        {
            uint16_t sum = currentData.laserRight_mm +
                        currentData.laserFront_mm;

            if (sum < result.rightFront.distance_mm)
            {
                result.rightFront.distance_mm = sum;
                result.rightFront.timestamp_ms = t;
            }
        }
        
        // ------------------------------------------------
        // BEST LEFT / RIGHT BALANCE
        // ------------------------------------------------

        if (currentData.laserLeft_valid &&
            currentData.laserRight_valid)
        {
            uint16_t leftDistance =
                (uint16_t)currentData.laserLeft_mm;

            uint16_t rightDistance =
                (uint16_t)currentData.laserRight_mm;

            uint16_t difference =
                (leftDistance > rightDistance)
                    ? leftDistance - rightDistance
                    : rightDistance - leftDistance;

            if (difference < result.leftRightBalance.difference_mm)
            {
                result.leftRightBalance.difference_mm = difference;
                result.leftRightBalance.left_mm = leftDistance;
                result.leftRightBalance.right_mm = rightDistance;
                result.leftRightBalance.timestamp_ms = t;
            }
        }
    }

    /*car.stop();

    car.turn(fast * (1));

    delay(SWEEP_TIME);*/

    car.stop();

    return result;
}

// ============================================================
// FUNNEL NAVIGATION SETTINGS
// ============================================================

// Scan calibration:
// 1000 ms rotation ~= 90 degrees.
static const unsigned long SCAN_SWEEP_TIME_MS = 1000UL;
static const unsigned long SCAN_PAUSE_MS = 100UL;
static const float SCAN_SWEEP_ANGLE_DEG = 90.0f;

// scanWithRobot() finishes at approximately -90 degrees
// relative to the heading at which the scan began.
static const float SCAN_END_HEADING_DEG = -90.0f;


// Sensor mounting directions, converted to CCW-positive:
//
// supplied mounting:
// LEFT  = 270 deg
// FRONT =   0 deg
// RIGHT =  90 deg
//
// CCW-positive equivalent:
static const float LEFT_LASER_OFFSET_DEG  = +90.0f;
static const float FRONT_LASER_OFFSET_DEG =   0.0f;
static const float RIGHT_LASER_OFFSET_DEG = -90.0f;


// Trigger settings
static const uint16_t FRONT_FORCED_SCAN_MM = 180;

static const unsigned long VERY_FAST_RESCAN_TIME_MS = 2500UL;
static const unsigned long FAST_RESCAN_TIME_MS      = 5000UL;

static const float WIDTH_RESCAN_FACTOR = 0.75f;


// Safety override
static const uint16_t CLOSE_OBSTACLE_MM = 200;
static const float OBSTACLE_DANGER_CONE_DEG = 35.0f;
static const float AVOIDANCE_SHIFT_DEG = 45.0f;


// ------------------------------------------------------------
// IMPORTANT:
// This must be calibrated specifically using:
//
//     car.turn(fast)
//
// If your 1000 ms -> 90 deg measurement was made at fast,
// leave this at 1000.
//
// If the scan was made at veryFast / PWM 100, this value
// needs a separate calibration.
// ------------------------------------------------------------
static const unsigned long FAST_TURN_90_MS = 1000UL;

// ============================================================
// ANGLE HELPERS
// ============================================================

static float normalizeAngle180(float angle)
{
    while (angle > 180.0f)
        angle -= 360.0f;

    while (angle < -180.0f)
        angle += 360.0f;

    return angle;
}


static float clampScanHeading(float angle)
{
    if (angle > 90.0f)
        return 90.0f;

    if (angle < -90.0f)
        return -90.0f;

    return angle;
}


// Convert timestamp from scanWithRobot() into robot heading
// relative to the heading at which that scan began.
//
// 0 ms       ->   0 deg
// 1000 ms    -> +90 deg
//
// 100 ms pause
//
// 1100 ms    -> +90 deg
// 2100 ms    ->   0 deg
// 3100 ms    -> -90 deg
//
static float scanTimestampToAngle(unsigned long timestamp_ms)
{
    if (timestamp_ms <= SCAN_SWEEP_TIME_MS)
    {
        float angle =
            SCAN_SWEEP_ANGLE_DEG *
            ((float)timestamp_ms /
             (float)SCAN_SWEEP_TIME_MS);

        return clampScanHeading(angle);
    }

    const unsigned long secondSweepStart =
        SCAN_SWEEP_TIME_MS +
        SCAN_PAUSE_MS;

    if (timestamp_ms < secondSweepStart)
        return +90.0f;

    unsigned long secondSweepElapsed =
        timestamp_ms - secondSweepStart;

    float angle =
        +90.0f -
        SCAN_SWEEP_ANGLE_DEG *
        ((float)secondSweepElapsed /
         (float)SCAN_SWEEP_TIME_MS);

    return clampScanHeading(angle);
}

enum ClosestLaser
{
    CLOSEST_NONE,
    CLOSEST_LEFT,
    CLOSEST_FRONT,
    CLOSEST_RIGHT
};


struct ClosestObstacle
{
    bool valid;
    ClosestLaser sensor;

    uint16_t distance_mm;

    // Bearing relative to the heading at the beginning
    // of the scan. CCW positive.
    float bearing_deg;
};


static void considerObstacle(
    ClosestObstacle &closest,
    const LaserMinimum &minimum,
    float sensorOffsetDeg,
    ClosestLaser sensor)
{
    if (minimum.distance_mm == UINT16_MAX)
        return;

    if (closest.valid &&
        minimum.distance_mm >= closest.distance_mm)
        return;

    float robotHeading =
        scanTimestampToAngle(
            minimum.timestamp_ms);

    float obstacleBearing =
        normalizeAngle180(
            robotHeading +
            sensorOffsetDeg);

    closest.valid = true;
    closest.sensor = sensor;
    closest.distance_mm = minimum.distance_mm;
    closest.bearing_deg = obstacleBearing;
}


static ClosestObstacle findClosestObstacle(
    const LaserScanResult &scan)
{
    ClosestObstacle closest;

    closest.valid = false;
    closest.sensor = CLOSEST_NONE;
    closest.distance_mm = UINT16_MAX;
    closest.bearing_deg = 0.0f;

    considerObstacle(
        closest,
        scan.left,
        LEFT_LASER_OFFSET_DEG,
        CLOSEST_LEFT);

    considerObstacle(
        closest,
        scan.front,
        FRONT_LASER_OFFSET_DEG,
        CLOSEST_FRONT);

    considerObstacle(
        closest,
        scan.right,
        RIGHT_LASER_OFFSET_DEG,
        CLOSEST_RIGHT);

    return closest;
}

static float chooseFunnelHeading(
    const LaserScanResult &scan)
{
    float targetHeading = 0.0f;

    bool haveBalance =
        scan.leftRightBalance.difference_mm != UINT16_MAX;

    if (haveBalance)
    {
        targetHeading =
            scanTimestampToAngle(
                scan.leftRightBalance.timestamp_ms);
    }


    // ------------------------------------------------
    // Safety override
    // ------------------------------------------------

    ClosestObstacle closest =
        findClosestObstacle(scan);

    if (!closest.valid ||
        closest.distance_mm > CLOSE_OBSTACLE_MM)
    {
        return clampScanHeading(targetHeading);
    }


    // Is our intended heading actually pointing toward
    // the closest obstacle?
    float obstacleRelativeToTarget =
        normalizeAngle180(
            closest.bearing_deg -
            targetHeading);

    if (fabs(obstacleRelativeToTarget) >
        OBSTACLE_DANGER_CONE_DEG)
    {
        return clampScanHeading(targetHeading);
    }


    // Very close obstacle lies approximately in the
    // direction we were planning to drive.
    //
    // Shift away from it.

    if (fabs(obstacleRelativeToTarget) > 5.0f)
    {
        if (obstacleRelativeToTarget > 0.0f)
        {
            // obstacle is CCW / left of intended heading
            // -> move heading clockwise
            targetHeading -= AVOIDANCE_SHIFT_DEG;
        }
        else
        {
            // obstacle is clockwise / right
            // -> move heading CCW
            targetHeading += AVOIDANCE_SHIFT_DEG;
        }
    }
    else
    {
        // Obstacle approximately straight ahead.
        // Use side clearance to choose escape direction.

        if (haveBalance &&
            scan.leftRightBalance.left_mm >
            scan.leftRightBalance.right_mm)
        {
            // More room on left
            targetHeading += AVOIDANCE_SHIFT_DEG;
        }
        else
        {
            // More room on right
            targetHeading -= AVOIDANCE_SHIFT_DEG;
        }
    }

    return clampScanHeading(targetHeading);
}

static void turnRelativeFast(float angleDeg)
{
    angleDeg =
        normalizeAngle180(angleDeg);

    if (fabs(angleDeg) < 1.0f)
        return;

    unsigned long turnTimeMs =
        (unsigned long)(
            fabs(angleDeg) *
            ((float)FAST_TURN_90_MS / 90.0f)
            + 0.5f);


    // Turn on the spot.
    car.straight(0);

    if (angleDeg > 0.0f)
    {
        // CCW
        car.turn(fast);
    }
    else
    {
        // CW
        car.turn(-fast);
    }

    delay(turnTimeMs);

    car.stop();
}

static void correctHeadingAfterScan(
    float desiredHeadingDeg)
{
    float correction =
        normalizeAngle180(
            desiredHeadingDeg -
            SCAN_END_HEADING_DEG);

    turnRelativeFast(correction);
}

void funnelNavigation(const SensorReading &sensors)
{
    // ------------------------------------------------
    // STATE PRESERVED BETWEEN CALLS
    // ------------------------------------------------

    static bool haveCompletedScan = false;

    static unsigned long lastScanCompleted_ms = 0;

    static float balancedWidth_mm = NAN;

    static bool geometryTriggerArmed = false;


    // ------------------------------------------------
    // CURRENT DRIVING SPEED
    // ------------------------------------------------
    //
    // Change this to "fast" if desired.
    // The timeout automatically changes with it.
    //
    const int driveSpeed = veryFast;


    unsigned long maximumTimeWithoutScan;

    if (driveSpeed >= veryFast)
        maximumTimeWithoutScan =
            VERY_FAST_RESCAN_TIME_MS;
    else
        maximumTimeWithoutScan =
            FAST_RESCAN_TIME_MS;


    // ------------------------------------------------
    // DECIDE WHETHER WE NEED A SCAN
    // ------------------------------------------------

    bool shouldScan = false;


    // First entry into funnel always begins with a scan.
    if (!haveCompletedScan)
    {
        shouldScan = true;

        Serial.println(
            F("FUNNEL: initial scan"));
    }


    // ------------------------------------------------
    // Trigger 1:
    // Nose getting dangerously close
    // ------------------------------------------------

    if (!shouldScan &&
        sensors.laserFront_valid &&
        sensors.laserFront_mm <= FRONT_FORCED_SCAN_MM)
    {
        shouldScan = true;

        Serial.println(
            F("FUNNEL: front obstacle scan"));
    }


    // ------------------------------------------------
    // Trigger 2:
    // Too long since the previous scan
    // ------------------------------------------------

    if (!shouldScan &&
        millis() - lastScanCompleted_ms >=
            maximumTimeWithoutScan)
    {
        shouldScan = true;

        Serial.println(
            F("FUNNEL: timed scan"));
    }


    // ------------------------------------------------
    // Trigger 3:
    // Front distance <= 3/4 of balanced funnel width
    // ------------------------------------------------

    if (!isnan(balancedWidth_mm) &&
        sensors.laserFront_valid)
    {
        float geometryScanDistance =
            WIDTH_RESCAN_FACTOR *
            balancedWidth_mm;


        // Hysteresis:
        //
        // After a scan, don't immediately scan again just
        // because we're already below the threshold.
        //
        // Re-arm only if the new heading gives us noticeably
        // more forward space.
        if (sensors.laserFront_mm >
            geometryScanDistance + 100.0f)
        {
            geometryTriggerArmed = true;
        }


        if (!shouldScan &&
            geometryTriggerArmed &&
            sensors.laserFront_mm <=
                geometryScanDistance)
        {
            shouldScan = true;
            geometryTriggerArmed = false;

            Serial.println(
                F("FUNNEL: geometry scan"));
        }
    }


    // ------------------------------------------------
    // NO SCAN REQUIRED
    // ------------------------------------------------

    if (!shouldScan)
    {
        car.turn(0);
        car.straight(driveSpeed);
        return;
    }


    // ========================================================
    // RUN SCAN
    // ========================================================

    car.stop();

    LaserScanResult scan =
        scanWithRobot();


    // ------------------------------------------------
    // Get balanced funnel width
    // ------------------------------------------------

    if (scan.leftRightBalance.difference_mm !=
        UINT16_MAX)
    {
        uint16_t newBalancedWidth =
            scan.leftRightBalance.left_mm +
            scan.leftRightBalance.right_mm;

        // Same broad plausibility check used by scanner.
        if (newBalancedWidth <= 1600)
        {
            balancedWidth_mm =
                (float)newBalancedWidth;
        }
    }


    // ------------------------------------------------
    // Pick desired direction
    // ------------------------------------------------

    float desiredHeading =
        chooseFunnelHeading(scan);


    // ------------------------------------------------
    // Debug output
    // ------------------------------------------------

    Serial.println();
    Serial.println(F("---- FUNNEL SCAN ----"));

    Serial.print(F("Desired heading: "));

    if (desiredHeading >= 0)
        Serial.print('+');

    Serial.print(desiredHeading, 1);
    Serial.println(F(" deg"));


    if (!isnan(balancedWidth_mm))
    {
        Serial.print(F("Balanced width: "));
        Serial.print(balancedWidth_mm, 0);
        Serial.println(F(" mm"));

        Serial.print(F("Next geometry scan: "));
        Serial.print(
            WIDTH_RESCAN_FACTOR *
            balancedWidth_mm,
            0);
        Serial.println(F(" mm"));
    }


    ClosestObstacle closest =
        findClosestObstacle(scan);

    if (closest.valid)
    {
        Serial.print(F("Closest obstacle: "));
        Serial.print(closest.distance_mm);
        Serial.print(F(" mm @ "));

        if (closest.bearing_deg >= 0)
            Serial.print('+');

        Serial.print(
            closest.bearing_deg,
            1);

        Serial.println(F(" deg"));
    }


    // ------------------------------------------------
    // Correct robot heading
    // ------------------------------------------------

    correctHeadingAfterScan(
        desiredHeading);


    // Reset scan timers only AFTER scanning and steering.
    lastScanCompleted_ms = millis();

    haveCompletedScan = true;

    geometryTriggerArmed = false;


    // ------------------------------------------------
    // Resume forward motion
    // ------------------------------------------------

    car.turn(0);
    car.straight(driveSpeed);
}