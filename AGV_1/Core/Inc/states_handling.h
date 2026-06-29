#ifndef STATES_HANDLING_H
#define STATES_HANDLING_H

#include "esp32_comm.h"
#include "line_sensor_weight.h"
#include "controllers.h"
#include <stdint.h>

/* ===== CONFIG ===== */
#define AGV_BASE_SPEED          30.0f
#define AGV_ROTATE_SPEED        35.0f
#define AGV_REACQUIRE_SPEED     35.0f
#define AGV_REACQUIRE_THRESHOLD 0.5f
#define AGV_LOST_LINE_TIMEOUT_MS 1000

#define AGV_PID_KP 35.0f
#define AGV_PID_KI 0.0f
#define AGV_PID_KD 10.0f

/* ===== STATES ===== */
typedef enum {
    AGV_STATE_IDLE,
    AGV_STATE_FOLLOW_LINE,
    AGV_STATE_ROTATE,
    AGV_STATE_REACQUIRE_LINE,
    AGV_STATE_STOP,
    AGV_STATE_LOST_LINE
} AGV_State;


/* ===== SYSTEM ===== */
typedef struct {
    AGV_State state;
    AGV_Command current_cmd;

    ESP32_Comm comm;   
    LineSensor line_sensor;
    PID_parameters pid;

    uint32_t state_entry_tick;

} AGV_System;

/* ===== API ===== */
void AGV_Init(AGV_System *agv);
void AGV_Update(AGV_System *agv);

#endif
