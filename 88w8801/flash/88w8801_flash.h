#ifndef _88W8801_FLASH_
#define _88W8801_FLASH_
#include <stm32f4xx.h>

// JEDEC ID
// Manufacturer ID
#define MANUFACTURER_ID_WINBOND 0xEF
// Device ID
#define DEVICE_ID_W25Q16DV 0x4015

typedef struct {
    SPI_TypeDef *SPIx;
    GPIO_TypeDef *CS_GPIO_Port;
    uint32_t CS_Pin;
} flashSettings;

void flashInit(flashSettings *flashsInit);
uint32_t flashGetJEDECID(void);
void flashReadMemory(uint32_t u32Address, uint8_t *pu8Data, uint32_t u32Length);
uint32_t flashEraseMemory(uint32_t u32Address, uint32_t u32Length);
uint32_t flashWriteMemory(uint32_t u32Address, uint8_t *pu8Data, uint32_t u32Length);
#endif
