# ESP32 Firmware Documentation

## 1. Scope and Role
This firmware runs on an ESP32 and is responsible for the communication, coordination and peripheral management tasks of the AGV subsystem:
- Wi‑Fi connectivity and MQTT integration for backend communication and telemetry
- RFID reading via MFRC522 and publishing tag events
- UART bridge and protocol handling with the main MCU (STM32)
- Local order processing and coordination logic (lightweight order manager)

Firmware is implemented in C++ and built with PlatformIO for the ESP32 target. The application currently follows the Arduino `setup()` / `loop()` model, with networking callbacks and polling used for coordination. Third-party libraries are included under `libdeps/`.

## 2. Hardware Interfaces
### 2.1 MCU and RTOS
- MCU: ESP32 (Xtensa) running the Arduino core.

### 2.2 SPI / RFID
- MFRC522 (RC522) communicates over SPI. CS/SS, SCK, MOSI, MISO and RST pins are configured in `include/config.h`.

### 2.3 UART
- UART to STM32 is used for command/status exchange. UART settings (baud rate, parity, stop bits) are configured in `include/config.h`.
- The UART driver transmits framed messages and keeps the protocol handling lightweight so it does not block the main loop.

### 2.4 Network
- Wi‑Fi interface uses the ESP-IDF/WiFi library (wrappers in `src/network_manager.cpp`).
- MQTT client implemented via `PubSubClient` for publish/subscribe to the backend broker.

## 3. Software Architecture
### 3.1 Concurrency model
- The firmware is organized around the Arduino main loop, interrupt-driven callbacks, and synchronous helper modules:
  - `NetworkManager`: handles Wi‑Fi connection, MQTT loop, reconnection logic
  - `RFIDReader`: polls/handles MFRC522 events and dispatches tag events when real hardware is enabled
  - `OrderManager`: processes orders and coordinates local state transitions
  - `STM32Comm`: sends UART frames to the STM32

### 3.2 Message flow
- External commands originate from MQTT (backend) or STM32 (local commands). MQTT callback data is buffered in `NetworkManager::incoming` and consumed from `loop()`.
- Tag events and state updates are published to MQTT in JSON format.

### 3.3 Non-blocking design
- Drivers use callbacks, short polling intervals, and early returns where possible. Long operations are kept out of the hot path to avoid blocking the main loop.

## 4. Timing and Real-Time Behavior
- Wi‑Fi and MQTT operate with cooperative timing — reconnect/backoff is handled inside `NetworkManager::loop()`.
- MQTT payloads are delivered through the PubSubClient callback, then consumed by the main loop.
- RFID reads are polling-based; timing and debounce behavior are controlled in `RfidManager::readHardware()`.

## 5. Peripherals / Sensors
### 5.1 RFID (MFRC522)
- Handles tag detection, UID reading, and optional authentication.
- On tag detection the firmware publishes an MQTT event and may forward an event to STM32 over UART.

### 5.2 Other inputs
- The ESP32 firmware expects sensor-heavy control (line-following, real-time obstacle handling) to be executed on the STM32; the ESP32 handles coordination and higher-level events.

## 6. Local Decision & Coordination Logic
- `OrderManager` implements simple state transitions for order acceptance, execution, and completion.
- The module may locally accept or reject orders based on internal state or STM32-reported conditions.

## 7. Communication
### 7.1 UART protocol (ESP32 <-> STM32)
- The UART protocol framing and commands are declared in `include/STM32_comm.h` and implemented in `src/STM32_comm.cpp`.
- Messages use a framed byte protocol with header, payload and checksum. The current implementation in `sendMoveCommand()` writes a 3-byte packet: `HEADER | CMD | CRC` where CRC is `HEADER XOR CMD`.

### 7.2 MQTT topics and payloads
- Topics and base prefixes are configured in `include/config.h`.
- Payloads are JSON using `ArduinoJson`. Typical messages include `status`, `order/update`, `rfid/event` and acknowledgements.

## 8. Relevant Modules
- `include/config.h` — project runtime configuration (MQTT, UART, pins)
- `include/STM32_comm.h` — UART protocol definitions
- `include/RFID_reader.h` — RFID interface
- `src/main.cpp` — Arduino setup and main loop initialization
- `src/network_manager.cpp` — Wi‑Fi and MQTT logic
- `src/STM32_comm.cpp` — UART framing and handling
- `src/RFID_reader.cpp` — MFRC522 integration
- `src/order_manager.cpp` — order processing and coordination logic
- `test_harness/` — test scripts and utilities

## 9. Configuration Notes
- Ensure `include/config.h` reflects the target board pinout and network credentials.
- `platformio.ini` contains defined environments; select the correct board before building.
- Verify UART baud rate matches the STM32 firmware's expectation.

## 10. Known Limitations and Extensions
- Offline MQTT buffering is limited; consider persistent queueing for long offline periods.
- Some STM32-originated safety events are currently only acted upon locally; adding explicit MQTT notifications improves visibility.
- Consider migrating MQTT client to an ESP-IDF native client for better stability under heavy network load.

## 11. Troubleshooting
- Use `pio device monitor` to view boot logs and runtime messages.
- If MQTT fails to connect: check network credentials, broker reachability, and TLS (if enabled).
- For UART issues: verify wiring, correct TTL levels, and matching UART settings on both devices.

---
