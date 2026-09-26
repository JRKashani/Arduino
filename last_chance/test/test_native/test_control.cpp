#include <unity.h>
#include "Control.h"

// gain 0.05, maxOutput 30, deadband 15 mm (Stage-3-like settings).

void test_zero_within_deadband(void) {
  TEST_ASSERT_EQUAL_INT(0, steerFromError(10.0f, 0.05f, 30, 15.0f));
  TEST_ASSERT_EQUAL_INT(0, steerFromError(-15.0f, 0.05f, 30, 15.0f));
}

void test_proportional_positive(void) {
  // 0.05 * 200 = 10
  TEST_ASSERT_EQUAL_INT(10, steerFromError(200.0f, 0.05f, 30, 15.0f));
}

void test_proportional_negative(void) {
  TEST_ASSERT_EQUAL_INT(-10, steerFromError(-200.0f, 0.05f, 30, 15.0f));
}

void test_clamp(void) {
  TEST_ASSERT_EQUAL_INT(30, steerFromError(5000.0f, 0.05f, 30, 15.0f));
  TEST_ASSERT_EQUAL_INT(-30, steerFromError(-5000.0f, 0.05f, 30, 15.0f));
}

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

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_zero_within_deadband);
  RUN_TEST(test_proportional_positive);
  RUN_TEST(test_proportional_negative);
  RUN_TEST(test_clamp);
  RUN_TEST(test_pd_equals_p_when_no_rate);
  RUN_TEST(test_pd_derivative_acts_inside_deadband);
  RUN_TEST(test_pd_damps_when_approaching);
  RUN_TEST(test_pd_clamp);
  return UNITY_END();
}
