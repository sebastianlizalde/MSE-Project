/**
 ******************************************************************************
 * @file    servo.c
 * @brief   Servo Motor Control Module - Implementation
 *          TIM3_CH1 (PC6) → Servo A
 *          TIM3_CH4 (PC9) → Servo B
 *
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date May 2026
 ******************************************************************************
 */
 
#include "servo.h"
 
/* =========================================================================
 * Private Variables
 * ========================================================================= */
 
static uint8_t s_initialized = 0U;
 
/* =========================================================================
 * Private Helpers
 * ========================================================================= */
 
/**
 * @brief Converts angle (0-180 deg) to CCR value in microseconds.
 *
 * Linear mapping between SERVO_CCR_0DEG and SERVO_CCR_180DEG:
 *   CCR = SERVO_CCR_0DEG + (angle * (SERVO_CCR_180DEG - SERVO_CCR_0DEG)) / 180
 *
 * Example:
 *   0   deg -> 1000 + (0   * 1000) / 180 = 1000
 *   90  deg -> 1000 + (90  * 1000) / 180 = 1500
 *   180 deg -> 1000 + (180 * 1000) / 180 = 2000
 */
static uint32_t prv_angleToCCR(uint8_t angle)
{
    if (angle > 180U) { angle = 180U; }
    return SERVO_CCR_0DEG +
           ((uint32_t)angle * (SERVO_CCR_180DEG - SERVO_CCR_0DEG)) / 180U;
}
 
static TIM_Channel_t prv_getChannel(SoilMoisture_Section_t section)
{
    if (section == SOIL_SECTION_A) { return SERVO_CHANNEL_A; }
    return SERVO_CHANNEL_B;
}
 
/* =========================================================================
 * FR-1 - servo_init
 * ========================================================================= */
Servo_Status_t servo_init(void)
{
    GPIO_PinCfg_t cfg;
 
    /*------------------------------------------------------------------
     * Step 1: Configure PC6 and PC9 as Alternate Function 2 (TIM3).
     *------------------------------------------------------------------*/
    gpio_initPort(SERVO_PORT);
 
    cfg.mode  = GPIO_MODE_ALT_FN;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_HIGH;
    cfg.pull  = GPIO_PULL_NONE;
 
    if (gpio_setPinMode(SERVO_PORT, SERVO_PIN_A, &cfg) != GPIO_OK)
    {
        return SERVO_ERR_INIT;
    }
    gpio_setAlternateFunction(SERVO_PORT, SERVO_PIN_A, SERVO_AF);
 
    if (gpio_setPinMode(SERVO_PORT, SERVO_PIN_B, &cfg) != GPIO_OK)
    {
        return SERVO_ERR_INIT;
    }
    gpio_setAlternateFunction(SERVO_PORT, SERVO_PIN_B, SERVO_AF);
 
    /*------------------------------------------------------------------
     * Step 2: Configure TIM3 at 50 Hz.
     *
     * PSC = 15  -> 16 MHz / 16 = 1 MHz (1 tick = 1 us)
     * ARR = 19999 -> period = 20 000 us = 20 ms = 50 Hz
     *------------------------------------------------------------------*/
    tim_initTimer(SERVO_TIM);
 
    TIM3->CR1  &= ~TIM_CR1_CEN;
    TIM3->PSC   = SERVO_PSC;
    TIM3->ARR   = SERVO_ARR;
    TIM3->CNT   = 0U;
    TIM3->EGR  |= TIM_EGR_UG;
    TIM3->SR   &= ~TIM_SR_UIF;
 
    /*------------------------------------------------------------------
     * Step 3: Configure CH1 and CH4 in PWM Mode 1.
     *------------------------------------------------------------------*/
    if (tim_setTimerCompareMode(SERVO_TIM, SERVO_CHANNEL_A,
                                TIM_COMPARE_MODE_PWM1) != TIM_OK)
    {
        return SERVO_ERR_INIT;
    }
 
    if (tim_setTimerCompareMode(SERVO_TIM, SERVO_CHANNEL_B,
                                TIM_COMPARE_MODE_PWM1) != TIM_OK)
    {
        return SERVO_ERR_INIT;
    }
 
    /*------------------------------------------------------------------
     * Step 4: Start both servos at closed position (0 deg).
     *------------------------------------------------------------------*/
    tim_setTimerCompareChannelValue(SERVO_TIM, SERVO_CHANNEL_A,
                                    SERVO_CCR_0DEG);
    tim_setTimerCompareChannelValue(SERVO_TIM, SERVO_CHANNEL_B,
                                    SERVO_CCR_0DEG);
 
    /*------------------------------------------------------------------
     * Step 5: Enable channels and start TIM3.
     *------------------------------------------------------------------*/
    tim_enableTimerCompareChannel(SERVO_TIM, SERVO_CHANNEL_A);
    tim_enableTimerCompareChannel(SERVO_TIM, SERVO_CHANNEL_B);
    tim_enableTimer(SERVO_TIM);
 
    s_initialized = 1U;
    return SERVO_OK;
}
 
/* =========================================================================
 * FR-2 - servo_setAngle
 * ========================================================================= */
Servo_Status_t servo_setAngle(SoilMoisture_Section_t section, uint8_t angle)
{
    if (section >= SOIL_SECTION_MAX) { return SERVO_ERR_INVALID; }
    if (s_initialized == 0U)        { return SERVO_ERR_INIT;    }
 
    uint32_t     ccr     = prv_angleToCCR(angle);
    TIM_Channel_t channel = prv_getChannel(section);
 
    tim_setTimerCompareChannelValue(SERVO_TIM, channel, ccr);
    return SERVO_OK;
}
 
/* =========================================================================
 * FR-3 - servo_open
 * ========================================================================= */
Servo_Status_t servo_open(SoilMoisture_Section_t section)
{
    return servo_setAngle(section, SERVO_ANGLE_OPEN);
}
 
/* =========================================================================
 * FR-4 - servo_close
 * ========================================================================= */
Servo_Status_t servo_close(SoilMoisture_Section_t section)
{
    return servo_setAngle(section, SERVO_ANGLE_CLOSED);
}