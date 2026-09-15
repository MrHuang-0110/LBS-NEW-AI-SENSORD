/* usart.c - host link (USART1, emitter role only)
 *
 * Framing/parsing lives in agreement_comx.c (rx_data_queue.c feeds bytes in).
 * Received bytes are stored by the RX interrupt, never parsed there.
 */
#include "usart.h"
#include "agreement_comx.h"
#include "rx_data_queue.h"

UART_HandleTypeDef huart1;

#define HOST_TX_BUF_SIZE    (FRAME_MAX_PAYLOAD + 7U)

static uint8_t  s_tx_buf[HOST_TX_BUF_SIZE];
static volatile uint16_t s_tx_len;
static volatile uint16_t s_tx_pos;
static volatile bool     s_tx_busy;

void host_uart_init(void)
{
    huart1.Instance          = HOST_UART_INSTANCE;
    huart1.Init.BaudRate     = HOST_UART_BAUD;
    huart1.Init.WordLength   = UART_WORDLENGTH_8B;
    huart1.Init.StopBits     = UART_STOPBITS_1;
    huart1.Init.Parity       = UART_PARITY_NONE;
    huart1.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart1.Init.Mode         = UART_MODE_TX_RX;
    if (HAL_UART_Init(&huart1) != HAL_OK) {
        Error_Handler();
    }

    rx_queue_init();
    // pi-lens-ignore: no-reserved-identifiers
    __HAL_UART_ENABLE_IT(&huart1, UART_IT_RXNE);
}

/* Frame writer callback (matches MultiUart_SendFrame's signature).
 *
 * Keep TX interrupt-driven instead of polling/blocking: IR frame generation can
 * occupy the CPU for tens of milliseconds, but the host still expects upload
 * frames about every 10 ms. The buffer is single-depth by design; an upload is
 * shorter than 1 ms at 115200 bps, so a busy TX means the next periodic upload
 * can be safely dropped. */
void host_uart_send_more(void *data, uint16_t length)
{
    uint16_t i;

    if (data == NULL || length == 0U || length > HOST_TX_BUF_SIZE) {
        return;
    }

    if (s_tx_busy) {
        return;
    }

    for (i = 0U; i < length; i++) {
        s_tx_buf[i] = ((uint8_t *)data)[i];
    }
    s_tx_len = length;
    s_tx_pos = 0U;
    s_tx_busy = true;

    huart1.Instance->CR1 |= USART_CR1_TXEIE;
}

void host_uart_irq_handler(void)
{
    if (((USART1->SR & USART_SR_TXE) != 0U) &&
        ((USART1->CR1 & USART_CR1_TXEIE) != 0U)) {
        if (s_tx_pos < s_tx_len) {
            USART1->DR = s_tx_buf[s_tx_pos++];
        } else {
            USART1->CR1 &= ~USART_CR1_TXEIE;
            USART1->CR1 |= USART_CR1_TCIE;
        }
    }

    if (((USART1->SR & USART_SR_TC) != 0U) &&
        ((USART1->CR1 & USART_CR1_TCIE) != 0U)) {
        USART1->CR1 &= ~USART_CR1_TCIE;
        s_tx_busy = false;
    }
}
