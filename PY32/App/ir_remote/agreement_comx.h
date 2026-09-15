/* agreement_comx.h - host wire protocol (framing + frame parser)
 *
 * Frame: [0x5A][ObjectID][SourceID][len][typeIndex][payload...][sum&0xFF][0xA5]
 * Same framing as every other product in this repo; ObjectID/SourceID come
 * from Project/senords.h for the active product macro.
 */
#ifndef IR_AGREEMENT_COMX_H
#define IR_AGREEMENT_COMX_H

#include "main.h"

#define FRAME_HEADER        0x5A
#define FRAME_FOOTER        0xA5
#define FRAME_MAX_PAYLOAD   32U

/* host command indices handled by this product */
#define CMD_PLEASE_LINK     0x09
#define CMD_SET_COLOR       0xD1
#define CMD_REBOOT          0xEE

/* outbound upload index (same as the other sensors) */
#define IDX_UPLOAD          0xED

typedef struct {
    uint8_t object_id;
    uint8_t source_id;
    uint8_t index;
    uint8_t len;
    uint8_t payload[FRAME_MAX_PAYLOAD];
} host_frame_t;

/* Drop any partially received frame (call once at start-up). */
void frame_parser_reset(void);

/* Pull bytes from rx_data_queue and return true when a whole valid frame
 * (header, length and checksum checked) has been assembled. */
bool frame_parser_poll(host_frame_t *out);

/* Build and send one frame through the given writer callback. */
void MultiUart_SendFrame(void (*transfer)(void *, uint16_t),
                         const uint8_t *data,
                         uint16_t len,
                         uint8_t index);

#endif /* IR_AGREEMENT_COMX_H */
