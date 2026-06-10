/**
 *  5-channel digital line sensor with weighted position output.
 *
 * Sensor layout (left → right):
 *   S1   S2   S3   S4   S5
 *   -2   -1    0   +1   +2   ← position weights
 *
 * Computed position:
 *   • Range [-2.0, +2.0]  — weighted average of active sensors
 *   • 0.0                 — perfectly centred
 *   • Negative            — line is to the left
 *   • Positive            — line is to the right
 *   • LINE_LOST_VALUE     — no sensor active (line lost)
 *
 * Intersection detection:
 *   • active_count >= INTERSECTION_THRESHOLD sensors simultaneously
 *     active indicates a cross-section or T-junction.
 */

#ifndef LINE_SENSOR_WEIGHT_H
#define LINE_SENSOR_WEIGHT_H

#include <stdint.h>
#include <stdbool.h>

/* ---------------------------------------------------------------
 * Configuration
 * --------------------------------------------------------------- */
#define NUM_SENSORS              5u
#define LINE_LOST_VALUE          999.0f
#define INTERSECTION_THRESHOLD   4u     /* ≥4 sensors = intersection */

/* ---------------------------------------------------------------
 * GPIO mapping — adjust to match your IOC / board layout.
 * From the IOC: PA0–PA4 are configured as GPIO_Input.
 * --------------------------------------------------------------- */
#define SENSOR_GPIO_PORT    GPIOA
#define SENSOR_S1_PIN       GPIO_PIN_0   /* leftmost  */
#define SENSOR_S2_PIN       GPIO_PIN_1
#define SENSOR_S3_PIN       GPIO_PIN_2   /* centre    */
#define SENSOR_S4_PIN       GPIO_PIN_3
#define SENSOR_S5_PIN       GPIO_PIN_4   /* rightmost */

/*
 * Logic polarity: set to 1 if a dark line reads GPIO_PIN_SET,
 *                 set to 0 if a dark line reads GPIO_PIN_RESET.
 * This is the only place you need to change for inverted sensors.
 */
#define SENSOR_LINE_LEVEL   GPIO_PIN_RESET

/* ---------------------------------------------------------------
 * Data structure
 * --------------------------------------------------------------- */
typedef struct {
    uint8_t  raw[NUM_SENSORS];   /* 1 = line under sensor, 0 = no line */
    float    position;           /* weighted average; LINE_LOST_VALUE if lost */
    uint8_t  active_count;       /* number of sensors currently seeing line  */
    bool     line_detected;      /* convenience flag: position != LINE_LOST_VALUE */
    bool     intersection;       /* true when active_count >= threshold       */
} LineSensor;

/* ---------------------------------------------------------------
 * Public API
 * --------------------------------------------------------------- */

/** @brief Initialise the sensor struct to a known state. */
void LineSensor_Init(LineSensor *sensor);

/**
 * @brief  Read all 5 GPIO pins and populate sensor->raw[].
 *         Call this before LineSensor_Compute().
 */
void LineSensor_Read(LineSensor *sensor);

/**
 * @brief  Compute weighted position from sensor->raw[].
 *         Updates position, active_count, line_detected, intersection.
 */
void LineSensor_Compute(LineSensor *sensor);

/**
 * @brief  Convenience: read + compute in one call.
 *         Use this in the main control loop.
 */
void LineSensor_Update(LineSensor *sensor);

#endif /* LINE_SENSOR_WEIGHT_H */
