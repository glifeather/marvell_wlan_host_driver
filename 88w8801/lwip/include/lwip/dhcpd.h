#ifndef _LWIP_IPV4_DHCPD_
#define _LWIP_IPV4_DHCPD_
#include "lwip/opt.h"
#if LWIP_IPV4 && LWIP_DHCPD
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

#define DHCP_MAX_OPTION_LEN 32

struct dhcpd_address_list {
  // 0: Before DHCP_DISCOVER or after DHCP_REQUEST
  // 1: Between DHCP_DISCOVER and DHCP_REQUEST
  // 2: After DHCP_DECLINE or DHCP_RELEASE
  u8_t offer;
  u8_t hostname[DHCP_MAX_OPTION_LEN];
  u8_t mac_addr[NETIF_MAX_HWADDR_LEN];
  ip4_addr_t ip_addr;
  struct dhcpd_address_list *next;
};

typedef void (*dhcpd_inform_fn)(u8_t *name, u8_t *mac, u8_t *ip);

err_t dhcpd_start(struct netif *netif, dhcpd_inform_fn access);
void dhcpd_stop(void);
void dhcpd_erase(u8_t *mac, dhcpd_inform_fn info, struct dhcpd_address_list *addr);

#ifdef __cplusplus
}
#endif

#endif
#endif
