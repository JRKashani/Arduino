#include "Control.h"

namespace {
int clampInt(long value, int lo, int hi) {
  if (value < lo) return lo;
  if (value > hi) return hi;
  return (int)value;
}
} // namespace

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

int slewLimit(int current, int target, float maxRatePerS, float dtS) {
  if (maxRatePerS <= 0.0f || dtS <= 0.0f) return current;
  int step = (int)(maxRatePerS * dtS + 0.5f);
  if (step < 1) step = 1;
  if (target > current + step) return current + step;
  if (target < current - step) return current - step;
  return target;
}

int compensateDeadzone(int cmd, int comp) {
  if (cmd > 0) return cmd + comp;
  if (cmd < 0) return cmd - comp;
  return 0;
}
