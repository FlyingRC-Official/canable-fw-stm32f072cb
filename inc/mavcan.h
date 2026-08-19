#ifndef MAVCAN_H
#define MAVCAN_H

#include <stdint.h>
#include "can.h"

/* MAVLink 2 bridge used by DroneCAN Web Tools. */
void mavcan_init(void);
void mavcan_process(void);
void mavcan_receive(const uint8_t *buffer, uint16_t length);
uint8_t mavcan_send_frame(const can_frame_t *frame);

#endif
