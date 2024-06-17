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
 * 2024-06-06     Glif         Code cleanup and refactoring
 */
#include "lwip/opt.h"
#if LWIP_IPV4 && LWIP_NAT
#include <string.h>
#include "lwip/nat.h"
#include "lwip/timeouts.h"
#include "lwip/prot/ip4.h"
#include "lwip/prot/ip.h"
#if LWIP_ICMP
#include "lwip/prot/icmp.h"
#endif
#if LWIP_TCP
#include "lwip/prot/tcp.h"
#endif
#if LWIP_UDP
#include "lwip/prot/udp.h"
#endif

#ifndef LWIP_DEBUG
#undef LWIP_DEBUGF
#define LWIP_DEBUGF(...) \
    do { \
    } while (0)
#endif

#if !defined NAT_DEBUG || defined __DOXYGEN__
#define NAT_DEBUG LWIP_DBG_OFF
#endif

#if LWIP_ICMP
#define NAT_ICMP_QUEUE_NUM (MAX_CLIENT_NUM * 4)
#endif
#if LWIP_TCP
#define NAT_TCP_QUEUE_NUM       (MAX_CLIENT_NUM * 32)
#define NAT_TCP_SRC_PORT_OFFSET 39999
#endif
#if LWIP_UDP
#define NAT_UDP_QUEUE_NUM       (MAX_CLIENT_NUM * 32)
#define NAT_UDP_SRC_PORT_OFFSET 39999
#endif
#define NAT_TIMER_INTERVAL 10
#define NAT_DEFAULT_TTL    (NAT_TIMER_INTERVAL * 3)

struct nat_conf_list {
  struct nat_conf conf;
  struct nat_conf_list *next;
};

struct nat_queue {
  u8_t ttl;
  ip4_addr_t src;
  ip4_addr_t dst;
  struct nat_conf_list *conf_list;
  union {
#if LWIP_ICMP
    struct {
      u16_t id;
      u16_t seqno;
    } icmp;
#endif
#if LWIP_TCP || LWIP_UDP
    struct {
      u16_t nport;
      u16_t sport;
      u16_t dport;
    } tcp_udp;
#endif
  } params;
};

static void nat_timer(void *arg);
static void nat_timeout(struct nat_queue *queue, u8_t num);
static void nat_free(struct nat_conf_list **conf_list_prev, u8_t head);
static void nat_reset_ttl(struct nat_queue *queue, u8_t num, struct nat_conf_list *conf_list);
static void *nat_check_hdr(struct pbuf *p, u16_t hdr_size);
static void nat_chksum(u8_t *chksum, u8_t *ptr_old, u8_t *ptr_new, u16_t ptr_len);
static void nat_apply_in(struct pbuf *p, struct ip_hdr *ip_hdr, struct nat_queue *nat_queue);
static void nat_apply_out(struct pbuf *p, struct ip_hdr *ip_hdr, struct nat_queue *nat_queue);

static struct nat_conf_list *nat_conf_list = NULL;
#if LWIP_ICMP
static struct nat_queue icmp_queue[NAT_ICMP_QUEUE_NUM] = {0};
#endif
#if LWIP_TCP
static struct nat_queue tcp_queue[NAT_TCP_QUEUE_NUM] = {0};
#endif
#if LWIP_UDP
static struct nat_queue udp_queue[NAT_UDP_QUEUE_NUM] = {0};
#endif

void nat_init(void) { sys_timeout(NAT_TIMER_INTERVAL * 1000, nat_timer, NULL); }

err_t nat_add(struct nat_conf *conf) {
  LWIP_ASSERT_CORE_LOCKED();
  if (!conf || !(conf->netif_in && conf->netif_out)) return ERR_ARG;
  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("nat_add()\n"));

  struct nat_conf_list *conf_list = (struct nat_conf_list *)mem_malloc(sizeof(struct nat_conf_list));
  if (!conf_list) {
    LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("nat_add(): could not allocate nat_conf_list\n"));
    return ERR_MEM;
  }
  memcpy(&conf_list->conf, conf, sizeof(struct nat_conf));
  conf_list->next = nat_conf_list;

  if (nat_conf_list) {
    while (nat_conf_list->next) nat_conf_list = nat_conf_list->next;
    nat_conf_list->next = conf_list;
    nat_conf_list = conf_list->next;
    conf_list->next = NULL;
  } else nat_conf_list = conf_list;
  return ERR_OK;
}

void nat_remove(struct nat_conf *conf) {
  LWIP_ASSERT_CORE_LOCKED();
  if (!conf) return;
  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("nat_remove()\n"));

  if (nat_conf_list) {
    if (!memcmp(&nat_conf_list->conf, conf, sizeof(struct nat_conf))) {
      nat_free(&nat_conf_list, 1);
      return;
    }
    struct nat_conf_list *conf_list = nat_conf_list;
    while (conf_list->next) {
      if (!memcmp(&conf_list->next->conf, conf, sizeof(struct nat_conf))) {
        nat_free(&conf_list, 0);
        return;
      }
      conf_list = conf_list->next;
    }
  }
  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_remove(): configuration does not exist\n"));
}

nat_state_t nat_input(struct pbuf *p) {
  LWIP_ASSERT_CORE_LOCKED();
  do {
    if (!p) break;
    LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("nat_input()\n"));

    struct ip_hdr *ip_hdr = (struct ip_hdr *)p->payload;
    switch (IPH_PROTO(ip_hdr)) {
#if LWIP_ICMP
    case IP_PROTO_ICMP: {
      struct icmp_echo_hdr *icmp_hdr = (struct icmp_echo_hdr *)nat_check_hdr(p, sizeof(struct icmp_echo_hdr));
      if (icmp_hdr) {
        if (ICMPH_TYPE(icmp_hdr) == ICMP_ER) {
          struct nat_queue *nat_queue;
          u8_t num = NAT_ICMP_QUEUE_NUM;
          while (num--) {
            nat_queue = icmp_queue + num;
            if (!nat_queue->ttl || nat_queue->dst.addr != ip_hdr->src.addr || nat_queue->params.icmp.id != icmp_hdr->id || nat_queue->params.icmp.seqno != icmp_hdr->seqno) continue;
            nat_queue->ttl = 0;
            nat_apply_in(p, ip_hdr, nat_queue);
            return NAT_APPLIED;
          }
        }
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_input(): %" U16_F " bytes icmp echo reply packet, discarded\n", p->tot_len));
      break;
    }
#endif
#if LWIP_TCP
    case IP_PROTO_TCP: {
      struct tcp_hdr *tcp_hdr = (struct tcp_hdr *)nat_check_hdr(p, sizeof(struct tcp_hdr));
      if (tcp_hdr) {
        struct nat_queue *nat_queue;
        u8_t num = NAT_TCP_QUEUE_NUM;
        while (num--) {
          nat_queue = tcp_queue + num;
          if (!nat_queue->ttl || nat_queue->dst.addr != ip_hdr->src.addr || nat_queue->params.tcp_udp.dport != tcp_hdr->src || nat_queue->params.tcp_udp.nport != tcp_hdr->dest) continue;
          nat_queue->ttl = NAT_DEFAULT_TTL;
          tcp_hdr->dest = nat_queue->params.tcp_udp.sport;
          nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.nport, (u8_t *)&tcp_hdr->dest, 2);
          nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, (u8_t *)&nat_queue->src.addr, 4);
          nat_apply_in(p, ip_hdr, nat_queue);
          return NAT_APPLIED;
        }
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_input(): %" U16_F " bytes tcp packet, discarded\n", p->tot_len));
      break;
    }
#endif
#if LWIP_UDP
    case IP_PROTO_UDP: {
      struct udp_hdr *udp_hdr = (struct udp_hdr *)nat_check_hdr(p, sizeof(struct udp_hdr));
      if (udp_hdr) {
        struct nat_queue *nat_queue;
        u8_t num = NAT_UDP_QUEUE_NUM;
        while (num--) {
          nat_queue = udp_queue + num;
          if (!nat_queue->ttl || nat_queue->dst.addr != ip_hdr->src.addr || nat_queue->params.tcp_udp.dport != udp_hdr->src || nat_queue->params.tcp_udp.nport != udp_hdr->dest) continue;
          nat_queue->ttl = NAT_DEFAULT_TTL;
          udp_hdr->dest = nat_queue->params.tcp_udp.sport;
          nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.nport, (u8_t *)&udp_hdr->dest, 2);
          nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, (u8_t *)&nat_queue->src.addr, 4);
          nat_apply_in(p, ip_hdr, nat_queue);
          return NAT_APPLIED;
        }
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_input(): %" U16_F " bytes udp packet, discarded\n", p->tot_len));
      break;
    }
#endif
    }
  } while (0);

  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_STATE, ("NAT direct\n"));
  return NAT_DIRECT;
}

nat_state_t nat_output(struct pbuf *p) {
  LWIP_ASSERT_CORE_LOCKED();
  do {
    if (!p) break;
    LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("nat_output()\n"));

    struct ip_hdr *ip_hdr = (struct ip_hdr *)p->payload;
    struct nat_conf_list *conf_list = nat_conf_list;
    while (conf_list) {
      if (ip4_addr_netcmp(&ip_hdr->src, &conf_list->conf.src_ip_addr, &conf_list->conf.src_netmask) || ip4_addr_netcmp(&ip_hdr->dest, &conf_list->conf.dst_ip_addr, &conf_list->conf.dst_netmask)) break;
      conf_list = conf_list->next;
    }
    if (!conf_list) break;

    switch (IPH_PROTO(ip_hdr)) {
#if LWIP_ICMP
    case IP_PROTO_ICMP: {
      struct icmp_echo_hdr *icmp_hdr = (struct icmp_echo_hdr *)nat_check_hdr(p, sizeof(struct icmp_echo_hdr));
      if (icmp_hdr) {
        if (ICMPH_TYPE(icmp_hdr) == ICMP_ECHO) {
          struct nat_queue *nat_queue;
          u8_t num = NAT_ICMP_QUEUE_NUM, vacant = NAT_ICMP_QUEUE_NUM;
          while (num--) {
            if ((nat_queue = icmp_queue + num)->ttl) {
              if (nat_queue->src.addr != ip_hdr->src.addr || nat_queue->dst.addr != ip_hdr->dest.addr || nat_queue->params.icmp.id != icmp_hdr->id || nat_queue->params.icmp.seqno != icmp_hdr->seqno) continue;
              nat_apply_out(p, ip_hdr, nat_queue);
              return NAT_APPLIED;
            } else if (vacant == NAT_ICMP_QUEUE_NUM) vacant = num;
          }
          if (vacant != NAT_ICMP_QUEUE_NUM) {
            (nat_queue = icmp_queue + vacant)->ttl = NAT_DEFAULT_TTL;
            ip4_addr_copy(nat_queue->src, ip_hdr->src);
            ip4_addr_copy(nat_queue->dst, ip_hdr->dest);
            nat_queue->conf_list = conf_list;
            nat_queue->params.icmp.id = icmp_hdr->id;
            nat_queue->params.icmp.seqno = icmp_hdr->seqno;
            nat_apply_out(p, ip_hdr, nat_queue);
            return NAT_APPLIED;
          }
          LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): icmp_queue is not available\n"));
        }
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): %" U16_F " bytes icmp echo packet, discarded\n", p->tot_len));
      break;
    }
#endif
#if LWIP_TCP
    case IP_PROTO_TCP: {
      struct tcp_hdr *tcp_hdr = (struct tcp_hdr *)nat_check_hdr(p, sizeof(struct tcp_hdr));
      if (tcp_hdr) {
        struct nat_queue *nat_queue;
        u8_t num = NAT_TCP_QUEUE_NUM, vacant = NAT_TCP_QUEUE_NUM;
        while (num--) {
          if ((nat_queue = tcp_queue + num)->ttl) {
            if (nat_queue->src.addr != ip_hdr->src.addr || nat_queue->dst.addr != ip_hdr->dest.addr || nat_queue->params.tcp_udp.sport != tcp_hdr->src || nat_queue->params.tcp_udp.dport != tcp_hdr->dest) continue;
            tcp_hdr->src = nat_queue->params.tcp_udp.nport;
            nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.sport, (u8_t *)&tcp_hdr->src, 2);
            nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->src.addr, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, 4);
            nat_apply_out(p, ip_hdr, nat_queue);
            return NAT_APPLIED;
          } else if (vacant == NAT_TCP_QUEUE_NUM) vacant = num;
        }
        if (vacant != NAT_TCP_QUEUE_NUM) {
          (nat_queue = tcp_queue + vacant)->ttl = NAT_DEFAULT_TTL;
          ip4_addr_copy(nat_queue->src, ip_hdr->src);
          ip4_addr_copy(nat_queue->dst, ip_hdr->dest);
          nat_queue->conf_list = conf_list;
          nat_queue->params.tcp_udp.nport = lwip_htons(NAT_TCP_SRC_PORT_OFFSET + NAT_TCP_QUEUE_NUM - vacant);
          nat_queue->params.tcp_udp.sport = tcp_hdr->src;
          nat_queue->params.tcp_udp.dport = tcp_hdr->dest;
          tcp_hdr->src = nat_queue->params.tcp_udp.nport;
          nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.sport, (u8_t *)&tcp_hdr->src, 2);
          nat_chksum((u8_t *)&tcp_hdr->chksum, (u8_t *)&nat_queue->src.addr, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, 4);
          nat_apply_out(p, ip_hdr, nat_queue);
          return NAT_APPLIED;
        }
        LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): tcp_queue is not available\n"));
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): %" U16_F " bytes tcp packet, discarded\n", p->tot_len));
      break;
    }
#endif
#if LWIP_UDP
    case IP_PROTO_UDP: {
      struct udp_hdr *udp_hdr = (struct udp_hdr *)nat_check_hdr(p, sizeof(struct udp_hdr));
      if (udp_hdr) {
        struct nat_queue *nat_queue;
        u8_t num = NAT_UDP_QUEUE_NUM, vacant = NAT_UDP_QUEUE_NUM;
        while (num--) {
          if ((nat_queue = udp_queue + num)->ttl) {
            if (nat_queue->src.addr != ip_hdr->src.addr || nat_queue->dst.addr != ip_hdr->dest.addr || nat_queue->params.tcp_udp.sport != udp_hdr->src || nat_queue->params.tcp_udp.dport != udp_hdr->dest) continue;
            udp_hdr->src = nat_queue->params.tcp_udp.nport;
            nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.sport, (u8_t *)&udp_hdr->src, 2);
            nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->src.addr, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, 4);
            nat_apply_out(p, ip_hdr, nat_queue);
            return NAT_APPLIED;
          } else if (vacant == NAT_UDP_QUEUE_NUM) vacant = num;
        }
        if (vacant != NAT_UDP_QUEUE_NUM) {
          (nat_queue = udp_queue + vacant)->ttl = NAT_DEFAULT_TTL;
          ip4_addr_copy(nat_queue->src, ip_hdr->src);
          ip4_addr_copy(nat_queue->dst, ip_hdr->dest);
          nat_queue->conf_list = conf_list;
          nat_queue->params.tcp_udp.nport = lwip_htons(NAT_UDP_SRC_PORT_OFFSET + NAT_UDP_QUEUE_NUM - vacant);
          nat_queue->params.tcp_udp.sport = udp_hdr->src;
          nat_queue->params.tcp_udp.dport = udp_hdr->dest;
          udp_hdr->src = nat_queue->params.tcp_udp.nport;
          nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->params.tcp_udp.sport, (u8_t *)&udp_hdr->src, 2);
          nat_chksum((u8_t *)&udp_hdr->chksum, (u8_t *)&nat_queue->src.addr, (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, 4);
          nat_apply_out(p, ip_hdr, nat_queue);
          return NAT_APPLIED;
        }
        LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): udp_queue is not available\n"));
      } else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_WARNING, ("nat_output(): %" U16_F " bytes udp packet, discarded\n", p->tot_len));
      break;
    }
#endif
    }
  } while (0);

  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_STATE, ("NAT direct\n"));
  return NAT_DIRECT;
}

static void nat_timer(void *arg) {
  LWIP_UNUSED_ARG(arg);
#if LWIP_DEBUG_TIMERNAMES
  LWIP_DEBUGF(TIMERS_DEBUG, ("tcpip: nat_timer()\n"));
#endif

#if LWIP_ICMP
  nat_timeout(icmp_queue, NAT_ICMP_QUEUE_NUM);
#endif
#if LWIP_TCP
  nat_timeout(tcp_queue, NAT_TCP_QUEUE_NUM);
#endif
#if LWIP_UDP
  nat_timeout(udp_queue, NAT_UDP_QUEUE_NUM);
#endif

  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("update NAT timer\n"));
  sys_timeout(NAT_TIMER_INTERVAL * 1000, nat_timer, NULL);
}

static void nat_timeout(struct nat_queue *queue, u8_t num) {
  while (num--) {
    if (!(queue + num)->ttl) continue;
    (queue + num)->ttl -= (queue + num)->ttl > NAT_TIMER_INTERVAL ? NAT_TIMER_INTERVAL : (queue + num)->ttl;
  }
}

static void nat_free(struct nat_conf_list **conf_list_prev, u8_t head) {
  struct nat_conf_list *conf_list;
  if (head) *conf_list_prev = (conf_list = *conf_list_prev)->next;
  else (*conf_list_prev)->next = (conf_list = (*conf_list_prev)->next)->next;

#if LWIP_ICMP
  nat_reset_ttl(icmp_queue, NAT_ICMP_QUEUE_NUM, conf_list);
#endif
#if LWIP_TCP
  nat_reset_ttl(tcp_queue, NAT_TCP_QUEUE_NUM, conf_list);
#endif
#if LWIP_UDP
  nat_reset_ttl(udp_queue, NAT_UDP_QUEUE_NUM, conf_list);
#endif
  mem_free(conf_list);
}

static void nat_reset_ttl(struct nat_queue *queue, u8_t num, struct nat_conf_list *conf_list) {
  while (num--) {
    if ((queue + num)->conf_list != conf_list) continue;
    (queue + num)->ttl = 0;
  }
}

static void *nat_check_hdr(struct pbuf *p, u16_t hdr_size) {
  u8_t iphdr_size = IPH_HL_BYTES((struct ip_hdr *)p->payload);
  void *ret = NULL;
  if (!pbuf_header(p, -iphdr_size)) {
    if (p->tot_len >= hdr_size) ret = p->payload;
    pbuf_header(p, iphdr_size);
  }
  return ret;
}

static void nat_chksum(u8_t *chksum, u8_t *ptr_old, u8_t *ptr_new, u16_t ptr_len) {
  if (!(chksum && ptr_old && ptr_new && ptr_len) || ptr_len & 1) return;
  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE, ("nat_chksum(chksum %p): ptr_old %p, ptr_new %p, ptr_len %" U16_F "\n", chksum, ptr_old, ptr_new, ptr_len));

  u16_t len = ptr_len;
  s32_t x = ~(*chksum * 0x100 + *(chksum + 1)) & 0xFFFF;
  do {
    if ((x -= (*ptr_old * 0x100 + *(ptr_old + 1)) & 0xFFFF) <= 0) x = (x - 1) & 0xFFFF;
    ptr_old += 2;
  } while (len -= 2);
  do {
    if ((x += (*ptr_new * 0x100 + *(ptr_new + 1)) & 0xFFFF) & 0x10000) x = (x + 1) & 0xFFFF;
    ptr_new += 2;
  } while (ptr_len -= 2);
  x = ~x & 0xFFFF;

  *chksum = x / 0x100;
  *(chksum + 1) = x & 0xFF;
  LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_STATE, ("chksum 0x%X\n", *(u16_t *)chksum));
}

static void nat_apply_in(struct pbuf *p, struct ip_hdr *ip_hdr, struct nat_queue *nat_queue) {
  struct pbuf *q = NULL;
  if (pbuf_header(p, PBUF_LINK_HLEN)) {
    if (!(q = pbuf_alloc(PBUF_LINK, 0, PBUF_RAM))) {
      LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("nat_apply_in(): could not allocate pbuf\n"));
      pbuf_free(p), p = NULL;
      return;
    } else pbuf_cat(q, p);
  } else if (pbuf_header(p, -PBUF_LINK_HLEN)) {
    LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("nat_apply_in(): failed to restore header\n"));
    pbuf_free(p), p = NULL;
    return;
  } else q = p;

  ip4_addr_copy(ip_hdr->dest, nat_queue->src);
  nat_chksum((u8_t *)&IPH_CHKSUM(ip_hdr), (u8_t *)&nat_queue->conf_list->conf.netif_out->ip_addr.addr, (u8_t *)&ip_hdr->dest.addr, 4);

  struct netif *netif_in = nat_queue->conf_list->conf.netif_in;
  ip4_addr_t ip_addr_in;
  ip4_addr_copy(ip_addr_in, ip_hdr->dest);
  if (netif_in->output(netif_in, q, &ip_addr_in) != ERR_OK) LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("nat_apply_in(): failed to send modified packet\n"));
  else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_STATE, ("NAT applied\n"));
  pbuf_free(q);
}

static void nat_apply_out(struct pbuf *p, struct ip_hdr *ip_hdr, struct nat_queue *nat_queue) {
  struct netif *netif_out = nat_queue->conf_list->conf.netif_out;
  ip4_addr_copy(ip_hdr->src, netif_out->ip_addr);
  nat_chksum((u8_t *)&IPH_CHKSUM(ip_hdr), (u8_t *)&nat_queue->src.addr, (u8_t *)&ip_hdr->src.addr, 4);

  ip4_addr_t ip_addr_out;
  ip4_addr_copy(ip_addr_out, ip_hdr->dest);
  if (netif_out->output(netif_out, p, &ip_addr_out) != ERR_OK) LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_TRACE | LWIP_DBG_LEVEL_SERIOUS, ("nat_apply_out(): failed to send modified packet\n"));
  else LWIP_DEBUGF(NAT_DEBUG | LWIP_DBG_STATE, ("NAT applied\n"));
}
#endif
