#include <unity.h>
#include "Control.h"

// --- PD tests: kp 0.08, kd 0.05, maxOutput 30, deadband 15 mm ---

void test_pd_equals_p_when_no_rate(void) {
  // 0.08 * 200 = 16, dError = 0
  TEST_ASSERT_EQUAL_INT(16, steerFromPD(200.0f, 0.0f, 0.08f, 0.05f, 30, 15.0f));
}

void test_pd_derivative_acts_inside_deadband(void) {
  // Inside distance deadband (P=0) but closing at -100 mm/s: D = 0.05*-100 = -5
  TEST_ASSERT_EQUAL_INT(-5, steerFromPD(5.0f, -100.0f, 0.08f, 0.05f, 30, 15.0f));
}

void test_pd_damps_when_approaching(void) {
  // Too far (P=+16) but closing fast (-200 mm/s => D=-10): net 6, less aggressive
  TEST_ASSERT_EQUAL_INT(6, steerFromPD(200.0f, -200.0f, 0.08f, 0.05f, 30, 15.0f));
}

void test_pd_clamp(void) {
  TEST_ASSERT_EQUAL_INT(30, steerFromPD(2000.0f, 2000.0f, 0.08f, 0.05f, 30, 15.0f));
}

// --- Speed scheduling: base 50, min 30, maxSteer 25 ---

void test_speed_full_when_straight(void) {
  TEST_ASSERT_EQUAL_INT(50, scaleSpeedBySteer(50, 30, 0, 25));
}

void test_speed_min_at_max_steer_either_sign(void) {
  TEST_ASSERT_EQUAL_INT(30, scaleSpeedBySteer(50, 30, 25, 25));
  TEST_ASSERT_EQUAL_INT(30, scaleSpeedBySteer(50, 30, -25, 25));
  TEST_ASSERT_EQUAL_INT(30, scaleSpeedBySteer(50, 30, 40, 25)); // beyond clamp
}

void test_speed_linear_midpoint(void) {
  // half steer => halfway between 50 and 30
  TEST_ASSERT_EQUAL_INT(40, scaleSpeedBySteer(50, 30, 12, 24));
}

// --- Proximity ramp: start 60, full 30, max 25 ---

void test_ramp_zero_when_far(void) {
  TEST_ASSERT_EQUAL_INT(0, rampByProximity(60.0f, 60.0f, 30.0f, 25));
  TEST_ASSERT_EQUAL_INT(0, rampByProximity(120.0f, 60.0f, 30.0f, 25));
}

void test_ramp_max_when_close(void) {
  TEST_ASSERT_EQUAL_INT(25, rampByProximity(30.0f, 60.0f, 30.0f, 25));
  TEST_ASSERT_EQUAL_INT(25, rampByProximity(10.0f, 60.0f, 30.0f, 25));
}

void test_ramp_linear_midpoint(void) {
  // 45 cm is halfway => 12.5 -> 13
  TEST_ASSERT_EQUAL_INT(13, rampByProximity(45.0f, 60.0f, 30.0f, 25));
}

// --- Slew limiter ---

void test_slew_limits_step(void) {
  // 150/s over 0.1 s => step 15
  TEST_ASSERT_EQUAL_INT(15, slewLimit(0, 25, 150.0f, 0.1f));
  TEST_ASSERT_EQUAL_INT(-15, slewLimit(0, -25, 150.0f, 0.1f));
}

void test_slew_reaches_close_target(void) {
  TEST_ASSERT_EQUAL_INT(10, slewLimit(0, 10, 150.0f, 0.1f));
}

void test_slew_min_one_step(void) {
  TEST_ASSERT_EQUAL_INT(1, slewLimit(0, 25, 150.0f, 0.001f));
}

// --- Dead-zone compensation ---

void test_deadzone_comp(void) {
  TEST_ASSERT_EQUAL_INT(0, compensateDeadzone(0, 4));
  TEST_ASSERT_EQUAL_INT(7, compensateDeadzone(3, 4));
  TEST_ASSERT_EQUAL_INT(-7, compensateDeadzone(-3, 4));
}

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_pd_equals_p_when_no_rate);
  RUN_TEST(test_pd_derivative_acts_inside_deadband);
  RUN_TEST(test_pd_damps_when_approaching);
  RUN_TEST(test_pd_clamp);
  RUN_TEST(test_speed_full_when_straight);
  RUN_TEST(test_speed_min_at_max_steer_either_sign);
  RUN_TEST(test_speed_linear_midpoint);
  RUN_TEST(test_ramp_zero_when_far);
  RUN_TEST(test_ramp_max_when_close);
  RUN_TEST(test_ramp_linear_midpoint);
  RUN_TEST(test_slew_limits_step);
  RUN_TEST(test_slew_reaches_close_target);
  RUN_TEST(test_slew_min_one_step);
  RUN_TEST(test_deadzone_comp);
  return UNITY_END();
}
