#include "packet.h"
#include <string.h>

/* ── CRC16 (Modbus standard) ─────────────────────────────── */
uint16_t PKT_CRC16(uint8_t *data, uint16_t len)
{
    uint16_t crc = 0xFFFF;         // Modbus CRC starts at 0xFFFF
    for (uint16_t i = 0; i < len; i++)
    {
        crc ^= (uint16_t)data[i];  // XOR byte into low byte of CRC
        for (uint8_t j = 0; j < 8; j++)
        {
            if (crc & 0x0001)
            {
                crc >>= 1;
                crc ^= 0xA001;     // Modbus polynomial (reversed 0x8005)
            } 
            else
            {
                crc >>= 1;
            }
        }
    }
    return crc;
}

/* ── Build a packet into out_buf ─────────────────────────── */
/* Returns total number of bytes written                      */
uint16_t PKT_Build(uint8_t node_id, uint8_t msg_type,
                   uint8_t *payload, uint8_t payload_len,
                   uint8_t *out_buf)
{

    if (payload_len > PKT_MAX_PAYLOAD) payload_len = PKT_MAX_PAYLOAD;

    uint8_t idx = 0;

    // header
    out_buf[idx++] = PKT_START_BYTE;
    out_buf[idx++] = node_id;
    out_buf[idx++] = msg_type;
    out_buf[idx++] = payload_len;

    // payload
    for (uint8_t i = 0; i < payload_len; i++)
    {
        out_buf[idx++] = payload[i];
    }

    // CRC over everything except START_BYTE
    uint16_t crc = PKT_CRC16(&out_buf[1], idx - 1);
    out_buf[idx++] = (uint8_t)(crc >> 8);    // CRC high byte
    out_buf[idx++] = (uint8_t)(crc & 0xFF);  // CRC low byte

    // // temporarily corrupt CRC for testing by flipping bits in CRC low byte
    // out_buf[idx - 1] ^= 0xFF;
    
    return idx; // total packet length
}

/* ── Parse one byte at a time (state machine) ────────────── */
/* Call this for every byte from RS485_ReadByte()             */
/* Returns 1 when a complete valid packet is received         */
/* Returns 0 while still waiting for more bytes              */
/* Returns 2 on CRC error (packet received but corrupted)    */
uint8_t PKT_Parse(uint8_t byte, Packet *out_pkt)
{
    static ParseState state       = PARSE_WAIT_START;
    static Packet     pkt         = {0};
    static uint8_t    payload_idx = 0;
    static uint8_t    raw[PKT_MAX_TOTAL]; // raw bytes for CRC check
    static uint8_t    raw_idx     = 0;

    switch (state)
    {
        case PARSE_WAIT_START:
            if (byte == PKT_START_BYTE)
            {
                // reset everything for new packet
                memset(&pkt, 0, sizeof(pkt));
                raw_idx     = 0;
                payload_idx = 0;
                state       = PARSE_NODE_ID;
            }
            break;

        case PARSE_NODE_ID:
            pkt.node_id    = byte;
            raw[raw_idx++] = byte;
            state          = PARSE_MSG_TYPE;
            break;

        case PARSE_MSG_TYPE:
            pkt.msg_type   = byte;
            raw[raw_idx++] = byte;
            state          = PARSE_LENGTH;
            break;

        case PARSE_LENGTH:
            pkt.length     = byte;
            raw[raw_idx++] = byte;
            if (pkt.length == 0) {
                state = PARSE_CRC_HIGH; // no payload, go straight to CRC
            } else if (pkt.length > PKT_MAX_PAYLOAD) {
                state = PARSE_WAIT_START; // invalid length, reset
            } else {
                state = PARSE_PAYLOAD;
            }
            break;

        case PARSE_PAYLOAD:
            pkt.payload[payload_idx++] = byte;
            raw[raw_idx++]             = byte;
            if (payload_idx >= pkt.length) {
                state = PARSE_CRC_HIGH;
            }
            break;

        case PARSE_CRC_HIGH:
            pkt.crc = (uint16_t)byte << 8; // store high byte
            state   = PARSE_CRC_LOW;
            break;

        case PARSE_CRC_LOW:
            pkt.crc |= byte;               // combine low byte
            state    = PARSE_WAIT_START;   // reset for next packet

            // verify CRC
            uint16_t expected = PKT_CRC16(raw, raw_idx);
            if (pkt.crc == expected) {
                *out_pkt = pkt;
                return 1; // valid packet
            } else {
                return 2; // CRC error
            }
    }
    return 0; // still waiting
}