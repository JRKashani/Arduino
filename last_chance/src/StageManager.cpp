#include "StageManager.h"
#include "DEFINE.h"
#include "Log.h"
#include "MotionCapsule.h"
#include "SensorsCapsule.h"
#include <Control.h>

namespace {
Stage current = STAGE_NONE;
bool emergencyLatched = false;

// Stage 3 derivative-term state (approach rate of the right-wall distance).
bool wallDerivInit = false;
float wallPrevErrorMm = 0.0f;
unsigned long wallPrevDerivMs = 0;
float wallDErrFilt = 0.0f; // low-passed dError, mm/s

void resetWallFollowState() {
  wallDerivInit = false;
  wallDErrFilt = 0.0f;
}

// Stage 3: follow the wall on the right at RIGHT_WALL_FOLLOW_TARGET_MM (measured
// from the robot center; the sensor layer already reports center-relative cm).
void runWallFollow(const SensorData &d) {
  if (!d.rightWallValid) {
    // Lost the wall (out of range / gap): creep straight and reset the
    // derivative so reacquiring the wall does not cause a kick.
    driveContinuous(DRIVE_SPEED, 0);
    resetWallFollowState();
    STAGE_LOG(Serial.println(F("WALL3 no-wall (rightWall invalid) -> creep straight")));
    return;
  }
  const float errorMm = d.rightWallCm * 10.0f - (float)RIGHT_WALL_FOLLOW_TARGET_MM;

  // Approach rate (mm/s), recomputed on a fixed cadence for a stable dt and
  // low-passed to tame laser noise. d(error)/dt is a heading proxy: negative
  // means closing on the wall.
  const unsigned long now = millis();
  if (!wallDerivInit) {
    wallDerivInit = true;
    wallPrevErrorMm = errorMm;
    wallPrevDerivMs = now;
    wallDErrFilt = 0.0f;
  } else if (now - wallPrevDerivMs >= WALL_FOLLOW_DERIV_INTERVAL_MS) {
    const float dt = (now - wallPrevDerivMs) / 1000.0f;
    const float raw = (errorMm - wallPrevErrorMm) / dt;
    wallDErrFilt = WALL_FOLLOW_DERIV_LPF_ALPHA * raw +
                   (1.0f - WALL_FOLLOW_DERIV_LPF_ALPHA) * wallDErrFilt;
    wallPrevErrorMm = errorMm;
    wallPrevDerivMs = now;
  }

  const int corr = steerFromPD(errorMm, wallDErrFilt, WALL_FOLLOW_GAIN,
                               WALL_FOLLOW_DERIV_GAIN, WALL_FOLLOW_MAX_STEER,
                               WALL_FOLLOW_DEADBAND_MM);
  // Hardware sign (verified from the crash-into-wall logs): a POSITIVE command
  // steers the robot away from the right wall, negative steers toward it.
  // error > 0 (too far) must steer toward the wall => command = -corr.
  const int steerCmd = -corr;
  driveContinuous(DRIVE_SPEED, steerCmd);

  STAGE_LOG(
    Serial.print(F("WALL3 rWall=")); Serial.print(d.rightWallCm, 1);
    Serial.print(F("cm err=")); Serial.print(errorMm, 0);
    Serial.print(F("mm dErr=")); Serial.print(wallDErrFilt, 0);
    Serial.print(F("mm/s steer=")); Serial.print(steerCmd);
    Serial.print(steerCmd > 0 ? F(" away") : (steerCmd < 0 ? F(" toward") : F(" hold")));
    if (corr != 0 && abs(corr) >= WALL_FOLLOW_MAX_STEER) Serial.print(F(" CLAMPED"));
    Serial.print(F(" spd=")); Serial.print(DRIVE_SPEED);
    Serial.print(F(" front="));
    if (d.frontValid) Serial.print(d.frontCm, 1); else Serial.print(F("--"));
    Serial.println(F("cm")));
}
} // namespace

void startStage(uint8_t stage) {
  stopMotion();
  emergencyLatched = false;
  if (stage == STAGE_WALL) {
    current = STAGE_WALL;
    resetWallFollowState();
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
  resetWallFollowState();
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
