/* main.c - IR remote (emitter / receiver, one firmware for both ends)
 *
 * PB2 decides the role at power-up:
 *   HIGH = emitter  (host UART link + IR sender)
 *   LOW  = receiver (IR receiver + RGB + battery ADC)
 *
 * See driver_ir.c for the application logic and ir_proto.c for the IR codec.
 */
#include "main.h"
#include "adc.h"
#include "agreement_comx.h"
#include "driver_ir.h"
#include "usart.h"

#if BAT_ADC_SWEEP
#include <stdio.h>

/* Bring-up helper: print the raw value of every ADC channel once a second so
 * the channel wired to PB1 (BAT_ADC_CH) can be identified. Frames are not used
 * here (the text is longer than FRAME_MAX_PAYLOAD): the line is plain text.
 * Never ship a build with BAT_ADC_SWEEP = 1.
 */
static void bat_sweep_loop(void)
{
    static char text[192];

    /* the sweep needs the host UART even on a receiver board */
    host_uart_init();

    for (;;) {
        /* PY32F002B has 8 external ADC channels (0..7); 8 = temp sensor and
         * 9 = VREFINT are internal and channel 10 does not exist on this part.
         * VREFINT (~1.2 V -> raw ~1489) is the ADC sanity reference. */
        int n = sprintf(text,
                        "ch0=%u ch1=%u ch2=%u ch3=%u ch4=%u ch5=%u "
                        "ch6=%u ch7=%u vrefint=%u\r\n",
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_0),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_1),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_2),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_3),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_4),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_5),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_6),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_7),
                        (unsigned)bat_adc_sample_channel(ADC_CHANNEL_VREFINT));

        host_uart_send_more(text, (uint16_t)n);

        /* Derived values for the configured battery channel: this separates a
         * raw-conversion problem from a percentage-mapping problem. With a 1/2
         * divider and a 4.8 V pack the raw value should be ~2978 here. */
        {
            unsigned raw  = (unsigned)bat_adc_sample_channel(BAT_ADC_CH);
            unsigned mv   = (raw * 3300U) / 4095U;
            unsigned pack = mv * BAT_DIVIDER;

            n = sprintf(text,
                        "BAT_ADC_CH=%u raw=%u mv=%u pack=%umV pct=%u%% "
                        "(expect raw~2978 for a 4.8V pack)\r\n",
                        (unsigned)BAT_ADC_CH, raw, mv, pack,
                        (unsigned)bat_mv_to_percent((uint16_t)pack));
            host_uart_send_more(text, (uint16_t)n);
        }

        HAL_Delay(1000);
    }
}
#endif /* BAT_ADC_SWEEP */

int main(void)
{
    HAL_Init();
    ir_app_init();

#if BAT_ADC_SWEEP
    bat_sweep_loop();
#endif

    while (1) {
        ir_app_poll();
    }
}

void Error_Handler(void)
{
    // pi-lens-ignore: no-reserved-identifiers
    __disable_irq();
    while (1) {
    }
}
