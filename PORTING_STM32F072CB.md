# STM32F072CB SLCAN Port Notes

This is a first-pass SLCAN port for the schematic provided on 2026-05-08.

## Target

- MCU: STM32F072CBT6
- Flash: 128 KiB
- RAM: 16 KiB
- USB FS: PA11/PA12
- CAN: PB8/PB9, AF4
- CAN transceiver: TJA1051T/3
- CAN transceiver S pin: PC13, active high for silent/standby, held low for normal mode
- LEDs: PA0/PA1/PA2, active low
- Clock: firmware currently uses internal HSI48 with USB CRS, matching the original CANable SLCAN firmware style. The schematic's 8 MHz HSE is not required by this build.

## Changed Files

- `Makefile`: target device changed to `STM32F072xB`, linker/startup switched to F072CB, HSE value set to 8 MHz.
- `STM32F072CB_FLASH.ld`: flash/RAM sizes changed to 128 KiB/16 KiB.
- `src/startup_stm32f072xb.s`: copied from STM32Cube F0 CMSIS templates.
- `src/can.c`: keeps CAN on PB8/PB9 and drives PC13 low for normal transceiver mode.
- `inc/led.h`, `src/led.c`: moved LEDs to PA0/PA1/PA2 and made them active low.

## Build

Install GNU Make and Arm GNU Toolchain, then from this directory:

```sh
make clean
make
```

Expected outputs:

```text
build/stm32f072cb-slcan-f072cb-port.bin
build/stm32f072cb-slcan-f072cb-port.hex
```

If building from a Git checkout, the version suffix will use `git describe` instead of `f072cb-port`.

## Flash

Put the board into STM32 ROM DFU mode with BOOT0 high, then flash from the build directory:

```sh
dfu-util -d 0483:df11 -a 0 -s 0x08000000:leave -D build/stm32f072cb-slcan-f072cb-port.bin
```

After flashing, return BOOT0 low and reconnect USB. Windows should enumerate the firmware as a CDC serial device. On older Windows installs, use the included `windows-driver/cantact.inf` for VID `AD50`, PID `60C4`.

## DroneCAN GUI

- Interface: SLCAN
- Port: the new COM port
- Bitrate: 1000000 for most DroneCAN devices

Wire CAN_H, CAN_L, and GND, and make sure the DroneCAN node is externally powered if your adapter does not provide bus power.

