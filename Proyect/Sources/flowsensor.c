/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
*
* Redistribution, modification or use of this software in source or binary
* forms is permitted as long as the files maintain this copyright. Users are
* permitted to modify this and use it to learn about the field of embedded
* software. Carlos Villarreal and CETYS Universidad are not liable for any
* misuse of this material.
*
*****************************************************************************/
/**
* @file flowsensor.c
* @brief Driver implementation for the two YF-S201 flow sensors.
*
* Detects rising edges on each pin to count pulses.
* flow_update() is called in the main loop to update the counters.
*
* Changes from original version:
*   - Added support for two sensors (section A on PA0, section B on PB0)
*   - API updated with Flow_Section_t to identify each section
*   - Pins updated to avoid conflicts with other modules
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date May 2026
*
*/

/*** Includes ***/
#include "flowsensor.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/* Pulse counters for each section */
static uint32_t s_pulses_a = 0U;
static uint32_t s_pulses_b = 0U;

/* Last read state of each sensor (to detect rising edge) */
static GPIO_PinState_t s_last_a = GPIO_PIN_LOW;
static GPIO_PinState_t s_last_b = GPIO_PIN_LOW;

/*** External Variables ***/

/*** Function Prototypes ***/

/*** Function Definitions ***/

/*
 * Initializes the two flow sensor pins as inputs
 * with internal pull-down to prevent false readings when the sensor
 * is not generating pulses.
 *
 * Sensor A: PA0
 * Sensor B: PB0
 */
void flow_init(void)
{
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_INPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    cfg.pull  = GPIO_PULL_DOWN;

    /* Section A sensor on PA0 */
    gpio_initPort(FLOW_A_PORT);
    gpio_setPinMode(FLOW_A_PORT, FLOW_A_PIN, &cfg);

    /* Section B sensor on PB0 */
    gpio_initPort(FLOW_B_PORT);
    gpio_setPinMode(FLOW_B_PORT, FLOW_B_PIN, &cfg);

    /* Read initial state to avoid false edge on startup */
    gpio_readPin(FLOW_A_PORT, FLOW_A_PIN, &s_last_a);
    gpio_readPin(FLOW_B_PORT, FLOW_B_PIN, &s_last_b);

    s_pulses_a = 0U;
    s_pulses_b = 0U;
}

/*
 * Checks the current state of both sensors and counts rising
 * edges (LOW -> HIGH) as pulses.
 * Must be called on every iteration of the main loop.
 */
void flow_update(void)
{
    GPIO_PinState_t state_a, state_b;

    gpio_readPin(FLOW_A_PORT, FLOW_A_PIN, &state_a);
    gpio_readPin(FLOW_B_PORT, FLOW_B_PIN, &state_b);

    /* Rising edge on sensor A */
    if ((state_a == GPIO_PIN_HIGH) && (s_last_a == GPIO_PIN_LOW))
    {
        s_pulses_a++;
    }

    /* Rising edge on sensor B */
    if ((state_b == GPIO_PIN_HIGH) && (s_last_b == GPIO_PIN_LOW))
    {
        s_pulses_b++;
    }

    s_last_a = state_a;
    s_last_b = state_b;
}

/*
 * Returns the accumulated pulse count of the given section.
 */
uint32_t flow_getPulses(Flow_Section_t section)
{
    return (section == FLOW_SECTION_A) ? s_pulses_a : s_pulses_b;
}

/*
 * Resets the pulse counter of the given section to zero.
 * Call after computing flow rate to start a new interval.
 */
void flow_resetPulses(Flow_Section_t section)
{
    if (section == FLOW_SECTION_A)
        s_pulses_a = 0U;
    else
        s_pulses_b = 0U;
}

/*
 * Converts pulses per second to liters per minute using the sensor
 * calibration factor.
 * Formula: flow (L/min) = pulses_per_second / FLOW_PULSES_PER_LITER_MIN
 */
float flow_getLitersPerMin(uint32_t pulses_per_second)
{
    return (float)pulses_per_second / FLOW_PULSES_PER_LITER_MIN;
}
