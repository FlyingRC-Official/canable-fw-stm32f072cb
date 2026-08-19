#include <string.h>
#include "mavcan.h"
#include "system.h"
#include "usbd_cdc_if.h"

#define MAVLINK2_MAGIC 0xFDU
#define MAVLINK2_HEADER_LEN 10U
#define MAVLINK2_CORE_HEADER_LEN 9U
#define MAVLINK2_CHECKSUM_LEN 2U
#define MAVLINK2_SIGNATURE_LEN 13U
#define MAVLINK2_SIGNED_FLAG 0x01U

#define MAVLINK_MSG_ID_HEARTBEAT 0U
#define MAVLINK_MSG_ID_CAN_FRAME 386U
#define MAVLINK_MSG_HEARTBEAT_LEN 9U
#define MAVLINK_MSG_CAN_FRAME_LEN 16U
#define MAVLINK_MSG_HEARTBEAT_CRC 50U
#define MAVLINK_MSG_CAN_FRAME_CRC 132U

#define MAVCAN_SYSTEM_ID 1U
#define MAVCAN_COMPONENT_ID 1U
#define MAVCAN_BUS_INDEX 0U
#define MAVCAN_HEARTBEAT_INTERVAL_MS 1000U
#define MAVCAN_CAN_ID_EXTENDED 0x80000000UL
#define MAVCAN_CAN_ID_REMOTE 0x40000000UL
#define MAVCAN_CAN_ID_MASK 0x1FFFFFFFUL

typedef enum {
    PARSER_WAIT_MAGIC = 0,
    PARSER_HEADER,
    PARSER_PAYLOAD,
    PARSER_CHECKSUM_LOW,
    PARSER_CHECKSUM_HIGH,
    PARSER_SIGNATURE,
} mavcan_parser_stage_t;

typedef struct {
    mavcan_parser_stage_t stage;
    uint8_t header[MAVLINK2_CORE_HEADER_LEN];
    uint8_t payload[MAVLINK_MSG_CAN_FRAME_LEN];
    uint8_t header_index;
    uint16_t payload_index;
    uint16_t checksum;
    uint16_t received_checksum;
    uint8_t signature_remaining;
} mavcan_parser_t;

static mavcan_parser_t parser;
static uint8_t tx_sequence;
static uint32_t last_heartbeat_ms;

static void crc_accumulate(uint8_t data, uint16_t *crc)
{
    uint8_t tmp = (uint8_t)(data ^ (uint8_t)(*crc & 0xFFU));
    tmp = (uint8_t)(tmp ^ (uint8_t)(tmp << 4));
    *crc = (uint16_t)((*crc >> 8) ^ ((uint16_t)tmp << 8)
        ^ ((uint16_t)tmp << 3) ^ ((uint16_t)tmp >> 4));
}

static void put_le32(uint8_t *buffer, uint32_t value)
{
    buffer[0] = (uint8_t)value;
    buffer[1] = (uint8_t)(value >> 8);
    buffer[2] = (uint8_t)(value >> 16);
    buffer[3] = (uint8_t)(value >> 24);
}

static uint32_t get_le32(const uint8_t *buffer)
{
    return (uint32_t)buffer[0]
        | ((uint32_t)buffer[1] << 8)
        | ((uint32_t)buffer[2] << 16)
        | ((uint32_t)buffer[3] << 24);
}

static uint8_t mavlink_send(uint32_t message_id, const uint8_t *payload,
    uint8_t payload_length, uint8_t crc_extra)
{
    uint8_t packet[MAVLINK2_HEADER_LEN + MAVLINK_MSG_CAN_FRAME_LEN
        + MAVLINK2_CHECKSUM_LEN];
    uint16_t crc = 0xFFFFU;
    uint16_t packet_length;

    if (payload == NULL || payload_length > MAVLINK_MSG_CAN_FRAME_LEN) {
        return USBD_FAIL;
    }

    packet[0] = MAVLINK2_MAGIC;
    packet[1] = payload_length;
    packet[2] = 0U;
    packet[3] = 0U;
    packet[4] = tx_sequence++;
    packet[5] = MAVCAN_SYSTEM_ID;
    packet[6] = MAVCAN_COMPONENT_ID;
    packet[7] = (uint8_t)message_id;
    packet[8] = (uint8_t)(message_id >> 8);
    packet[9] = (uint8_t)(message_id >> 16);
    memcpy(&packet[MAVLINK2_HEADER_LEN], payload, payload_length);

    for (uint16_t i = 1U; i < (uint16_t)(MAVLINK2_HEADER_LEN + payload_length); ++i) {
        crc_accumulate(packet[i], &crc);
    }
    crc_accumulate(crc_extra, &crc);
    packet_length = (uint16_t)(MAVLINK2_HEADER_LEN + payload_length);
    packet[packet_length++] = (uint8_t)crc;
    packet[packet_length++] = (uint8_t)(crc >> 8);
    return CDC_Transmit_FS(packet, packet_length);
}

static void mavcan_send_heartbeat(void)
{
    uint8_t payload[MAVLINK_MSG_HEARTBEAT_LEN] = {0};

    /* MAV_TYPE_ONBOARD_CONTROLLER, MAV_AUTOPILOT_INVALID, active, MAVLink v2. */
    payload[4] = 18U;
    payload[5] = 8U;
    payload[6] = 0U;
    payload[7] = 4U;
    payload[8] = 3U;
    (void)mavlink_send(MAVLINK_MSG_ID_HEARTBEAT, payload,
        sizeof(payload), MAVLINK_MSG_HEARTBEAT_CRC);
}

static void parser_reset(void)
{
    parser.stage = PARSER_WAIT_MAGIC;
    parser.header_index = 0U;
    parser.payload_index = 0U;
    parser.signature_remaining = 0U;
}

static uint32_t parser_message_id(void)
{
    return (uint32_t)parser.header[6]
        | ((uint32_t)parser.header[7] << 8)
        | ((uint32_t)parser.header[8] << 16);
}

static void parser_dispatch(void)
{
    can_frame_t frame = {0};
    uint32_t raw_id;
    uint8_t payload_length = parser.header[0];

    if (parser_message_id() != MAVLINK_MSG_ID_CAN_FRAME
        || payload_length < 8U
        || payload_length > MAVLINK_MSG_CAN_FRAME_LEN) {
        return;
    }

    crc_accumulate(MAVLINK_MSG_CAN_FRAME_CRC, &parser.checksum);
    if (parser.checksum != parser.received_checksum) {
        return;
    }

    raw_id = get_le32(parser.payload);
    frame.id = raw_id & MAVCAN_CAN_ID_MASK;
    frame.is_extended = (raw_id & MAVCAN_CAN_ID_EXTENDED) != 0U;
    frame.is_remote = (raw_id & MAVCAN_CAN_ID_REMOTE) != 0U;
    frame.dlc = parser.payload[7];
    if (frame.dlc > 8U || (!frame.is_extended && frame.id > 0x7FFU)) {
        return;
    }
    memcpy(frame.data, &parser.payload[8], sizeof(frame.data));
    (void)can_tx(&frame);
}

static void parser_byte(uint8_t byte)
{
    switch (parser.stage) {
        case PARSER_WAIT_MAGIC:
            if (byte == MAVLINK2_MAGIC) {
                parser.checksum = 0xFFFFU;
                parser.header_index = 0U;
                parser.payload_index = 0U;
                memset(parser.payload, 0, sizeof(parser.payload));
                parser.stage = PARSER_HEADER;
            }
            break;

        case PARSER_HEADER:
            parser.header[parser.header_index++] = byte;
            crc_accumulate(byte, &parser.checksum);
            if (parser.header_index == MAVLINK2_CORE_HEADER_LEN) {
                parser.stage = parser.header[0] == 0U
                    ? PARSER_CHECKSUM_LOW : PARSER_PAYLOAD;
            }
            break;

        case PARSER_PAYLOAD:
            if (parser.payload_index < sizeof(parser.payload)) {
                parser.payload[parser.payload_index] = byte;
            }
            ++parser.payload_index;
            crc_accumulate(byte, &parser.checksum);
            if (parser.payload_index == parser.header[0]) {
                parser.stage = PARSER_CHECKSUM_LOW;
            }
            break;

        case PARSER_CHECKSUM_LOW:
            parser.received_checksum = byte;
            parser.stage = PARSER_CHECKSUM_HIGH;
            break;

        case PARSER_CHECKSUM_HIGH:
            parser.received_checksum |= (uint16_t)byte << 8;
            parser_dispatch();
            if ((parser.header[1] & MAVLINK2_SIGNED_FLAG) != 0U) {
                parser.signature_remaining = MAVLINK2_SIGNATURE_LEN;
                parser.stage = PARSER_SIGNATURE;
            } else {
                parser_reset();
            }
            break;

        case PARSER_SIGNATURE:
            if (--parser.signature_remaining == 0U) {
                parser_reset();
            }
            break;

        default:
            parser_reset();
            break;
    }
}

void mavcan_init(void)
{
    parser_reset();
    can_disable();
    can_set_bitrate(CAN_BITRATE_1000K);
    can_set_silent(0U);
    can_set_autoretransmit(1U);
    can_enable();
    mavcan_send_heartbeat();
    last_heartbeat_ms = system_millis();
}

void mavcan_process(void)
{
    uint32_t now = system_millis();
    if ((uint32_t)(now - last_heartbeat_ms) >= MAVCAN_HEARTBEAT_INTERVAL_MS) {
        mavcan_send_heartbeat();
        last_heartbeat_ms = now;
    }
}

void mavcan_receive(const uint8_t *buffer, uint16_t length)
{
    if (buffer == NULL) {
        return;
    }
    for (uint16_t i = 0U; i < length; ++i) {
        parser_byte(buffer[i]);
    }
}

uint8_t mavcan_send_frame(const can_frame_t *frame)
{
    uint8_t payload[MAVLINK_MSG_CAN_FRAME_LEN] = {0};
    uint32_t raw_id;

    if (frame == NULL || frame->dlc > 8U) {
        return USBD_FAIL;
    }

    raw_id = frame->id & MAVCAN_CAN_ID_MASK;
    if (frame->is_extended) {
        raw_id |= MAVCAN_CAN_ID_EXTENDED;
    }
    if (frame->is_remote) {
        raw_id |= MAVCAN_CAN_ID_REMOTE;
    }
    put_le32(payload, raw_id);
    payload[4] = 0U;
    payload[5] = 0U;
    payload[6] = MAVCAN_BUS_INDEX;
    payload[7] = frame->dlc;
    memcpy(&payload[8], frame->data, sizeof(frame->data));
    return mavlink_send(MAVLINK_MSG_ID_CAN_FRAME, payload,
        sizeof(payload), MAVLINK_MSG_CAN_FRAME_CRC);
}
