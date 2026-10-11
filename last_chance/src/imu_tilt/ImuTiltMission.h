#ifndef IMU_TILT_MISSION_H
#define IMU_TILT_MISSION_H

#include <ADXL335_IMU.h>

// Standalone stationary experiment. All motor outputs are held low.
// No automatic baseline: raw and robot angles are absolute at every boot.
class ImuTiltMission {
public:
  enum class State { Idle, Settle, Capture, Review };
  ImuTiltMission();
  bool begin();
  bool startTrial(unsigned long id, unsigned long settleMs,
                  unsigned long durationMs, unsigned long periodUs);
  void update();
  void abort();
  State status() const { return state_; }

private:
  ADXL335_IMU imu_;
  State state_ = State::Idle;
  bool ready_ = false;
  char input_[96] = {};
  uint8_t inputLength_ = 0;
  bool overflow_ = false;
  unsigned long id_ = 0, settleStartMs_ = 0, settleMs_ = 0;
  unsigned long captureStartUs_ = 0, durationUs_ = 0, periodUs_ = 20000;
  unsigned long nextUs_ = 0, sequence_ = 0, samples_ = 0, missed_ = 0;
  void command();
  void info();
  void finish(bool aborted);
  void sample(unsigned long elapsedUs);
  static void stopMotors();
};

#endif
