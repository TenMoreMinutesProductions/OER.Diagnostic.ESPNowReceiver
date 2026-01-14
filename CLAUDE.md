# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Project Overview

This is an ESP32-S3 template project using PlatformIO with the Arduino framework.

**Target Board:** ESP32-S3-DevKitC-1 (N8R2 variant: 8MB Flash, 2MB PSRAM)

## Development Environment

**IDE:** JetBrains CLion with PlatformIO plugin

## Build Commands

```bash
pio run                    # Build
pio run --target upload    # Upload to ESP32-S3
pio run --target clean     # Clean build
pio device monitor         # Serial monitor (115200 baud)
```

## Getting Started

When creating a new prop from this template:

1. Set `PROJECT_NAME` in `config.h` to match your repo folder name
2. Set `DEVICE_IDENTIFIER` for mDNS/MQTT (e.g., `oer-ud-radio`)
3. Enable/disable modules as needed
4. Replace `SampleFunction.cpp` with your puzzle logic

## Project Structure

```
src/
├── main.cpp              → Entry points: setup() and loop()
├── config.h              → Project identity and module flags
├── setup.cpp/.h          → Module initialization, startup diagnostics
├── loop.cpp/.h           → Main loop dispatcher
├── callbacks.cpp/.h      → MQTT, ESP-NOW, and reset handlers
├── SampleFunction.cpp/.h → Hardware diagnostic (replace with puzzle logic)
└── modules/              → Conditionally-compiled modules
```

## Configuration (config.h)

```cpp
// Set these for your project:
#define PROJECT_NAME "OER.Room.YourRoom.YourProp"  // Repo folder name
#define DEVICE_IDENTIFIER "oer-room-prop"          // mDNS/MQTT name
```

## Module System

Six conditionally-compiled modules in `src/modules/`:

| Module | Flag | Requires | Purpose |
|--------|------|----------|---------|
| WiFi | `USE_WIFI` | - | Auto-reconnect on Core 0 |
| MQTT | `USE_MQTT` | WiFi | Pub/sub with auto-status |
| mDNS | `USE_MDNS` | WiFi | `hostname.local` discovery |
| OTA | `USE_OTA` | WiFi | Over-the-air firmware updates |
| ESP-NOW | `USE_ESPNOW` | - | Low-latency P2P (host/client) |
| Heartbeat | `USE_HEARTBEAT` | - | LED status indicator (FreeRTOS) |

## Hardware Diagnostic (SampleFunction.cpp)

The included `SampleFunction.cpp` is a **hardware diagnostic tool** for verifying your ESP32-S3 is working correctly. It displays every 100ms:

- Heap memory usage (internal RAM)
- PSRAM usage (external RAM)
- Per-core CPU usage
- Active FreeRTOS tasks and their core affinity

**After confirming the ESP32 works:**
1. Delete `SampleFunction.cpp` and `SampleFunction.h`
2. Create your puzzle logic files
3. Update `loop.cpp` to call your new function

## Heartbeat Module

Visual status indicator via LED, runs as FreeRTOS task on Core 0.

| State | Pattern | Meaning |
|-------|---------|---------|
| `HB_BOOTING` | Fast blink (100ms) | Initializing |
| `HB_AP_ONLY` | Double pulse | Awaiting config |
| `HB_CONNECTING` | Triple pulse | Connecting to WiFi |
| `HB_NORMAL` | Slow blink (1000ms) | Normal operation |
| `HB_ERROR` | SOS pattern | Error state |

## Callbacks (callbacks.cpp)

All event handlers are in `callbacks.cpp`:

- `onMqttMessage()` - Handle incoming MQTT messages
- `onEspNowReceive()` - Handle ESP-NOW data
- `onEspNowSend()` - ESP-NOW send confirmation
- `onPropReset()` - Reset button or MQTT reset command

## ESP32-S3 Notes

- Different GPIO numbering than original ESP32—verify pin mappings
- PSRAM enabled via build flags; use `ps_malloc()` for PSRAM allocations
- No DAC pins—use I2S or external DAC for audio
- Native USB support available
- Onboard RGB LED on GPIO48 for heartbeat
