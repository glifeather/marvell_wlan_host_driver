#ifndef _OV2640_
#define _OV2640_
#include <stm32f4xx.h>

typedef enum {
    CAM_ERR_OK,
    CAM_ERR_MISMATCHING_MID,
    CAM_ERR_MISMATCHING_PID,
    CAM_ERR_NOT_DIVIDED_BY_4
} cam_err_e;

// 器件地址
#define OV2640_ADDR 0x60

// 制造商ID
#define OV2640_MID 0x7FA2
#define OV2640_PID 0x2642

// DSP寄存器地址（此时OV2640_DSP_RA_DLMT为0x0）
#define OV2640_DSP_R_BYPASS   0x05
#define OV2640_DSP_QS         0x44
#define OV2640_DSP_CTRL       0x50
#define OV2640_DSP_HSIZE1     0x51
#define OV2640_DSP_VSIZE1     0x52
#define OV2640_DSP_XOFFL      0x53
#define OV2640_DSP_YOFFL      0x54
#define OV2640_DSP_VHYX       0x55
#define OV2640_DSP_DPRP       0x56
#define OV2640_DSP_TEST       0x57
#define OV2640_DSP_ZMOW       0x5A
#define OV2640_DSP_ZMOH       0x5B
#define OV2640_DSP_ZMHH       0x5C
#define OV2640_DSP_BPADDR     0x7C
#define OV2640_DSP_BPDATA     0x7D
#define OV2640_DSP_CTRL2      0x86
#define OV2640_DSP_CTRL3      0x87
#define OV2640_DSP_SIZEL      0x8C
#define OV2640_DSP_HSIZE2     0xC0
#define OV2640_DSP_VSIZE2     0xC1
#define OV2640_DSP_CTRL0      0xC2
#define OV2640_DSP_CTRL1      0xC3
#define OV2640_DSP_R_DVP_SP   0xD3
#define OV2640_DSP_IMAGE_MODE 0xDA
#define OV2640_DSP_RESET      0xE0
#define OV2640_DSP_MS_SP      0xF0
#define OV2640_DSP_SS_ID      0xF7
#define OV2640_DSP_SS_CTRL    0xF8
#define OV2640_DSP_MC_BIST    0xF9
#define OV2640_DSP_MC_AL      0xFA
#define OV2640_DSP_MC_AH      0xFB
#define OV2640_DSP_MC_D       0xFC
#define OV2640_DSP_P_STATUS   0xFE
#define OV2640_DSP_RA_DLMT    0xFF

// Sensor寄存器地址（此时OV2640_DSP_RA_DLMT为0x1）
#define OV2640_SENSOR_GAIN       0x00
#define OV2640_SENSOR_COM1       0x03
#define OV2640_SENSOR_REG04      0x04
#define OV2640_SENSOR_REG08      0x08
#define OV2640_SENSOR_COM2       0x09
#define OV2640_SENSOR_PIDH       0x0A
#define OV2640_SENSOR_PIDL       0x0B
#define OV2640_SENSOR_COM3       0x0C
#define OV2640_SENSOR_COM4       0x0D
#define OV2640_SENSOR_AEC        0x10
#define OV2640_SENSOR_CLKRC      0x11
#define OV2640_SENSOR_COM7       0x12
#define OV2640_SENSOR_COM8       0x13
#define OV2640_SENSOR_COM9       0x14
#define OV2640_SENSOR_COM10      0x15
#define OV2640_SENSOR_HSTART     0x17
#define OV2640_SENSOR_HEND       0x18
#define OV2640_SENSOR_VSTART     0x19
#define OV2640_SENSOR_VEND       0x1A
#define OV2640_SENSOR_MIDH       0x1C
#define OV2640_SENSOR_MIDL       0x1D
#define OV2640_SENSOR_AEW        0x24
#define OV2640_SENSOR_AEB        0x25
#define OV2640_SENSOR_W          0x26
#define OV2640_SENSOR_REG2A      0x2A
#define OV2640_SENSOR_FRARL      0x2B
#define OV2640_SENSOR_ADDVSL     0x2D
#define OV2640_SENSOR_ADDVHS     0x2E
#define OV2640_SENSOR_YAVG       0x2F
#define OV2640_SENSOR_REG32      0x32
#define OV2640_SENSOR_ARCOM2     0x34
#define OV2640_SENSOR_REG45      0x45
#define OV2640_SENSOR_FLL        0x46
#define OV2640_SENSOR_FLH        0x47
#define OV2640_SENSOR_COM19      0x48
#define OV2640_SENSOR_ZOOMS      0x49
#define OV2640_SENSOR_COM22      0x4B
#define OV2640_SENSOR_COM25      0x4E
#define OV2640_SENSOR_BD50       0x4F
#define OV2640_SENSOR_BD60       0x50
#define OV2640_SENSOR_REG5D      0x5D
#define OV2640_SENSOR_REG5E      0x5E
#define OV2640_SENSOR_REG5F      0x5F
#define OV2640_SENSOR_REG60      0x60
#define OV2640_SENSOR_HISTO_LOW  0x61
#define OV2640_SENSOR_HISTO_HIGH 0x62

typedef enum {
    IMAGE_MODE_RGB565,
    IMAGE_MODE_JPEG
} camImageMode;

typedef enum {
    AUTO_EXPOSURE_0,
    AUTO_EXPOSURE_1,
    AUTO_EXPOSURE_2,
    AUTO_EXPOSURE_3,
    AUTO_EXPOSURE_4
} camAutoExposure;

typedef enum {
    WHITE_BALANCE_AUTO,
    WHITE_BALANCE_SUNNY,
    WHITE_BALANCE_CLOUDY,
    WHITE_BALANCE_OFFICE,
    WHITE_BALANCE_HOME
} camWhiteBalance;

typedef enum {
    INC_VALUE_MINUS_2,
    INC_VALUE_MINUS_1,
    INC_VALUE_0,
    INC_VALUE_PLUS_1,
    INC_VALUE_PLUS_2
} camIncValue;

typedef enum {
    EFFECT_NORMAL,
    EFFECT_INVERT,
    EFFECT_GRAYSCALE,
    EFFECT_REDDISH,
    EFFECT_GREENISH,
    EFFECT_BLUISH,
    EFFECT_VINTAGE
} camEffect;

typedef struct {
    GPIO_TypeDef *SCL_GPIO_Port;
    GPIO_TypeDef *SDA_GPIO_Port;
    GPIO_TypeDef *PDN_GPIO_Port;
    GPIO_TypeDef *RST_GPIO_Port;
    uint32_t SCL_Pin;
    uint32_t SDA_Pin;
    uint32_t PDN_Pin;
    uint32_t RST_Pin;
} camSettings;

// 初始化
cam_err_e camInit(camSettings *camsInit);
// 设置图像模式
void camSetImageMode(camImageMode camimParam);
// 设置拍摄窗口
void camSetWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
// 根据拍摄窗口设置拍摄尺寸
void camSetSize(uint16_t width, uint16_t height);
// 根据拍摄尺寸设置图像窗口（width/height均为4的倍数）
cam_err_e camSetImageWindow(uint16_t x, uint16_t y, uint16_t width, uint16_t height);
// 根据图像窗口设置图像尺寸（width/height均为4的倍数）
cam_err_e camSetImageSize(uint16_t width, uint16_t height);
// 设置自动曝光
void camSetAutoExposure(camAutoExposure camaeParam);
// 设置白平衡
void camSetWhiteBalance(camWhiteBalance camwbParam);
// 设置饱和度
void camSetSaturation(camIncValue camivParam);
// 设置亮度
void camSetBrightness(camIncValue camivParam);
// 设置对比度
void camSetContrast(camIncValue camivParam);
// 设置效果
void camSetEffect(camEffect cameParam);
#endif
