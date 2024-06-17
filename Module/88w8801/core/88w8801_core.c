#include <string.h>
#include "88w8801_core.h"
#include "88w8801/sdio/88w8801_sdio.h"
#include "netif/ethernetif.h"
#ifdef USE_FLASH_FIRMWARE
#include "88w8801/flash/88w8801_flash.h"
#endif

#ifdef WLAN_CORE_DEBUG
#include <stdio.h>
#define CORE_DEBUG printf
#else
#define CORE_DEBUG(...) \
    do { \
    } while (0)
#endif

uint8_t wlan_tx_buf[TX_BUF_SIZE];
static uint8_t wlan_rx_buf[RX_BUF_SIZE];
static uint8_t mp_regs_buf[SDIO_BLK_SIZE];
static wlan_cb_t *wlan_callback = NULL;
static wlan_core_t wlan_core;
/* 在88w8801_firmware.c中定义 */
extern const uint8_t fw_mrvl88w8801[0x3E630];

static uint8_t wlan_ass_supplicant_pmk_pkg(uint8_t *bssid);
static uint8_t wlan_ret_scan(uint8_t *rx_buf);
static uint8_t wlan_process_data(uint8_t *rx_buf);
static uint8_t wlan_process_cmdrsp(uint8_t *rx_buf);
static uint8_t wlan_process_event(uint8_t *rx_buf);
static uint8_t wlan_get_read_port(uint8_t *port);
static uint8_t wlan_get_write_port(uint8_t *port);
static uint8_t wlan_prepare_cmd(uint16_t cmd_id, uint16_t cmd_action, uint8_t *data_buf, uint16_t data_len);
static uint8_t wlan_download_fw(void);

/**
 * @param callback 回调函数表
 * @return core_err_e中某一状态码
 * @brief 初始化芯片
 */
uint8_t wlan_init(wlan_cb_t *callback) {
    wlan_callback = callback;
    /* Init core */
    memset(&wlan_core, 0, sizeof(wlan_core_t));
    /* Port 0 is reserved for command */
    wlan_core.curr_rd_port = wlan_core.curr_wr_port = 1;
    /* Init control port */
    uint8_t ctrl_port[4] = {0};
    if (sdio_cmd52(false, SDIO_FUNC_1, IO_PORT_0_REG, 0, ctrl_port) || sdio_cmd52(false, SDIO_FUNC_1, IO_PORT_1_REG, 0, ctrl_port + 1) || sdio_cmd52(false, SDIO_FUNC_1, IO_PORT_2_REG, 0, ctrl_port + 2)) return CORE_ERR_UNKNOWN_IO_PORT;
    /* Little endian */
    CORE_DEBUG("Control port: 0x%lX\n", *(uint32_t *)ctrl_port);
    wlan_core.ctrl_port = *(uint32_t *)ctrl_port;
    /* Download firmware */
    if ((*ctrl_port = wlan_download_fw())) return *ctrl_port;
    /* Enable host interrupt for SDIO and init firmware */
    return sdio_cmd52(true, SDIO_FUNC_1, HOST_INT_MASK_REG, UP_LD_HOST_INT_MASK, NULL) ? CORE_ERR_INT_MASK_FAILED : wlan_prepare_cmd(HOST_ID_FUNC_INIT, HOST_ACT_GEN_GET, NULL, 0);
}

/**
 * @return core_err_e中某一状态码
 * @brief 停止芯片运行
 */
uint8_t wlan_shutdown(void) { return wlan_prepare_cmd(HOST_ID_FUNC_SHUTDOWN, HOST_ACT_GEN_GET, NULL, 0); }

/**
 * @return core_err_e中某一状态码
 * @brief 发生SDIO中断时读取封包并解析
 */
uint8_t wlan_process_packet(void) {
    /* 读取控制寄存器并清除芯片中断 */
    if (sdio_cmd53(false, SDIO_FUNC_1, REG_PORT, 0, mp_regs_buf, MAX_MP_REGS) || sdio_cmd52(true, SDIO_FUNC_1, HOST_INT_STATUS_REG, *(mp_regs_buf + HOST_INT_STATUS_REG) & ~UP_LD_HOST_INT_STATUS, NULL)) return CORE_ERR_INT_STATUS_FAILED;
    wlan_core.read_bitmap = *(mp_regs_buf + RD_BITMAP_U) << 8 | *(mp_regs_buf + RD_BITMAP_L);
    wlan_core.write_bitmap = *(mp_regs_buf + WR_BITMAP_U) << 8 | *(mp_regs_buf + WR_BITMAP_L);
    uint8_t read_port;
    while (!wlan_get_read_port(&read_port)) {
        CORE_DEBUG("Rx: Port %d, Size %d\n", read_port, *(mp_regs_buf + RD_LEN_P0_U + (read_port << 1)) << 8 | *(mp_regs_buf + RD_LEN_P0_L + (read_port << 1)));
        /* 使用CMD53读取数据 */
        if (sdio_cmd53(false, SDIO_FUNC_1, wlan_core.ctrl_port + read_port, 0, wlan_rx_buf, *(mp_regs_buf + RD_LEN_P0_U + (read_port << 1)) << 8 | *(mp_regs_buf + RD_LEN_P0_L + (read_port << 1)))) return CORE_ERR_INVALID_RX_BUFFER;
        /* 解析从芯片发来的数据 */
        switch (*(wlan_rx_buf + 2)) {
        case TYPE_DATA: read_port = wlan_process_data(wlan_rx_buf); break;
        case TYPE_CMD_CMDRSP: read_port = wlan_process_cmdrsp(wlan_rx_buf); break;
        case TYPE_EVENT: read_port = wlan_process_event(wlan_rx_buf); break;
        default:
            CORE_DEBUG("Warning: Invalid rx 0x%X\n", *(wlan_rx_buf + 2));
            read_port = CORE_ERR_OK;
            break;
        }
        if (read_port) return read_port;
    }
    return CORE_ERR_OK;
}

/**
 * @param channel 需搜索的通道
 * @param channel_num 搜索通道个数
 * @param max_time 最大搜索时间
 * @return core_err_e中某一状态码
 * @brief 执行普通搜索
 */
uint8_t wlan_scan(uint8_t *channel, uint8_t channel_num, uint16_t max_time) {
    uint16_t scan_para_len = sizeof(MrvlIEtypesHeader_t) + channel_num * sizeof(ChanScanParamSet_t);
    uint8_t scan_para[scan_para_len];
    /* 组合通道列表 */
    MrvlIEtypes_ChanListParamSet_t *channel_list = (MrvlIEtypes_ChanListParamSet_t *)scan_para;
    channel_list->header.type = TLV_TYPE_CHANLIST;
    channel_list->header.len = channel_num * sizeof(ChanScanParamSet_t);
    ChanScanParamSet_t *channel_list_para[channel_num];
    for (uint8_t index = 0; index < channel_num; ++index) {
        *(channel_list_para + index) = (ChanScanParamSet_t *)(scan_para + sizeof(MrvlIEtypesHeader_t) + index * sizeof(ChanScanParamSet_t));
        (*(channel_list_para + index))->chan_number = *(channel + index);
        (*(channel_list_para + index))->max_scan_time = max_time;
        (*(channel_list_para + index))->radio_type = (*(channel_list_para + index))->chan_scan_mode = (*(channel_list_para + index))->min_scan_time = 0;
    }
    return wlan_prepare_cmd(HOST_ID_802_11_SCAN, HOST_ACT_GEN_GET, scan_para, scan_para_len);
}

/**
 * @param ssid AP名称
 * @param ssid_len AP名称长度
 * @param max_time 最大搜索时间
 * @return core_err_e中某一状态码
 * @brief 执行特定搜索
 */
uint8_t wlan_scan_ssid(uint8_t *ssid, uint8_t ssid_len, uint16_t max_time) {
    uint16_t scan_para_len = 2 * sizeof(MrvlIEtypesHeader_t) + ssid_len + MAX_CHANNEL_NUM * sizeof(ChanScanParamSet_t);
    uint8_t scan_para[scan_para_len];
    /* 组合SSID */
    MrvlIEtypes_SSIDParamSet_t *ssid_tlv = (MrvlIEtypes_SSIDParamSet_t *)scan_para;
    ssid_tlv->header.type = TLV_TYPE_SSID;
    ssid_tlv->header.len = ssid_len;
    memcpy(ssid_tlv->ssid, ssid, ssid_len);
    /* 组合通道列表 */
    MrvlIEtypes_ChanListParamSet_t *channel_list = (MrvlIEtypes_ChanListParamSet_t *)(scan_para + sizeof(MrvlIEtypesHeader_t) + ssid_len);
    channel_list->header.type = TLV_TYPE_CHANLIST;
    channel_list->header.len = MAX_CHANNEL_NUM * sizeof(ChanScanParamSet_t);
    ChanScanParamSet_t *channel_list_para[MAX_CHANNEL_NUM];
    for (uint8_t index = 0; index < MAX_CHANNEL_NUM; ++index) {
        *(channel_list_para + index) = (ChanScanParamSet_t *)(scan_para + 2 * sizeof(MrvlIEtypesHeader_t) + ssid_len + index * sizeof(ChanScanParamSet_t));
        (*(channel_list_para + index))->chan_number = index + 1;
        (*(channel_list_para + index))->max_scan_time = max_time;
        (*(channel_list_para + index))->radio_type = (*(channel_list_para + index))->chan_scan_mode = (*(channel_list_para + index))->min_scan_time = 0;
    }
    return wlan_prepare_cmd(HOST_ID_802_11_SCAN, HOST_ACT_GEN_GET, scan_para, scan_para_len);
}

/**
 * @param ssid AP名称
 * @param ssid_len AP名称长度
 * @param pwd AP密码
 * @param pwd_len AP密码长度
 * @return core_err_e中某一状态码
 * @brief 连接AP
 */
uint8_t wlan_sta_connect(uint8_t *ssid, uint8_t ssid_len, uint8_t *pwd, uint8_t pwd_len) {
    memcpy(wlan_core.ap_info.ssid, ssid, ssid_len);
    wlan_core.ap_info.ssid_len = ssid_len;
    memcpy(wlan_core.ap_info.pwd, pwd, pwd_len);
    wlan_core.ap_info.pwd_len = pwd_len;
    /* 执行特定搜索，状态变为连接中 */
    return (ssid_len = wlan_scan_ssid(ssid, ssid_len, MAX_SCAN_TIME)) ? ssid_len : (wlan_core.ap_info.con_status = CON_STATUS_CONNECTING, CORE_ERR_OK);
}

/**
 * @return core_err_e中某一状态码
 * @brief STA模式下主动断开
 */
uint8_t wlan_sta_disconnect(void) { return wlan_core.ap_info.con_status != CON_STATUS_CONNECTED ? CORE_ERR_OK : wlan_prepare_cmd(HOST_ID_802_11_DEAUTHENTICATE, HOST_ACT_GEN_GET, NULL, 0); }

/**
 * @param ssid AP名称
 * @param ssid_len AP名称长度
 * @param pwd AP密码
 * @param pwd_len AP密码长度
 * @param sec_type AP认证类型
 * @param broadcast_ssid SSID是否可见
 * @return core_err_e中某一状态码
 * @brief 创建AP，不带参数则创建一个名称为Marvell Micro AP且无认证类型的AP
 */
uint8_t wlan_ap_start(uint8_t *ssid, uint8_t ssid_len, uint8_t *pwd, uint8_t pwd_len, wlan_security_type sec_type, bool broadcast_ssid) {
    uint8_t sys_config[0x100], sys_config_len = 0;
    /* 组合SSID */
    if (ssid && ssid_len && ssid_len < MAX_SSID_LENGTH) {
        MrvlIEtypes_SSIDParamSet_t *ssid_tlv = (MrvlIEtypes_SSIDParamSet_t *)sys_config;
        ssid_tlv->header.type = TLV_TYPE_SSID;
        ssid_tlv->header.len = ssid_len;
        memcpy(ssid_tlv->ssid, ssid, ssid_len);
        sys_config_len += sizeof(MrvlIEtypesHeader_t) + ssid_len;
        MrvlIETypes_ApBCast_SSID_Ctrl_t *ssid_broadcast_tlv = (MrvlIETypes_ApBCast_SSID_Ctrl_t *)(sys_config + sys_config_len);
        ssid_broadcast_tlv->header.type = TLV_TYPE_UAP_BCAST_SSID_CTL;
        ssid_broadcast_tlv->header.len = 1;
        ssid_broadcast_tlv->broadcast_ssid = broadcast_ssid;
        sys_config_len += sizeof(MrvlIETypes_ApBCast_SSID_Ctrl_t);
    }
    /* 创建WPA/WPA2认证类型的AP（WEP替换为WPA） */
    if (sec_type != SECURITY_TYPE_NONE) {
        if (pwd && pwd_len && pwd_len < MAX_PHRASE_LENGTH) {
            MrvlIEtypes_PassPhrase_t *phrase_tlv = (MrvlIEtypes_PassPhrase_t *)(sys_config + sys_config_len);
            phrase_tlv->header.type = TLV_TYPE_UAP_WPA_PASSPHRASE;
            phrase_tlv->header.len = pwd_len;
            memcpy(phrase_tlv->phrase, pwd, pwd_len);
            sys_config_len += sizeof(MrvlIEtypesHeader_t) + pwd_len;
        }
        MrvlIEtypes_Enc_Protocol_t *enc_protocol_tlv = (MrvlIEtypes_Enc_Protocol_t *)(sys_config + sys_config_len);
        enc_protocol_tlv->header.type = TLV_TYPE_UAP_ENCRYPT_PROTOCOL;
        enc_protocol_tlv->header.len = 2;
        enc_protocol_tlv->enc_protocol = 1 << (sec_type != SECURITY_TYPE_WPA2 ? 3 : 5);
        sys_config_len += sizeof(MrvlIEtypes_Enc_Protocol_t);
        MrvlIEtypes_AKMP_t *akmp_tlv = (MrvlIEtypes_AKMP_t *)(sys_config + sys_config_len);
        akmp_tlv->header.type = TLV_TYPE_UAP_AKMP;
        akmp_tlv->header.len = sizeof(MrvlIEtypes_AKMP_t) - sizeof(MrvlIEtypesHeader_t);
        akmp_tlv->key_mgmt = 2;
        akmp_tlv->operation = 0;
        sys_config_len += sizeof(MrvlIEtypes_AKMP_t);
        MrvlIEtypes_PTK_cipher_t *ptk_cipher_tlv = (MrvlIEtypes_PTK_cipher_t *)(sys_config + sys_config_len);
        ptk_cipher_tlv->header.type = TLV_TYPE_PWK_CIPHER;
        ptk_cipher_tlv->header.len = sizeof(MrvlIEtypes_PTK_cipher_t) - sizeof(MrvlIEtypesHeader_t);
        ptk_cipher_tlv->protocol = 1 << (sec_type != SECURITY_TYPE_WPA2 ? 3 : 5);
        ptk_cipher_tlv->cipher = WPA_CIPHER_CCMP;
        sys_config_len += sizeof(MrvlIEtypes_PTK_cipher_t);
        MrvlIEtypes_GTK_cipher_t *gtk_cipher_tlv = (MrvlIEtypes_GTK_cipher_t *)(sys_config + sys_config_len);
        gtk_cipher_tlv->header.type = TLV_TYPE_GWK_CIPHER;
        gtk_cipher_tlv->header.len = sizeof(MrvlIEtypes_GTK_cipher_t) - sizeof(MrvlIEtypesHeader_t);
        gtk_cipher_tlv->cipher = WPA_CIPHER_CCMP;
        sys_config_len += sizeof(MrvlIEtypes_GTK_cipher_t);
    }
    return sys_config_len ? wlan_prepare_cmd(HOST_ID_APCMD_SYS_CONFIGURE, HOST_ACT_GEN_SET, sys_config, sys_config_len) : wlan_prepare_cmd(HOST_ID_APCMD_BSS_START, HOST_ACT_GEN_GET, NULL, 0);
}

/**
 * @return core_err_e中某一状态码
 * @brief 关闭AP
 */
uint8_t wlan_ap_stop(void) { return wlan_prepare_cmd(HOST_ID_APCMD_BSS_STOP, HOST_ACT_GEN_GET, NULL, 0); }

/**
 * @brief AP模式下显示接入STA索引与MAC地址
 */
void wlan_ap_show(void) { for (uint8_t index = 0; index < MAX_CLIENT_NUM; ++index) if ((wlan_core.sta_info + index)->used) CORE_DEBUG("STA MAC (Index %02d): 0x%02X:0x%02X:0x%02X:0x%02X:0x%02X:0x%02X\n", index, *((wlan_core.sta_info + index)->sta_mac_addr), *((wlan_core.sta_info + index)->sta_mac_addr + 1), *((wlan_core.sta_info + index)->sta_mac_addr + 2), *((wlan_core.sta_info + index)->sta_mac_addr + 3), *((wlan_core.sta_info + index)->sta_mac_addr + 4), *((wlan_core.sta_info + index)->sta_mac_addr + 5)); }

/**
 * @param mac_addr STA的MAC地址
 * @return core_err_e中某一状态码
 * @brief AP模式下断开某一STA
 */
uint8_t wlan_ap_deauth(uint8_t *mac_addr) { return wlan_prepare_cmd(HOST_ID_APCMD_STA_DEAUTH, HOST_ACT_GEN_GET, mac_addr, MAC_ADDR_LENGTH); }

/**
 * @param data_buf 数据缓冲区
 * @param data_len 缓冲区长度
 * @param bss_type BSS网络类型
 * @return core_err_e中某一状态码
 * @brief 发送数据
 */
uint8_t wlan_send_data(uint8_t *data_buf, uint16_t data_len, wlan_bss_type bss_type) {
    TxPD *tx_packet = (TxPD *)wlan_tx_buf;
    tx_packet->pack_len = sizeof(TxPD) + data_len;
    tx_packet->pack_type = TYPE_DATA;
    tx_packet->bss_type = bss_type;
    tx_packet->tx_pkt_length = data_len;
    tx_packet->tx_pkt_offset = sizeof(TxPD) - SDIO_HDR_SIZE;
    tx_packet->bss_num = tx_packet->tx_pkt_type = tx_packet->tx_control = tx_packet->priority = tx_packet->flags = tx_packet->pkt_delay_2ms = tx_packet->reserved1 = 0;
    if (data_buf) memcpy(tx_packet->payload, data_buf, data_len);
    uint8_t wr_bitmap[2];
    do {
        if (sdio_cmd52(false, SDIO_FUNC_1, WR_BITMAP_L, 0, wr_bitmap) || sdio_cmd52(false, SDIO_FUNC_1, WR_BITMAP_U, 0, wr_bitmap + 1)) return CORE_ERR_SEND_DATA_FAILED;
        wlan_core.write_bitmap = *(uint16_t *)wr_bitmap;
    } while (wlan_get_write_port(wr_bitmap));
    CORE_DEBUG("Tx: Port %d, Size %d\n", *wr_bitmap, tx_packet->pack_len);
    return sdio_cmd53(true, SDIO_FUNC_1, wlan_core.ctrl_port + *wr_bitmap, 0, (uint8_t *)tx_packet, tx_packet->pack_len) ? CORE_ERR_SEND_DATA_FAILED : CORE_ERR_OK;
}

/**
 * @param bssid MAC地址
 * @return core_err_e中某一状态码
 * @brief 组合HOST_ID_SUPPLICANT_PMK封包，用于连接时芯片四次握手认证
 */
static uint8_t wlan_ass_supplicant_pmk_pkg(uint8_t *bssid) {
    uint8_t pmk_tlv[0x80];
    MrvlIEtypes_SSIDParamSet_t *pmk_ssid_tlv = (MrvlIEtypes_SSIDParamSet_t *)pmk_tlv;
    pmk_ssid_tlv->header.type = TLV_TYPE_SSID;
    pmk_ssid_tlv->header.len = wlan_core.ap_info.ssid_len;
    memcpy(pmk_ssid_tlv->ssid, wlan_core.ap_info.ssid, wlan_core.ap_info.ssid_len);
    uint16_t pmk_len = sizeof(MrvlIEtypesHeader_t) + wlan_core.ap_info.ssid_len;
    MrvlIETypes_BSSIDList_t *pmk_bssid_tlv = (MrvlIETypes_BSSIDList_t *)(pmk_tlv + pmk_len);
    pmk_bssid_tlv->header.type = TLV_TYPE_BSSID;
    pmk_bssid_tlv->header.len = MAC_ADDR_LENGTH;
    memcpy(pmk_bssid_tlv->mac_addr, bssid, MAC_ADDR_LENGTH);
    pmk_len += sizeof(MrvlIETypes_BSSIDList_t);
    MrvlIEtypes_PassPhrase_t *pmk_phrase_tlv = (MrvlIEtypes_PassPhrase_t *)(pmk_tlv + pmk_len);
    pmk_phrase_tlv->header.type = TLV_TYPE_PASSPHRASE;
    pmk_phrase_tlv->header.len = wlan_core.ap_info.pwd_len;
    memcpy(pmk_phrase_tlv->phrase, wlan_core.ap_info.pwd, wlan_core.ap_info.pwd_len);
    pmk_len += sizeof(MrvlIEtypesHeader_t) + wlan_core.ap_info.pwd_len;
    return wlan_prepare_cmd(HOST_ID_SUPPLICANT_PMK, HOST_ACT_GEN_SET, pmk_tlv, pmk_len);
}

/**
 * @param rx_buf rx缓冲区
 * @return core_err_e中某一状态码
 * @brief 解析搜索命令响应，搜索的是IEEE的TLV而非Marvell的TLV
 */
static uint8_t wlan_ret_scan(uint8_t *rx_buf) {
    HOST_DS_802_11_SCAN_RSP *scan_rsp = (HOST_DS_802_11_SCAN_RSP *)rx_buf;
    CORE_DEBUG("bss_descript_size: %d\nnumber_of_sets: %d\n", scan_rsp->bss_descript_size, scan_rsp->number_of_sets);
    /* 判断搜索到的AP个数 */
    if (!scan_rsp->number_of_sets) {
        if (wlan_core.ap_info.con_status == CON_STATUS_CONNECTING) {
            CORE_DEBUG("Warning: Cannot connect to AP at this time\n");
            wlan_core.ap_info.con_status = CON_STATUS_NOT_CONNECTED;
            if (wlan_callback && wlan_callback->wlan_cb_sta_connect) wlan_callback->wlan_cb_sta_connect(CORE_ERR_UNHANDLED_STATUS);
        } else if (wlan_callback && wlan_callback->wlan_cb_scan) wlan_callback->wlan_cb_scan(CORE_ERR_OK, NULL, 0, 0, SECURITY_TYPE_NONE);
        return CORE_ERR_OK;
    }
    bss_desc_set_t *bss_desc_set = (bss_desc_set_t *)scan_rsp->bss_desc_and_tlv_buffer;
    IEEEType *rates, *ie_params, *vendor_data_ptr[8], *rsn_data_ptr = NULL;
    wlan_security_type sec_type;
    wlan_vendor *vendor;
    uint16_t ie_size;
    uint8_t ssid[MAX_SSID_LENGTH + 1], channel = 0, vendor_tlv_count = 0;
    for (uint8_t index = 0; index < scan_rsp->number_of_sets; ++index) {
        rates = NULL;
        sec_type = SECURITY_TYPE_WEP;
        ie_params = &bss_desc_set->ie_parameters;
        ie_size = bss_desc_set->ie_length > sizeof(bss_desc_set_t) + sizeof(bss_desc_set->ie_length) + sizeof(bss_desc_set->ie_parameters) ? bss_desc_set->ie_length - (sizeof(bss_desc_set_t) - sizeof(bss_desc_set->ie_length) - sizeof(bss_desc_set->ie_parameters)) : 0;
        while (ie_size) {
            /* 判断TLV */
            switch (ie_params->header.type) {
            case TLV_TYPE_SSID:
                memcpy(ssid, ie_params->data, ie_params->header.length > MAX_SSID_LENGTH ? MAX_SSID_LENGTH : ie_params->header.length);
                *(ssid + (ie_params->header.length > MAX_SSID_LENGTH ? MAX_SSID_LENGTH : ie_params->header.length)) = '\0';
                break;
            case TLV_TYPE_RATES: rates = ie_params; break;
            case TLV_TYPE_PHY_DS: channel = *ie_params->data; break;
            case TLV_TYPE_RSN_PARAMSET:
                /* 收到RSN即为WPA2 */
                sec_type = SECURITY_TYPE_WPA2;
                rsn_data_ptr = ie_params;
                break;
            case TLV_TYPE_VENDOR_SPECIFIC_IE:
                if (sec_type != SECURITY_TYPE_WPA2) {
                    vendor = (wlan_vendor *)ie_params->data;
                    if (!*vendor->oui && *(vendor->oui + 1) == 0x50 && *(vendor->oui + 2) == 0xF2 && vendor->oui_type == 0x1) sec_type = SECURITY_TYPE_WPA;
                }
                if (vendor_tlv_count < 8) {
                    *(vendor_data_ptr + vendor_tlv_count) = ie_params;
                    ++vendor_tlv_count;
                }
                break;
            }
            ie_size -= TLV_STRUCTLEN(ie_params);
            ie_params = (IEEEType *)TLV_NEXT(ie_params);
        }
        if (!(bss_desc_set->cap_info & WLAN_CAPABILITY_PRIVACY)) sec_type = SECURITY_TYPE_NONE;
        /* SSID名称 */
        CORE_DEBUG("SSID '%s', ", ssid);
        /* MAC地址 */
        CORE_DEBUG("MAC %02X:%02X:%02X:%02X:%02X:%02X, ", *bss_desc_set->bssid, *(bss_desc_set->bssid + 1), *(bss_desc_set->bssid + 2), *(bss_desc_set->bssid + 3), *(bss_desc_set->bssid + 4), *(bss_desc_set->bssid + 5));
        /* 信号强度及通道 */
        CORE_DEBUG("RSSI %d, Channel %d\nCapability: 0x%04X (Security: ", bss_desc_set->rssi, channel, bss_desc_set->cap_info);
        switch (wlan_core.ap_info.sec_type = sec_type) {
        case SECURITY_TYPE_NONE: CORE_DEBUG("%s", "OPEN"); break;
        case SECURITY_TYPE_WEP: CORE_DEBUG("%s", "WEP"); break;
        case SECURITY_TYPE_WPA: CORE_DEBUG("%s", "WPA"); break;
        case SECURITY_TYPE_WPA2: CORE_DEBUG("%s", "WPA2"); break;
        }
        CORE_DEBUG(", Mode: %s)\n", bss_desc_set->cap_info & WLAN_CAPABILITY_IBSS ? "Ad-Hoc" : "Infrastructure");
        if (rates) {
            CORE_DEBUG("Rates:");
            for (uint8_t index = 0; index < rates->header.length; ++index) CORE_DEBUG(" %d Mbps", (*(rates->data + index) & 0x7F) >> 1);
            CORE_DEBUG("\n");
        }
        if (wlan_core.ap_info.con_status == CON_STATUS_CONNECTING) {
            /* 连接准备 */
            uint8_t associate_para[0x200];
            MrvlIEtypes_SSIDParamSet_t *ssid_tlv = (MrvlIEtypes_SSIDParamSet_t *)associate_para;
            ssid_tlv->header.type = TLV_TYPE_SSID;
            ssid_tlv->header.len = wlan_core.ap_info.ssid_len;
            memcpy(ssid_tlv->ssid, ssid, wlan_core.ap_info.ssid_len);
            uint16_t associate_para_len = sizeof(MrvlIEtypesHeader_t) + wlan_core.ap_info.ssid_len;
            MrvlIETypes_PhyParamDSSet_t *phy_tlv = (MrvlIETypes_PhyParamDSSet_t *)(associate_para + associate_para_len);
            phy_tlv->header.type = TLV_TYPE_PHY_DS;
            phy_tlv->header.len = 1;
            phy_tlv->channel = channel;
            associate_para_len += sizeof(MrvlIETypes_PhyParamDSSet_t);
            MrvlIETypes_CfParamSet_t *cf_tlv = (MrvlIETypes_CfParamSet_t *)(associate_para + associate_para_len);
            memset(cf_tlv, 0, sizeof(MrvlIETypes_CfParamSet_t));
            cf_tlv->header.type = TLV_TYPE_CF;
            cf_tlv->header.len = sizeof(MrvlIETypes_CfParamSet_t) - sizeof(MrvlIEtypesHeader_t);
            associate_para_len += sizeof(MrvlIETypes_CfParamSet_t);
            if (sec_type == SECURITY_TYPE_NONE) {
                MrvlIETypes_AuthType_t *auth_tlv = (MrvlIETypes_AuthType_t *)(associate_para + associate_para_len);
                auth_tlv->header.type = TLV_TYPE_AUTH_TYPE;
                auth_tlv->header.len = sizeof(MrvlIETypes_AuthType_t) - sizeof(MrvlIEtypesHeader_t);
                auth_tlv->auth_type = AUTH_TYPE_OPEN;
                associate_para_len += sizeof(MrvlIETypes_AuthType_t);
            }
            MrvlIEtypes_ChanListParamSet_t *channel_list = (MrvlIEtypes_ChanListParamSet_t *)(associate_para + associate_para_len);
            ChanScanParamSet_t *channel_list_para = (ChanScanParamSet_t *)(associate_para + associate_para_len + sizeof(MrvlIEtypesHeader_t));
            channel_list->header.type = TLV_TYPE_CHANLIST;
            channel_list->header.len = sizeof(ChanScanParamSet_t);
            channel_list_para->chan_number = channel;
            channel_list_para->max_scan_time = MAX_SCAN_TIME;
            channel_list_para->radio_type = channel_list_para->chan_scan_mode = channel_list_para->min_scan_time = 0;
            associate_para_len += sizeof(MrvlIEtypes_ChanListParamSet_t);
            uint8_t rate_tlv[] = {0x01, 0x00, 0x0C, 0x00, 0x82, 0x84, 0x8B, 0x8C, 0x12, 0x96, 0x98, 0x24, 0xB0, 0x48, 0x60, 0x6C};
            memcpy(associate_para + associate_para_len, rate_tlv, sizeof(rate_tlv));
            associate_para_len += sizeof(rate_tlv);
            if (sec_type >= SECURITY_TYPE_WPA) {
                if ((*rate_tlv = wlan_ass_supplicant_pmk_pkg(bss_desc_set->bssid))) {
                    wlan_core.ap_info.con_status = CON_STATUS_NOT_CONNECTED;
                    if (wlan_callback && wlan_callback->wlan_cb_sta_connect) wlan_callback->wlan_cb_sta_connect(CORE_ERR_UNHANDLED_STATUS);
                    return *rate_tlv;
                }
                MrvlIETypes_Vendor_t *vendor_tlv;
                for (uint8_t index = 0; index < vendor_tlv_count; ++index) {
                    vendor_tlv = (MrvlIETypes_Vendor_t *)(associate_para + associate_para_len);
                    vendor_tlv->header.type = TLV_TYPE_VENDOR_SPECIFIC_IE;
                    vendor_tlv->header.len = (*(vendor_data_ptr + index))->header.length;
                    memcpy(vendor_tlv->vendor, (*(vendor_data_ptr + index))->data, vendor_tlv->header.len);
                    associate_para_len += sizeof(MrvlIEtypesHeader_t) + vendor_tlv->header.len;
                }
                if (sec_type == SECURITY_TYPE_WPA2) {
                    MrvlIETypes_RSN_t *rsn_tlv = (MrvlIETypes_RSN_t *)(associate_para + associate_para_len);
                    rsn_tlv->header.type = TLV_TYPE_RSN_PARAMSET;
                    rsn_tlv->header.len = rsn_data_ptr->header.length;
                    memcpy(rsn_tlv->rsn, rsn_data_ptr->data, rsn_tlv->header.len);
                    associate_para_len += sizeof(MrvlIEtypesHeader_t) + rsn_tlv->header.len;
                }
            }
            memcpy(wlan_core.ap_info.ap_mac_addr, bss_desc_set->bssid, MAC_ADDR_LENGTH);
            wlan_core.ap_info.cap_info = bss_desc_set->cap_info;
            if ((*rate_tlv = wlan_prepare_cmd(HOST_ID_802_11_ASSOCIATE, HOST_ACT_GEN_GET, associate_para, associate_para_len))) {
                wlan_core.ap_info.con_status = CON_STATUS_NOT_CONNECTED;
                if (wlan_callback && wlan_callback->wlan_cb_sta_connect) wlan_callback->wlan_cb_sta_connect(CORE_ERR_UNHANDLED_STATUS);
                return *rate_tlv;
            }
        } else if (wlan_callback && wlan_callback->wlan_cb_scan) wlan_callback->wlan_cb_scan(CORE_ERR_UNHANDLED_STATUS, ssid, bss_desc_set->rssi, channel, sec_type);
        /* 解析下一个AP */
        bss_desc_set = (bss_desc_set_t *)((uint8_t *)bss_desc_set + sizeof(bss_desc_set->ie_length) + bss_desc_set->ie_length);
    }
    if (wlan_core.ap_info.con_status != CON_STATUS_CONNECTING && wlan_callback && wlan_callback->wlan_cb_scan) wlan_callback->wlan_cb_scan(CORE_ERR_OK, NULL, 0, 0, SECURITY_TYPE_NONE);
    return CORE_ERR_OK;
}

/**
 * @param rx_buf rx缓冲区
 * @return core_err_e中某一状态码
 * @brief 处理数据
 */
static uint8_t wlan_process_data(uint8_t *rx_buf) {
    switch (*(rx_buf + SDIO_HDR_SIZE)) {
    case BSS_TYPE_STA: ethernetif_data_input(rx_buf, BSS_TYPE_STA); break;
    case BSS_TYPE_UAP: {
        uint8_t *payload = rx_buf + ((RxPD *)rx_buf)->rx_pkt_offset + SDIO_HDR_SIZE;
        /* 多播封包，转发并由lwIP处理 */
        if (*payload & 1) {
            if ((*payload = wlan_send_data(payload, ((RxPD *)rx_buf)->rx_pkt_length, BSS_TYPE_UAP))) return *payload;
            ethernetif_data_input(rx_buf, BSS_TYPE_UAP);
        }
        /* 单播封包地址不同，转发 */
        else if (memcmp(payload, wlan_core.mac_addr, MAC_ADDR_LENGTH)) return wlan_send_data(payload, ((RxPD *)rx_buf)->rx_pkt_length, BSS_TYPE_UAP);
        /* 单播封包地址相同，由lwIP处理 */
        else ethernetif_data_input(rx_buf, BSS_TYPE_UAP);
        break;
    }
    }
    return CORE_ERR_OK;
}

/**
 * @param rx_buf rx缓冲区
 * @return core_err_e中某一状态码
 * @brief 处理命令响应
 */
static uint8_t wlan_process_cmdrsp(uint8_t *rx_buf) {
    if (((HOST_DS_COMMAND *)rx_buf)->result != HOST_RESULT_OK) return CORE_ERR_INVALID_CMD_RESPONSE;
    switch (((HOST_DS_COMMAND *)rx_buf)->command & ~HOST_RET_BIT) {
    case HOST_ID_GET_HW_SPEC:
        wlan_core.mp_end_port = ((HOST_DS_GET_HW_SPEC *)(rx_buf + CMD_HDR_SIZE))->mp_end_port;
        return wlan_prepare_cmd(HOST_ID_802_11_MAC_ADDR, HOST_ACT_GEN_GET, NULL, 0);
    case HOST_ID_802_11_SCAN: return wlan_ret_scan(rx_buf + CMD_HDR_SIZE);
    case HOST_ID_802_11_ASSOCIATE:
        switch (((HOST_DS_802_11_ASSOCIATE_RSP *)(rx_buf + CMD_HDR_SIZE))->assoc_rsp.Capability) {
        /**
         * 0xFFFC: Connection timeout
         * 0xFFFD: Authentication refused
         * 0xFFFE: Authentication unhandled message
         * 0xFFFF: Internal error
         */
        case 0xFFFC:
        case 0xFFFD:
        case 0xFFFE:
        case 0xFFFF:
            CORE_DEBUG("Error: Association 0x%X\n", ((HOST_DS_802_11_ASSOCIATE_RSP *)(rx_buf + CMD_HDR_SIZE))->assoc_rsp.Capability);
            wlan_core.ap_info.con_status = CON_STATUS_NOT_CONNECTED;
            if (wlan_callback && wlan_callback->wlan_cb_sta_connect) wlan_callback->wlan_cb_sta_connect(CORE_ERR_UNHANDLED_STATUS);
            break;
        default: CORE_DEBUG("Capability 0x%X\n", ((HOST_DS_802_11_ASSOCIATE_RSP *)(rx_buf + CMD_HDR_SIZE))->assoc_rsp.Capability); break;
        }
        break;
    case HOST_ID_MAC_CONTROL: return wlan_prepare_cmd(HOST_ID_GET_HW_SPEC, HOST_ACT_GEN_GET, NULL, 0);
    case HOST_ID_802_11_MAC_ADDR:
        memcpy(wlan_core.mac_addr, ((HOST_DS_802_11_MAC_ADDR *)(rx_buf + CMD_HDR_SIZE))->mac_addr, MAC_ADDR_LENGTH);
        ethernetif_netif_init(wlan_core.mac_addr);
        if (wlan_callback && wlan_callback->wlan_cb_init) wlan_callback->wlan_cb_init(CORE_ERR_OK);
        break;
    case HOST_ID_FUNC_INIT: return wlan_prepare_cmd(HOST_ID_MAC_CONTROL, HOST_ACT_GEN_GET, NULL, HOST_ACT_MAC_RX_ON | HOST_ACT_MAC_TX_ON | HOST_ACT_MAC_ETHERNETII_ENABLE);
    case HOST_ID_FUNC_SHUTDOWN:
        if (wlan_callback && wlan_callback->wlan_cb_init) wlan_callback->wlan_cb_init(CORE_ERR_UNHANDLED_STATUS);
        break;
    case HOST_ID_APCMD_SYS_CONFIGURE: return wlan_prepare_cmd(HOST_ID_APCMD_BSS_START, HOST_ACT_GEN_GET, NULL, 0);
    case HOST_ID_APCMD_BSS_STOP:
        ethernetif_link_down(BSS_TYPE_UAP);
        if (wlan_callback && wlan_callback->wlan_cb_ap_stop) wlan_callback->wlan_cb_ap_stop();
        break;
    case HOST_ID_SUPPLICANT_PMK: return sdio_cmd53(true, SDIO_FUNC_1, wlan_core.ctrl_port + CTRL_PORT, 0, wlan_tx_buf, *(wlan_tx_buf + 1) << 8 | *wlan_tx_buf) ? CORE_ERR_INVALID_CMD_RESPONSE : CORE_ERR_OK;
    case HOST_ID_802_11_DEAUTHENTICATE:
    case HOST_ID_APCMD_BSS_START:
    case HOST_ID_11N_ADDBA_RSP: break;
    default: CORE_DEBUG("Warning: Invalid cmd response 0x%X\n", ((HOST_DS_COMMAND *)rx_buf)->command & ~HOST_RET_BIT); break;
    }
    return CORE_ERR_OK;
}

/**
 * @param rx_buf rx缓冲区
 * @return core_err_e中某一状态码
 * @brief 处理事件
 */
static uint8_t wlan_process_event(uint8_t *rx_buf) {
    switch (*(uint16_t *)(rx_buf + SDIO_HDR_SIZE)) {
    case EVENT_DEAUTHENTICATED:
        /* STA模式下，被AP断开或AP关闭 */
        CORE_DEBUG("EVENT_DEAUTHENTICATED\n");
        wlan_core.ap_info.con_status = CON_STATUS_NOT_CONNECTED;
        ethernetif_link_down(BSS_TYPE_STA);
        if (wlan_callback && wlan_callback->wlan_cb_sta_disconnect) wlan_callback->wlan_cb_sta_disconnect();
        break;
    /* WMM参数内AP改变或AC队列运行状态改变 */
    case EVENT_WMM_STATUS_CHANGE: CORE_DEBUG("EVENT_WMM_STATUS_CHANGE\n"); break;
    case EVENT_PORT_RELEASE:
        /* STA模式下，成功与AP建立连接 */
        CORE_DEBUG("EVENT_PORT_RELEASE\n");
        wlan_core.ap_info.con_status = CON_STATUS_CONNECTED;
        ethernetif_link_up(BSS_TYPE_STA, NULL);
        if (wlan_callback && wlan_callback->wlan_cb_sta_connect) wlan_callback->wlan_cb_sta_connect(CORE_ERR_OK);
        break;
    case EVENT_MICRO_AP_STA_DEAUTH:
        /* AP模式下，断开某一STA或STA主动断开 */
        CORE_DEBUG("EVENT_MICRO_AP_STA_DEAUTH\n");
        for (uint8_t index = 0; index < MAX_CLIENT_NUM; ++index) {
            if (memcmp((wlan_core.sta_info + index)->sta_mac_addr, rx_buf + EVENT_HDR_SIZE + 2, MAC_ADDR_LENGTH)) continue;
            memset((wlan_core.sta_info + index)->sta_mac_addr, 0, MAC_ADDR_LENGTH);
            (wlan_core.sta_info + index)->used = 0;
            ethernetif_dhcpd_erase(rx_buf + EVENT_HDR_SIZE + 2, wlan_callback ? wlan_callback->wlan_cb_ap_disconnect : NULL);
            return CORE_ERR_OK;
        }
        /* 对应STA信息不存在 */
        break;
    case EVENT_MICRO_AP_STA_ASSOC:
        /* AP模式下，某一STA接入 */
        CORE_DEBUG("EVENT_MICRO_AP_STA_ASSOC\n");
        for (uint8_t index = 0; index < MAX_CLIENT_NUM; ++index) {
            if ((wlan_core.sta_info + index)->used) continue;
            memcpy((wlan_core.sta_info + index)->sta_mac_addr, rx_buf + EVENT_HDR_SIZE + 2, MAC_ADDR_LENGTH);
            (wlan_core.sta_info + index)->used = 1;
            return CORE_ERR_OK;
        }
        /* 无空闲STA信息节点 */
        return wlan_ap_deauth(rx_buf + EVENT_HDR_SIZE + 2);
    case EVENT_MICRO_AP_BSS_START:
        /* AP模式开启 */
        CORE_DEBUG("EVENT_MICRO_AP_BSS_START\n");
        if (wlan_callback) {
            ethernetif_link_up(BSS_TYPE_UAP, wlan_callback->wlan_cb_ap_connect);
            if (wlan_callback->wlan_cb_ap_start) wlan_callback->wlan_cb_ap_start();
        } else ethernetif_link_up(BSS_TYPE_UAP, NULL);
        break;
    /* 收到ADDBA请求 */
    case EVENT_ADDBA: return wlan_prepare_cmd(HOST_ID_11N_ADDBA_RSP, HOST_ACT_GEN_GET, rx_buf + EVENT_HDR_SIZE, 0);
    /* 收到DELBA请求 */
    case EVENT_DELBA: CORE_DEBUG("EVENT_DELBA\n"); break;
    /* AP模式空闲中 */
    case EVENT_MICRO_AP_BSS_IDLE: CORE_DEBUG("EVENT_MICRO_AP_BSS_IDLE\n"); break;
    /* AP模式连接中 */
    case EVENT_MICRO_AP_BSS_ACTIVE: CORE_DEBUG("EVENT_MICRO_AP_BSS_ACTIVE\n"); break;
    /* WPA/WPA2 AP模式下，STA完成四次握手 */
    case EVENT_MICRO_AP_EV_RSN_CONNECT: CORE_DEBUG("EVENT_MICRO_AP_EV_RSN_CONNECT\n"); break;
    default: CORE_DEBUG("Warning: Unknown event ID 0x%X\n", *(uint16_t *)(rx_buf + SDIO_HDR_SIZE)); break;
    }
    return CORE_ERR_OK;
}

/**
 * @param port 读取端口
 * @return core_err_e中某一状态码
 * @brief 获取读取端口
 */
static uint8_t wlan_get_read_port(uint8_t *port) {
    if (wlan_core.read_bitmap & CTRL_PORT_MASK) {
        wlan_core.read_bitmap &= ~CTRL_PORT_MASK;
        *port = CTRL_PORT;
    } else if (wlan_core.read_bitmap & 1 << wlan_core.curr_rd_port) {
        wlan_core.read_bitmap &= ~(1 << wlan_core.curr_rd_port);
        *port = wlan_core.curr_rd_port;
        if (++wlan_core.curr_rd_port == MAX_PORT) wlan_core.curr_rd_port = 1;
    } else return CORE_ERR_END_OF_READ_PORT;
    return CORE_ERR_OK;
}

/**
 * @param port 写入端口
 * @return core_err_e中某一状态码
 * @brief 获取写入端口
 */
static uint8_t wlan_get_write_port(uint8_t *port) {
    if (wlan_core.write_bitmap & 1 << wlan_core.curr_wr_port) {
        wlan_core.write_bitmap &= ~(1 << wlan_core.curr_wr_port);
        *port = wlan_core.curr_wr_port;
        if (++wlan_core.curr_wr_port == wlan_core.mp_end_port) wlan_core.curr_wr_port = 1;
    } else return CORE_ERR_OCCUPIED_WRITE_PORT;
    return CORE_ERR_OK;
}

/**
 * @param cmd_id 命令ID
 * @param cmd_action 命令动作
 * @param data_buf 命令缓冲区
 * @param data_len 缓冲区长度
 * @return core_err_e中某一状态码
 * @brief 组封包
 */
static uint8_t wlan_prepare_cmd(uint16_t cmd_id, uint16_t cmd_action, uint8_t *data_buf, uint16_t data_len) {
    HOST_DS_COMMAND *cmd = (HOST_DS_COMMAND *)wlan_tx_buf;
    cmd->pack_type = TYPE_CMD_CMDRSP;
    cmd->seq_num = cmd->result = 0;
    switch (cmd->command = cmd_id) {
    case HOST_ID_GET_HW_SPEC:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_GET_HW_SPEC)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        break;
    case HOST_ID_802_11_SCAN:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_SCAN) + data_len - sizeof(cmd->params.scan.tlv_buffer)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        cmd->params.scan.bss_mode = HOST_BSS_MODE_ANY;
        memset(cmd->params.scan.bssid, 0, MAC_ADDR_LENGTH);
        memcpy(cmd->params.scan.tlv_buffer, data_buf, data_len);
        break;
    case HOST_ID_802_11_ASSOCIATE:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_ASSOCIATE) + data_len - sizeof(cmd->params.associate.tlv_buffer)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        memcpy(cmd->params.associate.peer_sta_addr, wlan_core.ap_info.ap_mac_addr, MAC_ADDR_LENGTH);
        cmd->params.associate.cap_info = wlan_core.ap_info.cap_info;
        cmd->params.associate.listen_interval = 0xA;
        cmd->params.associate.beacon_period = 0x40;
        cmd->params.associate.dtim_period = 0x0;
        memcpy(cmd->params.associate.tlv_buffer, data_buf, data_len);
        break;
    case HOST_ID_802_11_DEAUTHENTICATE:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_DEAUTHENTICATE)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        memcpy(cmd->params.deauth.mac_addr, wlan_core.ap_info.ap_mac_addr, MAC_ADDR_LENGTH);
        cmd->params.deauth.reason_code = STA_LEAVING;
        break;
    case HOST_ID_MAC_CONTROL:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_MAC_CONTROL)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        cmd->params.mac_ctrl.action = data_len;
        break;
    case HOST_ID_802_11_MAC_ADDR:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_MAC_ADDR) + data_len) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        if ((cmd->params.mac_addr.action = cmd_action) != HOST_ACT_GEN_GET) memcpy(cmd->params.mac_addr.mac_addr, data_buf, data_len);
        else memset(cmd->params.mac_addr.mac_addr, 0, MAC_ADDR_LENGTH);
        break;
    case HOST_ID_FUNC_INIT:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        break;
    case HOST_ID_FUNC_SHUTDOWN:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        break;
    case HOST_ID_APCMD_SYS_CONFIGURE:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + 2 + data_len) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_UAP << 4;
        cmd->params.sys_config.action = cmd_action;
        memcpy(cmd->params.sys_config.tlv_buffer, data_buf, data_len);
        break;
    case HOST_ID_APCMD_BSS_START:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_UAP << 4;
        break;
    case HOST_ID_APCMD_BSS_STOP:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_UAP << 4;
        break;
    case HOST_ID_APCMD_STA_DEAUTH:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_DEAUTHENTICATE)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_UAP << 4;
        memcpy(cmd->params.deauth.mac_addr, data_buf, data_len);
        cmd->params.deauth.reason_code = LEAVING_NETWORK_DEAUTH;
        break;
    case HOST_ID_SUPPLICANT_PMK:
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_802_11_SUPPLICANT_PMK) + data_len - sizeof(cmd->params.esupplicant_psk.tlv_buffer)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        cmd->params.esupplicant_psk.action = cmd_action;
        cmd->params.esupplicant_psk.cache_result = 0;
        memcpy(cmd->params.esupplicant_psk.tlv_buffer, data_buf, data_len);
        break;
    case HOST_ID_11N_ADDBA_RSP: {
        cmd->size = (cmd->pack_len = CMD_HDR_SIZE + sizeof(HOST_DS_11N_ADDBA_RSP)) - SDIO_HDR_SIZE;
        cmd->bss = BSS_TYPE_STA << 4;
        cmd->params.add_ba_rsp.add_rsp_result = cmd->params.add_ba_rsp.status_code = 0;
        HOST_DS_11N_ADDBA_REQ *add_ba_req = (HOST_DS_11N_ADDBA_REQ *)data_buf;
        memcpy(cmd->params.add_ba_rsp.peer_mac_addr, add_ba_req->peer_mac_addr, MAC_ADDR_LENGTH);
        cmd->params.add_ba_rsp.dialog_token = add_ba_req->dialog_token;
        cmd->params.add_ba_rsp.block_ack_param_set = add_ba_req->block_ack_param_set;
        cmd->params.add_ba_rsp.block_ack_tmo = add_ba_req->block_ack_tmo;
        cmd->params.add_ba_rsp.ssn = add_ba_req->ssn;
        break;
    }
    }
    return wlan_core.ap_info.sec_type < SECURITY_TYPE_WPA || cmd_id != HOST_ID_802_11_ASSOCIATE ? sdio_cmd53(true, SDIO_FUNC_1, wlan_core.ctrl_port + CTRL_PORT, 0, wlan_tx_buf, *(wlan_tx_buf + 1) << 8 | *wlan_tx_buf) : CORE_ERR_OK;
}

/**
 * @return core_err_e中某一状态码
 * @brief 将固件下载到芯片
 */
static uint8_t wlan_download_fw(void) {
#ifndef USE_FLASH_FIRMWARE
    const uint8_t *fw_data = fw_mrvl88w8801;
#else
    /* 16-1024 bytes */
    uint8_t *fw_data = wlan_tx_buf;
#endif
    uint8_t fw_next[2];
    uint16_t fw_next_16;
    uint32_t fw_len = sizeof(fw_mrvl88w8801);
    while (fw_len) {
        *fw_next = *(fw_next + 1) = 0;
        if (fw_len != sizeof(fw_mrvl88w8801)) while (!(*fw_next & CARD_IO_READY && *fw_next & DN_LD_CARD_RDY)) if (sdio_cmd52(false, SDIO_FUNC_1, CARD_TO_HOST_EVENT_REG, 0, fw_next)) return CORE_ERR_FIRMWARE_FAILED;
        /* Get next block length */
        if (sdio_cmd52(false, SDIO_FUNC_1, READ_BASE_0_REG, 0, fw_next) || sdio_cmd52(false, SDIO_FUNC_1, READ_BASE_1_REG, 0, fw_next + 1)) return CORE_ERR_FIRMWARE_FAILED;
        /* Little endian */
        if (!(fw_next_16 = *(uint16_t *)fw_next)) continue;
        CORE_DEBUG("Required: %d bytes, remaining: %ld bytes\n", fw_next_16, fw_len);
        if (fw_next_16 & 1) {
            /* CRC failed */
            CORE_DEBUG("Error: Odd size is invalid\n");
            return CORE_ERR_FIRMWARE_FAILED;
        }
        if (fw_next_16 > fw_len) fw_next_16 = fw_len;
#ifdef USE_FLASH_FIRMWARE
        flashReadMemory(FLASH_FIRMWARE_ADDRESS + sizeof(fw_mrvl88w8801) - fw_len, fw_data, fw_next_16);
#endif
        /* Write block */
        if (sdio_cmd53(true, SDIO_FUNC_1, wlan_core.ctrl_port, 0, (uint8_t *)fw_data, fw_next_16)) return CORE_ERR_FIRMWARE_FAILED;
        fw_len -= fw_next_16;
#ifndef USE_FLASH_FIRMWARE
        fw_data += fw_next_16;
#endif
    }
    /* Check firmware status */
    for (fw_next_16 = 0; fw_next_16 < MAX_POLL_TRIES; ++fw_next_16) {
        if (sdio_cmd52(false, SDIO_FUNC_1, CARD_FW_STATUS0_REG, 0, fw_next) || sdio_cmd52(false, SDIO_FUNC_1, CARD_FW_STATUS1_REG, 0, fw_next + 1)) break;
        if (*(uint16_t *)fw_next == FIRMWARE_READY) {
            CORE_DEBUG("Firmware is active (Index %d)\n", fw_next_16);
            return CORE_ERR_OK;
        }
    }
    return CORE_ERR_FIRMWARE_FAILED;
}
