/**
 ******************************************************************************
 * @file    uart.c
 * @brief   UART Driver Implementation for STM32F411RE.
 *          USART2 @ 115200 bps, PA2 TX / PA3 RX
 *          Uses gpio_driver instead of HAL/gpio.h
 *
 * Initialization sequence:
 *   1. Enable GPIOA clock (AHB1ENR bit 0).
 *   2. Enable USART2 clock (APB1ENR bit 17).
 *   3. Configure PA2 and PA3 as Alternate Function AF7 via gpio_driver.
 *   4. Set BRR for 115200 bps @ 16 MHz.
 *   5. Enable TE + RE + UE in CR1.
 *   6. Ensure 1 stop bit in CR2 (bits [13:12] = 00).
 *
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date May 2026
 ******************************************************************************
 */

/*** Includes ***/
#include "uart.h"

/*** Function Definitions ***/

/**
 * @brief Initializes USART2 at 115200 bps with PA2 (TX) and PA3 (RX).
 */
void uart_init(void)
{
    /* ── 1. Enable clocks ──────────────────────────────────────────────── */
    RCC->AHB1ENR |= (1U << 0U);   /* GPIOA clock (bit 0 of AHB1ENR)  */
    RCC->APB1ENR |= USART2EN;     /* USART2 clock (bit 17 of APB1ENR) */
    (void)RCC->APB1ENR;           /* Bus flush for stabilization */

    /* ── 2. Configure PA2 (TX) and PA3 (RX) as AF7 via gpio_driver ──────── */
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ALT_FN;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_HIGH;
    cfg.pull  = GPIO_PULL_NONE;

    gpio_setPinMode(GPIO_PORT_A, 2U, &cfg);                /* PA2 -> TX */
    gpio_setAlternateFunction(GPIO_PORT_A, 2U, AF7_USART2);

    gpio_setPinMode(GPIO_PORT_A, 3U, &cfg);                /* PA3 -> RX */
    gpio_setAlternateFunction(GPIO_PORT_A, 3U, AF7_USART2);

    /* ── 3. Baud rate: 115200 bps @ 16 MHz APB1 ──────────────────────────── */
    /*      BRR = 16 000 000 / 115 200 approx 139  -> error < 0.1 %               */
    USART2->BRR = UART_BRR_115200;

    /* ── 4. Control register 1: 8-bit, no parity, TX + RX + USART ON ───── */
    USART2->CR1  = 0U;
    USART2->CR1 |= CR1_TE;   /* Enable transmitter  */
    USART2->CR1 |= CR1_RE;   /* Enable receiver    */
    USART2->CR1 |= CR1_UE;   /* Enable USART        */

    /* ── 5. Control register 2: 1 stop bit (bits [13:12] = 00) ───────────── */
    USART2->CR2 &= ~CR2_STOP;
}

/**
 * @brief Transmits one character via USART2 (blocking until DR is empty).
 * @param data Character to send.
 */
void uart_sendChar(char data)
{
    while (!(USART2->SR & SR_TXE)) { }          /* Wait for TXE (TX Data Register Empty) */
    USART2->DR = (uint32_t)((uint8_t)data);
}

/**
 * @brief Transmits a null-terminated string.
 * @param str Pointer to the string to send.
 */
void uart_sendString(const char *str)
{
    while (*str != '\0')
    {
        uart_sendChar(*str++);
    }
}

/**
 * @brief Receives one character via USART2 (blocking until a byte arrives).
 * @return Received character.
 */
char uart_receiveChar(void)
{
    while (!(USART2->SR & SR_RXNE)) { }         /* Wait for RXNE (RX Not Empty) */
    return (char)((uint8_t)(USART2->DR & 0xFFU));
}
