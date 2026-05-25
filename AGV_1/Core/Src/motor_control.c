#include "motor_control.h"

/* ===== External TIM ===== */
extern TIM_HandleTypeDef htim3;

/* ===== Config ===== */
#define PWM_MAX     999.0f

/* ===== GPIO Mapping ===== */
#define L_IN1_PORT GPIOB
#define L_IN1_PIN  GPIO_PIN_12
#define L_IN2_PORT GPIOB
#define L_IN2_PIN  GPIO_PIN_13

#define R_IN1_PORT GPIOB
#define R_IN1_PIN  GPIO_PIN_14
#define R_IN2_PORT GPIOB
#define R_IN2_PIN  GPIO_PIN_15

/* ===== Helper ===== */
static float clamp(float v, float min, float max)
{
    if (v < min) return min;
    if (v > max) return max;
    return v;
}

static void setPWM_Left(float speed)
{
    speed = clamp(speed, 0, 100);
    uint32_t pwm = (uint32_t)((speed / 100.0f) * PWM_MAX);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, pwm);
}

static void setPWM_Right(float speed)
{
    speed = clamp(speed, 0, 100);
    uint32_t pwm = (uint32_t)((speed / 100.0f) * PWM_MAX);
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_2, pwm);
}

/* ===== Direction ===== */ 
static void Left_Forward(void)
{
    HAL_GPIO_WritePin(L_IN1_PORT, L_IN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(L_IN2_PORT, L_IN2_PIN, GPIO_PIN_RESET);
}

static void Right_Forward(void)
{
    HAL_GPIO_WritePin(R_IN1_PORT, R_IN1_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(R_IN2_PORT, R_IN2_PIN, GPIO_PIN_RESET);
}

static void Right_Backward(void)
{
    HAL_GPIO_WritePin(R_IN1_PORT, R_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R_IN2_PORT, R_IN2_PIN, GPIO_PIN_SET);
}

/* ===== Public Functions ===== */

void Motor_Init(void)
{
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1);
    HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2);

    Motor_Stop();
}

void Motor_Stop(void)
{
    setPWM_Left(0);
    setPWM_Right(0);

    HAL_GPIO_WritePin(L_IN1_PORT, L_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(L_IN2_PORT, L_IN2_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R_IN1_PORT, R_IN1_PIN, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(R_IN2_PORT, R_IN2_PIN, GPIO_PIN_RESET);
}

void Motor_Forward(float speed)
{
    Left_Forward();
    Right_Forward();

    setPWM_Left(speed);
    setPWM_Right(speed);
}

void Motor_RotateRight(float speed)
{
    Left_Forward();
    Right_Backward();

    setPWM_Left(speed);
    setPWM_Right(speed);
}

/* ===== PID Control ===== */
void Motor_ApplyPID(float base_speed, float correction)
{
    float diff = correction * 0.4f;

    float left  = clamp(base_speed + diff, 0, 100);
    float right = clamp(base_speed - diff, 0, 100);

    Left_Forward();
    Right_Forward();

    setPWM_Left(left);
    setPWM_Right(right);
}
