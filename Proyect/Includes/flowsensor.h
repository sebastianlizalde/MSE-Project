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
* @file flowsensor.h
* @brief Driver para los dos sensores de flujo YF-S201 del proyecto.
*
* Mide el caudal de agua que pasa por cada seccion contando los pulsos
* que genera el sensor. A mayor caudal, mas pulsos por segundo.
*
* Pines:
*   PA0 - Sensor de flujo seccion A
*   PB0 - Sensor de flujo seccion B
*
* Conexion: VCC -> 5V, GND -> GND, SIGNAL -> pin indicado.
* El pin de senal necesita resistencia pull-up de 10k a 3.3V.
*
* Factor de calibracion YF-S201: 7.5 pulsos por litro/minuto.
* Ajustar FLOW_PULSES_PER_LITER_MIN segun el modelo exacto del sensor.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

#ifndef __FLOWSENSOR_H__
#define __FLOWSENSOR_H__

/*** Includes ***/
#include "gpio_driver.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Pines de los dos sensores */
#define FLOW_A_PORT     GPIO_PORT_A
#define FLOW_A_PIN      0U      /* PA0 - Flujo seccion A */

#define FLOW_B_PORT     GPIO_PORT_B
#define FLOW_B_PIN      0U      /* PB0 - Flujo seccion B */

/* Factor de calibracion del sensor (pulsos por litro por minuto).
 * YF-S201: 7.5  |  YF-B1: 5.5  |  YF-B5: 1.5
 * Ajustar segun el modelo exacto del sensor. */
#define FLOW_PULSES_PER_LITER_MIN   7.5f

/*** Type Prototypes ***/

/* Identificador de seccion */
typedef enum
{
    FLOW_SECTION_A = 0,
    FLOW_SECTION_B = 1
} Flow_Section_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa los dos sensores de flujo */
void flow_init(void);

/* Debe llamarse en cada iteracion del loop para contar pulsos */
void flow_update(void);

/* Retorna el numero de pulsos acumulados de una seccion */
uint32_t flow_getPulses(Flow_Section_t section);

/* Reinicia el contador de pulsos de una seccion a cero */
void flow_resetPulses(Flow_Section_t section);

/* Convierte pulsos por segundo a litros por minuto */
float flow_getLitersPerMin(uint32_t pulses_per_second);

#endif /* __FLOWSENSOR_H__ */
