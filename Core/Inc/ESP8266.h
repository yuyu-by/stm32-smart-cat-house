#ifndef __ESP8266_H
#define __ESP8266_H

#include "stm32f1xx_hal.h"

#define ESP8266_RESPONSE_BUFFER_SIZE 128U
#define ESP8266_RX_RING_SIZE         2048U

typedef enum
{
    ESP8266_STATUS_OK = 0,
    ESP8266_STATUS_TIMEOUT,
    ESP8266_STATUS_ERROR,
    ESP8266_STATUS_HAL_ERROR,
    ESP8266_STATUS_BAD_ARGUMENT,
    ESP8266_STATUS_WRONG_PASSWORD,
    ESP8266_STATUS_AP_NOT_FOUND,
    ESP8266_STATUS_CONNECT_FAILED
} ESP8266_Status;

ESP8266_Status ESP8266_TestAT(uint32_t timeout_ms);
ESP8266_Status ESP8266_ConnectWiFi(const char *ssid,
                                   const char *password,
                                   uint32_t timeout_ms);
ESP8266_Status ESP8266_StartAccessPoint(const char *ssid,
                                        const char *password,
                                        uint32_t timeout_ms);
ESP8266_Status ESP8266_StartTcpServer(uint16_t port,
                                      uint32_t timeout_ms);
ESP8266_Status ESP8266_StartInterruptReceive(void);
uint8_t ESP8266_PollHttpRequest(uint8_t *connection_id);
ESP8266_Status ESP8266_SendHttpResponse(uint8_t connection_id,
                                        const char *body,
                                        uint32_t timeout_ms);
const char *ESP8266_GetLastResponse(void);

#endif
