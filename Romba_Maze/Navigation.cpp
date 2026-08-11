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

// ================================================================
// WALL FOLLOWING NAVIGATION
// ================================================================

namespace
{

// ------------------------------------------------
// Wall side
// ------------------------------------------------

enum WallSide : int8_t
{
    WALL_NONE  = 0,
    WALL_LEFT  = 1,
    WALL_RIGHT = -1
};


// ------------------------------------------------
// Wall-following state machine
// ------------------------------------------------

enum WallFollowState : uint8_t
{
    WF_NOT_STARTED,

    WF_EXIT_FUNNEL,
    WF_INITIAL_SCAN,

    WF_ACQUIRE_WALL,
    WF_ALIGN_WALL,
    WF_FOLLOW_WALL,

    WF_SCAN_FRONT_CORNER,
    WF_SCAN_LOST_WALL,
    WF_TURN_CORNER,

    WF_FAILED
};


// ------------------------------------------------
// Tunable constants
// ------------------------------------------------

// Desired side distance
const uint16_t WALL_TARGET_DISTANCE = 300;

// How close to 300 mm is considered good
const uint16_t WALL_DISTANCE_TOLERANCE = 40;


// ----- Front obstacle thresholds -----

// Start slowing down
const uint16_t FRONT_SLOW_DISTANCE = 450;

// At this distance we stop and scan
const uint16_t FRONT_CORNER_DISTANCE = 300;

// Safety threshold
const uint16_t FRONT_EMERGENCY_DISTANCE = 180;


// ----- Wall detection -----

// If the side laser suddenly sees farther than this,
// consider that the wall may have disappeared.
const uint16_t WALL_LOST_DISTANCE = 900;

// Maximum distance considered a useful wall during
// the initial wall-side decision.
const uint16_t INITIAL_WALL_DETECTION_DISTANCE = 1200;


// ----- Exit from funnel -----

const unsigned long FUNNEL_EXIT_TIME_MS = 1000;


// ----- Control timing -----

const unsigned long CONTROL_INTERVAL_MS = 40;


// ----- Wall following controller -----

// Starting values - WILL need physical tuning.
const float WALL_KP = 0.10f;
const float WALL_KD = 0.55f;

// Slightly stronger controller while acquiring the wall
const float ACQUIRE_KP = 0.13f;
const float ACQUIRE_KD = 0.45f;


// ----- Alignment -----

// Change in side distance between samples that is considered
// approximately parallel to the wall.
const int16_t ALIGN_MAX_DELTA_MM = 8;

// Number of consecutive good samples required.
const uint8_t ACQUIRE_STABLE_SAMPLES = 4;
const uint8_t ALIGN_STABLE_SAMPLES = 5;


// ----- Lost-wall filtering -----

// Do not declare the wall lost because of one bad laser reading.
const uint8_t WALL_LOST_REQUIRED_SAMPLES = 3;


// ----- Corner turn -----

// When turning after a corner, accept the wall when it falls
// inside this broad range. The normal controller will then
// correct it back to exactly ~300 mm.
const uint16_t CORNER_REACQUIRE_MIN = 180;
const uint16_t CORNER_REACQUIRE_MAX = 500;

const uint8_t CORNER_REACQUIRE_SAMPLES = 2;

// Prevent the robot from immediately thinking the OLD wall
// is the newly-acquired wall.
const unsigned long CORNER_MIN_TURN_MS = 350;

// Safety limit: never spin forever.
const unsigned long CORNER_MAX_TURN_MS = 1600;


// ------------------------------------------------
// Internal variables
// ------------------------------------------------

WallFollowState wallState = WF_NOT_STARTED;
WallSide wallSide = WALL_NONE;

LaserScanResult lastWallScan;

unsigned long wallStateStarted_ms = 0;
unsigned long lastWallControl_ms = 0;

uint16_t previousWallDistance_mm = 0;
bool havePreviousWallDistance = false;

uint8_t acquireStableCount = 0;
uint8_t alignStableCount = 0;
uint8_t lostWallCount = 0;
uint8_t cornerReacquireCount = 0;

// +1 = turn left
// -1 = turn right
int8_t cornerTurnDirection = 0;


// ================================================================
// HELPER FUNCTIONS
// ================================================================

bool validLaserReading(bool validFlag, uint16_t distance)
{
    return validFlag &&
           distance >= 50 &&
           distance <= 4000;
}


// ------------------------------------------------
// Get current distance to the wall we are following
// ------------------------------------------------

bool getCurrentWallDistance(uint16_t &distance)
{
    if (wallSide == WALL_LEFT)
    {
        distance = currentData.laserLeft_mm;

        return validLaserReading(
            currentData.laserLeft_valid,
            currentData.laserLeft_mm);
    }

    if (wallSide == WALL_RIGHT)
    {
        distance = currentData.laserRight_mm;

        return validLaserReading(
            currentData.laserRight_valid,
            currentData.laserRight_mm);
    }

    return false;
}


// ------------------------------------------------
// Front laser validity
// ------------------------------------------------

bool getFrontDistance(uint16_t &distance)
{
    distance = currentData.laserFront_mm;

    return validLaserReading(
        currentData.laserFront_valid,
        currentData.laserFront_mm);
}


// ------------------------------------------------
// Change wall-following state
// ------------------------------------------------

void enterWallState(WallFollowState newState)
{
    wallState = newState;
    wallStateStarted_ms = millis();

    acquireStableCount = 0;
    alignStableCount = 0;
    lostWallCount = 0;
    cornerReacquireCount = 0;
}


// ------------------------------------------------
// Print scan result
// ------------------------------------------------

void printWallScan(const LaserScanResult &scan)
{
    Serial.println();
    Serial.println("========== WALL SCAN ==========");

    Serial.print("LEFT: ");
    Serial.print(scan.left.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.left.timestamp_ms);
    Serial.println(" ms");

    Serial.print("FRONT: ");
    Serial.print(scan.front.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.front.timestamp_ms);
    Serial.println(" ms");

    Serial.print("RIGHT: ");
    Serial.print(scan.right.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.right.timestamp_ms);
    Serial.println(" ms");

    Serial.println("--- PAIR MINIMA ---");

    Serial.print("LEFT + RIGHT: ");
    Serial.print(scan.leftRight.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.leftRight.timestamp_ms);
    Serial.println(" ms");

    Serial.print("LEFT + FRONT: ");
    Serial.print(scan.leftFront.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.leftFront.timestamp_ms);
    Serial.println(" ms");

    Serial.print("RIGHT + FRONT: ");
    Serial.print(scan.rightFront.distance_mm);
    Serial.print(" mm @ ");
    Serial.print(scan.rightFront.timestamp_ms);
    Serial.println(" ms");

    Serial.println("===============================");
}


// ------------------------------------------------
// Decide which wall to follow
// ------------------------------------------------
//
// IMPORTANT:
// scanWithRobot() rotates the whole robot.
//
// Therefore simply saying:
//
//      scan.left < scan.right
//
// does NOT necessarily mean the wall is physically on the left.
//
// After scanWithRobot() finishes, the robot has returned to its
// original heading. We therefore take one fresh measurement in
// that heading and use that to select LEFT or RIGHT.
//
// ------------------------------------------------

WallSide chooseWallSide()
{
    readSensors(STAGE_WALL_FOLLOWING);

    bool leftValid =
        validLaserReading(
            currentData.laserLeft_valid,
            currentData.laserLeft_mm) &&
        currentData.laserLeft_mm <= INITIAL_WALL_DETECTION_DISTANCE;

    bool rightValid =
        validLaserReading(
            currentData.laserRight_valid,
            currentData.laserRight_mm) &&
        currentData.laserRight_mm <= INITIAL_WALL_DETECTION_DISTANCE;


    // Only left sees a useful wall
    if (leftValid && !rightValid)
        return WALL_LEFT;


    // Only right sees a useful wall
    if (rightValid && !leftValid)
        return WALL_RIGHT;


    // Both sides see walls:
    // follow the closer one.
    if (leftValid && rightValid)
    {
        if (currentData.laserLeft_mm <= currentData.laserRight_mm)
            return WALL_LEFT;
        else
            return WALL_RIGHT;
    }


    return WALL_NONE;
}


// ------------------------------------------------
// Wall steering controller
// ------------------------------------------------
//
// SteeringDualH behavior:
//
//      positive car.turn() = LEFT
//      negative car.turn() = RIGHT
//
// For LEFT wall:
//      too far  -> steer LEFT
//      too close -> steer RIGHT
//
// For RIGHT wall:
//      exactly reversed.
//
// ------------------------------------------------

void steerAlongWall(
    uint16_t wallDistance,
    int baseSpeed,
    float kp,
    float kd)
{
    int16_t error =
        (int16_t)wallDistance -
        (int16_t)WALL_TARGET_DISTANCE;


    int16_t derivative = 0;

    if (havePreviousWallDistance)
    {
        derivative =
            (int16_t)wallDistance -
            (int16_t)previousWallDistance_mm;
    }


    float controller =
        kp * error +
        kd * derivative;


    // LEFT wall:
    // positive controller -> left
    //
    // RIGHT wall:
    // invert it.
    if (wallSide == WALL_RIGHT)
        controller = -controller;


    int correction = (int)controller;


    // Avoid making the steering correction absurdly large
    // compared with forward velocity.
    int maxCorrection = abs(baseSpeed) * 2 / 3;

    if (maxCorrection < 8)
        maxCorrection = 8;


    correction =
        constrain(
            correction,
            -maxCorrection,
            maxCorrection);


    // straight() stores the forward velocity in SteeringDualH.
    car.straight(baseSpeed);

    // turn() then creates differential motor speed around
    // that forward velocity.
    car.turn(correction);


    previousWallDistance_mm = wallDistance;
    havePreviousWallDistance = true;
}


// ------------------------------------------------
// Start an on-the-spot corner turn
// ------------------------------------------------

void startCornerTurn(int8_t direction)
{
    cornerTurnDirection = direction;

    cornerReacquireCount = 0;

    havePreviousWallDistance = false;

    // IMPORTANT:
    // car.stop() makes the stored forward velocity zero.
    // Therefore car.turn() now rotates on the spot.
    car.stop();

    enterWallState(WF_TURN_CORNER);

    car.turn(cornerTurnDirection * fast);
}


// ------------------------------------------------
// Front corner detected
// ------------------------------------------------

void beginFrontCorner()
{
    car.stop();

    Serial.println();
    Serial.println("Front corner detected.");

    enterWallState(WF_SCAN_FRONT_CORNER);
}


// ------------------------------------------------
// Side wall disappeared
// ------------------------------------------------

void beginLostWallCorner()
{
    car.stop();

    Serial.println();
    Serial.println("Wall disappeared.");

    enterWallState(WF_SCAN_LOST_WALL);
}

} // namespace


// ================================================================
// RESET
// ================================================================

void resetWallFollowingNavigation()
{
    car.stop();

    wallState = WF_NOT_STARTED;
    wallSide = WALL_NONE;

    havePreviousWallDistance = false;

    acquireStableCount = 0;
    alignStableCount = 0;
    lostWallCount = 0;
    cornerReacquireCount = 0;

    cornerTurnDirection = 0;

    lastWallControl_ms = 0;

    Serial.println("Wall navigation reset.");
}


// ================================================================
// MAIN WALL FOLLOWING FUNCTION
// ================================================================

void wallFollowingNavigation()
{
    unsigned long now = millis();


    // ============================================================
    // FIRST CALL
    // ============================================================

    if (wallState == WF_NOT_STARTED)
    {
        Serial.println();
        Serial.println("=================================");
        Serial.println("WALL FOLLOWING START");
        Serial.println("=================================");

        wallSide = WALL_NONE;

        havePreviousWallDistance = false;

        enterWallState(WF_EXIT_FUNNEL);

        // Head out of funnel quickly.
        car.straight(veryFast);

        return;
    }


    // ============================================================
    // EXIT FUNNEL
    // ============================================================

    if (wallState == WF_EXIT_FUNNEL)
    {
        // Keep checking the front laser while exiting.
        if (now - lastWallControl_ms >= CONTROL_INTERVAL_MS)
        {
            lastWallControl_ms = now;

            readSensors(STAGE_WALL_FOLLOWING);

            uint16_t frontDistance;

            if (getFrontDistance(frontDistance))
            {
                // Do not blindly finish the full 1000 ms if
                // something is already in front.
                if (frontDistance <= FRONT_CORNER_DISTANCE)
                {
                    car.stop();

                    Serial.println(
                        "Exit interrupted by front obstacle.");

                    enterWallState(WF_INITIAL_SCAN);

                    return;
                }
            }
        }


        if (now - wallStateStarted_ms >= FUNNEL_EXIT_TIME_MS)
        {
            car.stop();

            Serial.println("Funnel exit complete.");

            enterWallState(WF_INITIAL_SCAN);
        }

        return;
    }


    // ============================================================
    // INITIAL SCAN
    // ============================================================

    if (wallState == WF_INITIAL_SCAN)
    {
        car.stop();

        Serial.println();
        Serial.println("Running initial wall scan...");

        lastWallScan = scanWithRobot();

        printWallScan(lastWallScan);


        wallSide = chooseWallSide();


        if (wallSide == WALL_LEFT)
        {
            Serial.println("Selected wall: LEFT");
        }
        else if (wallSide == WALL_RIGHT)
        {
            Serial.println("Selected wall: RIGHT");
        }
        else
        {
            Serial.println(
                "ERROR: No usable wall detected.");

            car.stop();

            enterWallState(WF_FAILED);

            return;
        }


        havePreviousWallDistance = false;

        enterWallState(WF_ACQUIRE_WALL);

        Serial.println(
            "Acquiring 300 mm wall distance...");

        return;
    }


    // Everything below is controlled at fixed intervals.
    if (now - lastWallControl_ms < CONTROL_INTERVAL_MS)
        return;

    lastWallControl_ms = now;


    // ============================================================
    // ACQUIRE WALL DISTANCE
    // ============================================================

    if (wallState == WF_ACQUIRE_WALL)
    {
        readSensors(STAGE_WALL_FOLLOWING);


        // ----------------------------
        // FRONT HAS PRIORITY
        // ----------------------------

        uint16_t frontDistance;

        if (getFrontDistance(frontDistance))
        {
            if (frontDistance <= FRONT_CORNER_DISTANCE)
            {
                beginFrontCorner();
                return;
            }
        }


        // ----------------------------
        // SIDE WALL
        // ----------------------------

        uint16_t wallDistance;

        bool wallValid =
            getCurrentWallDistance(wallDistance);


        if (!wallValid ||
            wallDistance > WALL_LOST_DISTANCE)
        {
            lostWallCount++;

            if (lostWallCount >=
                WALL_LOST_REQUIRED_SAMPLES)
            {
                beginLostWallCorner();
            }

            return;
        }

        lostWallCount = 0;


        // Stronger correction during initial acquisition.
        steerAlongWall(
            wallDistance,
            slow,
            ACQUIRE_KP,
            ACQUIRE_KD);


        // Are we approximately 300 mm away?
        int16_t distanceError =
            (int16_t)wallDistance -
            (int16_t)WALL_TARGET_DISTANCE;


        if (abs(distanceError) <=
            WALL_DISTANCE_TOLERANCE)
        {
            acquireStableCount++;
        }
        else
        {
            acquireStableCount = 0;
        }


        if (acquireStableCount >=
            ACQUIRE_STABLE_SAMPLES)
        {
            Serial.println(
                "Wall distance acquired.");

            alignStableCount = 0;

            enterWallState(WF_ALIGN_WALL);
        }

        return;
    }


    // ============================================================
    // ALIGN PARALLEL WITH WALL
    // ============================================================

    if (wallState == WF_ALIGN_WALL)
    {
        readSensors(STAGE_WALL_FOLLOWING);


        // ----------------------------
        // FRONT PRIORITY
        // ----------------------------

        uint16_t frontDistance;

        if (getFrontDistance(frontDistance))
        {
            if (frontDistance <= FRONT_CORNER_DISTANCE)
            {
                beginFrontCorner();
                return;
            }
        }


        // ----------------------------
        // SIDE WALL
        // ----------------------------

        uint16_t wallDistance;

        bool wallValid =
            getCurrentWallDistance(wallDistance);


        if (!wallValid ||
            wallDistance > WALL_LOST_DISTANCE)
        {
            lostWallCount++;

            if (lostWallCount >=
                WALL_LOST_REQUIRED_SAMPLES)
            {
                beginLostWallCorner();
            }

            return;
        }

        lostWallCount = 0;


        int16_t delta = 999;

        if (havePreviousWallDistance)
        {
            delta =
                (int16_t)wallDistance -
                (int16_t)previousWallDistance_mm;
        }


        steerAlongWall(
            wallDistance,
            slow,
            WALL_KP,
            WALL_KD);


        int16_t distanceError =
            (int16_t)wallDistance -
            (int16_t)WALL_TARGET_DISTANCE;


        // Parallel means:
        //
        // 1. approximately the correct distance
        // 2. side distance isn't changing much anymore
        if (abs(distanceError) <=
                WALL_DISTANCE_TOLERANCE &&
            abs(delta) <= ALIGN_MAX_DELTA_MM)
        {
            alignStableCount++;
        }
        else
        {
            alignStableCount = 0;
        }


        if (alignStableCount >= ALIGN_STABLE_SAMPLES)
        {
            Serial.println(
                "Robot aligned with wall.");

            enterWallState(WF_FOLLOW_WALL);
        }

        return;
    }


    // ============================================================
    // NORMAL WALL FOLLOWING
    // ============================================================

    if (wallState == WF_FOLLOW_WALL)
    {
        readSensors(STAGE_WALL_FOLLOWING);


        // --------------------------------------------------------
        // 1. FRONT LASER HAS HIGHEST PRIORITY
        // --------------------------------------------------------

        uint16_t frontDistance;
        bool frontValid =
            getFrontDistance(frontDistance);


        if (frontValid)
        {
            // Emergency case
            if (frontDistance <=
                FRONT_EMERGENCY_DISTANCE)
            {
                car.stop();

                Serial.println(
                    "EMERGENCY: obstacle very close ahead.");

                beginFrontCorner();
                return;
            }


            // Normal corner
            if (frontDistance <=
                FRONT_CORNER_DISTANCE)
            {
                beginFrontCorner();
                return;
            }
        }


        // --------------------------------------------------------
        // 2. CHECK WHETHER SIDE WALL DISAPPEARED
        // --------------------------------------------------------

        uint16_t wallDistance;

        bool wallValid =
            getCurrentWallDistance(wallDistance);


        if (!wallValid ||
            wallDistance > WALL_LOST_DISTANCE)
        {
            lostWallCount++;

            // Require several consecutive readings so one
            // VL53L0X glitch doesn't trigger a corner.
            if (lostWallCount >=
                WALL_LOST_REQUIRED_SAMPLES)
            {
                beginLostWallCorner();
            }

            return;
        }

        lostWallCount = 0;


        // --------------------------------------------------------
        // 3. SELECT SPEED
        // --------------------------------------------------------

        int driveSpeed = fast;


        // Start slowing before the corner.
        if (frontValid &&
            frontDistance <= FRONT_SLOW_DISTANCE)
        {
            driveSpeed = slow;
        }


        // --------------------------------------------------------
        // 4. FOLLOW WALL
        // --------------------------------------------------------

        steerAlongWall(
            wallDistance,
            driveSpeed,
            WALL_KP,
            WALL_KD);

        return;
    }


    // ============================================================
    // FRONT / INSIDE CORNER
    // ============================================================
    //
    // Example:
    //
    // LEFT wall:
    //
    //       |
    // ------+
    // robot →
    //
    // The robot must turn RIGHT.
    //
    // Therefore:
    //
    // LEFT wall  -> turn RIGHT
    // RIGHT wall -> turn LEFT
    //
    // ============================================================

    if (wallState == WF_SCAN_FRONT_CORNER)
    {
        car.stop();

        Serial.println(
            "Scanning front corner...");

        lastWallScan = scanWithRobot();

        printWallScan(lastWallScan);


        if (wallSide == WALL_LEFT)
        {
            Serial.println(
                "Front corner -> turning RIGHT.");

            startCornerTurn(-1);
        }
        else
        {
            Serial.println(
                "Front corner -> turning LEFT.");

            startCornerTurn(+1);
        }

        return;
    }


    // ============================================================
    // LOST SIDE WALL / OUTSIDE CORNER
    // ============================================================
    //
    // LEFT wall disappears:
    //
    // --------
    //         |
    //         |
    // robot →
    //
    // Follow the wall around by turning LEFT.
    //
    // Therefore:
    //
    // LEFT wall  -> turn LEFT
    // RIGHT wall -> turn RIGHT
    //
    // ============================================================

    if (wallState == WF_SCAN_LOST_WALL)
    {
        car.stop();

        Serial.println(
            "Scanning after wall loss...");

        lastWallScan = scanWithRobot();

        printWallScan(lastWallScan);


        if (wallSide == WALL_LEFT)
        {
            Serial.println(
                "Lost LEFT wall -> turning LEFT.");

            startCornerTurn(+1);
        }
        else
        {
            Serial.println(
                "Lost RIGHT wall -> turning RIGHT.");

            startCornerTurn(-1);
        }

        return;
    }


    // ============================================================
    // TURN AROUND CORNER
    // ============================================================

    if (wallState == WF_TURN_CORNER)
    {
        // Because startCornerTurn() did car.stop() first,
        // turn() is an on-the-spot turn.
        car.turn(cornerTurnDirection * fast);


        readSensors(STAGE_WALL_FOLLOWING);


        unsigned long turnTime =
            now - wallStateStarted_ms;


        uint16_t wallDistance;

        bool wallValid =
            getCurrentWallDistance(wallDistance);


        uint16_t frontDistance;

        bool frontValid =
            getFrontDistance(frontDistance);


        bool frontClear =
            !frontValid ||
            frontDistance >
                FRONT_CORNER_DISTANCE;


        bool wallReacquired =
            wallValid &&
            wallDistance >= CORNER_REACQUIRE_MIN &&
            wallDistance <= CORNER_REACQUIRE_MAX;


        // Don't allow the old wall to instantly satisfy
        // the condition before we've actually turned.
        if (turnTime >= CORNER_MIN_TURN_MS &&
            frontClear &&
            wallReacquired)
        {
            cornerReacquireCount++;
        }
        else
        {
            cornerReacquireCount = 0;
        }


        // Require more than one valid sample.
        if (cornerReacquireCount >=
            CORNER_REACQUIRE_SAMPLES)
        {
            car.stop();

            Serial.println(
                "Wall reacquired after corner.");

            havePreviousWallDistance = false;

            enterWallState(WF_ACQUIRE_WALL);

            return;
        }


        // Never rotate indefinitely.
        if (turnTime >= CORNER_MAX_TURN_MS)
        {
            car.stop();

            Serial.println();
            Serial.println(
                "ERROR: corner turn timed out.");

            Serial.println(
                "Wall was not reacquired.");

            enterWallState(WF_FAILED);

            return;
        }

        return;
    }


    // ============================================================
    // FAILED
    // ============================================================

    if (wallState == WF_FAILED)
    {
        car.stop();
        return;
    }
}

