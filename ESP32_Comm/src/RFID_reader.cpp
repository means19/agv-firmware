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

    // 2. Read the UID of the card and convert it to a String (Hex)
    String currentUID = "";
    for (byte i = 0; i < mfrc522.uid.size; i++) {
        if (mfrc522.uid.uidByte[i] < 0x10) currentUID += "0";
        currentUID += String(mfrc522.uid.uidByte[i], HEX);
    }
    currentUID.toUpperCase();

    // 3. Debounce - Avoid multiple activations from holding the card

    if (currentUID == lastScannedUID && (millis() - lastScanTime < debounceDelay)) {
        mfrc522.PICC_HaltA(); // Stop the card to prevent further reads
        return "";
    }

    // 4. If we reach here, it's a new scan. Update the last scanned UID and time.
    lastScannedUID = currentUID;
    lastScanTime = millis();

    // 5. Halt the card and stop encryption to allow for the next scan
    mfrc522.PICC_HaltA();
    mfrc522.PCD_StopCrypto1(); 

    Serial.print("[RFID] Scanned Physical UID: ");
    Serial.println(currentUID);

    // 6. Map the physical UID to an application node ID and return it
    return currentUID;
}