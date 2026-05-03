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
* @file TIM_Driver.c
* @brief TIM Driver implementation for STM32F411RE
*
*
* @author Sebastian Lizalde Herrera
* @date 23/04/2026
*
*/

/*** Includes ***/
#include <stdint.h>
#include "TIM_Driver.h"
 
/*** Local Variables ***/
 
static TIM_RegDef_t * const tim_peripherals[TIM_TIMER_MAX] = {
    TIM1,   // TIM_TIMER_1  
    TIM2,   // TIM_TIMER_2  
    TIM3,   // TIM_TIMER_3  
    TIM4,   // TIM_TIMER_4  
    TIM5,   // TIM_TIMER_5  
    TIM9,   // TIM_TIMER_9  
    TIM10,  // TIM_TIMER_10 
    TIM11,  // TIM_TIMER_11 
};
 
static const uint8_t tim_max_channels[TIM_TIMER_MAX] = {
    4U,  // TIM_TIMER_1  
    4U,  // TIM_TIMER_2  
    4U,  // TIM_TIMER_3  
    4U,  // TIM_TIMER_4  
    4U,  // TIM_TIMER_5  
    2U,  // TIM_TIMER_9  
    1U,  // TIM_TIMER_10 
    1U,  // TIM_TIMER_11 
};
 
static void tim_enableClock(TIM_Timer_t timer)
{
    switch (timer)
    {
        case TIM_TIMER_1:
            RCC_TIM->APB2ENR |= RCC_APB2ENR_TIM1EN;
            break;
        case TIM_TIMER_2:
            RCC_TIM->APB1ENR |= RCC_APB1ENR_TIM2EN;
            break;
        case TIM_TIMER_3:
            RCC_TIM->APB1ENR |= RCC_APB1ENR_TIM3EN;
            break;
        case TIM_TIMER_4:
            RCC_TIM->APB1ENR |= RCC_APB1ENR_TIM4EN;
            break;
        case TIM_TIMER_5:
            RCC_TIM->APB1ENR |= RCC_APB1ENR_TIM5EN;
            break;
        case TIM_TIMER_9:
            RCC_TIM->APB2ENR |= RCC_APB2ENR_TIM9EN;
            break;
        case TIM_TIMER_10:
            RCC_TIM->APB2ENR |= RCC_APB2ENR_TIM10EN;
            break;
        case TIM_TIMER_11:
            RCC_TIM->APB2ENR |= RCC_APB2ENR_TIM11EN;
            break;
        default:
            break;
    }
}
 
static void tim_writeCCR(TIM_RegDef_t *pTIM, TIM_Channel_t channel, uint32_t value)
{
    switch (channel)
    {
        case TIM_CHANNEL_1: 
            pTIM->CCR1 = value; 
            break;
        case TIM_CHANNEL_2: 
            pTIM->CCR2 = value; 
            break;
        case TIM_CHANNEL_3: 
            pTIM->CCR3 = value; 
            break;
        case TIM_CHANNEL_4: 
            pTIM->CCR4 = value; 
            break;
        default:            
            break;
    }
}
 
static void tim_writeOCMode(TIM_RegDef_t *pTIM, TIM_Channel_t channel, TIM_OC_Mode_t mode)
{
    uint32_t reg;
 
    switch (channel)
    {
        case TIM_CHANNEL_1:
            reg  = pTIM->CCMR1;
            reg &= ~(0x73U << 0U);              // Clear CC1S[1:0] + OC1M[2:0]
            reg &= ~(0x7U  << TIM_CCMR_OC1M_SHIFT);
            reg |=  ((uint32_t)mode << TIM_CCMR_OC1M_SHIFT);
            reg |=  TIM_CCMR_OC1PE;             // Enable preload on CCR1
            pTIM->CCMR1 = reg;
            break;
 
        case TIM_CHANNEL_2:
            reg  = pTIM->CCMR1;
            reg &= ~(0x73U << 8U);              // Clear CC2S[1:0] + OC2M[2:0]
            reg &= ~(0x7U  << TIM_CCMR_OC2M_SHIFT);
            reg |=  ((uint32_t)mode << TIM_CCMR_OC2M_SHIFT);
            reg |=  TIM_CCMR_OC2PE;             // Enable preload on CCR2
            pTIM->CCMR1 = reg;
            break;
 
        case TIM_CHANNEL_3:
            reg  = pTIM->CCMR2;
            reg &= ~(0x73U << 0U);              // Clear CC3S[1:0] + OC3M[2:0]
            reg &= ~(0x7U  << TIM_CCMR_OC3M_SHIFT);
            reg |=  ((uint32_t)mode << TIM_CCMR_OC3M_SHIFT);
            reg |=  TIM_CCMR_OC3PE;             // Enable preload on CCR3
            pTIM->CCMR2 = reg;
            break;
 
        case TIM_CHANNEL_4:
            reg  = pTIM->CCMR2;
            reg &= ~(0x73U << 8U);              // Clear CC4S[1:0] + OC4M[2:0]
            reg &= ~(0x7U  << TIM_CCMR_OC4M_SHIFT);
            reg |=  ((uint32_t)mode << TIM_CCMR_OC4M_SHIFT);
            reg |=  TIM_CCMR_OC4PE;             // Enable preload on CCR4
            pTIM->CCMR2 = reg;
            break;
 
        default:
            break;
    }
}
 
/*** Function Definitions ***/
 
void tim_init(void)
{
    TIM_Timer_t timer;
 
    for (timer = TIM_TIMER_1; timer < TIM_TIMER_MAX; timer++)
    {
        tim_enableClock(timer);
        tim_peripherals[timer]->CR1  = 0x00000000U;
        tim_peripherals[timer]->PSC  = 0x00000000U;
        tim_peripherals[timer]->ARR  = 0x0000FFFFU;
        tim_peripherals[timer]->CNT  = 0x00000000U;
        tim_peripherals[timer]->CCER = 0x00000000U;
        tim_peripherals[timer]->SR   = 0x00000000U;
    }
}
 
TIM_Status_t tim_initTimer(TIM_Timer_t timer)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    tim_enableClock(timer);
 
    TIM_RegDef_t *pTIM = tim_peripherals[timer];
 
    pTIM->CR1  = 0x00000000U;
    pTIM->PSC  = 0x00000000U;
    pTIM->ARR  = 0x0000FFFFU;
    pTIM->CNT  = 0x00000000U;
    pTIM->CCER = 0x00000000U;
    pTIM->SR   = 0x00000000U;
 
    return TIM_OK;
}
 
TIM_Status_t tim_setTimerMs(TIM_Timer_t timer, uint32_t ms)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if (ms == 0U)
    {
        return TIM_ERR_PARAM;
    }
 
    uint32_t counts = (TIM_SYSTEM_CLOCK_HZ / 1000U) * ms;
    uint32_t psc    = counts / 65536U;
    uint32_t arr    = (counts / (psc + 1U)) - 1U;
 
    TIM_RegDef_t *pTIM = tim_peripherals[timer];
 
    pTIM->PSC = psc;
    pTIM->ARR = arr;
    pTIM->CNT = 0U;
    pTIM->SR &= ~TIM_SR_UIF;   /* Clear any pending update flag */
 
    return TIM_OK;
}
 
TIM_Status_t tim_setTimerFreq(TIM_Timer_t timer, uint32_t freq_hz)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if ((freq_hz == 0U) || (freq_hz > TIM_SYSTEM_CLOCK_HZ))
    {
        return TIM_ERR_PARAM;
    }
 
    uint32_t counts = TIM_SYSTEM_CLOCK_HZ / freq_hz;
    uint32_t psc    = counts / 65536U;
    uint32_t arr    = (counts / (psc + 1U)) - 1U;
 
    TIM_RegDef_t *pTIM = tim_peripherals[timer];
 
    pTIM->PSC = psc;
    pTIM->ARR = arr;
    pTIM->CNT = 0U;
    pTIM->SR &= ~TIM_SR_UIF;
 
    return TIM_OK;
}
 
TIM_Status_t tim_enableTimer(TIM_Timer_t timer)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    TIM_RegDef_t *pTIM = tim_peripherals[timer];
 
    pTIM->CR1 |= (TIM_CR1_ARPE | TIM_CR1_CEN);
 
    return TIM_OK;
}
 
TIM_Status_t tim_disableTimer(TIM_Timer_t timer)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    tim_peripherals[timer]->CR1 &= ~TIM_CR1_CEN;
 
    return TIM_OK;
}
 
TIM_Status_t tim_waitTimer(TIM_Timer_t timer)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    TIM_RegDef_t *pTIM = tim_peripherals[timer];
 
    // Iterate until the update interrupt flag is set
    while ((pTIM->SR & TIM_SR_UIF) == 0U)
    {
        
    }
 
    pTIM->SR &= ~TIM_SR_UIF;
 
    return TIM_OK;
}
 
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Timer_t timer, TIM_Channel_t channel, uint32_t value)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if ((uint8_t)channel >= tim_max_channels[timer])
    {
        return TIM_ERR_CHANNEL;
    }
 
    tim_writeCCR(tim_peripherals[timer], channel, value);
 
    return TIM_OK;
}

TIM_Status_t tim_setTimerCompareMode(TIM_Timer_t timer, TIM_Channel_t channel, TIM_OC_Mode_t mode)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if ((uint8_t)channel >= tim_max_channels[timer])
    {
        return TIM_ERR_CHANNEL;
    }
 
    if (mode >= TIM_OC_MODE_MAX)
    {
        return TIM_ERR_MODE;
    }
 
    tim_writeOCMode(tim_peripherals[timer], channel, mode);
 
    return TIM_OK;
}

TIM_Status_t tim_enableTimerCompareChannel(TIM_Timer_t timer, TIM_Channel_t channel)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if ((uint8_t)channel >= tim_max_channels[timer])
    {
        return TIM_ERR_CHANNEL;
    }
 
    // The enable bit for each channel is located at position: channel * 4 in CCER 
    tim_peripherals[timer]->CCER |= (1U << ((uint8_t)channel * 4U));
 
    return TIM_OK;
}

TIM_Status_t tim_disableTimerCompareChannel(TIM_Timer_t timer, TIM_Channel_t channel)
{
    if (timer >= TIM_TIMER_MAX)
    {
        return TIM_ERR_TIMER;
    }
 
    if ((uint8_t)channel >= tim_max_channels[timer])
    {
        return TIM_ERR_CHANNEL;
    }
 
    tim_peripherals[timer]->CCER &= ~(1U << ((uint8_t)channel * 4U));
 
    return TIM_OK;
}