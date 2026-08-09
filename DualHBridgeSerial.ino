// Code by Reichenstein7 (thejamerson.com)
// Edited by Shaul Salomon 21-Nov-2017

//Keyboard Controls:
//
// 1 -Motor 1 Left
// 2 -Motor 1 Stop
// 3 -Motor 1 Right
//
// 4 -Motor 2 Left
// 5 -Motor 2 Stop
// 6 -Motor 2 Right
//
// 7 - Change Next Command to Slow
// 8 - Change Next Command to Fast

// Motor A - Left
int dir1PinA  = 7;
int dir2PinA  = 8;
int speedPinA = 9; // Needs to be a PWM pin to be able to control motor speed

// Motor B - Right
int dir1PinB  = 5;
int dir2PinB  = 4;
int speedPinB = 3; // Needs to be a PWM pin to be able to control motor speed

int allPins[6] = {dir1PinA, dir2PinA, speedPinA, dir1PinB, dir2PinB, speedPinB};

int speed; // A global variable that determines the speed required from the motors

//Change these numbers to specify different slow/fast configurations
const int slow = 100;
const int fast = 200;


void setup() {  // Setup runs once per reset
  // initialize serial communication @ 9600 baud:
  Serial.begin(9600);

  //Define Dual H-Bridge Motor Controller Pins

  pinMode(dir1PinA,  OUTPUT);
  pinMode(dir2PinA,  OUTPUT);
  pinMode(speedPinA, OUTPUT);
  pinMode(dir1PinB,  OUTPUT);
  pinMode(dir2PinB,  OUTPUT);
  pinMode(speedPinB, OUTPUT);

  Serial.println("-----------------------------------------");
  Serial.println("CHOOSE FROM THE FOLLOWING OPTIONS:");
  Serial.println("-----------------------------------------");
  Serial.println("1\t\tMotor 1 Forward");
  Serial.println("2\t\tMotor 1 Stop");
  Serial.println("3\t\tMotor 1 Reverse");
  Serial.println("4\t\tMotor 2 Forward");
  Serial.println("5\t\tMotor 2 Stop");
  Serial.println("6\t\tMotor 2 Reverse");
  Serial.println("7\t\tNext command will go Slow");
  Serial.println("8\t\tNext command will go Fast");
  Serial.println("Anything Else\tStop Both Motors");
  Serial.println("-----------------------------------------\n");

  speed = slow; //start at low speed
}

void loop() {

  // Initialize the Serial interface:

  if (Serial.available() > 0) {
    int inByte = Serial.read();

    switch (inByte) {

      //______________Motor 1______________

      case '1': // Motor 1 Forward
        digitalWrite(dir1PinA, LOW);
        digitalWrite(dir2PinA, HIGH);
        analogWrite(speedPinA, speed);//Sets speed variable via PWM
        Serial.print("Motor 1 Forward "); // Prints out “Motor 1 Forward” on the serial monitor
        Serial.println((speed == slow ? "Slow" : "Fast"));
        Serial.println("   "); // Creates a blank line printed on the serial monitor
        break;

      case '2': // Motor 1 Stop (Freespin)
        digitalWrite(dir1PinA, LOW);
        digitalWrite(dir2PinA, LOW);
        analogWrite(speedPinA, 0);
        Serial.println("Motor 1 Stop");
        Serial.println("   ");
        break;

      case '3': // Motor 1 Reverse
        digitalWrite(dir2PinA, LOW);
        digitalWrite(dir1PinA, HIGH);
        analogWrite(speedPinA, speed);
        Serial.print("Motor 1 Reverse ");
        Serial.println((speed == slow ? "Slow" : "Fast")); // A compact form of IF statement.
        // Equivilent to:
        // if (speed == slow) println("Slow"); else println("Fast");
        Serial.println("   ");
        break;

      //______________Motor 2______________

      case '4': // Motor 2 Forward
        digitalWrite(dir1PinB, LOW);
        digitalWrite(dir2PinB, HIGH);
        analogWrite(speedPinB, speed);
        Serial.print("Motor 2 Forward ");
        Serial.println((speed == slow ? "Slow" : "Fast"));
        Serial.println("   ");
        break;

      case '5': // Motor 1 Stop (Freespin)
        analogWrite(speedPinB, 0);
        digitalWrite(dir1PinB, LOW);
        digitalWrite(dir2PinB, HIGH);
        Serial.println("Motor 2 Stop");
        Serial.println("   ");
        break;

      case '6': // Motor 2 Reverse
        analogWrite(speedPinB, speed);
        digitalWrite(dir2PinB, LOW);
        digitalWrite(dir1PinB, HIGH);
        Serial.print("Motor 2 Reverse ");
        Serial.println((speed == slow ? "Slow" : "Fast"));
        Serial.println("   ");
        break;

      case '7': // Change the speed to slow
        speed = slow;
        Serial.println("Next command will go Slow");
        Serial.println("   ");
        break;

      case '8': // Change the speed to fast
        speed = fast;
        Serial.println("Next command will go Fast");
        Serial.println("   ");
        break;

      //default:
        // turn all the connections off if an unmapped key is pressed:
        //for (int i = 0; i < 6; i++) {
        //  digitalWrite(allPins[i], LOW);
        //}
        //Serial.println("Stop Both Motors");
        //Serial.println("   ");
    }
  }
}
