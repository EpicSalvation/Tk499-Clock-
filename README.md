# TKM32F499 Clock Project

A clock project based on the TKM32F499 4.3" SmartBoard (ARM Cortex-M4, 240MHz).

## Project Structure

```
Tk499-Clock-/
├── src/                    # Source files
│   ├── clock_main.c        # Clock application with LCD display
│   ├── lcd.c               # LCD driver implementation
│   ├── blink_test.c        # LED blink test (hardware verification)
│   └── system_tkm32f499.c  # System initialization
├── inc/                    # Header files
│   ├── lcd.h               # LCD driver header
│   ├── tkm32f499.h         # MCU peripheral definitions
│   └── system_tkm32f499.h  # System function prototypes
├── startup/                # Startup code
│   └── startup_tkm32f499.s # Vector table and reset handler
├── linker/                 # Linker scripts
│   └── tkm32f499.ld        # Memory layout (SDRAM at 0x70020000)
├── CMSIS/Include/          # ARM CMSIS headers (bundled)
├── vendor/                 # Critical vendor files
│   ├── Bootloader.bin      # For board recovery
│   └── *.pdf               # Documentation
├── LESSONS_LEARNED.md      # Technical notes and gotchas
├── LCD_RENDERING.md        # LCD graphics programming guide
├── Makefile                # Build configuration
└── README.md               # This file
```

## Prerequisites

### ARM GCC Toolchain

**Ubuntu/Debian:**
```bash
sudo apt update
sudo apt install gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi
```

**Fedora:**
```bash
sudo dnf install arm-none-eabi-gcc arm-none-eabi-newlib
```

**macOS (Homebrew):**
```bash
brew install arm-none-eabi-gcc
```

**Windows:**
Download from [ARM Developer](https://developer.arm.com/tools-and-software/open-source-software/developer-tools/gnu-toolchain/gnu-rm/downloads)

### CMSIS Headers

Already bundled in `CMSIS/Include/`. No additional installation needed.

## Building

```bash
git clone https://github.com/EpicSalvation/Tk499-Clock-.git
cd Tk499-Clock-
make            # Build the clock app (default)
make blink_test # Build the LED blink test
make all        # Build both targets
make clean      # Remove build directory
```

### Build Targets

**Clock Application** (`make clock` or just `make`):
- Full clock app with LCD display
- Output: `build/clock.bin`, `build/clock.elf`

**LED Blink Test** (`make blink_test`):
- Minimal test that blinks PA8 LED
- Useful for verifying hardware and flashing process work
- Output: `build/blink_test.bin`, `build/blink_test.elf`

## Flashing

The TKM32F499 uses **USB drag-and-drop** flashing. No external programmer needed.

### Flash Application

1. Hold **APP button** (PA1/SW2)
2. While holding APP, press **RESET**
3. Release RESET, then release APP
4. A USB drive named **"TK499_V2"** appears
5. Copy the binary:
   ```bash
   cp build/clock.bin /media/$USER/TK499_V2/
   # Or for the blink test:
   cp build/blink_test.bin /media/$USER/TK499_V2/
   ```
6. Wait for the drive to auto-unmount
7. Press **RESET** to run

### Bootloader Recovery

If the board stops working (no "TK499_V2" drive appears):

1. Hold **BOOT button** (PA13/SW3)
2. While holding BOOT, press **RESET**
3. Release RESET, then release BOOT
4. A USB drive named **"TK499"** appears (ROM mode)
5. Copy the bootloader:
   ```bash
   cp vendor/Bootloader.bin /media/$USER/TK499/
   ```
6. Wait for auto-unmount, press RESET
7. Now APP+RESET should show "TK499_V2"

## Application Descriptions

### Clock Application
Displays a clock interface on the 4.3" LCD (800x480, 24-bit color):
- Blue header bar with title "Clock Demo"
- Large centered time display "12:00:00" (6x scaled font)
- Orange footer bar with status info
- Decorative separator lines
- LED blinks slowly to indicate the program is running

### LED Blink Test
A minimal test program that blinks the LED on **PA8** (D3 on the SmartBoard):
- ~1 second on, ~1 second off
- Confirms the hardware and flashing process work correctly
- Useful for troubleshooting when the LCD isn't working

## Hardware

- **Board:** TKM32F499 4.3" SmartBoard
- **MCU:** TKM32F499 (ARM Cortex-M4, 240MHz)
- **Memory:** Code runs from external SDRAM at 0x70020000
- **LED:** PA8 (accent LED, active high)
- **Buttons:** APP (PA1), BOOT (PA13), RESET

## Key Technical Notes

This chip is unusual - see `LESSONS_LEARNED.md` for details:

- Code executes from **SDRAM** (0x70020000), not internal flash
- Vector table must be **remapped** to internal SRAM at startup
- GPIO uses **STM32F1-style** registers (CRL/CRH), not STM32F4-style
- **Extended GPIO**: Ports have 24 pins (0-23), not 16 - requires special registers for pins 16-23
- LCD uses **TK80** peripheral with 24-bit RGB888 color (see `LCD_RENDERING.md`)
- Two-stage bootloader: ROM bootloader + secondary bootloader in SPI flash

## Troubleshooting

**"TK499_V2" drive doesn't appear:**
- Try APP+RESET again (timing matters)
- If "TK499" appears instead, your bootloader needs recovery (see above)

**LED doesn't blink after flashing:**
- Make sure you copied the `.bin` file, not `.elf`
- Wait for drive to auto-unmount before pressing RESET
- Check `LESSONS_LEARNED.md` for vector table remapping requirements

**Build errors about missing headers:**
- Verify `CMSIS/Include/` exists with `core_cm4.h` and related files

## License

This project is open source. Feel free to modify and distribute.
