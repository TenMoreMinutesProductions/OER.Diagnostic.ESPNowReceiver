#ifndef SETUP_H
#define SETUP_H

#include <Arduino.h>

// Main setup initialization
void setupInit();

// Print startup information block
void printStartupInfo();

// ============================================================
//                       LOGGING
// ============================================================
// Simple logging - outputs to Serial, and to MQTT {base}/log when connected.
// Use descriptive messages with [Component] prefix:
//
//   propLog("[Input] Button pressed on GPIO 4");
//   propLog("[Prop] State: IDLE -> SOLVED");
//   propLog("[Audio] Playing track 3");
//   propLog("[Error] Solenoid timeout after 5s");
//
void propLog(const char* message);
void propLog(const String& message);

// ============================================================
//                       RESET
// ============================================================
// Request a prop reset - can be called from any core.
// Sets a flag that main loop checks and handles.
void propRequestReset();

// Check if reset was requested (called by main loop)
bool propResetRequested();

// Clear the reset request flag
void propClearResetRequest();

#endif
