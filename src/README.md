# Install and first upload

Set up the Elektor STM32U385 Dev Board in the Arduino IDE. On Windows, follow
the shorter [Windows install](../docs/windows-install.md) guide instead.

The `src` folder is laid out the way Arduino expects, so installing means
copying two folders:

```
src/
  Arduino15/packages/     the board package   → paste into your Arduino15 folder
  Arduino/libraries/      the library         → paste into your sketchbook
```

## 1. Install the STM32 boards

Skip this if *STM32 MCU based boards* 3.1.0 or newer is already installed.

1. *File → Preferences*, add to *Additional boards manager URLs*:
   ```
   https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
   ```
2. *Tools → Board → Boards Manager*, search **STM32 MCU based boards**,
   install it (3.1.0 or newer: the first version with STM32U3 support).
3. Close the IDE.

## 2. Copy two folders

Get the files with *Code → Download ZIP* on GitHub, or `git clone`. Then copy
each folder into place; it merges with the folder already there.

| Copy this | Windows | Linux | macOS |
|---|---|---|---|
| `src/Arduino15/packages` | `%LOCALAPPDATA%\Arduino15` | `~/.arduino15` | `~/Library/Arduino15` |
| `src/Arduino/libraries` | `Documents\Arduino` | `~/Arduino` | `~/Documents/Arduino` |

Linux, from the repository folder:

```sh
cp -r src/Arduino15/packages ~/.arduino15/
cp -r src/Arduino/libraries ~/Arduino/
```

## 3. Install dfu-util

dfu-util uploads the sketches. Version 0.9 or newer:

| | |
|---|---|
| Windows | Copy `dfu-util.exe` and `libusb-1.0.dll` from the `win64` folder of [dfu-util-0.11-binaries.tar.xz](https://dfu-util.sourceforge.net/releases/dfu-util-0.11-binaries.tar.xz) into `Arduino15\packages\elektor\hardware\stm32\1.0.0\tools\win` |
| Linux | `sudo apt install dfu-util` (or your distribution's package) |
| macOS | `brew install dfu-util` |

## 4. Give access to the USB device (once)

**Windows:** put the board in DFU mode (hold BOOT, press RESET, release
BOOT), start [Zadig](https://zadig.akeo.ie/), choose *Options → List All
Devices*, select **STM32 BOOTLOADER**, choose **WinUSB** and click
*Install Driver*.

**Linux:** install the udev rule:

```sh
sudo cp src/Arduino15/packages/elektor/hardware/stm32/1.0.0/tools/linux/99-elektor-u385.rules /etc/udev/rules.d/
sudo udevadm control --reload-rules && sudo udevadm trigger
```

**macOS:** nothing to do.

## 5. First upload

1. Plug in USB-C. Hold **BOOT**, press and release **RESET**, release **BOOT**.
   The board is now in DFU mode.
2. Start the IDE. *Tools → Board → Elektor STM32U385 Dev Board (260660)*.
3. *File → Examples → Elektor STM32U385 Dev Board → 00_LedCycle*, *Upload*.
   The four LEDs start their light patterns.

Every upload works like this: BOOT + RESET, then *Upload*. That is the
default *Upload method*, **USB DFU (board already in DFU mode, dfu-util)**.

**Optional, no buttons:** choose *Upload method → USB DFU (auto-reset, no
buttons, wears option bytes)* and pick the board's serial port. The IDE then
resets the board into the bootloader through the USB serial port (the
"1200 baud touch", as on the Arduino Leonardo). The catch: every upload
writes the chip's option bytes twice, and they are rated for about 10,000
writes, so roughly 5,000 uploads. That is why it is not the default.
[docs/uploading.md](../docs/uploading.md) explains how it works and its
other limits.

---

## More

### Libraries for two examples

Install from *Tools → Manage Libraries*:

| Library | Needed by |
|---|---|
| **STM32duino Low Power** and **STM32duino RTC** (2.0.0 or newer) | `05_LowPowerModes` |
| **SdFat** by Bill Greiman (2.3.0 or newer) | `07_SDCard` |

### Tools menu

| Menu | Options |
|---|---|
| Upload method | **USB DFU (board already in DFU mode, dfu-util)** (default) · USB DFU (auto-reset, no buttons, wears option bytes) · STM32CubeProgrammer (SWD, K5 header) |
| USB support | **CDC (generic 'Serial' supersede U(S)ART)** (default, `Serial` = USB) · CDC (no generic 'Serial') · HID (keyboard and mouse) · None |
| U(S)ART support | **Enabled (generic Serial)** · Enabled (no generic Serial) · Disabled |
| Optimize, Debug symbols, C runtime library | As in STM32duino |

With USB set to *HID* or *None*, the auto-reset upload does not work. Use
the default BOOT + RESET upload then.

### Stay up to date with git (link instead of copy)

Link the folders instead of copying them, and `git pull` updates the
install:

```sh
git clone https://github.com/Saad-Imtiaz/Elektor-STM32U385-DevBoard ~/Elektor-STM32U385-DevBoard
ln -s ~/Elektor-STM32U385-DevBoard/src/Arduino15/packages/elektor ~/.arduino15/packages/elektor
ln -s ~/Elektor-STM32U385-DevBoard/src/Arduino/libraries/Elektor_STM32U385_DevBoard ~/Arduino/libraries/Elektor_STM32U385_DevBoard
```

Windows (Command Prompt; junctions need no admin rights):

```bat
git clone https://github.com/Saad-Imtiaz/Elektor-STM32U385-DevBoard %USERPROFILE%\Elektor-STM32U385-DevBoard
mklink /J "%LOCALAPPDATA%\Arduino15\packages\elektor" "%USERPROFILE%\Elektor-STM32U385-DevBoard\src\Arduino15\packages\elektor"
mklink /J "%USERPROFILE%\Documents\Arduino\libraries\Elektor_STM32U385_DevBoard" "%USERPROFILE%\Elektor-STM32U385-DevBoard\src\Arduino\libraries\Elektor_STM32U385_DevBoard"
```

### arduino-cli

```sh
arduino-cli config add board_manager.additional_urls \
  https://github.com/stm32duino/BoardManagerFiles/raw/main/package_stmicroelectronics_index.json
arduino-cli core update-index
arduino-cli core install STMicroelectronics:stm32
# board in DFU mode (BOOT + RESET), the default upload method
arduino-cli compile --upload -b elektor:stm32:u385board \
  ~/Arduino/libraries/Elektor_STM32U385_DevBoard/examples/00_LedCycle
# optional: auto-reset, no buttons (wears the option bytes)
arduino-cli compile --upload -b elektor:stm32:u385board:upload_method=dfuAuto -p /dev/ttyACM0 mysketch
```

### Upgrading from an older install

Earlier versions were installed in `<sketchbook>/hardware/elektor`. Delete
that folder: if it is still there, Arduino uses it and ignores the new copy
in Arduino15.

### Windows notes

- If STM32CubeProgrammer or DfuSe is installed, the DFU device may have ST's
  driver, which dfu-util can't use. Replace it with WinUSB in Zadig.
- The USB serial port of a running sketch needs no driver on Windows 10
  and 11.
- The upload script runs in the BusyBox shell that comes with the STM32duino
  core, so no extra shell is needed.
