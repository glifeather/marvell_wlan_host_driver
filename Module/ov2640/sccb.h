#ifndef _SCCB_
#define _SCCB_
#include <stdbool.h>
#include <stm32f4xx.h>

typedef struct {
    uint8_t u8Addr;
    GPIO_TypeDef *SCL_GPIO_Port;
    GPIO_TypeDef *SDA_GPIO_Port;
    uint32_t SCL_Pin;
    uint32_t SDA_Pin;
} sccbSettings;

void sccbInit(sccbSettings *sccbsInit);
uint8_t sccbReadReg(uint8_t u8Reg);
bool sccbWriteReg(uint8_t u8Reg, uint8_t u8Data);
#endif
