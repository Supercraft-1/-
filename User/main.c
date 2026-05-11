#include "stm32f10x.h"
#include "AD.h"
#include "Serial.h"
#include "Key.h"

/*==========================================================
 * 主程序
 * STM32只负责：
 * 1. 高速采样
 * 2. DMA缓存
 * 3. 串口发送原始数据
 * MATLAB负责：
 * 去直流、滤波、FFT、背景扣除
 *==========================================================*/

uint8_t Run_Flag = 0;
uint8_t KeyNum;

/*==========================================================
 * 发送ADC数据帧
 *==========================================================*/
void Send_ADC_Frame(uint8_t frame_id, uint16_t *buf, uint16_t len)
{
    uint8_t head[4];

    head[0] = 0xAA;
    head[1] = 0x55;
    head[2] = frame_id;
    head[3] = (uint8_t)(len & 0xFF);

    Serial_SendArray(head, 4);

    /* uint16_t数据按二进制发送 */
    Serial_SendArray((uint8_t *)buf, len * 2);
}

int main(void)
{
    /* 初始化串口 */
    Serial_Init();

    /* 初始化按键 */
    Key_Init();

    /* 初始化ADC */
    AD_Init();

    /* 默认关闭采样 */
    AD_Stop();

    while (1)
    {
        /* 按键控制开始/停止 */
        KeyNum = Key_GetNum();

        if (KeyNum == 1)
        {
            Run_Flag = !Run_Flag;

            if (Run_Flag)
            {
                AD_Start();
            }
            else
            {
                AD_Stop();
            }
        }

        /* 正在采样 */
        if (Run_Flag)
        {
            /* 前半区采满 */
            if (ADC_Half_Flag)
            {
                ADC_Half_Flag = 0;

                Send_ADC_Frame(
                    1,
                    &ADC_Buffer[0],
                    ADC_BUF_LEN / 2
                );
            }

            /* 后半区采满 */
            if (ADC_Full_Flag)
            {
                ADC_Full_Flag = 0;

                Send_ADC_Frame(
                    2,
                    &ADC_Buffer[ADC_BUF_LEN / 2],
                    ADC_BUF_LEN / 2
                );
            }
        }
    }
}
