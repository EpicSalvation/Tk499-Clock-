/**
 * TKM32F499 LED Blink Test
 *
 * A minimal test program that blinks the LED on PA8 (D3 on SmartBoard).
 * Useful for verifying that the hardware and flashing process work correctly.
 *
 * Build with: make blink_test
 * Direct register access - no library code
 */

#include <stdint.h>

/* Core registers */
#define SCB_VTOR    (*(volatile uint32_t *)0xE000ED08)
#define NVIC_ICER   ((volatile uint32_t *)0xE000E180)

/* RCC registers */
#define RCC_BASE    0x40023800
#define RCC_AHB1ENR (*(volatile uint32_t *)(RCC_BASE + 0x30))

/* GPIOA registers (for LED on PA8) */
#define GPIOA_BASE  0x40020000
#define GPIOA_CRL   (*(volatile uint32_t *)(GPIOA_BASE + 0x00))
#define GPIOA_CRH   (*(volatile uint32_t *)(GPIOA_BASE + 0x04))
#define GPIOA_ODR   (*(volatile uint32_t *)(GPIOA_BASE + 0x0C))
#define GPIOA_BSRR  (*(volatile uint32_t *)(GPIOA_BASE + 0x10))

/* Memory addresses */
#define T_SRAM_BASE  0x20000000
#define T_SDRAM_BASE 0x70020000

/* Remap vector table - CRITICAL for code to run */
static void RemapVtorTable(void)
{
    int i;

    /* Enable internal SRAM clock (bit 13) */
    RCC_AHB1ENR |= (1 << 13);

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

    /* Enable GPIOA clock (bit 0 of AHB1ENR) */
    RCC_AHB1ENR |= (1 << 0);

    /* Small delay for clock to stabilize */
    delay(1000);

    /* Configure PA8 as push-pull output, 50MHz */
    /* CRH controls pins 8-15, pin 8 is bits [3:0] */
    /* MODE=11 (50MHz), CNF=00 (push-pull) -> 0x03 */
    GPIOA_CRH &= ~(0x0F << 0);  /* Clear bits [3:0] */
    GPIOA_CRH |= (0x03 << 0);   /* Set output 50MHz push-pull */

    /* Blink forever - ~1 second on, ~1 second off */
    while (1) {
        GPIOA_BSRR = (1 << 8);      /* Set PA8 high (LED on) */
        delay(5000000);
        GPIOA_BSRR = (1 << 24);     /* Set PA8 low (LED off) */
        delay(5000000);
    }

    return 0;
}
