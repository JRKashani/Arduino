/*
  VL53L0X_Test.ino

  Minimal diagnostic test for ONE Pololu VL53L0X
  carrier board, product #2490.

  WIRING
    VIN -> Mega 5V
    GND -> Mega GND
    SDA -> Mega SDA / pin 20
    SCL -> Mega SCL / pin 21

    XSHUT -> not connected
    GPIO1 -> not connected
    VDD   -> not connected

  Requires:
    Pololu VL53L0X Arduino library
*/
/*
#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

void setup()
{
  Serial.begin(9600);

  Wire.begin();

  sensor.setTimeout(500);

  if (!sensor.init())
  {
    Serial.println("ERROR: VL53L0X not detected.");
    Serial.println("Check VIN, GND, SDA, and SCL.");

    while (true)
    {
      delay(1000);
    }
  }

  // One measurement approximately every 100 ms.
  sensor.startContinuous(100);

  Serial.println("VL53L0X_Test ready.");
}

void loop()
{
  uint16_t distanceMm =
      sensor.readRangeContinuousMillimeters();

  if (sensor.timeoutOccurred())
  {
    Serial.println("Distance: TIMEOUT");
  }
  else
  {
    Serial.print("Distance: ");
    Serial.print(distanceMm);
    Serial.println(" mm");
  }
}*/

#include <Wire.h>
#include <VL53L0X.h>

VL53L0X sensor;

void setup()
{
    Serial.begin(9600);
    delay(1000);

    Wire.begin();

    Serial.println("Starting VL53L0X test...");

    sensor.setTimeout(500);

    Serial.println("Calling sensor.init()...");

    if (!sensor.init())
    {
        Serial.println("INIT FAILED");
    }
    else
    {
        Serial.println("INIT SUCCESS");

        sensor.startContinuous(100);
    }
}

void loop()
{
    uint16_t distance = sensor.readRangeContinuousMillimeters();

    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" mm");

    if (sensor.timeoutOccurred())
    {
        Serial.println("TIMEOUT");
    }

    delay(100);
}