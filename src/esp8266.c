/**
 * ESP8266 WiFi Module Driver
 *
 * Uses UART2 for communication with ESP8266 AT firmware.
 * TKM32F499 uses a different UART peripheral than STM32!
 */

#include "esp8266.h"
#include "tkm32f499.h"
#include <string.h>
#include <stdio.h>

/* Simple delay */
static void esp_delay(volatile uint32_t count)
{
    while (count--) {
        __asm volatile ("nop");
    }
}

/* Millisecond delay (approximate at 192MHz - loop ~4 cycles) */
static void delay_ms(uint32_t ms)
{
    esp_delay(ms * 48000);
}

/* Send a single byte via UART2 */
static void UART2_SendByte(uint8_t byte)
{
    /* Wait for TX buffer empty */
    while (!(UART2->CSR & UART_CSR_TXC));
    UART2->TDR = byte;
    /* Wait for transmission complete */
    while (!(UART2->CSR & UART_CSR_TXC));
}

/* Send a string via UART2 */
static void UART2_SendString(const char *str)
{
    while (*str) {
        UART2_SendByte(*str++);
    }
}

/* Receive with timeout, returns -1 on timeout */
static int UART2_ReceiveByteTimeout(uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 1000;
    while (timeout--) {
        if (UART2->CSR & UART_CSR_RXAVL) {
            /* Small delay to ensure byte is fully received */
            esp_delay(100);
            return (uint8_t)UART2->RDR;
        }
        esp_delay(24);  /* ~1us */
    }
    return -1;
}

/* Clear receive buffer with timeout */
static void ESP_ClearRxBuffer(void)
{
    uint32_t timeout = 10000;
    while ((UART2->CSR & UART_CSR_RXAVL) && timeout--) {
        (void)UART2->RDR;
    }
}

/* Read response until timeout or pattern found */
static int ESP_ReadResponse(char *buffer, uint16_t max_len, uint32_t timeout_ms, const char *end_pattern)
{
    uint16_t idx = 0;
    int byte;

    while (idx < max_len - 1) {
        byte = UART2_ReceiveByteTimeout(timeout_ms);
        if (byte < 0) {
            break;  /* Timeout */
        }
        buffer[idx++] = (char)byte;
        buffer[idx] = '\0';

        /* Check for end pattern */
        if (end_pattern && strstr(buffer, end_pattern)) {
            break;
        }
        /* Also check for ERROR */
        if (strstr(buffer, "ERROR")) {
            break;
        }
    }

    return idx;
}

/* Initialize UART2 for ESP8266 communication */
static void UART2_Init(uint32_t baudrate)
{
    /*
     * TKM32F499 UART2 initialization
     * APB2 clock assumed to be 120MHz (sysclk/2)
     */

    /* Enable UART2 clock on APB2ENR bit 3 */
    RCC->APB2ENR |= (1 << 3);  /* RCC_APB2Periph_UART2 = 0x00000008 */

    /* Enable GPIOA clock */
    RCC->AHB1ENR |= (1 << 0);

    /* Small delay for clock to stabilize */
    for (volatile int i = 0; i < 10000; i++);

    /*
     * Configure PA2 (TX) and PA3 (RX) for UART2
     * Need to set alternate function AF7 (GPIO_AF_UART2345)
     */

    /* PA2: AF push-pull output for TX */
    GPIOA->CRL &= ~(0xF << 8);    /* Clear PA2 bits */
    GPIOA->CRL |= (0x0B << 8);    /* AF push-pull, 50MHz */

    /* PA3: Input with pull-up for RX */
    GPIOA->CRL &= ~(0xF << 12);   /* Clear PA3 bits */
    GPIOA->CRL |= (0x08 << 12);   /* Input pull-up/down */
    GPIOA->ODR |= (1 << 3);       /* Pull-up */

    /* Set alternate function for PA2 and PA3 to AF7 (UART2/3/4/5) */
    /* AFRL: 4 bits per pin, PA2 = bits [11:8], PA3 = bits [15:12] */
    GPIOA->AFRL &= ~(0xFF << 8);           /* Clear AF for PA2 and PA3 */
    GPIOA->AFRL |= (GPIO_AF_UART2345 << 8);   /* PA2 = AF7 */
    GPIOA->AFRL |= (GPIO_AF_UART2345 << 12);  /* PA3 = AF7 */

    /* Configure UART2 */
    UART2->GCR = 0;  /* Disable UART first */

    /* Set baud rate - try 192MHz (bootloader may not use 240MHz) */
    /* Formula from HAL: BRR = CLK / baudrate / 16 */
    /*                   FRABRG = (CLK / baudrate) % 16 */
    uint32_t sys_clk = 192000000;  /* Try 192MHz */
    uint32_t div = sys_clk / baudrate;
    UART2->BRR = div / 16;
    UART2->FRABRG = div % 16;

    /* 8 data bits, 1 stop bit, no parity */
    UART2->CCR = UART_CCR_CHAR_8BIT;  /* 8-bit character */

    /* Enable UART, TX, and RX */
    UART2->GCR = UART_GCR_UARTEN | UART_GCR_TXEN | UART_GCR_RXEN;

    /* Small delay for UART to stabilize */
    for (volatile int j = 0; j < 10000; j++);

    /* UART2 is now ready */
}

int ESP_Init(void)
{
    int i;

    /* Enable GPIOD clock */
    RCC->AHB1ENR |= (1 << 3);

    /* Configure PD0 (RST) and PD1 (CH_PD) as push-pull outputs */
    /* Per reference code: PD0=RST, PD1=CH_PD */
    GPIOD->CRL &= ~(0xFF << 0);   /* Clear PD0 and PD1 */
    GPIOD->CRL |= (0x33 << 0);    /* Output push-pull, 50MHz */

    /* Set CH_PD high (enable ESP8266) */
    GPIOD->BSRR = (1 << 1);
    /* Set RST high (not in reset) */
    GPIOD->BSRR = (1 << 0);

    /* Initialize UART2 at 115200 baud */
    UART2_Init(115200);

    /* Reset ESP8266 */
    GPIOD->BSRR = (1 << 16);  /* PD0 low (reset) */
    delay_ms(100);
    GPIOD->BSRR = (1 << 0);   /* PD0 high (release) */

    /* Wait for ESP8266 to boot (needs 2-3 seconds) */
    delay_ms(4000);
    ESP_ClearRxBuffer();

    /* Try AT command - more retries for reliability */
    for (i = 0; i < 10; i++) {
        if (ESP_Test() == ESP_OK) {
            /* Disable echo to get cleaner responses */
            ESP_SendCommand("ATE0", NULL, 0, 1000);
            return ESP_OK;
        }
        delay_ms(1000);
        ESP_ClearRxBuffer();
    }

    return ESP_ERROR;
}

void ESP_Reset(void)
{
    /* Pull RST low */
    GPIOD->BSRR = (1 << 16);  /* PD0 low */
    delay_ms(200);

    /* Release RST */
    GPIOD->BSRR = (1 << 0);   /* PD0 high */
    delay_ms(500);  /* ESP8266 needs time to boot */
}

int ESP_SendCommand(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms)
{
    char cmd_buf[128];

    /* Clear any pending data */
    ESP_ClearRxBuffer();

    /* Build command with AT prefix and CRLF */
    if (cmd[0] == 'A' && cmd[1] == 'T') {
        /* Already has AT prefix */
        snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", cmd);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+%s\r\n", cmd);
    }

    /* Send command */
    UART2_SendString(cmd_buf);

    /* Wait for response */
    char local_buf[256];
    char *buf = response ? response : local_buf;
    uint16_t size = response ? resp_size : sizeof(local_buf);

    int len = ESP_ReadResponse(buf, size, timeout_ms, "OK");

    /* Check for various success patterns */
    if (len > 0) {
        /* Check for OK (case variations) */
        if (strstr(buf, "OK") || strstr(buf, "ok") || strstr(buf, "Ok")) {
            return ESP_OK;
        }
        /* Check for ready (after boot) */
        if (strstr(buf, "ready") || strstr(buf, "READY")) {
            return ESP_OK;
        }
        /* Check for ERROR */
        if (strstr(buf, "ERROR") || strstr(buf, "error")) {
            return ESP_ERROR;
        }
        /* Got some response but no recognized pattern - might still be OK */
        /* If we got more than 2 chars, consider it a response */
        if (len > 2) {
            return ESP_OK;  /* Assume success if we got any substantial response */
        }
    }

    return ESP_TIMEOUT;
}

int ESP_Test(void)
{
    return ESP_SendCommand("AT", NULL, 0, 1000);
}

int ESP_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];
    char response[256];
    int result;

    /* Set WiFi mode to Station */
    result = ESP_SendCommand("CWMODE=1", NULL, 0, 1000);
    if (result != ESP_OK) {
        return result;
    }

    delay_ms(100);

    /* Connect to AP */
    snprintf(cmd, sizeof(cmd), "CWJAP=\"%s\",\"%s\"", ssid, password);
    result = ESP_SendCommand(cmd, response, sizeof(response), 15000);

    return result;
}

int ESP_DisconnectWiFi(void)
{
    return ESP_SendCommand("CWQAP", NULL, 0, 1000);
}

int ESP_IsConnected(void)
{
    char response[128];

    if (ESP_SendCommand("CWJAP?", response, sizeof(response), 2000) == ESP_OK) {
        /* If connected, response contains SSID */
        if (strstr(response, "No AP") || strstr(response, "ERROR")) {
            return 0;
        }
        return 1;
    }
    return 0;
}

int ESP_GetIP(char *ip_buf)
{
    char response[128];

    if (ESP_SendCommand("CIFSR", response, sizeof(response), 2000) == ESP_OK) {
        /* Parse IP from response: +CIFSR:STAIP,"x.x.x.x" */
        char *start = strstr(response, "STAIP,\"");
        if (start) {
            start += 7;
            char *end = strchr(start, '"');
            if (end) {
                int len = end - start;
                if (len < 16) {
                    memcpy(ip_buf, start, len);
                    ip_buf[len] = '\0';
                    return ESP_OK;
                }
            }
        }
    }
    return ESP_ERROR;
}

int ESP_GetNTPTime(ESP_Time_t *time, int8_t timezone_offset)
{
    char response[700];  /* Need space for full HTTP response (~644 bytes) */
    int i;

    /* Check if we have a valid IP (WiFi connected) */
    if (ESP_SendCommand("CIFSR", response, sizeof(response), 5000) != ESP_OK) {
        return ESP_ERROR;
    }
    if (strstr(response, "0.0.0.0") || strstr(response, "ERROR")) {
        return ESP_ERROR;  /* Not connected to WiFi */
    }

    /* Set single connection mode */
    ESP_SendCommand("CIPMUX=0", NULL, 0, 1000);

    /* Close any existing connection */
    ESP_SendCommand("CIPCLOSE", NULL, 0, 500);
    delay_ms(200);

    /* Connect to worldclockapi.com for UTC time */
    if (ESP_SendCommand("CIPSTART=\"TCP\",\"worldclockapi.com\",80", response, sizeof(response), 10000) != ESP_OK) {
        if (!strstr(response, "CONNECT") && !strstr(response, "Linked")) {
            return ESP_ERROR;
        }
    }

    /* Send CIPSEND command - 59 bytes for our HTTP request */
    ESP_ClearRxBuffer();
    UART2_SendString("AT+CIPSEND=59\r\n");

    /* Wait for ">" prompt */
    memset(response, 0, sizeof(response));
    ESP_ReadResponse(response, 100, 5000, ">");
    if (!strstr(response, ">")) {
        ESP_SendCommand("CIPCLOSE", NULL, 0, 500);
        return ESP_ERROR;
    }

    /* Send HTTP request */
    const char *req = "GET /api/json/utc/now HTTP/1.0\r\nHost: worldclockapi.com\r\n\r\n";
    while (*req) {
        UART2_SendByte((uint8_t)*req++);
    }

    /* Wait for SEND OK */
    memset(response, 0, sizeof(response));
    ESP_ReadResponse(response, 60, 10000, "SEND OK");

    /* Read HTTP response - need ~644 bytes for headers + JSON body */
    memset(response, 0, sizeof(response));
    int total = 0;
    int b;

    for (i = 0; i < 300 && total < 680; i++) {
        b = UART2_ReceiveByteTimeout(100);
        if (b >= 0) {
            response[total++] = (char)b;
            i = 0;  /* Reset timeout when data arrives */
        }
    }
    response[total] = '\0';

    /* Close connection */
    ESP_SendCommand("CIPCLOSE", NULL, 0, 1000);

    /* Parse JSON: "currentDateTime":"2024-01-15T14:30:45Z" */
    char *dt = strstr(response, "currentDateTime");
    if (!dt) {
        return ESP_ERROR;
    }

    /* Find the date string after the colon and quote */
    dt = strchr(dt, ':');
    if (!dt) return ESP_ERROR;
    dt++;  /* Skip ':' */
    while (*dt == ' ' || *dt == '"') dt++;

    /* Parse: YYYY-MM-DDTHH:MM:SS */
    /* Year */
    time->year = 0;
    for (i = 0; i < 4 && *dt >= '0' && *dt <= '9'; i++) {
        time->year = time->year * 10 + (*dt++ - '0');
    }
    if (*dt == '-') dt++;

    /* Month */
    time->month = 0;
    for (i = 0; i < 2 && *dt >= '0' && *dt <= '9'; i++) {
        time->month = time->month * 10 + (*dt++ - '0');
    }
    if (*dt == '-') dt++;

    /* Day */
    time->day = 0;
    for (i = 0; i < 2 && *dt >= '0' && *dt <= '9'; i++) {
        time->day = time->day * 10 + (*dt++ - '0');
    }
    if (*dt == 'T') dt++;

    /* Hours */
    time->hours = 0;
    for (i = 0; i < 2 && *dt >= '0' && *dt <= '9'; i++) {
        time->hours = time->hours * 10 + (*dt++ - '0');
    }
    if (*dt == ':') dt++;

    /* Minutes */
    time->minutes = 0;
    for (i = 0; i < 2 && *dt >= '0' && *dt <= '9'; i++) {
        time->minutes = time->minutes * 10 + (*dt++ - '0');
    }
    if (*dt == ':') dt++;

    /* Seconds */
    time->seconds = 0;
    for (i = 0; i < 2 && *dt >= '0' && *dt <= '9'; i++) {
        time->seconds = time->seconds * 10 + (*dt++ - '0');
    }

    /* Apply timezone offset (NIST time is UTC) */
    int hours_adj = (int)time->hours + timezone_offset;
    if (hours_adj < 0) {
        hours_adj += 24;
        /* Note: day rollback not handled for simplicity */
    } else if (hours_adj >= 24) {
        hours_adj -= 24;
    }
    time->hours = (uint8_t)hours_adj;

    /* Validate parsed values */
    if (time->year >= 2020 && time->year <= 2100 &&
        time->month >= 1 && time->month <= 12 &&
        time->day >= 1 && time->day <= 31 &&
        time->hours <= 23 && time->minutes <= 59 && time->seconds <= 59) {
        return ESP_OK;
    }

    return ESP_ERROR;
}

void ESP_SendRaw(const uint8_t *data, uint16_t len)
{
    while (len--) {
        UART2_SendByte(*data++);
    }
}

uint16_t ESP_ReceiveRaw(uint8_t *buffer, uint16_t max_len, uint32_t timeout_ms)
{
    uint16_t count = 0;
    int byte;

    while (count < max_len) {
        byte = UART2_ReceiveByteTimeout(timeout_ms);
        if (byte < 0) break;
        buffer[count++] = (uint8_t)byte;
    }

    return count;
}

int ESP_GetFirmwareVersion(char *version_buf)
{
    char response[128];
    int result;
    int i;

    /* Clear buffer and add delay */
    ESP_ClearRxBuffer();
    delay_ms(100);

    result = ESP_SendCommand("GMR", response, sizeof(response), 3000);

    /* Copy response, replacing non-printable chars with dots */
    for (i = 0; i < 63 && response[i]; i++) {
        if (response[i] >= 32 && response[i] <= 126) {
            version_buf[i] = response[i];
        } else {
            version_buf[i] = '.';
        }
    }
    version_buf[i] = '\0';

    return result;
}
