#include "main.h"
#include "stm32f4xx_it.h"

extern DMA_HandleTypeDef hdma_dcmi;
extern DCMI_HandleTypeDef hdcmi;
extern _Bool data_full;

void DMA2_Stream1_IRQHandler(void) {
    if (__HAL_DMA_GET_TC_FLAG_INDEX(&hdma_dcmi)) {
        HAL_DCMI_Suspend(&hdcmi);
        data_full = 1;
    }
    HAL_DMA_IRQHandler(&hdma_dcmi);
}
