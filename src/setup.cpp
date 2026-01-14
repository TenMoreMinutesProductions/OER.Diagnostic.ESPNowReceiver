#include "setup.h"
#include "config.h"
#include "esp_task_wdt.h"

// Module includes
#if USE_WIFI
#endif

#if USE_MQTT
  extern void onMqttMessage(String topic, String payload);
#endif

#if USE_MDNS
#endif

#if USE_OTA
#endif

#if USE_ESPNOW
  #include "modules/espnow_module.h"
  extern void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len);
  extern void onEspNowSend(const uint8_t* mac, bool success);
#endif

#if USE_HEARTBEAT
  #include "modules/heartbeat_module.h"
#endif

// ============================================================
//                    RESET FLAG (Cross-Core Safe)
// ============================================================
static volatile bool _resetRequested = false;

void propRequestReset() {
  _resetRequested = true;
}

bool propResetRequested() {
  return _resetRequested;
}

void propClearResetRequest() {
  _resetRequested = false;
}

// ============================================================
//                 RESET BUTTON TASK (Core 0)
// ============================================================
#ifdef RESET_PIN
static TaskHandle_t _resetTaskHandle = nullptr;

// Monitors reset button on Core 0, independent of main loop
static void resetButtonTask(void* param) {
  unsigned long pressStart = 0;
  bool wasPressed = false;

  while (true) {
    bool isPressed = (digitalRead(RESET_PIN) == LOW);

    if (isPressed && !wasPressed) {
      // Button just pressed
      pressStart = millis();
      wasPressed = true;
    } else if (isPressed && wasPressed) {
      // Button held - check for 1 second hold
      if (millis() - pressStart >= 1000) {
        Serial.println("[Reset] Button held for 1s - requesting reset");
        propRequestReset();
        // Wait for button release before allowing another reset
        while (digitalRead(RESET_PIN) == LOW) {
          vTaskDelay(50 / portTICK_PERIOD_MS);
        }
        wasPressed = false;
      }
    } else {
      wasPressed = false;
    }

    vTaskDelay(50 / portTICK_PERIOD_MS);
  }
}
#endif

// ============================================================
//                       LOGGING
// ============================================================
void propLog(const char* message) {
  Serial.println(message);
  #if USE_MQTT
    if (mqttIsConnected()) {
      mqttPublish("log", message, false);
    }
  #endif
}

void propLog(const String& message) {
  propLog(message.c_str());
}

// ============================================================
//                    RESET REASON
// ============================================================
static const char* getResetReasonString() {
  esp_reset_reason_t reason = esp_reset_reason();
  switch (reason) {
    case ESP_RST_POWERON:   return "Normal Startup";
    case ESP_RST_SW:        return "Software Reset";
    case ESP_RST_PANIC:     return "Crash/Exception";
    case ESP_RST_INT_WDT:   return "Interrupt Watchdog";
    case ESP_RST_TASK_WDT:  return "Task Watchdog";
    case ESP_RST_WDT:       return "Watchdog";
    case ESP_RST_DEEPSLEEP: return "Deep Sleep Wake";
    case ESP_RST_BROWNOUT:  return "Brownout (Low Voltage)";
    case ESP_RST_SDIO:      return "SDIO";
    default:                return "Unknown";
  }
}

void printStartupInfo() {
  Serial.println();
  Serial.println("========================================");
  Serial.print("Project:  ");
  Serial.println(PROJECT_NAME);
  Serial.print("Device:   ");
  Serial.println(DEVICE_IDENTIFIER);
  Serial.print("Built:    ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.print("Status:   ");
  Serial.println(getResetReasonString());
  Serial.println("----------------------------------------");
  Serial.println("Modules Enabled:");

  #if USE_WIFI
    Serial.println("  [X] WiFi");
  #else
    Serial.println("  [ ] WiFi");
  #endif

  #if USE_MQTT
    Serial.println("  [X] MQTT");
  #else
    Serial.println("  [ ] MQTT");
  #endif

  #if USE_MDNS
    Serial.println("  [X] mDNS");
  #else
    Serial.println("  [ ] mDNS");
  #endif

  #if USE_OTA
    Serial.println("  [X] OTA");
  #else
    Serial.println("  [ ] OTA");
  #endif

  #if USE_ESPNOW
    Serial.println("  [X] ESP-NOW");
  #else
    Serial.println("  [ ] ESP-NOW");
  #endif

  #if USE_HEARTBEAT
    Serial.println("  [X] Heartbeat");
  #else
    Serial.println("  [ ] Heartbeat");
  #endif

  Serial.println("----------------------------------------");
}

void printNetworkInfo() {
  Serial.println("Network Info:");

  #if USE_WIFI
    if (wifiIsConnected()) {
      Serial.print("  IP Address: ");
      Serial.println(wifiGetIP());
      Serial.print("  MAC:        ");
      Serial.println(wifiGetMAC());
    } else {
      Serial.println("  WiFi:       Not connected");
    }
  #endif

  #if USE_MDNS
    Serial.print("  mDNS:       ");
    Serial.println(mdnsGetHostname());
  #endif

  #if USE_MQTT
    Serial.print("  MQTT Topic: ");
    Serial.println(mqttGetBaseTopic());
    Serial.print("  MQTT:       ");
    Serial.println(mqttIsConnected() ? "Connected" : "Disconnected");
  #endif

  #if USE_OTA
    Serial.println("  OTA:        Ready");
  #endif

  #if USE_ESPNOW
    Serial.print("  ESP-NOW:    ");
    #if ESPNOW_HOST
      Serial.println("Host mode");
    #else
      Serial.println("Client mode");
    #endif
    Serial.print("  ESP-NOW MAC: ");
    Serial.println(espnowGetMAC());
  #endif

  #if USE_HEARTBEAT
    Serial.print("  Heartbeat:  ");
    Serial.print(heartbeatGetStateString());
    Serial.print(" (GPIO ");
    Serial.print(HEARTBEAT_PIN);
    Serial.println(")");
  #endif

  Serial.println("========================================");
  Serial.println();
}

void setupInit() {
  // Initialize Serial
  Serial.begin(115200);
  delay(2000);  // Give serial monitor time to connect

  // Print startup header
  Serial.println();
  Serial.println("========================================");
  Serial.print("[Boot] ");
  Serial.print(DEVICE_IDENTIFIER);
  Serial.print(" | Built: ");
  Serial.print(__DATE__);
  Serial.print(" ");
  Serial.println(__TIME__);
  Serial.println("========================================");

  // Initialize heartbeat LED first (visual feedback during boot)
  #if USE_HEARTBEAT
    heartbeatInit(HEARTBEAT_PIN);
    heartbeatSetState(HB_BOOTING);
  #endif

  // Print startup header
  printStartupInfo();

  // Initialize watchdog (60 second timeout, reboot on hang)
  // If loop() doesn't call esp_task_wdt_reset() within 60s, ESP32 reboots.
  // This is safely outside MQTT keepalive (15s) and LWT timeout (~22s).
  esp_task_wdt_init(60, true);
  esp_task_wdt_add(NULL);
  Serial.println("[Watchdog] Initialized (60s timeout)");

  // Initialize pins
  pinMode(INPUT_PIN, INPUT);
  pinMode(OUTPUT_PIN, OUTPUT);
  #ifdef RESET_PIN
    pinMode(RESET_PIN, INPUT_PULLUP);
  #endif

  // Initialize modules based on configuration

  #if USE_WIFI
    #if USE_HEARTBEAT
      heartbeatSetState(HB_CONNECTING);
    #endif
    wifiInit(WIFI_SSID, WIFI_PASS);
  #endif

  #if USE_MDNS
    mdnsInit(DEVICE_IDENTIFIER);
    mdnsAddService("oer", "tcp", 80);  // Register for _oer._tcp discovery
  #endif

  #if USE_MQTT
    mqttInit(MQTT_BROKER, MQTT_PORT, DEVICE_IDENTIFIER, MQTT_TOPIC_PREFIX);
    mqttSetCallback(onMqttMessage);
  #endif

  #if USE_OTA
    otaInit(DEVICE_IDENTIFIER, OTA_PASSWORD);
  #endif

  #if USE_ESPNOW
    uint8_t hostMac[] = ESPNOW_HOST_MAC;
    #if ESPNOW_HOST
      espnowInit(true, nullptr);
    #else
      espnowInit(false, hostMac);
    #endif
    espnowSetReceiveCallback(onEspNowReceive);
    espnowSetSendCallback(onEspNowSend);
  #endif

  // Start reset button task on Core 0 (if RESET_PIN defined)
  #ifdef RESET_PIN
    xTaskCreatePinnedToCore(
      resetButtonTask,
      "ResetTask",
      2048,
      NULL,
      1,
      &_resetTaskHandle,
      0  // Core 0
    );
    Serial.println("[Reset] Button task started on Core 0 (GPIO " + String(RESET_PIN) + ")");
  #endif

  // Print network status after all modules initialized
  printNetworkInfo();

  // Set heartbeat to normal operation
  #if USE_HEARTBEAT
    heartbeatSetState(HB_NORMAL);
  #endif

  Serial.println("[Setup] Complete. Starting main loop...");
  Serial.println();
}
