#include "FillLight.h"
#include "main.h"

static uint8_t fill_light_is_on = 0U;

void FillLight_Init(void)
{
    FillLight_Off();
}

void FillLight_On(void)
{
    HAL_GPIO_WritePin(FILL_LIGHT_CTRL_GPIO_Port,
                      FILL_LIGHT_CTRL_Pin,
                      GPIO_PIN_SET);
    fill_light_is_on = 1U;
}

void FillLight_Off(void)
{
    HAL_GPIO_WritePin(FILL_LIGHT_CTRL_GPIO_Port,
                      FILL_LIGHT_CTRL_Pin,
                      GPIO_PIN_RESET);
    fill_light_is_on = 0U;
}

void FillLight_AutoUpdate(uint8_t light_percent)
{
    if (light_percent <= FILL_LIGHT_ON_THRESHOLD_PERCENT)
    {
        FillLight_On();
    }
    else if (light_percent >= FILL_LIGHT_OFF_THRESHOLD_PERCENT)
    {
        FillLight_Off();
    }
}

uint8_t FillLight_IsOn(void)
{
    return fill_light_is_on;
}
