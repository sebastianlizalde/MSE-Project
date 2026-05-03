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
* @file pwm.h
* @brief Brief description of what this module does.
*
* A complete description of what this module does.
*
* @author Michelle Urbina
* @date 24 de abril
*
*/

#ifndef PWM_H
#define PWM_H

/*** Includes ***/
#include <stdint.h>

/*** Preprocessor Definitions ***/
// Initializes TIM4_CH1 for PWM output on PB6
void pwm_init(void);

/*** Type Prototypes ***/
// Configures frequency (Hz) and duty cycle (0-100%)
void pwm_setSignal(uint32_t frequency, uint8_t duty_cycle);

/*** Global Variables ***/
// Starts the PWM signal
void pwm_start(void);

/*** Function Prototypes ***/
// Stops the PWM signal
void pwm_stop(void);


#endif /* PWM_H */
    