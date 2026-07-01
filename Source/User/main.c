 
#include "hk32f030m.h" 
#include "data_analysis.h"
#include "usart.h"
#include "queue.h"
#include "timer.h"
#include "senords.h"
#if (KT_MOTOR||BIG_MOTOR||SMALL_MOTOR)
#include "pwm.h"
#include "spi.h"
#include "kt782xx.h"
#else
#if (COLOR)
#include "ltr381xx.h"
#include "bsp_i2c.h"
#endif
#endif
#include "systick_delay.h"
 


	
static void IWDG_Config(uint8_t prv, uint16_t rlv)
{
  IWDG_WriteAccessCmd( IWDG_WriteAccess_Enable);
  IWDG_SetPrescaler( prv );
  IWDG_SetReload( rlv );
  IWDG_ReloadCounter();
  IWDG_Enable();
}
static void IWDG_Feed(void)
{
  IWDG_ReloadCounter();
}
int main(void)
{ 
	FLASH->INT_VEC_OFFSET = FLASH_BASE | 0x2000; /* Vector Table Relocation in Internal FLASH. */	 
	
	rx_queue_init();
	Timer_Init();
	delay_init();
	
	#if KT_MOTOR
	spi_init();
	KT782XX_Init();
	setKT782xxCfg();
	#endif

	#if (KT_MOTOR||BIG_MOTOR||SMALL_MOTOR)
	encorder_init();
	pwm_init();	 
	#else
	#if COLOR
	Sf_I2C_Init();
 
	ltr381_rgb_init();
	ltr381_cfg_init();
	#endif
	#endif
	init_analysis(Usart_SendArray);
	
	USART_Config();
	IWDG_Config(IWDG_Prescaler_128, 49); 
  while (1)
  {    
		  extern void pull_data_from_queue(void);
		  extern volatile bool uploadState;
			pull_data_from_queue();
		   
		  #if 1
		  if(uploadState)
			{ 
				uploading_data(); 
			  uploadState = false;
			}
		   #endif
	 
		  IWDG_Feed();
  }
}




#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(char* file , uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     tex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */	
       /* Infinite loop */
	
	while (1)
  {		
  }
}
#endif /* USE_FULL_ASSERT */


