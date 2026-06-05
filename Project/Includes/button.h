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
* @file button.h
* @brief Driver para los tres botones de paro del sistema de riego.
*
* Maneja tres botones independientes, cada uno con logica toggle:
* al presionarlo para el sistema, al presionarlo de nuevo lo reanuda.
*
* Botones:
*   PC13 - STOP global: para o reanuda TODO el sistema
*   PE0  - STOP-A:      para o reanuda solo la seccion A
*   PE1  - STOP-B:      para o reanuda solo la seccion B
*
* Conexion: un extremo del boton al pin, el otro a GND.
* Pull-up interno habilitado — en reposo el pin lee HIGH,
* al presionar lee LOW.
*
* Los estados de paro se leen desde otros modulos con las
* funciones button_isStoppedGlobal(), button_isStoppedA() y
* button_isStoppedB().
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

#ifndef __BUTTON_H__
#define __BUTTON_H__

/*** Includes ***/
#include "gpio_driver.h"
#include "stm32f4xx.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Pines de los tres botones */
#define BTN_GLOBAL_PORT     GPIO_PORT_C
#define BTN_GLOBAL_PIN      13U     /* PC13 - STOP global         */

#define BTN_A_PORT          GPIO_PORT_B
#define BTN_A_PIN           13U     /* PB13 - STOP seccion A      */

#define BTN_B_PORT          GPIO_PORT_B
#define BTN_B_PIN           14U     /* PB14 - STOP seccion B      */

/* Tiempo minimo entre presiones para evitar rebotes (ms) */
#define BTN_DEBOUNCE_MS     50U

/*** Type Prototypes ***/

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa los tres botones con pull-up interno */
void button_init(void);

/* Revisa el estado de los botones — llamar en cada iteracion del loop */
void button_poll(void);

/* Retorna 1 si el sistema esta en paro global, 0 si esta activo */
uint8_t button_isStoppedGlobal(void);

/* Retorna 1 si la seccion A esta en paro, 0 si esta activa */
uint8_t button_isStoppedA(void);

/* Retorna 1 si la seccion B esta en paro, 0 si esta activa */
uint8_t button_isStoppedB(void);

#endif /* __BUTTON_H__ */
