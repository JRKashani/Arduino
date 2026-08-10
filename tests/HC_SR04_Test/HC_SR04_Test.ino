/*
  HC_SR04_Test.ino
  ------------------------------------------------------------
  Minimal diagnostic test for ONE HC-SR04 ultrasonic sensor.
  To check each of your 4 units, connect one at a time to the
  pins below, upload, verify, then swap in the next physical
  sensor using the same wiring.

  WIRING
    VCC  -> Mega 5V
    GND  -> Mega GND
    Trig -> Mega pin 9
    Echo -> Mega pin 8

  ELECTRICAL NOTES
    - 5V sensor, 5V logic Echo pulse - Mega is 5V logic too,
      so no voltage divider is needed (unlike 3.3V boards).
    - All sensors must share a common GND with the Mega.

  TIMING NOTES
    - Trigger pulse must be >=10us HIGH per datasheet.
    - pulseIn() is given an explicit timeout so a missing echo
      can't block the program for pulseIn's 1-second default.
      HC-SR04's rated max range is ~400-450 cm. At the speed
      of sound (~343 m/s), a 450 cm round trip takes about
      26 ms, so a 30 ms (30000 us) timeout comfortably covers
      the full usable range while staying responsive.
    - A ~100 ms delay between readings keeps the trigger cycle
      above the sensor's recommended ~60 ms minimum, avoiding
      leftover echo ring-down from the previous pulse.
  ------------------------------------------------------------
*/

const uint8_t TRIG_PIN = 9;
const uint8_t ECHO_PIN = 8;

const unsigned long ECHO_TIMEOUT_US = 30000UL; // ~30 ms, see notes above

void setup() {
  Serial.begin(9600);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  digitalWrite(TRIG_PIN, LOW);

  Serial.println("HC_SR04_Test ready.");
}

void loop() {
  // Clean 10us trigger pulse.
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  unsigned long duration = pulseIn(ECHO_PIN, HIGH, ECHO_TIMEOUT_US);

  if (duration == 0) {
    // pulseIn() returns 0 on timeout - no echo received.
    Serial.println("Distance: NO ECHO");
  } else {
    // Speed of sound ~343 m/s -> ~58.0 us per round-trip cm.
    float distanceCm = duration / 58.0;
    Serial.print("Distance: ");
    Serial.print(distanceCm, 1);
    Serial.println(" cm");
  }

  delay(100); // keep cycle above ~60ms minimum; readable update rate
}