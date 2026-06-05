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
* @file tim_driver.h
* @brief Driver para los timers del STM32F411RE.
*
* Permite configurar y controlar los timers TIM1 al TIM11.
* Se usa para generar delays, senales PWM y medir tiempo.
* Todos los modulos del proyecto que necesiten un timer usan este driver.
*
* Timers disponibles:
*   TIM1        - APB2, avanzado, 4 canales
*   TIM2, TIM5  - APB1, 32 bits, 4 canales
*   TIM3, TIM4  - APB1, 16 bits, 4 canales
*   TIM9        - APB2, 16 bits, 2 canales
*   TIM10, TIM11- APB2, 16 bits, 1 canal
*
* Reloj del sistema asumido: 16 MHz (HSI, sin PLL)
*
* @author Luis Angel Lugo Muniz, Carlos Araiza, Kheara Kieley
* @date Mayo 2026
*
*/

#ifndef __TIM_DRIVER_H__
#define __TIM_DRIVER_H__

/*** Includes ***/
#include "stm32f4xx.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* Frecuencia del reloj del sistema en Hz */
#define TIM_SYSCLK_HZ   16000000UL

/*** Type Prototypes ***/

/* Identificadores de cada timer */
typedef enum
{
    TIM_ID_1  = 0,
    TIM_ID_2,
    TIM_ID_3,
    TIM_ID_4,
    TIM_ID_5,
    TIM_ID_9,
    TIM_ID_10,
    TIM_ID_11,
    TIM_ID_MAX
} TIM_Id_t;

/* Canales de captura/comparacion */
typedef enum
{
    TIM_CH_1 = 1,
    TIM_CH_2 = 2,
    TIM_CH_3 = 3,
    TIM_CH_4 = 4
} TIM_Channel_t;

/* Modos de comparacion y PWM */
typedef enum
{
    TIM_COMPARE_MODE_FROZEN     = 0x0U,  /* Sin efecto en la salida    */
    TIM_COMPARE_MODE_ACTIVE     = 0x1U,  /* Activo al igualar          */
    TIM_COMPARE_MODE_INACTIVE   = 0x2U,  /* Inactivo al igualar        */
    TIM_COMPARE_MODE_TOGGLE     = 0x3U,  /* Invierte al igualar        */
    TIM_COMPARE_MODE_FORCE_LOW  = 0x4U,  /* Fuerza nivel bajo          */
    TIM_COMPARE_MODE_FORCE_HIGH = 0x5U,  /* Fuerza nivel alto          */
    TIM_COMPARE_MODE_PWM1       = 0x6U,  /* PWM modo 1 (alto < CCR)    */
    TIM_COMPARE_MODE_PWM2       = 0x7U   /* PWM modo 2 (bajo < CCR)    */
} TIM_CompareMode_t;

/* Codigos de retorno */
typedef enum
{
    TIM_OK                  =  0,
    TIM_ERR_INVALID_TIMER   = -1,
    TIM_ERR_INVALID_CHANNEL = -2,
    TIM_ERR_INVALID_PARAM   = -3,
    TIM_ERR_CHANNEL_UNSUP   = -4,   /* Canal no disponible en ese timer */
    TIM_BUSY                = -5    /* Timer todavia contando (no-bloqueante) */
} TIM_Status_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa todos los timers y habilita sus relojes */
void         tim_init(void);

/* Habilita el reloj de un timer especifico */
TIM_Status_t tim_initTimer(TIM_Id_t timer);

/* Configura el periodo del timer en milisegundos */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms);

/* Configura el timer para generar eventos a una frecuencia en Hz */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz);

/* Arranca el timer */
TIM_Status_t tim_enableTimer(TIM_Id_t timer);

/* Detiene el timer */
TIM_Status_t tim_disableTimer(TIM_Id_t timer);

/* Espera bloqueando hasta que el timer termine un periodo */
TIM_Status_t tim_waitTimer(TIM_Id_t timer);

/* Revisa si el timer termino sin bloquear (retorna TIM_BUSY si sigue contando) */
TIM_Status_t tim_isTimerExpired(TIM_Id_t timer);

/* Escribe el valor del CCR de un canal (define el duty cycle en PWM) */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value);

/* Configura el modo de comparacion o PWM de un canal */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t          timer,
                                     TIM_Channel_t     channel,
                                     TIM_CompareMode_t mode);

/* Habilita la salida de un canal de captura/comparacion */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel);

/* Deshabilita la salida de un canal de captura/comparacion */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel);

#endif /* __TIM_DRIVER_H__ */
