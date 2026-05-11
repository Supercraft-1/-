#ifndef __AD_H
#define __AD_H

#include "stm32f10x.h"

/* ADC缓冲区长度
 * 2048个uint16_t数据
 * 前1024个点为前半区
 * 后1024个点为后半区
 */
#define ADC_BUF_LEN 2048

/* ADC DMA采样缓冲区 */
extern uint16_t ADC_Buffer[ADC_BUF_LEN];

/* DMA前半区采满标志 */
extern volatile uint8_t ADC_Half_Flag;

/* DMA后半区采满标志 */
extern volatile uint8_t ADC_Full_Flag;

/* ADC + DMA + TIM3 初始化 */
void AD_Init(void);

/* 开始采样 */
void AD_Start(void);

/* 停止采样 */
void AD_Stop(void);

#endif
