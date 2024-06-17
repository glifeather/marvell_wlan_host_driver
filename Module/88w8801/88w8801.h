#ifndef _88W8801_
#define _88W8801_

// 抑制编译器警告
#ifndef UNUSED
#define UNUSED(x) (void)x
#endif
typedef unsigned char uint8_t;

// 主机名称
#define LWIP_HOSTNAME "STM32-88W8801"

// NAT 模式（STA + AP）
// #define LWIP_NAT 1

// AP 配置
#define LWIP_AP_IP      192, 168, 10, 1
#define LWIP_AP_NETMASK 255, 255, 255, 0
#define LWIP_AP_GATEWAY LWIP_AP_IP

// DHCP 服务器配置
// 提供 IP 偏移
#define LWIP_OFFERED_IP_OFFSET 99
// 广播地址偏移
#define LWIP_BROADCAST_OFFSET 254

// 最大客户端数
#define MAX_CLIENT_NUM 2

// 写入固件到 Flash
// #define WRITE_FIRMWARE_TO_FLASH

// 使用 Flash 中的固件（启用 WRITE_FIRMWARE_TO_FLASH 时无效）
// #define USE_FLASH_FIRMWARE
#ifdef WRITE_FIRMWARE_TO_FLASH
#undef USE_FLASH_FIRMWARE
#endif

// Flash 固件地址
#define FLASH_FIRMWARE_ADDRESS 0x000000

// 调试标志
// #define WLAN_DEBUG

#ifdef WLAN_DEBUG
// 清屏标志
#define CLEAR_FLAG '\r'
#define WLAN_CORE_DEBUG
#define WLAN_FLASH_DEBUG
#define WLAN_LWIP_DEBUG
#define WLAN_SDIO_DEBUG
#endif

#ifdef WLAN_LWIP_DEBUG
// lwIP 总调试标志
#define LWIP_DEBUG
// lwIP 分调试标志（原始）
#define ETHARP_DEBUG     LWIP_DBG_ON
#define NETIF_DEBUG      LWIP_DBG_ON
#define PBUF_DEBUG       LWIP_DBG_ON
#define ICMP_DEBUG       LWIP_DBG_ON
#define IGMP_DEBUG       LWIP_DBG_ON
#define INET_DEBUG       LWIP_DBG_ON
#define IP_DEBUG         LWIP_DBG_ON
#define IP_REASS_DEBUG   LWIP_DBG_ON
#define RAW_DEBUG        LWIP_DBG_ON
#define MEM_DEBUG        LWIP_DBG_ON
#define MEMP_DEBUG       LWIP_DBG_ON
#define SYS_DEBUG        LWIP_DBG_ON
#define TIMERS_DEBUG     LWIP_DBG_ON
#define TCP_DEBUG        LWIP_DBG_ON
#define TCP_INPUT_DEBUG  LWIP_DBG_ON
#define TCP_FR_DEBUG     LWIP_DBG_ON
#define TCP_RTO_DEBUG    LWIP_DBG_ON
#define TCP_CWND_DEBUG   LWIP_DBG_ON
#define TCP_WND_DEBUG    LWIP_DBG_ON
#define TCP_OUTPUT_DEBUG LWIP_DBG_ON
#define TCP_RST_DEBUG    LWIP_DBG_ON
#define TCP_QLEN_DEBUG   LWIP_DBG_ON
#define UDP_DEBUG        LWIP_DBG_ON
#define DHCP_DEBUG       LWIP_DBG_ON
#define AUTOIP_DEBUG     LWIP_DBG_ON
#define DNS_DEBUG        LWIP_DBG_ON
// lwIP 分调试标志（新增）
#define DHCPD_DEBUG LWIP_DBG_ON
#define NAT_DEBUG   LWIP_DBG_ON
#endif
#endif
