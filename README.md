# Marvell WLAN Host Driver
88W8801 WLAN 基础主机驱动，在 STM32F4 系列单片机上调试通过。驱动基于回调函数运行，无操作系统。
## 接口列表 / Interface List
- SDIO（连接无线模块，使用 DMA 以**简化传输**）
- SPI（SPI1 连接板载 Flash 用于存取固件，SPI2 连接 TFT\-LCD 用于调试）
- DCMI（连接摄像头模块，用于采集图像）
- GPIO 等杂项
## 设备列表 / Device List
- 单片机 *STM32F407*
- 无线模块 *88W8801*
- 板载 Flash *W25Q16DV*
- 128\*160 TFT\-LCD 驱动芯片 *ST7735S*
- 摄像头模块 *OV2640*
## 模块说明 / Module Description
- *88W8801* 详见 [参考 / Reference](#参考--reference)
    - *Flash* 读写 Flash，存取固件
    - *lwIP* TCP/IP 协议栈（版本号 2\.1\.3），添加 DHCP 服务器及 NAT 模块，[查看移植/修改内容](88w8801/lwip/FILES)
    - *SDIO* 设备通信接口实现
    - *Core* 处理封包，调用相关模块
    - *Wrapper* 简单的接口封装
- *Debug/ST7735* 输出调试信息到 TFT\-LCD
- *OV2640* 通过 SCCB 调整摄像头参数
- *SysTime* 时间管理（使用 SysTick 中断，无溢出控制）
- 其他说明请查看各级目录下的 README
## 配置文件 / Configuration Files
- 无线模块 [88w8801\.h](88w8801/88w8801.h)
- lwIP [lwipopts\.h](88w8801/lwip/include/lwipopts.h)
- TFT\-LCD [st7735\.h](st7735/st7735.h)
- 其他配置请查看各级目录下的 *.h
## 示例工程 / Example Project
请使用 *STM32CubeMX* 打开 [CaptureHandler\.ioc](example/CaptureHandler.ioc)，生成工程后将 *example* 目录下其余内容（不包括 PDF 文档）分别**覆盖**原有文件，最后**新建**一个 *Module* 目录并添加所有模块即可。
- 软件版本号如下：
    - Keil uVision: *MDK\-ARM 5\.38a*

      STM32F4xx\_DFP: *2\.17\.0*
    - STM32CubeMX: *6\.11\.1*

      STM32Cube FW\_F4: *1\.28\.0*
- [main\.c](example/main.c) 中默认禁用摄像头调试，可通过 *CAM\_DEBUG* 宏改变图像传输目的地
- 代码简陋，仅供参考
## 参考 / Reference
1. STMicroelectronics, [Datasheet DS8626](https://www.st.com/content/ccc/resource/technical/document/datasheet/ef/92/76/6d/bb/c2/4f/f7/DM00037051.pdf/files/DM00037051.pdf/jcr:content/translations/en.DM00037051.pdf)
2. STMicroelectronics, [Reference Manual RM0090](https://www.st.com/content/ccc/resource/technical/document/reference_manual/3d/6d/5a/66/b4/99/40/d4/DM00031020.pdf/files/DM00031020.pdf/jcr:content/translations/en.DM00031020.pdf)
3. IoT Wireless Link, [Marvell 88W8801 SDIO Wi\-Fi](https://github.com/sj15712795029/stm32_sdio_wifi_marvell8801_wifi)
4. SD Card Association, [SDIO Simplified Specification \(Version 3\.00\)](https://www.sdcard.org/cms/wp-content/themes/sdcard-org/dl.php?f=PartE1_SDIO_Simplified_Specification_Ver3.00.pdf)
5. Winbond, [W25Q16DV Datasheet](https://www.winbond.com/resource-files/w25q16dv%20revk%2005232016%20doc.pdf)
6. IEEE, [IEEE Std 802\.11\-2016](https://ieeexplore.ieee.org/document/7786995)
7. Swedish Institute of Computer Science, [lwIP \- A Lightweight TCP/IP Stack](https://savannah.nongnu.org/projects/lwip/)
8. RT\-Thread Development Team, [lwIP NAT Component](https://github.com/RT-Thread/rt-thread/tree/master/components/net/lwip-nat)
9. ALIENTEK Ltd\., [OV2640 Camera Module](http://www.openedv.com/docs/modules/camera/ov2640.html)
