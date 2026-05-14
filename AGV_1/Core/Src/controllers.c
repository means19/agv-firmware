#include   "controllers.h"


/**
 ******************************************************************************
 * @file    controllers.cpp
 * @class   PID controller implementation
 * @brief   PID controller with fleaxible parameters and output limits
 * 
 * @param   PID_parameters *pid: Pointer to PID parameters structure
 * @param   float kp: Proportional gain 
 * @param   float ki: Integral gain
 * @param   float kd: Derivative gain
******************************************************************************
 */

void PID_Init(PID_parameters *pid, float kp, float ki, float kd, float out_min, float out_max)
{
    pid->Kp = kp;
    pid->Ki = ki;
    pid->Kd = kd;

    pid->prev_error = 0.0f;
    pid->integral = 0.0f;

    pid->output_min = out_min;
    pid->output_max = out_max;
}

void PID_Reset(PID_parameters *pid)
{
    pid->prev_error = 0.0f;
    pid->integral = 0.0f;
}

float PID_Compute(PID_parameters *pid, float setpoint, float measurement)
{
    float error = setpoint - measurement;
    pid->integral += error;
    float derivative = error - pid->prev_error;

    float output = (pid->Kp * error) + (pid->Ki * pid->integral) + (pid->Kd * derivative);

    // Clamp output to min/max
    if (output > pid->output_max)
        output = pid->output_max;
    else if (output < pid->output_min)
        output = pid->output_min;

    pid->prev_error = error;

    return output;
}


