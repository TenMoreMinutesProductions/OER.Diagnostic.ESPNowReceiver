#ifndef HEARTBEAT_MODULE_H
#define HEARTBEAT_MODULE_H

#include <Arduino.h>

// Heartbeat LED states with associated colors
typedef enum {
    HB_BOOTING,      // Fast blink BLUE (100ms)  - Initializing
    HB_AP_ONLY,      // Double pulse YELLOW      - Awaiting config
    HB_CONNECTING,   // Triple pulse CYAN        - Connecting to WiFi
    HB_NORMAL,       // Slow blink GREEN (1s)    - Normal operation
    HB_ERROR         // SOS pattern RED          - Error state
} HeartbeatState;

// Initialize heartbeat module - creates FreeRTOS task on Core 0
// Uses NeoPixel (WS2812) protocol for ESP32-S3 onboard RGB LED
void heartbeatInit(uint8_t pin);

// Set the current heartbeat state (thread-safe)
void heartbeatSetState(HeartbeatState state);

// Get the current heartbeat state
HeartbeatState heartbeatGetState();

// Get string representation of current state
const char* heartbeatGetStateString();

// Set LED brightness (0-255, default 10)
void heartbeatSetBrightness(uint8_t brightness);

#endif
