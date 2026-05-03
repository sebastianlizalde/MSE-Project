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
* @file pwm.c
* @brief Brief description of what this module does.
*
* A complete description of what this module does.
*
* @author Your Name
* @date 24 de abril
*
*/

/*** Includes ***/
#include "pwm.h"
#include "TIM_Driver.h"
#include "GPIO_stm32f4.h"// 
// CCR = 500          → 500/1000 = 50% duty cycle

/*** Preprocessor Definitions ***/
#define PWM_PORT      GPIO_PORT_B
#define PWM_PIN       6U           // PB6 = TIM4_CH1
#define PWM_TIMER     TIM_TIMER_4  
#define PWM_CHANNEL   TIM_CHANNEL_1
#define PWM_AF       GPIO_AF2 // AF2 = TIM4 on PB6

void pwm_init(void)
{
    // Initialize clock for port B
    gpio_initPort(PWM_PORT);
    // Set PB6 as alternate function mode
    gpio_setPinMode(PWM_PORT, PWM_PIN, GPIO_MODE_ALTFN);

    // Configure PB6 as AF2 (TIM4_CH1)
    gpio_setAlternateFunction(PWM_PORT, PWM_PIN, PWM_AF);

    // Initialize timer for PWM
    tim_initTimer(PWM_TIMER);
}

void pwm_setSignal(uint32_t frequency, uint8_t duty_cycle)
{
    // Configure PSC and ARR for the desired frequency
    tim_setTimerFreq(PWM_TIMER, frequency);

    // Calculate CCR for the desired duty cycle
    uint32_t counts = TIM_SYSTEM_CLOCK_HZ / frequency;
    uint32_t ccr    = (counts * duty_cycle) / 100U;

    tim_setTimerCompareMode(PWM_TIMER, PWM_CHANNEL, TIM_OC_MODE_PWM1);
    tim_setTimerCompareChannelValue(PWM_TIMER, PWM_CHANNEL, ccr);
}

void pwm_start(void)
{
    // Enable PWM channel and start timer
    tim_enableTimerCompareChannel(PWM_TIMER, PWM_CHANNEL);
    tim_enableTimer(PWM_TIMER);
}

void pwm_stop(void)
{
    // Stop timer and disable PWM channel
    tim_disableTimer(PWM_TIMER);
    tim_disableTimerCompareChannel(PWM_TIMER, PWM_CHANNEL);
}