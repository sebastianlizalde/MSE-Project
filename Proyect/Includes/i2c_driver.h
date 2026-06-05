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
* @file i2c_driver.h
* @brief Driver para la comunicacion I2C del STM32F411RE.
*
* Controla el bus I2C1 en modo estandar a 100 kHz.
* Se usa para comunicarse con las dos pantallas LCD (0x27 y 0x26).
*
* Pines:
*   PB8 -> SCL (reloj)
*   PB9 -> SDA (datos)
*
* Incluye recuperacion automatica del bus en caso de que quede colgado,
* por ejemplo si la bomba genera ruido electrico durante una transmision.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*
*/

#ifndef __I2C_DRIVER_H__
#define __I2C_DRIVER_H__

/*** Includes ***/
#include "stm32f4xx.h"
#include "gpio_driver.h"
#include <stdint.h>
#include <stddef.h>

/*** Preprocessor Definitions ***/

/* Pines del bus I2C */
#define I2C_SCL_PORT        GPIO_PORT_B
#define I2C_SCL_PIN         8U
#define I2C_SDA_PORT        GPIO_PORT_B
#define I2C_SDA_PIN         9U
#define I2C_AF              4U          /* AF4 = I2C1 en PB8/PB9 */

/* Parametros de configuracion para 100 kHz con APB1 a 16 MHz */
#define I2C_CR2_FREQ_MHZ    16U
#define I2C_CCR_SM_100K     80U
#define I2C_TRISE_SM        17U

/* Tiempo maximo de espera antes de intentar recuperar el bus */
#define I2C_TIMEOUT         160000U

/*** Type Prototypes ***/

/* Codigos de retorno */
typedef enum
{
    I2C_OK            = 0,
    I2C_ERR           = 1,
    I2C_BUSY          = 2,
    I2C_TIMEOUT_ERROR = 3,
    I2C_INVALID       = 4
} I2C_Status_t;

/*** Global Variables ***/

/*** Function Prototypes ***/

/* Configura el bus I2C1 a 100 kHz con pines PB8/PB9 */
void         i2c_init(void);

/* Recupera el bus si quedo colgado (genera pulsos manuales de reloj) */
void         i2c_recover(void);

/* --- API de alto nivel (usada por los modulos del proyecto) --- */

/* Envia datos directamente a un dispositivo por su direccion I2C */
I2C_Status_t i2c_writeDevice(uint8_t dev_addr,
                              const uint8_t *data, uint32_t len);

/* Escribe datos en un registro especifico de un dispositivo */
I2C_Status_t i2c_writeRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                 const uint8_t *data, uint32_t len);

/* Lee datos desde un registro especifico de un dispositivo */
I2C_Status_t i2c_readRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                uint8_t *data, uint32_t len);

/* Lee datos directamente de un dispositivo */
I2C_Status_t i2c_readDevice(uint8_t dev_addr,
                             uint8_t *data, uint32_t len);

/* --- Low-level API (used internally by lcd.c) --- */

/* Genera la condicion de inicio (START) en el bus */
I2C_Status_t i2c_start(void);

/* Genera la condicion de parada (STOP) en el bus */
I2C_Status_t i2c_stop(void);

/* Envia la direccion del dispositivo con bit de lectura o escritura */
I2C_Status_t i2c_sendAddress(uint8_t address, uint8_t read);

/* Envia un byte de datos por el bus */
I2C_Status_t i2c_writeByte(uint8_t data);

#endif /* __I2C_DRIVER_H__ */
