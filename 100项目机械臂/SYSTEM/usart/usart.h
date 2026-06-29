#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "stm32f4xx_conf.h"
#include "sys.h" 

//如果想串口中断接收，请不要注释以下宏定义
void uart_init(u32 baud);
void uart3_init(u32 baud);
void uart2_send_data(uint8_t dat);
void uart2_init(uint32_t baudrate);
void uart2_send_buf(uint8_t *buf, uint16_t len);
#endif


