/* driver_ir.c - emitter / receiver application logic
 *
 * Emitter (PB2 high):
 *   - host UART link, 0x09 handshake before any upload
 *   - 0xD1 <state> sets the colour, mirrors it on the local RGB LED and sends
 *     an IR burst to the receiver
 *   - uploads ir_packet_t {state, bat} every 10 ms (index 0xED) while linked
 *   - refresh burst (3 frames) every second (receiver re-sync + fresh battery
 *     reading)
 *   - after each burst a short window listens for the battery reply
 *
 * Receiver (PB2 low):
 *   - no host UART, no uploads
 *   - decodes the colour frame, drives the RGB LED
 *   - holds the last valid red/green/blue frame for IR_HOLD_MS: a repeated
 *     colour, a colour change and the emitter's 1 s refresh burst all restart
 *     the timer, and the RGB goes off once it expires; command 0 (off) turns
 *     it off immediately and is never held
 *   - a frame carrying our forward address whose command byte was damaged
 *     still counts as "the emitter is sending" and restarts the timer, so a
 *     marginal link cannot drop a colour on one corrupted refresh frame
 *   - any 38 kHz carrier seen on PA1 also restarts the timer: at very short
 *     range an overloaded demodulator distorts the frame beyond decoding, but
 *     the carrier still proves the emitter is there
 *   - samples the battery every second, blinks PB0 below BAT_LOW_PERCENT
 *   - answers every accepted frame with a reverse frame carrying the percent
 */
#include "driver_ir.h"
#include "adc.h"
#include "agreement_comx.h"
#include "gpio.h"
#include "ir_proto.h"
#include "rx_data_queue.h"
#include "senords.h"    /* ir_packet_t, USER_ObjectID/SourceID */
#include "tim.h"
#include "usart.h"

/* upload cadence, same order of magnitude as the other sensors */
#define UPLOAD_INTERVAL_MS          10U

/* IR schedule (3 frames on a change and on the 1 s refresh; a single frame
 * after a second of idle is the least reliable case for the receiver's AGC
 * demodulator, so the refresh reuses the burst) */
#define IR_BURST_FRAMES             3U
#define IR_FRAME_GAP_MS             40U
#define IR_REFRESH_MS               1000U
#define IR_RX_WINDOW_MS             350U

/* receiver answer policy: wait until the emitter burst is over */
#define IR_REPLY_SILENCE_MS         200U
#define IR_REPLY_MIN_INTERVAL_MS    500U

/* emitter marks the battery unknown again when no reply arrives */
#define IR_BAT_STALE_MS             3000U

/* receiver: how long the last valid colour frame keeps the RGB lit. The
 * emitter re-sends every second while a colour is active, so a live link
 * holds the colour indefinitely; a stopped/silent emitter fades after 5 s. */
#define IR_HOLD_MS                  5000U

#define BAT_SAMPLE_INTERVAL_MS      1000U

#define STATUS_SLOW_BLINK_MS        500U
#define BAT_LOW_BLINK_MS            250U

/* IR_DIAG_IR_LED: per-frame flash (emitter) and carrier hold (receiver) */
#define IR_DIAG_FLASH_MS            40U
#define IR_DIAG_CARRIER_HOLD_MS     300U

static ir_role_t s_role;
static uint8_t   s_color;
static bool      s_linked;

/* emitter */
static uint8_t   s_burst_left;
static uint32_t  s_next_frame_ms;
static uint32_t  s_last_refresh_ms;
static uint32_t  s_last_upload_ms;
static uint8_t   s_battery = BAT_UNKNOWN;   /* reverse link only */
static uint32_t  s_last_tx_ms;              /* last IR frame sent */
#if IR_REVERSE_LINK
static uint32_t  s_listen_until_ms;
static uint32_t  s_battery_rx_ms;
#endif

/* receiver */
static uint8_t   s_bat_percent = BAT_UNKNOWN;
static bool      s_bat_sampled;
static uint32_t  s_last_bat_ms;
static uint32_t  s_hold_ms;     /* last accepted non-zero colour frame */
#if IR_DIAG_IR_LED
static uint32_t  s_diag_carrier_ms;
#endif
#if IR_REVERSE_LINK
static bool      s_reply_pending;
static uint32_t  s_last_frame_ms;
static uint32_t  s_last_reply_ms;
#endif

ir_role_t ir_app_role(void)
{
    return s_role;
}

uint8_t ir_app_color(void)
{
    return s_color;
}

uint8_t ir_app_battery(void)
{
    return s_battery;
}

void uploading_data(void);

void ir_background_poll(void)
{
    uint32_t now;

    if (s_role != IR_ROLE_EMITTER || !s_linked) {
        return;
    }

    now = HAL_GetTick();
    if ((now - s_last_upload_ms) >= UPLOAD_INTERVAL_MS) {
        s_last_upload_ms = now;
        uploading_data();
    }
}

/* ------------------------------------------------------------------ */
/* shared helpers                                                      */
/* ------------------------------------------------------------------ */

static void ir_apply_color(uint8_t color)
{
    s_color = color;
    gpio_rgb_set(color);
}

static void ir_start_burst(uint8_t frames)
{
    s_burst_left = frames;
    s_next_frame_ms = HAL_GetTick();
#if IR_REVERSE_LINK
    s_listen_until_ms = 0U;
#endif
}

/* ------------------------------------------------------------------ */
/* emitter                                                             */
/* ------------------------------------------------------------------ */

static void emitter_handle_frame(const host_frame_t *frame)
{
    switch (frame->index) {
        case CMD_PLEASE_LINK:
            if (frame->len == (uint8_t)strlen("Please Link") &&
                memcmp(frame->payload, "Please Link", frame->len) == 0) {
                /* The reply text below is deliberately kept in its legacy,
                 * misspelled form: it is the exact string the whole wire
                 * protocol family sends (Project/senords.c, gray_v2) and the
                 * host matches it literally, so it must NOT be "corrected". */
                // pi-lens-ignore: typos
                static const char link_reply[] = "Play Aplication";

                MultiUart_SendFrame(host_uart_send_more,
                                    (const uint8_t *)link_reply,
                                    (uint16_t)strlen(link_reply),
                                    CMD_PLEASE_LINK);
                s_linked = true;
            }
            break;

        case CMD_SET_COLOR:
            /* out of range values are ignored on purpose */
            if (frame->len >= 1U && frame->payload[0] < (uint8_t)IR_COLOR_NUM) {
                /* Hosts commonly re-send their state at up to 10 Hz. Starting
                 * a fresh burst for every repeat would transmit IR back to
                 * back with no carrier-off time at all, which overloads the
                 * receiver's AGC and makes it deaf. Only a real change
                 * re-bursts; the 1 s refresh keeps an unchanged colour alive. */
                if (frame->payload[0] != s_color) {
                    ir_apply_color(frame->payload[0]);
                    ir_start_burst(IR_BURST_FRAMES);
                }
            }
            break;

        case CMD_REBOOT:
            NVIC_SystemReset();
            while (1) {
            }

        default:
            break;
    }
}

static void emitter_ir_service(uint32_t now)
{
    if (s_burst_left > 0U) {
        if ((int32_t)(now - s_next_frame_ms) >= 0) {
            ir_send_frame(IR_ADDR_FORWARD, s_color);
            s_last_tx_ms = HAL_GetTick();
            s_burst_left--;
            if (s_burst_left > 0U) {
                /* measured AFTER the blocking send: using the pre-send tick
                 * would put this in the past and send frames back to back */
                s_next_frame_ms = s_last_tx_ms + IR_FRAME_GAP_MS;
            }
#if IR_REVERSE_LINK
            else {
                s_listen_until_ms = now + IR_RX_WINDOW_MS;
            }
#endif
        }
        return;
    }

#if IR_REVERSE_LINK
    if (s_listen_until_ms != 0U) {
        if ((int32_t)(now - s_listen_until_ms) < 0) {
            uint8_t addr;
            uint8_t cmd;

            if (ir_poll_decode(&addr, &cmd) &&
                addr == IR_ADDR_REVERSE && cmd <= 100U) {
                s_battery = cmd;
                s_battery_rx_ms = now;
                s_listen_until_ms = 0U;
            }
            return;
        }
        s_listen_until_ms = 0U;
    }
#endif

    /* periodic refresh: re-syncs a receiver that missed a frame or reset,
     * and pulls a fresh battery reading. Sent as the same 3-frame burst as a
     * colour change: the receiver's demodulator needs the repeated frame to
     * recover from long idle, and one dropped frame must not end its hold. */
    if ((now - s_last_refresh_ms) >= IR_REFRESH_MS) {
        s_last_refresh_ms = now;
        ir_start_burst(IR_BURST_FRAMES);
    }
}

#if IR_LOOP_TEST
/* Boot signature: sweeps the RGB once at start-up in BOTH roles. It proves the
 * test build is actually running and that the RGB hardware works, independently
 * of the PB2 role strap and of the IR link. */
static void loop_test_boot_signature(void)
{
    static const uint8_t seq[3] = { IR_COLOR_RED, IR_COLOR_GREEN, IR_COLOR_BLUE };
    uint8_t i;

    for (i = 0U; i < 3U; i++) {
        gpio_rgb_set(seq[i]);
        HAL_Delay(250U);
    }
    gpio_rgb_set(IR_COLOR_OFF);
    HAL_Delay(150U);
}

/* Boot indication of the RAW PB2 level (before/independent of IR_TEST_ROLE):
 * 1 long pulse = PB2 HIGH (emitter strap), 3 quick pulses = PB2 LOW (receiver
 * strap). Makes a missing / wrong mode strap visible without a multimeter. */
static void loop_test_pb2_indication(void)
{
    uint8_t pulses = gpio_role_is_emitter() ? 1U : 3U;
    uint8_t i;

    for (i = 0U; i < pulses; i++) {
        gpio_status_led(true);
        HAL_Delay(200U);
        gpio_status_led(false);
        HAL_Delay(200U);
    }
    HAL_Delay(400U);
}

/* Bench test: drive a colour sequence locally so the IR link can be checked
 * without a host. The receiver is untouched and only follows IR frames. */
#define IR_LOOP_INTERVAL_MS     2000U
#define IR_TX_FLASH_MS          50U

static void loop_test_service(uint32_t now)
{
    static uint32_t s_next_ms;
    static uint8_t  s_step = 0xFFU;  /* forces the first colour immediately */

    if (s_step == 0xFFU) {
        s_step = 0U;
        ir_apply_color(IR_COLOR_RED);
        ir_start_burst(IR_BURST_FRAMES);
        s_next_ms = now + IR_LOOP_INTERVAL_MS;
        return;
    }

    if ((int32_t)(now - s_next_ms) >= 0) {
        s_step = (uint8_t)((s_step + 1U) % 3U);
        ir_apply_color((uint8_t)(IR_COLOR_RED + s_step));
        ir_start_burst(IR_BURST_FRAMES);
        s_next_ms = now + IR_LOOP_INTERVAL_MS;
    }
}
#endif /* IR_LOOP_TEST */

#if IR_LOOP_TEST
/* Bench test LED on the emitter: a short flash after every IR frame that is
 * actually transmitted. IR_TX_FLASH_MS stays below the 108 ms spacing of the
 * 3-frame burst, so the individual frames can be counted. This board has no IR receiver head (so there is no
 * reply to indicate) and no RGB LEDs, which makes this the only visible proof
 * that the emitter is sending. */
static void loop_test_led(uint32_t now)
{
    gpio_status_led((now - s_last_tx_ms) < IR_TX_FLASH_MS);
}
#endif /* IR_LOOP_TEST */

static void emitter_status_led(uint32_t now)
{
#if IR_DIAG_IR_LED
    /* proof that a frame really went out, counted by the flash length */
    gpio_status_led((now - s_last_tx_ms) < IR_DIAG_FLASH_MS);
#elif IR_LOOP_TEST
    loop_test_led(now);
#else
    if (!s_linked) {
        gpio_status_led(false);
        return;
    }
    gpio_status_led(((now / STATUS_SLOW_BLINK_MS) & 1U) != 0U);
#endif
}

static void emitter_poll(void)
{
    uint32_t now = HAL_GetTick();
    host_frame_t frame;

    /* decode everything the UART ISR buffered */
    while (frame_parser_poll(&frame)) {
        emitter_handle_frame(&frame);
    }

#if IR_REVERSE_LINK
    if (s_battery != BAT_UNKNOWN && (now - s_battery_rx_ms) > IR_BAT_STALE_MS) {
        s_battery = BAT_UNKNOWN;
    }
#endif

#if IR_LOOP_TEST
    loop_test_service(now);
#endif

    emitter_ir_service(now);

    if (s_linked && (now - s_last_upload_ms) >= UPLOAD_INTERVAL_MS) {
        s_last_upload_ms = now;
        uploading_data();
    }

    emitter_status_led(now);
}

/* ------------------------------------------------------------------ */
/* receiver                                                            */
/* ------------------------------------------------------------------ */

static void receiver_sample_battery(uint32_t now)
{
    s_bat_percent = bat_mv_to_percent(bat_adc_read_pack_mv());
    s_last_bat_ms = now;
    s_bat_sampled = true;
}

static void receiver_service(uint32_t now)
{
    uint8_t addr;
    uint8_t cmd;

    if (ir_poll_decode(&addr, &cmd)) {
        /* Decoding a frame blocks for up to ~68 ms; re-read the tick so the
         * hold window is not shortened by the decode time itself. */
        now = HAL_GetTick();

        if (addr == IR_ADDR_FORWARD && cmd < (uint8_t)IR_COLOR_NUM) {
            ir_apply_color(cmd);

            /* Only red/green/blue are held: command 0 is "off now" and must
             * not start (or extend) the timer. */
            if (cmd != (uint8_t)IR_COLOR_OFF) {
                s_hold_ms = now;
            }
#if IR_REVERSE_LINK
            s_last_frame_ms = now;
            s_reply_pending = true;
#endif
        }
    }

    /* Any sign that the emitter is still transmitting keeps an active colour
     * alive: a frame carrying our forward address (even when its command byte
     * was damaged), or simply the demodulator seeing 38 kHz carrier - held at
     * very short range the module is overloaded and no frame survives, but the
     * carrier still proves the emitter is still there. True silence lets the
     * hold expire as before. */
    {
        bool ours = ir_addr_seen(&addr) && addr == IR_ADDR_FORWARD;
        bool carrier = ir_carrier_seen();

#if IR_DIAG_IR_LED
        if (carrier) {
            s_diag_carrier_ms = HAL_GetTick();
        }
#endif
        if ((ours || carrier) && s_color != (uint8_t)IR_COLOR_OFF) {
            now = HAL_GetTick();
            s_hold_ms = now;
        }
    }

    /* Drop a colour nobody refreshed in time. Unsigned difference, so a
     * HAL_GetTick() wrap is handled correctly; s_color guards the switch so
     * the RGB is driven only on the one poll that actually times out. */
    if (s_color != (uint8_t)IR_COLOR_OFF &&
        (now - s_hold_ms) >= IR_HOLD_MS) {
        ir_apply_color(IR_COLOR_OFF);
    }

    if (!s_bat_sampled || (now - s_last_bat_ms) >= BAT_SAMPLE_INTERVAL_MS) {
        receiver_sample_battery(now);
    }

#if IR_REVERSE_LINK
    if (s_reply_pending &&
        (now - s_last_frame_ms) >= IR_REPLY_SILENCE_MS &&
        (now - s_last_reply_ms) >= IR_REPLY_MIN_INTERVAL_MS) {
        uint8_t percent = (s_bat_percent > 100U) ? 100U : s_bat_percent;

        ir_send_frame(IR_ADDR_REVERSE, percent);
        s_reply_pending = false;
        s_last_reply_ms = HAL_GetTick();
    }
#endif
}

static void receiver_status_led(uint32_t now)
{
#if IR_DIAG_IR_LED
    /* carrier-activity probe instead of the battery indication */
    gpio_status_led((now - s_diag_carrier_ms) < IR_DIAG_CARRIER_HOLD_MS);
#else
    if (s_bat_sampled && s_bat_percent <= BAT_LOW_PERCENT) {
        gpio_status_led(((now / BAT_LOW_BLINK_MS) & 1U) != 0U);
    } else {
        gpio_status_led(true);
    }
#endif
}

/* ------------------------------------------------------------------ */
/* public entry points                                                 */
/* ------------------------------------------------------------------ */

void ir_app_init(void)
{
    gpio_board_init();
    gpio_latch_role();

#if IR_TEST_ROLE == 1
    s_role = IR_ROLE_EMITTER;   /* bench override: ignore PB2 */
#elif IR_TEST_ROLE == 2
    s_role = IR_ROLE_RECEIVER;  /* bench override: ignore PB2 */
#else
    s_role = gpio_role_is_emitter() ? IR_ROLE_EMITTER : IR_ROLE_RECEIVER;
#endif

    tim_us_init();
    ir_proto_init();
    bat_adc_init();

    s_linked = false;
    s_burst_left = 0U;
    s_last_refresh_ms = HAL_GetTick();
    s_last_upload_ms = HAL_GetTick();
    s_bat_sampled = false;
    s_hold_ms = HAL_GetTick();
#if IR_DIAG_IR_LED
    s_diag_carrier_ms = HAL_GetTick();
#endif
#if IR_REVERSE_LINK
    s_listen_until_ms = 0U;
    s_reply_pending = false;
#endif

    ir_apply_color(IR_COLOR_OFF);

#if IR_LOOP_TEST
    loop_test_boot_signature();   /* red -> green -> blue once, both roles */
    loop_test_pb2_indication();   /* 1 pulse = PB2 high, 3 pulses = PB2 low */
#endif

    if (s_role == IR_ROLE_EMITTER) {
        host_uart_init();
        frame_parser_reset();
    }

    gpio_status_led(false);
}

void ir_app_poll(void)
{
    if (s_role == IR_ROLE_EMITTER) {
        emitter_poll();
    } else {
        receiver_service(HAL_GetTick());
        receiver_status_led(HAL_GetTick());
    }
}

/* Index 0xED upload: exactly the two fields agreed with the host. */
void uploading_data(void)
{
    ir_packet_t packet;

    packet.state = s_color;
    packet.bat   = s_battery;
    MultiUart_SendFrame(host_uart_send_more, (const uint8_t *)&packet,
                        (uint16_t)sizeof(packet), IDX_UPLOAD);
}
