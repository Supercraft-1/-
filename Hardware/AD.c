#include "stm32f10x.h"
#include "AD.h"

/*==========================================================
 * ADC采样缓冲区
 * DMA会自动把ADC1采集到的数据放入这个数组
 *==========================================================*/
__align(4) uint16_t ADC_Buffer[ADC_BUF_LEN];

/*==========================================================
 * DMA采样状态标志
 * Half_Flag = 1：前半区采满
 * Full_Flag = 1：后半区采满
 *==========================================================*/
volatile uint8_t ADC_Half_Flag = 0;
volatile uint8_t ADC_Full_Flag = 0;

/*==========================================================
 * TIM3初始化
 * 功能：5kHz定时触发ADC
 *==========================================================*/
void TIM3_Init_5kHz(void)
{
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;

    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 72MHz / 14400 = 5000Hz */
    TIM_TimeBaseStructure.TIM_Period = 14400 - 1;
    TIM_TimeBaseStructure.TIM_Prescaler = 0;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);

    /* TIM3更新事件作为TRGO输出 */
    TIM_SelectOutputTrigger(TIM3, TIM_TRGOSource_Update);

    TIM_Cmd(TIM3, DISABLE);
}

/*==========================================================
 * DMA初始化
 *==========================================================*/
void DMA_ADC1_Init(void)
{
    DMA_InitTypeDef DMA_InitStructure;
    NVIC_InitTypeDef NVIC_InitStructure;

    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);

    DMA_DeInit(DMA1_Channel1);

    DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)&ADC1->DR;
    DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)ADC_Buffer;
    DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;
    DMA_InitStructure.DMA_BufferSize = ADC_BUF_LEN;
    DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
    DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;
    DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;
    DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;
    DMA_InitStructure.DMA_Mode = DMA_Mode_Circular;
    DMA_InitStructure.DMA_Priority = DMA_Priority_High;
    DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;

    DMA_Init(DMA1_Channel1, &DMA_InitStructure);

    DMA_ITConfig(DMA1_Channel1, DMA_IT_HT, ENABLE);
    DMA_ITConfig(DMA1_Channel1, DMA_IT_TC, ENABLE);

    NVIC_InitStructure.NVIC_IRQChannel = DMA1_Channel1_IRQn;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;

    NVIC_Init(&NVIC_InitStructure);

    DMA_Cmd(DMA1_Channel1, ENABLE);
}

/*==========================================================
 * ADC初始化
 *==========================================================*/
void AD_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    RCC_ADCCLKConfig(RCC_PCLK2_Div6);

    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    ADC_DeInit(ADC1);

    ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;
    ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_T3_TRGO;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfChannel = 1;

    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(
        ADC1,
        ADC_Channel_0,
        1,
        ADC_SampleTime_55Cycles5
    );

    ADC_DMACmd(ADC1, ENABLE);
    ADC_ExternalTrigConvCmd(ADC1, ENABLE);

    ADC_Cmd(ADC1, ENABLE);

    ADC_ResetCalibration(ADC1);
    while (ADC_GetResetCalibrationStatus(ADC1) == SET);

    ADC_StartCalibration(ADC1);
    while (ADC_GetCalibrationStatus(ADC1) == SET);

    DMA_ADC1_Init();
    TIM3_Init_5kHz();
}

/*==========================================================
 * 开始采样
 *==========================================================*/
void AD_Start(void)
{
    ADC_Half_Flag = 0;
    ADC_Full_Flag = 0;

    /* 必须先关DMA才能设置CNDTR，保证从数组0位置开始存 */
    TIM_Cmd(TIM3, DISABLE);
    DMA_Cmd(DMA1_Channel1, DISABLE);
    DMA_SetCurrDataCounter(DMA1_Channel1, ADC_BUF_LEN);
    
    DMA_Cmd(DMA1_Channel1, ENABLE);
    TIM_Cmd(TIM3, ENABLE);
}

/*==========================================================
 * 停止采样
 *==========================================================*/
void AD_Stop(void)
{
    TIM_Cmd(TIM3, DISABLE);
}

/*==========================================================
 * DMA中断函数
 *==========================================================*/
void DMA1_Channel1_IRQHandler(void)
{
    if (DMA_GetITStatus(DMA1_IT_HT1) != RESET)
    {
        ADC_Half_Flag = 1;
        DMA_ClearITPendingBit(DMA1_IT_HT1);
    }

    if (DMA_GetITStatus(DMA1_IT_TC1) != RESET)
    {
        ADC_Full_Flag = 1;
        DMA_ClearITPendingBit(DMA1_IT_TC1);
    }
}