# APM32F072CBT6 port notes

## Scope

The APM32F072CBT6 is treated as a pin-compatible replacement for the original
STM32F072CBT6. The port uses HSI48 and does not enable the board's 8 MHz HSE.
The USB VID/PID, strings, 96-bit UID serial number and SLCAN command set are
preserved.

## Platform changes

- `startup_apm32f072.S` supplies `.apm32_isr_vector` and the Geehy interrupt
  names `USBD_IRQHandler` and `CEC_CAN_IRQHandler`.
- `system.c` owns the 1 ms SysTick and platform-neutral millisecond, delay and
  critical-section functions.
- `can_frame_t` is the only CAN frame type visible to the application/SLCAN
  layer. APM32 `CAN_Tx_Message` and `CAN_Rx_Message` remain private to `can.c`.
- The USB CDC callbacks feed the existing six-packet RX and 32-packet TX ring
  buffers. The TX tail advances only from the Geehy send-complete callback.
- The CDC descriptor is bus-powered, uses `AD50:60C4`, and formats the three
  32-bit UID words at the documented `0x1FFFF7AC` base as 24 hexadecimal
  characters.

## Vendor source boundary

`vendor/geehy/` contains the minimum headers plus the device startup/system,
RCM, CRS, FMC, GPIO, CAN, USB device peripheral sources and USB CDC core/class
needed by the build. See `vendor/geehy/README.md` and the preserved Geehy
copyright notice.

## Memory

- Flash: `0x08000000`, 128 KiB.
- RAM: `0x20000000`, 16 KiB.
- Link-time heap: 1 KiB (the Geehy CDC class allocates its class context).
- Reserved stack: 2 KiB.

The linker assertion fails the build if static RAM, heap and stack exceed the
16 KiB region.

## Validation commands

```sh
make clean
make VERSION=APM32F072
make size VERSION=APM32F072
arm-none-eabi-readelf -S build/apm32f072cb-slcan-APM32F072.elf
arm-none-eabi-nm -u build/apm32f072cb-slcan-APM32F072.elf
```

Board-level USB enumeration and CAN traffic tests are intentionally separate
from this compile-and-flash acceptance step.
