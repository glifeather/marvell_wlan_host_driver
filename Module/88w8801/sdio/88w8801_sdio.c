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

static uint8_t sdio_cis_parse(uint8_t func_num, uint32_t cis_ptr);
static uint8_t sdio_check_err(void);
static uint8_t sdio_cmd3(uint32_t para, uint32_t *resp);
static uint8_t sdio_cmd5(uint32_t para, uint32_t *resp, uint16_t retry_max);
static uint8_t sdio_cmd7(uint32_t para, uint32_t *resp);
static void sdio_cmd53_dma(bool write, uint8_t *buf);

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
    SDIO_InitTypeDef sdioStruct = {0};
    sdioStruct.BusWide = SDIO_BUS_WIDE_1B;
    sdioStruct.ClockDiv = SDIO_INIT_CLK_DIV;
    SDIO_Init(SDIO, sdioStruct);
    SDIO_PowerState_ON(SDIO);
    __SDIO_ENABLE(SDIO);
    /**
     * 同时启用SDIO模式与DMA，因为写入同一寄存器至少需间隔3*SDIO_CK+2*PCLK2
     * 另外如果在CMD53中不停启用DMA，DMA将锁住
     */
    SDIO->DCTRL |= SDIO_DCTRL_SDIOEN | SDIO_DCTRL_DMAEN;
    /* 发送CMD5并设置OCR寄存器（3.2-3.4V） */
    uint32_t cmd_resp;
    uint8_t err;
    if ((err = sdio_cmd5(0, &cmd_resp, SDIO_RETRY_MAX)) || (err = sdio_cmd5(0x300000, &cmd_resp, SDIO_RETRY_MAX))) return err;
    /* 解析R4，获取Func编号 */
    sdio_core.func_total_num = FUNC_NUM_IN_R4(cmd_resp);
    uint8_t index;
    for (index = SDIO_FUNC_0; index < sdio_core.func_total_num; ++index) (sdio_core.func + index)->func_num = index;
    /* 发送CMD3获取地址，获取R6 */
    if ((err = sdio_cmd3(0, &cmd_resp))) return err;
    /* 解析RCA并发送CMD7 */
    if ((err = sdio_cmd7(RCA_IN_R6(cmd_resp) << 16, &cmd_resp))) return err;
    /* 获取CCCR版本和SDIO版本 */
    if ((err = sdio_get_cccr_version(&sdio_core.cccr_version)) || (err = sdio_get_sdio_version(&sdio_core.sdio_version))) return err;
    /* 切换到4位总线，24MHz时钟 */
    if ((err = sdio_set_bus_width(SDIO_BUS_WIDTH_4))) return err;
    sdioStruct.BusWide = SDIO_BUS_WIDE_4B;
    sdioStruct.ClockDiv = SDIO_TRANSFER_CLK_DIV;
    SDIO_Init(SDIO, sdioStruct);
    /* 解析每个Func的CIS指针 */
    for (index = SDIO_FUNC_0; index < sdio_core.func_total_num; ++index) if ((err = sdio_get_cis_ptr(index, &cmd_resp)) || (err = sdio_cis_parse(index, cmd_resp))) return err;
    /* 启用Func */
    for (index = SDIO_FUNC_1; index < sdio_core.func_total_num; ++index) if ((err = sdio_enable_func(index))) return err;
    /* 启用中断 */
    if ((err = sdio_enable_mgr_int())) return err;
    for (index = SDIO_FUNC_1; index < sdio_core.func_total_num; ++index) if ((err = sdio_enable_func_int(index))) return err;
    /* 设置分块大小 */
    for (index = SDIO_FUNC_1; index < sdio_core.func_total_num; ++index) if ((err = sdio_set_blk_size(index, SDIO_BLK_SIZE))) return err;
    return SDIO_ERR_OK;
}

/**
 * @param cccr_version CCCR版本
 * @return sdio_err_e中某一状态码
 * @brief 通过读取CCCR寄存器地址0x0处获取CCCR版本
 * @brief |   7 6 5 4  |   3 2 1 0  |
 * @brief |:----------:|:----------:|
 * @brief |SDIO version|CCCR version|
 */
uint8_t sdio_get_cccr_version(uint8_t *cccr_version) {
    if (!cccr_version) return SDIO_ERR_INVALID_PARA;
    /**
     * 读取CCCR0
     * | 00h| 01h| 02h| 03h| 04h-0Fh|
     * |:--:|:--:|:--:|:--:|:------:|
     * |1.00|1.10|2.00|3.00|Reserved|
     */
    uint8_t version;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_SDIO_VERSION, 0, &version)) return SDIO_ERR_GET_VER_FAILED;
    *cccr_version = version & 0xF;
    return SDIO_ERR_OK;
}

/**
 * @param sdio_version SDIO版本
 * @return sdio_err_e中某一状态码
 * @brief 通过读取CCCR寄存器地址0x0处获取SDIO版本
 * @brief |   7 6 5 4  |   3 2 1 0  |
 * @brief |:----------:|:----------:|
 * @brief |SDIO version|CCCR version|
 */
uint8_t sdio_get_sdio_version(uint8_t *sdio_version) {
    if (!sdio_version) return SDIO_ERR_INVALID_PARA;
    /**
     * 读取CCCR0
     * | 00h| 01h|       02h       | 03h| 04h| 05h-0Fh|
     * |:--:|:--:|:---------------:|:--:|:--:|:------:|
     * |1.00|1.10|1.20 (Unreleased)|2.00|3.00|Reserved|
     */
    uint8_t version;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_SDIO_VERSION, 0, &version)) return SDIO_ERR_GET_VER_FAILED;
    *sdio_version = version >> 4 & 0xF;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号启用指定Func
 */
uint8_t sdio_enable_func(uint8_t func_num) {
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARA;
    /* 修改CCCR2 */
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
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARA;
    /* 修改CCCR2 */
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ENABLE, enable & ~(1 << func_num), NULL)) return SDIO_ERR_DISABLE_FUNC_FAILED;
    /* 更新Func状态 */
    (sdio_core.func + func_num)->func_status = false;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号启用指定Func中断
 */
uint8_t sdio_enable_func_int(uint8_t func_num) {
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARA;
    /* 修改CCCR4 */
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
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARA;
    /* 修改CCCR4 */
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable & ~(1 << func_num), NULL)) return SDIO_ERR_DISABLE_FUNC_INT_FAILED;
    /* 更新Func中断状态 */
    (sdio_core.func + func_num)->func_int_status = false;
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 启用CCCR中断总开关
 */
uint8_t sdio_enable_mgr_int(void) {
    /* 修改CCCR4，中断总开关位于bit0处 */
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable | 0x1, NULL)) return SDIO_ERR_ENABLE_MGR_INT_FAILED;
    /* 更新中断总开关状态 */
    sdio_core.sdio_int_mgr = true;
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 禁用CCCR中断总开关
 */
uint8_t sdio_disable_mgr_int(void) {
    /* 修改CCCR4，中断总开关位于bit0处 */
    uint8_t enable;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, 0, &enable) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_INT_ENABLE, enable & ~0x1, NULL)) return SDIO_ERR_DISABLE_MGR_INT_FAILED;
    /* 更新中断总开关状态 */
    sdio_core.sdio_int_mgr = false;
    return SDIO_ERR_OK;
}

/**
 * @param int_pending 中断挂起状态
 * @return sdio_err_e中某一状态码
 * @brief 获取中断挂起状态
 */
uint8_t sdio_get_int_pending(uint8_t *int_pending) {
    if (!int_pending) return SDIO_ERR_INVALID_PARA;
    /* 读取PENDING */
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_INT_PENDING, 0, int_pending)) return SDIO_ERR_GET_PENDING_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @return sdio_err_e中某一状态码
 * @brief 终止指定Func
 */
uint8_t sdio_set_func_abort(uint8_t func_num) {
    if (func_num >= SDIO_FUNC_NUM) return SDIO_ERR_INVALID_PARA;
    /* 修改ABORT */
    uint8_t abort;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, 0, &abort) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, abort | func_num, NULL)) return SDIO_ERR_SET_ABORT_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @return sdio_err_e中某一状态码
 * @brief 重置SDIO
 */
uint8_t sdio_reset(void) {
    /* 修改ABORT，重置标志位于bit3处 */
    uint8_t abort;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, 0, &abort) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_CCCR_IO_ABORT, abort | 0x8, NULL)) return SDIO_ERR_RESET_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @param bus_width 数据宽度
 * @return sdio_err_e中某一状态码
 * @brief 设置SDIO数据宽度
 * @brief | 00b |   01b  |   10b   |           11b           |
 * @brief |:---:|:------:|:-------:|:-----------------------:|
 * @brief |1-bit|Reserved|4-bit bus|8-bit bus (Embedded SDIO)|
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
 * @brief | 00b |   01b  |   10b   |           11b           |
 * @brief |:---:|:------:|:-------:|:-----------------------:|
 * @brief |1-bit|Reserved|4-bit bus|8-bit bus (Embedded SDIO)|
 */
uint8_t sdio_get_bus_width(bus_width_e *bus_width) {
    if (!bus_width) return SDIO_ERR_INVALID_PARA;
    if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_CCCR_BUS_CONTROL, 0, bus_width)) return SDIO_ERR_GET_BUS_WIDTH_FAILED;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param cis_ptr CIS指针
 * @return sdio_err_e中某一状态码
 * @brief 根据Func编号获取CIS指针
 */
uint8_t sdio_get_cis_ptr(uint8_t func_num, uint32_t *cis_ptr) {
    if (func_num >= SDIO_FUNC_NUM || !cis_ptr) return SDIO_ERR_INVALID_PARA;
    /* 组合0x9-0xB处数据得到CIS指针 */
    *cis_ptr = 0;
    for (uint8_t index = 0, ptr; index < 3; ++index) {
        if (sdio_cmd52(false, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_CIS_PTR + index, 0, &ptr)) return SDIO_ERR_GET_CIS_PTR_FAILED;
        *cis_ptr |= ptr << index * 8;
    }
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param blk_size 分块大小
 * @return sdio_err_e中某一状态码
 * @brief 设置指定Func分块大小
 */
uint8_t sdio_set_blk_size(uint8_t func_num, uint16_t blk_size) {
    /* 设置分块大小 */
    if (sdio_cmd52(true, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_BLK_SIZE, (uint8_t)blk_size, NULL) || sdio_cmd52(true, SDIO_FUNC_0, SDIO_FBR_BASE(func_num) + SDIO_CCCR_BLK_SIZE + 1, (uint8_t)(blk_size >> 8), NULL)) return SDIO_ERR_EMPTY_BLK_SIZE;
    /* 更新Func结构体 */
    (sdio_core.func + func_num)->cur_blk_size = blk_size;
    return SDIO_ERR_OK;
}

/**
 * @param func_num Func编号
 * @param blk_size 分块大小
 * @return sdio_err_e中某一状态码
 * @brief 通过Func结构体获取分块大小
 */
uint8_t sdio_get_blk_size(uint8_t func_num, uint16_t *blk_size) {
    if (!blk_size) return SDIO_ERR_INVALID_PARA;
    *blk_size = (sdio_core.func + func_num)->cur_blk_size;
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param func_num Func编号
 * @param address 地址
 * @param para 写入的参数
 * @param resp 读取的数据
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD52
 */
uint8_t sdio_cmd52(bool write, uint8_t func_num, uint32_t address, uint8_t para, uint8_t *resp) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = write ? 0x80000000 : 0x0;
    sdioCmdStruct.Argument |= func_num << 28;
    sdioCmdStruct.Argument |= write && resp ? 0x8000000 : 0x0;
    sdioCmdStruct.Argument |= address << 9;
    sdioCmdStruct.Argument |= para;
    sdioCmdStruct.CmdIndex = SDIO_CMD52;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令发送 */
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT));
    if (sdio_check_err()) return SDIO_ERR_CMD52_FAILED;
    if (!write && resp) *resp = (uint8_t)SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param func_num Func编号
 * @param address 地址
 * @param inc_addr 地址是否累加
 * @param buf 读/写缓冲区
 * @param size 读/写数据长度
 * @return sdio_err_e中某一状态码
 * @brief 执行CMD53
 */
uint8_t sdio_cmd53(bool write, uint8_t func_num, uint32_t address, uint8_t inc_addr, uint8_t *buf, uint32_t size) {
    if (!(sdio_core.func + func_num)->cur_blk_size) return SDIO_ERR_EMPTY_BLK_SIZE;
    uint8_t err = SDIO_ERR_OK;
    /* 读取操作开启DMA */
    if (!write) sdio_cmd53_dma(write, buf);
    /**
     * CMD53参数格式
     * |R/W Flag|Func Num|Blk Mode|OP Code|Reg Addr|Byte/Blk Cnt|
     * |:------:|:------:|:------:|:-----:|:------:|:----------:|
     * | 1 byte | 3 bytes| 1 byte | 1 byte|17 bytes|   9 bytes  |
     */
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = write ? 0x80000000 : 0x0;
    sdioCmdStruct.Argument |= func_num << 28;
    sdioCmdStruct.Argument |= 0x8000000;
    /* OP Code: 1地址递增，0固定地址 */
    sdioCmdStruct.Argument |= inc_addr ? 0x4000000 : 0x0;
    sdioCmdStruct.Argument |= address << 9;
    sdioCmdStruct.Argument |= (size + (sdio_core.func + func_num)->cur_blk_size - 1) / (sdio_core.func + func_num)->cur_blk_size;
    sdioCmdStruct.CmdIndex = SDIO_CMD53;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令响应 */
    while (!(err || __SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDREND))) if (sdio_check_err()) err = write + SDIO_ERR_CMD53_READ_FAILED;
    /* 清除命令响应标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CMDREND);
    /* 写入操作开启DMA */
    if (write) sdio_cmd53_dma(write, buf);
    /* 开始数据传输 */
    SDIO_DataInitTypeDef sdioDataStruct;
    sdioDataStruct.DataTimeOut = SDIO_STOPTRANSFERTIMEOUT;
    sdioDataStruct.DataLength = (sdioCmdStruct.Argument & 0x1FF) * (sdio_core.func + func_num)->cur_blk_size;
    switch ((sdio_core.func + func_num)->cur_blk_size) {
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
    default: sdioDataStruct.DataBlockSize = SDIO_DATABLOCK_SIZE_1B; break;
    }
    sdioDataStruct.TransferDir = write ? SDIO_TRANSFER_DIR_TO_CARD : SDIO_TRANSFER_DIR_TO_SDIO;
    sdioDataStruct.TransferMode = SDIO_TRANSFER_MODE_BLOCK;
    sdioDataStruct.DPSM = SDIO_DPSM_ENABLE;
    SDIO_ConfigData(SDIO, &sdioDataStruct);
    /* 等待数据传输 */
    while (!(err || __SDIO_GET_FLAG(SDIO, SDIO_FLAG_DATAEND))) if (sdio_check_err()) err = write + SDIO_ERR_CMD53_READ_FAILED;
    /* 清除数据传输标志 */
    __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DATAEND | SDIO_FLAG_DBCKEND);
    /* 关闭DMA */
    LL_DMA_DeInit(DMA2, LL_DMA_STREAM_3);
    LL_DMA_DisableStream(DMA2, LL_DMA_STREAM_3);
    LL_DMA_ClearFlag_TC3(DMA2);
    return err;
}

/**
 * @param func_num Func编号
 * @param cis_ptr CIS指针
 * @return sdio_err_e中某一状态码
 * @brief 解析CIS并存入Func结构体中
 */
static uint8_t sdio_cis_parse(uint8_t func_num, uint32_t cis_ptr) {
    // No SDIO card tuple can be longer than 257 bytes
    // 1 byte TPL_CODE + 1 byte TPL_LINK + FFh bytes tuple body
    uint8_t data[0xFF], tpl_code = CISTPL_NULL, tpl_link, index, len;
    while (tpl_code != CISTPL_END) {
        if (sdio_cmd52(false, SDIO_FUNC_0, cis_ptr++, 0, &tpl_code)) return SDIO_ERR_CIS_PARSE_FAILED;
        if (tpl_code == CISTPL_NULL) continue;
        /* 本节点数据大小 */
        if (sdio_cmd52(false, SDIO_FUNC_0, cis_ptr++, 0, &tpl_link)) return SDIO_ERR_CIS_PARSE_FAILED;
        for (index = 0; index < tpl_link; ++index) if (sdio_cmd52(false, SDIO_FUNC_0, cis_ptr + index, 0, data + index)) return SDIO_ERR_CIS_PARSE_FAILED;
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
                // 16.7.3 CISTPL_FUNCE Tuple for Function 0 (Extended Data 00h)
                SDIO_DEBUG("Max Block Size Case1: Func %d, Size %d\n", func_num, *(uint16_t *)(data + 1));
                SDIO_DEBUG("Max Transfer Rate Code: 0x%02X\n", *(data + 3));
                (sdio_core.func + func_num)->max_blk_size = *(uint16_t *)(data + 1);
            } else {
                // 16.7.4 CISTPL_FUNCE Tuple for Function 1-7 (Extended Data 01h)
                // TPLFE_MAX_BLK_SIZE
                SDIO_DEBUG("Max Block Size Case2: Func %d, Size %d\n", func_num, *(uint16_t *)(data + 0xC));
                (sdio_core.func + func_num)->max_blk_size = *(uint16_t *)(data + 0xC);
            }
            break;
        default: SDIO_DEBUG("CIS Tuple 0x%02X: Addr 0x%08lX, Size %d\n", tpl_code, cis_ptr - 2, tpl_link); break;
        }
        /* TPL_LINK为0xFF时说明当前节点为尾节点 */
        if (tpl_link == 0xFF) break;
        cis_ptr += tpl_link;
    }
    return SDIO_ERR_OK;
}

/**
 * @return 错误数量
 * @brief 判断错误类型并清除对应标志
 */
static uint8_t sdio_check_err(void) {
    uint8_t err = 0;
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CCRCFAIL)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CCRCFAIL);
        ++err;
        SDIO_DEBUG("Error: CMD%ld CRC failed\n", (uint32_t)(SDIO->CMD & SDIO_CMD_CMDINDEX));
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CTIMEOUT)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_CTIMEOUT);
        ++err;
        SDIO_DEBUG("Error: CMD%ld timeout\n", (uint32_t)(SDIO->CMD & SDIO_CMD_CMDINDEX));
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DCRCFAIL)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DCRCFAIL);
        ++err;
        SDIO_DEBUG("Error: Data CRC failed\n");
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_DTIMEOUT)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_DTIMEOUT);
        ++err;
        SDIO_DEBUG("Error: Data timeout\n");
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_TXUNDERR)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_TXUNDERR);
        ++err;
        SDIO_DEBUG("Error: Data underrun\n");
    }
    if (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_RXOVERR)) {
        __SDIO_CLEAR_FLAG(SDIO, SDIO_FLAG_RXOVERR);
        ++err;
        SDIO_DEBUG("Error: Data overrun\n");
    }
    return err;
}

/**
 * @param para CMD3参数
 * @param resp CMD3返回值
 * @return sdio_err_e中某一状态码
 * @brief 发送CMD3
 */
static uint8_t sdio_cmd3(uint32_t para, uint32_t *resp) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = para;
    sdioCmdStruct.CmdIndex = SDIO_CMD3;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令发送 */
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT));
    if (sdio_check_err()) return SDIO_ERR_CMD3_FAILED;
    if (resp) *resp = SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param para CMD5参数
 * @param resp CMD5返回值
 * @param retry_max 最大尝试次数
 * @return sdio_err_e中某一状态码
 * @brief 发送CMD5
 */
static uint8_t sdio_cmd5(uint32_t para, uint32_t *resp, uint16_t retry_max) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = para;
    sdioCmdStruct.CmdIndex = SDIO_CMD5;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    for (uint16_t index = 0; index < retry_max; ++index) {
        SDIO_SendCommand(SDIO, &sdioCmdStruct);
        /* 等待命令发送 */
        while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT));
        if (sdio_check_err()) continue;
        /* 判断是否成功 */
        para = SDIO_GetResponse(SDIO, SDIO_RESP1);
        if (!C_IN_R4(para)) continue;
        if (resp) *resp = para;
        return SDIO_ERR_OK;
    }
    return SDIO_ERR_CMD5_FAILED;
}

/**
 * @param para CMD7参数
 * @param resp CMD7返回值
 * @return sdio_err_e中某一状态码
 * @brief 发送CMD7
 */
static uint8_t sdio_cmd7(uint32_t para, uint32_t *resp) {
    SDIO_CmdInitTypeDef sdioCmdStruct;
    sdioCmdStruct.Argument = para;
    sdioCmdStruct.CmdIndex = SDIO_CMD7;
    sdioCmdStruct.Response = SDIO_RESPONSE_SHORT;
    sdioCmdStruct.WaitForInterrupt = SDIO_WAIT_NO;
    sdioCmdStruct.CPSM = SDIO_CPSM_ENABLE;
    SDIO_SendCommand(SDIO, &sdioCmdStruct);
    /* 等待命令发送 */
    while (__SDIO_GET_FLAG(SDIO, SDIO_FLAG_CMDACT));
    if (sdio_check_err()) return SDIO_ERR_CMD7_FAILED;
    if (resp) *resp = SDIO_GetResponse(SDIO, SDIO_RESP1);
    return SDIO_ERR_OK;
}

/**
 * @param write 读/写操作
 * @param buf 读/写缓冲区
 * @brief CMD53开启DMA
 */
static void sdio_cmd53_dma(bool write, uint8_t *buf) {
    LL_DMA_InitTypeDef dmaStruct = {0};
    dmaStruct.PeriphOrM2MSrcAddress = (uint32_t)&SDIO->FIFO;
    dmaStruct.MemoryOrM2MDstAddress = (uint32_t)buf;
    dmaStruct.Direction = write ? LL_DMA_DIRECTION_MEMORY_TO_PERIPH : LL_DMA_DIRECTION_PERIPH_TO_MEMORY;
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
}
