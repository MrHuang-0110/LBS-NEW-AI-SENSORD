#include "driver_gray.h"
#include "rx_data_queue.h"
#include "agreement_comx.h"
#include "usart.h"
#include "stmflash.h"
#include <string.h>

/* 硬件配置 */
//#define ADC_SAMPLE_COUNT         5      // 每次采集的样本数
#define ADC_MEDIAN_SAMPLE_COUNT  5      // 中值滤波样本数
#define ADC_AVERAGE_SAMPLE_COUNT 3      // 平均值滤波样本数（取中间3个）
#define CALIBRATION_SAMPLE_COUNT 10     // 校准采样次数
#define CALIBRATION_DELAY_MS     2000   // 校准步骤间延迟
#define CALIBRATION_BLINK_MS     200    // 校准提示闪烁周期
#define DATA_UPDATE_INTERVAL_MS  10     // 数据更新间隔（主循环调用）

/* 静态全局变量 */
static driver_gray_t gray_driver;                        // 驱动程序状态
static uint32_t ambient_values[GRAY_NUM_CHANNELS];       // 环境光值（LED关闭）
static uint32_t reflect_values[GRAY_NUM_CHANNELS];       // 反射值（LED开启）
static uint32_t effective_values[GRAY_NUM_CHANNELS];     // 有效值（反射-环境）
static uint32_t filtered_values[GRAY_NUM_CHANNELS];      // 滤波后值（用于归一化）

static uint32_t adc_sample_buffer[ADC_MEDIAN_SAMPLE_COUNT][GRAY_NUM_CHANNELS]; // ADC采样缓冲区
static uint32_t adc_sample_bufferv2[ADC_MEDIAN_SAMPLE_COUNT][GRAY_NUM_CHANNELS]; // ADC采样缓冲区
/* 兼容旧代码的状态变量 */
static volatile uint8_t linke_state = 0;     // 连接状态
static volatile uint8_t button_pressed = 0;  // 按钮按下标志
static uint8_t machine_is_calibrate = 0;

/* 静态函数声明 */
static void adc_sample_with_filter(void);
static void update_effective_values(void);
static void apply_moving_average_filter(void);
static uint32_t median_filter(uint32_t *samples, uint8_t count);
static void calibration_state_machine(void);
static void calibration_white_step(void);
static void calibration_black_step(void);
static void calibration_complete_step(void);
static void calibration_user_prompt(uint8_t step);
static void send_calibration_prompt(uint8_t step);
static uint16_t normalize_value(uint8_t ch, uint32_t value);

/*----------------------------------------------------------------------------*\
 * LED控制函数
\*----------------------------------------------------------------------------*/
static void _delay_us(uint32_t nus) 
{
    uint32_t ticks = nus * 64;               
    uint32_t start_val = SysTick->VAL;       
    uint32_t elapsed_ticks = 0;

    while (elapsed_ticks < ticks) {
        uint32_t current_val = SysTick->VAL;
        if (current_val <= start_val) {
            elapsed_ticks += start_val - current_val;
        } else {
            elapsed_ticks += (SysTick->LOAD - current_val) + start_val;
        }
        start_val = current_val;
    }
}
 
void close_all_led(void)
{
    _IO_LED0(0);
    _IO_LED1(0);
    _IO_LED2(0);
    _IO_LED3(0);
    _IO_LED4(0);
    _IO_LED5(0);
    _IO_LED6(0);
}

void open_all_led(void)
{
    _IO_LED0(1);
    _IO_LED1(1);
    _IO_LED2(1);
    _IO_LED3(1);
    _IO_LED4(1);
    _IO_LED5(1);
    _IO_LED6(1);
}

void set_gray_led(COLOR_STATE color)
{
    switch((uint8_t)color)
    {
        case BLACK:
            _IO_RED(0); _IO_GREEN(0); _IO_BLUE(0);
            break;
        case RED:
            _IO_RED(1); _IO_GREEN(0); _IO_BLUE(0);
            break;
        case GREEN:
            _IO_RED(0); _IO_GREEN(1); _IO_BLUE(0);
            break;
        case BLUE:
            _IO_RED(0); _IO_GREEN(0); _IO_BLUE(1);
            break;
        case YELLO:
            _IO_RED(1); _IO_GREEN(1); _IO_BLUE(0);
            break;
        case PINK_RED:
            _IO_RED(1); _IO_GREEN(0); _IO_BLUE(1);
            break;
        case CYAN:
            _IO_RED(0); _IO_GREEN(1); _IO_BLUE(1);
            break;
        case WHITE:
            _IO_RED(1); _IO_GREEN(1); _IO_BLUE(1);
            break;
        default:
            _IO_RED(0); _IO_GREEN(0); _IO_BLUE(0);
            break;
    }
}

 
uint8_t Gray_SaveConfig(const driver_gray_t* cfg) {
    return FLASH_write(CFG_FILE_ADDR, (uint8_t*)cfg, sizeof(driver_gray_t));
}
uint8_t Gray_LoadConfig(driver_gray_t* cfg) {
    memcpy(cfg, (const void*)CFG_FILE_ADDR, sizeof(driver_gray_t));
    if (cfg->magic != 0xA5A5A5A5) {
        return 0;  
    }
    return 1;
}

/*----------------------------------------------------------------------------*\
 * 初始化函数
\*----------------------------------------------------------------------------*/

void gray_driver_init(void)
{
    memset(&gray_driver, 0, sizeof(gray_driver));
    
	  if(Gray_LoadConfig(&gray_driver)!=1)
		{ 
			uint32_t white_value[] = {1131,1920,1665,2213,1059,1531,1136};
			uint32_t black_value[] = {603,900,743,944,432,653,512};
			uint32_t threshold[] = {264,510,461,634,313,439};
			// 初始化校准数据
			for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
			{
        gray_driver.ch[i].white_value = white_value[i];
        gray_driver.ch[i].black_value = black_value[i];
        gray_driver.ch[i].threshold = threshold[i];
        gray_driver.ch[i].calibrated = 0;
			   
			}  		
      gray_driver.magic = 0xA5A5A5A5;
			gray_driver.calibrate_led = WHITE;
			
		}
		
		gray_driver.calib_state = CALIB_STATE_IDLE;
		gray_driver.calib_step = 0;
		gray_driver.calib_timestamp = 0;	
    
    // 兼容历史配置：确保阈值始终在归一化范围内
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        if (gray_driver.ch[i].threshold > GRAY_NORMALIZED_MAX)
        {
            gray_driver.ch[i].threshold = GRAY_NORMALIZED_MAX;
        }
    }
		
    // 初始化滤波缓冲区
    memset(ambient_values, 0, sizeof(ambient_values));
    memset(reflect_values, 0, sizeof(reflect_values));
    memset(effective_values, 0, sizeof(effective_values));
    memset(filtered_values, 0, sizeof(filtered_values));

    // 设置校准LED颜色
    set_gray_led((COLOR_STATE)gray_driver.calibrate_led);
}

void gray_calibration_init(void)
{
    // 重置所有通道的校准数据
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        gray_driver.ch[i].white_value = 0;
        gray_driver.ch[i].black_value = 4095;
        gray_driver.ch[i].calibrated = 0;
    }
    gray_driver.calib_state = CALIB_STATE_IDLE;
}

/*----------------------------------------------------------------------------*\
 * 校准控制API
\*----------------------------------------------------------------------------*/

void gray_calibration_start(void)
{
    if (gray_driver.calib_state == CALIB_STATE_IDLE)
    {
        gray_driver.calib_state = CALIB_STATE_WHITE_PREPARE;
        gray_driver.calib_step = 0;
        gray_driver.calib_timestamp = HAL_GetTick();

        // 发送校准开始提示
        send_calibration_prompt(1); // 步骤1：准备白线
    }
}

void gray_calibration_stop(void)
{
    gray_driver.calib_state = CALIB_STATE_IDLE;
    // 恢复正常LED颜色
    set_gray_led((COLOR_STATE)gray_driver.calibrate_led);
}

void gray_calibration_reset(void)
{
    gray_calibration_init();
}

uint8_t gray_calibration_is_active(void)
{
    return (gray_driver.calib_state != CALIB_STATE_IDLE);
}

calib_state_t gray_calibration_get_state(void)
{
    return gray_driver.calib_state;
}

void gray_calibration_process(void)
{
    if (gray_driver.calib_state != CALIB_STATE_IDLE)
    {
        calibration_state_machine();
    }
}

/*----------------------------------------------------------------------------*\
 * 阈值设置API
\*----------------------------------------------------------------------------*/

void gray_set_threshold(uint8_t ch, uint16_t threshold)
{
    if (ch >= GRAY_NUM_CHANNELS) return;
    if (threshold > GRAY_NORMALIZED_MAX) threshold = GRAY_NORMALIZED_MAX;
    gray_driver.ch[ch].threshold = threshold;
}

uint16_t gray_get_threshold(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return GRAY_DEFAULT_THRESHOLD;
    return gray_driver.ch[ch].threshold;
}

void gray_set_all_thresholds(uint16_t threshold)
{
    if (threshold > GRAY_NORMALIZED_MAX) threshold = GRAY_NORMALIZED_MAX;
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        gray_driver.ch[i].threshold = threshold;
    }
}

void gray_set_ch_thresholds(uint8_t ch,uint16_t threshold)
{ 
   if (threshold > GRAY_NORMALIZED_MAX) threshold = GRAY_NORMALIZED_MAX;
	 gray_driver.ch[ch].threshold = threshold;
}
/*----------------------------------------------------------------------------*\
 * 数据获取API
\*----------------------------------------------------------------------------*/

void gray_update_sensor_data(void)
{
    adc_sample_with_filter();
    update_effective_values();
    apply_moving_average_filter();
}

uint16_t gray_get_normalized_value(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;
    return normalize_value(ch, filtered_values[ch]);
}

uint8_t gray_get_line_state(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;

    uint16_t norm_val = gray_get_normalized_value(ch);
    uint16_t threshold = gray_driver.ch[ch].threshold;

    // 添加迟滞，防止抖动（阈值±20）
    static uint8_t last_state[GRAY_NUM_CHANNELS] = {0};

    if (norm_val < threshold)
        last_state[ch] = 1; // 黑线
    else if (norm_val > threshold)
        last_state[ch] = 0; // 白线

    return last_state[ch];
}

uint32_t gray_get_raw_value(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;
    return effective_values[ch];
}

void gray_get_all_normalized_values(uint16_t *values)
{
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        values[i] = gray_get_normalized_value(i);
    }
}

void gray_get_all_line_states(uint8_t *states)
{
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        states[i] = gray_get_line_state(i);
    }
}

/*----------------------------------------------------------------------------*\
 * 校准数据获取API
\*----------------------------------------------------------------------------*/

uint32_t gray_get_calib_white(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;
    return gray_driver.ch[ch].white_value;
}

uint32_t gray_get_calib_black(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;
    return gray_driver.ch[ch].black_value;
}

uint8_t gray_is_channel_calibrated(uint8_t ch)
{
    if (ch >= GRAY_NUM_CHANNELS) return 0;
    return gray_driver.ch[ch].calibrated;
}

/*----------------------------------------------------------------------------*\
 * 数据上传函数
\*----------------------------------------------------------------------------*/

 

/*----------------------------------------------------------------------------*\
 * 内部函数实现
\*----------------------------------------------------------------------------*/
 
static void adc_sample_with_filter(void)
{
    uint32_t temp_buffer[ADC_MEDIAN_SAMPLE_COUNT];

    // 采集多组样本进行中值滤波
    for (int sample = 0; sample < ADC_MEDIAN_SAMPLE_COUNT; sample++)
    {
        // 关闭LED，采集环境光
        set_gray_led(BLACK);
        _delay_us(30);

        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_sample_buffer[sample], GRAY_NUM_CHANNELS);
         while (HAL_DMA_GetState(&hdma_adc1) != HAL_DMA_STATE_READY);

        // 开启LED，采集反射光
        set_gray_led((COLOR_STATE)gray_driver.calibrate_led);
        _delay_us(30);

        HAL_ADC_Start_DMA(&hadc1, (uint32_t*)adc_sample_bufferv2[sample], GRAY_NUM_CHANNELS);
         while (HAL_DMA_GetState(&hdma_adc1) != HAL_DMA_STATE_READY);
    }

    // 对每个通道进行中值滤波
    for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
    {
        // 提取环境光样本
        for (int i = 0; i < ADC_MEDIAN_SAMPLE_COUNT; i++)
        {
            temp_buffer[i] = adc_sample_buffer[i][ch];
        }
        ambient_values[ch] = median_filter(temp_buffer, ADC_MEDIAN_SAMPLE_COUNT);

        // 提取反射光样本
        for (int i = 0; i < ADC_MEDIAN_SAMPLE_COUNT; i++)
        {
            temp_buffer[i] = adc_sample_bufferv2[i][ch];
        }
        reflect_values[ch] = median_filter(temp_buffer, ADC_MEDIAN_SAMPLE_COUNT);
    }
}

// 计算有效值（反射-环境，防止负值）
static void update_effective_values(void)
{
    for (int i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        if (reflect_values[i] > ambient_values[i])
            effective_values[i] = reflect_values[i] - ambient_values[i];
        else
            effective_values[i] = 0;
    }
}

// 移动平均滤波
static void apply_moving_average_filter(void)
{
    static uint32_t history[GRAY_NUM_CHANNELS][4] = {0};
    static uint8_t history_index = 0;

    // 更新历史数据
    for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
    {
        history[ch][history_index] = effective_values[ch];
    }

    // 计算移动平均值（4点平均）
    for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
    {
        uint32_t sum = 0;
        for (int i = 0; i < 4; i++)
        {
            sum += history[ch][i];
        }
        filtered_values[ch] = sum / 4;
    }

    history_index = (history_index + 1) % 4;
}

// 中值滤波算法
static uint32_t median_filter(uint32_t *samples, uint8_t count)
{
    // 复制数组以避免修改原数据
    uint32_t sorted[ADC_MEDIAN_SAMPLE_COUNT]; // 最大支持ADC_MEDIAN_SAMPLE_COUNT个样本
    if (count > ADC_MEDIAN_SAMPLE_COUNT) count = ADC_MEDIAN_SAMPLE_COUNT;

    for (uint8_t i = 0; i < count; i++)
    {
        sorted[i] = samples[i];
    }

    // 对于小样本数（≤5），使用插入排序更高效
    for (uint8_t i = 1; i < count; i++)
    {
        uint32_t key = sorted[i];
        int8_t j = i - 1;

        // 将sorted[0..i-1]中大于key的元素后移
        while (j >= 0 && sorted[j] > key)
        {
            sorted[j + 1] = sorted[j];
            j = j - 1;
        }
        sorted[j + 1] = key;
    }

    // 返回中值
    return sorted[count / 2];
}

// 归一化函数（0~GRAY_NORMALIZED_MAX）
static uint16_t normalize_value(uint8_t ch, uint32_t value)
{
   // if (!gray_driver.ch[ch].calibrated)
   // {
        // 未校准：直接映射0~4095到0~GRAY_NORMALIZED_MAX
       // return (value * GRAY_NORMALIZED_MAX) / 4095;
  //  }

    uint32_t black = gray_driver.ch[ch].black_value;
    uint32_t white = gray_driver.ch[ch].white_value;

    // 防止除零
    if (white <= black)
        return GRAY_NORMALIZED_MAX / 2;

    // 限制在黑白范围内
    if (value <= black)
        return 0;
    if (value >= white)
        return GRAY_NORMALIZED_MAX;

    // 线性映射：value在black~white之间映射到0~GRAY_NORMALIZED_MAX
    return ((value - black) * GRAY_NORMALIZED_MAX) / (white - black);
}

/*----------------------------------------------------------------------------*\
 * 校准状态机实现
\*----------------------------------------------------------------------------*/

static void calibration_state_machine(void)
{
    uint32_t current_time = HAL_GetTick();
    static uint32_t state_start_time = 0;
    const uint32_t STATE_TIMEOUT_MS = 30000; // 30秒超时

    // 状态进入时记录时间
    static calib_state_t last_state = CALIB_STATE_IDLE;
    if (last_state != gray_driver.calib_state)
    {
        state_start_time = current_time;
        last_state = gray_driver.calib_state;
    }

    // 检查超时
    if (current_time - state_start_time > STATE_TIMEOUT_MS)
    {
        // 校准超时，重置
        gray_calibration_stop();
 
        return;
    }

    switch (gray_driver.calib_state)
    {
        case CALIB_STATE_WHITE_PREPARE:
            calibration_user_prompt(1); // 提示放置白线
            if (current_time - gray_driver.calib_timestamp > CALIBRATION_DELAY_MS)
            {
                gray_driver.calib_state = CALIB_STATE_WHITE_SAMPLE;
                gray_driver.calib_timestamp = current_time;
            }
            break;

        case CALIB_STATE_WHITE_SAMPLE:
            calibration_white_step();
            break;

        case CALIB_STATE_BLACK_PREPARE:
            calibration_user_prompt(2); // 提示切换黑线
            if (current_time - gray_driver.calib_timestamp > CALIBRATION_DELAY_MS)
            {
                gray_driver.calib_state = CALIB_STATE_BLACK_SAMPLE;
                gray_driver.calib_timestamp = current_time;
            }
            break;

        case CALIB_STATE_BLACK_SAMPLE:
            calibration_black_step();
            break;

        case CALIB_STATE_COMPLETE:
            calibration_complete_step();
            break;

        default:
            break;
    }
}

static void calibration_white_step(void)
{
    static uint8_t sample_count = 0;
    static uint32_t white_samples[GRAY_NUM_CHANNELS][CALIBRATION_SAMPLE_COUNT];
    uint32_t current_time = HAL_GetTick();

    // 采样阶段：LED闪烁提示
    if ((current_time / CALIBRATION_BLINK_MS) % 2 == 0)
        set_gray_led(RED);
    else
        set_gray_led(BLACK);
 
    if (current_time - gray_driver.calib_timestamp > 100) // 每100ms采样一次
    {
        gray_driver.calib_timestamp = current_time;

        // 采集数据
        adc_sample_with_filter();
        update_effective_values();

        // 保存白线样本
        for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
        {
            white_samples[ch][sample_count] = effective_values[ch];
        }

        sample_count++;

        // 采样完成
        if (sample_count >= CALIBRATION_SAMPLE_COUNT)
        {
            // 计算每个通道的白线中值
            for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
            {
                gray_driver.ch[ch].white_value = median_filter(white_samples[ch], CALIBRATION_SAMPLE_COUNT);
            }

            sample_count = 0;
            gray_driver.calib_state = CALIB_STATE_BLACK_PREPARE;
            gray_driver.calib_timestamp = current_time;
            send_calibration_prompt(2); // 步骤2：准备黑线
        }
    }
}

static void calibration_black_step(void)
{
    static uint8_t sample_count = 0;
    static uint32_t black_samples[GRAY_NUM_CHANNELS][CALIBRATION_SAMPLE_COUNT];
    uint32_t current_time = HAL_GetTick();

    // 采样阶段：LED闪烁提示（不同颜色）
    if ((current_time / CALIBRATION_BLINK_MS) % 2 == 0)
        set_gray_led(RED); // 用红色提示不同阶段
    else
        set_gray_led(BLACK);

    // 定期采样
    if (current_time - gray_driver.calib_timestamp > 100) // 每100ms采样一次
    {
        gray_driver.calib_timestamp = current_time;

        // 采集数据
        adc_sample_with_filter();
        update_effective_values();

        // 保存黑线样本
        for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
        {
            black_samples[ch][sample_count] = effective_values[ch];
        }

        sample_count++;

        // 采样完成
        if (sample_count >= CALIBRATION_SAMPLE_COUNT)
        {
            // 计算每个通道的黑线中值
            for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
            {
                gray_driver.ch[ch].black_value = median_filter(black_samples[ch], CALIBRATION_SAMPLE_COUNT);

                // 验证校准数据有效性
                if (gray_driver.ch[ch].white_value > gray_driver.ch[ch].black_value)
                {
                    gray_driver.ch[ch].calibrated = 1;
                }
                else
                {
                    // 如果白线值小于等于黑线值，校准无效
                    gray_driver.ch[ch].calibrated = 0;
                    gray_driver.ch[ch].white_value = 4095;
                    gray_driver.ch[ch].black_value = 0;
                }
            }

            sample_count = 0;
            gray_driver.calib_state = CALIB_STATE_COMPLETE;
            gray_driver.calib_timestamp = current_time;
        }
    }
}

static void calibration_complete_step(void)
{
    uint32_t current_time = HAL_GetTick();

    // 完成提示：LED快速闪烁
    if ((current_time / 100) % 2 == 0)
        set_gray_led(GREEN);
    else
        set_gray_led(BLACK);
    
		apply_moving_average_filter();
		
		 /*计算阈值*/
		 for (int ch = 0; ch < GRAY_NUM_CHANNELS; ch++)
		 { 
		    if(gray_driver.ch[ch].calibrated)
				{
           uint32_t black = gray_driver.ch[ch].black_value;
           uint32_t white = gray_driver.ch[ch].white_value;
           uint32_t midpoint = black + ((white - black) / 2);

           // 阈值在归一化空间使用，防止超过0~999
           gray_driver.ch[ch].threshold = normalize_value(ch, midpoint);
           if (gray_driver.ch[ch].threshold > GRAY_NORMALIZED_MAX)
           {
               gray_driver.ch[ch].threshold = GRAY_NORMALIZED_MAX;
           }
				}
		 }
		 
    if (current_time - gray_driver.calib_timestamp > 2000) // 显示2秒
    {
        // 恢复正常状态
        gray_driver.calib_state = CALIB_STATE_IDLE;
        set_gray_led((COLOR_STATE)gray_driver.calibrate_led);
        
        // 发送校准完成提示
        send_calibration_prompt(3); // 步骤3：校准完成
			  Gray_SaveConfig((const driver_gray_t*)&gray_driver);
    }
}

static void calibration_user_prompt(uint8_t step)
{
    // 通过LED提示用户
    uint32_t current_time = HAL_GetTick();

    switch (step)
    {
        case 1: // 准备白线：慢闪白色
            if ((current_time / 500) % 2 == 0)
                set_gray_led(WHITE);
            else
                set_gray_led(BLACK);
            break;

        case 2: // 准备黑线：慢闪红色
            if ((current_time / 500) % 2 == 0)
                set_gray_led(RED);
            else
                set_gray_led(BLACK);
            break;
    }
}

static void send_calibration_prompt(uint8_t step)
{ 
   machine_is_calibrate = 1;
	  
}
 
/*----------------------------------------------------------------------------*\
 * 外部接口函数（兼容现有代码）
\*----------------------------------------------------------------------------*/

void gray_set_calibration_led_color(COLOR_STATE color)
{
    gray_driver.calibrate_led = (uint8_t)color;
    set_gray_led(color);
}

// 处理校准命令（从队列）
void gray_handle_calibration_command(void)
{
    // 这个函数需要与现有的pull_data_from_queue集成
    // 暂时留空，由用户根据实际队列处理逻辑填充
}

// 兼容现有代码的函数
void pull_data_from_queue(void)
{
    // 保持现有逻辑，但调用新的校准处理
    // 这里简化实现，实际需要根据队列处理
    char data[32];
    QUEUE_DATA_TYPE *rx_data;
    rx_data = cbRead(&rx_queue);

    if(rx_data != NULL)
    {
        if((uint8_t)rx_data->head[0] == 0x5A && (uint8_t)rx_data->head[rx_data->len - 1] == 0xA5)
        {
            memset(data,0,sizeof(data));
            memcpy(data,rx_data->head+5,rx_data->head[3]);
            switch(rx_data->head[4])
            {
                case 0x09:
                {
                    if(strcmp(data,(const char*)"Please Link") == 0)
                    {
                        linke_state = 1;
                        MultiUart_SendFrame(uart_transmit,(uint8_t*)"Play Aplication",strlen("Play Aplication"),0x09);
                    }
                    break;
                }
                case CALIBRATE_MODE_START:
                {
									typedef struct __attribute__((packed)) {
											uint8_t  start_calibrate; 
									}set_cfg_pake;
									set_cfg_pake paket;
									memcpy(&paket, data, sizeof(set_cfg_pake)); 	
                  button_pressed = paket.start_calibrate;
                  break;
                }
                case SET_LED_RGB:
                {
										typedef struct __attribute__((packed)) {
												uint8_t  rgb; 
										}set_cfg_pake;
										set_cfg_pake paket;
									  memcpy(&paket, data, sizeof(set_cfg_pake)); 	
										gray_driver.calibrate_led = paket.rgb;
                    set_gray_led((COLOR_STATE)gray_driver.calibrate_led);
                    break;
                }
                case SET_THRESHOLD:
                {
											typedef struct __attribute__((packed)) {
											uint16_t threshold;
											uint8_t  ch; 
											}set_cfg_pake;
											
										set_cfg_pake paket;
 										memcpy(&paket, data, sizeof(set_cfg_pake)); 	
										gray_set_ch_thresholds(paket.ch,paket.threshold);
                    break;
                }
								case 0xEE:
								{ 
						 
									NVIC_SystemReset();
									while(1);
				
								}
            }
        }
        cbReadFinish(&rx_queue);
    }
}

// 初始化配置（兼容现有代码）
void cfg_file_init(void)
{
    gray_driver_init();
}

// 初始化阈值（兼容现有代码）
void threshold_init(void)
{
    gray_set_all_thresholds(GRAY_DEFAULT_THRESHOLD);
}

// 获取传感器读数（兼容现有代码）
void get_sensor_readings(void)
{
    gray_update_sensor_data();
}

// 上传灰度数据（兼容现有代码）
void upload_gray_data(void)
{
 
    // 旧代码逻辑：检查连接状态
    if (!linke_state)
        return;

    // 检查按钮按下，开始校准
    if (button_pressed)
    {
        close_all_led();
        gray_calibration_start();
        button_pressed = 0;
        linke_state = 0;
			  machine_is_calibrate = 0;
        return;
    }

    // 处理校准状态机
    if (gray_calibration_is_active())
    {
        gray_calibration_process();
        return;
    }

    // 正常模式：更新传感器数据并发送
    gray_update_sensor_data();

    gray_packet_t packet;
    for (uint8_t i = 0; i < GRAY_NUM_CHANNELS; i++)
    {
        packet.values[i] = gray_get_normalized_value(i);
        packet.states[i] = gray_get_line_state(i);
			  packet.threshold[i] = gray_get_threshold(i);
			   
    }
		char versionStr[16];
    reverseBytesAndStoreSimplified(FLASH_read(AplicationVERSION),(uint8_t*)versionStr);
		packet.version =  atoi(versionStr); 
		packet.is_calibrate = machine_is_calibrate;
		
    // 控制LED显示状态
    _IO_LED0(packet.states[0]);
    _IO_LED1(packet.states[1]);
    _IO_LED2(packet.states[2]);
    _IO_LED3(packet.states[3]);
    _IO_LED4(packet.states[4]);
    _IO_LED5(packet.states[5]);
    _IO_LED6(packet.states[6]);

    // 通过串口发送数据
    MultiUart_SendFrame(uart_transmit, (uint8_t*)&packet, sizeof(packet), 0xED);
}
