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
* @file TIM_Driver.h
* @brief Defines the functions, types, and peripherals for the TIM Driver.
*
*
* @author Sebastian Lizalde Herrera
* @date 23/04/2026
*
*/

#ifndef __TIM_DRIVER_H__
#define __TIM_DRIVER_H__
 
/*** Includes ***/
#include <stdint.h>
#include "GPIO_stm32f4.h"
/*** Preprocessor Definitions ***/
 
//#define PERIPH_BASE             (0x40000000UL)
#define APB1PERIPH_BASE         (PERIPH_BASE + 0x00000000UL)
#define TIM2_BASE               (APB1PERIPH_BASE + 0x0000UL)  
#define TIM3_BASE               (APB1PERIPH_BASE + 0x0400UL)  
#define TIM4_BASE               (APB1PERIPH_BASE + 0x0800UL)  
#define TIM5_BASE               (APB1PERIPH_BASE + 0x0C00UL)  
#define APB2PERIPH_BASE         (PERIPH_BASE + 0x00010000UL)
#define TIM1_BASE               (APB2PERIPH_BASE + 0x0000UL)  
#define TIM9_BASE               (APB2PERIPH_BASE + 0x4000UL)  
#define TIM10_BASE              (APB2PERIPH_BASE + 0x4400UL)  
#define TIM11_BASE              (APB2PERIPH_BASE + 0x4800UL)  
//#define AHB1PERIPH_BASE         (PERIPH_BASE + 0x00020000UL)
//#define RCC_BASE                (AHB1PERIPH_BASE + 0x3800UL)
#define TIM1                    ((TIM_RegDef_t *) TIM1_BASE)
#define TIM2                    ((TIM_RegDef_t *) TIM2_BASE)
#define TIM3                    ((TIM_RegDef_t *) TIM3_BASE)
#define TIM4                    ((TIM_RegDef_t *) TIM4_BASE)
#define TIM5                    ((TIM_RegDef_t *) TIM5_BASE)
#define TIM9                    ((TIM_RegDef_t *) TIM9_BASE)
#define TIM10                   ((TIM_RegDef_t *) TIM10_BASE)
#define TIM11                   ((TIM_RegDef_t *) TIM11_BASE)
#define RCC_TIM                 ((RCC_TIM_RegDef_t *) RCC_BASE)
#define RCC_APB1ENR_TIM2EN      (1U << 0)
#define RCC_APB1ENR_TIM3EN      (1U << 1)
#define RCC_APB1ENR_TIM4EN      (1U << 2)
#define RCC_APB1ENR_TIM5EN      (1U << 3)
#define RCC_APB2ENR_TIM1EN      (1U << 0)
#define RCC_APB2ENR_TIM9EN      (1U << 16)
#define RCC_APB2ENR_TIM10EN     (1U << 17)
#define RCC_APB2ENR_TIM11EN     (1U << 18)
#define TIM_CR1_CEN             (1U << 0)   // Counter enable        
#define TIM_CR1_UDIS            (1U << 1)   // Update disable        
#define TIM_CR1_URS             (1U << 2)   // Update request source 
#define TIM_CR1_OPM             (1U << 3)   // One-pulse mode      
#define TIM_CR1_ARPE            (1U << 7)   
#define TIM_SR_UIF              (1U << 0)   // Update interrupt flag  
#define TIM_SR_CC1IF            (1U << 1)   
#define TIM_SR_CC2IF            (1U << 2)   
#define TIM_SR_CC3IF            (1U << 3)   
#define TIM_SR_CC4IF            (1U << 4)   
#define TIM_CCMR_OC1M_SHIFT     (4U)
#define TIM_CCMR_OC2M_SHIFT     (12U)
#define TIM_CCMR_OC3M_SHIFT     (4U)        // CCMR2 
#define TIM_CCMR_OC4M_SHIFT     (12U)       // CCMR2 
#define TIM_CCMR_OC1PE          (1U << 3)   // OC1 preload enable    
#define TIM_CCMR_OC2PE          (1U << 11)  // OC2 preload enable    
#define TIM_CCMR_OC3PE          (1U << 3)   // OC3 preload (CCMR2)   
#define TIM_CCMR_OC4PE          (1U << 11)  // OC4 preload (CCMR2)   
#define TIM_CCMR_CC1S_OUTPUT    (0U << 0)   // CC1 configured as output 
#define TIM_CCMR_CC2S_OUTPUT    (0U << 8)   // CC2 configured as output 
#define TIM_CCMR_CC3S_OUTPUT    (0U << 0)   // CC3 configured as output (CCMR2) 
#define TIM_CCMR_CC4S_OUTPUT    (0U << 8)   // CC4 configured as output (CCMR2) 
#define TIM_CCER_CC1E           (1U << 0)   // CC1 output enable  
#define TIM_CCER_CC2E           (1U << 4)   // CC2 output enable  
#define TIM_CCER_CC3E           (1U << 8)   // CC3 output enable  
#define TIM_CCER_CC4E           (1U << 12)  // CC4 output enable  
#define TIM_SYSTEM_CLOCK_HZ     (16000000UL)  // Default HSI: 16 MHz */
 
/*** Type Prototypes ***/
 
typedef struct
{
    volatile uint32_t CR1;    // 0x00 - Control register 1              
    volatile uint32_t CR2;    // 0x04 - Control register 2              
    volatile uint32_t SMCR;   // 0x08 - Slave mode control register     
    volatile uint32_t DIER;   // 0x0C - DMA/Interrupt enable register   
    volatile uint32_t SR;     // 0x10 - Status register                 
    volatile uint32_t EGR;    // 0x14 - Event generation register       
    volatile uint32_t CCMR1;  // 0x18 - Capture/compare mode register 1 
    volatile uint32_t CCMR2;  // 0x1C - Capture/compare mode register 2 
    volatile uint32_t CCER;   // 0x20 - Capture/compare enable register 
    volatile uint32_t CNT;    // 0x24 - Counter                         
    volatile uint32_t PSC;    // 0x28 - Prescaler                       
    volatile uint32_t ARR;    // 0x2C - Auto-reload register            
    volatile uint32_t RCR;    // 0x30 - Repetition counter (TIM1 only)  
    volatile uint32_t CCR1;   // 0x34 - Capture/compare register 1      
    volatile uint32_t CCR2;   // 0x38 - Capture/compare register 2      
    volatile uint32_t CCR3;   // 0x3C - Capture/compare register 3      
    volatile uint32_t CCR4;   // 0x40 - Capture/compare register 4      
    volatile uint32_t BDTR;   // 0x44 - Break and dead-time (TIM1 only) 
    volatile uint32_t DCR;    // 0x48 - DMA control register            
    volatile uint32_t DMAR;   // 0x4C - DMA address for full transfer   
} TIM_RegDef_t;
 
typedef struct
{
    volatile uint32_t CR;           // 0x00 
    volatile uint32_t PLLCFGR;     // 0x04 
    volatile uint32_t CFGR;        // 0x08 
    volatile uint32_t CIR;         // 0x0C 
    volatile uint32_t AHB1RSTR;    // 0x10 
    volatile uint32_t AHB2RSTR;    // 0x14 
    volatile uint32_t RESERVED0[2];// 0x18-0x1C 
    volatile uint32_t APB1RSTR;    // 0x20 
    volatile uint32_t APB2RSTR;    // 0x24 
    volatile uint32_t RESERVED1[2]; // 0x28-0x2C 
    volatile uint32_t AHB1ENR;     // 0x30 
    volatile uint32_t AHB2ENR;     // 0x34 
    volatile uint32_t RESERVED2[2];// 0x38-0x3C 
    volatile uint32_t APB1ENR;     // 0x40 - TIM2/3/4/5 clocks 
    volatile uint32_t APB2ENR;     // 0x44 - TIM1/9/10/11 clocks 
} RCC_TIM_RegDef_t;
 
typedef enum
{
    TIM_TIMER_1 = 0,
    TIM_TIMER_2,
    TIM_TIMER_3,
    TIM_TIMER_4,
    TIM_TIMER_5,
    TIM_TIMER_9,
    TIM_TIMER_10,
    TIM_TIMER_11,
    TIM_TIMER_MAX,   // Centinela
} TIM_Timer_t;
 
typedef enum
{
    TIM_CHANNEL_1 = 0,
    TIM_CHANNEL_2,
    TIM_CHANNEL_3,
    TIM_CHANNEL_4,
    TIM_CHANNEL_MAX,  // Centinela
} TIM_Channel_t;
 
typedef enum
{
    TIM_OC_MODE_FROZEN          = 0x0U,
    TIM_OC_MODE_ACTIVE_ON_MATCH = 0x1U,
    TIM_OC_MODE_INACTIVE_ON_MATCH = 0x2U,
    TIM_OC_MODE_TOGGLE          = 0x3U,
    TIM_OC_MODE_FORCE_INACTIVE  = 0x4U,
    TIM_OC_MODE_FORCE_ACTIVE    = 0x5U,
    TIM_OC_MODE_PWM1            = 0x6U,  // PWM mode 1 (active while CNT < CCR)
    TIM_OC_MODE_PWM2            = 0x7U,  // PWM mode 2 (inactive while CNT < CCR)
    TIM_OC_MODE_MAX,
} TIM_OC_Mode_t;

typedef enum
{
    TIM_OK           =  0,
    TIM_ERR_TIMER    = -1,  // Invalid timer identifier
    TIM_ERR_CHANNEL  = -2,  // Invalid channel identifier
    TIM_ERR_MODE     = -3,  // Invalid compare mode
    TIM_ERR_PARAM    = -4,  // Invalid parameter
} TIM_Status_t;
 
/*** Function Prototypes ***/
 
void tim_init(void); //FR-1
 
TIM_Status_t tim_initTimer(TIM_Timer_t timer); //FR-2
 
TIM_Status_t tim_setTimerMs(TIM_Timer_t timer, uint32_t ms); //FR-3
 
TIM_Status_t tim_setTimerFreq(TIM_Timer_t timer, uint32_t freq_hz); //FR-4
 
TIM_Status_t tim_enableTimer(TIM_Timer_t timer); //FR-5
 
TIM_Status_t tim_disableTimer(TIM_Timer_t timer); //FR-6
 
TIM_Status_t tim_waitTimer(TIM_Timer_t timer); //FR-7
 
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Timer_t timer, TIM_Channel_t channel, uint32_t value); //FR-8

TIM_Status_t tim_setTimerCompareMode(TIM_Timer_t timer, TIM_Channel_t channel, TIM_OC_Mode_t mode); //FR-9

TIM_Status_t tim_enableTimerCompareChannel(TIM_Timer_t timer, TIM_Channel_t channel); //FR-10
 
TIM_Status_t tim_disableTimerCompareChannel(TIM_Timer_t timer, TIM_Channel_t channel); //FR-11
 
#endif /* __TIM_DRIVER_H__ */