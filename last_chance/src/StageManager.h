#ifndef STAGE_MANAGER_H
#define STAGE_MANAGER_H

#include <Arduino.h>

/* Mission stages, numbered to match the track segments and serial commands. */
enum Stage : uint8_t {
  STAGE_NONE = 0,
  STAGE_OPEN = 1,
  STAGE_FUNNEL = 2,
  STAGE_WALL = 3,
  STAGE_WADI = 4,
  STAGE_LIGHT = 5
};

/* Start a stage, stopping any current motion first. */
void startStage(uint8_t stage);
void stopStage();
bool stageRunning();

/* Run the active stage controller once per loop. */
void runStageManager();

#endif  // STAGE_MANAGER_H
