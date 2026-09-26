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

int steerFromPD(float error, float dError, float kp, float kd,
                int maxOutput, float deadband) {
  const bool inBand = (error <= deadband && error >= -deadband);
  const float p = inBand ? 0.0f : kp * error;
  const float out = p + kd * dError;
  long rounded = (long)(out >= 0.0f ? out + 0.5f : out - 0.5f);
  return clampInt(rounded, -maxOutput, maxOutput);
}

int scaleSpeedBySteer(int baseSpeed, int minSpeed, int steer, int maxSteer) {
  if (maxSteer <= 0) return baseSpeed;
  int mag = steer < 0 ? -steer : steer;
  if (mag > maxSteer) mag = maxSteer;
  const float speed = baseSpeed - (float)(baseSpeed - minSpeed) * mag / maxSteer;
  return (int)(speed + 0.5f);
}

int rampByProximity(float distance, float startDist, float fullDist, int maxOutput) {
  if (distance >= startDist) return 0;
  if (distance <= fullDist || startDist <= fullDist) return maxOutput;
  const float frac = (startDist - distance) / (startDist - fullDist);
  return (int)(frac * maxOutput + 0.5f);
}
