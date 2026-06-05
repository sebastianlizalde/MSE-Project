/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
******************************************************************************/
/**
* @file lcd.h
* @brief LCD driver — single HD44780 display via PCF8574 I2C adapter (0x27).
*
* The potentiometer on PA1 selects which view to show:
*
*   Zone A (0    - 1364)  -> Section A view  (section A status)
*   Zone B (1365 - 2729)  -> Section B view  (section B status)
*   Zone D (2730 - 4095)  -> Tank view       (water level + pump)
*
* Wiring:
*   PB8 -> SCL | PB9 -> SDA | VCC -> 5V | GND -> GND
*   External pull-ups 4.7k to 3.3V on SDA and SCL.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date May 2026
*/

#ifndef __LCD_H__
#define __LCD_H__

/*** Includes ***/
#include "i2c_driver.h"
#include <stdint.h>

/*** Preprocessor Definitions ***/

/* I2C address of the LCD */
#define LCD_ADDR            0x27U
#define LCD_ADDR_SECCIONES  0x27U   /* compatibility alias for irrigation.c */
#define LCD_ADDR_DEPOSITO   0x27U   /* compatibility alias for main.c        */

/* Display dimensions */
#define LCD_COLS            16U
#define LCD_ROWS             2U

/* PCF8574 adapter bit mapping */
#define LCD_RS              (1U << 0U)
#define LCD_RW              (1U << 1U)
#define LCD_EN              (1U << 2U)
#define LCD_BL              (1U << 3U)

/* Splash screen duration (ms) */
#define LCD_SPLASH_MS       2000U

/*
 * Potentiometer zones (12-bit ADC: 0 to 4095)
 * Divided into three equal zones of ~1365 counts each.
 *
 *   0    - 1364  -> Section A
 *   1365 - 2729  -> Section B
 *   2730 - 4095  -> Tank / Deposit
 */
#define LCD_POT_ZONE_A_MAX  1364U
#define LCD_POT_ZONE_B_MAX  2729U

/* Active view identifier */
typedef enum
{
    LCD_VIEW_SECTION_A = 0U,
    LCD_VIEW_SECTION_B = 1U,
    LCD_VIEW_DEPOSITO  = 2U
} LCD_View_t;

/*** Function Prototypes ***/

/* Initializes the display in 4-bit mode */
void lcd_init(uint8_t addr);

/* Clears the entire display */
void lcd_clear(uint8_t addr);

/* Positions the cursor (row 0-1, col 0-15) */
void lcd_setCursor(uint8_t addr, uint8_t row, uint8_t col);

/* Writes a text string at the current cursor position */
void lcd_writeString(uint8_t addr, const char *str);

/* Writes an unsigned integer at the current cursor position */
void lcd_writeNumber(uint8_t addr, uint32_t num);

/* Converts ADC value (0-4095) to LCD_View_t */
LCD_View_t lcd_viewFromPot(uint16_t pot_raw);

/*
 * Updates the LCD based on the pot zone and button states.
 * Only rewrites when the view or any button changes (prevents flickering).
 *
 * View A -> row 0: "SecA: ACTIVE   "
 *           row 1: "SecB: ACTIVE   "
 *
 * View B -> row 0: "SecB: ACTIVE   "
 *           row 1: "               "
 *
 * View D -> row 0: "Deposit: ACTIVE"
 *           row 1: "               "
 *
 * When any stop button is active the text changes to STOP.
 */
void lcd_updatePB1(uint16_t pot_raw,
                   uint8_t  stopped_a,
                   uint8_t  stopped_b,
                   uint8_t  stopped_global);

#endif /* __LCD_H__ */
