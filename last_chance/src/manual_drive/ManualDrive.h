#ifndef MANUAL_DRIVE_H
#define MANUAL_DRIVE_H

#include <ManualCommand.h>
#include "../SensorsCapsule.h"

// What the motors are actually doing this loop (after watchdog and safety).
struct ManualDriveStatus {
  int speed;      // applied speed (0 while blocked)
  int steer;      // applied steer, positive = left
  bool blocked;   // forward motion held by the front safety stop
  bool safetyOn;
};

void setupManualDrive();
// Apply a parsed Drive/Stop/Safety command. Drive refreshes the watchdog.
void handleManualCommand(const ManualCommand &cmd);
// Watchdog + front safety, then command the motors. Call every loop.
void updateManualDrive(const SensorData &d);
const ManualDriveStatus &manualDriveStatus();

#endif
