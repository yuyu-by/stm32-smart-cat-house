#include "TCRT5000.h"
#include "main.h"

static GPIO_PinState tcrt5000_raw_level = GPIO_PIN_SET;
static GPIO_PinState tcrt5000_last_level = GPIO_PIN_SET;
static uint8_t tcrt5000_same_count = 0U;
static uint8_t tcrt5000_detected = 0U;

void TCRT5000_Init(void)
{
    tcrt5000_raw_level = HAL_GPIO_ReadPin(TCRT5000_DO_GPIO_Port,
                                          TCRT5000_DO_Pin);
    tcrt5000_last_level = tcrt5000_raw_level;
    tcrt5000_same_count = TCRT5000_DEBOUNCE_COUNT;
    tcrt5000_detected =
        (tcrt5000_raw_level == TCRT5000_DETECTED_LEVEL) ? 1U : 0U;
}

void TCRT5000_Update(void)
{
    tcrt5000_raw_level = HAL_GPIO_ReadPin(TCRT5000_DO_GPIO_Port,
                                          TCRT5000_DO_Pin);

    if (tcrt5000_raw_level == tcrt5000_last_level)
    {
        if (tcrt5000_same_count < TCRT5000_DEBOUNCE_COUNT)
        {
            tcrt5000_same_count++;
        }
    }
    else
    {
        tcrt5000_last_level = tcrt5000_raw_level;
        tcrt5000_same_count = 1U;
    }

    if (tcrt5000_same_count >= TCRT5000_DEBOUNCE_COUNT)
    {
        tcrt5000_detected =
            (tcrt5000_last_level == TCRT5000_DETECTED_LEVEL) ? 1U : 0U;
    }
}

GPIO_PinState TCRT5000_GetRawLevel(void)
{
    return tcrt5000_raw_level;
}

uint8_t TCRT5000_IsDetected(void)
{
    return tcrt5000_detected;
}
