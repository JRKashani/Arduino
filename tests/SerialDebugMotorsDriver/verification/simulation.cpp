// Executes the actual sketch and local motor libraries against synthetic echoes.
// No robot or serial port is accessed. See run.ps1.
#include <Arduino.h>
#include <assert.h>
#include <iostream>
#include <limits.h>
#include "../SerialDebugMotorsDriver.ino"

FakeSerial Serial;
unsigned long fakeTime = 0;
int pwm[64] = {};
float wallCm = 100, yawDeg = 0, simSpeed = 20, simYawRate = 0;
float actualAngle = 45, actualForward = 3;
int missingPin = -1;
bool forceFront = false;
float forcedFrontCm = 100;

unsigned long millis() { return fakeTime; }
void pinMode(int, int) {}
void digitalWrite(int, int) {}
void analogWrite(int pin, int value) { pwm[pin] = value; }
void delayMicroseconds(unsigned int) {}
bool moving() { return pwm[LEFT_PWM] != 0 || pwm[RIGHT_PWM] != 0; }
void advance(unsigned long ms) {
  if (moving()) { wallCm -= simSpeed*ms/1000.0f; yawDeg += simYawRate*ms/1000.0f; }
  fakeTime += ms;
}
unsigned long pulseIn(int pin, int, unsigned long timeout) {
  if (pin == missingPin) { advance(timeout/1000); return 0; }
  const float h = yawDeg*3.14159265359f/180;
  const float a = actualAngle*3.14159265359f/180;
  float range = pin == FRONT_ECHO ? wallCm/cosf(h) :
      (wallCm-actualForward*cosf(h)-RIGHT_SENSOR_LATERAL_CM*sinf(h))/cosf(a-h);
  if (pin == FRONT_ECHO && forceFront) range = forcedFrontCm;
  const unsigned long duration = static_cast<unsigned long>((range-US_OFFSET_CM)/US_SCALE*58+0.5f);
  advance(duration/1000);
  return duration;
}
void tick(unsigned long ms) {
  const unsigned long start = fakeTime;
  while (fakeTime-start < ms) { loop(); advance(1); }
}
void command(const char *c) { Serial.input += c; loop(); }
void reset() {
  robot.stop(); robot = SteeringDualH();
  fakeTime = 0; wallCm = 100; yawDeg = 0; simSpeed = 20; simYawRate = 0;
  missingPin = -1; forceFront = false;
  mode = IDLE; mountCalibrated = firstPoseReady = monitor = false;
  testSpeed = 40; testBias = INITIAL_BIAS; turnAmount = 10;
  mountAngleDeg = 45; rightForwardCm = 0; trialNumber = 0;
  previousFront = middleRight = latestFront = latestRight = RangeSample();
  observation = WallObservation(); nextIsFront = true; lastPingEnd = lastMonitor = 0;
  Serial = FakeSerial(); setup(); tick(400);
}
void calibrate() {
  wallCm = 60; command("A\r\n"); tick(1600);
  assert(firstPoseReady && !mountCalibrated && !moving());
  wallCm = 100; command("Z\n"); tick(1600);
  assert(mountCalibrated && !moving());
  assert(fabsf(mountAngleDeg-actualAngle) < 0.1f);
  assert(fabsf(rightForwardCm-actualForward) < 0.1f);
}
void beginRun(const char *c = "V") {
  command(c); tick(3020);
  assert(mode == RUNNING && moving());
}
void contains(const char *s) {
  if (Serial.output.find(s) == std::string::npos) {
    std::cerr << "Missing output: " << s << '\n' << Serial.output; abort();
  }
}
void geometryTests() {
  for (float a : {32.0f, 45.0f, 58.0f}) {
    float estimated, x;
    const float cosine = cosf(a*3.14159265359f/180);
    assert(estimateMount(60, 57/cosine, 100, 97/cosine, estimated, x));
    assert(fabsf(estimated-a) < 0.001f && fabsf(x-3) < 0.001f);
    for (float h : {-12.0f, -5.0f, 0.0f, 5.0f, 12.0f}) {
      const float radians = h*3.14159265359f/180;
      const float front = 90/cosf(radians);
      const float right = (90-3*cosf(radians)-8*sinf(radians))/cosf((a-h)*3.14159265359f/180);
      float gotHeading, normal;
      assert(wallGeometry(front, right, a, 3, 8, gotHeading, normal));
      assert(fabsf(gotHeading-h) < 0.001f && fabsf(normal-90) < 0.001f);
    }
  }
  float a, x;
  assert(!estimateMount(60, 80, 65, 87, a, x));
  assert(!estimateMount(60, 80, 100, 80, a, x));
  assert(!estimateMount(60, 80, 100, 40, a, x));
  assert(inRange(20) && inRange(170) && !inRange(19.9f) && !inRange(170.1f));
  LineFit fit;
  for (int i = 0; i < 10; ++i) fit.add(i*0.13f, 2-20*i*0.13f);
  assert(fit.enough() && fabsf(fit.slope()+20) < 0.001f && fit.r2() > 0.999f);
}

int main() {
  geometryTests();
  for (const char *c : {"V", "T"}) {
    reset(); beginRun(c); tick(2200);
    assert(!moving() && mode == IDLE && !mountCalibrated);
    assert(mountAngleDeg == 45 && rightForwardCm == 0);
    assert(fabsf(-distanceFit.slope()*10-200) < 3);
    contains(",1,TIME_LIMIT"); contains("APPROXIMATE:"); contains("angle_measured=0");
  }
  reset(); command("L"); tick(4000); assert(!moving() && trialNumber == 0);
  contains("For steering trials");
  reset(); calibrate();
  beginRun(); tick(2200);
  assert(!moving() && mode == IDLE);
  assert(fabsf(-distanceFit.slope()*10-200) < 2);
  assert(fabsf(headingFit.slope()) < 0.1f); // aligned readings do not invent yaw
  contains(",1,TIME_LIMIT");

  for (const char *c : {"L", "R"}) {
    reset(); calibrate(); simYawRate = c[0] == 'L' ? 3.0f : -3.0f;
    beginRun(c); tick(1800);
    assert(!moving() && fabsf(headingFit.slope()-simYawRate) < 0.15f);
    contains(",1,TIME_LIMIT");
  }
  reset(); calibrate(); command("V"); tick(1000); command("S"); tick(4000);
  assert(!moving() && trialNumber == 0); contains("USER_STOP");

  reset(); calibrate(); beginRun(); command("SV"); tick(3500);
  assert(!moving() && trialNumber == 1); contains(",0,USER_STOP");

  for (int pin : {FRONT_ECHO, RIGHT_ECHO}) {
    reset(); calibrate(); beginRun(); missingPin = pin; tick(200);
    assert(!moving() && mode == IDLE); contains("ECHO_MISSING_OR_OUTSIDE_20_170");
  }
  for (float bad : {19.0f, 171.0f}) {
    reset(); calibrate(); beginRun(); forceFront = true; forcedFrontCm = bad; tick(200);
    assert(!moving()); contains(",0,FRONT_ECHO_MISSING_OR_OUTSIDE_20_170");
  }
  reset(); calibrate(); beginRun(); tick(900); forceFront = true; forcedFrontCm = 39;
  tick(200); assert(!moving()); contains("FRONT_40CM_MARGIN");

  reset(); calibrate(); beginRun(); advance(STALE_MS+1); serviceControl();
  assert(!moving()); contains("STALE_SENSORS");

  reset(); calibrate(); beginRun(); simYawRate = 40; tick(500);
  assert(!moving()); contains("HEADING_LIMIT");

  reset(); calibrate(); beginRun(); simSpeed = -20; tick(600);
  assert(!moving()); contains("MOVING_AWAY_OR_BAD_ECHO");

  reset(); calibrate(); simSpeed = 0; beginRun(); tick(2200);
  assert(!moving()); contains(",0,TIME_LIMIT");

  reset(); calibrate(); command("V"); tick(2900);
  latestFront.cm = 39; advance(101); serviceControl();
  assert(!moving() && trialNumber == 0); contains("START_REQUIRES");

  // millis rollover: establish fresh observations near wrap and cross it in run.
  reset(); calibrate(); fakeTime = ULONG_MAX-4000UL;
  lastPingEnd = fakeTime-QUIET_MS; tick(400);
  beginRun(); tick(2200); assert(!moving()); contains(",1,TIME_LIMIT");
  std::cout << "PASS: geometry, calibration, velocity, signed turns, command stop, "
               "echo limits, clearance, stale data, drift, reverse, stalled motor, "
               "start guard and millis rollover.\n";
}
