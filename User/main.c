#include "stm32f10x.h"                  
#include "Delay.h"
#include "OLED.h"
#include "AD.h"
#include "Serial.h"
#include "Key.h"

uint16_t ADValue;      // 存储滤波后的采样值
float Voltage;         // 存储计算后的电压
uint8_t Run_Flag = 0;  // 0停止，1运行
uint8_t KeyNum;        // 存储按键编号

/**
  * 函    数：获取多次采样的平均值（均值滤波）
  * 放在 main 之前定义，避免编译器报错
  */
uint16_t Get_AD_Average(uint8_t Count)
{
    uint32_t Sum = 0;
    for (uint8_t i = 0; i < Count; i++)
    {
        Sum += AD_GetValue();
        Delay_ms(2); 
    }
    return (uint16_t)(Sum / Count);
}

int main(void)
{
    /* 1. 硬件初始化 */
    OLED_Init();
    AD_Init();
    Serial_Init();
    Key_Init(); 
    
    OLED_ShowString(1, 1, "Status:STOP   ");
    OLED_ShowString(2, 1, "AD:0000 V:0.00V");
    
    while(1)
    {
        /* 2. 按键检测 */
        KeyNum = Key_GetNum(); 
        if (KeyNum == 1) // 如果 PB1 按下
        {
            Run_Flag = !Run_Flag; // 切换运行/停止状态
            
            // 同步更新 OLED 状态文字
            if (Run_Flag == 1) OLED_ShowString(1, 8, "RUNNING");
            else               OLED_ShowString(1, 8, "STOP   ");
        }
        
        /* 3. 只有开启时才执行采样和发送 */
        if (Run_Flag == 1)
        {
            // 获取滤波后的数据
            ADValue = Get_AD_Average(5); 
            
            // 计算电压
            Voltage = (float)ADValue / 4095.0 * 5.0; 
            
            // OLED 显示数据
            OLED_ShowNum(2, 4, ADValue, 4);
            OLED_ShowNum(2, 11, (uint16_t)Voltage, 1);
            OLED_ShowNum(2, 13, (uint16_t)(Voltage * 100) % 100, 2);
            
            // 串口发送给 MATLAB (纯数字 + 换行)
            Serial_SendNumber(ADValue, 4); 
            Serial_SendString("\r\n"); 
            
            Delay_ms(50); // 采样周期约 70-80ms (50ms+20ms采样)
        }
        // 如果 Run_Flag 为 0，程序会在这里快速循环检测按键，不发数据
    }
}
