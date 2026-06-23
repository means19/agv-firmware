/**
 * @file    hcsr04.c
 * @brief   HC-SR04 Ultrasonic Sensor Driver - non-blocking implementation
 *
 * Design summary:
 * - Trigger pulse is generated in HCSR04_Trigger() (blocking for 10 us only).
 * - ECHO pin is handled by EXTI on both edges.
 * - Rising edge captures timer count; falling edge computes pulse width.
 * - HCSR04_Update() enforces a timeout without blocking the main loop.
 */

#include "Hcsr04.h"

/* Timer handle stored from HCSR04_Init() */
static TIM_HandleTypeDef *_htim;

/* Driver context shared between main and ISR */
volatile HCSR04_Context g_hcsr04 = {
    .state = HCSR04_STATE_IDLE,
    .rise_us = 0U,
    .trigger_ms = 0U,
    .rise_ms = 0U,
    .distance_cm = HCSR04_NO_ECHO_CM,
    .new_data = 0U
};

/* Read timer counter in microseconds (1 tick = 1 us). */
static uint16_t get_us(void)
{
    return (uint16_t)__HAL_TIM_GET_COUNTER(_htim);
}

/* Compute elapsed ticks with wrap-around on 16-bit timer. */
static uint16_t elapsed_us(uint16_t start, uint16_t end)
{
    return (uint16_t)(end - start);
}

/* Short busy-wait used only for the 10 us trigger pulse. */
static void delay_us(uint16_t us)
{
    uint16_t start = get_us();
    while ((uint16_t)(get_us() - start) < us)
    {
    }
}

void HCSR04_Init(TIM_HandleTypeDef *htim)
{
    _htim = htim;
    HAL_TIM_Base_Start(_htim);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
    /* Reset driver state in case of warm restart. */
    g_hcsr04.state = HCSR04_STATE_IDLE;
    g_hcsr04.new_data = 0U;
    g_hcsr04.distance_cm = HCSR04_NO_ECHO_CM;
}

void HCSR04_Trigger(void)
{
    if (g_hcsr04.state != HCSR04_STATE_IDLE)
    {
        return;
    }

    /* Arm measurement window before emitting trigger. */
    g_hcsr04.state = HCSR04_STATE_WAIT_RISE;
    g_hcsr04.trigger_ms = HAL_GetTick();
    g_hcsr04.rise_ms = 0U;
    g_hcsr04.new_data = 0U;

    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    delay_us(10U);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);
}

void HCSR04_Update(void)
{
    if (g_hcsr04.state == HCSR04_STATE_IDLE)
    {
        return;
    }

    uint32_t now_ms = HAL_GetTick();
    uint32_t start_ms = (g_hcsr04.state == HCSR04_STATE_WAIT_RISE)
                            ? g_hcsr04.trigger_ms
                            : g_hcsr04.rise_ms;

    if ((uint32_t)(now_ms - start_ms) >= HCSR04_TIMEOUT_MS)
    {
        /* Timeout: no echo received. Mark as no obstacle. */
        g_hcsr04.state = HCSR04_STATE_IDLE;
        g_hcsr04.distance_cm = HCSR04_NO_ECHO_CM;
        g_hcsr04.new_data = 1U;
    }
}

void HCSR04_EXTI_Handler(uint16_t GPIO_Pin)
{
    if (GPIO_Pin != ECHO_PIN)
    {
        return;
    }

    GPIO_PinState pin_state = HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN);
    uint16_t now_us = get_us();

    if (pin_state == GPIO_PIN_SET)
    {
        if (g_hcsr04.state == HCSR04_STATE_WAIT_RISE)
        {
            g_hcsr04.rise_us = now_us;
            g_hcsr04.rise_ms = HAL_GetTick();
            g_hcsr04.state = HCSR04_STATE_WAIT_FALL;
        }
        return;
    }

    if (g_hcsr04.state == HCSR04_STATE_WAIT_FALL)
    {
        /* Falling edge: pulse width ready -> distance in cm. */
        uint16_t echo_us = elapsed_us(g_hcsr04.rise_us, now_us);
        g_hcsr04.distance_cm = (float)echo_us * HCSR04_SOUND_CM_PER_US;
        g_hcsr04.new_data = 1U;
        g_hcsr04.state = HCSR04_STATE_IDLE;
    }
}

uint8_t HCSR04_TryGetDistanceCm(float *out_cm)
{
    if (g_hcsr04.new_data == 0U)
    {
        return 0U;
    }

    if (out_cm != NULL)
    {
        *out_cm = g_hcsr04.distance_cm;
    }

    g_hcsr04.new_data = 0U;
    return 1U;
}

float HCSR04_GetLastDistanceCm(void)
{
    return g_hcsr04.distance_cm;
}

int Object_detected(void)
{
	/* TẠM THỜI COMMENT ĐOẠN ĐỌC KHOẢNG CÁCH LẠI
	float distance = HCSR04_GetLastDistanceCm();
    if (distance > 0.0f && distance < ERROR_ALLOWED_RANGE)
    {
        return 1;
    }
    */
    return 0;
}
