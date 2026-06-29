#include "STM32_comm.h"
#include "config.h"
#include <Arduino.h>
#include "order_manager.h"



void sendMoveCommand(MOVE_cmd cmd) {
    uint8_t packet[3];

    packet[0] = 0xB1;  // header
    packet[1] = static_cast<uint8_t>(cmd);
    packet[2] = packet[0] ^ packet[1]; // simple XOR checksum

    STM32_SERIAL.write(packet, 3);
}

// hardcode other pins here for instance flags 