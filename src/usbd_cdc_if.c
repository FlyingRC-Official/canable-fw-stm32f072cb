#include <string.h>
#include "usb_device.h"
#include "usbd_cdc_if.h"
#ifdef MAVCAN_BRIDGE
#include "mavcan.h"
#else
#include "slcan.h"
#endif
#include "system.h"
#include "error.h"

static usbrx_buf_t rxbuf;
static usbtx_buf_t txbuf;
#ifndef MAVCAN_BRIDGE
static uint8_t slcan_str[SLCAN_MTU];
static uint8_t slcan_str_index;
#endif

static USBD_STA_T cdc_init(void);
static USBD_STA_T cdc_deinit(void);
static USBD_STA_T cdc_control(uint8_t command, uint8_t *buffer, uint16_t length);
static USBD_STA_T cdc_send(uint8_t *buffer, uint16_t length);
static USBD_STA_T cdc_send_end(uint8_t ep, uint8_t *buffer, uint32_t *length);
static USBD_STA_T cdc_receive(uint8_t *buffer, uint32_t *length);

USBD_CDC_INTERFACE_T USBD_CDC_INTERFACE_FS = {
    "CANable CDC FS",
    cdc_init,
    cdc_deinit,
    cdc_control,
    cdc_send,
    cdc_send_end,
    cdc_receive,
};

static USBD_STA_T cdc_init(void)
{
    (void)USBD_CDC_ConfigRxBuffer(&gUsbDeviceFS, rxbuf.buf[rxbuf.head]);
    (void)USBD_CDC_ConfigTxBuffer(&gUsbDeviceFS, txbuf.buf[txbuf.tail], 0U);
    return USBD_OK;
}

static USBD_STA_T cdc_deinit(void) { return USBD_OK; }

static USBD_STA_T cdc_control(uint8_t command, uint8_t *buffer, uint16_t length)
{
    (void)length;
    if (command == USBD_CDC_GET_LINE_CODING && buffer != NULL) {
        buffer[0] = 0x00U;
        buffer[1] = 0xC2U;
        buffer[2] = 0x01U;
        buffer[3] = 0x00U;
        buffer[4] = 0U;
        buffer[5] = 0U;
        buffer[6] = 8U;
    }
    return USBD_OK;
}

static USBD_STA_T cdc_send(uint8_t *buffer, uint16_t length)
{
    USBD_CDC_INFO_T *cdc = (USBD_CDC_INFO_T *)gUsbDeviceFS.devClass[gUsbDeviceFS.classID]->classData;
    if (cdc == NULL || cdc->cdcTx.state != USBD_CDC_XFER_IDLE) return USBD_BUSY;
    (void)USBD_CDC_ConfigTxBuffer(&gUsbDeviceFS, buffer, length);
    return USBD_CDC_TxPacket(&gUsbDeviceFS);
}

static USBD_STA_T cdc_send_end(uint8_t ep, uint8_t *buffer, uint32_t *length)
{
    (void)ep;
    (void)buffer;
    (void)length;
    if (txbuf.active) {
        txbuf.tail = (uint8_t)((txbuf.tail + 1U) % NUM_TX_BUFS);
        txbuf.active = 0U;
    }
    return USBD_OK;
}

static USBD_STA_T cdc_receive(uint8_t *buffer, uint32_t *length)
{
    uint8_t next = (uint8_t)((rxbuf.head + 1U) % NUM_RX_BUFS);
    (void)buffer;
    if (next == rxbuf.tail) {
        error_assert(ERR_FULLBUF_USBRX);
    } else {
        rxbuf.msglen[rxbuf.head] = *length;
        rxbuf.head = next;
    }
    (void)USBD_CDC_ConfigRxBuffer(&gUsbDeviceFS, rxbuf.buf[rxbuf.head]);
    (void)USBD_CDC_RxPacket(&gUsbDeviceFS);
    return USBD_OK;
}

void cdc_process(void)
{
    uint32_t irq_state = system_irq_save();
    if (rxbuf.tail != rxbuf.head) {
        uint8_t tail = rxbuf.tail;
#ifdef MAVCAN_BRIDGE
        mavcan_receive(rxbuf.buf[tail], (uint16_t)rxbuf.msglen[tail]);
#else
        for (uint32_t i = 0U; i < rxbuf.msglen[tail]; ++i) {
            uint8_t c = rxbuf.buf[tail][i];
            if (c == '\r') {
                uint8_t reply = slcan_parse_str(slcan_str, slcan_str_index) == 0 ? '\r' : '\a';
                (void)CDC_Transmit_FS(&reply, 1U);
                slcan_str_index = 0U;
            } else if (slcan_str_index < SLCAN_MTU) {
                slcan_str[slcan_str_index++] = c;
            } else {
                slcan_str_index = 0U;
            }
        }
#endif
        rxbuf.tail = (uint8_t)((tail + 1U) % NUM_RX_BUFS);
    }
    system_irq_restore(irq_state);
    cdc_tx_process();
}

uint8_t CDC_Transmit_FS(uint8_t *buffer, uint16_t length)
{
    uint8_t next;
    uint32_t irq_state;
    if (buffer == NULL || length > TX_BUF_SIZE) return USBD_FAIL;

    irq_state = system_irq_save();
    next = (uint8_t)((txbuf.head + 1U) % NUM_TX_BUFS);
    if (next == txbuf.tail) {
        system_irq_restore(irq_state);
        error_assert(ERR_USBTX_BUSY);
        return USBD_BUSY;
    }
    memcpy(txbuf.buf[txbuf.head], buffer, length);
    txbuf.msglen[txbuf.head] = length;
    txbuf.head = next;
    system_irq_restore(irq_state);
    cdc_tx_process();
    return USBD_OK;
}

void cdc_tx_process(void)
{
    uint32_t irq_state = system_irq_save();
    if (!txbuf.active && txbuf.tail != txbuf.head) {
        if (cdc_send(txbuf.buf[txbuf.tail], txbuf.msglen[txbuf.tail]) == USBD_OK) {
            txbuf.active = 1U;
        }
    }
    system_irq_restore(irq_state);
}
