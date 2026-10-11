#pragma once
#include <stdint.h>
#include <cassert>
extern uint16_t laserValue;
extern uint8_t laserStatus;
extern bool laserInitOk;
class VL53L0X {
public:
  enum vcselPeriodType { VcselPeriodPreRange, VcselPeriodFinalRange };
  static const uint8_t RESULT_RANGE_STATUS=0x14;
  uint8_t last_status=0;
  void setAddress(uint8_t) {}
  void setTimeout(uint16_t) {}
  bool init() { return laserInitOk; }
  bool setSignalRateLimit(float value) { assert(value == 0.1f); return true; }
  bool setVcselPulsePeriod(vcselPeriodType type, uint8_t value) {
    assert(value == (type == VcselPeriodPreRange ? 18 : 14)); return true;
  }
  bool setMeasurementTimingBudget(uint32_t value) { assert(value == 33000); return true; }
  uint16_t readRangeSingleMillimeters() { return laserValue; }
  bool timeoutOccurred() { return false; }
  uint8_t readReg(uint8_t) { return laserStatus << 3; }
};
