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
* @file irrigation.c
* @brief Irrigation control module implementation.
*
* Changes from original version:
*   - Section A LED moved from PA5 to PB12 (PA5 is now the pump PWM)
*   - LCD updated to new addr API: lcd_setCursor(addr, row, col)
*     and lcd_writeString(addr, str) using LCD_ADDR_SECCIONES (0x27)
*   - Button logic integrated: if global stop or section stop is active,
*     the valve closes and the LED turns off until resumed
*   - FIX: LED is now updated directly from the sensor reading each cycle,
*     regardless of whether the valve state changed.
*     LED ON  = humidity < 30% (dry soil, valve should open)
*     LED OFF = humidity >= 30% (moist soil, valve closed or half)
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date May 2026
*
*/

/*** Includes ***/
#include "irrigation.h"
#include "servo.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/* Internal state of each valve */
typedef enum
{
    VALVE_IDLE   = 0,   /* Waiting for humidity to drop below 30% */
    VALVE_FULL   = 1,   /* Servo 180 deg — humidity between 0% and 60%  */
    VALVE_HALF   = 2,   /* Servo 90 deg  — humidity between 60% and 80% */
    VALVE_CLOSED = 3    /* Servo 0 deg   — humidity above 80%      */
} ValveState_t;

/* Data for each irrigation section */
typedef struct
{
    GPIO_Port_t  ledPort;
    uint8_t      ledPin;
    ValveState_t state;
} IrrigationSection_t;

/*** Local Variables ***/

static uint8_t s_initialized = 0U;

/* Initial configuration of both sections with their LEDs */
static IrrigationSection_t s_sections[SOIL_SECTION_MAX] =
{
    { IRRIGATION_LED_PORT_A, IRRIGATION_LED_PIN_A, VALVE_IDLE },  /* Section A */
    { IRRIGATION_LED_PORT_B, IRRIGATION_LED_PIN_B, VALVE_IDLE }   /* Section B */
};

/*** External Variables ***/

/*** Function Prototypes ***/
static GPIO_Status_t prv_initOutputPin(GPIO_Port_t port, uint8_t pin);
static void          prv_applyValveState(uint8_t i, ValveState_t newState);

/*** Function Definitions ***/

/*
 * Configures a pin as a digital push-pull output.
 */
static GPIO_Status_t prv_initOutputPin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_OUTPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    cfg.pull  = GPIO_PULL_NONE;
    return gpio_setPinMode(port, pin, &cfg);
}

/*
 * Applies a valve state to a section:
 * only moves the servo — the LED is handled directly in irrigation_update()
 * based on the sensor reading.
 */
static void prv_applyValveState(uint8_t i, ValveState_t newState)
{
    SoilMoisture_Section_t section = (SoilMoisture_Section_t)i;

    s_sections[i].state = newState;

    /* Move the servo physically — without this the state changes in memory
     * but the servo never receives the signal. */
    switch (newState)
    {
        case VALVE_FULL:
            servo_open(section);
            break;
        case VALVE_HALF:
            servo_setAngle(section, 90U);
            break;
        case VALVE_CLOSED:
        case VALVE_IDLE:
        default:
            servo_close(section);
            break;
    }
}

/* ------------------------------------------------------------------ */

/*
 * Initializes the test LEDs for both sections and the servo motors.
 * Everything starts in the closed state.
 */
Irrigation_Status_t irrigation_init(void)
{
    uint8_t i;

    /* Enable LED ports */
    gpio_initPort(IRRIGATION_LED_PORT_A);
    gpio_initPort(IRRIGATION_LED_PORT_B);

    for (i = 0U; i < (uint8_t)SOIL_SECTION_MAX; i++)
    {
        if (prv_initOutputPin(s_sections[i].ledPort,
                              s_sections[i].ledPin) != GPIO_OK)
        {
            return IRRIGATION_ERR_INIT;
        }
        gpio_clearPin(s_sections[i].ledPort, s_sections[i].ledPin);
        s_sections[i].state = VALVE_IDLE;
    }

    if (servo_init() != SERVO_OK)
    {
        return IRRIGATION_ERR_INIT;
    }

    s_initialized = 1U;
    return IRRIGATION_OK;
}

/*
 * Reads the humidity sensors and updates the state of each valve.
 * Respects stop buttons: if a section is stopped, closes its valve
 * and turns off its LED regardless of humidity.
 *
 * The LED is updated EVERY cycle directly from the sensor:
 *   percent < 30%  -> LED ON  (dry soil, valve open or opening)
 *   percent >= 30% -> LED OFF (soil has enough moisture)
 *
 * State machine with hysteresis (for the servo):
 *   IDLE / CLOSED: if humidity < 30% -> open (FULL)
 *   FULL:          if humidity >= 60% -> move to half (HALF)
 *   HALF:          if humidity >= 80% -> close (CLOSED)
 *                  if humidity < 30%  -> reopen (FULL)
 */
Irrigation_Status_t irrigation_update(void)
{
    uint8_t             i;
    uint8_t             percent;
    Irrigation_Status_t result = IRRIGATION_OK;

    if (s_initialized == 0U) return IRRIGATION_ERR_INIT;

    for (i = 0U; i < (uint8_t)SOIL_SECTION_MAX; i++)
    {
        SoilMoisture_Section_t section = (SoilMoisture_Section_t)i;
        ValveState_t           current = s_sections[i].state;

        /* If global stop or section stop is active, close and skip */
        uint8_t stopped = button_isStoppedGlobal() ||
                          ((i == 0U) ? button_isStoppedA() : button_isStoppedB());

        if (stopped)
        {
            if (current != VALVE_CLOSED && current != VALVE_IDLE)
            {
                prv_applyValveState(i, VALVE_CLOSED);
            }
            /* LED off when section is stopped */
            gpio_clearPin(s_sections[i].ledPort, s_sections[i].ledPin);
            continue;
        }

        /* Read humidity sensor */
        if (soilMoisture_readPercent(section, &percent) != SOIL_OK)
        {
            result = IRRIGATION_ERR_SENSOR;
            continue;
        }

        /* --- LED updated directly from sensor, every cycle ---
         * Always updated, regardless of state changes.
         * LED ON  = dry soil (needs irrigation)
         * LED OFF = moist soil (no irrigation needed)
         */
        if (percent < IRRIGATION_THRESHOLD_TRIGGER)
        {
            gpio_setPin(s_sections[i].ledPort, s_sections[i].ledPin);
        }
        else
        {
            gpio_clearPin(s_sections[i].ledPort, s_sections[i].ledPin);
        }

        /* --- Direct servo control (same logic as the test main) ---
         *
         * servo_open or servo_close is called EVERY 500 ms cycle,
         * without a state machine. If sensor reads dry -> open.
         * If reads moist -> close. Same as:
         *   servo_open(section);  delay(3000);
         *   servo_close(section); delay(3000);
         * but controlled by humidity instead of fixed time.
         */
        if (percent < IRRIGATION_THRESHOLD_TRIGGER)
        {
            prv_applyValveState(i, VALVE_FULL);    /* open */
        }
        else
        {
            prv_applyValveState(i, VALVE_CLOSED);  /* closed */
        }
    }

    return result;
}

/*
 * Opens the valve of a section manually (servo to 180 deg).
 */
Irrigation_Status_t irrigation_openValve(SoilMoisture_Section_t section)
{
    if (section >= SOIL_SECTION_MAX) return IRRIGATION_ERR_INVALID;
    if (s_initialized == 0U)        return IRRIGATION_ERR_INIT;

    prv_applyValveState((uint8_t)section, VALVE_FULL);
    gpio_setPin(s_sections[(uint8_t)section].ledPort,
                s_sections[(uint8_t)section].ledPin);
    return IRRIGATION_OK;
}

/*
 * Closes the valve of a section manually (servo to 0 deg).
 */
Irrigation_Status_t irrigation_closeValve(SoilMoisture_Section_t section)
{
    if (section >= SOIL_SECTION_MAX) return IRRIGATION_ERR_INVALID;
    if (s_initialized == 0U)        return IRRIGATION_ERR_INIT;

    prv_applyValveState((uint8_t)section, VALVE_CLOSED);
    gpio_clearPin(s_sections[(uint8_t)section].ledPort,
                  s_sections[(uint8_t)section].ledPin);
    return IRRIGATION_OK;
}

/*
 * Returns the current valve state of a section as a 3-char string
 * for display on the LCD:
 *   "ABT" - open    (180 deg)
 *   "MED" - half    (90 deg)
 *   "CRR" - closed  (0 deg)
 *   "ESP" - waiting (IDLE)
 *   "---" - unknown
 */
const char *irrigation_getValveStateStr(SoilMoisture_Section_t section)
{
    if (section >= SOIL_SECTION_MAX) return "---";

    switch (s_sections[(uint8_t)section].state)
    {
        case VALVE_FULL:   return "ABT";
        case VALVE_HALF:   return "MED";
        case VALVE_CLOSED: return "CRR";
        case VALVE_IDLE:   return "ESP";
        default:           return "---";
    }
}