/**
  ******************************************************************************
  * @file    adc.c
  * @brief   This file provides code for the configuration
  *          of the ADC instances.
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
#include "adc.h"
#include "usart.h"

/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA1_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA1_Channel1_IRQn interrupt configuration */
//  HAL_NVIC_SetPriority(DMA1_Channel1_IRQn, 0, 1);
 // HAL_NVIC_EnableIRQ(DMA1_Channel1_IRQn);

}

/* ADC1 init function */
void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */
  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV4;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode = ADC_SCAN_ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.NbrOfConversion = 4;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.Overrun = ADC_OVR_DATA_PRESERVED;
  hadc1.Init.SamplingTimeCommon1 = ADC_SAMPLETIME_39CYCLES_5;
  hadc1.Init.SamplingTimeCommon2 = ADC_SAMPLETIME_1CYCLE_5;
  hadc1.Init.OversamplingMode = DISABLE;
  hadc1.Init.TriggerFrequencyMode = ADC_TRIGGER_FREQ_HIGH;
    if(HAL_ADC_Init(&hadc1) != HAL_OK)
    {
        Error_Handler();
    }
	
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = ADC_REGULAR_RANK_1;
  sConfig.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_1;
  sConfig.Rank = ADC_REGULAR_RANK_2;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_2;
  sConfig.Rank = ADC_REGULAR_RANK_3;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /** Configure Regular Channel
  */
  sConfig.Channel = ADC_CHANNEL_3;
  sConfig.Rank = ADC_REGULAR_RANK_4;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

void HAL_ADC_MspInit(ADC_HandleTypeDef* adcHandle)
{

  GPIO_InitTypeDef GPIO_InitStruct = {0};
  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspInit 0 */

  /* USER CODE END ADC1_MspInit 0 */
    /* ADC1 clock enable */
    __HAL_RCC_ADC_CLK_ENABLE();

    __HAL_RCC_GPIOA_CLK_ENABLE();
    /**ADC1 GPIO Configuration
    PA0     ------> ADC1_IN0
    PA1     ------> ADC1_IN1
    PA2     ------> ADC1_IN2
    PA3     ------> ADC1_IN3
    */
    GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3;
    GPIO_InitStruct.Mode = GPIO_MODE_ANALOG;
    GPIO_InitStruct.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

    /* ADC1 DMA Init */
    /* ADC1 Init */
    hdma_adc1.Instance = DMA1_Channel1;
    hdma_adc1.Init.Request = DMA_REQUEST_ADC1;
    hdma_adc1.Init.Direction = DMA_PERIPH_TO_MEMORY;
    hdma_adc1.Init.PeriphInc = DMA_PINC_DISABLE;
    hdma_adc1.Init.MemInc = DMA_MINC_ENABLE;
    hdma_adc1.Init.PeriphDataAlignment = DMA_PDATAALIGN_HALFWORD;
    hdma_adc1.Init.MemDataAlignment = DMA_MDATAALIGN_HALFWORD;
    hdma_adc1.Init.Mode = DMA_CIRCULAR;
    hdma_adc1.Init.Priority = DMA_PRIORITY_LOW;
    if (HAL_DMA_Init(&hdma_adc1) != HAL_OK)
    {
      Error_Handler();
    }

    __HAL_LINKDMA(adcHandle,DMA_Handle,hdma_adc1);
 
   /* USER CODE BEGIN ADC1_MspInit 1 */
   
  /* USER CODE END ADC1_MspInit 1 */
  }
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef* adcHandle)
{

  if(adcHandle->Instance==ADC1)
  {
  /* USER CODE BEGIN ADC1_MspDeInit 0 */

  /* USER CODE END ADC1_MspDeInit 0 */
    /* Peripheral clock disable */
    __HAL_RCC_ADC_CLK_DISABLE();

    /**ADC1 GPIO Configuration
    PA0     ------> ADC1_IN0
    PA1     ------> ADC1_IN1
    PA2     ------> ADC1_IN2
    PA3     ------> ADC1_IN3
    PA4     ------> ADC1_IN4
    */
    HAL_GPIO_DeInit(GPIOA, GPIO_PIN_0|GPIO_PIN_1|GPIO_PIN_2|GPIO_PIN_3);

    /* ADC1 DMA DeInit */
    HAL_DMA_DeInit(adcHandle->DMA_Handle);
  /* USER CODE BEGIN ADC1_MspDeInit 1 */

  /* USER CODE END ADC1_MspDeInit 1 */
  }
}

/* USER CODE BEGIN 1 */
 
#define STM32_FLASH_BASE 0x08000000 		//STM32 FLASH的起始地址
#define AplicationAddr      STM32_FLASH_BASE + 0x3800
#define AplicationVERSION   STM32_FLASH_BASE + 0X7900
#define ADC_MAX_VALUE  4095
uint8_t LINK_STATE = 0;
__IO uint16_t adcCovValueBuff[SAMPLE_DEPTH][GRAY_CHANNELS] = {0};
 
uint16_t adcAverageBuff[GRAY_CHANNELS] = {0};
uint8_t  adcLineState[GRAY_CHANNELS];
uint32_t calibrateTick;
// 校准数据：每个通道的极值
static uint16_t white_min_adc[GRAY_CHANNELS];   // 白线最小ADC值（最白，值最小）
static uint16_t black_max_adc[GRAY_CHANNELS];   // 黑线最大ADC值（最黑，值最大）
static uint8_t  calib_mode = 0;                  // 0:正常模式 1:校准模式
static uint8_t  calib_done = 0;                   // 0:未校准 1:已校准
static uint32_t calib_startTick = 0;
static uint32_t calib_lastToggle = 0;
static uint8_t  calib_active = 0;
static uint16_t gray_norm[GRAY_CHANNELS];
extern void stop_adc_dma_channle(void);
extern void start_adc_dma_channle(void);
extern void GraySensor_StopCalibration(void);

 

static void GraySensor_Process(void)
{ 
    uint32_t sum[GRAY_CHANNELS] = {0};
    // 累加采样值
    for (int i = 0; i < SAMPLE_DEPTH; i++) {
        for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
            sum[ch] += adcCovValueBuff[i][ch];
        }
    }

    // 计算平均值并更新极值（校准模式）
    for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
        uint16_t adc_val = (uint16_t)(sum[ch] / SAMPLE_DEPTH);
        adcAverageBuff[ch] = adc_val;

        if (calib_mode) {
						for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
            if (adc_val < white_min_adc[ch]) white_min_adc[ch] = adc_val;
            if (adc_val > black_max_adc[ch]) black_max_adc[ch] = adc_val;
						}
						uint32_t now = HAL_GetTick();
						uint32_t elapsed = now - calib_startTick;

						// 检查5秒是否结束
						if (elapsed >= 5000)
						{
								HAL_GPIO_WritePin(LED7_GPIO_Port, LED7_Pin, GPIO_PIN_RESET); // 关LED
								calib_active = 0;            // 校准结束
								GraySensor_StopCalibration(); // 停止传感器校准 
						}

						// 每200ms翻转一次LED
						if (now - calib_lastToggle >= 200)
						{
								HAL_GPIO_TogglePin(LED7_GPIO_Port, LED7_Pin);
								calib_lastToggle = now;
						}
        }
    }

    for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
        uint16_t norm = 500;
        if (calib_done && (black_max_adc[ch] > white_min_adc[ch])) {
            uint32_t range = black_max_adc[ch] - white_min_adc[ch];
            // 修正点：adc_val → adcAverageBuff[ch]
            int32_t tmp = (int32_t)(black_max_adc[ch] - adcAverageBuff[ch]) * 1000 / range;
            if (tmp < 0) tmp = 0;
            if (tmp > 1000) tmp = 1000;
            norm = (uint16_t)tmp;
        }
        gray_norm[ch] = norm;
    }

    // 后续LED控制等保持不变...
    for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
        adcLineState[ch] = (gray_norm[ch] > 500) ? 1 : 0;
    }	
    adcLineState[0] ? LED1_OFF : LED1_ON;
    adcLineState[1] ? LED2_OFF : LED2_ON;
    adcLineState[2] ? LED3_OFF : LED3_ON;
    adcLineState[3] ? LED4_OFF : LED4_ON;	 
}
 
 
// 开始校准：重置极值，进入校准模式
void GraySensor_StartCalibration(void) {
	
    calib_mode = 1;
    calib_done = 0;
	
    for (int ch = 0; ch < GRAY_CHANNELS; ch++) {
        white_min_adc[ch] = ADC_MAX_VALUE;   // 初始化为最大值，以便更新最小值
        black_max_adc[ch] = 0;                // 初始化为0，以便更新最大值
    }
		calib_active = 1;
    calib_startTick = HAL_GetTick();
    calib_lastToggle = calib_startTick;		
}

// 停止校准：退出校准模式，标记校准完成
void GraySensor_StopCalibration(void) {
    calib_mode = 0;
    calib_done = 1;
}

void CalibrateGray(void)
{ 
	 GraySensor_StartCalibration();  
}

static unsigned int FLASH_read(unsigned int adder)
{
  return *(volatile unsigned int*)adder;
}

static void reverseBytesAndStoreSimplified(uint32_t value, uint8_t *array) {
    array[0] = (value & 0x000000FF) << 24; // 提取最低字节并左移24位（实际上是将其放在最高位，但需要先与其他位进行或操作来清零其他位，这里简写为直接赋值后左移）
    array[1] = (value & 0x0000FF00) << 8;  // 提取次低字节并左移8位（同理）
    array[2] = (value & 0x00FF0000) >> 8;  // 提取次高字节并右移8位
    array[3] = (value & 0xFF000000) >> 24; // 提取最高字节并右移24位
 
    // 但上面的方法虽然逻辑上正确，却引入了不必要的位移和与操作。更简洁的方法是直接赋值后交换：
    uint8_t temp[4];
    temp[0] = (value >> 24) & 0xFF;
    temp[1] = (value >> 16) & 0xFF;
    temp[2] = (value >> 8) & 0xFF;
    temp[3] = value & 0xFF;
 
    array[0] = temp[3];
    array[1] = temp[2];
    array[2] = temp[1];
    array[3] = temp[0];
 
}

void updataGrayValue(void)
{ 
  uint32_t sum = 0;
  
	if(LINK_STATE == 0)
	   return;
	
	GraySensor_Process();
	
	int len;
	char versionStr[4];
	static unsigned int version;
	char comxBufer[64];
	version =  FLASH_read(AplicationVERSION);
  reverseBytesAndStoreSimplified(version,(uint8_t*)versionStr);
	memset(comxBufer,0,sizeof(comxBufer));
  len = sprintf(comxBufer,"%d/%d/%d/%d/%d/%d/%d/%d/%.3s/",gray_norm[0],gray_norm[1],gray_norm[2],gray_norm[3],adcLineState[0],adcLineState[1],adcLineState[2],adcLineState[3],versionStr);
 	SendCOMdata(USER_SourceID,comxBufer,len,0xED);
}
 
