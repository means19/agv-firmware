/**
 * @file    line_sensor_weight.c
 * @brief   5-channel digital line sensor implementation.
 */

#include "line_sensor_weight.h"
#include "stm32f1xx_hal.h"

/* ---------------------------------------------------------------
 * Weights: index 0 = leftmost sensor (S1), index 4 = rightmost (S5).
 * Symmetric around centre (index 2 = 0.0).
 * --------------------------------------------------------------- */
static const float SENSOR_WEIGHTS[NUM_SENSORS] = {
    -2.0f, -1.0f, 0.0f, 1.0f, 2.0f
};

static const uint16_t SENSOR_PINS[NUM_SENSORS] = {
    SENSOR_S1_PIN,
    SENSOR_S2_PIN,
    SENSOR_S3_PIN,
    SENSOR_S4_PIN,
    SENSOR_S5_PIN
};

/* ---------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------- */

void LineSensor_Init(LineSensor *sensor)
{
    for (uint8_t i = 0u; i < NUM_SENSORS; i++) {
        sensor->raw[i] = 0u;
    }
    sensor->position      = LINE_LOST_VALUE;
    sensor->active_count  = 0u;
    sensor->line_detected = false;
    sensor->intersection  = false;
}

void LineSensor_Read(LineSensor *sensor)
{
    for (uint8_t i = 0u; i < NUM_SENSORS; i++) {
        GPIO_PinState pin_state = HAL_GPIO_ReadPin(SENSOR_GPIO_PORT, SENSOR_PINS[i]);
        sensor->raw[i] = (pin_state == SENSOR_LINE_LEVEL) ? 1u : 0u;
    }
}

void LineSensor_Compute(LineSensor *sensor)
{
    float   weighted_sum = 0.0f;
    uint8_t active       = 0u;

    for (uint8_t i = 0u; i < NUM_SENSORS; i++) {
        if (sensor->raw[i] != 0u) {
            weighted_sum += SENSOR_WEIGHTS[i];
            active++;
        }
    }

    sensor->active_count = active;
    sensor->intersection = (active >= INTERSECTION_THRESHOLD);

    if (active == 0u) {
        sensor->position      = LINE_LOST_VALUE;
        sensor->line_detected = false;
    } else {
        sensor->position      = weighted_sum / (float)active;
        sensor->line_detected = true;
    }
}

void LineSensor_Update(LineSensor *sensor)
{
    LineSensor_Read(sensor);
    LineSensor_Compute(sensor);
}