/* adc.c - battery measurement (PB1, 1/2 divider, 3xAAA)
 *
 * Single conversion + polling, 8 samples averaged. Only the receiver role
 * uses the result; the emitter board exposes none, so the pack voltage is
 * relayed to the emitter over the IR reverse channel instead.
 *
 * Clock and pin setup live in HAL_ADC_MspInit() (py32f002b_hal_msp.c).
 */
#include "adc.h"

#define BAT_SAMPLES        8U
#define BAT_VREFINT_MV     1200U   /* PY32F002B internal VREFINT typical */

ADC_HandleTypeDef hadc;

static void adc_select_channel(uint32_t channel)
{
    ADC_ChannelConfTypeDef cfg = {0};

    HAL_ADC_Stop(&hadc);

    /* PY32 HAL adds channels into CHSELR instead of replacing the old one.
     * Keep this driver in single-channel mode, otherwise sweep/debug reads can
     * leave several channels selected and the following conversion is bogus. */
    hadc.Instance->CHSELR = 0U;

    cfg.Channel = channel;
    cfg.Rank    = ADC_RANK_CHANNEL_NUMBER;
    if (HAL_ADC_ConfigChannel(&hadc, &cfg) != HAL_OK) {
        Error_Handler();
    }
}

void bat_adc_init(void)
{
    hadc.Instance = ADC1;
    hadc.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV1;
    hadc.Init.Resolution            = ADC_RESOLUTION_12B;
    hadc.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
    hadc.Init.ScanConvMode          = ADC_SCAN_DIRECTION_FORWARD;
    hadc.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
    hadc.Init.LowPowerAutoWait      = DISABLE;
    hadc.Init.ContinuousConvMode    = DISABLE;
    hadc.Init.DiscontinuousConvMode = DISABLE;
    hadc.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
    hadc.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
    hadc.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
    hadc.Init.SamplingTimeCommon    = ADC_SAMPLETIME_239CYCLES_5;
    if (HAL_ADC_Init(&hadc) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_ADCEx_Calibration_Start(&hadc) != HAL_OK) {
        Error_Handler();
    }
}

uint16_t bat_adc_sample_channel(uint32_t channel)
{
    uint32_t sum = 0;
    uint32_t i;

    adc_select_channel(channel);

    /* Discard the first conversion after selecting a channel. This lets the
     * internal path (especially VREFINT) settle and avoids stale sample data. */
    if (HAL_ADC_Start(&hadc) != HAL_OK) {
        HAL_ADC_Stop(&hadc);
        return 0;
    }
    if (HAL_ADC_PollForConversion(&hadc, 1000U) != HAL_OK) {
        HAL_ADC_Stop(&hadc);
        return 0;
    }
    (void)HAL_ADC_GetValue(&hadc);
    HAL_ADC_Stop(&hadc);

    for (i = 0; i < BAT_SAMPLES; i++) {
        if (HAL_ADC_Start(&hadc) != HAL_OK) {
            HAL_ADC_Stop(&hadc);
            return 0;
        }
        if (HAL_ADC_PollForConversion(&hadc, 1000U) != HAL_OK) {
            HAL_ADC_Stop(&hadc);
            return 0;
        }
        sum += HAL_ADC_GetValue(&hadc);
        HAL_ADC_Stop(&hadc);
    }

    return (uint16_t)(sum / BAT_SAMPLES);
}

uint16_t bat_adc_read_pack_mv(void)
{
    uint32_t bat_raw = bat_adc_sample_channel(BAT_ADC_CH);
    uint32_t vref_raw = bat_adc_sample_channel(ADC_CHANNEL_VREFINT);
    uint32_t pin_mv;

    if (vref_raw == 0U) {
        return 0U;
    }

    /* The ADC conversion reference is not fixed at 3.3 V on this hardware.
     * Use the internal VREFINT reading as a ratio so the battery pin voltage is
     * computed correctly even when the ADC reference moves with the supply. */
    pin_mv = (bat_raw * BAT_VREFINT_MV) / vref_raw;

    return (uint16_t)(pin_mv * BAT_DIVIDER);
}

uint8_t bat_mv_to_percent(uint16_t pack_mv)
{
    if (pack_mv <= BAT_MV_EMPTY) {
        return 0;
    }
    if (pack_mv >= BAT_MV_FULL) {
        return 100;
    }
    return (uint8_t)(((uint32_t)(pack_mv - BAT_MV_EMPTY) * 100U) /
                     (BAT_MV_FULL - BAT_MV_EMPTY));
}
