#pragma once
#include "order_manager.h"

inline void initTagMappings(OrderManager& orderMgr) {
    
    // ─────────────────────────────────────────────────────────────────
    // TAG MAPPING CONFIGURATION <---> VDA 5050 NODE ID
    // ─────────────────────────────────────────────────────────────────
    
    // Ví dụ các node thực tế trên sa bàn
    orderMgr.addTagMapping("F4F0C373", "node_start");
    orderMgr.addTagMapping("14FECF73", "edge_1");
    orderMgr.addTagMapping("D49ABF73", "node_end");
}