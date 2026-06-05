/**
 ******************************************************************************
 * @file    soil_moisture.c
 * @brief   Soil Moisture Sensor Module - Implementation
 *          2 YL-69 sensors per section, average as final reading.
 *
 *   PC0 ADC1_CH10 - Sec A sensor 1
 *   PC2 ADC1_CH12 - Sec A sensor 2
 *   PC1 ADC1_CH11 - Sec B sensor 1
 *   PC3 ADC1_CH13 - Sec B sensor 2
 *
 * @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
 * @date May 2026
 ******************************************************************************
 */

#include "soil_moisture.h"

/* =========================================================================
 * Private Constants
 * ========================================================================= */

#define SOIL_GPIO_PORT      GPIO_PORT_C
#define SOIL_PIN_A1         0U      /* PC0 - ADC1_CH10 */
#define SOIL_PIN_A2         2U      /* PC2 - ADC1_CH12 */
#define SOIL_PIN_B1         1U      /* PC1 - ADC1_CH11 */
#define SOIL_PIN_B2         3U      /* PC3 - ADC1_CH13 */
#define SOIL_ADC_INSTANCE   ADC_INSTANCE_1
#define SOIL_SAMPLE_TIME    ADC_SAMPLETIME_480CYCLES

/* =========================================================================
 * Private Variables
 * ========================================================================= */

static uint8_t s_initialized = 0U;

/* =========================================================================
 * Private Helpers
 * ========================================================================= */

/*
 * Reads a single ADC channel and returns the raw value.
 */
static SoilMoisture_Status_t prv_readChannel(uint8_t channel, uint16_t *rawOut)
{
    if (adc_setChannel(SOIL_ADC_INSTANCE, channel,
                       SOIL_SAMPLE_TIME) != ADC_OK) return SOIL_ERR_ADC;
    if (adc_startSingleConversion(SOIL_ADC_INSTANCE) != ADC_OK) return SOIL_ERR_ADC;
    if (adc_readData(SOIL_ADC_INSTANCE, rawOut) != ADC_OK) return SOIL_ERR_ADC;
    return SOIL_OK;
}

/*
 * Converts a raw value to humidity percentage.
 * Inverse mapping: higher raw = drier soil = lower %.
 */
static uint8_t prv_rawToPercent(uint16_t raw)
{
    uint32_t pct;
    if (raw >= SOIL_MOISTURE_RAW_DRY)       return 0U;
    if (raw <= SOIL_MOISTURE_RAW_WET)       return 100U;
    pct = ((uint32_t)(SOIL_MOISTURE_RAW_DRY - raw) * 100U)
          / (uint32_t)(SOIL_MOISTURE_RAW_DRY - SOIL_MOISTURE_RAW_WET);
    return (uint8_t)pct;
}

/* =========================================================================
 * Public API
 * ========================================================================= */

/*
 * Initializes the 4 pins as analog inputs and enables ADC1.
 */
SoilMoisture_Status_t soilMoisture_init(void)
{
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ANALOG;
    cfg.pull  = GPIO_PULL_NONE;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;

    gpio_initPort(SOIL_GPIO_PORT);

    if (gpio_setPinMode(SOIL_GPIO_PORT, SOIL_PIN_A1, &cfg) != GPIO_OK) return SOIL_ERR_INIT;
    if (gpio_setPinMode(SOIL_GPIO_PORT, SOIL_PIN_A2, &cfg) != GPIO_OK) return SOIL_ERR_INIT;
    if (gpio_setPinMode(SOIL_GPIO_PORT, SOIL_PIN_B1, &cfg) != GPIO_OK) return SOIL_ERR_INIT;
    if (gpio_setPinMode(SOIL_GPIO_PORT, SOIL_PIN_B2, &cfg) != GPIO_OK) return SOIL_ERR_INIT;

    if (adc_init(SOIL_ADC_INSTANCE)      != ADC_OK) return SOIL_ERR_INIT;
    if (adc_enableAdc(SOIL_ADC_INSTANCE) != ADC_OK) return SOIL_ERR_INIT;

    s_initialized = 1U;
    return SOIL_OK;
}

/*
 * Reads the average humidity from the two sensors of the section.
 * The returned value is (sensor1 + sensor2) / 2 as a percentage.
 */
SoilMoisture_Status_t soilMoisture_readPercent(SoilMoisture_Section_t section,
                                                uint8_t *percentOut)
{
    uint16_t raw1, raw2;
    uint8_t  ch1, ch2;

    if (section >= SOIL_SECTION_MAX) return SOIL_ERR_INVALID;
    if (percentOut == (uint8_t *)0)  return SOIL_ERR_INVALID;
    if (s_initialized == 0U)         return SOIL_ERR_INIT;

    if (section == SOIL_SECTION_A)
    {
        ch1 = SOIL_MOISTURE_CHANNEL_A1;
        ch2 = SOIL_MOISTURE_CHANNEL_A2;
    }
    else
    {
        ch1 = SOIL_MOISTURE_CHANNEL_B1;
        ch2 = SOIL_MOISTURE_CHANNEL_B2;
    }

    if (prv_readChannel(ch1, &raw1) != SOIL_OK) return SOIL_ERR_ADC;
    if (prv_readChannel(ch2, &raw2) != SOIL_OK) return SOIL_ERR_ADC;

    uint8_t pct1 = prv_rawToPercent(raw1);
    uint8_t pct2 = prv_rawToPercent(raw2);

    *percentOut = (uint8_t)((pct1 + pct2) / 2U);
    return SOIL_OK;
}

/*
 * Reads the raw value of the first sensor of the section (for calibration).
 */
SoilMoisture_Status_t soilMoisture_readRaw(SoilMoisture_Section_t section,
                                            uint16_t *rawOut)
{
    uint8_t ch;
    if (section >= SOIL_SECTION_MAX) return SOIL_ERR_INVALID;
    if (rawOut == (uint16_t *)0)     return SOIL_ERR_INVALID;
    if (s_initialized == 0U)         return SOIL_ERR_INIT;

    ch = (section == SOIL_SECTION_A) ? SOIL_MOISTURE_CHANNEL_A1
                                     : SOIL_MOISTURE_CHANNEL_B1;
    return prv_readChannel(ch, rawOut);
}

/*
 * Classifies the average humidity level of the section.
 */
SoilMoisture_Status_t soilMoisture_getStatus(SoilMoisture_Section_t section,
                                              SoilMoisture_Level_t *levelOut)
{
    uint8_t percent;

    if (section >= SOIL_SECTION_MAX)         return SOIL_ERR_INVALID;
    if (levelOut == (SoilMoisture_Level_t*)0) return SOIL_ERR_INVALID;

    if (soilMoisture_readPercent(section, &percent) != SOIL_OK)
        return SOIL_ERR_ADC;

    if (percent < SOIL_MOISTURE_THRESHOLD_TRIGGER_PCT)
        *levelOut = SOIL_LEVEL_DRY;
    else if (percent > SOIL_MOISTURE_THRESHOLD_WET_PCT)
        *levelOut = SOIL_LEVEL_WET;
    else
        *levelOut = SOIL_LEVEL_OK;

    return SOIL_OK;
}