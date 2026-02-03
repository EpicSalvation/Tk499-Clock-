# LCD Rendering Guide

This document explains how the TKM32F499 clock application renders graphics and text to the 4.3" LCD display.

## Table of Contents

1. [Overview](#overview)
2. [Hardware Interface](#hardware-interface)
3. [The Display Protocol](#the-display-protocol)
4. [Drawing Primitives](#drawing-primitives)
5. [The Font System](#the-font-system)
6. [Scaling Text](#scaling-text)
7. [Drawing Lines and Rectangles](#drawing-lines-and-rectangles)
8. [Positioning and Layout](#positioning-and-layout)
9. [Complete Example](#complete-example)
10. [Color Reference](#color-reference)
11. [Backlight Control](#backlight-control)

---

## Overview

The LCD is an **800x480 pixel** display (TK043F1168 panel with HX8369-compatible controller). The TKM32F499 uses **24-bit RGB888 color** internally, with colors specified as `0x00RRGGBB`. The display connects via the **TK80** peripheral (a custom 8080-style parallel interface), not FSMC.

For compatibility with common color definitions, the API accepts RGB565 colors and converts them internally to RGB888.

### Coordinate System

```
(0,0) ─────────────────────────────────► X (480 pixels)
  │
  │
  │
  │
  │
  ▼
  Y (272 pixels)
```

- Origin `(0,0)` is the **top-left** corner
- X increases to the right (0-799)
- Y increases downward (0-479)

---

## Hardware Interface

The LCD connects via the **TK80** peripheral (a custom 8080-style parallel interface at `0x60000000`):

```c
/* TK80 LCD Controller Registers */
typedef struct {
    volatile uint32_t CR;       /* Control Register */
    volatile uint32_t CFGR1;    /* Configuration Register 1 */
    volatile uint32_t CFGR2;    /* Configuration Register 2 */
    volatile uint32_t SR;       /* Status Register */
    volatile uint32_t CMDIR;    /* Command Input Register */
    volatile uint32_t DINR;     /* Data Input Register */
    /* ... more registers ... */
    volatile uint32_t CFGR3;    /* Config Reg 3 (pixel count for block fill) */
} TK80_TypeDef;

#define TK80    ((TK80_TypeDef *)0x60000000)
```

### How It Works

The TK80 provides dedicated registers for LCD commands and data:

| Register | Purpose | Usage |
|----------|---------|-------|
| `TK80->CMDIR` | Command register | Send LCD commands (0x2A, 0x2B, 0x2C, etc.) |
| `TK80->DINR` | Data register | Send pixel data (24-bit RGB888) |
| `TK80->SR` | Status register | Check busy flag (bit 16) |
| `TK80->CFGR3` | Pixel count | For hardware-accelerated block fills |

The TK80 handles all timing and control signals automatically.

```c
void LCD_WriteCmd(uint16_t cmd)
{
    TK80->CMDIR = cmd;
    while (TK80->SR & 0x10000);  /* Wait for not busy */
}

void LCD_WriteData(uint16_t data)
{
    TK80->DINR = data;
    /* No wait needed for streaming pixel data */
}
```

---

## The Display Protocol

LCD controllers use a command/data protocol. The controller used here follows the common MIPI DCS (Display Command Set) standard.

### Setting the Drawing Window

Before drawing pixels, you must define the rectangular region to draw into:

```c
void LCD_SetWindow(uint16_t x1, uint16_t y1, uint16_t x2, uint16_t y2)
{
    // Command 0x2A: Set column (X) address range
    LCD_WriteCmd(0x2A);
    LCD_WriteData(x1 >> 8);      // Start X high byte
    LCD_WriteData(x1 & 0xFF);    // Start X low byte
    LCD_WriteData(x2 >> 8);      // End X high byte
    LCD_WriteData(x2 & 0xFF);    // End X low byte

    // Command 0x2B: Set row (Y) address range
    LCD_WriteCmd(0x2B);
    LCD_WriteData(y1 >> 8);      // Start Y high byte
    LCD_WriteData(y1 & 0xFF);    // Start Y low byte
    LCD_WriteData(y2 >> 8);      // End Y high byte
    LCD_WriteData(y2 & 0xFF);    // End Y low byte

    // Command 0x2C: Begin memory write
    LCD_WriteCmd(0x2C);
    // Now subsequent LCD_WriteData() calls write pixels
}
```

### Pixel Data Flow

After `LCD_SetWindow()`, each `LCD_WriteData()` call writes one pixel. The LCD controller automatically advances to the next position:

```
Window: (10,20) to (12,21)  →  A 3×2 pixel region

Write order:
┌─────┬─────┬─────┐
│  1  │  2  │  3  │  ← Row 20
├─────┼─────┼─────┤
│  4  │  5  │  6  │  ← Row 21
└─────┴─────┴─────┘
  ↑     ↑     ↑
 X=10  X=11  X=12

Pixels fill left-to-right, then wrap to next row.
```

---

## Drawing Primitives

### Drawing a Single Pixel

```c
void LCD_DrawPixel(uint16_t x, uint16_t y, uint16_t color)
{
    LCD_SetWindow(x, y, x, y);  // 1×1 window
    LCD_WriteData(color);        // Write single pixel
}
```

### Filling a Rectangle

For efficiency, fill a rectangle by setting a window and streaming pixels:

```c
void LCD_FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    LCD_SetWindow(x, y, x + w - 1, y + h - 1);

    uint32_t total = (uint32_t)w * h;
    for (uint32_t i = 0; i < total; i++) {
        LCD_WriteData(color);  // Same color for every pixel
    }
}
```

### Clearing the Screen

Clearing is just filling a screen-sized rectangle:

```c
void LCD_Clear(uint16_t color)
{
    LCD_SetWindow(0, 0, LCD_WIDTH - 1, LCD_HEIGHT - 1);

    for (uint32_t i = 0; i < LCD_WIDTH * LCD_HEIGHT; i++) {
        LCD_WriteData(color);
    }
}
```

---

## The Font System

Text rendering uses a **bitmap font** - each character is stored as a grid of bits indicating which pixels are "on."

### Font Storage Format

The font is 8 pixels wide × 16 pixels tall. Each character is stored as 16 bytes (one byte per row):

```c
static const uint8_t font8x16[][16] = {
    /* 'A' (ASCII 65, stored at index 65-32=33) */
    {
        0x00,  // Row 0:  ........
        0x00,  // Row 1:  ........
        0x10,  // Row 2:  ...#....
        0x38,  // Row 3:  ..###...
        0x6C,  // Row 4:  .##.##..
        0xC6,  // Row 5:  ##...##.
        0xC6,  // Row 6:  ##...##.
        0xFE,  // Row 7:  #######.
        0xC6,  // Row 8:  ##...##.
        0xC6,  // Row 9:  ##...##.
        0xC6,  // Row 10: ##...##.
        0xC6,  // Row 11: ##...##.
        0x00,  // Row 12: ........
        0x00,  // Row 13: ........
        0x00,  // Row 14: ........
        0x00   // Row 15: ........
    },
    // ... more characters
};
```

### Decoding a Byte into Pixels

Each byte represents 8 horizontal pixels. Bit 7 (MSB) is the leftmost pixel:

```
Byte: 0x38 = 0011 1000 binary

Bit position:  7  6  5  4  3  2  1  0
Bit value:     0  0  1  1  1  0  0  0
Pixel:         .  .  #  #  #  .  .  .
               ↑                    ↑
            Left                 Right
```

### The Character Drawing Function

```c
uint8_t LCD_DrawChar(uint16_t x, uint16_t y, char c, uint16_t fg, uint16_t bg)
{
    // Get pointer to this character's font data
    // ASCII 32 (space) is at index 0
    const uint8_t *glyph = font8x16[c - 32];

    // Draw 16 rows
    for (int row = 0; row < 16; row++) {
        uint8_t byte = glyph[row];

        // Draw 8 columns
        for (int col = 0; col < 8; col++) {
            // Check if this bit is set (pixel is "on")
            // 0x80 >> col creates masks: 10000000, 01000000, 00100000, ...
            if (byte & (0x80 >> col)) {
                LCD_DrawPixel(x + col, y + row, fg);  // Foreground
            } else {
                LCD_DrawPixel(x + col, y + row, bg);  // Background
            }
        }
    }

    return 8;  // Return character width for cursor advancement
}
```

### Drawing a String

```c
void LCD_DrawString(uint16_t x, uint16_t y, const char *str, uint16_t fg, uint16_t bg)
{
    while (*str) {
        x += LCD_DrawChar(x, y, *str, fg, bg);  // Draw char, advance x
        str++;
    }
}
```

---

## Scaling Text

For larger text (like the clock display), we scale the font by drawing each font pixel as a block of screen pixels.

### How Scaling Works

With `scale = 3`, each 1×1 font pixel becomes a 3×3 block:

```
Original 'A' (8×16)          Scaled 3× (24×48)

    ...#....                     .........###............
    ..###...          →          .........###............
    .##.##..                     .........###............
                                 ......#########.........
                                 ......#########.........
                                 ......#########.........
                                 ...######...######......
                                 ...######...######......
                                 ...######...######......
```

### Scaled Character Function

```c
uint8_t LCD_DrawCharLarge(uint16_t x, uint16_t y, char c,
                          uint16_t fg, uint16_t bg, uint8_t scale)
{
    const uint8_t *glyph = font8x16[c - 32];

    for (int row = 0; row < 16; row++) {
        uint8_t byte = glyph[row];

        for (int col = 0; col < 8; col++) {
            uint16_t color = (byte & (0x80 >> col)) ? fg : bg;

            // Draw a scale × scale block instead of a single pixel
            for (int sy = 0; sy < scale; sy++) {
                for (int sx = 0; sx < scale; sx++) {
                    LCD_DrawPixel(
                        x + col * scale + sx,
                        y + row * scale + sy,
                        color
                    );
                }
            }
        }
    }

    return 8 * scale;  // Scaled width
}
```

### Size Calculations

| Scale | Character Size | "12:00:00" Width | "12:00:00" Height |
|-------|---------------|------------------|-------------------|
| 1× | 8 × 16 | 64 px | 16 px |
| 2× | 16 × 32 | 128 px | 32 px |
| 4× | 32 × 64 | 256 px | 64 px |
| 6× | 48 × 96 | 384 px | 96 px |

The clock app uses **6× scaling** for the time display, which works well on the 800×480 screen.

---

## Drawing Lines and Rectangles

### Horizontal and Vertical Lines

Lines are drawn as thin rectangles:

```c
// Horizontal line: width × 1 rectangle
void draw_hline(uint16_t x, uint16_t y, uint16_t length, uint16_t color)
{
    LCD_FillRect(x, y, length, 1, color);
}

// Vertical line: 1 × height rectangle
void draw_vline(uint16_t x, uint16_t y, uint16_t length, uint16_t color)
{
    LCD_FillRect(x, y, 1, length, color);
}

// Thicker line: width × thickness rectangle
void draw_thick_hline(uint16_t x, uint16_t y, uint16_t length,
                      uint16_t thickness, uint16_t color)
{
    LCD_FillRect(x, y, length, thickness, color);
}
```

### Using Lines in the Clock App

```c
#define LINE_THICKNESS  2
#define LINE_MARGIN     60

// Draw separator line above the time
LCD_FillRect(LINE_MARGIN, LINE_ABOVE_Y,
             LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS,
             COLOR_WHITE);
```

This creates a horizontal line that:
- Starts at x = 60
- Has width = 480 - 120 = 360 pixels
- Is 2 pixels thick
- Is white colored

---

## Positioning and Layout

### Centering Elements

To center an element horizontally:

```c
x = (LCD_WIDTH - element_width) / 2;
```

To center vertically:

```c
y = (LCD_HEIGHT - element_height) / 2;
```

### Calculating Text Width

```c
uint16_t string_width(const char *str, uint8_t scale)
{
    uint16_t len = 0;
    while (*str++) len++;
    return len * 8 * scale;  // chars × char_width × scale
}
```

### Layout Constants in the Clock App

```c
#define BAR_HEIGHT      32      // Height of header/footer bars
#define BAR_TEXT_Y      8       // Vertical padding in bars
#define TIME_SCALE      6       // 6× scaling for time
#define TIME_CHARS      8       // "12:00:00"

// Calculate centered time position (800×480 display)
#define TIME_WIDTH   (TIME_CHARS * 8 * TIME_SCALE)    // 384 pixels
#define TIME_HEIGHT  (16 * TIME_SCALE)                 // 96 pixels
#define TIME_X       ((LCD_WIDTH - TIME_WIDTH) / 2)    // 208
#define TIME_Y       ((LCD_HEIGHT - TIME_HEIGHT) / 2)  // 192
```

### Text Alignment Examples

```c
// Left-aligned (with padding)
LCD_DrawString(4, y, "Left", fg, bg);

// Center-aligned
LCD_DrawString((LCD_WIDTH - string_width("Center", 1)) / 2, y, "Center", fg, bg);

// Right-aligned (with padding)
LCD_DrawString(LCD_WIDTH - string_width("Right", 1) - 4, y, "Right", fg, bg);
```

---

## Complete Example

Here's how the clock app screen is drawn (800×480 display):

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ TK499                      Clock Demo                                   v1.0 │ ← Blue bar (32px)
├──────────────────────────────────────────────────────────────────────────────┤
│                                                                              │
│                    ════════════════════════════════════                      │ ← White line
│                                                                              │
│                              12:00:00                                        │ ← Green time (6×)
│                                                                              │
│                    ════════════════════════════════════                      │ ← White line
│                                                                              │
├──────────────────────────────────────────────────────────────────────────────┤
│ Status: OK              TKM32F499 SmartBoard                          240MHz │ ← Orange bar (32px)
└──────────────────────────────────────────────────────────────────────────────┘
```

### Drawing Order

```c
// 1. Clear to black
LCD_Clear(COLOR_BLACK);

// 2. Draw top bar
LCD_FillRect(0, 0, LCD_WIDTH, BAR_HEIGHT, COLOR_BLUE);

// 3. Draw bottom bar
LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, LCD_WIDTH, BAR_HEIGHT, COLOR_ORANGE);

// 4. Draw text in top bar (left, center, right)
LCD_DrawString(4, BAR_TEXT_Y, "TK499", COLOR_WHITE, COLOR_BLUE);
LCD_DrawString((LCD_WIDTH - string_width("Clock Demo", 1)) / 2, BAR_TEXT_Y,
               "Clock Demo", COLOR_CYAN, COLOR_BLUE);
LCD_DrawString(LCD_WIDTH - string_width("v1.0", 1) - 4, BAR_TEXT_Y,
               "v1.0", COLOR_WHITE, COLOR_BLUE);

// 5. Draw text in bottom bar
LCD_DrawString(4, LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y,
               "Status: OK", COLOR_BLACK, COLOR_ORANGE);
// ... center and right text ...

// 6. Draw line above time
LCD_FillRect(LINE_MARGIN, LINE_ABOVE_Y,
             LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);

// 7. Draw centered time (6× scale)
LCD_DrawStringLarge(TIME_X, TIME_Y, "12:00:00", COLOR_GREEN, COLOR_BLACK, TIME_SCALE);

// 8. Draw line below time
LCD_FillRect(LINE_MARGIN, LINE_BELOW_Y,
             LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);
```

---

## Color Reference

The TK80 uses **RGB888** format internally (24 bits: 8 red, 8 green, 8 blue), but for convenience the API accepts **RGB565** format and converts automatically.

### RGB565 Colors (API accepts these)

| Color | Hex Value | Description |
|-------|-----------|-------------|
| Black | `0x0000` | No color |
| White | `0xFFFF` | Full brightness |
| Red | `0xF800` | Pure red |
| Green | `0x07E0` | Pure green |
| Blue | `0x001F` | Pure blue |
| Yellow | `0xFFE0` | Red + Green |
| Cyan | `0x07FF` | Green + Blue |
| Orange | `0xFD20` | Red + some Green |

### RGB888 Colors (hardware native)

| Color | Hex Value | Format |
|-------|-----------|--------|
| Black | `0x00000000` | 0x00RRGGBB |
| White | `0x00FFFFFF` | |
| Red | `0x00FF0000` | |
| Green | `0x0000FF00` | |
| Blue | `0x000000FF` | |

### Creating Custom Colors

```c
// RGB565: RRRRR GGGGGG BBBBB
#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

// Example: Create a purple color (red + blue)
#define COLOR_PURPLE RGB565(20, 0, 20)  // 0xA014

// RGB888 (native format)
#define RGB888(r, g, b) (((r & 0xFF) << 16) | ((g & 0xFF) << 8) | (b & 0xFF))
```

---

## Backlight Control

The LCD backlight is separate from the display controller. It's powered by an **MP3302** boost converter and controlled via **PD8**.

### Hardware Setup

```
PD8 (GPIO) ───► MP3302 (EN pin) ───► Backlight LEDs (LEDA/LEDK)
                  │
              Boost converter
              3.3V → ~20V
```

### Simple On/Off Control

For basic control, toggle PD8 directly:

```c
// Initialize (called automatically by LCD_Init)
void LCD_BacklightInit(void)
{
    // Enable GPIOD clock
    RCC->AHB1ENR |= (1 << 3);

    // Configure PD8 as push-pull output
    GPIOD->CRH &= ~(0x0F << 0);  // Clear bits [3:0]
    GPIOD->CRH |= (0x03 << 0);   // 50MHz push-pull

    LCD_BacklightOn();
}

void LCD_BacklightOn(void)
{
    GPIOD->BSRR = (1 << 8);      // Set PD8 high
}

void LCD_BacklightOff(void)
{
    GPIOD->BSRR = (1 << 24);     // Set PD8 low (reset)
}
```

### PWM Brightness Control

For variable brightness (0-100%), use software PWM via TIM3:

```c
// Initialize PWM brightness control
LCD_BrightnessInit();

// Set brightness level (0 = off, 100 = full)
LCD_SetBrightness(75);  // 75% brightness

// Get current level
uint8_t level = LCD_GetBrightness();
```

### How PWM Dimming Works

The backlight LEDs respond to average voltage. PWM rapidly switches the enable signal on and off - the LED integrates this into a perceived brightness:

```
100% brightness:  ████████████████  (always on)
 75% brightness:  ████████████      (on 75% of cycle)
 50% brightness:  ████████          (on 50% of cycle)
 25% brightness:  ████              (on 25% of cycle)
  0% brightness:                    (always off)
```

### PWM Implementation Details

TIM3 generates interrupts at 100kHz (100 ticks per 1kHz PWM cycle):

```c
/* Timer interrupt handler - called 100,000 times/second */
void TIM3_IRQHandler(void)
{
    TIM3->SR &= ~(1 << 0);  // Clear interrupt flag

    pwm_counter++;
    if (pwm_counter >= 100) pwm_counter = 0;

    // Compare counter against brightness threshold
    if (pwm_counter < pwm_brightness) {
        GPIOD->BSRR = (1 << 8);   // On
    } else {
        GPIOD->BSRR = (1 << 24);  // Off
    }
}
```

**PWM Parameters:**
- **Frequency:** 1kHz (no visible flicker)
- **Resolution:** 100 levels (0-100%)
- **Timer clock:** 120MHz (APB1 × 2)
- **Prescaler:** 12 (10MHz tick)
- **Period:** 100 ticks (100kHz interrupt rate)

### Usage Examples

```c
// Fade in effect
for (int i = 0; i <= 100; i++) {
    LCD_SetBrightness(i);
    delay(20000);  // ~20ms per step = 2 second fade
}

// Dim for night mode
LCD_SetBrightness(20);

// Full brightness for daytime
LCD_SetBrightness(100);

// Screen off (save power)
LCD_SetBrightness(0);
```

### Choosing Control Method

| Method | Pros | Cons |
|--------|------|------|
| `LCD_BacklightOn/Off` | Simple, no timer needed | Only on/off, no dimming |
| `LCD_SetBrightness` | Variable brightness | Uses TIM3, interrupt overhead |

Use simple on/off if you don't need dimming. Use PWM brightness for:
- Night/day modes
- Power saving
- Fade effects
- User preference settings
