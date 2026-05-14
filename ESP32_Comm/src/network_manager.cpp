#include "network_manager.h"
#include "config.h"

NetworkManager* NetworkManager::_instance = nullptr;

// ─────────────────────────────────────────────
void NetworkManager::begin() {
    _instance = this;
    incoming.hasNew = false;

    mqttClient.setClient(wifiClient);
    mqttClient.setServer(MQTT_SERVER, MQTT_PORT);
    mqttClient.setCallback(mqttCallback);
    mqttClient.setBufferSize(2048);  // large enough for VDA 5050 order messages

    connectWifi();
    connectMqtt();
}

// ─────────────────────────────────────────────
void NetworkManager::loop() {
    // Re-connect WiFi if dropped
    if (WiFi.status() != WL_CONNECTED) {
        connectWifi();
        return;
    }

    // Re-connect MQTT, but don't spam — wait between attempts
    if (!mqttClient.connected()) {
        unsigned long now = millis();
        if (now - lastReconnectTime >= MQTT_RECONNECT_MS) {
            lastReconnectTime = now;
            connectMqtt();
        }
        return;
    }

    // Let PubSubClient process incoming messages
    mqttClient.loop();
}

// ─────────────────────────────────────────────
bool NetworkManager::publishState(const String& json) {
    if (!mqttClient.connected()) return false;
    return mqttClient.publish(TOPIC_STATE, json.c_str());
}

// ─────────────────────────────────────────────
bool NetworkManager::publishConnection(const String& status) {
    if (!mqttClient.connected()) return false;
    String payload = "{\"connectionState\":\"" + status + "\"}";
    // retained=true so master control always knows our last connection status
    return mqttClient.publish(TOPIC_CONNECTION, payload.c_str(), true);
}

// ─────────────────────────────────────────────
bool NetworkManager::isConnected() {
    return mqttClient.connected();
}

// ─────────────────────────────────────────────
void NetworkManager::connectWifi() {
    Serial.print("[NET] Connecting to WiFi");
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20) {
        delay(500);
        Serial.print(".");
        attempts++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\n[NET] WiFi OK: " + WiFi.localIP().toString());
    } else {
        Serial.println("\n[NET] WiFi failed, will retry");
    }
}

// ─────────────────────────────────────────────
void NetworkManager::connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.print("[NET] Connecting to MQTT...");

    // Last-will: broker sends this if we disconnect unexpectedly (VDA 5050 §6.14)
    String lastWill = "{\"connectionState\":\"CONNECTIONBROKEN\"}";

    bool ok = mqttClient.connect(
        AGV_CLIENT_ID,
        nullptr, nullptr,
        TOPIC_CONNECTION, 1, true, lastWill.c_str()
    );

    if (ok) {
        Serial.println("connected");
        mqttClient.subscribe(TOPIC_ORDER,           0);
        mqttClient.subscribe(TOPIC_INSTANT_ACTIONS, 0);

#if USE_REAL_RFID == 0
        mqttClient.subscribe("test/rfid", 0);  // fake RFID reads — only in test mode
#endif
        publishConnection("ONLINE");
    } else {
        Serial.print("failed, rc=");
        Serial.println(mqttClient.state());
    }
}

// ─────────────────────────────────────────────
// PubSubClient requires a plain static function for its callback.
// We store the class instance in _instance so we can reach it here.
void NetworkManager::mqttCallback(char* topic, byte* payload, unsigned int length) {
    if (_instance == nullptr) return;

    _instance->incoming.topic   = String(topic);
    _instance->incoming.payload = "";
    for (unsigned int i = 0; i < length; i++) {
        _instance->incoming.payload += (char)payload[i];
    }
    _instance->incoming.hasNew = true;
}