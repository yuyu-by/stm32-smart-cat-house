#include "ESP8266.h"
#include "usart.h"
#include <stdio.h>
#include <string.h>

static char esp8266_response[ESP8266_RESPONSE_BUFFER_SIZE];
static char esp8266_ipd_header[64];
static uint16_t esp8266_ipd_header_length = 0U;
static uint16_t esp8266_ipd_payload_length = 0U;
static uint16_t esp8266_ipd_payload_received = 0U;
static uint8_t esp8266_ipd_connection_id = 0U;
static uint8_t esp8266_ipd_header_ready = 0U;
static volatile uint8_t esp8266_rx_ring[ESP8266_RX_RING_SIZE];
static volatile uint16_t esp8266_rx_head = 0U;
static volatile uint16_t esp8266_rx_tail = 0U;
static uint8_t esp8266_rx_interrupt_byte = 0U;
static volatile uint8_t esp8266_interrupt_receive_enabled = 0U;

static void ESP8266_ResetIpdParser(void)
{
    esp8266_ipd_header_length = 0U;
    esp8266_ipd_payload_length = 0U;
    esp8266_ipd_payload_received = 0U;
    esp8266_ipd_connection_id = 0U;
    esp8266_ipd_header_ready = 0U;
    esp8266_ipd_header[0] = '\0';
}

static void ESP8266_PauseInterruptReceive(void)
{
    if (esp8266_interrupt_receive_enabled != 0U)
    {
        esp8266_interrupt_receive_enabled = 0U;
        (void)HAL_UART_AbortReceive_IT(&huart2);
    }
}

static void ESP8266_ResumeInterruptReceive(void)
{
    esp8266_interrupt_receive_enabled = 1U;
    (void)HAL_UART_Receive_IT(&huart2, &esp8266_rx_interrupt_byte, 1U);
}

static ESP8266_Status ESP8266_SendCommand(const char *command,
                                          const char *expected_response,
                                          uint32_t timeout_ms)
{
    uint8_t byte;
    uint16_t length = 0U;
    uint32_t start_tick;
    uint32_t elapsed;
    uint32_t remaining;
    HAL_StatusTypeDef hal_status;

    if ((command == NULL) || (expected_response == NULL))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    memset(esp8266_response, 0, sizeof(esp8266_response));
    __HAL_UART_FLUSH_DRREGISTER(&huart2);

    if (HAL_UART_Transmit(&huart2,
                          (uint8_t *)command,
                          (uint16_t)strlen(command),
                          timeout_ms) != HAL_OK)
    {
        return ESP8266_STATUS_HAL_ERROR;
    }

    start_tick = HAL_GetTick();
    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {
        elapsed = HAL_GetTick() - start_tick;
        remaining = timeout_ms - elapsed;
        hal_status = HAL_UART_Receive(&huart2, &byte, 1U, remaining);

        if (hal_status == HAL_TIMEOUT)
        {
            break;
        }
        if (hal_status != HAL_OK)
        {
            return ESP8266_STATUS_HAL_ERROR;
        }

        if (length < (ESP8266_RESPONSE_BUFFER_SIZE - 1U))
        {
            esp8266_response[length++] = (char)byte;
            esp8266_response[length] = '\0';
        }

        if (strstr(esp8266_response, expected_response) != NULL)
        {
            return ESP8266_STATUS_OK;
        }
        if ((strstr(esp8266_response, "ERROR") != NULL) ||
            (strstr(esp8266_response, "FAIL") != NULL))
        {
            return ESP8266_STATUS_ERROR;
        }
    }

    return ESP8266_STATUS_TIMEOUT;
}

ESP8266_Status ESP8266_TestAT(uint32_t timeout_ms)
{
    return ESP8266_SendCommand("AT\r\n", "OK", timeout_ms);
}

ESP8266_Status ESP8266_ConnectWiFi(const char *ssid,
                                   const char *password,
                                   uint32_t timeout_ms)
{
    char command[96];
    int command_length;
    ESP8266_Status status;

    if ((ssid == NULL) || (password == NULL))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    status = ESP8266_SendCommand("AT+CWMODE=1\r\n", "OK", 2000U);
    if (status != ESP8266_STATUS_OK)
    {
        return status;
    }

    command_length = snprintf(command, sizeof(command),
                              "AT+CWJAP=\"%s\",\"%s\"\r\n",
                              ssid, password);
    if ((command_length < 0) ||
        ((uint32_t)command_length >= sizeof(command)))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    status = ESP8266_SendCommand(command, "OK", timeout_ms);
    if (status == ESP8266_STATUS_ERROR)
    {
        if (strstr(esp8266_response, "+CWJAP:2") != NULL)
        {
            return ESP8266_STATUS_WRONG_PASSWORD;
        }
        if (strstr(esp8266_response, "+CWJAP:3") != NULL)
        {
            return ESP8266_STATUS_AP_NOT_FOUND;
        }
        if (strstr(esp8266_response, "+CWJAP:4") != NULL)
        {
            return ESP8266_STATUS_CONNECT_FAILED;
        }
    }

    return status;
}

ESP8266_Status ESP8266_StartAccessPoint(const char *ssid,
                                        const char *password,
                                        uint32_t timeout_ms)
{
    char command[96];
    int command_length;
    ESP8266_Status status;

    if ((ssid == NULL) || (password == NULL) ||
        (strlen(password) < 8U))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    status = ESP8266_SendCommand("AT+CWMODE=2\r\n", "OK", 2000U);
    if (status != ESP8266_STATUS_OK)
    {
        return status;
    }

    /* Make sure phones receive a 192.168.4.x address from the ESP8266. */
    (void)ESP8266_SendCommand("AT+CIPAP=\"192.168.4.1\"\r\n",
                              "OK", 2000U);
    (void)ESP8266_SendCommand("AT+CWDHCP=0,1\r\n",
                              "OK", 2000U);

    command_length = snprintf(command, sizeof(command),
                              "AT+CWSAP=\"%s\",\"%s\",5,3\r\n",
                              ssid, password);
    if ((command_length < 0) ||
        ((uint32_t)command_length >= sizeof(command)))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    return ESP8266_SendCommand(command, "OK", timeout_ms);
}

ESP8266_Status ESP8266_StartTcpServer(uint16_t port,
                                      uint32_t timeout_ms)
{
    char command[32];
    int command_length;
    ESP8266_Status status;

    /* A previous MCU reset may leave the ESP8266 server running. */
    (void)ESP8266_SendCommand("AT+CIPSERVER=0\r\n", "OK", 1000U);

    status = ESP8266_SendCommand("AT+CIPMUX=1\r\n", "OK", timeout_ms);
    if (status != ESP8266_STATUS_OK)
    {
        return status;
    }

    command_length = snprintf(command, sizeof(command),
                              "AT+CIPSERVER=1,%u\r\n",
                              (unsigned int)port);
    if ((command_length < 0) ||
        ((uint32_t)command_length >= sizeof(command)))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    return ESP8266_SendCommand(command, "OK", timeout_ms);
}

ESP8266_Status ESP8266_StartInterruptReceive(void)
{
    esp8266_rx_head = 0U;
    esp8266_rx_tail = 0U;
    ESP8266_ResetIpdParser();
    esp8266_interrupt_receive_enabled = 1U;

    if (HAL_UART_Receive_IT(&huart2,
                           &esp8266_rx_interrupt_byte,
                           1U) != HAL_OK)
    {
        esp8266_interrupt_receive_enabled = 0U;
        return ESP8266_STATUS_HAL_ERROR;
    }

    return ESP8266_STATUS_OK;
}

uint8_t ESP8266_PollHttpRequest(uint8_t *connection_id)
{
    uint8_t byte;
    unsigned int parsed_connection_id;
    unsigned int parsed_payload_length;
    char *ipd_start;

    if (connection_id == NULL)
    {
        return 0U;
    }

    while (esp8266_rx_tail != esp8266_rx_head)
    {
        byte = esp8266_rx_ring[esp8266_rx_tail];
        esp8266_rx_tail = (uint16_t)((esp8266_rx_tail + 1U) %
                                     ESP8266_RX_RING_SIZE);

        if (esp8266_ipd_header_ready == 0U)
        {
            if (esp8266_ipd_header_length <
                (sizeof(esp8266_ipd_header) - 1U))
            {
                esp8266_ipd_header[esp8266_ipd_header_length++] = (char)byte;
                esp8266_ipd_header[esp8266_ipd_header_length] = '\0';
            }
            else
            {
                ESP8266_ResetIpdParser();
                continue;
            }

            if (byte == ':')
            {
                ipd_start = strstr(esp8266_ipd_header, "+IPD,");
                if ((ipd_start != NULL) &&
                    (sscanf(ipd_start, "+IPD,%u,%u:",
                            &parsed_connection_id,
                            &parsed_payload_length) == 2))
                {
                    esp8266_ipd_connection_id =
                        (uint8_t)parsed_connection_id;
                    esp8266_ipd_payload_length =
                        (uint16_t)parsed_payload_length;
                    esp8266_ipd_payload_received = 0U;
                    esp8266_ipd_header_ready = 1U;
                }
                else
                {
                    ESP8266_ResetIpdParser();
                }
            }
        }
        else
        {
            esp8266_ipd_payload_received++;
            if (esp8266_ipd_payload_received >= esp8266_ipd_payload_length)
            {
                *connection_id = esp8266_ipd_connection_id;
                ESP8266_ResetIpdParser();
                return 1U;
            }
        }
    }

    return 0U;
}

ESP8266_Status ESP8266_SendHttpResponse(uint8_t connection_id,
                                        const char *body,
                                        uint32_t timeout_ms)
{
    char command[32];
    char response[256];
    int command_length;
    int response_length;
    ESP8266_Status status;

    if (body == NULL)
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    response_length = snprintf(response, sizeof(response),
                               "HTTP/1.1 200 OK\r\n"
                               "Content-Type: text/html\r\n"
                               "Connection: close\r\n"
                               "Content-Length: %u\r\n\r\n%s",
                               (unsigned int)strlen(body), body);
    if ((response_length < 0) ||
        ((uint32_t)response_length >= sizeof(response)))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    command_length = snprintf(command, sizeof(command),
                              "AT+CIPSEND=%u,%u\r\n",
                              (unsigned int)connection_id,
                              (unsigned int)response_length);
    if ((command_length < 0) ||
        ((uint32_t)command_length >= sizeof(command)))
    {
        return ESP8266_STATUS_BAD_ARGUMENT;
    }

    ESP8266_PauseInterruptReceive();
    status = ESP8266_SendCommand(command, ">", timeout_ms);
    if (status != ESP8266_STATUS_OK)
    {
        ESP8266_ResumeInterruptReceive();
        return status;
    }

    if (HAL_UART_Transmit(&huart2, (uint8_t *)response,
                          (uint16_t)response_length,
                          timeout_ms) != HAL_OK)
    {
        ESP8266_ResumeInterruptReceive();
        return ESP8266_STATUS_HAL_ERROR;
    }

    HAL_Delay(100U);
    (void)snprintf(command, sizeof(command),
                   "AT+CIPCLOSE=%u\r\n",
                   (unsigned int)connection_id);
    (void)ESP8266_SendCommand(command, "OK", timeout_ms);
    ESP8266_ResumeInterruptReceive();
    return ESP8266_STATUS_OK;
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    uint16_t next_head;

    if ((huart->Instance == USART2) &&
        (esp8266_interrupt_receive_enabled != 0U))
    {
        next_head = (uint16_t)((esp8266_rx_head + 1U) %
                               ESP8266_RX_RING_SIZE);
        if (next_head != esp8266_rx_tail)
        {
            esp8266_rx_ring[esp8266_rx_head] = esp8266_rx_interrupt_byte;
            esp8266_rx_head = next_head;
        }

        (void)HAL_UART_Receive_IT(&huart2,
                                 &esp8266_rx_interrupt_byte,
                                 1U);
    }
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if ((huart->Instance == USART2) &&
        (esp8266_interrupt_receive_enabled != 0U))
    {
        (void)HAL_UART_Receive_IT(&huart2,
                                 &esp8266_rx_interrupt_byte,
                                 1U);
    }
}

const char *ESP8266_GetLastResponse(void)
{
    return esp8266_response;
}
