#include "can.h"
#include "system.h"
#include "usb_device.h"

void NMI_Handler(void) { for (;;) { } }
void HardFault_Handler(void) { for (;;) { } }
void USBD_IRQHandler(void) { usb_irq_handler(); }
void CEC_CAN_IRQHandler(void) { can_irq_handler(); }
void SysTick_Handler(void) { system_tick_isr(); }
