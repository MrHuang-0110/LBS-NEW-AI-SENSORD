#ifndef __BSP_I2C_H
#define __BSP_I2C_H

#include "hk32f030m.h"


/**************************I2C参数定义********************************/

#define             sEE_I2C_SCL_GPIO_CLK                    RCC_AHBPeriph_GPIOC     
#define             sEE_I2C_SCL_PORT                        GPIOC   
#define             sEE_I2C_SCL_PIN                         GPIO_Pin_6
#define             sEE_I2C_SCL_SOURCE                      GPIO_PinSource6


#define             sEE_I2C_SDA_GPIO_CLK                    RCC_AHBPeriph_GPIOC
#define             sEE_I2C_SDA_PORT                        GPIOC 
#define             sEE_I2C_SDA_PIN                         GPIO_Pin_5
#define             sEE_I2C_SDA_SOURCE                      GPIO_PinSource5


#define Sf_SCL_H     GPIO_SetBits(sEE_I2C_SCL_PORT, sEE_I2C_SCL_PIN)
#define Sf_SCL_L     GPIO_ResetBits(sEE_I2C_SCL_PORT, sEE_I2C_SCL_PIN)

#define Sf_SDA_H     GPIO_SetBits(sEE_I2C_SDA_PORT, sEE_I2C_SDA_PIN)
#define Sf_SDA_L     GPIO_ResetBits(sEE_I2C_SDA_PORT, sEE_I2C_SDA_PIN)


#define Sf_I2C_ACK_OK     0
#define Sf_I2C_ACK_FAIL   1   


void Sf_I2C_Init(void);
void Sf_I2C_Delay(uint8_t t);
void Sf_I2C_Start(void);
void Sf_I2C_Stop(void);
void Sf_I2C_Ack(void);
void Sf_I2C_Nack(void);
int8_t Sf_I2C_GetAck(void);
void Sf_I2C_SendOneByte(uint8_t dataValue);
uint8_t Sf_I2C_ReceiveOneByte(uint8_t ack_cmd);

#endif












