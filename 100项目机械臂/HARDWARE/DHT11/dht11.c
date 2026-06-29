#include "sys.h"
#include "delay.h"
#include "dht11.h"

static GPIO_InitTypeDef	GPIO_InitStructure;	

void dht11_init(void)
{
	
	
	RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOG,ENABLE);
	
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed=GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;//开漏输出模式，高电平由外部的上拉电阻提供
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOG,&GPIO_InitStructure);	
	
	//根据手册，模块的DQ引脚初始电平为高电平
	PGout(9)=1;
	
}


int32_t dht11_read(uint8_t *buf)
{

	uint32_t t=0;
	int32_t i,j;
	uint8_t d=0;//0000 0000
	uint8_t *p=buf;
	uint16_t check_sum=0;
	
	//PG9配置为开漏输出模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_OUT;
	GPIO_InitStructure.GPIO_Speed=GPIO_Low_Speed;
	GPIO_InitStructure.GPIO_OType=GPIO_OType_OD;//开漏输出模式，高电平由外部的上拉电阻提供
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	
	GPIO_Init(GPIOG,&GPIO_InitStructure);	
	
	//唤醒DHT11
	PGout(9)=0;
	delay_ms(18);
	PGout(9)=1;
	delay_us(30);
	
	//PG9配置为输入模式
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_9;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_IN;
	GPIO_InitStructure.GPIO_PuPd=GPIO_PuPd_NOPULL;
	GPIO_Init(GPIOG,&GPIO_InitStructure);	

	//等待DHT11响应信号出现
	t=0;
	while(PGin(9))
	{
		t++;
		delay_us(1);
		if(t>=4000)
			return -1;
	}	
	
	
	//检测DHT11低电平的响应信号80us，超时检测法
	t=0;
	while(PGin(9)==0)
	{
		t++;
		delay_us(1);
		if(t>=100) //80<超时值
			return -2;
	}
	
	//检测DHT11高电平的响应信号80us，超时检测法
	t=0;
	while(PGin(9))
	{
		t++;
		delay_us(1);
		if(t>=100) //80<超时值
			return -3;
	}

	//连续接收5个字节
	for(j=0; j<5; j++)
	{
		//成功接收1个字节,最高有效位优先接收(bit7 bit6 ... bit0)
		d=0;//0000 0000
		for(i=7; i>=0; i--)
		{
			
			//检测数据前置低电平的50us，超时检测法
			t=0;
			while(PGin(9)==0)
			{
				t++;
				delay_us(1);
				if(t>100) //50<超时值
					return -4;
			}

			//延时40us(延时时间的范围：28<延时时间<70)
			delay_us(40);
		
			//判断是否为数据1
			if(PGin(9))
			{
				//对d变量对应的bit置1即可
				d|=1<<i;
				
				//等待剩余的高电平持续完毕，超时检测法
				t=0;
				while(PGin(9))
				{
					t++;
					delay_us(1);
					if(t>=100) //30(70-40=30)<超时值
						return -5;
				}			
			
			}
		
		
		}	
		p[j]=d;
	
	}
	
	//延时50us
	delay_us(50);
	
	//计算校验和
	check_sum=(p[0]+p[1]+p[2]+p[3])&0x00FF;
	
	//判断校验和是否正确
	if(p[4]!=check_sum)
		return -6;

		
	
	return 0;
}

