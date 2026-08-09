#include <Motor.h>

int slow   = 50;
int medium = 100;
int fast   = 150;

Motor leftWheel;           // Default constructor
Motor rightWheel(7, 8, 9); // Instantiate the object with the pins

void setup() {
  leftWheel.setPins(5, 4, 3); // dirPin1, dirPin2, pwmPin
  leftWheel.setVelocity(medium);
}

void loop() {
  rightWheel.setVelocity(fast); // CW
  delay(2000);
  rightWheel.stop();
  delay(500);
  rightWheel.setVelocity(-fast); // CCW
  delay(2000);
  rightWheel.stop();
  delay(500);
  
  rightWheel.flip(); // Toggle the definition for positive velocity (CW/CCW)
  
  rightWheel.setVelocity(slow); // CCW
  delay(2000);
  rightWheel.stop();
  delay(500);
  rightWheel.setVelocity(-slow); // CW
  delay(2000);
  rightWheel.stop();

  rightWheel.flip(); // Toggle back the definition for positive velocity (CW/CCW)
  
  delay(1000);

  // Revert the direction of the left wheel
  leftWheel.setVelocity(-leftWheel.velocity());

  delay(3000);

}