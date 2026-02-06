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
#define FLASH_BASE          ((uint32_t)0x70220000)  /* QSPI Flash (memory-mapped) */
#define SRAM_BASE           ((uint32_t)0x70020000)  /* Internal SDRAM */
#define PERIPH_BASE         ((uint32_t)0x40000000)

#define APB1PERIPH_BASE     (PERIPH_BASE)
#define APB2PERIPH_BASE     (PERIPH_BASE + 0x10000)
#define AHB1PERIPH_BASE     (PERIPH_BASE + 0x20000)
#define AHB2PERIPH_BASE     ((uint32_t)0x60000000)

/* TK80 LCD Controller Base Address */
#define TK80_BASE           (AHB2PERIPH_BASE + 0x00000000)

/* GPIO Base Addresses */
#define GPIOA_BASE          (AHB1PERIPH_BASE + 0x0000)
#define GPIOB_BASE          (AHB1PERIPH_BASE + 0x0400)
#define GPIOC_BASE          (AHB1PERIPH_BASE + 0x0800)
#define GPIOD_BASE          (AHB1PERIPH_BASE + 0x0C00)
#define GPIOE_BASE          (AHB1PERIPH_BASE + 0x1000)
#define GPIOF_BASE          (AHB1PERIPH_BASE + 0x1400)

/* RCC Base Address */
#define RCC_BASE            (AHB1PERIPH_BASE + 0x3800)

/* Timer Base Addresses (APB1 timers) */
#define TIM2_BASE           (APB1PERIPH_BASE + 0x0000)
#define TIM3_BASE           (APB1PERIPH_BASE + 0x0400)
#define TIM4_BASE           (APB1PERIPH_BASE + 0x0800)

/* UART Base Addresses (TKM32F499 uses UART, not USART!) */
#define UART1_BASE          (APB2PERIPH_BASE + 0x0800)
#define UART2_BASE          (APB2PERIPH_BASE + 0x0C00)
#define UART3_BASE          (APB2PERIPH_BASE + 0x1000)

/* ============================================================ */
/* GPIO Registers (TKM32F499 has EXTENDED GPIO with 24 pins!)    */
/* ============================================================ */
typedef struct {
    volatile uint32_t CRL;      /* 0x00: Control Register Low (pins 0-7) */
    volatile uint32_t CRH;      /* 0x04: Control Register High (pins 8-15) */
    volatile uint32_t IDR;      /* 0x08: Input Data Register */
    volatile uint32_t ODR;      /* 0x0C: Output Data Register */
    volatile uint32_t BSRR;     /* 0x10: Bit Set/Reset Register */
    volatile uint32_t BRR;      /* 0x14: Bit Reset Register */
    volatile uint32_t LCKR;     /* 0x18: Lock Register */
    uint32_t RESERVED;          /* 0x1C: Reserved */
    volatile uint32_t AFRL;     /* 0x20: Alternate Function Low (pins 0-7) */
    volatile uint32_t AFRH;     /* 0x24: Alternate Function High (pins 8-15) */
    volatile uint32_t CRH_EXT;  /* 0x28: Control Register Extended (pins 16-23) */
    volatile uint32_t BSRR_EXT; /* 0x2C: Bit Set/Reset Extended */
    volatile uint32_t AFRH_EXT; /* 0x30: Alternate Function Extended (pins 16-23) */
} GPIO_TypeDef;

/* ============================================================ */
/* TK80 LCD Controller Registers                                 */
/* ============================================================ */
typedef struct {
    volatile uint32_t CR;       /* Control Register (offset 0x00) */
    volatile uint32_t CFGR1;    /* Configuration Register 1 (offset 0x04) */
    volatile uint32_t CFGR2;    /* Configuration Register 2 (offset 0x08) */
    volatile uint32_t SR;       /* Status Register (offset 0x0C) */
    volatile uint32_t CMDIR;    /* Command Input Register (offset 0x10) */
    volatile uint32_t DINR;     /* Data Input Register (offset 0x14) */
    volatile uint32_t CMDOR;    /* Command Output Register (offset 0x18) */
    uint32_t RESERVED0;         /* Reserved (offset 0x1C) */
    volatile uint32_t DOUTR;    /* Data Output Register (offset 0x20) */
    volatile uint32_t BRDR;     /* Border Register (offset 0x24) */
    uint32_t RESERVED1;         /* Reserved (offset 0x28) */
    uint32_t RESERVED2;         /* Reserved (offset 0x2C) */
    volatile uint32_t CFGR3;    /* Configuration Register 3 (offset 0x30) */
} TK80_TypeDef;

/* ============================================================ */
/* RCC Registers (TKM32F499 specific layout!)                    */
/* ============================================================ */
typedef struct {
    volatile uint32_t CR;           /* offset 0x00 */
    volatile uint32_t PLLCFGR;      /* offset 0x04 */
    volatile uint32_t CFGR;         /* offset 0x08 */
    volatile uint32_t CIR;          /* offset 0x0C */
    volatile uint32_t AHB1RSTR;     /* offset 0x10 */
    volatile uint32_t AHB2RSTR;     /* offset 0x14 */
    volatile uint32_t APB1RSTR;     /* offset 0x18 */
    volatile uint32_t APB2RSTR;     /* offset 0x1C */
    volatile uint32_t AHB1ENR;      /* offset 0x20 */
    volatile uint32_t AHB2ENR;      /* offset 0x24 */
    volatile uint32_t APB1ENR;      /* offset 0x28 */
    volatile uint32_t APB2ENR;      /* offset 0x2C */
    volatile uint32_t BDCR;         /* offset 0x30 */
    volatile uint32_t CSR;          /* offset 0x34 */
    volatile uint32_t PLLLCDCFGR;   /* offset 0x38 */
    volatile uint32_t PLLDCKCFGR;   /* offset 0x3C */
} RCC_TypeDef;

/* ============================================================ */
/* Timer Registers (General Purpose Timers TIM2-TIM4)            */
/* ============================================================ */
typedef struct {
    volatile uint32_t CR1;      /* Control register 1 */
    volatile uint32_t CR2;      /* Control register 2 */
    volatile uint32_t SMCR;     /* Slave mode control register */
    volatile uint32_t DIER;     /* DMA/Interrupt enable register */
    volatile uint32_t SR;       /* Status register */
    volatile uint32_t EGR;      /* Event generation register */
    volatile uint32_t CCMR1;    /* Capture/compare mode register 1 */
    volatile uint32_t CCMR2;    /* Capture/compare mode register 2 */
    volatile uint32_t CCER;     /* Capture/compare enable register */
    volatile uint32_t CNT;      /* Counter */
    volatile uint32_t PSC;      /* Prescaler */
    volatile uint32_t ARR;      /* Auto-reload register */
    uint32_t RESERVED0;
    volatile uint32_t CCR1;     /* Capture/compare register 1 */
    volatile uint32_t CCR2;     /* Capture/compare register 2 */
    volatile uint32_t CCR3;     /* Capture/compare register 3 */
    volatile uint32_t CCR4;     /* Capture/compare register 4 */
    uint32_t RESERVED1;
    volatile uint32_t DCR;      /* DMA control register */
    volatile uint32_t DMAR;     /* DMA address for full transfer */
} TIM_TypeDef;

/* ============================================================ */
/* UART Registers (TKM32F499 specific - NOT STM32 USART!)        */
/* ============================================================ */
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

/* UART CSR (Control/Status) register bits */
#define UART_CSR_TXC        (1 << 0)    /* TX complete/empty, ready to send */
#define UART_CSR_RXAVL      (1 << 1)    /* RX data available */
#define UART_CSR_TXFULL     (1 << 2)    /* TX FIFO full */
#define UART_CSR_TXEMPTY    (1 << 3)    /* TX FIFO empty */

/* UART GCR (General Control) register bits */
#define UART_GCR_UARTEN     (1 << 0)    /* UART enable */
#define UART_GCR_DMAMODE    (1 << 1)    /* DMA mode */
#define UART_GCR_AUTOFLOWEN (1 << 2)    /* Auto flow control enable */
#define UART_GCR_RXEN       (1 << 3)    /* Receiver enable */
#define UART_GCR_TXEN       (1 << 4)    /* Transmitter enable */

/* UART CCR (Character Control) register bits */
#define UART_CCR_PEN        (1 << 0)    /* Parity enable */
#define UART_CCR_PSEL       (1 << 1)    /* Parity select (0=even, 1=odd) */
#define UART_CCR_SPB        (1 << 2)    /* Stop bits (0=1 stop, 1=2 stop) */
#define UART_CCR_BRK        (1 << 3)    /* Break */
#define UART_CCR_CHAR_5BIT  (0 << 4)    /* 5-bit character */
#define UART_CCR_CHAR_6BIT  (1 << 4)    /* 6-bit character */
#define UART_CCR_CHAR_7BIT  (2 << 4)    /* 7-bit character */
#define UART_CCR_CHAR_8BIT  (3 << 4)    /* 8-bit character */

/* GPIO Alternate Function for UART */
#define GPIO_AF_UART1       ((uint8_t)0x08)
#define GPIO_AF_UART2345    ((uint8_t)0x07)

/* ============================================================ */
/* TOUCHPAD Peripheral (Built-in Resistive Touch ADC)            */
/* ============================================================ */
#define TOUCHPAD_BASE       (APB2PERIPH_BASE + 0x6400)

typedef struct {
    volatile uint32_t ADDATA;   /* 0x00: ADC data */
    volatile uint32_t ADCFG;    /* 0x04: ADC configuration */
    volatile uint32_t ADCR;     /* 0x08: ADC control */
    volatile uint32_t ADCHS;    /* 0x0C: ADC channel select */
    volatile uint32_t ADCMPR;   /* 0x10: ADC compare */
    volatile uint32_t ADSTA;    /* 0x14: ADC status */
    volatile uint32_t ADDR0;    /* 0x18: ADC data register 0 */
    volatile uint32_t ADDR1;    /* 0x1C: ADC data register 1 */
    volatile uint32_t ADDR2;    /* 0x20: ADC data register 2 */
    volatile uint32_t ADDR3;    /* 0x24: ADC data register 3 */
    volatile uint32_t ADDR4;    /* 0x28: ADC data register 4 */
    volatile uint32_t ADDR5;    /* 0x2C: ADC data register 5 */
    volatile uint32_t ADDR6;    /* 0x30: ADC data register 6 */
    volatile uint32_t ADDR7;    /* 0x34: ADC data register 7 */
    volatile uint32_t ADDR8;    /* 0x38: ADC data register 8 */
    volatile uint32_t ADDR9;    /* 0x3C: ADC data register 9 */
    uint32_t RESERVED0;         /* 0x40: Reserved */
    uint32_t RESERVED1;         /* 0x44: Reserved */
    volatile uint32_t TPXDR;    /* 0x48: Touch panel X data */
    volatile uint32_t TPYDR;    /* 0x4C: Touch panel Y data */
    volatile uint32_t TPCR;     /* 0x50: Touch panel control */
    volatile uint32_t TPFR;     /* 0x54: Touch panel filter */
    volatile uint32_t TPCSR;    /* 0x58: Touch panel channel select */
} TOUCHPAD_TypeDef;

#define TOUCHPAD    ((TOUCHPAD_TypeDef *)TOUCHPAD_BASE)

/* TOUCHPAD IRQ number */
#define TOUCHPAD_IRQn       86

/* GPIO Alternate Function for Touchpad ADC */
#define GPIO_AF_TOUCHPAD    ((uint8_t)0x0D)

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
#define TIM2    ((TIM_TypeDef *)TIM2_BASE)
#define TIM3    ((TIM_TypeDef *)TIM3_BASE)
#define TIM4    ((TIM_TypeDef *)TIM4_BASE)
#define TK80    ((TK80_TypeDef *)TK80_BASE)
#define UART1   ((UART_TypeDef *)UART1_BASE)
#define UART2   ((UART_TypeDef *)UART2_BASE)
#define UART3   ((UART_TypeDef *)UART3_BASE)

/* ============================================================ */
/* GPIO Configuration Types (STM32F1-style)                      */
/* ============================================================ */
/* GPIO Mode: combines CNF[1:0] and MODE[1:0] into 4-bit value   */
/* Bits [1:0] = MODE, Bits [3:2] = CNF                           */
typedef enum {
    GPIO_Mode_AIN         = 0x00,  /* Analog input */
    GPIO_Mode_IN_FLOATING = 0x04,  /* Floating input */
    GPIO_Mode_IPD         = 0x28,  /* Input pull-down (special) */
    GPIO_Mode_IPU         = 0x48,  /* Input pull-up (special) */
    GPIO_Mode_Out_OD      = 0x14,  /* Output open-drain */
    GPIO_Mode_Out_PP      = 0x10,  /* Output push-pull */
    GPIO_Mode_AF_OD       = 0x1C,  /* Alternate function open-drain */
    GPIO_Mode_AF_PP       = 0x18   /* Alternate function push-pull */
} GPIOMode_TypeDef;

typedef enum {
    GPIO_Speed_10MHz = 0x01,
    GPIO_Speed_2MHz  = 0x02,
    GPIO_Speed_50MHz = 0x03
} GPIOSpeed_TypeDef;

typedef struct {
    uint32_t GPIO_Pin;
    GPIOSpeed_TypeDef GPIO_Speed;
    GPIOMode_TypeDef GPIO_Mode;
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

/* RCC AHB1 Peripheral Clock Enable */
#define RCC_AHBPeriph_GPIOA     ((uint32_t)0x00000001)
#define RCC_AHBPeriph_GPIOB     ((uint32_t)0x00000002)
#define RCC_AHBPeriph_GPIOC     ((uint32_t)0x00000004)
#define RCC_AHBPeriph_GPIOD     ((uint32_t)0x00000008)
#define RCC_AHBPeriph_GPIOE     ((uint32_t)0x00000010)
#define RCC_AHBPeriph_GPIOF     ((uint32_t)0x00000020)

/* RCC AHB2 Peripheral Clock Enable */
#define RCC_AHB2Periph_TK80     ((uint32_t)0x80000000)

/* GPIO Alternate Function for TK80 */
#define GPIO_AF_TK80            ((uint8_t)0x0C)

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
