#pragma once
#include <Arduino.h>
#include "config.h"

// ─────────────────────────────────────────────────────────────────
//  RFID_reader.h
//
//  Right now this module only supports simulation mode.
//  Real RFID hardware reads are NOT implemented yet.
//
//  Simulation mode (current):
//    Tags arrive via the "test/rfid" MQTT topic published by
//    Test_publisher.py on your PC. main.cpp routes them directly
//    to orderMgr.onTagRead() — this class is not involved.
//
//  To add real hardware later:
//    1. Add your RFID library include at the top of RFID_reader.cpp
//    2. Fill in readHardware() — return the tag UID string or ""
//    3. Set USE_REAL_RFID to 1 in config.h
//    That's it. Everything else stays the same.
// ─────────────────────────────────────────────────────────────────

class RfidManager {
public:
    String lastTag;  // UID of the last scanned tag, e.g. "A1:B2:C3:D4"
    bool   hasNew;   // true = new tag ready, main.cpp hasn't read it yet

    void begin();    // called once in setup()
    void loop();     // called every loop() — polls hardware when ready

private:
    unsigned long lastPollTime;
    unsigned long lastFireTime;
    String        lastFiredTag;

    // ── Replace this function body in RFID_reader.cpp when adding hardware ──
    String readHardware();
};