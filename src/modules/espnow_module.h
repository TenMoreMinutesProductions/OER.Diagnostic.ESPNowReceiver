#ifndef ESPNOW_MODULE_H
#define ESPNOW_MODULE_H

#include <Arduino.h>

// Callback function type for incoming ESP-NOW messages
typedef void (*EspNowReceiveCallback)(const uint8_t* mac, const uint8_t* data, int len);

// Callback function type for send status
typedef void (*EspNowSendCallback)(const uint8_t* mac, bool success);

// Initialize ESP-NOW
// isHost: true = Host mode (broadcasts), false = Client mode (listens to host)
// hostMac: MAC address of host (only used in client mode)
// Returns true if initialized successfully
bool espnowInit(bool isHost, const uint8_t* hostMac = nullptr);

// Set callback for received messages
void espnowSetReceiveCallback(EspNowReceiveCallback callback);

// Set callback for send status
void espnowSetSendCallback(EspNowSendCallback callback);

// Send data to a specific peer or broadcast
// mac: Target MAC address (nullptr for broadcast in host mode)
// data: Data to send
// len: Data length
bool espnowSend(const uint8_t* mac, const uint8_t* data, size_t len);

// Send string message
bool espnowSendString(const uint8_t* mac, const String& message);

// Broadcast data to all peers (host mode only)
bool espnowBroadcast(const uint8_t* data, size_t len);

// Add a peer
bool espnowAddPeer(const uint8_t* mac);

// Remove a peer
bool espnowRemovePeer(const uint8_t* mac);

// Get own MAC address as string
String espnowGetMAC();

// Check if ESP-NOW is initialized
bool espnowIsInitialized();

// Check if running as host
bool espnowIsHost();

// Sync ESP-NOW to current WiFi channel
// Call this after WiFi connects if using both WiFi and ESP-NOW
void espnowSyncChannel();

#endif
