#ifndef SLCAN_H
#define SLCAN_H

#include <stdint.h>
#include "can.h"

#define SLCAN_MTU 30
#define SLCAN_STD_ID_LEN 3
#define SLCAN_EXT_ID_LEN 8

int8_t slcan_parse_frame(uint8_t *buf, const can_frame_t *frame);
int8_t slcan_parse_str(uint8_t *buf, uint8_t len);

#endif
