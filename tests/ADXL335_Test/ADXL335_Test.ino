/*
  ADXL335_Test.ino

  Minimal diagnostic test for an ADXL335 accelerometer.

  SAFE WIRING FOR A BASIC ADXL335 BREAKOUT
    VCC -> Mega 3.3V
    GND -> Mega GND
    X   -> Mega A0
    Y   -> Mega A1
    Z   -> Mega A2

  IMPORTANT:
    The ADXL335 IC itself is NOT a 5V device.

  This test prints:
    - raw ADC values
    - approximate roll
    - approximate pitch

  It cannot measure yaw.

  Roll/pitch are most useful while the sensor is stationary.
*/

#include <math.h>

const uint8_t X_PIN = A0;
const uint8_t Y_PIN = A1;
const uint8_t Z_PIN = A2;

// Approximate values for ADXL335 powered from 3.3 V.
// Zero-g output is approximately VCC / 2.
// Sensitivity is ratiometric, approximately 0.33 V/g at 3.3 V.
const float ZERO_G_VOLTAGE = 1.65f;
const float SENSITIVITY = 0.33f;

// Mega ADC uses approximately 5 V as its default reference.
const float ADC_REFERENCE = 5.0f;

void setup()
{
  Serial.begin(9600);

  Serial.println("ADXL335_Test ready.");
  Serial.println("Yaw is not available from ADXL335.");
}

void loop()
{
  int rawX = analogRead(X_PIN);
  int rawY = analogRead(Y_PIN);
  int rawZ = analogRead(Z_PIN);

  float voltageX = rawX * ADC_REFERENCE / 1023.0f;
  float voltageY = rawY * ADC_REFERENCE / 1023.0f;
  float voltageZ = rawZ * ADC_REFERENCE / 1023.0f;

  float ax = (voltageX - ZERO_G_VOLTAGE) / SENSITIVITY;
  float ay = (voltageY - ZERO_G_VOLTAGE) / SENSITIVITY;
  float az = (voltageZ - ZERO_G_VOLTAGE) / SENSITIVITY;

  float roll =
      atan2(ay, az) * 180.0f / PI;

  float pitch =
      atan2(-ax, sqrt(ay * ay + az * az))
      * 180.0f / PI;

  Serial.print("X: ");
  Serial.print(rawX);

  Serial.print(" | Y: ");
  Serial.print(rawY);

  Serial.print(" | Z: ");
  Serial.print(rawZ);

  Serial.print(" | Roll: ");
  Serial.print(roll, 1);
  Serial.print(" deg");

  Serial.print(" | Pitch: ");
  Serial.print(pitch, 1);
  Serial.println(" deg");

  delay(100);
}