#include <Arduino.h>
#include "config.h"
#include "agv_types.h"
#include "network_manager.h"
#include "order_manager.h"
#include "state_manager.h"
#include "RFID_reader.h"


#define RX_PIN 16
#define TX_PIN 17

#define AGV_LOST_PIN 4
#define AGV_OBJ_DETECT_PIN 5


// ── Module instances ──────────────────────────────────────────────
NetworkManager network;
StateManager   stateMgr;
OrderManager   orderMgr;
RfidManager    rfid;    // always compiled — but loop() does nothing until
                        // readHardware() is filled in and USE_REAL_RFID = 1

namespace {
bool lastLostHigh     = false;
bool lastObstacleHigh = false;
}

// ── Thin wrapper: state_manager calls this to publish without
//    needing to know about network_manager ─────────────────────────
bool publishState(const String& json) {
    return network.publishState(json);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, RX_PIN, TX_PIN);
    delay(500);
    Serial.println("[MAIN] AGV gateway starting...");

    pinMode(AGV_LOST_PIN, INPUT_PULLDOWN);
    pinMode(AGV_OBJ_DETECT_PIN, INPUT_PULLDOWN);

    stateMgr.begin();
    orderMgr.begin(&stateMgr.state);
    network.begin();

#if USE_REAL_RFID == 1
    rfid.init();
    Serial.println("[MAIN] RFID: real hardware");
#else
    Serial.println("[MAIN] RFID: simulation — tags via test/rfid MQTT topic");
#endif

    // Add tag mappings here if your tag UIDs differ from your nodeIds
    // Example: orderMgr.addTagMapping("A1:B2:C3:D4", "node_loading_dock");

    Serial.println("[MAIN] Ready");
}

// ─────────────────────────────────────────────────────────────────
void loop() {
    // 1. Keep WiFi + MQTT alive
    network.loop();

    bool lostHigh     = (digitalRead(AGV_LOST_PIN) == HIGH);
    bool obstacleHigh = (digitalRead(AGV_OBJ_DETECT_PIN) == HIGH);

    if (lostHigh && !lastLostHigh) {
        if (stateMgr.state.errorCount < MAX_ERRORS) {
            stateMgr.state.errors[stateMgr.state.errorCount++] = {
                "AGV_LOST", "AGV lost line/position", "FATAL"
            };
        }
        stateMgr.publishNow(publishState);
    }

    if (obstacleHigh && !lastObstacleHigh) {
        if (stateMgr.state.errorCount < MAX_ERRORS) {
            stateMgr.state.errors[stateMgr.state.errorCount++] = {
                "OBSTACLE", "Object detected", "WARNING"
            };
        }
        stateMgr.publishNow(publishState);
}

    lastLostHigh     = lostHigh;
    lastObstacleHigh = obstacleHigh;

    // 2. Route incoming MQTT messages
    if (network.incoming.hasNew) {
        String topic   = network.incoming.topic;
        String payload = network.incoming.payload;
        network.incoming.hasNew = false;

        if (topic == TOPIC_ORDER) {
            orderMgr.handleOrder(payload);
            stateMgr.publishNow(publishState);

        } else if (topic == TOPIC_INSTANT_ACTIONS) {
            orderMgr.handleInstantAction(payload);
            stateMgr.publishNow(publishState);

#if USE_REAL_RFID == 0
        } else if (topic == "test/rfid") {
            // Simulated tag from Test_publisher.py — treated as a real RFID read
            Serial.println("[SIM] Tag: " + payload);
            orderMgr.onTagRead(payload);
            stateMgr.publishNow(publishState);
#endif
        }
    }

#if USE_REAL_RFID == 1
    // 3. Poll real RFID hardware
    String tag = rfid.readHardware();
    
    if (tag != "") {
        Serial.printf("[MAIN] Real RFID tag scanned/mapped: %s\n", tag.c_str());
        orderMgr.onTagRead(tag);
        stateMgr.publishNow(publishState); // Push state to server
    }
#endif

    // 4. Heartbeat — publishes state every 30 s even if nothing changed
    stateMgr.loop(publishState);
}