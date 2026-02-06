/**
 * TKM32F499 Touch Panel Driver Header
 *
 * Driver for FT6206/FT6336 capacitive touch controller via I2C.
 * Uses bit-banged I2C on PB12 (SCL) and PB3 (SDA).
 */

#ifndef __TOUCH_H
#define __TOUCH_H

#include <stdint.h>

/* Touch state structure */
typedef struct {
    uint16_t x;
    uint16_t y;
    uint8_t pressed;
} Touch_State_t;

/**
 * Initialize the touch panel
 */
void Touch_Init(void);

/**
 * Read current touch state
 * @param state Pointer to touch state structure to fill
 * @return 1 if touch detected, 0 if no touch
 */
uint8_t Touch_Read(Touch_State_t *state);

/**
 * Check if a point is within a rectangular region
 * @param x Touch X coordinate
 * @param y Touch Y coordinate
 * @param rx Region X start
 * @param ry Region Y start
 * @param rw Region width
 * @param rh Region height
 * @return 1 if point is in region, 0 otherwise
 */
uint8_t Touch_InRegion(uint16_t x, uint16_t y,
                       uint16_t rx, uint16_t ry,
                       uint16_t rw, uint16_t rh);

/**
 * Get raw ADC values for debugging
 * @param raw_x Pointer to store raw X value
 * @param raw_y Pointer to store raw Y value
 */
void Touch_GetRaw(uint16_t *raw_x, uint16_t *raw_y);

#endif /* __TOUCH_H */
