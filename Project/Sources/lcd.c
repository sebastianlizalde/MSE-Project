/******************************************************************************
* Copyright (C) 2026 by Carlos Villarreal - CETYS Universidad
******************************************************************************/
/**
* @file lcd.c
* @brief LCD driver for PB1 — buttons + potentiometer.
*
* Implements the low-level layer for the HD44780 via PCF8574 I2C,
* and the pot-based view selection logic for PB1.
*
* @author Oskar Garcia, Sebastian Lizalde, Luis Lugo, Carlos Araiza
* @date May 2026
*/

/*** Includes ***/
#include "lcd.h"
#include "systick.h"

/*** Preprocessor Definitions ***/
#define LCD_CMD_CLEAR       0x01U
#define LCD_CMD_HOME        0x02U
#define LCD_CMD_ENTRY       0x06U
#define LCD_CMD_DISPLAY_ON  0x0CU
#define LCD_CMD_4BIT_2LINE  0x28U
#define LCD_CMD_ROW0        0x80U
#define LCD_CMD_ROW1        0xC0U

/*** Local Variables ***/
static uint8_t    s_backlight = LCD_BL;
static LCD_View_t s_last_view = (LCD_View_t)0xFFU; /* fuerza primer update */

/*** Private Function Prototypes ***/
static void prv_pcfWrite(uint8_t addr, uint8_t data);
static void prv_pulseEn (uint8_t addr, uint8_t data);
static void prv_nibble  (uint8_t addr, uint8_t nibble, uint8_t rs);
static void prv_byte    (uint8_t addr, uint8_t byte,   uint8_t rs);
static void prv_cmd     (uint8_t addr, uint8_t c);
static void prv_dat     (uint8_t addr, uint8_t d);
static void prv_str     (uint8_t addr, const char *s);
static void prv_pad16   (uint8_t addr, uint8_t used);
static void prv_rowSection(uint8_t addr, uint8_t row,
                            char sec, uint8_t stopped);

/*** Private: I2C / LCD low-level ***/

static void prv_pcfWrite(uint8_t addr, uint8_t data)
{
    i2c_start();
    i2c_sendAddress(addr, 0U);
    i2c_writeByte(data);
    i2c_stop();
    uint32_t t = systick_getTick();
    while ((systick_getTick() - t) < 1U) { }
}

static void prv_pulseEn(uint8_t addr, uint8_t data)
{
    prv_pcfWrite(addr, data |  LCD_EN);
    prv_pcfWrite(addr, data & ~LCD_EN);
}

static void prv_nibble(uint8_t addr, uint8_t nibble, uint8_t rs)
{
    uint8_t data = (uint8_t)((nibble << 4U) | s_backlight | rs);
    prv_pulseEn(addr, data);
}

static void prv_byte(uint8_t addr, uint8_t byte, uint8_t rs)
{
    prv_nibble(addr, (byte >> 4U) & 0x0FU, rs);
    prv_nibble(addr,  byte        & 0x0FU, rs);
}

static void prv_cmd(uint8_t addr, uint8_t c) { prv_byte(addr, c, 0U);     }
static void prv_dat(uint8_t addr, uint8_t d) { prv_byte(addr, d, LCD_RS); }

static void prv_str(uint8_t addr, const char *s)
{
    while (*s != '\0') { prv_dat(addr, (uint8_t)*s++); }
}

/* Pads with spaces up to 16 columns */
static void prv_pad16(uint8_t addr, uint8_t used)
{
    while (used < LCD_COLS) { prv_dat(addr, ' '); used++; }
}

/*** Public: basic API ***/

void lcd_init(uint8_t addr)
{
    uint32_t t = systick_getTick();
    while ((systick_getTick() - t) < 50U) { }

    prv_nibble(addr, 0x03U, 0U);
    t = systick_getTick(); while ((systick_getTick() - t) < 5U) { }
    prv_nibble(addr, 0x03U, 0U);
    t = systick_getTick(); while ((systick_getTick() - t) < 1U) { }
    prv_nibble(addr, 0x03U, 0U);
    t = systick_getTick(); while ((systick_getTick() - t) < 1U) { }

    prv_nibble(addr, 0x02U, 0U);
    t = systick_getTick(); while ((systick_getTick() - t) < 1U) { }

    prv_cmd(addr, LCD_CMD_4BIT_2LINE);
    prv_cmd(addr, LCD_CMD_DISPLAY_ON);
    prv_cmd(addr, LCD_CMD_CLEAR);
    t = systick_getTick(); while ((systick_getTick() - t) < 2U) { }
    prv_cmd(addr, LCD_CMD_ENTRY);

    s_last_view = (LCD_View_t)0xFFU;
}

void lcd_clear(uint8_t addr)
{
    prv_cmd(addr, LCD_CMD_CLEAR);
    uint32_t t = systick_getTick();
    while ((systick_getTick() - t) < 2U) { }
}

void lcd_setCursor(uint8_t addr, uint8_t row, uint8_t col)
{
    uint8_t ddram = (row == 0U) ? (uint8_t)(LCD_CMD_ROW0 + col)
                                : (uint8_t)(LCD_CMD_ROW1 + col);
    prv_cmd(addr, ddram);
}

void lcd_writeString(uint8_t addr, const char *str)
{
    while (*str != '\0') { prv_dat(addr, (uint8_t)*str++); }
}

void lcd_writeNumber(uint8_t addr, uint32_t num)
{
    if (num == 0U) { prv_dat(addr, '0'); return; }
    char   buf[10];
    int8_t i = 0;
    while (num > 0U) { buf[i++] = '0' + (char)(num % 10U); num /= 10U; }
    while (--i >= 0) { prv_dat(addr, (uint8_t)buf[i]); }
}

/*** Public: pot-based view selection logic ***/

/*
 * Converts the ADC value (0-4095) to the corresponding view.
 *
 *   0    - 1364  -> LCD_VIEW_SECTION_A
 *   1365 - 2729  -> LCD_VIEW_SECTION_B
 *   2730 - 4095  -> LCD_VIEW_DEPOSITO
 */
LCD_View_t lcd_viewFromPot(uint16_t pot_raw)
{
    if (pot_raw <= LCD_POT_ZONE_A_MAX) return LCD_VIEW_SECTION_A;
    if (pot_raw <= LCD_POT_ZONE_B_MAX) return LCD_VIEW_SECTION_B;
    return LCD_VIEW_DEPOSITO;
}

/*
 * Writes a row with the status of a section.
 * Format (exactly 16 chars):
 *   "SecA: ACTIVE   " or "SecA: STOP     "
 */
static void prv_rowSection(uint8_t addr, uint8_t row,
                            char sec, uint8_t stopped)
{
    lcd_setCursor(addr, row, 0U);
    prv_dat(addr, 'S');
    prv_dat(addr, 'e');
    prv_dat(addr, 'c');
    prv_dat(addr, (uint8_t)sec);
    prv_dat(addr, ':');
    prv_dat(addr, ' ');                       /* 6 chars usados */

    if (stopped)
    {
        prv_str(addr, "STOP  ");              /* 6 chars */
        prv_pad16(addr, 12U);
    }
    else
    {
        prv_str(addr, "ACTIVA");              /* 6 chars */
        prv_pad16(addr, 12U);
    }
}

/*
 * Updates the LCD based on the pot view and stop button states.
 * Only rewrites when the view or a button changes — prevents flickering.
 */
void lcd_updatePB1(uint16_t pot_raw,
                   uint8_t  stopped_a,
                   uint8_t  stopped_b,
                   uint8_t  stopped_global)
{
    LCD_View_t view = lcd_viewFromPot(pot_raw);

    /* Effective state of each section considering the global stop */
    uint8_t eff_a = stopped_a || stopped_global;
    uint8_t eff_b = stopped_b || stopped_global;

    /* Static variables to detect changes and avoid unnecessary rewrites */
    static uint8_t    s_last_eff_a = 0xFFU;
    static uint8_t    s_last_eff_b = 0xFFU;

    uint8_t view_changed = (view    != s_last_view);
    uint8_t btn_changed  = (eff_a   != s_last_eff_a) ||
                           (eff_b   != s_last_eff_b);

    /* If nothing changed, do not touch the LCD */
    if (!view_changed && !btn_changed) return;

    /* Clear screen only when the view changes to avoid glitches */
    if (view_changed)
    {
        lcd_clear(LCD_ADDR);
        s_last_view = view;
    }

    s_last_eff_a = eff_a;
    s_last_eff_b = eff_b;

    switch (view)
    {
        case LCD_VIEW_SECTION_A:
            /*
             * Left — Section A only
             * Row 0: Sec A status
             * Row 1: empty
             */
            prv_rowSection(LCD_ADDR, 0U, 'A', eff_a);
            lcd_setCursor(LCD_ADDR, 1U, 0U);
            prv_pad16(LCD_ADDR, 0U);
            break;

        case LCD_VIEW_SECTION_B:
            /*
             * Center — Section B only
             * Row 0: Sec B status
             * Row 1: empty
             */
            prv_rowSection(LCD_ADDR, 0U, 'B', eff_b);
            lcd_setCursor(LCD_ADDR, 1U, 0U);
            prv_pad16(LCD_ADDR, 0U);
            break;

        case LCD_VIEW_DEPOSITO:
            /*
             * PB4 not yet connected — show placeholder
             * Row 0: "Deposit: -----"
             * Row 1: global status
             */
            lcd_setCursor(LCD_ADDR, 0U, 0U);
            prv_str(LCD_ADDR, "Deposito:       ");
            lcd_setCursor(LCD_ADDR, 1U, 0U);
            if (stopped_global)
                prv_str(LCD_ADDR, "Sistema: STOP   ");
            else
                prv_str(LCD_ADDR, "Sistema: ACTIVO ");
            break;

        default:
            break;
    }
}