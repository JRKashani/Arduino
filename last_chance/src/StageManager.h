#ifndef STAGE_MANAGER_H
#define STAGE_MANAGER_H

#include <Arduino.h>

// Mission stages, numbered to match the track segments and the serial commands.
enum Stage : uint8_t {
  STAGE_NONE   = 0,
  STAGE_OPEN   = 1, // forward drive in open space toward the funnel
  STAGE_FUNNEL = 2, // centered pass through the funnel
  STAGE_WALL   = 3, // follow the right wall at 30 cm  (implemented)
  STAGE_WADI   = 4, // centered pass through the sloped wadi (IMU roll)
  STAGE_LIGHT  = 5  // drive to the light, stop 12 cm from the pole
};

// Begin running a stage; stops any current motion first. Unknown/unimplemented
// stages print a notice and do not start.
void startStage(uint8_t stage);
void stopStage();
bool stageRunning();

// Call every loop. When a stage is active: enforces the front-distance safety
// stop, then drives the active stage's closed-loop controller. No-op otherwise.
void runStageManager();

#endif
