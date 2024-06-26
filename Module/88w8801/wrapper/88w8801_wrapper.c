#include "88w8801_wrapper.h"
#include "88w8801/sdio/88w8801_sdio.h"
#include "lwip/timeouts.h"
#include "systime/systime.h"

#define CHECK_TIMEOUTS_INTERVAL 0x7F

static core_err_e *sys_status = NULL;

uint16_t wrapper_init(core_err_e *status, wlan_cb_t *callback, GPIO_TypeDef *PDN_GPIO_Port, uint32_t PDN_Pin) {
    sys_status = status;
    uint8_t err = sdio_init(PDN_GPIO_Port, PDN_Pin);
    return err ? err << 8 : wlan_init(callback);
}

uint8_t wrapper_proc(void) {
    if (!((sys_now() & CHECK_TIMEOUTS_INTERVAL) != CHECK_TIMEOUTS_INTERVAL || *sys_status)) sys_check_timeouts();
    return __SDIO_GET_FLAG(SDIO, SDIO_FLAG_SDIOIT) ? (__SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_SDIOIT), wlan_process_packet()) : CORE_ERR_OK;
}

#if LWIP_TCP
err_t wrapper_tcp_connect(struct tcp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port, tcp_connected_fn connected, wlan_bss_type bss_type) {
    if (!(*pcb = tcp_new())) return ERR_MEM;
    tcp_bind_netif(*pcb, netif_get_by_index(bss_type + 1));
    err_t err = tcp_connect(*pcb, ipaddr, port, connected);
    if (err) wrapper_tcp_close(pcb);
    return err;
}

err_t wrapper_tcp_bind(struct tcp_pcb **pcb, uint16_t port, tcp_accept_fn accept, wlan_bss_type bss_type) {
    if (!(*pcb = tcp_new())) return ERR_MEM;
    tcp_bind_netif(*pcb, netif_get_by_index(bss_type + 1));
    err_t err = tcp_bind(*pcb, IP_ADDR_ANY, port);
    if (err) return wrapper_tcp_close(pcb), err;
    tcp_accept(*pcb = tcp_listen(*pcb), accept);
    return ERR_OK;
}

void wrapper_tcp_close(struct tcp_pcb **pcb) { tcp_close(*pcb), *pcb = NULL; }

err_t wrapper_tcp_write(struct tcp_pcb **pcb, uint8_t *data_buf, uint16_t data_len) { return tcp_write(*pcb, data_buf, data_len, TCP_WRITE_FLAG_COPY); }
#endif

#if LWIP_UDP
err_t wrapper_udp_new(struct udp_pcb **pcb, wlan_bss_type bss_type) {
    if (!(*pcb = udp_new())) return ERR_MEM;
    udp_bind_netif(*pcb, netif_get_by_index(bss_type + 1));
    return ERR_OK;
}

void wrapper_udp_remove(struct udp_pcb **pcb) { udp_remove(*pcb), *pcb = NULL; }

err_t wrapper_udp_bind(struct udp_pcb **pcb, uint16_t port, udp_recv_fn recv) {
    err_t err = udp_bind(*pcb, IP_ADDR_ANY, port);
    if (err) return err;
    udp_recv(*pcb, recv, NULL);
    return ERR_OK;
}

#if LWIP_UDP_CONNECTION
err_t wrapper_udp_connect(struct udp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port) { return udp_connect(*pcb, ipaddr, port); }

void wrapper_udp_disconnect(struct udp_pcb **pcb) { udp_disconnect(*pcb); }

err_t wrapper_udp_send(struct udp_pcb **pcb, uint8_t *data_buf, uint16_t data_len) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_ROM);
    if (!p) return ERR_MEM;
    p->payload = data_buf;
    err_t err = udp_send(*pcb, p);
    pbuf_free(p);
    return err;
}
#else
err_t wrapper_udp_sendto(struct udp_pcb **pcb, const ip_addr_t *ipaddr, uint16_t port, uint8_t *data_buf, uint16_t data_len) {
    struct pbuf *p = pbuf_alloc(PBUF_TRANSPORT, data_len, PBUF_ROM);
    if (!p) return ERR_MEM;
    p->payload = data_buf;
    err_t err = udp_sendto(*pcb, p, ipaddr, port);
    pbuf_free(p);
    return err;
}
#endif
#endif
