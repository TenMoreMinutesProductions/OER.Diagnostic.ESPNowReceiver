# OER.Template.ESP32

ESP32-S3 PlatformIO template for Otherworld Escape Rooms props.

## Purpose

A starter template for building escape room props with ESP32-S3. Provides a modular architecture with WiFi, MQTT, mDNS, OTA updates, and ESP-NOW support—all conditionally compiled based on your needs.

## Getting Started

1. Clone or use this template to create a new prop repository
2. Edit `src/config.h` - set `DEVICE_IDENTIFIER` and enable needed modules
3. Add puzzle logic in `src/SampleFunction.cpp` or create new source files
4. Build and upload

## Hardware

**Target Board:** ESP32-S3-DevKitC-1

**Module:** ESP32-S3 N8R2
- 8MB Flash (QIO mode)
- 2MB PSRAM (QIO QSPI)
- Dual-core Xtensa LX7 @ 240MHz
- WiFi 802.11 b/g/n + Bluetooth 5 (LE)
- Native USB support

**Common Sensors & Components:**
- RFID readers (RC522, PN532)
- Keypads and buttons
- Reed switches and magnetic sensors
- IR sensors and break beams
- Load cells and pressure sensors
- Rotary encoders
- LED strips (WS2812B, etc.)
- Servos and solenoids for locks
- Audio modules (DFPlayer, DY-HV8F, etc.)

## Modules

Five conditionally-compiled modules in `src/modules/`:

| Module | Flag | Requires | Purpose |
|--------|------|----------|---------|
| WiFi | `USE_WIFI` | - | Auto-reconnect on Core 0 |
| MQTT | `USE_MQTT` | WiFi | Pub/sub with auto-status |
| mDNS | `USE_MDNS` | WiFi | `hostname.local` discovery |
| OTA | `USE_OTA` | WiFi | Over-the-air firmware updates |
| ESP-NOW | `USE_ESPNOW` | - | Low-latency P2P (host/client modes) |

## Typical Prop Architecture

```
┌─────────────┐     MQTT      ┌───────────┐
│  ESP32-S3   │◄─────────────►│  Node-RED │
│   Prop      │               │ Controller│
└─────────────┘               └───────────┘
      │                             │
      │ ESP-NOW                     │ Dashboard
      ▼                             ▼
┌─────────────┐               ┌───────────┐
│ Other Props │               │Game Master│
└─────────────┘               └───────────┘
```

## Build Commands

```bash
pio run                  # Build
pio run --target upload  # Upload to board
pio run --target clean   # Clean build
pio device monitor       # Serial monitor (115200 baud)

# OTA upload (when enabled)
pio run --target upload --upload-port DeviceName.local
```

## Notes

- The ESP32-S3 has different GPIO numbering than the original ESP32. Check pin assignments in `config.h`.
- PSRAM is enabled by default via build flags. Use `ps_malloc()` for PSRAM allocations.
- The S3 lacks DAC pins. Use I2S or external DAC for audio output.

## Documentation

See `template_use.md` for comprehensive module API documentation with code examples.
