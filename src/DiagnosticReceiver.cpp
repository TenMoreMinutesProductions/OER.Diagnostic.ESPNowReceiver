// ============================================================
//            ESP-NOW DIAGNOSTIC RECEIVER
// ============================================================

#include "DiagnosticReceiver.h"
#include "config.h"

// ============================================================
//                    STATE
// ============================================================

static uint32_t _totalReceived = 0;
static uint32_t _totalMissed = 0;
static uint32_t _signalLossEvents = 0;

static uint32_t _lastSequenceNumber = 0;
static unsigned long _lastPingTime = 0;
static unsigned long _lastHeartbeatTime = 0;
static unsigned long _testStartTime = 0;

static bool _signalLost = false;
static bool _firstPingReceived = false;
static bool _testComplete = false;
static bool _summaryPrinted = false;

// Store transmitter MAC for display
static uint8_t _transmitterMac[6] = {0};
static bool _transmitterKnown = false;

// ============================================================
//                    HELPER FUNCTIONS
// ============================================================

static void formatUptime(unsigned long ms, char* buffer, size_t bufferSize) {
    unsigned long totalSecs = ms / 1000;
    unsigned long hours = totalSecs / 3600;
    unsigned long mins = (totalSecs % 3600) / 60;
    unsigned long secs = totalSecs % 60;
    snprintf(buffer, bufferSize, "%02lu:%02lu:%02lu", hours, mins, secs);
}

static void formatMac(const uint8_t* mac, char* buffer, size_t bufferSize) {
    snprintf(buffer, bufferSize, "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
}

static void printHelp() {
    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║              SERIAL COMMANDS                           ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.println("║  S - Print statistics summary                          ║");
    Serial.println("║  R - Reset all counters                                ║");
    Serial.println("║  H - Print this help message                           ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
}

static void printFinalSummary() {
    unsigned long duration = millis() - _testStartTime;
    char durationStr[16];
    formatUptime(duration, durationStr, sizeof(durationStr));

    float successRate = 0;
    uint32_t total = _totalReceived + _totalMissed;
    if (total > 0) {
        successRate = (_totalReceived * 100.0f) / total;
    }

    char macStr[18];
    formatMac(_transmitterMac, macStr, sizeof(macStr));

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║            RECEIVER TEST COMPLETE                      ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Test duration:      %s                         ║\n", durationStr);
    Serial.printf("║  Packets received:   %-10lu                       ║\n", _totalReceived);
    Serial.printf("║  Packets missed:     %-10lu                       ║\n", _totalMissed);
    Serial.printf("║  Signal loss events: %-10lu                       ║\n", _signalLossEvents);
    Serial.printf("║  Success rate:       %6.2f%%                          ║\n", successRate);
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Transmitter MAC:    %s                 ║\n", macStr);
    Serial.printf("║  Last sequence:      %-10lu                       ║\n", _lastSequenceNumber);
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Test finished. Reset device to run again.");
}

// ============================================================
//                    PUBLIC FUNCTIONS
// ============================================================

void diagnosticReceiverInit() {
    _totalReceived = 0;
    _totalMissed = 0;
    _signalLossEvents = 0;
    _lastSequenceNumber = 0;
    _lastPingTime = 0;
    _lastHeartbeatTime = millis();
    _testStartTime = 0;
    _signalLost = false;
    _firstPingReceived = false;
    _testComplete = false;
    _summaryPrinted = false;
    _transmitterKnown = false;

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         ESP-NOW DIAGNOSTIC RECEIVER                    ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Expecting: %d packets from transmitter            ║\n", TEST_PACKET_COUNT);
    Serial.println("║  Test ends: On packet #10000 or 10s timeout            ║");
    Serial.println("║  Commands: S=stats, R=reset, H=help                    ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.println("║  TIP: Capture serial output to file for logging        ║");
    Serial.println("║       pio device monitor | tee log.txt                 ║");
    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
    Serial.println("Waiting for first ping from transmitter...");
    Serial.println();
}

void diagnosticReceiverLoop() {
    // If test complete, just print summary once
    if (_testComplete) {
        if (!_summaryPrinted) {
            printFinalSummary();
            _summaryPrinted = true;
        }
        return;
    }

    unsigned long now = millis();
    char uptimeStr[16];

    // Check for test completion via timeout (10s after last packet)
    if (_firstPingReceived && (now - _lastPingTime >= TEST_END_TIMEOUT_MS)) {
        _testComplete = true;
        return;
    }

    // Check for signal loss (3s timeout) - only if test still running
    if (_firstPingReceived && !_signalLost) {
        if (now - _lastPingTime >= SIGNAL_TIMEOUT_MS) {
            _signalLost = true;
            _signalLossEvents++;

            formatUptime(now - _testStartTime, uptimeStr, sizeof(uptimeStr));
            unsigned long silenceMs = now - _lastPingTime;
            Serial.printf("[%s] *** SIGNAL LOST *** No ping for %lu ms (last seq=%lu)\n",
                          uptimeStr, silenceMs, _lastSequenceNumber);
        }
    }

    // 60-second heartbeat status
    if (_firstPingReceived && (now - _lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS)) {
        _lastHeartbeatTime = now;

        formatUptime(now - _testStartTime, uptimeStr, sizeof(uptimeStr));

        float progress = (_lastSequenceNumber * 100.0f) / TEST_PACKET_COUNT;
        float successRate = 0;
        uint32_t total = _totalReceived + _totalMissed;
        if (total > 0) {
            successRate = (_totalReceived * 100.0f) / total;
        }

        Serial.println();
        Serial.printf("[%s] Progress: %lu/%d (%.1f%%) | Received: %lu | Missed: %lu | Success: %.1f%%\n",
                      uptimeStr, _lastSequenceNumber, TEST_PACKET_COUNT, progress,
                      _totalReceived, _totalMissed, successRate);
        Serial.println();
    }

    // Handle serial commands
    if (Serial.available()) {
        char cmd = Serial.read();
        switch (cmd) {
            case 's':
            case 'S':
                diagnosticReceiverPrintStats();
                break;
            case 'r':
            case 'R':
                diagnosticReceiverReset();
                formatUptime(now, uptimeStr, sizeof(uptimeStr));
                Serial.printf("[%s] Counters reset\n", uptimeStr);
                break;
            case 'h':
            case 'H':
            case '?':
                printHelp();
                break;
        }
    }
}

void diagnosticReceiverOnPing(const uint8_t* mac, const uint8_t* data, int len) {
    // Ignore packets if test is complete
    if (_testComplete) return;

    unsigned long now = millis();
    char uptimeStr[16];

    // Validate message
    if (len != sizeof(PingMessage)) {
        return;  // Silently ignore invalid packets
    }

    const PingMessage* ping = (const PingMessage*)data;

    if (ping->magic != PING_MAGIC) {
        return;  // Silently ignore non-ping packets
    }

    // Store transmitter MAC on first ping
    if (!_transmitterKnown) {
        memcpy(_transmitterMac, mac, 6);
        _transmitterKnown = true;
    }

    // Handle signal restoration
    if (_signalLost) {
        formatUptime(now - _testStartTime, uptimeStr, sizeof(uptimeStr));
        unsigned long silenceMs = now - _lastPingTime;
        uint32_t expectedSeq = _lastSequenceNumber + 1;
        uint32_t actualMissed = (ping->sequenceNumber > expectedSeq) ?
                                 (ping->sequenceNumber - expectedSeq) : 0;

        Serial.printf("[%s] *** SIGNAL RESTORED *** after %lu ms",
                      uptimeStr, silenceMs);
        if (actualMissed > 0) {
            Serial.printf(" (missed %lu packets)", actualMissed);
        }
        Serial.println();

        _signalLost = false;
    }

    // Check for missed packets (sequence gaps) - count but don't log individually
    if (_firstPingReceived && ping->sequenceNumber > _lastSequenceNumber + 1) {
        uint32_t missed = ping->sequenceNumber - _lastSequenceNumber - 1;
        _totalMissed += missed;
    }

    // Record this ping
    _lastSequenceNumber = ping->sequenceNumber;
    _lastPingTime = now;
    _totalReceived++;

    if (!_firstPingReceived) {
        _firstPingReceived = true;
        _testStartTime = now;
        _lastHeartbeatTime = now;
        char macStr[18];
        formatMac(mac, macStr, sizeof(macStr));
        Serial.printf("[00:00:00] First ping received from %s (seq=%lu)\n",
                      macStr, ping->sequenceNumber);
    }

    // Check if we've received the final packet
    if (ping->sequenceNumber >= TEST_PACKET_COUNT) {
        _testComplete = true;
    }
}

void diagnosticReceiverPrintStats() {
    char uptimeStr[16];
    formatUptime(millis() - _testStartTime, uptimeStr, sizeof(uptimeStr));

    float successRate = 0;
    uint32_t total = _totalReceived + _totalMissed;
    if (total > 0) {
        successRate = (_totalReceived * 100.0f) / total;
    }

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║              DIAGNOSTIC STATISTICS                     ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Test duration:      %s                         ║\n", uptimeStr);
    Serial.printf("║  Pings received:     %-10lu                       ║\n", _totalReceived);
    Serial.printf("║  Pings missed:       %-10lu                       ║\n", _totalMissed);
    Serial.printf("║  Signal loss events: %-10lu                       ║\n", _signalLossEvents);
    Serial.printf("║  Success rate:       %6.2f%%                          ║\n", successRate);
    Serial.println("╠════════════════════════════════════════════════════════╣");

    if (_transmitterKnown) {
        char macStr[18];
        formatMac(_transmitterMac, macStr, sizeof(macStr));
        Serial.printf("║  Transmitter MAC:    %s                 ║\n", macStr);
        Serial.printf("║  Last sequence:      %-10lu                       ║\n", _lastSequenceNumber);
    } else {
        Serial.println("║  Transmitter:        Not yet detected                  ║");
    }

    Serial.printf("║  Signal status:      %-10s                       ║\n",
                  _signalLost ? "LOST" : (_firstPingReceived ? "OK" : "WAITING"));

    Serial.println("╚════════════════════════════════════════════════════════╝");
    Serial.println();
}

void diagnosticReceiverReset() {
    _totalReceived = 0;
    _totalMissed = 0;
    _signalLossEvents = 0;
}

uint32_t diagnosticReceiverGetReceived() {
    return _totalReceived;
}

uint32_t diagnosticReceiverGetMissed() {
    return _totalMissed;
}

uint32_t diagnosticReceiverGetLossEvents() {
    return _signalLossEvents;
}
