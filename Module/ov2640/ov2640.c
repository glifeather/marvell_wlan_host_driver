#include <stm32f4xx_ll_gpio.h>
#include <stm32f4xx_ll_utils.h>
#include "ov2640.h"
#include "sccb.h"

// SVGA初始化参数
static const uint8_t g_ppu8SVGAInit[][2] = {
    {OV2640_DSP_RA_DLMT, 0x00},
    {0x2C, 0xFF},
    {0x2E, 0xDF},
    {OV2640_DSP_RA_DLMT, 0x01},
    {0x3C, 0x32},
    {OV2640_SENSOR_CLKRC, 0x00},
    {OV2640_SENSOR_COM2, 0x02},
    {OV2640_SENSOR_REG04, 0x28},
    {OV2640_SENSOR_COM8, 0xE5},
    {OV2640_SENSOR_COM9, 0x48},
    {0x2C, 0x0C},
    {0x33, 0x78},
    {0x3A, 0x33},
    {0x3B, 0xFB},
    {0x3E, 0x00},
    {0x43, 0x11},
    {0x16, 0x10},
    {0x39, 0x92},
    {0x35, 0xDA},
    {0x22, 0x1A},
    {0x37, 0xC3},
    {0x23, 0x00},
    {OV2640_SENSOR_ARCOM2, 0xC0},
    {0x36, 0x1A},
    {0x06, 0x88},
    {0x07, 0xC0},
    {0x0D, 0x87},
    {0x0E, 0x41},
    {0x4C, 0x00},
    {OV2640_SENSOR_COM19, 0x00},
    {0x5B, 0x00},
    {0x42, 0x03},
    {0x4A, 0x81},
    {0x21, 0x99},
    {OV2640_SENSOR_AEW, 0x40},
    {OV2640_SENSOR_AEB, 0x38},
    {OV2640_SENSOR_VV, 0x82},
    {0x5C, 0x00},
    {0x63, 0x00},
    {OV2640_SENSOR_COM3, 0x3C},
    {OV2640_SENSOR_HISTO_LOW, 0x70},
    {OV2640_SENSOR_HISTO_HIGH, 0x80},
    {0x7C, 0x05},
    {0x20, 0x80},
    {0x28, 0x30},
    {0x6C, 0x00},
    {0x6D, 0x80},
    {0x6E, 0x00},
    {0x70, 0x02},
    {0x71, 0x94},
    {0x73, 0xC1},
    {OV2640_SENSOR_COM7, 0x40},
    {OV2640_SENSOR_HREFST, 0x11},
    {OV2640_SENSOR_HREFEND, 0x43},
    {OV2640_SENSOR_VSTRT, 0x00},
    {OV2640_SENSOR_VEND, 0x4B},
    {OV2640_SENSOR_REG32, 0x09},
    {0x37, 0xC0},
    {OV2640_SENSOR_BD50, 0xCA},
    {OV2640_SENSOR_BD60, 0xA8},
    {0x5A, 0x23},
    {0x6D, 0x00},
    {0x3D, 0x38},
    {OV2640_DSP_RA_DLMT, 0x00},
    {0xE5, 0x7F},
    {OV2640_DSP_MC_BIST, 0xC0},
    {0x41, 0x24},
    {OV2640_DSP_RESET, 0x14},
    {0x76, 0xFF},
    {0x33, 0xA0},
    {0x42, 0x20},
    {0x43, 0x18},
    {0x4C, 0x00},
    {OV2640_DSP_CTRL3, 0xD5},
    {0x88, 0x3F},
    {0xD7, 0x03},
    {0xD9, 0x10},
    {OV2640_DSP_R_DVP_SP, 0x02},
    {0xC8, 0x08},
    {0xC9, 0x80},
    {OV2640_DSP_BPADDR, 0x00},
    {OV2640_DSP_BPDATA, 0x00},
    {OV2640_DSP_BPADDR, 0x03},
    {OV2640_DSP_BPDATA, 0x48},
    {OV2640_DSP_BPDATA, 0x48},
    {OV2640_DSP_BPADDR, 0x08},
    {OV2640_DSP_BPDATA, 0x20},
    {OV2640_DSP_BPDATA, 0x10},
    {OV2640_DSP_BPDATA, 0x0E},
    {0x90, 0x00},
    {0x91, 0x0E},
    {0x91, 0x1A},
    {0x91, 0x31},
    {0x91, 0x5A},
    {0x91, 0x69},
    {0x91, 0x75},
    {0x91, 0x7E},
    {0x91, 0x88},
    {0x91, 0x8F},
    {0x91, 0x96},
    {0x91, 0xA3},
    {0x91, 0xAF},
    {0x91, 0xC4},
    {0x91, 0xD7},
    {0x91, 0xE8},
    {0x91, 0x20},
    {0x92, 0x00},
    {0x93, 0x06},
    {0x93, 0xE3},
    {0x93, 0x05},
    {0x93, 0x05},
    {0x93, 0x00},
    {0x93, 0x04},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x93, 0x00},
    {0x96, 0x00},
    {0x97, 0x08},
    {0x97, 0x19},
    {0x97, 0x02},
    {0x97, 0x0C},
    {0x97, 0x24},
    {0x97, 0x30},
    {0x97, 0x28},
    {0x97, 0x26},
    {0x97, 0x02},
    {0x97, 0x98},
    {0x97, 0x80},
    {0x97, 0x00},
    {0x97, 0x00},
    {OV2640_DSP_CTRL1, 0xED},
    {0xA4, 0x00},
    {0xA8, 0x00},
    {0xC5, 0x11},
    {0xC6, 0x51},
    {0xBF, 0x80},
    {OV2640_DSP_WB_MODE, 0x10},
    {0xB6, 0x66},
    {0xB8, 0xA5},
    {0xB7, 0x64},
    {0xB9, 0x7C},
    {0xB3, 0xAF},
    {0xB4, 0x97},
    {0xB5, 0xFF},
    {0xB0, 0xC5},
    {0xB1, 0x94},
    {0xB2, 0x0F},
    {0xC4, 0x5C},
    {OV2640_DSP_HSIZE8, 0x64},
    {OV2640_DSP_VSIZE8, 0x4B},
    {OV2640_DSP_SIZEL, 0x00},
    {OV2640_DSP_CTRL2, 0x3D},
    {OV2640_DSP_CTRLL, 0x00},
    {OV2640_DSP_HSIZE, 0xC8},
    {OV2640_DSP_VSIZE, 0x96},
    {OV2640_DSP_XOFFL, 0x00},
    {OV2640_DSP_YOFFL, 0x00},
    {OV2640_DSP_VHYX, 0x00},
    {OV2640_DSP_ZMOW, 0xC8},
    {OV2640_DSP_ZMOH, 0x96},
    {OV2640_DSP_ZMHH, 0x00},
    {OV2640_DSP_CTRL1, 0xED},
    {0x7F, 0x00},
    {OV2640_DSP_IMAGE_MODE, 0x09},
    {0xE5, 0x1F},
    {0xE1, 0x67},
    {OV2640_DSP_RESET, 0x00},
    {0xDD, 0x7F},
    {OV2640_DSP_R_BYPASS, 0x00}};
// 自动曝光参数
static const uint8_t g_ppu8AutoExposure[][6] = {
    {OV2640_SENSOR_AEW, 0x20, OV2640_SENSOR_AEB, 0x18, OV2640_SENSOR_VV, 0x60},
    {OV2640_SENSOR_AEW, 0x34, OV2640_SENSOR_AEB, 0x1C, OV2640_SENSOR_VV, 0x00},
    {OV2640_SENSOR_AEW, 0x3E, OV2640_SENSOR_AEB, 0x38, OV2640_SENSOR_VV, 0x81},
    {OV2640_SENSOR_AEW, 0x48, OV2640_SENSOR_AEB, 0x40, OV2640_SENSOR_VV, 0x81},
    {OV2640_SENSOR_AEW, 0x58, OV2640_SENSOR_AEB, 0x50, OV2640_SENSOR_VV, 0x92}};
// 白平衡参数
static const uint8_t g_ppu8WhiteBalance[][6] = {
    {OV2640_DSP_WB_PARAM1, 0x5E, OV2640_DSP_WB_PARAM2, 0x41, OV2640_DSP_WB_PARAM3, 0x54},
    {OV2640_DSP_WB_PARAM1, 0x65, OV2640_DSP_WB_PARAM2, 0x41, OV2640_DSP_WB_PARAM3, 0x4F},
    {OV2640_DSP_WB_PARAM1, 0x52, OV2640_DSP_WB_PARAM2, 0x41, OV2640_DSP_WB_PARAM3, 0x66},
    {OV2640_DSP_WB_PARAM1, 0x42, OV2640_DSP_WB_PARAM2, 0x3F, OV2640_DSP_WB_PARAM3, 0x71}};
// 对比度参数
static const uint8_t g_ppu8Contrast[][2] = {
    {0x18, 0x34},
    {0x1C, 0x2A},
    {0x20, 0x20},
    {0x24, 0x16},
    {0x28, 0x0C}};
// 效果参数
static const uint8_t g_ppu8Effect[][3] = {
    {0x00, 0x80, 0x80},
    {0x18, 0x80, 0x80},
    {0x40, 0x80, 0x80},
    {0x58, 0x80, 0x80},
    {0x18, 0x40, 0xC0},
    {0x18, 0x40, 0x40},
    {0x18, 0xA0, 0x40},
    {0x18, 0x40, 0xA6}};

cam_err_e camInit(camSettings *camsInit) {
    sccbSettings sccbsInit;
    sccbsInit.u8Addr = OV2640_ADDR;
    sccbsInit.SCL_GPIO_Port = camsInit->SCL_GPIO_Port;
    sccbsInit.SDA_GPIO_Port = camsInit->SDA_GPIO_Port;
    sccbsInit.SCL_Pin = camsInit->SCL_Pin;
    sccbsInit.SDA_Pin = camsInit->SDA_Pin;
    sccbInit(&sccbsInit);
    LL_GPIO_ResetOutputPin(camsInit->PDN_GPIO_Port, camsInit->PDN_Pin);
    LL_GPIO_ResetOutputPin(camsInit->RST_GPIO_Port, camsInit->RST_Pin);
    LL_mDelay(10);
    LL_GPIO_SetOutputPin(camsInit->RST_GPIO_Port, camsInit->RST_Pin);
    LL_mDelay(10);
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x01);
    sccbWriteReg(OV2640_SENSOR_COM7, 0x80);
    LL_mDelay(5);
    if ((sccbReadReg(OV2640_SENSOR_MIDH) << 8 | sccbReadReg(OV2640_SENSOR_MIDL)) != OV2640_MID) return CAM_ERR_MISMATCHING_MID;
    if ((sccbReadReg(OV2640_SENSOR_PIDH) << 8 | sccbReadReg(OV2640_SENSOR_PIDL)) != OV2640_PID) return CAM_ERR_MISMATCHING_PID;
    for (uint8_t i = 0; i < sizeof(g_ppu8SVGAInit) / 2; ++i) sccbWriteReg(**(g_ppu8SVGAInit + i), *(*(g_ppu8SVGAInit + i) + 1));
    return CAM_ERR_OK;
}

void camSetWindow(uint16_t x, uint16_t y, uint16_t u16Width, uint16_t u16Height) {
    u16Width = x + u16Width / 2, u16Height = y + u16Height / 2;
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x01);
    sccbWriteReg(OV2640_SENSOR_COM1, (sccbReadReg(OV2640_SENSOR_COM1) & 0xF0) | (u16Height & 0x3) << 2 | (y & 0x3));
    sccbWriteReg(OV2640_SENSOR_VSTRT, y >> 2);
    sccbWriteReg(OV2640_SENSOR_VEND, u16Height >> 2);
    sccbWriteReg(OV2640_SENSOR_REG32, (sccbReadReg(OV2640_SENSOR_REG32) & 0xC0) | (u16Width & 0x7) << 3 | (x & 0x7));
    sccbWriteReg(OV2640_SENSOR_HREFST, x >> 3);
    sccbWriteReg(OV2640_SENSOR_HREFEND, u16Width >> 3);
}

void camSetSize(uint16_t u16Width, uint16_t u16Height) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_RESET, 0x04);
    sccbWriteReg(OV2640_DSP_HSIZE8, u16Width >> 3 & 0xFF);
    sccbWriteReg(OV2640_DSP_VSIZE8, u16Height >> 3 & 0xFF);
    sccbWriteReg(OV2640_DSP_SIZEL, (u16Width >> 4 & 0x80) | (u16Width & 0x7) << 3 | (u16Height & 0x7));
    sccbWriteReg(OV2640_DSP_RESET, 0x00);
}

cam_err_e camSetImageWindow(uint16_t x, uint16_t y, uint16_t u16Width, uint16_t u16Height) {
    if (u16Width % 4 || u16Height % 4) return CAM_ERR_NOT_DIVIDED_BY_4;
    u16Width /= 4, u16Height /= 4;
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_RESET, 0x04);
    sccbWriteReg(OV2640_DSP_HSIZE, u16Width & 0xFF);
    sccbWriteReg(OV2640_DSP_VSIZE, u16Height & 0xFF);
    sccbWriteReg(OV2640_DSP_XOFFL, x & 0xFF);
    sccbWriteReg(OV2640_DSP_YOFFL, y & 0xFF);
    sccbWriteReg(OV2640_DSP_VHYX, (u16Height >> 1 & 0x80) | (y >> 4 & 0x70) | (u16Width >> 5 & 0x8) | (x >> 8 & 0x7));
    sccbWriteReg(OV2640_DSP_TEST, u16Width >> 2 & 0x80);
    sccbWriteReg(OV2640_DSP_RESET, 0x00);
    return CAM_ERR_OK;
}

cam_err_e camSetImageSize(uint16_t u16Width, uint16_t u16Height) {
    if (u16Width % 4 || u16Height % 4) return CAM_ERR_NOT_DIVIDED_BY_4;
    u16Width /= 4, u16Height /= 4;
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_RESET, 0x04);
    sccbWriteReg(OV2640_DSP_ZMOW, u16Width & 0xFF);
    sccbWriteReg(OV2640_DSP_ZMOH, u16Height & 0xFF);
    sccbWriteReg(OV2640_DSP_ZMHH, (u16Height >> 6 & 0x4) | (u16Width >> 8 & 0x3));
    sccbWriteReg(OV2640_DSP_RESET, 0x00);
    return CAM_ERR_OK;
}

void camSetAutoExposure(camAutoExposure camaeParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x01);
    uint8_t *pu8Param = (uint8_t *)*(g_ppu8AutoExposure + camaeParam);
    for (uint8_t i = 0; i < 3; ++i) sccbWriteReg(*(pu8Param + i * 2), *(pu8Param + i * 2 + 1));
}

void camSetWhiteBalance(camWhiteBalance camwbParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_WB_MODE, camwbParam != CAM_WHITE_BALANCE_AUTO ? 0x40 : 0x10);
    if (camwbParam-- != CAM_WHITE_BALANCE_AUTO) {
        uint8_t *pu8Param = (uint8_t *)*(g_ppu8WhiteBalance + camwbParam);
        for (uint8_t i = 0; i < 3; ++i) sccbWriteReg(*(pu8Param + i * 2), *(pu8Param + i * 2 + 1));
    }
}

void camSetSaturation(camIncValue camivParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x00);
    sccbWriteReg(OV2640_DSP_BPDATA, 0x02);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x03);
    sccbWriteReg(OV2640_DSP_BPDATA, (camivParam + 2) << 4 | 0x8);
    sccbWriteReg(OV2640_DSP_BPDATA, (camivParam + 2) << 4 | 0x8);
}

void camSetBrightness(camIncValue camivParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x00);
    sccbWriteReg(OV2640_DSP_BPDATA, 0x04);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x09);
    sccbWriteReg(OV2640_DSP_BPDATA, camivParam << 4);
    sccbWriteReg(OV2640_DSP_BPDATA, 0x00);
}

void camSetContrast(camIncValue camivParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x00);
    sccbWriteReg(OV2640_DSP_BPDATA, 0x04);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x07);
    sccbWriteReg(OV2640_DSP_BPDATA, 0x20);
    sccbWriteReg(OV2640_DSP_BPDATA, **(g_ppu8Contrast + camivParam));
    sccbWriteReg(OV2640_DSP_BPDATA, *(*(g_ppu8Contrast + camivParam) + 1));
    sccbWriteReg(OV2640_DSP_BPDATA, 0x06);
}

void camSetEffect(camEffect cameParam) {
    sccbWriteReg(OV2640_DSP_RA_DLMT, 0x00);
    sccbWriteReg(OV2640_DSP_BPADDR, 0x00);
    sccbWriteReg(OV2640_DSP_BPDATA, **(g_ppu8Effect + cameParam));
    sccbWriteReg(OV2640_DSP_BPADDR, 0x05);
    sccbWriteReg(OV2640_DSP_BPDATA, *(*(g_ppu8Effect + cameParam) + 1));
    sccbWriteReg(OV2640_DSP_BPDATA, *(*(g_ppu8Effect + cameParam) + 2));
}
