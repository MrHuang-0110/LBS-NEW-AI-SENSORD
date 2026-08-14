/**
  ******************************************************************************
  * File Name          : USART.c
  * Description        : This file provides code for the configuration
  *                      of the USART instances.
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usart.h"
#include "stdio.h"
#include "main.h"

/* USER CODE BEGIN 0 */
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->ISR&0X40)==0);//循环发送,直到发送完毕   
    USART1->TDR = (u8) ch;      
	return ch;
}
/* USER CODE END 0 */

UART_HandleTypeDef huart1;

/* USART1 init function */
uint8_t aRxBuffer[1];//HAL库使用的串口接收缓冲
void MX_USART1_UART_Init(void)
{

  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;

	
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  
	HAL_UART_Receive_IT(&huart1, (u8 *)aRxBuffer, 1);
	
}

void HAL_UART_MspInit(UART_HandleTypeDef* uartHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspInit 0 */

  /* USER CODE END USART1_MspInit 0 */
    /* USART1 clock enable */
    __HAL_RCC_USART1_CLK_ENABLE();

    __HAL_RCC_GPIOB_CLK_ENABLE();
    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PB6     ------> USART1_TX
    */
    GPIO_InitStruct.Pin = GPIO_PIN_7|GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
    GPIO_InitStruct.Alternate = GPIO_AF0_USART1;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

    /* USART1 interrupt Init */
    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspInit 1 */
  __HAL_UART_CLEAR_IDLEFLAG(&huart1);
  __HAL_UART_ENABLE_IT(&huart1,UART_IT_IDLE);
	__HAL_UART_ENABLE_IT(&huart1,UART_IT_RXNE);
  /* USER CODE END USART1_MspInit 1 */
  }
}

void HAL_UART_MspDeInit(UART_HandleTypeDef* uartHandle)
{

  if(uartHandle->Instance==USART1)
  {
  /* USER CODE BEGIN USART1_MspDeInit 0 */

  /* USER CODE END USART1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_USART1_CLK_DISABLE();

    /**USART1 GPIO Configuration
    PB7     ------> USART1_RX
    PB6     ------> USART1_TX
    */
    HAL_GPIO_DeInit(GPIOB, GPIO_PIN_7|GPIO_PIN_6);

    /* USART1 interrupt Deinit */
    HAL_NVIC_DisableIRQ(USART1_IRQn);
  /* USER CODE BEGIN USART1_MspDeInit 1 */

  /* USER CODE END USART1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
static void Usart_SendArray( uint8_t *array, uint16_t num)
{
   HAL_UART_Transmit(&huart1,array,num,10);  
	 while(__HAL_UART_GET_FLAG(&huart1,UART_FLAG_TC)!=SET);
}

void SendCOMdata(uint8_t id,char *data,uint8_t len,uint8_t Index)
{ 
	char SendCOMdataBufer[32],Loop = 0;

	uint8_t myCRC = 0;

	memset(SendCOMdataBufer,0,sizeof(SendCOMdataBufer));

	SendCOMdataBufer[0] = 0x5A;
	SendCOMdataBufer[1] = USER_ObjectID;
	SendCOMdataBufer[2] = id;
	SendCOMdataBufer[3] = len;
	SendCOMdataBufer[4] = Index;
	
  strcpy(&SendCOMdataBufer[5],data);
	
	for(Loop = 0;Loop<(SendCOMdataBufer[3]+7)-2;Loop++)
	{ 
	   myCRC+=SendCOMdataBufer[Loop];
	}
	
	myCRC&=0xFF;

	SendCOMdataBufer[(SendCOMdataBufer[3]+7)-2]= myCRC;
	SendCOMdataBufer[(SendCOMdataBufer[3]+7)-1] = 0xA5;

	Usart_SendArray((uint8_t*)SendCOMdataBufer,SendCOMdataBufer[3]+7);
}
/* USER CODE END 1 */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
