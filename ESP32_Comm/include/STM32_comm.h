#pragma once

#include <stdint.h>

// ──────────────────────────────────────────
//  add the STM32 communication protocol functions here, so main.cpp can call them without
//__________________________________________

enum MOVE_cmd
{
    CMD_FORWARD = 0x00,
    CMD_ROTATE = 0x01,

    
    CMD_LEFT = 0x02,
    CMD_RIGHT = 0x03,
    CMD_STOP = 0x04
};



void sendMoveCommand(uint8_t cmd);
