#ifndef __SSD1306_H
#define __SSD1306_H

#include "main.h"
#include "i2c.h"
#include <string.h>
#include <stdio.h>

#define OLED_ADDR 0x7A // 默认I2C地址 (0x3C << 1)

void OLED_Init(void);
void OLED_Clear(void);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size);
void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size);
void OLED_ShowFloat(uint8_t x, uint8_t y, float num, uint8_t int_len, uint8_t dec_len, uint8_t size);

#endif