#include "Ultrasonic.h"

Ultrasonic ultrasonic(12, 13);

void setup() {
  Serial.begin(9600);
}

void loop()
{
  Serial.print(ultrasonic.ranging(CM));
  Serial.println("\tcentimeters");
  Serial.print(ultrasonic.ranging(INC));
  Serial.println("\tinches");
  Serial.print(ultrasonic.timing());
  Serial.println("\tmicroseconds");
  delay(100);
}
