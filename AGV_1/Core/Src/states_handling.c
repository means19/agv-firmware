/**
 * @file    states_handling.c
 * @brief   AGV finite state machine implementation.
 *
 * Key design decisions:
 *
 * 1. No HAL_Delay() anywhere. Timing uses HAL_GetTick() deltas.
 *
 * 2. State entry actions are performed once via state_entry_tick
 * being updated in AGV_TransitionTo(). This avoids repeating
 * one-shot actions (like PID_Reset) every loop iteration.
 *
 * 3. PID is ONLY computed in AGV_STATE_FOLLOW_LINE. All other
 * states use fixed motor commands.
 *
 * 4. Rotation is always in ONE direction (right). CMD_ROTATE
 * unconditionally calls Motor_RotateRight().
 *
 * 5. REACQUIRE_LINE uses a position threshold, not a timer.
 * The robot creeps forward-right until the weighted sensor
 * position is within AGV_REACQUIRE_THRESHOLD of centre (0.0).
 *
 * 6. UPDATED: ESP32 handles RFID. STM32 transitions state IMMEDIATELY
 * upon receiving a valid UART command from ESP32.
 */

#include "states_handling.h"
#include "ESP32_comm.h"
#include "line_sensor_weight.h"
#include "controllers.h"
#include "stm32f1xx_hal.h"
#include <math.h>

/* ===== INTERNAL ===== */
static void AGV_TransitionTo(AGV_System *agv, AGV_State new_state)
{
    agv->state = new_state;
    agv->state_entry_tick = HAL_GetTick();

    if (new_state == AGV_STATE_STOP || new_state == AGV_STATE_IDLE)
        Motor_Stop();

    if (new_state == AGV_STATE_FOLLOW_LINE)
        PID_Reset(&agv->pid);
}

static void AGV_HandleCommand(AGV_System *agv, AGV_Command cmd)
{
    agv->current_cmd = cmd;

    switch (cmd)
    {
        case CMD_FORWARD:
        case CMD_LEFT:
        case CMD_RIGHT:
            AGV_TransitionTo(agv, AGV_STATE_FOLLOW_LINE);
            break;

        case CMD_ROTATE:
            AGV_TransitionTo(agv, AGV_STATE_ROTATE);
            break;

        case CMD_STOP:
            AGV_TransitionTo(agv, AGV_STATE_STOP);
            break;

        default:
            break;
    }
}

/* ===== STATES ===== */

static void State_FollowLine(AGV_System *agv)
{
    if (!agv->line_sensor.line_detected) {
        AGV_TransitionTo(agv, AGV_STATE_LOST_LINE);
        return;
    }

    float correction = PID_Compute(&agv->pid, 0.0f, agv->line_sensor.position);
    Motor_ApplyPID(AGV_BASE_SPEED, correction);
}

static void State_LostLine(AGV_System *agv)
{
    if (agv->line_sensor.line_detected) {
        AGV_TransitionTo(agv, AGV_STATE_FOLLOW_LINE);
        return;
    }

    if (HAL_GetTick() - agv->state_entry_tick > AGV_LOST_LINE_TIMEOUT_MS) {
        AGV_TransitionTo(agv, AGV_STATE_ROTATE);
        return;
    }

    Motor_Forward(AGV_REACQUIRE_SPEED);
}

static void State_Rotate(AGV_System *agv)
{
    if (agv->line_sensor.line_detected) {
        AGV_TransitionTo(agv, AGV_STATE_REACQUIRE_LINE);
        return;
    }

    Motor_RotateRight(AGV_ROTATE_SPEED);
}

static void State_ReacquireLine(AGV_System *agv)
{
    if (!agv->line_sensor.line_detected) {
        AGV_TransitionTo(agv, AGV_STATE_ROTATE);
        return;
    }

    if (fabsf(agv->line_sensor.position) <= AGV_REACQUIRE_THRESHOLD) {
        AGV_TransitionTo(agv, AGV_STATE_FOLLOW_LINE);
        return;
    }

    Motor_RotateRight(AGV_REACQUIRE_SPEED);
}

/* ===== PUBLIC ===== */

void AGV_Init(AGV_System *agv)
{
    ESP32_Init(&agv->comm);
    LineSensor_Init(&agv->line_sensor);

    PID_Init(&agv->pid,
             AGV_PID_KP,
             AGV_PID_KI,
             AGV_PID_KD,
             -100,
             100);

    agv->state = AGV_STATE_IDLE;
    agv->current_cmd = CMD_STOP;

    Motor_Stop();
}

void AGV_Update(AGV_System *agv)
{
    LineSensor_Update(&agv->line_sensor);

    /* 🔥 EVENT-DRIVEN COMMAND DIRECTLY FROM ESP32 UART */
    AGV_Command cmd;
    // ESP32_GetCommand() checks if a new command is available from the ESP32 via UART.
    // returns true if a new command is available and assigns it to the cmd variable.
    if (ESP32_GetCommand(&agv->comm, &cmd)) {
        AGV_HandleCommand(agv, cmd);
    }

    switch (agv->state)
    {
        case AGV_STATE_FOLLOW_LINE:
            State_FollowLine(agv);
            break;

        case AGV_STATE_LOST_LINE:
            State_LostLine(agv);
            break;

        case AGV_STATE_ROTATE:
            State_Rotate(agv);
            break;

        case AGV_STATE_REACQUIRE_LINE:
            State_ReacquireLine(agv);
            break;

        case AGV_STATE_STOP:
        case AGV_STATE_IDLE:
        default:
            break;
    }
}
