//initalizing all the variables
#include "Globals.h"

void setup()
{
  // declarations: 5 stages - open area, funnel, wall, wadi, flashlight; one information function
  Serial.begin(9600);

  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);

  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);

  car.setBias(1);
  car.attach(46, 48, 44, 49, 47, 45);
  car.flipRight();

}

void loop()
{
  // call info_vector with an adress of the vector
  info_input();

  Serial.println(currentData.US60);
  Serial.println(currentData.US120);
  Serial.println(currentData.US240);
  Serial.println(currentData.US300);
  // check if a stage had changed
  // activate 1 out of 5 movement functions
  //repeat
}
