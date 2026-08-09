  #include "Globals.h"
  
  const int redLedPin   =  11;
  const int greenLedPin =  10;

  const int trigPin60  = 13;
  const int echoPin60  = 12;

  const int trigPin120 = 4;
  const int echoPin120 = 5;

  const int trigPin240 = 3;
  const int echoPin240 = 2;

  const int trigPin300 = 8;
  const int echoPin300 = 9;

  Ultrasonic US60(trigPin60, echoPin60);
  Ultrasonic US120(trigPin120, echoPin120);
  Ultrasonic US240(trigPin240, echoPin240);
  Ultrasonic US300(trigPin300, echoPin300);

  SteeringDualH car;

  int slow       =  50;
  int fast       =  80;
  int veryFast   = 150;

  int tolerance  =   1;
  float K = 1;
