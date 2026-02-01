# TKM32F499 Clock Project Makefile
#
# Build system for ARM Cortex-M4 based TKM32F499 microcontroller

# Project name
PROJECT = tkm32f499_clock

# Toolchain
PREFIX = arm-none-eabi-
CC = $(PREFIX)gcc
AS = $(PREFIX)gcc -x assembler-with-cpp
CP = $(PREFIX)objcopy
SZ = $(PREFIX)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary -S

# Directories
BUILD_DIR = build
SRC_DIR = src
INC_DIR = inc
STARTUP_DIR = startup
LINKER_DIR = linker

# Source files
C_SOURCES = \
	$(SRC_DIR)/main.c \
	$(SRC_DIR)/system_tkm32f499.c

# Assembly sources
ASM_SOURCES = \
	$(STARTUP_DIR)/startup_tkm32f499.s

# Include paths
C_INCLUDES = \
	-I$(INC_DIR) \
	-ICMSIS/Include

# MCU flags
CPU = -mcpu=cortex-m4
FPU = -mfpu=fpv4-sp-d16
FLOAT-ABI = -mfloat-abi=soft
MCU = $(CPU) -mthumb $(FPU) $(FLOAT-ABI)

# Compiler flags
CFLAGS = $(MCU) $(C_INCLUDES) -Wall -fdata-sections -ffunction-sections
CFLAGS += -g -gdwarf-2 -O2

# Assembler flags
ASFLAGS = $(MCU) -Wall -fdata-sections -ffunction-sections

# Linker flags
LDSCRIPT = $(LINKER_DIR)/tkm32f499.ld
LDFLAGS = $(MCU) -specs=nano.specs -T$(LDSCRIPT) -lc -lm -lnosys
LDFLAGS += -Wl,-Map=$(BUILD_DIR)/$(PROJECT).map,--cref
LDFLAGS += -Wl,--gc-sections

# Object files
OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
vpath %.c $(sort $(dir $(C_SOURCES)))

OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

# Default target
all: $(BUILD_DIR)/$(PROJECT).elf $(BUILD_DIR)/$(PROJECT).hex $(BUILD_DIR)/$(PROJECT).bin

# Build directory
$(BUILD_DIR):
	mkdir -p $@

# Compile C sources
$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

# Compile assembly sources
$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

# Link
$(BUILD_DIR)/$(PROJECT).elf: $(OBJECTS) Makefile
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@
	$(SZ) $@

# Create HEX file
$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(HEX) $< $@

# Create BIN file
$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf | $(BUILD_DIR)
	$(BIN) $< $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Flash using OpenOCD (adjust interface/target as needed)
flash: $(BUILD_DIR)/$(PROJECT).bin
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(PROJECT).bin 0x08000000 verify reset exit"

# Flash using ST-Link utility
flash-stlink: $(BUILD_DIR)/$(PROJECT).bin
	st-flash write $(BUILD_DIR)/$(PROJECT).bin 0x08000000

# Debug with GDB
debug: $(BUILD_DIR)/$(PROJECT).elf
	$(PREFIX)gdb -x gdbinit $(BUILD_DIR)/$(PROJECT).elf

# Print size info
size: $(BUILD_DIR)/$(PROJECT).elf
	$(SZ) --format=berkeley $(BUILD_DIR)/$(PROJECT).elf

# Phony targets
.PHONY: all clean flash flash-stlink debug size

# Dependencies
-include $(wildcard $(BUILD_DIR)/*.d)
