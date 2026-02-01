# TKM32F499 Clock Project

A clock project based on the TKM32F499 development board (ARM Cortex-M4).

## Project Structure

```
Tk499-Clock-/
├── src/                    # Source files
│   ├── main.c              # Main application (Hello World)
│   └── system_tkm32f499.c  # System initialization and drivers
├── inc/                    # Header files
│   ├── tkm32f499.h         # MCU peripheral definitions
│   └── system_tkm32f499.h  # System function prototypes
├── startup/                # Startup code
│   └── startup_tkm32f499.s # Vector table and reset handler
├── linker/                 # Linker scripts
│   └── tkm32f499.ld        # Memory layout definition
├── Makefile                # Build configuration
└── README.md               # This file
```

## Prerequisites

### 1. Install ARM GCC Toolchain

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

**Fedora:**
```bash
sudo dnf install arm-none-eabi-gcc arm-none-eabi-newlib
```

**macOS (using Homebrew):**
```bash
brew install arm-none-eabi-gcc
```

**Windows:**
Download and install from [ARM Developer](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)

### 2. CMSIS Headers (Bundled)

CMSIS headers for Cortex-M4 are already included in this repository at `CMSIS/Include/`. No additional installation is required.

If you need to update or reinstall them manually, download from ARM's GitHub and place them **relative to the project root**:

```
Tk499-Clock-/
└── CMSIS/
    └── Include/
        ├── core_cm4.h
        ├── cmsis_compiler.h
        ├── cmsis_gcc.h
        └── cmsis_version.h
```

To download fresh copies:
```bash
cd Tk499-Clock-
mkdir -p CMSIS/Include
cd CMSIS/Include
wget https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include/core_cm4.h
wget https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include/cmsis_compiler.h
wget https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include/cmsis_gcc.h
wget https://raw.githubusercontent.com/ARM-software/CMSIS_5/develop/CMSIS/Core/Include/cmsis_version.h
```

### 3. Install Flashing Tools

**OpenOCD:**
```bash
# Ubuntu/Debian
sudo apt install openocd

# macOS
brew install openocd

# Fedora
sudo dnf install openocd
```

**ST-Link Tools (alternative):**
```bash
# Ubuntu/Debian
sudo apt install stlink-tools

# macOS
brew install stlink
```

## Building

1. Clone the repository:
```bash
git clone https://github.com/EpicSalvation/Tk499-Clock-.git
cd Tk499-Clock-
```

2. Build the project:
```bash
make
```

3. The build outputs will be in the `build/` directory:
   - `tkm32f499_clock.elf` - ELF executable (for debugging)
   - `tkm32f499_clock.hex` - Intel HEX format
   - `tkm32f499_clock.bin` - Raw binary (for flashing)

## Flashing

### Using OpenOCD
```bash
make flash
```

### Using ST-Link
```bash
make flash-stlink
```

### Manual Flashing
```bash
st-flash write build/tkm32f499_clock.bin 0x08000000
```

## Hello World Behavior

The Hello World example blinks an LED connected to GPIO PA0:
- LED toggles every 500ms (1Hz blink rate)
- Indicates the board is running correctly

To modify the LED pin, edit `src/main.c`:
```c
#define LED_PORT    GPIOA
#define LED_PIN     GPIO_Pin_0
```

## Debugging

### Using GDB with OpenOCD
1. Start OpenOCD in one terminal:
```bash
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg
```

2. Connect with GDB in another terminal:
```bash
arm-none-eabi-gdb build/tkm32f499_clock.elf
(gdb) target remote :3333
(gdb) monitor reset halt
(gdb) load
(gdb) continue
```

## Cleaning

Remove all build artifacts:
```bash
make clean
```

## Hardware Requirements

- TKM32F499 development board
- ST-Link V2 programmer (or compatible)
- USB cable
- LED (if not built-in)

## Troubleshooting

**"arm-none-eabi-gcc: command not found"**
- Ensure the ARM toolchain is installed and in your PATH

**"No ST-LINK detected"**
- Check USB connection
- Install udev rules for ST-Link (Linux)
- Try running with sudo

**Build errors about missing CMSIS headers**
- Verify that `CMSIS/Include/` exists in the project root with `core_cm4.h` and related files
- If missing, re-download from ARM's GitHub (see Prerequisites section above)

## License

This project is open source. Feel free to modify and distribute.
