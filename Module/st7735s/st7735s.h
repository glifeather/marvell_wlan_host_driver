// Created by Glif.
#ifndef _ST7735S_
#define _ST7735S_
#include <stdbool.h>
#include <stm32f4xx.h>

// 屏幕方向
// 0 竖屏
// 1 竖屏 旋转180度
// 2 横屏
// 3 横屏 旋转180度
#ifndef LCD_DIRECTION
#define LCD_DIRECTION 0
#endif

// 屏幕大小
#define LCD_LONG  160
#define LCD_SHORT 128
#ifndef LCD_X
#define LCD_X (LCD_DIRECTION > 1 ? LCD_LONG : LCD_SHORT)
#endif
#ifndef LCD_Y
#define LCD_Y (LCD_DIRECTION > 1 ? LCD_SHORT : LCD_LONG)
#endif

// 字体大小
// 6x12, 8x16
#define LCD_FONT_WIDTH  6
#define LCD_FONT_HEIGHT 12

// 常用颜色
#define LCD_COLOR_BLACK   0x0000 // 黑色
#define LCD_COLOR_NAVY    0x000F // 海军蓝
#define LCD_COLOR_DGREEN  0x03E0 // 深绿色
#define LCD_COLOR_DCYAN   0x03EF // 深青色
#define LCD_COLOR_MAROON  0x7800 // 紫褐色
#define LCD_COLOR_DGRAY   0x7BEF // 深灰色
#define LCD_COLOR_BLUE    0x001F // 蓝色
#define LCD_COLOR_GREEN   0x07E0 // 绿色
#define LCD_COLOR_CYAN    0x07FF // 青色
#define LCD_COLOR_RED     0xF800 // 红色
#define LCD_COLOR_PURPLE  0x780F // 紫色
#define LCD_COLOR_YELLOW  0xFFE0 // 黄色
#define LCD_COLOR_MAGENTA 0xF81F // 品红
#define LCD_COLOR_OLIVE   0x7BE0 // 橄榄绿
#define LCD_COLOR_LGRAY   0xC618 // 灰白色
#define LCD_COLOR_WHITE   0xFFFF // 白色

typedef struct {
    SPI_TypeDef *SPIx;
    GPIO_TypeDef *CS_GPIO_Port;
    GPIO_TypeDef *DC_GPIO_Port;
    GPIO_TypeDef *RST_GPIO_Port;
    uint32_t CS_Pin;
    uint32_t DC_Pin;
    uint32_t RST_Pin;
} lcdSettings;

// 初始化（原点位于左上角）
void lcdInit(lcdSettings *lcdsInit);
// 设置前景色（默认黑色）
void lcdSetForeColor(uint16_t u16ForeColor);
// 设置背景色（默认白色）
void lcdSetBackColor(uint16_t u16BackColor);
// 使用背景色填充屏幕
void lcdClear(void);
// 绘制点
void lcdDrawPoint(uint16_t x, uint16_t y);
// 绘制矩形（传入左上角及右下角坐标）
void lcdDrawRect(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by, bool bFill);
// 绘制图像（RGB565格式）
void lcdDrawImage(const uint8_t *pu8Data, uint16_t x, uint16_t y, uint16_t u16Width, uint16_t u16Height, bool bBigEndian);
// 绘制半角字符
void lcdDrawSBC(uint8_t u8Data, uint16_t x, uint16_t y);
// 绘制半角字符串（传入字符串内容）
void lcdDrawSBCStr(const uint8_t *pu8Data, uint16_t x, uint16_t y);
// 绘制全角字符串（传入字符串字模）
void lcdDrawDBCStr(const uint8_t *pu8Data, uint16_t x, uint16_t y, uint8_t u8Size, uint8_t u8Length);
// 绘制8位有符号数
void lcdDrawInt8(int8_t i8Data, uint16_t x, uint16_t y);
// 绘制8位无符号数
void lcdDrawUInt8(uint8_t u8Data, uint16_t x, uint16_t y);
// 绘制16位有符号数
void lcdDrawInt16(int16_t i16Data, uint16_t x, uint16_t y);
// 绘制16位无符号数
void lcdDrawUInt16(uint16_t u16Data, uint16_t x, uint16_t y);
#endif
