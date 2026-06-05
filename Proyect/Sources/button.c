/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
******************************************************************************/
/**
* @file button.c
* @brief Driver para los tres botones de paro del sistema de riego.
*
* Polling con debounce independiente por boton. Cada boton tiene su propio
* timestamp de ultimo flanco para no bloquear los otros botones durante
* el periodo de debounce.
*
* Pines:
*   PC13 - STOP global
*   PB13 - STOP seccion A
*   PB14 - STOP seccion B
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*/

/*** Includes ***/
#include "button.h"
#include "systick.h"

/*** Local Variables ***/

/* Estados de paro de cada seccion */
static uint8_t s_stopped_global = 0U;
static uint8_t s_stopped_a      = 0U;
static uint8_t s_stopped_b      = 0U;

/* Ultimo estado leido de cada boton */
static GPIO_PinState_t s_last_global = GPIO_PIN_HIGH;
static GPIO_PinState_t s_last_a      = GPIO_PIN_HIGH;
static GPIO_PinState_t s_last_b      = GPIO_PIN_HIGH;

/* Timestamp de debounce independiente por boton */
static uint32_t s_t_global = 0U;
static uint32_t s_t_a      = 0U;
static uint32_t s_t_b      = 0U;

/*** Function Definitions ***/

/*
 * Inicializa los tres botones como entradas con pull-up interno.
 * En reposo cada pin lee HIGH. Al presionar el boton lee LOW.
 */
void button_init(void)
{
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_INPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    cfg.pull  = GPIO_PULL_UP;

    /* Boton global: PC13 */
    gpio_initPort(BTN_GLOBAL_PORT);
    gpio_setPinMode(BTN_GLOBAL_PORT, BTN_GLOBAL_PIN, &cfg);

    /* Boton seccion A: PB13 */
    gpio_initPort(BTN_A_PORT);
    gpio_setPinMode(BTN_A_PORT, BTN_A_PIN, &cfg);

    /* Boton seccion B: PB14 */
    gpio_initPort(BTN_B_PORT);
    gpio_setPinMode(BTN_B_PORT, BTN_B_PIN, &cfg);
}

/*
 * Revisa el estado de los tres botones y actualiza las banderas de paro.
 * Debe llamarse en cada iteracion del loop principal.
 *
 * Cada boton tiene su propio timestamp de debounce para que presionar
 * uno no bloquee la deteccion de los otros.
 */
void button_poll(void)
{
    GPIO_PinState_t now_global, now_a, now_b;
    uint32_t        tick = systick_getTick();

    gpio_readPin(BTN_GLOBAL_PORT, BTN_GLOBAL_PIN, &now_global);
    gpio_readPin(BTN_A_PORT,      BTN_A_PIN,      &now_a);
    gpio_readPin(BTN_B_PORT,      BTN_B_PIN,      &now_b);

    /* Boton global: flanco HIGH->LOW con debounce propio */
    if ((now_global == GPIO_PIN_LOW) && (s_last_global == GPIO_PIN_HIGH))
    {
        if ((tick - s_t_global) >= BTN_DEBOUNCE_MS)
        {
            s_t_global       = tick;
            s_stopped_global ^= 1U;
        }
    }

    /* Boton seccion A: flanco HIGH->LOW con debounce propio */
    if ((now_a == GPIO_PIN_LOW) && (s_last_a == GPIO_PIN_HIGH))
    {
        if ((tick - s_t_a) >= BTN_DEBOUNCE_MS)
        {
            s_t_a       = tick;
            s_stopped_a ^= 1U;
        }
    }

    /* Boton seccion B: flanco HIGH->LOW con debounce propio */
    if ((now_b == GPIO_PIN_LOW) && (s_last_b == GPIO_PIN_HIGH))
    {
        if ((tick - s_t_b) >= BTN_DEBOUNCE_MS)
        {
            s_t_b       = tick;
            s_stopped_b ^= 1U;
        }
    }

    s_last_global = now_global;
    s_last_a      = now_a;
    s_last_b      = now_b;
}

/* Retorna 1 si el sistema esta en paro global, 0 si esta activo */
uint8_t button_isStoppedGlobal(void) { return s_stopped_global; }

/* Retorna 1 si la seccion A esta en paro, 0 si esta activa */
uint8_t button_isStoppedA(void) { return s_stopped_a; }

/* Retorna 1 si la seccion B esta en paro, 0 si esta activa */
uint8_t button_isStoppedB(void) { return s_stopped_b; }