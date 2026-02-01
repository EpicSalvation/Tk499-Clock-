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
 * @brief  Initialize GPIO pin(s)
 * @param  GPIOx: GPIO peripheral (GPIOA, GPIOB, etc.)
 * @param  GPIO_InitStruct: Pointer to init structure
 */
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct)
{
    uint32_t pinpos;
    uint32_t pos;
    uint32_t currentpin;

    for (pinpos = 0; pinpos < 16; pinpos++) {
        pos = ((uint32_t)1) << pinpos;
        currentpin = GPIO_InitStruct->GPIO_Pin & pos;

        if (currentpin == pos) {
            /* Configure Mode */
            GPIOx->MODER &= ~(0x03 << (pinpos * 2));
            GPIOx->MODER |= (((uint32_t)GPIO_InitStruct->GPIO_Mode) << (pinpos * 2));

            if ((GPIO_InitStruct->GPIO_Mode == GPIO_Mode_OUT) ||
                (GPIO_InitStruct->GPIO_Mode == GPIO_Mode_AF)) {
                /* Configure Speed */
                GPIOx->OSPEEDR &= ~(0x03 << (pinpos * 2));
                GPIOx->OSPEEDR |= ((uint32_t)(GPIO_InitStruct->GPIO_Speed) << (pinpos * 2));

                /* Configure Output Type */
                GPIOx->OTYPER &= ~(0x01 << pinpos);
                GPIOx->OTYPER |= (((uint16_t)GPIO_InitStruct->GPIO_OType) << pinpos);
            }

            /* Configure Pull-up/Pull-down */
            GPIOx->PUPDR &= ~(0x03 << (pinpos * 2));
            GPIOx->PUPDR |= (((uint32_t)GPIO_InitStruct->GPIO_PuPd) << (pinpos * 2));
        }
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
