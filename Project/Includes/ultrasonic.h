/**
 ******************************************************************************
 * @file    ultrasonic.h
 * @brief   JSN-SR04T module with distance calibration
 *          Target: STM32F411RE NUCLEO-64
 *
 * Hardware:
 *   TRIG -> PA9 (D8) GPIO push-pull output
 *   ECHO -> PB3 (D3) GPIO input (voltage divider 10k/20k required)
 *   TIM5 -> free-running @1MHz for microsecond measurement
 *
 * ═══════════════════════════════════════════════════════
 *  HOW TO CALIBRATE:
 *
 *  1. Place an object at exactly 30 cm from the sensor
 *  2. Observe the distance printed on UART
 *  3. Calculate: CALIB_OFFSET_CM = printed_dist - 30
 *     Example: prints 33 cm -> CALIB_OFFSET_CM = 3
 *              prints 27 cm -> CALIB_OFFSET_CM = -3
 *
 *  4. Adjust CALIB_SOUND_SPEED according to ambient temperature:
 *     15C -> 3403    20C -> 3434    25C -> 3464
 *     30C -> 3494    35C -> 3525    40C -> 3555
 *     (Tijuana in summer -> use 3525 or 3555)
 *
 *  5. Adjust DIST_* thresholds according to your tank dimensions
 * ═══════════════════════════════════════════════════════
 ******************************************************************************
 */

#ifndef ULTRASONIC_H
#define ULTRASONIC_H

#include "gpio_driver.h"
#include "stm32f4xx.h"
#include <stdint.h>

/* ─────────────────────────────────────────────────────
   CALIBRATION - EDIT HERE
   ───────────────────────────────────────────────────── */

/* Speed of sound x10 according to ambient temperature:
 * 20C=3434  25C=3464  30C=3494  35C=3525  40C=3555  */
#define CALIB_SOUND_SPEED   3300U   /* 330 m/s per JSN-SR04T datasheet */

/* Correction offset in cm (can be negative):
 * If sensor reads too high -> positive value
 * If sensor reads too low  -> negative value
 * Start with 0, calibrate with known distance object */
#define CALIB_OFFSET_CM        0    /* no offset */

/* ─────────────────────────────────────────────────────
   LEVEL THRESHOLDS - EDIT HERE (CALIBRATE with your tank)
   Sensor mounted on TOP: small distance = full, large = empty.
     dist <= DIST_FULL_CM  -> GREEN  (full)
     dist <= DIST_MID_CM   -> YELLOW (mid)
     dist >  DIST_MID_CM   -> RED    (low)
   Must satisfy: DIST_FULL_CM < DIST_MID_CM
   PROVISIONAL values - adjust after measuring real full/empty cm.
   ───────────────────────────────────────────────────── */
#define DIST_FULL_CM        4U    /**< <= 4 cm  -> Green (full). Set 1-2 cm
                                    *   ABOVE the actual full reading so the
                                    *   pump stops BEFORE overflowing.        */
#define DIST_MID_CM         7U    /**< <= 7 cm  -> Yellow (mid)               */

/* Number of samples for median filter (always odd: 3, 5, or 7) */
#define NUM_SAMPLES          3U     /* 3 = fastest measurement (~120 ms) */

/* ─── Hardware pins ─── */
#define TRIG_PORT   GPIO_PORT_A
#define TRIG_PIN    9U             /* PA9  D8 */

#define ECHO_PORT   GPIO_PORT_B
#define ECHO_PIN    3U             /* PB3  D3 */

/* ─── Semaphore LEDs ─── */
#define LED_GREEN_PORT      GPIO_PORT_C
#define LED_GREEN_PIN       7U     /* PC7  D9  - FULL  */

#define LED_YELLOW_PORT     GPIO_PORT_B
#define LED_YELLOW_PIN      10U    /* PB10 D6  - MID   */

#define LED_RED_PORT        GPIO_PORT_B
#define LED_RED_PIN         4U     /* PB4  D5  - LOW   */

/* ─── Water level enumeration ─── */
typedef enum {
    WATER_LEVEL_UNKNOWN = 0,
    WATER_LEVEL_LOW,
    WATER_LEVEL_MID,
    WATER_LEVEL_HIGH
} WaterLevel_t;

/* ─── Public API ─── */
void          ultrasonic_init(void);
uint32_t      ultrasonic_measure_cm(void);
WaterLevel_t  ultrasonic_updateLevel(uint32_t dist_cm, uint32_t *level_pct);

#endif /* ULTRASONIC_H */
