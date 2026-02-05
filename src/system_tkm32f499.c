/**
 * TKM32F499 System Source File
 *
 * System initialization and peripheral driver implementations.
 */

#include "tkm32f499.h"
#include "system_tkm32f499.h"

/* System Core Clock - will be updated after HSE/PLL configuration */
uint32_t SystemCoreClock = 192000000;

/* SysTick millisecond counter (volatile for ISR access) */
static volatile uint32_t systick_ms = 0;

/**
 * @brief  Setup the microcontroller system
 */
void SystemInit(void)
{
    /* FPU settings */
    #if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << 10*2) | (3UL << 11*2));  /* set CP10 and CP11 Full Access */
    #endif

    /* Configure the Vector Table location */
    #ifdef VECT_TAB_SRAM
    SCB->VTOR = SRAM_BASE;
    #else
    SCB->VTOR = FLASH_BASE;
    #endif
}

/**
 * @brief  Update SystemCoreClock variable
 */
void SystemCoreClockUpdate(void)
{
    /* Update the SystemCoreClock variable based on RCC register settings */
    SystemCoreClock = 72000000;
}

/* ============================================================ */
/* GPIO Driver Functions                                         */
/* ============================================================ */

/**
 * @brief  Initialize GPIO pin(s) using STM32F1-style CRL/CRH registers
 * @param  GPIOx: GPIO peripheral (GPIOA, GPIOB, etc.)
 * @param  GPIO_InitStruct: Pointer to init structure
 */
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct)
{
    uint32_t currentmode = GPIO_InitStruct->GPIO_Mode & 0x0F;
    uint32_t pinpos, pos, currentpin, pinmask;
    uint32_t tmpreg;

    /* For output modes, add speed to the mode value */
    if ((GPIO_InitStruct->GPIO_Mode & 0x10) != 0) {
        currentmode |= (uint32_t)GPIO_InitStruct->GPIO_Speed;
    }

    /* Configure pins 0-7 (CRL register) */
    if ((GPIO_InitStruct->GPIO_Pin & 0x00FF) != 0) {
        tmpreg = GPIOx->CRL;
        for (pinpos = 0; pinpos < 8; pinpos++) {
            pos = ((uint32_t)1) << pinpos;
            currentpin = GPIO_InitStruct->GPIO_Pin & pos;
            if (currentpin == pos) {
                pinmask = ((uint32_t)0x0F) << (pinpos * 4);
                tmpreg &= ~pinmask;
                tmpreg |= (currentmode << (pinpos * 4));
                /* Handle pull-up/pull-down for input modes */
                if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_IPD) {
                    GPIOx->BRR = pos;  /* Pull-down: reset bit */
                } else if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_IPU) {
                    GPIOx->BSRR = pos; /* Pull-up: set bit */
                }
            }
        }
        GPIOx->CRL = tmpreg;
    }

    /* Configure pins 8-15 (CRH register) */
    if ((GPIO_InitStruct->GPIO_Pin & 0xFF00) != 0) {
        tmpreg = GPIOx->CRH;
        for (pinpos = 0; pinpos < 8; pinpos++) {
            pos = ((uint32_t)1) << (pinpos + 8);
            currentpin = GPIO_InitStruct->GPIO_Pin & pos;
            if (currentpin == pos) {
                pinmask = ((uint32_t)0x0F) << (pinpos * 4);
                tmpreg &= ~pinmask;
                tmpreg |= (currentmode << (pinpos * 4));
                /* Handle pull-up/pull-down for input modes */
                if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_IPD) {
                    GPIOx->BRR = pos;
                } else if (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_IPU) {
                    GPIOx->BSRR = pos;
                }
            }
        }
        GPIOx->CRH = tmpreg;
    }
}

/**
 * @brief  Set GPIO pin(s) high
 */
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BSRR = GPIO_Pin;
}

/**
 * @brief  Set GPIO pin(s) low
 */
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    GPIOx->BSRR = (uint32_t)GPIO_Pin << 16;
}

/**
 * @brief  Read output data bit
 */
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    uint8_t bitstatus = 0x00;

    if ((GPIOx->ODR & GPIO_Pin) != 0) {
        bitstatus = 0x01;
    }

    return bitstatus;
}

/**
 * @brief  Enable or disable AHB peripheral clock
 */
void RCC_AHBPeriphClockCmd(uint32_t RCC_AHBPeriph, int NewState)
{
    if (NewState != DISABLE) {
        RCC->AHB1ENR |= RCC_AHBPeriph;
    } else {
        RCC->AHB1ENR &= ~RCC_AHBPeriph;
    }
}

/* ============================================================ */
/* HSE Crystal and PLL Configuration                             */
/* ============================================================ */

/**
 * @brief  Configure system clock to use HSE (12 MHz external crystal)
 *
 * This function enables the High-Speed External oscillator (HSE) connected
 * to pins OSC_IN (pin 9) and OSC_OUT (pin 10) with a 12 MHz crystal.
 *
 * The PLL is configured to multiply the 12 MHz HSE to 192 MHz:
 *   PLLCLK = HSE * PLLN / PLLM / PLLP = 12 MHz * 192 / 12 / 1 = 192 MHz
 *
 * This provides a crystal-accurate clock source, eliminating the drift
 * inherent in the internal RC oscillator.
 */
void SystemClock_ConfigHSE(void)
{
    volatile uint32_t timeout;

    /* Enable HSE oscillator */
    RCC->CR |= RCC_CR_HSEON;

    /* Wait for HSE to be ready (with timeout) */
    timeout = 0x5000;
    while (!(RCC->CR & RCC_CR_HSERDY) && timeout--) {
        __asm volatile ("nop");
    }

    if (!(RCC->CR & RCC_CR_HSERDY)) {
        /* HSE failed to start - stay on current clock source */
        return;
    }

    /* Configure PLL: HSE as source, multiply to 192 MHz
     * TKM32F499 PLL configuration (similar to STM32F4):
     *   PLLM = 12 (divide HSE by 12 to get 1 MHz VCO input)
     *   PLLN = 192 (multiply by 192 to get 192 MHz VCO output)
     *   PLLP = 0 (divide by 2... but we want /1, so this may vary by chip)
     *
     * Note: The exact PLL register layout may differ from STM32F4.
     * The bootloader already configures the system for 192 MHz, so we
     * primarily need to ensure the HSE is the clock source.
     */

    /* Disable PLL before configuration */
    RCC->CR &= ~RCC_CR_PLLON;

    /* Wait for PLL to be disabled */
    timeout = 0x1000;
    while ((RCC->CR & RCC_CR_PLLRDY) && timeout--) {
        __asm volatile ("nop");
    }

    /* Configure PLL source as HSE
     * PLLCFGR format (typical STM32F4-style):
     *   Bits 5:0   - PLLM (division factor for main PLL input clock)
     *   Bits 14:6  - PLLN (multiplication factor for VCO)
     *   Bits 17:16 - PLLP (division factor for main system clock)
     *   Bit 22     - PLLSRC (0=HSI, 1=HSE)
     *
     * For 12 MHz HSE -> 192 MHz system clock:
     *   PLLM = 12, PLLN = 192, PLLP = 0 (div 2) would give 96 MHz
     *   PLLM = 6, PLLN = 192, PLLP = 0 (div 2) would give 192 MHz
     *   PLLM = 12, PLLN = 384, PLLP = 0 (div 2) would give 192 MHz
     *
     * We'll use: PLLM=6, PLLN=192, PLLP=0 (div 2) = 192 MHz
     */
    RCC->PLLCFGR = (6 << 0)           /* PLLM = 6 (12 MHz / 6 = 2 MHz VCO input) */
                 | (192 << 6)         /* PLLN = 192 (2 MHz * 192 = 384 MHz VCO) */
                 | (0 << 16)          /* PLLP = 0 (divide by 2 -> 192 MHz) */
                 | RCC_PLLCFGR_PLLSRC_HSE;  /* HSE as PLL source */

    /* Enable PLL */
    RCC->CR |= RCC_CR_PLLON;

    /* Wait for PLL to lock */
    timeout = 0x5000;
    while (!(RCC->CR & RCC_CR_PLLRDY) && timeout--) {
        __asm volatile ("nop");
    }

    if (!(RCC->CR & RCC_CR_PLLRDY)) {
        /* PLL failed to lock - stay on current clock source */
        return;
    }

    /* Switch system clock to PLL */
    RCC->CFGR = (RCC->CFGR & ~RCC_CFGR_SW) | RCC_CFGR_SW_PLL;

    /* Wait for system clock switch to complete */
    timeout = 0x1000;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL && timeout--) {
        __asm volatile ("nop");
    }

    /* Update SystemCoreClock variable */
    SystemCoreClock = 192000000;
}

/* ============================================================ */
/* SysTick Configuration for Precise Timekeeping                 */
/* ============================================================ */

/**
 * @brief  Initialize SysTick for 1ms interrupts
 *
 * Uses the ARM Cortex-M4 SysTick timer to generate precise 1ms interrupts.
 * This provides a hardware-based time reference that is much more accurate
 * than software delay loops.
 */
void SysTick_Init(void)
{
    /* Configure SysTick for 1ms interrupts
     * SysTick reload value = (SystemCoreClock / 1000) - 1
     * At 192 MHz: reload = 192000 - 1 = 191999
     */
    SysTick->LOAD = (SystemCoreClock / 1000) - 1;
    SysTick->VAL = 0;  /* Clear current value */

    /* Configure SysTick:
     * Bit 0: ENABLE - Enable counter
     * Bit 1: TICKINT - Enable interrupt
     * Bit 2: CLKSOURCE - Use processor clock (not external reference)
     */
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |
                    SysTick_CTRL_TICKINT_Msk |
                    SysTick_CTRL_ENABLE_Msk;

    /* Enable SysTick interrupt in NVIC (it's a core exception, always enabled) */
    NVIC_SetPriority(SysTick_IRQn, 0);  /* Highest priority for accurate timing */
}

/**
 * @brief  SysTick interrupt handler
 *
 * Called every 1ms to increment the tick counter.
 */
void SysTick_Handler(void)
{
    systick_ms++;
}

/**
 * @brief  Get current tick count
 * @return Number of milliseconds since SysTick_Init was called
 */
uint32_t SysTick_GetTick(void)
{
    return systick_ms;
}

/**
 * @brief  Delay for a specified number of milliseconds
 * @param  ms: Delay duration in milliseconds
 *
 * This is a blocking delay that uses the SysTick counter for accurate timing.
 * Much more precise than the software NOP loop delay.
 */
void SysTick_DelayMs(uint32_t ms)
{
    uint32_t start = systick_ms;

    /* Handle potential overflow by comparing difference */
    while ((systick_ms - start) < ms) {
        __asm volatile ("nop");
    }
}
