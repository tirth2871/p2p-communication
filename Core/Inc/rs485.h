/* ============================================================
 * Project      : P2P RS-485 — Phase 2
 * MCU          : STM32F407G-DISC1
 * Description  : Interrupt RX, circular buffer, ACK handling
 * ============================================================ */

#ifndef RS485_H
#define RS485_H

#include "stm32f4xx_hal.h"
#include <stdint.h>
#include <string.h>

/* ── Config ──────────────────────────────────────────────── */
#define RS485_DIR_PORT      GPIOA
#define RS485_DIR_PIN       GPIO_PIN_8
#define RS485_BUF_SIZE      64
#define RS485_MSG_MAX       32
#define RS485_TIMEOUT_MS    200

/* ── Circular Buffer ─────────────────────────────────────── */
typedef struct {
    uint8_t  buf[RS485_BUF_SIZE];
    uint16_t head;
    uint16_t tail;
    uint16_t count;
} RS485_RingBuf;

extern RS485_RingBuf rs485_rxbuf;
extern UART_HandleTypeDef huart2;

/* ── API ─────────────────────────────────────────────────── */
void    RS485_Init(void);
void    RS485_TX_Mode(void);
void    RS485_RX_Mode(void);
void    RS485_Send(uint8_t *data, uint16_t len);
uint8_t RS485_MessageReady(uint8_t *out_buf, uint16_t *out_len);
void    RS485_IRQHandler(void);

#endif