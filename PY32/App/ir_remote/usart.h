/* usart.h - host link (USART1, emitter role only) */
#ifndef IR_USART_H
#define IR_USART_H

#include "main.h"

extern UART_HandleTypeDef huart1;

/* Configure USART1 (PA3 TX / PA4 RX, 115200 8N1) and enable the RX interrupt.
 * Only called in the emitter role: the receiver has no host connection. */
void host_uart_init(void);

/* Non-blocking frame writer callback used by MultiUart_SendFrame. */
void host_uart_send_more(void *data, uint16_t length);

/* TX interrupt service hook; USART1_IRQHandler also handles RX bytes. */
void host_uart_irq_handler(void);

#endif /* IR_USART_H */
