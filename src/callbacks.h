#ifndef CALLBACKS_H
#define CALLBACKS_H

#include <Arduino.h>

// ============================================================
//                   CALLBACK DECLARATIONS
// ============================================================
// These callbacks are called by the modules when events occur.
// Implementations are in callbacks.cpp - customize them there.

#if USE_MQTT
void onMqttMessage(String topic, String payload);
#endif

#if USE_ESPNOW
void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len);
void onEspNowSend(const uint8_t* mac, bool success);
#endif

void onPropReset();

#endif
