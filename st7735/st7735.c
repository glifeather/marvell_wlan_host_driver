// Created by Glif.
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_ll_utils.h>
#include "st7735.h"

#define CS_RESET  LL_GPIO_ResetOutputPin(g_lcdsInit.CS_GPIO_Port, g_lcdsInit.CS_Pin)
#define CS_SET    LL_GPIO_SetOutputPin(g_lcdsInit.CS_GPIO_Port, g_lcdsInit.CS_Pin)
#define DC_RESET  LL_GPIO_ResetOutputPin(g_lcdsInit.DC_GPIO_Port, g_lcdsInit.DC_Pin)
#define DC_SET    LL_GPIO_SetOutputPin(g_lcdsInit.DC_GPIO_Port, g_lcdsInit.DC_Pin)
#define RST_RESET LL_GPIO_ResetOutputPin(g_lcdsInit.RST_GPIO_Port, g_lcdsInit.RST_Pin)
#define RST_SET   LL_GPIO_SetOutputPin(g_lcdsInit.RST_GPIO_Port, g_lcdsInit.RST_Pin)

#define LCD_FONT_WIDTH_BYTES ((LCD_FONT_WIDTH + 7) / 8)

static lcdSettings g_lcdsInit;
static uint16_t g_u16ForeColor = LCD_COLOR_BLACK, g_u16BackColor = LCD_COLOR_WHITE;
// Defined in sbc_data.c
extern const uint8_t g_ppu8SBCData[][LCD_FONT_HEIGHT * LCD_FONT_WIDTH_BYTES];

static void lcdWriteCommand(uint8_t u8Command) {
    DC_RESET;
    while (!LL_SPI_IsActiveFlag_TXE(g_lcdsInit.SPIx));
    LL_SPI_TransmitData8(g_lcdsInit.SPIx, u8Command);
    while (!LL_SPI_IsActiveFlag_RXNE(g_lcdsInit.SPIx));
    LL_SPI_ReceiveData8(g_lcdsInit.SPIx);
}

static void lcdWriteData8(uint8_t u8Data) {
    DC_SET;
    while (!LL_SPI_IsActiveFlag_TXE(g_lcdsInit.SPIx));
    LL_SPI_TransmitData8(g_lcdsInit.SPIx, u8Data);
    while (!LL_SPI_IsActiveFlag_RXNE(g_lcdsInit.SPIx));
    LL_SPI_ReceiveData8(g_lcdsInit.SPIx);
}

static void lcdWriteData16(uint16_t u16Data) {
    lcdWriteData8(u16Data >> 8);
    lcdWriteData8(u16Data & 0xFF);
}

static void lcdInitArea(uint16_t xStart, uint16_t yStart, uint16_t xEnd, uint16_t yEnd) {
    // CASET
    lcdWriteCommand(0x2A);
    lcdWriteData16(xStart + 2 - LCD_DIRECTION / 2);
    lcdWriteData16(xEnd + 2 - LCD_DIRECTION / 2);
    // RASET
    lcdWriteCommand(0x2B);
    lcdWriteData16(yStart + 1 + LCD_DIRECTION / 2);
    lcdWriteData16(yEnd + 1 + LCD_DIRECTION / 2);
    // RAMWR
    lcdWriteCommand(0x2C);
}

void lcdClear(void) {
    CS_RESET;
    lcdInitArea(0, 0, LCD_X - 1, LCD_Y - 1);
    uint32_t u32Area = LCD_X * LCD_Y;
    while (u32Area--) lcdWriteData16(g_u16BackColor);
    CS_SET;
}

void lcdInit(lcdSettings *lcdsInit) {
    g_lcdsInit.SPIx = lcdsInit->SPIx;
    g_lcdsInit.CS_GPIO_Port = lcdsInit->CS_GPIO_Port;
    g_lcdsInit.DC_GPIO_Port = lcdsInit->DC_GPIO_Port;
    g_lcdsInit.RST_GPIO_Port = lcdsInit->RST_GPIO_Port;
    g_lcdsInit.CS_Pin = lcdsInit->CS_Pin;
    g_lcdsInit.DC_Pin = lcdsInit->DC_Pin;
    g_lcdsInit.RST_Pin = lcdsInit->RST_Pin;
    LL_SPI_Enable(g_lcdsInit.SPIx);
    RST_RESET;
    LL_mDelay(1);
    RST_SET;
    LL_mDelay(120);
    CS_RESET;
    // SLPOUT
    lcdWriteCommand(0x11);
    LL_mDelay(120);
    // FRMCTR1
    lcdWriteCommand(0xB1);
    lcdWriteData8(0x05);
    lcdWriteData8(0x3C);
    lcdWriteData8(0x3C);
    // FRMCTR2
    lcdWriteCommand(0xB2);
    lcdWriteData8(0x05);
    lcdWriteData8(0x3C);
    lcdWriteData8(0x3C);
    // FRMCTR3
    lcdWriteCommand(0xB3);
    lcdWriteData8(0x05);
    lcdWriteData8(0x3C);
    lcdWriteData8(0x3C);
    lcdWriteData8(0x05);
    lcdWriteData8(0x3C);
    lcdWriteData8(0x3C);
    // INVCTR
    lcdWriteCommand(0xB4);
    lcdWriteData8(0x03);
    // PWCTR1
    lcdWriteCommand(0xC0);
    lcdWriteData8(0x28);
    lcdWriteData8(0x08);
    lcdWriteData8(0x04);
    // PWCTR2
    lcdWriteCommand(0xC1);
    lcdWriteData8(0xC0);
    // PWCTR3
    lcdWriteCommand(0xC2);
    lcdWriteData8(0x0D);
    lcdWriteData8(0x00);
    // PWCTR4
    lcdWriteCommand(0xC3);
    lcdWriteData8(0x8D);
    lcdWriteData8(0x2A);
    // PWCTR5
    lcdWriteCommand(0xC4);
    lcdWriteData8(0x8D);
    lcdWriteData8(0xEE);
    // VMCTR1
    lcdWriteCommand(0xC5);
    lcdWriteData8(0x1A);
    // MADCTL
    lcdWriteCommand(0x36);
    switch (LCD_DIRECTION) {
    case 0: lcdWriteData8(0xC0); break;
    case 1: lcdWriteData8(0x00); break;
    case 2: lcdWriteData8(0xA0); break;
    case 3: lcdWriteData8(0x60); break;
    }
    // GMCTRP1
    lcdWriteCommand(0xE0);
    lcdWriteData8(0x04);
    lcdWriteData8(0x22);
    lcdWriteData8(0x07);
    lcdWriteData8(0x0A);
    lcdWriteData8(0x2E);
    lcdWriteData8(0x30);
    lcdWriteData8(0x25);
    lcdWriteData8(0x2A);
    lcdWriteData8(0x28);
    lcdWriteData8(0x26);
    lcdWriteData8(0x2E);
    lcdWriteData8(0x3A);
    lcdWriteData8(0x00);
    lcdWriteData8(0x01);
    lcdWriteData8(0x03);
    lcdWriteData8(0x13);
    // GMCTRN1
    lcdWriteCommand(0xE1);
    lcdWriteData8(0x04);
    lcdWriteData8(0x16);
    lcdWriteData8(0x06);
    lcdWriteData8(0x0D);
    lcdWriteData8(0x2D);
    lcdWriteData8(0x26);
    lcdWriteData8(0x23);
    lcdWriteData8(0x27);
    lcdWriteData8(0x27);
    lcdWriteData8(0x25);
    lcdWriteData8(0x2D);
    lcdWriteData8(0x3B);
    lcdWriteData8(0x00);
    lcdWriteData8(0x01);
    lcdWriteData8(0x04);
    lcdWriteData8(0x13);
    // COLMOD
    lcdWriteCommand(0x3A);
    lcdWriteData8(0x05);
    // DISPON
    lcdWriteCommand(0x29);
    CS_SET;
    lcdClear();
}

void lcdSetForeColor(uint16_t u16ForeColor) { g_u16ForeColor = u16ForeColor; }

void lcdSetBackColor(uint16_t u16BackColor) { g_u16BackColor = u16BackColor; }

void lcdDrawPoint(uint16_t x, uint16_t y) {
    CS_RESET;
    lcdInitArea(x, y, x, y);
    lcdWriteData16(g_u16ForeColor);
    CS_SET;
}

void lcdDrawRect(uint16_t ax, uint16_t ay, uint16_t bx, uint16_t by, bool bFill) {
    if (ax > bx) bx ^= ax ^= bx, ax ^= bx;
    if (ay > by) by ^= ay ^= by, ay ^= by;
    const uint16_t u16Position = ay;
    if (bFill) {
        do {
            do lcdDrawPoint(ax, ay); while (ay++ < by);
            ay = u16Position;
        } while (ax++ < bx);
    } else {
        do {
            lcdDrawPoint(ax, ay);
            lcdDrawPoint(bx, ay);
        } while (ay++ < by);
        ay = u16Position;
        do {
            lcdDrawPoint(ax, ay);
            lcdDrawPoint(ax, by);
        } while (ax++ < bx);
    }
}

void lcdDrawImage(const uint8_t *pu8Data, uint16_t x, uint16_t y, uint16_t u16Width, uint16_t u16Height, bool bBigEndian) {
    CS_RESET;
    lcdInitArea(x, y, x + u16Width - 1, y + u16Height - 1);
    pu8Data -= 2;
    uint32_t u32Area = u16Width * u16Height;
    if (bBigEndian) while (u32Area--) lcdWriteData8(*++pu8Data), lcdWriteData8(*++pu8Data);
    else while (u32Area--) lcdWriteData16(*(const uint16_t *)(pu8Data += 2));
    CS_SET;
}

void lcdDrawSBC(uint8_t u8Data, uint16_t x, uint16_t y) {
    if (u8Data < ' ' || u8Data > '~') return;
    CS_RESET;
    lcdInitArea(x, y, x + LCD_FONT_WIDTH - 1, y + LCD_FONT_HEIGHT - 1);
    const uint8_t *pu8Element = *(g_ppu8SBCData + u8Data - ' ');
    u8Data = sizeof(*g_ppu8SBCData);
    while (u8Data--) {
        x = u8Data % LCD_FONT_WIDTH_BYTES ? 0 : LCD_FONT_WIDTH_BYTES * 8 - LCD_FONT_WIDTH;
        y = 8;
        while (y-- > x) lcdWriteData16(*pu8Element & 1 << y ? g_u16ForeColor : g_u16BackColor);
        ++pu8Element;
    }
    CS_SET;
}

void lcdDrawSBCStr(const uint8_t *pu8Data, uint16_t x, uint16_t y) {
    uint8_t u8Length = -1;
    while (*(pu8Data + (++u8Length)) != '\0') lcdDrawSBC(*(pu8Data + u8Length), x + u8Length * LCD_FONT_WIDTH, y);
}

void lcdDrawDBCStr(const uint8_t *pu8Data, uint16_t x, uint16_t y, uint8_t u8Size, uint8_t u8Length) {
    CS_RESET;
    const uint16_t u16FontWidth = u8Size * u8Length;
    lcdInitArea(x, y, x + u16FontWidth - 1, y + u8Size - 1);
    const uint8_t u8FontWidthBytes = (u8Size + 7) / 8;
    uint8_t u8Element;
    x = u16FontWidth;
    while (x--) {
        u8Element = u8FontWidthBytes;
        while (--u8Element) {
            y = 8;
            while (y--) lcdWriteData16(*pu8Data & 1 << y ? g_u16ForeColor : g_u16BackColor);
            ++pu8Data;
        }
        u8Element = u8FontWidthBytes * 8 - u8Size;
        y = 8;
        while (y-- > u8Element) lcdWriteData16(*pu8Data & 1 << y ? g_u16ForeColor : g_u16BackColor);
        pu8Data += (u8Size - (x % u8Length ? 1 : u16FontWidth)) * u8FontWidthBytes + 1;
    }
    CS_SET;
}

void lcdDrawInt8(int8_t i8Data, uint16_t x, uint16_t y) {
    uint8_t pu8Order[3];
    lcdDrawSBC(i8Data >= 0 ? (*pu8Order = i8Data, ' ') : (*pu8Order = -i8Data, '-'), x, y);
    i8Data = 2;
    do *(pu8Order + i8Data) = *pu8Order % 10, *pu8Order /= 10; while (*pu8Order && --i8Data);
    while (i8Data < 3) lcdDrawSBC(*(pu8Order + i8Data++) + '0', x += LCD_FONT_WIDTH, y);
}

void lcdDrawUInt8(uint8_t u8Data, uint16_t x, uint16_t y) {
    uint8_t pu8Order[3];
    x -= LCD_FONT_WIDTH, *pu8Order = u8Data, u8Data = 2;
    do *(pu8Order + u8Data) = *pu8Order % 10, *pu8Order /= 10; while (*pu8Order && --u8Data);
    while (u8Data < 3) lcdDrawSBC(*(pu8Order + u8Data++) + '0', x += LCD_FONT_WIDTH, y);
}

void lcdDrawInt16(int16_t i16Data, uint16_t x, uint16_t y) {
    uint16_t pu16Order[5];
    lcdDrawSBC(i16Data >= 0 ? (*pu16Order = i16Data, ' ') : (*pu16Order = -i16Data, '-'), x, y);
    i16Data = 4;
    do *(pu16Order + i16Data) = *pu16Order % 10, *pu16Order /= 10; while (*pu16Order && --i16Data);
    while (i16Data < 5) lcdDrawSBC(*(pu16Order + i16Data++) + '0', x += LCD_FONT_WIDTH, y);
}

void lcdDrawUInt16(uint16_t u16Data, uint16_t x, uint16_t y) {
    uint16_t pu16Order[5];
    x -= LCD_FONT_WIDTH, *pu16Order = u16Data, u16Data = 4;
    do *(pu16Order + u16Data) = *pu16Order % 10, *pu16Order /= 10; while (*pu16Order && --u16Data);
    while (u16Data < 5) lcdDrawSBC(*(pu16Order + u16Data++) + '0', x += LCD_FONT_WIDTH, y);
}
