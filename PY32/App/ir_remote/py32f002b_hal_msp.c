/* py32f002b_hal_msp.c - MSP (clock / pin / NVIC) setup for the IR remote
 *
 * All peripheral clock enables and alternate-function setup are kept here so
 * the driver files stay free of clock plumbing.
 *
 * PA3/PA4 -> USART1 (AF1), emitter role only
 * PB1     -> ADC channel for the battery divider, receiver role only
 */
#include "main.h"
#include "adc.h"
#include "usart.h"
#include "tim.h"

void HAL_MspInit(void)
{
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_SYSCFG_CLK_ENABLE();
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_PWR_CLK_ENABLE();
}

void HAL_UART_MspInit(UART_HandleTypeDef *huart)
{
    GPIO_InitTypeDef gpio = {0};

    if (huart->Instance != HOST_UART_INSTANCE) {
        return;
    }

    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_GPIOA_CLK_ENABLE();
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_USART1_CLK_ENABLE();

    gpio.Pin       = HOST_UART_TX_PIN | HOST_UART_RX_PIN;
    gpio.Mode      = GPIO_MODE_AF_PP;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF1_USART1;
    HAL_GPIO_Init(HOST_UART_TX_PORT, &gpio);

    HAL_NVIC_SetPriority(USART1_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART1_IRQn);
}

void HAL_UART_MspDeInit(UART_HandleTypeDef *huart)
{
    if (huart->Instance != HOST_UART_INSTANCE) {
        return;
    }

    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_USART1_CLK_DISABLE();
    HAL_GPIO_DeInit(HOST_UART_TX_PORT, HOST_UART_TX_PIN | HOST_UART_RX_PIN);
    HAL_NVIC_DisableIRQ(USART1_IRQn);
}

/* TIM14 is only used as a 1 us free running counter: no interrupt, no channel. */
void HAL_TIM_Base_MspInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM14) {
        return;
    }
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_TIM14_CLK_ENABLE();
}

void HAL_TIM_Base_MspDeInit(TIM_HandleTypeDef *htim)
{
    if (htim->Instance != TIM14) {
        return;
    }
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_TIM14_CLK_DISABLE();
}

void HAL_ADC_MspInit(ADC_HandleTypeDef *hadc_handle)
{
    GPIO_InitTypeDef gpio = {0};

    if (hadc_handle->Instance != ADC1) {
        return;
    }

    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_GPIOB_CLK_ENABLE();
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_ADC_CLK_ENABLE();

    /* PB1 as analog input (battery divider) */
    gpio.Pin  = BAT_GPIO_PIN;
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(BAT_GPIO_PORT, &gpio);

    /* ADC_COMP_IRQn is not used: the battery is read by polling. */
}

void HAL_ADC_MspDeInit(ADC_HandleTypeDef *hadc_handle)
{
    if (hadc_handle->Instance != ADC1) {
        return;
    }
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_RCC_ADC_CLK_DISABLE();
    HAL_GPIO_DeInit(BAT_GPIO_PORT, BAT_GPIO_PIN);
}
