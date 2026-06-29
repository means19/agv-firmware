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
#include "esp32_comm.h"
#include "line_sensor_weight.h"
#include "controllers.h"
#include "motor_control.h"  /* Đã bổ sung */
#include "Hcsr04.h"         /* Đã bổ sung module Siêu âm */
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

    /* UPDATE: Rotate based on the last navigation command */
    if (agv->current_cmd == CMD_LEFT) {
        Motor_RotateLeft(AGV_ROTATE_SPEED);
    } else {
        Motor_RotateRight(AGV_ROTATE_SPEED);
    }
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

    /* UPDATE: Check the corresponding line based on the current command */
    if (agv->current_cmd == CMD_LEFT) {
        Motor_RotateLeft(AGV_REACQUIRE_SPEED);
    } else {
        Motor_RotateRight(AGV_REACQUIRE_SPEED);
    }
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

    /* 1. READ UART COMMANDS FROM ESP32 */
    AGV_Command cmd;
    if (ESP32_GetCommand(&agv->comm, &cmd)) {
        AGV_HandleCommand(agv, cmd);
    }

    /* 2. SAFETY CORE: CHECK FOR OBSTACLES (OVERRIDE COMMANDS) */
    if (Object_detected()) {
        /* If an obstacle is detected and the vehicle is not stopped, immediately stop */
        if (agv->state != AGV_STATE_STOP) {
            AGV_TransitionTo(agv, AGV_STATE_STOP);
            /* Bật LED báo lỗi Error_1 trên PB1 (Cấu hình tùy chọn) */
            // HAL_GPIO_WritePin(GPIOB, Error_1_Pin, GPIO_PIN_SET);
        }
    } else {
        /* TẮT LED lỗi (Cấu hình tùy chọn) */
        // HAL_GPIO_WritePin(GPIOB, Error_1_Pin, GPIO_PIN_RESET);

        /* TỰ ĐỘNG PHỤC HỒI:
         * Nếu xe đang dừng an toàn nhưng bản chất lệnh hệ thống vẫn là ĐI,
         * xe sẽ tự động tiếp tục nhiệm vụ khi rút vật cản ra.
         */
        if (agv->state == AGV_STATE_STOP) {
            if (agv->current_cmd == CMD_FORWARD ||
                agv->current_cmd == CMD_LEFT ||
                agv->current_cmd == CMD_RIGHT)
            {
                AGV_TransitionTo(agv, AGV_STATE_FOLLOW_LINE);
            }
            else if (agv->current_cmd == CMD_ROTATE)
            {
                AGV_TransitionTo(agv, AGV_STATE_ROTATE);
            }
        }
    }

    /* 3. EXECUTE STATE */
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
            /* Xe đang dừng, không làm gì cả để tiết kiệm CPU */
            break;
    }
}

