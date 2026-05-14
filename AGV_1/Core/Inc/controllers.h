#ifndef CONTROLLERS
#define CONTROLLERS

typedef struct
{
    float Kp;
    float Ki;
    float Kd;

    float prev_error;
    float integral;

    float output_min;
    float output_max;

} PID_parameters;

// Initialize PID
void PID_Init(PID_parameters *pid, float kp, float ki, float kd, float out_min, float out_max);

// Reset PID state
void PID_Reset(PID_parameters *pid);

// Compute PID output
float PID_Compute(PID_parameters *pid, float setpoint, float measurement);

#endif
