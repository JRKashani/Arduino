  #include "Globals_and_friends.h"
  
  const int redLedPin   =  1;
  const int greenLedPin =  1;

  const int trigPin60   =  1;
  const int echoPin60   =  1;

  const int trigPin120  =  13;
  const int echoPin120  =  12;

  const int trigPin240  =  2;
  const int echoPin240  =  1;

  const int trigPin300  =  1;
  const int echoPin300  =  1;

  Ultrasonic US60  (trigPin60,  echoPin60 );
  Ultrasonic US120 (trigPin120, echoPin120);
  Ultrasonic US240 (trigPin240, echoPin240);
  Ultrasonic US300 (trigPin300, echoPin300);

  SteeringDualH car;

  int slow       =  50;
  int fast       =  80;
  int veryFast   = 150;

  int tolerance_US            = 30;
  int tolerance_PhotoResistor = 80;
  float K = 1;

  SensorReading currentData;