/**
 * @file    esp32_comm.h
 * @brief   ESP32 → STM32 UART packet parser.
 *
 * ===========================================================
 * PROTOCOL (ESP32 → STM32)
 * ===========================================================
 * Fixed 3-byte packets:
 *
 *   Byte 0 — HEADER  : 0xB1
 *   Byte 1 — CMD     : 0x00–0x04 (see AGV_Command enum)
 *   Byte 2 — CRC     : HEADER XOR CMD
 *
 * Example — turn left:
 *   B1  01  B0
 *
 * ===========================================================
 * USAGE
 * ===========================================================
 * In HAL_UART_RxCpltCallback():
 *   comm_process_byte(&parser, rx_byte, &cmd_buffer);
 *   HAL_UART_Receive_IT(&huart1, &rx_byte, 1); // re-arm
*/

#ifndef ESP32_COMM_H
#define ESP32_COMM_H

#include <stdint.h>
#include <stdbool.h>

/* ===== Commands ===== */
typedef enum {
	CMD_FORWARD = 0x00,
	CMD_ROTATE = 0x01,

	CMD_LEFT = 0x02,
	CMD_RIGHT = 0x03,
	CMD_STOP = 0x04
} AGV_Command;

/* ===== Config ===== */
#define QUEUE_SIZE 10
#define HEADER     0xB1

/* ===== Struct ===== */
typedef struct {
    AGV_Command buffer[QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;

    /* parser state */
    uint8_t state;
    uint8_t temp_cmd;

} ESP32_Comm;

/* ===== API ===== */
void ESP32_Init(ESP32_Comm *comm);

/* UART interrupt */
void ESP32_ReceiveByte(ESP32_Comm *comm, uint8_t byte);

/* main loop */
bool ESP32_GetCommand(ESP32_Comm *comm, AGV_Command *cmd);
bool ESP32_HasCommand(ESP32_Comm *comm);

#endif
