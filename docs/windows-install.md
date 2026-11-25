# Windows install

Set up the Elektor STM32U385 Dev Board in the Arduino IDE in five steps.
Most of it is copying two folders.

**You need:** Arduino IDE 2.x, this repository
(on GitHub: *Code → Download ZIP*, then right-click → *Extract All*), and a
USB-C cable.

## 1. Install the STM32 boards

Skip this step if *STM32 MCU based boards* 3.1.0 or newer is already
installed.

1. *File → Preferences*. Paste this into *Additional boards manager URLs*:
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
2. *Tools → Board → Boards Manager*, search **STM32 MCU based boards**,
   click *Install*.
3. Close the Arduino IDE.

## 2. Copy two folders

Open the `src` folder of the unpacked ZIP. Copy each folder below and paste
it into the folder on the right. Windows merges it with what is already
there.

| Copy this | Paste it into |
|---|---|
| `src\Arduino15\packages` | `%LOCALAPPDATA%\Arduino15` |
| `src\Arduino\libraries` | `Documents\Arduino` |

To open `%LOCALAPPDATA%\Arduino15`, type it into the Explorer address bar
and press Enter.

## 3. Add dfu-util

dfu-util is the program that uploads sketches to the board.

1. Download
   [dfu-util-0.11-binaries.tar.xz](https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz)
   and open it (Windows 11 opens it directly; on Windows 10 use
   [7-Zip](https://www.7-zip.org/)).
2. From its `win64` folder, copy **`dfu-util.exe`** and **`libusb-1.0.dll`**.
3. Paste both into this folder (type it into the Explorer address bar):
   ```
   %LOCALAPPDATA%\Arduino15\packages\elektor\hardware\stm32\1.0.0\tools\win
   ```

## 4. Install the USB driver (once per PC)

1. Plug in the board. Hold **BOOT**, press and release **RESET**, release
   **BOOT**. The board is now in DFU mode.
2. Download and start [Zadig](https://zadig.akeo.ie/).
3. *Options → List All Devices*, select **STM32 BOOTLOADER**, choose
   **WinUSB** and click *Install Driver* (or *Replace Driver*).

## 5. First upload

1. Start the Arduino IDE.
2. *Tools → Board → Elektor STM32U385 Dev Board (260660)*.
3. *File → Examples → Elektor STM32U385 Dev Board → 00_LedCycle*, click
   *Upload*. If the IDE asks for a port, pick any.

The four LEDs start blinking. Done.

**Every upload works the same way:** BOOT + RESET, then *Upload*. That is
the default *Upload method*, **USB DFU (board already in DFU mode)**.

**Optional, no buttons:** *Tools → Upload method → USB DFU (auto-reset, no
buttons, wears option bytes)*, and pick the board's COM port under
*Tools → Port*. The IDE then resets the board into DFU by itself. Each upload
this way writes the chip's option bytes twice, and they are rated for about
10,000 writes, so roughly 5,000 uploads. Fine for a while, not for every
day for years.

---

## If something doesn't work

| Problem | Fix |
|---|---|
| The board is not in *Tools → Board* | Check that this file exists: `%LOCALAPPDATA%\Arduino15\packages\elektor\hardware\stm32\1.0.0\boards.txt`. If you pasted `Arduino15` instead of `packages`, it ends up one folder too deep. |
| The board shows up, but changes have no effect | Delete the old copy in `Documents\Arduino\hardware\elektor` if you have one. Arduino uses that one instead. |
| No *Elektor STM32U385 Dev Board* under *File → Examples* | Select the Elektor board first. Then check that this file exists: `Documents\Arduino\libraries\Elektor_STM32U385_DevBoard\library.properties` |
| `dfu-util not found` | `dfu-util.exe` and `libusb-1.0.dll` are not in the `tools\win` folder (step 3). |
| `No DFU device found` or `Cannot open DFU device` | Do BOOT + RESET again. If it still fails, repeat step 4 (Zadig). |
| Upload with auto-reset fails | The sketch crashed, sleeps, or has USB turned off. Do BOOT + RESET, set *Upload method* to *USB DFU (board already in DFU mode)* and upload again. |

**Good to know**

- Your Documents folder is in OneDrive? Then the sketchbook is
  `OneDrive\Documents\Arduino`. *File → Preferences* shows the right path.
- The ZIP often unpacks to a folder inside a folder of the same name. The
  `src` folder is in the inner one.
- Two examples need extra libraries from *Tools → Manage Libraries*:
  **STM32duino Low Power** and **STM32duino RTC** for `05_LowPowerModes`,
  **SdFat** by Bill Greiman for `07_SDCard`.
- **Updating:** download the new ZIP and repeat step 2. Choose *Replace the
  files* when Windows asks. dfu-util stays in place.
- How the button-free upload works: [uploading.md](uploading.md).
