// Host tests for lib/ManualCommand (protocol parsing and drive rules).
#include <cstdio>
#include "ManualCommand.h"

static int failures = 0;
#define CHECK(cond) do { if (!(cond)) { std::printf("FAIL %s:%d %s\n", __FILE__, __LINE__, #cond); ++failures; } } while (0)

static ManualCommand parse(const char *line) { return parseManualCommand(line, 120, 80); }

static void testDrive() {
  ManualCommand c = parse("D,60,-15");
  CHECK(c.type == ManualCommandType::Drive && c.speed == 60 && c.steer == -15);
  c = parse("D,-40,0");
  CHECK(c.type == ManualCommandType::Drive && c.speed == -40 && c.steer == 0);
}

static void testClamp() {
  ManualCommand c = parse("D,500,-999");
  CHECK(c.type == ManualCommandType::Drive && c.speed == 120 && c.steer == -80);
}

static void testMalformed() {
  const char *bad[] = {"", "D", "D,", "D,1", "D,1,", "D,,1", "D,1,2,3", "D,a,1",
                       "D,1,2x", "D,1234567,0", "stop", "SAFETY", "SAFETY,on", "X"};
  for (const char *line : bad) CHECK(parse(line).type == ManualCommandType::Invalid);
  CHECK(parseManualCommand(nullptr, 120, 80).type == ManualCommandType::Invalid);
}

static void testKeywords() {
  CHECK(parse("STOP").type == ManualCommandType::Stop);
  CHECK(parse("SAFETY,ON").type == ManualCommandType::SafetyOn);
  CHECK(parse("SAFETY,OFF").type == ManualCommandType::SafetyOff);
  CHECK(parse("INFO").type == ManualCommandType::Info);
}

static void testSafety() {
  CHECK(frontBlocksForward(true, 50, true, 20.0f, 20.0f));
  CHECK(!frontBlocksForward(true, 50, true, 20.1f, 20.0f));
  CHECK(!frontBlocksForward(false, 50, true, 10.0f, 20.0f)); // safety off
  CHECK(!frontBlocksForward(true, 0, true, 10.0f, 20.0f));   // spin in place
  CHECK(!frontBlocksForward(true, -40, true, 10.0f, 20.0f)); // reverse
  CHECK(!frontBlocksForward(true, 50, false, 10.0f, 20.0f)); // invalid front
}

static void testWatchdog() {
  CHECK(!watchdogExpired(1599, 1000, 600));
  CHECK(watchdogExpired(1600, 1000, 600));
  CHECK(!watchdogExpired(100, 0xFFFFFF00UL, 600)); // millis() wrap
  CHECK(watchdogExpired(400, 0xFFFFFF00UL, 600));
}

int main() {
  testDrive(); testClamp(); testMalformed(); testKeywords(); testSafety(); testWatchdog();
  std::printf(failures ? "%d FAILED\n" : "ALL PASSED\n", failures);
  return failures ? 1 : 0;
}
