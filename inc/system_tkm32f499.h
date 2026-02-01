/**
 * TKM32F499 System Header File
 *
 * System initialization and clock configuration functions.
 */

#ifndef __SYSTEM_TKM32F499_H
#define __SYSTEM_TKM32F499_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* System Clock Frequency (default 72MHz) */
extern uint32_t SystemCoreClock;

/**
 * @brief  Setup the microcontroller system
 *         Initialize the Embedded Flash Interface, the PLL and update
 *         the SystemCoreClock variable.
 */
void SystemInit(void);

/**
 * @brief  Update SystemCoreClock variable according to Clock Register Values
 */
void SystemCoreClockUpdate(void);

#ifdef __cplusplus
}
#endif

#endif /* __SYSTEM_TKM32F499_H */
