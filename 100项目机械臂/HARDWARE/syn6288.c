#include "syn6288.h"
#include <string.h>

#define FRAME_BUF_SIZE  256

// 异或校验计算函数
static uint8_t SYN_Calc_Check(uint8_t *buf, uint16_t len)
{
    uint8_t check = 0;
    while(len--)
    {
        check ^= *buf++;
    }
    return check;
}

void syn6288_init(void)
{
    uart2_init(9600);
}

void syn6288_speak_gbk(uint8_t *gbk_str)
{
    uint8_t frame[FRAME_BUF_SIZE] = {0};
    uint16_t index = 0;
    uint16_t str_len = strlen((char *)gbk_str);
    uint16_t i;
    uint8_t check;

    /* 有效载荷：指令(1字节)+编码标识(1字节)+文本内容N字节 */
    //uint16_t payload_len = 2 + str_len;
	uint16_t payload_len = 2 + 1 + str_len;

    // 帧头 FD
    frame[index++] = 0xFD;
    // 有效载荷长度 高字节、低字节
    frame[index++] = (payload_len >> 8) & 0xFF;
    frame[index++] = payload_len & 0xFF;

    // 播放指令 0x01
    frame[index++] = 0x01;
    // 编码选择 0x00=GBK
    frame[index++] = 0x00;

    // 填充GBK字符串
    for(i = 0; i < str_len; i++)
    {
        frame[index++] = gbk_str[i];
    }

    /* SYN6288校验范围：从【长度高字节】开始，到文本末尾，不含帧头FD */
    //check = SYN_Calc_Check(&frame[1], index - 1);
	check = SYN_Calc_Check(frame, index);
    frame[index++] = check;

    // 逐字节发送整帧数据
    for(i = 0; i < index; i++)
    {
        uart2_send_data(frame[i]);
    }
}