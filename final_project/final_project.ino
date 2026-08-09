//initalizing all the variables
  SteeringDualH car;
  Ultrasonic US60(trigPin60, echoPin60);
  Ultrasonic US120(trigPin120, echoPin120);
  Ultrasonic US240(trigPin240, echoPin240);
  Ultrasonic US300(trigPin300, echoPin300);

void setup()
{
  // declarations: 5 stages - open area, funnel, wall, wadi, flashlight; one information function
  // 

}

void loop()
{
  // call info_vector with an adress of the vector
  // check if a stage had changed
  // activate 1 out of 5 movement functions
  //repeat
}
