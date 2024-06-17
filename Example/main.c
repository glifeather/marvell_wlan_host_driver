/* USER CODE BEGIN Header */
// Created by Glif.
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "dcmi.h"
#include "dma.h"
#include "spi.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "88w8801/wrapper/88w8801_wrapper.h"
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
#include "88w8801/flash/88w8801_program.h"
#endif
#include "ov2640/ov2640.h"
#include "st7735s/st7735s.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */
#ifdef WLAN_DEBUG
#include <stdio.h>
#define MAIN_DEBUG printf
#else
#define MAIN_DEBUG(...) \
    do { \
    } while (0)
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
/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static core_err_e g_ceStatus = CORE_ERR_UNHANDLED_STATUS;
static wlan_cb_t g_wlanCallback = {0};
static struct udp_pcb *g_upSession = NULL;
static uint8_t g_pu8FrameBuffer[CAM_BUF_SIZE];
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void DCMICaptureFrame(DCMI_HandleTypeDef *hdcmi);
static void wlanInitCallback(core_err_e ceStatus);
static void wlanSTAConnectCallback(core_err_e ceStatus);
static void wlanSTADisconnectCallback(void);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */
  g_wlanCallback.wlan_cb_init = wlanInitCallback;
  g_wlanCallback.wlan_cb_sta_connect = wlanSTAConnectCallback;
  g_wlanCallback.wlan_cb_sta_disconnect = wlanSTADisconnectCallback;
  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_SPI1_Init();
  MX_SPI2_Init();
  MX_DCMI_Init();
  /* USER CODE BEGIN 2 */
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
#endif
#if defined(WRITE_FIRMWARE_TO_FLASH) || defined(USE_FLASH_FIRMWARE)
  flashSettings flashsInit;
  flashsInit.SPIx = SPI1;
  flashsInit.CS_GPIO_Port = FLASH_CS_GPIO_Port;
  flashsInit.CS_Pin = FLASH_CS_Pin;
  flashInitFirmware(&flashsInit);
#endif
  MAIN_DEBUG("Initializing...\n");
  camSettings camsInit;
  camsInit.SCL_GPIO_Port = SCCB_SCL_GPIO_Port;
  camsInit.SCL_Pin = SCCB_SCL_Pin;
  camsInit.SDA_GPIO_Port = SCCB_SDA_GPIO_Port;
  camsInit.SDA_Pin = SCCB_SDA_Pin;
  camsInit.PDN_GPIO_Port = CAM_PDN_GPIO_Port;
  camsInit.PDN_Pin = CAM_PDN_Pin;
  camsInit.RST_GPIO_Port = CAM_RST_GPIO_Port;
  camsInit.RST_Pin = CAM_RST_Pin;
  uint8_t u8Err = camInit(&camsInit);
  if (u8Err) MAIN_DEBUG(MSG_ERROR_INVOKE, u8Err, "init camera");
  camSetImageMode(IMAGE_MODE_RGB565);
  camSetImageSize(CAM_IMAGE_W, CAM_IMAGE_H);
  if ((u8Err = wrapper_init(&g_ceStatus, &g_wlanCallback, WLAN_PDN_GPIO_Port, WLAN_PDN_Pin))) MAIN_DEBUG(MSG_ERROR_INVOKE, u8Err, "init WLAN");
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    if ((u8Err = wrapper_proc())) MAIN_DEBUG(MSG_ERROR_INVOKE, u8Err, "process packet");
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  LL_FLASH_SetLatency(LL_FLASH_LATENCY_5);
  while(LL_FLASH_GetLatency()!= LL_FLASH_LATENCY_5)
  {
  }
  LL_PWR_SetRegulVoltageScaling(LL_PWR_REGU_VOLTAGE_SCALE1);
  LL_RCC_HSE_Enable();

   /* Wait till HSE is ready */
  while(LL_RCC_HSE_IsReady() != 1)
  {

  }
  LL_RCC_PLL_ConfigDomain_SYS(LL_RCC_PLLSOURCE_HSE, LL_RCC_PLLM_DIV_8, 336, LL_RCC_PLLP_DIV_2);
  LL_RCC_PLL_Enable();

   /* Wait till PLL is ready */
  while(LL_RCC_PLL_IsReady() != 1)
  {

  }
  while (LL_PWR_IsActiveFlag_VOS() == 0)
  {
  }
  LL_RCC_SetAHBPrescaler(LL_RCC_SYSCLK_DIV_1);
  LL_RCC_SetAPB1Prescaler(LL_RCC_APB1_DIV_4);
  LL_RCC_SetAPB2Prescaler(LL_RCC_APB2_DIV_2);
  LL_RCC_SetSysClkSource(LL_RCC_SYS_CLKSOURCE_PLL);

   /* Wait till System clock is ready */
  while(LL_RCC_GetSysClkSource() != LL_RCC_SYS_CLKSOURCE_STATUS_PLL)
  {

  }
  LL_SetSystemCoreClock(168000000);

   /* Update the time base */
  if (HAL_InitTick (TICK_INT_PRIORITY) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
void HAL_DCMI_FrameEventCallback(DCMI_HandleTypeDef *hdcmi) {
  HAL_DCMI_Stop(hdcmi);
#ifdef CAM_DEBUG
  lcdDrawImage(g_pu8FrameBuffer, 0, 0, CAM_IMAGE_W, CAM_IMAGE_H, false);
#else
  uint8_t u8Err = wrapper_udp_send(&g_upSession, g_pu8FrameBuffer, CAM_BUF_SIZE);
  if (u8Err) MAIN_DEBUG(MSG_ERROR_INVOKE, u8Err, "send data");
#endif
  DCMICaptureFrame(hdcmi);
}

static void DCMICaptureFrame(DCMI_HandleTypeDef *hdcmi) {
  __HAL_DCMI_ENABLE_IT(hdcmi, DCMI_IT_FRAME);
  HAL_DCMI_Start_DMA(hdcmi, DCMI_MODE_SNAPSHOT, (uint32_t)g_pu8FrameBuffer, CAM_BUF_SIZE / 4);
}

static void wlanInitCallback(core_err_e ceStatus) {
  if ((g_ceStatus = ceStatus)) {
    MAIN_DEBUG(MSG_ERROR_CALLBACK, ceStatus, "init WLAN");
    if ((ceStatus = wrapper_init(&g_ceStatus, &g_wlanCallback, WLAN_PDN_GPIO_Port, WLAN_PDN_Pin))) MAIN_DEBUG(MSG_ERROR_INVOKE, ceStatus, "init WLAN");
    return;
  }
#ifdef CAM_DEBUG
  DCMICaptureFrame(&hdcmi);
#else
  MAIN_DEBUG("Connecting to '%s'...\n", WLAN_AP_SSID);
  if ((ceStatus = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_INVOKE, ceStatus, "connect");
#endif
}

static void wlanSTAConnectCallback(core_err_e ceStatus) {
  if (ceStatus) {
    MAIN_DEBUG(MSG_ERROR_CALLBACK, ceStatus, "connect");
    if ((ceStatus = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1))) MAIN_DEBUG(MSG_ERROR_INVOKE, ceStatus, "connect");
    return;
  }
  MAIN_DEBUG("Creating udp_pcb...\n");
  if ((ceStatus = wrapper_udp_new(&g_upSession, BSS_TYPE_STA))) {
    MAIN_DEBUG(MSG_ERROR_INVOKE, ceStatus, "create udp_pcb");
    return;
  }
  MAIN_DEBUG("Connecting udp_pcb with '%s:%d'...\n", LWIP_UDP_IP, LWIP_UDP_PORT);
  ip_addr_t ipAddress;
  ipaddr_aton(LWIP_UDP_IP, &ipAddress);
  if ((ceStatus = wrapper_udp_connect(&g_upSession, &ipAddress, LWIP_UDP_PORT))) {
    MAIN_DEBUG(MSG_ERROR_INVOKE, ceStatus, "connect udp_pcb");
    wrapper_udp_remove(&g_upSession);
    return;
  }
  MAIN_DEBUG("Connected. Ready to send data.\n");
#ifndef CAM_DEBUG
  DCMICaptureFrame(&hdcmi);
#endif
}

static void wlanSTADisconnectCallback(void) {
  MAIN_DEBUG("Reconnecting to '%s'...\n", WLAN_AP_SSID);
  uint8_t u8Err = wlan_sta_connect((uint8_t *)WLAN_AP_SSID, sizeof(WLAN_AP_SSID) - 1, (uint8_t *)WLAN_AP_PWD, sizeof(WLAN_AP_PWD) - 1);
  if (u8Err) MAIN_DEBUG(MSG_ERROR_INVOKE, u8Err, "reconnect");
}
/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
