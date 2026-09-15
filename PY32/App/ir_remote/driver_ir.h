/* driver_ir.h - application logic for the emitter / receiver roles */
#ifndef IR_DRIVER_IR_H
#define IR_DRIVER_IR_H

#include "main.h"

/* Latch the role from PB2, bring up GPIO / time base / IR codec / ADC and,
 * in the emitter role only, the host UART. */
void ir_app_init(void);

/* Main loop body. Never returns early; the whole application is polling. */
void ir_app_poll(void);

/* Last colour that is in effect locally (commanded value in the emitter role,
 * decoded value in the receiver role). */
uint8_t ir_app_color(void);

/* Battery percent last known by the emitter (BAT_UNKNOWN until a reverse frame
 * has been received). */
uint8_t ir_app_battery(void);

#endif /* IR_DRIVER_IR_H */
