#ifndef LOG_H
#define LOG_H

#include <Arduino.h>

// Rate-limited logging. Each USE of the macro gets its own function-local
// static timer (one per expansion site), so independent log statements do not
// starve each other the way a single shared timer would.
//
// Usage (statements separated by ';', not top-level ','):
//   LOG_EVERY_MS(250, Serial.print(F("x=")); Serial.println(x));
//   STAGE_LOG(Serial.println(F("...")));   // uses STAGE_LOG_INTERVAL_MS
//
// The first hit always fires (the static starts at 0).
#define LOG_EVERY_MS(interval_ms, ...)                                   \
  do {                                                                   \
    static unsigned long _log_last_ms = 0;                               \
    const unsigned long _log_now_ms = millis();                          \
    if (_log_now_ms - _log_last_ms >= (unsigned long)(interval_ms)) {     \
      _log_last_ms = _log_now_ms;                                        \
      __VA_ARGS__;                                                       \
    }                                                                    \
  } while (0)

// Convenience wrapper at the shared stage cadence (needs DEFINE.h in scope).
#define STAGE_LOG(...) LOG_EVERY_MS(STAGE_LOG_INTERVAL_MS, __VA_ARGS__)

#endif
