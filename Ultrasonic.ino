int trigPin = 6;
int echoPin = 7;
int duration, distance;

void setup() {
  Serial.begin (9600);
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  }


void loop() {
digitalWrite(trigPin,HIGH); // send a pulse
delayMicroseconds(10);
digitalWrite(trigPin,LOW); 
duration = pulseIn(echoPin,HIGH);// get the pulse width
distance = duration/58; // convert the time to [cm]- formula by manufactur datasheet
Serial.print("The distance is "); // print the distance
Serial.print(distance); Serial.println(" [cm] ");
delay(1); // wait for new mesure
}

