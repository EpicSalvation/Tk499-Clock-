/**
 * TKM32F499 LCD Driver Header
 *
 * Driver for the 4.3" LCD on the TK499 SmartBoard.
 * Uses parallel 8080-style interface via FSMC.
 */

#ifndef __LCD_H
#define __LCD_H

#include <stdint.h>

/* LCD Dimensions - TK043F1168 is 800x480 in landscape mode */
#define LCD_WIDTH   800
#define LCD_HEIGHT  480

/* Common 16-bit RGB565 Colors */
#define COLOR_BLACK     0x0000
#define COLOR_WHITE     0xFFFF
#define COLOR_RED       0xF800
#define COLOR_GREEN     0x07E0
#define COLOR_BLUE      0x001F
#define COLOR_YELLOW    0xFFE0
#define COLOR_CYAN      0x07FF
#define COLOR_MAGENTA   0xF81F
#define COLOR_GRAY      0x8410
#define COLOR_ORANGE    0xFD20

/* LCD is connected via TK80 parallel interface peripheral */
/* See tkm32f499.h for TK80 register definitions */

/* Function Prototypes */

/**
 * Initialize the LCD controller and display
 */
void LCD_Init(void);

/**
 * Set the drawing window/region
 */
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2);

/**
 * Clear the entire screen with a color
 */
void LCD_Clear(uint16_t color);

/**
 * Draw a single pixel
 */
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color);

/**
 * Fill a rectangular area with a color
 */
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);

/**
 * Draw a character at position (x, y)
 * Returns the width of the character drawn
 */
uint8_t LCD_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);

/**
 * Draw a string at position (x, y)
 */
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

/**
 * Draw a large character (scaled 2x) for clock display
 */
uint8_t LCD_DrawCharLarge(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * Draw a string with scaling
 */
void LCD_DrawStringLarge(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg, uint8_t scale);

/**
 * Draw a single clock digit (48x64 pixels)
 * Supports '0'-'9' and ':'
 * @return width of character drawn (48 for digits, 24 for colon)
 */
uint8_t LCD_DrawClockDigit(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg);

/**
 * Draw a clock time string using the large 48x64 font
 * Supports digits 0-9 and colon only
 * Example: LCD_DrawClockTime(x, y, "12:00:00", COLOR_GREEN, COLOR_BLACK);
 */
void LCD_DrawClockTime(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg);

/**
 * Write a command to the LCD
 */
void LCD_WriteCmd(uint16_t cmd);

/**
 * Write data to the LCD
 */
void LCD_WriteData(uint16_t data);

/**
 * Read data from the LCD
 */
uint16_t LCD_ReadData(void);

/**
 * Initialize backlight control (PD8)
 * Called automatically by LCD_Init()
 */
void LCD_BacklightInit(void);

/**
 * Turn backlight on
 */
void LCD_BacklightOn(void);

/**
 * Turn backlight off
 */
void LCD_BacklightOff(void);

/**
 * Set backlight state (1 = on, 0 = off)
 */
void LCD_Backlight(uint8_t on);

/**
 * Initialize PWM-based brightness control
 * Uses TIM3 to generate software PWM on PD8
 * Call this instead of LCD_BacklightInit() for variable brightness
 */
void LCD_BrightnessInit(void);

/**
 * Set backlight brightness level
 * @param level Brightness from 0 (off) to 100 (full brightness)
 */
void LCD_SetBrightness(uint8_t level);

/**
 * Get current brightness level
 * @return Current brightness (0-100)
 */
uint8_t LCD_GetBrightness(void);

#endif /* __LCD_H */
