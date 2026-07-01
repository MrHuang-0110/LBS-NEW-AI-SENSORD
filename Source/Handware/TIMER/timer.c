#include "timer.h"
#include "stdbool.h"

volatile static uint32_t ticktime;
volatile bool uploadState = false;
uint32_t getTickTime(void)
{ 
  return ticktime;
}

void Timer_Init(void)
{ 
 TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
		NVIC_InitTypeDef NVIC_InitStructure; 
	
		// 开启定时器时钟,即内部时钟CK_INT=32M
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE);
	
		// 自动重装载寄存器的值，累计TIM_Period+1个频率后产生一个更新或者中断
    TIM_TimeBaseStructure.TIM_Period = 1000-1;	

	  // 时钟预分频数为
    TIM_TimeBaseStructure.TIM_Prescaler= 32-1;
 
	  // 初始化定时器
    TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure);
		
		// 清除计数器中断标志位
    TIM_ClearFlag(TIM6, TIM_FLAG_Update);
	  
		// 开启计数器中断
    TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE);
		
		// 使能计数器
    TIM_Cmd(TIM6, ENABLE);	 
	
		// 设置中断来源
    NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;	
		
		// 设置优先级为
    NVIC_InitStructure.NVIC_IRQChannelPriority = 1;	 
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);  
}

void TIM6_IRQHandler(void)
{
	static uint32_t uploadingTick = 0;
  if ( TIM_GetITStatus( TIM6, TIM_IT_Update) != RESET ) 
	{	
		TIM_ClearITPendingBit(TIM6,TIM_FLAG_Update);
		//IWDG_ReloadCounter();
    extern void uploading_data(void);
    extern bool linkState;	
    if(linkState)
    { 
		   
			uploadingTick++;
			if(uploadingTick > 5)
			{
			    uploadState = true;
				  
				  uploadingTick = 0;
			}

			 
		}			
		ticktime++;
	}	
}
