#include "StageManager.h"
#include "DEFINE.h"
#include "Log.h"
#include "MotionCapsule.h"
#include "SensorsCapsule.h"
#include <Control.h>

namespace {
Stage current = STAGE_NONE;
bool emergencyLatched = false;

enum class EntryMode : uint8_t {
  Approach, Settling, ChooseDirection, Turning, SeekEquality,
  Align45, CenterDrive, CorrectionOut, CorrectionDrive, CorrectionBack
};
EntryMode entryMode = EntryMode::Approach;
EntryMode entryAfterSettle = EntryMode::ChooseDirection;
unsigned long entrySettleStartMs = 0;
TurnDirection entryFirstTurn = TurnDirection::Left;
TurnDirection entryCorrectionTurn = TurnDirection::Left;
bool entryMatchInitialized = false;
float entryPreviousMatchErrorCm = 0.0f;

const __FlashStringHelper *entryModeName(EntryMode mode) {
  switch (mode) {
    case EntryMode::Approach: return F("APPROACH");
    case EntryMode::Settling: return F("SETTLING");
    case EntryMode::ChooseDirection: return F("CHOOSE_SIDE");
    case EntryMode::Turning: return F("TURN_90");
    case EntryMode::SeekEquality: return F("MATCH_FRONT_SIDE");
    case EntryMode::Align45: return F("ALIGN_45");
    case EntryMode::CenterDrive: return F("CENTER");
    case EntryMode::CorrectionOut: return F("CORRECTION_TURN_OUT");
    case EntryMode::CorrectionDrive: return F("CORRECTION_DRIVE");
    case EntryMode::CorrectionBack: return F("CORRECTION_TURN_BACK");
  }
  return F("UNKNOWN");
}

void logEntryDecision(const __FlashStringHelper *decision) {
  Serial.print(F("ENTRY stage=")); Serial.print(static_cast<uint8_t>(current));
  Serial.print(F(" t=")); Serial.print(millis());
  Serial.print(F("ms state=")); Serial.print(entryModeName(entryMode));
  Serial.print(F(" decision=")); Serial.println(decision);
}
static_assert(ENTRY_TURN_CALIBRATION_INDEX < MOTOR_TURN_90DEG_COUNT,
              "Stage 1 turn calibration index out of range");
static_assert(STAGE1_WALL_TARGET_CM > FRONT_EMERGENCY_STOP_CM,
              "Stage 1 target must be above the front safety stop");

TurnDirection opposite(TurnDirection direction) {
  return direction == TurnDirection::Left ? TurnDirection::Right : TurnDirection::Left;
}

void settleEntryReadings(EntryMode next) {
  stopMotion();
  resetDistanceFilters();
  entrySettleStartMs = millis();
  entryAfterSettle = next;
  entryMode = EntryMode::Settling;
  Serial.print(F("ENTRY filters reset; settle_ms=")); Serial.print(ENTRY_MEASURE_SETTLE_MS);
  Serial.print(F(" next=")); Serial.println(entryModeName(next));
}

bool startEntryTurn(TurnDirection direction, float degrees, EntryMode next) {
  stopMotion();
  logEntryDecision(direction == TurnDirection::Left ? F("TURN_LEFT") : F("TURN_RIGHT"));
  Serial.print(F("ENTRY turn_angle_deg=")); Serial.println(degrees, 1);
  if (!turnCalibrated(direction, degrees, ENTRY_TURN_CALIBRATION_INDEX)) {
    Serial.println(F("ENTRY turn rejected -> STOP"));
    stopStage();
    return false;
  }
  entryMode = next;
  Serial.print(F("ENTRY state=")); Serial.println(entryModeName(next));
  return true;
}

void runEntryStages(const SensorData &d) {
  if (entryMode == EntryMode::Settling) {
    if (millis() - entrySettleStartMs >= ENTRY_MEASURE_SETTLE_MS) {
      logEntryDecision(F("fresh-reading wait finished; enter next state"));
      entryMode = entryAfterSettle;
      Serial.print(F("ENTRY state=")); Serial.println(entryModeName(entryMode));
    } else {
      logEntryDecision(F("STOP; wait for filters to refill"));
    }
    return;
  }
  if (entryMode == EntryMode::Approach) {
    // No trustworthy front range means no forward motion.
    if (!d.frontValid) {
      logEntryDecision(F("STOP; invalid front reading"));
      stopMotion(); return;
    }
    // Use the current calibrated sample too: the median can lag while closing.
    const float frontCm = min(d.frontCm,
        d.frontRawCm + ULTRASONIC_FRONT_FORWARD_OFFSET_CM);
    Serial.print(F("ENTRY approach front_used_cm=")); Serial.print(frontCm, 1);
    Serial.print(F(" target_cm=")); Serial.println(STAGE1_WALL_TARGET_CM, 1);
    if (frontCm > STAGE1_WALL_TARGET_CM) {
      logEntryDecision(F("DRIVE_FORWARD; front greater than target"));
      driveContinuous(STAGE1_APPROACH_PWM, 0, MOTION_STRAIGHT_BIAS);
      return;
    }
    logEntryDecision(F("STOP; target reached; prepare side comparison"));
    settleEntryReadings(EntryMode::ChooseDirection);
    Serial.print(F("ENTRY wall reached at ")); Serial.print(frontCm, 1);
    Serial.println(F("cm -> compare sides"));
    return; // compare a fresh side snapshot after stopping
  }

  if (entryMode == EntryMode::ChooseDirection) {
    stopMotion();
    // Invalid or equal readings cannot identify the closer side. Stay stopped.
    if (!d.leftWallValid || !d.rightWallValid) {
      logEntryDecision(F("STOP; invalid side reading"));
      return;
    }
    if (d.leftWallCm == d.rightWallCm) {
      logEntryDecision(F("STOP; sides equal; turn direction unknown"));
      return;
    }
    entryFirstTurn = d.leftWallCm < d.rightWallCm
        ? TurnDirection::Left : TurnDirection::Right;
    Serial.print(F("ENTRY L=")); Serial.print(d.leftWallCm, 1);
    Serial.print(F("cm R=")); Serial.print(d.rightWallCm, 1);
    Serial.println(entryFirstTurn == TurnDirection::Left
        ? F("cm -> calibrated 90 deg LEFT") : F("cm -> calibrated 90 deg RIGHT"));
    startEntryTurn(entryFirstTurn, 90.0f, EntryMode::Turning);
    return;
  }

  if (entryMode == EntryMode::SeekEquality) {
    // Left 90 -> old forward is now right. Right 90 -> it is now left.
    const bool sideValid = entryFirstTurn == TurnDirection::Left
        ? d.rightWallValid : d.leftWallValid;
    const float sideCm = entryFirstTurn == TurnDirection::Left
        ? d.rightWallCm : d.leftWallCm;
    if (!d.frontValid || !sideValid) {
      logEntryDecision(F("STOP; invalid front or original-wall side reading"));
      stopMotion(); return;
    }
    const float frontCm = min(d.frontCm,
        d.frontRawCm + ULTRASONIC_FRONT_FORWARD_OFFSET_CM);
    Serial.print(F("ENTRY match front=")); Serial.print(frontCm, 1);
    Serial.print(F("cm old-forward side="));
    Serial.print(entryFirstTurn == TurnDirection::Left ? F("RIGHT ") : F("LEFT "));
    Serial.print(sideCm, 1);
    Serial.print(F("cm tolerance_cm=")); Serial.println(STAGE1_EQUAL_TOLERANCE_CM, 1);
    const float errorCm = frontCm - sideCm;
    // Stop at equality or its crossing: sampling can skip over the exact value.
    const bool crossed = entryMatchInitialized && entryPreviousMatchErrorCm > 0 && errorCm < 0;
    if (fabs(errorCm) <= STAGE1_EQUAL_TOLERANCE_CM || crossed) {
      logEntryDecision(crossed ? F("STOP; equality crossed; hand off to STAGE 2")
                             : F("STOP; distances equal within tolerance; hand off to STAGE 2"));
      current = STAGE_FUNNEL;
      Serial.println(F("STAGE 1 complete -> STAGE 2: funnel; turn back 45 deg, then center."));
      startEntryTurn(opposite(entryFirstTurn), 45.0f, EntryMode::Align45);
    } else if (errorCm < 0) {
      // Already past equality at the first measurement: forward cannot fix it.
      Serial.println(F("ENTRY front already shorter than side -> STOP; reposition."));
      stopStage();
    } else {
      logEntryDecision(F("DRIVE_FORWARD; front greater than original-wall side"));
      entryMatchInitialized = true;
      entryPreviousMatchErrorCm = errorCm;
      driveContinuous(STAGE1_APPROACH_PWM, 0, MOTION_STRAIGHT_BIAS);
    }
    return;
  }

  if (entryMode == EntryMode::CenterDrive) {
    if (!d.frontValid || !d.leftWallValid || !d.rightWallValid) {
      logEntryDecision(F("STOP; invalid front or side reading"));
      stopMotion();
      return;
    }
    const float errorCm = d.leftWallCm - d.rightWallCm;
    Serial.print(F("ENTRY center L=")); Serial.print(d.leftWallCm, 1);
    Serial.print(F("cm R=")); Serial.print(d.rightWallCm, 1);
    Serial.print(F("cm difference=")); Serial.print(errorCm, 1);
    Serial.print(F("cm tolerance_cm=")); Serial.println(STAGE2_EQUAL_TOLERANCE_CM, 1);
    if (fabs(errorCm) <= STAGE2_EQUAL_TOLERANCE_CM) {
      logEntryDecision(F("DRIVE_FORWARD; sides equal within tolerance"));
      driveContinuous(STAGE2_DRIVE_PWM, 0, MOTION_STRAIGHT_BIAS);
      return;
    }
    // Move away from the nearer wall, toward the side with more clearance.
    entryCorrectionTurn = errorCm > 0 ? TurnDirection::Left : TurnDirection::Right;
    Serial.println(entryCorrectionTurn == TurnDirection::Left
        ? F("ENTRY correction LEFT: 45 deg / 5 cm / 45 deg back")
        : F("ENTRY correction RIGHT: 45 deg / 5 cm / 45 deg back"));
    startEntryTurn(entryCorrectionTurn, 45.0f, EntryMode::CorrectionOut);
    return;
  }

  // These phases own one timed movement; never overwrite it with live drive.
  if (motionBusy()) {
    logEntryDecision(F("CONTINUE_TIMED_MOTION; monitor front safety"));
    return;
  }
  logEntryDecision(F("TIMED_MOTION_FINISHED; advance maneuver"));
  switch (entryMode) {
    case EntryMode::Turning:
      entryMatchInitialized = false;
      settleEntryReadings(EntryMode::SeekEquality);
      break;
    case EntryMode::Align45:
    case EntryMode::CorrectionBack:
      settleEntryReadings(EntryMode::CenterDrive);
      break;
    case EntryMode::CorrectionOut:
      Serial.print(F("ENTRY drive_distance_cm=")); Serial.println(STAGE2_CORRECTION_DISTANCE_CM, 1);
      if (!driveStageCm(STAGE2_CORRECTION_DISTANCE_CM)) {
        Serial.println(F("ENTRY correction drive rejected -> STOP"));
        stopStage();
      } else {
        entryMode = EntryMode::CorrectionDrive;
      }
      break;
    case EntryMode::CorrectionDrive:
      startEntryTurn(opposite(entryCorrectionTurn), 45.0f, EntryMode::CorrectionBack);
      break;
    default: break;
  }
}

// Stage 3 derivative-term state (approach rate of the right-wall distance).
bool wallDerivInit = false;
float wallPrevErrorMm = 0.0f;
unsigned long wallPrevDerivMs = 0;
float wallDErrFilt = 0.0f; // low-passed dError, mm/s
// Slew-limiter state: last steer command actually sent and when.
int lastSteerCmd = 0;
unsigned long lastSteerMs = 0;

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
  lastSteerCmd = 0;
  lastSteerMs = millis();
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
    lastSteerCmd = -WALL_SEARCH_STEER; lastSteerMs = now;
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
  lastSteerCmd = -WALL_SEARCH_STEER; lastSteerMs = now;
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
    const float raw = constrain((errorMm - wallPrevErrorMm) / dt,
                                -WALL_FOLLOW_DERR_MAX_MMPS, WALL_FOLLOW_DERR_MAX_MMPS);
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
  // Dead-zone compensation only while actually correcting distance, so the
  // robot does not dither around the target from D-term noise.
  const bool correcting = fabs(errorMm) > WALL_FOLLOW_DEADBAND_MM;
  const int pdCmd = correcting ? compensateDeadzone(-corr, STEER_DEADZONE_COMP) : -corr;

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
  const int targetCmd = constrain(cornerAway > 0 ? max(pdCmd, cornerAway) : pdCmd,
                                  -WALL_FOLLOW_MAX_STEER, WALL_FOLLOW_MAX_STEER);
  // Slew limit: no bang-bang flips between +-max within a loop or two.
  const int steerCmd = slewLimit(lastSteerCmd, targetCmd, WALL_FOLLOW_STEER_SLEW_PER_S,
                                 (now - lastSteerMs) / 1000.0f);
  lastSteerCmd = steerCmd;
  lastSteerMs = now;

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
    Serial.print(F("cm tgt=")); Serial.print(targetCmd);
    Serial.print(F(" pd=")); Serial.print(pdCmd);
    Serial.print(F(" crn=")); Serial.println(cornerAway));
}
} // namespace

void startStage(uint8_t stage) {
  stopMotion();
  setStageSensorLogging(stage == STAGE_OPEN || stage == STAGE_FUNNEL);
  emergencyLatched = false;
  if (stage == STAGE_OPEN) {
    current = STAGE_OPEN;
    entryMode = EntryMode::Approach;
    Serial.print(F("STAGE 1: approach wall @")); Serial.print(STAGE1_WALL_TARGET_CM, 1);
    Serial.println(F(" cm, turn 90, match front/side -> Stage 2. S/X stops."));
    Serial.println(F("ENTRY full logging ON: every sensor update and decision. USnew marks fresh ultrasonic samples."));
  } else if (stage == STAGE_FUNNEL) {
    current = STAGE_FUNNEL;
    Serial.println(F("STAGE 2: funnel centering; start already aligned along funnel. S/X stops."));
    Serial.println(F("ENTRY full logging ON: every sensor update and decision. USnew marks fresh ultrasonic samples."));
    settleEntryReadings(EntryMode::CenterDrive);
  } else if (stage == STAGE_WALL) {
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
  setStageSensorLogging(false);
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
  const bool entryStage = current == STAGE_OPEN || current == STAGE_FUNNEL;
  const float safetyFrontCm = entryStage
      ? min(d.frontCm, d.frontRawCm + ULTRASONIC_FRONT_FORWARD_OFFSET_CM)
      : d.frontCm;
  if (d.frontValid && safetyFrontCm <= FRONT_EMERGENCY_STOP_CM) {
    stopMotion();
    // A cancelled timed turn cannot be resumed or counted as a completed turn.
    if (entryStage) {
      logEntryDecision(F("STOP_AND_CANCEL; front at/below emergency threshold"));
      Serial.print(F("ENTRY safety front_used_cm=")); Serial.print(safetyFrontCm, 1);
      Serial.print(F(" threshold_cm=")); Serial.println(FRONT_EMERGENCY_STOP_CM, 1);
      Serial.println(F("ENTRY front safety stop -> stage cancelled. Reposition before restarting 1/2."));
      stopStage();
      return;
    }
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
    case STAGE_OPEN: runEntryStages(d); break;
    case STAGE_FUNNEL: runEntryStages(d); break;
    case STAGE_WALL: runWallFollow(d); break;
    default: stopStage(); break;
  }
}
