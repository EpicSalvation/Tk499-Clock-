# TKM32F499 Clock Project Makefile
#
# Build system for ARM Cortex-M4 based TKM32F499 microcontroller
#
# Targets:
#   make clock      - Build the clock application (default)
#   make blink_test - Build the LED blink test
#   make all        - Build both targets
#   make clean      - Remove build directory

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

# Common source files
COMMON_SOURCES = \
	$(SRC_DIR)/system_tkm32f499.c

# Clock application sources
CLOCK_SOURCES = \
	$(SRC_DIR)/clock_main.c \
	$(SRC_DIR)/lcd.c \
	$(SRC_DIR)/esp8266.c \
	$(COMMON_SOURCES)

# Blink test sources
BLINK_SOURCES = \
	$(SRC_DIR)/blink_test.c

# LCD test sources (minimal standalone test)
LCD_TEST_SOURCES = \
	$(SRC_DIR)/lcd_test.c

# TK80 minimal test sources
TK80_MIN_SOURCES = \
	$(SRC_DIR)/tk80_min_test.c

# LCD fill only test sources
LCD_FILL_SOURCES = \
	$(SRC_DIR)/lcd_fill_only.c

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
LDFLAGS += -Wl,--gc-sections

# Build directory
$(BUILD_DIR):
	mkdir -p $@

# Assembly object files (shared)
$(BUILD_DIR)/startup_tkm32f499.o: $(STARTUP_DIR)/startup_tkm32f499.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) $< -o $@

# C object files
$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -Wa,-a,-ad,-alms=$(BUILD_DIR)/$(notdir $(<:.c=.lst)) $< -o $@

#######################################
# Clock Application Target
#######################################
CLOCK_PROJECT = clock
CLOCK_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(CLOCK_SOURCES:.c=.o)))
CLOCK_OBJECTS += $(BUILD_DIR)/startup_tkm32f499.o

clock: $(BUILD_DIR)/$(CLOCK_PROJECT).elf $(BUILD_DIR)/$(CLOCK_PROJECT).hex $(BUILD_DIR)/$(CLOCK_PROJECT).bin
	@echo "Clock application built successfully!"
	@echo "Flash $(BUILD_DIR)/$(CLOCK_PROJECT).bin to your device"

$(BUILD_DIR)/$(CLOCK_PROJECT).elf: $(CLOCK_OBJECTS) Makefile
	$(CC) $(CLOCK_OBJECTS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(CLOCK_PROJECT).map,--cref -o $@
	$(SZ) $@

$(BUILD_DIR)/$(CLOCK_PROJECT).hex: $(BUILD_DIR)/$(CLOCK_PROJECT).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(CLOCK_PROJECT).bin: $(BUILD_DIR)/$(CLOCK_PROJECT).elf | $(BUILD_DIR)
	$(BIN) $< $@

#######################################
# Blink Test Target
#######################################
BLINK_PROJECT = blink_test
BLINK_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(BLINK_SOURCES:.c=.o)))
BLINK_OBJECTS += $(BUILD_DIR)/startup_tkm32f499.o

blink_test: $(BUILD_DIR)/$(BLINK_PROJECT).elf $(BUILD_DIR)/$(BLINK_PROJECT).hex $(BUILD_DIR)/$(BLINK_PROJECT).bin
	@echo "Blink test built successfully!"
	@echo "Flash $(BUILD_DIR)/$(BLINK_PROJECT).bin to your device"

$(BUILD_DIR)/$(BLINK_PROJECT).elf: $(BLINK_OBJECTS) Makefile
	$(CC) $(BLINK_OBJECTS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(BLINK_PROJECT).map,--cref -o $@
	$(SZ) $@

$(BUILD_DIR)/$(BLINK_PROJECT).hex: $(BUILD_DIR)/$(BLINK_PROJECT).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(BLINK_PROJECT).bin: $(BUILD_DIR)/$(BLINK_PROJECT).elf | $(BUILD_DIR)
	$(BIN) $< $@

#######################################
# LCD Test Target
#######################################
LCD_TEST_PROJECT = lcd_test
LCD_TEST_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(LCD_TEST_SOURCES:.c=.o)))
LCD_TEST_OBJECTS += $(BUILD_DIR)/startup_tkm32f499.o

lcd_test: $(BUILD_DIR)/$(LCD_TEST_PROJECT).elf $(BUILD_DIR)/$(LCD_TEST_PROJECT).hex $(BUILD_DIR)/$(LCD_TEST_PROJECT).bin
	@echo "LCD test built successfully!"
	@echo "Flash $(BUILD_DIR)/$(LCD_TEST_PROJECT).bin to your device"

$(BUILD_DIR)/$(LCD_TEST_PROJECT).elf: $(LCD_TEST_OBJECTS) Makefile
	$(CC) $(LCD_TEST_OBJECTS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(LCD_TEST_PROJECT).map,--cref -o $@
	$(SZ) $@

$(BUILD_DIR)/$(LCD_TEST_PROJECT).hex: $(BUILD_DIR)/$(LCD_TEST_PROJECT).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(LCD_TEST_PROJECT).bin: $(BUILD_DIR)/$(LCD_TEST_PROJECT).elf | $(BUILD_DIR)
	$(BIN) $< $@

#######################################
# TK80 Minimal Test Target
#######################################
TK80_MIN_PROJECT = tk80_min_test
TK80_MIN_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(TK80_MIN_SOURCES:.c=.o)))
TK80_MIN_OBJECTS += $(BUILD_DIR)/startup_tkm32f499.o

tk80_min_test: $(BUILD_DIR)/$(TK80_MIN_PROJECT).elf $(BUILD_DIR)/$(TK80_MIN_PROJECT).hex $(BUILD_DIR)/$(TK80_MIN_PROJECT).bin
	@echo "TK80 minimal test built successfully!"
	@echo "Flash $(BUILD_DIR)/$(TK80_MIN_PROJECT).bin to your device"

$(BUILD_DIR)/$(TK80_MIN_PROJECT).elf: $(TK80_MIN_OBJECTS) Makefile
	$(CC) $(TK80_MIN_OBJECTS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(TK80_MIN_PROJECT).map,--cref -o $@
	$(SZ) $@

$(BUILD_DIR)/$(TK80_MIN_PROJECT).hex: $(BUILD_DIR)/$(TK80_MIN_PROJECT).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(TK80_MIN_PROJECT).bin: $(BUILD_DIR)/$(TK80_MIN_PROJECT).elf | $(BUILD_DIR)
	$(BIN) $< $@

#######################################
# LCD Fill Only Test Target
#######################################
LCD_FILL_PROJECT = lcd_fill_only
LCD_FILL_OBJECTS = $(addprefix $(BUILD_DIR)/,$(notdir $(LCD_FILL_SOURCES:.c=.o)))
LCD_FILL_OBJECTS += $(BUILD_DIR)/startup_tkm32f499.o

lcd_fill_only: $(BUILD_DIR)/$(LCD_FILL_PROJECT).elf $(BUILD_DIR)/$(LCD_FILL_PROJECT).hex $(BUILD_DIR)/$(LCD_FILL_PROJECT).bin
	@echo "LCD fill only test built successfully!"
	@echo "Flash $(BUILD_DIR)/$(LCD_FILL_PROJECT).bin to your device"

$(BUILD_DIR)/$(LCD_FILL_PROJECT).elf: $(LCD_FILL_OBJECTS) Makefile
	$(CC) $(LCD_FILL_OBJECTS) $(LDFLAGS) -Wl,-Map=$(BUILD_DIR)/$(LCD_FILL_PROJECT).map,--cref -o $@
	$(SZ) $@

$(BUILD_DIR)/$(LCD_FILL_PROJECT).hex: $(BUILD_DIR)/$(LCD_FILL_PROJECT).elf | $(BUILD_DIR)
	$(HEX) $< $@

$(BUILD_DIR)/$(LCD_FILL_PROJECT).bin: $(BUILD_DIR)/$(LCD_FILL_PROJECT).elf | $(BUILD_DIR)
	$(BIN) $< $@

#######################################
# Default and Utility Targets
#######################################

# Default target - build the clock app
.DEFAULT_GOAL := clock

# Build all targets
all: clock blink_test

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Flash clock app using OpenOCD (adjust interface/target as needed)
flash: $(BUILD_DIR)/$(CLOCK_PROJECT).bin
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(CLOCK_PROJECT).bin 0x08000000 verify reset exit"

# Flash using ST-Link utility
flash-stlink: $(BUILD_DIR)/$(CLOCK_PROJECT).bin
	st-flash write $(BUILD_DIR)/$(CLOCK_PROJECT).bin 0x08000000

# Flash blink test
flash-blink: $(BUILD_DIR)/$(BLINK_PROJECT).bin
	openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
		-c "program $(BUILD_DIR)/$(BLINK_PROJECT).bin 0x08000000 verify reset exit"

# Debug with GDB
debug: $(BUILD_DIR)/$(CLOCK_PROJECT).elf
	$(PREFIX)gdb -x gdbinit $(BUILD_DIR)/$(CLOCK_PROJECT).elf

# Print size info for clock app
size: $(BUILD_DIR)/$(CLOCK_PROJECT).elf
	$(SZ) --format=berkeley $(BUILD_DIR)/$(CLOCK_PROJECT).elf

# Print size info for blink test
size-blink: $(BUILD_DIR)/$(BLINK_PROJECT).elf
	$(SZ) --format=berkeley $(BUILD_DIR)/$(BLINK_PROJECT).elf

# Phony targets
.PHONY: all clock blink_test lcd_test tk80_min_test lcd_fill_only clean flash flash-stlink flash-blink debug size size-blink

# Dependencies
-include $(wildcard $(BUILD_DIR)/*.d)
