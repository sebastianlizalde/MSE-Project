/**
 ******************************************************************************
 * @file    soil_moisture.h
 * @brief   Soil Moisture Sensor Module - Public API
 *          Sensor:  YL-69 / HW-103 (LM393 comparator board)
 *          Target:  STM32F411RET6 (NUCLEO-F411RE)
 *          Method:  Bare-Metal, ADC1
 *
 * Pin Assignment (2 sensors per section):
 *   PC0  ->  SOIL_A1  ->  ADC1_CH10  (Section A, sensor 1)
 *   PC2  ->  SOIL_A2  ->  ADC1_CH12  (Section A, sensor 2)
 *   PC1  ->  SOIL_B1  ->  ADC1_CH11  (Section B, sensor 1)
 *   PC3  ->  SOIL_B2  ->  ADC1_CH13  (Section B, sensor 2)
 *
 * Humidity reported per section is the average of its two sensors.
 *
 * ADC Behavior (YL-69 resistive sensor):
 *   Dry soil -> high resistance -> high voltage -> HIGH ADC value
 *   Wet soil -> low  resistance -> low  voltage -> LOW  ADC value
 *
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date May 2026
 ******************************************************************************
 */

#ifndef SOIL_MOISTURE_H
#define SOIL_MOISTURE_H

#include <stdint.h>
#include "adc_driver.h"
#include "gpio_driver.h"

/* =========================================================================
 * ADC Channel Definitions - 2 sensors per section
 * ========================================================================= */

#define SOIL_MOISTURE_CHANNEL_A1    10U     /* PC0 - Section A, sensor 1 */
#define SOIL_MOISTURE_CHANNEL_A2    12U     /* PC2 - Section A, sensor 2 */
#define SOIL_MOISTURE_CHANNEL_B1    11U     /* PC1 - Section B, sensor 1 */
#define SOIL_MOISTURE_CHANNEL_B2    13U     /* PC3 - Section B, sensor 2 */

#define SOIL_MOISTURE_VREF_MV       3300U
#define SOIL_MOISTURE_ADC_MAX       4095U

/* =========================================================================
 * Calibration Values
 * Adjust with real sensor measurements in air and in water
 * ========================================================================= */

#define SOIL_MOISTURE_RAW_DRY       3200U   /* ADC counts - dry soil / air          */
#define SOIL_MOISTURE_RAW_WET        800U   /* ADC counts - saturated soil / water  */

/* =========================================================================
 * Irrigation Thresholds
 * ========================================================================= */

#define SOIL_MOISTURE_THRESHOLD_TRIGGER_PCT  30U  /* % - starts irrigation cycle */
#define SOIL_MOISTURE_THRESHOLD_MID_PCT      60U  /* % - servo moves to 90 deg   */
#define SOIL_MOISTURE_THRESHOLD_WET_PCT      80U  /* % - servo closes to 0 deg   */

/* =========================================================================
 * Enumerations
 * ========================================================================= */

typedef enum
{
    SOIL_SECTION_A = 0,
    SOIL_SECTION_B = 1,
    SOIL_SECTION_MAX
} SoilMoisture_Section_t;

typedef enum
{
    SOIL_LEVEL_WET  = 0,  /**< > 80% - saturated soil  */
    SOIL_LEVEL_OK   = 1,  /**< 30-80% - normal range   */
    SOIL_LEVEL_DRY  = 2   /**< < 30% - dry soil        */
} SoilMoisture_Level_t;

typedef enum
{
    SOIL_OK          =  0,
    SOIL_ERR_INIT    = -1,
    SOIL_ERR_INVALID = -2,
    SOIL_ERR_ADC     = -3
} SoilMoisture_Status_t;

/* =========================================================================
 * Public API
 * ========================================================================= */

/* Initializes the 4 analog pins and ADC1 */
SoilMoisture_Status_t soilMoisture_init(void);

/* Reads the average of the two sensors of a section as a percentage */
SoilMoisture_Status_t soilMoisture_readPercent(SoilMoisture_Section_t section,
                                                uint8_t *percentOut);

/* Reads the raw value of one individual sensor (for calibration) */
SoilMoisture_Status_t soilMoisture_readRaw(SoilMoisture_Section_t section,
                                            uint16_t *rawOut);

/* Classifies the humidity level of a section */
SoilMoisture_Status_t soilMoisture_getStatus(SoilMoisture_Section_t section,
                                              SoilMoisture_Level_t *levelOut);

#endif /* SOIL_MOISTURE_H */
