# TKM32F499 Clock Project

A clock project based on the TKM32F499 4.3" SmartBoard (ARM Cortex-M4, 240MHz).

## Features

- **NTP Time Sync** - Automatic time synchronization via ESP8266 WiFi module
- **Day/Night Themes** - Automatic theme switching based on time of day (7 AM - 10 PM)
- **Touch Screen** - Tap the sun/moon icon to manually toggle themes
- **Large Display** - 800x480 LCD with 6x scaled time display

## Project Structure

```
Tk499-Clock-/
├── src/                    # Source files
│   ├── clock_main.c        # Clock application with LCD display
│   ├── lcd.c               # LCD driver implementation
│   ├── touch.c             # Touchscreen driver
│   ├── esp8266.c           # ESP8266 WiFi driver
│   └── system_tkm32f499.c  # System initialization
├── inc/                    # Header files
│   ├── lcd.h               # LCD driver header
│   ├── touch.h             # Touchscreen driver header
│   ├── esp8266.h           # ESP8266 driver header
│   └── tkm32f499.h         # MCU peripheral definitions
├── startup/                # Startup code
│   └── startup_tkm32f499.s # Vector table and reset handler
├── linker/                 # Linker scripts
│   └── tkm32f499.ld        # Memory layout (SDRAM at 0x70020000)
├── ntp-server/             # Python NTP server for local network
├── CMSIS/Include/          # ARM CMSIS headers (bundled)
├── vendor/                 # Critical vendor files
│   ├── Bootloader.bin      # For board recovery
│   └── *.pdf               # Documentation
├── LESSONS_LEARNED.md      # Technical notes and gotchas
├── LCD_RENDERING.md        # LCD graphics programming guide
├── ESP8266_WIFI.md         # WiFi module integration guide
├── TOUCHSCREEN.md          # Touchscreen implementation guide
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

## Configuration

Edit `src/clock_main.c` to set your WiFi credentials and timezone:

```c
#define WIFI_SSID       "YourSSID"
#define WIFI_PASSWORD   "YourPassword"
#define TIMEZONE_OFFSET -5  /* Hours from UTC (e.g., -5 for EST, -8 for PST) */
```

## Building

```bash
git clone https://github.com/EpicSalvation/Tk499-Clock-.git
cd Tk499-Clock-
make            # Build the clock app (default)
make clean      # Remove build directory
```

Output: `build/clock.bin`, `build/clock.elf`

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

## Usage

### Display

- **Top bar**: Shows "TKM32F499 Clock" title and version
- **Center**: Large time display (HH:MM:SS)
- **Below time**: Current date
- **Bottom bar**: Status messages (WiFi connection, sync status)
- **Upper right**: Sun/moon icon for theme toggle

### Theme Toggle

- **Automatic**: Day theme (7 AM - 10 PM), Night theme (10 PM - 7 AM)
- **Manual**: Tap the sun/moon icon to override automatic switching
- **Night mode**: Reduced backlight brightness (20%)

### NTP Time Sync

The clock syncs time via HTTP through the ESP8266 WiFi module:
1. Connects to configured WiFi network
2. Fetches time from `time.nist.gov` (or local NTP server)
3. Re-syncs every 5 minutes

See `ESP8266_WIFI.md` for details on the WiFi module integration.

## Hardware

- **Board:** TKM32F499 4.3" SmartBoard
- **MCU:** TKM32F499 (ARM Cortex-M4, 240MHz)
- **Display:** 800x480 LCD (TK043F1168)
- **Touch:** Resistive touch panel (built-in TOUCHPAD ADC)
- **WiFi:** ESP8266 module on UART2
- **Memory:** Code runs from external SDRAM at 0x70020000

## Key Technical Notes

This chip is unusual - see `LESSONS_LEARNED.md` for details:

- Code executes from **SDRAM** (0x70020000), not internal flash
- Vector table must be **remapped** to internal SRAM at startup
- GPIO uses **STM32F1-style** registers (CRL/CRH), not STM32F4-style
- **Extended GPIO**: Ports have 24 pins (0-23), not 16
- LCD uses **TK80** peripheral with 24-bit RGB888 color (see `LCD_RENDERING.md`)
- Touchscreen uses **built-in TOUCHPAD ADC**, not external XPT2046 (see `TOUCHSCREEN.md`)

## Documentation

- `LESSONS_LEARNED.md` - Technical notes and gotchas for this MCU
- `LCD_RENDERING.md` - LCD graphics programming guide
- `ESP8266_WIFI.md` - WiFi module integration
- `TOUCHSCREEN.md` - Touchscreen implementation details

## Troubleshooting

**"TK499_V2" drive doesn't appear:**
- Try APP+RESET again (timing matters)
- If "TK499" appears instead, your bootloader needs recovery (see above)

**Clock shows wrong time:**
- Check WiFi credentials in `src/clock_main.c`
- Verify `TIMEZONE_OFFSET` is correct for your location
- Check status bar for WiFi connection errors

**Touch screen not responding:**
- The RTP version uses built-in TOUCHPAD ADC, not XPT2046
- See `TOUCHSCREEN.md` for implementation details

## License

This project is open source. Feel free to modify and distribute.
