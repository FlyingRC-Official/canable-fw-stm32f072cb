#include "usb_device.h"
#include "usbd_cdc_if.h"
#include "can.h"
#ifdef MAVCAN_BRIDGE
#include "mavcan.h"
#else
#include "slcan.h"
#endif
#include "system.h"
#include "led.h"

int main(void)
{
    can_frame_t frame;
#ifndef MAVCAN_BRIDGE
    uint8_t msg_buf[SLCAN_MTU];
#endif

    system_init();
    can_init();
    led_init();
    usb_init();
    led_blue_blink(2);
#ifdef MAVCAN_BRIDGE
    mavcan_init();
#endif

    for (;;) {
        cdc_process();
        cdc_tx_process();
        led_process();
        can_process();
#ifdef MAVCAN_BRIDGE
        mavcan_process();
#endif

        if (can_is_rx_pending() && can_rx(&frame) == CAN_STATUS_OK) {
#ifdef MAVCAN_BRIDGE
            (void)mavcan_send_frame(&frame);
#else
            int8_t msg_len = slcan_parse_frame(msg_buf, &frame);
            if (msg_len > 0) {
                (void)CDC_Transmit_FS(msg_buf, (uint16_t)msg_len);
            }
#endif
        }
    }
}
