#include "ManualLink.h"
#include <Arduino.h>

namespace {
char line[32];
uint8_t length = 0;
bool overflow = false;

void finishLine(ManualLineHandler handler) {
  if (overflow) Serial.println(F("ERROR,LINE_TOO_LONG"));
  else if (length) { line[length] = '\0'; handler(line); }
  length = 0;
  overflow = false;
}
} // namespace

void pollManualLink(ManualLineHandler handler) {
  // Bounded per call so a serial flood cannot starve sensing or the watchdog.
  for (uint8_t budget = 0; budget < 64 && Serial.available(); ++budget) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') { finishLine(handler); continue; }
    if (overflow) continue;
    if (length < sizeof(line) - 1) line[length++] = c;
    else overflow = true;
  }
}
