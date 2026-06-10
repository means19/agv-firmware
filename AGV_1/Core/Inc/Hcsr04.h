/**
 * @file    hcsr04.h
 * @brief   HC-SR04 Ultrasonic Sensor Driver (non-blocking)
 *          STM32F103C8T6 (Blue Pill), STM32 HAL, TIM2 + EXTI
 *
 * Usage overview:
 * - Configure ECHO pin as EXTI on both edges.
 * - Configure a timer to 1 MHz (1 tick = 1 us).
 * - Call HCSR04_Init() once.
 * - Periodically call HCSR04_Trigger() (>= 60 ms).
 * - Call HCSR04_Update() each loop for timeout handling.
 * - Call HCSR04_EXTI_Handler() from HAL_GPIO_EXTI_Callback().
 */

#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f1xx_hal.h"

/* ---------- Pin config ---------- */
#define TRIG_PORT   GPIOA
#define TRIG_PIN    GPIO_PIN_5

#define ECHO_PORT   GPIOA
#define ECHO_PIN    GPIO_PIN_8

/* ---------- Timing and thresholds ---------- */
#define HCSR04_TIMEOUT_MS       30U
#define HCSR04_NO_ECHO_CM        999.0f
#define HCSR04_SOUND_CM_PER_US   0.01715f
#define ERROR_ALLOWED_RANGE      15.0f

/* ---------- Driver state ---------- */
typedef enum
{
	HCSR04_STATE_IDLE = 0,
	HCSR04_STATE_WAIT_RISE,
	HCSR04_STATE_WAIT_FALL
} HCSR04_State;

typedef struct
{
	volatile HCSR04_State state;   /* Shared state between ISR and main */
	volatile uint16_t rise_us;     /* Timer capture at rising edge (us) */
	volatile uint32_t trigger_ms;  /* Timestamp when trigger was sent */
	volatile uint32_t rise_ms;     /* Timestamp when rising edge occurred */
	volatile float distance_cm;    /* Latest computed distance */
	volatile uint8_t new_data;     /* 1 when distance_cm is updated */
} HCSR04_Context;

extern volatile HCSR04_Context g_hcsr04;

/* ---------- API ---------- */
/**
 * @brief  Initialize driver. Call once after MX_TIM2_Init() and MX_GPIO_Init().
 * @param  htim  Pointer to timer configured for 1 MHz (1 tick = 1 us).
 */
void HCSR04_Init(TIM_HandleTypeDef *htim);

/**
 * @brief  Send a 10 us trigger pulse and arm the state machine.
 * @note   Call no faster than every 60 ms.
 */
void HCSR04_Trigger(void);

/**
 * @brief  Handle timeout when no echo is received.
 * @note   Call this in the main loop (non-blocking).
 */
void HCSR04_Update(void);

/**
 * @brief  EXTI handler for ECHO pin. Call from HAL_GPIO_EXTI_Callback().
 * @param  GPIO_Pin  Pin passed by HAL.
 */
void HCSR04_EXTI_Handler(uint16_t GPIO_Pin);

/**
 * @brief  Get new distance if available (clears new_data flag).
 * @param  out_cm  Pointer to store distance in cm.
 * @return 1 if new data was returned, 0 otherwise.
 */
uint8_t HCSR04_TryGetDistanceCm(float *out_cm);

/**
 * @brief  Get the latest measured distance (may be stale).
 */
float HCSR04_GetLastDistanceCm(void);

/**
 * @brief  Convenience helper for obstacle detection threshold.
 * @return 1 if distance is valid and within ERROR_ALLOWED_RANGE.
 */
int Object_detected(void);
#endif /* HCSR04_H */
