#pragma once
#include <math.h>
#include <stdint.h>

struct LineFit {
  uint16_t n = 0;
  float x = 0, y = 0, xx = 0, xy = 0, yy = 0, first = 0, last = 0;
  void clear() { *this = LineFit(); }
  void add(float t, float value) {
    if (!n) first = t;
    last = t;
    ++n;
    x += t; y += value; xx += t*t; xy += t*value; yy += value*value;
  }
  float slope() const {
    const float d = n*xx - x*x;
    return d > 0.000001f ? (n*xy - x*y)/d : 0;
  }
  float r2() const {
    const float d = (n*xx - x*x)*(n*yy - y*y);
    const float c = n*xy - x*y;
    // Constant heading has useful zero slope, but undefined R2 (reported as 0).
    if (d <= 0.000001f) return 0;
    const float result = c*c/d;
    return result > 1 ? 1 : result;
  }
  bool enough() const { return n >= 5 && last-first >= 0.5f; }
};

struct RangeSample {
  float cm = 0;
  unsigned long at = 0;
  bool valid = false;
};

struct WallObservation {
  float front = 0, right = 0, heading = 0, normal = 0;
  unsigned long at = 0;
  bool valid = false;
};

struct PoseAverage {
  LineFit front, right;
  float frontMin = 1000, frontMax = 0, rightMin = 1000, rightMax = 0;
  void clear() { *this = PoseAverage(); }
  void add(float f, float r) {
    front.add(front.n, f); right.add(right.n, r);
    if (f < frontMin) frontMin = f;
    if (f > frontMax) frontMax = f;
    if (r < rightMin) rightMin = r;
    if (r > rightMax) rightMax = r;
  }
  float f() const { return front.y/front.n; }
  float r() const { return right.y/right.n; }
  bool stable() const {
    return front.n >= 8 && frontMax-frontMin <= 2 && rightMax-rightMin <= 2;
  }
};

// Two positions, both square to the SAME wall:
// front = right*cos(alpha) + forward sensor separation.
// Differencing cancels that separation and constant range offsets.
inline bool estimateMount(float f1, float r1, float f2, float r2,
                          float &angleDeg, float &forwardCm) {
  const float df = f2-f1, dr = r2-r1;
  if (fabsf(df) < 25 || fabsf(dr) < 25) return false;
  const float cosine = df/dr;
  if (cosine < 0.5f || cosine > 0.8660254f) return false; // 30..60 deg
  angleDeg = acosf(cosine)*180.0f/3.14159265359f;
  forwardCm = ((f1-r1*cosine)+(f2-r2*cosine))*0.5f;
  return isfinite(forwardCm) && fabsf(forwardCm) <= 30;
}

// Coordinates relative to front transducer: x forward, y right.
// Positive heading = robot turned LEFT relative to the wall normal.
inline bool wallGeometry(float front, float right, float angleDeg,
                         float rightForwardCm, float rightLateralCm,
                         float &headingDeg, float &normalCm) {
  const float a = angleDeg*3.14159265359f/180.0f;
  const float denominator = rightLateralCm + right*sinf(a);
  if (denominator <= 1) return false;
  const float h = atan2f(front-rightForwardCm-right*cosf(a), denominator);
  headingDeg = h*180.0f/3.14159265359f;
  normalCm = front*cosf(h);
  return isfinite(headingDeg) && isfinite(normalCm);
}
