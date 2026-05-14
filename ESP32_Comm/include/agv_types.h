#pragma once
#include <Arduino.h>

// ── Hard limits for static arrays ────────────────────────────────
// Increase these if your orders have more nodes/edges than this.
#define MAX_NODES      20
#define MAX_EDGES      20
#define MAX_ACTIONS     5
#define MAX_ERRORS      5
#define MAX_TAG_MAP    20

// ── Action status strings (VDA 5050 §6.11) ───────────────────────
// Used in Action.status and actionStates throughout the project.
#define ACTION_WAITING  "WAITING"
#define ACTION_RUNNING  "RUNNING"
#define ACTION_FINISHED "FINISHED"
#define ACTION_FAILED   "FAILED"

// ── Action attached to a node ─────────────────────────────────────
struct Action {
    String actionId;
    String actionType;  // e.g. "pick", "startCharging", "cancelOrder"
    String status;      // "WAITING" / "RUNNING" / "FINISHED" / "FAILED"
};

// ── One node the AGV will travel to ──────────────────────────────
// released = true  → base  (AGV is allowed to go here now)
// released = false → horizon (master control hasn't released it yet)
struct Node {
    String   nodeId;
    uint32_t sequenceId;
    float    x;
    float    y;
    bool     released;
    Action   actions[MAX_ACTIONS];
    int      actionCount;
};

// ── One edge connecting two nodes ────────────────────────────────
struct Edge {
    String   edgeId;
    uint32_t sequenceId;
    String   startNodeId;
    String   endNodeId;
    float    maxSpeed;
    bool     released;
};

// ── AGV position on the map ───────────────────────────────────────
struct Position {
    float  x;
    float  y;
    float  theta;   // orientation in radians
    String mapId;
    bool   initialized;
};

// ── Battery info ──────────────────────────────────────────────────
struct Battery {
    float batteryCharge;    // percent 0-100
    float batteryVoltage;
    bool  charging;
};

// ── One error to report back to master control ────────────────────
struct AGVError {
    String errorType;
    String errorDescription;
    String errorLevel;  // "WARNING" or "FATAL"
};

// ── RFID tag → nodeId mapping entry ──────────────────────────────
// Use this when the physical tag UID differs from the nodeId string.
// Example: tag "A1:B2:C3" maps to nodeId "node_loading_dock"
struct TagMapEntry {
    String tagUid;
    String nodeId;
};

// ── Full AGV state (owned by state_manager) ───────────────────────
// All modules read/write this struct directly — no getters/setters.
struct AGVState {
    // Current order info
    String   orderId;
    uint32_t orderUpdateId;

    // Last node the AGV passed
    String   lastNodeId;
    uint32_t lastNodeSequenceId;

    // Remaining nodes and edges to travel
    Node nodeStates[MAX_NODES];
    int  nodeCount;
    Edge edgeStates[MAX_EDGES];
    int  edgeCount;

    // Actions in progress
    Action actionStates[MAX_ACTIONS];
    int    actionStateCount;

    // Physical info
    Position position;
    Battery  battery;

    // Status flags
    bool driving;
    bool paused;
    bool newBaseRequest;  // true = AGV is running out of base nodes, ask for more

    // Active errors
    AGVError errors[MAX_ERRORS];
    int      errorCount;

    String operatingMode;  // "AUTOMATIC", "MANUAL", etc.
};