# How uploading works

The STM32U385 has a USB DFU bootloader in ROM. Everything here uses it;
no ST-LINK or extra hardware is needed.

## BOOT button (default upload method, always works)

1. Hold **BOOT (SW3)**.
2. Press and release **RESET (SW4)** while still holding BOOT.
3. Release BOOT. The board shows up as `0483:df11 STM Device in DFU Mode`.
4. Upload with *Tools → Upload method → USB DFU (board already in DFU mode)*,
   the default.

Unplugging USB, holding BOOT and plugging it back in works too.

## Without buttons (optional)

*USB DFU (auto-reset, no buttons, wears option bytes)* uploads without
touching the board, as long as a sketch built with this board package is
running with USB serial on. It is not the default because every upload
writes the option bytes twice (see the drawbacks below).

1. The IDE opens the board's USB serial port at **1200 baud** and closes it.
2. The sketch notices (a 1 ms SysTick hook in the variant) and sets the boot
   option bits `nSWBOOT0 = 0`, `nBOOT0 = 0` ("boot from system memory"),
   then reloads the option bytes. That resets the chip into a genuine ROM
   boot, exactly as if BOOT were held.
3. `src/Arduino15/packages/elektor/hardware/stm32/1.0.0/tools/dfu-upload.sh` waits for the DFU device, reads the option bytes and
   sees the boot bits cleared. It flashes the sketch **without** leaving DFU,
   then writes the option bytes back with only those two bits set again.
   The bootloader reloads them, which resets the chip into the new sketch.
4. As a safety net, every sketch built with this package checks the bits
   first thing at start-up and restores them if they are still cleared.

Only `nSWBOOT0` and `nBOOT0` (FLASH_OPTR bits 26 and 27) are ever changed.
Read protection and every other option byte are written back unchanged.

### Why not just jump to the bootloader?

That was tried first. `FLASH->BOOT1R` points to the ROM bootloader at
`0x0BF8F000` (its vector table is readable there; `0x0BF80000` reads as
zeros and the rest of system flash faults when read). A software jump to it
runs, but the U3 ROM checks how the chip booted and hands control straight
back to the user program. A real boot through the option bytes is the only
software route that works on this chip.

### Drawbacks

| | |
|---|---|
| Option-byte wear | Two option-byte writes per upload. Flash is rated for about 10,000 cycles, so roughly 5,000 uploads. Use the BOOT button method if you upload very often. |
| Reset cause | After a button-free upload the sketch reports "option byte load" instead of "reset pin" or "power-on". |
| 1200 baud | Opening the port at 1200 baud from any program reboots the board into DFU (same as other native-USB Arduino boards). |
| Needs a live sketch | Does not work if the sketch has crashed, is in deep sleep/shutdown, has called `Serial.end()`, was built with USB set to *None* or *HID*, defines its own `HAL_SYSTICK_Callback()`, or was not built with this board package. Use the BOOT button then. |
| Interrupted upload | If the upload stops after the board entered DFU (cable pulled, PC crash), the board keeps starting in DFU until the next successful upload. Upload again; nothing is lost. |
| Power loss during the option-byte write | A few milliseconds per upload. If it ever happens, fix the option bytes with STM32CubeProgrammer through BOOT-button DFU or SWD (K5). |

## Manual recovery with dfu-util

If the board stays in DFU mode and you want it to run the sketch that is
already in flash, read the option bytes, set bits 26 and 27 of the first
word, and write them back (this is what `dfu-upload.sh` does):

```sh
dfu-util -a 1 -d 0483:df11 -s 0x40022040:64 -U ob.bin
# first 4 bytes are FLASH_OPTR, little-endian; byte 3 must have 0x0C set
python3 -c "b=bytearray(open('ob.bin','rb').read()); b[3]|=0x0C; open('ob_fixed.bin','wb').write(b)"
dfu-util -a 1 -d 0483:df11 -s 0x40022040 -D ob_fixed.bin
```

The original value on the test board was `FLASH_OPTR = 0x1FEFF0AA`
(RDP level 0, `0xAA`).
