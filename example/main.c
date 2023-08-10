#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"
#include "88w8801/wrapper/88w8801_wrapper.h"
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
#include "88w8801/flash/88w8801_program.h"
#endif
// TODO: Add needed module
// #include "ov2640/ov2640.h"
#include "st7735/st7735.h"

#ifdef WLAN_DEBUG
#include <stdio.h>
#define MAIN_DEBUG printf
#else
#define MAIN_DEBUG(...)
#endif

#define MSG_ERROR_INVOKE   "Error %d: Failed to %s\n"
#define MSG_ERROR_CALLBACK "Error %d: Failed to %s, retrying...\n"

#define WLAN_AP_SSID "SSID"
#define WLAN_AP_PWD  "Password"

#define LWIP_UDP_IP   "192.168.1.200"
#define LWIP_UDP_PORT 8080

// For camera debug
// #define CAM_DEBUG
#ifndef WLAN_DEBUG
#undef CAM_DEBUG
#endif
#define CAM_IMAGE_W  LCD_X
#define CAM_IMAGE_H  LCD_Y
#define CAM_BUF_SIZE (CAM_IMAGE_W * CAM_IMAGE_H * 2)

static void wlan_cb_init(core_err_e status);
static void wlan_cb_sta_connect(core_err_e status);
static void wlan_cb_sta_disconnect(void);
static void SystemClock_Config(void);

// Used for external operation
_Bool data_full = 0;

static core_err_e sys_status = CORE_ERR_UNHANDLED_STATUS;
static wlan_cb_t wlan_cb = {wlan_cb_init, NULL, wlan_cb_sta_connect, wlan_cb_sta_disconnect, NULL, NULL, NULL, NULL};
static struct udp_pcb *pcb = NULL;
static uint8_t data_buf[CAM_BUF_SIZE];

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_DMA_Init();
    MX_SPI1_Init();
    MX_SPI2_Init();
    MX_DCMI_Init();
    #ifdef WLAN_DEBUG
    lcdSettings lcdInfo;
    lcdInfo.SPIx = SPI2;
    lcdInfo.CS_GPIO_Port = LCD_CS_GPIO_Port;
    lcdInfo.CS_Pin = LCD_CS_Pin;
    lcdInfo.DC_GPIO_Port = LCD_DC_GPIO_Port;
    lcdInfo.DC_Pin = LCD_DC_Pin;
    lcdInfo.RST_GPIO_Port = LCD_RST_GPIO_Port;
    lcdInfo.RST_Pin = LCD_RST_Pin;
    lcdInit(&lcdInfo);
    #endif
    #if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
    flashSettings flashInfo;
    flashInfo.SPIx = SPI1;
    flashInfo.CS_GPIO_Port = FLASH_CS_GPIO_Port;
    flashInfo.CS_Pin = FLASH_CS_Pin;
    flashInitFirmware(&flashInfo);
    #endif
    MAIN_DEBUG("Initializing...\n");
    // TODO: Add needed module
    // camSettings camsInit;
    // camsInit.SCL_GPIO_Port = SCCB_SCL_GPIO_Port;
    // camsInit.SCL_Pin = SCCB_SCL_Pin;
    // camsInit.SDA_GPIO_Port = SCCB_SDA_GPIO_Port;
    // camsInit.SDA_Pin = SCCB_SDA_Pin;
    // camsInit.PDN_GPIO_Port = CAM_PDN_GPIO_Port;
    // camsInit.PDN_Pin = CAM_PDN_Pin;
    // camsInit.RST_GPIO_Port = CAM_RST_GPIO_Port;
    // camsInit.RST_Pin = CAM_RST_Pin;
    // uint8_t err = camInit(&camsInit);
    // if (err) MAIN_DEBUG(MSG_ERROR_INVOKE, err, "init camera");
    // camSetImageMode(IMAGE_MODE_RGB565);
    // camSetImageSize(CAM_IMAGE_W, CAM_IMAGE_H);
    // TODO: Remove this
    uint8_t err;
    if ((err = wrapper_init(&sys_status, &wlan_cb, WLAN_PDN_GPIO_Port, WLAN_PDN_Pin))) MAIN_DEBUG(MSG_ERROR_INVOKE, err, "init WLAN");
    while (1) {
        if ((err = wrapper_proc())) MAIN_DEBUG(MSG_ERROR_INVOKE, err, "process packet");
        #ifdef CAM_DEBUG
        if (data_full) {
            lcdDrawImage(data_buf);
        #else
        if (pcb && data_full) {
            if ((err = wrapper_udp_send(&pcb, data_buf, CAM_BUF_SIZE))) MAIN_DEBUG(MSG_ERROR_INVOKE, err, "send data");
        #endif
            data_full = 0;
            HAL_DCMI_Resume(&hdcmi);
        }
    }
}

void Error_Handler(void) {
    __disable_irq();
    while (1) {}
}

static void wlan_cb_init(core_err_e status) {
    if ((sys_status = status)) {
        MAIN_DEBUG(MSG_ERROR_CALLBACK, status, "init WLAN");
        if ((status = wrapper_init(&sys_status, &wlan_cb, WLAN_PDN_GPIO_Port, WLAN_PDN_Pin))) MAIN_DEBUG(MSG_ERROR_INVOKE, status, "init WLAN");
        return;
    }
    #ifdef CAM_DEBUG
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)data_buf, CAM_BUF_SIZE / 4);
    #else
    MAIN_DEBUG("Connecting to '%s'...\n", WLAN_AP_SSID);
    if ((status = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_INVOKE, status, "connect");
    #endif
}

static void wlan_cb_sta_connect(core_err_e status) {
    if (status) {
        MAIN_DEBUG(MSG_ERROR_CALLBACK, status, "connect");
        if ((status = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_INVOKE, status, "connect");
        return;
    }
    MAIN_DEBUG("Creating udp_pcb...\n");
    if ((status = wrapper_udp_new(&pcb, BSS_TYPE_STA))) {
        MAIN_DEBUG(MSG_ERROR_INVOKE, status, "create udp_pcb");
        return;
    }
    MAIN_DEBUG("Connecting udp_pcb with '%s:%d'...\n", LWIP_UDP_IP, LWIP_UDP_PORT);
    ip_addr_t ipaddr;
    ipaddr_aton(LWIP_UDP_IP, &ipaddr);
    if ((status = wrapper_udp_connect(&pcb, &ipaddr, LWIP_UDP_PORT))) {
        MAIN_DEBUG(MSG_ERROR_INVOKE, status, "connect udp_pcb");
        wrapper_udp_remove(&pcb);
        return;
    }
    MAIN_DEBUG("Connected. Ready to send data.\n");
    #ifndef CAM_DEBUG
    HAL_DCMI_Start_DMA(&hdcmi, DCMI_MODE_CONTINUOUS, (uint32_t)data_buf, CAM_BUF_SIZE / 4);
    #endif
}

static void wlan_cb_sta_disconnect(void) {
    MAIN_DEBUG("Reconnecting to '%s'...\n", WLAN_AP_SSID);
    uint8_t err = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1);
    if (err) MAIN_DEBUG(MSG_ERROR_INVOKE, err, "reconnect");
}

static void SystemClock_Config(void) {
    LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
    while (LL_FLASH_GetLatency() != LL_FLASH_LATENCY_5);
    LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
    while (!LL_PWR_IsActiveFlag_VOS());
    LL_RCC_HSE_Enable();
    /* Wait till HSE is ready */
    while (!LL_RCC_HSE_IsReady());
    LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336, LL_RCC_PLLP_DIV_2);
    LL_RCC_PLL_Enable();
    /* Wait till PLL is ready */
    while (!LL_RCC_PLL_IsReady());
    LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
    LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
    LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
    LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);
    /* Wait till System clock is ready */
    while (LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL);
    LL_SetSystemCoreClock(168000000);
    /* Update the time base */
    if (HAL_InitTick(TICK_INT_PRIORITY) != HAL_OK) Error_Handler();
}
