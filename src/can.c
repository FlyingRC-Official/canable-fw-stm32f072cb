#include <string.h>
#include "apm32f0xx.h"
#include "apm32f0xx_can.h"
#include "apm32f0xx_gpio.h"
#include "apm32f0xx_misc.h"
#include "apm32f0xx_rcm.h"
#include "can.h"
#include "led.h"
#include "error.h"

#define CAN_S_PIN GPIO_PIN_13
#define TXQUEUE_LEN 28U

typedef enum { OFF_BUS = 0, ON_BUS = 1 } can_bus_state_t;
typedef struct {
    can_frame_t frame[TXQUEUE_LEN];
    uint8_t head;
    uint8_t tail;
} can_tx_queue_t;

static uint16_t prescaler = 48U;
static can_bus_state_t bus_state = OFF_BUS;
static uint8_t can_silent;
static uint8_t can_autoretransmit = ENABLE;
static can_tx_queue_t txqueue;

void can_init(void)
{
    GPIO_Config_T gpio;

    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_CAN);
    RCM_EnableAHBPeriphClock(RCM_AHB_PERIPH_GPIOB | RCM_AHB_PERIPH_GPIOC);

    GPIO_ConfigStructInit(&gpio);
    gpio.pin = CAN_S_PIN;
    gpio.mode = GPIO_MODE_OUT;
    gpio.outtype = GPIO_OUT_TYPE_PP;
    gpio.speed = GPIO_SPEED_2MHz;
    gpio.pupd = GPIO_PUPD_NO;
    GPIO_Config(GPIOC, &gpio);
    GPIO_ClearBit(GPIOC, CAN_S_PIN);

    gpio.pin = GPIO_PIN_8 | GPIO_PIN_9;
    gpio.mode = GPIO_MODE_AF;
    gpio.speed = GPIO_SPEED_50MHz;
    GPIO_Config(GPIOB, &gpio);
    GPIO_ConfigPinAF(GPIOB, GPIO_PIN_SOURCE_8, GPIO_AF_PIN4);
    GPIO_ConfigPinAF(GPIOB, GPIO_PIN_SOURCE_9, GPIO_AF_PIN4);

    NVIC_EnableIRQRequest(CEC_CAN_IRQn, 1U);
}

void can_enable(void)
{
    CAN_Config_T config;
    CAN_FilterConfig_T filter;

    if (bus_state == ON_BUS) {
        return;
    }

    CAN_Reset();
    CAN_ConfigStructInit(&config);
    config.prescaler = prescaler;
    config.mode = can_silent ? CAN_MODE_SILENT : CAN_MODE_NORMAL;
    config.syncJumpWidth = CAN_SJW_1;
    config.timeSegment1 = CAN_TIME_SEGMENT1_4;
    config.timeSegment2 = CAN_TIME_SEGMENT2_3;
    config.timeTrigComMode = DISABLE;
    config.autoBusOffManage = ENABLE;
    config.autoWakeUpMode = DISABLE;
    config.nonAutoRetran = can_autoretransmit ? DISABLE : ENABLE;
    config.rxFIFOLockMode = DISABLE;
    config.txFIFOPriority = ENABLE;
    if (CAN_Config(&config) == ERROR) {
        error_assert(ERR_CAN_TXFAIL);
        return;
    }

    memset(&filter, 0, sizeof(filter));
    filter.filterFIFO = CAN_FIFO_0;
    filter.filterNumber = CAN_FILTER_NUMBER_0;
    filter.filterMode = CAN_FILTER_MODE_IDMASK;
    filter.filterScale = CAN_FILTER_SCALE_32BIT;
    filter.filterActivation = ENABLE;
    CAN_ConfigFilter(&filter);
    CAN_EnableInterrupt(CAN_INT_F0OVR);
    bus_state = ON_BUS;
    led_blue_on();
}

void can_disable(void)
{
    if (bus_state == ON_BUS) {
        CAN_Reset();
        bus_state = OFF_BUS;
        led_green_on();
    }
}

void can_set_bitrate(can_bitrate_t bitrate)
{
    static const uint16_t dividers[CAN_BITRATE_INVALID] = {
        600U, 300U, 120U, 60U, 48U, 24U, 12U, 8U, 6U
    };
    if (bus_state == OFF_BUS && bitrate < CAN_BITRATE_INVALID) {
        prescaler = dividers[bitrate];
        led_green_on();
    }
}

void can_set_silent(uint8_t silent)
{
    if (bus_state == OFF_BUS) {
        can_silent = silent ? ENABLE : DISABLE;
        GPIO_WriteBitValue(GPIOC, CAN_S_PIN, silent ? Bit_SET : Bit_RESET);
        led_green_on();
    }
}

void can_set_autoretransmit(uint8_t autoretransmit)
{
    if (bus_state == OFF_BUS) {
        can_autoretransmit = autoretransmit ? ENABLE : DISABLE;
        led_green_on();
    }
}

can_status_t can_tx(const can_frame_t *frame)
{
    uint8_t next = (uint8_t)((txqueue.head + 1U) % TXQUEUE_LEN);
    if (frame == NULL || frame->dlc > 8U) {
        return CAN_STATUS_ERROR;
    }
    if (next == txqueue.tail) {
        error_assert(ERR_FULLBUF_CANTX);
        return CAN_STATUS_BUSY;
    }
    txqueue.frame[txqueue.head] = *frame;
    txqueue.head = next;
    return CAN_STATUS_OK;
}

void can_process(void)
{
    CAN_Tx_Message tx = {0};
    can_frame_t *frame;

    if (bus_state == OFF_BUS || txqueue.tail == txqueue.head) {
        return;
    }

    frame = &txqueue.frame[txqueue.tail];
    tx.typeID = frame->is_extended ? CAN_TYPEID_EXT : CAN_TYPEID_STD;
    tx.remoteTxReq = frame->is_remote ? CAN_RTXR_REMOTE : CAN_RTXR_DATA;
    tx.stanID = frame->id;
    tx.extenID = frame->id;
    tx.dataLengthCode = frame->dlc;
    memcpy(tx.data, frame->data, sizeof(tx.data));

    if (CAN_TxMessage(&tx) != CAN_TX_MAILBOX_FULL) {
        txqueue.tail = (uint8_t)((txqueue.tail + 1U) % TXQUEUE_LEN);
        led_green_on();
    }
}

can_status_t can_rx(can_frame_t *frame)
{
    CAN_Rx_Message rx;
    if (frame == NULL || !can_is_rx_pending()) {
        return CAN_STATUS_ERROR;
    }
    CAN_RxMessage(CAN_FIFO_0, &rx);
    frame->id = (rx.typeID == CAN_TYPEID_EXT) ? rx.extenID : rx.stanID;
    frame->dlc = rx.dataLengthCode;
    frame->is_extended = (rx.typeID == CAN_TYPEID_EXT);
    frame->is_remote = (rx.remoteTxReq == CAN_RTXR_REMOTE);
    memcpy(frame->data, rx.data, sizeof(frame->data));
    led_blue_on();
    return CAN_STATUS_OK;
}

uint8_t can_is_rx_pending(void)
{
    return (bus_state == ON_BUS) && (CAN_PendingMessage(CAN_FIFO_0) != 0U);
}

void can_irq_handler(void)
{
    if (CAN_ReadIntFlag(CAN_INT_F0OVR)) {
        CAN_ClearIntFlag(CAN_INT_F0OVR);
        error_assert(ERR_CANRXFIFO_OVERFLOW);
    }
}
