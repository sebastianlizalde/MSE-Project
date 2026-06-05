/**
 * @file    systick.h
 * @brief   SysTick driver — 1 ms tick counter for non-blocking delays.
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date Mayo 2026
 */

#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>

/**
 * @brief Configures SysTick to fire every 1 ms at 16 MHz HSI.
 *        Must be called once before any call to systick_getTick().
 */
void systick_init(void);

/**
 * @brief Returns the number of milliseconds elapsed since systick_init().
 * @return Elapsed time in milliseconds (wraps after ~49 days).
 */
uint32_t systick_getTick(void);

#endif /* SYSTICK_H */