# Validation record - 2026-08-05

## Tool versions

- Arm GNU Toolchain 15.2.Rel1, GCC 15.2.1 (Build arm-15.86).
- GNU Make 4.4.1 (xPack Windows Build Tools 4.4.1-1).
- pyOCD 0.44.1 in an isolated virtual environment.
- Geehy APM32F0xx DFP 1.1.4.
- Geehy APM32F0xx SDK V1.8.7 subset.

## Build validation

Both `PROTOCOL=slcan` and `PROTOCOL=mavcan` builds completed with zero
application compiler/linker warnings. A second clean build produced
byte-identical ELF, HEX, BIN and MAP files for both variants.

| Variant | Text | Data | BSS including heap/stack | Flash image |
| --- | ---: | ---: | ---: | ---: |
| SLCAN | 17,432 | 368 | 7,888 | 17,800 bytes |
| MAVCAN | 14,908 | 368 | 7,896 | 15,276 bytes |

For the MAVCAN ELF, `.apm32_isr_vector` is 192 bytes at `0x08000000`.
The final image contains no STM32 HAL, STM32 CMSIS, `MX_USB`, or `USBD_LL`
symbols. Both Flash and RAM use remain below the APM32F072CBT6 limits.

## Probe and flash validation

- Probe: Arm DAPLink CMSIS-DAP.
- UID: `CBBA2E3A193110A0655DADC28F261FF5`.
- Pack target: `APM32F072CB` from Geehy APM32F0xx DFP 1.1.4.
- The MAVCAN ELF was programmed after a chip erase.
- All 15,276 binary bytes matched during pyOCD readback comparison.
- The target was reset and USB CDC re-enumerated as COM10.

## MAVCAN live validation

The MAVCAN image opens CAN bus 1 at 1 Mbit/s and bridges it with MAVLink 2
`CAN_FRAME` messages over USB CDC. A three-second COM10 capture contained:

- 17 valid `HEARTBEAT` packets (including queued pre-open heartbeats).
- 21 valid `CAN_FRAME` packets.
- 38 of 38 known MAVLink packets passing X.25 CRC validation.

Live traffic decoded as extended CAN ID `0x18ABC500`, DLC 8, with payloads
such as `01 26 F9 61 30 00 C0 C3` through `01 26 F9 61 30 00 C0 C7`.
This validates the external CAN-to-APM32-to-USB MAVCAN receive path.

The compatibility build accepts MAVLink 2 `CAN_FRAME` payloads shortened by
trailing-zero truncation and enables CAN automatic retransmission. Live web-tool
validation discovered node 125 as `org.ardupilot.FlyingRC-L4CAN-GPS`, completed
`GetNodeInfo`, and fetched its parameter table successfully.

The default SLCAN image remains a separate build and retains the previously
validated USB identity, SLCAN commands and CAN receive behavior.
