#include "systick.h"
#include "stm32f4xx.h"

static volatile uint32_t s_tick_ms = 0U;

void systick_init(void)
{
    /* Actualiza SystemCoreClock con la frecuencia real del MCU */
    SystemCoreClockUpdate();

    /* Configura SysTick a 1 ms usando el clock real */
    SysTick_Config(SystemCoreClock / 1000U);
}

uint32_t systick_getTick(void)
{
    return s_tick_ms;
}

void SysTick_Handler(void)
{
    s_tick_ms++;
}