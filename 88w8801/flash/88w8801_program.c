#include "88w8801/88w8801.h"
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
#include "88w8801_flash.h"

#ifdef WLAN_FLASH_DEBUG
#include <stdio.h>
#define FLASH_DEBUG printf
#else
#define FLASH_DEBUG(...)
#endif

#ifdef WRITE_FIRMWARE_TO_FLASH
#define FIRMWARE_SIZE 0x3E630
#define LAST_DWORD    0x188CDB1F
#endif

void flashInitFirmware(flashSettings *flashsInit) {
    flashInit(flashsInit);
    uint32_t u32Data = flashGetJEDECID();
    if (u32Data != (MANUFACTURER_ID_WINBOND << 16 | DEVICE_ID_W25Q16DV)) return;
    FLASH_DEBUG("Manufacturer ID: 0x%X\nDevice ID: 0x%X\n", u32Data >> 16, (uint16_t)u32Data);
    #ifdef WRITE_FIRMWARE_TO_FLASH
    u32Data = flashEraseMemory(FLASH_FIRMWARE_ADDRESS, FIRMWARE_SIZE);
    FLASH_DEBUG("Erase %d sectors from 0x%06X\n", u32Data, FLASH_FIRMWARE_ADDRESS);
    // Defined in 88w8801_firmware.c
    extern const uint8_t fw_mrvl88w8801[FIRMWARE_SIZE];
    u32Data = flashWriteMemory(FLASH_FIRMWARE_ADDRESS, (uint8_t *)fw_mrvl88w8801, FIRMWARE_SIZE);
    FLASH_DEBUG("Write %d pages to flash\n", u32Data);
    flashReadMemory(FLASH_FIRMWARE_ADDRESS + FIRMWARE_SIZE - 4, (uint8_t *)&u32Data, 4);
    if (u32Data != LAST_DWORD) FLASH_DEBUG("Error: Expected 0x%X instead of 0x%08X\n", LAST_DWORD, u32Data);
    #endif
}
#endif
