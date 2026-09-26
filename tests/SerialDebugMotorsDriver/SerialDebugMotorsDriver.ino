#include <Arduino.h>
#include <SteeringDualH.h>

// Manual calibration. Mega 2560, Serial Monitor 115200 baud.
// Established wiring; matches last_chance/DEFINE.h.
const uint8_t LEFT_DIR_1 = 46, LEFT_DIR_2 = 48, LEFT_PWM = 44;
const uint8_t RIGHT_DIR_1 = 49, RIGHT_DIR_2 = 47, RIGHT_PWM = 45;
const unsigned long COUNTDOWN_MS = 2000UL;
const unsigned long STRAIGHT_TEST_MS = 5000UL;
const int CIRCLE_PWM = 60;
const uint8_t MANUAL_TURN = 0, STRAIGHT_60CM = 1, CIRCLE_60CM = 2;

SteeringDualH robot;
int testSpeed = 40, testBias = -8, turnAmount = 10;
unsigned long turnDurationMs = 1000UL;
enum TrialState { IDLE, COUNTDOWN, RUNNING };
TrialState trialState = IDLE;
unsigned long phaseStartedMs = 0, trialDurationMs = 0;
int trialCorrection = 0;
bool stopwatchTrial = false;
int trialSpeed = 0, trialBias = 0;
uint8_t trialKind = MANUAL_TURN;

// Last 20 manually stopped trials, oldest first when printed. RAM only.
const uint8_t RESULT_CAPACITY = 20;
unsigned long resultTimeMs[RESULT_CAPACITY];
int resultVelocity[RESULT_CAPACITY], resultHarshness[RESULT_CAPACITY];
int resultBias[RESULT_CAPACITY];
uint8_t resultKind[RESULT_CAPACITY];
uint8_t resultCount = 0, nextResult = 0;

void printResultHeader() {
  Serial.println(F("time_ms,velocity_pwm,harshness,bias,trial"));
}

void printResult(uint8_t index) {
  Serial.print(resultTimeMs[index]); Serial.print(',');
  Serial.print(resultVelocity[index]); Serial.print(',');
  Serial.print(resultHarshness[index]); Serial.print(',');
  Serial.print(resultBias[index]); Serial.print(',');
  if (resultKind[index] == STRAIGHT_60CM) Serial.println(F("STRAIGHT_60CM"));
  else if (resultKind[index] == CIRCLE_60CM) Serial.println(F("CIRCLE_TARGET_RADIUS_60CM"));
  else Serial.println(F("TURN_MANUAL"));
}

void saveTrial(unsigned long elapsedMs) {
  const uint8_t index = nextResult;
  resultTimeMs[index] = elapsedMs;
  resultVelocity[index] = trialSpeed;
  resultHarshness[index] = trialCorrection;
  resultBias[index] = trialBias;
  resultKind[index] = trialKind;
  nextResult = (nextResult + 1) % RESULT_CAPACITY;
  if (resultCount < RESULT_CAPACITY) ++resultCount;
  Serial.println(F("TOC: trial saved; distance/angle judged by you."));
  printResultHeader();
  printResult(index);
}

void printResults() {
  printResultHeader();
  const uint8_t first = resultCount == RESULT_CAPACITY ? nextResult : 0;
  for (uint8_t i = 0; i < resultCount; ++i) {
    printResult((first + i) % RESULT_CAPACITY);
  }
  Serial.println(F("Last 20 trials in RAM; copy this table before reset."));
}

void stopAll() { robot.stop(); }

void printSettings() {
  Serial.print(F("PWM=")); Serial.print(testSpeed);
  Serial.print(F(" bias=")); Serial.print(testBias);
  Serial.print(F(" turn amount=")); Serial.print(turnAmount);
  Serial.print(F(" turn time ms=")); Serial.println(turnDurationMs);
}

void printMenu() {
  Serial.println(F("\n=== MANUAL MOTOR CALIBRATION ==="));
  Serial.println(F("1: left forward  2: left stop  3: left reverse"));
  Serial.println(F("4: right forward 5: right stop 6: right reverse"));
  Serial.println(F("F/B: both forward/backward until S; S: stop/cancel"));
  Serial.println(F("+/-: PWM +/-5 (0..150); ]/[: bias +/-1 (-100..100)"));
  Serial.println(F("T: 5-second straight trial"));
  Serial.println(F("L/R: timed left/right steering trial"));
  Serial.println(F("J/K: TIC left/right turn until S; send S at 90 deg to save TOC"));
  Serial.println(F("V: TIC forward with selected PWM/bias; S at 60 cm saves TOC"));
  Serial.println(F("C/E: TIC left/right circle at PWM 60; S stops and saves"));
  Serial.println(F("Circle target: 60 cm RADIUS at robot center; measure it manually."));
  Serial.println(F("G: print saved trials (last 20, lost on reset)"));
  Serial.println(F(">/<: harshness +/-5; }/{: harshness +/-1 (1..100)"));
  Serial.println(F("./,: turn time +/-250 ms (250..5000)"));
  Serial.println(F("P: settings; H/?: help. Settings changes stop the motors."));
  Serial.println(F("Trials have a 2-second countdown. S cancels at any time."));
  Serial.println(F("Measure heading change by hand; no sensor angle estimation."));
  Serial.println(F("Positive bias speeds the right wheel relative to the left."));
  printSettings();
}

void driveOneWheel(bool right, int velocity) {
  stopAll();
  // Right polarity matches robot.flipRight().
  const uint8_t dir1 = right ? RIGHT_DIR_2 : LEFT_DIR_1;
  const uint8_t dir2 = right ? RIGHT_DIR_1 : LEFT_DIR_2;
  digitalWrite(dir1, velocity < 0 ? HIGH : LOW);
  digitalWrite(dir2, velocity < 0 ? LOW : HIGH);
  analogWrite(right ? RIGHT_PWM : LEFT_PWM, abs(velocity));
}

void stopWheel(bool right) {
  analogWrite(right ? RIGHT_PWM : LEFT_PWM, 0);
  digitalWrite(right ? RIGHT_DIR_1 : LEFT_DIR_1, LOW);
  digitalWrite(right ? RIGHT_DIR_2 : LEFT_DIR_2, LOW);
}

void startTrial(int correction, bool stopwatch, int velocity, uint8_t kind) {
  stopAll();
  if (velocity == 0 || (correction != 0 && abs(correction) >= velocity)) {
    Serial.println(F("Use PWM > 0 and turn amount < PWM for forward steering."));
    return;
  }
  trialCorrection = correction;
  stopwatchTrial = stopwatch;
  trialSpeed = velocity;
  trialBias = testBias;
  trialKind = kind;
  trialDurationMs = correction == 0 ? STRAIGHT_TEST_MS : turnDurationMs;
  printSettings();
  Serial.print(F("Trial PWM=")); Serial.print(trialSpeed);
  Serial.print(F(" signed harshness=")); Serial.println(trialCorrection);
  Serial.print(F("Starts in ")); Serial.print(COUNTDOWN_MS);
  Serial.println(F(" ms; S cancels countdown or stops/saves stopwatch motion."));
  phaseStartedMs = millis();
  trialState = COUNTDOWN;
}

void serviceTrial() {
  const unsigned long now = millis();
  if (trialState == COUNTDOWN && now - phaseStartedMs >= COUNTDOWN_MS) {
    Serial.println(F("RUNNING"));
    phaseStartedMs = millis(); // TIC: exclude countdown and serial printing.
    robot.straight(trialSpeed);
    if (trialCorrection != 0) robot.turn(trialCorrection);
    trialState = RUNNING;
  } else if (trialState == RUNNING && !stopwatchTrial &&
             now - phaseStartedMs >= trialDurationMs) {
    stopAll();
    trialState = IDLE;
    Serial.print(F("DONE: PWM=")); Serial.print(testSpeed);
    Serial.print(F(" bias=")); Serial.print(testBias);
    Serial.print(F(" correction=")); Serial.print(trialCorrection);
    Serial.print(F(" elapsed ms=")); Serial.println(now - phaseStartedMs);
    Serial.println(F("Record distance or heading change; reposition before repeating."));
  }
}

void setup() {
  Serial.begin(115200);
  robot.attach(LEFT_DIR_1, LEFT_DIR_2, LEFT_PWM,
               RIGHT_DIR_1, RIGHT_DIR_2, RIGHT_PWM);
  robot.flipRight();
  robot.setBias(testBias);
  stopAll();
  printMenu();
}

void loop() {
  if (Serial.available()) {
    char c = Serial.read();
    if (c >= 'a' && c <= 'z') c -= 'a' - 'A';
    const bool whitespace = c == '\r' || c == '\n' || c == ' ' || c == '\t';
    if (!whitespace) {
      if (c == 'S' || trialState != IDLE) {
        const unsigned long stoppedMs = millis();
        const bool save = c == 'S' && trialState == RUNNING && stopwatchTrial;
        stopAll();
        trialState = IDLE;
        if (save) saveTrial(stoppedMs - phaseStartedMs);
        else Serial.println(F("STOPPED/CANCELLED. Send the next command separately."));
        while (Serial.available()) Serial.read();
      } else {
        switch (c) {
          case '1': driveOneWheel(false, testSpeed); Serial.println(F("Left forward")); break;
          case '2': stopWheel(false); Serial.println(F("Left stopped")); break;
          case '3': driveOneWheel(false, -testSpeed); Serial.println(F("Left reverse")); break;
          case '4': driveOneWheel(true, testSpeed); Serial.println(F("Right forward")); break;
          case '5': stopWheel(true); Serial.println(F("Right stopped")); break;
          case '6': driveOneWheel(true, -testSpeed); Serial.println(F("Right reverse")); break;
          case 'F': stopAll(); robot.straight(testSpeed); Serial.println(F("Forward; S stops")); break;
          case 'B': stopAll(); robot.straight(-testSpeed); Serial.println(F("Backward; S stops")); break;
          case 'T': startTrial(0, false, testSpeed, MANUAL_TURN); break;
          case 'L': startTrial(turnAmount, false, testSpeed, MANUAL_TURN); break;
          case 'R': startTrial(-turnAmount, false, testSpeed, MANUAL_TURN); break;
          case 'J': startTrial(turnAmount, true, testSpeed, MANUAL_TURN); break;
          case 'K': startTrial(-turnAmount, true, testSpeed, MANUAL_TURN); break;
          case 'V': startTrial(0, true, testSpeed, STRAIGHT_60CM); break;
          case 'C': startTrial(turnAmount, true, CIRCLE_PWM, CIRCLE_60CM); break;
          case 'E': startTrial(-turnAmount, true, CIRCLE_PWM, CIRCLE_60CM); break;
          case 'G': printResults(); break;
          case '+': case '-':
            stopAll();
            testSpeed = constrain(testSpeed + (c == '+' ? 5 : -5), 0, 150);
            printSettings(); break;
          case ']': case '[':
            stopAll();
            testBias = constrain(testBias + (c == ']' ? 1 : -1), -100, 100);
            robot.setBias(testBias); printSettings(); break;
          case '>': case '<':
            stopAll();
            turnAmount = constrain(turnAmount + (c == '>' ? 5 : -5), 1, 100);
            printSettings(); break;
          case '}': case '{':
            stopAll();
            turnAmount = constrain(turnAmount + (c == '}' ? 1 : -1), 1, 100);
            printSettings(); break;
          case '.': case ',':
            stopAll();
            if (c == '.' && turnDurationMs < 5000UL) turnDurationMs += 250UL;
            if (c == ',' && turnDurationMs > 250UL) turnDurationMs -= 250UL;
            printSettings(); break;
          case 'P': printSettings(); break;
          case 'H': case '?': printMenu(); break;
          default: stopAll(); Serial.println(F("Unknown command; stopped. H for help.")); break;
        }
      }
    }
  }
  serviceTrial();
}
