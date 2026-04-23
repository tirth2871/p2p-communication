/* ============================================================
 * Project      : P2P RS-485 — Phase 3
 * MCU          : STM32F407G-DISC1
 * Description  : Packet framing, CRC16, message types
 * ============================================================ */

#ifndef PACKET_H
#define PACKET_H

#include "stm32f4xx_hal.h"
#include <stdint.h>

/* ── Packet constants ────────────────────────────────────── */
#define PKT_START_BYTE      0xAA    // every packet begins with this
#define PKT_MAX_PAYLOAD     32      // max bytes of payload data
#define PKT_OVERHEAD        6       // START + NODE_ID + MSG_TYPE + LENGTH + CRC_H + CRC_L
#define PKT_MAX_TOTAL       (PKT_OVERHEAD + PKT_MAX_PAYLOAD)

/* ── Message types ───────────────────────────────────────── */
#define MSG_DATA            0x01    // data message
#define MSG_ACK             0x02    // acknowledgement
#define MSG_ERROR           0x03    // error response

/* ── Packet structure ────────────────────────────────────── */
/*
    Byte 0   : START_BYTE (0xAA)
    Byte 1   : sender NODE_ID (1 or 2)
    Byte 2   : MSG_TYPE (MSG_DATA, MSG_ACK, MSG_ERROR)
    Byte 3   : PAYLOAD_LENGTH (0–32)
    Byte 4–N : PAYLOAD
    Byte N+1 : CRC16 high byte
    Byte N+2 : CRC16 low byte
*/

typedef struct {
    uint8_t  node_id;
    uint8_t  msg_type;
    uint8_t  length;
    uint8_t  payload[PKT_MAX_PAYLOAD];
    uint16_t crc;
} Packet;

/* ── Parser state machine ────────────────────────────────── */
typedef enum {
    PARSE_WAIT_START,
    PARSE_NODE_ID,
    PARSE_MSG_TYPE,
    PARSE_LENGTH,
    PARSE_PAYLOAD,
    PARSE_CRC_HIGH,
    PARSE_CRC_LOW
} ParseState;

/* ── API ─────────────────────────────────────────────────── */
uint16_t PKT_CRC16(uint8_t *data, uint16_t len);
uint16_t PKT_Build(uint8_t node_id, uint8_t msg_type,
                   uint8_t *payload, uint8_t payload_len,
                   uint8_t *out_buf);
uint8_t  PKT_Parse(uint8_t byte, Packet *out_pkt);

#endif