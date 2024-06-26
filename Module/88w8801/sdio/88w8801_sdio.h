/**
 * CCCR: Card Common Control Registers
 * FBR: Function Basic Registers
 * RCA: Relative Card Address
 * CIS: Card Information Structure
 * |Command|Response|
 * |:-----:|:------:|
 * | CMD3  |   R6   |
 * | CMD5  |   R4   |
 * | CMD7  |   R1b  |
 * | CMD52 |   R5   |
 * | CMD53 |   R5   |
 */
#ifndef _88W8801_SDIO_
#define _88W8801_SDIO_
#include <stdbool.h>
#include <stm32f4xx.h>

typedef enum {
    SDIO_ERR_OK,
    SDIO_ERR_INVALID_PARAMS,
    SDIO_ERR_CMD3_FAILED,
    SDIO_ERR_CMD5_FAILED,
    SDIO_ERR_CMD7_FAILED,
    SDIO_ERR_CMD52_FAILED,
    SDIO_ERR_CMD53_READ_FAILED,
    SDIO_ERR_CMD53_WRITE_FAILED,
    SDIO_ERR_GET_VERSION_FAILED,
    SDIO_ERR_ENABLE_FUNC_FAILED,
    SDIO_ERR_DISABLE_FUNC_FAILED,
    SDIO_ERR_ENABLE_MGR_INT_FAILED,
    SDIO_ERR_DISABLE_MGR_INT_FAILED,
    SDIO_ERR_ENABLE_FUNC_INT_FAILED,
    SDIO_ERR_DISABLE_FUNC_INT_FAILED,
    SDIO_ERR_GET_INT_PENDING_FAILED,
    SDIO_ERR_SET_FUNC_ABORT_FAILED,
    SDIO_ERR_RESET_FAILED,
    SDIO_ERR_SET_BUS_WIDTH_FAILED,
    SDIO_ERR_GET_BUS_WIDTH_FAILED,
    SDIO_ERR_EMPTY_BLOCK_SIZE,
    SDIO_ERR_GET_CIS_FAILED,
    SDIO_ERR_PARSE_CIS_FAILED
} sdio_err_e;

/* 最大尝试次数 */
#define SDIO_RETRY_MAX 100
/* 分块大小/B */
#define SDIO_BLOCK_SIZE 0x100

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

/* R4响应 */
// #define SDIO_R4_OCR(x) ((x) & 0xFFFFFF)
// #define SDIO_R4_S18A(x) ((x) >> 24 & 0x1)
// #define SDIO_R4_MEM(x) ((x) >> 27 & 0x1)
#define SDIO_R4_FUNC_NUM(x) ((x) >> 28 & 0x7)
#define SDIO_R4_C(x) ((x) >> 31 & 0x1)

/* R6响应 */
// #define SDIO_R6_CS(x) ((x) & 0xFFFF)
#define SDIO_R6_RCA(x) ((x) >> 16 & 0xFFFF)

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
#define SDIO_CCCR_CIS_POINTER 0x9
// #define SDIO_CCCR_BUS_SUSPEND 0xC
// #define SDIO_CCCR_FUNC_SELECT 0xD
// #define SDIO_CCCR_EXEC_FLAGS 0xE
// #define SDIO_CCCR_READY_FLAGS 0xF
#define SDIO_CCCR_BLOCK_SIZE 0x10
// #define SDIO_CCCR_POWER_CONTROL 0x12
// #define SDIO_CCCR_BUS_SPEED_SELECT 0x13
// #define SDIO_CCCR_UHSI_SUPPORT 0x14
// #define SDIO_CCCR_DRIVER_STRENGTH 0x15
// #define SDIO_CCCR_INT_EXTENSION 0x16

typedef enum {
    CISTPL_NULL = 0x00,
    CISTPL_VERS_1 = 0x15,
    CISTPL_MANFID = 0x20,
    CISTPL_FUNCID = 0x21,
    CISTPL_FUNCE = 0x22,
    CISTPL_END = 0xFF
} cis_tuple_code_e;

typedef enum {
    SDIO_BUS_WIDTH_1 = 0x0,
    SDIO_BUS_WIDTH_4 = 0x2,
    SDIO_BUS_WIDTH_8 = 0x3
} bus_width_e;

typedef struct {
    bool func_status;
    bool func_int_status;
    uint16_t cur_block_size;
    uint16_t max_block_size;
} sdio_func_t;

typedef struct {
    uint16_t manf_code;
    uint16_t manf_info;
    bool mgr_int_status;
    uint8_t func_total_num;
    sdio_func_t func[SDIO_FUNC_NUM];
} sdio_core_t;

uint8_t sdio_init(GPIO_TypeDef *PDN_GPIO_Port, uint32_t PDN_Pin);
uint8_t sdio_cmd52(bool write, uint8_t func_num, uint32_t reg_addr, uint8_t param, uint8_t *resp);
uint8_t sdio_cmd53(bool write, uint8_t func_num, uint32_t reg_addr, bool inc_addr, uint8_t *data_buf, uint32_t data_len);
uint8_t sdio_get_cccr_version(uint8_t *cccr_version);
uint8_t sdio_get_sdio_version(uint8_t *sdio_version);
uint8_t sdio_enable_func(uint8_t func_num);
uint8_t sdio_disable_func(uint8_t func_num);
uint8_t sdio_enable_mgr_int(void);
uint8_t sdio_disable_mgr_int(void);
uint8_t sdio_enable_func_int(uint8_t func_num);
uint8_t sdio_disable_func_int(uint8_t func_num);
uint8_t sdio_get_int_pending(uint8_t *int_pending);
uint8_t sdio_set_func_abort(uint8_t func_num);
uint8_t sdio_reset(void);
uint8_t sdio_set_bus_width(bus_width_e bus_width);
uint8_t sdio_get_bus_width(bus_width_e *bus_width);
uint8_t sdio_set_block_size(uint8_t func_num, uint16_t block_size);
uint8_t sdio_get_block_size(uint8_t func_num, uint16_t *block_size);
#endif
