#include "RFID_reader.h"
#include "config.h"

// ─────────────────────────────────────────────────────────────────
// RFID reader implementation for the ESP32 + MFRC522 module.
//
// Features:
// - Initializes the RFID hardware over SPI
// - Reads physical card UIDs
// - Applies a debounce delay to avoid duplicate scans
// - Maps raw UIDs to application node IDs
//
// Enable the real RFID flow by setting USE_REAL_RFID = 1 in config.h.
// ─────────────────────────────────────────────────────────────────

// Constructor
// Initializes the MFRC522 driver and resets scan state.
RfidManager::RfidManager(uint8_t ss_pin, uint8_t rst_pin) : mfrc522(ss_pin, rst_pin) {
    lastScannedUID = "";
    lastScanTime = 0;
}

// Starts SPI and initializes the RFID reader.
void RfidManager::init() {
    SPI.begin();
    mfrc522.PCD_Init();
    Serial.println("[RFID] MFRC522 Initialized successfully");
}

// Reads a card UID, applies debounce, and returns the mapped node ID.
// Returns an empty string if no card is present, the read fails, or the
// debounce window has not elapsed.
String RfidManager::readHardware() {
    // 1. Check if a new card is present and can be read
    if (!mfrc522.PICC_IsNewCardPresent() || !mfrc522.PICC_ReadCardSerial()) {
        return ""; // No card present
    }

    // 2. Debounce - Avoid multiple activations from holding the card

    if (millis() - lastScanTime < debounceDelay) {
        mfrc522.PICC_HaltA(); // Stop reading the current card
        return "";
    }

    // 3. Read the UID of the card and convert it to a String (Hex)
    String uidString = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) uidString += "0";
        uidString += String(mfrc522.uid.uidByte[i], HEX);
    }
    uidString.toUpperCase();

    // 4. Update the scan time and put the card to sleep
    lastScanTime = millis();
    mfrc522.PICC_HaltA();
    
    Serial.print("[RFID] Scanned Physical UID: ");
    Serial.println(uidString);
    
    // 5. Return the corresponding Node ID
    return mapUidToNodeId(uidString);
}

// Maps a physical RFID UID to an application-level node ID.
// Unrecognized UIDs are returned unchanged.
String RfidManager::mapUidToNodeId(String uid) {
    // TODO: Implement your actual mapping logic here. This is just a placeholder.
    // Can replace this with a lookup table, database query, or any other mapping mechanism.
    if (uid == "F4F0C373") return "node_start";
    if (uid == "14FECF73") return "edge_1";
    if (uid == "D49ABF73") return "node_end";
    
    
    // If the UID is not recognized, return it as is or return a default value
    return uid; 
}