#ifndef __OLED_H
#define __OLED_H

#include "main.h"
#include "i2c.h"
#include <stdint.h>

#define OLED_ADDRESS   (0x3C << 1)
#define OLED_WIDTH     128
#define OLED_HEIGHT    64

uint8_t OLED_Init(void);
void OLED_Clear(void);
void OLED_Refresh(void);
void OLED_DrawPoint(uint8_t x, uint8_t y);
void OLED_ClearPoint(uint8_t x, uint8_t y);
void OLED_ShowChar(uint8_t x, uint8_t y, char chr, uint8_t size);
void OLED_ShowString(uint8_t x, uint8_t y, const char *str, uint8_t size);

#endif

