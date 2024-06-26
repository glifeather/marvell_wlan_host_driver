#include <string.h>
#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_utils.h>
#include <stm32f4xx_ll_bus.h>
#include <stm32f4xx_ll_dma.h>
#include "88w8801_sdio.h"
#include "88w8801/88w8801.h"

#ifdef WLAN_SDIO_DEBUG
#include <stdio.h>
#define SDIO_DEBUG printf
#else
#define SDIO_DEBUG(...) \
    do { \
    } while (0)
#endif

static sdio_core_t sdio_core;

static uint8_t sdio_check_err(void);
static uint8_t sdio_cmd3(uint32_t param, uint32_t *resp);
static uint8_t sdio_cmd5(uint32_t param, uint32_t *resp, uint16_t retry_max);
static uint8_t sdio_cmd7(uint32_t param, uint32_t *resp);
static uint8_t sdio_cmd53_send(bool write, uint8_t func_num, uint32_t reg_addr, bool inc_addr, uint16_t block_count);
static uint8_t sdio_try_high_speed(void);
static uint8_t sdio_get_cis(uint8_t func_num, uint32_t *cis_pointer);
static uint8_t sdio_parse_cis(uint8_t func_num, uint32_t cis_pointer);

/**
 * @param PDN_GPIO_Port PDN所在GPIO端口
 * @param PDN_Pin PDN引脚
 * @return sdio_err_e中某一状态码
 * @brief 初始化SDIO
 */
uint8_t sdio_init(GPIO_TypeDef *PDN_GPIO_Port, uint32_t PDN_Pin) {
    /* 使用PDN引脚重置芯片 */
    LL_GPIO_ResetOutputPin(PDN_GPIO_Port, PDN_Pin);
    LL_mDelay(10);
    LL_GPIO_SetOutputPin(PDN_GPIO_Port, PDN_Pin);
    LL_mDelay(10);
    memset(&sdio_core, 0, sizeof(sdio_core_t));
    (sdio_core.func + SDIO_FUNC_0)->func_status = true;
    LL_APB2_GRP1_EnableClock(LL_APB2_GRP1_PERIPH_SDIO);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOC);
    LL_AHB1_GRP1_EnableClock(LL_AHB1_GRP1_PERIPH_GPIOD);
    LL_GPIO_InitTypeDef gpioStruct;
    gpioStruct.Pin = LL_GPIO_PIN_8 | LL_GPIO_PIN_9 | LL_GPIO_PIN_10 | LL_GPIO_PIN_11 | LL_GPIO_PIN_12;
    gpioStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    gpioStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpioStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpioStruct.Pull = LL_GPIO_PULL_UP;
    gpioStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOC, &gpioStruct);
    gpioStruct.Pin = LL_GPIO_PIN_2;
    gpioStruct.Mode = LL_GPIO_MODE_ALTERNATE;
    gpioStruct.Speed = LL_GPIO_SPEED_FREQ_VERY_HIGH;
    gpioStruct.OutputType = LL_GPIO_OUTPUT_PUSHPULL;
    gpioStruct.Pull = LL_GPIO_PULL_UP;
    gpioStruct.Alternate = LL_GPIO_AF_12;
    LL_GPIO_Init(GPIOD, &gpioStruct);
    /* 切换到1位总线，400kHz时钟 */
    SDIO_InitTypeDef sdioStruct = {0};
    sdioStruct.BusWide = SDIO_BUS_WIDE_1B;
    sdioStruct.HardwareFlowControl = SDIO_HARDWARE_FLOW_CONTROL_ENABLE;
    sdioStruct.ClockDiv = SDIO_INIT_CLK_DIV;
    SDIO_Init(SDIO, sdioStruct);
    SDIO_PowerState_ON(SDIO);
    __SDIO_ENABLE(SDIO);
    /**
     * 同时启用SDIO模式与DMA，因为写入同一寄存器至少需间隔3*SDIO_CK+2*PCLK2
     * 另外如果在CMD53中不停启用DMA，DMA将锁住
     */
    SDIO->DCTRL |= SDIO_DCTRL_SDIOEN | SDIO_DCTRL_DMAEN;
    /* 执行CMD5并设置OCR寄存器（3.2-3.4V） */
    uint8_t err;
    uint32_t cmd_resp;
    if ((err = sdio_cmd5(0, &cmd_resp, SDIO_RETRY_MAX)) || (err = sdio_cmd5(0x300000, &cmd_resp, SDIO_RETRY_MAX))) return err;
    /* 解析R4，获取Func编号 */
    sdio_core.func_total_num = SDIO_R4_FUNC_NUM(cmd_resp);
    /* 执行CMD3获取地址，获取R6 */
    if ((err = sdio_cmd3(0, &cmd_resp))) return err;
    /* 解析RCA并执行CMD7 */
    if ((err = sdio_cmd7(SDIO_R6_RCA(cmd_resp) << 16, &cmd_resp))) return err;
    /* 切换到4位总线，48MHz时钟 */
    if ((err = sdio_set_bus_width(SDIO_BUS_WIDTH_4)) || (err = sdio_try_high_speed())) return err;
    sdioStruct.ClockBypass = SDIO_CLOCK_BYPASS_ENABLE;
    sdioStruct.BusWide = SDIO_BUS_WIDE_4B;
    sdioStruct.ClockDiv = SDIO_TRANSFER_CLK_DIV;
    SDIO_Init(SDIO, sdioStruct);
    /* 解析CIS */
    if ((err = sdio_get_cis(SDIO_FUNC_0, &cmd_resp)) || (err = sdio_parse_cis(SDIO_FUNC_0, cmd_resp))) return err;
    /* 设置分块大小 */
    if ((err = sdio_set_block_size(SDIO_FUNC_0, SDIO_BLOCK_SIZE))) return err;
    /* 启用中断总开关 */
    if ((err = sdio_enable_mgr_int())) return err;
    for (uint8_t index = SDIO_FUNC_1; index < sdio_core.func_total_num; ++index) {
        if ((err = sdio_get_cis(index, &cmd_resp)) || (err = sdio_parse_cis(index, cmd_resp))) return err;
        if ((err = sdio_set_block_size(index, SDIO_BLOCK_SIZE))) return err;
        /* 启用Func */
        if ((err = sdio_enable_func(index))) return err;
        /* 启用中断 */
        if ((err = sdio_enable_func_int(index))) return err;
    }
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param func_num Func编号
 * @param reg_addr 寄存器地址
 * @param param CMD52参数
 * @param resp CMD52返回值
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD52
 */
uint8_t sdio_cmd52(bool write, uint8_t func_num, uint32_t reg_addr, uint8_t param, uint8_t *resp) {
    /**
     * CMD52参数格式
     * |R/W Flag|Func Num|RAW Flag|Reserved|Reg Addr|Reserved|Param|
     * |:------:|:------:|:------:|:------:|:------:|:------:|:---:|
     * |    1   |    3   |    1   |    1   |   17   |    1   |  8  |
     */
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = write << 31;
    sdioCmdStruct.Argument |= func_num << 28;
    sdioCmdStruct.Argument |= (write && resp) << 27;
    sdioCmdStruct.Argument |= reg_addr << 9;
    sdioCmdStruct.Argument |= param;
    sdioCmdStruct.CmdIndex = SDIO_CMD52;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令响应 */
    while (!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND)) if (sdio_check_err()) return SDIO_ERR_CMD52_FAILED;
    /* 清除命令响应标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
    if (resp) *resp = (uint8_t)SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param func_num Func编号
 * @param reg_addr 寄存器地址
 * @param inc_addr 地址是否累加
 * @param data_buf 数据缓冲区
 * @param data_len 缓冲区长度
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD53
 */
uint8_t sdio_cmd53(bool write, uint8_t func_num, uint32_t reg_addr, bool inc_addr, uint8_t *data_buf, uint32_t data_len) {
    if (!(sdio_core.func + func_num)->cur_block_size) return SDIO_ERR_EMPTY_BLOCK_SIZE;
    uint8_t err = SDIO_ERR_OK;
    /* 计算分块数 */
    data_len = (data_len + (sdio_core.func + func_num)->cur_block_size - 1) / (sdio_core.func + func_num)->cur_block_size;
    if (write) err = sdio_cmd53_send(write, func_num, reg_addr, inc_addr, data_len);
    /* 设置数据格式 */
    SDIO_DataInitTypeDef sdioDataStruct = {0};
    sdioDataStruct.DataTimeOut = SDIO_STOPTRANSFERTIMEOUT;
    sdioDataStruct.DataLength = data_len * (sdio_core.func + func_num)->cur_block_size;
    switch ((sdio_core.func + func_num)->cur_block_size) {
    case 0x1: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_1B; break;
    case 0x2: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_2B; break;
    case 0x4: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_4B; break;
    case 0x8: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_8B; break;
    case 0x10: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_16B; break;
    case 0x20: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_32B; break;
    case 0x40: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_64B; break;
    case 0x80: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_128B; break;
    case 0x100: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_256B; break;
    case 0x200: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_512B; break;
    case 0x400: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_1024B; break;
    case 0x800: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_2048B; break;
    case 0x1000: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_4096B; break;
    case 0x2000: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_8192B; break;
    case 0x4000: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_16384B; break;
    }
    sdioDataStruct.TransferDir = !write << SDIO_DCTRL_DTDIR_Pos;
    sdioDataStruct.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    sdioDataStruct.DPSM = SDIO_DPSM_ENABLE;
    SDIO_ConfigData(SDIO, &sdioDataStruct);
    /* 开启DMA */
    LL_DMA_InitTypeDef dmaStruct = {0};
    dmaStruct.PeriphOrM2MSrcAddress = (uint32_t)&SDIO->FIFO;
    dmaStruct.MemoryOrM2MDstAddress = (uint32_t)data_buf;
    dmaStruct.Direction = write << DMA_SxCR_DIR_Pos;
    dmaStruct.Mode = LL_DMA_MODE_PFCTRL;
    dmaStruct.PeriphOrM2MSrcIncMode = LL_DMA_PERIPH_NOINCREMENT;
    dmaStruct.MemoryOrM2MDstIncMode = LL_DMA_MEMORY_INCREMENT;
    dmaStruct.PeriphOrM2MSrcDataSize = LL_DMA_PDATAALIGN_WORD;
    dmaStruct.MemoryOrM2MDstDataSize = LL_DMA_MDATAALIGN_WORD;
    dmaStruct.Channel = LL_DMA_CHANNEL_4;
    dmaStruct.Priority = LL_DMA_PRIORITY_VERYHIGH;
    dmaStruct.FIFOMode = LL_DMA_FIFOMODE_ENABLE;
    dmaStruct.FIFOThreshold = LL_DMA_FIFOTHRESHOLD_FULL;
    dmaStruct.MemBurst = LL_DMA_MBURST_INC4;
    dmaStruct.PeriphBurst = LL_DMA_PBURST_INC4;
    LL_DMA_Init(DMA2, LL_DMA_STREAM_3, &dmaStruct);
    LL_DMA_EnableStream(DMA2, LL_DMA_STREAM_3);
    if (!write) err = sdio_cmd53_send(write, func_num, reg_addr, inc_addr, data_len);
    /* 等待数据传输 */
    while (!(err || __SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND))) if (sdio_check_err()) err = write + SDIO_ERR_CMD53_READ_FAILED;
    /* 清除数据传输标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
    /* 关闭DMA */
    LL_DMA_DeInit(DMA2, LL_DMA_STREAM_3);
    return err;
}

/**
 * @param cccr_version CCCR版本
 * @return sdio_err_e中某一状态码
 * @brief 通过读取CCCR寄存器地址0x0处获取CCCR版本
 * @brief |    [7:4]   |    [3:0]   |
 * @brief |:----------:|:----------:|
 * @brief |SDIO version|CCCR version|
 */
uint8_t sdio_get_cccr_version(uint8_t *cccr_version) {
    if (!cccr_version) return SDIO_ERR_INVALID_PARAMS;
    /**
     * | 0x0| 0x1| 0x2| 0x3| 0x4-0xF|
     * |:--:|:--:|:--:|:--:|:------:|
     * |1.00|1.10|2.00|3.00|Reserved|
     */
    uint8_t version;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_SDIO_VERSION, 0, &version)) return SDIO_ERR_GET_VERSION_FAILED;
    *cccr_version = version & 0xF;
    return SDIO_ERR_OK;
}

/**
 * @param sdio_version SDIO版本
 * @return sdio_err_e中某一状态码
 * @brief 通过读取CCCR寄存器地址0x0处获取SDIO版本
 * @brief |    [7:4]   |    [3:0]   |
 * @brief |:----------:|:----------:|
 * @brief |SDIO version|CCCR version|
 */
uint8_t sdio_get_sdio_version(uint8_t *sdio_version) {
    if (!sdio_version) return SDIO_ERR_INVALID_PARAMS;
    /**
     * | 0x0| 0x1|       0x2       | 0x3| 0x4| 0x5-0xF|
     * |:--:|:--:|:---------------:|:--:|:--:|:------:|
     * |1.00|1.10|1.20 (Unreleased)|2.00|3.00|Reserved|
     */
    uint8_t version;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_SDIO_VERSION, 0, &version)) return SDIO_ERR_GET_VERSION_FAILED;
    *sdio_version = version >> 4 & 0xF;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号启用指定Func
 */
uint8_t sdio_enable_func(uint8_t func_num) {
    if (func_num < SDIO_FUNC_1 || func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARAMS;
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, enable | 1 << func_num, NULL)) return SDIO_ERR_ENABLE_FUNC_FAILED;
    /* 等待Func准备好 */
    uint8_t ready = 0;
    while (!(ready & 1 << func_num)) if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_READY, 0, &ready)) return SDIO_ERR_ENABLE_FUNC_FAILED;
    /* 更新Func状态 */
    (sdio_core.func + func_num)->func_status = true;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号禁用指定Func
 */
uint8_t sdio_disable_func(uint8_t func_num) {
    if (func_num < SDIO_FUNC_1 || func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARAMS;
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, enable & ~(1 << func_num), NULL)) return SDIO_ERR_DISABLE_FUNC_FAILED;
    /* 更新Func状态 */
    (sdio_core.func + func_num)->func_status = false;
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 启用CCCR中断总开关
 */
uint8_t sdio_enable_mgr_int(void) {
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable | 0x1, NULL)) return SDIO_ERR_ENABLE_MGR_INT_FAILED;
    /* 更新中断总开关状态 */
    sdio_core.mgr_int_status = true;
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 禁用CCCR中断总开关
 */
uint8_t sdio_disable_mgr_int(void) {
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable & ~0x1, NULL)) return SDIO_ERR_DISABLE_MGR_INT_FAILED;
    /* 更新中断总开关状态 */
    sdio_core.mgr_int_status = false;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号启用指定Func中断
 */
uint8_t sdio_enable_func_int(uint8_t func_num) {
    if (func_num < SDIO_FUNC_1 || func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARAMS;
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable | 1 << func_num, NULL)) return SDIO_ERR_ENABLE_FUNC_INT_FAILED;
    /* 更新Func中断状态 */
    (sdio_core.func + func_num)->func_int_status = true;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号禁用指定Func中断
 */
uint8_t sdio_disable_func_int(uint8_t func_num) {
    if (func_num < SDIO_FUNC_1 || func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARAMS;
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable & ~(1 << func_num), NULL)) return SDIO_ERR_DISABLE_FUNC_INT_FAILED;
    /* 更新Func中断状态 */
    (sdio_core.func + func_num)->func_int_status = false;
    return SDIO_ERR_OK;
}

/**
 * @param int_pending 中断挂起状态
 * @return sdio_err_e中某一状态码
 * @brief 获取中断挂起状态
 */
uint8_t sdio_get_int_pending(uint8_t *int_pending) { return int_pending ? (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_PENDING, 0, int_pending) ? SDIO_ERR_GET_INT_PENDING_FAILED : SDIO_ERR_OK) : SDIO_ERR_INVALID_PARAMS; }

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 终止指定Func
 */
uint8_t sdio_set_func_abort(uint8_t func_num) { return func_num < SDIO_FUNC_NUM ? (sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, func_num, NULL) ? SDIO_ERR_SET_FUNC_ABORT_FAILED : SDIO_ERR_OK) : SDIO_ERR_INVALID_PARAMS; }

/**
 * @return sdio_err_e中某一状态码
 * @brief 重置SDIO
 */
uint8_t sdio_reset(void) { return sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, 0x8, NULL) ? SDIO_ERR_RESET_FAILED : SDIO_ERR_OK; }

/**
 * @param bus_width 数据宽度
 * @return sdio_err_e中某一状态码
 * @brief 设置SDIO数据宽度
 * @brief |0x0|   0x1  |0x2|       0x3       |
 * @brief |:-:|:------:|:-:|:---------------:|
 * @brief | 1 |Reserved| 4 |8 (Embedded SDIO)|
 */
uint8_t sdio_set_bus_width(bus_width_e bus_width) {
    uint8_t width;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_BUS_CONTROL, 0, &width) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_BUS_CONTROL, (width & ~0x3) | bus_width, NULL)) return SDIO_ERR_SET_BUS_WIDTH_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @param bus_width 数据宽度
 * @return sdio_err_e中某一状态码
 * @brief 获取SDIO数据宽度
 * @brief |0x0|   0x1  |0x2|       0x3       |
 * @brief |:-:|:------:|:-:|:---------------:|
 * @brief | 1 |Reserved| 4 |8 (Embedded SDIO)|
 */
uint8_t sdio_get_bus_width(bus_width_e *bus_width) { return bus_width ? (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_BUS_CONTROL, 0, bus_width) ? SDIO_ERR_GET_BUS_WIDTH_FAILED : SDIO_ERR_OK) : SDIO_ERR_INVALID_PARAMS; }

/**
 * @param func_num Func编号
 * @param block_size 分块大小
 * @return sdio_err_e中某一状态码
 * @brief 设置指定Func分块大小
 */
uint8_t sdio_set_block_size(uint8_t func_num, uint16_t block_size) {
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARAMS;
    if (sdio_cmd52(true, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_BLOCK_SIZE, (uint8_t)block_size, NULL) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_BLOCK_SIZE + 1, (uint8_t)(block_size >> 8), NULL)) return SDIO_ERR_EMPTY_BLOCK_SIZE;
    /* 更新Func结构体 */
    (sdio_core.func + func_num)->cur_block_size = block_size;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param block_size 分块大小
 * @return sdio_err_e中某一状态码
 * @brief 通过Func结构体获取分块大小
 */
uint8_t sdio_get_block_size(uint8_t func_num, uint16_t *block_size) {
    if (func_num >= SDIO_FUNC_NUM || !block_size) return SDIO_ERR_INVALID_PARAMS;
    *block_size = (sdio_core.func + func_num)->cur_block_size;
    return SDIO_ERR_OK;
}

/**
 * @return 错误数
 * @brief 检查错误并清除对应标志
 */
static uint8_t sdio_check_err(void) {
    uint8_t err = 0;
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CCRCFAIL)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CCRCFAIL);
        ++err;
        SDIO_DEBUG("Error: CMD%ld CRC failed\n", SDIO->CMD & SDIO_CMD_CMDINDEX);
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DCRCFAIL)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DCRCFAIL);
        ++err;
        SDIO_DEBUG("Error: Data CRC failed\n");
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CTIMEOUT)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CTIMEOUT);
        ++err;
        SDIO_DEBUG("Error: CMD%ld timeout\n", SDIO->CMD & SDIO_CMD_CMDINDEX);
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DTIMEOUT)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DTIMEOUT);
        ++err;
        SDIO_DEBUG("Error: Data timeout\n");
    }
    return err;
}

/**
 * @param param CMD3参数
 * @param resp CMD3返回值
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD3
 */
static uint8_t sdio_cmd3(uint32_t param, uint32_t *resp) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = param;
    sdioCmdStruct.CmdIndex = SDIO_CMD3;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令响应 */
    while (!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND)) if (sdio_check_err()) return SDIO_ERR_CMD3_FAILED;
    /* 清除命令响应标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
    if (resp) *resp = SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param param CMD5参数
 * @param resp CMD5返回值
 * @param retry_max 最大尝试次数
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD5
 */
static uint8_t sdio_cmd5(uint32_t param, uint32_t *resp, uint16_t retry_max) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = param;
    sdioCmdStruct.CmdIndex = SDIO_CMD5;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    while (retry_max--) {
        SDIO_SendCommand(SDIO, &sdioCmdStruct);
        /* 等待命令响应 */
        while (!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND)) if (sdio_check_err()) continue;
        /* 清除命令响应标志 */
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
        /* 判断是否成功 */
        if (!SDIO_R4_C(param = SDIO_GetResponse(SDIO, SDIO_RESP1))) continue;
        if (resp) *resp = param;
        return SDIO_ERR_OK;
    }
    return SDIO_ERR_CMD5_FAILED;
}

/**
 * @param param CMD7参数
 * @param resp CMD7返回值
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD7
 */
static uint8_t sdio_cmd7(uint32_t param, uint32_t *resp) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = param;
    sdioCmdStruct.CmdIndex = SDIO_CMD7;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令响应 */
    while (!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND)) if (sdio_check_err()) return SDIO_ERR_CMD7_FAILED;
    /* 清除命令响应标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
    if (resp) *resp = SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param func_num Func编号
 * @param reg_addr 寄存器地址
 * @param inc_addr 地址是否累加
 * @param block_count 分块数
 * @return sdio_err_e中某一状态码
 * @brief 发送CMD53并等待响应
 */
static uint8_t sdio_cmd53_send(bool write, uint8_t func_num, uint32_t reg_addr, bool inc_addr, uint16_t block_count) {
    /**
     * CMD53参数格式
     * |R/W Flag|Func Num|Block Mode|OP Code|Reg Addr|Byte/Block Count|
     * |:------:|:------:|:--------:|:-----:|:------:|:--------------:|
     * |    1   |    3   |     1    |   1   |   17   |        9       |
     */
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = write << 31;
    sdioCmdStruct.Argument |= func_num << 28;
    sdioCmdStruct.Argument |= 1 << 27;
    sdioCmdStruct.Argument |= inc_addr << 26;
    sdioCmdStruct.Argument |= reg_addr << 9;
    sdioCmdStruct.Argument |= block_count;
    sdioCmdStruct.CmdIndex = SDIO_CMD53;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令响应 */
    while (!__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND)) if (sdio_check_err()) return write + SDIO_ERR_CMD53_READ_FAILED;
    /* 清除命令响应标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 尝试将SDIO切换到高速模式
 */
static uint8_t sdio_try_high_speed(void) {
    uint8_t speed;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_BUS_SPEED_SELECT, 0, &speed) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_BUS_SPEED_SELECT, speed | 0x2, NULL)) return SDIO_ERR_BUS_SPEED_UNCHANGED;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param cis_pointer CIS指针
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号获取CIS
 */
static uint8_t sdio_get_cis(uint8_t func_num, uint32_t *cis_pointer) {
    if (func_num >= SDIO_FUNC_NUM || !cis_pointer) return SDIO_ERR_INVALID_PARAMS;
    /* 组合0x9-0xB处数据 */
    for (uint8_t index = 0; index < 3; ++index) if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_CIS_POINTER + index, 0, (uint8_t *)cis_pointer + index)) return SDIO_ERR_GET_CIS_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param cis_pointer CIS指针
 * @return sdio_err_e中某一状态码
 * @brief 解析CIS并存入Func结构体中
 */
static uint8_t sdio_parse_cis(uint8_t func_num, uint32_t cis_pointer) {
    // No SDIO card tuple can be longer than 0x101 bytes
    // 0x1 byte TPL_CODE + 0x1 byte TPL_LINK + 0xFF bytes tuple body
    uint8_t data[0xFF], tpl_code = CISTPL_NULL, tpl_link, index, len;
    while (tpl_code != CISTPL_END) {
        if (sdio_cmd52(false, SDIO_FUNC_0, cis_pointer++, 0, &tpl_code)) return SDIO_ERR_PARSE_CIS_FAILED;
        if (tpl_code == CISTPL_NULL) continue;
        /* 本节点长度 */
        if (sdio_cmd52(false, SDIO_FUNC_0, cis_pointer++, 0, &tpl_link)) return SDIO_ERR_PARSE_CIS_FAILED;
        for (index = 0; index < tpl_link; ++index) if (sdio_cmd52(false, SDIO_FUNC_0, cis_pointer + index, 0, data + index)) return SDIO_ERR_PARSE_CIS_FAILED;
        switch (tpl_code) {
        case CISTPL_VERS_1:
            SDIO_DEBUG("Product Info:");
            for (index = 2; *(data + index) != 0xFF; index += len + 1) {
                len = strlen((char *)data + index);
                if (len) SDIO_DEBUG(" %s", data + index);
            }
            SDIO_DEBUG("\n");
            break;
        case CISTPL_MANFID:
            // 16.6 CISTPL_MANFID: Manufacturer Identification String Tuple
            // TPLMID_MANF
            SDIO_DEBUG("Manufacturer Code: 0x%04X\n", *(uint16_t *)data);
            sdio_core.manf_code = *(uint16_t *)data;
            // TPLMID_CARD
            SDIO_DEBUG("Manufacturer Info: 0x%04X\n", *(uint16_t *)(data + 2));
            sdio_core.manf_info = *(uint16_t *)(data + 2);
            break;
        case CISTPL_FUNCID:
            // 16.7.1 CISTPL_FUNCID: Function Identification Tuple
            // TPLFID_FUNCTION
            SDIO_DEBUG("Card Function Code: 0x%02X\n", *data);
            // TPLFID_SYSINIT
            SDIO_DEBUG("System Init Bit Mask: 0x%02X\n", *(data + 1));
            break;
        case CISTPL_FUNCE:
            // 16.7.2 CISTPL_FUNCE: Function Extension Tuple
            if (!*data) {
                // 16.7.3 CISTPL_FUNCE Tuple for Function 0 (Extended Data 0x0)
                SDIO_DEBUG("[0x0] Max Block Size: Func %d, Size %d\n", func_num, *(uint16_t *)(data + 1));
                SDIO_DEBUG("[0x0] Max Transfer Rate Code: 0x%02X\n", *(data + 3));
                (sdio_core.func + func_num)->max_block_size = *(uint16_t *)(data + 1);
            } else {
                // 16.7.4 CISTPL_FUNCE Tuple for Function 1-7 (Extended Data 0x1)
                // TPLFE_MAX_BLOCK_SIZE
                SDIO_DEBUG("[0x1] Max Block Size: Func %d, Size %d\n", func_num, *(uint16_t *)(data + 0xC));
                (sdio_core.func + func_num)->max_block_size = *(uint16_t *)(data + 0xC);
            }
            break;
        default: SDIO_DEBUG("CIS Tuple 0x%02X: Addr 0x%08lX, Size %d\n", tpl_code, cis_pointer - 2, tpl_link); break;
        }
        /* 到达尾节点 */
        if (tpl_link == 0xFF) break;
        cis_pointer += tpl_link;
    }
    return SDIO_ERR_OK;
}
