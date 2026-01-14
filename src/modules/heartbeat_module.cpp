#include "heartbeat_module.h"
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <Adafruit_NeoPixel.h>

// Task configuration
#define HEARTBEAT_TASK_STACK 2048
#define HEARTBEAT_TASK_PRIORITY 1

// Timing constants (milliseconds)
#define SHORT_BLINK  100
#define LONG_BLINK   300
#define PULSE_GAP    100
#define PULSE_PAUSE  1500
#define SLOW_BLINK   1000
#define SOS_PAUSE    2000

// Default brightness (0-255) - keep low to avoid blinding
#define DEFAULT_BRIGHTNESS 10

// Colors (RGB values)
#define COLOR_OFF     0x000000
#define COLOR_RED     0xFF0000
#define COLOR_GREEN   0x00FF00
#define COLOR_BLUE    0x0000FF
#define COLOR_YELLOW  0xFFFF00
#define COLOR_CYAN    0x00FFFF

// Module state
static uint8_t _pin;
static volatile HeartbeatState _state = HB_BOOTING;
static TaskHandle_t _taskHandle = NULL;
static Adafruit_NeoPixel* _pixel = NULL;
static uint8_t _brightness = DEFAULT_BRIGHTNESS;

// Get color for current state
static uint32_t getStateColor() {
    switch (_state) {
        case HB_BOOTING:    return COLOR_BLUE;
        case HB_AP_ONLY:    return COLOR_YELLOW;
        case HB_CONNECTING: return COLOR_CYAN;
        case HB_NORMAL:     return COLOR_GREEN;
        case HB_ERROR:      return COLOR_RED;
        default:            return COLOR_BLUE;
    }
}

// Helper: set LED on/off with current state color
static void setLED(bool on) {
    if (_pixel) {
        if (on) {
            _pixel->setPixelColor(0, getStateColor());
        } else {
            _pixel->setPixelColor(0, COLOR_OFF);
        }
        _pixel->show();
    }
}

// Helper: single blink
static void blink(uint16_t on_ms, uint16_t off_ms) {
    setLED(true);
    vTaskDelay(pdMS_TO_TICKS(on_ms));
    setLED(false);
    vTaskDelay(pdMS_TO_TICKS(off_ms));
}

// Helper: pulse pattern (N quick blinks followed by pause)
static void pulsePattern(uint8_t count) {
    for (uint8_t i = 0; i < count; i++) {
        blink(SHORT_BLINK, PULSE_GAP);
    }
    vTaskDelay(pdMS_TO_TICKS(PULSE_PAUSE));
}

// Helper: SOS pattern (... --- ...)
static void sosPattern() {
    // S: 3 short
    for (int i = 0; i < 3; i++) blink(SHORT_BLINK, PULSE_GAP);
    vTaskDelay(pdMS_TO_TICKS(LONG_BLINK));
    // O: 3 long
    for (int i = 0; i < 3; i++) blink(LONG_BLINK, PULSE_GAP);
    vTaskDelay(pdMS_TO_TICKS(LONG_BLINK));
    // S: 3 short
    for (int i = 0; i < 3; i++) blink(SHORT_BLINK, PULSE_GAP);
    vTaskDelay(pdMS_TO_TICKS(SOS_PAUSE));
}

// FreeRTOS task running on Core 0
void heartbeatTask(void *param) {
    for (;;) {
        switch (_state) {
            case HB_BOOTING:
                blink(SHORT_BLINK, SHORT_BLINK);
                break;
            case HB_AP_ONLY:
                pulsePattern(2);
                break;
            case HB_CONNECTING:
                pulsePattern(3);
                break;
            case HB_NORMAL:
                blink(SLOW_BLINK, SLOW_BLINK);
                break;
            case HB_ERROR:
                sosPattern();
                break;
        }
    }
}

void heartbeatInit(uint8_t pin) {
    _pin = pin;

    // Create NeoPixel object (1 LED, GRB format for WS2812)
    _pixel = new Adafruit_NeoPixel(1, _pin, NEO_GRB + NEO_KHZ800);
    _pixel->begin();
    _pixel->setBrightness(_brightness);
    _pixel->setPixelColor(0, COLOR_OFF);
    _pixel->show();

    xTaskCreatePinnedToCore(
        heartbeatTask,
        "Heartbeat",
        HEARTBEAT_TASK_STACK,
        NULL,
        HEARTBEAT_TASK_PRIORITY,
        &_taskHandle,
        0  // Core 0
    );
}

void heartbeatSetState(HeartbeatState state) {
    _state = state;  // Atomic for single-byte enum
}

HeartbeatState heartbeatGetState() {
    return _state;
}

const char* heartbeatGetStateString() {
    switch (_state) {
        case HB_BOOTING:    return "Booting";
        case HB_AP_ONLY:    return "AP Only";
        case HB_CONNECTING: return "Connecting";
        case HB_NORMAL:     return "Normal";
        case HB_ERROR:      return "Error";
        default:            return "Unknown";
    }
}

void heartbeatSetBrightness(uint8_t brightness) {
    _brightness = brightness;
    if (_pixel) {
        _pixel->setBrightness(_brightness);
    }
}
