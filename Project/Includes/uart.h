/**
 ******************************************************************************
 * @file    uart.h
 * @brief   UART Driver - Public API
 *          USART2 @ 115200 bps, PA2 TX / PA3 RX
 *          Adapted for STM32F411RE at 16 MHz HSI
 *
 * Hardware mapping (NUCLEO-F411RE):
 *   PA2 -> USART2_TX  (AF7)  -> ST-LINK virtual COM port
 *   PA3 -> USART2_RX  (AF7)  -> ST-LINK virtual COM port
 *
 * BRR for 115200 bps @ 16 MHz APB1:
 *   16 000 000 / 115 200 approx 138.88  -> BRR = 0x008BU
 *   Equivalent to 139 decimal giving ~115 108 bps (< 0.1% error)
 *
 * API:
 *   uart_init()              - Configure USART2 + GPIO
 *   uart_sendChar(c)         - Transmit one character
 *   uart_sendString(str)     - Transmit a null-terminated string
 *   uart_receiveChar()       - Receive one character (blocking)
 ******************************************************************************
 */

#ifndef UART_H
#define UART_H

#include "stm32f4xx.h"
#include "gpio_driver.h"
#include <stdint.h>

/* ── Registers and bit definitions ─────────────────────────────────────── */

/** USART2 clock enable bit in RCC->APB1ENR */
#define USART2EN        (1U << 17U)

/** BRR = FCLK / BAUD = 16 000 000 / 115 200 approx 139 */
#define UART_BRR_115200  139U

/** CR1 bits */
#define CR1_UE   (1U << 13U)   /**< USART Enable          */
#define CR1_TE   (1U << 3U)    /**< Transmitter Enable    */
#define CR1_RE   (1U << 2U)    /**< Receiver Enable       */

/** CR2 bits */
#define CR2_STOP (3U << 12U)   /**< Stop bits mask [13:12] */

/** SR bits */
#define SR_TXE   (1U << 7U)    /**< Transmit Data Register Empty */
#define SR_RXNE  (1U << 5U)    /**< Read Data Register Not Empty */

/** Alternate Function 7 -> USART2 on PA2/PA3 (RM0383 Table 9) */
#define AF7_USART2   7U

/* ── Public API ─────────────────────────────────────────────────────────── */

void uart_init(void);
void uart_sendChar(char data);
void uart_sendString(const char *str);
char uart_receiveChar(void);

#endif /* UART_H */
