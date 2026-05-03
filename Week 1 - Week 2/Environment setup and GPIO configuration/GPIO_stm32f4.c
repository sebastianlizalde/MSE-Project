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
* @file GPIO_stm32f4.c
* @brief Driver GPIO para STM32F4
*
* @author Sebastian Lizalde Herrera
* @date 23/04/2026
*/

/*** Includes ***/
#include <stdint.h>
#include "GPIO_stm32f4.h"

/*** Local Variables ***/

static GPIO_RegDef_t * const gpio_ports[GPIO_PORT_MAX] = {
    GPIOA,  // GPIO_PORT_A = 0
    GPIOB,  // GPIO_PORT_B = 1
    GPIOC,  // GPIO_PORT_C = 2
    GPIOD,  // GPIO_PORT_D = 3
    GPIOE,  // GPIO_PORT_E = 4
    GPIOH,  // GPIO_PORT_H = 5
};

static const uint32_t gpio_clk_masks[GPIO_PORT_MAX] = {
    RCC_AHB1ENR_GPIOAEN,  // GPIO_PORT_A
    RCC_AHB1ENR_GPIOBEN,  // GPIO_PORT_B
    RCC_AHB1ENR_GPIOCEN,  // GPIO_PORT_C
    RCC_AHB1ENR_GPIODEN,  // GPIO_PORT_D
    RCC_AHB1ENR_GPIOEEN,  // GPIO_PORT_E
    RCC_AHB1ENR_GPIOHEN,  // GPIO_PORT_H
};

/*** Function Definitions ***/

void gpio_init(void)
{
    RCC->AHB1ENR |= (RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIODEN | RCC_AHB1ENR_GPIOEEN | RCC_AHB1ENR_GPIOHEN);

    GPIOB->MODER = 0x00000000U;
    GPIOC->MODER = 0x00000000U;
    GPIOD->MODER = 0x00000000U;
    GPIOE->MODER = 0x00000000U;
    GPIOH->MODER = 0x00000000U;
}

GPIO_Status_t gpio_initPort(GPIO_Port_t port)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    RCC->AHB1ENR |= gpio_clk_masks[port];

    return GPIO_OK;
}

GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, GPIO_Mode_t mode)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    if (mode >= GPIO_MODE_MAX)
    {
        return GPIO_ERR_MODE;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    pGPIO->MODER &= ~(0x3U << (pin * 2U));
    pGPIO->MODER |=  ((uint32_t)mode << (pin * 2U));

    return GPIO_OK;
}

GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    pGPIO->BSRR = (1U << pin);

    return GPIO_OK;
}

GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    pGPIO->BSRR = (1U << (pin + 16U));

    return GPIO_OK;
}

GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    pGPIO->ODR ^= (1U << pin);

    return GPIO_OK;
}

GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, uint8_t *state)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    if (state == (void *)0)
    {
        return GPIO_ERR_NULL;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    *state = (uint8_t)((pGPIO->IDR >> pin) & 0x1U);

    return GPIO_OK;
}

GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, GPIO_AltFn_t af)
{
    if (port >= GPIO_PORT_MAX)
    {
        return GPIO_ERR_PORT;
    }

    if (pin > GPIO_PIN_MAX)
    {
        return GPIO_ERR_PIN;
    }

    if (af >= GPIO_AF_MAX)
    {
        return GPIO_ERR_ALTFN;
    }

    GPIO_RegDef_t *pGPIO = gpio_ports[port];

    if (pin <= 7U)
    {
        pGPIO->AFR[0] &= ~(0xFU << (pin * 4U));
        pGPIO->AFR[0] |=  ((uint32_t)af << (pin * 4U));
    }
    else
    {
        uint8_t pin_shifted = pin - 8U;
        pGPIO->AFR[1] &= ~(0xFU << (pin_shifted * 4U));
        pGPIO->AFR[1] |=  ((uint32_t)af << (pin_shifted * 4U));
    }

    return GPIO_OK;
}