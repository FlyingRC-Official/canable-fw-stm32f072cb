#ifndef USBD_BOARD_H
#define USBD_BOARD_H

#include "apm32f0xx.h"
#include "apm32f0xx_usb.h"
#include "apm32f0xx_usb_device.h"

#define USBD_SUP_CLASS_MAX_NUM 1
#define USBD_SUP_INTERFACE_MAX_NUM 1
#define USBD_SUP_CONFIGURATION_MAX_NUM 1
#define USBD_SUP_STR_DESC_MAX_NUM 128
#define USBD_SUP_LPM 0
#define USBD_SUP_SELF_PWR 0

#define USBD_EP0_OUT_ADDR 0x00
#define USBD_EP0_OUT_PMA_SIZE 0x18
#define USBD_EP0_IN_ADDR 0x80
#define USBD_EP0_IN_PMA_SIZE 0x58
#define USBD_CDC_EP_OUT_ADDR 0x01
#define USBD_CDC_EP_OUT_PMA_SIZE 0x110
#define USBD_CDC_EP_IN_ADDR 0x81
#define USBD_CDC_EP_IN_PMA_SIZE 0xC0
#define USBD_CDC_EP_CMD_ADDR 0x82
#define USBD_CDC_EP_CMD_PMA_SIZE 0x100

#define USBD_DEBUG_LEVEL 0U
#define USBD_USR_LOG(...) do { } while (0)
#define USBD_USR_Debug(...) do { } while (0)

#endif
