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
* @file irrigation.h
* @brief Modulo de control de riego para las dos secciones de plantas.
*
* Lee los sensores de humedad YL-69 y controla los servomotores
* que abren o cierran la llave de agua de cada seccion.
* Tambien enciende los LEDs de prueba segun el estado de cada valvula.
*
* Pines LEDs de prueba:
*   PB12 - LED seccion A (movido de PA5 para no chocar con la bomba)
*   PA6  - LED seccion B
*
* Logica de riego con histeresis:
*   Humedad < 30%  -> servo 180 grados (valvula completamente abierta)
*   Humedad = 60%  -> servo 90 grados  (valvula a la mitad)
*   Humedad = 80%  -> servo 0 grados   (valvula cerrada, ciclo terminado)
*   Se mantiene cerrada hasta que baje de 30% de nuevo
*
* Si los botones de paro estan activos, la seccion correspondiente
* se mantiene detenida independientemente de la humedad.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

#ifndef __IRRIGATION_H__
#define __IRRIGATION_H__

/*** Includes ***/
#include "gpio_driver.h"
#include "soil_moisture.h"
#include "button.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Pines de los LEDs de prueba de cada seccion */
#define IRRIGATION_LED_PORT_A   GPIO_PORT_B
#define IRRIGATION_LED_PIN_A    12U     /* PB12 - LED seccion A */

#define IRRIGATION_LED_PORT_B   GPIO_PORT_A
#define IRRIGATION_LED_PIN_B    6U      /* PA6  - LED seccion B */

/* Umbrales de humedad para la logica de riego */
#define IRRIGATION_THRESHOLD_TRIGGER    30U     /* % - inicia el ciclo de riego  */
#define IRRIGATION_THRESHOLD_HALF       60U     /* % - servo pasa a 90 grados    */
#define IRRIGATION_THRESHOLD_CLOSE      80U     /* % - servo cierra a 0 grados   */

/*** Type Prototypes ***/

/* Codigos de retorno */
typedef enum
{
    IRRIGATION_OK           =  0,
    IRRIGATION_ERR_INIT     = -1,
    IRRIGATION_ERR_INVALID  = -2,
    IRRIGATION_ERR_SENSOR   = -3
} Irrigation_Status_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa los LEDs y los servomotores de ambas secciones */
Irrigation_Status_t irrigation_init(void);

/* Lee los sensores y actualiza valvulas, LEDs y LCD de secciones */
Irrigation_Status_t irrigation_update(void);

/* Abre la valvula de una seccion manualmente (servo a 180 grados) */
Irrigation_Status_t irrigation_openValve(SoilMoisture_Section_t section);

/* Cierra la valvula de una seccion manualmente (servo a 0 grados) */
Irrigation_Status_t irrigation_closeValve(SoilMoisture_Section_t section);

/* Retorna el estado de la valvula como string de 3 chars para LCD */
const char *irrigation_getValveStateStr(SoilMoisture_Section_t section);

#endif /* __IRRIGATION_H__ */