#include <Ultrasonic.h>
#include <SteeringDualH.h>

extern UltraSonic US60;
extern UltraSonic US120;
extern UltraSonic US240;
extern UltraSonic US300;

extern const int redLedPin;
extern const int greenLedPin;

extern const int trigPin60;
extern const int echoPin60;

extern const int trigPin120;
extern const int echoPin120;

extern const int trigPin240;
extern const int echoPin240;

extern const int trigPin300;
extern const int echoPin300;

extern SteeringDualH car;

extern int slow;
extern int fast;
extern int veryFast;

extern int tolerance;
extern float K;

