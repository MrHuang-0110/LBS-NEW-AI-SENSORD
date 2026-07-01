#include "spi.h"

uint8_t spi_write_byte(uint8_t byte)
{  
  /* 等待发送缓冲区为空，TXE事件 */
  while (SPI_I2S_GetFlagStatus(SPI1 , SPI_I2S_FLAG_TXE) == RESET);
	
  /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
  SPI_SendData8(SPI1 , byte);
  
  /* 等待接收缓冲区非空，RXNE事件 */
  while (SPI_I2S_GetFlagStatus(SPI1 , SPI_I2S_FLAG_RXNE) == RESET);
 
	 uint8_t Data8 = SPI_ReceiveData8(SPI1);
  
  /* 读取数据寄存器，获取接收缓冲区数据 */
  return Data8; 
}

uint8_t spi_red_byte(uint8_t byte)
{ 
   return spi_write_byte(byte);
}

uint16_t spi_write_harltByte(uint16_t byte)
{  
  /* 等待发送缓冲区为空，TXE事件 */
  while (SPI_I2S_GetFlagStatus(SPI1 , SPI_I2S_FLAG_TXE) == RESET);
	
  /* 写入数据寄存器，把要写入的数据写入发送缓冲区 */
  SPI_I2S_SendData16(SPI1 , byte);
  
  /* 等待接收缓冲区非空，RXNE事件 */
  while (SPI_I2S_GetFlagStatus(SPI1 , SPI_I2S_FLAG_RXNE) == RESET);
 
	 uint16_t Data16 = SPI_I2S_ReceiveData16(SPI1);
  
  /* 读取数据寄存器，获取接收缓冲区数据 */
  return Data16; 
}

uint8_t spi_red_harltByte(uint8_t byte)
{ 
   return spi_write_harltByte(byte);
}

void spi_init(void){ 
  
	 
	SPI_InitTypeDef  SPI_InitStructure;
  GPIO_InitTypeDef GPIO_InitStructure;

  	/* 使能SPI时钟 */
	SPI_APBxClock_FUN ( SPI_CLK, ENABLE );
  
  	/* 使能SPI引脚相关的时钟 */
 	SPI_CS_APBxClock_FUN ( SPI_CS_CLK|SPI_SCK_CLK|
												 SPI_MISO_CLK|SPI_MOSI_CLK, ENABLE );
  
  /* 配置GPIO的复用功能连接 */
  GPIO_PinAFConfig(SPI_SCK_PORT, SPI_SCK_GPIO_PinSource, SPI_SCK_AF);
  GPIO_PinAFConfig(SPI_MISO_PORT, SPI_MISO_GPIO_PinSource, SPI_MISO_AF);
  GPIO_PinAFConfig(SPI_MOSI_PORT, SPI_MOSI_GPIO_PinSource, SPI_MOSI_AF);
  
  
  /* 配置SPI的 CS引脚，普通IO即可 */
  GPIO_InitStructure.GPIO_Pin = SPI_CS_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Schmit = GPIO_Schmit_Disable;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_Init(SPI_CS_PORT,&GPIO_InitStructure);
  
  /* 配置SPI的 SCK引脚*/
  GPIO_InitStructure.GPIO_Pin = SPI_SCK_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_Init(SPI_SCK_PORT, &GPIO_InitStructure);

  /* 配置SPI的 MISO引脚*/
  GPIO_InitStructure.GPIO_Pin = SPI_MISO_PIN;
  GPIO_Init(SPI_MISO_PORT, &GPIO_InitStructure);

  /* 配置SPI的 MOSI引脚*/
  GPIO_InitStructure.GPIO_Pin = SPI_MOSI_PIN;
  GPIO_Init(SPI_MOSI_PORT, &GPIO_InitStructure);

  /* 停止信号: CS引脚高电平*/
   CS_HIGH();

  
  /* SPI1配置 */
  SPI_InitStructure.SPI_Direction = SPI_Direction_2Lines_FullDuplex;
  
  SPI_InitStructure.SPI_Mode = SPI_Mode_Master;
  SPI_InitStructure.SPI_DataSize = SPI_DataSize_16b;
  SPI_InitStructure.SPI_CPOL = SPI_CPOL_High;
  SPI_InitStructure.SPI_CPHA = SPI_CPHA_2Edge;
  SPI_InitStructure.SPI_NSS = SPI_NSS_Soft;
  SPI_InitStructure.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
  
  SPI_InitStructure.SPI_FirstBit = SPI_FirstBit_MSB;
  SPI_InitStructure.SPI_CRCPolynomial =7;
  SPI_Init(SPIx, &SPI_InitStructure);
  
  /* Initialize the FIFO threshold */
	SPI_RxFIFOThresholdConfig(SPIx, SPI_RxFIFOThreshold_HF);
  
  SPI_Cmd(SPIx, ENABLE);
}
