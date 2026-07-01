#ifndef __COLORDISCRIMIATION_DRIVER_H
#define __COLORDISCRIMIATION_DRIVER_H


#include "stdbool.h"
#include "systick_delay.h"
#include "string.h"
#include "stdlib.h"
#include <stdint.h>


typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;

 

/* I2C 设备地址 (7位) */
#define LTR381RGB_I2C_ADDR         0x53

/* 寄存器地址映射 (依据数据手册 Page 11-23) */
#define LTR381RGB_MAIN_CTRL         0x00   // 操作模式控制, 软件复位
#define LTR381RGB_ALS_CS_MEAS_RATE  0x04   // 测量速率和分辨率
#define LTR381RGB_ALS_CS_GAIN       0x05   // 模拟增益
#define LTR381RGB_PART_ID           0x06   // 器件ID
#define LTR381RGB_MAIN_STATUS       0x07   // 状态寄存器
#define LTR381RGB_CS_DATA_IR_0      0x0A   // IR通道数据 (低字节)
#define LTR381RGB_CS_DATA_IR_1      0x0B   // IR通道数据 (中字节)
#define LTR381RGB_CS_DATA_IR_2      0x0C   // IR通道数据 (高字节)
#define LTR381RGB_CS_DATA_GREEN_0   0x0D   // 绿色/ALS 数据 (低字节)
#define LTR381RGB_CS_DATA_GREEN_1   0x0E   // 绿色/ALS 数据 (中字节)
#define LTR381RGB_CS_DATA_GREEN_2   0x0F   // 绿色/ALS 数据 (高字节)
#define LTR381RGB_CS_DATA_BLUE_0    0x10   // 蓝色数据 (低字节)
#define LTR381RGB_CS_DATA_BLUE_1    0x11   // 蓝色数据 (中字节)
#define LTR381RGB_CS_DATA_BLUE_2    0x12   // 蓝色数据 (高字节)
#define LTR381RGB_CS_DATA_RED_0     0x13   // 红色数据 (低字节)
#define LTR381RGB_CS_DATA_RED_1     0x14   // 红色数据 (中字节)
#define LTR381RGB_CS_DATA_RED_2     0x15   // 红色数据 (高字节)
#define LTR381RGB_INT_CFG           0x19   // 中断配置
#define LTR381RGB_INT_PST           0x1A   // 中断持续设置

/* MAIN_CTRL 寄存器位定义 (Page 12) */
#define LTR381RGB_SW_RESET          (1 << 4)   // 软件复位
#define LTR381RGB_CS_MODE           (1 << 2)   // 0: ALS模式, 1: CS模式 (RGB+IR)
#define LTR381RGB_ALS_CS_ENABLE     (1 << 1)   // 1: 激活, 0: 待机

/* ALS_CS_MEAS_RATE 寄存器位定义 (Page 13) */
#define LTR381RGB_RESOLUTION_MASK   0x70       // 分辨率掩码 (bits 6:4)
#define LTR381RGB_MEAS_RATE_MASK    0x07       // 测量速率掩码 (bits 2:0)

/* 分辨率/积分时间枚举 (依据 Page 13, 24) */
typedef enum {
    LTR381RGB_20BIT_400MS = 0x00,   // 20位, 400ms
    LTR381RGB_19BIT_200MS = 0x10,   // 19位, 200ms
    LTR381RGB_18BIT_100MS = 0x20,   // 18位, 100ms (默认)
    LTR381RGB_17BIT_50MS  = 0x30,   // 17位, 50ms
    LTR381RGB_16BIT_25MS  = 0x40    // 16位, 25ms
} ltr381_resolution_t;

/* 测量速率枚举 (依据 Page 13) */
typedef enum {
    LTR381RGB_RATE_25MS   = 0x00,
    LTR381RGB_RATE_50MS   = 0x01,
    LTR381RGB_RATE_100MS  = 0x02,   // 默认
    LTR381RGB_RATE_200MS  = 0x03,
    LTR381RGB_RATE_500MS  = 0x04,
    LTR381RGB_RATE_1000MS = 0x05,
    LTR381RGB_RATE_2000MS = 0x06
} ltr381_meas_rate_t;

/* ALS/CS 增益枚举 (依据 Page 14) */
typedef enum {
    LTR381RGB_GAIN_1  = 0x00,
    LTR381RGB_GAIN_3  = 0x01,   // 默认
    LTR381RGB_GAIN_6  = 0x02,
    LTR381RGB_GAIN_9  = 0x03,
    LTR381RGB_GAIN_18 = 0x04
} ltr381_gain_t;

/* 工作模式 (依据 Page 12) */
typedef enum {
    LTR381RGB_MODE_ALS = 0,       // ALS模式 (ALS + IR + 温度补偿)
    LTR381RGB_MODE_CS  = 1        // 色彩传感器模式 (RGB + IR + 补偿)
} ltr381_mode_t;

/* 配置结构体 */
typedef struct {
    ltr381_mode_t mode;              // 工作模式: ALS 或 CS
    uint8_t enable;                  // 1: 激活, 0: 待机
    ltr381_resolution_t resolution;  // 分辨率/积分时间
    ltr381_meas_rate_t meas_rate;    // 测量速率
    ltr381_gain_t gain;              // 模拟增益
} ltr381_config_t;

/* RGB 原始数据 (20位有效) */
typedef struct {
    uint32_t red;     // 红色通道数据 (0 ~ 2^20-1)
    uint32_t green;   // 绿色通道数据
    uint32_t blue;    // 蓝色通道数据
    uint32_t ir;      // 红外通道数据
} ltr381_rgb_raw_t;

/* ==================== 用户需实现的底层 I2C 函数 ==================== */
int8_t ltr381_i2c_write(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);
int8_t ltr381_i2c_read(uint8_t dev_addr, uint8_t reg_addr, uint8_t *data, uint16_t len);

/* ==================== API 函数 ==================== */

/**
 * @brief 初始化传感器 (检查器件ID)
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_init(void);

/**
 * @brief 软件复位
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_soft_reset(void);

/**
 * @brief 配置传感器 (模式、使能、分辨率、速率、增益)
 * @param cfg 配置结构体指针
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_configure(const ltr381_config_t *cfg);

/**
 * @brief 获取当前配置
 * @param cfg 存储配置的结构体指针
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_get_config(ltr381_config_t *cfg);

/**
 * @brief 读取原始RGB和IR数据 (20位)
 * @param rgb 存储各通道数据的结构体指针
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_read_raw(ltr381_rgb_raw_t *rgb);

/**
 * @brief 计算环境光照度 (Lux) - 依据数据手册公式
 * @param lux 计算出的照度值
 * @return 0:成功, -1:失败
 * @note 需要先配置并读取原始数据，该函数会内部调用读取绿色和IR通道
 */
int8_t ltr381_rgb_read_lux(float *lux);

/**
 * @brief 检查数据是否就绪 (新数据标志)
 * @param new_data 1: 新数据可用, 0: 无新数据
 * @return 0:成功, -1:失败
 */
int8_t ltr381_rgb_data_ready(uint8_t *new_data);

 
int8_t ltr381_cfg_init(void);
 
#endif
