#include "bsp_i2c.h"

void Sf_SDA_OUT(void);
void Sf_SDA_IN(void);
uint8_t Sf_SDA_READ(void);

void Sf_I2C_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStructure;
  
  RCC_AHBPeriphClockCmd( sEE_I2C_SCL_GPIO_CLK | sEE_I2C_SDA_GPIO_CLK, ENABLE);
  
  // Software I2C SDA
  GPIO_InitStructure.GPIO_Pin = sEE_I2C_SDA_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;
  GPIO_Init(sEE_I2C_SDA_PORT, &GPIO_InitStructure);
  
  // Software I2C SCL
  GPIO_InitStructure.GPIO_Pin = sEE_I2C_SCL_PIN;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_Init(sEE_I2C_SCL_PORT, &GPIO_InitStructure);
  
  Sf_SDA_OUT();
  Sf_SCL_H;
  Sf_SDA_H;
}


/**
  * @brief  Set SDA as output
  * @param  None
  * @retval None
  */
void Sf_SDA_OUT(void)
{
  sEE_I2C_SDA_PORT->MODER &= ~(0x3<<(sEE_I2C_SDA_SOURCE*2));
  sEE_I2C_SDA_PORT->MODER |=  (0x1<<(sEE_I2C_SDA_SOURCE*2));
}

/**
  * @brief  Set SDA as input
  * @param  None
  * @retval None
  */
void Sf_SDA_IN(void)
{
  sEE_I2C_SDA_PORT->MODER &= ~(0x3<<(sEE_I2C_SDA_SOURCE*2));
}

/**
  * @brief  Read SDA level
  * @param  None
  * @retval None
  */
uint8_t Sf_SDA_READ(void)
{
  return GPIO_ReadInputDataBit(sEE_I2C_SDA_PORT, sEE_I2C_SDA_PIN);
}


/**
  * @brief  Delay some time
  * @param  None
  * @retval None
  */
void Sf_I2C_Delay(uint8_t t)
{
  t = t*5;
  while(--t);
}

/**
  * @brief  Generate iic start condition
  * @param  None
  * @retval None
  */
void Sf_I2C_Start(void)
{ 
  Sf_SDA_H;
  Sf_SDA_OUT();
  Sf_SCL_H;
  Sf_I2C_Delay(10);
  Sf_SDA_L;
  Sf_I2C_Delay(10);
  Sf_SCL_L;
}

/**
  * @brief  Generate iic stop condition
  * @param  None
  * @retval None
  */
void Sf_I2C_Stop(void)
{
  Sf_SDA_OUT();
  Sf_SCL_L;
  Sf_I2C_Delay(5);
  Sf_SDA_L;
  Sf_I2C_Delay(5);
  Sf_SCL_H;
  Sf_I2C_Delay(5);
  Sf_SDA_H;
  Sf_I2C_Delay(5);
}

/**
  * @brief  Generate an acknowledge
  * @param  
  * @retval 
  */
void Sf_I2C_Ack(void)
{
  Sf_SCL_L;
  Sf_SDA_OUT();
  Sf_SDA_L;
  Sf_I2C_Delay(10);
  Sf_SCL_H;
  Sf_I2C_Delay(10);
  Sf_SCL_L;
}

/**
  * @brief  Generate an no acknowledge
  * @param  
  * @retval 
  */
void Sf_I2C_Nack(void)
{
  Sf_SCL_L;
  Sf_SDA_OUT();
  Sf_SDA_H;
  Sf_I2C_Delay(10);
  Sf_SCL_H;
  Sf_I2C_Delay(10);
  Sf_SCL_L;
}

/**
  * @brief  Get ack state
  * @param  None
  * @retval The ack state, IIC_OK or IIC_ERR
  */
int8_t Sf_I2C_GetAck(void)
{
  uint8_t timeout = 0;
  
  Sf_SDA_IN();
  Sf_I2C_Delay(4);
  Sf_SCL_H;
  Sf_I2C_Delay(3);
  while(Sf_SDA_READ())
  {
    timeout++;
    if( timeout > 200)
    {
      Sf_I2C_Stop();
      return Sf_I2C_ACK_FAIL;
    }
  }
  Sf_SCL_L;
  Sf_I2C_Delay(3);
  return Sf_I2C_ACK_OK;
}

/**
  * @brief  Send one byte by iic
  * @param  dataValue: the data to be send
  * @retval None
  */
void Sf_I2C_SendOneByte(uint8_t dataValue)
{
  uint8_t bit_n;
  
  Sf_SDA_OUT();
  Sf_SCL_L;
  for( bit_n = 0; bit_n < 8; bit_n ++ )
  {
    if(dataValue&0x80)
    {
      Sf_SDA_H;
    }
    else
    {
      Sf_SDA_L;
    }
    
    dataValue <<= 1;
    Sf_I2C_Delay(5);
    Sf_SCL_H;
    Sf_I2C_Delay(10);
    Sf_SCL_L;
    Sf_I2C_Delay(3);
  }
}

/**
  * @brief  Receive one byte by iic
  * @param  adk_cmd: Send an acknowledge or not, 
  *         1 - send an acknowledge
  *         0 - don't send an acknowledge
  * @retval the received one byte data
  */
uint8_t Sf_I2C_ReceiveOneByte(uint8_t ack_cmd)
{
  uint8_t bit_n;
  uint8_t dataValue =0x00;
  
  Sf_SDA_IN();
  for( bit_n = 0; bit_n < 8; bit_n ++ )
  {
    Sf_SCL_L;
    Sf_I2C_Delay(10);
    Sf_SCL_H;
    dataValue <<= 1;
    if( Sf_SDA_READ() )
    {
      dataValue |= 0x01;
    }
    Sf_I2C_Delay(10);
  }
  
  if( !ack_cmd )
  {
    Sf_I2C_Nack();
  }
  else
  {
    Sf_I2C_Ack();
  }
  
  return dataValue;
}



