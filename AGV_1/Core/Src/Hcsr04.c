/**
 * @file    hcsr04.c
 * @brief   HC-SR04 Ultrasonic Sensor Driver - implementation
 */

#include "Hcsr04.h"

/* Timer handle stored from HCSR04_Init() */
static TIM_HandleTypeDef *_htim;

/* ------------------------------------------------------------------ */
/* Internal helper: read TIM2 counter in microseconds                  */
/* ------------------------------------------------------------------ */
static uint32_t get_us(void)
{
    return __HAL_TIM_GET_COUNTER(_htim);
}

/* Internal helper: busy-wait for a given number of microseconds       */
static void delay_us(uint32_t us)
{
    uint32_t start = get_us();
    while ((uint16_t)(get_us() - start) < us);
}

/* ------------------------------------------------------------------ */

void HCSR04_Init(TIM_HandleTypeDef *htim)
{
    _htim = htim;
    HAL_TIM_Base_Start(_htim);                          /* start 1 MHz counter */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET); /* TRIG idle LOW   */
    HAL_Delay(50);                                      /* sensor warm-up      */
}

/* ------------------------------------------------------------------ */

float HCSR04_Read_cm(void)
{
    uint32_t t_start, t_end;

    /* 1. Send 10 us trigger pulse */
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_SET);
    delay_us(10);
    HAL_GPIO_WritePin(TRIG_PORT, TRIG_PIN, GPIO_PIN_RESET);

    /* 2. Wait for ECHO to go HIGH (sensor starts sending ultrasound) */
    t_start = get_us();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_RESET)
    {
        if ((uint16_t)(get_us() - t_start) >= HCSR04_TIMEOUT_US)
            return -1.0f;  /* no response from sensor */
    }

    /* 3. Measure how long ECHO stays HIGH (round-trip travel time) */
    t_start = get_us();
    while (HAL_GPIO_ReadPin(ECHO_PORT, ECHO_PIN) == GPIO_PIN_SET)
    {
        if ((uint16_t)(get_us() - t_start) >= HCSR04_TIMEOUT_US)
            return -1.0f;  /* echo too long = out of range */
    }
    t_end = get_us();

    /* 4. Convert time to distance
     *    distance = echo_us * (343 m/s / 2) * 100 cm/m / 1 000 000
     *             = echo_us * 0.01715 cm/us                         */
    uint16_t echo_us = (uint16_t)(t_end - t_start);
    return echo_us * 0.01715f;
}

int Object_detected (void)
{
    float distance = HCSR04_Read_cm();
    if (distance > 0 && distance < ERROR_ALLOWED_RANGE) {
        return 1; /* Object detected within error range */
    }
    return 0;
}
