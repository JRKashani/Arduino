#include "StageManager.h"
#include "DEFINE.h"
#include "MotionCapsule.h"
#include "SensorsCapsule.h"
#include <Control.h>

namespace {
Stage current = STAGE_NONE;
bool emergencyLatched = false;

// Stage 3: follow the wall on the right at RIGHT_WALL_FOLLOW_TARGET_MM (measured
// from the robot center; the sensor layer already reports center-relative cm).
void runWallFollow(const SensorData &d) {
  if (!d.rightWallValid) {
    // Lost the wall (out of range / gap): creep straight until it returns.
    driveContinuous(DRIVE_SPEED, 0);
    return;
  }
  const float errorMm = d.rightWallCm * 10.0f - (float)RIGHT_WALL_FOLLOW_TARGET_MM;
  const int steer = steerFromError(errorMm, WALL_FOLLOW_GAIN,
                                   WALL_FOLLOW_MAX_STEER, WALL_FOLLOW_DEADBAND_MM);
  // error > 0 => too far from the right wall => steer toward it (turn right =
  // negative harshness). If the robot corrects the wrong way on the bench,
  // flip the sign on this one line.
  driveContinuous(DRIVE_SPEED, -steer);
}
} // namespace

void startStage(uint8_t stage) {
  stopMotion();
  emergencyLatched = false;
  if (stage == STAGE_WALL) {
    current = STAGE_WALL;
    Serial.println(F("STAGE 3: right-wall follow @30 cm. S/X stops."));
  } else if (stage >= STAGE_OPEN && stage <= STAGE_LIGHT) {
    current = STAGE_NONE;
    Serial.print(F("Stage ")); Serial.print(stage);
    Serial.println(F(" not implemented yet."));
  } else {
    current = STAGE_NONE;
    Serial.println(F("Unknown stage."));
  }
}

void stopStage() {
  if (current != STAGE_NONE) Serial.println(F("Stage stopped."));
  current = STAGE_NONE;
  emergencyLatched = false;
  stopMotion();
}

bool stageRunning() { return current != STAGE_NONE; }

void runStageManager() {
  if (current == STAGE_NONE) return;
  const SensorData &d = latestSensorData();

  // Safety override: stop if something is close ahead. Latched so we report it
  // once and stay stopped until the path clears.
  if (d.frontValid && d.frontCm <= FRONT_EMERGENCY_STOP_CM) {
    stopMotion();
    if (!emergencyLatched) {
      emergencyLatched = true;
      Serial.print(F("SAFETY STOP: front ")); Serial.print(d.frontCm, 1);
      Serial.println(F(" cm."));
    }
    return;
  }
  if (emergencyLatched && d.frontValid && d.frontCm >= FRONT_EMERGENCY_CLEAR_CM) {
    emergencyLatched = false;
    Serial.println(F("Front clear; resuming."));
  }
  if (emergencyLatched) { stopMotion(); return; }

  switch (current) {
    case STAGE_WALL: runWallFollow(d); break;
    default: stopStage(); break;
  }
}
