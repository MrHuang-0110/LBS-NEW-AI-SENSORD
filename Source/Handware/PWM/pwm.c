#include "math.h"
#include "pwm.h"
#include "timer.h"

#define FILTER_BUFER_SIZE 16

typedef struct {
    // ??????????
    int32_t current_pos;           // ??????????????
    int32_t last_pos;              // ???????
    int32_t position_buffer[FILTER_BUFER_SIZE];    // ?????????
    uint32_t time_buffer[FILTER_BUFER_SIZE];       // ????????
    uint8_t buffer_index;          // ??????????
    float speed;                   // ???????????
    float speed_filtered;          // ?????????
 
	  uint32_t last_time;
	
    // ????????????
    uint16_t last_hw_cnt;          // ???????????
    int32_t total_position;        // 32?????????
    uint32_t encoder_ppr;          // ??????????????
    uint32_t last_update_time;     // ???????????
} EncoderData_TypeDef;

static EncoderData_TypeDef encoder_speed  = {0};
static uint8_t encoder_buffer_count = 0;
static void refresh_encoder_speed(uint32_t current_time);
/**
 * ???????????????????
 * ?????????????????????1ms??10ms??
 * @param current_time ????????tick??
 */
void encoder_update_speed(uint32_t current_time)
{
    uint16_t current_cnt = TIM1->CNT;  // ????????????
    int16_t diff;
    
    // ??????????????????????
    diff = (int16_t)(current_cnt - encoder_speed.last_hw_cnt);
    
    // ???????????
    encoder_speed.total_position += diff;
    
    // ?????????????????????
    encoder_speed.current_pos = encoder_speed.total_position;
    
    // ??????????????
    encoder_speed.last_hw_cnt = current_cnt;
    
    // ????????????
    refresh_encoder_speed(current_time);
    
    // ?????????????
    encoder_speed.last_update_time = current_time;
}

/**
 * ????????????????????????refresh_speed????
 */
static void refresh_encoder_speed(uint32_t current_time)
{ 
    // 1. ?????????????????????
    encoder_speed.position_buffer[encoder_speed.buffer_index] = encoder_speed.current_pos;
    encoder_speed.time_buffer[encoder_speed.buffer_index] = current_time;  // ???????
    encoder_speed.buffer_index = (encoder_speed.buffer_index + 1) % FILTER_BUFER_SIZE;

    if (encoder_buffer_count < FILTER_BUFER_SIZE) {
        encoder_buffer_count++;
        return;
    }

    // 2. ?????????????????
    int32_t oldest_pos = encoder_speed.position_buffer[encoder_speed.buffer_index];
    uint32_t oldest_time_ms =encoder_speed.time_buffer[encoder_speed.buffer_index];
    
    int32_t newest_pos = encoder_speed.position_buffer[(encoder_speed.buffer_index + (FILTER_BUFER_SIZE - 1)) % FILTER_BUFER_SIZE];
    uint32_t newest_time_ms = encoder_speed.time_buffer[(encoder_speed.buffer_index + (FILTER_BUFER_SIZE - 1)) % FILTER_BUFER_SIZE];
    
    // 3. ????????????
    uint32_t time_diff_ms = newest_time_ms - oldest_time_ms;
    
    // ??????????????
    if (time_diff_ms == 0) {
        return;  // ?????0???????????
    }
    
    // 4. ??????
    float time_diff_sec = (float)time_diff_ms / 1000.0f;  // ???????
    
    if (time_diff_sec > 0) {
        int32_t pos_diff = newest_pos - oldest_pos;  // ???????????????
        
        // 5. ???????/???
        // RPM = (?????? / PPR) / ???(??) ?? 60
        float revolutions = (float)pos_diff / (float)encoder_speed.encoder_ppr;
        float raw_rpm = (revolutions / time_diff_sec) * 60.0f;
 
        // 7. ??????
        const float alpha = 0.3f;
        encoder_speed.speed_filtered = (1.0f - alpha) * encoder_speed.speed_filtered + alpha * raw_rpm;
        
        // 8. dead zone
        if (fabsf(encoder_speed.speed_filtered) < 0.5f) {
            encoder_speed.speed_filtered = 0.0f;
        }

        encoder_speed.speed = encoder_speed.speed_filtered;
    }
    
    encoder_speed.last_pos = encoder_speed.current_pos;
    encoder_speed.last_time = current_time;
}

/**
 * ???????????????????????????
 * ??????????????????
 */
int16_t get_encoder_increment(void)
{
    static uint16_t last_cnt_for_increment = 0;
    
    uint16_t current_cnt = TIM1->CNT;
    int16_t diff = (int16_t)(current_cnt - last_cnt_for_increment);
    
    last_cnt_for_increment = current_cnt;
    
    return diff;
}

/**
 * ????????????????
 */
int32_t encoder_get_total_position(void)
{
    return encoder_speed.total_position;
}

/**
 * ??????????RPM??
 */
float encoder_get_speed(void)
{
    return encoder_speed.speed;
}

uint32_t getEnctordPrr(void)
{
  return encoder_speed.encoder_ppr;
}
/**
 * ?????????????
 * @param reset_type 0=??????????????1=???????????????
 */
void encoder_reset_position(uint8_t reset_type)
{
    if (reset_type == 1) {
        // ?????????????????????????
        __disable_irq();
        TIM1->CNT = 32768;
        encoder_speed.last_hw_cnt = TIM1->CNT;
        __enable_irq();
    }
    
    // ????????????
    encoder_speed.total_position = 0;
    encoder_speed.current_pos = 0;
    
    // ?????????????
    encoder_speed.buffer_index = 0;
    encoder_buffer_count = 0;
    for(int i = 0; i < FILTER_BUFER_SIZE; i++) {
        encoder_speed.position_buffer[i] = 0;
        encoder_speed.time_buffer[i] = getTickTime();
    }
    
    encoder_speed.speed = 0.0f;
    encoder_speed.speed_filtered = 0.0f;
}
#if 0
void encorder_init(void)
{ 
 
		TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_ICInitTypeDef TIM_ICInitStructure;
	
    /* TIM1 2 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    /* GPIOA clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);

    /* TIM1 channel 2 pin (PD.01,PD?02) configuration */
    GPIO_InitStructure.GPIO_Pin =  GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOD, &GPIO_InitStructure);

		/*??????IO??*/
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
	
    /* Connect TIM pins to AF */
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_3);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 65535;
    TIM_TimeBaseStructure.TIM_Prescaler = 15-1;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
		
    #if 1
    //???1
    //??????????????????????????????????????????
     TIM_ICStructInit(&TIM_ICInitStructure);
    //??????
     TIM_ICInitStructure.TIM_Channel = TIM_Channel_1;
    //??????????????? 						
     TIM_ICInitStructure.TIM_ICFilter = 0;
    //????????????? ?????????????????????????										
     TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;	
     TIM_ICInit(TIM1, &TIM_ICInitStructure);
 
     //???2
     TIM_ICStructInit(&TIM_ICInitStructure);
     TIM_ICInitStructure.TIM_Channel = TIM_Channel_2;
     TIM_ICInitStructure.TIM_ICFilter = 0;
     TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
     TIM_ICInit(TIM1, &TIM_ICInitStructure);
     #endif
		 
    /* Prescaler configuration */
    //  TIM_PrescalerConfig(TIM1, 0, TIM_PSCReloadMode_Immediate);
		 //TIM_PrescalerConfig(TIM1, (uint16_t)((SystemCoreClock ) / 6000000) - 1, TIM_PSCReloadMode_Immediate);
    TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Falling);
    //TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising, TIM_ICPolarity_Falling);
 
    /* TIM Interrupts enable */
    TIM_ITConfig(TIM1, TIM_IT_Update, ENABLE);
    /* TIM1 enable counter */
    TIM_Cmd(TIM1, ENABLE);
 
    /*???????*/
	  GPIO_ResetBits(GPIOD,GPIO_Pin_6);
		 
}
#else
void encorder_init(void)
{ 
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    GPIO_InitTypeDef GPIO_InitStructure;
    
    /* TIM1 clock enable */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_TIM1, ENABLE);
    /* GPIOD clock enable */
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);

    /* TIM1 channel 1&2 pins (PD.01, PD.02) configuration */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1 | GPIO_Pin_2;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;  // ??????
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       // ????????????
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    /* ??????IO?? */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
    GPIO_Init(GPIOD, &GPIO_InitStructure);
    
    /* Connect TIM pins to AF */
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource1, GPIO_AF_3);
    GPIO_PinAFConfig(GPIOD, GPIO_PinSource2, GPIO_AF_3);

    /* Time base configuration */
    TIM_TimeBaseStructure.TIM_Period = 65535;          // 16??????
    TIM_TimeBaseStructure.TIM_Prescaler = 8-1;           // ?????????????
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;

    TIM_TimeBaseInit(TIM1, &TIM_TimeBaseStructure);
 
 
    TIM_EncoderInterfaceConfig(TIM1, TIM_EncoderMode_TI12, 
                              TIM_ICPolarity_Rising,     // ???1????
                              TIM_ICPolarity_Falling);   // ???2????
    
    /* ????????????????????????????? */
    // ????IC1??IC2????????4??????????????0x4??
    // CCMR1???????IC1F??[7:4]????IC2F??[15:12]??
    TIM1->CCMR1 = (TIM1->CCMR1 & ~(0xF << 4)) | (0x4 << 4);   // ???1???
    TIM1->CCMR1 = (TIM1->CCMR1 & ~(0xF << 12)) | (0x4 << 12); // ???2???
		
															
   /* ???????????????????? */
    TIM1->CNT = 32768;
    
    /* ?????????????????? */
    encoder_speed.last_hw_cnt = TIM1->CNT;
    encoder_speed.total_position = 0;
    encoder_speed.current_pos = 0;
    encoder_speed.last_pos = 0;
    encoder_speed.buffer_index = 0;
    encoder_buffer_count = 0;
    encoder_speed.speed = 0.0f;
    encoder_speed.speed_filtered = 0.0f;
   
    // ?????????PPR??????????????????
    #if BIG_MOTOR
    encoder_speed.encoder_ppr = 90;  // ?????2000????/?
    #else
    encoder_speed.encoder_ppr = 62;  // ????
    #endif
    
    // ????????
    for(int i = 0; i < FILTER_BUFER_SIZE; i++) {
        encoder_speed.position_buffer[i] = 0;
        encoder_speed.time_buffer[i] = getTickTime();
    }
    
    encoder_speed.last_update_time = getTickTime();
    
    /* ????????? */
    TIM_Cmd(TIM1, ENABLE);
    
    /* ??????? */
    GPIO_ResetBits(GPIOD, GPIO_Pin_6);
}
#endif
void pwm_init(void)
{ 
  GPIO_InitTypeDef GPIO_InitStructure;
	
 
	TIM_OCInitTypeDef  TIM_OCInitStructure;
  TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;

	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_GPIOD, ENABLE);
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);
	
 
	/*TIMER_CH1 2 PD3 4*/
 
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource3, GPIO_AF_4);
	GPIO_PinAFConfig(GPIOD, GPIO_PinSource4, GPIO_AF_4);
	
 /*GPIO PD3 Init*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Schmit = GPIO_Schmit_Disable;
  GPIO_Init(GPIOD, &GPIO_InitStructure);
 /*GPIO PD4 Init*/
  GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
  GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_10MHz;
  GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
  GPIO_InitStructure.GPIO_Schmit = GPIO_Schmit_Disable;
  GPIO_Init(GPIOD, &GPIO_InitStructure);	
 
	TIM_TimeBaseStructure.TIM_Period = 100-1;  //100 5KHZ
	TIM_TimeBaseStructure.TIM_Prescaler= 320-1;//63  5KHZ 320 1KHZ
	TIM_TimeBaseStructure.TIM_ClockDivision=TIM_CKD_DIV1;	
	TIM_TimeBaseStructure.TIM_CounterMode=TIM_CounterMode_Up;
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);
	
  /*Timer2 PD3 CH1*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC1Init(TIM2, &TIM_OCInitStructure);
	TIM_OC1PreloadConfig(TIM2, TIM_OCPreload_Enable);

  /*Timer2 PD4 CH2*/
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
	TIM_OCInitStructure.TIM_Pulse = 0;
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
  TIM_OC2Init(TIM2, &TIM_OCInitStructure);
	TIM_OC2PreloadConfig(TIM2, TIM_OCPreload_Enable);
 
	
  TIM_CtrlPWMOutputs(TIM2, ENABLE);
	TIM_Cmd(TIM2, ENABLE);	
}
