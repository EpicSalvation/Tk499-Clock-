/**
 * TKM32F499 Resistive Touch Panel Driver
 *
 * Uses the built-in hardware TOUCHPAD ADC peripheral (polling mode).
 * Based on vendor reference code for RTP (Resistive Touch Panel).
 */

#include "touch.h"
#include "tkm32f499.h"

/* Store raw ADC values */
static volatile uint16_t touch_raw_x = 0;
static volatile uint16_t touch_raw_y = 0;

void Touch_GetRaw(uint16_t *raw_x, uint16_t *raw_y)
{
    *raw_x = touch_raw_x;
    *raw_y = touch_raw_y;
}

void Touch_Init(void)
{
    /* Enable clocks for TOUCHPAD peripheral */
    /* Bit 8: ADC clock, Bit 25: Touch panel clock */
    RCC->APB2ENR |= (1 << 8) | (1 << 25);

    /* Enable GPIOB clock */
    RCC->AHB1ENR |= (1 << 1);

    /* Configure GPIO PB0-3 as analog input with alternate function */
    /* 0xb = analog mode for each pin */
    GPIOB->CRL &= 0xFFFF0000;
    GPIOB->CRL |= 0x0000BBBB;

    /* Set alternate function to 0xD for touchpad */
    GPIOB->AFRL &= 0xFFFF0000;
    GPIOB->AFRL |= 0x0000DDDD;

    /* Configure ADC */
    TOUCHPAD->ADCFG = 0xEE1E70;     /* ADC config: 16 prescale, compare settings */
    TOUCHPAD->ADCR = 0x400;         /* Scan mode */
    TOUCHPAD->ADCHS = 0xC;          /* Enable channels for touch */
    TOUCHPAD->TPFR = 0x0FFFFFF;     /* Filter config */

    /* Enable touch panel mode (no interrupt) */
    TOUCHPAD->TPCR = 0x1;

    /* Enable ADC */
    TOUCHPAD->ADCFG |= 0x1;

    /* Start ADC conversion */
    TOUCHPAD->ADCR |= (1 << 8);
}

/* Simple delay */
static void touch_delay(volatile int count)
{
    while (count--);
}

uint8_t Touch_Read(Touch_State_t *state)
{
    uint16_t raw_x, raw_y;
    uint16_t samples_x[10], samples_y[10];
    uint16_t i, j, temp;

    state->pressed = 0;
    state->x = 0;
    state->y = 0;

    /* Collect 10 samples */
    for (i = 0; i < 10; i++) {
        /* Wait for conversion flag or timeout */
        volatile int timeout = 1000;
        while (!(TOUCHPAD->TPCR & (1 << 16)) && timeout > 0) {
            timeout--;
        }

        /* Read X and Y values */
        samples_x[i] = TOUCHPAD->TPYDR & 0xFFF;
        samples_y[i] = TOUCHPAD->TPXDR & 0xFFF;

        /* Clear flag and restart conversion */
        TOUCHPAD->TPCR |= (1 << 16);
        TOUCHPAD->ADCR |= (1 << 8);

        touch_delay(100);
    }

    /* Bubble sort X values */
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9 - i; j++) {
            if (samples_x[j] > samples_x[j + 1]) {
                temp = samples_x[j + 1];
                samples_x[j + 1] = samples_x[j];
                samples_x[j] = temp;
            }
        }
    }

    /* Bubble sort Y values */
    for (i = 0; i < 9; i++) {
        for (j = 0; j < 9 - i; j++) {
            if (samples_y[j] > samples_y[j + 1]) {
                temp = samples_y[j + 1];
                samples_y[j + 1] = samples_y[j];
                samples_y[j] = temp;
            }
        }
    }

    /* Check for consistency - if spread is too large, touch is invalid */
    if ((samples_x[7] - samples_x[2] > 200) || (samples_y[7] - samples_y[2] > 200)) {
        touch_raw_x = 0;
        touch_raw_y = 0;
        return 0;
    }

    /* Average the middle 4 values */
    raw_x = (samples_x[3] + samples_x[4] + samples_x[5] + samples_x[6]) >> 2;
    raw_y = (samples_y[3] + samples_y[4] + samples_y[5] + samples_y[6]) >> 2;

    /* Store for debug */
    touch_raw_x = raw_x;
    touch_raw_y = raw_y;

    /* Check if touch is valid (within calibration range) */
    if (raw_x < 460 || raw_y < 450 || raw_x > 3940 || raw_y > 3800) {
        return 0;
    }

    state->pressed = 1;

    /* Map to screen coordinates (800x480) */
    int32_t x = raw_x;
    int32_t y = raw_y;

    /* Clamp to calibration range */
    if (x < 460) x = 460;
    if (x > 3940) x = 3940;
    if (y < 450) y = 450;
    if (y > 3800) y = 3800;

    /* Map to screen coordinates */
    state->x = (uint16_t)(((x - 460) * 800) / (3940 - 460));
    state->y = (uint16_t)(((y - 450) * 480) / (3800 - 450));

    /* Clamp to screen bounds */
    if (state->x >= 800) state->x = 799;
    if (state->y >= 480) state->y = 479;

    return 1;
}

uint8_t Touch_InRegion(uint16_t x, uint16_t y,
                       uint16_t rx, uint16_t ry,
                       uint16_t rw, uint16_t rh)
{
    return (x >= rx && x < (rx + rw) &&
            y >= ry && y < (ry + rh));
}
