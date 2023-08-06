/**
 * NAT - NAT implementation for lwIP supporting TCP/UDP and ICMP.
 * Copyright (c) 2009 Christian Walter, ?Embedded Solutions, Vienna 2009.
 * Copyright (c) 2010 lwIP project ;-)
 * COPYRIGHT (C) 2015, RT-Thread Development Team
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
 * Change Logs:
 * Date           Author       Notes
 * 2015-01-26     Hichard      porting to RT-Thread
 * 2015-01-27     Bernard      code cleanup for lwIP in RT-Thread
 * 2023-06-17     Glif         Code cleanup and refactoring
*/
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
