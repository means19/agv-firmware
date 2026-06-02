#pragma once
#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>

// ─────────────────────────────────────────────────────────────────
//  network_manager
//
//  Handles WiFi and MQTT connection.
//  Subscribes to "order" and "instantActions" topics.
//  Publishes AGV state to the "state" topic.
//
//  When a message arrives, it stores it so main.cpp can read it
//  each loop — no callbacks, no lambdas.
// ─────────────────────────────────────────────────────────────────

// Stores the last received MQTT message.
// main.cpp checks this each loop to know if something arrived.
struct IncomingMessage {
    String topic;
    String payload;
    bool   hasNew;  // true = unread message waiting
};

class NetworkManager {
public:
    void begin();
    void loop();

    // Send the AGV state JSON to the broker
    bool publishState(const String& json);

    // Send connection status ("ONLINE" or "OFFLINE")
    bool publishConnection(const String& status);

    bool isConnected();

    // main.cpp reads this to get incoming MQTT messages
    IncomingMessage incoming;

private:
    void connectWifi();
    void connectMqtt();

    // Static function required by PubSubClient — it cannot call a class method directly.
    // This just forwards the message to the class instance stored in _instance.
    static void mqttCallback(char* topic, byte* payload, unsigned int length);
    static NetworkManager* _instance;

    WiFiClient   wifiClient;
    PubSubClient mqttClient;

    unsigned long lastReconnectTime = 0;
};