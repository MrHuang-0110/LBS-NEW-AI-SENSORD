/* tim.c - 1 us free running time base (TIM14)
 *
 * IR envelope timing needs microsecond resolution (NEC bit marks are 560 us)
 * so HAL_GetTick() is far too coarse. TIM14 is used as a plain free running
 * counter: it has no interrupt and is never used for anything else here.
 * SystemCoreClock is 24 MHz, so prescaler 24 - 1 gives 1 MHz.
 */
#include "tim.h"

#define TIM_US_PRESCALER    (24U - 1U)

TIM_HandleTypeDef htim14;

void tim_us_init(void)
{
    TIM_ClockConfigTypeDef clk = {0};
    TIM_MasterConfigTypeDef master = {0};

    /* TIM14 clock + NVIC are handled by HAL_TIM_Base_MspInit() */
    htim14.Instance = TIM14;
    htim14.Init.Prescaler     = TIM_US_PRESCALER;
    htim14.Init.CounterMode   = TIM_COUNTERMODE_UP;
    htim14.Init.Period        = 0xFFFFU;
    htim14.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    if (HAL_TIM_Base_Init(&htim14) != HAL_OK) {
        Error_Handler();
    }

    clk.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
    if (HAL_TIM_ConfigClockSource(&htim14, &clk) != HAL_OK) {
        Error_Handler();
    }

    master.MasterOutputTrigger = TIM_TRGO_RESET;
    master.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
    if (HAL_TIMEx_MasterConfigSynchronization(&htim14, &master) != HAL_OK) {
        Error_Handler();
    }

    if (HAL_TIM_Base_Start(&htim14) != HAL_OK) {
        Error_Handler();
    }
}

uint16_t tim_us_now(void)
{
    return (uint16_t)TIM14->CNT;
}

uint16_t tim_us_elapsed(uint16_t start)
{
    return (uint16_t)(tim_us_now() - start); /* wrap safe for < 65536 us */
}

void tim_delay_us(uint32_t us)
{
    uint16_t start = tim_us_now();

    while (tim_us_elapsed(start) < (uint16_t)us) {
        /* busy wait */
    }
}
