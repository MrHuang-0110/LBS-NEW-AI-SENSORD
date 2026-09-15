/* gpio.h - board GPIO helpers for IR remote */
#ifndef IR_GPIO_H
#define IR_GPIO_H

#include "main.h"

void gpio_board_init(void);

/* color: IR_COLOR_OFF / RED / GREEN / BLUE (active HIGH pins) */
void gpio_rgb_set(uint8_t color);

/* status LED is active LOW */
void gpio_status_led(bool on);

/* IR emitter envelope, active LOW: true = emitting */
void gpio_ir_tx(bool emitting);

/* IR receiver demodulated output, true when a carrier mark is present */
bool gpio_ir_rx_low(void);

/* PB2 latched value: true = emitter board, false = receiver board */
bool gpio_role_is_emitter(void);

/* sample PB2 once (call after gpio_board_init) */
void gpio_latch_role(void);

#endif /* IR_GPIO_H */
