// ============================================================
//            ESP32-S3 HARDWARE DIAGNOSTIC TEST
// ============================================================
//
// THIS CODE IS FOR CONFIRMING YOUR ESP32-S3 IS WORKING CORRECTLY.
// DELETE THIS FILE AND CREATE YOUR OWN PUZZLE LOGIC BEFORE
// DEPLOYING YOUR PROP.
//
// What this diagnostic shows (every 5 seconds):
//   - Heap memory usage (internal RAM)
//   - PSRAM usage (external RAM)
//   - Largest free memory block (fragmentation check)
//   - Uptime
//
// Once you confirm the ESP32 is functioning:
//   1. Delete this file (SampleFunction.cpp)
//   2. Delete SampleFunction.h
//   3. Create your puzzle logic files
//   4. Update loop.cpp to call your new function
//
// ============================================================

#include "SampleFunction.h"
#include "config.h"
#include "setup.h"
#include <esp_heap_caps.h>

// ============================================================
//                    TIMING
// ============================================================

static unsigned long lastDiagnosticTime = 0;
static const unsigned long DIAGNOSTIC_INTERVAL_MS = 5000;  // 5 second update rate

// ============================================================
//                    MAIN FUNCTION
// ============================================================

void SampleFunction() {
    unsigned long now = millis();

    if (now - lastDiagnosticTime >= DIAGNOSTIC_INTERVAL_MS) {
        lastDiagnosticTime = now;

        // Heap (internal RAM)
        size_t heapFree = ESP.getFreeHeap();
        size_t heapTotal = ESP.getHeapSize();
        size_t heapUsed = heapTotal - heapFree;
        float heapPercent = (heapUsed * 100.0f) / heapTotal;

        // PSRAM (external RAM)
        size_t psramFree = ESP.getFreePsram();
        size_t psramTotal = ESP.getPsramSize();
        size_t psramUsed = psramTotal - psramFree;
        float psramPercent = (psramTotal > 0) ? (psramUsed * 100.0f) / psramTotal : 0;

        // Largest contiguous block (fragmentation indicator)
        size_t largestBlock = heap_caps_get_largest_free_block(MALLOC_CAP_8BIT);

        // Format uptime
        unsigned long secs = now / 1000;
        unsigned long mins = secs / 60;
        secs = secs % 60;

        Serial.println();
        Serial.println("========================================");
        Serial.printf("ESP32-S3 DIAGNOSTIC  Uptime: %lu:%02lu\n", mins, secs);
        Serial.println("========================================");
        Serial.printf("Heap:  %6d / %6d bytes (%5.1f%% used)\n",
                      heapUsed, heapTotal, heapPercent);
        Serial.printf("PSRAM: %6d / %7d bytes (%5.1f%% used)\n",
                      psramUsed, psramTotal, psramPercent);
        Serial.printf("Largest free block: %d bytes\n", largestBlock);
        Serial.println("========================================");
    }
}
