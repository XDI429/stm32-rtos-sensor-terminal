#include "ssd1306.h"
#include "oled_font.h"
#include <stdio.h> // 为了使用 printf 报错

extern I2C_HandleTypeDef hi2c1; // 确保你在 i2c.h 里定义了 hi2c1

void OLED_WriteCmd(uint8_t cmd) {
    uint8_t data[2] = {0x00, cmd};
    // 超时时间改为 100ms，如果失败则串口报错
    if (HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, data, 2, 100) != HAL_OK) {
        printf("OLED CMD Error!\r\n"); 
    }
}

void OLED_WriteData(uint8_t *data, uint16_t size) {
    uint8_t buffer[128];
    buffer[0] = 0x40;
    for (uint16_t i = 0; i < size; i++) {
        buffer[i + 1] = data[i];
    }
    // 超时时间改为 100ms，如果失败则串口报错
    if (HAL_I2C_Master_Transmit(&hi2c1, OLED_ADDR, buffer, size + 1, 100) != HAL_OK) {
        printf("OLED DATA Error!\r\n");
    }
}

void OLED_Init(void) {
    HAL_Delay(100);
    OLED_WriteCmd(0xAE); // 关闭显示
    OLED_WriteCmd(0x20); // 设置内存寻址模式
    OLED_WriteCmd(0x00); // 水平寻址
    OLED_WriteCmd(0xB0); // 页地址
    OLED_WriteCmd(0xC8); // 扫描方向
    OLED_WriteCmd(0x00); // 低列地址
    OLED_WriteCmd(0x10); // 高列地址
    OLED_WriteCmd(0x40); // 起始行
    OLED_WriteCmd(0x81); // 对比度
    OLED_WriteCmd(0xFF); // 最高亮度
    OLED_WriteCmd(0xA1); // 段重映射
    OLED_WriteCmd(0xA6); // 正常显示
    OLED_WriteCmd(0xA8); // 多路复用
    OLED_WriteCmd(0x3F);
    OLED_WriteCmd(0xA4); // 输出跟随RAM
    OLED_WriteCmd(0xD3); // 显示偏移
    OLED_WriteCmd(0x00);
    OLED_WriteCmd(0xD5); // 时钟分频
    OLED_WriteCmd(0xF0);
    OLED_WriteCmd(0xD9); // 预充电周期
    OLED_WriteCmd(0x22);
    OLED_WriteCmd(0xDA); // COM引脚配置
    OLED_WriteCmd(0x12);
    OLED_WriteCmd(0xDB); // VCOMH
    OLED_WriteCmd(0x20);
    OLED_WriteCmd(0x8D); // 电荷泵
    OLED_WriteCmd(0x14); // 开启电荷泵
    OLED_WriteCmd(0xAF); // 开启显示
    OLED_Clear();
}

void OLED_SetCursor(uint8_t x, uint8_t y) {
    OLED_WriteCmd(0xB0 + y);
    OLED_WriteCmd(((x & 0xF0) >> 4) | 0x10);
    OLED_WriteCmd((x & 0x0F) | 0x00);
}

void OLED_Clear(void) {
    uint8_t data[128];
    memset(data, 0, 128);
    for (uint8_t i = 0; i < 8; i++) {
        OLED_SetCursor(0, i);
        OLED_WriteData(data, 128);
    }
}

void OLED_ShowString(uint8_t x, uint8_t y, char *str, uint8_t size) {
    // 这里为了精简，只实现了 6x8 字库
    while (*str != '\0') {
        if (x > 122) { x = 0; y++; }
        if (y > 7) { y = 0; x = 0; }
        OLED_SetCursor(x, y);
        
        // 字符映射：简单映射 ASCII 32 (' ') 到 90 ('Z')
        uint8_t char_idx = 0;
        if (*str >= ' ' && *str <= 'Z') {
            char_idx = *str - ' ';
        } else {
            char_idx = 0; // 其他字符显示空格
        }
        
        OLED_WriteData((uint8_t*)&F6x8[char_idx], 6);
        x += 6;
        str++;
    }
}

void OLED_ShowNum(uint8_t x, uint8_t y, uint32_t num, uint8_t len, uint8_t size) {
    char buf[12];
    snprintf(buf, sizeof(buf), "%lu", (unsigned long)num);
    OLED_ShowString(x, y, buf, size);
}