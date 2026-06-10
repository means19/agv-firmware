#include "agv_identity.h"
#include "config.h"
#include <WiFi.h>
#include <Arduino.h>

// 1. Initialize global variables for AGV identity and MQTT topics (will be set in initAgvIdentity())
String agvSerial = "";
String agvClientId = "";
String topicOrder = "";
String topicInstantActions = "";
String topicState = "";
String topicConnection = "";

// 2. Initialize AGV identity and MQTT topics based on ESP32's MAC address
void initAgvIdentity() {
    // Get the MAC address of the ESP32 (e.g., "24:6F:28:AB:CD:EF")
    String mac = WiFi.macAddress();
    
    // Remove the colons (resulting in "246F28ABCDEF")
    mac.replace(":", "");
    
    // Get the last 4 characters as the serial
    agvSerial = mac.substring(mac.length() - 4); 
    
    // Create the client ID and topics
    agvClientId = "esp32_agv_" + agvSerial;
    
    String baseTopic = "uagv/v2/" + String(AGV_MANUFACTURER) + "/" + agvSerial;
    topicOrder          = baseTopic + "/order";
    topicInstantActions = baseTopic + "/instantActions";
    topicState          = baseTopic + "/state";
    topicConnection     = baseTopic + "/connection";

    Serial.println("[IDENTITY] MAC Address: " + mac);
    Serial.println("[IDENTITY] Assigned Serial: " + agvSerial);
    Serial.println("[IDENTITY] Base Topic: " + baseTopic);
}