#ifndef __RC522_H
#define __RC522_H

#include "stm32g0xx_hal.h"

// SPI 外设句柄
extern SPI_HandleTypeDef hspi1;

// RC522 引脚定义
#define RC522_CS_PIN          GPIO_PIN_4
#define RC522_CS_GPIO_PORT    GPIOA
#define RC522_RST_PIN         GPIO_PIN_5
#define RC522_RST_GPIO_PORT   GPIOA

// RC522 命令定义
#define PCD_IDLE              0x00    // 空闲命令
#define PCD_AUTHENT           0x0E    // 认证命令
#define PCD_RECEIVE           0x08    // 接收命令
#define PCD_TRANSCEIVE        0x0C    // 传输接收命令
#define PCD_RESETPHASE        0x0F    // 复位命令

// RC522 寄存器定义
#define CommandReg            0x01    // Command register address
#define ComIrqReg             0x04    // ComIrq register address
#define DivIrqReg             0x05    // DivIrq register address
#define FIFODataReg           0x09    // FIFO Data register address
#define ControlReg            0x0C    // Control register address

// MIFARE 卡密钥（默认 A 密钥）
#define KEY_A                 {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF}

#define MAX_BLOCKS            64   // MIFARE 1K 卡片总共 64 个块
#define SECTOR_SIZE           4    // 每个扇区有 4 个块

// 函数声明
void RC522_Init(void);
void RC522_Select(void);
void RC522_Deselect(void);
uint8_t RC522_SPI_Transmit(uint8_t data);
void RC522_WriteRegister(uint8_t reg, uint8_t value);
uint8_t RC522_ReadRegister(uint8_t reg);
void RC522_Command(uint8_t command);
uint8_t RC522_Authenticate(uint8_t sector, uint8_t block, uint8_t *key);
uint8_t RC522_ReadBlock(uint8_t sector, uint8_t block, uint8_t *buffer);
uint8_t RC522_CheckCard(void);
uint8_t RC522_ReadUID(uint8_t *uid);
void RC522_SPI_Init(void);

#endif /* __RC522_H */
