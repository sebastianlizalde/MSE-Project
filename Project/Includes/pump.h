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
* @file pump.h
* @brief Driver para el control de la bomba de agua del deposito.
*
* Controla la bomba mediante una senal PWM a traves de un transistor NPN
* (2N2222) ya que el pin del STM32 no puede entregar la corriente necesaria.
*
* Pin: PA5 -> TIM2_CH1 -> AF1 -> resistencia 330 ohm -> base transistor NPN
*
* La potencia de la bomba se ajusta segun el nivel de agua del deposito:
*   Nivel ALTO  (tanque lleno) -> bomba apagada   (0%)
*   Nivel MEDIO                -> bomba al 75%
*   Nivel BAJO  (tanque vacio) -> bomba al 100%
*   Sin senal                  -> bomba apagada   (0%)
*
* Cuando el STOP global o el STOP del deposito estan activos,
* la bomba se apaga independientemente del nivel de agua.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

#ifndef __PUMP_H__
#define __PUMP_H__

/*** Includes ***/
#include "pwm.h"
#include "ultrasonic.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Frecuencia de la senal PWM para la bomba (1 kHz es suficiente para DC) */
#define PUMP_PWM_FREQ_HZ    1000U

/* Niveles de potencia segun el nivel de agua */
#define PUMP_DUTY_OFF        0U    /* Tanque lleno o sin senal  */
#define PUMP_DUTY_MID       75U    /* Nivel medio               */
#define PUMP_DUTY_LOW      100U    /* Nivel bajo, maximo caudal */

/*** Type Prototypes ***/

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa el PWM para la bomba con duty cycle en 0% */
void pump_init(void);

/* Ajusta la potencia de la bomba segun el nivel de agua */
void pump_setLevel(WaterLevel_t level);

/* Apaga la bomba inmediatamente */
void pump_stop(void);

#endif /* __PUMP_H__ */
