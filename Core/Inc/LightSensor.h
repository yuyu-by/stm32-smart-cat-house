#ifndef __LIGHT_SENSOR_H
#define __LIGHT_SENSOR_H

#include "stm32f1xx_hal.h"

#define LIGHT_SENSOR_ADC_MAX 4095U

typedef enum
{
    LIGHT_SENSOR_STATUS_OK = 0,
    LIGHT_SENSOR_STATUS_TIMEOUT,
    LIGHT_SENSOR_STATUS_HAL_ERROR,
    LIGHT_SENSOR_STATUS_BAD_ARGUMENT
} LightSensor_Status;

LightSensor_Status LightSensor_Init(void);
LightSensor_Status LightSensor_ReadRaw(uint16_t *raw_value,
                                       uint32_t timeout_ms);
LightSensor_Status LightSensor_ReadAverage(uint16_t *average_value,
                                           uint8_t sample_count,
                                           uint32_t timeout_ms);
uint8_t LightSensor_RawToPercent(uint16_t raw_value);

#endif
