#ifndef USBD_CDC_IF_H
#define USBD_CDC_IF_H

#include <stdint.h>
#include "usbd_cdc.h"

#define TX_BUF_SIZE 64U
#define NUM_TX_BUFS 32U
#define NUM_RX_BUFS 6U
#define RX_BUF_SIZE USBD_CDC_FS_MP_SIZE

typedef struct {
    uint8_t buf[NUM_TX_BUFS][TX_BUF_SIZE];
    uint16_t msglen[NUM_TX_BUFS];
    volatile uint8_t head;
    volatile uint8_t tail;
    volatile uint8_t active;
} usbtx_buf_t;

typedef struct {
    uint8_t buf[NUM_RX_BUFS][RX_BUF_SIZE];
    uint32_t msglen[NUM_RX_BUFS];
    volatile uint8_t head;
    volatile uint8_t tail;
} usbrx_buf_t;

extern USBD_CDC_INTERFACE_T USBD_CDC_INTERFACE_FS;

uint8_t CDC_Transmit_FS(uint8_t *buf, uint16_t len);
void cdc_process(void);
void cdc_tx_process(void);

#endif
