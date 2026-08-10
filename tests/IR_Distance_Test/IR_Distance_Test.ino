/*
  Sharp_GP2Y0A21YK_Test.ino

  Minimal diagnostic test for ONE Sharp GP2Y0A21YK0F.

  WIRING
    VCC -> Mega 5V
    GND -> Mega GND
    Vo  -> Mega A0

  Recommended:
    10 uF or larger capacitor between VCC and GND,
    placed close to the sensor.

  The sensor's nominal useful range is 10-80 cm.

  Distance conversion below is approximate.
  Raw ADC is printed as well, so sensor operation can be
  verified independently of the conversion.

  Sharp recommends a 10 µF or larger bypass capacitor between VCC and GND near the sensor
*/

const uint8_t IR_PIN = A0;

void setup()
{
  Serial.begin(9600);

  Serial.println("Sharp GP2Y0A21YK test ready.");
}

void loop()
{
  int raw = analogRead(IR_PIN);

  // Mega default ADC reference is approximately 5 V.
  float voltage = raw * (5.0f / 1023.0f);

  Serial.print("Raw ADC: ");
  Serial.print(raw);

  Serial.print(" | Voltage: ");
  Serial.print(voltage, 2);
  Serial.print(" V");

  if (voltage > 0.05f)
  {
    // Simple approximation for GP2Y0A21YK.
    float distanceCm = 27.0f / voltage;

    Serial.print(" | Approx distance: ");
    Serial.print(distanceCm, 1);
    Serial.println(" cm");
  }
  else
  {
    Serial.println(" | Distance: INVALID");
  }

  delay(100);
}

/*
Raw ADC: 298 | Voltage: 1.46 V | Approx distance: 18.5 cm
Raw ADC: 301 | Voltage: 1.47 V | Approx distance: 18.4 cm
*/