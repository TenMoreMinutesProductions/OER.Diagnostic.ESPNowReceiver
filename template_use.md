# ESP32 Escape Room Template

A modular ESP32 template for escape room props with WiFi, MQTT, mDNS, OTA updates, and ESP-NOW communication.

## Quick Start

1. Copy this template folder for your new puzzle
2. Edit `src/config.h` to configure your device
3. Delete `src/SampleFunction.cpp` and `src/SampleFunction.h` (template examples only)
4. Create your puzzle logic in new source files
5. Build and upload with `pio run --target upload`

### Claude Code Auto-Configuration

When Claude Code is first run against a newly created prop, it should use `AskUserQuestion` to prompt for:
- WiFi credentials (SSID/password)
- MQTT settings (broker IP, port, topic prefix)
- Which modules to enable: WiFi, MQTT, mDNS, OTA, ESP-NOW, Heartbeat
- Device identifier name

If the user declines a module:
- Set the `USE_*` flag to `0` in `config.h`
- Delete the unused module files from `src/modules/`
- Remove related `#include` statements and function calls from `main.cpp`, `setup.cpp`, `loop.cpp`

Always delete `src/SampleFunction.cpp` and `src/SampleFunction.h` after setup. Before deleting, reference these files in `OER.Template.ESP32/src/` for the expected function structure and patterns.

---

## Configuration (config.h)

### Device Identifier

```cpp
#define DEVICE_IDENTIFIER "LaserMaze"  // Used for MQTT, mDNS, OTA
```

This single identifier is used everywhere:
- MQTT client name: `client_LaserMaze`
- MQTT base topic: `SP/LaserMaze`
- mDNS hostname: `LaserMaze.local`
- OTA hostname: `LaserMaze`

### Enable/Disable Modules

```cpp
#define USE_WIFI   1   // WiFi connectivity
#define USE_MQTT   1   // MQTT pub/sub (requires WiFi)
#define USE_MDNS   1   // mDNS discovery (requires WiFi)
#define USE_OTA    1   // Over-the-air updates (requires WiFi)
#define USE_ESPNOW 0   // ESP-NOW communication
```

Set to `1` to enable, `0` to disable.

### WiFi Configuration

```cpp
#define WIFI_SSID "YourNetworkName"
#define WIFI_PASS "YourPassword"
```

### MQTT Configuration

```cpp
#define MQTT_BROKER "192.168.1.218"  // Broker IP address
#define MQTT_PORT 1883                // Default MQTT port
#define MQTT_TOPIC_PREFIX "SP/"       // Topic prefix (e.g., "EscapeRoom/")
```

### ESP-NOW Configuration

```cpp
#define ESPNOW_HOST 1  // 1 = Host (broadcasts), 0 = Client (listens)

// Host MAC address (only needed for clients)
#define ESPNOW_HOST_MAC {0xAA, 0xBB, 0xCC, 0xDD, 0xEE, 0xFF}
```

---

## Core Distribution

| Module | Core | Notes |
|--------|------|-------|
| WiFi | Core 0 | Auto-reconnect task |
| OTA | Core 0 | Update handling task |
| ESP-NOW | Core 0 | Event-driven callbacks |
| MQTT | Core 1 | Synced with game logic |
| Game Logic | Core 1 | Your puzzle code |

---

## MQTT Usage

### Automatic Behavior

On connection, the module automatically:
- Publishes `{DEVICE_IDENTIFIER} Online` to `{prefix}{identifier}/status` (retained)
- Subscribes to `{prefix}{identifier}/cmd` for commands
- Sets "Offline" as LWT (Last Will and Testament)

### Receiving Messages

Edit the callback in `main.cpp`:

```cpp
void onMqttMessage(String topic, String payload) {
  Serial.print("[MQTT] ");
  Serial.print(topic);
  Serial.print(": ");
  Serial.println(payload);

  // Handle commands
  if (payload == "SOLVE") {
    solvePuzzle();
  } else if (payload == "RESET") {
    resetPuzzle();
  } else if (payload == "HINT") {
    giveHint();
  }
}
```

### Publishing Messages

```cpp
#include "modules/mqtt_module.h"

// Publish to base topic + suffix (e.g., "SP/LaserMaze/state")
mqttPublish("state", "SOLVED");

// Publish with retain flag
mqttPublish("state", "READY", true);

// Publish to absolute topic (starts with /)
mqttPublish("/escape/global/alert", "Laser puzzle solved!");
```

### Subscribing to Additional Topics

```cpp
// Subscribe to base topic + suffix
mqttSubscribe("hints");  // Subscribes to "SP/LaserMaze/hints"

// Subscribe to absolute topic
mqttSubscribe("/escape/global/#");
```

### Check Connection Status

```cpp
if (mqttIsConnected()) {
  mqttPublish("heartbeat", "alive");
}
```

### Get Base Topic

```cpp
String topic = mqttGetBaseTopic();  // Returns "SP/LaserMaze"
```

---

## ESP-NOW Usage

### Host vs Client Mode

**Host (ESPNOW_HOST = 1)**
- Broadcasts messages to all clients
- Receives responses from any client
- Typically the central controller

**Client (ESPNOW_HOST = 0)**
- Listens for broadcasts from host
- Sends responses back to host only
- Typically individual props

### Receiving Messages

Edit the callback in `main.cpp`:

```cpp
void onEspNowReceive(const uint8_t* mac, const uint8_t* data, int len) {
  // Convert to string
  char msg[len + 1];
  memcpy(msg, data, len);
  msg[len] = '\0';

  String message = String(msg);

  // Handle commands
  if (message == "SOLVE") {
    solvePuzzle();
  } else if (message == "RESET") {
    resetPuzzle();
  } else if (message.startsWith("LED:")) {
    int value = message.substring(4).toInt();
    setLED(value);
  }
}
```

### Send Status Callback

```cpp
void onEspNowSend(const uint8_t* mac, bool success) {
  if (!success) {
    Serial.println("ESP-NOW send failed, retrying...");
    // Implement retry logic if needed
  }
}
```

### Sending Messages (Host Mode)

```cpp
#include "modules/espnow_module.h"

// Broadcast to all clients
espnowBroadcast((uint8_t*)"SOLVE", 5);

// Broadcast a string
String cmd = "LED:255";
espnowBroadcast((uint8_t*)cmd.c_str(), cmd.length());
```

### Sending Messages (Client Mode)

```cpp
// Send to host (use nullptr for registered host)
espnowSendString(nullptr, "BUTTON_PRESSED");

// Send data
uint8_t data[] = {0x01, 0x02, 0x03};
espnowSend(nullptr, data, sizeof(data));
```

### Sending to Specific Peer

```cpp
// Define target MAC
uint8_t targetMac[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};

// Add as peer first
espnowAddPeer(targetMac);

// Send message
espnowSendString(targetMac, "Hello specific device");
```

### Managing Peers

```cpp
// Add a peer
uint8_t peerMac[] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66};
espnowAddPeer(peerMac);

// Remove a peer
espnowRemovePeer(peerMac);

// Get own MAC address
String myMac = espnowGetMAC();
Serial.println(myMac);  // "AA:BB:CC:DD:EE:FF"
```

### Check Status

```cpp
if (espnowIsInitialized()) {
  if (espnowIsHost()) {
    espnowBroadcast((uint8_t*)"PING", 4);
  }
}
```

---

## WiFi Usage

### Check Connection

```cpp
#include "modules/wifi_module.h"

if (wifiIsConnected()) {
  Serial.println("WiFi connected");
}
```

### Get Network Info

```cpp
String ip = wifiGetIP();    // "192.168.1.105"
String mac = wifiGetMAC();  // "AA:BB:CC:DD:EE:FF"
```

---

## OTA Updates

### Upload via Network

1. Build the project: `pio run`
2. Find your device IP or use mDNS name
3. Upload: `pio run --target upload --upload-port LaserMaze.local`

Or in `platformio.ini`:
```ini
upload_protocol = espota
upload_port = LaserMaze.local
```

### Check Update Status

```cpp
#include "modules/ota_module.h"

if (otaIsUpdating()) {
  // Pause game logic during update
  return;
}
```

---

## mDNS Usage

### Access Device by Name

Once connected, access your device at:
```
http://LaserMaze.local
```

### Get Hostname

```cpp
#include "modules/mdns_module.h"

String hostname = mdnsGetHostname();  // "LaserMaze.local"
```

### Add Service Advertisement

```cpp
mdnsAddService("http", "tcp", 80);   // Advertise web server
mdnsAddService("mqtt", "tcp", 1883); // Advertise MQTT
```

---

## Typical Escape Room Patterns

### Puzzle State Machine

```cpp
enum PuzzleState {
  STATE_IDLE,
  STATE_ACTIVE,
  STATE_SOLVED,
  STATE_HINT
};

PuzzleState currentState = STATE_IDLE;

void updatePuzzle() {
  switch (currentState) {
    case STATE_IDLE:
      // Wait for game start
      break;
    case STATE_ACTIVE:
      // Check win conditions
      if (checkSolved()) {
        currentState = STATE_SOLVED;
        onSolved();
      }
      break;
    case STATE_SOLVED:
      // Puzzle complete
      break;
  }
}

void onSolved() {
  #if USE_MQTT
    mqttPublish("state", "SOLVED", true);
  #endif
  #if USE_ESPNOW
    espnowBroadcast((uint8_t*)"SOLVED", 6);
  #endif
}
```

### Handle Reset Command

```cpp
void resetPuzzle() {
  currentState = STATE_IDLE;
  // Reset hardware
  digitalWrite(LOCK_PIN, LOW);
  // Reset variables
  attempts = 0;

  #if USE_MQTT
    mqttPublish("state", "READY", true);
  #endif
}
```

### Heartbeat / Status Updates

```cpp
unsigned long lastHeartbeat = 0;
const unsigned long HEARTBEAT_INTERVAL = 30000;  // 30 seconds

void sendHeartbeat() {
  unsigned long now = millis();
  if (now - lastHeartbeat >= HEARTBEAT_INTERVAL) {
    lastHeartbeat = now;

    #if USE_MQTT
      mqttPublish("heartbeat", "alive");
    #endif
  }
}
```

### Trigger External Effects

```cpp
void triggerEffect(const char* effect) {
  #if USE_MQTT
    // Send to effects controller
    String topic = "/escape/effects/" + String(effect);
    mqttPublish(topic.c_str(), "1");
  #endif
}

// Usage:
triggerEffect("lightning");
triggerEffect("fog");
triggerEffect("sound/thunder");
```

---

## Project Structure

```
ESP32/
├── platformio.ini          # Build configuration
├── src/
│   ├── config.h            # ← EDIT THIS: All configuration
│   ├── main.cpp            # Callbacks for MQTT/ESP-NOW
│   ├── setup.cpp/h         # Module initialization
│   ├── loop.cpp/h          # Main loop
│   ├── SampleFunction.cpp/h # ← REPLACE: Your puzzle logic
│   └── modules/
│       ├── wifi_module.cpp/h
│       ├── mqtt_module.cpp/h
│       ├── mdns_module.cpp/h
│       ├── ota_module.cpp/h
│       └── espnow_module.cpp/h
└── template_use.md         # This file
```

---

## Build Commands

```bash
# Build
pio run

# Upload via USB
pio run --target upload

# Upload via OTA
pio run --target upload --upload-port DeviceName.local

# Monitor serial output
pio device monitor

# Clean build
pio run --target clean
```

---

## Troubleshooting

### WiFi Won't Connect
- Check SSID/password in config.h
- Verify 2.4GHz network (ESP32 doesn't support 5GHz)
- Check serial output for connection status

### MQTT Not Connecting
- Verify broker IP is correct
- Check broker is running and accessible
- Ensure WiFi is connected first

### ESP-NOW Not Working
- Verify MAC addresses are correct
- Both devices must be on same WiFi channel (or both disconnected)
- Check serial output for initialization messages

### OTA Upload Fails
- Ensure device is on same network
- Check firewall isn't blocking
- Try using IP address instead of .local name

---

## Serial Output Example

```
========================================
Firmware: setup.cpp
Compiled: Nov 22 2025 15:30:00
Device:   LaserMaze
----------------------------------------
Modules Enabled:
  [X] WiFi
  [X] MQTT
  [X] mDNS
  [X] OTA
  [ ] ESP-NOW
----------------------------------------
[WiFi] Connecting to MyNetwork
..........
[WiFi] Connected! IP: 192.168.1.105
[WiFi] Task started on Core 0
[mDNS] Started: LaserMaze.local
[MQTT] Broker: 192.168.1.218:1883
[MQTT] Base topic: SP/LaserMaze
[MQTT] Connecting as client_LaserMaze... connected!
[MQTT] Subscribed to: SP/LaserMaze/cmd
[OTA] Ready. Hostname: LaserMaze
[OTA] Task started on Core 0
----------------------------------------
Network Info:
  IP Address: 192.168.1.105
  MAC:        AA:BB:CC:DD:EE:FF
  mDNS:       LaserMaze.local
  MQTT Topic: SP/LaserMaze
  MQTT:       Connected
  OTA:        Ready
========================================
[Setup] Complete. Starting main loop...
```
