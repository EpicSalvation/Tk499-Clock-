# Vendor Files

Critical files from the TKM32F499 vendor package needed for development and recovery.

## Files

| File | Description |
|------|-------------|
| `Bootloader.bin` | Secondary bootloader binary. Flash this if your board stops working. |
| `TKM32F499 Program and data download method.pdf` | How to flash programs via USB drag-and-drop |
| `TK499 storage space and Bootloader written.pdf` | Memory map and bootloader architecture |
| `4.3inch_TK499_SmartBorad_Schematic.pdf` | Circuit schematic for the 4.3" SmartBoard |

## Bootloader Recovery

If your board won't run programs:

1. Hold **BOOT** (PA13) + press **RESET**
2. Release RESET, then release BOOT
3. A "TK499" USB drive should appear
4. Copy `Bootloader.bin` to the drive
5. Wait for auto-unmount, press RESET
6. Board should now work - use APP+RESET for "TK499_V2" mode
