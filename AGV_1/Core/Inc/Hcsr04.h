/**
 * @file    hcsr04.h
 * @brief   HC-SR04 Ultrasonic Sensor Driver
 *          STM32F103C8T6 (Blue Pill), STM32 HAL, TIM2
 *
 * How it works:
 *   1. Pull TRIG HIGH for 10 us to start a measurement
 *   2. Sensor sends 8 ultrasonic pulses at 40 kHz
 *   3. ECHO pin goes HIGH while waiting for the reflection
 *   4. Measure how long ECHO stays HIGH (in microseconds)
 *   5. Distance (cm) = echo_time_us x 0.01715
 *      (speed of sound 343 m/s at 20 C, divided by 2 for round-trip)
 *
 * Pins used
 *   TRIG --> PB1  (GPIO Output)
 *   ECHO --> PB11 (GPIO Input)
 *
 * Timer used:
 *   TIM2 -- free-running 1 MHz counter (1 tick = 1 us)
 *
 * CubeMX settings for TIM2:
 *   Clock Source : Internal Clock
 *   Prescaler    : 32 - 1      (32 MHz / 32 = 1 MHz)
 *   Counter Mode : Up
 *   Period       : 65535 - 1
 */

#ifndef HCSR04_H
#define HCSR04_H

#include "stm32f1xx_hal.h"

/* ---------- Pin config ---------- */
#define TRIG_PORT   GPIOB
#define TRIG_PIN    GPIO_PIN_1

#define ECHO_PORT   GPIOB
#define ECHO_PIN    GPIO_PIN_11

/* ---------- Timeout, ERROR RANGE ---------- */
/* HC-SR04 max range ~4 m = ~23 200 us echo time. 25 000 us is safe. */
#define HCSR04_TIMEOUT_US   25000UL
#define ERROR_ALLOWED_RANGE 15
/* ---------- API ---------- */

/**
 * @brief  Call once in main() after MX_TIM2_Init() and MX_GPIO_Init().
 * @param  htim  Pointer to htim2 
void HCSR04_Init(TIM_HandleTypeDef *htim);


 * @brief  Measure distance. Blocks for up to ~25 ms.
 * @return Distance in centimetres, or -1.0f on timeout (no obstacle).
 * @note   Do not call faster than every 60 ms.
 */
float HCSR04_Read_cm(void);
int Object_detected (void);
#endif /* HCSR04_H */
