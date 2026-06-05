/**
 ******************************************************************************
 * @file    servo.h
 * @brief   Servo Motor Control Module - Public API
 *          Controls two servo motors for irrigation valve control
 *          replacing the solenoid valve system.
 *
 * Hardware Assignment:
 *   PC6  ->  TIM3_CH1 (AF2) - Servo A - Section A
 *   PC9  ->  TIM3_CH4 (AF2) - Servo B - Section B
 *
 * PWM Servo Signal (standard):
 *   Frequency  : 50 Hz  (period = 20 ms)
 *   Pulse 1 ms  -> 0   deg  (valve closed)
 *   Pulse 1.5ms -> 90  deg  (mid position)
 *   Pulse 2 ms  -> 180 deg  (valve open)
 *
 *   With ARR = 19999 and PSC = 15 (1 MHz tick):
 *   CCR for 0   deg = 1000
 *   CCR for 90  deg = 1500
 *   CCR for 180 deg = 2000
 *
 * Functional Requirements:
 *   FR-1  servo_init      - Configure TIM3 and GPIO for both servos
 *   FR-2  servo_setAngle  - Move servo to a given angle (0-180 deg)
 *   FR-3  servo_open      - Move servo to open position (0 deg)
 *   FR-4  servo_close     - Move servo to closed position (180 deg)
 *
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date May 2026
 ******************************************************************************
 */

#ifndef SERVO_H
#define SERVO_H

#include <stdint.h>
#include "gpio_driver.h"
#include "tim_driver.h"
#include "soil_moisture.h"

/* =========================================================================
 * Pin Definitions
 * ========================================================================= */

#define SERVO_PORT          GPIO_PORT_C
#define SERVO_PIN_A         6U      /* PC6 - TIM3_CH1 - Servo Section A */
#define SERVO_PIN_B         9U      /* PC9 - TIM3_CH4 - Servo Section B */
#define SERVO_AF            2U      /* AF2 = TIM3 on PC6/PC9 (RM0383 Table 9) */

/* =========================================================================
 * Timer Configuration
 * ========================================================================= */

#define SERVO_TIM           TIM_ID_3
#define SERVO_CHANNEL_A     TIM_CH_1    /* TIM3_CH1 - PC6 */
#define SERVO_CHANNEL_B     TIM_CH_4    /* TIM3_CH4 - PC9 */

/**
 * @brief PWM configuration for standard servo signal at 50 Hz.
 *
 * PSC = 15  -> tick = 1 us  (16 MHz / 16 = 1 MHz)
 * ARR = 19999 -> period = 20 ms = 50 Hz
 *
 * Pulse widths in counts (1 count = 1 us):
 *   0   deg -> CCR = 1000  (1.0 ms)
 *   90  deg -> CCR = 1500  (1.5 ms)
 *   180 deg -> CCR = 2000  (2.0 ms)
 */
#define SERVO_PSC           15U
#define SERVO_ARR           19999U
#define SERVO_CCR_0DEG      1000U
#define SERVO_CCR_90DEG     1500U
#define SERVO_CCR_180DEG    2000U

/* =========================================================================
 * Angle Definitions
 * ========================================================================= */

#define SERVO_ANGLE_CLOSED  180U      /* Valve closed */
#define SERVO_ANGLE_OPEN    0U        /* Valve open   */

/* =========================================================================
 * Enumerations
 * ========================================================================= */

typedef enum
{
    SERVO_OK            =  0,
    SERVO_ERR_INIT      = -1,
    SERVO_ERR_INVALID   = -2
} Servo_Status_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/**
 * @brief  FR-1 - Initialize TIM3 and GPIO for both servos.
 *         Configures PC6 and PC9 as AF2 (TIM3).
 *         Configures TIM3 at 50 Hz with PSC=15, ARR=19999.
 *         Both servos start at closed position (0 deg).
 * @retval SERVO_OK        Initialization successful.
 * @retval SERVO_ERR_INIT  GPIO or Timer failure.
 */
Servo_Status_t servo_init(void);

/**
 * @brief  FR-2 - Move servo to a specific angle (0-180 deg).
 * @param[in] section  SOIL_SECTION_A or SOIL_SECTION_B.
 * @param[in] angle    Desired angle (0-180 degrees).
 * @retval SERVO_OK          Movement applied.
 * @retval SERVO_ERR_INVALID Invalid section or angle.
 */
Servo_Status_t servo_setAngle(SoilMoisture_Section_t section, uint8_t angle);

/**
 * @brief  FR-3 - Move servo to fully open position (0 deg).
 *         Equivalent to opening the water valve of the section.
 * @param[in] section  SOIL_SECTION_A or SOIL_SECTION_B.
 * @retval SERVO_OK          Servo moved to 0 deg.
 * @retval SERVO_ERR_INVALID Invalid section.
 */
Servo_Status_t servo_open(SoilMoisture_Section_t section);

/**
 * @brief  FR-4 - Move servo to fully closed position (180 deg).
 *         Equivalent to closing the water valve of the section.
 * @param[in] section  SOIL_SECTION_A or SOIL_SECTION_B.
 * @retval SERVO_OK          Servo moved to 180 deg.
 * @retval SERVO_ERR_INVALID Invalid section.
 */
Servo_Status_t servo_close(SoilMoisture_Section_t section);

#endif /* SERVO_H */
