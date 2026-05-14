/*
    * motor_control.h
    *
    * Simple motor control module for AGV. Provides functions to initialize,
    * stop, move forward, rotate, and apply PID corrections to the motors.
    * no structures or complex data types are used, just straightforward functions.
*/

#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include "stm32f1xx_hal.h"

/* ===== Public API ===== */

void Motor_Init(void);

void Motor_Stop(void);

void Motor_Forward(float speed);

void Motor_RotateRight(float speed);

void Motor_ApplyPID(float base_speed, float correction);

#endif
