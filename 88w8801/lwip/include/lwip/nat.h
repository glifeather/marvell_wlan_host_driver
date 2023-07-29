#ifndef _LWIP_IPV4_NAT_
#define _LWIP_IPV4_NAT_
#include "lwip/opt.h"
#if LWIP_IPV4 && LWIP_NAT
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
  NAT_DIRECT,
  NAT_APPLIED
} nat_state_t;

PACK_STRUCT_BEGIN
struct nat_conf {
  ip4_addr_t src_ip_addr;
  ip4_addr_t src_netmask;
  ip4_addr_t dst_ip_addr;
  ip4_addr_t dst_netmask;
  struct netif *netif_in;
  struct netif *netif_out;
} PACK_STRUCT_STRUCT;
PACK_STRUCT_END

void nat_init(void);
err_t nat_add(struct nat_conf *conf);
void nat_remove(struct nat_conf *conf);
nat_state_t nat_input(struct pbuf *p);
nat_state_t nat_output(struct pbuf *p);

#ifdef __cplusplus
}
#endif

#endif
#endif
