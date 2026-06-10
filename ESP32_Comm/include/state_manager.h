#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "agv_types.h"
#include "config.h"

// ─────────────────────────────────────────────────────────────────
//  state_manager
//
//  Owns the AGVState struct and serializes it to JSON for publishing.
//  Two publish triggers:
//    1. publishNow()  — called immediately after any state change
//    2. loop()        — fires a heartbeat every 30 s (VDA 5050 §6.10)
// ─────────────────────────────────────────────────────────────────

class StateManager {
public:
    AGVState state;  // public — all modules read/write this directly

    void begin() {
        headerId = 0;
        lastPublishTime = 0;

        // Set initial state values
        state.orderId          = "";
        state.orderUpdateId    = 0;
        state.lastNodeId       = "";
        state.nodeCount        = 0;
        state.edgeCount        = 0;
        state.actionStateCount = 0;
        state.errorCount       = 0;
        state.driving          = false;
        state.paused           = false;
        state.newBaseRequest   = false;
        state.operatingMode    = "AUTOMATIC";
        state.position         = { 0, 0, 0, AGV_MAP_ID, false };
        state.battery          = { 100.0f, 0.0f, false };
    }

    void addError(const String& type, const String& desc, const String& level) {
        // 1. Check if error of the same type already exists — if yes, skip to avoid duplicates
        for (int i = 0; i < state.errorCount; i++) {
            if (state.errors[i].errorType == type) return; 
        }

        // 2. If not, add new error
        if (state.errorCount < MAX_ERRORS) {
            state.errors[state.errorCount].errorType = type;
            state.errors[state.errorCount].errorDescription = desc;
            state.errors[state.errorCount].errorLevel = level;
            state.errorCount++;
        }
    }

    // Clears an error of the specified type (if it exists)
    void clearError(const String& type) {
        for (int i = 0; i < state.errorCount; i++) {
            if (state.errors[i].errorType == type) {
                // Find the error and remove it by shifting subsequent errors up
                for (int j = i; j < state.errorCount - 1; j++) {
                    state.errors[j] = state.errors[j + 1];
                }
                // Clear the last error slot (now a duplicate after shifting)
                state.errorCount--;
                return;
            }
        }
    }
    // ────────────────────────────────────────────────────────

    // Build JSON and return it as a String (network_manager will publish it)
    String buildJson() {
        // ArduinoJson v7: Chỉ dùng JsonDocument
        JsonDocument doc;

        // Header
        doc["headerId"]     = headerId++;
        doc["version"]      = VDA_VERSION;
        doc["manufacturer"] = AGV_MANUFACTURER;
        doc["serialNumber"] = agvSerial; 

        // Order info
        doc["orderId"]            = state.orderId;
        doc["orderUpdateId"]      = state.orderUpdateId;
        doc["lastNodeId"]         = state.lastNodeId;
        doc["lastNodeSequenceId"] = state.lastNodeSequenceId;

        // Remaining nodes to traverse
        JsonArray nodeArr = doc["nodeStates"].to<JsonArray>();
        for (int i = 0; i < state.nodeCount; i++) {
            JsonObject n = nodeArr.add<JsonObject>();
            n["nodeId"]     = state.nodeStates[i].nodeId;
            n["sequenceId"] = state.nodeStates[i].sequenceId;
            n["released"]   = state.nodeStates[i].released;
        }

        // Remaining edges
        JsonArray edgeArr = doc["edgeStates"].to<JsonArray>();
        for (int i = 0; i < state.edgeCount; i++) {
            JsonObject e = edgeArr.add<JsonObject>();
            e["edgeId"]     = state.edgeStates[i].edgeId;
            e["sequenceId"] = state.edgeStates[i].sequenceId;
            e["released"]   = state.edgeStates[i].released;
        }

        // Position
        JsonObject pos = doc["agvPosition"].to<JsonObject>();
        pos["x"]           = state.position.x;
        pos["y"]           = state.position.y;
        pos["theta"]       = state.position.theta;
        pos["mapId"]       = state.position.mapId;
        pos["initialized"] = state.position.initialized;

        // Action states
        JsonArray actArr = doc["actionStates"].to<JsonArray>();
        for (int i = 0; i < state.actionStateCount; i++) {
            JsonObject a = actArr.add<JsonObject>();
            a["actionId"]     = state.actionStates[i].actionId;
            a["actionType"]   = state.actionStates[i].actionType;
            a["actionStatus"] = state.actionStates[i].status;
        }

        // Battery
        JsonObject bat = doc["batteryState"].to<JsonObject>();
        bat["batteryCharge"]  = state.battery.batteryCharge;
        bat["batteryVoltage"] = state.battery.batteryVoltage;
        bat["charging"]       = state.battery.charging;

        // Flags
        doc["driving"]        = state.driving;
        doc["paused"]         = state.paused;
        doc["newBaseRequest"] = state.newBaseRequest;
        doc["operatingMode"]  = state.operatingMode;

        // Errors
        JsonArray errArr = doc["errors"].to<JsonArray>();
        for (int i = 0; i < state.errorCount; i++) {
            JsonObject e = errArr.add<JsonObject>();
            e["errorType"]        = state.errors[i].errorType;
            e["errorDescription"] = state.errors[i].errorDescription;
            e["errorLevel"]       = state.errors[i].errorLevel;
        }

        String output;
        serializeJson(doc, output);
        return output;
    }

    // Call every loop() — handles the 30 s heartbeat
    // publishFn is provided by main.cpp to avoid a circular dependency
    void loop(bool (*publishFn)(const String&)) {
        if (millis() - lastPublishTime >= STATE_HEARTBEAT_MS) {
            publishNow(publishFn);
        }
    }

    void publishNow(bool (*publishFn)(const String&)) {
        String json = buildJson();
        if (publishFn(json)) {
            lastPublishTime = millis();
            Serial.println("[STATE] Published state");
        } else {
            Serial.println("[STATE] Publish failed (not connected?)");
        }
    }

private:
    uint32_t      headerId;
    unsigned long lastPublishTime;
};