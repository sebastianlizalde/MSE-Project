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
* @file tim_driver.c
* @brief Implementacion del driver de timers para el STM32F411RE.
*
* Unifica las dos versiones del equipo. Mantiene la API con enums (TIM_Id_t)
* de soil_moisture/ultrasonico y agrega tim_isTimerExpired() de flujo_wifi
* para el modulo de botones.
*
* @author Luis Angel Lugo Muniz, Carlos Araiza, Kheara Kieley
* @date Mayo 2026
*
*/

/*** Includes ***/
#include "tim_driver.h"

/*** Preprocessor Definitions ***/

/*** Type Prototypes ***/

/*** Local Variables ***/

/*** External Variables ***/

/*** Function Prototypes ***/
static TIM_TypeDef  *prv_getTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateTimer(TIM_Id_t timer);
static TIM_Status_t  prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch);
static uint8_t       prv_maxChannels(TIM_Id_t timer);

/*** Function Definitions ***/

/*
 * Habilita los relojes de todos los timers y los resetea
 * para dejarlos en un estado limpio al iniciar.
 */
void tim_init(void)
{
    /* Habilitar relojes */
    RCC->APB2ENR |= (RCC_APB2ENR_TIM1EN  | RCC_APB2ENR_TIM9EN  |
                     RCC_APB2ENR_TIM10EN  | RCC_APB2ENR_TIM11EN);
    RCC->APB1ENR |= (RCC_APB1ENR_TIM2EN  | RCC_APB1ENR_TIM3EN  |
                     RCC_APB1ENR_TIM4EN   | RCC_APB1ENR_TIM5EN);

    /* Reset para limpiar todos los registros */
    RCC->APB2RSTR |=  (RCC_APB2RSTR_TIM1RST  | RCC_APB2RSTR_TIM9RST  |
                       RCC_APB2RSTR_TIM10RST | RCC_APB2RSTR_TIM11RST);
    RCC->APB2RSTR &= ~(RCC_APB2RSTR_TIM1RST  | RCC_APB2RSTR_TIM9RST  |
                       RCC_APB2RSTR_TIM10RST | RCC_APB2RSTR_TIM11RST);

    RCC->APB1RSTR |=  (RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST |
                       RCC_APB1RSTR_TIM4RST | RCC_APB1RSTR_TIM5RST);
    RCC->APB1RSTR &= ~(RCC_APB1RSTR_TIM2RST | RCC_APB1RSTR_TIM3RST |
                       RCC_APB1RSTR_TIM4RST | RCC_APB1RSTR_TIM5RST);
}

/*
 * Habilita el reloj de un solo timer.
 * Util para inicializar individualmente sin tocar los demas.
 */
TIM_Status_t tim_initTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    switch (timer)
    {
        case TIM_ID_1:  RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;  break;
        case TIM_ID_2:  RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;  break;
        case TIM_ID_3:  RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;  break;
        case TIM_ID_4:  RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;  break;
        case TIM_ID_5:  RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;  break;
        case TIM_ID_9:  RCC->APB2ENR |= RCC_APB2ENR_TIM9EN;  break;
        case TIM_ID_10: RCC->APB2ENR |= RCC_APB2ENR_TIM10EN; break;
        case TIM_ID_11: RCC->APB2ENR |= RCC_APB2ENR_TIM11EN; break;
        default: break;
    }

    /* Lectura de sincronizacion para asegurar que el reloj quedo habilitado */
    (void)RCC->APB1ENR;
    (void)RCC->APB2ENR;

    return TIM_OK;
}

/*
 * Configura el timer para que genere un evento cada 'ms' milisegundos.
 * Usa PSC=15999 para que cada tick sea 1 ms con reloj a 16 MHz.
 * Maximo: 65535 ms para timers de 16 bits, mas para TIM2/TIM5 (32 bits).
 */
TIM_Status_t tim_setTimerMs(TIM_Id_t timer, uint32_t ms)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;
    if (ms == 0U)     return TIM_ERR_INVALID_PARAM;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    uint32_t psc = (TIM_SYSCLK_HZ / 1000UL) - 1UL;  /* = 15999 */
    uint32_t arr = ms - 1UL;

    /* Timers de 16 bits: PSC y ARR no pueden superar 65535 */
    if (timer != TIM_ID_2 && timer != TIM_ID_5)
    {
        if (arr > 0xFFFFUL || psc > 0xFFFFUL) return TIM_ERR_INVALID_PARAM;
    }

    TIMx->CR1 &= ~TIM_CR1_CEN;
    TIMx->PSC  = psc;
    TIMx->ARR  = arr;
    TIMx->CNT  = 0U;
    TIMx->SR  &= ~TIM_SR_UIF;
    TIMx->EGR |= TIM_EGR_UG;    /* Fuerza la carga de PSC y ARR */
    TIMx->SR  &= ~TIM_SR_UIF;   /* Limpia el flag que levanta UG */

    return TIM_OK;
}

/*
 * Configura el timer para generar eventos a una frecuencia en Hz.
 * Busca la combinacion de PSC y ARR que mejor se aproxima a la frecuencia pedida.
 */
TIM_Status_t tim_setTimerFreq(TIM_Id_t timer, uint32_t hz)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;
    if (hz == 0U)     return TIM_ERR_INVALID_PARAM;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    uint8_t  is32bit = (timer == TIM_ID_2 || timer == TIM_ID_5);
    uint32_t arr_max = is32bit ? 0xFFFFFFFFUL : 0xFFFFUL;

    uint32_t psc   = 0U;
    uint32_t arr   = 0U;
    uint8_t  found = 0U;

    /* Prueba valores de PSC hasta encontrar un ARR que quepa */
    for (psc = 0U; psc <= 0xFFFFUL; psc++)
    {
        uint32_t div = (psc + 1UL) * hz;
        if (div == 0U) continue;
        arr = (TIM_SYSCLK_HZ / div);
        if (arr > 0U) arr -= 1UL;

        if (arr <= arr_max) { found = 1U; break; }
    }

    if (!found) return TIM_ERR_INVALID_PARAM;

    TIMx->CR1 &= ~TIM_CR1_CEN;
    TIMx->PSC  = psc;
    TIMx->ARR  = arr;
    TIMx->CNT  = 0U;
    TIMx->SR  &= ~TIM_SR_UIF;
    TIMx->EGR |= TIM_EGR_UG;
    TIMx->SR  &= ~TIM_SR_UIF;

    return TIM_OK;
}

/*
 * Arranca el timer (activa el bit CEN en CR1).
 */
TIM_Status_t tim_enableTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    prv_getTimer(timer)->CR1 |= TIM_CR1_CEN;
    return TIM_OK;
}

/*
 * Detiene el timer (desactiva el bit CEN en CR1).
 */
TIM_Status_t tim_disableTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    prv_getTimer(timer)->CR1 &= ~TIM_CR1_CEN;
    return TIM_OK;
}

/*
 * Espera bloqueando hasta que el timer complete un periodo.
 * El timer debe estar corriendo antes de llamar esta funcion.
 */
TIM_Status_t tim_waitTimer(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    while (!(TIMx->SR & TIM_SR_UIF)) { /* espera */ }
    TIMx->SR &= ~TIM_SR_UIF;

    return TIM_OK;
}

/*
 * Revisa si el timer termino sin bloquear el programa.
 * Retorna TIM_OK si termino, TIM_BUSY si todavia esta contando.
 * Usado por el modulo de botones para revisar debounce sin detener el loop.
 */
TIM_Status_t tim_isTimerExpired(TIM_Id_t timer)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    if (!(TIMx->SR & TIM_SR_UIF)) return TIM_BUSY;

    TIMx->SR &= ~TIM_SR_UIF;
    return TIM_OK;
}

/*
 * Escribe el valor del CCR de un canal.
 * En modo PWM, este valor define el ancho del pulso (duty cycle).
 */
TIM_Status_t tim_setTimerCompareChannelValue(TIM_Id_t      timer,
                                              TIM_Channel_t channel,
                                              uint32_t      value)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);

    switch (channel)
    {
        case TIM_CH_1: TIMx->CCR1 = value; break;
        case TIM_CH_2: TIMx->CCR2 = value; break;
        case TIM_CH_3: TIMx->CCR3 = value; break;
        case TIM_CH_4: TIMx->CCR4 = value; break;
        default: return TIM_ERR_INVALID_CHANNEL;
    }

    return TIM_OK;
}

/*
 * Configura el modo de comparacion de un canal (por ejemplo, PWM modo 1).
 * Tambien activa el preload para que los cambios de CCR sean suaves.
 */
TIM_Status_t tim_setTimerCompareMode(TIM_Id_t          timer,
                                     TIM_Channel_t     channel,
                                     TIM_CompareMode_t mode)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx = prv_getTimer(timer);
    uint32_t ocm = (uint32_t)mode & 0x7UL;

    switch (channel)
    {
        case TIM_CH_1:
            TIMx->CCMR1 &= ~(TIM_CCMR1_OC1M | TIM_CCMR1_CC1S);
            TIMx->CCMR1 |=  (ocm << 4U) | TIM_CCMR1_OC1PE;
            break;
        case TIM_CH_2:
            TIMx->CCMR1 &= ~(TIM_CCMR1_OC2M | TIM_CCMR1_CC2S);
            TIMx->CCMR1 |=  (ocm << 12U) | TIM_CCMR1_OC2PE;
            break;
        case TIM_CH_3:
            TIMx->CCMR2 &= ~(TIM_CCMR2_OC3M | TIM_CCMR2_CC3S);
            TIMx->CCMR2 |=  (ocm << 4U) | TIM_CCMR2_OC3PE;
            break;
        case TIM_CH_4:
            TIMx->CCMR2 &= ~(TIM_CCMR2_OC4M | TIM_CCMR2_CC4S);
            TIMx->CCMR2 |=  (ocm << 12U) | TIM_CCMR2_OC4PE;
            break;
        default: return TIM_ERR_INVALID_CHANNEL;
    }

    TIMx->CR1 |= TIM_CR1_ARPE;  /* Activa el preload del ARR */
    return TIM_OK;
}

/*
 * Habilita la salida de un canal en el registro CCER.
 * Para TIM1 (timer avanzado) tambien activa el bit MOE en BDTR.
 */
TIM_Status_t tim_enableTimerCompareChannel(TIM_Id_t      timer,
                                            TIM_Channel_t channel)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx  = prv_getTimer(timer);
    uint32_t     shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER |= (1UL << shift);

    /* TIM1 requiere MOE para activar las salidas */
    if (timer == TIM_ID_1) TIM1->BDTR |= TIM_BDTR_MOE;

    return TIM_OK;
}

/*
 * Deshabilita la salida de un canal en el registro CCER.
 */
TIM_Status_t tim_disableTimerCompareChannel(TIM_Id_t      timer,
                                             TIM_Channel_t channel)
{
    TIM_Status_t st = prv_validateChannel(timer, channel);
    if (st != TIM_OK) return st;

    TIM_TypeDef *TIMx  = prv_getTimer(timer);
    uint32_t     shift = ((uint32_t)(channel - 1U)) * 4U;
    TIMx->CCER &= ~(1UL << shift);

    return TIM_OK;
}

/* --- Funciones internas --- */

/* Convierte el enum de timer al puntero del registro correspondiente */
static TIM_TypeDef *prv_getTimer(TIM_Id_t timer)
{
    switch (timer)
    {
        case TIM_ID_1:  return TIM1;
        case TIM_ID_2:  return TIM2;
        case TIM_ID_3:  return TIM3;
        case TIM_ID_4:  return TIM4;
        case TIM_ID_5:  return TIM5;
        case TIM_ID_9:  return TIM9;
        case TIM_ID_10: return TIM10;
        case TIM_ID_11: return TIM11;
        default:        return (TIM_TypeDef *)0;
    }
}

/* Verifica que el ID del timer sea valido */
static TIM_Status_t prv_validateTimer(TIM_Id_t timer)
{
    if (timer >= TIM_ID_MAX) return TIM_ERR_INVALID_TIMER;
    return TIM_OK;
}

/* Retorna el numero maximo de canales de cada timer */
static uint8_t prv_maxChannels(TIM_Id_t timer)
{
    switch (timer)
    {
        case TIM_ID_10:
        case TIM_ID_11: return 1U;
        case TIM_ID_9:  return 2U;
        default:        return 4U;
    }
}

/* Verifica que el timer y el canal sean validos y compatibles */
static TIM_Status_t prv_validateChannel(TIM_Id_t timer, TIM_Channel_t ch)
{
    TIM_Status_t st = prv_validateTimer(timer);
    if (st != TIM_OK) return st;

    if (ch < TIM_CH_1 || ch > TIM_CH_4)      return TIM_ERR_INVALID_CHANNEL;
    if ((uint8_t)ch > prv_maxChannels(timer)) return TIM_ERR_CHANNEL_UNSUP;

    return TIM_OK;
}
