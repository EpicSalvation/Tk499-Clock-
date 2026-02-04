/**
 * TKM32F499 Clock Application
 *
 * Displays time on the 4.3" LCD screen with NTP sync via ESP8266 WiFi.
 * Build with: make clock
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "tkm32f499.h"
#include "lcd.h"
#include "esp8266.h"

/* ============================================================ */
/* WiFi Configuration - EDIT THESE!                              */
/* ============================================================ */
#define WIFI_SSID       "DeezNutz"
#define WIFI_PASSWORD   "biteme69"
#define TIMEZONE_OFFSET -5  /* Hours from UTC (e.g., -5 for EST, -8 for PST) */

/* ============================================================ */
/* Core registers                                                */
/* ============================================================ */
#define SCB_VTOR    (*(volatile uint32_t *)0xE000ED08)
#define NVIC_ICER   ((volatile uint32_t *)0xE000E180)

/* Memory addresses */
#define T_SRAM_BASE  0x20000000
#define T_SDRAM_BASE 0x70020000

/* Remap vector table - CRITICAL for code to run */
static void RemapVtorTable(void)
{
    int i;

    /* Enable internal SRAM clock (bit 13) */
    RCC->AHB1ENR |= (1 << 13);

    /* Disable all interrupts */
    for (i = 0; i < 3; i++) {
        NVIC_ICER[i] = 0xFFFFFFFF;
    }

    /* Set VTOR to internal SRAM with bit 29 */
    SCB_VTOR = 0;
    SCB_VTOR |= (1 << 29);

    /* Copy vector table from SDRAM to SRAM */
    for (i = 0; i < 512; i += 4) {
        *(volatile uint32_t *)(T_SRAM_BASE + i) = *(volatile uint32_t *)(T_SDRAM_BASE + i);
    }
}

/* Simple delay */
static void delay(volatile uint32_t count)
{
    while (count--) {
        __asm volatile ("nop");
    }
}

/* Delay in milliseconds (approximate) */
static void delay_ms(uint32_t ms)
{
    delay(ms * 24000);
}

/* ============================================================ */
/* Layout constants                                              */
/* ============================================================ */
#define BAR_HEIGHT      32
#define BAR_TEXT_Y      8
#define LINE_THICKNESS  3
#define TIME_SCALE      6
#define TIME_CHARS      8

#define TIME_WIDTH      (TIME_CHARS * 8 * TIME_SCALE)
#define TIME_HEIGHT     (16 * TIME_SCALE)
#define TIME_X          ((LCD_WIDTH - TIME_WIDTH) / 2)
#define TIME_Y          ((LCD_HEIGHT - TIME_HEIGHT) / 2)

#define LINE_ABOVE_Y    (TIME_Y - 16)
#define LINE_BELOW_Y    (TIME_Y + TIME_HEIGHT + 12)
#define LINE_MARGIN     120

#define STATUS_Y        (LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y)

/* ============================================================ */
/* Helper functions                                              */
/* ============================================================ */

static uint16_t string_width(const char *str, uint8_t scale)
{
    uint16_t len = 0;
    while (*str++) len++;
    return len * 8 * scale;
}

/* Update status message in bottom bar */
static void update_status(const char *msg)
{
    /* Clear left portion of bottom bar */
    LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, 200, BAR_HEIGHT, COLOR_ORANGE);
    LCD_DrawString(4, STATUS_Y, msg, COLOR_BLACK, COLOR_ORANGE);
}

/* Display time string */
static void display_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);

    /* Clear time area and redraw */
    LCD_FillRect(TIME_X - 4, TIME_Y - 4, TIME_WIDTH + 8, TIME_HEIGHT + 8, COLOR_BLACK);
    LCD_DrawStringLarge(TIME_X, TIME_Y, time_str, COLOR_GREEN, COLOR_BLACK, TIME_SCALE);
}

/* ============================================================ */
/* Main application                                              */
/* ============================================================ */

int main(void)
{
    ESP_Time_t ntp_time;
    int wifi_connected = 0;
    uint32_t sync_counter = 0;

    /* Local time keeping */
    uint8_t hours = 12, minutes = 0, seconds = 0;

    /* CRITICAL: Remap vector table first */
    RemapVtorTable();

    /* Enable GPIO clocks */
    RCC->AHB1ENR |= (1 << 0) | (1 << 1) | (1 << 2) | (1 << 3) | (1 << 4);
    delay(1000);

    /* Configure PA8 as output for LED */
    GPIOA->CRH &= ~(0x0F << 0);
    GPIOA->CRH |= (0x03 << 0);

    /* Debug: 2 quick blinks */
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);
    GPIOA->BSRR = (1 << 8); delay(500000); GPIOA->BSRR = (1 << 24); delay(500000);

    /* Initialize the LCD */
    LCD_Init();
    LCD_BrightnessInit();
    LCD_SetBrightness(100);

    /* Clear screen to black */
    LCD_Clear(COLOR_BLACK);

    /* Draw top bar (blue) */
    LCD_FillRect(0, 0, LCD_WIDTH, BAR_HEIGHT, COLOR_BLUE);
    LCD_DrawString(4, BAR_TEXT_Y, "TK499", COLOR_WHITE, COLOR_BLUE);
    LCD_DrawString((LCD_WIDTH - string_width("NTP Clock", 1)) / 2, BAR_TEXT_Y,
                   "NTP Clock", COLOR_CYAN, COLOR_BLUE);
    LCD_DrawString(LCD_WIDTH - string_width("v1.0", 1) - 4, BAR_TEXT_Y,
                   "v1.0", COLOR_WHITE, COLOR_BLUE);

    /* Draw bottom bar (orange) */
    LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, LCD_WIDTH, BAR_HEIGHT, COLOR_ORANGE);
    update_status("Starting...");
    LCD_DrawString((LCD_WIDTH - string_width("TKM32F499 SmartBoard", 1)) / 2,
                   STATUS_Y, "TKM32F499 SmartBoard", COLOR_BLACK, COLOR_ORANGE);
    LCD_DrawString(LCD_WIDTH - string_width("240MHz", 1) - 4,
                   STATUS_Y, "240MHz", COLOR_BLACK, COLOR_ORANGE);

    /* Draw separator lines */
    LCD_FillRect(LINE_MARGIN, LINE_ABOVE_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);
    LCD_FillRect(LINE_MARGIN, LINE_BELOW_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, COLOR_WHITE);

    /* Display initial time */
    display_time(hours, minutes, seconds);

    /* Initialize ESP8266 */
    update_status("Init WiFi...");
    delay_ms(500);

    if (ESP_Init() == ESP_OK) {
        update_status("WiFi OK");
        delay_ms(500);

        /* Connect to WiFi */
        update_status("Connecting...");
        if (ESP_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD) == ESP_OK) {
            wifi_connected = 1;
            update_status("Connected!");
            delay_ms(500);

            /* Show IP address before getting time */
            char ip_buf[20];
            if (ESP_GetIP(ip_buf) == ESP_OK) {
                update_status(ip_buf);
            } else {
                update_status("No IP!");
            }
            delay_ms(3000);  /* Show IP for 3 seconds */

            /* Get time from HTTP API */
            update_status("Getting time...");
            if (ESP_GetNTPTime(&ntp_time, TIMEZONE_OFFSET) == ESP_OK) {
                hours = ntp_time.hours;
                minutes = ntp_time.minutes;
                seconds = ntp_time.seconds;
                display_time(hours, minutes, seconds);
                update_status("Time synced");
            } else {
                update_status("Time failed");
            }
        } else {
            update_status("WiFi failed");
        }
    } else {
        update_status("ESP8266 err");
    }

    /* Main loop */
    while (1) {
        /* Toggle LED */
        GPIOA->ODR ^= (1 << 8);

        /* Simple ~1 second delay */
        delay_ms(1000);

        /* Increment time */
        seconds++;
        if (seconds >= 60) {
            seconds = 0;
            minutes++;
            if (minutes >= 60) {
                minutes = 0;
                hours++;
                if (hours >= 24) {
                    hours = 0;
                }
            }
        }

        /* Update display */
        display_time(hours, minutes, seconds);

        /* Re-sync with NTP every ~10 minutes if connected */
        sync_counter++;
        if (wifi_connected && sync_counter >= 600) {
            sync_counter = 0;
            update_status("NTP sync...");
            if (ESP_GetNTPTime(&ntp_time, TIMEZONE_OFFSET) == ESP_OK) {
                hours = ntp_time.hours;
                minutes = ntp_time.minutes;
                seconds = ntp_time.seconds;
                update_status("Synced");
            } else {
                update_status("Sync failed");
            }
        }
    }

    return 0;
}
