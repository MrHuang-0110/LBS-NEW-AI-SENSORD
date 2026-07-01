#include "usart.h"
#include "queue.h"
#include "timer.h"

#define USART_TX_TIMEOUT_MS  50U  /* ??????????50ms?????????? */


/**
  * @brief  ????????????§Ø??????NVIC
  * @param  ??
  * @retval ??
  */
static void NVIC_Configuration(void)
{
  NVIC_InitTypeDef NVIC_InitStructure;
  
  
  /* ????USART??§Ø?? */
  NVIC_InitStructure.NVIC_IRQChannel = DEBUG_USART_IRQ;
  /* ?????*/
  NVIC_InitStructure.NVIC_IRQChannelPriority =1;
  /* ????§Ø? */
  NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
  /* ?????????NVIC */
  NVIC_Init(&NVIC_InitStructure);
 
}

void USART_RESET(void)
{ 
   DEBUG_USARTx->CR1&=~USART_CR1_UE;
}
 /**
  * @brief  USART GPIO ????,????????????
  * @param  ??
  * @retval ??
  */
void USART_Config(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
   
	USART_DeInit(DEBUG_USARTx);
	
	// ?????GPIO?????
	RCC_AHBPeriphClockCmd(DEBUG_USART_TX_GPIO_CLK | DEBUG_USART_RX_GPIO_CLK, ENABLE);
	
	// ?????????????
	RCC_APB2PeriphClockCmd(DEBUG_USART_CLK, ENABLE);
  
  // ??USART????????IO????
	GPIO_PinAFConfig(DEBUG_USART_TX_GPIO_PORT, DEBUG_USART_TX_PIN_SOURCE, DEBUG_USART_TX_PIN_AF);
	GPIO_PinAFConfig(DEBUG_USART_RX_GPIO_PORT, DEBUG_USART_RX_PIN_SOURCE, DEBUG_USART_RX_PIN_AF);

	// ??USART Tx??GPIO?????????????
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_TX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
	GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(DEBUG_USART_TX_GPIO_PORT, &GPIO_InitStructure);

  // ??USART Rx??GPIO???????????
	GPIO_InitStructure.GPIO_Pin = DEBUG_USART_RX_GPIO_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
	GPIO_Init(DEBUG_USART_RX_GPIO_PORT, &GPIO_InitStructure);
	
	// ???????????????
	// ???¨°?????
	USART_InitStructure.USART_BaudRate = DEBUG_USART_BAUDRATE;
	// ???? ?????????
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	// ??????¦Ë
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	// ????§µ??¦Ë
	USART_InitStructure.USART_Parity = USART_Parity_No ;
	// ?????????????
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	// ???¨´?????????????
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	// ???????????????
	USART_Init(DEBUG_USARTx, &USART_InitStructure);
	USART_ClearITPendingBit(DEBUG_USARTx,USART_IT_RXNE);
	// ?????§Ø??????????
	NVIC_Configuration();
	
	// ??????????§Ø?
  USART_ITConfig(DEBUG_USARTx, USART_IT_RXNE, ENABLE);	
	USART_ITConfig(DEBUG_USARTx, USART_IT_IDLE, ENABLE);	
 
	
	USART_ClearITPendingBit(DEBUG_USARTx, USART_IT_IDLE);
	USART_ClearITPendingBit(DEBUG_USARTx, USART_IT_RXNE);
	// ??????
	USART_Cmd(DEBUG_USARTx, ENABLE);	
 
}

/* send with timeout */
void Usart_SendByte( USART_TypeDef * pUSARTx, uint8_t ch)
{
	uint32_t timeout_tick = getTickTime() + USART_TX_TIMEOUT_MS;
	USART_SendData(pUSARTx,ch);
		
	while (USART_GetFlagStatus(pUSARTx, USART_FLAG_TXE) == RESET)
	{
		if (getTickTime() >= timeout_tick)
			break;
	}
}

/* send array with timeout */
void Usart_SendArray( uint8_t *array, uint16_t num)
{
  uint8_t i;
	for(i=0; i<num; i++)
  {
	  uint32_t timeout_tick = getTickTime() + USART_TX_TIMEOUT_MS;
	  USART_SendData(DEBUG_USARTx,array[i]);
		
	  while (USART_GetFlagStatus(DEBUG_USARTx, USART_FLAG_TXE) == RESET)
	  {
		  if (getTickTime() >= timeout_tick)
			  break;
	  }
  }
 
}
 
void DEBUG_USART_IRQHandler(void)
{
	uint8_t ucCh;
	QUEUE_DATA_TYPE *data_p; 
 
		
	if(USART_GetITStatus(DEBUG_USARTx,USART_IT_RXNE)!=RESET)
	{	
		ucCh  = USART_ReceiveData( DEBUG_USARTx );
		
		/*???§Õ????????????§Õ????????*/
		data_p = cbWrite(&rx_queue); 
		
		if (data_p != NULL)	//?????????¦Ä???????????
		{		
			//????????§Õ???????????????????dma§Õ?????
			*(data_p->head + data_p->len) = ucCh;
				
			if( ++data_p->len >= QUEUE_NODE_DATA_LEN)
			{
				cbWriteFinish(&rx_queue);			
			}
		}
	}
	
	if( USART_GetITStatus( DEBUG_USARTx, USART_IT_IDLE ) == SET )                                         
	{
		USART_ClearITPendingBit(DEBUG_USARTx, USART_IT_IDLE);
		cbWriteFinish(&rx_queue);
		ucCh = USART_ReceiveData( DEBUG_USARTx );
	}
}
