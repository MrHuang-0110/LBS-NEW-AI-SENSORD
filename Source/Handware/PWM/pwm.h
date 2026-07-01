#ifndef __PWM_H
#define __PWM_H
#include "hk32f030m.h" 

void pwm_init(void);
void encorder_init(void);
void encoder_update_speed(uint32_t current_time);
void encoder_reset_position(uint8_t reset_type);
float encoder_get_speed(void);
int32_t encoder_get_total_position(void);
int16_t get_encoder_increment(void);
uint32_t getEnctordPrr(void);
#endif

