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
* @file gpio_driver.h
* @brief Driver para controlar los pines GPIO del STM32F411RE.
*
* Permite configurar pines como entrada, salida o funcion alternativa,
* leerlos y escribirlos. Todos los modulos del proyecto usan este driver.
*
* @author Luis Angel Lugo Muniz, Carlos Araiza, Kheara Kieley
* @date Mayo 2026
*
*/

#ifndef __GPIO_DRIVER_H__
#define __GPIO_DRIVER_H__

/*** Includes ***/
#include "stm32f4xx.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/* Puertos disponibles */
typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_H = 7,
    GPIO_PORT_MAX
} GPIO_Port_t;

/* Modo del pin */
typedef enum
{
    GPIO_MODE_INPUT  = 0x00U,   /* Entrada digital */
    GPIO_MODE_OUTPUT = 0x01U,   /* Salida digital  */
    GPIO_MODE_ALT_FN = 0x02U,   /* Funcion alternativa (UART, I2C, TIM...) */
    GPIO_MODE_ANALOG = 0x03U    /* Analogico (ADC) */
} GPIO_Mode_t;

/* Resistencia interna del pin */
typedef enum
{
    GPIO_PULL_NONE = 0x00U,     /* Sin resistencia */
    GPIO_PULL_UP   = 0x01U,     /* Pull-up */
    GPIO_PULL_DOWN = 0x02U      /* Pull-down */
} GPIO_Pull_t;

/* Tipo de salida */
typedef enum
{
    GPIO_OTYPE_PUSH_PULL  = 0x00U,  /* Push-pull (normal) */
    GPIO_OTYPE_OPEN_DRAIN = 0x01U   /* Open-drain (para I2C) */
} GPIO_OType_t;

/* Velocidad del pin */
typedef enum
{
    GPIO_SPEED_LOW    = 0x00U,
    GPIO_SPEED_MEDIUM = 0x01U,
    GPIO_SPEED_HIGH   = 0x02U,
    GPIO_SPEED_VHIGH  = 0x03U
} GPIO_Speed_t;

/* Estado logico del pin */
typedef enum
{
    GPIO_PIN_LOW  = 0,
    GPIO_PIN_HIGH = 1
} GPIO_PinState_t;

/* Codigos de retorno */
typedef enum
{
    GPIO_OK               =  0,
    GPIO_ERR_INVALID_PORT = -1,
    GPIO_ERR_INVALID_PIN  = -2,
    GPIO_ERR_INVALID_MODE = -3,
    GPIO_ERR_NULL_PTR     = -4,
    GPIO_ERR_INVALID_AF   = -5
} GPIO_Status_t;

/* Configuracion completa de un pin */
typedef struct
{
    GPIO_Mode_t  mode;
    GPIO_Pull_t  pull;
    GPIO_OType_t otype;
    GPIO_Speed_t speed;
} GPIO_PinCfg_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Inicializa todos los puertos GPIO */
void          gpio_init(void);

/* Habilita el reloj de un puerto especifico */
GPIO_Status_t gpio_initPort(GPIO_Port_t port);

/* Configura el modo, velocidad y resistencia de un pin */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg);

/* Pone el pin en HIGH */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin);

/* Pone el pin en LOW */
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin);

/* Invierte el estado actual del pin */
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin);

/* Lee el estado digital del pin */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state);

/* Configura la funcion alternativa de un pin (AF0-AF15) */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af);

/* Configura la resistencia pull-up o pull-down de un pin */
GPIO_Status_t gpio_setPullUpDown(GPIO_Port_t port, uint8_t pin, GPIO_Pull_t pull);

#endif /* __GPIO_DRIVER_H__ */
