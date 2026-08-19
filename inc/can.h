#ifndef CAN_H
#define CAN_H

#include <stdint.h>

typedef enum {
    CAN_BITRATE_10K = 0,
    CAN_BITRATE_20K,
    CAN_BITRATE_50K,
    CAN_BITRATE_100K,
    CAN_BITRATE_125K,
    CAN_BITRATE_250K,
    CAN_BITRATE_500K,
    CAN_BITRATE_750K,
    CAN_BITRATE_1000K,
    CAN_BITRATE_INVALID,
} can_bitrate_t;

typedef enum {
    CAN_STATUS_OK = 0,
    CAN_STATUS_BUSY,
    CAN_STATUS_ERROR,
} can_status_t;

/* Public CAN frame representation. No device-library types escape this API. */
typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t is_extended;
    uint8_t is_remote;
    uint8_t data[8];
} can_frame_t;

void can_init(void);
void can_enable(void);
void can_disable(void);
void can_set_bitrate(can_bitrate_t bitrate);
void can_set_silent(uint8_t silent);
void can_set_autoretransmit(uint8_t autoretransmit);
can_status_t can_tx(const can_frame_t *frame);
can_status_t can_rx(can_frame_t *frame);
void can_process(void);
uint8_t can_is_rx_pending(void);
void can_irq_handler(void);

#endif
