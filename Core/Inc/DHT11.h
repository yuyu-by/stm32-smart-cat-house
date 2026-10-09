#ifndef __DHT11_H
#define __DHT11_H

#include "stm32f1xx_hal.h"

typedef enum
{
    DHT11_STATUS_OK = 0,
    DHT11_STATUS_TIMEOUT,
    DHT11_STATUS_CHECKSUM_ERROR,
    DHT11_STATUS_BAD_ARGUMENT
} DHT11_Status;

void DHT11_Init(void);
DHT11_Status DHT11_Read(uint8_t *temperature, uint8_t *humidity);

#endif
