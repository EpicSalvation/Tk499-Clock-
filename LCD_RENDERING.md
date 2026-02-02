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

---

## Overview

The LCD is a 480×272 pixel display using 16-bit RGB565 color format. Each pixel requires 2 bytes, giving 65,536 possible colors. The display connects to the microcontroller via the FSMC (Flexible Static Memory Controller), which makes LCD access look like simple memory read/write operations.

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
- X increases to the right (0-479)
- Y increases downward (0-271)

---

## Hardware Interface

The LCD connects via **FSMC** (Flexible Static Memory Controller), which maps the LCD to memory addresses:

```c
#define LCD_BASE   ((uint32_t)0x60000000)
#define LCD_CMD    (*(volatile uint16_t *)(LCD_BASE))           // Commands
#define LCD_DATA   (*(volatile uint16_t *)(LCD_BASE + (1<<19))) // Data
```

### How It Works

The FSMC makes the LCD appear as two memory locations:

| Address | Purpose | Usage |
|---------|---------|-------|
| `0x60000000` | Command register | Tell LCD what operation to perform |
| `0x60080000` | Data register | Send pixel data or parameters |

The difference is controlled by address line **A18**:
- A18 = 0 → Command mode
- A18 = 1 → Data mode

Writing to these addresses triggers the appropriate control signals (chip select, read/write strobe, etc.) automatically.

```c
void LCD_WriteCmd(uint16_t cmd)
{
    LCD_CMD = cmd;    // Write to 0x60000000
}

void LCD_WriteData(uint16_t data)
{
    LCD_DATA = data;  // Write to 0x60080000
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
| 3× | 24 × 48 | 192 px | 48 px |
| 4× | 32 × 64 | 256 px | 64 px |

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
#define BAR_HEIGHT      24      // Height of header/footer bars
#define BAR_TEXT_Y      4       // Vertical padding in bars
#define TIME_SCALE      4       // 4× scaling for time
#define TIME_CHARS      8       // "12:00:00"

// Calculate centered time position
#define TIME_WIDTH   (TIME_CHARS * 8 * TIME_SCALE)    // 256 pixels
#define TIME_HEIGHT  (16 * TIME_SCALE)                 // 64 pixels
#define TIME_X       ((LCD_WIDTH - TIME_WIDTH) / 2)    // 112
#define TIME_Y       ((LCD_HEIGHT - TIME_HEIGHT) / 2)  // 104
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

Here's how the clock app screen is drawn:

```
┌────────────────────────────────────────────────────────────────┐
│ TK499            Clock Demo                              v1.0  │ ← Blue bar (24px)
├────────────────────────────────────────────────────────────────┤
│                                                                │
│                 ═══════════════════════                        │ ← White line
│                                                                │
│                       12:00:00                                 │ ← Green time (4×)
│                                                                │
│                 ═══════════════════════                        │ ← White line
│                                                                │
├────────────────────────────────────────────────────────────────┤
│ Status: OK     TKM32F499 SmartBoard                    240MHz  │ ← Orange bar (24px)
└────────────────────────────────────────────────────────────────┘
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

// 7. Draw centered time
LCD_DrawStringLarge(TIME_X, TIME_Y, "12:00:00", COLOR_GREEN, COLOR_BLACK, TIME_SCALE);

// 8. Draw line below time
LCD_FillRect(LINE_MARGIN, LINE_BELOW_Y,
             LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);
```

---

## Color Reference

Colors use **RGB565** format (16 bits: 5 red, 6 green, 5 blue):

| Color | Hex Value | RGB565 Bits |
|-------|-----------|-------------|
| Black | `0x0000` | R=0, G=0, B=0 |
| White | `0xFFFF` | R=31, G=63, B=31 |
| Red | `0xF800` | R=31, G=0, B=0 |
| Green | `0x07E0` | R=0, G=63, B=0 |
| Blue | `0x001F` | R=0, G=0, B=31 |
| Yellow | `0xFFE0` | R=31, G=63, B=0 |
| Cyan | `0x07FF` | R=0, G=63, B=31 |
| Orange | `0xFD20` | R=31, G=41, B=0 |

### Creating Custom Colors

```c
// RGB565: RRRRR GGGGGG BBBBB
#define RGB565(r, g, b) (((r & 0x1F) << 11) | ((g & 0x3F) << 5) | (b & 0x1F))

// Example: Create a purple color (red + blue)
#define COLOR_PURPLE RGB565(20, 0, 20)  // 0xA014
```
