/**
 * TKM32F499 System Source File
 *
 * System initialization and peripheral driver implementations.
 */

#include "tkm32f499.h"
#include "system_tkm32f499.h"

/* System Core Clock (default to 72MHz) */
uint32_t SystemCoreClock = 72000000;

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
