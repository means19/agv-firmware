#pragma once
#include <Arduino.h>

// ── Board Selection ───────────────────────────────────────────────
// Chọn board bạn đang sử dụng bằng cách thay đổi giá trị BOARD_TYPE:
// 1 = ESP32 tiêu chuẩn (VD: upesy_wroom)
// 2 = ESP32-C3         (VD: esp32-c3-devkitm-1)
#define BOARD_TYPE 1

#if BOARD_TYPE == 1
    // ── Pinout cho ESP32 WROOM ────────────────────────────────────
    #define UART_RX_PIN        16
    #define UART_TX_PIN        17
    #define STM32_SERIAL       Serial2  // WROOM dùng Serial2

    #define RFID_SCK_PIN       18
    #define RFID_MISO_PIN      19
    #define RFID_MOSI_PIN      23
    #define RFID_SS_PIN        5
    #define RFID_RST_PIN       22

    #define AGV_LOST_PIN       4
    #define AGV_OBJ_DETECT_PIN 13

#elif BOARD_TYPE == 2
    // ── Pinout cho ESP32-C3 ───────────────────────────────────────
    #define UART_RX_PIN        20
    #define UART_TX_PIN        21
    #define STM32_SERIAL       Serial1  // C3 dùng Serial1

    #define RFID_SCK_PIN       4
    #define RFID_MISO_PIN      5
    #define RFID_MOSI_PIN      6
    #define RFID_SS_PIN        7
    #define RFID_RST_PIN       10

    #define AGV_LOST_PIN       2
    #define AGV_OBJ_DETECT_PIN 3
#endif


// ── WiFi ──────────────────────────────────────────────────────────
#define WIFI_SSID     "Hieu"
#define WIFI_PASSWORD "12345678"

// ── MQTT broker ───────────────────────────────────────────────────
#define MQTT_SERVER   "10.178.236.221"
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
extern String topicDebug;

// ── Timing ────────────────────────────────────────────────────────
#define RFID_POLL_MS        20 
#define STATE_HEARTBEAT_MS  30000  
#define TAG_DEBOUNCE_MS     3000   
#define MQTT_RECONNECT_MS   5000

// ── RFID mode ─────────────────────────────────────────────────────
// 0 = simulation — tags come from "test/rfid" MQTT topic (Test_publisher.py)
// 1 = real hardware — tags come from the MFRC522 reader
//
// Keep this 0 until you have filled in readHardware() in RFID_reader.cpp
#define USE_REAL_RFID 1