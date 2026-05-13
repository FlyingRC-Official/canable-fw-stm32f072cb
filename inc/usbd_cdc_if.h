#ifndef __USBD_CDC_IF_H__
#define __USBD_CDC_IF_H__

#include "usbd_cdc.h"

// Buffer settings
#define TX_BUF_SIZE  64 // Linear TX buf size
#define NUM_TX_BUFS 32 // Number of TX buffers in FIFO
#define NUM_RX_BUFS 6 // Number of RX buffers in FIFO
#define RX_BUF_SIZE CDC_DATA_FS_MAX_PACKET_SIZE // Size of RX buffer item

// Transmit buffering: circular buffer FIFO
typedef struct _usbtx_buf_
{
	uint8_t buf[NUM_TX_BUFS][TX_BUF_SIZE];
	uint16_t msglen[NUM_TX_BUFS];
	uint8_t head;
	uint8_t tail;
	uint8_t active;

} usbtx_buf_t;

// Receive buffering: circular buffer FIFO
typedef struct _usbrx_buf_
{
	// Receive buffering: circular buffer FIFO
	uint8_t buf[NUM_RX_BUFS][RX_BUF_SIZE];
	uint32_t msglen[NUM_RX_BUFS];
	uint8_t head;
	uint8_t tail;

} usbrx_buf_t;


// CDC Interface callback.
extern USBD_CDC_ItfTypeDef USBD_Interface_fops_FS;


// Prototypes
uint8_t CDC_Transmit_FS(uint8_t* Buf, uint16_t Len);
void cdc_process(void);
void cdc_tx_process(void);



#endif /* __USBD_CDC_IF_H__ */

