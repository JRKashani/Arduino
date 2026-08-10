/*
  Button_Test.ino
  ------------------------------------------------------------
  Minimal diagnostic test for one momentary push button.

  WIRING
    Button terminal A -> Mega pin 2
    Button terminal B -> GND

  NOTE: if using a common 4-leg tactile button, the legs are
  internally paired (two pins per pair, shorted together).
  Use one leg from each pair (e.g. diagonal corners) - using
  two legs from the same pair will always read as "pressed".

  Uses the Mega's internal pull-up (INPUT_PULLUP), so no
  external resistor is needed. Idle = HIGH, pressed = LOW.

  BEHAVIOR
    Prints "Button: PRESSED" / "Button: RELEASED" only on
    state changes, with a short software debounce.
  ------------------------------------------------------------
*/

const uint8_t BUTTON_PIN = 2;
const unsigned long DEBOUNCE_DELAY_MS = 50;

int lastStableState = HIGH;
int lastRawState = HIGH;
unsigned long lastChangeTime = 0;

void setup() {
  Serial.begin(9600);
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  Serial.println("Button_Test ready. Waiting for presses...");
}

void loop() {
  int rawState = digitalRead(BUTTON_PIN);

  // Any raw change resets the debounce timer.
  if (rawState != lastRawState) {
    lastChangeTime = millis();
    lastRawState = rawState;
  }

  // Only accept the new state once it's been stable for
  // DEBOUNCE_DELAY_MS - filters out mechanical contact bounce.
  if ((millis() - lastChangeTime) > DEBOUNCE_DELAY_MS) {
    if (rawState != lastStableState) {
      lastStableState = rawState;
      Serial.println(lastStableState == LOW ? "Button: PRESSED" : "Button: RELEASED");
    }
  }
}