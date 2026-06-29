#include "sys.h"
#include "usart.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "queue.h"
#include <string.h>

#pragma import(__use_no_semihosting)
struct __FILE
{
	int handle;
};
FILE __stdout;

void _sys_exit(int x)
{
	x = x;
}

//==================== USART1 重定向printf，同时支持接收指令 ====================
int fputc(int ch, FILE *f)
{
	while((USART1->SR&0X40)==0);
	USART1->DR = (u8) ch;
	return ch;
}

extern QueueHandle_t g_usart_queue;

// 串口1接收缓存
static uint8_t g_usart1_rx_buf[32] = {0};
static uint16_t g_usart1_rx_len = 0;
// 串口3接收缓存
static uint8_t g_usart3_rx_buf[32] = {0};
static uint16_t g_usart3_rx_len = 0;

void uart_init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1,ENABLE);

	GPIO_PinAFConfig(GPIOA,GPIO_PinSource9,GPIO_AF_USART1);
	GPIO_PinAFConfig(GPIOA,GPIO_PinSource10,GPIO_AF_USART1);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9 | GPIO_Pin_10;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOA,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART1, &USART_InitStructure);

	// 开启接收+空闲中断，既能printf发送，也能接收电脑指令
	USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
	USART_Cmd(USART1, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

// USART1中断：电脑串口指令入队
void USART1_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)
	{
		if(g_usart1_rx_len < sizeof(g_usart1_rx_buf))
		{
			g_usart1_rx_buf[g_usart1_rx_len++] = USART_ReceiveData(USART1);
		}
		USART_ClearITPendingBit(USART1, USART_IT_RXNE);
	}

	if(USART_GetITStatus(USART1, USART_IT_IDLE) != RESET)
	{
		(void)USART_ReceiveData(USART1);
		xQueueSendFromISR(g_usart_queue, g_usart1_rx_buf, &xHigherPriorityTaskWoken);
		g_usart1_rx_len = 0;
		memset(g_usart1_rx_buf, 0, sizeof(g_usart1_rx_buf));
		USART_ClearITPendingBit(USART1, USART_IT_IDLE);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
void uart2_init(uint32_t baudrate)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    USART_InitTypeDef USART_InitStructure;

    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);

    // PA2 TX AF7, PA3 RX AF7


    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
    GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);
	
	USART_InitStructure.USART_BaudRate = baudrate;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART2, &USART_InitStructure);

	USART_Cmd(USART2, ENABLE);
}

// 单字节发送
void uart2_send_data(uint8_t dat)
{
    while(USART_GetFlagStatus(USART2, USART_FLAG_TC) == RESET);
    USART_SendData(USART2, dat);
}

// 批量缓冲区发送
void uart2_send_buf(uint8_t *buf, uint16_t len)
{
    uint16_t i;
    for(i = 0; i < len; i++)
    {
        uart2_send_data(buf[i]);
    }
}
//==================== USART3 蓝牙模块 PB10(TX3) PB11(RX3) ====================
void uart3_init(u32 baud)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3,ENABLE);

	// 修正：PB10=USART3_TX，PB11=USART3_RX
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource10,GPIO_AF_USART3);
	GPIO_PinAFConfig(GPIOB,GPIO_PinSource11,GPIO_AF_USART3);

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10 | GPIO_Pin_11;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = baud;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_Init(USART3, &USART_InitStructure);

	USART_ITConfig(USART3, USART_IT_RXNE, ENABLE);
	USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
	USART_Cmd(USART3, ENABLE);

	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

// USART3中断：手机蓝牙指令入同一个队列
void USART3_IRQHandler(void)
{
	BaseType_t xHigherPriorityTaskWoken = pdFALSE;
	if(USART_GetITStatus(USART3, USART_IT_RXNE) != RESET)
	{
		if(g_usart3_rx_len < sizeof(g_usart3_rx_buf))
		{
			g_usart3_rx_buf[g_usart3_rx_len++] = USART_ReceiveData(USART3);
		}
		USART_ClearITPendingBit(USART3, USART_IT_RXNE);
	}

	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)
	{
		(void)USART_ReceiveData(USART3);
		xQueueSendFromISR(g_usart_queue, g_usart3_rx_buf, &xHigherPriorityTaskWoken);
		g_usart3_rx_len = 0;
		memset(g_usart3_rx_buf, 0, sizeof(g_usart3_rx_buf));
		USART_ClearITPendingBit(USART3, USART_IT_IDLE);
		portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
	}
}
