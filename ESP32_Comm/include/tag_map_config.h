#pragma once
#include "order_manager.h"

inline void initTagMappings(OrderManager& orderMgr) {
    
    // ─────────────────────────────────────────────────────────────────
    // TAG MAPPING CONFIGURATION <---> VDA 5050 NODE ID
    // ─────────────────────────────────────────────────────────────────
    
    // Ví dụ các node thực tế trên sa bàn
    orderMgr.addTagMapping("6D7D1205", "Assy_Drop_1");
    orderMgr.addTagMapping("63D34D10", "West_C");
    orderMgr.addTagMapping("C3C0B634", "West_N");
}