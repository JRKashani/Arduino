#ifndef IMU_CAPSULE_H
#define IMU_CAPSULE_H

#include <ADXL335_IMU.h>

bool setupImuCapsule();
void runImuCapsule();
const ADXL335Reading &latestImuReading();

#endif
