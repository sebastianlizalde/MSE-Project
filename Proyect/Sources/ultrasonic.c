/**
 ******************************************************************************
 * @file    ultrasonic.c
 * @brief   JSN-SR04T with median filter + spike filter - Target: STM32F411RE
 *
 *   TRIG -> PA9 (D8)  GPIO push-pull output
 *   ECHO -> PB3 (D3)  GPIO input + 10kOhm/20kOhm divider
 *   TIM5 -> free-running @1MHz
 *
 * Fixes compared to the previous version:
 *
 *  1. TIMEOUT_ECHO_START_US increased to 100 000 us (100 ms).
 *     Water reflects weakly; the echo takes longer to reach the sensor.
 *     With 50 ms many valid measurements were discarded as timeout.
 *
 *  2. Inter-sample pause increased from 60 ms to 100 ms.
 *     Datasheet requires minimum 20 ms between triggers; with water
 *     echoes reverberate longer and 60 ms was not enough to clear
 *     the channel before the next TRIG.
 *
 *  3. Spike filter: discards samples that differ more than
 *     SPIKE_THRESHOLD_CM (8 cm) from the previous sample.
 *     Eliminates random jumps caused by wall echoes.
 *
 *  4. WATER_LEVEL_UNKNOWN no longer turns on the red LED automatically.
 *     Keeps the last known LED state to avoid red/pump-off flickering
 *     when there is only one bad reading.
 *     Only turns red on after UNKNOWN_COUNT_MAX consecutive cycles
 *     without a valid reading.
 ******************************************************************************
 */

#include "ultrasonic.h"
#include "stm32f4xx.h"
#include <stdint.h>

/* ── Measurement parameters ─────────────────────────────────────────────── */
/* HC-SR04: blind zone ~2 cm (the JSN-SR04T was 25 cm). Allows reading
 * short tanks without a PLA extension. */
#define SENSOR_MIN_CM           2U

/* ECHO rising edge timeout: 100 ms - more margin for weak water echoes */
#define TIMEOUT_ECHO_START_US   100000UL

/* ECHO pulse duration timeout: 40 ms (approx 680 cm @ 340 m/s, plenty of margin)    */
#define TIMEOUT_ECHO_PULSE_US   40000UL

/* Inter-trigger pause: 40 ms - enough for the echo to dissipate,
 * and keeps total measurement short (~120 ms with 3 samples) */
#define INTER_SAMPLE_DELAY_US   40000UL

/* Discard samples that differ more than this value from the previous one (spike)   */
#define SPIKE_THRESHOLD_CM      8U

/* Valid sensor range                                                   */
#define DIST_MAX_VALID_CM       450U

/* Consecutive cycles without a valid reading before declaring real UNKNOWN     */
#define UNKNOWN_COUNT_MAX       3U

/* ── Persistent state ─────────────────────────────────────────────────── */
static uint32_t     s_last_valid_cm    = 0U;   /* last valid distance   */
static uint32_t     s_unknown_count    = 0U;   /* consecutive UNKNOWN cycles   */

/* ══════════════════════════════════════════════════════════════════════════ */
/*  TIM5 @1 MHz - free-running for us measurement                                 */
/* ══════════════════════════════════════════════════════════════════════════ */
static void prv_tim5_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_TIM5EN;
    (void)RCC->APB1ENR;
    TIM5->CR1 = 0U;
    TIM5->PSC = 15U;              /* 16 MHz / (15+1) = 1 MHz -> 1 tick = 1 us */
    TIM5->ARR = 0xFFFFFFFFUL;     /* 32-bit free-running                      */
    TIM5->CNT = 0U;
    TIM5->EGR = TIM_EGR_UG;
    TIM5->SR  = 0U;
    TIM5->CR1|= TIM_CR1_CEN;
}

static void prv_delay_us(uint32_t us)
{
    uint32_t start = TIM5->CNT;
    while ((TIM5->CNT - start) < us) { }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  INITIALIZATION                                                            */
/* ══════════════════════════════════════════════════════════════════════════ */
void ultrasonic_init(void)
{
    GPIO_PinCfg_t cfg;
    prv_tim5_init();

    /* TRIG: PA9 push-pull output */
    gpio_initPort(TRIG_PORT);
    cfg.mode  = GPIO_MODE_OUTPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_VHIGH;
    cfg.pull  = GPIO_PULL_NONE;
    gpio_setPinMode(TRIG_PORT, TRIG_PIN, &cfg);
    gpio_clearPin(TRIG_PORT, TRIG_PIN);

    /* ECHO: PB3 input with no pull (external 10k/20k divider) */
    gpio_initPort(ECHO_PORT);
    cfg.mode  = GPIO_MODE_INPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_VHIGH;
    cfg.pull  = GPIO_PULL_NONE;
    gpio_setPinMode(ECHO_PORT, ECHO_PIN, &cfg);

    /* Level LEDs */
    cfg.mode  = GPIO_MODE_OUTPUT;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    cfg.pull  = GPIO_PULL_NONE;

    gpio_initPort(LED_GREEN_PORT);
    gpio_setPinMode(LED_GREEN_PORT,  LED_GREEN_PIN,  &cfg);
    gpio_clearPin(LED_GREEN_PORT,  LED_GREEN_PIN);

    gpio_initPort(LED_YELLOW_PORT);
    gpio_setPinMode(LED_YELLOW_PORT, LED_YELLOW_PIN, &cfg);
    gpio_clearPin(LED_YELLOW_PORT, LED_YELLOW_PIN);

    gpio_initPort(LED_RED_PORT);
    gpio_setPinMode(LED_RED_PORT,    LED_RED_PIN,    &cfg);
    gpio_clearPin(LED_RED_PORT,    LED_RED_PIN);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  SINGLE MEASUREMENT                                                       */
/* ══════════════════════════════════════════════════════════════════════════ */
static uint32_t prv_single_measure(void)
{
    GPIO_PinState_t st;
    uint32_t t0, t1;

    /* TRIG pulse: LOW 5 us -> HIGH 20 us -> LOW */
    gpio_clearPin(TRIG_PORT, TRIG_PIN);
    prv_delay_us(5U);
    gpio_setPin(TRIG_PORT, TRIG_PIN);
    prv_delay_us(20U);
    gpio_clearPin(TRIG_PORT, TRIG_PIN);

    /* Wait for ECHO rising edge (100 ms timeout) */
    t0 = TIM5->CNT;
    do {
        gpio_readPin(ECHO_PORT, ECHO_PIN, &st);
        if ((TIM5->CNT - t0) >= TIMEOUT_ECHO_START_US) return 0U;
    } while (st == GPIO_PIN_LOW);

    /* Measure ECHO pulse duration */
    t0 = TIM5->CNT;
    do {
        gpio_readPin(ECHO_PORT, ECHO_PIN, &st);
        if ((TIM5->CNT - t0) >= TIMEOUT_ECHO_PULSE_US) return 0U;
    } while (st == GPIO_PIN_HIGH);
    t1 = TIM5->CNT;

    /* Distance = (time_us x sound_speed x 10) / 200 000 - offset */
    int32_t d = (int32_t)((t1 - t0) * CALIB_SOUND_SPEED / 200000UL)
              - (int32_t)CALIB_OFFSET_CM;

    if (d < 0)                            return 0U;
    if ((uint32_t)d > DIST_MAX_VALID_CM)  return 0U;
    return (uint32_t)d;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  SORT (bubble sort - small N, sufficient)                                   */
/* ══════════════════════════════════════════════════════════════════════════ */
static void prv_sort(uint32_t *a, uint32_t n)
{
    for (uint32_t i = 0U; i < n - 1U; i++)
        for (uint32_t j = 0U; j < n - 1U - i; j++)
            if (a[j] > a[j + 1U]) {
                uint32_t t = a[j]; a[j] = a[j + 1U]; a[j + 1U] = t;
            }
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  MEASUREMENT WITH MEDIAN + SPIKE FILTER                                   */
/*                                                                            */
/*  Toma NUM_SAMPLES disparos con pausa de 100 ms entre cada uno.            */
/*  Applies spike filter: discards samples that differ more than              */
/*  SPIKE_THRESHOLD_CM de la muestra inmediatamente anterior.                */
/*  Retorna la mediana de las muestras que pasaron el filtro.                */
/*                                                                            */
/*  If not enough valid samples, returns s_last_valid_cm                      */
/*  (last known good value) to avoid false transitions.                       */
/*  Solo devuelve 0 si se acumulan UNKNOWN_COUNT_MAX ciclos fallidos         */
/*  consecutive cycles -- signal that the tank is truly empty or no signal.   */
/* ══════════════════════════════════════════════════════════════════════════ */
uint32_t ultrasonic_measure_cm(void)
{
    uint32_t buf[NUM_SAMPLES];
    uint32_t n          = 0U;
    uint32_t prev       = 0U;   /* previous sample for spike filter        */

    for (uint32_t i = 0U; i < NUM_SAMPLES; i++)
    {
        uint32_t d = prv_single_measure();

        if (d == 0U || d < SENSOR_MIN_CM)
        {
            /* Invalid sample - ignore, do not update prev */
        }
        else if (prev != 0U &&
                 ((d > prev ? d - prev : prev - d) > SPIKE_THRESHOLD_CM))
        {
            /* Spike: difference greater than threshold compared to previous.
             * Could be a wall echo or bounce - discard. */
            prev = d;   /* update prev to avoid always blocking */
        }
        else
        {
            buf[n++] = d;
            prev = d;
        }

        prv_delay_us(INTER_SAMPLE_DELAY_US);
    }

    /* No valid samples in this cycle */
    if (n == 0U)
    {
        s_unknown_count++;
        if (s_unknown_count >= UNKNOWN_COUNT_MAX)
        {
            /* Several consecutive cycles without signal - declare real UNKNOWN */
            s_last_valid_cm = 0U;
            return 0U;
        }
        /* Still within tolerance - return last good value */
        return s_last_valid_cm;
    }

    /* Valid samples found: reset unknown counter and compute median */
    s_unknown_count = 0U;
    prv_sort(buf, n);
    s_last_valid_cm = buf[n / 2U];
    return s_last_valid_cm;
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  LEDs                                                                      */
/* ══════════════════════════════════════════════════════════════════════════ */
static void prv_leds_all_off(void)
{
    gpio_clearPin(LED_GREEN_PORT,  LED_GREEN_PIN);
    gpio_clearPin(LED_YELLOW_PORT, LED_YELLOW_PIN);
    gpio_clearPin(LED_RED_PORT,    LED_RED_PIN);
}

/* ══════════════════════════════════════════════════════════════════════════ */
/*  LEVEL CLASSIFICATION                                                    */
/*                                                                            */
/*  dist_cm == 0 → UNKNOWN real (varios ciclos fallidos consecutivos).       */
/*  On UNKNOWN: turns off all LEDs and turns red on only after                */
/*  UNKNOWN_COUNT_MAX fallos - no en el primer eco perdido.                  */
/* ══════════════════════════════════════════════════════════════════════════ */
WaterLevel_t ultrasonic_updateLevel(uint32_t dist_cm, uint32_t *level_pct)
{
    WaterLevel_t level;

    if (dist_cm == 0U)
    {
        /* Real UNKNOWN - multiple cycles without signal */
        if (level_pct != (uint32_t *)0) { *level_pct = 0U; }   /* NULL guard */
        prv_leds_all_off();
        gpio_setPin(LED_RED_PORT, LED_RED_PIN);
        return WATER_LEVEL_UNKNOWN;
    }

    /* Classification for sensor mounted on TOP pointing down at water:
     *   SMALL distance  = water close to sensor = tank FULL  (green)
     *   LARGE distance  = water far away         = tank LOW   (red)
     *
     *   dist <= DIST_FULL_CM      -> GREEN   (full)
     *   dist <= DIST_MID_CM       -> YELLOW  (mid)
     *   dist >  DIST_MID_CM       -> RED     (low)
     *
     * Thresholds in ultrasonic.h - CALIBRATE with the actual tank. */
    if      (dist_cm <= DIST_FULL_CM) level = WATER_LEVEL_HIGH;
    else if (dist_cm <= DIST_MID_CM)  level = WATER_LEVEL_MID;
    else                              level = WATER_LEVEL_LOW;

    if (level_pct != (uint32_t *)0) { *level_pct = dist_cm; }   /* NULL guard */

    prv_leds_all_off();
    switch (level)
    {
        case WATER_LEVEL_HIGH: gpio_setPin(LED_GREEN_PORT,  LED_GREEN_PIN);  break;
        case WATER_LEVEL_MID:  gpio_setPin(LED_YELLOW_PORT, LED_YELLOW_PIN); break;
        case WATER_LEVEL_LOW:  gpio_setPin(LED_RED_PORT,    LED_RED_PIN);    break;
        default: break;
    }
    return level;
}