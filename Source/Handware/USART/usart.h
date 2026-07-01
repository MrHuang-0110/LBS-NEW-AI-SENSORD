#ifndef __USART_H
#define __USART_H

#include "hk32f030m.h"

// 串口1-USART1
#define  DEBUG_USARTx                   USART1
#define  DEBUG_USART_CLK                RCC_APB2Periph_USART1
#define  DEBUG_USART_BAUDRATE           115200

// USART GPIO 引脚宏定义
#define  DEBUG_USART_GPIO_APBxClkCmd    RCC_APB2PeriphClockCmd
		
#define  DEBUG_USART_TX_GPIO_CLK        RCC_AHBPeriph_GPIOA   
#define  DEBUG_USART_TX_GPIO_PORT       GPIOA   
#define  DEBUG_USART_TX_GPIO_PIN        GPIO_Pin_3
#define  DEBUG_USART_TX_PIN_SOURCE      GPIO_PinSource3
#define  DEBUG_USART_TX_PIN_AF          GPIO_AF_1

#define  DEBUG_USART_RX_GPIO_CLK        RCC_AHBPeriph_GPIOB   
#define  DEBUG_USART_RX_GPIO_PORT       GPIOB
#define  DEBUG_USART_RX_GPIO_PIN        GPIO_Pin_4
#define  DEBUG_USART_RX_PIN_SOURCE      GPIO_PinSource4
#define  DEBUG_USART_RX_PIN_AF          GPIO_AF_1

#define  DEBUG_USART_IRQ                USART1_IRQn
#define  DEBUG_USART_IRQHandler         USART1_IRQHandler

typedef struct USART_IT_TX {
    uint8_t Buffer[32];  // 发送缓冲区
    uint8_t Len;          // 待发送数据总长度
    uint8_t Index;        // 当前发送位置索引
} Usart1_it_tx;

 
void USART_Config(void);
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch);
void Usart_SendArray( uint8_t *array, uint16_t num);
 
#endif
