/* Card Common Control Registers (CCCR)
 * Function Basic Registers (FBR)
 * Relative Card Address (RCA)
 * Card Information Structure (CIS) */

/* 命令 - 响应
 * CMD3 - R6
 * CMD5 - R4
 * CMD7 - R1b
 * CMD52 - R5 */
#ifndef _88W8801_SDIO_
#define _88W8801_SDIO_
#include <stm32f4xx.h>

typedef enum {
    SDIO_ERR_OK,
    SDIO_ERR_INVALID_PARA,
    SDIO_ERR_CMD3_FAILED,
    SDIO_ERR_CMD5_FAILED,
    SDIO_ERR_CMD7_FAILED,
    SDIO_ERR_CMD52_FAILED,
    SDIO_ERR_CMD53_READ_FAILED,
    SDIO_ERR_CMD53_WRITE_FAILED,
    SDIO_ERR_GET_VER_FAILED,
    SDIO_ERR_ENABLE_FUNC_FAILED,
    SDIO_ERR_DISABLE_FUNC_FAILED,
    SDIO_ERR_ENABLE_FUNC_INT_FAILED,
    SDIO_ERR_DISABLE_FUNC_INT_FAILED,
    SDIO_ERR_ENABLE_MGR_INT_FAILED,
    SDIO_ERR_DISABLE_MGR_INT_FAILED,
    SDIO_ERR_GET_PENDING_FAILED,
    SDIO_ERR_SET_ABORT_FAILED,
    SDIO_ERR_RESET_FAILED,
    SDIO_ERR_SET_BUS_WIDTH_FAILED,
    SDIO_ERR_GET_BUS_WIDTH_FAILED,
    SDIO_ERR_GET_CIS_PTR_FAILED,
    SDIO_ERR_EMPTY_BLK_SIZE,
    SDIO_ERR_CIS_PARSE_FAILED
} sdio_err_e;

/* SDIOCLK时钟/Hz */
#define SDIO_CLK_BASE 48000000
/* 最大尝试次数 */
#define SDIO_RETRY_MAX 100
/* 分块大小/B */
#define SDIO_BLK_SIZE 0x100
/* 超时时间/ms */
#define SDIO_DATATIMEOUT_BASE 100

/* SDIO_CK分频系数 */
#define SDIO_CLK_400KHZ (SDIO_CLK_BASE / 400000 - 2)
// #define SDIO_CLK_1MHZ (SDIO_CLK_BASE / 1000000 - 2)
#define SDIO_CLK_24MHZ (SDIO_CLK_BASE / 24000000 - 2)

/* 超时时间（以SDIO_CK计） */
// #define SDIO_DATATIMEOUT_1MHZ (SDIO_DATATIMEOUT_BASE * 1000)
#define SDIO_DATATIMEOUT_24MHZ (SDIO_DATATIMEOUT_BASE * 24000)

/* Func编号 */
#define SDIO_FUNC_0 0
#define SDIO_FUNC_1 1
// #define SDIO_FUNC_2 2
// #define SDIO_FUNC_3 3
// #define SDIO_FUNC_4 4
// #define SDIO_FUNC_5 5
// #define SDIO_FUNC_6 6
// #define SDIO_FUNC_7 7
#define SDIO_FUNC_NUM 8

/* 读写操作 */
#define SDIO_EXCU_READ 0
#define SDIO_EXCU_WRITE 1

/* R4响应 */
// #define OCR_IN_R4(x) ((x) & 0xFFFFFF)
// #define S18A_IN_R4(x) ((x) >> 24 & 0x1)
// #define MEM_IN_R4(x) ((x) >> 27 & 0x1)
#define FUNC_NUM_IN_R4(x) ((x) >> 28 & 0x7)
#define C_IN_R4(x) ((x) >> 31 & 0x1)

/* R6响应 */
// #define CS_IN_R6(x) ((x) & 0xFFFF)
#define RCA_IN_R6(x) ((x) >> 16 & 0xFFFF)

/* FBR寄存器地址 */
#define SDIO_FBR_BASE(x) ((x) * 0x100)

/* CMD命令 */
#define SDIO_CMD3 3
#define SDIO_CMD5 5
#define SDIO_CMD7 7
#define SDIO_CMD52 52
#define SDIO_CMD53 53

/* CCCR寄存器地址 */
#define SDIO_CCCR_SDIO_VERSION 0x0
// #define SDIO_CCCR_SD_VERSION 0x1
#define SDIO_CCCR_IO_ENABLE 0x2
#define SDIO_CCCR_IO_READY 0x3
#define SDIO_CCCR_INT_ENABLE 0x4
#define SDIO_CCCR_INT_PENDING 0x5
#define SDIO_CCCR_IO_ABORT 0x6
#define SDIO_CCCR_BUS_CONTROL 0x7
// #define SDIO_CCCR_CARD_CAP 0x8
#define SDIO_CCCR_CIS_PTR 0x9
// #define SDIO_CCCR_BUS_SUSPEND 0xC
// #define SDIO_CCCR_FUNC_SEL 0xD
// #define SDIO_CCCR_EXEC_FLAG 0xE
// #define SDIO_CCCR_READY_FLG 0xF
#define SDIO_CCCR_BLK_SIZE 0x10
// #define SDIO_CCCR_PWR_CONTROL 0x12
// #define SDIO_CCCR_BUS_SPEED_SEL 0x13
// #define SDIO_CCCR_UHSI_SUPPORT 0x14
// #define SDIO_CCCR_DRV_STRENGTH 0x15
// #define SDIO_CCCR_INT_EXTERN 0x16

typedef enum {
    CISTPL_NULL = 0x00,
    CISTPL_VERS_1 = 0x15,
    CISTPL_MANFID = 0x20,
    CISTPL_FUNCID = 0x21,
    CISTPL_FUNCE = 0x22,
    CISTPL_END = 0xFF
} cis_tuple_code_e;

typedef enum {
    SDIO_DISABLE,
    SDIO_ENABLE
} sdio_status_e;

typedef enum {
    SDIO_BUS_WIDTH_1 = 0x0,
    SDIO_BUS_WIDTH_4 = 0x2,
    SDIO_BUS_WIDTH_8 = 0x3
} bus_width_e;

typedef struct {
    uint8_t func_num;
    sdio_status_e func_status;
    sdio_status_e func_int_status;
    uint16_t cur_blk_size;
    uint16_t max_blk_size;
} sdio_func_t;

typedef struct {
    uint8_t func_total_num;
    uint8_t cccr_version;
    uint8_t sdio_version;
    uint16_t manf_code;
    uint16_t manf_info;
    sdio_status_e sdio_int_mgr;
    sdio_func_t func[SDIO_FUNC_NUM];
} sdio_core_t;

uint8_t sdio_init(GPIO_TypeDef *PDN_GPIO_Port, uint32_t PDN_Pin);
uint8_t sdio_get_cccr_version(uint8_t *cccr_version);
uint8_t sdio_get_sdio_version(uint8_t *sdio_version);
uint8_t sdio_enable_func(uint8_t func_num);
uint8_t sdio_disable_func(uint8_t func_num);
uint8_t sdio_enable_func_int(uint8_t func_num);
uint8_t sdio_disable_func_int(uint8_t func_num);
uint8_t sdio_enable_mgr_int(void);
uint8_t sdio_disable_mgr_int(void);
uint8_t sdio_get_int_pending(uint8_t *int_pending);
uint8_t sdio_set_func_abort(uint8_t func_num);
uint8_t sdio_reset(void);
uint8_t sdio_set_bus_width(bus_width_e bus_width);
uint8_t sdio_get_bus_width(bus_width_e *bus_width);
uint8_t sdio_get_cis_ptr(uint8_t func_num, uint32_t *cis_ptr);
uint8_t sdio_set_blk_size(uint8_t func_num, uint16_t blk_size);
uint8_t sdio_get_blk_size(uint8_t func_num, uint16_t *blk_size);
uint8_t sdio_cmd52(uint8_t write, uint8_t func_num, uint32_t address, uint8_t para, uint8_t *resp);
uint8_t sdio_cmd53(uint8_t write, uint8_t func_num, uint32_t address, uint8_t inc_addr, uint8_t *buf, uint32_t size);
#endif
