#include "apm32f0xx_crs.h"
#include "apm32f0xx_misc.h"
#include "apm32f0xx_rcm.h"
#include "apm32f0xx_usb_device.h"
#include "usb_device.h"
#include "usbd_board.h"
#include "usbd_cdc.h"
#include "usbd_cdc_if.h"
#include "usbd_desc.h"

USBD_INFO_T gUsbDeviceFS;
static USBD_HANDLE_T usb_device_handler;

static void usb_user_callback(USBD_INFO_T *usb_info, uint8_t status)
{
    (void)usb_info;
    (void)status;
}

void usb_init(void)
{
    (void)USBD_CDC_RegisterItf(&gUsbDeviceFS, &USBD_CDC_INTERFACE_FS);
    (void)USBD_Init(&gUsbDeviceFS, USBD_SPEED_FS, &USBD_DESC_FS,
                    &USBD_CDC_CLASS, usb_user_callback);
}

void usb_irq_handler(void) { USBD_IsrHandler(&usb_device_handler); }

void USBD_HardwareInit(USBD_INFO_T *usb_info)
{
    RCM_EnableHSI48();
    RCM_ConfigUSBCLK(RCM_USBCLK_HSI48);
    RCM_EnableAPB1PeriphClock(RCM_APB1_PERIPH_USB | RCM_APB1_PERIPH_CRS);
    CRS_ConfigSynchronizationSource(CRS_SYNC_SOURCE_USB);
    CRS_EnableAutomaticCalibration();
    CRS_EnableFrequencyErrorCounter();

    usb_device_handler.usbGlobal = USBD;
    usb_device_handler.dataPoint = usb_info;
    usb_info->dataPoint = &usb_device_handler;
    usb_device_handler.usbCfg.sofStatus = DISABLE;
    usb_device_handler.usbCfg.speed = USB_SPEED_FSLS;
    usb_device_handler.usbCfg.devEndpointNum = 8U;
    usb_device_handler.usbCfg.lowPowerStatus = DISABLE;
    usb_device_handler.usbCfg.lpmStatus = DISABLE;
    usb_device_handler.usbCfg.batteryStatus = DISABLE;

    NVIC_EnableIRQRequest(USBD_IRQn, 1U);
    USBD_DisableInterrupt(USBD, USBD_INT_CTR | USBD_INT_WKUP | USBD_INT_SUS |
        USBD_INT_ERR | USBD_INT_RST | USBD_INT_SOF | USBD_INT_ESOF | USBD_INT_L1REQ);
    USBD_Config(&usb_device_handler);
    USBD_ConfigPMA(&usb_device_handler, USBD_EP0_OUT_ADDR, USBD_EP_BUFFER_SINGLE, USBD_EP0_OUT_PMA_SIZE);
    USBD_ConfigPMA(&usb_device_handler, USBD_EP0_IN_ADDR, USBD_EP_BUFFER_SINGLE, USBD_EP0_IN_PMA_SIZE);
    USBD_ConfigPMA(&usb_device_handler, USBD_CDC_EP_OUT_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_OUT_PMA_SIZE);
    USBD_ConfigPMA(&usb_device_handler, USBD_CDC_EP_IN_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_IN_PMA_SIZE);
    USBD_ConfigPMA(&usb_device_handler, USBD_CDC_EP_CMD_ADDR, USBD_EP_BUFFER_SINGLE, USBD_CDC_EP_CMD_PMA_SIZE);
    USBD_Start(usb_info->dataPoint);
}

void USBD_HardwareReset(USBD_INFO_T *usb_info)
{
    (void)usb_info;
    NVIC_DisableIRQRequest(USBD_IRQn);
    RCM_DisableAPB1PeriphClock(RCM_APB1_PERIPH_USB);
}

void USBD_StartCallback(USBD_INFO_T *i) { USBD_Start(i->dataPoint); }
void USBD_StopCallback(USBD_INFO_T *i) { USBD_Stop(i->dataPoint); }
void USBD_StopDeviceCallback(USBD_INFO_T *i) { USBD_StopDevice(i->dataPoint); }
void USBD_ResumeCallback(USBD_HANDLE_T *h) { (void)USBD_Resume(h->dataPoint); }
void USBD_SuspendCallback(USBD_HANDLE_T *h) { (void)USBD_Suspend(h->dataPoint); }
void USBD_EnumDoneCallback(USBD_HANDLE_T *h)
{
    (void)USBD_SetSpeed(h->dataPoint, USBD_DEVICE_SPEED_FS);
    (void)USBD_Reset(h->dataPoint);
}
void USBD_SetupStageCallback(USBD_HANDLE_T *h) { (void)USBD_SetupStage(h->dataPoint, (uint8_t *)h->setup); }
void USBD_DataOutStageCallback(USBD_HANDLE_T *h, uint8_t ep) { (void)USBD_DataOutStage(h->dataPoint, ep, h->epOUT[ep].buffer); }
void USBD_DataInStageCallback(USBD_HANDLE_T *h, uint8_t ep) { (void)USBD_DataInStage(h->dataPoint, ep, h->epIN[ep].buffer); }
void USBD_SOFCallback(USBD_HANDLE_T *h) { (void)USBD_HandleSOF(h->dataPoint); }
void USBD_IsoInInCompleteCallback(USBD_HANDLE_T *h, uint8_t ep) { (void)USBD_IsoInInComplete(h->dataPoint, ep); }
void USBD_IsoOutInCompleteCallback(USBD_HANDLE_T *h, uint8_t ep) { (void)USBD_IsoOutInComplete(h->dataPoint, ep); }
void USBD_ConnectCallback(USBD_HANDLE_T *h) { (void)USBD_Connect(h->dataPoint); }
void USBD_DisconnectCallback(USBD_HANDLE_T *h) { (void)USBD_Disconnect(h->dataPoint); }

void USBD_EP_OpenCallback(USBD_INFO_T *i, uint8_t ep, uint8_t type, uint16_t mps) { USBD_EP_Open(i->dataPoint, ep, type, mps); }
void USBD_EP_CloseCallback(USBD_INFO_T *i, uint8_t ep) { USBD_EP_Close(i->dataPoint, ep); }
USBD_STA_T USBD_EP_StallCallback(USBD_INFO_T *i, uint8_t ep) { USBD_EP_Stall(i->dataPoint, ep); return USBD_OK; }
USBD_STA_T USBD_EP_ClearStallCallback(USBD_INFO_T *i, uint8_t ep) { USBD_EP_ClearStall(i->dataPoint, ep); return USBD_OK; }
uint8_t USBD_EP_ReadStallStatusCallback(USBD_INFO_T *i, uint8_t ep) { return USBD_EP_ReadStallStatus(i->dataPoint, ep); }
uint32_t USBD_EP_ReadRxDataLenCallback(USBD_INFO_T *i, uint8_t ep) { return USBD_EP_ReadRxDataLen(i->dataPoint, ep); }
USBD_STA_T USBD_EP_ReceiveCallback(USBD_INFO_T *i, uint8_t ep, uint8_t *buf, uint32_t len) { USBD_EP_Receive(i->dataPoint, ep, buf, len); return USBD_OK; }
USBD_STA_T USBD_EP_TransferCallback(USBD_INFO_T *i, uint8_t ep, uint8_t *buf, uint32_t len) { USBD_EP_Transfer(i->dataPoint, ep, buf, len); return USBD_OK; }
USBD_STA_T USBD_EP_FlushCallback(USBD_INFO_T *i, uint8_t ep) { USBD_EP_Flush(i->dataPoint, ep); return USBD_OK; }
USBD_STA_T USBD_SetDevAddressCallback(USBD_INFO_T *i, uint8_t addr) { USBD_SetDevAddress(i->dataPoint, addr); return USBD_OK; }
