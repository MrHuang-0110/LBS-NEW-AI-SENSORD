/* ir_proto.c - NEC style IR envelope codec (see ir_proto.h) */
#include "ir_proto.h"

#define IR_HEADER_MARK_US       9000U
#define IR_HEADER_SPACE_US      4500U
#define IR_BIT_MARK_US          560U
#define IR_BIT0_SPACE_US        560U
#define IR_BIT1_SPACE_US        1690U
#define IR_STOP_MARK_US         560U
#define IR_TOLERANCE_PCT        25U

/* extra time allowed on top of the nominal interval before giving up */
#define IR_TIMEOUT_SLACK_US     2500U

/* A bit-1 space and a bit-0 space are separated here. */
#define IR_SPACE_ONE_THRESHOLD_US  ((IR_BIT0_SPACE_US + IR_BIT1_SPACE_US) / 2U)

/* The transmitter is a bare IR LED and the receiver is a 38 kHz demodulator
 * module (the black 3-pin part), so the carrier has to be generated here:
 * every mark is emitted as a 38 kHz burst.
 * 13 us on + 13 us off = 26 us period -> ~38.5 kHz, right on the module centre.
 * Direct register writes keep the duty cycle tight. */
#define IR_CARRIER_HALF_US      13U

#define IR_LED_ON()             (IR_TX_GPIO_PORT->BRR  = (uint32_t)IR_TX_GPIO_PIN)
#define IR_LED_OFF()            (IR_TX_GPIO_PORT->BSRR = (uint32_t)IR_TX_GPIO_PIN)

/* Implemented by driver_ir.c. Called during the blocking IR frame so the
 * emitter keeps the host upload heartbeat alive while the IR burst is sent. */
void ir_background_poll(void);

/* One mark = a burst of 38 kHz carrier for the requested duration. */
static void ir_mark(uint32_t us)
{
    uint32_t elapsed = 0U;
    uint32_t service_elapsed = 0U;

    while (elapsed < us) {
        IR_LED_ON();
        tim_delay_us(IR_CARRIER_HALF_US);
        IR_LED_OFF();
        tim_delay_us(IR_CARRIER_HALF_US);
        elapsed += (2U * IR_CARRIER_HALF_US);
        service_elapsed += (2U * IR_CARRIER_HALF_US);

        if (service_elapsed >= 1000U) {
            ir_background_poll();
            service_elapsed = 0U;
        }
    }

    ir_background_poll();
}

/* A space is simply carrier-off time. */
static void ir_space(uint32_t us)
{
    while (us > 1000U) {
        tim_delay_us(1000U);
        ir_background_poll();
        us -= 1000U;
    }

    tim_delay_us(us);
    ir_background_poll();
}

void ir_proto_init(void)
{
    gpio_ir_tx(false); /* idle high: not emitting */
}

void ir_send_frame(uint8_t addr, uint8_t cmd)
{
    uint8_t bytes[4];
    uint8_t i;
    uint8_t b;

    bytes[0] = addr;
    bytes[1] = (uint8_t)(~addr);
    bytes[2] = cmd;
    bytes[3] = (uint8_t)(~cmd);

    gpio_ir_tx(false);
    ir_mark(IR_HEADER_MARK_US);
    ir_space(IR_HEADER_SPACE_US);

    for (i = 0U; i < 4U; i++) {
        for (b = 0U; b < 8U; b++) {
            ir_mark(IR_BIT_MARK_US);
            ir_space(((bytes[i] >> b) & 0x01U) != 0U ? IR_BIT1_SPACE_US : IR_BIT0_SPACE_US);
        }
    }

    ir_mark(IR_STOP_MARK_US); /* trailing mark; receiver ignores it */
    gpio_ir_tx(false);
}

/* Wait for the line to become low / high, returning the elapsed microseconds. */
static bool ir_wait_low(uint32_t timeout_us, uint16_t *elapsed)
{
    uint16_t start = tim_us_now();

    while (tim_us_elapsed(start) < (uint16_t)timeout_us) {
        if (gpio_ir_rx_low()) {
            *elapsed = tim_us_elapsed(start);
            return true;
        }
    }
    return false;
}

static bool ir_wait_high(uint32_t timeout_us, uint16_t *elapsed)
{
    uint16_t start = tim_us_now();

    while (tim_us_elapsed(start) < (uint16_t)timeout_us) {
        if (!gpio_ir_rx_low()) {
            *elapsed = tim_us_elapsed(start);
            return true;
        }
    }
    return false;
}

static bool ir_in_range(uint16_t value, uint32_t nominal)
{
    uint32_t lo = (nominal * (100U - IR_TOLERANCE_PCT)) / 100U;
    uint32_t hi = (nominal * (100U + IR_TOLERANCE_PCT)) / 100U;

    return (value >= lo) && (value <= hi);
}

bool ir_poll_decode(uint8_t *addr, uint8_t *cmd)
{
    uint16_t mark_us;
    uint16_t space_us;
    uint8_t bytes[4];
    uint8_t i;
    uint8_t b;

    /* Fast path: line idle high, nothing to decode. */
    if (!gpio_ir_rx_low()) {
        return false;
    }

    /* We are inside some mark; only a header mark starts a frame. */
    if (!ir_wait_high(IR_HEADER_MARK_US + IR_TIMEOUT_SLACK_US, &mark_us)) {
        return false;
    }
    if (!ir_in_range(mark_us, IR_HEADER_MARK_US)) {
        return false;
    }

    if (!ir_wait_low(IR_HEADER_SPACE_US + IR_TIMEOUT_SLACK_US, &space_us)) {
        return false;
    }
    if (!ir_in_range(space_us, IR_HEADER_SPACE_US)) {
        return false;
    }

    /* ir_wait_low() left us at the start of the first data bit mark. */
    bytes[0] = 0U;
    bytes[1] = 0U;
    bytes[2] = 0U;
    bytes[3] = 0U;

    for (i = 0U; i < 4U; i++) {
        for (b = 0U; b < 8U; b++) {
            if (!ir_wait_high(IR_BIT_MARK_US + IR_TIMEOUT_SLACK_US, &mark_us)) {
                return false;
            }
            if (!ir_in_range(mark_us, IR_BIT_MARK_US)) {
                return false;
            }

            /* Measure the space; for the last bit this is the stop mark. */
            if (!ir_wait_low(IR_BIT1_SPACE_US + IR_TIMEOUT_SLACK_US, &space_us)) {
                return false;
            }
            if (space_us > IR_SPACE_ONE_THRESHOLD_US) {
                bytes[i] |= (uint8_t)(1U << b);
            }
        }
    }

    /* ~address and ~command must be the bitwise complement of their pair. */
    if (bytes[0] != (uint8_t)(~bytes[1])) {
        return false;
    }
    if (bytes[2] != (uint8_t)(~bytes[3])) {
        return false;
    }

    *addr = bytes[0];
    *cmd  = bytes[2];
    return true;
}
