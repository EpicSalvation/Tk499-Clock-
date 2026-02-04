/**
 * ESP8266 WiFi Module Driver
 *
 * Communicates with ESP8266 via UART2 using AT commands.
 * Supports WiFi connection and HTTP-based time synchronization.
 *
 * Hardware connections:
 *   PA2 (UART2_TX) -> ESP8266 RXD
 *   PA3 (UART2_RX) -> ESP8266 TXD
 *   PD0 -> ESP8266 RST (active low)
 *   PD1 -> ESP8266 CH_PD (active high)
 */

#ifndef __ESP8266_H
#define __ESP8266_H

#include <stdint.h>

/* Result codes */
#define ESP_OK          0
#define ESP_ERROR       -1
#define ESP_TIMEOUT     -2
#define ESP_BUSY        -3

/* Time structure */
typedef struct {
    uint8_t hours;
    uint8_t minutes;
    uint8_t seconds;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} ESP_Time_t;

/**
 * Initialize ESP8266 module
 * Sets up USART2, GPIO pins, and resets the module
 * @return ESP_OK on success, ESP_ERROR on failure
 */
int ESP_Init(void);

/**
 * Reset the ESP8266 module
 */
void ESP_Reset(void);

/**
 * Send AT command and wait for response
 * @param cmd Command string (without AT prefix or \r\n)
 * @param response Buffer to store response (can be NULL)
 * @param resp_size Size of response buffer
 * @param timeout_ms Timeout in milliseconds
 * @return ESP_OK if "OK" received, ESP_ERROR otherwise
 */
int ESP_SendCommand(const char *cmd, char *response, uint16_t resp_size, uint32_t timeout_ms);

/**
 * Test if ESP8266 is responding
 * @return ESP_OK if responding, ESP_ERROR otherwise
 */
int ESP_Test(void);

/**
 * Connect to WiFi network
 * @param ssid Network name
 * @param password Network password
 * @return ESP_OK on success, ESP_ERROR on failure
 */
int ESP_ConnectWiFi(const char *ssid, const char *password);

/**
 * Disconnect from WiFi network
 * @return ESP_OK on success
 */
int ESP_DisconnectWiFi(void);

/**
 * Check if connected to WiFi
 * @return 1 if connected, 0 if not
 */
int ESP_IsConnected(void);

/**
 * Get IP address
 * @param ip_buf Buffer to store IP string (at least 16 bytes)
 * @return ESP_OK on success
 */
int ESP_GetIP(char *ip_buf);

/**
 * Get time via HTTP from worldclockapi.com
 * Note: Uses HTTP since AT firmware 1.3.0 doesn't support SNTP
 * @param time Pointer to time structure to fill
 * @param timezone_offset Hours offset from UTC (e.g., -5 for EST)
 * @return ESP_OK on success, ESP_ERROR on failure
 */
int ESP_GetNTPTime(ESP_Time_t *time, int8_t timezone_offset);

/**
 * Send raw data to USART2
 * @param data Data buffer
 * @param len Length of data
 */
void ESP_SendRaw(const uint8_t *data, uint16_t len);

/**
 * Receive data from USART2 with timeout
 * @param buffer Buffer to store data
 * @param max_len Maximum bytes to receive
 * @param timeout_ms Timeout in milliseconds
 * @return Number of bytes received
 */
uint16_t ESP_ReceiveRaw(uint8_t *buffer, uint16_t max_len, uint32_t timeout_ms);

/**
 * Get firmware version from ESP8266
 * @param version_buf Buffer to store version string (at least 64 bytes)
 * @return ESP_OK on success
 */
int ESP_GetFirmwareVersion(char *version_buf);

#endif /* __ESP8266_H */
