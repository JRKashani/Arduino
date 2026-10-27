#include <Arduino.h>
#include "ImuTiltMission.h"

ImuTiltMission mission;

void setup() {
  // Hold motor outputs low before opening the console or initializing the IMU.
  Serial.begin(115200);
  mission.begin();
}

void loop() { mission.update(); }
