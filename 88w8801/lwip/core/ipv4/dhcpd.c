#include "lwip/opt.h"
#if LWIP_IPV4 && LWIP_DHCPD
#include <string.h>
#include "lwip/dhcpd.h"
#include "lwip/udp.h"
#include "lwip/prot/iana.h"
#include "lwip/prot/dhcp.h"
#if LWIP_DNS
#include "lwip/dns.h"
#endif
#if LWIP_NAT
#include "lwip/nat.h"
#include "88w8801/core/88w8801_core.h"
#endif

#ifndef LWIP_DEBUG
#undef LWIP_DEBUGF
#define LWIP_DEBUGF(...) \
    do { \
    } while (0)
#endif

#if !defined DHCPD_DEBUG || defined __DOXYGEN__
#define DHCPD_DEBUG LWIP_DBG_OFF
#endif

#define LWIP_COMBINEU32(addr) LWIP_MAKEU32(addr)

#define DHCP_EXTENDED_OPTIONS_LEN 0
#define DHCP_LEASE_TIME           86400
#define DHCP_INFORM_NULL          ((u8_t *)"-")
#define DHCP_OPTION_DOMAIN_NAME   15
#define DHCP_DOMAIN_NAME          "lan"

PACK_STRUCT_BEGIN
struct dhcpd_option_buf {
  u8_t code;
  u8_t len;
  u8_t data[];
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

struct dhcpd {
  u8_t msg_type;
  u8_t option_count;
  u8_t option_code[DHCP_MAX_OPTION_LEN];
  u8_t hostname_len;
  u8_t hostname[DHCP_MAX_OPTION_LEN];
  struct udp_pcb *pcb;
  struct dhcpd_address_list *addr;
  dhcpd_inform_fn access;
};

static void dhcpd_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port);
static err_t dhcpd_parse_request(struct pbuf *p);
static struct pbuf *dhcpd_create_msg(struct pbuf *p);
static err_t dhcpd_add_address(struct dhcp_msg *msg_in);
static err_t dhcpd_options(struct dhcpd_option_buf *option_buf, struct dhcpd_address_list *addr);

static struct dhcpd *dhcpd = NULL;

err_t dhcpd_start(struct netif *netif, dhcpd_inform_fn access) {
  LWIP_ASSERT_CORE_LOCKED();
  if (dhcpd) return ERR_OK;
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE, ("dhcpd_start()\n"));

  if (!(dhcpd = (struct dhcpd *)mem_malloc(sizeof(struct dhcpd)))) {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_start(): could not allocate dhcpd\n"));
    return ERR_MEM;
  }
  memset(dhcpd, 0, sizeof(struct dhcpd));

  if (!(dhcpd->pcb = udp_new())) return dhcpd_stop(), ERR_MEM;
  dhcpd->access = access;

  udp_bind_netif(dhcpd->pcb, netif);
  udp_bind(dhcpd->pcb, IP4_ADDR_ANY, LWIP_IANA_PORT_DHCP_SERVER);
  udp_recv(dhcpd->pcb, dhcpd_recv, NULL);
  return ERR_OK;
}

void dhcpd_stop(void) {
  LWIP_ASSERT_CORE_LOCKED();
  if (!dhcpd) return;
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE, ("dhcpd_stop()\n"));

  if (dhcpd->pcb) udp_remove(dhcpd->pcb);
  struct dhcpd_address_list *addr = dhcpd->addr;
  while (addr) {
    dhcpd->addr = addr->next;
    mem_free(addr);
    addr = dhcpd->addr;
  }
  mem_free(dhcpd), dhcpd = NULL;
}

void dhcpd_erase(u8_t *mac, dhcpd_inform_fn info, struct dhcpd_address_list *addr) {
  LWIP_ASSERT_CORE_LOCKED();
  if (!dhcpd) {
    if (mac && info) info(DHCP_INFORM_NULL, mac, DHCP_INFORM_NULL);
    return;
  }
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE, ("dhcpd_erase()\n"));

  if (!addr) {
    if (!mac) return;
    addr = dhcpd->addr;
    while (addr) {
      if (!memcmp(addr->mac_addr, mac, NETIF_MAX_HWADDR_LEN)) break;
      addr = addr->next;
    }
    if (!addr) {
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("dhcpd_erase(): MAC address does not exist\n"));
      if (info) info(DHCP_INFORM_NULL, mac, DHCP_INFORM_NULL);
      return;
    }
  }

#if LWIP_NAT
  struct nat_conf conf;
  conf.netif_in = netif_get_by_index(BSS_TYPE_UAP + 1);
  conf.netif_out = netif_get_by_index(BSS_TYPE_STA + 1);
  ip4_addr_copy(conf.src_ip_addr, addr->ip_addr);
  ip4_addr_copy(conf.dst_ip_addr, conf.netif_out->ip_addr);
  conf.src_netmask.addr = conf.dst_netmask.addr = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_NETMASK));
  nat_remove(&conf);
#endif
  if (info) info(*addr->hostname ? addr->hostname : DHCP_INFORM_NULL, addr->mac_addr, (u8_t *)ip4addr_ntoa((const ip4_addr_t *)&addr->ip_addr));

  // Erase client info
  addr->offer = 2;
  memset(addr->hostname, 0, sizeof(addr->hostname));
  memset(addr->mac_addr, 0, sizeof(addr->mac_addr));
}

static void dhcpd_recv(void *arg, struct udp_pcb *pcb, struct pbuf *p, const ip_addr_t *addr, u16_t port) {
  struct dhcp_msg *request_msg = (struct dhcp_msg *)p->payload;

  LWIP_UNUSED_ARG(arg);
  LWIP_UNUSED_ARG(pcb);
  LWIP_UNUSED_ARG(addr);
  LWIP_UNUSED_ARG(port);

  LWIP_ASSERT("invalid server address type\n", IP_IS_V4(addr));
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE, ("dhcpd_recv(pbuf %p): client %" U16_F ".%" U16_F ".%" U16_F ".%" U16_F ":%" U16_F " (", (void *)p, ip4_addr1_16(ip_2_ip4(addr)), ip4_addr2_16(ip_2_ip4(addr)), ip4_addr3_16(ip_2_ip4(addr)), ip4_addr4_16(ip_2_ip4(addr)), port));
  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE, ("len %" U16_F ", tot_len %" U16_F ")\n", p->len, p->tot_len));

  do {
    if (request_msg->op != DHCP_BOOTREQUEST) {
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("dhcpd_recv(): message type %" U16_F " is not DHCP request\n", (u16_t)request_msg->op));
      break;
    }

    if (dhcpd_parse_request(p)) {
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_recv(): unfolding DHCP message, too short on memory?\n"));
      break;
    }

    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_STATE, ("message type %" U16_F "\n", dhcpd->msg_type));
    struct pbuf *p_out = dhcpd_create_msg(p);
    if (!p_out) break;
    // Broadcast before completing allocation
    udp_sendto(dhcpd->pcb, p_out, IP_ADDR_BROADCAST, LWIP_IANA_PORT_DHCP_CLIENT);
    pbuf_free(p_out);
  } while (0);

  pbuf_free(p);
}

static err_t dhcpd_parse_request(struct pbuf *p) {
  if (p->len < DHCP_OPTIONS_OFS + sizeof(struct dhcpd_option_buf)) return ERR_BUF;

  u8_t *option = ((struct dhcp_msg *)p->payload)->options;
  struct dhcpd_option_buf *option_buf = (struct dhcpd_option_buf *)option;
  u16_t option_tot_len = p->len - DHCP_OPTIONS_OFS;

  do {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_STATE, ("option code %" U16_F ", len %" U16_F "\n", option_buf->code, option_buf->len));
    switch (option_buf->code) {
    case DHCP_OPTION_HOSTNAME: memcpy(dhcpd->hostname, option_buf->data, dhcpd->hostname_len = option_buf->len); break;
    case DHCP_OPTION_MESSAGE_TYPE: dhcpd->msg_type = *option_buf->data; break;
    case DHCP_OPTION_PARAMETER_REQUEST_LIST: memcpy(dhcpd->option_code, option_buf->data, dhcpd->option_count = option_buf->len); break;
    case DHCP_OPTION_END: return ERR_OK;
    }
    option_buf = (struct dhcpd_option_buf *)(option += option_buf->len + sizeof(struct dhcpd_option_buf));
  } while (option_tot_len -= option_buf->len + sizeof(struct dhcpd_option_buf));

  return ERR_BUF;
}

static struct pbuf *dhcpd_create_msg(struct pbuf *p) {
  struct pbuf *p_out = pbuf_alloc(PBUF_TRANSPORT, sizeof(struct dhcp_msg) + DHCP_EXTENDED_OPTIONS_LEN, PBUF_RAM);
  if (!p_out) {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_create_msg(): could not allocate pbuf\n"));
    return NULL;
  }
  LWIP_ASSERT("pbuf cannot hold dhcp_msg\n", (p_out->len >= sizeof(struct dhcp_msg) + DHCP_EXTENDED_OPTIONS_LEN));

  struct dhcp_msg *msg_in = (struct dhcp_msg *)p->payload, *msg_out = (struct dhcp_msg *)p_out->payload;
  memset(msg_out, 0, p_out->len);

  msg_out->op = DHCP_BOOTREPLY;
  /* @todo: make link layer independent */
  msg_out->htype = LWIP_IANA_HWTYPE_ETHERNET;
  msg_out->hlen = NETIF_MAX_HWADDR_LEN;
  msg_out->xid = msg_in->xid;
  msg_out->flags = msg_in->flags;
  ip4_addr_copy(msg_out->ciaddr, msg_in->ciaddr);

  struct dhcpd_address_list *addr = dhcpd->addr, *vacant = NULL;
  if (addr) {
    do {
      if (addr->offer & 2) vacant = addr;
      else if (!memcmp(addr->mac_addr, msg_in->chaddr, NETIF_MAX_HWADDR_LEN)) {
        // Update hostname
        if (dhcpd->hostname_len) {
          memset(addr->hostname, 0, sizeof(addr->hostname));
          memcpy(addr->hostname, dhcpd->hostname, dhcpd->hostname_len);
          dhcpd->hostname_len = 0;
        }
        break;
      }
    } while ((addr = addr->next));
    if (!addr && (addr = vacant)) {
      // Fill vacant node
      addr->offer = 0;
      if (dhcpd->hostname_len) {
        memcpy(addr->hostname, dhcpd->hostname, dhcpd->hostname_len);
        dhcpd->hostname_len = 0;
      }
      memcpy(addr->mac_addr, msg_in->chaddr, NETIF_MAX_HWADDR_LEN);
      LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_STATE, ("offered IP %s\n", ip4addr_ntoa((const ip4_addr_t *)&addr->ip_addr)));
    }
  }

  do {
    if (!addr) {
      // Create new node
      if (dhcpd_add_address(msg_in)) break;
      addr = dhcpd->addr;
    }
    ip4_addr_copy(msg_out->yiaddr, addr->ip_addr);

    memcpy(msg_out->chaddr, msg_in->chaddr, LWIP_MIN(DHCP_CHADDR_LEN, NETIF_MAX_HWADDR_LEN));
    msg_out->cookie = PP_HTONL(DHCP_MAGIC_COOKIE);
    if (!dhcpd_options((struct dhcpd_option_buf *)msg_out->options, addr)) return p_out;
  } while (0);

  pbuf_free(p_out);
  return NULL;
}

static err_t dhcpd_add_address(struct dhcp_msg *msg_in) {
  struct dhcpd_address_list *addr = dhcpd->addr;
  if (!(dhcpd->addr = (struct dhcpd_address_list *)mem_malloc(sizeof(struct dhcpd_address_list)))) {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_add_address(): could not allocate dhcpd_address_list\n"));
    dhcpd->addr = addr;
    return ERR_MEM;
  }
  memset(dhcpd->addr, 0, sizeof(struct dhcpd_address_list));

  if (dhcpd->hostname_len) {
    memcpy(dhcpd->addr->hostname, dhcpd->hostname, dhcpd->hostname_len);
    dhcpd->hostname_len = 0;
  }
  memcpy(dhcpd->addr->mac_addr, msg_in->chaddr, NETIF_MAX_HWADDR_LEN);
  dhcpd->addr->ip_addr.addr = addr ? lwip_htonl(lwip_ntohl(addr->ip_addr.addr) + 1) : PP_HTONL(LWIP_COMBINEU32(LWIP_AP_IP) + LWIP_OFFERED_IP_OFFSET);
  if (lwip_ntohl(dhcpd->addr->ip_addr.addr) >= LWIP_COMBINEU32(LWIP_AP_IP) + LWIP_BROADCAST_OFFSET) {
    LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_add_address(): invalid offered IP\n"));
    mem_free(dhcpd->addr);
    dhcpd->addr = addr;
    return ERR_VAL;
  }
  dhcpd->addr->next = addr;

  LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_STATE, ("offered IP %s\n", ip4addr_ntoa((const ip4_addr_t *)&dhcpd->addr->ip_addr)));
  return ERR_OK;
}

static err_t dhcpd_options(struct dhcpd_option_buf *option_buf, struct dhcpd_address_list *addr) {
  // At most (DHCP_OPTIONS_LEN + DHCP_EXTENDED_OPTIONS_LEN) bytes
  option_buf->code = DHCP_OPTION_MESSAGE_TYPE;
  option_buf->len = 1;
  switch (dhcpd->msg_type) {
  case DHCP_DISCOVER:
    *option_buf->data = DHCP_OFFER;
    addr->offer = 1;
    break;
  case DHCP_REQUEST:
    if (addr->offer) {
      *option_buf->data = DHCP_ACK;
      addr->offer = 0;
#if LWIP_NAT
      struct nat_conf conf;
      conf.netif_in = netif_get_by_index(BSS_TYPE_UAP + 1);
      conf.netif_out = netif_get_by_index(BSS_TYPE_STA + 1);
      ip4_addr_copy(conf.src_ip_addr, addr->ip_addr);
      ip4_addr_copy(conf.dst_ip_addr, conf.netif_out->ip_addr);
      conf.src_netmask.addr = conf.dst_netmask.addr = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_NETMASK));
      if (nat_add(&conf)) LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("dhcpd_options(): failed to add NAT configuration\n"));
#endif
      if (dhcpd->access) dhcpd->access(*addr->hostname ? addr->hostname : DHCP_INFORM_NULL, addr->mac_addr, (u8_t *)ip4addr_ntoa((const ip4_addr_t *)&addr->ip_addr));
    } else *option_buf->data = DHCP_NAK;
    break;
  // Response cancelled
  case DHCP_DECLINE:
  case DHCP_RELEASE: dhcpd_erase(NULL, NULL, addr); return ERR_BUF;
  case DHCP_INFORM: *option_buf->data = DHCP_ACK; break;
  default: LWIP_DEBUGF(DHCPD_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("dhcpd_options(): invalid message type\n")); return ERR_VAL;
  }
  option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));

  option_buf->code = DHCP_OPTION_LEASE_TIME;
  option_buf->len = 4;
  *((u32_t *)option_buf->data) = PP_HTONL(DHCP_LEASE_TIME);
  option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));

  option_buf->code = DHCP_OPTION_SERVER_ID;
  option_buf->len = 4;
  *((u32_t *)option_buf->data) = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_GATEWAY));
  option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));

  option_buf->code = DHCP_OPTION_T1;
  option_buf->len = 4;
  *((u32_t *)option_buf->data) = PP_HTONL(DHCP_LEASE_TIME / 2);
  option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));

  option_buf->code = DHCP_OPTION_T2;
  option_buf->len = 4;
  *((u32_t *)option_buf->data) = PP_HTONL(DHCP_LEASE_TIME / 8 * 7);
  option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));

  u8_t count = dhcpd->option_count++;
  while (--dhcpd->option_count) {
    switch (option_buf->code = *(dhcpd->option_code + count - dhcpd->option_count)) {
    case DHCP_OPTION_SUBNET_MASK:
      option_buf->len = 4;
      *((u32_t *)option_buf->data) = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_NETMASK));
      break;
    case DHCP_OPTION_ROUTER:
      option_buf->len = 4;
      *((u32_t *)option_buf->data) = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_GATEWAY));
      break;
#if LWIP_DNS
    case DHCP_OPTION_DNS_SERVER:
      option_buf->len = 4;
      *((u32_t *)option_buf->data) = dns_getserver(0)->addr;
      break;
#endif
    case DHCP_OPTION_DOMAIN_NAME: memcpy(option_buf->data, DHCP_DOMAIN_NAME, option_buf->len = sizeof(DHCP_DOMAIN_NAME) - 1); break;
    case DHCP_OPTION_BROADCAST:
      option_buf->len = 4;
      *((u32_t *)option_buf->data) = PP_HTONL(LWIP_COMBINEU32(LWIP_AP_IP) + LWIP_BROADCAST_OFFSET);
      break;
    default: option_buf->code = DHCP_OPTION_PAD; continue;
    }
    option_buf = (struct dhcpd_option_buf *)((u8_t *)option_buf + option_buf->len + sizeof(struct dhcpd_option_buf));
  }

  option_buf->code = DHCP_OPTION_END;
  return ERR_OK;
}
#endif
