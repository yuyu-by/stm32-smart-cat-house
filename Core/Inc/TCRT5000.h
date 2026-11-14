#ifndef __TCRT5000_H
#define __TCRT5000_H

#include "stm32f1xx_hal.h"

/* Most TCRT5000 digital modules output LOW when an object is detected. */
#define TCRT5000_DETECTED_LEVEL GPIO_PIN_RESET
#define TCRT5000_DEBOUNCE_COUNT 3U

void TCRT5000_Init(void);
void TCRT5000_Update(void);
GPIO_PinState TCRT5000_GetRawLevel(void);
uint8_t TCRT5000_IsDetected(void);

#endif
