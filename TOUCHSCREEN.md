# TKM32F499 Touchscreen Guide

This document describes how the resistive touchscreen works on the TKM32F499 4.3" SmartBoard.

## Hardware Overview

The TKM32F499 SmartBoard comes in two variants:
- **RTP (Resistive Touch Panel)** - Uses built-in hardware TOUCHPAD ADC
- **CTP (Capacitive Touch Panel)** - Uses external I2C touch controller (GT911/FT6206)

**Important:** The resistive version does NOT use an external XPT2046 chip as you might expect. Instead, it uses a hardware TOUCHPAD ADC peripheral built into the TKM32F499.

## Key Discovery

The vendor reference code has two different touch configurations that look similar but are completely different:

| Config | `use_XPT2046` | `USE_RTP` | Method |
|--------|---------------|-----------|--------|
| External XPT2046 | 1 | 0 | Bit-banged SPI to external chip |
| Built-in TOUCHPAD | 0 | 1 | Hardware TOUCHPAD ADC peripheral |

The 4.3" SmartBoard RTP version uses `USE_RTP=1`, which means it uses the **built-in TOUCHPAD peripheral**, not an external SPI chip.

## TOUCHPAD Peripheral

### Base Address
```c
#define TOUCHPAD_BASE  (APB2PERIPH_BASE + 0x6400)  // 0x40016400
```

### Register Structure
```c
typedef struct {
    volatile uint32_t ADDATA;   /* 0x00: ADC data */
    volatile uint32_t ADCFG;    /* 0x04: ADC configuration */
    volatile uint32_t ADCR;     /* 0x08: ADC control */
    volatile uint32_t ADCHS;    /* 0x0C: ADC channel select */
    volatile uint32_t ADCMPR;   /* 0x10: ADC compare */
    volatile uint32_t ADSTA;    /* 0x14: ADC status */
    volatile uint32_t ADDR0;    /* 0x18-0x3C: ADC data registers 0-9 */
    ...
    volatile uint32_t TPXDR;    /* 0x48: Touch panel X data */
    volatile uint32_t TPYDR;    /* 0x4C: Touch panel Y data */
    volatile uint32_t TPCR;     /* 0x50: Touch panel control */
    volatile uint32_t TPFR;     /* 0x54: Touch panel filter */
    volatile uint32_t TPCSR;    /* 0x58: Touch panel channel select */
} TOUCHPAD_TypeDef;
```

### GPIO Configuration

The touch panel uses GPIOB pins 0-3 in **analog mode**:

| Pin | Function |
|-----|----------|
| PB0 | Touch ADC input |
| PB1 | Touch ADC input |
| PB2 | Touch ADC input |
| PB3 | Touch ADC input |

```c
/* Configure as analog input */
GPIOB->CRL &= 0xFFFF0000;
GPIOB->CRL |= 0x0000BBBB;    /* 0xB = analog mode */

/* Set alternate function for touchpad */
GPIOB->AFRL &= 0xFFFF0000;
GPIOB->AFRL |= 0x0000DDDD;   /* 0xD = touchpad function */
```

### Clock Enable

```c
/* Enable TOUCHPAD peripheral clocks on APB2 */
RCC->APB2ENR |= (1 << 8);    /* ADC clock */
RCC->APB2ENR |= (1 << 25);   /* Touch panel clock */
```

## Initialization

```c
void Touch_Init(void)
{
    /* Enable clocks */
    RCC->APB2ENR |= (1 << 8) | (1 << 25);
    RCC->AHB1ENR |= (1 << 1);  /* GPIOB */

    /* Configure GPIO as analog */
    GPIOB->CRL &= 0xFFFF0000;
    GPIOB->CRL |= 0x0000BBBB;
    GPIOB->AFRL &= 0xFFFF0000;
    GPIOB->AFRL |= 0x0000DDDD;

    /* Configure ADC */
    TOUCHPAD->ADCFG = 0xEE1E70;   /* Prescale, compare settings */
    TOUCHPAD->ADCR = 0x400;       /* Scan mode */
    TOUCHPAD->ADCHS = 0xC;        /* Channel enable */
    TOUCHPAD->TPFR = 0x0FFFFFF;   /* Filter config */

    /* Enable touch panel mode */
    TOUCHPAD->TPCR = 0x1;

    /* Enable ADC and start conversion */
    TOUCHPAD->ADCFG |= 0x1;
    TOUCHPAD->ADCR |= (1 << 8);
}
```

## Reading Touch Data

### Polling Method

```c
uint8_t Touch_Read(Touch_State_t *state)
{
    /* Wait for conversion complete flag */
    while (!(TOUCHPAD->TPCR & (1 << 16)));

    /* Read values - note X/Y registers are swapped! */
    uint16_t raw_x = TOUCHPAD->TPYDR & 0xFFF;
    uint16_t raw_y = TOUCHPAD->TPXDR & 0xFFF;

    /* Clear flag and restart */
    TOUCHPAD->TPCR |= (1 << 16);
    TOUCHPAD->ADCR |= (1 << 8);

    /* Filter and calibrate... */
}
```

### Interrupt Method

The vendor code uses interrupts (IRQ 86) for better responsiveness:

```c
void TOUCHPAD_IRQHandler(void)
{
    if (TOUCHPAD->TPCR & (1 << 16)) {
        /* Read samples */
        sample_x = TOUCHPAD->TPYDR & 0xFFF;
        sample_y = TOUCHPAD->TPXDR & 0xFFF;

        /* Re-enable interrupt */
        TOUCHPAD->TPCR |= 0x2;
    }
}
```

**Note:** Using interrupts requires adding `TOUCHPAD_IRQHandler` to the vector table at position 86.

## Calibration

The raw ADC values need to be mapped to screen coordinates:

| Parameter | Value |
|-----------|-------|
| Raw X min | ~460 |
| Raw X max | ~3940 |
| Raw Y min | ~450 |
| Raw Y max | ~3800 |
| Screen width | 800 |
| Screen height | 480 |

```c
/* Map raw to screen coordinates */
screen_x = ((raw_x - 460) * 800) / (3940 - 460);
screen_y = ((raw_y - 450) * 480) / (3800 - 450);
```

## Filtering

The vendor code takes 10 samples, sorts them, discards outliers, and averages the middle values:

1. Collect 10 samples
2. Bubble sort both X and Y arrays
3. Check consistency: if `samples[7] - samples[2] > 200`, reject as noise
4. Average middle 4 values: `(samples[3] + samples[4] + samples[5] + samples[6]) / 4`

## Common Pitfalls

### 1. Wrong Touch Method
The biggest pitfall is assuming the board uses XPT2046 SPI when it actually uses the built-in TOUCHPAD peripheral. Check the vendor's `GUIConf.h` for `USE_RTP=1` vs `use_XPT2046=1`.

### 2. X/Y Register Swap
The TPXDR and TPYDR registers are swapped relative to their names:
- `TOUCHPAD->TPYDR` contains X data
- `TOUCHPAD->TPXDR` contains Y data

### 3. Interrupt Vector
If using interrupts, ensure `TOUCHPAD_IRQHandler` is in the vector table at position 86, or the system will hard fault.

### 4. Clock Enable
Both bit 8 (ADC) and bit 25 (touch panel) must be enabled in `RCC->APB2ENR`.

## Files

- `src/touch.c` - Touch driver implementation
- `inc/touch.h` - Touch driver header
- `inc/tkm32f499.h` - TOUCHPAD peripheral definitions

## References

- Vendor reference: `TK499_emWin6.14_SmartBoard_TK043F1168_ICO_Default_Program_for_RTP`
- Key files: `emWin/GUI.C` (interrupt handler), `Hardware/TK499_GPIO/TK499_GPIO.c` (init)
