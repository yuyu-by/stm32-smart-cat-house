#include "DHT11.h"
#include "main.h"

static uint32_t dht11_cycles_per_us = 1U;

static void DHT11_DelayUs(uint32_t microseconds)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t cycles = microseconds * dht11_cycles_per_us;

    while ((DWT->CYCCNT - start) < cycles)
    {
    }
}

static void DHT11_SetOutput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);
}

static void DHT11_SetInput(void)
{
    GPIO_InitTypeDef GPIO_InitStruct = {0};

    GPIO_InitStruct.Pin = DHT11_DATA_Pin;
    GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(DHT11_DATA_GPIO_Port, &GPIO_InitStruct);
}

static uint8_t DHT11_WaitForLevel(GPIO_PinState level, uint32_t timeout_us)
{
    uint32_t start = DWT->CYCCNT;
    uint32_t timeout_cycles = timeout_us * dht11_cycles_per_us;

    while (HAL_GPIO_ReadPin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin) != level)
    {
        if ((DWT->CYCCNT - start) >= timeout_cycles)
        {
            return 0U;
        }
    }

    return 1U;
}

void DHT11_Init(void)
{
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->CYCCNT = 0U;
    DWT->CTRL |= DWT_CTRL_CYCCNTENA_Msk;

    dht11_cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
    if (dht11_cycles_per_us == 0U)
    {
        dht11_cycles_per_us = 1U;
    }

    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);

    /* DHT11 needs at least one second after power-up before the first read. */
    HAL_Delay(1000U);
}

DHT11_Status DHT11_Read(uint8_t *temperature, uint8_t *humidity)
{
    uint8_t data[5] = {0U, 0U, 0U, 0U, 0U};
    uint8_t byte_index;
    uint8_t bit_index;
    uint32_t high_start;
    uint32_t high_cycles;
    uint32_t primask;
    DHT11_Status status = DHT11_STATUS_TIMEOUT;

    if ((temperature == NULL) || (humidity == NULL))
    {
        return DHT11_STATUS_BAD_ARGUMENT;
    }

    /* MCU start signal: pull DATA low for at least 18 ms, then release it. */
    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_RESET);
    HAL_Delay(20U);
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);
    DHT11_DelayUs(30U);
    DHT11_SetInput();

    primask = __get_PRIMASK();
    __disable_irq();

    /* Sensor response: about 80 us low followed by about 80 us high. */
    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100U) == 0U)
    {
        goto read_finished;
    }
    if (DHT11_WaitForLevel(GPIO_PIN_SET, 100U) == 0U)
    {
        goto read_finished;
    }
    if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100U) == 0U)
    {
        goto read_finished;
    }

    /* Read five bytes: humidity, humidity decimal, temperature,
       temperature decimal, checksum. */
    for (byte_index = 0U; byte_index < 5U; byte_index++)
    {
        for (bit_index = 0U; bit_index < 8U; bit_index++)
        {
            if (DHT11_WaitForLevel(GPIO_PIN_SET, 100U) == 0U)
            {
                goto read_finished;
            }

            high_start = DWT->CYCCNT;
            if (DHT11_WaitForLevel(GPIO_PIN_RESET, 100U) == 0U)
            {
                goto read_finished;
            }
            high_cycles = DWT->CYCCNT - high_start;

            data[byte_index] <<= 1;
            if (high_cycles > (40U * dht11_cycles_per_us))
            {
                data[byte_index] |= 1U;
            }
        }
    }

    if (data[4] != (uint8_t)(data[0] + data[1] + data[2] + data[3]))
    {
        status = DHT11_STATUS_CHECKSUM_ERROR;
        goto read_finished;
    }

    *humidity = data[0];
    *temperature = data[2];
    status = DHT11_STATUS_OK;

read_finished:
    if (primask == 0U)
    {
        __enable_irq();
    }

    DHT11_SetOutput();
    HAL_GPIO_WritePin(DHT11_DATA_GPIO_Port, DHT11_DATA_Pin, GPIO_PIN_SET);
    return status;
}
