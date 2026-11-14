#ifndef __BUZZER_H
#define __BUZZER_H

#include "stm32f1xx_hal.h"

typedef enum
{
    BUZZER_STATUS_OK = 0,
    BUZZER_STATUS_HAL_ERROR
} Buzzer_Status;

Buzzer_Status Buzzer_Init(void);
void Buzzer_On(void);
void Buzzer_Off(void);
void Buzzer_Beep(uint32_t duration_ms);

#endif
