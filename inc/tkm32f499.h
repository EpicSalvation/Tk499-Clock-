/**
 * TKM32F499 Main Header File
 *
 * This header provides basic definitions and includes for the TKM32F499
 * microcontroller (ARM Cortex-M4 based).
 */

#ifndef __TKM32F499_H
#define __TKM32F499_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ============================================================ */
/* Interrupt Number Definition                                   */
/* ============================================================ */
typedef enum IRQn {
    /* Cortex-M4 Processor Exceptions */
    NonMaskableInt_IRQn     = -14,
    HardFault_IRQn          = -13,
    MemoryManagement_IRQn   = -12,
    BusFault_IRQn           = -11,
    UsageFault_IRQn         = -10,
    SVCall_IRQn             = -5,
    DebugMonitor_IRQn       = -4,
    PendSV_IRQn             = -2,
    SysTick_IRQn            = -1,

    /* TKM32F499 Specific Interrupts */
    WWDG_IRQn               = 0,
    PVD_IRQn                = 1,
    TAMPER_IRQn             = 2,
    RTC_IRQn                = 3,
    FLASH_IRQn              = 4,
    RCC_IRQn                = 5,
    EXTI0_IRQn              = 6,
    EXTI1_IRQn              = 7,
    EXTI2_IRQn              = 8,
    EXTI3_IRQn              = 9,
    EXTI4_IRQn              = 10,
    DMA1_Channel1_IRQn      = 11,
    DMA1_Channel2_IRQn      = 12,
    DMA1_Channel3_IRQn      = 13,
    DMA1_Channel4_IRQn      = 14,
    DMA1_Channel5_IRQn      = 15,
    DMA1_Channel6_IRQn      = 16,
    DMA1_Channel7_IRQn      = 17,
    ADC1_2_IRQn             = 18,
    USB_HP_CAN1_TX_IRQn     = 19,
    USB_LP_CAN1_RX0_IRQn    = 20,
    CAN1_RX1_IRQn           = 21,
    CAN1_SCE_IRQn           = 22,
    EXTI9_5_IRQn            = 23,
    TIM1_BRK_IRQn           = 24,
    TIM1_UP_IRQn            = 25,
    TIM1_TRG_COM_IRQn       = 26,
    TIM1_CC_IRQn            = 27,
    TIM2_IRQn               = 28,
    TIM3_IRQn               = 29,
    TIM4_IRQn               = 30,
    I2C1_EV_IRQn            = 31,
    I2C1_ER_IRQn            = 32,
    I2C2_EV_IRQn            = 33,
    I2C2_ER_IRQn            = 34,
    SPI1_IRQn               = 35,
    SPI2_IRQn               = 36,
    USART1_IRQn             = 37,
    USART2_IRQn             = 38,
    USART3_IRQn             = 39,
    EXTI15_10_IRQn          = 40,
    RTCAlarm_IRQn           = 41,
    USBWakeUp_IRQn          = 42,
} IRQn_Type;

/* ============================================================ */
/* CMSIS Core Configuration                                      */
/* ============================================================ */
#define __CM4_REV               0x0001
#define __NVIC_PRIO_BITS        4
#define __Vendor_SysTickConfig  0
#define __MPU_PRESENT           1
#define __FPU_PRESENT           1

#include "core_cm4.h"

/* ============================================================ */
/* Peripheral Base Addresses                                     */
/* ============================================================ */
#define FLASH_BASE          ((uint32_t)0x08000000)
#define SRAM_BASE           ((uint32_t)0x20000000)
#define PERIPH_BASE         ((uint32_t)0x40000000)

#define APB1PERIPH_BASE     (PERIPH_BASE)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x10000)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x20000)

/* GPIO Base Addresses */
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE          (AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASE          (AHB1PERIPH_BASE + 0x1400)

/* RCC Base Address */
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800)

/* ============================================================ */
/* GPIO Registers                                                */
/* ============================================================ */
typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

/* ============================================================ */
/* RCC Registers                                                 */
/* ============================================================ */
typedef struct {
    volatile uint32_t CR;
    volatile uint32_t PLLCFGR;
    volatile uint32_t CFGR;
    volatile uint32_t CIR;
    volatile uint32_t AHB1RSTR;
    volatile uint32_t AHB2RSTR;
    volatile uint32_t AHB3RSTR;
    uint32_t RESERVED0;
    volatile uint32_t APB1RSTR;
    volatile uint32_t APB2RSTR;
    uint32_t RESERVED1[2];
    volatile uint32_t AHB1ENR;
    volatile uint32_t AHB2ENR;
    volatile uint32_t AHB3ENR;
    uint32_t RESERVED2;
    volatile uint32_t APB1ENR;
    volatile uint32_t APB2ENR;
} RCC_TypeDef;

/* ============================================================ */
/* Peripheral Declarations                                       */
/* ============================================================ */
#define GPIOA   ((GPIO_TypeDef *)GPIOA_BASE)
#define GPIOB   ((GPIO_TypeDef *)GPIOB_BASE)
#define GPIOC   ((GPIO_TypeDef *)GPIOC_BASE)
#define GPIOD   ((GPIO_TypeDef *)GPIOD_BASE)
#define GPIOE   ((GPIO_TypeDef *)GPIOE_BASE)
#define GPIOF   ((GPIO_TypeDef *)GPIOF_BASE)
#define RCC     ((RCC_TypeDef *)RCC_BASE)

/* ============================================================ */
/* GPIO Configuration Types                                      */
/* ============================================================ */
typedef enum {
    GPIO_Mode_IN   = 0x00,
    GPIO_Mode_OUT  = 0x01,
    GPIO_Mode_AF   = 0x02,
    GPIO_Mode_AN   = 0x03
} GPIOMode_TypeDef;

typedef enum {
    GPIO_OType_PP = 0x00,
    GPIO_OType_OD = 0x01
} GPIOOType_TypeDef;

typedef enum {
    GPIO_Speed_2MHz   = 0x00,
    GPIO_Speed_25MHz  = 0x01,
    GPIO_Speed_50MHz  = 0x02,
    GPIO_Speed_100MHz = 0x03
} GPIOSpeed_TypeDef;

typedef enum {
    GPIO_PuPd_NOPULL = 0x00,
    GPIO_PuPd_UP     = 0x01,
    GPIO_PuPd_DOWN   = 0x02
} GPIOPuPd_TypeDef;

typedef struct {
    uint32_t GPIO_Pin;
    GPIOMode_TypeDef GPIO_Mode;
    GPIOSpeed_TypeDef GPIO_Speed;
    GPIOOType_TypeDef GPIO_OType;
    GPIOPuPd_TypeDef GPIO_PuPd;
} GPIO_InitTypeDef;

/* GPIO Pin Definitions */
#define GPIO_Pin_0      ((uint16_t)0x0001)
#define GPIO_Pin_1      ((uint16_t)0x0002)
#define GPIO_Pin_2      ((uint16_t)0x0004)
#define GPIO_Pin_3      ((uint16_t)0x0008)
#define GPIO_Pin_4      ((uint16_t)0x0010)
#define GPIO_Pin_5      ((uint16_t)0x0020)
#define GPIO_Pin_6      ((uint16_t)0x0040)
#define GPIO_Pin_7      ((uint16_t)0x0080)
#define GPIO_Pin_8      ((uint16_t)0x0100)
#define GPIO_Pin_9      ((uint16_t)0x0200)
#define GPIO_Pin_10     ((uint16_t)0x0400)
#define GPIO_Pin_11     ((uint16_t)0x0800)
#define GPIO_Pin_12     ((uint16_t)0x1000)
#define GPIO_Pin_13     ((uint16_t)0x2000)
#define GPIO_Pin_14     ((uint16_t)0x4000)
#define GPIO_Pin_15     ((uint16_t)0x8000)
#define GPIO_Pin_All    ((uint16_t)0xFFFF)

/* RCC AHB Peripheral Clock Enable */
#define RCC_AHBPeriph_GPIOA     ((uint32_t)0x00000001)
#define RCC_AHBPeriph_GPIOB     ((uint32_t)0x00000002)
#define RCC_AHBPeriph_GPIOC     ((uint32_t)0x00000004)
#define RCC_AHBPeriph_GPIOD     ((uint32_t)0x00000008)
#define RCC_AHBPeriph_GPIOE     ((uint32_t)0x00000010)
#define RCC_AHBPeriph_GPIOF     ((uint32_t)0x00000020)

/* ============================================================ */
/* Function Prototypes                                           */
/* ============================================================ */
void GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_InitStruct);
void GPIO_SetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void GPIO_ResetBits(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
uint8_t GPIO_ReadOutputDataBit(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void RCC_AHBPeriphClockCmd(uint32_t RCC_AHBPeriph, int NewState);

#define ENABLE  1
#define DISABLE 0

#ifdef __cplusplus
}
#endif

#endif /* __TKM32F499_H */
