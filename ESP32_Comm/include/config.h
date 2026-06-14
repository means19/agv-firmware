#pragma once
#include <Arduino.h>

// ── WiFi ──────────────────────────────────────────────────────────
#define WIFI_SSID     "Hieu"
#define WIFI_PASSWORD "12345678"

// ── MQTT broker ───────────────────────────────────────────────────
#define MQTT_SERVER   "10.163.11.221"
#define MQTT_PORT     1884

// ── AGV identity (Constant) ────────────────────────────────────────
#define AGV_MANUFACTURER "DATN"
#define AGV_MAP_ID       "map_1"
#define VDA_VERSION      "2.1.0"

// ── AGV identity (Dynamic - Will be generated automatically from MAC Address) ─────
extern String agvSerial;
extern String agvClientId;

// ── MQTT topics (Dynamic) ────────────────────────────────────────────
extern String topicOrder;
extern String topicInstantActions;
extern String topicState;
extern String topicConnection;

// ── Timing ────────────────────────────────────────────────────────
#define RFID_POLL_MS        100    
#define STATE_HEARTBEAT_MS  30000  
#define TAG_DEBOUNCE_MS     3000   
#define MQTT_RECONNECT_MS   5000

// ── RFID mode ─────────────────────────────────────────────────────
// 0 = simulation — tags come from "test/rfid" MQTT topic (Test_publisher.py)
// 1 = real hardware — tags come from the MFRC522 reader
//
// Keep this 0 until you have filled in readHardware() in RFID_reader.cpp
#define USE_REAL_RFID 1