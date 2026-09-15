/* gpio.c - board GPIO setup and helpers for IR remote
 *
 * PA0  IR TX envelope  (out, idle HIGH = not emitting)
 * PA1  IR RX demod out (in, pull-up, idle HIGH)
 * PA3/PA4 are configured by usart.c (AF1) only in the emitter role.
 * PA5/PA6/PA7 RGB      (out, active HIGH)
 * PB0  status LED      (out, active LOW)
 * PB1  battery ADC     (analog, configured by adc.c, receiver only)
 * PB2  role select     (in, internal pull-up)
 */
#include "gpio.h"

static bool s_role_emitter;

void gpio_board_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_GPIOB_CLK_ENABLE();

    /* PA5/PA6/PA7 RGB off, PA0 not emitting (HIGH) */
    HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_ALL_PINS, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(IR_TX_GPIO_PORT, IR_TX_GPIO_PIN, GPIO_PIN_SET);
    HAL_GPIO_WritePin(STATUS_GPIO_PORT, STATUS_GPIO_PIN, GPIO_PIN_SET); /* active low */

    /* outputs: IR TX + RGB */
    gpio.Pin   = IR_TX_GPIO_PIN | RGB_ALL_PINS;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(GPIOA, &gpio);

    /* PB0 status LED + PB2 role select (pull-up input) */
    gpio.Pin   = STATUS_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_OUTPUT_PP;
    gpio.Pull  = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(STATUS_GPIO_PORT, &gpio);

    /* PB2 mode strap - see gpio_latch_role() for the hardware convention:
     * receiver board = externally pulled low, emitter board = left open and
     * held high by this internal pull-up. */
    gpio.Pin  = ROLE_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_PULLUP;
    HAL_GPIO_Init(ROLE_GPIO_PORT, &gpio);

    /* PA1 IR receiver input, pull-up keeps it defined while idle */
    gpio.Pin   = IR_RX_GPIO_PIN;
    gpio.Mode  = GPIO_MODE_INPUT;
    gpio.Pull  = GPIO_PULLUP;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    HAL_GPIO_Init(IR_RX_GPIO_PORT, &gpio);
}

/* Latched once, right after the role pin has settled. */
bool gpio_role_is_emitter(void)
{
    return s_role_emitter;
}

/* PB2 mode strap, latched once at boot.
 *
 * Hardware convention:
 *   receiver board -> PB2 is pulled LOW by an external resistor
 *   emitter board  -> PB2 is left OPEN; the internal pull-up enabled in
 *                     gpio_board_init() is what makes it read HIGH
 *
 * The internal pull-up on this part is weak, so allow a generous settle time
 * and require a majority of several samples before latching the role. This only
 * reads the pin: it never drives it, so it is safe even if PB2 is tied hard to
 * ground on a receiver board.
 */
#define ROLE_SETTLE_MS      5U
#define ROLE_SAMPLES        5U

void gpio_latch_role(void)
{
    GPIO_InitTypeDef gpio = {0};
    uint8_t high = 0U;
    uint8_t i;

    HAL_Delay(ROLE_SETTLE_MS);

    for (i = 0U; i < ROLE_SAMPLES; i++) {
        if (HAL_GPIO_ReadPin(ROLE_GPIO_PORT, ROLE_GPIO_PIN) == GPIO_PIN_SET) {
            high++;
        }
        HAL_Delay(1U);
    }

    s_role_emitter = (high > (ROLE_SAMPLES / 2U));

    /* The role is latched now and PB2 is never read again, so release the
     * internal pull-up: on a battery powered receiver board it would otherwise
     * keep sourcing current through the external pull-down the whole time. */
    gpio.Pin  = ROLE_GPIO_PIN;
    gpio.Mode = GPIO_MODE_INPUT;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(ROLE_GPIO_PORT, &gpio);
}

void gpio_rgb_set(uint8_t color)
{
    switch (color) {
        case IR_COLOR_RED:
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_RED_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_GREEN_PIN | RGB_BLUE_PIN, GPIO_PIN_RESET);
            break;
        case IR_COLOR_GREEN:
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_GREEN_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_RED_PIN | RGB_BLUE_PIN, GPIO_PIN_RESET);
            break;
        case IR_COLOR_BLUE:
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_BLUE_PIN, GPIO_PIN_SET);
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_RED_PIN | RGB_GREEN_PIN, GPIO_PIN_RESET);
            break;
        default:
            HAL_GPIO_WritePin(RGB_GPIO_PORT, RGB_ALL_PINS, GPIO_PIN_RESET);
            break;
    }
}

void gpio_status_led(bool on)
{
    HAL_GPIO_WritePin(STATUS_GPIO_PORT, STATUS_GPIO_PIN,
                      on ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

void gpio_ir_tx(bool emitting)
{
    HAL_GPIO_WritePin(IR_TX_GPIO_PORT, IR_TX_GPIO_PIN,
                      emitting ? GPIO_PIN_RESET : GPIO_PIN_SET);
}

bool gpio_ir_rx_low(void)
{
    return (IR_RX_GPIO_PORT->IDR & IR_RX_GPIO_PIN) == 0U;
}
