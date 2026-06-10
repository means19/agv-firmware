#ifndef RFID_READER_H
#define RFID_READER_H

#include <Arduino.h>
#include "config.h"
#include <SPI.h>
#include <MFRC522.h>

// ─────────────────────────────────────────────────────────────────
//  RFID_reader.h
//
//  This module now supports both simulation and real MFRC522 hardware.
//
//  Modes:
//    - Simulation mode (default in older setups):
//        Tags are injected via the "test/rfid" MQTT topic published by
//        Test_publisher.py on your PC. main.cpp routes them directly
//        to orderMgr.onTagRead().
//    - Hardware mode (MFRC522):
//        When USE_REAL_RFID is enabled in config.h, this class will
//        initialize the MFRC522 reader and attempt to read tags from
//        the physical device. readHardware() returns the tag UID string
//        (empty string if no tag).
//
//  Integration notes:
//    1. Set USE_REAL_RFID to 1 in config.h to enable hardware reads.
//    2. readHardware() performs debounced reads (see debounceDelay) and
//       should return "" when no new/valid tag is present.
//
//  Implementation details:
//    - The class holds an MFRC522 instance, a last-scanned UID and a
//      timestamp to avoid multiple detections of the same tag within
//      debounceDelay (default 2000 ms).
//    - mapUidToNodeId() converts a physical UID string into the system
//      Node ID used by the rest of the application.
// ─────────────────────────────────────────────────────────────────

class RfidManager {
private:
    MFRC522 mfrc522;
    String lastScannedUID;
    unsigned long lastScanTime;
    const unsigned long debounceDelay = 2000; // Avoid multiple reads of the same tag within 2 seconds

public:
    // GPIO pins for the RFID reader
    RfidManager(uint8_t ss_pin = 5, uint8_t rst_pin = 22);
    
    void init();
    String readHardware();
};

#endif