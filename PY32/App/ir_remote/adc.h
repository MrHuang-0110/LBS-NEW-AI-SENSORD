/* adc.h - battery measurement (receiver role only, PB1, 1/2 divider) */
#ifndef IR_ADC_H
#define IR_ADC_H

#include "main.h"

extern ADC_HandleTypeDef hadc;

/* ADC init + calibration. Called in both roles so the emitter board can run
 * the bring-up sweep, but only the receiver uses the result. */
void bat_adc_init(void);

/* Pack voltage in mV (pin voltage * BAT_DIVIDER). Returns 0 on conversion error. */
uint16_t bat_adc_read_pack_mv(void);

/* Raw averaged conversion of an arbitrary channel (bring-up sweep helper). */
uint16_t bat_adc_sample_channel(uint32_t channel);

/* Linear map BAT_MV_EMPTY..BAT_MV_FULL -> 0..100, clamped. */
uint8_t bat_mv_to_percent(uint16_t pack_mv);

#endif /* IR_ADC_H */
