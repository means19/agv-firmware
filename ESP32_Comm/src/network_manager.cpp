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

    // MQTT keep-alive: broker will consider us disconnected if we don't ping within this interval
    mqttClient.setKeepAlive(60);

    connectWifi();
}

// ─────────────────────────────────────────────
void NetworkManager::loop() {
    unsigned long now = millis();

    // 1. Manage WiFi connection — if we're not connected, try to reconnect every 5s and return immediately
    if (WiFi.status() != WL_CONNECTED) {
        if (now - lastReconnectTime >= 5000) {
            lastReconnectTime = now;
            connectWifi();
        }
        return;
    }

    // 2. Manage MQTT Non-blocking
    if (!mqttClient.connected()) {
        if (now - lastReconnectTime >= MQTT_RECONNECT_MS) {
            lastReconnectTime = now;
            connectMqtt();
        }
        return;
    }

    // 3. Process incoming messages
    mqttClient.loop();
}

// ─────────────────────────────────────────────
bool NetworkManager::publishState(const String& json) {
    if (!mqttClient.connected()) return false;
    return mqttClient.publish(topicState.c_str(), json.c_str());
}

// ─────────────────────────────────────────────
bool NetworkManager::publishConnection(const String& status) {
    if (!mqttClient.connected()) return false;
    String payload = "{\"connectionState\":\"" + status + "\"}";
    // retained=true so master control always knows our last connection status
    return mqttClient.publish(topicConnection.c_str(), payload.c_str(), true);
}

// ─────────────────────────────────────────────
bool NetworkManager::isConnected() {
    return mqttClient.connected();
}

// ─────────────────────────────────────────────
void NetworkManager::connectWifi() {
    Serial.println("[NET] Attempting WiFi connection...");
    
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

// ─────────────────────────────────────────────
void NetworkManager::connectMqtt() {
    if (WiFi.status() != WL_CONNECTED) return;

    Serial.print("[NET] Connecting to MQTT...");

    String lastWill = "{\"connectionState\":\"CONNECTIONBROKEN\"}";

    bool ok = mqttClient.connect(
        agvClientId.c_str(),
        nullptr, nullptr,
        topicConnection.c_str(), 1, true, lastWill.c_str()
    );

    if (ok) {
        Serial.println("connected");
        mqttClient.subscribe(topicOrder.c_str(),           0);
        mqttClient.subscribe(topicInstantActions.c_str(), 0);

#if USE_REAL_RFID == 0
        mqttClient.subscribe("test/rfid", 0);
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

    _instance->incoming.topic = String(topic);
    
    // PubSubClient's payload is not null-terminated, so we create a temporary buffer to convert it to a String safely.
    char* tempBuffer = new char[length + 1];
    memcpy(tempBuffer, payload, length);
    tempBuffer[length] = '\0';
    
    _instance->incoming.payload = String(tempBuffer);
    delete[] tempBuffer;
    
    _instance->incoming.hasNew = true;
}