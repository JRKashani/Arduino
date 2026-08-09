int trigPin = 6;
int echoPin = 7;

void setup() {
  Serial.begin (9600);
  setUltrasonicPins();
}


void loop()
{
  int distance = distanceCM();
  printDistance(distance);
  delay(50); // wait before new mesure
}

void setUltrasonicPins()
{

}

int distanceCM()
{

  return ;
}

void printDistance()
{

}

