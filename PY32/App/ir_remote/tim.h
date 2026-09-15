/* tim.h - 1 us free running time base used by the IR envelope codec */
#ifndef IR_TIM_H
#define IR_TIM_H

#include "main.h"

/* TIM14 free running at 1 MHz (1 tick = 1 us), no interrupt. */
void tim_us_init(void);

/* current 1 us counter value (wraps every 65536 us) */
uint16_t tim_us_now(void);

/* busy wait, only for IR envelope timings (always < 65536 us) */
void tim_delay_us(uint32_t us);

/* elapsed microseconds between two tim_us_now() samples */
uint16_t tim_us_elapsed(uint16_t start);

#endif /* IR_TIM_H */
