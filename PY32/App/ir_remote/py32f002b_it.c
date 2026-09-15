/* py32f002b_it.c - interrupt handlers
 *
 * The host UART RX interrupt only stores the received byte; frames are parsed
 * in the main loop (pull_data_from_queue / frame_parser_poll). Nothing blocks
 * here.
 */
#include "py32f002b_it.h"
#include "rx_data_queue.h"
#include "usart.h"

void NMI_Handler(void)
{
}

void HardFault_Handler(void)
{
    while (1) {
    }
}

void SVC_Handler(void)
{
}

void PendSV_Handler(void)
{
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

/* Only enabled in the emitter role (see HAL_UART_MspInit). */
void USART1_IRQHandler(void)
{
    if ((USART1->SR & USART_SR_RXNE) != 0U) {
        uint8_t byte = (uint8_t)(USART1->DR & 0xFFU);
        (void)rx_queue_push(byte);
    }

    host_uart_irq_handler();
}
