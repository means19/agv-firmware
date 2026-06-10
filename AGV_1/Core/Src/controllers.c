#include   "controllers.h"


/**
 ******************************************************************************
 * @file    controllers.c
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

    // 1. Calculating Proportional term
    float p_term = pid->Kp * error;

    // 2. Calculating Integral term (temporary integral for anti-windup)
    float temp_integral = pid->integral + error;
    float i_term = pid->Ki * temp_integral;

    // 3. Calculating Derivative term
    float derivative = error - pid->prev_error;
    float d_term = pid->Kd * derivative;

    // 4. Summing up all terms
    float output = p_term + i_term + d_term;

    // 5. Clamping (Output Limiting) & Anti-Windup
    if (output > pid->output_max)
    {
        output = pid->output_max;
        // If only the P term has exceeded the max, do not allow I to accumulate in the same direction
        if (error < 0 || p_term < pid->output_max)
        {
             pid->integral += error;
        }
    }
    else if (output < pid->output_min)
    {
        output = pid->output_min;
        if (error > 0 || p_term > pid->output_min)
        {
             pid->integral += error;
        }
    }
    else
    {
        // Normal state, allow I accumulation
        pid->integral = temp_integral;
    }

    // 6. Save state
    pid->prev_error = error;

    return output;
}


