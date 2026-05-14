#include "STM32_comm.h"
#include "config.h"
#include <Arduino.h>
#include "order_manager.h"



void sendMoveCommand(uint8_t cmd) {
    uint8_t packet[3];

    packet[0] = 0xB1;  // header
    packet[1] = cmd;
    packet[2] = packet[0] ^ packet[1]; // simple XOR checksum

    Serial2.write(packet, 3);  // or your UART port
}

// hardcode other pins here for instance flags 