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

int main(void)
{
    /* CRITICAL: Remap vector table first */
    RemapVtorTable();

    /* Enable GPIO clocks */
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
    delay(1000);

    /* Initialize the LCD */
    LCD_Init();

    /* Clear screen to a dark blue background */
    LCD_Clear(COLOR_BLUE);

    /* Draw title text at top of screen */
    LCD_DrawStringLarge(100, 20, "TK499 Clock", COLOR_WHITE, COLOR_BLUE, 3);

    /* Draw subtitle */
    LCD_DrawString(140, 80, "TKM32F499 SmartBoard", COLOR_CYAN, COLOR_BLUE);

    /* Draw a separator line using filled rectangle */
    LCD_FillRect(40, 100, LCD_WIDTH - 80, 2, COLOR_WHITE);

    /* Display placeholder time in large font */
    LCD_DrawStringLarge(100, 130, "12:00:00", COLOR_GREEN, COLOR_BLUE, 4);

    /* Draw status text */
    LCD_DrawString(160, 220, "LCD Display Working!", COLOR_YELLOW, COLOR_BLUE);

    /* Draw footer */
    LCD_DrawString(120, 250, "Hardware Test Successful", COLOR_GRAY, COLOR_BLUE);

    /* Blink the LED to show we're alive */
    /* Configure PA8 as output */
    GPIOA->CRH &= ~(0x0F << 0);
    GPIOA->CRH |= (0x03 << 0);

    /* Main loop - blink LED slowly */
    while (1) {
        GPIOA->BSRR = (1 << 8);   /* LED on */
        delay(2000000);
        GPIOA->BSRR = (1 << 24);  /* LED off */
        delay(2000000);
    }

    return 0;
}
