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
#include "touch.h"

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

/* Delay in milliseconds (approximate - used during init only) */
static void delay_ms(uint32_t ms)
{
    delay(ms * 24000);
}

/* ============================================================ */
/* SysTick-based timing (uses bootloader's clock, no PLL change) */
/* ============================================================ */
static volatile uint32_t systick_ms = 0;

/* SysTick interrupt handler - called every 1ms */
void SysTick_Handler(void)
{
    systick_ms++;
}

/* Initialize SysTick for 1ms interrupts */
static void SysTick_Init(void)
{
    /* Assume bootloader runs at 192 MHz (based on delay loop calibration)
     * SysTick reload = 192000000 / 1000 - 1 = 191999 for 1ms ticks
     */
    SysTick->LOAD = 191999;
    SysTick->VAL = 0;
    SysTick->CTRL = SysTick_CTRL_CLKSOURCE_Msk |  /* Use processor clock */
                    SysTick_CTRL_TICKINT_Msk |     /* Enable interrupt */
                    SysTick_CTRL_ENABLE_Msk;       /* Enable counter */
}

/* Get current tick count */
static uint32_t SysTick_GetTick(void)
{
    return systick_ms;
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
#define LINE_BELOW_Y    (TIME_Y + TIME_HEIGHT + 48)
#define LINE_MARGIN     120

#define DATE_SCALE      2
#define DATE_CHARS      18  /* "Wed, Feb 05, 2026" */
#define DATE_WIDTH      (DATE_CHARS * 8 * DATE_SCALE)
#define DATE_Y          (TIME_Y + TIME_HEIGHT + 8)

#define STATUS_Y        (LCD_HEIGHT - BAR_HEIGHT + BAR_TEXT_Y)

/* Theme toggle icon position (upper right, below version) */
#define ICON_SIZE       36
#define ICON_X          (LCD_WIDTH - ICON_SIZE - 8)
#define ICON_Y          (BAR_HEIGHT + 8)

/* ============================================================ */
/* Brightness slider layout                                      */
/* ============================================================ */
#define SLIDER_MARGIN_X     50
#define SLIDER_X            SLIDER_MARGIN_X
#define SLIDER_W            (LCD_WIDTH - 2 * SLIDER_MARGIN_X)
#define SLIDER_H            24
#define SLIDER_TRACK_H      6
#define SLIDER_KNOB_W       16
/* Centered vertically within the bottom bar */
#define SLIDER_Y            (LCD_HEIGHT - BAR_HEIGHT + (BAR_HEIGHT - SLIDER_H) / 2)
#define SLIDER_MIN_BRIGHT   5
#define SLIDER_MAX_BRIGHT   100
/* 85% translucent = 15% opaque (~38/255); 20% translucent = 80% opaque (~204/255) */
#define SLIDER_ALPHA_IDLE   38
#define SLIDER_ALPHA_ACTIVE 204

/* ============================================================ */
/* Theme support                                                  */
/* ============================================================ */
static uint8_t night_mode = 0;
static uint8_t manual_theme = 0;  /* Set to 1 when user manually toggles */
static uint8_t slider_active = 0; /* Set to 1 while brightness slider is being touched */

/* Current theme colors (set by apply_theme) */
static uint16_t theme_bg;
static uint16_t theme_time;

/* Track previous time string to avoid flicker (only redraw changed digits) */
static char prev_time_str[12] = "";
static uint16_t theme_date;
static uint16_t theme_bar;
static uint16_t theme_bartext;
static uint16_t theme_line;

/* Blend two RGB565 colors. alpha=0 → fully bg, alpha=255 → fully fg. */
static uint16_t blend565(uint16_t fg, uint16_t bg, uint8_t alpha)
{
    uint8_t r_fg = (fg >> 11) & 0x1F;
    uint8_t g_fg = (fg >> 5)  & 0x3F;
    uint8_t b_fg =  fg        & 0x1F;
    uint8_t r_bg = (bg >> 11) & 0x1F;
    uint8_t g_bg = (bg >> 5)  & 0x3F;
    uint8_t b_bg =  bg        & 0x1F;
    uint8_t r = (uint8_t)((r_fg * alpha + r_bg * (255 - alpha)) / 255);
    uint8_t g = (uint8_t)((g_fg * alpha + g_bg * (255 - alpha)) / 255);
    uint8_t b = (uint8_t)((b_fg * alpha + b_bg * (255 - alpha)) / 255);
    return ((uint16_t)r << 11) | ((uint16_t)g << 5) | b;
}

/*
 * Draw the brightness slider overlaid on the bottom bar.
 * active=0 → 85% translucent (nearly invisible)
 * active=1 → 20% translucent (highly visible)
 */
static void draw_brightness_slider(uint8_t active)
{
    uint8_t brightness = LCD_GetBrightness();
    if (brightness < SLIDER_MIN_BRIGHT) brightness = SLIDER_MIN_BRIGHT;
    if (brightness > SLIDER_MAX_BRIGHT) brightness = SLIDER_MAX_BRIGHT;

    /* Map brightness (5-100) → knob x position (SLIDER_X … SLIDER_X+SLIDER_W) */
    uint16_t knob_x = SLIDER_X +
        (uint16_t)((uint32_t)(brightness - SLIDER_MIN_BRIGHT) * SLIDER_W
                   / (SLIDER_MAX_BRIGHT - SLIDER_MIN_BRIGHT));

    /* Track uses the theme alpha; knob is slightly more opaque for contrast */
    uint8_t track_alpha = active ? SLIDER_ALPHA_ACTIVE : SLIDER_ALPHA_IDLE;
    uint8_t knob_alpha  = active ? 234 : 48;

    uint16_t track_color = blend565(theme_bartext, theme_bar, track_alpha);
    uint16_t knob_color  = blend565(theme_bartext, theme_bar, knob_alpha);

    /* Restore bar background for the slider area first */
    LCD_FillRect(SLIDER_X, SLIDER_Y, SLIDER_W, SLIDER_H, theme_bar);

    /* Draw thin horizontal track */
    uint16_t track_y = SLIDER_Y + (SLIDER_H - SLIDER_TRACK_H) / 2;
    LCD_FillRect(SLIDER_X, track_y, SLIDER_W, SLIDER_TRACK_H, track_color);

    /* Draw knob centered on knob_x, full slider height */
    uint16_t knob_left = (knob_x >= SLIDER_KNOB_W / 2)
                         ? knob_x - SLIDER_KNOB_W / 2 : 0;
    LCD_FillRect(knob_left, SLIDER_Y, SLIDER_KNOB_W, SLIDER_H, knob_color);
}

static void apply_theme(uint8_t night)
{
    night_mode = night;
    prev_time_str[0] = '\0';  /* Force full redraw of time */
    if (night) {
        theme_bg      = COLOR_NIGHT_BG;
        theme_time    = COLOR_NIGHT_TIME;
        theme_date    = COLOR_NIGHT_DATE;
        theme_bar     = COLOR_NIGHT_BAR;
        theme_bartext = COLOR_NIGHT_BARTEXT;
        theme_line    = COLOR_NIGHT_LINE;
        LCD_SetBrightness(20);
    } else {
        theme_bg      = COLOR_DAY_BG;
        theme_time    = COLOR_DAY_TIME;
        theme_date    = COLOR_DAY_DATE;
        theme_bar     = COLOR_DAY_BAR;
        theme_bartext = COLOR_DAY_BARTEXT;
        theme_line    = COLOR_DAY_LINE;
        LCD_SetBrightness(100);
    }
}

/* Draw theme toggle icon (sun for day mode, moon for night mode) */
static void draw_theme_icon(void)
{
    if (night_mode) {
        /* Moon: silvery white on dark background */
        LCD_DrawMoonIcon(ICON_X, ICON_Y, ICON_SIZE, 0xC618, theme_bg);  /* Silver-gray */
    } else {
        /* Sun: bright orange/yellow on light background */
        LCD_DrawSunIcon(ICON_X, ICON_Y, ICON_SIZE, COLOR_ORANGE, theme_bg);
    }
}

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
    LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, 200, BAR_HEIGHT, theme_bar);
    LCD_DrawString(4, STATUS_Y, msg, theme_bartext, theme_bar);
    /* Restore slider which may overlap the cleared area */
    draw_brightness_slider(slider_active);
}

/* Display time string - only updates digits that changed to prevent flicker */
static void display_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    char time_str[12];
    snprintf(time_str, sizeof(time_str), "%02d:%02d:%02d", hours, minutes, seconds);

    /* If first draw or theme changed, draw everything */
    if (prev_time_str[0] == '\0') {
        LCD_DrawStringLarge(TIME_X, TIME_Y, time_str, theme_time, theme_bg, TIME_SCALE);
    } else {
        /* Only redraw characters that changed */
        uint16_t char_width = 8 * TIME_SCALE;
        for (int i = 0; time_str[i] != '\0'; i++) {
            if (time_str[i] != prev_time_str[i]) {
                uint16_t x = TIME_X + i * char_width;
                /* Draw single character with background (overwrites old digit) */
                char single[2] = { time_str[i], '\0' };
                LCD_DrawStringLarge(x, TIME_Y, single, theme_time, theme_bg, TIME_SCALE);
            }
        }
    }

    /* Remember for next update */
    strcpy(prev_time_str, time_str);
}

/* Calculate day of week (0=Sun, 1=Mon, ... 6=Sat) using Zeller-like formula */
static uint8_t day_of_week(uint16_t year, uint8_t month, uint8_t day)
{
    /* Adjust for Jan/Feb (treat as months 13/14 of previous year) */
    if (month < 3) {
        month += 12;
        year--;
    }
    uint16_t k = year % 100;
    uint16_t j = year / 100;
    /* Zeller's congruence for Gregorian calendar */
    int h = (day + (13 * (month + 1)) / 5 + k + k / 4 + j / 4 - 2 * j) % 7;
    /* Convert: Zeller gives 0=Sat, 1=Sun, ... we want 0=Sun */
    return (uint8_t)((h + 6) % 7);
}

/* Display date string */
static void display_date(uint16_t year, uint8_t month, uint8_t day)
{
    static const char *days[] = {
        "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
    };
    static const char *months[] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    char date_str[24];
    const char *dow = days[day_of_week(year, month, day)];
    const char *mon = (month >= 1 && month <= 12) ? months[month - 1] : "???";
    snprintf(date_str, sizeof(date_str), "%s, %s %02d, %04d", dow, mon, day, year);

    /* Calculate width and center */
    uint16_t width = string_width(date_str, DATE_SCALE);
    uint16_t x = (LCD_WIDTH - width) / 2;

    /* Clear date area and redraw */
    LCD_FillRect(x - 4, DATE_Y - 2, width + 8, 16 * DATE_SCALE + 4, theme_bg);
    LCD_DrawStringLarge(x, DATE_Y, date_str, theme_date, theme_bg, DATE_SCALE);
}

/* Draw the static UI elements (bars, lines) */
static void draw_ui(void)
{
    /* Clear screen */
    LCD_Clear(theme_bg);

    /* Draw top bar */
    LCD_FillRect(0, 0, LCD_WIDTH, BAR_HEIGHT, theme_bar);
    LCD_DrawString(4, BAR_TEXT_Y, "TK499", theme_bartext, theme_bar);
    LCD_DrawString((LCD_WIDTH - string_width("NTP Clock", 1)) / 2, BAR_TEXT_Y,
                   "NTP Clock", theme_bartext, theme_bar);
    LCD_DrawString(LCD_WIDTH - string_width("v1.0", 1) - 4, BAR_TEXT_Y,
                   "v1.0", theme_bartext, theme_bar);

    /* Draw bottom bar */
    LCD_FillRect(0, LCD_HEIGHT - BAR_HEIGHT, LCD_WIDTH, BAR_HEIGHT, theme_bar);
    LCD_DrawString((LCD_WIDTH - string_width("TKM32F499 SmartBoard", 1)) / 2,
                   STATUS_Y, "TKM32F499 SmartBoard", theme_bartext, theme_bar);
    LCD_DrawString(LCD_WIDTH - string_width("192MHz", 1) - 4,
                   STATUS_Y, "192MHz", theme_bartext, theme_bar);

    /* Draw separator lines */
    LCD_FillRect(LINE_MARGIN, LINE_ABOVE_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, theme_line);
    LCD_FillRect(LINE_MARGIN, LINE_BELOW_Y, LCD_WIDTH - (LINE_MARGIN * 2), LINE_THICKNESS, theme_line);

    /* Draw theme toggle icon */
    draw_theme_icon();

    /* Draw brightness slider over the bottom bar */
    draw_brightness_slider(slider_active);
}

/* Check if we should be in night mode based on hour (8 PM - 6 AM) */
static uint8_t should_be_night(uint8_t hour)
{
    return (hour >= 20 || hour < 6);
}

/* Update theme based on time if needed (skipped if user manually set theme) */
static void update_theme_for_time(uint8_t hours, uint8_t minutes, uint8_t seconds,
                                  uint16_t year, uint8_t month, uint8_t day)
{
    /* Skip auto switching if user manually toggled the theme */
    if (manual_theme) return;

    uint8_t want_night = should_be_night(hours);
    if (want_night != night_mode) {
        apply_theme(want_night);
        draw_ui();
        display_time(hours, minutes, seconds);
        display_date(year, month, day);
        update_status(night_mode ? "Night mode" : "Day mode");
    }
}

/* Toggle theme manually */
static void toggle_theme(uint8_t hours, uint8_t minutes, uint8_t seconds,
                         uint16_t year, uint8_t month, uint8_t day)
{
    manual_theme = 1;  /* User has manually set the theme */
    apply_theme(!night_mode);
    draw_ui();
    display_time(hours, minutes, seconds);
    display_date(year, month, day);
    update_status(night_mode ? "Night mode" : "Day mode");
}

/* ============================================================ */
/* Main application                                              */
/* ============================================================ */

int main(void)
{
    ESP_Time_t ntp_time;
    int wifi_connected = 0;

    /* Local time keeping - default to midnight so night mode activates on cold start */
    uint8_t hours = 0, minutes = 0, seconds = 0;
    uint16_t year = 2024;
    uint8_t month = 1, day = 1;

    /* CRITICAL: Remap vector table first */
    RemapVtorTable();

    /* Initialize SysTick for accurate 1ms timing */
    SysTick_Init();

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

    /* Initialize touch panel */
    Touch_Init();

    /* Initialize theme based on default time and draw UI */
    apply_theme(should_be_night(hours));
    draw_ui();

    /* Display initial time and date */
    display_time(hours, minutes, seconds);
    display_date(year, month, day);

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
                year = ntp_time.year;
                month = ntp_time.month;
                day = ntp_time.day;
                /* Apply correct theme for current time */
                update_theme_for_time(hours, minutes, seconds, year, month, day);
                display_time(hours, minutes, seconds);
                display_date(year, month, day);
                update_status("Time synced");
            } else {
                /* Time sync failed - switch to noon (day mode) so display isn't dim */
                hours = 12;
                apply_theme(0);  /* Force day mode */
                draw_ui();
                display_time(hours, minutes, seconds);
                display_date(year, month, day);
                update_status("Time failed");
            }
        } else {
            /* WiFi failed - switch to noon (day mode) */
            hours = 12;
            apply_theme(0);
            draw_ui();
            display_time(hours, minutes, seconds);
            display_date(year, month, day);
            update_status("WiFi failed");
        }
    } else {
        /* ESP8266 init failed - switch to noon (day mode) */
        hours = 12;
        apply_theme(0);
        draw_ui();
        display_time(hours, minutes, seconds);
        display_date(year, month, day);
        update_status("ESP8266 err");
    }

    /* Turn LED off */
    GPIOA->BSRR = (1 << 24);

    /* Record starting tick for accurate timing */
    uint32_t last_second_tick = SysTick_GetTick();
    uint32_t last_sync_tick = last_second_tick;
    uint32_t last_touch_tick = 0;     /* For icon-tap debounce */
    uint32_t last_touch_read_tick = 0; /* Rate-limits all touch reads (~60 fps) */
    Touch_State_t touch;

    /* Main loop - uses hardware SysTick for precise 1-second intervals */
    while (1) {
        uint32_t current_tick = SysTick_GetTick();

        /* Check if 1 second (1000ms) has elapsed */
        if ((current_tick - last_second_tick) >= 1000) {
            last_second_tick += 1000;  /* Add exactly 1000ms to prevent drift */

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

            /* Check for day/night theme switch at hour boundaries */
            if (minutes == 0 && seconds == 0) {
                update_theme_for_time(hours, minutes, seconds, year, month, day);
            }
        }

        /* Re-sync with NTP every ~10 minutes (600000 ms) if connected */
        if (wifi_connected && (current_tick - last_sync_tick) >= 600000) {
            last_sync_tick = current_tick;
            update_status("NTP sync...");
            if (ESP_GetNTPTime(&ntp_time, TIMEZONE_OFFSET) == ESP_OK) {
                hours = ntp_time.hours;
                minutes = ntp_time.minutes;
                seconds = ntp_time.seconds;
                year = ntp_time.year;
                month = ntp_time.month;
                day = ntp_time.day;
                last_second_tick = SysTick_GetTick();  /* Reset after sync */
                display_date(year, month, day);
                update_status("Synced");
            } else {
                update_status("Sync failed");
            }
        }

        /* Check for touch input at ~60 fps (16ms gate) to avoid flooding the ADC */
        if ((current_tick - last_touch_read_tick) >= 16) {
            last_touch_read_tick = current_tick;
            Touch_Read(&touch);

            if (touch.pressed) {
                /* Priority 1: Brightness slider - continuous tracking, no debounce */
                if (Touch_InRegion(touch.x, touch.y,
                                   SLIDER_X, SLIDER_Y, SLIDER_W, SLIDER_H)) {
                    int32_t rel_x = (int32_t)touch.x - SLIDER_X;
                    if (rel_x < 0) rel_x = 0;
                    if (rel_x > (int32_t)SLIDER_W) rel_x = (int32_t)SLIDER_W;
                    uint8_t new_brightness = SLIDER_MIN_BRIGHT +
                        (uint8_t)((uint32_t)rel_x *
                                  (SLIDER_MAX_BRIGHT - SLIDER_MIN_BRIGHT) / SLIDER_W);
                    LCD_SetBrightness(new_brightness);
                    slider_active = 1;
                    draw_brightness_slider(1);
                    /* Suppress icon-tap debounce while the slider is in use */
                    last_touch_tick = current_tick;
                }
                /* Priority 2: Theme icon tap - 200ms debounce */
                else if ((current_tick - last_touch_tick) >= 200) {
                    if (Touch_InRegion(touch.x, touch.y,
                                       ICON_X - 8, ICON_Y - 8,
                                       ICON_SIZE + 16, ICON_SIZE + 16)) {
                        toggle_theme(hours, minutes, seconds, year, month, day);
                    }
                    last_touch_tick = current_tick;
                }
            } else {
                /* Finger lifted - fade slider back to idle state */
                if (slider_active) {
                    slider_active = 0;
                    draw_brightness_slider(0);
                }
            }
        }

        /* Wait for interrupt - reduces power consumption */
        __asm volatile ("wfi");
    }

    return 0;
}
