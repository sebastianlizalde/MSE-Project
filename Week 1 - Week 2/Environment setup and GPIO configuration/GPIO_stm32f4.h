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
* @file GPIO_stm32f4.h
* @brief Defines the functions and peripherals to be used in GPIO Driver.
*
*
* @author Sebastian Lizalde Herrera
* @date 23/04/2026
*
*/

#ifndef __GPIO_DRIVER_H__
#define __GPIO_DRIVER_H__

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/

#define PERIPH_BASE         (0x40000000UL)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x00020000UL)
#define GPIOA_BASE          (AHB1PERIPH_BASE+0x0000UL)
#define GPIOB_BASE          (AHB1PERIPH_BASE+0x0400UL)
#define GPIOC_BASE          (AHB1PERIPH_BASE+0x0800UL)
#define GPIOD_BASE          (AHB1PERIPH_BASE+0x0C00UL)
#define GPIOE_BASE          (AHB1PERIPH_BASE+0x1000UL)
#define GPIOH_BASE          (AHB1PERIPH_BASE+0x1C00UL)
#define RCC_BASE            (AHB1PERIPH_BASE+0x3800UL)
#define GPIOA               ((GPIO_RegDef_t *) GPIOA_BASE)
#define GPIOB               ((GPIO_RegDef_t *) GPIOB_BASE)
#define GPIOC               ((GPIO_RegDef_t *) GPIOC_BASE)
#define GPIOD               ((GPIO_RegDef_t *) GPIOD_BASE)
#define GPIOE               ((GPIO_RegDef_t *) GPIOE_BASE)
#define GPIOH               ((GPIO_RegDef_t *) GPIOH_BASE)
#define RCC                 ((RCC_RegDef_t *) RCC_BASE)
#define RCC_AHB1ENR_GPIOAEN (1U << 0)
#define RCC_AHB1ENR_GPIOBEN (1U << 1)
#define RCC_AHB1ENR_GPIOCEN (1U << 2)
#define RCC_AHB1ENR_GPIODEN (1U << 3)
#define RCC_AHB1ENR_GPIOEEN (1U << 4)
#define RCC_AHB1ENR_GPIOHEN (1U << 7)
#define GPIO_PIN_MAX        15U

/*** Type Prototypes ***/

typedef struct
{
    volatile uint32_t MODER; //Offset 0x00 - modo de cada pin
    volatile uint32_t OTYPER; //Offset 0x04 - tipo de salida
    volatile uint32_t OSPEEDR; //Offset 0x08 - velocidad
    volatile uint32_t PUPDR; //Offset 0x0C - pull-up/pull-down
    volatile uint32_t IDR; //Offset 0x10 - leer entradas
    volatile uint32_t ODR; //Offset 0x14 - escribir salidas
    volatile uint32_t BSRR; //Offset 0x18 - set/reset atomico
    volatile uint32_t LCKR; //Offset 0x1C - bloqueo
    volatile uint32_t AFR[2]; //Offset 0x20 - funcion alterna
}GPIO_RegDef_t;

typedef struct 
{
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t RESERVED0[2];
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    volatile uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR; // GPIO clock Bit
}RCC_RegDef_t;

typedef enum
{
    GPIO_PORT_A = 0,
    GPIO_PORT_B,
    GPIO_PORT_C,
    GPIO_PORT_D,
    GPIO_PORT_E,
    GPIO_PORT_H,
    GPIO_PORT_MAX, // Sentinel, used for validation only
}GPIO_Port_t;

typedef enum
{
    GPIO_MODE_INPUT = 0x00, // 00 - digital input
    GPIO_MODE_OUTPUT = 0x01, // 01 - digital output
    GPIO_MODE_ALTFN = 0x02, // 10 - alternate function
    GPIO_MODE_ANALOG = 0x03, // 11 - analog
    GPIO_MODE_MAX //Sentinel
}GPIO_Mode_t;

typedef enum
{
    GPIO_OK = 0,
    GPIO_ERR_PORT = -1, //Invalid port
    GPIO_ERR_PIN = -2, //Invalid pin
    GPIO_ERR_MODE = -3, //Invalid mode
    GPIO_ERR_NULL = -4, //Null pointer
    GPIO_ERR_ALTFN = -5, //Invalid alternate function
}GPIO_Status_t;

typedef enum
{
    GPIO_AF0  = 0,
    GPIO_AF1  = 1,
    GPIO_AF2  = 2,
    GPIO_AF3  = 3,
    GPIO_AF4  = 4,
    GPIO_AF5  = 5,
    GPIO_AF6  = 6,
    GPIO_AF7  = 7,
    GPIO_AF8  = 8,
    GPIO_AF9  = 9,
    GPIO_AF10 = 10,
    GPIO_AF11 = 11,
    GPIO_AF12 = 12,
    GPIO_AF13 = 13,
    GPIO_AF14 = 14,
    GPIO_AF15 = 15,
    GPIO_AF_MAX,  // Sentinel
} GPIO_AltFn_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

void gpio_init(void); //FR-1

GPIO_Status_t gpio_initPort(GPIO_Port_t port); //FR-2

GPIO_Status_t gpio_setPinMode(GPIO_Port_t port, uint8_t pin, GPIO_Mode_t mode); //FR-3

GPIO_Status_t gpio_setPin(GPIO_Port_t port, uint8_t pin); //FR-4

GPIO_Status_t gpio_clearPin(GPIO_Port_t port, uint8_t pin); //FR-5

GPIO_Status_t gpio_togglePin(GPIO_Port_t port, uint8_t pin); //FR-6

GPIO_Status_t gpio_readPin(GPIO_Port_t port, uint8_t pin, uint8_t *state); //FR-7

GPIO_Status_t gpio_setAlternateFunction(GPIO_Port_t port, uint8_t pin, GPIO_AltFn_t af);  // FR-8

#endif /* __GPIO_DRIVER_H__ */
