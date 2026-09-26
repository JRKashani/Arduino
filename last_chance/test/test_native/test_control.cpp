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

int main(int, char **) {
  UNITY_BEGIN();
  RUN_TEST(test_zero_within_deadband);
  RUN_TEST(test_proportional_positive);
  RUN_TEST(test_proportional_negative);
  RUN_TEST(test_clamp);
  return UNITY_END();
}
