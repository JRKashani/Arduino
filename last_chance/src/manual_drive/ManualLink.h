#ifndef MANUAL_LINK_H
#define MANUAL_LINK_H

// Newline-terminated command lines from the PC. Overlong lines are dropped
// whole and reported as ERROR,LINE_TOO_LONG.
typedef void (*ManualLineHandler)(const char *line);

// Call every loop; invokes handler once per complete line.
void pollManualLink(ManualLineHandler handler);

#endif
