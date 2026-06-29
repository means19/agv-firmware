#include <Arduino.h>
#include "config.h"
#include "agv_types.h"
#include "network_manager.h"
#include "order_manager.h"
#include "state_manager.h"
#include "RFID_reader.h"
#include "tag_map_config.h"
#include "agv_identity.h"

// ── Module instances ──────────────────────────────────────────────
NetworkManager network;
StateManager   stateMgr;
OrderManager   orderMgr;
RfidManager    rfid;    // always compiled — but loop() does nothing until
                        // readHardware() is filled in and USE_REAL_RFID = 1

namespace {
    // Lost line/position sensor state tracking for debouncing
    bool stableLostState = false;
    bool lastPhysicalLost = false;
    unsigned long lastLostDebounceTime = 0;

    // Obstacle detection sensor state tracking for debouncing
    bool stableObstacleState = false;
    bool lastPhysicalObstacle = false;
    unsigned long lastObstacleDebounceTime = 0;

    // Noise filtering time
    const unsigned long DEBOUNCE_DELAY_MS = 50;
}

// ── Thin wrapper: state_manager calls this to publish without
//    needing to know about network_manager ─────────────────────────
bool publishState(const String& json) {
    return network.publishState(json);
}

// ─────────────────────────────────────────────────────────────────
void setup() {
    Serial.begin(115200);
    
    // [ĐÃ SỬA] Sử dụng cổng UART và các chân RX/TX động được gọi từ config.h
    STM32_SERIAL.begin(115200, SERIAL_8N1, UART_RX_PIN, UART_TX_PIN);
    
    delay(500);
    Serial.println("[MAIN] AGV gateway starting...");

    // Cấu hình chân cảm biến (Lấy từ config.h)
    pinMode(AGV_LOST_PIN, INPUT_PULLDOWN);
    pinMode(AGV_OBJ_DETECT_PIN, INPUT_PULLDOWN);

    WiFi.mode(WIFI_STA);
    initAgvIdentity();

    stateMgr.begin();
    orderMgr.begin(&stateMgr.state);
    network.begin();

#if USE_REAL_RFID == 1
    rfid.init();
    Serial.println("[MAIN] RFID: real hardware");
#else
    Serial.println("[MAIN] RFID: simulation — tags via test/rfid MQTT topic");
#endif

    initTagMappings(orderMgr);

    Serial.println("[MAIN] Ready");
}

// ─────────────────────────────────────────────────────────────────
void loop() {
    // 1. Keep WiFi + MQTT alive
    network.loop();

    // 2. Debounce and handle physical sensor inputs (AGV lost, obstacle detection)
    // Read current physical states
    bool currentPhysicalLost = (digitalRead(AGV_LOST_PIN) == HIGH);
    bool currentPhysicalObstacle = (digitalRead(AGV_OBJ_DETECT_PIN) == HIGH);

    // --- Noise filtering for LOST LINE SIGNAL ---
    if (currentPhysicalLost != lastPhysicalLost) {
        lastLostDebounceTime = millis(); // Reset timer if signal changed
    }
    if ((millis() - lastLostDebounceTime) > DEBOUNCE_DELAY_MS) {
        // Signal has been stable for the debounce period
        if (currentPhysicalLost != stableLostState) {
            stableLostState = currentPhysicalLost; // Update stable state
            
            if (stableLostState) { // Upward edge: Just lost the line/position
                stateMgr.addError("AGV_LOST", "AGV lost line/position", "FATAL");
            } else {               // Downward edge: Just found the line/position
                stateMgr.clearError("AGV_LOST");
            }
            stateMgr.publishNow(publishState); // Notify server immediately of the change
        }
    }
    lastPhysicalLost = currentPhysicalLost;

    // --- Noise filtering for OBSTACLE SIGNAL ---
    if (currentPhysicalObstacle != lastPhysicalObstacle) {
        lastObstacleDebounceTime = millis();
    }
    if ((millis() - lastObstacleDebounceTime) > DEBOUNCE_DELAY_MS) {
        // Signal has been stable for the debounce period
        if (currentPhysicalObstacle != stableObstacleState) {
            stableObstacleState = currentPhysicalObstacle; 
            
            if (stableObstacleState) { // Upward edge: Object detected
                stateMgr.addError("OBSTACLE", "Object detected", "WARNING");
            } else {                   // Downward edge: Object removed
                stateMgr.clearError("OBSTACLE");
            }
            stateMgr.publishNow(publishState); // Notify server immediately of the change
        }
    }
    lastPhysicalObstacle = currentPhysicalObstacle;

    // 3. Route incoming MQTT messages
    if (network.incoming.hasNew) {
        String topic   = network.incoming.topic;
        String payload = network.incoming.payload;
        network.incoming.hasNew = false;

        if (topic == topicOrder) {
            orderMgr.handleOrder(payload);
            stateMgr.publishNow(publishState);

        } else if (topic == topicInstantActions) {
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
    // 3. Poll real RFID hardware (Có giới hạn tần suất quét để chống treo module)
    static unsigned long lastRfidPoll = 0;
    
    // Cứ mỗi RFID_POLL_MS (100ms) mới cho phép giao tiếp SPI với thẻ 1 lần
    if (millis() - lastRfidPoll >= RFID_POLL_MS) {
        lastRfidPoll = millis();
        
        String tag = rfid.readHardware();
        
        if (tag != "") {
            Serial.printf("[MAIN] Hardware read RAW UID: %s\n", tag.c_str());
            orderMgr.onTagRead(tag);
            stateMgr.publishNow(publishState); // Push state to server
        }
    }
#endif

    // 4. Heartbeat — publishes state every 30 s even if nothing changed
    stateMgr.loop(publishState);
}