/**
 * TKM32F499 Clock Application
 *
 * Displays text on the 4.3" LCD screen.
 * Build with: make clock
 */

#include <stdint.h>
#include "tkm32f499.h"
#include "lcd.h"

/* Core registers */
#define SCB_VTOR    (*(volatile uint32_t *)0xE000ED08)
#define NVIC_ICER   ((volatile uint32_t *)0xE000E180)

/* Memory addresses */
#define T_SRAM_BASE  0x20000000
#define T_SDRAM_BASE 0x70020000

/* Remap vector table - CRITICAL for code to run */
static void RemapVtorTable(void)
{
    int i;

    /* Enable internal SRAM clock (bit 13) */
    RCC->AHB1ENR |= (1 << 13);

    /* Disable all interrupts */
    for (i = 0; i < 3; i++) {
        NVIC_ICER[i] = 0xFFFFFFFF;
    }

    /* Set VTOR to internal SRAM with bit 29 */
    SCB_VTOR = 0;
    SCB_VTOR |= (1 << 29);

    /* Copy vector table from SDRAM to SRAM */
    for (i = 0; i < 512; i += 4) {
        *(volatile uint32_t *)(T_SRAM_BASE + i) = *(volatile uint32_t *)(T_SDRAM_BASE + i);
    }
}

/* Simple delay */
static void delay(volatile uint32_t count)
{
    while (count--) {
        __asm volatile ("nop");
    }
}

/*
 * Layout constants
 *
 * Screen: 800 x 480 pixels
 * Font: 8x16 pixels (width x height)
 */
#define BAR_HEIGHT      32      /* Height of top/bottom colored bars */
#define BAR_TEXT_Y      8       /* Y offset for text within bars (centers 16px text in 32px bar) */
#define LINE_THICKNESS  3       /* Thickness of separator lines */

/*
 * Time display using scaled 8x16 font
 * Scale factor 6 = 48x96 pixels per character
 */
#define TIME_SCALE      6       /* Scale factor for time display */
#define TIME_CHARS      8       /* "12:00:00" = 8 characters */

/* Calculate centered time position */
#define TIME_WIDTH      (TIME_CHARS * 8 * TIME_SCALE)   /* 8 chars * 8px * 6 = 384 */
#define TIME_HEIGHT     (16 * TIME_SCALE)               /* 16px * 6 = 96 */
#define TIME_X          ((LCD_WIDTH - TIME_WIDTH) / 2)  /* (800 - 384) / 2 = 208 */
#define TIME_Y          ((LCD_HEIGHT - TIME_HEIGHT) / 2) /* (480 - 96) / 2 = 192 */

/* Line positions (above and below centered time) */
#define LINE_ABOVE_Y    (TIME_Y - 16)
#define LINE_BELOW_Y    (TIME_Y + TIME_HEIGHT + 12)
#define LINE_MARGIN     120     /* Horizontal margin for lines */

/* Helper to calculate string width */
static uint16_t string_width(const char *str, uint8_t scale)
{
    uint16_t len = 0;
    while (*str++) len++;
    return len * 8 * scale;
}

int main(void)
{
    /* CRITICAL: Remap vector table first */
    RemapVtorTable();

    /* Enable GPIO clocks */
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
    delay(1000);

    /* Configure PA8 as output for LED - do this early for debugging */
    GPIOA->CRH &= ~(0x0F << 0);
    GPIOA->CRH |= (0x03 << 0);

    /* Debug: 2 quick blinks to show we got this far */
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);

    /* Initialize the LCD */
    LCD_Init();

    /* Initialize PWM brightness control (0-100%) */
    LCD_BrightnessInit();
    LCD_SetBrightness(100);

    /* Debug: 3 quick blinks to show LCD_Init completed */
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);

    /* Clear screen to black background */
    LCD_Clear(COLOR_BLACK);

    /*
     * Draw top bar (blue)
     * Full width, BAR_HEIGHT pixels tall at top of screen
     */
    LCD_FillRect(0, 0, LCD_WIDTH, BAR_HEIGHT, COLOR_BLUE);

    /*
     * Draw bottom bar (orange)
     * Full width, BAR_HEIGHT pixels tall at bottom of screen
     */
    LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, LCD_WIDTH, BAR_HEIGHT, COLOR_ORANGE);

    /*
     * Draw text in top bar (blue background)
     * - Top left corner
     * - Top center
     * - Top right corner
     */
    LCD_DrawString(4, BAR_TEXT_Y, "TK499", COLOR_WHITE, COLOR_BLUE);
    LCD_DrawString((LCD_WIDTH - string_width("Clock Demo", 1)) / 2, BAR_TEXT_Y,
                   "Clock Demo", COLOR_CYAN, COLOR_BLUE);
    LCD_DrawString(LCD_WIDTH - string_width("v1.0", 1) - 4, BAR_TEXT_Y,
                   "v1.0", COLOR_WHITE, COLOR_BLUE);

    /*
     * Draw text in bottom bar (orange background)
     * - Bottom left corner
     * - Bottom center
     * - Bottom right corner
     */
    LCD_DrawString(4, LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y,
                   "Status: OK", COLOR_BLACK, COLOR_ORANGE);
    LCD_DrawString((LCD_WIDTH - string_width("TKM32F499 SmartBoard", 1)) / 2,
                   LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y,
                   "TKM32F499 SmartBoard", COLOR_BLACK, COLOR_ORANGE);
    LCD_DrawString(LCD_WIDTH - string_width("240MHz", 1) - 4,
                   LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y,
                   "240MHz", COLOR_BLACK, COLOR_ORANGE);

    /*
     * Draw horizontal line above the time
     * Uses LCD_FillRect to draw a thin rectangle
     */
    LCD_FillRect(LINE_MARGIN, LINE_ABOVE_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);

    /*
     * Draw the centered time display
     * Using scaled 8x16 font at 6x scale = 48x96 pixels per character
     */
    LCD_DrawStringLarge(TIME_X, TIME_Y, "12:00:00", COLOR_GREEN, COLOR_BLACK, TIME_SCALE);

    /*
     * Draw horizontal line below the time
     */
    LCD_FillRect(LINE_MARGIN, LINE_BELOW_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);

    /* Main loop - blink LED slowly to show we're alive */
    while (1) {
        GPIOA->BSRR = (1 << 8);   /* LED on */
        delay(2000000);
        GPIOA->BSRR = (1 << 24);  /* LED off */
        delay(2000000);
    }

    return 0;
}
