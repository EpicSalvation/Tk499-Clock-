# ESP8266 WiFi Setup Guide

Complete guide for using the ESP8266 WiFi module on the TKM32F499 4.3" SmartBoard.

## Hardware Overview

The board has an onboard **ESP8266MOD** module connected to:

| Signal | TKM32F499 Pin | Description |
|--------|---------------|-------------|
| TX     | PA2 (UART2_TX) | MCU transmits to ESP |
| RX     | PA3 (UART2_RX) | MCU receives from ESP |
| RST    | PD0           | Reset control (active low) |
| CH_PD  | PD1           | Chip enable (active high) |

**Note:** CH_PD may also be directly tied to 3.3V on some boards.

## UART Configuration

The TKM32F499 uses a **custom UART peripheral** (not STM32 USART).

**CRITICAL: The system clock is 192MHz when running via bootloader, not 240MHz!**

### Clock Enable

```c
/* Enable UART2 clock - APB2ENR bit 3 */
RCC->APB2ENR |= (1 << 3);

/* Enable GPIOA and GPIOD clocks */
RCC->AHB1ENR |= (1 << 0) | (1 << 3);
```

### GPIO Setup

```c
/* PA2: TX - AF push-pull, 50MHz */
GPIOA->CRL &= ~(0xF << 8);
GPIOA->CRL |= (0x0B << 8);

/* PA3: RX - Input with pull-up */
GPIOA->CRL &= ~(0xF << 12);
GPIOA->CRL |= (0x08 << 12);
GPIOA->ODR |= (1 << 3);  /* Enable pull-up */

/* Set alternate function AF7 for PA2 and PA3 */
GPIOA->AFRL &= ~(0xFF << 8);
GPIOA->AFRL |= (0x07 << 8);   /* PA2 = AF7 */
GPIOA->AFRL |= (0x07 << 12);  /* PA3 = AF7 */
```

### UART Register Setup

```c
/* Disable UART first */
UART2->GCR = 0;

/* Baud rate: 115200 at 192MHz system clock (via bootloader) */
/* Formula: BRR = SYS_CLK / baudrate / 16 */
/*          FRABRG = (SYS_CLK / baudrate) % 16 */
uint32_t sys_clk = 192000000;  /* 192MHz when running via bootloader! */
uint32_t div = sys_clk / 115200;
UART2->BRR = div / 16;      /* = 104 */
UART2->FRABRG = div % 16;   /* = 2 */

/* 8 data bits, 1 stop bit, no parity */
UART2->CCR = (3 << 4);  /* 8-bit character */

/* Enable UART, TX, and RX */
UART2->GCR = (1 << 0) | (1 << 3) | (1 << 4);
```

**Wrong clock speed symptom:** First byte received correctly, then garbage (baud rate drift).

### Reset Control GPIO

```c
/* PD0 (RST) and PD1 (CH_PD) as push-pull outputs */
GPIOD->CRL &= ~(0xFF << 0);
GPIOD->CRL |= (0x33 << 0);

/* Set CH_PD high (enable) */
GPIOD->BSRR = (1 << 1);

/* Set RST high (not in reset) */
GPIOD->BSRR = (1 << 0);
```

## ESP8266 Initialization Sequence

```c
int ESP_Init(void)
{
    /* 1. Configure GPIOs and UART */
    UART2_Init(115200);

    /* 2. Configure reset pins */
    GPIOD->BSRR = (1 << 1);  /* CH_PD high */
    GPIOD->BSRR = (1 << 0);  /* RST high */

    /* 3. Reset ESP8266 */
    GPIOD->BSRR = (1 << 16); /* RST low */
    delay_ms(100);
    GPIOD->BSRR = (1 << 0);  /* RST high */

    /* 4. Wait for boot (2-3 seconds) */
    delay_ms(3000);

    /* 5. Clear any boot messages */
    ESP_ClearRxBuffer();

    /* 6. Test with AT command */
    for (int i = 0; i < 5; i++) {
        if (ESP_SendCommand("AT", NULL, 0, 1000) == ESP_OK) {
            return ESP_OK;
        }
        delay_ms(500);
    }

    return ESP_ERROR;
}
```

## AT Command Interface

### Sending Commands

```c
int ESP_SendCommand(const char *cmd, char *response,
                    uint16_t resp_size, uint32_t timeout_ms)
{
    char cmd_buf[128];

    /* Clear pending data */
    ESP_ClearRxBuffer();

    /* Build command with CRLF */
    if (cmd[0] == 'A' && cmd[1] == 'T') {
        snprintf(cmd_buf, sizeof(cmd_buf), "%s\r\n", cmd);
    } else {
        snprintf(cmd_buf, sizeof(cmd_buf), "AT+%s\r\n", cmd);
    }

    /* Send */
    UART2_SendString(cmd_buf);

    /* Read response */
    char local_buf[256];
    char *buf = response ? response : local_buf;
    int len = ESP_ReadResponse(buf, resp_size, timeout_ms, "OK");

    if (strstr(buf, "OK")) return ESP_OK;
    if (strstr(buf, "ERROR")) return ESP_ERROR;
    return ESP_TIMEOUT;
}
```

### Basic Send/Receive Functions

```c
void UART2_SendByte(uint8_t byte)
{
    while (!(UART2->CSR & (1 << 0)));  /* Wait TX ready */
    UART2->TDR = byte;
}

void UART2_SendString(const char *str)
{
    while (*str) {
        UART2_SendByte(*str++);
    }
}

int UART2_ReceiveByteTimeout(uint32_t timeout_ms)
{
    uint32_t timeout = timeout_ms * 1000;
    while (timeout--) {
        if (UART2->CSR & (1 << 1)) {  /* RX available */
            return (uint8_t)UART2->RDR;
        }
        /* Small delay (~1us at 240MHz) */
        for (volatile int i = 0; i < 24; i++);
    }
    return -1;  /* Timeout */
}
```

## WiFi Connection

### Connect to Access Point

```c
int ESP_ConnectWiFi(const char *ssid, const char *password)
{
    char cmd[128];

    /* Set Station mode */
    if (ESP_SendCommand("CWMODE=1", NULL, 0, 1000) != ESP_OK) {
        return ESP_ERROR;
    }
    delay_ms(100);

    /* Connect to AP */
    snprintf(cmd, sizeof(cmd), "CWJAP=\"%s\",\"%s\"", ssid, password);
    return ESP_SendCommand(cmd, NULL, 0, 15000);  /* 15s timeout */
}
```

### Check Connection Status

```c
int ESP_IsConnected(void)
{
    char response[128];
    if (ESP_SendCommand("CWJAP?", response, sizeof(response), 2000) == ESP_OK) {
        if (strstr(response, "No AP")) return 0;
        return 1;
    }
    return 0;
}
```

## AT Firmware Version

Check firmware version with `AT+GMR`. The SmartBoard may ship with older firmware.

| Feature | Minimum Version |
|---------|-----------------|
| Basic AT, WiFi | Any |
| TCP/UDP connections | Any |
| **SNTP time sync** | **v1.7.0+** |
| SSL/TLS (HTTPS) | v2.0.0+ |

If `AT+CIPSNTPCFG` returns `ERROR`, you have older firmware. Options:
1. Update firmware (see "Firmware Update" section below)
2. Use HTTP-based time sync from plain HTTP server

## Firmware Update (Future Reference)

The ESP8266 is soldered to the board but can potentially be updated using the TKM32F499 as a serial passthrough:

**Hardware connections for flashing:**
| Signal | TKM32F499 Pin | ESP8266 Pin |
|--------|---------------|-------------|
| TX     | PA2 (UART2_TX) | RXD |
| RX     | PA3 (UART2_RX) | TXD |
| RST    | PD0           | RST |
| CH_PD  | PD1           | CH_PD |
| GPIO0  | PB15          | GPIO0 |

**To enter ESP8266 bootloader mode:**
1. Pull GPIO0 (PB15) LOW
2. Pulse RST (PD0) LOW then HIGH
3. ESP8266 boots into flash mode instead of normal operation

**Approach:**
1. Write a serial passthrough program for TKM32F499 (USB ↔ UART2 bridge)
2. Control GPIO0 and RST to enter bootloader mode on startup
3. Use `esptool.py` on PC to flash new AT firmware (e.g., v1.7.0+ for SNTP support)
4. Flash clock firmware back to TKM32F499

**Note:** This has not been tested yet. Requires understanding of USB serial capability on TKM32F499.

## Time Synchronization

### HTTP-Based Time Sync (Firmware 1.3.0+)

Since the SmartBoard ships with AT firmware 1.3.0 which doesn't support SNTP commands, we use HTTP to fetch time from `worldclockapi.com`:

```c
int ESP_GetNTPTime(ESP_Time_t *time, int8_t timezone_offset)
{
    char response[700];  /* Need ~644 bytes for full HTTP response */

    /* Verify WiFi connection */
    if (ESP_SendCommand("CIFSR", response, sizeof(response), 5000) != ESP_OK) {
        return ESP_ERROR;
    }
    if (strstr(response, "0.0.0.0")) {
        return ESP_ERROR;  /* Not connected */
    }

    /* Set single connection mode and close any existing */
    ESP_SendCommand("CIPMUX=0", NULL, 0, 1000);
    ESP_SendCommand("CIPCLOSE", NULL, 0, 500);

    /* Connect to worldclockapi.com */
    ESP_SendCommand("CIPSTART=\"TCP\",\"worldclockapi.com\",80",
                    response, sizeof(response), 10000);
    if (!strstr(response, "CONNECT")) {
        return ESP_ERROR;
    }

    /* Send HTTP request (59 bytes) */
    UART2_SendString("AT+CIPSEND=59\r\n");
    /* Wait for ">" prompt, then send: */
    const char *req = "GET /api/json/utc/now HTTP/1.0\r\n"
                      "Host: worldclockapi.com\r\n\r\n";

    /* Read response and parse JSON:
     * {"currentDateTime":"2024-01-15T14:30:45Z",...}
     */
    /* ... parse currentDateTime field ... */

    return ESP_OK;
}
```

**Key points:**
- Response buffer must be ~700 bytes (HTTP headers + 234-byte JSON body)
- Parse `currentDateTime` field in ISO 8601 format: `YYYY-MM-DDTHH:MM:SSZ`
- Apply timezone offset after parsing (time is in UTC)

### SNTP Time Sync (Firmware 1.7.0+)

If you have newer firmware, you can use the simpler SNTP approach:

```c
/* Configure SNTP */
snprintf(cmd, sizeof(cmd), "CIPSNTPCFG=1,%d,\"pool.ntp.org\"",
         timezone_offset);
ESP_SendCommand(cmd, NULL, 0, 3000);

/* Wait for sync */
delay_ms(3000);

/* Query time */
ESP_SendCommand("CIPSNTPTIME?", response, sizeof(response), 5000);
/* Parse response: +CIPSNTPTIME:Mon Dec 25 14:30:45 2023 */
```

### Time Structure

```c
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} ESP_Time_t;
```

## Common Issues and Solutions

### No Response from ESP8266

1. **Check clock enable**: Must use `RCC->APB2ENR |= (1 << 3)` for UART2
2. **Check baud rate formula**: `BRR = SYS_CLK / baudrate / 16` (note the /16!)
3. **Check GPIO AF**: Must set AFRL to 0x07 (AF7) for PA2/PA3
4. **Check reset pins**: PD0 for RST, PD1 for CH_PD (not PB14/PB15!)

### TX Ready Flag Never Set

- UART clock not enabled properly
- Wrong APB2ENR bit (bit 3 for UART2, NOT APB1ENR bit 17)

### ESP8266 Boot Detection

ESP8266 sends boot messages at **74880 baud**, then switches to AT firmware baud rate (usually 115200). If you can detect data at 74880 but not 115200, your module may have non-standard firmware.

### NTP Sync Fails

1. Ensure WiFi is connected first
2. Wait 3+ seconds after CIPSNTPCFG before querying
3. Retry several times - first query often returns 1970 (not synced)
4. Check that year is valid (2020-2100) before accepting

## Complete Initialization Example

```c
#define WIFI_SSID     "YourNetwork"
#define WIFI_PASSWORD "YourPassword"
#define TIMEZONE      -5  /* EST */

int main(void)
{
    ESP_Time_t ntp_time;

    /* Initialize ESP8266 */
    if (ESP_Init() != ESP_OK) {
        /* Handle error */
        return -1;
    }

    /* Connect to WiFi */
    if (ESP_ConnectWiFi(WIFI_SSID, WIFI_PASSWORD) != ESP_OK) {
        /* Handle error */
        return -1;
    }

    /* Get NTP time */
    if (ESP_GetNTPTime(&ntp_time, TIMEZONE) == ESP_OK) {
        /* Use time */
        printf("%02d:%02d:%02d\n",
               ntp_time.hours, ntp_time.minutes, ntp_time.seconds);
    }

    return 0;
}
```

## API Reference

| Function | Description |
|----------|-------------|
| `ESP_Init()` | Initialize ESP8266 and UART |
| `ESP_Reset()` | Hardware reset ESP8266 |
| `ESP_Test()` | Send AT command to test connection |
| `ESP_SendCommand()` | Send AT command and get response |
| `ESP_ConnectWiFi()` | Connect to WiFi access point |
| `ESP_DisconnectWiFi()` | Disconnect from WiFi |
| `ESP_IsConnected()` | Check if connected to WiFi |
| `ESP_GetIP()` | Get assigned IP address |
| `ESP_GetNTPTime()` | Get time from NTP server |
