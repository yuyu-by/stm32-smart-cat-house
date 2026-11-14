#include "LightSensor.h"
#include "adc.h"

LightSensor_Status LightSensor_Init(void)
{
    if (HAL_ADCEx_Calibration_Start(&hadc1) != HAL_OK)
    {
        return LIGHT_SENSOR_STATUS_HAL_ERROR;
    }

    return LIGHT_SENSOR_STATUS_OK;
}

LightSensor_Status LightSensor_ReadRaw(uint16_t *raw_value,
                                       uint32_t timeout_ms)
{
    HAL_StatusTypeDef hal_status;

    if (raw_value == NULL)
    {
        return LIGHT_SENSOR_STATUS_BAD_ARGUMENT;
    }

    if (HAL_ADC_Start(&hadc1) != HAL_OK)
    {
        return LIGHT_SENSOR_STATUS_HAL_ERROR;
    }

    hal_status = HAL_ADC_PollForConversion(&hadc1, timeout_ms);
    if (hal_status != HAL_OK)
    {
        (void)HAL_ADC_Stop(&hadc1);
        return (hal_status == HAL_TIMEOUT) ? LIGHT_SENSOR_STATUS_TIMEOUT
                                          : LIGHT_SENSOR_STATUS_HAL_ERROR;
    }

    *raw_value = (uint16_t)HAL_ADC_GetValue(&hadc1);

    if (HAL_ADC_Stop(&hadc1) != HAL_OK)
    {
        return LIGHT_SENSOR_STATUS_HAL_ERROR;
    }

    return LIGHT_SENSOR_STATUS_OK;
}

LightSensor_Status LightSensor_ReadAverage(uint16_t *average_value,
                                           uint8_t sample_count,
                                           uint32_t timeout_ms)
{
    uint8_t index;
    uint16_t raw_value;
    uint32_t sum = 0U;
    LightSensor_Status status;

    if ((average_value == NULL) || (sample_count == 0U))
    {
        return LIGHT_SENSOR_STATUS_BAD_ARGUMENT;
    }

    for (index = 0U; index < sample_count; index++)
    {
        status = LightSensor_ReadRaw(&raw_value, timeout_ms);
        if (status != LIGHT_SENSOR_STATUS_OK)
        {
            return status;
        }

        sum += raw_value;
    }

    *average_value = (uint16_t)(sum / sample_count);
    return LIGHT_SENSOR_STATUS_OK;
}

uint8_t LightSensor_RawToPercent(uint16_t raw_value)
{
    if (raw_value > LIGHT_SENSOR_ADC_MAX)
    {
        raw_value = LIGHT_SENSOR_ADC_MAX;
    }

    /* The schematic pulls PA1 up to 3.3 V and the LDR down to GND.
       Stronger light lowers the LDR resistance, so a lower ADC value means
       more light. */
    return (uint8_t)(((uint32_t)(LIGHT_SENSOR_ADC_MAX - raw_value) * 100U) /
                     LIGHT_SENSOR_ADC_MAX);
}
