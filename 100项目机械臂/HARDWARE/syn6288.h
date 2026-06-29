#ifndef __SYN6288_H
#define __SYN6288_H

#include "usart.h"
#include <stdint.h>

// SYN6288 默认波特率 9600
#define SYN6288_BAUD     9600

// 帧结构固定头部
#define SYN_HEAD         0xFD
// 指令码：合成播放
#define SYN_CMD_TTS_PLAY 0x01

// 初始化SYN6288
void syn6288_init(void);
// GBK字符串直接语音播报
void syn6288_speak_gbk(uint8_t *gbk_str);

#endif
