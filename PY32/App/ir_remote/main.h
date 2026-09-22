/* main.h - IR remote (emitter / receiver on one board)
 *
 * Board: PY30F002BF15 (PY32F002Bx5, 24 KB Flash / 3 KB RAM)
 * One firmware for both ends: PB2 is latched once after reset,
 * HIGH = emitter, LOW = receiver.
 *
 * Pin map (see AGENTS.md / CLAUDE.md "IR remote" section):
 *   PA0  IR LED drive: 38 kHz carrier bursts, LOW = emitting
 *   PA1  38 kHz demodulator output, LOW = carrier present
 *   PA3  USART1 TX  (emitter role only)
 *   PA4  USART1 RX  (emitter role only)
 *   PA5  red    LED, HIGH = on
 *   PA6  green  LED, HIGH = on
 *   PA7  blue   LED, HIGH = on
 *   PB0  status LED, LOW = on (blinks on low battery)
 *   PB1  battery ADC, receiver only, 1/2 divider, 3xAAA
 *   PB2  role select, internal pull-up input
 */
#ifndef IR_MAIN_H
#define IR_MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "py32f0xx_hal.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

/* ------------------------------------------------------------------ */
/* role select: PB2, internal pull-up input, sampled once at boot      */
/* ------------------------------------------------------------------ */
#define ROLE_GPIO_PORT          GPIOB
#define ROLE_GPIO_PIN           GPIO_PIN_2

/* ------------------------------------------------------------------ */
/* IR physical layer: PA0 (TX envelope, active low) / PA1 (RX, active low) */
/* ------------------------------------------------------------------ */
#define IR_TX_GPIO_PORT         GPIOA
#define IR_TX_GPIO_PIN          GPIO_PIN_0
#define IR_RX_GPIO_PORT         GPIOA
#define IR_RX_GPIO_PIN          GPIO_PIN_1

/* This product's emitter board carries ONLY the IR LED - it has no 38 kHz
 * receiver head - so the receiver's battery reading can never be relayed
 * back and `bat` is always reported as BAT_UNKNOWN. The reverse link is
 * therefore compiled out. Set to 1 if a future hardware revision fits an IR
 * receiver on the emitter side (see Doc/IR_REMOTE_protocol.md). */
#ifndef IR_REVERSE_LINK
#define IR_REVERSE_LINK         0
#endif

/* ------------------------------------------------------------------ */
/* RGB / status LEDs                                                   */
/* ------------------------------------------------------------------ */
#define RGB_GPIO_PORT           GPIOA
#define RGB_RED_PIN             GPIO_PIN_5
#define RGB_GREEN_PIN           GPIO_PIN_6
#define RGB_BLUE_PIN            GPIO_PIN_7
#define RGB_ALL_PINS            (RGB_RED_PIN | RGB_GREEN_PIN | RGB_BLUE_PIN)

#define STATUS_GPIO_PORT        GPIOB
#define STATUS_GPIO_PIN         GPIO_PIN_0

/* ------------------------------------------------------------------ */
/* battery (receiver only)                                             */
/* ------------------------------------------------------------------ */
#define BAT_GPIO_PORT           GPIOB
#define BAT_GPIO_PIN            GPIO_PIN_1

/* ADC channel wired to PB1.
 * PY32F002B pin table: PB1 = ADC_IN0 (other ADC pins on this part: PA4 = IN2,
 * PA6 = IN3, PB0 = IN7 - the numbering is not a linear PA0..PA7 mapping). */
#ifndef BAT_ADC_CH
#define BAT_ADC_CH              ADC_CHANNEL_0
#endif

#define BAT_DIVIDER             2U        /* 1/2 divider on the pin   */
#define BAT_MV_EMPTY            3000U     /* pack voltage = 0 %       */
#define BAT_MV_FULL             4500U     /* pack voltage = 100 %     */
#define BAT_LOW_PERCENT         10U       /* blink threshold          */
#define BAT_UNKNOWN             0xFFU     /* reported when no IR reply */

/* optional bring-up aid: set to 1 to print a raw sweep of ADC channel 0..10
 * over the host UART (emitter board only, never ship with 1). The channel used
 * for the battery is BAT_ADC_CH above - PB1 = ADC_IN0 on PY32F002B. */
#ifndef BAT_ADC_SWEEP
#define BAT_ADC_SWEEP           0
#endif

/* ------------------------------------------------------------------ */
/* bench test mode                                                    */
/* ------------------------------------------------------------------ */
/* Set to 1 to run the IR link loopback test without a host:
 *   - emitter role: cycles red -> green -> blue every IR_LOOP_INTERVAL_MS and
 *     sends each colour over IR; PB0 is solid while the receiver's battery
 *     replies arrive (link confirmed both ways), off when nothing comes back.
 *   - receiver role: unchanged, it only follows the decoded IR frames.
 * Flash the SAME test build to both boards: matching colour sequences prove
 * the IR link. For bring-up only - never ship with 1. */
#ifndef IR_LOOP_TEST
#define IR_LOOP_TEST            0
#endif

/* Bench aid: force the role instead of reading PB2 at boot. Removes the PB2
 * strap / wiring from the equation while bringing a board up.
 *   0 = normal, read PB2 (shipping behaviour)
 *   1 = force EMITTER   (host link + IR sender)
 *   2 = force RECEIVER  (IR follower + RGB + battery)
 * Only meant for bench builds: with 1/2 the PB2 pin is ignored. */
#ifndef IR_TEST_ROLE
#define IR_TEST_ROLE            0
#endif

/* Bench diagnostic for the IR link itself: reuses the status LED (PB0) so no
 * extra hardware is needed, without changing what is transmitted.
 *   emitter : PB0 flashes briefly after EVERY IR frame actually sent, so you
 *             can see whether the 1 s refresh bursts are really going out
 *             (3 quick flashes per second) instead of the link-status blink.
 *   receiver: PB0 lights while the demodulator keeps seeing 38 kHz carrier,
 *             so it blinks in sync with incoming bursts. Dark PB0 while the
 *             emitter is pointed at it means the module sees nothing at that
 *             distance/angle (physical), not a hold-logic problem.
 * Never ship with 1 (it replaces the low-battery indication on the receiver
 * and the link indication on the emitter). */
#ifndef IR_DIAG_IR_LED
#define IR_DIAG_IR_LED          0
#endif

/* ------------------------------------------------------------------ */
/* host UART (emitter role only)                                       */
/* ------------------------------------------------------------------ */
#define HOST_UART_INSTANCE      USART1
#define HOST_UART_BAUD          115200U
#define HOST_UART_TX_PORT       GPIOA
#define HOST_UART_TX_PIN        GPIO_PIN_3
#define HOST_UART_RX_PORT       GPIOA
#define HOST_UART_RX_PIN        GPIO_PIN_4

/* ------------------------------------------------------------------ */
/* firmware version reported to the host (keep in sync with releases)   */
/* ------------------------------------------------------------------ */
#define IR_FW_VERSION           1U

/* role / colour ------------------------------------------------------ */
typedef enum {
    IR_ROLE_RECEIVER = 0,
    IR_ROLE_EMITTER  = 1
} ir_role_t;

typedef enum {
    IR_COLOR_OFF   = 0,
    IR_COLOR_RED   = 1,
    IR_COLOR_GREEN = 2,
    IR_COLOR_BLUE  = 3,
    IR_COLOR_NUM
} ir_color_t;

/* ------------------------------------------------------------------ */
/* app entry points (driver_ir.c)                                      */
/* ------------------------------------------------------------------ */
void ir_app_init(void);
void ir_app_poll(void);

/* role is latched in ir_app_init() */
ir_role_t ir_app_role(void);

/* GPIO helpers (gpio.c) */
void gpio_board_init(void);
void gpio_rgb_set(uint8_t color);
void gpio_status_led(bool on);
void gpio_ir_tx(bool emitting);
bool gpio_ir_rx_low(void);
bool gpio_role_is_emitter(void);

/* 1 us free running time base (tim.c) */
void tim_us_init(void);
uint16_t tim_us_now(void);
void tim_delay_us(uint32_t us);
uint16_t tim_us_elapsed(uint16_t start);

/* host UART (usart.c) */
void host_uart_init(void);
void host_uart_send_more(void *data, uint16_t length);

/* battery ADC (adc.c) */
void bat_adc_init(void);
uint16_t bat_adc_read_pack_mv(void);
uint8_t bat_mv_to_percent(uint16_t pack_mv);

/* IR link (ir_proto.c) */
void ir_proto_init(void);
void ir_send_frame(uint8_t addr, uint8_t cmd);
bool ir_poll_decode(uint8_t *addr, uint8_t *cmd);

void Error_Handler(void);

#ifdef __cplusplus
}
#endif

#endif /* IR_MAIN_H */
