// Created by Glif.
#include "88w8801/88w8801.h"
#ifdef WLAN_DEBUG
#include <stdio.h>
#include "st7735/st7735.h"

static uint16_t x = 0, y = 0;

int fputc(int ch, FILE *fp) {
    if (ch == *CLEAR_FLAG) return x = y = 0, lcdClear(), ch;
    if (ch != '\n') {
        lcdDrawChar(x, y, (uint8_t)ch);
        if ((x += 8) <= LCD_X - 8) return ch;
    }
    if ((x = 0, y += 16) > LCD_Y - 16) y = 0, lcdClear();
    return ch;
}
#endif
