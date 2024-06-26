// Created by Glif.
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

#include "88w8801/wrapper/88w8801_wrapper.h"
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
#include "88w8801/flash/88w8801_program.h"
#endif
#include "ov2640/ov2640.h"
#include "st7735s/st7735s.h"

// Used for camera debug.
// #define CAM_DEBUG

#ifdef WLAN_DEBUG
#include <stdio.h>
#define MAIN_DEBUG printf
#else
#undef CAM_DEBUG
#define MAIN_DEBUG(...) \
    do { \
    } while (0)
#endif

#define MSG_ERROR_FORMAT "%s %d: Failed to %s\n"

#define WLAN_AP_SSID "SSID"
#define WLAN_AP_PWD  "Password"

#define LWIP_UDP_IP   "192.168.1.200"
#define LWIP_UDP_PORT 8080

#define CAM_IMAGE_W  LCD_X
#define CAM_IMAGE_H  LCD_Y
#define CAM_BUF_SIZE (CAM_IMAGE_W * CAM_IMAGE_H * 2)

static void SystemClock_Config(void);
static void wlanInitCallback(core_err_e ceStatus);
static void wlanSTAConnectCallback(core_err_e ceStatus);
static void wlanSTADisconnectCallback(void);
static void moduleInit(void);
static void dcmiCaptureFrame(DCMI_HandleTypeDef *hdcmi);

static core_err_e g_ceStatus = CORE_ERR_UNHANDLED_STATUS;
static wlan_cb_t g_wlanCallback = {wlanInitCallback, NULL, wlanSTAConnectCallback, wlanSTADisconnectCallback, NULL, NULL, NULL, NULL};
static struct udp_pcb *g_pupSession = NULL;
static uint8_t g_pu8FrameBuffer[CAM_BUF_SIZE];

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();
    MX_DCMI_Init();
    moduleInit();
    uint8_t u8Err;
    while (1) {
        if ((u8Err = wrapper_proc())) MAIN_DEBUG(MSG_ERROR_FORMAT, "CORE", u8Err, "process packet");
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
    HAL_DCMI_Stop(hdcmi);
#ifdef CAM_DEBUG
    lcdDrawImage(g_pu8FrameBuffer, 0, 0, CAM_IMAGE_W, CAM_IMAGE_H, false);
#else
    const err_t errSession = wrapper_udp_send(&g_pupSession, g_pu8FrameBuffer, CAM_BUF_SIZE);
    if (errSession) MAIN_DEBUG(MSG_ERROR_FORMAT, "LWIP", errSession, "send data");
#endif
    dcmiCaptureFrame(hdcmi);
}

static void SystemClock_Config(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5);
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    LL_RCC_HSE_Enable();
    while (!LL_RCC_HSE_IsReady());
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336, LL_RCC_PLLP_DIV_2);
    // TODO: Selecting LL APIs for RCC will lead to a missing PLLQ.
    RCC->PLLCFGR |= LL_RCC_PLLQ_DIV_7;
    LL_RCC_PLL_Enable();
    while (!LL_RCC_PLL_IsReady());
    // The wrong generation of LL_PWR_IsActiveFlag_VOS() is fixed in STM32CubeMX 6.8.1.
    while (!LL_PWR_IsActiveFlag_VOS());
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);
    LL_SetSystemCoreClock(168000000);
    // LL_Init1msTick() or LL_InitTick() will not enable the SysTick interrupt, unless LL_SYSTICK_EnableIT() is called manually.
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) Error_Handler();
}

static void wlanInitCallback(core_err_e ceStatus) {
    // Used for wlan_shutdown().
    if ((g_ceStatus = ceStatus)) return;
#ifdef CAM_DEBUG
    dcmiCaptureFrame(&hdcmi);
#else
    MAIN_DEBUG("Connecting to '%s'...\n", WLAN_AP_SSID);
    if ((ceStatus = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_FORMAT, "CORE", ceStatus, "connect");
#endif
}

static void wlanSTAConnectCallback(core_err_e ceStatus) {
    if (ceStatus) {
        MAIN_DEBUG(MSG_ERROR_FORMAT, "CORE", ceStatus, "connect, retrying...");
        if ((ceStatus = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_FORMAT, "CORE", ceStatus, "connect");
        return;
    }
    MAIN_DEBUG("Creating udp_pcb...\n");
    err_t errSession = wrapper_udp_new(&g_pupSession, BSS_TYPE_STA);
    if (errSession) {
        MAIN_DEBUG(MSG_ERROR_FORMAT, "LWIP", errSession, "create udp_pcb");
        return;
    }
    MAIN_DEBUG("Connecting udp_pcb with '%s:%d'...\n", LWIP_UDP_IP, LWIP_UDP_PORT);
    ip_addr_t ipAddress;
    ipaddr_aton(LWIP_UDP_IP, &ipAddress);
    if ((errSession = wrapper_udp_connect(&g_pupSession, &ipAddress, LWIP_UDP_PORT))) {
        MAIN_DEBUG(MSG_ERROR_FORMAT, "LWIP", errSession, "connect udp_pcb");
        wrapper_udp_remove(&g_pupSession);
        return;
    }
    MAIN_DEBUG("Connected. Ready to send data.\n");
#ifndef CAM_DEBUG
    dcmiCaptureFrame(&hdcmi);
#endif
}

static void wlanSTADisconnectCallback(void) {
    MAIN_DEBUG("Reconnecting to '%s'...\n", WLAN_AP_SSID);
    const uint8_t u8Err = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1);
    if (u8Err) MAIN_DEBUG(MSG_ERROR_FORMAT, "CORE", u8Err, "reconnect");
}

static void moduleInit(void) {
#ifdef WLAN_DEBUG
    setvbuf(stdout, NULL, _IONBF, 0);
    lcdSettings lcdsInit;
    lcdsInit.SPIx = SPI2;
    lcdsInit.CS_GPIO_Port = LCD_CS_GPIO_Port;
    lcdsInit.CS_Pin = LCD_CS_Pin;
    lcdsInit.DC_GPIO_Port = LCD_DC_GPIO_Port;
    lcdsInit.DC_Pin = LCD_DC_Pin;
    lcdsInit.RST_GPIO_Port = LCD_RST_GPIO_Port;
    lcdsInit.RST_Pin = LCD_RST_Pin;
    lcdInit(&lcdsInit);
    MAIN_DEBUG("Initializing...\n");
#endif
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
    flashSettings flashsInit;
    flashsInit.SPIx = SPI1;
    flashsInit.CS_GPIO_Port = FLASH_CS_GPIO_Port;
    flashsInit.CS_Pin = FLASH_CS_Pin;
    flashInitFirmware(&flashsInit);
#endif
    camSettings camsInit;
    camsInit.SCL_GPIO_Port = SCCB_SCL_GPIO_Port;
    camsInit.SCL_Pin = SCCB_SCL_Pin;
    camsInit.SDA_GPIO_Port = SCCB_SDA_GPIO_Port;
    camsInit.SDA_Pin = SCCB_SDA_Pin;
    camsInit.PDN_GPIO_Port = CAM_PDN_GPIO_Port;
    camsInit.PDN_Pin = CAM_PDN_Pin;
    camsInit.RST_GPIO_Port = CAM_RST_GPIO_Port;
    camsInit.RST_Pin = CAM_RST_Pin;
    uint16_t u16Err = camInit(&camsInit);
    if (u16Err) {
        MAIN_DEBUG(MSG_ERROR_FORMAT, "CAM", u16Err, "init camera");
        return;
    }
    camSetImageSize(CAM_IMAGE_W, CAM_IMAGE_H);
    if ((u16Err = wrapper_init(&g_ceStatus, &g_wlanCallback, WLAN_PDN_GPIO_Port, WLAN_PDN_Pin))) MAIN_DEBUG(MSG_ERROR_FORMAT, (uint8_t)u16Err ? "CORE" : "SDIO", (uint8_t)u16Err | u16Err >> 8, "init WLAN");
}

static void dcmiCaptureFrame(DCMI_HandleTypeDef *hdcmi) {
    __HAL_DCMI_ENABLE_IT(hdcmi, DCMI_IT_FRAME);
    HAL_DCMI_Start_DMA(hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)g_pu8FrameBuffer, CAM_BUF_SIZE / 4);
}
