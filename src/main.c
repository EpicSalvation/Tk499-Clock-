/**
 * TKM32F499 Clock Project - Hello World
 *
 * This is a basic "Hello World" example that blinks an LED
 * to demonstrate the board is working correctly.
 */

#include "tkm32f499.h"
#include "system_tkm32f499.h"

/* LED Pin Configuration - Adjust based on your board */
#define LED_PORT    GPIOA
#define LED_PIN     GPIO_Pin_0

/* Simple delay function */
static void delay_ms(uint32_t ms)
{
    uint32_t i, j;
    for (i = 0; i < ms; i++) {
        for (j = 0; j < 8000; j++) {
            __NOP();
        }
    }
}

/* Initialize GPIO for LED */
static void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStruct;

    /* Enable GPIOA clock */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOA, ENABLE);

    /* Configure LED pin as output */
    GPIO_InitStruct.GPIO_Pin = LED_PIN;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(LED_PORT, &GPIO_InitStruct);
}

/* Toggle LED state */
static void LED_Toggle(void)
{
    if (GPIO_ReadOutputDataBit(LED_PORT, LED_PIN)) {
        GPIO_ResetBits(LED_PORT, LED_PIN);
    } else {
        GPIO_SetBits(LED_PORT, LED_PIN);
    }
}

/**
 * Main function - Entry point
 *
 * Initializes the system and blinks an LED to indicate
 * the "Hello World" is running successfully.
 */
int main(void)
{
    /* Initialize system clock */
    SystemInit();

    /* Initialize LED GPIO */
    LED_Init();

    /* Hello World - Blink LED forever */
    while (1) {
        LED_Toggle();
        delay_ms(500);  /* 500ms delay - LED blinks at 1Hz */
    }

    return 0;
}
