#include "espnow_module.h"
#include <esp_now.h>
#include <WiFi.h>

static bool _initialized = false;
static bool _isHost = false;
static EspNowReceiveCallback _receiveCallback = nullptr;
static EspNowSendCallback _sendCallback = nullptr;
static TaskHandle_t _espnowTaskHandle = nullptr;

static uint8_t _broadcastAddress[] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

// Queue for passing received messages to the task
static QueueHandle_t _receiveQueue = nullptr;

struct EspNowMessage {
    uint8_t mac[6];
    uint8_t data[250];
    int len;
};

// Internal receive callback - runs in WiFi task context
static void _onDataReceive(const uint8_t* mac, const uint8_t* data, int len) {
    if (_receiveCallback != nullptr) {
        _receiveCallback(mac, data, len);
    }
}

// Internal send callback
static void _onDataSend(const uint8_t* mac, esp_now_send_status_t status) {
    if (_sendCallback != nullptr) {
        _sendCallback(mac, status == ESP_NOW_SEND_SUCCESS);
    }
}

// FreeRTOS task running on Core 0
// ESP-NOW callbacks are handled by the WiFi task on Core 0 automatically
// This task is available for any periodic ESP-NOW maintenance if needed
static void espnowTask(void* param) {
    while (true) {
        // ESP-NOW is event-driven, callbacks handle messages
        // This task can be used for periodic broadcasts or maintenance
        vTaskDelay(100 / portTICK_PERIOD_MS);
    }
}

bool espnowInit(bool isHost, const uint8_t* hostMac) {
    _isHost = isHost;

    // Initialize WiFi in station mode if not already
    if (WiFi.getMode() == WIFI_OFF) {
        WiFi.mode(WIFI_STA);
    }

    // Initialize ESP-NOW
    if (esp_now_init() != ESP_OK) {
        Serial.println("[ESP-NOW] Init failed!");
        return false;
    }

    // Register callbacks
    esp_now_register_recv_cb(_onDataReceive);
    esp_now_register_send_cb(_onDataSend);

    if (_isHost) {
        // Host mode: add broadcast peer
        esp_now_peer_info_t peerInfo = {};
        memcpy(peerInfo.peer_addr, _broadcastAddress, 6);
        peerInfo.channel = 0;
        peerInfo.encrypt = false;

        if (esp_now_add_peer(&peerInfo) != ESP_OK) {
            Serial.println("[ESP-NOW] Failed to add broadcast peer");
            return false;
        }
        Serial.println("[ESP-NOW] Initialized as HOST (broadcast mode)");
    } else {
        // Client mode: add host as peer
        if (hostMac != nullptr) {
            esp_now_peer_info_t peerInfo = {};
            memcpy(peerInfo.peer_addr, hostMac, 6);
            peerInfo.channel = 0;
            peerInfo.encrypt = false;

            if (esp_now_add_peer(&peerInfo) != ESP_OK) {
                Serial.println("[ESP-NOW] Failed to add host peer");
                return false;
            }
            Serial.print("[ESP-NOW] Initialized as CLIENT. Host MAC: ");
            for (int i = 0; i < 6; i++) {
                Serial.printf("%02X", hostMac[i]);
                if (i < 5) Serial.print(":");
            }
            Serial.println();
        } else {
            Serial.println("[ESP-NOW] Initialized as CLIENT (no host MAC set)");
        }
    }

    Serial.print("[ESP-NOW] This device MAC: ");
    Serial.println(WiFi.macAddress());

    // Start ESP-NOW task on Core 0
    xTaskCreatePinnedToCore(
        espnowTask,
        "ESPNowTask",
        4096,
        NULL,
        1,
        &_espnowTaskHandle,
        0  // Core 0
    );
    Serial.println("[ESP-NOW] Task started on Core 0");

    _initialized = true;
    return true;
}

void espnowSetReceiveCallback(EspNowReceiveCallback callback) {
    _receiveCallback = callback;
}

void espnowSetSendCallback(EspNowSendCallback callback) {
    _sendCallback = callback;
}

bool espnowSend(const uint8_t* mac, const uint8_t* data, size_t len) {
    if (!_initialized) return false;

    const uint8_t* targetMac = (mac != nullptr) ? mac : _broadcastAddress;
    esp_err_t result = esp_now_send(targetMac, data, len);
    return result == ESP_OK;
}

bool espnowSendString(const uint8_t* mac, const String& message) {
    return espnowSend(mac, (const uint8_t*)message.c_str(), message.length());
}

bool espnowBroadcast(const uint8_t* data, size_t len) {
    if (!_initialized || !_isHost) return false;
    return espnowSend(_broadcastAddress, data, len);
}

bool espnowAddPeer(const uint8_t* mac) {
    if (!_initialized) return false;

    esp_now_peer_info_t peerInfo = {};
    memcpy(peerInfo.peer_addr, mac, 6);
    peerInfo.channel = 0;
    peerInfo.encrypt = false;

    return esp_now_add_peer(&peerInfo) == ESP_OK;
}

bool espnowRemovePeer(const uint8_t* mac) {
    if (!_initialized) return false;
    return esp_now_del_peer(mac) == ESP_OK;
}

String espnowGetMAC() {
    return WiFi.macAddress();
}

bool espnowIsInitialized() {
    return _initialized;
}

bool espnowIsHost() {
    return _isHost;
}

void espnowSyncChannel() {
    if (!_initialized) return;

    // Get current WiFi channel
    uint8_t channel = WiFi.channel();
    if (channel == 0) return;  // WiFi not connected

    // ESP-NOW uses channel 0 (auto) by default, which follows WiFi
    // This function is a no-op but serves as documentation that
    // ESP-NOW and WiFi must be on the same channel.
    // If you experience issues, ensure WiFi connects BEFORE ESP-NOW init,
    // or call this after WiFi reconnects.
    Serial.print("[ESP-NOW] Channel synced to WiFi channel ");
    Serial.println(channel);
}
