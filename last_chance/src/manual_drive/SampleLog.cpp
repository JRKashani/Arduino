#include "SampleLog.h"
#include <Arduino.h>

const char SAMPLE_FIELDS[] =
    "t_ms,speed,steer,blocked,"
    "l_cm,l_ok,l_raw_mm,r_cm,r_ok,r_raw_mm,"
    "f_cm,f_ok,f_sample_cm,d_cm,d_ok,d_sample_cm,us_new,"
    "ldr_l,ldr_r,imu_ok,roll_deg,pitch_deg,ax_g,ay_g,az_g";

namespace {
void field(long value) { Serial.print(','); Serial.print(value); }
void field(float value, uint8_t digits) { Serial.print(','); Serial.print(value, digits); }
void flag(bool value) { Serial.print(value ? F(",1") : F(",0")); }

void printDrive(const ManualDriveStatus &drive) {
  field((long)drive.speed); field((long)drive.steer); flag(drive.blocked);
}

void printLasers(const SensorData &d) {
  field(d.leftWallCm, 1); flag(d.leftWallValid); field((long)d.leftRawMm);
  field(d.rightWallCm, 1); flag(d.rightWallValid); field((long)d.rightRawMm);
}

void printUltrasonics(const SensorData &d) {
  field(d.frontCm, 1); flag(d.frontValid); field(d.frontRawCm, 1);
  field(d.rightDiagCm, 1); flag(d.rightDiagValid); field(d.rightDiagRawCm, 1);
  Serial.print(d.frontFresh ? F(",F") : (d.rightDiagFresh ? F(",D") : F(",N")));
}

void printImu(const ADXL335Reading &imu) {
  flag(imu.valid);
  field(imu.robotRollDeg, 2); field(imu.robotPitchDeg, 2);
  field(imu.ax, 3); field(imu.ay, 3); field(imu.az, 3);
}
} // namespace

void printSample(const SensorData &d, const ADXL335Reading &imu,
                 const ManualDriveStatus &drive) {
  Serial.print(F("S,")); Serial.print(d.timestampMs);
  printDrive(drive);
  printLasers(d);
  printUltrasonics(d);
  field((long)d.ldrLeft); field((long)d.ldrRight);
  printImu(imu);
  Serial.println();
}
