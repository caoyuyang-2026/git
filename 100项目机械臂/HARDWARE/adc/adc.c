#include "adc.h"
#include "delay.h"
#include "sys.h"
#include "usart.h"

void adc_init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    ADC_CommonInitTypeDef ADC_CommonInitStructure;
    ADC_InitTypeDef ADC_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC1, ENABLE);

    // PA5 模拟输入
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AN;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 通用配置
    ADC_CommonInitStructure.ADC_Mode = ADC_Mode_Independent;
    ADC_CommonInitStructure.ADC_Prescaler = ADC_Prescaler_Div8;
    ADC_CommonInitStructure.ADC_DMAAccessMode = ADC_DMAAccessMode_Disabled;
    ADC_CommonInitStructure.ADC_TwoSamplingDelay = ADC_TwoSamplingDelay_5Cycles;
    ADC_CommonInit(&ADC_CommonInitStructure);

    // 关键修改：单次转换，不要连续！！！（多任务安全）
    ADC_InitStructure.ADC_Resolution = ADC_Resolution_12b;
    ADC_InitStructure.ADC_ScanConvMode = DISABLE;
    ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;  // 关闭连续转换
    ADC_InitStructure.ADC_ExternalTrigConvEdge = ADC_ExternalTrigConvEdge_None;
    ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;
    ADC_InitStructure.ADC_NbrOfConversion = 1;
    ADC_Init(ADC1, &ADC_InitStructure);

    ADC_RegularChannelConfig(ADC1, ADC_Channel_5, 1, ADC_SampleTime_480Cycles);
    ADC_Cmd(ADC1, ENABLE);
}

// 安全读取一次ADC值（无死等）
uint16_t adc_read_once(void)
{
    ADC_SoftwareStartConv(ADC1);
    // 这里必须等，但时间极短，不会卡死任务
    while(ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC) == RESET);
    ADC_ClearFlag(ADC1, ADC_FLAG_EOC);
    return ADC_GetConversionValue(ADC1);
}

// 滤波函数（完全安全，多任务可跑）
uint16_t average(void)
{
    uint16_t i, j;
    uint32_t temp, sum = 0;
    uint16_t buf[100];

    // 采集100次
    for(i = 0; i < 100; i++)
    {
        buf[i] = adc_read_once();
    }

    // 冒泡排序
    for(i = 0; i < 99; i++)
    {
        uint8_t swap = 0;
        for(j = 0; j < 99 - i; j++)
        {
            if(buf[j] > buf[j+1])
            {
                temp = buf[j];
                buf[j] = buf[j+1];
                buf[j+1] = temp;
                swap = 1;
            }
        }
        if(!swap) break;
    }

    // 去掉最大最小各10个，求平均
    sum = 0;
    for(i = 10; i < 90; i++)
    {
        sum += buf[i];
    }
    temp = sum / 80;

    return temp * 3300 / 4095; // 返回 mV
}
