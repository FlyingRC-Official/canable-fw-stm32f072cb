PROTOCOL ?= slcan
ifeq ($(PROTOCOL),slcan)
PROTOCOL_SOURCE := src/slcan.c
else ifeq ($(PROTOCOL),mavcan)
PROTOCOL_SOURCE := src/mavcan.c
PROTOCOL_DEFS := -DMAVCAN_BRIDGE
else
$(error Unsupported PROTOCOL '$(PROTOCOL)'; use slcan or mavcan)
endif

PROJECT := apm32f072cb-$(PROTOCOL)
VERSION ?= dev
REMOTE ?= github.com/FlyingRC-Official/canable-fw-stm32f072cb
TARGET := $(PROJECT)-$(VERSION)
BUILD_DIR ?= build
OBJ_DIR := $(BUILD_DIR)/obj/$(PROTOCOL)

TOOLCHAIN_PREFIX ?= arm-none-eabi-
CC := $(TOOLCHAIN_PREFIX)gcc
SIZE := $(TOOLCHAIN_PREFIX)size
OBJCOPY := $(TOOLCHAIN_PREFIX)objcopy

LD_SCRIPT := APM32F072CB_FLASH.ld
CPU_FLAGS := -mcpu=cortex-m0plus -mthumb
DEFS := -DAPM32F072xB -DUSB_DEVICE -DPRINTF_DISABLE_SUPPORT_FLOAT
DEFS += -DPRINTF_DISABLE_SUPPORT_EXPONENTIAL
DEFS += -DGIT_VERSION=\"$(VERSION)\" -DGIT_REMOTE=\"$(REMOTE)\"
DEFS += $(PROTOCOL_DEFS)

GEEHY := vendor/geehy
DEVICE := $(GEEHY)/Libraries/Device/Geehy/APM32F0xx
PERIPH := $(GEEHY)/Libraries/APM32F0xx_StdPeriphDriver
USB := $(GEEHY)/Middlewares/APM32_USB_Library/Device

INCLUDES := -Iinc
INCLUDES += -I$(GEEHY)/Libraries/CMSIS/Include
INCLUDES += -I$(DEVICE)/Include
INCLUDES += -I$(PERIPH)/inc
INCLUDES += -I$(USB)/Core/Inc
INCLUDES += -I$(USB)/Class/CDC/Inc

APP_SOURCES := src/main.c src/system.c src/usbd_cdc_if.c src/usb_device.c
APP_SOURCES += src/usbd_desc.c src/interrupts.c src/can.c $(PROTOCOL_SOURCE)
APP_SOURCES += src/led.c src/error.c src/printf.c src/runtime.c
DEVICE_SOURCES := $(DEVICE)/Source/system_apm32f0xx.c
PERIPH_SOURCES := $(PERIPH)/src/apm32f0xx_can.c $(PERIPH)/src/apm32f0xx_crs.c
PERIPH_SOURCES += $(PERIPH)/src/apm32f0xx_fmc.c $(PERIPH)/src/apm32f0xx_gpio.c
PERIPH_SOURCES += $(PERIPH)/src/apm32f0xx_misc.c $(PERIPH)/src/apm32f0xx_rcm.c
PERIPH_SOURCES += $(PERIPH)/src/apm32f0xx_usb.c $(PERIPH)/src/apm32f0xx_usb_device.c
USB_SOURCES := $(USB)/Core/Src/usbd_core.c $(USB)/Core/Src/usbd_dataXfer.c
USB_SOURCES += $(USB)/Core/Src/usbd_stdReq.c $(USB)/Class/CDC/Src/usbd_cdc.c
STARTUP := $(DEVICE)/Source/gcc/startup_apm32f072.S
SOURCES := $(APP_SOURCES) $(DEVICE_SOURCES) $(PERIPH_SOURCES) $(USB_SOURCES)
OBJECTS := $(addprefix $(OBJ_DIR)/,$(notdir $(SOURCES:.c=.o))) $(OBJ_DIR)/startup_apm32f072.o

CFLAGS := $(CPU_FLAGS) $(DEFS) $(INCLUDES) -std=c11 -Os -g3
CFLAGS += -Wall -Wextra -Wshadow -Wundef -Wdouble-promotion
CFLAGS += -ffunction-sections -fdata-sections -fno-common
ASFLAGS := $(CPU_FLAGS) $(DEFS) $(INCLUDES) -x assembler-with-cpp -g3
LDFLAGS := $(CPU_FLAGS) -nostartfiles -T$(LD_SCRIPT) -Wl,--gc-sections -Wl,--cref
LDFLAGS += -Wl,--no-warn-rwx-segments
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(TARGET).map --specs=nano.specs --specs=nosys.specs
LDLIBS := -Wl,--start-group -lc -lm -lnosys -lgcc -Wl,--end-group

ELF := $(BUILD_DIR)/$(TARGET).elf
HEX := $(BUILD_DIR)/$(TARGET).hex
BIN := $(BUILD_DIR)/$(TARGET).bin
MAP := $(BUILD_DIR)/$(TARGET).map

define make_dir
	@mkdir -p "$1"
endef
define remove_dir
	@rm -rf "$1"
endef

.PHONY: all clean size flash
all: $(ELF) $(HEX) $(BIN)

$(ELF): $(OBJECTS) $(LD_SCRIPT) | $(BUILD_DIR)
	$(CC) $(OBJECTS) $(LDFLAGS) $(LDLIBS) -o $@
	$(SIZE) $@

$(HEX): $(ELF)
	$(OBJCOPY) -O ihex $< $@

$(BIN): $(ELF)
	$(OBJCOPY) -O binary $< $@

$(OBJ_DIR)/%.o: src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(DEVICE)/Source/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(PERIPH)/src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(USB)/Core/Src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

$(OBJ_DIR)/%.o: $(USB)/Class/CDC/Src/%.c | $(OBJ_DIR)
	$(CC) $(CFLAGS) -w -MMD -MP -c $< -o $@

$(OBJ_DIR)/startup_apm32f072.o: $(STARTUP) | $(OBJ_DIR)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR) $(OBJ_DIR):
	$(call make_dir,$@)

size: $(ELF)
	$(SIZE) -A $(ELF)

PYOCD ?= pyocd
PYOCD_TARGET ?= APM32F072CB
DFP_PATH ?= tools/Geehy.APM32F0xx_DFP.1.1.4.pack
PROBE_UID ?=
PYOCD_UID_ARG := $(if $(strip $(PROBE_UID)),--uid $(PROBE_UID),)
SWD_FREQUENCY ?=
CONNECT_MODE ?=
PYOCD_FREQ_ARG := $(if $(strip $(SWD_FREQUENCY)),--frequency $(SWD_FREQUENCY),)
PYOCD_CONNECT_ARG := $(if $(strip $(CONNECT_MODE)),--connect $(CONNECT_MODE),)
PYOCD_ARGS := --target $(PYOCD_TARGET) --pack "$(DFP_PATH)" $(PYOCD_UID_ARG) $(PYOCD_FREQ_ARG) $(PYOCD_CONNECT_ARG)

flash: all
	$(PYOCD) load $(PYOCD_ARGS) --erase chip $(ELF)
	$(PYOCD) commander $(PYOCD_ARGS) -c "compare 0x08000000 $(BIN)"
	$(PYOCD) reset $(PYOCD_ARGS)

clean:
	$(call remove_dir,$(BUILD_DIR))

-include $(OBJECTS:.o=.d)
