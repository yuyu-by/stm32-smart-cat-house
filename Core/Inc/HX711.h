#ifndef __HX711_H
#define __HX711_H

#include "stm32f1xx_hal.h"

#define HX711_DEFAULT_TIMEOUT_MS  500U
#define HX711_MAX_AVERAGE_SAMPLES 32U
#define HX711_VENDOR_GAP_VALUE     106.5f
#define HX711_ZERO_DEADBAND_COUNTS 500

typedef enum
{
    HX711_STATUS_OK = 0,
    HX711_STATUS_TIMEOUT,
    HX711_STATUS_BAD_ARGUMENT
} HX711_Status;

/* Channel A, gain 128. SCK and DT pins are defined by CubeMX in main.h. */
void HX711_Init(void);
uint8_t HX711_IsReady(void);
HX711_Status HX711_ReadRaw(int32_t *raw_value, uint32_t timeout_ms);
HX711_Status HX711_ReadAverage(int32_t *average_value, uint8_t samples,
                               uint32_t timeout_ms);
HX711_Status HX711_Tare(int32_t *offset_value, uint8_t samples,
                        uint32_t timeout_ms);
int32_t HX711_GetNetValue(int32_t average_value, int32_t offset_value);
int32_t HX711_GetWeightGram(int32_t net_value);

#endif
