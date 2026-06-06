/**
 * TKM32F499 Startup File (GCC)
 *
 * Minimal startup - matches reference Keil behavior
 */

    .syntax unified
    .cpu cortex-m4
    .fpu softvfp
    .thumb

.global g_pfnVectors
.global Default_Handler
.global Reset_Handler

/**
 * @brief  Reset Handler - Entry point after reset
 */
    .section .text.Reset_Handler
    .weak Reset_Handler
    .type Reset_Handler, %function
Reset_Handler:
    /* Enable FPU (CP10 and CP11) */
    ldr r0, =0xE000ED88
    ldr r1, [r0]
    orr r1, r1, #(0xF << 20)
    str r1, [r0]

    /* Set stack pointer */
    ldr sp, =_estack

    /* Zero BSS so static variables start with their declared initial value.
     * Without this, static uint8_t dst_enabled = 0 retains whatever residual
     * value the SDRAM had, which often makes TIMEZONE_OFFSET + dst_enabled
     * evaluate to 0 instead of -5, causing the clock to display UTC. */
    ldr  r0, =_sbss
    ldr  r1, =_ebss
    mov  r2, #0
bss_zero:
    cmp  r0, r1
    bge  bss_done
    str  r2, [r0], #4
    b    bss_zero
bss_done:

    /* Jump directly to main - no C library init needed for bare metal */
    bl main

    /* If main returns, loop forever */
hang:
    b hang

.size Reset_Handler, .-Reset_Handler

/**
 * @brief  Default Handler for unhandled exceptions
 */
    .section .text.Default_Handler,"ax",%progbits
Default_Handler:
Infinite_Loop:
    b Infinite_Loop
    .size Default_Handler, .-Default_Handler

/**
 * @brief  Vector Table
 */
    .section .isr_vector,"a",%progbits
    .type g_pfnVectors, %object

g_pfnVectors:
    .word _estack
    .word Reset_Handler
    .word NMI_Handler
    .word HardFault_Handler
    .word MemManage_Handler
    .word BusFault_Handler
    .word UsageFault_Handler
    .word 0
    .word 0
    .word 0
    .word 0
    .word SVC_Handler
    .word DebugMon_Handler
    .word 0
    .word PendSV_Handler
    .word SysTick_Handler

    /* External Interrupts - fill with defaults */
    .word Default_Handler  /* 0 */
    .word Default_Handler  /* 1 */
    .word Default_Handler  /* 2 */
    .word Default_Handler  /* 3 */
    .word Default_Handler  /* 4 */
    .word Default_Handler  /* 5 */
    .word Default_Handler  /* 6 */
    .word Default_Handler  /* 7 */
    .word Default_Handler  /* 8 */
    .word Default_Handler  /* 9 */
    .word Default_Handler  /* 10 */
    .word Default_Handler  /* 11 */
    .word Default_Handler  /* 12 */
    .word Default_Handler  /* 13 */
    .word Default_Handler  /* 14 */
    .word Default_Handler  /* 15 */
    .word Default_Handler  /* 16 */
    .word Default_Handler  /* 17 */
    .word Default_Handler  /* 18 */
    .word Default_Handler  /* 19 */
    .word Default_Handler  /* 20 */
    .word Default_Handler  /* 21 */
    .word Default_Handler  /* 22 */
    .word Default_Handler  /* 23 */
    .word Default_Handler  /* 24 */
    .word Default_Handler  /* 25 */
    .word Default_Handler  /* 26 */
    .word Default_Handler  /* 27 */
    .word Default_Handler  /* 28: TIM2 */
    .word TIM3_IRQHandler  /* 29: TIM3 */
    .word Default_Handler  /* 30: TIM4 */
    .word Default_Handler  /* 31 */

.size g_pfnVectors, .-g_pfnVectors

/**
 * @brief  Weak aliases for exception handlers
 */
    .weak NMI_Handler
    .thumb_set NMI_Handler, Default_Handler

    .weak HardFault_Handler
    .thumb_set HardFault_Handler, Default_Handler

    .weak MemManage_Handler
    .thumb_set MemManage_Handler, Default_Handler

    .weak BusFault_Handler
    .thumb_set BusFault_Handler, Default_Handler

    .weak UsageFault_Handler
    .thumb_set UsageFault_Handler, Default_Handler

    .weak SVC_Handler
    .thumb_set SVC_Handler, Default_Handler

    .weak DebugMon_Handler
    .thumb_set DebugMon_Handler, Default_Handler

    .weak PendSV_Handler
    .thumb_set PendSV_Handler, Default_Handler

    .weak SysTick_Handler
    .thumb_set SysTick_Handler, Default_Handler

    .weak TIM3_IRQHandler
    .thumb_set TIM3_IRQHandler, Default_Handler

    .end
