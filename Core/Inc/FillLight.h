#ifndef __FILL_LIGHT_H
#define __FILL_LIGHT_H

#include "stm32f1xx_hal.h"

#define FILL_LIGHT_ON_THRESHOLD_PERCENT  30U
#define FILL_LIGHT_OFF_THRESHOLD_PERCENT 40U

void FillLight_Init(void);
void FillLight_On(void);
void FillLight_Off(void);
void FillLight_AutoUpdate(uint8_t light_percent);
uint8_t FillLight_IsOn(void);

#endif
