#include "ManualDrive.h"
#include "../DEFINE.h"
#include "../MotionCapsule.h"

namespace {
int requestedSpeed = 0, requestedSteer = 0;
unsigned long lastCommandMs = 0;
ManualDriveStatus status = {0, 0, false, true};

void event(const __FlashStringHelper *name) {
  Serial.print(F("EVT,")); Serial.print(millis());
  Serial.print(','); Serial.println(name);
}

void clearRequest() { requestedSpeed = requestedSteer = 0; }

void checkWatchdog() {
  const bool moving = requestedSpeed != 0 || requestedSteer != 0;
  if (moving && watchdogExpired(millis(), lastCommandMs, MANUAL_WATCHDOG_MS)) {
    clearRequest();
    event(F("WATCHDOG_STOP"));
  }
}

void updateBlocked(const SensorData &d) {
  const bool blocked = frontBlocksForward(status.safetyOn, requestedSpeed,
                                          d.frontValid, d.frontCm, FRONT_EMERGENCY_STOP_CM);
  if (blocked != status.blocked) event(blocked ? F("SAFETY_BLOCK") : F("SAFETY_CLEAR"));
  status.blocked = blocked;
}

void applyMotors() {
  status.speed = status.blocked ? 0 : requestedSpeed;
  status.steer = requestedSteer;
  if (status.speed == 0 && status.steer == 0) stopMotion();
  else driveContinuous(status.speed, status.steer);
}
} // namespace

void setupManualDrive() {
  clearRequest();
  status = {0, 0, false, true};
  stopMotion();
}

void handleManualCommand(const ManualCommand &cmd) {
  switch (cmd.type) {
    case ManualCommandType::Drive:
      requestedSpeed = cmd.speed;
      requestedSteer = cmd.steer;
      lastCommandMs = millis();
      break;
    case ManualCommandType::Stop: clearRequest(); event(F("STOP")); break;
    case ManualCommandType::SafetyOn: status.safetyOn = true; event(F("SAFETY_ON")); break;
    case ManualCommandType::SafetyOff: status.safetyOn = false; event(F("SAFETY_OFF")); break;
    default: break;
  }
}

void updateManualDrive(const SensorData &d) {
  checkWatchdog();
  updateBlocked(d);
  applyMotors();
}

const ManualDriveStatus &manualDriveStatus() { return status; }
