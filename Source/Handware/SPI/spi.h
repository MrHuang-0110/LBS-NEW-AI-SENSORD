#ifndef __SPI_H
#define __SPI_H

#include "hk32f030m.h"
#include <stdio.h>


#define      SPIx                       SPI1
#define      SPI_APBxClock_FUN           RCC_APB2PeriphClockCmd
#define      SPI_CLK                    RCC_APB2Periph_SPI1

//CS(NSS)引脚 片选选普通GPIO即可
#define      SPI_CS_APBxClock_FUN        RCC_AHBPeriphClockCmd
#define      SPI_CS_CLK                  RCC_AHBPeriph_GPIOC    
#define      SPI_CS_PORT                 GPIOC
#define      SPI_CS_PIN                  GPIO_Pin_4
#define      SPI_CS_GPIO_PinSource       GPIO_PinSource4
#define      SPI_CS_AF                   GPIO_AF_2

//SCK引脚
#define      SPI_SCK_APBxClock_FUN       RCC_AHBPeriphClockCmd
#define      SPI_SCK_CLK                 RCC_AHBPeriph_GPIOC   
#define      SPI_SCK_PORT                GPIOC   
#define      SPI_SCK_PIN                 GPIO_Pin_5
#define      SPI_SCK_GPIO_PinSource      GPIO_PinSource5
#define      SPI_SCK_AF                  GPIO_AF_2


//MISO引脚
#define      SPI_MISO_APBxClock_FUN      RCC_AHBPeriphClockCmd
#define      SPI_MISO_CLK                RCC_AHBPeriph_GPIOC    
#define      SPI_MISO_PORT               GPIOC
#define      SPI_MISO_PIN                		 GPIO_Pin_6
#define      SPI_MISO_GPIO_PinSource     GPIO_PinSource6
#define      SPI_MISO_AF                 GPIO_AF_2

//MOSI引脚
#define      SPI_MOSI_APBxClock_FUN      RCC_AHBPeriphClockCmd
#define      SPI_MOSI_CLK                RCC_AHBPeriph_GPIOC    
#define      SPI_MOSI_PORT               GPIOC 
#define      SPI_MOSI_PIN                GPIO_Pin_7
#define      SPI_MOSI_GPIO_PinSource     GPIO_PinSource7
#define      SPI_MOSI_AF                 GPIO_AF_2

#define  		 CS_LOW()     						GPIO_ResetBits(SPI_CS_PORT, SPI_CS_PIN )
#define  		 CS_HIGH()    						GPIO_SetBits(SPI_CS_PORT, SPI_CS_PIN )

#define      MOSI_LOW()               GPIO_ResetBits(SPI_MOSI_PORT, SPI_MOSI_PIN )
#define      MOSI_HIGH()              GPIO_SetBits(SPI_MOSI_PORT, SPI_MOSI_PIN )

#define      MISO_READ()              GPIO_ReadInputDataBit(SPI_MISO_PORT,SPI_MISO_PIN)
 
#define      SCK_LOW()               GPIO_ResetBits(SPI_SCK_PORT, SPI_SCK_PIN )
#define      SCK_HIGH()							GPIO_SetBits(SPI_SCK_PORT, SPI_SCK_PIN )


#define SPIT_FLAG_TIMEOUT         ((uint32_t)0x14000)

               
typedef unsigned char u8;
typedef unsigned short u16;

void spi_init(void);
uint8_t spi_write_byte(uint8_t byte);
uint8_t spi_red_byte(uint8_t byte);
 uint16_t spi_write_harltByte(uint16_t byte);
uint8_t spi_red_harltByte(uint8_t byte);
#endif

