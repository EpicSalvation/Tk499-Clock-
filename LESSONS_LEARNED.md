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

**CRITICAL: The actual clock speed depends on how code is loaded!**

- **When running via USB bootloader: 192MHz** (not 240MHz!)
- The baud rate and delay calculations must use the correct clock

Delay loop calibration for 192MHz:
```c
/* ~4 cycles per iteration, so 48000 iterations ≈ 1ms at 192MHz */
static void delay_ms(uint32_t ms) {
    volatile uint32_t count = ms * 48000;
    while (count--) {
        __asm volatile ("nop");
    }
}
```

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

## UART Peripheral (NOT USART!)

**CRITICAL: The TKM32F499 uses a custom UART peripheral, NOT STM32-style USART!**

The register layout is completely different:

### UART Register Structure

```c
typedef struct {
    volatile uint32_t TDR;      /* 0x00: Transmit Data Register */
    volatile uint32_t RDR;      /* 0x04: Receive Data Register */
    volatile uint32_t CSR;      /* 0x08: Control/Status Register */
    volatile uint32_t ISR;      /* 0x0C: Interrupt Status Register */
    volatile uint32_t IER;      /* 0x10: Interrupt Enable Register */
    volatile uint32_t ICR;      /* 0x14: Interrupt Clear Register */
    volatile uint32_t GCR;      /* 0x18: General Control Register */
    volatile uint32_t CCR;      /* 0x1C: Character Control Register */
    volatile uint32_t BRR;      /* 0x20: Baud Rate Register */
    volatile uint32_t FRABRG;   /* 0x24: Fractional Baud Rate Generator */
} UART_TypeDef;
```

### UART Base Addresses (APB2 bus!)

```c
#define UART1_BASE  (0x40010000 + 0x0800)  /* 0x40010800 */
#define UART2_BASE  (0x40010000 + 0x0C00)  /* 0x40010C00 */
#define UART3_BASE  (0x40010000 + 0x1000)  /* 0x40011000 */
```

### UART Clock Enable (CRITICAL!)

**The RCC bits for UART are NOT where you'd expect from STM32 documentation!**

```c
/* UART clock enable - APB2ENR bits 2-6 */
RCC->APB2ENR |= (1 << 2);  /* UART1 */
RCC->APB2ENR |= (1 << 3);  /* UART2 */
RCC->APB2ENR |= (1 << 4);  /* UART3 */
RCC->APB2ENR |= (1 << 5);  /* UART4 */
RCC->APB2ENR |= (1 << 6);  /* UART5 */
```

**NOT** APB1ENR bit 17 as some documentation suggests!

### Baud Rate Calculation

```c
/* Formula: BRR = SYS_CLK / baudrate / 16 */
/*          FRABRG = (SYS_CLK / baudrate) % 16 */
/* CRITICAL: Use 192MHz when running via bootloader, not 240MHz! */
uint32_t sys_clk = 192000000;  /* 192MHz via bootloader */
uint32_t div = sys_clk / baudrate;
UART2->BRR = div / 16;
UART2->FRABRG = div % 16;
```

**Symptom of wrong clock speed:** First byte received correctly, then garbage. This indicates baud rate drift.

### CSR (Control/Status) Register Bits

```c
#define UART_CSR_TXC    (1 << 0)  /* TX complete/ready */
#define UART_CSR_RXAVL  (1 << 1)  /* RX data available */
```

### GCR (General Control) Register Bits

```c
#define UART_GCR_UARTEN (1 << 0)  /* UART enable */
#define UART_GCR_RXEN   (1 << 3)  /* RX enable */
#define UART_GCR_TXEN   (1 << 4)  /* TX enable */
```

### GPIO Alternate Function for UART

```c
/* PA2/PA3 for UART2 - use AF7 */
#define GPIO_AF_UART2345  0x07

GPIOA->AFRL &= ~(0xFF << 8);        /* Clear PA2, PA3 AF */
GPIOA->AFRL |= (0x07 << 8);         /* PA2 = AF7 */
GPIOA->AFRL |= (0x07 << 12);        /* PA3 = AF7 */
```

### Basic UART Send/Receive

```c
/* Send byte */
while (!(UART2->CSR & UART_CSR_TXC));  /* Wait for TX ready */
UART2->TDR = byte;

/* Receive byte */
while (!(UART2->CSR & UART_CSR_RXAVL)); /* Wait for RX data */
uint8_t data = UART2->RDR;
```

## ESP8266 WiFi Module

The 4.3" SmartBoard has an onboard ESP8266 module (ESP8266MOD).

### Hardware Connections

| Function | TKM32F499 Pin | Notes |
|----------|---------------|-------|
| UART TX  | PA2           | Connect to ESP RX |
| UART RX  | PA3           | Connect to ESP TX |
| RST      | PD0           | Reset control |
| CH_PD    | PD1           | Chip enable |

**Note:** The reference code uses PD0/PD1, not PB14/PB15 as some schematics suggest!

### ESP8266 Initialization Sequence

```c
int ESP_Init(void)
{
    /* 1. Enable GPIOD clock */
    RCC->AHB1ENR |= (1 << 3);

    /* 2. Configure PD0 (RST) and PD1 (CH_PD) as outputs */
    GPIOD->CRL &= ~(0xFF << 0);
    GPIOD->CRL |= (0x33 << 0);

    /* 3. Set CH_PD high (enable), RST high (not in reset) */
    GPIOD->BSRR = (1 << 1) | (1 << 0);

    /* 4. Initialize UART2 at 115200 */
    UART2_Init(115200);

    /* 5. Reset ESP8266: pull RST low, wait, release */
    GPIOD->BSRR = (1 << 16);  /* PD0 low */
    delay_ms(100);
    GPIOD->BSRR = (1 << 0);   /* PD0 high */

    /* 6. Wait for boot (3-4 seconds!) */
    delay_ms(4000);

    /* 7. Clear any boot messages */
    ESP_ClearRxBuffer();

    /* 8. Test with AT command (retry several times) */
    for (int i = 0; i < 10; i++) {
        if (ESP_Test() == ESP_OK) {
            ESP_SendCommand("ATE0", NULL, 0, 1000);  /* Disable echo */
            return ESP_OK;
        }
        delay_ms(1000);
        ESP_ClearRxBuffer();
    }
    return ESP_ERROR;
}
```

### ESP8266 Boot Sequence

1. ESP8266 sends boot messages at **74880 baud**
2. After boot, AT firmware runs at **115200 baud** (default)
3. Reset pulse: PD0 low for 100-200ms, then high
4. Wait **3-4 seconds** for boot to complete (critical!)

### AT Firmware Version Limitations

The SmartBoard's ESP8266 may have older AT firmware (e.g., **v1.3.0**).

| Feature | Minimum Firmware |
|---------|------------------|
| Basic AT commands | Any |
| WiFi connect (CWJAP) | Any |
| TCP connections (CIPSTART) | Any |
| **SNTP time sync (CIPSNTPCFG)** | **v1.7.0+** |
| SSL/TLS connections | v2.0.0+ |

**If SNTP commands return ERROR**, your firmware is too old. Options:
1. Update ESP8266 firmware (requires serial connection to ESP)
2. Use HTTP-based time sync via Date header (requires plain HTTP server)

See `ESP8266_WIFI.md` for complete WiFi setup guide.
