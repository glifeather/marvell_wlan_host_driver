/**
 * @file
 * Ethernet Interface Skeleton
 *
 */

/*
 * Copyright (c) 2001-2004 Swedish Institute of Computer Science.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT
 * SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT
 * OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING
 * IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGE.
 *
 * This file is part of the lwIP TCP/IP stack.
 *
 * Author: Adam Dunkels <adam@sics.se>
 *
 */

/*
 * This file is a skeleton for developing Ethernet network interface
 * drivers for lwIP. Add code to the low_level functions and do a
 * search-and-replace for the word "ethernetif" to replace it with
 * something that better describes your network interface.
 */

#include "lwip/opt.h"
#include "lwip/def.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/stats.h"
#include "lwip/snmp.h"
#include "lwip/ethip6.h"
#include "lwip/etharp.h"
#include "netif/ppp/pppoe.h"
// Modified
#include "88w8801/core/88w8801_core.h"

/* Define those to better describe your network interface. */
// Modified
#define IFNAME_WLAN 'w'
#define IFNAME_BASE '0'
#define IFNAME_STA  (IFNAME_BASE + BSS_TYPE_STA)
#define IFNAME_UAP  (IFNAME_BASE + BSS_TYPE_UAP)

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 * But this is only an example, anyway...
 */
// Modified
// struct ethernetif {
//   struct eth_addr *ethaddr;
//   /* Add whatever per-interface state that is needed here. */
// };

/**
 * In this function, the hardware should be initialized.
 * Called from ethernetif_init().
 *
 * @param netif the already initialized lwip network interface structure
 *        for this ethernetif
 */
static void
low_level_init(struct netif *netif) {
  // Modified
  // struct ethernetif *ethernetif = netif->state;

  /* set MAC hardware address length */
  netif->hwaddr_len = ETHARP_HWADDR_LEN;

  // Modified
  LWIP_DEBUGF(NETIF_DEBUG, ("netif MAC %02X:%02X:%02X:%02X:%02X:%02X\n", netif->hwaddr[0], netif->hwaddr[1], netif->hwaddr[2], netif->hwaddr[3], netif->hwaddr[4], netif->hwaddr[5]));

  /* maximum transfer unit */
  netif->mtu = 1500;

  /* device capabilities */
  /* don't set NETIF_FLAG_ETHARP if this device is not an ethernet one */
  // Modified
  netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP/* | NETIF_FLAG_LINK_UP*/;

#if LWIP_IPV6 && LWIP_IPV6_MLD
  /*
   * For hardware/netifs that implement MAC filtering.
   * All-nodes link-local is handled by default, so we must let the hardware know
   * to allow multicast packets in.
   * Should set mld_mac_filter previously. */
  if (netif->mld_mac_filter != NULL) {
    ip6_addr_t ip6_allnodes_ll;
    ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
    netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
  }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */

  /* Do whatever else is needed to initialize interface. */
}

/**
 * This function should do the actual transmission of the packet. The packet is
 * contained in the pbuf that is passed to the function. This pbuf
 * might be chained.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @param p the MAC packet to send (e.g. IP packet including MAC addresses and type)
 * @return ERR_OK if the packet could be sent
 *         an err_t value if the packet couldn't be sent
 *
 * @note Returning ERR_MEM here if a DMA queue of your MAC is full can lead to
 *       strange results. You might consider waiting for space in the DMA queue
 *       to become available since the stack doesn't retry to send a packet
 *       dropped because of memory failure (except for the TCP timers).
 */
static err_t
low_level_output(struct netif *netif, struct pbuf *p) {
  // Modified
  // struct ethernetif *ethernetif = netif->state;
  // struct pbuf *q;

#if ETH_PAD_SIZE
  pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

  // Modified
  /* Defined in 88w8801_core.c */
  extern u8_t wlan_tx_buf[TX_BUF_SIZE];
  pbuf_copy_partial(p, ((TxPD *)wlan_tx_buf)->payload, p->tot_len, 0);
  wlan_send_data(NULL, p->tot_len, netif->num);

  MIB2_STATS_NETIF_ADD(netif, ifoutoctets, p->tot_len);
  if (((u8_t *)p->payload)[0] & 1) {
    /* broadcast or multicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutnucastpkts);
  } else {
    /* unicast packet */
    MIB2_STATS_NETIF_INC(netif, ifoutucastpkts);
  }
  /* increase ifoutdiscards or ifouterrors on error */

#if ETH_PAD_SIZE
  pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

  LINK_STATS_INC(link.xmit);

  return ERR_OK;
}

/**
 * Should allocate a pbuf and transfer the bytes of the incoming
 * packet from the interface into the pbuf.
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return a pbuf filled with the received packet (including MAC header)
 *         NULL on memory error
 */
static struct pbuf *
low_level_input(struct netif *netif) {
  // Modified
  // struct ethernetif *ethernetif = netif->state;
  // struct pbuf *p, *q;
  struct pbuf *p;
  u16_t len;

  /* Obtain the size of the packet and put it into the "len"
     variable. */
  // Modified
  len = ((RxPD *)netif->state)->rx_pkt_length;

#if ETH_PAD_SIZE
  len += ETH_PAD_SIZE; /* allow room for Ethernet padding */
#endif

  /* We allocate a pbuf chain of pbufs from the pool. */
  p = pbuf_alloc(PBUF_RAW, len, PBUF_POOL);

  if (p != NULL) {

#if ETH_PAD_SIZE
    pbuf_remove_header(p, ETH_PAD_SIZE); /* drop the padding word */
#endif

    // Modified
    pbuf_take(p, netif->state + ((RxPD *)netif->state)->rx_pkt_offset + SDIO_HDR_SIZE, len);

    MIB2_STATS_NETIF_ADD(netif, ifinoctets, p->tot_len);
    if (((u8_t *)p->payload)[0] & 1) {
      /* broadcast or multicast packet */
      MIB2_STATS_NETIF_INC(netif, ifinnucastpkts);
    } else {
      /* unicast packet */
      MIB2_STATS_NETIF_INC(netif, ifinucastpkts);
    }
#if ETH_PAD_SIZE
    pbuf_add_header(p, ETH_PAD_SIZE); /* reclaim the padding word */
#endif

    LINK_STATS_INC(link.recv);
  } else {
    LINK_STATS_INC(link.memerr);
    LINK_STATS_INC(link.drop);
    MIB2_STATS_NETIF_INC(netif, ifindiscards);
  }

  return p;
}

/**
 * This function should be called when a packet is ready to be read
 * from the interface. It uses the function low_level_input() that
 * should handle the actual reception of bytes from the network
 * interface. Then the type of the received packet is determined and
 * the appropriate input function is called.
 *
 * @param netif the lwip network interface structure for this ethernetif
 */
static void
ethernetif_input(struct netif *netif) {
  // Modified
  // struct ethernetif *ethernetif = netif->state;
  // struct eth_hdr *ethhdr;
  struct pbuf *p;

  /* move received packet into a new pbuf */
  p = low_level_input(netif);
  /* if no packet could be read, silently ignore this */
  if (p != NULL) {
    /* pass all packets to ethernet_input, which decides what packets it supports */
    if (netif->input(p, netif) != ERR_OK) {
      LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_input: IP input error\n"));
      pbuf_free(p);
      p = NULL;
    }
  }
}

/**
 * Should be called at the beginning of the program to set up the
 * network interface. It calls the function low_level_init() to do the
 * actual setup of the hardware.
 *
 * This function should be passed as a parameter to netif_add().
 *
 * @param netif the lwip network interface structure for this ethernetif
 * @return ERR_OK if the loopif is initialized
 *         ERR_MEM if private data couldn't be allocated
 *         any other err_t on error
 */
err_t
ethernetif_init(struct netif *netif) {
  LWIP_ASSERT("netif != NULL", (netif != NULL));

  // Modified
  // struct ethernetif *ethernetif = mem_malloc(sizeof(struct ethernetif));
  // if (ethernetif == NULL) {
  //   LWIP_DEBUGF(NETIF_DEBUG, ("ethernetif_init: out of memory\n"));
  //   return ERR_MEM;
  // }

#if LWIP_NETIF_HOSTNAME
  /* Initialize interface hostname */
  // Modified
  netif->hostname = LWIP_HOSTNAME;
#endif /* LWIP_NETIF_HOSTNAME */

  /*
   * Initialize the snmp variables and counters inside the struct netif.
   * The last argument should be replaced with your link speed, in units
   * of bits per second.
   */
  MIB2_INIT_NETIF(netif, snmp_ifType_ethernet_csmacd, LINK_SPEED_OF_YOUR_NETIF_IN_BPS);

  // Modified
  // netif->state = ethernetif;
  // netif->name[0] = IFNAME0;
  // netif->name[1] = IFNAME1;
  /* We directly use etharp_output() here to save a function call.
   * You can instead declare your own function an call etharp_output()
   * from it if you have to do some checks before sending (e.g. if link
   * is available...) */
#if LWIP_IPV4
  netif->output = etharp_output;
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */
  netif->linkoutput = low_level_output;

  // Modified
  // ethernetif->ethaddr = (struct eth_addr *) & (netif->hwaddr[0]);

  /* initialize the hardware */
  low_level_init(netif);

  return ERR_OK;
}

// Modified
#include <string.h>
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "netif/ethernet.h"
#include "netif/ethernetif.h"
#if LWIP_DNS
#include "lwip/dns.h"
#endif
#if LWIP_NAT
#include "lwip/nat.h"
#endif

#define SET_IP4_ADDR(ipaddr, addr) IP4_ADDR(ipaddr, addr)

static struct netif lwip_sta, lwip_uap;

void ethernetif_netif_init(u8_t *mac_addr) {
  if (!mac_addr) return;
  /* Initialize lwIP */
  lwip_init();
#if LWIP_NAT
  nat_init();
#endif
  /* Copy MAC address */
  memcpy(lwip_sta.hwaddr, mac_addr, ETHARP_HWADDR_LEN);
  memcpy(lwip_uap.hwaddr, mac_addr, ETHARP_HWADDR_LEN);
  /* Set abbreviated name */
  lwip_sta.name[0] = lwip_uap.name[0] = IFNAME_WLAN;
  lwip_sta.name[1] = IFNAME_STA;
  lwip_uap.name[1] = IFNAME_UAP;
  ip4_addr_t ipaddr = {}, netmask = {}, gateway = {};
  /* Configure netif w0 - STA */
  LWIP_DEBUGF(NETIF_DEBUG, ("configure lwip_sta\n"));
  netif_add(&lwip_sta, &ipaddr, &netmask, &gateway, NULL, ethernetif_init, ethernet_input);
  /* Configure netif w1 - UAP */
  LWIP_DEBUGF(NETIF_DEBUG, ("configure lwip_uap\n"));
  SET_IP4_ADDR(&ipaddr, LWIP_AP_IP);
  SET_IP4_ADDR(&netmask, LWIP_AP_NETMASK);
  SET_IP4_ADDR(&gateway, LWIP_AP_GATEWAY);
  netif_add(&lwip_uap, &ipaddr, &netmask, &gateway, NULL, ethernetif_init, ethernet_input);
#if LWIP_DNS
  /* LAN only, possibly replaced while using DHCP */
  SET_IP4_ADDR(&ipaddr, LWIP_AP_GATEWAY);
  dns_setserver(0, &ipaddr);
#endif
  /* Set STA as default netif */
  netif_set_default(&lwip_sta);
  /* Set up netif */
  netif_set_up(&lwip_sta);
  netif_set_up(&lwip_uap);
}

void ethernetif_data_input(u8_t *rx_buf, u8_t bss_type) {
  switch (bss_type) {
  case BSS_TYPE_STA:
    lwip_sta.state = rx_buf;
    ethernetif_input(&lwip_sta);
    break;
  case BSS_TYPE_UAP:
    lwip_uap.state = rx_buf;
    ethernetif_input(&lwip_uap);
  }
}

void ethernetif_link_down(u8_t bss_type) {
  switch (bss_type) {
  case BSS_TYPE_STA:
  #if LWIP_DHCP
    LWIP_DEBUGF(NETIF_DEBUG, ("stop DHCP client\n"));
    dhcp_release_and_stop(&lwip_sta);
  #endif
    netif_set_link_down(&lwip_sta);
    break;
  case BSS_TYPE_UAP:
  #if LWIP_DHCPD
    LWIP_DEBUGF(NETIF_DEBUG, ("stop DHCP server\n"));
    dhcpd_stop();
  #endif
    netif_set_link_down(&lwip_uap);
  }
}

#if LWIP_DHCPD
void ethernetif_link_up(u8_t bss_type, dhcpd_inform_fn access) {
#else
void ethernetif_link_up(u8_t bss_type, u8_t *access) {
#endif
  switch (bss_type) {
  case BSS_TYPE_STA:
    netif_set_link_up(&lwip_sta);
  #if LWIP_DHCP
    LWIP_DEBUGF(NETIF_DEBUG, ("start DHCP client\n"));
    if (!netif_dhcp_data(&lwip_sta)) dhcp_start(&lwip_sta);
  #endif
    break;
  case BSS_TYPE_UAP:
    netif_set_link_up(&lwip_uap);
  #if LWIP_DHCPD
    LWIP_DEBUGF(NETIF_DEBUG, ("start DHCP server\n"));
    dhcpd_start(&lwip_uap, access);
  #endif
  }
}

#if LWIP_DHCPD
void ethernetif_dhcpd_erase(u8_t *mac_addr, dhcpd_inform_fn info) {
  dhcpd_erase(mac_addr, info, NULL);
#else
void ethernetif_dhcpd_erase(u8_t *mac_addr, u8_t *info) {
#endif
}
