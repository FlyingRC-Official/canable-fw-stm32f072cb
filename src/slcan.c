#include <string.h>
#include "can.h"
#include "error.h"
#include "slcan.h"
#include "printf.h"
#include "usbd_cdc_if.h"

static uint8_t hex_value(uint8_t c)
{
    if (c >= '0' && c <= '9') return (uint8_t)(c - '0');
    if (c >= 'A' && c <= 'F') return (uint8_t)(c - 'A' + 10U);
    if (c >= 'a' && c <= 'f') return (uint8_t)(c - 'a' + 10U);
    return 0xFFU;
}

static uint8_t hex_digit(uint8_t value)
{
    if (value < 10U) return (uint8_t)('0' + value);
    return (uint8_t)('A' + (value - 10U));
}

int8_t slcan_parse_frame(uint8_t *buf, const can_frame_t *frame)
{
    uint8_t pos = 0U;
    uint8_t id_len;

    if (buf == NULL || frame == NULL || frame->dlc > 8U) return -1;

    buf[pos++] = frame->is_extended
        ? (frame->is_remote ? 'R' : 'T')
        : (frame->is_remote ? 'r' : 't');
    id_len = frame->is_extended ? SLCAN_EXT_ID_LEN : SLCAN_STD_ID_LEN;
    for (uint8_t i = 0U; i < id_len; ++i) {
        uint8_t shift = (uint8_t)((id_len - i - 1U) * 4U);
        buf[pos++] = hex_digit((uint8_t)((frame->id >> shift) & 0x0FU));
    }
    buf[pos++] = hex_digit(frame->dlc);
    if (!frame->is_remote) {
        for (uint8_t i = 0U; i < frame->dlc; ++i) {
            buf[pos++] = hex_digit((uint8_t)(frame->data[i] >> 4));
            buf[pos++] = hex_digit((uint8_t)(frame->data[i] & 0x0FU));
        }
    }
    buf[pos++] = '\r';
    return (int8_t)pos;
}

int8_t slcan_parse_str(uint8_t *buf, uint8_t len)
{
    can_frame_t frame = {0};
    uint8_t id_len;
    uint8_t pos;

    if (buf == NULL || len == 0U) return -1;

    switch (buf[0]) {
        case 'O': can_enable(); return 0;
        case 'C': can_disable(); return 0;
        case 'S':
            if (len != 2U) return -1;
            pos = hex_value(buf[1]);
            if (pos >= CAN_BITRATE_INVALID) return -1;
            can_set_bitrate((can_bitrate_t)pos);
            return 0;
        case 'm':
        case 'M':
            if (len != 2U) return -1;
            pos = hex_value(buf[1]);
            if (pos > 1U) return -1;
            can_set_silent(pos);
            return 0;
        case 'a':
        case 'A':
            if (len != 2U) return -1;
            pos = hex_value(buf[1]);
            if (pos > 1U) return -1;
            can_set_autoretransmit(pos);
            return 0;
        case 'V': {
            static const char fw_id[] = GIT_VERSION " " GIT_REMOTE "\r";
            CDC_Transmit_FS((uint8_t *)fw_id, (uint16_t)strlen(fw_id));
            return 0;
        }
        case 'E': {
            char errstr[64];
            int n = snprintf_(errstr, sizeof(errstr), "CANable Error Register: %X", (unsigned int)error_reg());
            if (n > 0) CDC_Transmit_FS((uint8_t *)errstr, (uint16_t)n);
            return 0;
        }
        case 'T': frame.is_extended = 1U; frame.is_remote = 0U; break;
        case 't': frame.is_extended = 0U; frame.is_remote = 0U; break;
        case 'R': frame.is_extended = 1U; frame.is_remote = 1U; break;
        case 'r': frame.is_extended = 0U; frame.is_remote = 1U; break;
        default: return -1;
    }

    id_len = frame.is_extended ? SLCAN_EXT_ID_LEN : SLCAN_STD_ID_LEN;
    if (len < (uint8_t)(1U + id_len + 1U)) return -1;
    pos = 1U;
    for (uint8_t i = 0U; i < id_len; ++i) {
        uint8_t nibble = hex_value(buf[pos++]);
        if (nibble > 0x0FU) return -1;
        frame.id = (frame.id << 4) | nibble;
    }

    frame.dlc = hex_value(buf[pos++]);
    if (frame.dlc > 8U) return -1;
    if (frame.is_remote) {
        if (len != pos) return -1;
    } else {
        if (len != (uint8_t)(pos + frame.dlc * 2U)) return -1;
        for (uint8_t i = 0U; i < frame.dlc; ++i) {
            uint8_t high = hex_value(buf[pos++]);
            uint8_t low = hex_value(buf[pos++]);
            if (high > 0x0FU || low > 0x0FU) return -1;
            frame.data[i] = (uint8_t)((high << 4) | low);
        }
    }
    return can_tx(&frame) == CAN_STATUS_OK ? 0 : -1;
}
