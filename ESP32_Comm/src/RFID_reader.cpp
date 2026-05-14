#include "RFID_reader.h"
#include "config.h"

// ─────────────────────────────────────────────────────────────────

//
// In main.cpp, set USE_REAL_RFID 1 in config.h
// ─────────────────────────────────────────────────────────────────

void RfidManager::begin() {
    lastTag      = "";
    hasNew       = false;
    lastPollTime = 0;
    lastFireTime = 0;
    lastFiredTag = "";

    // Hardware init goes here (Step 2 above)

    Serial.println("[RFID] Stub ready — add hardware in RFID_reader.cpp");
}

void RfidManager::loop() {
    // Only run at the configured poll interval
    if (millis() - lastPollTime < RFID_POLL_MS) return;
    lastPollTime = millis();

    String tag = readHardware();

    if (tag.isEmpty()) {
        lastFiredTag = "";  // reset so the same tag can fire again when it returns
        return;
    }

    // Don't re-fire the same tag while it stays in the field
    if (tag == lastFiredTag && (millis() - lastFireTime) < TAG_DEBOUNCE_MS) {
        return;
    }

    lastFiredTag = tag;
    lastFireTime = millis();
    lastTag      = tag;
    hasNew       = true;

    Serial.println("[RFID] Tag: " + tag);
}

String RfidManager::readHardware() {
    // ── Hardware not connected yet — always returns "" ──
    // Fill this in when you add the real RFID reader (see steps above)
    return "";
}