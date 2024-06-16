#include <stm32f4xx_ll_gpio.h>
#include "sccb.h"

// 25us
#define DELAY_SPAN 5

#define SCL_HIGH LL_GPIO_SetOutputPin(g_sccbsInit.SCL_GPIO_Port, g_sccbsInit.SCL_Pin)
#define SCL_LOW  LL_GPIO_ResetOutputPin(g_sccbsInit.SCL_GPIO_Port, g_sccbsInit.SCL_Pin)

#define SDA_OUT  LL_GPIO_SetPinMode(g_sccbsInit.SDA_GPIO_Port, g_sccbsInit.SDA_Pin, LL_GPIO_MODE_OUTPUT)
#define SDA_IN   LL_GPIO_SetPinMode(g_sccbsInit.SDA_GPIO_Port, g_sccbsInit.SDA_Pin, LL_GPIO_MODE_INPUT)
#define SDA_HIGH LL_GPIO_SetOutputPin(g_sccbsInit.SDA_GPIO_Port, g_sccbsInit.SDA_Pin)
#define SDA_LOW  LL_GPIO_ResetOutputPin(g_sccbsInit.SDA_GPIO_Port, g_sccbsInit.SDA_Pin)
#define SDA_IS_H LL_GPIO_IsInputPinSet(g_sccbsInit.SDA_GPIO_Port, g_sccbsInit.SDA_Pin)

static void sccbStart(void);
static void sccbStop(void);
static void sccbNACK(void);
static void sccbDelay(void);
static uint8_t sccbReadData8(void);
static bool sccbWriteData8(uint8_t u8Data);

static sccbSettings g_sccbsInit;

void sccbInit(sccbSettings *sccbsInit) {
    g_sccbsInit.u8Addr = sccbsInit->u8Addr;
    g_sccbsInit.SCL_GPIO_Port = sccbsInit->SCL_GPIO_Port;
    g_sccbsInit.SDA_GPIO_Port = sccbsInit->SDA_GPIO_Port;
    g_sccbsInit.SCL_Pin = sccbsInit->SCL_Pin;
    g_sccbsInit.SDA_Pin = sccbsInit->SDA_Pin;
}

uint8_t sccbReadReg(uint8_t u8Reg) {
    sccbStart();
    sccbWriteData8(g_sccbsInit.u8Addr);
    sccbWriteData8(u8Reg);
    sccbStop();
    sccbStart();
    sccbWriteData8(g_sccbsInit.u8Addr | 0x1);
    u8Reg = sccbReadData8();
    sccbNACK();
    sccbStop();
    return u8Reg;
}

bool sccbWriteReg(uint8_t u8Reg, uint8_t u8Data) {
    sccbStart();
    u8Reg = sccbWriteData8(g_sccbsInit.u8Addr) && sccbWriteData8(u8Reg) && sccbWriteData8(u8Data);
    sccbStop();
    return u8Reg;
}

static void sccbStart(void) {
    SDA_HIGH;
    SCL_HIGH;
    sccbDelay();
    SDA_LOW;
    sccbDelay();
    SCL_LOW;
    sccbDelay();
}

static void sccbStop(void) {
    SDA_LOW;
    sccbDelay();
    SCL_HIGH;
    sccbDelay();
    SDA_HIGH;
    sccbDelay();
}

static void sccbNACK(void) {
    SDA_HIGH;
    SCL_HIGH;
    sccbDelay();
    SCL_LOW;
    sccbDelay();
    SDA_LOW;
    sccbDelay();
}

static void sccbDelay(void) {
    uint16_t u16Span = DELAY_SPAN * SystemCoreClock / 0xF4240;
    while (u16Span--) __NOP();
}

static uint8_t sccbReadData8(void) {
    SDA_IN;
    uint8_t i = 8, u8Data = 0;
    while (i--) {
        sccbDelay();
        SCL_HIGH;
        u8Data |= SDA_IS_H << i;
        sccbDelay();
        SCL_LOW;
    }
    SDA_OUT;
    sccbDelay();
    return u8Data;
}

static bool sccbWriteData8(uint8_t u8Data) {
    uint8_t i = 8;
    while (i--) {
        u8Data & 1 << i ? SDA_HIGH : SDA_LOW;
        sccbDelay();
        SCL_HIGH;
        sccbDelay();
        SCL_LOW;
    }
    SDA_IN;
    sccbDelay();
    SCL_HIGH;
    sccbDelay();
    i = SDA_IS_H;
    SCL_LOW;
    SDA_OUT;
    sccbDelay();
    return !i;
}
