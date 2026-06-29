#include "at24c02.h"
#include <stdio.h>
#include <stdint.h>

extern void delay_us(uint32_t t);

void eeprom_init(void)
{
	// 局部变量挪到函数最顶部，C89兼容
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB,ENABLE);
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_8|GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed=GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStructure);

	EEPROM_SCL_W=1;
	EEPROM_SDA_W=1;
}

void eeprom_sda_pin_mode(GPIOMode_TypeDef pin_mode)
{
	// 变量置顶
	GPIO_InitTypeDef GPIO_InitStructure;

	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=pin_mode;
	GPIO_InitStructure.GPIO_Speed=GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

void eeprom_i2c_start(void)
{
	eeprom_sda_pin_mode(GPIO_Mode_OUT);
	EEPROM_SCL_W=1;
	EEPROM_SDA_W=1;
	delay_us(5);
	EEPROM_SDA_W=0;
	delay_us(5);
}

void eeprom_i2c_stop(void)
{
	eeprom_sda_pin_mode(GPIO_Mode_OUT);
	EEPROM_SCL_W=1;
	EEPROM_SDA_W=0;
	delay_us(5);
	EEPROM_SDA_W=1;
	delay_us(5);
}

void eeprom_i2c_send_byte(uint8_t byte)
{
	int32_t i;
	eeprom_sda_pin_mode(GPIO_Mode_OUT);
	EEPROM_SCL_W=0;
	EEPROM_SDA_W=0;
	delay_us(5);
	for(i=7; i>=0; i--)
	{
		if(byte &(1<<i))
			EEPROM_SDA_W=1;
		else
			EEPROM_SDA_W=0;
		delay_us(5);
		EEPROM_SCL_W=1;
		delay_us(5);
		EEPROM_SCL_W=0;
	}
}

void eeprom_i2c_send_ack(uint8_t ack)
{
	eeprom_sda_pin_mode(GPIO_Mode_OUT);
	EEPROM_SCL_W=0;
	EEPROM_SDA_W=0;
	delay_us(5);
	if(ack)
		EEPROM_SDA_W=0;
	else
		EEPROM_SDA_W=1;
	delay_us(5);
	EEPROM_SCL_W=1;
	delay_us(5);
	EEPROM_SCL_W=0;
	delay_us(5);
}

uint8_t eeprom_i2c_wait_ack(void)
{
	uint8_t ack=0;
	eeprom_sda_pin_mode(GPIO_Mode_IN);
	EEPROM_SCL_W=1;
	delay_us(5);
	if(EEPROM_SDA_R ==0)
		ack=1;
	else
		ack=0;
	EEPROM_SCL_W=0;
	delay_us(5);
	return ack;
}

uint8_t eeprom_i2c_recv_byte(void)
{
	int32_t i;
	uint8_t d=0;
	eeprom_sda_pin_mode(GPIO_Mode_IN);
	for(i=7; i>=0; i--)
	{
		EEPROM_SCL_W=1;
		delay_us(5);
		if(EEPROM_SDA_R)
			d|=1<<i;
		EEPROM_SCL_W=0;
		delay_us(5);
	}
	return d;
}

int32_t eeprom_page_write(uint8_t word_addr,uint8_t *buf,uint8_t len)
{
	uint8_t ack;
	uint8_t *p=buf;
	eeprom_i2c_start();
	eeprom_i2c_send_byte(0xA0);
	ack=eeprom_i2c_wait_ack();
	if(!ack)
	{
		printf("device address fail\r\n");
		return -1;
	}
	eeprom_i2c_send_byte(word_addr);
	ack=eeprom_i2c_wait_ack();
	if(!ack)
	{
		printf("word address fail\r\n");
		return -2;
	}
	while(len--)
	{
		eeprom_i2c_send_byte(*p++);
		ack=eeprom_i2c_wait_ack();
		if(!ack)
		{
			printf("write data fail\r\n");
			return -3;
		}
	}
	eeprom_i2c_stop();
	printf("write data success\r\n");
	return 0;
}

int32_t eeprom_read(uint8_t word_addr,uint8_t *buf,uint8_t len)
{
	uint8_t ack;
	uint8_t *p=buf;
	eeprom_i2c_start();
	eeprom_i2c_send_byte(0xA0);
	ack=eeprom_i2c_wait_ack();
	if(!ack)
	{
		printf("device address fail\r\n");
		return -1;
	}
	eeprom_i2c_send_byte(word_addr);
	ack=eeprom_i2c_wait_ack();
	if(!ack)
	{
		printf("word address fail\r\n");
		return -2;
	}
	eeprom_i2c_start();
	eeprom_i2c_send_byte(0xA1);
	ack=eeprom_i2c_wait_ack();
	if(!ack)
	{
		printf("device address fail\r\n");
		return -3;
	}
	len=len-1;
	while(len--)
	{
		*p=eeprom_i2c_recv_byte();
		p++;
		eeprom_i2c_send_ack(1);
	}
	*p=eeprom_i2c_recv_byte();
	eeprom_i2c_send_ack(0);
	eeprom_i2c_stop();
	printf("read data success\r\n");
	return 0;
}
