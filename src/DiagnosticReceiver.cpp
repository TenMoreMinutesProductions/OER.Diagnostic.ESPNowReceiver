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

static bool _signalLost = false;
static bool _firstPingReceived = false;

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
    _signalLost = false;
    _firstPingReceived = false;
    _transmitterKnown = false;

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║         ESP-NOW DIAGNOSTIC RECEIVER                    ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.println("║  Expected: 10 pings/sec (100ms interval)               ║");
    Serial.println("║  Timeout:  3 seconds (signal loss detection)           ║");
    Serial.println("║  Heartbeat: 60 seconds                                 ║");
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
    unsigned long now = millis();
    char uptimeStr[16];
    formatUptime(now, uptimeStr, sizeof(uptimeStr));

    // Check for signal timeout (only after first ping received)
    if (_firstPingReceived && !_signalLost) {
        if (now - _lastPingTime >= SIGNAL_TIMEOUT_MS) {
            _signalLost = true;
            _signalLossEvents++;

            unsigned long silenceMs = now - _lastPingTime;
            Serial.printf("[%s] *** SIGNAL LOST *** No ping for %lu ms (last seq=%lu)\n",
                          uptimeStr, silenceMs, _lastSequenceNumber);
        }
    }

    // 60-second heartbeat status
    if (now - _lastHeartbeatTime >= HEARTBEAT_INTERVAL_MS) {
        _lastHeartbeatTime = now;

        Serial.println();
        Serial.printf("[%s] === HEARTBEAT === Receiver online\n", uptimeStr);

        if (_firstPingReceived) {
            float successRate = 0;
            uint32_t total = _totalReceived + _totalMissed;
            if (total > 0) {
                successRate = (_totalReceived * 100.0f) / total;
            }

            Serial.printf("             Received: %lu | Missed: %lu | Loss events: %lu | Success: %.1f%%\n",
                          _totalReceived, _totalMissed, _signalLossEvents, successRate);

            if (_transmitterKnown) {
                char macStr[18];
                formatMac(_transmitterMac, macStr, sizeof(macStr));
                Serial.printf("             Transmitter: %s | Last seq: %lu\n",
                              macStr, _lastSequenceNumber);
            }
        } else {
            Serial.println("             Waiting for first ping from transmitter...");
        }

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
    unsigned long now = millis();
    char uptimeStr[16];
    formatUptime(now, uptimeStr, sizeof(uptimeStr));

    // Validate message
    if (len != sizeof(PingMessage)) {
        Serial.printf("[%s] WARN: Invalid message size (%d bytes, expected %d)\n",
                      uptimeStr, len, sizeof(PingMessage));
        return;
    }

    const PingMessage* ping = (const PingMessage*)data;

    if (ping->magic != PING_MAGIC) {
        Serial.printf("[%s] WARN: Invalid magic byte (0x%02X, expected 0x%02X)\n",
                      uptimeStr, ping->magic, PING_MAGIC);
        return;
    }

    // Store transmitter MAC on first ping
    if (!_transmitterKnown) {
        memcpy(_transmitterMac, mac, 6);
        _transmitterKnown = true;
    }

    // Handle signal restoration
    if (_signalLost) {
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

    // Check for missed packets (sequence gaps)
    if (_firstPingReceived && ping->sequenceNumber > _lastSequenceNumber + 1) {
        uint32_t missed = ping->sequenceNumber - _lastSequenceNumber - 1;
        _totalMissed += missed;
        Serial.printf("[%s] MISSED %lu packet(s) (seq %lu -> %lu)\n",
                      uptimeStr, missed, _lastSequenceNumber, ping->sequenceNumber);
    }

    // Record this ping
    _lastSequenceNumber = ping->sequenceNumber;
    _lastPingTime = now;
    _totalReceived++;

    if (!_firstPingReceived) {
        _firstPingReceived = true;
        char macStr[18];
        formatMac(mac, macStr, sizeof(macStr));
        Serial.printf("[%s] First ping received from %s\n", uptimeStr, macStr);
    }

    // Silent operation - only log on signal loss/restore events and heartbeat
}

void diagnosticReceiverPrintStats() {
    char uptimeStr[16];
    formatUptime(millis(), uptimeStr, sizeof(uptimeStr));

    float successRate = 0;
    uint32_t total = _totalReceived + _totalMissed;
    if (total > 0) {
        successRate = (_totalReceived * 100.0f) / total;
    }

    Serial.println();
    Serial.println("╔════════════════════════════════════════════════════════╗");
    Serial.println("║              DIAGNOSTIC STATISTICS                     ║");
    Serial.println("╠════════════════════════════════════════════════════════╣");
    Serial.printf("║  Receiver uptime:    %s                         ║\n", uptimeStr);
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
    // Keep lastSequenceNumber to continue gap detection
    // Keep transmitter info
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
