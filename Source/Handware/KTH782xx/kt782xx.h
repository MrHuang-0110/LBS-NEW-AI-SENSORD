#ifndef __KT782XX_H
#define __KT782XX_H
#include "stdbool.h"
#include "stdint.h"

//typedef unsigned char uint8_t;
//typedef unsigned int  uint32_t;
//typedef unsigned short uint16_t;

 
#define compose_kt782xx_fram(addr,data) (1 << 15)|(0 << 14) |((addr & 0x3F) << 8) |(data & 0xFF)
#define compose_read_fram(addr,data) 		(0 << 15)|(1 << 14) |((addr & 0x3F) << 8) |(data & 0xFF)

#define regToangle(reg) (reg/65535)*360
 
typedef struct{ 
	uint8_t (*writeByte)(uint8_t);
	uint8_t (*readByte)(uint8_t);
}DEV_IO;


void KT782XX_Init(void);
void setKT782xxCfg(void);
void close_kt78xxcfg(void);
uint16_t read_angle(void);
void kt_set_zero(void);
float calculate_angle_with_zero_offset(uint16_t spi_raw_value);
#endif
