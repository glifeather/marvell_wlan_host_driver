#ifndef _88W8801_CORE_
#define _88W8801_CORE_
#include <stdint.h>
#include <stdbool.h>
#include "88w8801/88w8801.h"

typedef enum {
    CORE_ERR_OK,
    CORE_ERR_UNHANDLED_STATUS,
    CORE_ERR_UNKNOWN_IO_PORT,
    CORE_ERR_FIRMWARE_FAILED,
    CORE_ERR_INT_MASK_FAILED,
    CORE_ERR_INT_STATUS_FAILED,
    CORE_ERR_END_OF_READ_PORT,
    CORE_ERR_OCCUPIED_WRITE_PORT,
    CORE_ERR_SEND_DATA_FAILED,
    CORE_ERR_INVALID_CMD_RESPONSE,
    CORE_ERR_INVALID_RX_BUFFER
} core_err_e;

#define MAC_ADDR_LENGTH 6
#define MAX_POLL_TRIES 50000
#define MAX_MP_REGS 64
#define MAX_PORT 16
#define MAX_SSID_LENGTH 32
#define MAX_PHRASE_LENGTH 64
#define MAX_CHANNEL_NUM 14
#define MAX_SCAN_TIME 200

#define TX_BUF_SIZE 0x800
#define RX_BUF_SIZE 0x800

#define TYPE_DATA 0x0
#define TYPE_CMD_CMDRSP 0x1
#define TYPE_EVENT 0x3

#define SDIO_HDR_SIZE 4
#define CMD_HDR_SIZE (SDIO_HDR_SIZE + 8)
#define EVENT_HDR_SIZE (SDIO_HDR_SIZE + 4)

/* Reason code */
/* Deauthenticated because sending STA is leaving/has left IBSS/ESS */
#define LEAVING_NETWORK_DEAUTH 3
/* Requesting STA is leaving/resetting BSS */
#define STA_LEAVING 36

/* Host control registers */
/* Host control registers: Host interrupt mask */
#define HOST_INT_MASK_REG 0x2
/* Host control registers: Upload host interrupt mask */
#define UP_LD_HOST_INT_MASK 0x1
/* Host control registers: Download host interrupt mask */
// #define DN_LD_HOST_INT_MASK 0x2
/* Enable host interrupt mask */
// #define HIM_ENABLE (UP_LD_HOST_INT_MASK | DN_LD_HOST_INT_MASK)
/* Disable host interrupt mask */
// #define HIM_DISABLE 0xFF

/* Host control registers: Host interrupt status */
#define HOST_INT_STATUS_REG 0x3
/* Host control registers: Upload host interrupt status */
#define UP_LD_HOST_INT_STATUS 0x1
/* Host control registers: Download host interrupt status */
// #define DN_LD_HOST_INT_STATUS 0x2
/* Host control registers: Host interrupt status bit */
// #define HOST_INT_STATUS_BIT (UP_LD_HOST_INT_STATUS | DN_LD_HOST_INT_STATUS)

/* Port for registers */
#define REG_PORT 0x0
/* LSB of read bitmap */
#define RD_BITMAP_L 0x4
/* MSB of read bitmap */
#define RD_BITMAP_U 0x5
/* LSB of write bitmap */
#define WR_BITMAP_L 0x6
/* MSB of write bitmap */
#define WR_BITMAP_U 0x7
/* LSB of read length for port 0 */
#define RD_LEN_P0_L 0x8
/* MSB of read length for port 0 */
#define RD_LEN_P0_U 0x9
/* Ctrl port */
#define CTRL_PORT 0x0
/* Ctrl port mask */
#define CTRL_PORT_MASK 0x1
/* Data port mask */
// #define DATA_PORT_MASK 0xFFFE

/* Card control registers */
/* Card control registers: Card to host event */
#define CARD_TO_HOST_EVENT_REG 0x30
/* Card control registers: Card I/O ready */
#define CARD_IO_READY 0x8
/* Card control registers: CIS card ready */
// #define CIS_CARD_RDY 0x4
/* Card control registers: Upload card ready */
// #define UP_LD_CARD_RDY 0x2
/* Card control registers: Download card ready */
#define DN_LD_CARD_RDY 0x1

/* Card control registers: SQ Read base address 0 register */
#define READ_BASE_0_REG 0x40
/* Card control registers: SQ Read base address 1 register */
#define READ_BASE_1_REG 0x41

/* Firmware status 0 register (SCRATCH0_0) */
#define CARD_FW_STATUS0_REG 0x60
/* Firmware status 1 register (SCRATCH0_1) */
#define CARD_FW_STATUS1_REG 0x61
/* Firmware ready */
#define FIRMWARE_READY 0xFEDC
/* Rx length register (SCRATCH0_2) */
// #define CARD_RX_LEN_REG 0x62
/* Rx unit register (SCRATCH0_3) */
// #define CARD_RX_UNIT_REG 0x63

/* Host control registers: I/O port 0 */
#define IO_PORT_0_REG 0x78
/* Host control registers: I/O port 1 */
#define IO_PORT_1_REG 0x79
/* Host control registers: I/O port 2 */
#define IO_PORT_2_REG 0x7A

/* TLV type ID definition */
#define PROPRIETARY_TLV_BASE_ID 0x100

/* TLV type: SSID */
#define TLV_TYPE_SSID 0x0
/* TLV type: Rates */
#define TLV_TYPE_RATES 0x1
/* TLV type: PHY FH */
// #define TLV_TYPE_PHY_FH 0x2
/* TLV type: PHY DS */
#define TLV_TYPE_PHY_DS 0x3
/* TLV type: CF */
#define TLV_TYPE_CF 0x4
/* TLV type: IBSS */
// #define TLV_TYPE_IBSS 0x6
/* TLV type: Domain */
// #define TLV_TYPE_DOMAIN 0x7
/* TLV type: Power constraint */
// #define TLV_TYPE_POWER_CONSTRAINT 0x20
/* TLV type: Power capability */
// #define TLV_TYPE_POWER_CAPABILITY 0x21
/* TLV type: TLV_TYPE_RSN_PARAMSET */
#define TLV_TYPE_RSN_PARAMSET 0x30
/* TLV type: Vendor Specific IE */
#define TLV_TYPE_VENDOR_SPECIFIC_IE 0xDD

/* TLV type: Key material */
// #define TLV_TYPE_KEY_MATERIAL (PROPRIETARY_TLV_BASE_ID + 0x0) // 0x100
/* TLV type: Channel list */
#define TLV_TYPE_CHANLIST (PROPRIETARY_TLV_BASE_ID + 0x1) // 0x101
/* TLV type: Number of probes */
// #define TLV_TYPE_NUMPROBES (PROPRIETARY_TLV_BASE_ID + 0x2) // 0x102
/* TLV type: Beacon RSSI low */
// #define TLV_TYPE_RSSI_LOW (PROPRIETARY_TLV_BASE_ID + 0x4) // 0x104
/* TLV type: Beacon SNR low */
// #define TLV_TYPE_SNR_LOW (PROPRIETARY_TLV_BASE_ID + 0x5) // 0x105
/* TLV type: Fail count */
// #define TLV_TYPE_FAILCOUNT (PROPRIETARY_TLV_BASE_ID + 0x6) // 0x106
/* TLV type: BCN miss */
// #define TLV_TYPE_BCNMISS (PROPRIETARY_TLV_BASE_ID + 0x7) // 0x107
/* TLV type: LED behavior */
// #define TLV_TYPE_LEDBEHAVIOR (PROPRIETARY_TLV_BASE_ID + 0x9) // 0x109
/* TLV type: Passthrough */
// #define TLV_TYPE_PASSTHROUGH (PROPRIETARY_TLV_BASE_ID + 0xA) // 0x10A
/* TLV type: Power TBL 2.4 GHz */
// #define TLV_TYPE_POWER_TBL_2_4GHZ (PROPRIETARY_TLV_BASE_ID + 0xC) // 0x10C
/* TLV type: Power TBL 5 GHz */
// #define TLV_TYPE_POWER_TBL_5GHZ (PROPRIETARY_TLV_BASE_ID + 0xD) // 0x10D
/* TLV type: WMM queue status */
// #define TLV_TYPE_WMMQSTATUS (PROPRIETARY_TLV_BASE_ID + 0x10) // 0x110
/* TLV type: Wildcard SSID */
// #define TLV_TYPE_WILDCARDSSID (PROPRIETARY_TLV_BASE_ID + 0x12) // 0x112
/* TLV type: TSF timestamp */
// #define TLV_TYPE_TSFTIMESTAMP (PROPRIETARY_TLV_BASE_ID + 0x13) // 0x113
/* TLV type: ARP filter */
// #define TLV_TYPE_ARP_FILTER (PROPRIETARY_TLV_BASE_ID + 0x15) // 0x115
/* TLV type: Beacon RSSI high */
// #define TLV_TYPE_RSSI_HIGH (PROPRIETARY_TLV_BASE_ID + 0x16) // 0x116
/* TLV type: Beacon SNR high */
// #define TLV_TYPE_SNR_HIGH (PROPRIETARY_TLV_BASE_ID + 0x17) // 0x117
/* TLV type: Start BG scan later */
// #define TLV_TYPE_STARTBGSCANLATER (PROPRIETARY_TLV_BASE_ID + 0x1E) // 0x11E
/* TLV type: Authentication type */
#define TLV_TYPE_AUTH_TYPE (PROPRIETARY_TLV_BASE_ID + 0x1F) // 0x11F
/* TLV type: BSSID */
#define TLV_TYPE_BSSID (PROPRIETARY_TLV_BASE_ID + 0x23) // 0x123
/* TLV type: Link quality */
// #define TLV_TYPE_LINK_QUALITY (PROPRIETARY_TLV_BASE_ID + 0x24) // 0x124
/* TLV type: Data RSSI low */
// #define TLV_TYPE_RSSI_LOW_DATA (PROPRIETARY_TLV_BASE_ID + 0x26) // 0x126
/* TLV type: Data SNR low */
// #define TLV_TYPE_SNR_LOW_DATA (PROPRIETARY_TLV_BASE_ID + 0x27) // 0x127
/* TLV type: Data RSSI high */
// #define TLV_TYPE_RSSI_HIGH_DATA (PROPRIETARY_TLV_BASE_ID + 0x28) // 0x128
/* TLV type: Data SNR high */
// #define TLV_TYPE_SNR_HIGH_DATA (PROPRIETARY_TLV_BASE_ID + 0x29) // 0x129
/* TLV type: Channel band list */
// #define TLV_TYPE_CHANNELBANDLIST (PROPRIETARY_TLV_BASE_ID + 0x2A) // 0x12A
/* TLV type: AP Channel band config */
// #define TLV_TYPE_UAP_CHAN_BAND_CONFIG (PROPRIETARY_TLV_BASE_ID + 0x2A) // 0x12A
/* TLV type: AP Mac address */
// #define TLV_TYPE_UAP_MAC_ADDR (PROPRIETARY_TLV_BASE_ID + 0x2B) // 0x12B
/* TLV type: AP Beacon period */
// #define TLV_TYPE_UAP_BEACON_PERIOD (PROPRIETARY_TLV_BASE_ID + 0x2C) // 0x12C
/* TLV type: AP DTIM period */
// #define TLV_TYPE_UAP_DTIM_PERIOD (PROPRIETARY_TLV_BASE_ID + 0x2D) // 0x12D
/* TLV type: AP Tx power */
// #define TLV_TYPE_UAP_TX_POWER (PROPRIETARY_TLV_BASE_ID + 0x2F) // 0x12F
/* TLV type: AP SSID broadcast control */
#define TLV_TYPE_UAP_BCAST_SSID_CTL (PROPRIETARY_TLV_BASE_ID + 0x30) // 0x130
/* TLV type: AP Preamble control */
// #define TLV_TYPE_UAP_PREAMBLE_CTL (PROPRIETARY_TLV_BASE_ID + 0x31) // 0x131
/* TLV type: AP Antenna control */
// #define TLV_TYPE_UAP_ANTENNA_CTL (PROPRIETARY_TLV_BASE_ID + 0x32) // 0x132
/* TLV type: AP RTS threshold */
// #define TLV_TYPE_UAP_RTS_THRESHOLD (PROPRIETARY_TLV_BASE_ID + 0x33) // 0x133
/* TLV type: AP Tx data rate */
// #define TLV_TYPE_UAP_TX_DATA_RATE (PROPRIETARY_TLV_BASE_ID + 0x35) // 0x135
/* TLV type: AP Packet forwarding control */
// #define TLV_TYPE_UAP_PKT_FWD_CTL (PROPRIETARY_TLV_BASE_ID + 0x36) // 0x136
/* TLV type: STA information */
// #define TLV_TYPE_UAP_STA_INFO (PROPRIETARY_TLV_BASE_ID + 0x37) // 0x137
/* TLV type: AP STA MAC address filter */
// #define TLV_TYPE_UAP_STA_MAC_ADDR_FILTER (PROPRIETARY_TLV_BASE_ID + 0x38) // 0x138
/* TLV type: AP STA ageout timer */
// #define TLV_TYPE_UAP_STA_AGEOUT_TIMER (PROPRIETARY_TLV_BASE_ID + 0x39) // 0x139
/* TLV type: AP WEP keys */
// #define TLV_TYPE_UAP_WEP_KEY (PROPRIETARY_TLV_BASE_ID + 0x3B) // 0x13B
/* TLV type: AP WPA passphrase */
#define TLV_TYPE_UAP_WPA_PASSPHRASE (PROPRIETARY_TLV_BASE_ID + 0x3C) // 0x13C
/* TLV type: Passphrase */
#define TLV_TYPE_PASSPHRASE (PROPRIETARY_TLV_BASE_ID + 0x3C) // 0x13C
/* TLV type: Encryption Protocol TLV */
// #define TLV_TYPE_ENCRYPTION_PROTO (PROPRIETARY_TLV_BASE_ID + 0x40) // 0x140
/* TLV type: AP protocol */
#define TLV_TYPE_UAP_ENCRYPT_PROTOCOL (PROPRIETARY_TLV_BASE_ID + 0x40) // 0x140
/* TLV type: AP AKMP */
#define TLV_TYPE_UAP_AKMP (PROPRIETARY_TLV_BASE_ID + 0x41) // 0x141
/* TLV type: Cipher TLV */
// #define TLV_TYPE_CIPHER (PROPRIETARY_TLV_BASE_ID + 0x42) // 0x142
/* TLV type: PMK */
// #define TLV_TYPE_PMK (PROPRIETARY_TLV_BASE_ID + 0x44) // 0x144
/* TLV type: AP Fragment threshold */
// #define TLV_TYPE_UAP_FRAG_THRESHOLD (PROPRIETARY_TLV_BASE_ID + 0x46) // 0x146
/* TLV type: AP Group rekey timer */
// #define TLV_TYPE_UAP_GRP_REKEY_TIME (PROPRIETARY_TLV_BASE_ID + 0x47) // 0x147
/* TLV type: BCN miss */
// #define TLV_TYPE_PRE_BCNMISS (PROPRIETARY_TLV_BASE_ID + 0x49) // 0x149
/* TLV type: HT Capabilities */
// #define TLV_TYPE_HT_CAP (PROPRIETARY_TLV_BASE_ID + 0x4A) // 0x14A
/* TLV type: HT Information */
// #define TLV_TYPE_HT_INFO (PROPRIETARY_TLV_BASE_ID + 0x4B) // 0x14B
/* TLV type: Secondary Channel Offset */
// #define TLV_SECONDARY_CHANNEL_OFFSET (PROPRIETARY_TLV_BASE_ID + 0x4C) // 0x14C
/* TLV type: 20/40 BSS Coexistence */
// #define TLV_TYPE_2040BSS_COEXISTENCE (PROPRIETARY_TLV_BASE_ID + 0x4D) // 0x14D
/* TLV type: Overlapping BSS Scan Parameters */
// #define TLV_TYPE_OVERLAP_BSS_SCAN_PARAM (PROPRIETARY_TLV_BASE_ID + 0x4E) // 0x14E
/* TLV type: Extended capabilities */
// #define TLV_TYPE_EXTCAP (PROPRIETARY_TLV_BASE_ID + 0x4F) // 0x14F
/* TLV type: Set of MCS values that STA desires to use within the BSS */
// #define TLV_TYPE_HT_OPERATIONAL_MCS_SET (PROPRIETARY_TLV_BASE_ID + 0x50) // 0x150
/* TLV type: Rate scope */
// #define TLV_TYPE_RATE_DROP_PATTERN (PROPRIETARY_TLV_BASE_ID + 0x51) // 0x151
/* TLV type: Rate drop pattern */
// #define TLV_TYPE_RATE_DROP_CONTROL (PROPRIETARY_TLV_BASE_ID + 0x52) // 0x152
/* TLV type: Rate scope */
// #define TLV_TYPE_RATE_SCOPE (PROPRIETARY_TLV_BASE_ID + 0x53) // 0x153
/* TLV type: Power group */
// #define TLV_TYPE_POWER_GROUP (PROPRIETARY_TLV_BASE_ID + 0x54) // 0x154
/* TLV type: AP Max Station number */
// #define TLV_TYPE_UAP_MAX_STA_CNT (PROPRIETARY_TLV_BASE_ID + 0x55) // 0x155
/* TLV type: Scan Response */
// #define TLV_TYPE_BSS_SCAN_RSP (PROPRIETARY_TLV_BASE_ID + 0x56) // 0x156
/* TLV type: Scan Response Stats */
// #define TLV_TYPE_BSS_SCAN_INFO (PROPRIETARY_TLV_BASE_ID + 0x57) // 0x157
/* TLV type: 11h Basic Rpt */
// #define TLV_TYPE_CHANRPT_11H_BASIC (PROPRIETARY_TLV_BASE_ID + 0x5B) // 0x15B
/* TLV type: AP Retry limit */
// #define TLV_TYPE_UAP_RETRY_LIMIT (PROPRIETARY_TLV_BASE_ID + 0x5D) // 0x15D
/* TLV type: WAPI IE */
// #define TLV_TYPE_WAPI_IE (PROPRIETARY_TLV_BASE_ID + 0x5E) // 0x15E
/* TLV type: AP MCBC data rate */
// #define TLV_TYPE_UAP_MCBC_DATA_RATE (PROPRIETARY_TLV_BASE_ID + 0x62) // 0x162
/* TLV type: AP RSN replay protection */
// #define TLV_TYPE_UAP_RSN_REPLAY_PROTECT (PROPRIETARY_TLV_BASE_ID + 0x64) // 0x164
/* TLV type: Management frame */
// #define TLV_TYPE_UAP_MGMT_FRAME (PROPRIETARY_TLV_BASE_ID + 0x68) // 0x168
/* TLV type: Mgmt IE */
// #define TLV_TYPE_MGMT_IE (PROPRIETARY_TLV_BASE_ID + 0x69) // 0x169
/* TLV type: AP mgmt IE passthru mask */
// #define TLV_TYPE_UAP_MGMT_IE_PASSTHRU_MASK (PROPRIETARY_TLV_BASE_ID + 0x70) // 0x170
/* TLV type: AP pairwise handshake timeout */
// #define TLV_TYPE_UAP_EAPOL_PWK_HSK_TIMEOUT (PROPRIETARY_TLV_BASE_ID + 0x75) // 0x175
/* TLV type: AP pairwise handshake retries */
// #define TLV_TYPE_UAP_EAPOL_PWK_HSK_RETRIES (PROPRIETARY_TLV_BASE_ID + 0x76) // 0x176
/* TLV type: AP groupwise handshake timeout */
// #define TLV_TYPE_UAP_EAPOL_GWK_HSK_TIMEOUT (PROPRIETARY_TLV_BASE_ID + 0x77) // 0x177
/* TLV type: AP groupwise handshake retries */
// #define TLV_TYPE_UAP_EAPOL_GWK_HSK_RETRIES (PROPRIETARY_TLV_BASE_ID + 0x78) // 0x178
/* TLV type: AP PS STA ageout timer */
// #define TLV_TYPE_UAP_PS_STA_AGEOUT_TIMER (PROPRIETARY_TLV_BASE_ID + 0x7B) // 0x17B
/* TLV type: Action frame */
// #define TLV_TYPE_IEEE_ACTION_FRAME (PROPRIETARY_TLV_BASE_ID + 0x8C) // 0x18C
/* TLV type: Pairwise cipher */
#define TLV_TYPE_PWK_CIPHER (PROPRIETARY_TLV_BASE_ID + 0x91) // 0x191
/* TLV type: Group cipher */
#define TLV_TYPE_GWK_CIPHER (PROPRIETARY_TLV_BASE_ID + 0x92) // 0x192
/* TLV type: BSS Status */
// #define TLV_TYPE_BSS_STATUS (PROPRIETARY_TLV_BASE_ID + 0x93) // 0x193
/* TLV type: 20/40 coex config */
// #define TLV_TYPE_2040_BSS_COEX_CONTROL (PROPRIETARY_TLV_BASE_ID + 0x98) // 0x198
/* TLV type: Rx BA sync */
// #define TLV_TYPE_RXBA_SYNC (PROPRIETARY_TLV_BASE_ID + 0x99) // 0x199
/* TLV type: Max mgmt IE */
// #define TLV_TYPE_MAX_MGMT_IE (PROPRIETARY_TLV_BASE_ID + 0xAA) // 0x1AA
/* TLV type: BG scan repeat count */
// #define TLV_TYPE_REPEAT_COUNT (PROPRIETARY_TLV_BASE_ID + 0xB0) // 0x1B0

/* Capability information */
// #define WLAN_CAPABILITY_ESS 0x1
#define WLAN_CAPABILITY_IBSS 0x2
// #define WLAN_CAPABILITY_CF_POLLABLE 0x4
// #define WLAN_CAPABILITY_CF_POLL_REQUEST 0x8
#define WLAN_CAPABILITY_PRIVACY 0x10
// #define WLAN_CAPABILITY_SHORT_PREAMBLE 0x20
// #define WLAN_CAPABILITY_PBCC 0x40
// #define WLAN_CAPABILITY_CHANNEL_AGILITY 0x80
// #define WLAN_CAPABILITY_SPECTRUM_MGMT 0x100
// #define WLAN_CAPABILITY_SHORT_SLOT 0x400
// #define WLAN_CAPABILITY_DSSS_OFDM 0x2000

/* Firmware host command ID constants */
/* Host command ID: Get hardware specifications */
#define HOST_ID_GET_HW_SPEC 0x3
/* Host command ID: 802.11 scan */
#define HOST_ID_802_11_SCAN 0x6
/* Host command ID: 802.11 get log */
// #define HOST_ID_802_11_GET_LOG 0xB
/* Host command ID: MAC multicast address */
// #define HOST_ID_MAC_MULTICAST_ADR 0x10
/* Host command ID: 802.11 associate */
#define HOST_ID_802_11_ASSOCIATE 0x12
/* Host command ID: 802.11 SNMP MIB */
// #define HOST_ID_802_11_SNMP_MIB 0x16
/* Host command ID: MAC register access */
// #define HOST_ID_MAC_REG_ACCESS 0x19
/* Host command ID: BBP register access */
// #define HOST_ID_BBP_REG_ACCESS 0x1A
/* Host command ID: RF register access */
// #define HOST_ID_RF_REG_ACCESS 0x1B
/* Host command ID: 802.11 radio control */
// #define HOST_ID_802_11_RADIO_CONTROL 0x1C
/* Host command ID: 802.11 RF channel */
// #define HOST_ID_802_11_RF_CHANNEL 0x1D
/* Host command ID: 802.11 RF Tx power */
// #define HOST_ID_802_11_RF_TX_POWER 0x1E
/* Host command ID: 802.11 RF antenna */
// #define HOST_ID_802_11_RF_ANTENNA 0x20
/* Host command ID: 802.11 deauthenticate */
#define HOST_ID_802_11_DEAUTHENTICATE 0x24
/* Host command ID: 802.11 disassoicate */
// #define HOST_ID_802_11_DISASSOCIATE 0x26
/* Host command ID: MAC control */
#define HOST_ID_MAC_CONTROL 0x28
/* Host command ID: 802.11 Ad-Hoc start */
// #define HOST_ID_802_11_AD_HOC_START 0x2B
/* Host command ID: 802.11 Ad-Hoc join */
// #define HOST_ID_802_11_AD_HOC_JOIN 0x2C
/* Host command ID: 802.11 Ad-Hoc stop */
// #define HOST_ID_802_11_AD_HOC_STOP 0x40
/* Host command ID: 802.22 MAC address */
#define HOST_ID_802_11_MAC_ADDR 0x4D
/* Host command ID: 802.11 EEPROM access */
// #define HOST_ID_802_11_EEPROM_ACCESS 0x59
/* Host command ID: 802.11 D domain information */
// #define HOST_ID_802_11D_DOMAIN_INFO 0x5B
/* Host command ID: WMM Traffic Stream Status */
// #define HOST_ID_WMM_TS_STATUS 0x5D
/* Host command ID: 802.11 key material */
// #define HOST_ID_802_11_KEY_MATERIAL 0x5E
/* Host command ID: 802.11 sleep parameters */
// #define HOST_ID_802_11_SLEEP_PARAMS 0x66
/* Host command ID: 802.11 sleep period */
// #define HOST_ID_802_11_SLEEP_PERIOD 0x68
/* Host command ID: 802.11 BG scan config */
// #define HOST_ID_802_11_BG_SCAN_CONFIG 0x6B
/* Host command ID: 802.11 BG scan query */
// #define HOST_ID_802_11_BG_SCAN_QUERY 0x6C
/* Host command ID: WMM ADDTS req */
// #define HOST_ID_WMM_ADDTS_REQ 0x6E
/* Host command ID: WMM DELTS req */
// #define HOST_ID_WMM_DELTS_REQ 0x6F
/* Host command ID: WMM queue configuration */
// #define HOST_ID_WMM_QUEUE_CONFIG 0x70
/* Host command ID: 802.11 get status */
// #define HOST_ID_WMM_GET_STATUS 0x71
/* Host command ID: 802.11 subscribe event */
// #define HOST_ID_802_11_SUBSCRIBE_EVENT 0x75
/* Host command ID: 802.11 Tx rate query */
// #define HOST_ID_802_11_TX_RATE_QUERY 0x7F
/* Host command ID: WMM queue stats */
// #define HOST_ID_WMM_QUEUE_STATS 0x81
/* Host command ID: 802.11 IBSS coalescing status */
// #define HOST_ID_802_11_IBSS_COALESCING_STATUS 0x83
/* Host command ID: Memory access */
// #define HOST_ID_MEM_ACCESS 0x86
/* Host command ID: SDIO GPIO interrupt configuration */
// #define HOST_ID_SDIO_GPIO_INT_CONFIG 0x88
/* Host command ID: Mfg command */
// #define HOST_ID_MFG_COMMAND 0x89
/* Host command ID: Inactivity timeout ext */
// #define HOST_ID_INACTIVITY_TIMEOUT_EXT 0x8A
/* Host command ID: DBGS configuration */
// #define HOST_ID_DBGS_CFG 0x8B
/* Host command ID: Get memory */
// #define HOST_ID_GET_MEM 0x8C
/* Host command ID: Cal data dnld */
// #define HOST_ID_CFG_DATA 0x8F
/* Host command ID: SDIO pull control */
// #define HOST_ID_SDIO_PULL_CTRL 0x93
/* Host command ID: ECL system clock configuration */
// #define HOST_ID_ECL_SYSTEM_CLOCK_CONFIG 0x94
/* Host command ID: Extended version */
// #define HOST_ID_VERSION_EXT 0x97
/* Host command ID: MEF configuration */
// #define HOST_ID_MEF_CFG 0x9A
/* Host command ID: 802.11 RSSI INFO */
// #define HOST_ID_RSSI_INFO 0xA4
/* Host command ID: Function initialization */
#define HOST_ID_FUNC_INIT 0xA9
/* Host command ID: Function shutdown */
#define HOST_ID_FUNC_SHUTDOWN 0xAA
/* Host command ID: SYS_INFO */
// #define HOST_ID_APCMD_SYS_INFO 0xAE
/* Host command ID: SYS_RESET */
// #define HOST_ID_APCMD_SYS_RESET 0xAF
/* Host command ID: SYS_CONFIGURE */
#define HOST_ID_APCMD_SYS_CONFIGURE 0xB0
/* Host command ID: BSS_START */
#define HOST_ID_APCMD_BSS_START 0xB1
/* Host command ID: BSS_STOP */
#define HOST_ID_APCMD_BSS_STOP 0xB2
/* Host command ID: STA_LIST */
// #define HOST_ID_APCMD_STA_LIST 0xB3
/* Host command ID: STA_DEAUTH */
#define HOST_ID_APCMD_STA_DEAUTH 0xB5
/* Host command ID: SUPPLICANT_PMK */
#define HOST_ID_SUPPLICANT_PMK 0xC4
/* Host command ID: SUPPLICANT_PROFILE */
// #define HOST_ID_SUPPLICANT_PROFILE 0xC5
/* Host command ID: Delete a Block Ack Request */
// #define HOST_ID_11N_CFG 0xCD
/* Host command ID: Add Block Ack Request */
// #define HOST_ID_11N_ADDBA_REQ 0xCE
/* Host command ID: Add Block Ack Response */
#define HOST_ID_11N_ADDBA_RSP 0xCF
/* Host command ID: Delete a Block Ack Request */
// #define HOST_ID_11N_DELBA 0xD0
/* Host command ID: 802.11 Tx power configuration */
// #define HOST_ID_TXPWR_CFG 0xD1
/* Host command ID: Soft reset */
// #define HOST_ID_SOFT_RESET 0xD5
/* Host command ID: 802.11 b/g/n rate configration */
// #define HOST_ID_TX_RATE_CFG 0xD6
/* Host command ID: Configure Tx buffer size */
// #define HOST_ID_RECONFIGURE_TX_BUFF 0xD9
/* Host command ID: AMSDU Aggr Ctrl */
// #define HOST_ID_AMSDU_AGGR_CTRL 0xDF
/* Host command ID: Enhanced PS mode */
// #define HOST_ID_802_11_PS_MODE_ENH 0xE4
/* Host command ID: Host sleep configuration */
// #define HOST_ID_802_11_HS_CFG_ENH 0xE5
/* Host command ID: P2P params config */
// #define HOST_ID_P2P_PARAMS_CONFIG 0xEA
/* Host command ID: WiFi direct mode config */
// #define HOST_ID_WIFI_DIRECT_MODE_CONFIG 0xEB
/* Host command ID: CAU register access */
// #define HOST_ID_CAU_REG_ACCESS 0xED
/* Host command ID: Mgmt IE list */
// #define HOST_ID_MGMT_IE_LIST 0xF2
/* Host command ID: Set BSS_MODE */
// #define HOST_ID_SET_BSS_MODE 0xF7
/* Host command ID: Tx data pause */
// #define HOST_ID_CFG_TX_DATA_PAUSE 0x103
/* Host command ID: Coalescing configuration */
// #define HOST_ID_COALESCE_CFG 0x10A
/* Host command ID: Forward mgmt frame */
// #define HOST_ID_RX_MGMT_IND 0x10C
/* Host command ID: Remain on channel */
// #define HOST_ID_802_11_REMAIN_ON_CHANNEL 0x10D
/* Host command ID: OTP user data */
// #define HOST_ID_OTP_READ_USER_DATA 0x114
/* Host command ID: HS wakeup reason */
// #define HOST_ID_HS_WAKEUP_REASON 0x116
/* Host command ID: Reject addba request */
// #define HOST_ID_REJECT_ADDBA_REQ 0x119
/* Host command ID: Config low power mode */
// #define HOST_ID_CONFIG_LOW_POWER_MODE 0x128
/* Host command ID: Target device access */
// #define HOST_ID_TARGET_ACCESS 0x12A
/* Host command ID: Rx packet coalescing configuration */
// #define HOST_ID_RX_PKT_COALESCE_CFG 0x12C
/* Host command ID: EAPOL PKT */
// #define HOST_ID_802_11_EAPOL_PKT 0x12E

/* Command RET code, MSB is set to 1 */
#define HOST_RET_BIT 0x8000

/* General purpose action: Get */
#define HOST_ACT_GEN_GET 0x0
/* General purpose action: Set */
#define HOST_ACT_GEN_SET 0x1
/* General purpose action: Get_Current */
// #define HOST_ACT_GEN_GET_CURRENT 0x3
/* General purpose action: Remove */
// #define HOST_ACT_GEN_REMOVE 0x4

/* General result code */
/* General result code OK */
#define HOST_RESULT_OK 0x0
/* Genenral error */
// #define HOST_RESULT_ERROR 0x1
/* Command is not valid */
// #define HOST_RESULT_NOT_SUPPORT 0x2
/* Command is pending */
// #define HOST_RESULT_PENDING 0x3
/* System is busy (command ignored) */
// #define HOST_RESULT_BUSY 0x4
/* Data buffer is not big enough */
// #define HOST_RESULT_PARTIAL_DATA 0x5

/* Define action or option for HOST_ID_MAC_CONTROL */
/* MAC action: Rx on */
#define HOST_ACT_MAC_RX_ON 0x1
/* MAC action: Tx on */
#define HOST_ACT_MAC_TX_ON 0x2
/* MAC action: WEP enable */
// #define HOST_ACT_MAC_WEP_ENABLE 0x8
/* MAC action: EthernetII enable */
#define HOST_ACT_MAC_ETHERNETII_ENABLE 0x10
/* MAC action: Promiscous mode enable */
// #define HOST_ACT_MAC_PROMISCUOUS_ENABLE 0x80
/* MAC action: All multicast enable */
// #define HOST_ACT_MAC_ALL_MULTICAST_ENABLE 0x100
/* MAC action: RTS/CTS enable */
// #define HOST_ACT_MAC_RTS_CTS_ENABLE 0x200
/* MAC action: Strict protection enable */
// #define HOST_ACT_MAC_STRICT_PROTECTION_ENABLE 0x400
/* MAC action: Force 11n protection disable */
// #define HOST_ACT_MAC_FORCE_11N_PROTECTION_OFF 0x800
/* MAC action: Ad-Hoc G protection on */
// #define HOST_ACT_MAC_ADHOC_G_PROTECTION_ON 0x2000

/* Define action or option for HOST_ID_802_11_SCAN */
/* Scan type: BSS */
// #define HOST_BSS_MODE_BSS 0x1
/* Scan type: IBSS */
// #define HOST_BSS_MODE_IBSS 0x2
/* Scan type: Any */
#define HOST_BSS_MODE_ANY 0x3

/* Event ID: Dummy host wakeup signal */
// #define EVENT_DUMMY_HOST_WAKEUP_SIGNAL 0x1
/* Event ID: Link lost */
// #define EVENT_LINK_LOST 0x3
/* Event ID: Link sensed */
// #define EVENT_LINK_SENSED 0x4
/* Event ID: MIB changed */
// #define EVENT_MIB_CHANGED 0x6
/* Event ID: Init done */
// #define EVENT_INIT_DONE 0x7
/* Event ID: Deauthenticated */
#define EVENT_DEAUTHENTICATED 0x8
/* Event ID: Disassociated */
// #define EVENT_DISASSOCIATED 0x9
/* Event ID: Power save awake */
// #define EVENT_PS_AWAKE 0xA
/* Event ID: Power save sleep */
// #define EVENT_PS_SLEEP 0xB
/* Event ID: MIC error multicast */
// #define EVENT_MIC_ERR_MULTICAST 0xD
/* Event ID: MIC error unicast */
// #define EVENT_MIC_ERR_UNICAST 0xE
/* Event ID: Ad-Hoc BCN lost */
// #define EVENT_ADHOC_BCN_LOST 0x11
/* Event ID: WMM status change */
#define EVENT_WMM_STATUS_CHANGE 0x17
/* Event ID: BG scan report */
// #define EVENT_BG_SCAN_REPORT 0x18
/* Event ID: Beacon RSSI low */
// #define EVENT_RSSI_LOW 0x19
/* Event ID: Beacon SNR low */
// #define EVENT_SNR_LOW 0x1A
/* Event ID: Maximum fail */
// #define EVENT_MAX_FAIL 0x1B
/* Event ID: Beacon RSSI high */
// #define EVENT_RSSI_HIGH 0x1C
/* Event ID: Beacon SNR high */
// #define EVENT_SNR_HIGH 0x1D
/* Event ID: IBSS coalsced */
// #define EVENT_IBSS_COALESCED 0x1E
/* Event ID: Data RSSI low */
// #define EVENT_DATA_RSSI_LOW 0x24
/* Event ID: Data SNR low */
// #define EVENT_DATA_SNR_LOW 0x25
/* Event ID: Data RSSI high */
// #define EVENT_DATA_RSSI_HIGH 0x26
/* Event ID: Data SNR high */
// #define EVENT_DATA_SNR_HIGH 0x27
/* Event ID: Link Quality */
// #define EVENT_LINK_QUALITY 0x28
/* Event ID: Port release event */
#define EVENT_PORT_RELEASE 0x2B
/* Event ID: STA deauth */
#define EVENT_MICRO_AP_STA_DEAUTH 0x2C
/* Event ID: STA assoicated */
#define EVENT_MICRO_AP_STA_ASSOC 0x2D
/* Event ID: BSS started */
#define EVENT_MICRO_AP_BSS_START 0x2E
/* Event ID: Pre-Beacon Lost */
// #define EVENT_PRE_BEACON_LOST 0x31
/* Event ID: Add BA event */
#define EVENT_ADDBA 0x33
/* Event ID: Del BA event */
#define EVENT_DELBA 0x34
/* Event ID: BA stream timeout */
// #define EVENT_BA_STREAM_TIMEOUT 0x37
/* Event ID: AMSDU aggr control */
// #define EVENT_AMSDU_AGGR_CTRL 0x42
/* Event ID: BSS idle event */
#define EVENT_MICRO_AP_BSS_IDLE 0x43
/* Event ID: BSS active event */
#define EVENT_MICRO_AP_BSS_ACTIVE 0x44
/* Event ID: WEP ICV error */
// #define EVENT_WEP_ICV_ERR 0x46
/* Event ID: Host sleep enable */
// #define EVENT_HS_ACT_REQ 0x47
/* Event ID: BW changed */
// #define EVENT_BW_CHANGE 0x48
/* Event ID: WiFiDirect generic */
// #define EVENT_WIFIDIRECT_GENERIC_EVENT 0x49
/* Event ID: WiFiDirect service discovery */
// #define EVENT_WIFIDIRECT_SERVICE_DISCOVERY 0x4A
/* Event ID: RSN connect event */
#define EVENT_MICRO_AP_EV_RSN_CONNECT 0x51
/* Event ID: Tx data pause event */
// #define EVENT_TX_DATA_PAUSE 0x55
/* Event ID: RXBA_SYNC */
// #define EVENT_RXBA_SYNC 0x59
/* Event ID: Remain on channel expired */
// #define EVENT_REMAIN_ON_CHANNEL_EXPIRED 0x5F
/* Event ID: FW debug information */
// #define EVENT_FW_DEBUG_INFO 0x63
/* Event ID: BG scan stopped */
// #define EVENT_BG_SCAN_STOPPED 0x65
/* Event ID: SAD report */
// #define EVENT_SAD_REPORT 0x66
/* Event ID: Tx status */
// #define EVENT_TX_STATUS_REPORT 0x74
/* Event ID: BT coex WLAN parameter change */
// #define EVENT_BT_COEX_WLAN_PARAM_CHANGE 0x76

/* Setup the number of rates passed in the driver/firmware API */
#define HOST_SUPPORTED_RATES 14
/* Country code length used for 802.11D */
#define COUNTRY_CODE_LEN 3
/* Maximum number of AC QOS queues available in the driver/firmware */
#define MAX_AC_QUEUES 4
/* Size of a TSPEC, used to allocate necessary buffer space in commands */
#define WMM_TSPEC_SIZE 63
/* Extra IE bytes allocated in messages for appended IEs after a TSPEC */
#define WMM_ADDTS_EXTRA_IE_BYTES 256
/* Extra TLV bytes allocated in messages for configuring WMM queues */
#define WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES 64
/* Number of bins in the histogram for the HOST_DS_WMM_QUEUE_STATS */
#define WMM_STATS_PKTS_HIST_BINS 7
/* Max IE index to FW */
#define MAX_MGMT_IE_INDEX_TO_FW 4
/* Max IE index per BSS */
#define MAX_MGMT_IE_INDEX 12

typedef enum {
    SECURITY_TYPE_NONE,
    SECURITY_TYPE_WEP,
    SECURITY_TYPE_WPA,
    SECURITY_TYPE_WPA2
} wlan_security_type;

typedef enum {
    BSS_TYPE_STA = 0x00,
    BSS_TYPE_UAP = 0x01,
    BSS_TYPE_WIFIDIRECT = 0x02,
    BSS_TYPE_ANY = 0xFF
} wlan_bss_type;

typedef enum {
    AUTH_TYPE_OPEN = 0x00,
    AUTH_TYPE_SHARED = 0x01,
    AUTH_TYPE_NETWORK_EAP = 0x80
} wlan_auth_type;

#define WLAN_PACK_STRUCT __attribute__((packed))

typedef struct {
    uint8_t oui[3];
    uint8_t oui_type;
    uint16_t version;
    uint8_t multicast_oui[4];
    uint16_t unicast_num;
    uint8_t unicast_oui[1][4];
    uint16_t auth_num;
    uint8_t auth_oui[1][4];
} WLAN_PACK_STRUCT wlan_vendor;

typedef struct {
    /* Header type */
    uint16_t type;
    /* Header length */
    uint16_t len;
} WLAN_PACK_STRUCT MrvlIEtypesHeader_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t channel;
} WLAN_PACK_STRUCT MrvlIETypes_PhyParamDSSet_t;

typedef struct {
    /* OUI */
    uint8_t oui[3];
    /* OUI type */
    uint8_t oui_type;
    /* OUI subtype */
    uint8_t oui_subtype;
    /* Version */
    uint8_t version;
} WLAN_PACK_STRUCT MrvlIETypes_Vendor_context_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t vendor[64];
} WLAN_PACK_STRUCT MrvlIETypes_Vendor_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t rsn[64];
} WLAN_PACK_STRUCT MrvlIETypes_RSN_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t rate[1];
} WLAN_PACK_STRUCT MrvlIETypes_OpRateSet_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t count;
    uint8_t period;
    uint16_t max_duration;
    uint16_t duration_remaining;
} WLAN_PACK_STRUCT MrvlIETypes_CfParamSet_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint16_t auth_type;
} WLAN_PACK_STRUCT MrvlIETypes_AuthType_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t mac_addr[MAC_ADDR_LENGTH];
} WLAN_PACK_STRUCT MrvlIETypes_BSSIDList_t;

typedef struct {
    /* Capability bit map: ESS */
    uint8_t ess : 1;
    /* Capability bit map: IBSS */
    uint8_t ibss : 1;
    /* Capability bit map: CF pollable */
    uint8_t cf_pollable : 1;
    /* Capability bit map: CF poll request */
    uint8_t cf_poll_rqst : 1;
    /* Capability bit map: Privacy */
    uint8_t privacy : 1;
    /* Capability bit map: Short preamble */
    uint8_t short_preamble : 1;
    /* Capability bit map: PBCC */
    uint8_t pbcc : 1;
    /* Capability bit map: Channel agility */
    uint8_t chan_agility : 1;
    /* Capability bit map: Spectrum management */
    uint8_t spectrum_mgmt : 1;
    /* Capability bit map: Reserved */
    uint8_t rsrvd3 : 1;
    /* Capability bit map: Short slot time */
    uint8_t short_slot_time : 1;
    /* Capability bit map: APSD */
    uint8_t apsd : 1;
    /* Capability bit map: Reserved */
    uint8_t rsvrd2 : 1;
    /* Capability bit map: DSS OFDM */
    uint8_t dsss_ofdm : 1;
    /* Capability bit map: Reserved */
    uint8_t rsrvd1 : 2;
} WLAN_PACK_STRUCT IEEEtypes_CapInfo_t;

typedef uint16_t IEEEtypes_AId_t;

typedef uint16_t IEEEtypes_StatusCode_t;

typedef struct {
    /* First channel */
    uint8_t first_chan;
    /* Number of channels */
    uint8_t no_of_chan;
    /* Maximum Tx power in dBm */
    uint8_t max_tx_pwr;
} WLAN_PACK_STRUCT IEEEtypes_SubbandSet_t;

typedef struct {
    /* Capability information */
    uint16_t Capability;
    /* Association response status code */
    IEEEtypes_StatusCode_t StatusCode;
    /* Association ID */
    IEEEtypes_AId_t AId;
    /* IE data buffer */
    uint8_t IEBuffer[1];
} WLAN_PACK_STRUCT IEEEtypes_AssocRsp_t;

typedef struct {
    /* CF parameter: Element ID */
    uint8_t element_id;
    /* CF parameter: Length */
    uint8_t len;
    /* CF parameter: Count */
    uint8_t cfp_cnt;
    /* CF parameter: Period */
    uint8_t cfp_period;
    /* CF parameter: Maximum duration */
    uint16_t cfp_max_duration;
    /* CF parameter: Remaining duration */
    uint16_t cfp_duration_remaining;
} WLAN_PACK_STRUCT IEEEtypes_CfParamSet_t;

typedef struct {
    /* Element ID */
    uint8_t element_id;
    /* Length */
    uint8_t len;
    /* ATIM window value in ms */
    uint16_t atim_window;
} WLAN_PACK_STRUCT IEEEtypes_IbssParamSet_t;

typedef union {
    /* SS parameter: CF parameter set */
    IEEEtypes_CfParamSet_t cf_param_set;
    /* SS parameter: IBSS parameter set */
    IEEEtypes_IbssParamSet_t ibss_param_set;
} WLAN_PACK_STRUCT IEEEtypes_SsParamSet_t;

typedef struct {
    /* FH parameter: Element ID */
    uint8_t element_id;
    /* FH parameter: Length */
    uint8_t len;
    /* FH parameter: Dwell time in ms */
    uint16_t dwell_time;
    /* FH parameter: Hop set */
    uint8_t hop_set;
    /* FH parameter: Hop pattern */
    uint8_t hop_pattern;
    /* FH parameter: Hop index */
    uint8_t hop_index;
} WLAN_PACK_STRUCT IEEEtypes_FhParamSet_t;

typedef struct {
    /* DS parameter: Element ID */
    uint8_t element_id;
    /* DS parameter: Length */
    uint8_t len;
    /* DS parameter: Current channel */
    uint8_t current_chan;
} WLAN_PACK_STRUCT IEEEtypes_DsParamSet_t;

typedef union {
    /* FH parameter set */
    IEEEtypes_FhParamSet_t fh_param_set;
    /* DS parameter set */
    IEEEtypes_DsParamSet_t ds_param_set;
} WLAN_PACK_STRUCT IEEEtypes_PhyParamSet_t;

typedef struct {
    /* Element ID */
    uint8_t element_id;
    /* Length */
    uint8_t len;
    /* OUI */
    uint8_t oui[3];
    /* OUI type */
    uint8_t oui_type;
    /* OUI subtype */
    uint8_t oui_subtype;
    /* Version */
    uint8_t version;
} WLAN_PACK_STRUCT IEEEtypes_VendorHeader_t;

typedef struct {
    /* Parameter set count */
    uint8_t param_set_count : 4;
    /* Reserved */
    uint8_t reserved : 3;
    /* QoS UAPSD */
    uint8_t qos_uapsd : 1;
} WLAN_PACK_STRUCT IEEEtypes_WmmQosInfo_t;

typedef struct {
    /* Aifsn */
    uint8_t aifsn : 4;
    /* Acm */
    uint8_t acm : 1;
    /* Aci */
    uint8_t aci : 2;
    /* Reserved */
    uint8_t reserved : 1;
} WLAN_PACK_STRUCT IEEEtypes_WmmAciAifsn_t;

typedef struct {
    /* Minimum ecw */
    uint8_t ecw_min : 4;
    /* Maximum ecw */
    uint8_t ecw_max : 4;
} WLAN_PACK_STRUCT IEEEtypes_WmmEcw_t;

typedef struct {
    IEEEtypes_WmmAciAifsn_t aci_aifsn;
    IEEEtypes_WmmEcw_t ecw;
    uint16_t tx_op_limit;
} WLAN_PACK_STRUCT IEEEtypes_WmmAcParameters_t;

typedef struct {
    /* WMM Parameter IE - Vendor Specific Header
     * ElementID  [221/0xDD]
     * Len        [24]
     * Oui        [00:50:F2]
     * OuiType    [2]
     * OuiSubType [1]
     * Version    [1] */
    IEEEtypes_VendorHeader_t vend_hdr;
    /* QoS information */
    IEEEtypes_WmmQosInfo_t qos_info;
    /* Reserved */
    uint8_t reserved;
    /* AC parameters record WMM_AC_BE, WMM_AC_BK, WMM_AC_VI, WMM_AC_VO */
    IEEEtypes_WmmAcParameters_t ac_params[MAX_AC_QUEUES];
} WLAN_PACK_STRUCT IEEEtypes_WmmParameter_t;

typedef struct {
    uint8_t type;
    uint8_t length;
} WLAN_PACK_STRUCT IEEEHeader;

typedef struct {
    IEEEHeader header;
    uint8_t data[1];
} WLAN_PACK_STRUCT IEEEType;

typedef struct {
    uint16_t ie_length; // Total information element length, without sizeof(ie_length)
    uint8_t bssid[MAC_ADDR_LENGTH]; // BSSID
    uint8_t rssi; // RSSI value as received from peer
    uint64_t pkt_time_stamp; // Timestamp
    uint16_t bcn_interval; // Beacon interval
    uint16_t cap_info; // Capabilities information
    IEEEType ie_parameters; // IEEE parameters
} WLAN_PACK_STRUCT bss_desc_set_t;

// 已知数据域的大小，求整个结构体的大小
// 例如定义一个很大的buffer，然后定义一个IEEEType *指向该buffer
// buffer接收到数据后，要获取接收到的IEEEType实际大小不能用sizeof(IEEEType)，因为结构体中data的长度定义为1
// 此时应使用TLV_STRUCTLEN(p)
#define TLV_STRUCTLEN(tlv) (sizeof((tlv)->header) + (tlv)->header.length)

// 已知TLV的地址和大小，求下一个TLV的地址
#define TLV_NEXT(tlv) ((uint8_t *)(tlv) + TLV_STRUCTLEN(tlv))

typedef struct {
    /* HW interface version number */
    uint16_t hw_if_version;
    /* HW version number */
    uint16_t version;
    /* Reserved field */
    uint16_t reserved;
    /* Max no of multicast address */
    uint16_t num_of_mcast_adr;
    /* MAC address */
    uint8_t permanent_addr[MAC_ADDR_LENGTH];
    /* Region code */
    uint16_t region_code;
    /* Number of antenna used */
    uint16_t number_of_antenna;
    /* FW release number, e.g. 0x1234=1.2.3.4 */
    uint32_t fw_release_number;
    /* Reserved field */
    uint32_t reserved_1;
    /* Reserved field */
    uint32_t reserved_2;
    /* Reserved field */
    uint32_t reserved_3;
    /* FW/HW capability */
    uint32_t fw_cap_info;
    /* 802.11n device capabilities */
    uint32_t dot_11n_dev_cap;
    /* MIMO abstraction of MCSs supported by device */
    uint8_t dev_mcs_support;
    /* Valid end port at init */
    uint16_t mp_end_port;
    /* Mgmt IE buffer count */
    uint16_t mgmt_buf_count;
} WLAN_PACK_STRUCT HOST_DS_GET_HW_SPEC;

typedef struct {
    /* Action */
    uint16_t action;
    /* Type */
    uint16_t type;
    /* Data length */
    uint16_t data_len;
    /* Data */
} WLAN_PACK_STRUCT HOST_DS_802_11_CFG_DATA;

typedef struct {
    /* Action */
    uint16_t action;
    /* Reserved field */
    uint16_t reserved;
} WLAN_PACK_STRUCT HOST_DS_MAC_CONTROL;

typedef struct {
    /* Action */
    uint16_t action;
    /* MAC address */
    uint8_t mac_addr[MAC_ADDR_LENGTH];
} WLAN_PACK_STRUCT HOST_DS_802_11_MAC_ADDR;

#define MAX_MULTICAST_LIST_SIZE 32

typedef struct {
    /* Action */
    uint16_t action;
    /* Number of addresses */
    uint16_t num_of_adrs;
    /* List of MAC */
    uint8_t mac_list[MAC_ADDR_LENGTH * MAX_MULTICAST_LIST_SIZE];
} WLAN_PACK_STRUCT HOST_DS_MAC_MULTICAST_ADR;

typedef struct {
    /* Number of multicast transmitted frames */
    uint32_t mcast_tx_frame;
    /* Number of failures */
    uint32_t failed;
    /* Number of retries */
    uint32_t retry;
    /* Number of multiretries */
    uint32_t multiretry;
    /* Number of duplicate frames */
    uint32_t frame_dup;
    /* Number of RTS success */
    uint32_t rts_success;
    /* Number of RTS failure */
    uint32_t rts_failure;
    /* Number of acknowledgement failure */
    uint32_t ack_failure;
    /* Number of fragmented packets received */
    uint32_t rx_frag;
    /* Number of multicast frames received */
    uint32_t mcast_rx_frame;
    /* FCS error */
    uint32_t fcs_error;
    /* Number of transmitted frames */
    uint32_t tx_frame;
    /* Reserved field */
    uint32_t reserved;
    /* Number of WEP icv error for each key */
    uint32_t wep_icv_err_cnt[4];
    /* Beacon received count */
    uint32_t bcn_rcv_cnt;
    /* Beacon missed count */
    uint32_t bcn_miss_cnt;
} WLAN_PACK_STRUCT HOST_DS_802_11_GET_LOG;

typedef struct {
    /* Action */
    uint16_t action;
    /* Parameter used for exponential averaging for data */
    uint16_t ndata;
    /* Parameter used for exponential averaging for beacon */
    uint16_t nbcn;
    /* Reserved field 0 */
    uint16_t reserved[9];
    /* Reserved field 1 */
    uint64_t reserved_1;
} WLAN_PACK_STRUCT HOST_DS_802_11_RSSI_INFO;

typedef struct {
    /* Action */
    uint16_t action;
    /* Parameter used for exponential averaging for data */
    uint16_t ndata;
    /* Parameter used for exponential averaging for beacon */
    uint16_t nbcn;
    /* Last data RSSI in dBm */
    int16_t data_rssi_last;
    /* Last data NF in dBm */
    int16_t data_nf_last;
    /* Average data RSSI in dBm */
    int16_t data_rssi_avg;
    /* Average data NF in dBm */
    int16_t data_nf_avg;
    /* Last beacon RSSI in dBm */
    int16_t bcn_rssi_last;
    /* Last beacon NF in dBm */
    int16_t bcn_nf_last;
    /* Average beacon RSSI in dBm */
    int16_t bcn_rssi_avg;
    /* Average beacon NF in dBm */
    int16_t bcn_nf_avg;
    /* Last RSSI beacon TSF */
    uint64_t tsf_bcn;
} WLAN_PACK_STRUCT HOST_DS_802_11_RSSI_INFO_RSP;

typedef struct {
    /* SNMP query type */
    uint16_t query_type;
    /* SNMP object ID */
    uint16_t oid;
    /* SNMP buffer size */
    uint16_t buf_size;
    /* Value */
    uint8_t value[1];
} WLAN_PACK_STRUCT HOST_DS_802_11_SNMP_MIB;

typedef struct {
    /* Action */
    uint16_t action;
    /* Control */
    uint16_t control;
} WLAN_PACK_STRUCT HOST_DS_802_11_RADIO_CONTROL;

typedef struct {
    /* Action */
    uint16_t action;
    /* Current channel */
    uint16_t current_channel;
    /* RF type */
    uint16_t rf_type;
    /* Reserved field */
    uint16_t reserved;
    /* List of channels */
    uint8_t channel_list[32];
} WLAN_PACK_STRUCT HOST_DS_802_11_RF_CHANNEL;

typedef struct {
    /* Tx rate */
    uint8_t tx_rate;
    /* HT info
     * [Bit 0] RxRate format: LG = 0, HT = 1
     * [Bit 1] HT bandwidth: BW20 = 0, BW40 = 1
     * [Bit 2] HT guard interval: LGI = 0, SGI = 1 */
    uint8_t ht_info;
} WLAN_PACK_STRUCT HOST_TX_RATE_QUERY;

typedef struct {
    /* Action */
    uint16_t action;
    /* Tx rate configuration index */
    uint16_t cfg_index;
    /* MrvlRateScope_t RateScope;
     * MrvlRateDropPattern_t RateDrop; */
} WLAN_PACK_STRUCT HOST_DS_TX_RATE_CFG;

typedef struct {
    /* Action */
    uint16_t action;
    /* Power group configuration index */
    uint16_t cfg_index;
    /* Power group configuration mode */
    uint32_t mode;
    /* MrvlTypes_Power_Group_t PowerGrpCfg[1]; */
} WLAN_PACK_STRUCT HOST_DS_TXPWR_CFG;

typedef struct {
    /* Action */
    uint16_t action;
    /* Current power level */
    int16_t current_level;
    /* Maximum power */
    int8_t max_power;
    /* Minimum power */
    int8_t min_power;
} WLAN_PACK_STRUCT HOST_DS_802_11_RF_TX_POWER;

typedef struct {
    /* Action */
    uint16_t action;
    /* Antenna or 0xFFFF (diversity) */
    uint16_t antenna_mode;
} WLAN_PACK_STRUCT HOST_DS_802_11_RF_ANTENNA;

typedef struct {
    /* Null packet interval */
    uint16_t null_pkt_interval;
    /* Num dtims */
    uint16_t multiple_dtims;
    /* Becaon miss interval */
    uint16_t bcn_miss_timeout;
    /* Local listen interval */
    uint16_t local_listen_interval;
    /* Ad-Hoc awake period */
    uint16_t adhoc_wake_period;
    /* 0x1 - Firmware to automatically choose PS_POLL or NULL mode
     * 0x2 - PS_POLL
     * 0x3 - NULL mode */
    uint16_t mode;
    /* Delay to PS in ms */
    uint16_t delay_to_ps;
} WLAN_PACK_STRUCT ps_param;

typedef struct {
    /* Deep sleep inactivity timeout */
    uint16_t deep_sleep_timeout;
} WLAN_PACK_STRUCT auto_ds_param;

typedef struct {
    /* Response control
     * 0x0 - Response not needed
     * 0x1 - Response needed */
    uint16_t resp_ctrl;
} WLAN_PACK_STRUCT sleep_confirm_param;

/* Bitmap for get auto deepsleep */
#define BITMAP_AUTO_DS 0x1
/* Bitmap for STA power save */
#define BITMAP_STA_PS 0x10
/* Bitmap for UAP inactivity based PS */
#define BITMAP_UAP_INACT_PS 0x100
/* Bitmap for UAP DTIM PS */
#define BITMAP_UAP_DTIM_PS 0x200

typedef struct {
    /* Bitmap for enable power save mode */
    uint16_t ps_bitmap;
    /* Auto deep sleep parameter
     * STA power save parameter
     * UAP inactivity parameter
     * UAP DTIM parameter */
} WLAN_PACK_STRUCT auto_ps_param;

typedef struct {
    /* Action */
    uint16_t action;
    /* Data specific to action */
    /* For IEEE power save data will be as UINT16 mode (0x1 - Firmware to
       automatically choose PS_POLL or NULL mode, 0x2 - PS_POLL, 0x3 -
       NULL mode ) UINT16 NullPacketInterval UINT16 NumDtims UINT16
       BeaconMissInterval UINT16 LocalListenInterval UINT16 AdhocAwakePeriod */
    /* For auto deep sleep */
    /* UINT16 DeepSleepInactivityTimeout */
    /* For PS sleep confirm UINT16 ResponeCtrl
     * 0x0 - Response from firmware not needed
     * 0x1 - Response from firmware needed */
    union {
        /* PS param definition */
        ps_param opt_ps;
        /* Auto ds param definition */
        auto_ds_param auto_ds;
        /* Sleep confirm parameter definition */
        sleep_confirm_param sleep_cfm;
        /* Bitmap for get PS info and disable PS mode */
        uint16_t ps_bitmap;
        /* Auto ps param */
        auto_ps_param auto_ps;
    } params;
} WLAN_PACK_STRUCT HOST_DS_802_11_PS_MODE_ENH;

typedef struct {
    /* [Bit 0] Broadcast data
     * [Bit 1] Unicast data
     * [Bit 2] MAC events
     * [Bit 3] Multicast data */
    uint32_t conditions;
    /* GPIO pin or 0xFF for interface */
    uint8_t gpio;
    /* Gap in ms or 0xFF for special setting when
     * GPIO is used to wakeup host */
    uint8_t gap;
} WLAN_PACK_STRUCT hs_config_param;

typedef enum {
    /* HS action
     * 0x1 - Configure enhanced host sleep mode
     * 0x2 - Activate enhanced host sleep mode */
    HS_CONFIGURE = 0x1,
    HS_ACTIVATE = 0x2
} wlan_hs_sleep_action;

typedef struct {
    /* Response control
     * 0x0 - Response not needed
     * 0x1 - Response needed */
    uint16_t resp_ctrl;
} WLAN_PACK_STRUCT hs_activate_param;

typedef struct {
    /* Action
     * 0x1 - Configure enhanced host sleep mode
     * 0x2 - Activate enhanced host sleep mode */
    uint16_t action;
    union {
        /* Configure enhanced hs */
        hs_config_param hs_config;
        /* Activate enhanced hs */
        hs_activate_param hs_activate;
    } params;
} WLAN_PACK_STRUCT HOST_DS_802_11_HS_CFG_ENH;

typedef struct {
    /* Channel scan mode passive flag */
    uint8_t passive_scan : 1;
    /* Disble channel filtering flag */
    uint8_t disable_chan_filt : 1;
    /* Multidomain scan mode */
    uint8_t multidomain_scan : 1;
    /* Enable probe response timeout */
    uint8_t rsp_timeout_en : 1;
    /* Enable hidden ssid report */
    uint8_t hidden_ssid_report : 1;
    /* First channel in scan */
    uint8_t first_chan : 1;
    /* Reserved */
    uint8_t reserved_6_7 : 2;
} WLAN_PACK_STRUCT ChanScanMode_t;

typedef enum {
    SEC_CHAN_NONE = 0,
    SEC_CHAN_ABOVE = 1,
    SEC_CHAN_BELOW = 3
} wlan_second_channel;

typedef enum {
    CHAN_BW_20MHZ,
    CHAN_BW_10MHZ,
    CHAN_BW_40MHZ
} wlan_channel_bandwidth;

typedef struct {
    /* Channel scan parameter: Radio type */
    uint8_t radio_type;
    /* Channel scan parameter: Channel number */
    uint8_t chan_number;
    /* Channel scan parameter: Channel scan mode */
    uint8_t chan_scan_mode;
    /* Channel scan parameter: Minimum scan time */
    uint16_t min_scan_time;
    /* Channel scan parameter: Maximum scan time */
    uint16_t max_scan_time;
} WLAN_PACK_STRUCT ChanScanParamSet_t;

typedef struct {
    /* Header */
    MrvlIEtypesHeader_t header;
    /* Channel scan parameters */
    ChanScanParamSet_t chan_scan_param[1];
} WLAN_PACK_STRUCT MrvlIEtypes_ChanListParamSet_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t ssid[MAX_SSID_LENGTH];
} WLAN_PACK_STRUCT MrvlIEtypes_SSIDParamSet_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t phrase[MAX_PHRASE_LENGTH];
} WLAN_PACK_STRUCT MrvlIEtypes_PassPhrase_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint16_t enc_protocol;
} WLAN_PACK_STRUCT MrvlIEtypes_Enc_Protocol_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint16_t key_mgmt;
    uint16_t operation;
} WLAN_PACK_STRUCT MrvlIEtypes_AKMP_t;

#define WPA_CIPHER_WEP40 0x1
#define WPA_CIPHER_WEP104 0x2
#define WPA_CIPHER_TKIP 0x4
#define WPA_CIPHER_CCMP 0x8

typedef struct {
    MrvlIEtypesHeader_t header;
    uint16_t protocol;
    uint16_t cipher;
} WLAN_PACK_STRUCT MrvlIEtypes_PTK_cipher_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint16_t cipher;
} WLAN_PACK_STRUCT MrvlIEtypes_GTK_cipher_t;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t broadcast_ssid;
} WLAN_PACK_STRUCT MrvlIETypes_ApBCast_SSID_Ctrl_t;

typedef struct {
    /* BSS mode */
    uint8_t bss_mode;
    /* BSSID */
    uint8_t bssid[MAC_ADDR_LENGTH];
    /* TLV buffer */
    uint8_t tlv_buffer[1];
    /* MrvlIEtypes_SSIDParamSet_t SSIDParamSet;
     * MrvlIEtypes_ChanListParamSet_t ChanListParamSet;
     * MrvlIEtypes_RatesParamSet_t OpRateSet; */
} WLAN_PACK_STRUCT HOST_DS_802_11_SCAN;

typedef struct {
    /* Action */
    uint16_t action;
    /* Mgmt frame subtype mask */
    uint32_t mgmt_subtype_mask;
} WLAN_PACK_STRUCT HOST_DS_RX_MGMT_IND;

typedef struct {
    /* Size of BSS descriptor */
    uint16_t bss_descript_size;
    /* Number of sets */
    uint8_t number_of_sets;
    /* BSS descriptor and TLV buffer */
    uint8_t bss_desc_and_tlv_buffer[1];
} WLAN_PACK_STRUCT HOST_DS_802_11_SCAN_RSP;

typedef struct {
    /* Action */
    uint16_t action;
    /* 0x0 - Disable
     * 0x1 - Enable */
    uint8_t enable;
    /* BSS type */
    uint8_t bss_type;
    /* Num of channel per scan */
    uint8_t chan_per_scan;
    /* Reserved field */
    uint8_t reserved;
    /* Reserved field */
    uint16_t reserved1;
    /* Interval between consecutive scans */
    uint32_t scan_interval;
    /* Reserved field */
    uint32_t reserved2;
    /* Condition to trigger report to host */
    uint32_t report_condition;
    /* Reserved field */
    uint16_t reserved3;
} WLAN_PACK_STRUCT HOST_DS_802_11_BG_SCAN_CONFIG;

typedef struct {
    /* Flush */
    uint8_t flush;
} WLAN_PACK_STRUCT HOST_DS_802_11_BG_SCAN_QUERY;

typedef struct {
    /* Report condition */
    uint32_t report_condition;
    /* Scan response */
    HOST_DS_802_11_SCAN_RSP scan_resp;
} WLAN_PACK_STRUCT HOST_DS_802_11_BG_SCAN_QUERY_RSP;

typedef struct {
    /* Action */
    uint16_t action;
    /* Bitmap of subscribed events */
    uint16_t event_bitmap;
} WLAN_PACK_STRUCT HOST_DS_SUBSCRIBE_EVENT;

typedef struct {
    /* Action */
    uint16_t action;
    /* Reserved field */
    uint16_t reserved;
    /* User data length */
    uint16_t user_data_length;
    /* User data */
    uint8_t user_data[1];
} WLAN_PACK_STRUCT HOST_DS_OTP_USER_DATA;

typedef struct {
    /* Peer STA address */
    uint8_t peer_sta_addr[MAC_ADDR_LENGTH];
    /* Capability information */
    uint16_t cap_info;
    /* Listen interval */
    uint16_t listen_interval;
    /* Beacon period */
    uint16_t beacon_period;
    /* DTIM period */
    uint8_t dtim_period;
    uint8_t tlv_buffer[1];
    /* MrvlIEtypes_SSIDParamSet_t SSIDParamSet;
     * MrvlIEtypes_PhyParamSet_t PhyParamSet;
     * MrvlIEtypes_SsParamSet_t SsParamSet;
     * MrvlIEtypes_RatesParamSet_t RatesParamSet; */
} WLAN_PACK_STRUCT HOST_DS_802_11_ASSOCIATE;

typedef struct {
    /* Association response structure */
    IEEEtypes_AssocRsp_t assoc_rsp;
} WLAN_PACK_STRUCT HOST_DS_802_11_ASSOCIATE_RSP;

typedef struct {
    /* MAC address */
    uint8_t mac_addr[MAC_ADDR_LENGTH];
    /* Deauthentication reason code */
    uint16_t reason_code;
} WLAN_PACK_STRUCT HOST_DS_802_11_DEAUTHENTICATE;

typedef struct {
    /* Ad-Hoc SSID */
    uint8_t ssid[MAX_SSID_LENGTH];
    /* BSS mode */
    uint8_t bss_mode;
    /* Beacon period */
    uint16_t beacon_period;
    /* DTIM period */
    uint8_t dtim_period;
    /* SS parameter set */
    IEEEtypes_SsParamSet_t ss_param_set;
    /* PHY parameter set */
    IEEEtypes_PhyParamSet_t phy_param_set;
    /* Reserved field */
    uint16_t reserved1;
    /* Capability information */
    IEEEtypes_CapInfo_t cap;
    /* Supported data rates */
    uint8_t DataRate[HOST_SUPPORTED_RATES];
} WLAN_PACK_STRUCT HOST_DS_802_11_AD_HOC_START;

typedef struct {
    /* Padding */
    uint8_t pad[3];
    /* Ad-Hoc BSSID */
    uint8_t bssid[MAC_ADDR_LENGTH];
    /* Padding to sync with firmware structure */
    uint8_t pad2[2];
    /* Result */
    uint8_t result;
} WLAN_PACK_STRUCT HOST_DS_802_11_AD_HOC_START_RESULT;

typedef struct {
    /* Result */
    uint8_t result;
} WLAN_PACK_STRUCT HOST_DS_802_11_AD_HOC_JOIN_RESULT;

typedef struct {
    /* BSSID */
    uint8_t bssid[MAC_ADDR_LENGTH];
    /* SSID */
    uint8_t ssid[MAX_SSID_LENGTH];
    /* BSS mode */
    uint8_t bss_mode;
    /* Beacon period */
    uint16_t beacon_period;
    /* DTIM period */
    uint8_t dtim_period;
    /* Timestamp */
    uint8_t time_stamp[8];
    /* Local time */
    uint8_t local_time[8];
    /* PHY parameter set */
    IEEEtypes_PhyParamSet_t phy_param_set;
    /* SS parameter set */
    IEEEtypes_SsParamSet_t ss_param_set;
    /* Capability information */
    IEEEtypes_CapInfo_t cap;
    /* Supported data rates */
    uint8_t data_rates[HOST_SUPPORTED_RATES];
    /* DO NOT ADD ANY FIELDS TO THIS STRUCTURE
     * It is used in the Ad-Hoc join command and will cause
     * a binary layout mismatch with the firmware */
} WLAN_PACK_STRUCT adhoc_bssdesc_t;

typedef struct {
    /* Ad-Hoc BSS descriptor */
    adhoc_bssdesc_t bss_descriptor;
    /* Reserved field */
    uint16_t reserved1;
    /* Reserved field */
    uint16_t reserved2;
} WLAN_PACK_STRUCT HOST_DS_802_11_AD_HOC_JOIN;

typedef struct {
    /* Header */
    MrvlIEtypesHeader_t header;
    /* Country code */
    uint8_t country_code[COUNTRY_CODE_LEN];
    /* Set of subbands */
    IEEEtypes_SubbandSet_t sub_band[1];
} WLAN_PACK_STRUCT MrvlIEtypes_DomainParamSet_t;

typedef struct {
    /* Action */
    uint16_t action;
    /* Domain parameter set */
    MrvlIEtypes_DomainParamSet_t domain;
} WLAN_PACK_STRUCT HOST_DS_802_11D_DOMAIN_INFO;

typedef struct {
    /* Action */
    uint16_t action;
    /* Domain parameter set */
    MrvlIEtypes_DomainParamSet_t domain;
} WLAN_PACK_STRUCT HOST_DS_802_11D_DOMAIN_INFO_RSP;

typedef struct {
    /* Result of the ADDBA request operation */
    uint8_t add_req_result;
    /* Peer MAC address */
    uint8_t peer_mac_addr[MAC_ADDR_LENGTH];
    /* Dialog token */
    uint8_t dialog_token;
    /* Block ACK parameter set */
    uint16_t block_ack_param_set;
    /* Block ACK timeout value */
    uint16_t block_ack_tmo;
    /* Starting sequence number */
    uint16_t ssn;
} WLAN_PACK_STRUCT HOST_DS_11N_ADDBA_REQ;

typedef struct {
    /* Result of the ADDBA response operation */
    uint8_t add_rsp_result;
    /* Peer MAC address */
    uint8_t peer_mac_addr[MAC_ADDR_LENGTH];
    /* Dialog token */
    uint8_t dialog_token;
    /* Status code */
    uint16_t status_code;
    /* Block ACK parameter set */
    uint16_t block_ack_param_set;
    /* Block ACK timeout value */
    uint16_t block_ack_tmo;
    /* Starting sequence number */
    uint16_t ssn;
} WLAN_PACK_STRUCT HOST_DS_11N_ADDBA_RSP;

typedef struct {
    /* Result of the ADDBA request operation */
    uint8_t del_result;
    /* Peer MAC address */
    uint8_t peer_mac_addr[MAC_ADDR_LENGTH];
    /* Delete block ACK parameter set */
    uint16_t del_ba_param_set;
    /* Reason code sent for DELBA */
    uint16_t reason_code;
    /* Reserved */
    uint8_t reserved;
} WLAN_PACK_STRUCT HOST_DS_11N_DELBA;

typedef struct {
    /* Action */
    uint16_t action;
    /* Buffer size */
    uint16_t buf_size;
    /* End port for multiport */
    uint16_t mp_end_port;
    /* Reserved */
    uint16_t reserved3;
} HOST_DS_TXBUF_CFG;

typedef struct {
    /* Action */
    uint16_t action;
    /* 0x0 - Disable
     * 0x1 - Enable */
    uint16_t enable;
    /* Get the current buffer size valid */
    uint16_t curr_buf_size;
} WLAN_PACK_STRUCT HOST_DS_AMSDU_AGGR_CTRL;

typedef struct {
    /* Action */
    uint16_t action;
    /* HT Tx cap */
    uint16_t ht_tx_cap;
    /* HT Tx info */
    uint16_t ht_tx_info;
    /* Misc configuration */
    uint16_t misc_config;
} WLAN_PACK_STRUCT HOST_DS_11N_CFG;

typedef struct {
    /* Action */
    uint16_t action;
    /* [Bit 0] Host sleep activated
     * [Bit 1] Auto reconnect enabled
     * [Bit x] Reserved */
    uint32_t conditions;
} WLAN_PACK_STRUCT HOST_DS_REJECT_ADDBA_REQ;

typedef struct {
    /* Header */
    MrvlIEtypesHeader_t header;
    /* Queue index */
    uint8_t queue_index;
    /* Disabled flag */
    uint8_t disabled;
    /* Medium time allocation in 32us units */
    uint16_t medium_time;
    /* Flow required flag */
    uint8_t flow_required;
    /* Flow created flag */
    uint8_t flow_created;
    /* Reserved */
    uint32_t reserved;
} WLAN_PACK_STRUCT MrvlIEtypes_WmmQueueStatus_t;

typedef struct {
    /* Queue status TLV */
    uint8_t queue_status_tlv[sizeof(MrvlIEtypes_WmmQueueStatus_t) * MAX_AC_QUEUES];
    /* WMM parameter TLV */
    uint8_t wmm_param_tlv[sizeof(IEEEtypes_WmmParameter_t) + 2];
} WLAN_PACK_STRUCT HOST_DS_WMM_GET_STATUS;

typedef enum {
    WLAN_CMD_RESULT_SUCCESS,
    WLAN_CMD_RESULT_FAILURE,
    WLAN_CMD_RESULT_TIMEOUT,
    WLAN_CMD_RESULT_INVALID_DATA
} wlan_cmd_result_e;

typedef struct {
    wlan_cmd_result_e command_result;
    uint32_t timeout_ms;
    uint8_t dialog_token;
    uint8_t ieee_status_code;
    uint8_t tspec_data[WMM_TSPEC_SIZE];
    uint8_t addts_extra_ie_buf[WMM_ADDTS_EXTRA_IE_BYTES];
} WLAN_PACK_STRUCT HOST_DS_WMM_ADDTS_REQ;

typedef struct {
    wlan_cmd_result_e command_result;
    uint8_t dialog_token;
    uint8_t ieee_reason_code;
    uint8_t tspec_data[WMM_TSPEC_SIZE];
} WLAN_PACK_STRUCT HOST_DS_WMM_DELTS_REQ;

typedef enum {
    WMM_QUEUE_CONFIG_ACTION_GET,
    WMM_QUEUE_CONFIG_ACTION_SET,
    WMM_QUEUE_CONFIG_ACTION_DEFAULT,
    WMM_QUEUE_CONFIG_ACTION_MAX
} wlan_wmm_queue_config_action_e;

typedef enum {
    WMM_AC_BK,
    WMM_AC_BE,
    WMM_AC_VI,
    WMM_AC_VO
} wlan_wmm_ac_e;

typedef struct {
    /* Set, get or default */
    wlan_wmm_queue_config_action_e action;
    /* WMM_AC_BK(0) to WMM_AC_VO(3) */
    wlan_wmm_ac_e access_category;
    /* Ignored if 0 on a set command
     * Set to the 802.11e specified 500 TUs as default */
    uint16_t msdu_lifetime_expiry;
    /* Not supported */
    uint8_t tlv_buffer[WMM_QUEUE_CONFIG_EXTRA_TLV_BYTES];
} WLAN_PACK_STRUCT HOST_DS_WMM_QUEUE_CONFIG;

typedef enum {
    WLAN_WMM_STATS_ACTION_START,
    WLAN_WMM_STATS_ACTION_STOP,
    WLAN_WMM_STATS_ACTION_GET_CLR,
    /* Not used */
    WLAN_WMM_STATS_ACTION_SET_CFG,
    /* Not used */
    WLAN_WMM_STATS_ACTION_GET_CFG,
    WLAN_WMM_STATS_ACTION_MAX
} wlan_wmm_queue_stats_action_e;

typedef struct {
    /* Start, stop or get */
    wlan_wmm_queue_stats_action_e action;
    /* Set if select_bin is up, clear for AC */
    uint8_t select_is_userpri : 1;
    /* WMM_AC_BK(0) to WMM_AC_VO(3) or TID */
    uint8_t select_bin : 7;
    /* Number of successful packets transmitted */
    uint16_t pkt_count;
    /* Packets lost, not included in PktCount */
    uint16_t pkt_loss;
    /* Average queue delay in ms */
    uint32_t avg_queue_delay;
    /* Average transmission delay in ms */
    uint32_t avg_tx_delay;
    /* Calc used time, units of 32 ms */
    uint16_t used_time;
    /* Calc policed time, units of 32 ms */
    uint16_t policed_time;
    /* [0] - 0ms <= delay < 5ms
     * [1] - 5ms <= delay < 10ms
     * [2] - 10ms <= delay < 20ms
     * [3] - 20ms <= delay < 30ms
     * [4] - 30ms <= delay < 40ms
     * [5] - 40ms <= delay < 50ms
     * [6] - 50ms <= delay < MSDULifetime (TUs) */
    uint16_t delay_histogram[WMM_STATS_PKTS_HIST_BINS];
    /* Reserved */
    uint16_t reserved_1;
} WLAN_PACK_STRUCT HOST_DS_WMM_QUEUE_STATS;

typedef struct {
    /* TSID ranged from 0 to 7 */
    uint8_t tid;
    /* TSID specified is valid */
    uint8_t valid;
    /* AC TSID is active on */
    uint8_t access_category;
    /* UP specified for TSID */
    uint8_t user_priority;
    /* Power save mode for TSID
     * 0x0 - Legacy
     * 0x1 - UAPSD */
    uint8_t psb;
    /* 0x1 - Uplink
     * 0x2 - Downlink
     * 0x3 - Bidirectional */
    uint8_t flow_dir;
    /* Medium time granted for TSID */
    uint16_t medium_time;
} WLAN_PACK_STRUCT HOST_DS_WMM_TS_STATUS;

/* Key info flag for multicast key */
#define KEY_INFO_MCAST_KEY 0x1
/* Key info flag for unicast key */
#define KEY_INFO_UCAST_KEY 0x2
/* Key info flag for enable key */
#define KEY_INFO_ENABLE_KEY 0x4
/* Key info flag for default key */
#define KEY_INFO_DEFAULT_KEY 0x8
/* Key info flag for Tx key */
#define KEY_INFO_TX_KEY 0x10
/* Key info flag for Rx key */
#define KEY_INFO_RX_KEY 0x20
/* Key info flag for AES-CMAC key */
#define KEY_INFO_CMAC_AES_KEY 0x400
/* PN size for WPA/WPA2 */
#define WPA_PN_SIZE 8
/* PN size for PMF IGTK */
#define IGTK_PN_SIZE 8
/* WAPI key size */
#define WAPI_KEY_SIZE 32
/* key params fixed length */
#define KEY_PARAMS_FIXED_LEN 10
/* key index mask */
#define KEY_INDEX_MASK 0xF
/* A few details needed for WEP */
/* 104 bits */
#define MAX_WEP_KEY_SIZE 13
/* 40 bits */
#define MIN_WEP_KEY_SIZE 5
/* Packet number size */
#define PN_SIZE 16
/* Max seq size of WPA/WPA2 key */
#define SEQ_MAX_SIZE 8
/* WPA AES key length */
#define WPA_AES_KEY_LEN 16
/* WPA TKIP key length */
#define WPA_TKIP_KEY_LEN 32
/* AES-CMAC key length */
#define CMAC_AES_KEY_LEN 16
/* IGTK key length */
#define WPA_IGTK_KEY_LEN 16

typedef struct {
    /* Key length */
    uint16_t key_len;
    /* WEP key */
    uint8_t key[MAX_WEP_KEY_SIZE];
} WLAN_PACK_STRUCT wep_param_t;

typedef struct {
    /* WPA packet number */
    uint8_t pn[WPA_PN_SIZE];
    /* Key length */
    uint16_t key_len;
    /* TKIP key */
    uint8_t key[WPA_TKIP_KEY_LEN];
} WLAN_PACK_STRUCT tkip_param;

typedef struct {
    /* WPA packet number */
    uint8_t pn[WPA_PN_SIZE];
    /* Key length */
    uint16_t key_len;
    /* AES key */
    uint8_t key[WPA_AES_KEY_LEN];
} WLAN_PACK_STRUCT aes_param;

typedef struct {
    /* Packet number */
    uint8_t pn[PN_SIZE];
    /* Key length */
    uint16_t key_len;
    /* WAPI key */
    uint8_t key[WAPI_KEY_SIZE];
} WLAN_PACK_STRUCT wapi_param;

typedef struct {
    /* IGTK packet number */
    uint8_t ipn[IGTK_PN_SIZE];
    /* Key length */
    uint16_t key_len;
    /* AES-CMAC key */
    uint8_t key[CMAC_AES_KEY_LEN];
} WLAN_PACK_STRUCT cmac_aes_param;

typedef struct {
    /* Type ID */
    uint16_t type;
    /* Length of payload */
    uint16_t length;
    /* MAC address */
    uint8_t mac_addr[MAC_ADDR_LENGTH];
    /* Key index */
    uint8_t key_idx;
    /* Type of key
     * 0x0 - WEP
     * 0x1 - TKIP
     * 0x2 - AES
     * 0x3 - WAPI
     * 0x4 - AES-CMAC */
    uint8_t key_type;
    /* Key control info specific to key_type_id */
    uint16_t key_info;
    union {
        /* WEP key param */
        wep_param_t wep;
        /* TKIP key param */
        tkip_param tkip;
        /* AES key param */
        aes_param aes;
        /* WAPI key param */
        wapi_param wapi;
        /* AES-CMAC key param */
        cmac_aes_param cmac_aes;
    } key_params;
} WLAN_PACK_STRUCT MrvlIEtype_KeyParamSetV2_t;

typedef struct {
    /* Action */
    uint16_t action;
    /* Key parameter set */
    MrvlIEtype_KeyParamSetV2_t key_param_set;
} WLAN_PACK_STRUCT HOST_DS_802_11_KEY_MATERIAL;

typedef struct {
    /* Get, set or clear */
    uint16_t action;
    /* Cache result initialized to 0 */
    uint16_t cache_result;
    /* TLV buffer */
    uint8_t tlv_buffer[1];
    /* MrvlIEtypes_SSIDParamSet_t SSIDParamSet;
     * MrvlIEtypes_PMK_t pmk;
     * MrvlIEtypes_PassPhrase_t PassPhrase;
     * MrvlIEtypes_BSSID_t bssid; */
} WLAN_PACK_STRUCT HOST_DS_802_11_SUPPLICANT_PMK;

typedef struct {
    /* Get, set or get_current */
    uint16_t action;
    /* Reserved */
    uint16_t reserved;
    /* TLV buffer */
    uint8_t tlv_buffer[1];
    /* MrvlIEtypes_EncrProto_t EncrProto; */
} WLAN_PACK_STRUCT HOST_DS_802_11_SUPPLICANT_PROFILE;

typedef struct {
    /* Selected version string */
    uint8_t version_str_sel;
    /* Version string */
    char version_str[128];
} WLAN_PACK_STRUCT HOST_DS_VERSION_EXT;

typedef struct {
    /* Action */
    uint16_t action;
    /* 0x0 - Disable
     * 0x1 - Enable */
    uint16_t enable;
    /* BSSID */
    uint8_t bssid[MAC_ADDR_LENGTH];
    /* Beacon interval */
    uint16_t beacon_interval;
    /* ATIM window interval */
    uint16_t atim_window;
    /* Use G rate protection */
    uint16_t use_g_rate_protect;
} WLAN_PACK_STRUCT HOST_DS_802_11_IBSS_STATUS;

typedef struct {
    /* Size of buffer */
    uint16_t buf_size;
    /* Buffer number */
    uint16_t buf_count;
} WLAN_PACK_STRUCT custom_ie_info;

typedef struct {
    /* Type */
    uint16_t type;
    /* Length */
    uint16_t len;
    /* Number of tuples */
    uint16_t count;
    /* Custom IE info tuples */
    custom_ie_info info[MAX_MGMT_IE_INDEX];
} WLAN_PACK_STRUCT tlvbuf_max_mgmt_ie;

typedef struct {
    /* IE index */
    uint16_t ie_index;
    /* Mgmt subtype mask */
    uint16_t mgmt_subtype_mask;
    /* IE length */
    uint16_t ie_length;
    /* IE buffer */
    uint8_t ie_buffer[1];
} WLAN_PACK_STRUCT custom_ie;

typedef struct {
    /* Type */
    uint16_t type;
    /* Length */
    uint16_t len;
    /* IE data */
    custom_ie ie_data_list[MAX_MGMT_IE_INDEX_TO_FW];
    /* Max mgmt IE TLV */
    tlvbuf_max_mgmt_ie max_mgmt_ie;
} WLAN_PACK_STRUCT ds_misc_custom_ie;

typedef struct {
    /* Action */
    uint16_t action;
    /* Get/Set custom IE */
    ds_misc_custom_ie ds_mgmt_ie;
} WLAN_PACK_STRUCT HOST_DS_MGMT_IE_LIST_CFG;

typedef struct {
    /* Action */
    uint16_t action;
    /* Current system clock */
    uint16_t cur_sys_clk;
    /* Clock type */
    uint16_t sys_clk_type;
    /* Length of clocks */
    uint16_t sys_clk_len;
    /* System clocks */
    uint16_t sys_clk[16];
} WLAN_PACK_STRUCT HOST_DS_ECL_SYSTEM_CLOCK_CONFIG;

typedef struct {
    /* Action */
    uint16_t action;
    /* MAC register offset */
    uint16_t offset;
    /* MAC register value */
    uint32_t value;
} WLAN_PACK_STRUCT HOST_DS_MAC_REG_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* BBP register offset */
    uint16_t offset;
    /* BBP register value */
    uint8_t value;
    /* Reserved field */
    uint8_t reserved[3];
} WLAN_PACK_STRUCT HOST_DS_BBP_REG_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* RF register offset */
    uint16_t offset;
    /* RF register value */
    uint8_t value;
    /* Reserved field */
    uint8_t reserved[3];
} WLAN_PACK_STRUCT HOST_DS_RF_REG_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* Offset, multiple of 4 */
    uint16_t offset;
    /* Number of bytes */
    uint16_t byte_count;
    /* Value */
    uint8_t value;
} WLAN_PACK_STRUCT HOST_DS_802_11_EEPROM_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* Reserved field */
    uint16_t reserved;
    /* Address */
    uint32_t addr;
    /* Value */
    uint32_t value;
} WLAN_PACK_STRUCT HOST_DS_MEM_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* CSU target device
     * 0x1 - CSU
     * 0x2 - PSU */
    uint16_t csu_target;
    /* Target device address */
    uint16_t address;
    /* Data */
    uint8_t data;
} WLAN_PACK_STRUCT HOST_DS_TARGET_ACCESS;

typedef struct {
    /* Action */
    uint16_t action;
    /* In us, 0 means 1000us */
    uint16_t timeout_unit;
    /* Inactivity timeout for unicast data */
    uint16_t unicast_timeout;
    /* Inactivity timeout for multicast data */
    uint16_t mcast_timeout;
    /* Timeout for additional Rx traffic after Null PM1 packet exchange */
    uint16_t ps_entry_timeout;
    /* Reserved to further expansion */
    uint16_t reserved;
} WLAN_PACK_STRUCT HOST_DS_INACTIVITY_TIMEOUT;

typedef struct {
    /* Action */
    uint16_t action;
    /* TLV buffer */
    uint8_t tlv_buffer[1];
} WLAN_PACK_STRUCT HOST_DS_SYS_CONFIG;

typedef struct {
    /* System info */
    uint8_t sys_info[64];
} WLAN_PACK_STRUCT HOST_DS_SYS_INFO;

typedef struct {
    /* MAC address */
    uint8_t mac[MAC_ADDR_LENGTH];
    /* Reason code */
    uint16_t reason;
} WLAN_PACK_STRUCT HOST_DS_STA_DEAUTH;

typedef struct {
    /* STA number */
    uint16_t sta_count;
    /* MrvlIEtypes_sta_info_t sta_info[1]; */
} WLAN_PACK_STRUCT HOST_DS_STA_LIST;

typedef struct {
    /* Action */
    uint16_t action;
    /* Power mode */
    uint16_t power_mode;
} WLAN_PACK_STRUCT HOST_DS_POWER_MGMT_EXT;

typedef struct {
    /* Action */
    uint16_t action;
    /* Sleep period in ms */
    uint16_t sleep_pd;
} WLAN_PACK_STRUCT HOST_DS_802_11_SLEEP_PERIOD;

typedef struct {
    /* Action */
    uint16_t action;
    /* Sleep clock error in ppm */
    uint16_t error;
    /* Wakeup offset in us */
    uint16_t offset;
    /* Clock stabilization time in us */
    uint16_t stable_time;
    /* Control periodic calibration */
    uint8_t cal_control;
    /* Control the use of external sleep clock */
    uint8_t external_sleep_clk;
    /* Reserved field */
    uint16_t reserved;
} WLAN_PACK_STRUCT HOST_DS_802_11_SLEEP_PARAMS;

typedef struct {
    /* Action */
    uint16_t action;
    /* GPIO interrupt pin */
    uint16_t gpio_pin;
    /* GPIO interrupt edge
     * 0x0 - Rising edge
     * 0x1 - Falling edge */
    uint16_t gpio_int_edge;
    /* GPIO interrupt pulse width in us */
    uint16_t gpio_pulse_width;
} WLAN_PACK_STRUCT HOST_DS_SDIO_GPIO_INT_CONFIG;

typedef struct {
    /* Action */
    uint16_t action;
    /* Delay of pulling up in us */
    uint16_t pull_up;
    /* Delay of pulling down in us */
    uint16_t pull_down;
} WLAN_PACK_STRUCT HOST_DS_SDIO_PULL_CTRL;

typedef struct {
    /* Connection type */
    uint8_t con_type;
} WLAN_PACK_STRUCT HOST_DS_SET_BSS_MODE;

typedef struct {
    /* Action */
    uint16_t action;
    /* Enable/Disable Tx data pause */
    uint8_t enable_tx_pause;
    /* Max Tx buffer number allowed for all PS clients */
    uint8_t pause_tx_count;
} WLAN_PACK_STRUCT HOST_DS_CMD_TX_DATA_PAUSE;

typedef struct {
    /* Action
     * 0x0 - Get
     * 0x1 - Set
     * 0x4 - Clear */
    uint16_t action;
    /* Not used */
    uint8_t status;
    /* Reserved field */
    uint8_t reserved;
    /* Band config */
    uint8_t bandcfg;
    /* Channel */
    uint8_t channel;
    /* Remain period in ms */
    uint32_t remain_period;
} WLAN_PACK_STRUCT HOST_DS_REMAIN_ON_CHANNEL;

typedef struct {
    /* Action */
    uint16_t action;
    /* 0: Disable
     * 1: Listen
     * 2: Go
     * 3: P2P client
     * 4: Find
     * 5: Stop find */
    uint16_t mode;
} WLAN_PACK_STRUCT HOST_DS_WIFI_DIRECT_MODE;

typedef struct {
    /* Action */
    uint16_t action;
    /* MrvlIEtypes_NoA_setting_t NoA_setting;
     * MrvlIEtypes_OPP_PS_setting_t OPP_PS_setting; */
} WLAN_PACK_STRUCT HOST_DS_WIFI_DIRECT_PARAM_CONFIG;

typedef struct {
    uint8_t operation;
    uint8_t operand_len;
    uint16_t offset;
    uint8_t operand_byte_stream[4];
} WLAN_PACK_STRUCT coalesce_filt_field_param;

typedef struct {
    MrvlIEtypesHeader_t header;
    uint8_t num_of_fields;
    uint8_t pkt_type;
    uint16_t max_coalescing_delay;
    coalesce_filt_field_param params[1];
} WLAN_PACK_STRUCT coalesce_receive_filt_rule;

typedef struct {
    uint16_t action;
    uint16_t num_of_rules;
    coalesce_receive_filt_rule rule[1];
} WLAN_PACK_STRUCT HOST_DS_COALESCE_CONFIG;

typedef struct {
    /* Wakeup reason
     * 0x0 - Unknown
     * 0x1 - Broadcast data matched
     * 0x2 - Multicast data matched
     * 0x3 - Unicast data matched
     * 0x4 - Maskable event matched
     * 0x5 - Non-maskable event matched
     * 0x6 - Non-maskable condition matched (EAPoL rekey)
     * 0x7 - Magic pattern matched
     * Others - Reserved (set to 0) */
    uint16_t wakeup_reason;
} WLAN_PACK_STRUCT HOST_DS_HS_WAKEUP_REASON;

typedef struct {
    /* Enable LPM */
    uint8_t enable;
} WLAN_PACK_STRUCT HOST_CONFIG_LOW_PWR_MODE;

typedef struct {
    /* Action */
    uint16_t action;
    /* Packet threshold */
    uint32_t packet_threshold;
    /* Timeout */
    uint16_t delay;
} WLAN_PACK_STRUCT HOST_DS_RX_PKT_COAL_CFG;

typedef struct {
    /* Header */
    MrvlIEtypesHeader_t header;
    /* EAPoL packet buffer */
    uint8_t pkt_buf[1];
} WLAN_PACK_STRUCT MrvlIEtypes_eapol_pkt_t;

typedef struct {
    /* Action */
    uint16_t action;
    /* TLV buffer */
    MrvlIEtypes_eapol_pkt_t tlv_eapol;
} WLAN_PACK_STRUCT HOST_DS_EAPOL_PKT;

typedef struct {
    /* SDIO packet length */
    uint16_t pack_len;
    /* SDIO packet type */
    uint16_t pack_type;
    /* Command */
    uint16_t command;
    /* Size */
    uint16_t size;
    /* Sequence number */
    uint8_t seq_num;
    /* BSS type */
    uint8_t bss;
    /* Result */
    uint16_t result;
    /* Command body */
    union {
        /* Hardware specifications */
        HOST_DS_GET_HW_SPEC hw_spec;
        /* Config data */
        HOST_DS_802_11_CFG_DATA cfg_data;
        /* MAC control */
        HOST_DS_MAC_CONTROL mac_ctrl;
        /* MAC address */
        HOST_DS_802_11_MAC_ADDR mac_addr;
        /* MAC muticast address */
        HOST_DS_MAC_MULTICAST_ADR mc_addr;
        /* Get log */
        HOST_DS_802_11_GET_LOG get_log;
        /* RSSI information */
        HOST_DS_802_11_RSSI_INFO rssi_info;
        /* RSSI information response */
        HOST_DS_802_11_RSSI_INFO_RSP rssi_info_rsp;
        /* SNMP MIB */
        HOST_DS_802_11_SNMP_MIB smib;
        /* Radio control */
        HOST_DS_802_11_RADIO_CONTROL radio;
        /* RF channel */
        HOST_DS_802_11_RF_CHANNEL rf_channel;
        /* Tx rate query */
        HOST_TX_RATE_QUERY tx_rate;
        /* Tx rate configuration */
        HOST_DS_TX_RATE_CFG tx_rate_cfg;
        /* Tx power configuration */
        HOST_DS_TXPWR_CFG txp_cfg;
        /* RF Tx power configuration */
        HOST_DS_802_11_RF_TX_POWER txp;
        /* RF antenna */
        HOST_DS_802_11_RF_ANTENNA antenna;
        /* Enhanced power save command */
        HOST_DS_802_11_PS_MODE_ENH psmode_enh;
        HOST_DS_802_11_HS_CFG_ENH opt_hs_cfg;
        /* Scan */
        HOST_DS_802_11_SCAN scan;
        /* Mgmt frame subtype mask */
        HOST_DS_RX_MGMT_IND rx_mgmt_ind;
        /* Scan response */
        HOST_DS_802_11_SCAN_RSP scan_resp;
        HOST_DS_802_11_BG_SCAN_CONFIG bg_scan_config;
        HOST_DS_802_11_BG_SCAN_QUERY bg_scan_query;
        HOST_DS_802_11_BG_SCAN_QUERY_RSP bg_scan_query_resp;
        HOST_DS_SUBSCRIBE_EVENT subscribe_event;
        HOST_DS_OTP_USER_DATA otp_user_data;
        /* Associate */
        HOST_DS_802_11_ASSOCIATE associate;
        /* Associate response */
        HOST_DS_802_11_ASSOCIATE_RSP associate_rsp;
        /* Deauthenticate */
        HOST_DS_802_11_DEAUTHENTICATE deauth;
        /* Ad-Hoc start */
        HOST_DS_802_11_AD_HOC_START adhoc_start;
        /* Ad-Hoc start result */
        HOST_DS_802_11_AD_HOC_START_RESULT adhoc_start_result;
        /* Ad-Hoc join result */
        HOST_DS_802_11_AD_HOC_JOIN_RESULT adhoc_join_result;
        /* Ad-Hoc join */
        HOST_DS_802_11_AD_HOC_JOIN adhoc_join;
        /* Domain information */
        HOST_DS_802_11D_DOMAIN_INFO domain_info;
        /* Domain information response */
        HOST_DS_802_11D_DOMAIN_INFO_RSP domain_info_resp;
        /* Add BA request */
        HOST_DS_11N_ADDBA_REQ add_ba_req;
        /* Add BA response */
        HOST_DS_11N_ADDBA_RSP add_ba_rsp;
        /* Delete BA entry */
        HOST_DS_11N_DELBA del_ba;
        /* Tx buffer configuration */
        HOST_DS_TXBUF_CFG tx_buf;
        /* AMSDU aggr control configuration */
        HOST_DS_AMSDU_AGGR_CTRL amsdu_aggr_ctrl;
        /* HT configuration */
        HOST_DS_11N_CFG htcfg;
        /* Reject ADDBA request configuration */
        HOST_DS_REJECT_ADDBA_REQ reject_addba_req;
        /* WMM get status */
        HOST_DS_WMM_GET_STATUS get_wmm_status;
        /* WMM ADDTS */
        HOST_DS_WMM_ADDTS_REQ add_ts;
        /* WMM DELTS */
        HOST_DS_WMM_DELTS_REQ del_ts;
        /* WMM set/get queue config */
        HOST_DS_WMM_QUEUE_CONFIG queue_config;
        /* WMM on/of/get queue statistics */
        HOST_DS_WMM_QUEUE_STATS queue_stats;
        /* WMM get traffic stream status */
        HOST_DS_WMM_TS_STATUS ts_status;
        /* Key material */
        HOST_DS_802_11_KEY_MATERIAL key_material;
        /* E-Supplicant PSK */
        HOST_DS_802_11_SUPPLICANT_PMK esupplicant_psk;
        /* E-Supplicant profile */
        HOST_DS_802_11_SUPPLICANT_PROFILE esupplicant_profile;
        /* Extended version */
        HOST_DS_VERSION_EXT verext;
        /* Ad-Hoc coalescing */
        HOST_DS_802_11_IBSS_STATUS ibss_coalescing;
        /* Mgmt IE list configuration */
        HOST_DS_MGMT_IE_LIST_CFG mgmt_ie_list;
        /* System clock configuration */
        HOST_DS_ECL_SYSTEM_CLOCK_CONFIG sys_clock_cfg;
        /* MAC register access */
        HOST_DS_MAC_REG_ACCESS mac_reg;
        /* BBP register access */
        HOST_DS_BBP_REG_ACCESS bbp_reg;
        /* RF register access */
        HOST_DS_RF_REG_ACCESS rf_reg;
        /* EEPROM register access */
        HOST_DS_802_11_EEPROM_ACCESS eeprom;
        /* Memory access */
        HOST_DS_MEM_ACCESS mem;
        /* Target device access */
        HOST_DS_TARGET_ACCESS target;
        /* Inactivity timeout */
        HOST_DS_INACTIVITY_TIMEOUT inactivity_timeout;
        HOST_DS_SYS_CONFIG sys_config;
        HOST_DS_SYS_INFO sys_info;
        HOST_DS_STA_DEAUTH sta_deauth;
        HOST_DS_STA_LIST sta_list;
        HOST_DS_POWER_MGMT_EXT pm_cfg;
        /* Sleep period command */
        HOST_DS_802_11_SLEEP_PERIOD sleep_pd;
        /* Sleep params command */
        HOST_DS_802_11_SLEEP_PARAMS sleep_params;
        /* SDIO GPIO interrupt config command */
        HOST_DS_SDIO_GPIO_INT_CONFIG sdio_gpio_int;
        HOST_DS_SDIO_PULL_CTRL sdio_pull_ctl;
        HOST_DS_SET_BSS_MODE bss_mode;
        HOST_DS_CMD_TX_DATA_PAUSE tx_data_pause;
        HOST_DS_REMAIN_ON_CHANNEL remain_on_chan;
        HOST_DS_WIFI_DIRECT_MODE wifi_direct_mode;
        HOST_DS_WIFI_DIRECT_PARAM_CONFIG p2p_params_config;
        HOST_DS_COALESCE_CONFIG coalesce_config;
        HOST_DS_HS_WAKEUP_REASON hs_wakeup_reason;
        HOST_CONFIG_LOW_PWR_MODE low_pwr_mode_cfg;
        HOST_DS_RX_PKT_COAL_CFG rx_pkt_coal_cfg;
        HOST_DS_EAPOL_PKT eapol_pkt;
    } params;
} WLAN_PACK_STRUCT HOST_DS_COMMAND;

/* Bit mask for TxPD status field for null packet */
#define MRVDRV_TxPD_POWER_MGMT_NULL_PACKET 0x1
/* Bit mask for TxPD status field for last packet */
#define MRVDRV_TxPD_POWER_MGMT_LAST_PACKET 0x8

/* Packet type 802.11 */
#define PKT_TYPE_802DOT11 0x5
/* Packet type mgmt frame */
#define PKT_TYPE_MGMT_FRAME 0xE5
/* Packet type AMSDU */
#define PKT_TYPE_AMSDU 0xE6
/* Packet type BAR */
#define PKT_TYPE_BAR 0xE7
/* Packet type debugging */
#define PKT_TYPE_DEBUG 0xEF

typedef struct {
    /* SDIO packet length */
    uint16_t pack_len;
    /* SDIO packet type */
    uint16_t pack_type;
    /* BSS type */
    uint8_t bss_type;
    /* BSS number */
    uint8_t bss_num;
    /* Tx packet length */
    uint16_t tx_pkt_length;
    /* Tx packet offset */
    uint16_t tx_pkt_offset;
    /* Tx packet type */
    uint16_t tx_pkt_type;
    /* Tx control */
    uint32_t tx_control;
    /* Packet priority */
    uint8_t priority;
    /* Transmit packet flags */
    uint8_t flags;
    /* Amount of time the packet has been queued in the driver in 2ms */
    uint8_t pkt_delay_2ms;
    /* Reserved */
    uint8_t reserved1;
    /* 数据链路层上的帧 */
    uint8_t payload[];
} WLAN_PACK_STRUCT TxPD;

typedef struct {
    /* SDIO packet length */
    uint16_t pack_len;
    /* SDIO packet type */
    uint16_t pack_type;
    /* BSS type */
    uint8_t bss_type;
    /* BSS number */
    uint8_t bss_num;
    /* Rx packet length */
    uint16_t rx_pkt_length;
    /* Rx packet offset */
    uint16_t rx_pkt_offset;
    /* Rx packet type */
    uint16_t rx_pkt_type;
    /* Sequence number */
    uint16_t seq_num;
    /* Packet priority */
    uint8_t priority;
    /* Rx packet rate */
    uint8_t rx_rate;
    /* SNR */
    int8_t snr;
    /* Noise floor */
    int8_t nf;
    /* HT info
     * [Bit 0] RxRate format: LG = 0, HT = 1
     * [Bit 1] HT bandwidth: BW20 = 0, BW40 = 1
     * [Bit 2] HT guard interval: LGI = 0, SGI = 1 */
    uint8_t ht_info;
    /* Reserved */
    uint8_t reserved;
    /* 数据链路层上的帧 */
    uint8_t payload[];
} WLAN_PACK_STRUCT RxPD;

typedef enum {
    CON_STATUS_NOT_CONNECTED,
    CON_STATUS_CONNECTING,
    CON_STATUS_CONNECTED
} con_status_e;

typedef struct {
    uint8_t ap_mac_addr[MAC_ADDR_LENGTH];
    uint8_t ssid[MAX_SSID_LENGTH];
    uint8_t ssid_len;
    uint8_t pwd[MAX_PHRASE_LENGTH];
    uint8_t pwd_len;
    wlan_security_type sec_type;
    con_status_e con_status;
    uint16_t cap_info;
} ap_info_t;

typedef struct {
    uint8_t used;
    uint8_t sta_mac_addr[MAC_ADDR_LENGTH];
} sta_info_t;

typedef struct {
    void (*wlan_cb_init)(core_err_e status);
    void (*wlan_cb_scan)(core_err_e status, uint8_t *ssid, uint8_t rssi, uint8_t channel, wlan_security_type sec_type);
    void (*wlan_cb_sta_connect)(core_err_e status);
    void (*wlan_cb_sta_disconnect)(void);
    void (*wlan_cb_ap_start)(void);
    void (*wlan_cb_ap_stop)(void);
    void (*wlan_cb_ap_connect)(uint8_t *name, uint8_t *mac, uint8_t *ip);
    void (*wlan_cb_ap_disconnect)(uint8_t *name, uint8_t *mac, uint8_t *ip);
} wlan_cb_t;

typedef struct {
    uint8_t mac_addr[MAC_ADDR_LENGTH];
    sta_info_t sta_info[MAX_CLIENT_NUM];
    ap_info_t ap_info;
    uint32_t ctrl_port;
    uint16_t mp_end_port;
    uint16_t read_bitmap;
    uint16_t write_bitmap;
    uint16_t curr_rd_port;
    uint16_t curr_wr_port;
} wlan_core_t;

uint8_t wlan_init(wlan_cb_t *callback);
uint8_t wlan_shutdown(void);
uint8_t wlan_process_packet(void);
uint8_t wlan_scan(uint8_t *channel, uint8_t channel_num, uint16_t max_time);
uint8_t wlan_scan_ssid(uint8_t *ssid, uint8_t ssid_len, uint16_t max_time);
uint8_t wlan_sta_connect(uint8_t *ssid, uint8_t ssid_len, uint8_t *pwd, uint8_t pwd_len);
uint8_t wlan_sta_disconnect(void);
uint8_t wlan_ap_start(uint8_t *ssid, uint8_t ssid_len, uint8_t *pwd, uint8_t pwd_len, wlan_security_type sec_type, bool broadcast_ssid);
uint8_t wlan_ap_stop(void);
void wlan_ap_show(void);
uint8_t wlan_ap_deauth(uint8_t *mac_addr);
uint8_t wlan_send_data(uint8_t *data_buf, uint16_t data_len, wlan_bss_type bss_type);
#endif
