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

// Stage 3 wall-lost / outside-corner state.
enum class WallMode : uint8_t { Follow, Search, Done };
WallMode wallMode = WallMode::Follow;
uint8_t wallGoneCount = 0;
unsigned long wallSearchStartMs = 0;
bool cornerActive = false; // inside-corner anticipation currently steering

void resetWallFollowState() {
  wallDerivInit = false;
  wallDErrFilt = 0.0f;
}

void resetWallStage() {
  resetWallFollowState();
  wallMode = WallMode::Follow;
  wallGoneCount = 0;
  cornerActive = false;
}

// Wall counts as present only if the laser is valid and within the "gone"
// threshold; a far reading means the beam has passed the end of the wall.
bool wallPresent(const SensorData &d) {
  return d.rightWallValid && d.rightWallCm * 10.0f <= (float)LASER_WALL_GONE_THRESHOLD_MM;
}

// Returns true if the caller should skip PD following this loop (searching/done).
bool handleWallLost(const SensorData &d) {
  const bool present = wallPresent(d);
  const unsigned long now = millis();

  if (wallMode == WallMode::Done) { stopMotion(); return true; }

  if (wallMode == WallMode::Search) {
    // Hysteresis: reacquire only clearly inside the gone threshold, so a wall
    // hovering at the threshold does not flip lost/found every loop.
    if (d.rightWallValid && d.rightWallCm * 10.0f <= (float)WALL_REACQUIRE_MM) {
      wallMode = WallMode::Follow;
      wallGoneCount = 0;
      resetWallFollowState(); // no derivative kick on reacquire
      Serial.print(F("WALL3 wall reacquired at ")); Serial.print(d.rightWallCm, 1);
      Serial.print(F("cm after ")); Serial.print(now - wallSearchStartMs);
      Serial.println(F("ms -> FOLLOW"));
      return false;
    }
    if (now - wallSearchStartMs >= WALL_SEARCH_TIMEOUT_MS) {
      wallMode = WallMode::Done;
      stopMotion();
      Serial.println(F("WALL3 search timeout (end of wall?) -> STOP"));
      return true;
    }
    // Negative command = toward the right wall (hardware-verified sign).
    driveContinuous(WALL_SEARCH_SPEED, -WALL_SEARCH_STEER);
    STAGE_LOG(
      Serial.print(F("WALL3 SEARCH t=")); Serial.print(now - wallSearchStartMs);
      Serial.print(F("ms rWall="));
      if (d.rightWallValid) Serial.print(d.rightWallCm, 1); else Serial.print(F("--"));
      Serial.print(F("cm steer=")); Serial.print(-WALL_SEARCH_STEER);
      Serial.print(F(" spd=")); Serial.println(WALL_SEARCH_SPEED));
    return true;
  }

  // Follow mode: require a few consecutive gone readings before searching.
  if (present) { wallGoneCount = 0; return false; }
  if (wallGoneCount < 255) wallGoneCount++;
  if (wallGoneCount < WALL_LOST_CONFIRM_COUNT) {
    // Not confirmed yet: keep the last command (motors unchanged) and skip PD
    // so a glitch or far reading does not produce a huge error/derivative.
    return true;
  }
  wallMode = WallMode::Search;
  wallSearchStartMs = now;
  resetWallFollowState();
  Serial.print(F("WALL3 wall lost (rWall="));
  if (d.rightWallValid) Serial.print(d.rightWallCm, 1); else Serial.print(F("--"));
  Serial.println(F("cm) -> SEARCH right"));
  driveContinuous(WALL_SEARCH_SPEED, -WALL_SEARCH_STEER);
  return true;
}

// Stage 3: follow the wall on the right at RIGHT_WALL_FOLLOW_TARGET_MM (measured
// from the robot center; the sensor layer already reports center-relative cm).
void runWallFollow(const SensorData &d) {
  if (handleWallLost(d)) return; // searching for / waiting on a lost wall
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
  const int pdCmd = -corr;

  // Inside corner anticipation: a wall closing in ahead (nose) or ahead-right
  // (45 deg) adds away-steer before the side laser reaches the new segment.
  const int cornerFront = d.frontValid
      ? rampByProximity(d.frontCm, CORNER_FRONT_START_CM, CORNER_FRONT_FULL_CM, CORNER_MAX_STEER) : 0;
  const int cornerR45 = d.rightDiagValid
      ? rampByProximity(d.rightDiagCm, CORNER_R45_START_CM, CORNER_R45_FULL_CM, CORNER_MAX_STEER) : 0;
  const int cornerAway = max(cornerFront, cornerR45);
  // While a corner is ahead, take the more-away command so PD (e.g. its D term
  // reacting to the turn) cannot cancel the corner turn; PD still wins if it
  // wants even more away. With no corner, PD alone (it must be able to steer
  // toward the wall).
  const int steerCmd = cornerAway > 0
      ? constrain(max(pdCmd, cornerAway), -WALL_FOLLOW_MAX_STEER, WALL_FOLLOW_MAX_STEER)
      : pdCmd;

  if (cornerAway > 0 && !cornerActive) {
    cornerActive = true;
    Serial.print(F("WALL3 inside corner ahead: front="));
    if (d.frontValid) Serial.print(d.frontCm, 1); else Serial.print(F("--"));
    Serial.print(F("cm r45="));
    if (d.rightDiagValid) Serial.print(d.rightDiagCm, 1); else Serial.print(F("--"));
    Serial.println(F("cm -> steer away"));
  } else if (cornerAway == 0 && cornerActive) {
    cornerActive = false;
    Serial.println(F("WALL3 corner cleared -> PD only"));
  }

  // Slow down while correcting hard: less sideways travel per heading change.
  const int speed = scaleSpeedBySteer(DRIVE_SPEED, WALL_FOLLOW_MIN_SPEED,
                                      steerCmd, WALL_FOLLOW_MAX_STEER);
  driveContinuous(speed, steerCmd);

  STAGE_LOG(
    Serial.print(F("WALL3 rWall=")); Serial.print(d.rightWallCm, 1);
    Serial.print(F("cm err=")); Serial.print(errorMm, 0);
    Serial.print(F("mm dErr=")); Serial.print(wallDErrFilt, 0);
    Serial.print(F("mm/s steer=")); Serial.print(steerCmd);
    Serial.print(steerCmd > 0 ? F(" away") : (steerCmd < 0 ? F(" toward") : F(" hold")));
    if (corr != 0 && abs(corr) >= WALL_FOLLOW_MAX_STEER) Serial.print(F(" CLAMPED"));
    Serial.print(F(" spd=")); Serial.print(speed);
    Serial.print(F(" front="));
    if (d.frontValid) Serial.print(d.frontCm, 1); else Serial.print(F("--"));
    Serial.print(F("cm r45="));
    if (d.rightDiagValid) Serial.print(d.rightDiagCm, 1); else Serial.print(F("--"));
    Serial.print(F("cm pd=")); Serial.print(pdCmd);
    Serial.print(F(" crn=")); Serial.println(cornerAway));
}
} // namespace

void startStage(uint8_t stage) {
  stopMotion();
  emergencyLatched = false;
  if (stage == STAGE_WALL) {
    current = STAGE_WALL;
    resetWallStage();
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
  resetWallStage();
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
