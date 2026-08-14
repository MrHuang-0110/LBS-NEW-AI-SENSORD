#include "data_analysis.h"
#include "queue.h"
#include "stdbool.h"
#include "pwm.h"
#include "senords.h"
#include "systick_delay.h"
#include "timer.h"
#if (BIG_MOTOR||SMALL_MOTOR||COLOR)
#include "ltr381xx.h"
static DEV_SENORDS _dev;
#endif


bool linkState = false;
static char _pull_data_buf[32];
void uploading_data(void);
 
void pull_data_from_queue(void)
{
	char *data = _pull_data_buf;
	QUEUE_DATA_TYPE *rx_data;			  
	rx_data = cbRead(&rx_queue);  
	if(rx_data == NULL)return;
	
	 uint8_t head = rx_data->head[0],tail =rx_data->head[rx_data->len - 1];
	
   if(head == 0x5A && tail == 0xA5)
	{ 
			  uint8_t data_len = rx_data->head[3];
			  if (data_len > 27) data_len = 27;
			  memset(data,0,32);
			  memcpy(data, rx_data->head+5, data_len);
		    uint8_t typeIndex = rx_data->head[4];	
		    switch(typeIndex)
				{ 
				  case 0x09:
					{ 
						 if(strcmp(data,(const char*)"Please Link") == 0)
						 { 
							  linkState = false;
							  delay_ms(5);
							 
							    SendCOMdata(USER_SourceID,"Play Aplication",strlen("Play Aplication"),0x09);
                #if (BIG_MOTOR||SMALL_MOTOR)							 
							    TIM2->CCR1 = 0;
				          TIM2->CCR2 = 0; 					
                #endif							 
							    linkState = true;
						 }						
					   break;
					}
					#if (BIG_MOTOR||SMALL_MOTOR)
					case 0xED:
					{ 
						 TIM2->CCR1 = (data[0] << 8) | data[1];
				     TIM2->CCR2 = (data[2] << 8) | data[3];							    
					   break;
					}
					case 0xDD:
					{ 
						 encoder_reset_position(0);
						// SendCOMdata(USER_SourceID,"yes",strlen("yes"),0xDD);	
					   break;
					}
					#endif
				   case 0xEE:
					{ 
						// if (rx_data->head[3] == 0 && rx_data->len >= 7)
						// {
						     NVIC_SystemReset();
					//	 }
						 while(1){;};
					}
				}
		}
	  cbReadFinish(&rx_queue);
}


void uploading_data(void)
{
    if (!linkState) return;
    
		  #if (BIG_MOTOR||SMALL_MOTOR)
		   
	    motor_packet_t packet;
    memset(&packet, 0, sizeof(packet));
    
    encoder_update_speed(getTickTime());
    
    int speed = (int)encoder_get_speed();
    int pos = encoder_get_total_position();
    
    int angle = 0;
    int enctord_prr = getEnctordPrr();
    
    if (enctord_prr > 0) {
        angle = (int)((float)pos / (float)enctord_prr * 360.0f);
    }
		#else
			color_packet_t packet;
			memset(&packet, 0, sizeof(packet));
			ltr381_rgb_raw_t rgb;
			float lux;
			uint8_t ready;
			if (ltr381_rgb_data_ready(&ready) == 0 && ready) {
					if (ltr381_rgb_read_raw(&rgb) == 0) {
						packet.ReadRaw = 	rgb.red;
						packet.BlueRaw = 	rgb.blue;
						packet.GreenRaw = rgb.green;						
					}
					if(ltr381_rgb_read_lux(&lux) == 0) {
               packet.lux = lux;
         }
			}
		packet.version = *(volatile unsigned short*)0x08001900;		
    #endif
 
    #if (BIG_MOTOR || SMALL_MOTOR)
		packet.speed = speed;
		packet.pos = pos;
		packet.angle = angle;
		packet.version = *(volatile unsigned short*)0x08001900;
    #endif
		SendCOMdataByte(USER_SourceID,(uint8_t*)&packet, sizeof(packet),0xED);
   // SendCOMdata(USER_SourceID, cache_bufer, sizeof(motor_packet_t), 0xED);
}
