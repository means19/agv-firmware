#pragma once
#include <Arduino.h>
#include <ArduinoJson.h>
#include "agv_types.h"
#include "STM32_comm.h"

// ─────────────────────────────────────────────────────────────────
//  order_manager
//
//  The main VDA 5050 logic module.
//
//  What it does:
//    1. Parses incoming "order" JSON into base nodes and horizon nodes
//    2. Parses "instantActions" (pause, cancel, etc.)
//    3. When the RFID reader scans a tag, marks the matching node as
//       traversed and updates the AGV state
//    4. Keeps a lookup table: physical tag UID → VDA 5050 nodeId
// ─────────────────────────────────────────────────────────────────

class OrderManager {
public:
    // Give order_manager a pointer to the shared state so it can update it.
    void begin(AGVState* state);

    // Call these from main.cpp when an MQTT message arrives
    void handleOrder(const String& json);
    void handleInstantAction(const String& json);

    // Call this from main.cpp when the RFID reader scans a tag
    void onTagRead(const String& tagId);

    // Add a mapping: physical RFID tag UID → VDA 5050 nodeId
    // Only needed when the tag UID is different from the nodeId.
    // Example: addTagMapping("A1:B2:C3:D4", "node_loading_dock")
    void addTagMapping(const String& tagUid, const String& nodeId);

    MOVE_cmd getNextMoveCommand();


private:
    void parseNodes(JsonDocument& doc);
    void parseEdges(JsonDocument& doc);
    Action parseAction(JsonObjectConst obj);

    // Resolve tag UID to nodeId (checks table first, then direct match)
    String resolveNodeId(const String& tagId);

    // Mark a node as traversed, update lastNodeId, trim the queues
    void traverseNode(const String& nodeId);

    // Update an action's status in the state
    void setActionStatus(const String& actionId, const String& status);

    // Fail all WAITING/RUNNING actions (called on cancelOrder)
    void failAllActions();

    // Rebuild state.nodeStates and state.edgeStates from internal arrays
    void rebuildState();

    // ── Internal node/edge storage ────────────────────────────────
    // base = released nodes (AGV can go here now)
    // horizon = unreleased nodes (waiting for master control to release)
    Node baseNodes[MAX_NODES];
    int  baseCount;
    Node horizonNodes[MAX_NODES];
    int  horizonCount;

    Edge baseEdges[MAX_EDGES];
    int  baseEdgeCount;
    Edge horizonEdges[MAX_EDGES];
    int  horizonEdgeCount;

    // ── Tag mapping table ─────────────────────────────────────────
    TagMapEntry tagMap[MAX_TAG_MAP];
    int         tagMapCount;

    // ── Order identity ────────────────────────────────────────────
    String   currentOrderId;
    uint32_t currentUpdateId;

    // ── Debounce: don't re-process the same tag twice in a row ────
    String        lastTagId;
    unsigned long lastTagTime;

    AGVState* state;  // pointer to the shared state in state_manager
};