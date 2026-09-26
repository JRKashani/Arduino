#include "MotionTest.h"
#include "MotionCapsule.h"
#include "DEFINE.h"

void printMotionTestMenu() {
  Serial.println(F("\n=== MOTION CAPSULE TEST: 115200 baud ==="));
  Serial.print(F("F: forward ")); Serial.print(MOTION_TEST_DISTANCE_CM);
  Serial.println(F(" cm; L: left quarter-circle; R: right quarter-circle"));
  Serial.println(F("S: stop/cancel; H/?: help. Send one command at a time."));
  Serial.println(F("Timed estimates; nominal radius 60 cm. No obstacle stop."));
}

void runMotionTest() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\r' || c == '\n' || c == ' ' || c == '\t') continue;
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    if (c == 'S') {
      stopMotion();
      while (Serial.available()) Serial.read();
      Serial.println(F("STOPPED/CANCELLED"));
      return;
    }
    if (motionBusy()) {
      stopMotion();
      while (Serial.available()) Serial.read();
      Serial.println(F("Active test cancelled. Send the next command separately."));
      return;
    }
    bool accepted = true;
    switch (c) {
      case 'F': accepted = driveCm(MOTION_TEST_DISTANCE_CM); break;
      case 'L': accepted = turnQuarterCircle(TurnDirection::Left); break;
      case 'R': accepted = turnQuarterCircle(TurnDirection::Right); break;
      case 'H': case '?': printMotionTestMenu(); break;
      default: stopMotion(); Serial.println(F("Unknown command; stopped. H for help.")); break;
    }
    if (!accepted) Serial.println(F("Motion rejected: check distance or calibration settings."));
  }
  runMotionCapsule();
}
