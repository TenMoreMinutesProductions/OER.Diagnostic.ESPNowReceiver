#ifndef CONFIG_H
#define CONFIG_H

// ============================================================
//                    PROJECT IDENTITY
// ============================================================
// PROJECT_NAME: Set to your repo folder name (e.g., "OER.Room.UpsideDown.Radio")
// DEVICE_IDENTIFIER: Used for MQTT client/topics, mDNS hostname, and OTA
//                    Naming convention: oer-{room}-{prop} (e.g., oer-ud-radio)
#define PROJECT_NAME "OER.Diagnostic.ESPNowReceiver"
#define DEVICE_IDENTIFIER "oer-diagnostic-espnowreceiver"

// ============================================================
//                  MODULE ENABLE/DISABLE
// ============================================================
#define USE_WIFI 0      // WiFi connectivity
#define USE_MQTT 0      // MQTT pub/sub (requires WiFi)
#define USE_MDNS 0      // mDNS discovery (requires WiFi)
#define USE_OTA 0       // Over-the-air updates (requires WiFi)
#define USE_ESPNOW 1    // ESP-NOW communication
#define USE_HEARTBEAT 1 // Heartbeat LED status indicator

// Dependency checks
#if USE_MQTT && !USE_WIFI
#error "MQTT requires WiFi to be enabled (USE_WIFI 0)"
#endif
#if USE_MDNS && !USE_WIFI
#error "mDNS requires WiFi to be enabled (USE_WIFI 0)"
#endif
#if USE_OTA && !USE_WIFI
#error "OTA requires WiFi to be enabled (USE_WIFI 0)"
#endif

// ============================================================
//                   WIFI CONFIGURATION
// ============================================================
#if USE_WIFI
#define WIFI_SSID "[QUARANTINED] Virus Detected"
#define WIFI_PASS "Vince1138"
#endif

// ============================================================
//                   MQTT CONFIGURATION
// ============================================================
#if USE_MQTT
#define MQTT_BROKER "192.168.1.218"
#define MQTT_PORT 1883
#define MQTT_TOPIC_PREFIX "SP/"
// Derived values use DEVICE_IDENTIFIER:
// - Client name: "client_" + DEVICE_IDENTIFIER
// - Base topic:  MQTT_TOPIC_PREFIX + DEVICE_IDENTIFIER
// - Status topic: base + "/status"
// - Command topic: base + "/cmd" (auto-subscribed)
#endif

// ============================================================
//                    OTA CONFIGURATION
// ============================================================
#if USE_OTA
#define OTA_PASSWORD "" // Leave empty for no password
#endif

// ============================================================
//                  ESP-NOW CONFIGURATION
// ============================================================
#if USE_ESPNOW
#define ESPNOW_HOST 0 // 1 = Host (broadcasts), 0 = Client (listens)
// Host MAC address - set this to your host device's MAC
// Only used when ESPNOW_HOST is 0 (client mode)
#define ESPNOW_HOST_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
#endif

// ============================================================
//                HEARTBEAT CONFIGURATION
// ============================================================
#if USE_HEARTBEAT
#define HEARTBEAT_PIN 48 // GPIO48 = onboard RGB LED on ESP32-S3-DevKitC-1
// Heartbeat states - set initial state based on your boot sequence
// HB_BOOTING:    Fast blink (100ms)  - Initializing
// HB_AP_ONLY:    Double pulse        - Awaiting config (captive portal)
// HB_CONNECTING: Triple pulse        - Connecting to WiFi
// HB_NORMAL:     Slow blink (1000ms) - Normal operation
// HB_ERROR:      SOS pattern         - Error state
#endif

// ============================================================
//                     PIN DEFINITIONS
// ============================================================
// ESP32-S3 Pin Notes:
// - GPIO 0, 2, 3: Strapping pins (affect boot mode) - avoid for outputs
// - GPIO 19, 20: Reserved for native USB - do not use
// - GPIO 26-32: Not available on N8R2 (used for PSRAM/Flash)
// - GPIO 33-37: Not available on S3
// - GPIO 38-42: May be used for JTAG debugging
// - ADC1: GPIO 1-10 (safe to use with WiFi)
// - ADC2: GPIO 11-20 (conflicts with WiFi - avoid for ADC when WiFi active)
// - No DAC pins on S3 - use I2S or external DAC for audio
//
// Safe general-purpose pins: 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14
#define INPUT_PIN 4
#define OUTPUT_PIN 5

// Reset button pin (active LOW with internal pullup)
// Hold for 1 second to trigger prop reset
// Comment out to disable physical reset button
#define RESET_PIN 6

#endif
