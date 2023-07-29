#ifndef _LWIP_NETIF_ETHERNETIF_
#define _LWIP_NETIF_ETHERNETIF_
#include "lwip/dhcpd.h"
void ethernetif_netif_init(u8_t *mac_addr);
void ethernetif_data_input(u8_t *rx_buf, u8_t bss_type);
void ethernetif_link_down(u8_t bss_type);
#if LWIP_DHCPD
void ethernetif_link_up(u8_t bss_type, dhcpd_inform_fn access);
void ethernetif_dhcpd_erase(u8_t *mac_addr, dhcpd_inform_fn info);
#else
void ethernetif_link_up(u8_t bss_type, u8_t *access);
void ethernetif_dhcpd_erase(u8_t *mac_addr, u8_t *info);
#endif
#endif
