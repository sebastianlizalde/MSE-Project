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
* @file main.c
* @brief Automatic irrigation system - COMPLETE (PB1 + PB2 + PB3).
*
* PB1 - Brain board:
*   LCD HD44780 + PCF8574 (0x27)  PB8/PB9
*   Potenciometro 10k              PA1  (ADC1_CH1)
*   Boton STOP global              PC13 -> GND (pull-up interno)
*   Boton STOP-A                   PB13 -> GND (pull-up interno)
*   Boton STOP-B                   PB14 -> GND (pull-up interno)
*   UART debug                     PA2  (115200 baud)
*
* PB2 - Humidity and flow sensors:
*   YL-69 A1    PC0  (ADC1_CH10)    YL-69 A2    PC2  (ADC1_CH12)
*   YL-69 B1    PC1  (ADC1_CH11)    YL-69 B2    PC3  (ADC1_CH13)
*   Flujo A     PA0  (polling)      Flujo B     PB0  (polling)
*   LED valv A  PB12                LED valv B  PA6
*
* PB3 - Actuators and tank (EXTERNAL POWER SUPPLY, common GND with Nucleo):
*   Servo A           PC6  (TIM3_CH1, AF2)   Servo B   PC9  (TIM3_CH4, AF2)
*   TRIG ultrasonico  PA9 (D8)               ECHO ultrasonico  PB3 (D3, divisor 10k/20k)
*   Bomba PWM         PA5 (TIM2_CH1, transistor NPN 2N2222 + diodo flyback)
*   LED verde         PC7           LED amarillo      PB10
*   LED rojo          PB4
*
* Potentiometer (ADC 0-4095):
*   0    - 1364  -> LCD SecA: humidity + flow + valve status
*   1365 - 2729  -> LCD SecB: humidity + flow + valve status
*   2730 - 4095  -> LCD Deposit: distance + level + pump
*
* Update intervals:
*   Immediate   -> pot, buttons, flow pulses
*   Every 500 ms -> humidity, servos, LEDs, LCD
*   Every 1000 ms -> flow rate via UART
*   Every 2000 ms -> ultrasonic, pump, semaphore, deposit LCD
*
* Pump logic:
*   Level HIGH  -> pump OFF
*   Level MID   -> pump 75%
*   Level LOW   -> pump 100%
*   Global STOP active -> pump OFF immediately
*
* Semaphore logic (tank LEDs):
*   HIGH    -> green LED
*   MID     -> yellow LED
*   LOW     -> red LED
*   UNKNOWN -> red LED  (no sensor reading => alert, does NOT stay off)
*   Global STOP active -> semaphore off (system intentionally stopped)
*
* Servo logic (via irrigation_update):
*   Humidity < 30%  -> servo 0 deg   (valve open)
*   Humidity 30-80% -> servo 90 deg  (valve half)
*   Humidity > 80%  -> servo 180 deg (valve closed)
*   Section or global STOP -> servo 180 deg (valve closed)
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date May 2026
*/

/*** Includes ***/
#include "stm32f4xx.h"
#include "gpio_driver.h"
#include "tim_driver.h"
#include "timer.h"
#include "systick.h"
#include "uart.h"
#include "i2c_driver.h"
#include "lcd.h"
#include "adc_driver.h"
#include "soil_moisture.h"
#include "irrigation.h"
#include "servo.h"
#include "pump.h"
#include "pwm.h"
#include "ultrasonic.h"
#include "button.h"
#include "flowsensor.h"
#include "utils.h"

/*** Preprocessor Definitions ***/
#define INTERVAL_IRRIGATION_MS   500U
#define INTERVAL_FLOW_MS        1000U
#define INTERVAL_ULTRASONIC_MS   500U   /* reaccion rapida: corta la bomba a tiempo */

#define POT_GPIO_PORT           GPIO_PORT_A
#define POT_GPIO_PIN            1U
#define POT_ADC_CHANNEL         1U
#define POT_ADC_INSTANCE        ADC_INSTANCE_1

/*** Local Variables ***/
static uint16_t     g_pot_raw  = 0U;
static uint8_t      g_hum_a    = 0U;
static uint8_t      g_hum_b    = 0U;
static float        g_flow_a   = 0.0f;
static float        g_flow_b   = 0.0f;
static uint32_t     g_dist_cm  = 0U;
static WaterLevel_t g_level    = WATER_LEVEL_UNKNOWN;
static uint8_t      g_pump_duty = 0U;

/*** Function Prototypes ***/
static void     prv_systemInit(void);
static void     prv_potInit(void);
static uint16_t prv_potRead(void);
static void     prv_lcdSplash(void);
static void     prv_checkButtons(void);
static void     prv_updateLcd(void);
static void     prv_semaphoreUpdate(WaterLevel_t level);
static void     prv_semaphoreOff(void);
static void     prv_buildSectionRow0(char *buf, char sec,
                                     uint8_t hum, const char *valve,
                                     uint8_t stopped);
static void     prv_buildFlowRow1(char *buf, float flow, uint8_t stopped);
static void     prv_buildDepositoRow0(char *buf, uint32_t dist_cm,
                                      WaterLevel_t level);
static void     prv_buildDepositoRow1(char *buf, uint8_t pump_duty,
                                      uint8_t stopped_global);

/*** Function Definitions ***/

int main(void)
{
    prv_systemInit();
    prv_lcdSplash();

    uint32_t t_irrigation = 0U;
    uint32_t t_flow       = 0U;
    uint32_t t_ultrasonic = 0U;
    uint32_t pulses_a_prev = 0U;
    uint32_t pulses_b_prev = 0U;

    uart_sendString("\r\n=== Sistema de Riego Automatico - COMPLETO ===\r\n");
    uart_sendString("PC13=STOP global | PB13=STOP-A | PB14=STOP-B\r\n");
    uart_sendString("Pot: izq=SecA | centro=SecB | der=Deposito\r\n\r\n");

    while (1)
    {
        uint32_t now = systick_getTick();

        /* --- Always: pot, buttons, flow pulses --- */
        g_pot_raw = prv_potRead();
        button_poll();
        flow_update();
        prv_checkButtons();

        /* --- Every 500 ms: humidity + servos + LEDs + section LCD --- */
        if ((now - t_irrigation) >= INTERVAL_IRRIGATION_MS)
        {
            t_irrigation = now;

            /* irrigation_update handles servos, LEDs and section stop */
            irrigation_update();

            /* Read humidity for LCD and UART */
            soilMoisture_readPercent(SOIL_SECTION_A, &g_hum_a);
            soilMoisture_readPercent(SOIL_SECTION_B, &g_hum_b);

            char buf[32];
            utils_snprintf(buf, "HumA:%d%%  HumB:%d%%\r\n",
                           (int)g_hum_a, (int)g_hum_b);
            uart_sendString(buf);

            prv_updateLcd();
        }

        /* --- Every 1000 ms: flow rate A and B --- */
        if ((now - t_flow) >= INTERVAL_FLOW_MS)
        {
            t_flow = now;

            uint32_t pA  = flow_getPulses(FLOW_SECTION_A) - pulses_a_prev;
            pulses_a_prev = flow_getPulses(FLOW_SECTION_A);
            g_flow_a      = flow_getLitersPerMin(pA);

            uint32_t pB  = flow_getPulses(FLOW_SECTION_B) - pulses_b_prev;
            pulses_b_prev = flow_getPulses(FLOW_SECTION_B);
            g_flow_b      = flow_getLitersPerMin(pB);

            uint32_t aI = (uint32_t)g_flow_a;
            uint32_t aD = (uint32_t)((g_flow_a - (float)aI) * 10.0f);
            uint32_t bI = (uint32_t)g_flow_b;
            uint32_t bD = (uint32_t)((g_flow_b - (float)bI) * 10.0f);

            char buf[48];
            utils_snprintf(buf, "FlujoA:%d.%d L/m  FlujoB:%d.%d L/m\r\n",
                           (int)aI, (int)aD, (int)bI, (int)bD);
            uart_sendString(buf);
        }

        /* --- Every 2000 ms: ultrasonic + pump + semaphore --- */
        if ((now - t_ultrasonic) >= INTERVAL_ULTRASONIC_MS)
        {
            t_ultrasonic = now;

            if (!button_isStoppedGlobal())
            {
                /* Measure tank level */
                g_dist_cm = ultrasonic_measure_cm();
                g_level   = ultrasonic_updateLevel(g_dist_cm, (uint32_t*)0U);

                /* Control pump according to level */
                pump_setLevel(g_level);

                switch (g_level)
                {
                    case WATER_LEVEL_HIGH: g_pump_duty =   0U; break;
                    case WATER_LEVEL_MID:  g_pump_duty =  75U; break;
                    case WATER_LEVEL_LOW:  g_pump_duty = 100U; break;
                    default:               g_pump_duty =   0U; break;
                }

                /* Update tank LED semaphore.
                 * If sensor has no reading (UNKNOWN) the red LED turns on. */
                prv_semaphoreUpdate(g_level);

                char buf[48];
                utils_snprintf(buf, "Dep:%dcm Nivel:%d Bomba:%d%%\r\n",
                               (int)g_dist_cm, (int)g_level, (int)g_pump_duty);
                uart_sendString(buf);
            }
            else
            {
                /* Global stop: turn off pump and semaphore (system stopped) */
                pump_stop();
                g_pump_duty = 0U;
                g_level     = WATER_LEVEL_UNKNOWN;
                g_dist_cm   = 0U;
                prv_semaphoreOff();
            }

            prv_updateLcd();
        }
    }

    pump_stop();
    return 0;
}

/* ------------------------------------------------------------------ */
/*  Tank semaphore                                              */
/* ------------------------------------------------------------------ */

/*
 * Turns on only the LED for the current level and turns off the others.
 *   WATER_LEVEL_HIGH    -> green LED  (PC7)
 *   WATER_LEVEL_MID     -> amber LED  (PB10)
 *   WATER_LEVEL_LOW     -> red LED    (PB4)
 *   WATER_LEVEL_UNKNOWN -> red LED    (PB4) - no reading => alert, not off
 */
static void prv_semaphoreUpdate(WaterLevel_t level)
{
    gpio_clearPin(LED_GREEN_PORT,  LED_GREEN_PIN);
    gpio_clearPin(LED_YELLOW_PORT, LED_YELLOW_PIN);
    gpio_clearPin(LED_RED_PORT,    LED_RED_PIN);

    switch (level)
    {
        case WATER_LEVEL_HIGH:
            gpio_setPin(LED_GREEN_PORT,  LED_GREEN_PIN);
            break;
        case WATER_LEVEL_MID:
            gpio_setPin(LED_YELLOW_PORT, LED_YELLOW_PIN);
            break;
        case WATER_LEVEL_LOW:
            gpio_setPin(LED_RED_PORT,    LED_RED_PIN);
            break;
        case WATER_LEVEL_UNKNOWN:
        default:
            /* Sin lectura valida del ultrasonico: prender rojo por seguridad */
            gpio_setPin(LED_RED_PORT,    LED_RED_PIN);
            break;
    }
}

/*
 * Turns off all three semaphore LEDs. Used on global stop, where the
 * system is intentionally halted and should not show a sensor alert.
 */
static void prv_semaphoreOff(void)
{
    gpio_clearPin(LED_GREEN_PORT,  LED_GREEN_PIN);
    gpio_clearPin(LED_YELLOW_PORT, LED_YELLOW_PIN);
    gpio_clearPin(LED_RED_PORT,    LED_RED_PIN);
}

/* ------------------------------------------------------------------ */
/*  LCD row building                                  */
/* ------------------------------------------------------------------ */

/*
 * Section row 0 - exactly 16 chars:
 * "SecA: 65% ABT   " normal
 * "SecA: --% STOP  " stopped
 */
static void prv_buildSectionRow0(char *buf, char sec,
                                 uint8_t hum, const char *valve,
                                 uint8_t stopped)
{
    uint8_t pos = 0U;
    buf[pos++] = 'S'; buf[pos++] = 'e'; buf[pos++] = 'c';
    buf[pos++] = sec; buf[pos++] = ':'; buf[pos++] = ' ';

    if (stopped)
    {
        buf[pos++] = '-'; buf[pos++] = '-'; buf[pos++] = '%';
        buf[pos++] = ' ';
        buf[pos++] = 'S'; buf[pos++] = 'T'; buf[pos++] = 'O';
        buf[pos++] = 'P'; buf[pos++] = ' '; buf[pos++] = ' ';
    }
    else
    {
        if      (hum < 10U)  { buf[pos++] = ' '; buf[pos++] = ' '; }
        else if (hum < 100U) { buf[pos++] = ' '; }
        buf[pos++] = (hum >= 100U) ? '1' : '0' + (char)((hum % 100U) / 10U);
        if (hum < 10U) { pos--; buf[pos++] = '0' + (char)(hum % 10U); }
        else            { buf[pos++] = '0' + (char)(hum % 10U); }
        buf[pos++] = '%'; buf[pos++] = ' ';
        buf[pos++] = valve[0]; buf[pos++] = valve[1]; buf[pos++] = valve[2];
        while (pos < 16U) { buf[pos++] = ' '; }
    }
    buf[pos] = '\0';
}

/*
 * Flow row 1 - exactly 16 chars:
 * "Flow: X.X L/m  " normal
 * "Flow: -- L/m   " stopped
 */
static void prv_buildFlowRow1(char *buf, float flow, uint8_t stopped)
{
    uint8_t pos = 0U;
    buf[pos++] = 'F'; buf[pos++] = 'l'; buf[pos++] = 'u';
    buf[pos++] = 'j'; buf[pos++] = 'o'; buf[pos++] = ':';
    buf[pos++] = ' ';

    if (stopped)
    {
        buf[pos++] = '-'; buf[pos++] = '-';
        buf[pos++] = ' '; buf[pos++] = 'L';
        buf[pos++] = '/'; buf[pos++] = 'm';
    }
    else
    {
        uint32_t i = (uint32_t)flow;
        uint32_t d = (uint32_t)((flow - (float)i) * 10.0f);
        buf[pos++] = (i >= 10U) ? (char)('0' + i / 10U) : ' ';
        buf[pos++] = (char)('0' + i % 10U);
        buf[pos++] = '.';
        buf[pos++] = (char)('0' + d % 10U);
        buf[pos++] = ' ';
        buf[pos++] = 'L'; buf[pos++] = '/'; buf[pos++] = 'm';
    }

    while (pos < 16U) { buf[pos++] = ' '; }
    buf[pos] = '\0';
}

/*
 * Tank row 0 - exactly 16 chars:
 * "Dep: 38cm AMR   "
 * "Dep: ---  ---   " no reading
 */
static void prv_buildDepositoRow0(char *buf, uint32_t dist_cm,
                                  WaterLevel_t level)
{
    uint8_t pos = 0U;
    buf[pos++] = 'D'; buf[pos++] = 'e'; buf[pos++] = 'p';
    buf[pos++] = ':'; buf[pos++] = ' ';

    if (dist_cm == 0U)
    {
        buf[pos++] = '-'; buf[pos++] = '-'; buf[pos++] = '-';
        buf[pos++] = ' '; buf[pos++] = ' ';
        buf[pos++] = '-'; buf[pos++] = '-'; buf[pos++] = '-';
    }
    else
    {
        if      (dist_cm < 10U)  { buf[pos++] = ' '; buf[pos++] = ' '; }
        else if (dist_cm < 100U) { buf[pos++] = ' '; }
        if (dist_cm >= 100U) { buf[pos++] = (char)('0' + dist_cm / 100U); }
        buf[pos++] = (char)('0' + (dist_cm % 100U) / 10U);
        buf[pos++] = (char)('0' + dist_cm % 10U);
        buf[pos++] = 'c'; buf[pos++] = 'm'; buf[pos++] = ' ';

        switch (level)
        {
            case WATER_LEVEL_HIGH:
                buf[pos++] = 'V'; buf[pos++] = 'R'; buf[pos++] = 'D'; break;
            case WATER_LEVEL_MID:
                buf[pos++] = 'A'; buf[pos++] = 'M'; buf[pos++] = 'R'; break;
            case WATER_LEVEL_LOW:
                buf[pos++] = 'R'; buf[pos++] = 'J'; buf[pos++] = 'O'; break;
            default:
                buf[pos++] = '-'; buf[pos++] = '-'; buf[pos++] = '-'; break;
        }
    }

    while (pos < 16U) { buf[pos++] = ' '; }
    buf[pos] = '\0';
}

/*
 * Tank row 1 - exactly 16 chars:
 * "Pump: 75%      " active
 * "Pump: OFF      " off
 * "Pump: STOP     " stopped by button
 */
static void prv_buildDepositoRow1(char *buf, uint8_t pump_duty,
                                  uint8_t stopped_global)
{
    uint8_t pos = 0U;
    buf[pos++] = 'B'; buf[pos++] = 'o'; buf[pos++] = 'm';
    buf[pos++] = 'b'; buf[pos++] = 'a'; buf[pos++] = ':';
    buf[pos++] = ' ';

    if (stopped_global)
    {
        buf[pos++] = 'S'; buf[pos++] = 'T'; buf[pos++] = 'O'; buf[pos++] = 'P';
    }
    else if (pump_duty == 0U)
    {
        buf[pos++] = 'O'; buf[pos++] = 'F'; buf[pos++] = 'F';
    }
    else
    {
        if (pump_duty < 100U) { buf[pos++] = ' '; }
        buf[pos++] = (pump_duty >= 100U) ? '1' : (char)('0' + pump_duty / 10U);
        buf[pos++] = (char)('0' + pump_duty % 10U);
        buf[pos++] = '%';
    }

    while (pos < 16U) { buf[pos++] = ' '; }
    buf[pos] = '\0';
}

/* ------------------------------------------------------------------ */
/*  LCD update                                            */
/* ------------------------------------------------------------------ */

/*
 * Updates the LCD based on the pot zone.
 * Only rewrites if the view, a button, or data changes.
 */
static void prv_updateLcd(void)
{
    LCD_View_t view = lcd_viewFromPot(g_pot_raw);
    static LCD_View_t last_view = (LCD_View_t)0xFFU;

    if (view != last_view)
    {
        lcd_clear(LCD_ADDR);
        last_view = view;
    }

    char row0[17];
    char row1[17];

    switch (view)
    {
        case LCD_VIEW_SECTION_A:
        {
            uint8_t stopped = button_isStoppedGlobal() || button_isStoppedA();
            prv_buildSectionRow0(row0, 'A', g_hum_a,
                                 irrigation_getValveStateStr(SOIL_SECTION_A),
                                 stopped);
            prv_buildFlowRow1(row1, g_flow_a, stopped);
            lcd_setCursor(LCD_ADDR, 0U, 0U); lcd_writeString(LCD_ADDR, row0);
            lcd_setCursor(LCD_ADDR, 1U, 0U); lcd_writeString(LCD_ADDR, row1);
            break;
        }

        case LCD_VIEW_SECTION_B:
        {
            uint8_t stopped = button_isStoppedGlobal() || button_isStoppedB();
            prv_buildSectionRow0(row0, 'B', g_hum_b,
                                 irrigation_getValveStateStr(SOIL_SECTION_B),
                                 stopped);
            prv_buildFlowRow1(row1, g_flow_b, stopped);
            lcd_setCursor(LCD_ADDR, 0U, 0U); lcd_writeString(LCD_ADDR, row0);
            lcd_setCursor(LCD_ADDR, 1U, 0U); lcd_writeString(LCD_ADDR, row1);
            break;
        }

        case LCD_VIEW_DEPOSITO:
        {
            uint8_t stopped = button_isStoppedGlobal();
            prv_buildDepositoRow0(row0, g_dist_cm, g_level);
            prv_buildDepositoRow1(row1, g_pump_duty, stopped);
            lcd_setCursor(LCD_ADDR, 0U, 0U); lcd_writeString(LCD_ADDR, row0);
            lcd_setCursor(LCD_ADDR, 1U, 0U); lcd_writeString(LCD_ADDR, row1);
            break;
        }

        default:
            break;
    }
}

/* ------------------------------------------------------------------ */
/*  Button change detection                                    */
/* ------------------------------------------------------------------ */

/*
 * Detects any button change and prints to UART immediately.
 * Also forces an instant LCD rewrite.
 */
static void prv_checkButtons(void)
{
    static uint8_t prev_g = 0U;
    static uint8_t prev_a = 0U;
    static uint8_t prev_b = 0U;

    uint8_t cur_g = button_isStoppedGlobal();
    uint8_t cur_a = button_isStoppedA();
    uint8_t cur_b = button_isStoppedB();

    if (cur_g == prev_g && cur_a == prev_a && cur_b == prev_b) return;

    if (cur_g != prev_g)
    {
        uart_sendString(cur_g ? "[GLOBAL] Sistema DETENIDO\r\n"
                               : "[GLOBAL] Sistema REANUDADO\r\n");
        /* Turn off pump and semaphore immediately when global STOP is pressed */
        if (cur_g) { pump_stop(); g_pump_duty = 0U; prv_semaphoreOff(); }
    }
    if (cur_a != prev_a)
        uart_sendString(cur_a ? "[BTN-A]  Seccion A DETENIDA\r\n"
                               : "[BTN-A]  Seccion A REANUDADA\r\n");
    if (cur_b != prev_b)
        uart_sendString(cur_b ? "[BTN-B]  Seccion B DETENIDA\r\n"
                               : "[BTN-B]  Seccion B REANUDADA\r\n");

    prev_g = cur_g;
    prev_a = cur_a;
    prev_b = cur_b;

    prv_updateLcd();
}

/* ------------------------------------------------------------------ */
/*  System initialization                                         */
/* ------------------------------------------------------------------ */

static void prv_systemInit(void)
{
    gpio_init();
    systick_init();
    tim_init();
    timer_init();
    uart_init();
    i2c_init();
    lcd_init(LCD_ADDR);
    prv_potInit();

    /* Humidity sensors and valves */
    soilMoisture_init();
    irrigation_init();

    /* Pump - must be initialized before the ultrasonic
     * so PA5 is configured as PWM and not as GPIO */
    pump_init();

    /* Ultrasonic - initializes TIM5 internally and configures the LEDs */
    ultrasonic_init();

    /* Tank semaphore - pines ya definidos en ultrasonic.h */
    GPIO_PinCfg_t led_cfg;
    led_cfg.mode  = GPIO_MODE_OUTPUT;
    led_cfg.otype = GPIO_OTYPE_PUSH_PULL;
    led_cfg.speed = GPIO_SPEED_LOW;
    led_cfg.pull  = GPIO_PULL_NONE;

    gpio_initPort(LED_GREEN_PORT);
    gpio_setPinMode(LED_GREEN_PORT,  LED_GREEN_PIN,  &led_cfg);
    gpio_initPort(LED_YELLOW_PORT);
    gpio_setPinMode(LED_YELLOW_PORT, LED_YELLOW_PIN, &led_cfg);
    gpio_initPort(LED_RED_PORT);
    gpio_setPinMode(LED_RED_PORT,    LED_RED_PIN,    &led_cfg);

    prv_semaphoreOff();

    /* Buttons and flow sensors */
    button_init();
    flow_init();
}

static void prv_potInit(void)
{
    GPIO_PinCfg_t cfg;
    cfg.mode  = GPIO_MODE_ANALOG;
    cfg.pull  = GPIO_PULL_NONE;
    cfg.otype = GPIO_OTYPE_PUSH_PULL;
    cfg.speed = GPIO_SPEED_LOW;
    gpio_setPinMode(POT_GPIO_PORT, POT_GPIO_PIN, &cfg);

    adc_init(POT_ADC_INSTANCE);
    adc_enableAdc(POT_ADC_INSTANCE);
    adc_setChannel(POT_ADC_INSTANCE, POT_ADC_CHANNEL,
                   ADC_SAMPLETIME_480CYCLES);
}

static uint16_t prv_potRead(void)
{
    uint16_t val = 0U;
    adc_setChannel(POT_ADC_INSTANCE, POT_ADC_CHANNEL,
                   ADC_SAMPLETIME_480CYCLES);
    adc_startSingleConversion(POT_ADC_INSTANCE);
    adc_readData(POT_ADC_INSTANCE, &val);
    return val;
}

static void prv_lcdSplash(void)
{
    lcd_clear(LCD_ADDR);
    lcd_setCursor(LCD_ADDR, 0U, 0U);
    lcd_writeString(LCD_ADDR, " Sistema Riego  ");
    lcd_setCursor(LCD_ADDR, 1U, 0U);
    lcd_writeString(LCD_ADDR, "PB1 + PB2 + PB3 ");
    timer_delay_ms(2000U);
    lcd_clear(LCD_ADDR);
}