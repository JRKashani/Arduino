/*
  LDR_Test.ino

  Minimal diagnostic test for one photoresistor.

  WIRING

    5V
     |
    LDR
     |
     +------ Mega A0
     |
    10 kOhm
     |
    GND

  With this wiring:
    more light -> larger ADC value
    less light -> smaller ADC value

  This is NOT a lux measurement.
*/

const uint8_t LDR_PIN = A0;

void setup()
{
  Serial.begin(9600);

  Serial.println("LDR_Test ready.");
}

void loop()
{
  int lightRaw = analogRead(LDR_PIN);

  Serial.print("Light raw: ");
  Serial.println(lightRaw);

  delay(200);
}