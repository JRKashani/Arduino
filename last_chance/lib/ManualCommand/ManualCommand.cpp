#include "ManualCommand.h"

#include <stdlib.h>
#include <string.h>

namespace {
const ManualCommand INVALID = {ManualCommandType::Invalid, 0, 0};

int clampInt(long value, int limit) {
  if (value > limit) return limit;
  if (value < -limit) return -limit;
  return (int)value;
}

// Parse a signed decimal integer that fills [text, end); false otherwise.
bool parseInt(const char *text, const char *end, long &value) {
  if (text == end || end - text > 6) return false;
  char *stop = 0;
  value = strtol(text, &stop, 10);
  return stop == end;
}

ManualCommand parseDrive(const char *args, int maxSpeed, int maxSteer) {
  const char *comma = strchr(args, ',');
  if (!comma || strchr(comma + 1, ',')) return INVALID;
  long speed = 0, steer = 0;
  if (!parseInt(args, comma, speed) ||
      !parseInt(comma + 1, comma + 1 + strlen(comma + 1), steer)) return INVALID;
  ManualCommand cmd = {ManualCommandType::Drive,
                       clampInt(speed, maxSpeed), clampInt(steer, maxSteer)};
  return cmd;
}
} // namespace

ManualCommand parseManualCommand(const char *line, int maxSpeed, int maxSteer) {
  if (!line) return INVALID;
  if (!strncmp(line, "D,", 2)) return parseDrive(line + 2, maxSpeed, maxSteer);
  ManualCommand cmd = {ManualCommandType::Invalid, 0, 0};
  if (!strcmp(line, "STOP")) cmd.type = ManualCommandType::Stop;
  else if (!strcmp(line, "SAFETY,ON")) cmd.type = ManualCommandType::SafetyOn;
  else if (!strcmp(line, "SAFETY,OFF")) cmd.type = ManualCommandType::SafetyOff;
  else if (!strcmp(line, "INFO")) cmd.type = ManualCommandType::Info;
  return cmd;
}

bool frontBlocksForward(bool safetyOn, int speed, bool frontValid,
                        float frontCm, float stopCm) {
  return safetyOn && speed > 0 && frontValid && frontCm <= stopCm;
}

bool watchdogExpired(unsigned long nowMs, unsigned long lastCommandMs,
                     unsigned long timeoutMs) {
  return nowMs - lastCommandMs >= timeoutMs;
}
