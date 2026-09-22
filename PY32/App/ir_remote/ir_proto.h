/* ir_proto.h - IR link codec (NEC style envelope)
 *
 * The transmitter is a BARE IR LED and the receiver is a 38 kHz demodulator
 * module (the black 3-pin part), so the MCU generates the carrier itself:
 * every mark on PA0 (LOW = mark) is emitted as a 38 kHz burst, and PA1 reads
 * the demodulated envelope back (LOW = carrier present).
 *
 * Frame: 9 ms mark + 4.5 ms space
 *        32 bits, each = 560 us mark + (560 us = 0 / 1690 us = 1) space
 *        560 us stop mark, then idle
 *        bit order: address(8) ~address(8) command(8) ~command(8), LSB first
 *
 * The address byte selects the direction:
 *   IR_ADDR_FORWARD (0x5A): emitter -> receiver, command = colour 0..3
 *   IR_ADDR_REVERSE (0xA5): receiver -> emitter, command = battery percent 0..100
 */
#ifndef IR_PROTO_H
#define IR_PROTO_H

#include "main.h"

#define IR_ADDR_FORWARD     0x5A
#define IR_ADDR_REVERSE     0xA5

/* Blocking send, one frame takes ~68 ms. */
void ir_send_frame(uint8_t addr, uint8_t cmd);

/* Non-blocking check for a complete, validated frame.
 * Returns false immediately while the line is idle, so it is safe to call
 * from the main loop on every pass. */
bool ir_poll_decode(uint8_t *addr, uint8_t *cmd);

/* Liveness probe for a marginal link: true if the last decode attempt already
 * validated the address / ~address pair, even when the frame was dropped
 * afterwards because a command bit was damaged. *addr receives that address.
 * Reading the flag clears it, so it reports one frame at a time. */
bool ir_addr_seen(uint8_t *addr);

/* Liveness probe for a link whose waveform cannot be decoded at all: true if
 * the demodulator output went low (38 kHz carrier present) since the last
 * read, even when no frame survived. An overloaded receiver at very short
 * range distorts the marks but still pulls its output low, so this is what
 * keeps a colour alive while the emitter is physically held at the receiver.
 * Reading the flag clears it. */
bool ir_carrier_seen(void);

/* Reset the decoder state machine. */
void ir_proto_init(void);

#endif /* IR_PROTO_H */
