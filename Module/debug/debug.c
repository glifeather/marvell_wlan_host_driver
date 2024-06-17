// Created by Glif.
#include "88w8801/88w8801.h"
#ifdef WLAN_DEBUG
#include <stdio.h>
#include "st7735s/st7735s.h"

static uint16_t x = 0, y = 0;

#ifdef __GNUC__
int __io_putchar(int ch) {
#else
int fputc(int ch, FILE *fp) {
    UNUSED(fp);
#endif
    switch (ch) {
    case CLEAR_FLAG: x = y = 0, lcdClear(); break;
    case '\n':
        if ((x = 0, y += LCD_FONT_HEIGHT) > LCD_Y - LCD_FONT_HEIGHT) y = 0, lcdClear();
        break;
    default:
        if (x > LCD_X - LCD_FONT_WIDTH && (x = 0, y += LCD_FONT_HEIGHT) > LCD_Y - LCD_FONT_HEIGHT) y = 0, lcdClear();
        lcdDrawSBC((uint8_t)ch, x, y), x += LCD_FONT_WIDTH;
        break;
    }
    return ch;
}
#endif
