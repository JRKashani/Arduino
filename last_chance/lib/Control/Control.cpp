#include "Control.h"

namespace {
int clampInt(long value, int lo, int hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return (int)value;
}
} // namespace

int steerFromError(float error, float gain, int maxOutput, float deadband) {
  if (error <= deadband && error >= -deadband) return 0;
  float out = gain * error;
  long rounded = (long)(out >= 0.0f ? out + 0.5f : out - 0.5f);
  return clampInt(rounded, -maxOutput, maxOutput);
}
