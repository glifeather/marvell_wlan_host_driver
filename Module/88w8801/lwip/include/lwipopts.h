#ifndef _LWIP_LWIPOPTS_
#define _LWIP_LWIPOPTS_
#include "88w8801/88w8801.h"

// 分配内存时禁用临界区域保护
#define SYS_LIGHTWEIGHT_PROT 0

// 无操作系统
#define NO_SYS 1

// 对于 32 位 CPU，应采用 4 字节对齐
#define MEM_ALIGNMENT 4

// 内存堆数组大小
#define MEM_SIZE 0x4000

// 启用 IPv4
#define LWIP_IPV4 1

// 启用 DHCP 客户端
#define LWIP_DHCP 1

// 启用 DNS
#define LWIP_DNS 1

// 启用 TCP 选择性确认
#define LWIP_TCP_SACK_OUT 1

// TCP 分段大小
#define TCP_MSS 1460

// 启用主机名称设置
#define LWIP_NETIF_HOSTNAME 1

// 禁用 Netconn
#define LWIP_NETCONN 0

// 禁用 Socket
#define LWIP_SOCKET 0

// 禁用统计收集
#define LWIP_STATS 0

// 启用 DHCP 服务器
#define LWIP_DHCPD 1
#if !defined LWIP_DHCPD || defined __DOXYGEN__
#define LWIP_DHCPD 0
#else
#define LWIP_IP_ACCEPT_UDP_PORT(port) (port) == PP_NTOHS(LWIP_IANA_PORT_DHCP_SERVER)
#endif
#if !LWIP_IPV4
#undef LWIP_DHCPD
#define LWIP_DHCPD 0
#endif

// 配置 NAT
#if !defined LWIP_NAT || defined __DOXYGEN__
#define LWIP_NAT 0
#endif
#if !LWIP_IPV4
#undef LWIP_NAT
#define LWIP_NAT 0
#endif

// 添加相关超时
#define MEMP_NUM_SYS_TIMEOUT (LWIP_NUM_SYS_TIMEOUT_INTERNAL + LWIP_NAT)
#endif
