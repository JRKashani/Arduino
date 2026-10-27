#ifndef MANUAL_COMMAND_H
#define MANUAL_COMMAND_H

// Line protocol for the manual drive logger (PC -> robot), plus the pure
// drive rules. Arduino-free so it can be unit-tested on the PC.
//
//   D,<speed>,<steer>   drive; also the heartbeat (positive steer = left)
//   STOP                stop motors
//   SAFETY,ON|OFF       front safety stop for forward motion
//   INFO                print firmware identity and configuration

enum class ManualCommandType { Drive, Stop, SafetyOn, SafetyOff, Info, Invalid };

struct ManualCommand {
  ManualCommandType type;
  int speed; // clamped to +-maxSpeed (Drive only)
  int steer; // clamped to +-maxSteer (Drive only)
};

// Parse one line without its newline. Out-of-range numbers are clamped;
// malformed text returns Invalid.
ManualCommand parseManualCommand(const char *line, int maxSpeed, int maxSteer);

// Forward motion is blocked while the front is valid and at/below the stop
// distance. Turning in place and reversing stay allowed.
bool frontBlocksForward(bool safetyOn, int speed, bool frontValid,
                        float frontCm, float stopCm);

// True once the PC has been silent for at least timeoutMs (wrap-safe).
bool watchdogExpired(unsigned long nowMs, unsigned long lastCommandMs,
                     unsigned long timeoutMs);

#endif
