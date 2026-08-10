void openAreaNavigation(const SensorReading &sensors)
{
  bool isLeftFrontClose = false;
  bool isRightFrontClose = false;
  int right_or_left = 0; // -1 for left, 1 for right
  float US60Distance = 0;
  float US300Distance = 0;
  float laserFrontDistance = 0;
  //delay(3000); // Wait for 10 seconds before starting the navigation

  while(!isLeftFrontClose && !isRightFrontClose)
  {
    US60Distance = sensors.US60_cm;
    US300Distance = sensors.US300_cm;
    laserFrontDistance = 10 * sensors.laserFront_mm;
    
    Serial.print("US60Distance: ");
    Serial.println(US60Distance);    
    Serial.print("US300Distance: ");
    Serial.println(US300Distance);    
    Serial.print("laserFrontDistance: ");
    Serial.println(laserFrontDistance);
    
    //delay(100);    
    /*
    Serial.print("US60_valid: ");
    Serial.println(sensors.US60_valid);
    Serial.print("US300_valid: ");
    Serial.println(sensors.US120_valid);
    Serial.print("laserFront_valid: ");
    Serial.println(sensors.laserFront_valid);*/

    car.straight(slow); 
    // Check if both front sensors (US60 and US300) have valid readings
    // that are under 50.0 cm.
    isLeftFrontClose = (sensors.US60_valid && US60Distance < 50.0f);
    isRightFrontClose = (sensors.US300_valid && US300Distance < 50.0f);

    if ((isLeftFrontClose || isRightFrontClose) && (sensors.laserFront_valid && laserFrontDistance < 100.0f))
    {
      car.stop();
      right_or_left = (US60Distance < US300Distance) ? -1 : 1;
      car.turn(right_or_left * 20); // Turn left or right based on which sensor is closer
      delay(100);       
    }

  }
  // Blink the white LED 3 times to indicate stage transition
  car.stop();
  for (int i = 0; i <= 3; i++)
  {
    digitalWrite(whiteLedPin, HIGH);
    delay(125);
    digitalWrite(whiteLedPin, LOW);
    delay(125);
  }
}