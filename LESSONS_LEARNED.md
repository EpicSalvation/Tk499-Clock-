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

## RCC Register Layout (CRITICAL!)

The TKM32F499 has a **different RCC register layout** from STM32F4! Do NOT use STM32F4 offsets!

| Register  | TKM32F499 Offset | STM32F4 Offset (WRONG!) |
|-----------|------------------|-------------------------|
| AHB1ENR   | 0x20             | 0x30                    |
| AHB2ENR   | 0x24             | 0x34                    |
| APB1ENR   | 0x28             | 0x40                    |
| APB2ENR   | 0x2C             | 0x44                    |

Using wrong offsets means clocks are never enabled and peripherals won't work!

## GPIO Registers

The TKM32F499 uses **STM32F1-style GPIO registers** (CRL/CRH), NOT STM32F4-style (MODER/OTYPER).

**CRITICAL: The TKM32F499 has EXTENDED GPIO with 24 pins per port (0-23), not just 16!**

```c
typedef struct {
    volatile uint32_t CRL;       /* 0x00: Control Register Low (pins 0-7) */
    volatile uint32_t CRH;       /* 0x04: Control Register High (pins 8-15) */
    volatile uint32_t IDR;       /* 0x08: Input Data Register */
    volatile uint32_t ODR;       /* 0x0C: Output Data Register */
    volatile uint32_t BSRR;      /* 0x10: Bit Set/Reset Register */
    volatile uint32_t BRR;       /* 0x14: Bit Reset Register */
    volatile uint32_t LCKR;      /* 0x18: Lock Register */
    volatile uint32_t RESERVED;  /* 0x1C: Reserved */
    volatile uint32_t AFRL;      /* 0x20: Alternate Function Low (pins 0-7) */
    volatile uint32_t AFRH;      /* 0x24: Alternate Function High (pins 8-15) */
    volatile uint32_t CRH_EXT;   /* 0x28: Control Register Extended (pins 16-23) */
    volatile uint32_t BSRR_EXT;  /* 0x2C: Bit Set/Reset Extended */
    volatile uint32_t AFRH_EXT;  /* 0x30: Alternate Function Extended (pins 16-23) */
} GPIO_TypeDef;
```

GPIO base addresses:
- GPIOA: `0x40020000`
- GPIOB: `0x40020400`
- GPIOC: `0x40020800`
- GPIOD: `0x40020C00`
- GPIOE: `0x40021000`

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

## TK80 LCD Controller

The TKM32F499 has a custom **TK80** peripheral for driving LCDs in MCU/8080 mode. This is NOT the same as STM32's FSMC.

### TK80 Base Address and Registers

```c
#define TK80_BASE       0x60000000
#define TK80_CR         (TK80_BASE + 0x00)  /* Control Register */
#define TK80_CFGR1      (TK80_BASE + 0x04)  /* Configuration Register 1 */
#define TK80_CFGR2      (TK80_BASE + 0x08)  /* Configuration Register 2 */
#define TK80_SR         (TK80_BASE + 0x0C)  /* Status Register */
#define TK80_CMDIR      (TK80_BASE + 0x10)  /* Command Input Register */
#define TK80_DINR       (TK80_BASE + 0x14)  /* Data Input Register */
#define TK80_CFGR3      (TK80_BASE + 0x30)  /* Config Reg 3 (pixel count for block fill) */
```

### TK80 Configuration Values (LCD-specific!)

Different LCD panels need different CFGR values:

| LCD Panel | CFGR1 | CFGR2 |
|-----------|-------|-------|
| TK043F1168 (4.3" SmartBoard) | 0x050202 | 0x01 |
| TK020F9168 | 0x05050503 | 0x0503 |
| TK022RB417 | 0x05020202 | 0x0501 |

### TK80 Clock Enable

```c
RCC->AHB2ENR |= (1 << 31);  /* Enable TK80 clock */
```

### Writing Commands and Data

```c
void WriteComm(uint8_t cmd) {
    TK80->CMDIR = cmd;
    while (TK80->SR & 0x10000);  /* Wait for not busy */
}

void WriteData(uint32_t dat) {
    TK80->DINR = dat;
    while (TK80->SR & 0x10000);
}
```

### Block Fill (Fast Rectangle Fill)

```c
void FillRect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
    BlockWrite(x, x + w - 1, y, y + h - 1);
    TK80->CFGR3 = (uint32_t)w * h;  /* Set pixel count */
    TK80->DINR = color;              /* Single color write fills all pixels */
    while (TK80->SR & 0x10000);
}
```

## 4.3" SmartBoard LCD (TK043F1168)

### Hardware Configuration

- **LCD Controller**: HX8369 compatible
- **Resolution**: 800x480
- **LCD Reset Pin**: PA14 (not PD6!)
- **Backlight Pin**: PD8 (directly controls MP3302 boost converter EN pin)

### Backlight PWM Brightness Control

PD8 controls the MP3302 boost converter enable. For variable brightness, use software PWM via TIM3:

```c
/* Initialize PWM (call after LCD_Init) */
LCD_BrightnessInit();

/* Set brightness 0-100% */
LCD_SetBrightness(75);

/* Get current brightness */
uint8_t level = LCD_GetBrightness();
```

PWM implementation uses TIM3 interrupt at 100kHz to generate 1kHz PWM with 100 brightness levels. See `LCD_RENDERING.md` for full details.

### LCD Reset Sequence

```c
/* Configure PA14 as output */
GPIOA->CRH = (GPIOA->CRH & ~(0x0F << 24)) | (0x02 << 24);

/* Toggle reset */
GPIOA->BRR = (1 << 14);   /* PA14 low */
delay(50000);
GPIOA->BSRR = (1 << 14);  /* PA14 high */
delay(50000);
```

### GPIO Configuration for TK80 (24-bit color)

**CRITICAL: For 24-bit color, you MUST configure PE16-PE23 using the extended GPIO registers!**

```c
/* PB8-11 as AF push-pull for TK80 control (CS, RS, WR, RD) */
GPIOB->CRH = 0x0000AAAA;
GPIOB->AFRH = 0x0000CCCC;  /* AF12 */

/* PE0-15 as AF push-pull for TK80 data (Blue + Green channels) */
GPIOE->CRL = 0xAAAAAAAA;      /* PE0-7: Blue channel */
GPIOE->CRH = 0xAAAAAAAA;      /* PE8-15: Green channel */
GPIOE->AFRL = 0xCCCCCCCC;     /* AF12 */
GPIOE->AFRH = 0xCCCCCCCC;     /* AF12 */

/* PE16-23 as AF push-pull for TK80 data (Red channel) - EXTENDED GPIO! */
GPIOE->CRH_EXT = 0xAAAAAAAA;  /* PE16-23: Red channel */
GPIOE->AFRH_EXT = 0xCCCCCCCC; /* AF12 */
```

Without configuring PE16-23, only Blue and Green will display - Red will be missing!

### IM0-IM3 Interface Mode Pins

The LCD has hardware mode pins that configure the interface:

| Mode | IM0 | IM1 | IM2 | IM3 |
|------|-----|-----|-----|-----|
| RGB888 | 1 | 0 | 1 | 1 |
| 24-bit | 0 | 1 | 0 | 1 |

## LCD Color Format (SOLVED!)

### Color Format: 0x00RRGGBB

In RGB888 mode (command 0x3A = 0x77), with PE16-23 properly configured:

```c
/* Color format: 0x00RRGGBB */
#define RED     0x00FF0000
#define GREEN   0x0000FF00
#define BLUE    0x000000FF
#define WHITE   0x00FFFFFF
#define BLACK   0x00000000
#define YELLOW  0x00FFFF00  /* Red + Green */
#define CYAN    0x0000FFFF  /* Green + Blue */
#define MAGENTA 0x00FF00FF  /* Red + Blue */
```

### Data Bus Mapping

| GPIO Pins | Byte | Color Channel |
|-----------|------|---------------|
| PE0-PE7   | 0    | Blue          |
| PE8-PE15  | 1    | Green         |
| PE16-PE23 | 2    | Red           |

### Why Red Wasn't Working (Root Cause)

The red channel uses **PE16-PE23**, which are extended GPIO pins requiring special registers:
- `CRH_EXT` (offset 0x28) for pin configuration
- `AFRH_EXT` (offset 0x30) for alternate function

Without configuring these extended registers, only 16 bits (Blue + Green) were transmitted.

### Complete TK80 GPIO Setup for 24-bit Color

```c
/* Enable GPIOE clock */
RCC_AHB1ENR |= (1 << 4);

/* PE0-7: Blue (standard GPIO) */
GPIOE_CRL = 0xAAAAAAAA;
GPIOE_AFRL = 0xCCCCCCCC;  /* AF12 */

/* PE8-15: Green (standard GPIO) */
GPIOE_CRH = 0xAAAAAAAA;
GPIOE_AFRH = 0xCCCCCCCC;  /* AF12 */

/* PE16-23: Red (EXTENDED GPIO - critical!) */
GPIOE_CRH_EXT = 0xAAAAAAAA;
GPIOE_AFRH_EXT = 0xCCCCCCCC;  /* AF12 */
```
