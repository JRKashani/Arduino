#include <SteeringDualH.h>

SteeringDualH robot;

int slow   = 70;
int fast   = 150

void setup() {
//  robot.attach(45, 47, 49, 44, 46, 48);  // for Mega

  robot.attach(7, 8, 9, 5, 4, 3);
//  robot.flipLeft();    // change the definition for forward for the left wheel
//  robot.setBias(-10);  // compansate for speed differences between the wheels
}

void loop() {
  robot.straight(slow); //slow forward
  delay(2000);
  robot.turn(10);  //turn slightly left
  delay(1500);
  robot.turn(-50); //stronger turn right
  delay(1500);
  robot.stop();
  delay(1000);
  robot.straight(-fast); //fast backwards
  delay(500);
  robot.stop();
  delay(1000);
  robot.turn(50);   //slowly turn clockwise
  delay(2000);
  robot.stop();
  delay(500);
  robot.turn(-250);   //quickly turn clockwise
  delay(500);
  robot.stop();
  delay(1000);
}
