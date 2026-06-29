/**
  ******************************************************************************
  * @file    main.c
  * @author  STM32 FreeRTOS Demo
  * @brief   基于FreeRTOS的多任务程序
  *          串口W/A/S/D/Q/E手动控制舵机
  *          U开启超声波自动控制 / O关闭超声波自动控制
  *          AT24C02：xxx#写入角度、R读取、L手动加载保存角度（不上电自动加载）
  ******************************************************************************
  */

#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "led.h"
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "semphr.h"
#include "oled.h"
#include "servo.h"
#include "sr04.h"
#include "myimagedot.h"
#include "at24c02.h"
#include <string.h>
#include <stdlib.h>

/* -------------------------- 任务优先级配置 -------------------------- */
#define START_TASK_PRIO        1
#define SERVO_TASK_PRIO        2
#define USART_TASK_PRIO        3
#define OLED_TASK_PRIO         4
#define SR04_TASK_PRIO         2
#define EEPROM_TASK_PRIO       2

/* -------------------------- 任务堆栈大小配置 -------------------------- */
#define START_STK_SIZE         1024
#define SERVO_STK_SIZE         256
#define USART_STK_SIZE         256
#define OLED_STK_SIZE          256
#define SR04_STK_SIZE          256
#define EEPROM_STK_SIZE        256

/* -------------------------- 任务句柄 -------------------------- */
TaskHandle_t StartTask_Handler;
TaskHandle_t ServoTask_Handler;
TaskHandle_t UsartTask_Handler;
TaskHandle_t OledTask_Handler;
TaskHandle_t Sr04Task_Handler;
TaskHandle_t EepromTask_Handler;

/* -------------------------- 内核对象 -------------------------- */
SemaphoreHandle_t g_oled_mutex;
SemaphoreHandle_t g_servo_angle_mutex;
SemaphoreHandle_t g_eeprom_mutex;
QueueHandle_t     g_usart_queue;

/* 全局舵机角度变量 */
uint8_t g_servo1_angle = 90;
uint8_t g_servo2_angle = 0;
uint8_t g_servo3_angle = 90;

/* 超声波使能开关：上电默认0关闭，串口U开启 O关闭 */
uint8_t g_sr04_enable = 0;

#define EEPROM_DATA_LEN    8
uint8_t eeprom_w_buf[EEPROM_DATA_LEN] = {0};
uint8_t eeprom_r_buf[EEPROM_DATA_LEN] = {0};

static void task_servo(void *pvParameters);
static void task_usart(void *pvParameters);
static void task_oled(void *pvParameters);
static void task_sr04(void *pvParameters);
static void task_eeprom(void *pvParameters);

/**
  * @brief  解析空格分隔十进制数字，最多8个
  */
uint8_t parse_num_from_str(char *str, uint8_t *out_buf)
{
	char *tok;
	uint8_t cnt = 0;
	memset(out_buf, 0, EEPROM_DATA_LEN);
	tok = strtok(str, " ");
	while(tok != NULL && cnt < EEPROM_DATA_LEN)
	{
		int val = atoi(tok);
		if(val >= 0 && val <= 255)
		{
			out_buf[cnt++] = (uint8_t)val;
		}
		tok = strtok(NULL, " ");
	}
	return cnt;
}

/**
  * @brief  硬件初始化函数
  */
void hardware_init(void)
{
    uart_init(9600);
    uart3_init(9600);
    LED_Init();
    OLED_Init();
    Servo_Init();
    sr04_init();
    eeprom_init();
	syn6288_init();
}

/**
  * @brief  开始任务函数
  */
void task_start(void *pvParameters)
{
    uint8_t i = 0;
    uint8_t x = 0;

    hardware_init();
    g_oled_mutex = xSemaphoreCreateMutex();
    g_servo_angle_mutex = xSemaphoreCreateMutex();
    g_eeprom_mutex = xSemaphoreCreateMutex();
    g_usart_queue = xQueueCreate(10, 32);

    // 【已删除】上电自动读取EEPROM加载角度代码

    OLED_Clear();
    // 开场动画
    for(x=0; x<128; x++)
    {
        OLED_DrawBMP(0,0,32,4, g_image_dot_tbl[i].address);
        i++;
        if(i > 11) i = 0;
        oled_fill(x,7,1,0xFF);
        vTaskDelay(10);
    }
    OLED_Clear();

    /* 创建各个子任务 */
    xTaskCreate((TaskFunction_t)task_oled,
               (const char*)"task_oled",
               OLED_STK_SIZE,
               NULL,
               OLED_TASK_PRIO,
               &OledTask_Handler);

    xTaskCreate((TaskFunction_t)task_usart,
                (const char*)"task_usart",
                USART_STK_SIZE,
                NULL,
                USART_TASK_PRIO,
                &UsartTask_Handler);

    xTaskCreate((TaskFunction_t)task_servo,
                (const char*)"task_servo",
                SERVO_STK_SIZE,
                NULL,
                SERVO_TASK_PRIO,
                &ServoTask_Handler);

    xTaskCreate((TaskFunction_t)task_sr04,
                (const char*)"task_sr04",
                SR04_STK_SIZE,
                NULL,
                SR04_TASK_PRIO,
                &Sr04Task_Handler);

    xTaskCreate((TaskFunction_t)task_eeprom,
                (const char*)"task_eeprom",
                EEPROM_STK_SIZE,
                NULL,
                EEPROM_TASK_PRIO,
                &EepromTask_Handler);

    vTaskDelete(NULL);
}

/**
  * @brief  EEPROM后台空轮询任务（预留扩展）
  */
static void task_eeprom(void *pvParameters)
{
	while(1)
	{
		vTaskDelay(pdMS_TO_TICKS(500));
	}
}

/**
  * @brief  OLED显示任务
  */
void task_oled(void *pvParameters)
{
    while(1)
    {
        xSemaphoreTake(g_oled_mutex, portMAX_DELAY);

        oled_show_string_fmt(0, 0, "项目机械臂");
        oled_show_string_fmt(0, 2, "舵机1: %d 度", g_servo1_angle);
        oled_show_string_fmt(0, 4, "舵机2: %d 度", g_servo2_angle);
        oled_show_string_fmt(0, 6, "舵机3: %d 度", g_servo3_angle);

        xSemaphoreGive(g_oled_mutex);
        vTaskDelay(200);
    }
}

/**
  * @brief  串口接收任务
  */
void task_usart(void *pvParameters)
{
    char buf[32] = {0};
    BaseType_t rt;
	int i;

    while(1)
    {
        rt = xQueueReceive(g_usart_queue, buf, portMAX_DELAY);
        if(rt == pdTRUE)
        {
            xSemaphoreTake(g_servo_angle_mutex, portMAX_DELAY);
            switch(buf[0])
            {
                case 'S':
                case 's':
                    if(g_servo1_angle < 180)
                    {
                        g_servo1_angle += 10;
                        if(g_servo1_angle > 180) g_servo1_angle = 180;
                    }
                    printf("舵机1角度: %d度\r\n", g_servo1_angle);
                    break;
                case 'W':
                case 'w':
                    if(g_servo1_angle > 0 )
                    {
                        g_servo1_angle -= 10;
                        if(g_servo1_angle < 0) g_servo1_angle = 0;
                    }
                    printf("舵机1角度: %d度\r\n", g_servo1_angle);
                    break;
                case 'D':
                case 'd':
                    if(g_servo2_angle < 180)
                    {
                        g_servo2_angle += 10;
                        if(g_servo2_angle > 180) g_servo2_angle = 180;
                    }
                    printf("舵机2角度: %d度\r\n", g_servo2_angle);
                    break;
                case 'A':
                case 'a':
                    if(g_servo2_angle > 0)
                    {
                        g_servo2_angle -= 10;
                        if(g_servo2_angle < 0) g_servo2_angle = 0;
                    }
                    printf("舵机2角度: %d度\r\n", g_servo2_angle);
                    break;
                case 'Q':
                case 'q':
                    if(g_servo3_angle < 90)
                    {
                        g_servo3_angle += 10;
                        if(g_servo3_angle > 90) g_servo3_angle = 90;
                    }
                    printf("舵机3角度: %d度\r\n", g_servo3_angle);
                    break;
                case 'E':
                case 'e':
                    if(g_servo3_angle > 20)
                    {
                        g_servo3_angle -= 10;
                        if(g_servo3_angle < 20) g_servo3_angle = 20;
                    }
                    printf("舵机3角度: %d度\r\n", g_servo3_angle);
                    break;

                /* 超声波开关指令 */
                case 'U':
                case 'u':
                    g_sr04_enable = 1;
                    printf("【超声波自动控制已开启】\r\n");
                    break;
                case 'O':
                case 'o':
                    g_sr04_enable = 0;
                    printf("【超声波自动控制已关闭】\r\n");
                    break;

                /* EEPROM读取全部8字节 R/r */
                case 'R':
                case 'r':
                {
                    xSemaphoreGive(g_servo_angle_mutex);
                    xSemaphoreTake(g_eeprom_mutex, portMAX_DELAY);
                    eeprom_read(0, eeprom_r_buf, EEPROM_DATA_LEN);
                    xSemaphoreGive(g_eeprom_mutex);

                    printf("EEPROM读出8字节数据: ");
                    for( i=0;i<EEPROM_DATA_LEN;i++)
                    {
                        printf("%d ", eeprom_r_buf[i]);
                    }
                    printf("\r\n");
                    goto buf_reset;
                }

                /* 新增：手动加载保存的角度 L/l 指令 */
                case 'L':
                case 'l':
                {
                    xSemaphoreGive(g_servo_angle_mutex);
                    xSemaphoreTake(g_eeprom_mutex, portMAX_DELAY);
                    eeprom_read(0, eeprom_r_buf, 3);
                    xSemaphoreGive(g_eeprom_mutex);

                    xSemaphoreTake(g_servo_angle_mutex, portMAX_DELAY);
                    g_servo1_angle = eeprom_r_buf[0];
                    g_servo2_angle = eeprom_r_buf[1];
                    g_servo3_angle = eeprom_r_buf[2];
                    xSemaphoreGive(g_servo_angle_mutex);

                    printf("手动加载EEPROM角度完成: S1=%d S2=%d S3=%d\r\n",
                           g_servo1_angle,g_servo2_angle,g_servo3_angle);
                    goto buf_reset;
                }
					case 'P':
					case 'p':
				{
				// 不占用舵机锁
					syn6288_speak_gbk("来给生活比个耶");
					vTaskDelay(2000);
					g_servo3_angle=30;
					vTaskDelay(500);
					g_servo3_angle=90;
					vTaskDelay(500);
					break;
				}				

                default:
                    /* 带#代表写入数字到EEPROM */
                    if(strchr(buf, '#') != NULL)
                    {
                        char *p_hash = strchr(buf, '#');
                        *p_hash = '\0';
                        parse_num_from_str(buf, eeprom_w_buf);

                        xSemaphoreGive(g_servo_angle_mutex);
                        xSemaphoreTake(g_eeprom_mutex, portMAX_DELAY);
                        eeprom_page_write(0, eeprom_w_buf, EEPROM_DATA_LEN);
                        xSemaphoreGive(g_eeprom_mutex);
                        vTaskDelay(pdMS_TO_TICKS(10));

                        printf("EEPROM写入成功: ");
                        for( i=0;i<EEPROM_DATA_LEN;i++)
                        {
                            printf("%d ", eeprom_w_buf[i]);
                        }
                        printf("\r\n");
                        goto buf_reset;
                    }
                    else
                    {
                        printf("未知命令: %c\r\n", buf[0]);
                    }
                    break;
            }
            xSemaphoreGive(g_servo_angle_mutex);

buf_reset:
            memset(buf, 0, sizeof(buf));
        }
    }
}

/**
  * @brief  超声波测距任务
  */
void task_sr04(void *pvParameters)
{
    int dist;
    while(1)
    {
        vTaskDelay(pdMS_TO_TICKS(300));

        if(g_sr04_enable == 0)
        {
            continue;
        }

        dist = get_distance();
        printf("检测距离: %d cm\r\n", dist);

        xSemaphoreTake(g_servo_angle_mutex, portMAX_DELAY);
        if(dist > 0 && dist <= 30)
        {
            g_servo1_angle = 30;
            g_servo2_angle = 0;
            g_servo3_angle = 30;
			vTaskDelay(400);
			g_servo3_angle = 70;
			vTaskDelay(400);
			g_servo3_angle = 30;
			vTaskDelay(400);
			g_servo1_angle = 90;
            g_servo3_angle = 90;
			vTaskDelay(400);
        }
        xSemaphoreGive(g_servo_angle_mutex);
    }
}

/**
  * @brief  舵机控制任务
  */
void task_servo(void *pvParameters)
{
    static uint8_t last_angle1 = 90;
    static uint8_t last_angle2 = 0;
    static uint8_t last_angle3 = 90;

    while(1)
    {
        if(last_angle1 != g_servo1_angle)
        {
            Servo_SetAngle1(g_servo1_angle);
            last_angle1 = g_servo1_angle;
        }
        if(last_angle2 != g_servo2_angle)
        {
            Servo_SetAngle2(g_servo2_angle);
            last_angle2 = g_servo2_angle;
        }
        if(last_angle3 != g_servo3_angle)
        {
            Servo_SetAngle3(g_servo3_angle);
            last_angle3 = g_servo3_angle;
        }
        vTaskDelay(100);
    }
}

void vApplicationTickHook(void)
{
}

int main(void)
{
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);
    xTaskCreate((TaskFunction_t)task_start,
                (const char*)"task_start",
                START_STK_SIZE,
                NULL,
                START_TASK_PRIO,
                &StartTask_Handler);
    vTaskStartScheduler();
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for(;;);
}

void vApplicationIdleHook(void)
{
}

void vApplicationStackOverflowHook(TaskHandle_t pxTask, char *pcTaskName)
{
    (void)pcTaskName;
    (void)pxTask;
    taskDISABLE_INTERRUPTS();
    for(;;);
}
