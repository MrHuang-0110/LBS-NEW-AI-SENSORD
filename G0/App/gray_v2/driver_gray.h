#ifndef __DRIVER_GRAY_H
#define __DRIVER_GRAY_H
#include "driver_sys.h"
#include "adc.h"

#define CALIBRATE_MODE_START 0xD0
#define SET_LED_RGB          0xD1
#define SET_THRESHOLD        0xD2

#define GRAY_NUM_CHANNELS    7
#define GRAY_NORMALIZED_MAX  999  // 0~999范围
#define GRAY_DEFAULT_THRESHOLD GRAY_NORMALIZED_MAX/2

// 校准步骤枚举
typedef enum {
    CALIB_STATE_IDLE = 0,
    CALIB_STATE_WHITE_PREPARE,    // 准备白线校准
    CALIB_STATE_WHITE_SAMPLE,     // 采集白线值
    CALIB_STATE_BLACK_PREPARE,    // 准备黑线校准
    CALIB_STATE_BLACK_SAMPLE,     // 采集黑线值
    CALIB_STATE_COMPLETE          // 校准完成
} calib_state_t;

// LED颜色枚举
typedef enum {
    BLACK,
    RED,
    GREEN,
    BLUE,
    YELLO,
    PINK_RED,
    CYAN,
    WHITE,
    MAX_COLOR_NUM
} COLOR_STATE;

// 单个通道校准数据
typedef struct {
    uint32_t white_value;     // 白线基准值
    uint32_t black_value;     // 黑线基准值
    uint32_t threshold;       // 动态阈值（可手动调整）
    uint8_t calibrated;       // 校准标志
	   
} gray_calib_ch_t;

// 灰度传感器全局结构
typedef struct {
	  uint32_t magic;
    gray_calib_ch_t ch[GRAY_NUM_CHANNELS];
    uint8_t calibrate_led;    // 校准用LED颜色
    calib_state_t calib_state;// 校准状态机
    uint8_t calib_step;       // 校准步骤提示
    uint32_t calib_timestamp; // 校准时间戳
} driver_gray_t;

// 数据包结构（用于上传）
typedef struct __attribute__((packed)) {
	  uint32_t version;
	  uint32_t threshold[GRAY_NUM_CHANNELS];
    uint16_t values[GRAY_NUM_CHANNELS];  // 归一化值 0~999
    uint8_t  states[GRAY_NUM_CHANNELS];  // 黑白状态 0=白/1=黑
	  uint8_t  is_calibrate;
} gray_packet_t;

#define _IO_RED(x)  	x>0?HAL_GPIO_WritePin(GPIOB, LED_R_Pin, GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB, LED_R_Pin, GPIO_PIN_RESET)
#define _IO_GREEN(x)  x>0?HAL_GPIO_WritePin(GPIOB, LED_G_Pin, GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB, LED_G_Pin, GPIO_PIN_RESET)
#define _IO_BLUE(x)  	x>0?HAL_GPIO_WritePin(GPIOB, LED_B_Pin, GPIO_PIN_SET):HAL_GPIO_WritePin(GPIOB, LED_B_Pin, GPIO_PIN_RESET)

#define _IO_LED0(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED0_GPIO_Port, LED0_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED0_GPIO_Port, LED0_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED1(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED1_GPIO_Port, LED1_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED1_GPIO_Port, LED1_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED2(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED2_GPIO_Port, LED2_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED2_GPIO_Port, LED2_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED3(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED3_GPIO_Port, LED3_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED3_GPIO_Port, LED3_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED4(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED4_GPIO_Port, LED4_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED4_GPIO_Port, LED4_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED5(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED5_GPIO_Port, LED5_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED5_GPIO_Port, LED5_Pin, GPIO_PIN_SET); \
    } \
} while(0)

#define _IO_LED6(x)  do { \
    if ((x) == 2) { \
        HAL_GPIO_TogglePin(LED6_GPIO_Port, LED6_Pin); \
    } else if ((x) > 0) { \
        HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, GPIO_PIN_RESET); \
    } else { \
        HAL_GPIO_WritePin(LED6_GPIO_Port, LED6_Pin, GPIO_PIN_SET); \
    } \
} while(0)

// LED控制函数
void close_all_led(void);
void open_all_led(void);
void set_gray_led(COLOR_STATE color);

// 数据队列处理
void pull_data_from_queue(void);

// 初始化函数
void gray_driver_init(void);
void gray_calibration_init(void);
void threshold_init(void);                     // 初始化阈值（兼容旧代码）
void cfg_file_init(void);                      // 初始化配置文件（兼容旧代码）
void get_sensor_readings(void);                // 获取传感器读数（兼容旧代码）

// 校准控制API
void gray_calibration_start(void);                      // 开始校准流程
void gray_calibration_stop(void);                       // 停止校准
void gray_calibration_reset(void);                      // 重置校准数据
uint8_t gray_calibration_is_active(void);               // 校准是否进行中
calib_state_t gray_calibration_get_state(void);         // 获取校准状态
void gray_calibration_process(void);                    // 校准状态机处理（需周期性调用）

// 阈值设置API
void gray_set_threshold(uint8_t ch, uint16_t threshold); // 设置通道阈值(0~GRAY_NORMALIZED_MAX)
uint16_t gray_get_threshold(uint8_t ch);                 // 获取通道阈值
void gray_set_all_thresholds(uint16_t threshold);        // 设置所有通道阈值
void gray_set_ch_thresholds(uint8_t ch,uint16_t threshold);
// 数据获取API
void gray_update_sensor_data(void);                     // 更新传感器数据（需周期性调用）
uint16_t gray_get_normalized_value(uint8_t ch);         // 获取归一化值(0~GRAY_NORMALIZED_MAX)
uint8_t gray_get_line_state(uint8_t ch);                // 获取黑白状态(0=白/1=黑)
uint32_t gray_get_raw_value(uint8_t ch);                // 获取原始ADC值（环境光补偿后）
void gray_get_all_normalized_values(uint16_t *values);  // 获取所有通道归一化值
void gray_get_all_line_states(uint8_t *states);         // 获取所有通道黑白状态

// 校准数据获取API
uint32_t gray_get_calib_white(uint8_t ch);              // 获取白线校准值
uint32_t gray_get_calib_black(uint8_t ch);              // 获取黑线校准值
uint8_t gray_is_channel_calibrated(uint8_t ch);         // 通道是否已校准

// 数据上传函数
void gray_upload_data(void);                            // 上传数据包（通过串口）
void upload_gray_data(void);                            // 上传灰度数据（兼容旧代码）

// 内部函数（供其他模块调用）
void gray_set_calibration_led_color(COLOR_STATE color); // 设置校准LED颜色
void gray_handle_calibration_command(void);             // 处理校准命令（从队列）

uint8_t Gray_SaveConfig(const driver_gray_t* cfg);
uint8_t Gray_LoadConfig(driver_gray_t* cfg);
#endif
