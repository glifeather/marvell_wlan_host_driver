#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_spi.h>
#include <stm32f4xx_ll_utils.h>
#include "88w8801_flash.h"

#define WRITE_ENABLE 0x6
#define READ_SR1     0x5
#define ERASE_SECTOR 0x20
#define PAGE_PROGRAM 0x2
#define JEDEC_ID     0x9F
#define READ_MEMORY  0x3

#define SR1_BUSY 0x1

#define CS_RESET    LL_GPIO_ResetOutputPin(g_flashsInit.CS_GPIO_Port, g_flashsInit.CS_Pin)
#define CS_SET      LL_GPIO_SetOutputPin(g_flashsInit.CS_GPIO_Port, g_flashsInit.CS_Pin)
#define PLACEHOLDER 0xFF
#define SECTOR_SIZE 0x1000
#define PAGE_SIZE   0x100

static flashSettings g_flashsInit;

static uint8_t flashWriteData8(uint8_t u8Data) {
    while (!LL_SPI_IsActiveFlag_TXE(g_flashsInit.SPIx));
    LL_SPI_TransmitData8(g_flashsInit.SPIx, u8Data);
    while (!LL_SPI_IsActiveFlag_RXNE(g_flashsInit.SPIx));
    return LL_SPI_ReceiveData8(g_flashsInit.SPIx);
}

static void flashWriteEnable(void) {
    CS_RESET;
    // Automatically reset
    flashWriteData8(WRITE_ENABLE);
    // Execute the instruction
    CS_SET;
}

static void flashWaitForCompletion(void) {
    LL_mDelay(1);
    CS_RESET;
    flashWriteData8(READ_SR1);
    while (flashWriteData8(PLACEHOLDER) & SR1_BUSY);
    // Complete the instruction
    CS_SET;
    LL_mDelay(1);
}

static void flashEraseSector(uint32_t u32Address) {
    flashWriteEnable();
    flashWaitForCompletion();
    CS_RESET;
    // Up to 4096 bytes (1 sector)
    // Reset each byte to 0xFF with aligned address
    flashWriteData8(ERASE_SECTOR);
    flashWriteData8((uint8_t)(u32Address >> 16));
    flashWriteData8((uint8_t)(u32Address >> 8));
    flashWriteData8((uint8_t)u32Address);
    // Execute the instruction
    CS_SET;
    flashWaitForCompletion();
}

static void flashPageProgram(uint32_t u32Address, uint8_t *pu8Data, uint8_t u8UpperBound) {
    flashWriteEnable();
    flashWaitForCompletion();
    CS_RESET;
    // Up to 256 bytes (1 page)
    // If an entire page is to be programmed, the last address byte should be set to 0
    flashWriteData8(PAGE_PROGRAM);
    flashWriteData8((uint8_t)(u32Address >> 16));
    flashWriteData8((uint8_t)(u32Address >> 8));
    flashWriteData8((uint8_t)u32Address);
    // 1 - 256 bytes
    do flashWriteData8(*pu8Data++); while (u8UpperBound--);
    // Execute the instruction
    CS_SET;
    flashWaitForCompletion();
}

void flashInit(flashSettings *flashsInit) {
    g_flashsInit.SPIx = flashsInit->SPIx;
    g_flashsInit.CS_GPIO_Port = flashsInit->CS_GPIO_Port;
    g_flashsInit.CS_Pin = flashsInit->CS_Pin;
    LL_SPI_Enable(g_flashsInit.SPIx);
}

uint32_t flashGetJEDECID(void) {
    uint8_t pu8Data[4] = {};
    CS_RESET;
    flashWriteData8(JEDEC_ID);
    *(pu8Data + 2) = flashWriteData8(PLACEHOLDER);
    *(pu8Data + 1) = flashWriteData8(PLACEHOLDER);
    *pu8Data = flashWriteData8(PLACEHOLDER);
    CS_SET;
    return *(uint32_t *)pu8Data;
}

void flashReadMemory(uint32_t u32Address, uint8_t *pu8Data, uint32_t u32Length) {
    CS_RESET;
    // Up to the entire memory
    flashWriteData8(READ_MEMORY);
    flashWriteData8((uint8_t)(u32Address >> 16));
    flashWriteData8((uint8_t)(u32Address >> 8));
    flashWriteData8((uint8_t)u32Address);
    while (u32Length--) *pu8Data++ = flashWriteData8(PLACEHOLDER);
    // Complete the instruction
    CS_SET;
}

uint32_t flashEraseMemory(uint32_t u32Address, uint32_t u32Length) {
    uint32_t u32Sector = u32Length / SECTOR_SIZE, u32SectorCount = u32Sector;
    u32Address -= SECTOR_SIZE;
    // Require aligned address
    while (u32Sector--) flashEraseSector(u32Address += SECTOR_SIZE);
    if (u32Length -= u32SectorCount * SECTOR_SIZE) flashEraseSector(u32Address += SECTOR_SIZE), ++u32SectorCount;
    return u32SectorCount;
}

uint32_t flashWriteMemory(uint32_t u32Address, uint8_t *pu8Data, uint32_t u32Length) {
    uint32_t u32Page = u32Length / PAGE_SIZE, u32PageCount = u32Page;
    u32Address -= PAGE_SIZE, pu8Data -= PAGE_SIZE;
    // Require aligned address
    while (u32Page--) flashPageProgram(u32Address += PAGE_SIZE, pu8Data += PAGE_SIZE, PAGE_SIZE - 1);
    if (u32Length -= u32PageCount * PAGE_SIZE) flashPageProgram(u32Address += PAGE_SIZE, pu8Data += PAGE_SIZE, u32Length - 1), ++u32PageCount;
    return u32PageCount;
}
