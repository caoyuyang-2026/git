#ifndef __SERVO_H
#define __SERVO_H

#include "stm32f4xx.h"

// 996数字舵机参数
#define SERVO_MIN_ANGLE    0
#define SERVO_MAX_ANGLE    180

#define SERVO_MIN_PWM      600    // 0°
#define SERVO_MAX_PWM      2400   // 180°

void Servo_Init(void);
void Servo_SetAngle1(uint8_t angle);      // PC7 CH2
void Servo_SetAngle2(uint8_t angle);  // PC8 CH3
void Servo_SetAngle3(uint8_t angle);  // PC9 CH4

#endif
