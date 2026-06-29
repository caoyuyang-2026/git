#ifndef __AT24C02_H
#define __AT24C02_H

#include "stm32f4xx.h"
#include "sys.h"   // 补上sys.h，识别PBout/PBin宏

//I2C引脚定义
#define EEPROM_SCL_W	PBout(8)
#define EEPROM_SDA_W	PBout(9)
#define EEPROM_SDA_R	PBin(9)

void eeprom_init(void);
void eeprom_sda_pin_mode(GPIOMode_TypeDef pin_mode);
void eeprom_i2c_start(void);
void eeprom_i2c_stop(void);
void eeprom_i2c_send_byte(uint8_t byte);
void eeprom_i2c_send_ack(uint8_t ack);
uint8_t eeprom_i2c_wait_ack(void);
uint8_t eeprom_i2c_recv_byte(void);

int32_t eeprom_page_write(uint8_t word_addr,uint8_t *buf,uint8_t len);
int32_t eeprom_read(uint8_t word_addr,uint8_t *buf,uint8_t len);

#endif
