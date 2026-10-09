#include "oled.h"
#include "oledfont.h"
#include <string.h>

/* OLED显存：128列、8页，每页8个像素 */
static uint8_t OLED_GRAM[OLED_WIDTH][8];

/* 发送一条SSD1306命令 */
static HAL_StatusTypeDef OLED_WriteCommand(uint8_t command)
{
    uint8_t data[2];

    data[0] = 0x00;
    data[1] = command;

    return HAL_I2C_Master_Transmit(
        &hi2c1,
        OLED_ADDRESS,
        data,
        2,
        100
    );
}

/* 发送一页显示数据 */
static HAL_StatusTypeDef OLED_WriteData(uint8_t *data, uint16_t size)
{
    uint8_t packet[OLED_WIDTH + 1];

    if (size > OLED_WIDTH)
        return HAL_ERROR;

    packet[0] = 0x40;
    memcpy(&packet[1], data, size);

    return HAL_I2C_Master_Transmit(
        &hi2c1,
        OLED_ADDRESS,
        packet,
        size + 1,
        100
    );
}

uint8_t OLED_Init(void)
{
    static const uint8_t init_commands[] =
    {
        0xAE,       /* 关闭显示 */
        0x00,
        0x10,
        0x40,
        0x81, 0xCF,
        0xA1,
        0xC8,
        0xA6,
        0xA8, 0x3F,
        0xD3, 0x00,
        0xD5, 0x80,
        0xD9, 0xF1,
        0xDA, 0x12,
        0xDB, 0x40,
        0x20, 0x02,
        0x8D, 0x14,
        0xA4,
        0xA6,
        0xAF        /* 开启显示 */
    };

    uint16_t i;

    HAL_Delay(100);

    /* 检查地址0x3C的OLED是否回应 */
    if (HAL_I2C_IsDeviceReady(
            &hi2c1,
            OLED_ADDRESS,
            3,
            100) != HAL_OK)
    {
        return 0;
    }

    for (i = 0;
         i < sizeof(init_commands);
         i++)
    {
        if (OLED_WriteCommand(init_commands[i]) != HAL_OK)
            return 0;
    }

    OLED_Clear();
    OLED_Refresh();//会通过IIC将保存到数组里的数组显示到OLED上

    return 1;
}

void OLED_Clear(void)
{
    memset(OLED_GRAM, 0, sizeof(OLED_GRAM));
}

void OLED_Refresh(void)
{
    uint8_t page;
    uint8_t column_data[OLED_WIDTH];
    uint16_t x;

    for (page = 0; page < 8; page++)
    {
        OLED_WriteCommand(0xB0 + page);
        OLED_WriteCommand(0x00);
        OLED_WriteCommand(0x10);

        for (x = 0; x < OLED_WIDTH; x++)
        {
            column_data[x] = OLED_GRAM[x][page];
        }

        OLED_WriteData(column_data, OLED_WIDTH);
    }
}

void OLED_DrawPoint(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    OLED_GRAM[x][y / 8] |= (1U << (y % 8));
}

void OLED_ClearPoint(uint8_t x, uint8_t y)
{
    if (x >= OLED_WIDTH || y >= OLED_HEIGHT)
        return;

    OLED_GRAM[x][y / 8] &= ~(1U << (y % 8));
}

void OLED_ShowChar(
    uint8_t x,
    uint8_t y,
    char chr,
    uint8_t size)
{
    uint8_t i;
    uint8_t bit;
    uint8_t data;
    uint8_t bytes;
    uint8_t start_y = y;
    uint8_t index;

    if (chr < ' ' || chr > '~')
        return;

    index = (uint8_t)(chr - ' ');
    bytes = (size / 8 + ((size % 8) ? 1 : 0))
            * (size / 2);

    for (i = 0; i < bytes; i++)
    {
        if (size == 12)
            data = asc2_1206[index][i];
        else if (size == 16)
            data = asc2_1608[index][i];
        else if (size == 24)
            data = asc2_2412[index][i];
        else
            return;

        for (bit = 0; bit < 8; bit++)
        {
            if (data & 0x80)
                OLED_DrawPoint(x, y);
            else
                OLED_ClearPoint(x, y);

            data <<= 1;
            y++;

            if ((y - start_y) == size)
            {
                y = start_y;
                x++;
                break;
            }
        }
    }
}

void OLED_ShowString(
    uint8_t x,
    uint8_t y,
    const char *str,
    uint8_t size)
{
    while (*str >= ' ' && *str <= '~')
    {
        OLED_ShowChar(x, y, *str, size);

        x += size / 2;

        if (x > OLED_WIDTH - size / 2)
        {
            x = 0;
            y += size;
        }

        if (y >= OLED_HEIGHT)
            break;

        str++;
    }
}
