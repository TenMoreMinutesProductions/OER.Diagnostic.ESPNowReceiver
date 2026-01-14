// ============================================================
//            ESP-NOW DIAGNOSTIC RECEIVER
// ============================================================
//
// Receives pings from OER.Diagnostic.ESPNowTransmitter and logs:
// - Each received ping with timestamp
// - Signal loss events (no ping for 3+ seconds)
// - Missed packets (sequence gaps)
// - 60-second heartbeat status
//
// Serial Commands:
//   S - Print statistics summary
//   R - Reset all counters
//   H - Print help
//
// To save logs: Capture Serial output to a file using your
// terminal program (PuTTY, screen, or PlatformIO monitor).
//
// ============================================================

#ifndef DIAGNOSTICRECEIVER_H
#define DIAGNOSTICRECEIVER_H

#include <Arduino.h>

// ============================================================
//                   PING MESSAGE STRUCTURE
// ============================================================
// Must match the transmitter's structure exactly.

#pragma pack(push, 1)
struct PingMessage {
    uint8_t magic;           // 0xAA to identify our messages
    uint32_t sequenceNumber; // Incrementing sequence for gap detection
    uint32_t uptimeMs;       // Transmitter uptime in milliseconds
};
#pragma pack(pop)

#define PING_MAGIC 0xAA

// ============================================================
//                    CONFIGURATION
// ============================================================

#define SIGNAL_TIMEOUT_MS     3000   // Signal loss after 3 seconds
#define HEARTBEAT_INTERVAL_MS 60000  // Status heartbeat every 60 seconds
#define TEST_PACKET_COUNT     10000  // Expected packets from transmitter
#define TEST_END_TIMEOUT_MS   10000  // End test after 10s of no packets

// ============================================================
//                    FUNCTIONS
// ============================================================

// Initialize the diagnostic receiver system
void diagnosticReceiverInit();

// Call from loop - handles timeouts, heartbeat, and serial commands
void diagnosticReceiverLoop();

// Call from ESP-NOW receive callback with raw data
void diagnosticReceiverOnPing(const uint8_t* mac, const uint8_t* data, int len);

// Get statistics
uint32_t diagnosticReceiverGetReceived();
uint32_t diagnosticReceiverGetMissed();
uint32_t diagnosticReceiverGetLossEvents();

// Print current statistics
void diagnosticReceiverPrintStats();

// Reset all counters
void diagnosticReceiverReset();

#endif
