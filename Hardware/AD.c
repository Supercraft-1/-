#include "stm32f10x.h"                  // Device header

void AD_Init(void)
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1,ENABLE);//开启adc时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);//开启GPIOA时钟
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//分频 72/6=12MHz
	
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN; //ADC专硕模式
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	
	ADC_RegularChannelConfig(ADC1,ADC_Channel_0,1,ADC_SampleTime_55Cycles5);//初始化ADC规则组
	
	ADC_InitTypeDef ADC_InitStructure;
	
	ADC_InitStructure.ADC_ContinuousConvMode =DISABLE ;//关闭连续转换
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;//数据对齐方式 右对齐
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None ;//不适用外部中断触发，软件触发
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent ;
	ADC_InitStructure.ADC_NbrOfChannel = 1; //一个通道
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;//非扫描模式
	ADC_Init(ADC1,&ADC_InitStructure);
	
	ADC_Cmd(ADC1,ENABLE);
	
	ADC_ResetCalibration(ADC1); //开始复位校准
	while(ADC_GetResetCalibrationStatus(ADC1) == SET);//变为0说明复位校准完成
	ADC_StartCalibration(ADC1);//开始校准
	while(ADC_GetCalibrationStatus(ADC1) == SET);
	

}

uint16_t AD_GetValue(void)	
{

	ADC_SoftwareStartConvCmd(ADC1,ENABLE);
	
	while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET); //是否转换完成？
	
	return ADC_GetConversionValue(ADC1); //读取DR寄存器 然后FLAG自动置0
	
}
