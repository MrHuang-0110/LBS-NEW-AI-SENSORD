/* agreement_comx.c - host wire protocol framing and frame parser */
#include "agreement_comx.h"
#include "rx_data_queue.h"
#include "senords.h"    /* USER_ObjectID / USER_SourceID */

typedef enum {
    PS_HEAD = 0,
    PS_OBJECT_ID,
    PS_SOURCE_ID,
    PS_LENGTH,
    PS_INDEX,
    PS_PAYLOAD,
    PS_CRC,
    PS_FOOTER
} parser_state_t;

static parser_state_t s_state;
static host_frame_t   s_frame;
static uint8_t        s_crc;
static uint8_t        s_received;

void frame_parser_reset(void)
{
    s_state = PS_HEAD;
    s_crc = 0U;
    s_received = 0U;
}

/* Resynchronise on the next 0x5A instead of dropping the whole buffer. */
static void parser_resync(uint8_t byte)
{
    frame_parser_reset();
    if (byte == FRAME_HEADER) {
        s_crc = byte;
        s_state = PS_OBJECT_ID;
    }
}

bool frame_parser_poll(host_frame_t *out)
{
    uint8_t byte;

    while (rx_queue_pop(&byte)) {
        switch (s_state) {
            case PS_HEAD:
                if (byte == FRAME_HEADER) {
                    s_crc = byte;
                    s_received = 0U;
                    s_state = PS_OBJECT_ID;
                }
                break;

            case PS_OBJECT_ID:
                s_frame.object_id = byte;
                s_crc = (uint8_t)(s_crc + byte);
                s_state = PS_SOURCE_ID;
                break;

            case PS_SOURCE_ID:
                s_frame.source_id = byte;
                s_crc = (uint8_t)(s_crc + byte);
                s_state = PS_LENGTH;
                break;

            case PS_LENGTH:
                if (byte > FRAME_MAX_PAYLOAD) {
                    parser_resync(byte);
                    break;
                }
                s_frame.len = byte;
                s_crc = (uint8_t)(s_crc + byte);
                s_state = PS_INDEX;
                break;

            case PS_INDEX:
                s_frame.index = byte;
                s_crc = (uint8_t)(s_crc + byte);
                s_state = (s_frame.len == 0U) ? PS_CRC : PS_PAYLOAD;
                break;

            case PS_PAYLOAD:
                s_frame.payload[s_received++] = byte;
                s_crc = (uint8_t)(s_crc + byte);
                if (s_received >= s_frame.len) {
                    s_state = PS_CRC;
                }
                break;

            case PS_CRC:
                if (byte == s_crc) {
                    s_state = PS_FOOTER;
                } else {
                    parser_resync(byte);
                }
                break;

            case PS_FOOTER:
                frame_parser_reset();
                if (byte == FRAME_FOOTER) {
                    *out = s_frame;
                    return true;
                }
                break;

            default:
                frame_parser_reset();
                break;
        }
    }

    return false;
}

void MultiUart_SendFrame(void (*transfer)(void *, uint16_t),
                         const uint8_t *data,
                         uint16_t len,
                         uint8_t index)
{
    uint8_t buf[FRAME_MAX_PAYLOAD + 7U];
    uint8_t crc = 0U;
    uint16_t total;
    uint16_t i;

    if (transfer == NULL || len > FRAME_MAX_PAYLOAD) {
        return;
    }

    buf[0] = FRAME_HEADER;
    buf[1] = USER_ObjectID;
    buf[2] = USER_SourceID;
    buf[3] = (uint8_t)len;
    buf[4] = index;
    for (i = 0U; i < len; i++) {
        buf[5U + i] = data[i];
    }

    total = (uint16_t)(len + 7U);
    for (i = 0U; i < (uint16_t)(total - 2U); i++) {
        crc = (uint8_t)(crc + buf[i]);
    }
    buf[total - 2U] = crc;
    buf[total - 1U] = FRAME_FOOTER;

    transfer((void *)buf, total);
}
