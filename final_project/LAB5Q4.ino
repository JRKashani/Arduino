#include <Ultrasonic.h>
#include <SteeringDualH.h>

float distance, delta, mapped_delta;
int target_dis  =  20;
int trigPin     =   2;
int echoPin     =   3;
int redLedPin   =  13;
int greenLedPin =  12;

int slow       =  50;
int fast       =  80;
int veryFast   = 150;

int tolerance  =   1;
float K = 1;

SteeringDualH car;
Ultrasonic sensor(trigPin, echoPin);

void setup() {
  Serial.begin(9600);
  pinMode(redLedPin, OUTPUT);
  pinMode(greenLedPin, OUTPUT);
  digitalWrite(redLedPin, LOW);
  digitalWrite(greenLedPin, LOW);
  car.setBias(1);
  car.attach(46, 48, 44, 49, 47, 45);
  car.flipRight();
}

void loop() {
  // put your main code here, to run repeatedly:
  car.straight(slow);
  distance = sensor.ranging(CM);
  distance = abs(distance);
  delta = distance - target_dis;
  Serial.print("delta: ");
  Serial.println(delta);
  if (abs(delta) > tolerance)
  {
    Serial.println((delta > 0 ? "Turn Right!" : "Turn Left!"));
    car.turn(-delta*K);
  }
  delay(50);
}
