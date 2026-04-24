#include "rs485.h"

RS485_RingBuf rs485_rxbuf = {0};

/* ── Ring buffer: write ──────────────────────────────────── */
static void _ring_write(uint8_t byte) {
    if (rs485_rxbuf.count >= RS485_BUF_SIZE) return; // drop if full
    rs485_rxbuf.buf[rs485_rxbuf.head] = byte;
    rs485_rxbuf.head = (rs485_rxbuf.head + 1) % RS485_BUF_SIZE;
    rs485_rxbuf.count++;
}

/* ── Direction control ───────────────────────────────────── */
void RS485_TX_Mode(void) {
    HAL_GPIO_WritePin(RS485_DIR_PORT, RS485_DIR_PIN, GPIO_PIN_SET);
    for (volatile int i = 0; i < 1000; i++); // let transceiver switch
}

void RS485_RX_Mode(void) {
    HAL_GPIO_WritePin(RS485_DIR_PORT, RS485_DIR_PIN, GPIO_PIN_RESET);
}

/* ── Init ────────────────────────────────────────────────── */
void RS485_Init(void) {
    RS485_RX_Mode();
    // enable RXNE interrupt — fires every time a byte arrives
    __HAL_UART_ENABLE_IT(&huart2, UART_IT_RXNE);

    // tell the CPU's interrupt controller to listen for USART2
    HAL_NVIC_SetPriority(USART2_IRQn, 0, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
}

/* ── Send ────────────────────────────────────────────────── */
void RS485_Send(uint8_t *data, uint16_t len) {
    RS485_TX_Mode();
    HAL_UART_Transmit(&huart2, data, len, HAL_MAX_DELAY);
    // wait for last bit to physically leave the wire before switching
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_TC) == RESET);
    HAL_Delay(5); // add 2ms after TC before switching back
    RS485_RX_Mode();
    HAL_Delay(5); // let transceiver switch before accepting bytes

    // flush anything that arrived during TX or switching transient
    while (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        volatile uint8_t dummy = (uint8_t)(huart2.Instance->DR & 0xFF);
        (void)dummy;
    }

    // also flush ring buffer
    rs485_rxbuf.head  = 0;
    rs485_rxbuf.tail  = 0;
    rs485_rxbuf.count = 0;
}

/* ── Check if '\n'-terminated message is in buffer ──────── */
uint8_t RS485_MessageReady(uint8_t *out_buf, uint16_t *out_len) {
    uint16_t count = rs485_rxbuf.count;
    uint16_t tail  = rs485_rxbuf.tail;
    uint16_t i;

    for (i = 0; i < count && i < RS485_MSG_MAX; i++) {
        uint8_t byte = rs485_rxbuf.buf[(tail + i) % RS485_BUF_SIZE];
        if (byte == '\n') {
            // copy message (without '\n') into out_buf
            for (uint16_t j = 0; j < i; j++) {
                out_buf[j] = rs485_rxbuf.buf[(tail + j) % RS485_BUF_SIZE];
            }
            out_buf[i] = '\0';
            *out_len   = i;
            // consume bytes from ring buffer
            rs485_rxbuf.tail  = (tail + i + 1) % RS485_BUF_SIZE;
            rs485_rxbuf.count -= (i + 1);
            return 1;
        }
    }
    return 0;
}

/* ── IRQ: called from USART2_IRQHandler ──────────────────── */
void RS485_IRQHandler(void) {
    if (__HAL_UART_GET_FLAG(&huart2, UART_FLAG_RXNE)) {
        uint8_t byte = (uint8_t)(huart2.Instance->DR & 0xFF);
        _ring_write(byte);
    }
}

uint8_t RS485_ReadByte(uint8_t *out)
{
    if (rs485_rxbuf.count == 0) return 0; // no data
    *out = rs485_rxbuf.buf[rs485_rxbuf.tail];
    rs485_rxbuf.tail = (rs485_rxbuf.tail + 1) % RS485_BUF_SIZE;
    rs485_rxbuf.count--;
    return 1;
}