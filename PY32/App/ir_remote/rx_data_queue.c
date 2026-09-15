/* rx_data_queue.c - byte FIFO between the host UART ISR and the main loop */
#include "rx_data_queue.h"

static volatile uint8_t  s_fifo[RX_FIFO_SIZE];
static volatile uint16_t s_head;   /* written by the ISR   */
static volatile uint16_t s_tail;   /* written by main loop */

void rx_queue_init(void)
{
    s_head = 0U;
    s_tail = 0U;
}

bool rx_queue_push(uint8_t byte)
{
    uint16_t next = (uint16_t)((s_head + 1U) & RX_FIFO_MASK);

    if (next == s_tail) {
        return false; /* full: drop the byte, framing will resync */
    }
    s_fifo[s_head] = byte;
    s_head = next;
    return true;
}

bool rx_queue_pop(uint8_t *byte)
{
    if (s_tail == s_head) {
        return false;
    }
    *byte = s_fifo[s_tail];
    s_tail = (uint16_t)((s_tail + 1U) & RX_FIFO_MASK);
    return true;
}

uint16_t rx_queue_level(void)
{
    return (uint16_t)((s_head - s_tail) & RX_FIFO_MASK);
}
