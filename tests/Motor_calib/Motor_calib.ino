#include <Arduino.h>
#include "Globals.h"

// ============================================================
// CALIBRATION SETTINGS
// ============================================================

// Straight-line PWM values to test
const int straightPWM[] = {50, 70, 90, 110, 130, 150};
const int NUM_STRAIGHT_PWM =
    sizeof(straightPWM) / sizeof(straightPWM[0]);

// Turn corrections to test.
// Positive = left, negative = right
const int turnCorrection[] = {
    -80, -60, -40, -20,
     20,  40,  60,  80
};
const int NUM_TURN_CORRECTIONS =
    sizeof(turnCorrection) / sizeof(turnCorrection[0]);

// Durations for each test
const unsigned long testTimes[] = {
    250,
    500,
    750,
    1000,
    1500
};

const int NUM_TIMES =
    sizeof(testTimes) / sizeof(testTimes[0]);

// Repeat each measurement.
// 2 is reasonable for initial calibration.
// Change to 3 for better data.
const int REPEATS = 2;


// ============================================================
// REGRESSION STORAGE
// Fits:
//
//      y = slope * time + intercept
//
// Straight:
//      distance_mm = velocity_mm_s * time_s + intercept
//
// Turning:
//      angle_deg = angularVelocity_deg_s * time_s + intercept
// ============================================================

struct Regression
{
    int n = 0;

    double sumX = 0;
    double sumY = 0;

    double sumXX = 0;
    double sumXY = 0;
    double sumYY = 0;

    void clear()
    {
        n = 0;

        sumX = 0;
        sumY = 0;

        sumXX = 0;
        sumXY = 0;
        sumYY = 0;
    }

    void add(double x, double y)
    {
        n++;

        sumX += x;
        sumY += y;

        sumXX += x * x;
        sumXY += x * y;
        sumYY += y * y;
    }

    double slope()
    {
        double denominator =
            n * sumXX - sumX * sumX;

        if (fabs(denominator) < 0.000001)
            return 0;

        return
            (n * sumXY - sumX * sumY)
            / denominator;
    }

    double intercept()
    {
        if (n == 0)
            return 0;

        return
            (sumY - slope() * sumX)
            / n;
    }

    double rSquared()
    {
        double numerator =
            n * sumXY - sumX * sumY;

        double denominatorX =
            n * sumXX - sumX * sumX;

        double denominatorY =
            n * sumYY - sumY * sumY;

        double denominator =
            denominatorX * denominatorY;

        if (denominator <= 0)
            return 0;

        return
            (numerator * numerator)
            / denominator;
    }
};


Regression straightRegression[NUM_STRAIGHT_PWM];
Regression turnRegression[NUM_TURN_CORRECTIONS];

int calibratedTurnBasePWM = 0;


// ============================================================
// SERIAL INPUT
// ============================================================

void clearSerialInput()
{
    while (Serial.available())
        Serial.read();
}


void waitForEnter()
{
    clearSerialInput();

    while (true)
    {
        if (Serial.available())
        {
            char c = Serial.read();

            if (c == '\n' || c == '\r')
            {
                delay(50);
                clearSerialInput();
                return;
            }
        }
    }
}


float readFloatFromSerial()
{
    String input = "";

    while (true)
    {
        if (Serial.available())
        {
            char c = Serial.read();

            if (c == '\n' || c == '\r')
            {
                if (input.length() > 0)
                    return input.toFloat();
            }
            else
            {
                input += c;
            }
        }
    }
}


int readIntFromSerial()
{
    return (int)readFloatFromSerial();
}


// ============================================================
// MOTOR TEST FUNCTIONS
// ============================================================

void runStraightTest(
    int pwm,
    unsigned long durationMs)
{
    car.straight(pwm);

    delay(durationMs);

    car.stop();

    delay(300);
}


void runTurnTest(
    int basePWM,
    int correction,
    unsigned long durationMs)
{
    // Establish forward speed
    car.straight(basePWM);

    delay(100);

    // Apply differential steering correction
    car.turn(correction);

    delay(durationMs);

    car.stop();

    delay(300);
}


// ============================================================
// STRAIGHT DISTANCE CALIBRATION
// ============================================================

void calibrateDistance()
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("STRAIGHT DISTANCE CALIBRATION");
    Serial.println("======================================");

    Serial.println(
        "Measure the actual distance travelled in mm.");

    Serial.println(
        "Place the robot at the same starting line "
        "before every trial.");

    Serial.println();

    for (int p = 0; p < NUM_STRAIGHT_PWM; p++)
    {
        straightRegression[p].clear();

        for (int t = 0; t < NUM_TIMES; t++)
        {
            for (int r = 0; r < REPEATS; r++)
            {
                int pwm = straightPWM[p];
                unsigned long duration = testTimes[t];

                Serial.println();
                Serial.println("--------------------------------------");

                Serial.print("PWM: ");
                Serial.println(pwm);

                Serial.print("Time: ");
                Serial.print(duration);
                Serial.println(" ms");

                Serial.print("Repeat: ");
                Serial.print(r + 1);
                Serial.print("/");
                Serial.println(REPEATS);

                Serial.println(
                    "Position robot, then press ENTER.");

                waitForEnter();

                Serial.println("RUNNING");

                runStraightTest(
                    pwm,
                    duration);

                Serial.println("STOPPED");

                Serial.println(
                    "Enter measured distance in mm:");

                float distance =
                    readFloatFromSerial();

                double timeSeconds =
                    duration / 1000.0;

                straightRegression[p].add(
                    timeSeconds,
                    distance);

                // Raw CSV output
                Serial.print("RAW,DISTANCE,");
                Serial.print(pwm);
                Serial.print(",");
                Serial.print(duration);
                Serial.print(",");
                Serial.println(distance, 2);
            }
        }
    }

    printDistanceModels();
}


// ============================================================
// TURN / HEADING CALIBRATION
// ============================================================

void calibrateTurning()
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("TURNING CALIBRATION");
    Serial.println("======================================");

    Serial.println(
        "Enter the forward PWM used while turning:");

    Serial.println(
        "Example: 80");

    calibratedTurnBasePWM =
        readIntFromSerial();

    Serial.print("Base PWM = ");
    Serial.println(calibratedTurnBasePWM);

    Serial.println();
    Serial.println(
        "Measure HEADING CHANGE, not path angle.");

    Serial.println(
        "Use positive degrees for LEFT.");

    Serial.println(
        "Use negative degrees for RIGHT.");

    Serial.println();

    for (int c = 0; c < NUM_TURN_CORRECTIONS; c++)
    {
        turnRegression[c].clear();

        for (int t = 0; t < NUM_TIMES; t++)
        {
            for (int r = 0; r < REPEATS; r++)
            {
                int correction =
                    turnCorrection[c];

                unsigned long duration =
                    testTimes[t];

                Serial.println();
                Serial.println("--------------------------------------");

                Serial.print("Base PWM: ");
                Serial.println(calibratedTurnBasePWM);

                Serial.print("Turn correction: ");
                Serial.println(correction);

                Serial.print("Time: ");
                Serial.print(duration);
                Serial.println(" ms");

                Serial.print("Repeat: ");
                Serial.print(r + 1);
                Serial.print("/");
                Serial.println(REPEATS);

                Serial.println(
                    "Reset robot position and heading.");

                Serial.println(
                    "Press ENTER when ready.");

                waitForEnter();

                Serial.println("RUNNING");

                runTurnTest(
                    calibratedTurnBasePWM,
                    correction,
                    duration);

                Serial.println("STOPPED");

                Serial.println(
                    "Enter heading change in degrees:");

                float angle =
                    readFloatFromSerial();

                double timeSeconds =
                    duration / 1000.0;

                turnRegression[c].add(
                    timeSeconds,
                    angle);

                // Raw CSV output
                Serial.print("RAW,ANGLE,");
                Serial.print(calibratedTurnBasePWM);
                Serial.print(",");
                Serial.print(correction);
                Serial.print(",");
                Serial.print(duration);
                Serial.print(",");
                Serial.println(angle, 2);
            }
        }
    }

    printTurnModels();
}


// ============================================================
// RESULTS
// ============================================================

void printDistanceModels()
{
    Serial.println();
    Serial.println();
    Serial.println("======================================");
    Serial.println("DISTANCE CALIBRATION RESULTS");
    Serial.println("======================================");

    Serial.println(
        "PWM,velocity_mm_s,intercept_mm,R2");

    for (int i = 0; i < NUM_STRAIGHT_PWM; i++)
    {
        double velocity =
            straightRegression[i].slope();

        double intercept =
            straightRegression[i].intercept();

        double r2 =
            straightRegression[i].rSquared();

        Serial.print(straightPWM[i]);
        Serial.print(",");

        Serial.print(velocity, 3);
        Serial.print(",");

        Serial.print(intercept, 3);
        Serial.print(",");

        Serial.println(r2, 4);
    }

    Serial.println();
    Serial.println(
        "Model:");

    Serial.println(
        "distance_mm = velocity_mm_s * time_s + intercept");
}


void printTurnModels()
{
    Serial.println();
    Serial.println();
    Serial.println("======================================");
    Serial.println("TURN CALIBRATION RESULTS");
    Serial.println("======================================");

    Serial.println(
        "basePWM,correction,deg_s,intercept_deg,R2");

    for (int i = 0;
         i < NUM_TURN_CORRECTIONS;
         i++)
    {
        double angularVelocity =
            turnRegression[i].slope();

        double intercept =
            turnRegression[i].intercept();

        double r2 =
            turnRegression[i].rSquared();

        Serial.print(calibratedTurnBasePWM);
        Serial.print(",");

        Serial.print(turnCorrection[i]);
        Serial.print(",");

        Serial.print(angularVelocity, 3);
        Serial.print(",");

        Serial.print(intercept, 3);
        Serial.print(",");

        Serial.println(r2, 4);
    }

    Serial.println();
    Serial.println(
        "Model:");

    Serial.println(
        "angle_deg = angularVelocity_deg_s * time_s + intercept");
}


// ============================================================
// MENU
// ============================================================

void printMenu()
{
    Serial.println();
    Serial.println("======================================");
    Serial.println("ROBOT MOTION CALIBRATION");
    Serial.println("======================================");

    Serial.println("D - Distance calibration");
    Serial.println("A - Angle calibration");
    Serial.println("P - Print current results");
    Serial.println();
}


void setup()
{
    Serial.begin(115200);

    delay(1000);

    car.stop();

    printMenu();
}


void loop()
{
    if (!Serial.available())
        return;

    char command =
        toupper(Serial.read());

    clearSerialInput();

    switch (command)
    {
        case 'D':
            calibrateDistance();
            printMenu();
            break;

        case 'A':
            calibrateTurning();
            printMenu();
            break;

        case 'P':
            printDistanceModels();
            printTurnModels();
            printMenu();
            break;
    }
}