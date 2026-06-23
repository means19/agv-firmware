#include "order_manager.h"
#include "STM32_comm.h"
#include "config.h"
#include <Arduino.h>
#include "network_manager.h"

extern NetworkManager network;

// ─────────────────────────────────────────────
void OrderManager::begin(AGVState* sharedState) {
    state            = sharedState;
    baseCount        = 0;
    horizonCount     = 0;
    baseEdgeCount    = 0;
    horizonEdgeCount = 0;
    tagMapCount      = 0;
    currentUpdateId  = 0;
    lastTagTime      = 0;
}

// ─────────────────────────────────────────────
void OrderManager::addTagMapping(const String& tagUid, const String& nodeId) {
    if (tagMapCount >= MAX_TAG_MAP) return;
    tagMap[tagMapCount].tagUid = tagUid;
    tagMap[tagMapCount].nodeId = nodeId;
    tagMapCount++;
}

// ─────────────────────────────────────────────────────────────────
//  Handle incoming "order" message from master control
// ─────────────────────────────────────────────────────────────────
void OrderManager::handleOrder(const String& json) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, json);
    if (err) {
        Serial.println("[ORDER] Bad JSON: " + String(err.c_str()));
        return;
    }

    String   newOrderId  = doc["orderId"]       | String("");
    uint32_t newUpdateId = doc["orderUpdateId"] | 0u;

    if (newOrderId.isEmpty()) {
        Serial.println("[ORDER] Missing orderId, ignored");
        return;
    }

    // Reject an older update for the same order
    if (newOrderId == currentOrderId && newUpdateId < currentUpdateId) {
        Serial.println("[ORDER] Old orderUpdateId, rejected");
        return;
    }

    // Duplicate — already have this update, ignore
    if (newOrderId == currentOrderId && newUpdateId == currentUpdateId) {
        Serial.println("[ORDER] Duplicate order, ignored");
        return;
    }

    bool isUpdate = (newOrderId == currentOrderId);

    if (!isUpdate) {
        // Brand new order — clear everything
        baseCount        = 0;
        horizonCount     = 0;
        baseEdgeCount    = 0;
        horizonEdgeCount = 0;
        state->actionStateCount = 0;
        state->errorCount       = 0;
        currentOrderId   = newOrderId;
    }

    currentUpdateId = newUpdateId;

    parseNodes(doc);
    parseEdges(doc);
    rebuildState();

    state->orderId       = currentOrderId;
    state->orderUpdateId = currentUpdateId;

    Serial.printf("[ORDER] %s '%s' update=%u base=%d horizon=%d\n",
        isUpdate ? "Updated" : "New order",
        currentOrderId.c_str(), currentUpdateId,
        baseCount, horizonCount);
}

// ─────────────────────────────────────────────
void OrderManager::parseNodes(JsonDocument& doc) {
    for (JsonObjectConst n : doc["nodes"].as<JsonArrayConst>()) {
        Node node;
        node.nodeId      = n["nodeId"]     | String("");
        node.sequenceId  = n["sequenceId"] | 0u;
        
        // [FIX LỖI PARSING] Đọc x, y từ object con "nodePosition" chuẩn VDA 5050
        if (n.containsKey("nodePosition")) {
            node.x = n["nodePosition"]["x"] | 0.0f;
            node.y = n["nodePosition"]["y"] | 0.0f;
        } else {
            node.x = 0.0f;
            node.y = 0.0f;
        }

        node.released    = n["released"]   | false;
        node.actionCount = 0;

        for (JsonObjectConst a : n["actions"].as<JsonArrayConst>()) {
            if (node.actionCount < MAX_ACTIONS) {
                node.actions[node.actionCount++] = parseAction(a);
            }
        }

        if (node.released) {
            if (baseCount < MAX_NODES) baseNodes[baseCount++] = node;
        } else {
            if (horizonCount < MAX_NODES) horizonNodes[horizonCount++] = node;
        }
    }
}

// ─────────────────────────────────────────────
void OrderManager::parseEdges(JsonDocument& doc) {
    for (JsonObjectConst e : doc["edges"].as<JsonArrayConst>()) {
        Edge edge;
        edge.edgeId      = e["edgeId"]      | String("");
        edge.sequenceId  = e["sequenceId"]  | 0u;
        edge.startNodeId = e["startNodeId"] | String("");
        edge.endNodeId   = e["endNodeId"]   | String("");
        edge.maxSpeed    = e["maxSpeed"]    | 1.0f;
        edge.released    = e["released"]    | false;

        if (edge.released) {
            if (baseEdgeCount < MAX_EDGES) baseEdges[baseEdgeCount++] = edge;
        } else {
            if (horizonEdgeCount < MAX_EDGES) horizonEdges[horizonEdgeCount++] = edge;
        }
    }
}

// ─────────────────────────────────────────────
Action OrderManager::parseAction(JsonObjectConst obj) {
    Action a;
    a.actionId   = obj["actionId"]   | String("");
    a.actionType = obj["actionType"] | String("");
    a.status     = ACTION_WAITING;
    return a;
}

// ─────────────────────────────────────────────────────────────────
//  Handle incoming "instantActions" message
// ─────────────────────────────────────────────────────────────────
void OrderManager::handleInstantAction(const String& json) {
    JsonDocument doc;
    if (deserializeJson(doc, json)) {
        Serial.println("[ORDER] Bad instantAction JSON");
        return;
    }

    for (JsonObjectConst a : doc["actions"].as<JsonArrayConst>()) {
        String type = a["actionType"] | String("");
        String id   = a["actionId"]   | String("");

        Serial.println("[ORDER] instantAction: " + type);

        if (type == "startPause") {
            state->paused = true;
            setActionStatus(id, ACTION_FINISHED);

        } else if (type == "stopPause") {
            state->paused = false;
            setActionStatus(id, ACTION_FINISHED);

        } else if (type == "cancelOrder") {
            if (currentOrderId.isEmpty()) {
                // No active order to cancel
                setActionStatus(id, ACTION_FAILED);
            } else {
                failAllActions();
                baseCount        = 0;
                horizonCount     = 0;
                baseEdgeCount    = 0;
                horizonEdgeCount = 0;
                state->driving   = false;
                rebuildState();
                setActionStatus(id, ACTION_FINISHED);
                Serial.println("[ORDER] Order cancelled");
            }

        } else if (type == "stateRequest") {
            // Just trigger a state publish — handled in main.cpp by checking hasNew
            setActionStatus(id, ACTION_FINISHED);

        } else {
            Serial.println("[ORDER] Unknown instantAction: " + type);
            setActionStatus(id, ACTION_FAILED);
        }
    }
}

// ─────────────────────────────────────────────────────────────────
//  RFID tag read — the main event that drives node traversal
// ─────────────────────────────────────────────────────────────────
void OrderManager::onTagRead(const String& tagId) {
    
    // 1. BẮT BUỘC: Tra bảng Map ngay lập tức khi đọc được mã UID thô
    String nodeId = resolveNodeId(tagId);

    // Hàm resolveNodeId sẽ trả về chính chuỗi gốc nếu không tìm thấy trong Map
    if (nodeId == tagId) {
        Serial.println("[ORDER] UID chưa được khai báo trong tag_map_config: " + tagId);
        return;
    }

    // 2. In Log thông báo đã MAP thành công để giám sát trên máy tính
    Serial.println("[ORDER] MAP THÀNH CÔNG! UID: " + tagId + " ---> Node: " + nodeId);

    // 3. Tính năng mới: Luôn cập nhật vị trí hiện tại của xe (Localization)
    // Dù xe đang đứng chơi (chưa có Order), dẫm lên thẻ là phải biết mình ở đâu
    state->lastNodeId = nodeId;
    state->position.initialized = true;

    // 4. Nếu xe đang dừng hoặc KHÔNG CÓ LỘ TRÌNH (Order), thì chỉ cập nhật vị trí rồi thoát
    if (state->paused || baseCount == 0) {
        Serial.println("[ORDER] Xe đang rảnh rỗi. Đã cập nhật tọa độ mới lên Server.");
        return;
    }

    // 5. Nếu xe ĐANG CHẠY THEO LỘ TRÌNH (Có Order) thì thực thi thuật toán chạy tiếp
    // Debounce — same tag scanned twice quickly, ignore the second
    if (nodeId == lastTagId && (millis() - lastTagTime) < TAG_DEBOUNCE_MS) {
        return;
    }
    
    lastTagId   = nodeId;
    lastTagTime = millis();

    // Tính toán góc rẽ và gửi lệnh xuống STM32
    traverseNode(nodeId);
}

// ─────────────────────────────────────────────
String OrderManager::resolveNodeId(const String& tagId) {
    // Check the mapping table first
    for (int i = 0; i < tagMapCount; i++) {
        if (tagMap[i].tagUid == tagId) return tagMap[i].nodeId;
    }
    // If not in table, assume the tag UID is the nodeId directly
    return tagId;
}

// ─────────────────────────────────────────────
void OrderManager::traverseNode(const String& nodeId) {
    // Check if this tag matches the next expected base node
    if (baseNodes[0].nodeId == nodeId) {

        // 1. Tính toán lệnh di chuyển NGAY BÂY GIỜ
        MOVE_cmd cmd = getNextMoveCommand();

        // 2. In Log Lệnh Di Chuyển ra Serial Monitor để kiểm tra
        Serial.print("[KINEMATICS] Node: ");
        Serial.print(nodeId);
        Serial.print(" -> Lệnh gửi STM32: ");
        
        switch (cmd) {
            case CMD_FORWARD: Serial.println("ĐI THẲNG (0x00)"); break;
            case CMD_LEFT:    Serial.println("RẼ TRÁI (0x01)"); break;
            case CMD_RIGHT:   Serial.println("RẼ PHẢI (0x02)"); break;
            case CMD_STOP:    Serial.println("DỪNG LẠI (0x03)"); break;
            case CMD_ROTATE:  Serial.println("XOAY TẠI CHỖ (0x04)"); break;
            default:          Serial.println("KHÔNG XÁC ĐỊNH"); break;
        }
        // -------------------------------------------------------------
        
        // Push log to debug topic
        String debugLog = "[KINEMATICS] Node: " + nodeId;
        if (cmd == CMD_FORWARD) debugLog += " -> Lệnh: ĐI THẲNG";
        else if (cmd == CMD_LEFT) debugLog += " -> Lệnh: RẼ TRÁI";
        else if (cmd == CMD_RIGHT) debugLog += " -> Lệnh: RẼ PHẢI";
        else debugLog += " -> Lệnh: DỪNG LẠI";
        
        network.publishDebug(debugLog);
        // ----------------------------------------------------------

        // Update the AGV's last known position
        state->position.x           = baseNodes[0].x;
        state->position.y           = baseNodes[0].y;
        state->position.initialized = true;
        state->lastNodeId           = baseNodes[0].nodeId;
        state->lastNodeSequenceId   = baseNodes[0].sequenceId;

        // Activate this node's actions
        for (int i = 0; i < baseNodes[0].actionCount; i++) {
            setActionStatus(baseNodes[0].actions[i].actionId, ACTION_RUNNING);
        }

        // Remove this node from the front of the base queue (shift array left)
        for (int i = 0; i < baseCount - 1; i++) baseNodes[i] = baseNodes[i + 1];
        baseCount--;

        // Also remove the edge that led to this node
        if (baseEdgeCount > 0) {
            for (int i = 0; i < baseEdgeCount - 1; i++) baseEdges[i] = baseEdges[i + 1];
            baseEdgeCount--;
        }

        // If base is almost empty, tell master control to send more nodes
        if (baseCount == 0 && horizonCount > 0) {
            state->newBaseRequest = true;
        }

        rebuildState();

        Serial.printf("[ORDER] Traversed: %s | base=%d horizon=%d\n",
            nodeId.c_str(), baseCount, horizonCount);

        sendMoveCommand(cmd);

    } else {
        // Mid-edge tag — just update position, no node release
        Serial.println("[ORDER] Edge marker: " + nodeId);
    }
}

// ─────────────────────────────────────────────
void OrderManager::setActionStatus(const String& actionId, const String& status) {
    for (int i = 0; i < state->actionStateCount; i++) {
        if (state->actionStates[i].actionId == actionId) {
            state->actionStates[i].status = status;
            return;
        }
    }
    // Action not in state yet — add it
    if (state->actionStateCount < MAX_ACTIONS) {
        state->actionStates[state->actionStateCount].actionId = actionId;
        state->actionStates[state->actionStateCount].status   = status;
        state->actionStateCount++;
    }
}

// ─────────────────────────────────────────────
void OrderManager::failAllActions() {
    for (int i = 0; i < state->actionStateCount; i++) {
        String s = state->actionStates[i].status;
        if (s == ACTION_WAITING || s == ACTION_RUNNING) {
            state->actionStates[i].status = ACTION_FAILED;
        }
    }
}

// ─────────────────────────────────────────────
// Copy internal base/horizon arrays into state so state_manager
// can serialize them into the published JSON.
void OrderManager::rebuildState() {
    state->nodeCount = 0;
    state->edgeCount = 0;

    for (int i = 0; i < baseCount; i++)
        state->nodeStates[state->nodeCount++] = baseNodes[i];
    for (int i = 0; i < horizonCount; i++)
        state->nodeStates[state->nodeCount++] = horizonNodes[i];

    for (int i = 0; i < baseEdgeCount; i++)
        state->edgeStates[state->edgeCount++] = baseEdges[i];
    for (int i = 0; i < horizonEdgeCount; i++)
        state->edgeStates[state->edgeCount++] = horizonEdges[i];
}

MOVE_cmd OrderManager::getNextMoveCommand() {
    if (baseCount < 2) {
        return CMD_STOP;
    }

    // Current node = first in base
    Node& current = baseNodes[0];

    // Next node
    Node& next = baseNodes[1];

    // If we don't have previous → assume forward
    if (state->lastNodeId.isEmpty()) {
        return CMD_FORWARD;
    }

    // Find previous node (from state)
    float prevX = state->position.x;
    float prevY = state->position.y;

    float v1x = current.x - prevX;
    float v1y = current.y - prevY;

    float v2x = next.x - current.x;
    float v2y = next.y - current.y;

    float cross = v1x * v2y - v1y * v2x;

    if (cross > 0.01f) return CMD_LEFT;
    if (cross < -0.01f) return CMD_RIGHT;
    return CMD_FORWARD;
}
