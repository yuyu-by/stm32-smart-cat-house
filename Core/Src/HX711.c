#include "HX711.h"
#include "main.h"

/*
 * Short guard delay for the HX711 serial interface.
 * HAL GPIO calls plus this loop keep both SCK high and low longer than the
 * data-sheet minimum while staying far below the 60 us power-down threshold.
 */
static void HX711_ClockDelay(void)
{
    volatile uint8_t i;

    for (i = 0U; i < 24U; i++)
    {
        __NOP();
    }
}

void HX711_Init(void)
{
    /* A high pulse longer than 60 us resets/powers down the HX711. */
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
    HAL_Delay(1U);

    /* Falling edge powers the device up and starts a new conversion. */
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    HAL_Delay(100U);
}

uint8_t HX711_IsReady(void)
{
    return (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) ==
            GPIO_PIN_RESET) ? 1U : 0U;
}

HX711_Status HX711_ReadRaw(int32_t *raw_value, uint32_t timeout_ms)
{
    uint32_t data = 0U;
    uint32_t start_tick;
    uint32_t primask;
    uint8_t bit;

    if ((raw_value == NULL) || (timeout_ms == 0U))
    {
        return HX711_STATUS_BAD_ARGUMENT;
    }

    /* SCK must remain low while waiting for DOUT/DT to become ready. */
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    start_tick = HAL_GetTick();

    while (HX711_IsReady() == 0U)
    {
        if ((HAL_GetTick() - start_tick) >= timeout_ms)
        {
            return HX711_STATUS_TIMEOUT;
        }
    }

    /*
     * Do not allow an interrupt to hold SCK high for more than 60 us.
     * Read 24 MSB-first data bits, then issue pulse 25 for A/128 gain.
     */
    primask = __get_PRIMASK();
    __disable_irq();

    for (bit = 0U; bit < 24U; bit++)
    {
        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
        HX711_ClockDelay();

        data <<= 1;
        if (HAL_GPIO_ReadPin(HX711_DT_GPIO_Port, HX711_DT_Pin) == GPIO_PIN_SET)
        {
            data |= 1U;
        }

        HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
        HX711_ClockDelay();
    }

    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_SET);
    HX711_ClockDelay();
    HAL_GPIO_WritePin(HX711_SCK_GPIO_Port, HX711_SCK_Pin, GPIO_PIN_RESET);
    HX711_ClockDelay();

    if (primask == 0U)
    {
        __enable_irq();
    }

    /* Sign-extend the HX711 24-bit two's-complement result to int32_t. */
    if ((data & 0x00800000UL) != 0U)
    {
        data |= 0xFF000000UL;
    }

    *raw_value = (int32_t)data;
    return HX711_STATUS_OK;
}

HX711_Status HX711_ReadAverage(int32_t *average_value, uint8_t samples,
                               uint32_t timeout_ms)
{
    int64_t sum = 0;
    int32_t raw_value;
    uint8_t sample;
    HX711_Status status;

    if ((average_value == NULL) || (samples == 0U) ||
        (samples > HX711_MAX_AVERAGE_SAMPLES) || (timeout_ms == 0U))
    {
        return HX711_STATUS_BAD_ARGUMENT;
    }

    for (sample = 0U; sample < samples; sample++)
    {
        status = HX711_ReadRaw(&raw_value, timeout_ms);
        if (status != HX711_STATUS_OK)
        {
            return status;
        }

        sum += raw_value;
    }

    *average_value = (int32_t)(sum / samples);
    return HX711_STATUS_OK;
}

HX711_Status HX711_Tare(int32_t *offset_value, uint8_t samples,
                        uint32_t timeout_ms)
{
    /* Tare is the empty-scale average saved as the zero offset. */
    return HX711_ReadAverage(offset_value, samples, timeout_ms);
}

int32_t HX711_GetNetValue(int32_t average_value, int32_t offset_value)
{
    return average_value - offset_value;
}

int32_t HX711_GetWeightGram(int32_t net_value)
{
    if ((net_value >= -HX711_ZERO_DEADBAND_COUNTS) &&
        (net_value <= HX711_ZERO_DEADBAND_COUNTS))
    {
        return 0;
    }

    /* Trial conversion copied from the vendor example's GapValue = 106.5. */
    return (int32_t)((float)net_value / HX711_VENDOR_GAP_VALUE);
}
