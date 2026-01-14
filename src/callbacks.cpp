#include "callbacks.h"
#include "config.h"
#include "setup.h"
#include "DiagnosticReceiver.h"

// ============================================================
//                   CALLBACK FUNCTIONS
// ============================================================
// Customize these for your puzzle-specific logic.

#if USE_MQTT
// Called when MQTT message is received on subscribed topics
void onMqttMessage(String topic, String payload) {
  propLog("[MQTT] Received: " + payload);

  // Handle reset command (case-insensitive)
  if (payload.equalsIgnoreCase("reset")) {
    propLog("[MQTT] Reset command received");
    propRequestReset();
    return;
  }

  // Add your puzzle-specific MQTT handling here
  // Example:
  // if (payload.equalsIgnoreCase("solve")) {
  //   solvePuzzle();
  // }
}
#endif

#if USE_ESPNOW
// Called when ESP-NOW message is received
void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
  // Forward to diagnostic receiver for processing
  diagnosticReceiverOnPing(mac, data, len);
}

// Called when ESP-NOW send completes
void onEspNowSend(const uint8_t* mac, bool success) {
  Serial.print("[ESP-NOW] Send ");
  Serial.println(success ? "OK" : "FAILED");
}
#endif

// ============================================================
//                    RESET HANDLER
// ============================================================
// Called when reset is triggered via button (held 1s) or MQTT command.

void onPropReset() {
  propLog("[Reset] Resetting prop to initial state...");

  // Reset outputs
  digitalWrite(OUTPUT_PIN, LOW);

  // Add your reset logic here
  // - Reset game state variables
  // - Turn off LEDs/motors
  // - Reset audio players
  //
  // To integrate with SampleFunction state machine:
  //   extern void resetSampleState();
  //   resetSampleState();

  propLog("[Reset] Complete");
}
