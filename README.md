# CANable SLCAN/MAVCAN firmware for APM32F072CBT6

This branch ports the CANable SLCAN firmware to the Geehy APM32F072CBT6. It
keeps the original CANable PCB pinout, USB identity (`AD50:60C4`) and serial
protocol while replacing the STM32 HAL and USB stack with the Geehy APM32F0xx
standard peripheral library and USB Device CDC middleware. A separate MAVCAN
build provides direct compatibility with DroneCAN Web Tools without injecting
binary MAVLink traffic into the SLCAN serial stream.

## Hardware configuration

- Core clock: 48 MHz from HSI48; USB HSI48 is trimmed from USB SOF by CRS.
- USB: PA11/PA12.
- CAN: PB8/PB9, alternate function 4, 8 time quanta per bit.
- CAN transceiver S input: PC13, low for normal high-speed operation.
- LEDs: PA0/PA1/PA2, active low.
- Memory map: 128 KiB Flash at `0x08000000`, 16 KiB RAM at `0x20000000`.

## Supported commands

- `O`, `C`: open and close the CAN channel.
- `S0` through `S8`: 10, 20, 50, 100, 125, 250, 500, 750 and 1000 kbit/s.
- `M0`, `M1`: normal and silent mode.
- `A0`, `A1`: disable and enable automatic retransmission.
- `tIIILDD...`, `TIIIIIIIILDD...`: standard and extended data frames.
- `rIIIL`, `RIIIIIIIIL`: standard and extended remote frames.
- `V`: firmware version and source remote.
- `E`: firmware error register.

Each accepted command is acknowledged with carriage return; invalid commands
receive BEL. Configure bitrate/mode before opening the channel.

## Build

The validated compiler is Arm GNU Toolchain 15.2.Rel1. Add its `bin` directory
to `PATH`, then run:

```sh
make clean
make VERSION=APM32F072
make size VERSION=APM32F072
```

Outputs are written to `build/` as
`apm32f072cb-slcan-<VERSION>.elf/.hex/.bin/.map`.

Build the MAVCAN variant with:

```sh
make PROTOCOL=mavcan VERSION=APM32F072
make PROTOCOL=mavcan VERSION=APM32F072 size
```

This produces `apm32f072cb-mavcan-<VERSION>.elf/.hex/.bin/.map`. MAVCAN mode
uses MAVLink 2 `CAN_FRAME` messages on USB CDC, emits a 1 Hz heartbeat, and
opens CAN bus 1 at 1 Mbit/s. It is intended for direct use with
`https://can.ardupilot.org` or `https://can.vimdrones.com`. Flash the SLCAN
image again when using SLCAN-only host software.

The active Makefile compiles only `src/`, `inc/` and `vendor/geehy/`. Legacy
STM32 Cube/HAL files retained from the upstream branch are not part of the
APM32 build.

## Flash with CMSIS-DAP and pyOCD

Install pyOCD 0.44.1 in an isolated Python environment and download the Geehy
APM32F0xx DFP 1.1.4 pack. Then run:

```sh
make flash VERSION=APM32F072 \
  PYOCD=/path/to/pyocd \
  DFP_PATH=/path/to/Geehy.APM32F0xx_DFP.1.1.4.pack \
  PROBE_UID=optional-cmsis-dap-uid
```

Add `PROTOCOL=mavcan` to the flash command to program the DroneCAN Web Tools
variant.

`make flash` selects `APM32F072CB`, performs chip erase, programming and
binary readback comparison, then resets the target. Leave `PROBE_UID` empty
when only one probe is attached. `SWD_FREQUENCY=100k CONNECT_MODE=under-reset`
can be supplied for a target that cannot be acquired at the default settings.

See [PORTING_APM32F072CB.md](PORTING_APM32F072CB.md) for implementation and
validation details.

## License

Application sources remain MIT licensed. Geehy SDK files are isolated below
`vendor/geehy/` and are governed by the unmodified
`vendor/geehy/GEEHY COPYRIGHT NOTICE.txt`. See [LICENSE.md](LICENSE.md).
