/* rx_data_queue.h - byte FIFO between the host UART ISR and the main loop
 *
 * Per-platform copy (this repo keeps one per platform, see AGENTS.md).
 * The ISR only pushes; the main loop only pops and parses.
 */
#ifndef IR_RX_DATA_QUEUE_H
#define IR_RX_DATA_QUEUE_H

#include "main.h"

/* power of two */
#define RX_FIFO_SIZE    64U
#define RX_FIFO_MASK    (RX_FIFO_SIZE - 1U)

void rx_queue_init(void);

/* ISR context */
bool rx_queue_push(uint8_t byte);

/* main loop context */
bool rx_queue_pop(uint8_t *byte);

/* number of buffered bytes */
uint16_t rx_queue_level(void);

#endif /* IR_RX_DATA_QUEUE_H */
