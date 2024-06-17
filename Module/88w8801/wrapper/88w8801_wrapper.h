#ifndef _88W8801_WRAPPER_
#define _88W8801_WRAPPER_
#include <stm32f4xx.h>
#include "88w8801/core/88w8801_core.h"
#include "lwip/opt.h"
#if LWIP_TCP
#include "lwip/tcp.h"
#endif
#if LWIP_UDP
#include "lwip/udp.h"
#endif

#define UDP_USE_CONNECTION 1

uint8_t wrapper_init(core_err_e *status, wlan_cb_t *callback, GPIO_TypeDef *PDN_GPIO_Port, uint32_t PDN_Pin);
uint8_t wrapper_proc(void);
#if LWIP_TCP
err_t wrapper_tcp_connect(struct tcp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port, tcp_connected_fn connected, wlan_bss_type bss_type);
err_t wrapper_tcp_bind(struct tcp_pcb **pcb, uint16_t port, tcp_accept_fn accept, wlan_bss_type bss_type);
void wrapper_tcp_close(struct tcp_pcb **pcb);
err_t wrapper_tcp_write(struct tcp_pcb **pcb, uint8_t *data_buf, uint16_t data_len);
#endif
#if LWIP_UDP
err_t wrapper_udp_new(struct udp_pcb **pcb, wlan_bss_type bss_type);
void wrapper_udp_remove(struct udp_pcb **pcb);
err_t wrapper_udp_bind(struct udp_pcb **pcb, uint16_t port, udp_recv_fn recv);
#if UDP_USE_CONNECTION
err_t wrapper_udp_connect(struct udp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port);
void wrapper_udp_disconnect(struct udp_pcb **pcb);
err_t wrapper_udp_send(struct udp_pcb **pcb, uint8_t *data_buf, uint16_t data_len);
#else
err_t wrapper_udp_sendto(struct udp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port, uint8_t *data_buf, uint16_t data_len);
#endif
#endif
#endif
