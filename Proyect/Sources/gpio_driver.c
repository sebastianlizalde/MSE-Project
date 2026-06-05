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
* @file gpio_driver.c
* @brief Implementacion del driver GPIO para el STM32F411RE.
*
* Contiene todas las funciones para configurar y controlar los pines GPIO.
* Unifica las dos versiones del equipo en una sola API comun.
*
* @author Luis Angel Lugo Muniz, Carlos Araiza, Kheara Kieley
* @date Mayo 2026
*
*/

/*** Includes ***/
#include "gpio_driver.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/
static GPIO_TypeDef  *prv_getPort(GPIO_Port_t port);
static GPIO_Status_t  prv_validate(GPIO_Port_t port, uint8_t pin);

/*** Function Definitions ***/

/*
 * Habilita el reloj de todos los puertos y los deja en estado por defecto.
 * Debe llamarse una sola vez al inicio del programa.
 */
void gpio_init(void)
{
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN |
                     RCC_AHB1ENR_GPIOCEN  | RCC_AHB1ENR_GPIODEN |
                     RCC_AHB1ENR_GPIOEEN  | RCC_AHB1ENR_GPIOHEN);

    /* Estado por defecto al encender el micro */
    GPIOA->MODER = 0xA8000000UL;
    GPIOB->MODER = 0x00000280UL;
    GPIOC->MODER = 0x00000000UL;
    GPIOD->MODER = 0x00000000UL;
    GPIOE->MODER = 0x00000000UL;
    GPIOH->MODER = 0x00000000UL;
}

/*
 * Habilita el reloj de un solo puerto.
 * Util cuando no se quiere habilitar todos los puertos a la vez.
 */
GPIO_Status_t gpio_initPort(GPIO_Port_t port)
{
    if (port >= GPIO_PORT_MAX) return GPIO_ERR_INVALID_PORT;

    RCC->AHB1ENR |= (1UL << port);
    (void)RCC->AHB1ENR;  /* Lectura de sincronizacion */

    return GPIO_OK;
}

/*
 * Configura el modo, tipo de salida, velocidad y resistencia de un pin.
 * Recibe una estructura con todos los parametros juntos.
 */
GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, const GPIO_PinCfg_t *cfg)
{
    if (cfg == (GPIO_PinCfg_t *)0) return GPIO_ERR_NULL_PTR;

    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    GPIOx->MODER   &= ~(0x3UL << (pin * 2U));
    GPIOx->MODER   |=  ((uint32_t)cfg->mode  << (pin * 2U));

    GPIOx->OTYPER  &= ~(0x1UL << pin);
    GPIOx->OTYPER  |=  ((uint32_t)cfg->otype << pin);

    GPIOx->OSPEEDR &= ~(0x3UL << (pin * 2U));
    GPIOx->OSPEEDR |=  ((uint32_t)cfg->speed << (pin * 2U));

    GPIOx->PUPDR   &= ~(0x3UL << (pin * 2U));
    GPIOx->PUPDR   |=  ((uint32_t)cfg->pull  << (pin * 2U));

    return GPIO_OK;
}

/*
 * Pone el pin en HIGH de forma atomica usando el registro BSRR.
 */
GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->BSRR = (1UL << pin);
    return GPIO_OK;
}

/*
 * Pone el pin en LOW de forma atomica usando el registro BSRR.
 */
GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->BSRR = (1UL << (pin + 16U));
    return GPIO_OK;
}

/*
 * Invierte el estado actual del pin usando el registro ODR.
 */
GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    prv_getPort(port)->ODR ^= (1UL << pin);
    return GPIO_OK;
}

/*
 * Lee el estado del pin (HIGH o LOW) desde el registro IDR.
 */
GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, GPIO_PinState_t *state)
{
    if (state == (GPIO_PinState_t *)0) return GPIO_ERR_NULL_PTR;

    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    *state = (prv_getPort(port)->IDR & (1UL << pin)) ? GPIO_PIN_HIGH : GPIO_PIN_LOW;
    return GPIO_OK;
}

/*
 * Configura la funcion alternativa de un pin (AF0-AF15).
 * Tambien pone el pin en modo ALT_FN automaticamente.
 */
GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, uint8_t af)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;
    if (af > 15U) return GPIO_ERR_INVALID_AF;

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    /* AFR[0] = pines 0-7, AFR[1] = pines 8-15 */
    uint8_t idx   = pin >> 3U;
    uint8_t shift = (pin & 0x7U) << 2U;

    GPIOx->AFR[idx] &= ~(0xFUL << shift);
    GPIOx->AFR[idx] |=  ((uint32_t)af << shift);

    return GPIO_OK;
}

/*
 * Configura la resistencia pull-up o pull-down de un pin.
 * Agregada para compatibilidad con los botones del proyecto.
 */
GPIO_Status_t gpio_setPullUpDown(GPIO_Port_t port, uint8_t pin, GPIO_Pull_t pull)
{
    GPIO_Status_t status = prv_validate(port, pin);
    if (status != GPIO_OK) return status;

    GPIO_TypeDef *GPIOx = prv_getPort(port);

    GPIOx->PUPDR &= ~(0x3UL << (pin * 2U));
    GPIOx->PUPDR |=  ((uint32_t)pull << (pin * 2U));

    return GPIO_OK;
}

/* --- Funciones internas --- */

/* Convierte el enum de puerto al puntero del registro correspondiente */
static GPIO_TypeDef *prv_getPort(GPIO_Port_t port)
{
    switch (port)
    {
        case GPIO_PORT_A: return GPIOA;
        case GPIO_PORT_B: return GPIOB;
        case GPIO_PORT_C: return GPIOC;
        case GPIO_PORT_D: return GPIOD;
        case GPIO_PORT_E: return GPIOE;
        case GPIO_PORT_H: return GPIOH;
        default:          return (GPIO_TypeDef *)0;
    }
}

/* Verifica que el puerto y el pin sean validos antes de operar */
static GPIO_Status_t prv_validate(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX) return GPIO_ERR_INVALID_PORT;
    if (pin  >  15U)           return GPIO_ERR_INVALID_PIN;
    return GPIO_OK;
}
