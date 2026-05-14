#include "esp32_comm.h"

/* ===== Init ===== */
void ESP32_Init(ESP32_Comm *comm)
{
    comm->head = 0;
    comm->tail = 0;
    comm->state = 0;
    comm->temp_cmd = 0;
}

/* ===== Push ===== */
static void push(ESP32_Comm *comm, AGV_Command cmd)
{
    uint8_t next = (comm->tail + 1) % QUEUE_SIZE;

    if (next == comm->head) {
        return; // full → drop
    }

    comm->buffer[comm->tail] = cmd;
    comm->tail = next;
}

/* ===== Pop ===== */
bool ESP32_GetCommand(ESP32_Comm *comm, AGV_Command *cmd)
{
    if (comm->head == comm->tail)
        return false;

    *cmd = comm->buffer[comm->head];
    comm->head = (comm->head + 1) % QUEUE_SIZE;
    return true;
}

bool ESP32_HasCommand(ESP32_Comm *comm)
{
    return (comm->head != comm->tail);
}

/* ===== UART Parser ===== */
void ESP32_ReceiveByte(ESP32_Comm *comm, uint8_t byte)
{
    switch (comm->state)
    {
        case 0: // wait header
            if (byte == HEADER)
                comm->state = 1;
            break;

        case 1: // cmd
            if (byte <= CMD_ROTATE) {
                comm->temp_cmd = byte;
                comm->state = 2;
            } else {
                comm->state = 0;
            }
            break;

        case 2: // crc
        {
            uint8_t crc = HEADER ^ comm->temp_cmd;

            if (crc == byte) {
                push(comm, (AGV_Command)comm->temp_cmd);
            }

            comm->state = 0;
            break;
        }
    }
}
