#ifndef USB_DEVICE_H
#define USB_DEVICE_H

#include "usbd_core.h"

extern USBD_INFO_T gUsbDeviceFS;

void usb_init(void);
void usb_irq_handler(void);

#endif
