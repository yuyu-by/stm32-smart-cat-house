#include "Buzzer.h"
#include "tim.h"

Buzzer_Status Buzzer_Init(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);

    if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK)
    {
        return BUZZER_STATUS_HAL_ERROR;
    }

    return BUZZER_STATUS_OK;
}

void Buzzer_On(void)
{
    uint32_t half_period = (__HAL_TIM_GET_AUTORELOAD(&htim3) + 1U) / 2U;

    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, half_period);
}

void Buzzer_Off(void)
{
    __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, 0U);
}

void Buzzer_Beep(uint32_t duration_ms)
{
    Buzzer_On();
    HAL_Delay(duration_ms);
    Buzzer_Off();
}
