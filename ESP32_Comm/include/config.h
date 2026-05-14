#pragma once

// ── WiFi ──────────────────────────────────────────────────────────
#define WIFI_SSID     "NORMIES"
#define WIFI_PASSWORD "0378580982"

// ── MQTT broker ───────────────────────────────────────────────────
#define MQTT_SERVER   "10.17.227.227"
#define MQTT_PORT     1884

// ── AGV identity ──────────────────────────────────────────────────
#define AGV_CLIENT_ID    "esp32_agv_1"
#define AGV_SERIAL       "0001"
#define AGV_MANUFACTURER "DATN"
#define AGV_MAP_ID       "floor_1"
#define VDA_VERSION      "2.1.0"

// ── MQTT topics ───────────────────────────────────────────────────
#define TOPIC_ORDER           "uagv/v2/DATN/0001/order"
#define TOPIC_INSTANT_ACTIONS "uagv/v2/DATN/0001/instantActions"
#define TOPIC_STATE           "uagv/v2/DATN/0001/state"
#define TOPIC_CONNECTION      "uagv/v2/DATN/0001/connection"

// ── Timing ────────────────────────────────────────────────────────
#define RFID_POLL_MS        100    // how often to poll the RFID reader (ms)
#define STATE_HEARTBEAT_MS  30000  // publish state every 30 s
#define TAG_DEBOUNCE_MS     3000   // ignore same tag for 3 s after first read
#define MQTT_RECONNECT_MS   5000   // wait 5 s between reconnect attempts

// ── RFID mode ─────────────────────────────────────────────────────
// 0 = simulation — tags come from "test/rfid" MQTT topic (Test_publisher.py)
// 1 = real hardware — tags come from the MFRC522 reader
//
// Keep this 0 until you have filled in readHardware() in RFID_reader.cpp
#define USE_REAL_RFID 0