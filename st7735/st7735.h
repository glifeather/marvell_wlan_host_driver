// Created by Glif.
#ifndef _ST7735_
#define _ST7735_
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

// 常用颜色
#define COLOR_BLACK   0x0000 // 黑色
#define COLOR_NAVY    0x000F // 海军蓝
#define COLOR_DGREEN  0x03E0 // 深绿色
#define COLOR_DCYAN   0x03EF // 深青色
#define COLOR_MAROON  0x7800 // 紫褐色
#define COLOR_DGRAY   0x7BEF // 深灰色
#define COLOR_BLUE    0x001F // 蓝色
#define COLOR_GREEN   0x07E0 // 绿色
#define COLOR_CYAN    0x07FF // 青色
#define COLOR_RED     0xF800 // 红色
#define COLOR_PURPLE  0x780F // 紫色
#define COLOR_YELLOW  0xFFE0 // 黄色
#define COLOR_MAGENTA 0xF81F // 品红
#define COLOR_OLIVE   0x7BE0 // 橄榄绿
#define COLOR_LGRAY   0xC618 // 灰白色
#define COLOR_WHITE   0xFFFF // 白色

typedef struct {
    SPI_TypeDef *SPIx;
    GPIO_TypeDef *CS_GPIO_Port;
    GPIO_TypeDef *DC_GPIO_Port;
    GPIO_TypeDef *RST_GPIO_Port;
    uint32_t CS_Pin;
    uint32_t DC_Pin;
    uint32_t RST_Pin;
} lcdSettings;

typedef struct {
    uint16_t x;
    uint16_t y;
    uint16_t u16Width;
    uint16_t u16Height;
    uint16_t u16ZoomWidth;
    uint16_t u16ZoomHeight;
    _Bool bBigEndian;
} lcdMeasurement;

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
// 绘制矩形（指定左上角坐标与右下角坐标）
void lcdDrawRect(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by, _Bool bFill);
// 绘制字符（8*16）
void lcdDrawChar(uint16_t x, uint16_t y, uint8_t u8Data);
// 绘制字符串
void lcdDrawStr(uint16_t x, uint16_t y, const uint8_t *pu8Data);
// 绘制8位有符号数
void lcdDrawInt8(uint16_t x, uint16_t y, int8_t i8Data);
// 绘制8位无符号数
void lcdDrawUInt8(uint16_t x, uint16_t y, uint8_t u8Data);
// 绘制16位有符号数
void lcdDrawInt16(uint16_t x, uint16_t y, int16_t i16Data);
// 绘制16位无符号数
void lcdDrawUInt16(uint16_t x, uint16_t y, uint16_t u16Data);
// 绘制中文字符串（传入二维数组起始地址）
void lcdDrawChinese(uint16_t x, uint16_t y, const uint8_t *pu8Data, uint8_t u8Size, uint8_t u8Length);
// 设置图像比例（默认1:1全屏）
void lcdSetImageScale(lcdMeasurement *lcdmImage);
// 绘制图像
void lcdDrawImage(const uint8_t *pu8Data);
#endif
