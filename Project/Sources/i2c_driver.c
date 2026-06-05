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
* @file i2c_driver.c
* @brief Implementacion del driver I2C para el STM32F411RE.
*
* Correccion principal: se agrego un delay minimo entre transacciones
* I2C para evitar que el PCF8574 de la LCD reciba datos corruptos
* que causan caracteres raros en pantalla.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date Mayo 2026
*/

/*** Includes ***/
#include "i2c_driver.h"
#include "stm32f4xx.h"

/*** Preprocessor Definitions ***/
#define I2C_ERROR_FLAGS     (I2C_SR1_BERR | I2C_SR1_ARLO | I2C_SR1_AF)
#define I2C_INTER_DELAY     200U    /* Ciclos NOP entre transacciones */

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/
static void         prv_delay_cycles(uint32_t n);
static uint8_t      prv_wait_sr1(uint32_t mask);
static uint8_t      prv_has_error(void);
static void         prv_stop_internal(void);
static I2C_Status_t prv_check_bus(void);
static I2C_Status_t prv_start_internal(void);
static I2C_Status_t prv_send_addr(uint8_t addr7, uint8_t read);
static I2C_Status_t prv_write_byte_internal(uint8_t byte);
static I2C_Status_t prv_wait_btf(void);

/*** Function Definitions ***/

/*
 * Genera un retardo simple con instrucciones NOP.
 * No usa timers para no interferir con el resto del sistema.
 */
static void prv_delay_cycles(uint32_t n)
{
    volatile uint32_t i = n;
    while (i--) { __asm("nop"); }
}

/*
 * Espera hasta que el flag indicado en SR1 se active.
 * Retorna 0 si se acaba el tiempo o hay error en el bus.
 */
static uint8_t prv_wait_sr1(uint32_t mask)
{
    uint32_t t = I2C_TIMEOUT;
    while (!(I2C1->SR1 & mask))
    {
        if (--t == 0U)                     return 0U;
        if (I2C1->SR1 & I2C_ERROR_FLAGS)  return 0U;
    }
    return 1U;
}

/*
 * Revisa si hay error en el bus y limpia los flags.
 * Retorna 1 si habia error, 0 si estaba bien.
 */
static uint8_t prv_has_error(void)
{
    if (I2C1->SR1 & I2C_ERROR_FLAGS)
    {
        I2C1->SR1 &= ~I2C_ERROR_FLAGS;
        return 1U;
    }
    return 0U;
}

/*
 * Genera la condicion de STOP en el bus.
 * Solo llama recover si el bus no se libera despues de muchos intentos,
 * para no ser agresivo y no interrumpir transacciones validas.
 */
static void prv_stop_internal(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    uint32_t t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t) { }
    /* Solo recuperar si el bus quedo completamente trabado */
    if (t == 0U) { i2c_recover(); }
    /* Delay entre transacciones para que el PCF8574 procese cada byte */
    prv_delay_cycles(I2C_INTER_DELAY);
}

/*
 * Verifica que el bus este libre antes de iniciar una transmision.
 */
static I2C_Status_t prv_check_bus(void)
{
    if (!(I2C1->SR2 & I2C_SR2_BUSY)) return I2C_OK;

    uint32_t t = I2C_TIMEOUT;
    while ((I2C1->SR2 & I2C_SR2_BUSY) && --t) { }

    if (t == 0U)
    {
        i2c_recover();
        if (I2C1->SR2 & I2C_SR2_BUSY) return I2C_BUSY;
    }
    return I2C_OK;
}

/*
 * Genera la condicion de START en el bus.
 */
static I2C_Status_t prv_start_internal(void)
{
    I2C1->CR1 |= I2C_CR1_START;
    if (!prv_wait_sr1(I2C_SR1_SB)) return I2C_ERR;
    return I2C_OK;
}

/*
 * Envia la direccion del dispositivo con bit de lectura o escritura.
 */
static I2C_Status_t prv_send_addr(uint8_t addr7, uint8_t read)
{
    I2C1->DR = (uint8_t)((addr7 << 1U) | (read & 1U));
    if (!prv_wait_sr1(I2C_SR1_ADDR)) return I2C_ERR;
    if (prv_has_error())              return I2C_ERR;
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return I2C_OK;
}

/*
 * Escribe un byte en el bus esperando que DR este listo.
 */
static I2C_Status_t prv_write_byte_internal(uint8_t byte)
{
    if (!prv_wait_sr1(I2C_SR1_TXE)) return I2C_ERR;
    I2C1->DR = byte;
    return I2C_OK;
}

/*
 * Espera a que se complete la transferencia del ultimo byte.
 */
static I2C_Status_t prv_wait_btf(void)
{
    if (!prv_wait_sr1(I2C_SR1_BTF)) return I2C_ERR;
    return I2C_OK;
}

/* ------------------------------------------------------------------ */

/*
 * Configura el bus I2C1 a 100 kHz con pines PB8 (SCL) y PB9 (SDA).
 */
void i2c_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ALT_FN;
    cfg.otype = GPIO_OTYPE_OPEN_DRAIN;
    cfg.speed = GPIO_SPEED_HIGH;
    cfg.pull  = GPIO_PULL_NONE;

    gpio_initPort(I2C_SCL_PORT);
    gpio_setPinMode(I2C_SCL_PORT, I2C_SCL_PIN, &cfg);
    gpio_setAlternateFunction(I2C_SCL_PORT, I2C_SCL_PIN, I2C_AF);
    gpio_setPinMode(I2C_SDA_PORT, I2C_SDA_PIN, &cfg);
    gpio_setAlternateFunction(I2C_SDA_PORT, I2C_SDA_PIN, I2C_AF);

    I2C1->CR1 |=  I2C_CR1_SWRST;
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    I2C1->CR2   = I2C_CR2_FREQ_MHZ & I2C_CR2_FREQ;
    I2C1->CCR   = I2C_CCR_SM_100K  & I2C_CCR_CCR;
    I2C1->TRISE = I2C_TRISE_SM;
    I2C1->CR1  |= I2C_CR1_PE;
}

/*
 * Recupera el bus I2C cuando queda colgado generando pulsos manuales
 * de reloj en PB8/PB9 hasta liberar el dispositivo atascado.
 */
void i2c_recover(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    GPIOB->MODER   = (GPIOB->MODER   & ~(3UL << 16U)) | (1UL << 16U);
    GPIOB->OTYPER  |=  (1UL << 8U);
    GPIOB->OSPEEDR |=  (3UL << 16U);
    GPIOB->MODER   = (GPIOB->MODER   & ~(3UL << 18U)) | (1UL << 18U);
    GPIOB->OTYPER  |=  (1UL << 9U);
    GPIOB->OSPEEDR |=  (3UL << 18U);

    GPIOB->BSRR = (1UL << 8U) | (1UL << 9U);
    prv_delay_cycles(500U);

    for (uint32_t i = 0U; i < 9U; i++)
    {
        if (GPIOB->IDR & (1UL << 9U)) break;
        GPIOB->BSRR = (1UL << (8U + 16U));
        prv_delay_cycles(500U);
        GPIOB->BSRR = (1UL << 8U);
        prv_delay_cycles(500U);
    }

    GPIOB->BSRR = (1UL << (9U + 16U));
    prv_delay_cycles(500U);
    GPIOB->BSRR = (1UL << 9U);
    prv_delay_cycles(500U);

    I2C1->CR1 |=  I2C_CR1_SWRST;
    prv_delay_cycles(100U);
    I2C1->CR1 &= ~I2C_CR1_SWRST;

    i2c_init();
}

/* ------------------------------------------------------------------ */
/* API de alto nivel                                                    */
/* ------------------------------------------------------------------ */

I2C_Status_t i2c_writeDevice(uint8_t dev_addr,
                              const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U)       return I2C_ERR;
    if (prv_check_bus() != I2C_OK)       return I2C_BUSY;
    if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (prv_write_byte_internal(data[i]) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    }

    if (prv_wait_btf() != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    prv_stop_internal();
    return I2C_OK;
}

I2C_Status_t i2c_writeRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                 const uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U)       return I2C_ERR;
    if (prv_check_bus() != I2C_OK)       return I2C_BUSY;
    if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_write_byte_internal(reg_addr) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (prv_write_byte_internal(data[i]) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    }

    if (prv_wait_btf() != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    prv_stop_internal();
    return I2C_OK;
}

I2C_Status_t i2c_readRegDevice(uint8_t dev_addr, uint8_t reg_addr,
                                uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U)       return I2C_ERR;
    if (prv_check_bus() != I2C_OK)       return I2C_BUSY;
    if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 0U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_write_byte_internal(reg_addr) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_wait_btf()              != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }

    if (len == 1U)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (prv_send_addr(dev_addr, 1U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
        I2C1->CR1 |= I2C_CR1_STOP;
        if (!prv_wait_sr1(I2C_SR1_RXNE)) return I2C_ERR;
        data[0] = (uint8_t)I2C1->DR;
        I2C1->CR1 |= I2C_CR1_ACK;
        return I2C_OK;
    }

    I2C1->CR1 |= I2C_CR1_ACK;
    if (prv_send_addr(dev_addr, 1U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (i == (len - 2U))
        {
            if (!prv_wait_sr1(I2C_SR1_BTF)) { prv_stop_internal(); return I2C_ERR; }
            I2C1->CR1 &= ~I2C_CR1_ACK;
            data[i] = (uint8_t)I2C1->DR;
        }
        else if (i == (len - 1U))
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            data[i] = (uint8_t)I2C1->DR;
            I2C1->CR1 |= I2C_CR1_ACK;
        }
        else
        {
            if (!prv_wait_sr1(I2C_SR1_RXNE)) { prv_stop_internal(); return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;
        }
    }
    return I2C_OK;
}

I2C_Status_t i2c_readDevice(uint8_t dev_addr,
                             uint8_t *data, uint32_t len)
{
    if (data == NULL || len == 0U)       return I2C_ERR;
    if (prv_check_bus() != I2C_OK)       return I2C_BUSY;

    if (len == 1U)
    {
        I2C1->CR1 &= ~I2C_CR1_ACK;
        if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
        if (prv_send_addr(dev_addr, 1U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
        I2C1->CR1 |= I2C_CR1_STOP;
        if (!prv_wait_sr1(I2C_SR1_RXNE)) return I2C_ERR;
        data[0] = (uint8_t)I2C1->DR;
        I2C1->CR1 |= I2C_CR1_ACK;
        return I2C_OK;
    }

    I2C1->CR1 |= I2C_CR1_ACK;
    if (prv_start_internal()        != I2C_OK) { prv_stop_internal(); return I2C_ERR; }
    if (prv_send_addr(dev_addr, 1U) != I2C_OK) { prv_stop_internal(); return I2C_ERR; }

    for (uint32_t i = 0U; i < len; i++)
    {
        if (i == (len - 2U))
        {
            if (!prv_wait_sr1(I2C_SR1_BTF)) { prv_stop_internal(); return I2C_ERR; }
            I2C1->CR1 &= ~I2C_CR1_ACK;
            data[i] = (uint8_t)I2C1->DR;
        }
        else if (i == (len - 1U))
        {
            I2C1->CR1 |= I2C_CR1_STOP;
            data[i] = (uint8_t)I2C1->DR;
            I2C1->CR1 |= I2C_CR1_ACK;
        }
        else
        {
            if (!prv_wait_sr1(I2C_SR1_RXNE)) { prv_stop_internal(); return I2C_ERR; }
            data[i] = (uint8_t)I2C1->DR;
        }
    }
    return I2C_OK;
}

/* ------------------------------------------------------------------ */
/* Low-level API (used internally by lcd.c)                            */
/* ------------------------------------------------------------------ */

I2C_Status_t i2c_start(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (I2C1->SR2 & I2C_SR2_BUSY)
    {
        if (--timeout == 0U)
        {
            i2c_recover();
            timeout = I2C_TIMEOUT;
            while (I2C1->SR2 & I2C_SR2_BUSY)
            {
                if (--timeout == 0U) return I2C_BUSY;
            }
            break;
        }
    }

    I2C1->CR1 |= I2C_CR1_START;
    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_SB))
    {
        if (--timeout == 0U) { I2C1->CR1 |= I2C_CR1_STOP; return I2C_TIMEOUT_ERROR; }
    }
    return I2C_OK;
}

I2C_Status_t i2c_stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    uint32_t timeout = I2C_TIMEOUT;
    while (I2C1->SR2 & I2C_SR2_BUSY)
    {
        if (--timeout == 0U) { i2c_recover(); break; }
    }
    /* Delay entre transacciones para que el PCF8574 procese cada byte */
    prv_delay_cycles(I2C_INTER_DELAY);
    return I2C_OK;
}

I2C_Status_t i2c_sendAddress(uint8_t address, uint8_t read)
{
    uint32_t timeout = I2C_TIMEOUT;
    uint8_t  addr    = (uint8_t)(address << 1U);
    if (read != 0U) addr |= 1U;

    (void)I2C1->SR1;
    I2C1->DR = addr;

    while (!(I2C1->SR1 & I2C_SR1_ADDR))
    {
        if (--timeout == 0U) { I2C1->CR1 |= I2C_CR1_STOP; return I2C_TIMEOUT_ERROR; }
    }
    (void)I2C1->SR1;
    (void)I2C1->SR2;
    return I2C_OK;
}

I2C_Status_t i2c_writeByte(uint8_t data)
{
    uint32_t timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_TXE))
    {
        if (--timeout == 0U) return I2C_TIMEOUT_ERROR;
    }
    I2C1->DR = data;

    timeout = I2C_TIMEOUT;
    while (!(I2C1->SR1 & I2C_SR1_BTF))
    {
        if (--timeout == 0U) return I2C_TIMEOUT_ERROR;
    }
    return I2C_OK;
}