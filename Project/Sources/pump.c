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
* @file pump.c
* @brief Implementacion del driver para la bomba de agua.
*
* Unifica las dos versiones del equipo. Se usa la version con PWM
* del proyecto ultrasonico (PA5, TIM2_CH1) en lugar de la version
* GPIO simple de flujo_wifi (PB6), ya que el PWM permite controlar
* la potencia de la bomba segun el nivel del deposito.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

/*** Includes ***/
#include "pump.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/

/*** Function Definitions ***/

/*
 * Inicializa el modulo PWM en PA5 a 1 kHz y deja la bomba apagada.
 */
void pump_init(void)
{
    pwm_init(PUMP_PWM_FREQ_HZ);
    pwm_setSignal(PUMP_DUTY_OFF);
    pwm_start();
}

/*
 * Ajusta la potencia de la bomba segun el nivel de agua del deposito.
 *   WATER_LEVEL_HIGH    -> 0%   (tanque lleno, apagar bomba)
 *   WATER_LEVEL_MID     -> 75%  (nivel medio)
 *   WATER_LEVEL_LOW     -> 100% (nivel bajo, maxima potencia)
 *   WATER_LEVEL_UNKNOWN -> 0%   (sin lectura valida, apagar por seguridad)
 */
void pump_setLevel(WaterLevel_t level)
{
    switch (level)
    {
        case WATER_LEVEL_HIGH:
            pwm_setSignal(PUMP_DUTY_OFF);
            break;
        case WATER_LEVEL_MID:
            pwm_setSignal(PUMP_DUTY_MID);
            break;
        case WATER_LEVEL_LOW:
            pwm_setSignal(PUMP_DUTY_LOW);
            break;
        case WATER_LEVEL_UNKNOWN:
        default:
            pwm_setSignal(PUMP_DUTY_OFF);
            break;
    }
}

/*
 * Apaga la bomba inmediatamente poniendo el duty cycle en 0%.
 * Llamar cuando se activa un STOP o al salir del loop principal.
 */
void pump_stop(void)
{
    pwm_setSignal(PUMP_DUTY_OFF);
}
