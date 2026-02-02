# TKM32F499 Lessons Learned

Key discoveries from getting the TKM32F499 4.3inch SmartBoard working.

## Memory Map

The TKM32F499 runs code from **external SDRAM**, not internal flash like most STM32 chips.

- Code executes from: `0x70020000` (SDRAM)
- Stack/heap in: `0x70220000` (SDRAM)
- Internal SRAM: `0x20000000` (used for vector table)

The linker script must reflect this - see `linker/tkm32f499.ld`.

## Vector Table Remapping (CRITICAL)

Code **will not work** without remapping the vector table from SDRAM to internal SRAM.

```c
static void RemapVtorTable(void)
{
    /* Enable internal SRAM clock (bit 13) */
    RCC_AHB1ENR |= (1 << 13);

    /* Disable all interrupts */
    for (int i = 0; i < 3; i++) {
        NVIC_ICER[i] = 0xFFFFFFFF;
    }

    /* Set VTOR to internal SRAM with bit 29 */
    SCB_VTOR = 0;
    SCB_VTOR |= (1 << 29);

    /* Copy vector table from SDRAM to SRAM */
    for (int i = 0; i < 512; i += 4) {
        *(volatile uint32_t *)(0x20000000 + i) = *(volatile uint32_t *)(0x70020000 + i);
    }
}
```

This must be called at the **very start** of `main()`.

## GPIO Registers

The TKM32F499 uses **STM32F1-style GPIO registers** (CRL/CRH), NOT STM32F4-style (MODER/OTYPER).

```c
typedef struct {
    volatile uint32_t CRL;   /* Control Register Low (pins 0-7) */
    volatile uint32_t CRH;   /* Control Register High (pins 8-15) */
    volatile uint32_t IDR;   /* Input Data Register */
    volatile uint32_t ODR;   /* Output Data Register */
    volatile uint32_t BSRR;  /* Bit Set/Reset Register */
    volatile uint32_t BRR;   /* Bit Reset Register */
    volatile uint32_t LCKR;  /* Lock Register */
} GPIO_TypeDef;
```

GPIO base addresses:
- GPIOA: `0x40020000`
- GPIOB: `0x40020400`
- GPIOC: `0x40020800`
- GPIOD: `0x40020C00`

## Bootloader System

The TKM32F499 has a **two-stage bootloader**:

1. **ROM Bootloader** (permanent, in chip) - Always works
2. **Secondary Bootloader** (in SPI FLASH) - Can be corrupted

### USB Drive Names Tell You Which Mode You're In

- **"TK499"** = ROM's native download mode (for flashing bootloader)
- **"TK499_V2"** = Secondary bootloader mode (for flashing applications)

### Button Combinations (4.3inch SmartBoard)

| Action | Button Combo | Result |
|--------|--------------|--------|
| Flash bootloader | BOOT (PA13) + RESET | "TK499" drive appears |
| Flash application | APP (PA1) + RESET | "TK499_V2" drive appears |

Procedure: Hold button, press RESET, release RESET, release button.

### Bootloader Recovery

If your board stops running programs:

1. Use BOOT + RESET to enter ROM mode ("TK499" drive)
2. Copy `Bootloader.bin` to the drive
3. Wait for auto-unmount, press RESET
4. Now APP + RESET should show "TK499_V2"

## 4.3inch SmartBoard Pinout

Key pins for the 4.3inch SmartBoard (different from evaluation board):

| Function | Pin | Notes |
|----------|-----|-------|
| User LED (D3) | PA8 | Active high, through 4.7K resistor |
| APP Button | PA1 (SW2) | For entering app download mode |
| BOOT Button | PA13 (SW3) | For entering bootloader download mode |
| LCD Backlight | Controlled by MP3302 boost converter | |

## Build & Flash Workflow

1. Build: `make`
2. Enter download mode: APP + RESET (get "TK499_V2" drive)
3. Copy: `cp build/tkm32f499_clock.bin /media/<user>/TK499_V2/`
4. Wait for drive to auto-unmount
5. Press RESET to run

## Clock Speed

The TKM32F499 runs at **240MHz**. Delay loops need large values:
- `delay(5000000)` is roughly 1 second with a simple NOP loop
