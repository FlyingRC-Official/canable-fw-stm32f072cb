#include <string.h>
#include "apm32f0xx.h"
#include "system.h"
#include "usbd_board.h"
#include "usbd_cdc.h"
#include "usbd_desc.h"

#define USB_VID 0xAD50U
#define USB_PID 0x60C4U
#define USB_UID_BASE 0x1FFFF7ACUL
#define USB_CONFIG_DESC_SIZE 67U

static uint8_t device_desc[] = {
    18U, USBD_DESC_DEVICE, 0x00U, 0x02U, 0x02U, 0x02U, 0x00U,
    USBD_EP0_PACKET_MAX_SIZE,
    (uint8_t)USB_VID, (uint8_t)(USB_VID >> 8),
    (uint8_t)USB_PID, (uint8_t)(USB_PID >> 8),
    0x00U, 0x02U, USBD_DESC_STR_MFC, USBD_DESC_STR_PRODUCT,
    USBD_DESC_STR_SERIAL, 0x01U
};

static uint8_t config_desc[USB_CONFIG_DESC_SIZE] = {
    0x09U, USBD_DESC_CONFIGURATION, USB_CONFIG_DESC_SIZE, 0x00U,
    0x02U, 0x01U, 0x00U, 0x80U, 0x32U,
    0x09U, USBD_DESC_INTERFACE, 0x00U, 0x00U, 0x01U,
    0x02U, 0x02U, 0x01U, 0x00U,
    0x05U, 0x24U, 0x00U, 0x10U, 0x01U,
    0x05U, 0x24U, 0x01U, 0x00U, 0x01U,
    0x04U, 0x24U, 0x02U, 0x02U,
    0x05U, 0x24U, 0x06U, 0x00U, 0x01U,
    0x07U, USBD_DESC_ENDPOINT, USBD_CDC_CMD_EP_ADDR, 0x03U,
    USBD_CDC_CMD_MP_SIZE, 0x00U, USBD_CDC_FS_INTERVAL,
    0x09U, USBD_DESC_INTERFACE, 0x01U, 0x00U, 0x02U,
    0x0AU, 0x00U, 0x00U, 0x00U,
    0x07U, USBD_DESC_ENDPOINT, USBD_CDC_DATA_OUT_EP_ADDR, 0x02U,
    USBD_CDC_FS_MP_SIZE, 0x00U, 0x00U,
    0x07U, USBD_DESC_ENDPOINT, USBD_CDC_DATA_IN_EP_ADDR, 0x02U,
    USBD_CDC_FS_MP_SIZE, 0x00U, 0x00U
};

static uint8_t other_speed_desc[USB_CONFIG_DESC_SIZE];
static uint8_t qualifier_desc[] = {
    10U, USBD_DESC_DEVICE_QUALIFIER, 0x00U, 0x02U,
    0x02U, 0x02U, 0x00U, USBD_EP0_PACKET_MAX_SIZE, 0x01U, 0x00U
};
static uint8_t lang_desc[] = { 4U, USBD_DESC_STRING, 0x09U, 0x04U };
static uint8_t string_desc[USBD_SUP_STR_DESC_MAX_NUM];
static uint8_t serial_ascii[25];

static USBD_DESC_INFO_T desc_info(uint8_t *data, uint8_t size)
{
    USBD_DESC_INFO_T info = { data, size };
    return info;
}

static USBD_DESC_INFO_T ascii_string(const char *text)
{
    size_t chars = strlen(text);
    if (chars > (sizeof(string_desc) - 2U) / 2U) chars = (sizeof(string_desc) - 2U) / 2U;
    string_desc[0] = (uint8_t)(chars * 2U + 2U);
    string_desc[1] = USBD_DESC_STRING;
    for (size_t i = 0U; i < chars; ++i) {
        string_desc[2U + i * 2U] = (uint8_t)text[i];
        string_desc[3U + i * 2U] = 0U;
    }
    return desc_info(string_desc, string_desc[0]);
}

static USBD_DESC_INFO_T device_handler(uint8_t speed) { (void)speed; return desc_info(device_desc, sizeof(device_desc)); }
static USBD_DESC_INFO_T config_handler(uint8_t speed) { (void)speed; return desc_info(config_desc, sizeof(config_desc)); }
static USBD_DESC_INFO_T config_string_handler(uint8_t speed) { (void)speed; return ascii_string("CDC Config"); }
static USBD_DESC_INFO_T interface_string_handler(uint8_t speed) { (void)speed; return ascii_string("CDC Interface"); }
static USBD_DESC_INFO_T lang_handler(uint8_t speed) { (void)speed; return desc_info(lang_desc, sizeof(lang_desc)); }
static USBD_DESC_INFO_T manufacturer_handler(uint8_t speed) { (void)speed; return ascii_string("Protofusion Labs"); }
static USBD_DESC_INFO_T product_handler(uint8_t speed) { (void)speed; return ascii_string("CANable " GIT_VERSION " " GIT_REMOTE); }

static USBD_DESC_INFO_T serial_handler(uint8_t speed)
{
    (void)speed;
    system_hex32((char *)&serial_ascii[0], *(const uint32_t *)(USB_UID_BASE + 0U));
    system_hex32((char *)&serial_ascii[8], *(const uint32_t *)(USB_UID_BASE + 4U));
    system_hex32((char *)&serial_ascii[16], *(const uint32_t *)(USB_UID_BASE + 8U));
    serial_ascii[24] = '\0';
    return ascii_string((const char *)serial_ascii);
}

static USBD_DESC_INFO_T other_speed_handler(uint8_t speed)
{
    (void)speed;
    memcpy(other_speed_desc, config_desc, sizeof(config_desc));
    other_speed_desc[1] = USBD_DESC_OTHER_SPEED;
    return desc_info(other_speed_desc, sizeof(other_speed_desc));
}

static USBD_DESC_INFO_T qualifier_handler(uint8_t speed) { (void)speed; return desc_info(qualifier_desc, sizeof(qualifier_desc)); }

USBD_DESC_T USBD_DESC_FS = {
    "CANable CDC descriptor",
    device_handler,
    config_handler,
    config_string_handler,
    interface_string_handler,
    lang_handler,
    manufacturer_handler,
    product_handler,
    serial_handler,
    NULL,
    other_speed_handler,
    qualifier_handler,
};
