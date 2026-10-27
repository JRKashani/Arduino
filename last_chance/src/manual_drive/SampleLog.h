#ifndef SAMPLE_LOG_H
#define SAMPLE_LOG_H

#include <ADXL335_IMU.h>
#include "../SensorsCapsule.h"
#include "ManualDrive.h"

// Column names of the S record, in order (also sent as the FIELDS line).
extern const char SAMPLE_FIELDS[];

// One S record per sensor update: command, distances, LDRs, IMU.
void printSample(const SensorData &d, const ADXL335Reading &imu,
                 const ManualDriveStatus &drive);

#endif
