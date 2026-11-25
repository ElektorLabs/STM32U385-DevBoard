# Problems and Solutions

Bringing up the Elektor STM32U385 Dev Board (260660) in the Arduino IDE

When the first assembled board arrived, it sat on my desk in DFU mode. The
plan sounded simple: upload a blank sketch, blink the LEDs, get serial
working, then turn it into something anyone can use from the Arduino IDE
without opening the schematic. It took longer than a blank sketch should. This
document records every hurdle we hit along the way and how we got past each
one, in the order they happened. If you are bringing up a new STM32U3 board
yourself, most of these will save you an afternoon.

## 1. No tools on the bench

The first hurdle had nothing to do with the chip. The PC had no ARM compiler,
no arduino-cli and no access to the USB device.

Why not just install everything in one go? Part of the setup needs root
rights, and the automated session I was working with was not allowed to
download a compiler by itself. So I installed the pieces by hand: the xPack
GCC toolchain, arduino-cli 1.3.0, dfu-util 0.11 and the STM32duino core.

Even with dfu-util installed, the first attempt failed with
`LIBUSB_ERROR_ACCESS`. On Linux, a normal user cannot talk to a raw USB
device until a udev rule allows it. We wrote two rules: one for the ROM
bootloader (USB ID 0483:df11) and one for the USB serial port of a running
sketch (0483:5740). Both are in `src/Arduino15/packages/elektor/hardware/stm32/1.0.0/tools/linux/99-elektor-u385.rules`.

One more thing I found only at the end: the STM32duino core does not ship
dfu-util, although some older notes online say it does. The README now tells
you to install it from your package manager.

Tip: on a fresh Linux machine, install the udev rule before you plug the
board in. It saves you a confusing permission error on day one.

## 2. A new chip with little reference code

The STM32U385RGT6Q is part of the STM32U3 series, which is new. Most example
code on the internet is written for the U5 or the L4, and the names in the
HAL (ST's Hardware Abstraction Layer) are close but not the same.

Before going to Arduino, I wanted to know the chip itself worked, so we built
a small bare-metal test first: CMSIS headers, the TinyUSB stack for USB
serial, and the four LEDs. That is where the naming differences showed up.
Macros such as `__HAL_RCC_MSIRC0_ENABLE` do not exist on the U3, the boot
address registers are called `BOOT0R` and `BOOT1R` rather than `NSBOOTADDx`,
and the flash ECC register is `ECCCR`, not `ECCR`. None of these are hard to
fix once the compiler points at them. The lesson is to read the U3 header
file, not a U5 forum post.

The bare-metal test worked: LEDs blinking in turn, and text on USB serial.
The hardware was fine. Time to move to Arduino.

## 3. Arduino support for the U3

STM32duino supports the U3 series from version 3.1.0, with a generic variant
for the U375/U385 in the 64-pin package. Generic means every pin is called
`PA5` or `PC10`, and the user has to know from the schematic which pin is an
LED, which goes to the microSD slot and which to the mikroBUS socket.

So we wrote a board package of our own. It reuses the STM32duino core as it
is (the `build.core=STMicroelectronics:arduino` reference), and adds three
things on top. The first is a variant that gives every pin a board name:
`LED1_A`, `BUTTON_USER1`, `SD_CS`, `GROVE_I2C_POWER`, `MIKROBUS_CS` and so on.
The second is a clock setup: 96 MHz from the internal MSIS oscillator, with
HSI48 and its clock recovery system for USB, so no crystal is needed for
USB. The third is a library, U385Board, with helpers for LEDs, buttons,
clocks and chip information, plus nine examples.

Why a separate package instead of a patch to STM32duino? It installs by
dropping one folder into the sketchbook, it can be updated with `git pull`,
and it does not touch the official core.

## 4. Uploading without pressing BOOT

On this board, uploading normally means holding BOOT, pressing RESET and
releasing BOOT. That gets old after the tenth upload. Arduino boards with
native USB, such as the Leonardo, solve it with the "1200 baud touch": the IDE
opens the serial port at 1200 baud, the sketch sees it and resets itself into
the bootloader. Detecting the touch was easy. Getting into the bootloader was
not.

### First try: jump to the bootloader

The usual trick on STM32 is to jump from the running sketch straight into the
ROM bootloader. That means knowing where it is. The obvious address,
0x0BF80000, reads as all zeros. I then scanned the system memory to find it,
and the board crashed, because parts of that area fault when read. The right
address came from the `FLASH->BOOT1R` register: 0x0BF8F000, with a valid
stack pointer and reset vector.

The jump worked, and the bootloader started. Then it handed control straight
back to my sketch. The U3 ROM checks how the chip actually booted, and a
software jump does not count. On this chip, there is no way around that from
code alone.

### What works: the option bytes

The option bytes are a small block of flash settings that the chip reads at
every reset. Two bits in them, `nSWBOOT0` and `nBOOT0` (bits 26 and 27 of
`FLASH_OPTR`), decide whether the chip starts the user program or the ROM
bootloader. If the sketch clears both bits and reloads the option bytes, the
chip resets into a real ROM boot, exactly as if BOOT were held down.

The catch is that the bits have to be set back after the upload, or the board
starts in DFU forever. My first version forgot this in one corner case, and
the board stayed in DFU after an upload. We fixed it in two places. The
upload script `dfu-upload.sh` now reads the option bytes over DFU, flashes
the sketch, and writes the option bytes back with the two bits set again,
which also starts the new sketch. As a safety net, every sketch built with
this package checks the bits at start-up and restores them if they are still
cleared.

One more surprise: now and then the option byte write was rejected. The
flash error flags looked clean, so I never found the exact cause. Clearing
the flags before the write and allowing one retry made it reliable. On the
test board, the option bytes read 0x1FEFF0AA before and after every upload,
and a full button-free upload takes about 4.4 s.

## 5. Changing the clock at run time

The clock example steps the system clock through 96, 48, 24 and 12 MHz. On
paper, that is one HAL call. In practice, three things have to happen in the
right order.

The first is the flash wait states. Flash cannot keep up with a 96 MHz core,
so the CPU waits two cycles per access at that speed. `HAL_RCC_OscConfig`
changes the MSIS frequency but does not touch the wait states, so we set
them ourselves: more wait states before speeding up, fewer only after
slowing down. The second is the voltage range: range 1 (0.9 V) allows up to
96 MHz, range 2 (0.75 V) up to 48 MHz. The third is the EPOD booster, which
must be on above 24 MHz. Get the order wrong and the chip runs from flash
faster than flash allows, which ends in a crash that looks random.

Tested on the board: 96 MHz uses 2 wait states, 48 MHz uses 1, 24 MHz uses 1
in range 2, and 12 MHz uses 0. USB serial stays up all the way down to
12 MHz. Below that, USB cannot run.

## 6. Low-power modes

The low-power example puts the chip into Sleep, Stop or Shutdown for 10 s,
woken by the RTC alarm. Shutdown worked first time. The other two did not.

Sleep woke up immediately, reporting 0 s. Why? Sleep only stops the CPU
clock, and any interrupt wakes it again. With USB connected, the host sends a
start-of-frame packet every millisecond, and each one raises an interrupt. The
fix is to go back to sleep in a loop until the 10 s are over or a button
fires. The CPU still spends most of its time halted, but with USB active the
saving in Sleep mode is small.

After Stop, the board came back and the serial port reappeared, but the
"woke up" message never arrived. The sketch was alive, since it answered the
next command. The cause sits in the STM32duino USB code: `Serial.end()` does
not clear the flag that says a terminal has the port open. After waking, the
sketch believed the PC was still listening and printed before the PC had even
reopened the port. Now the example waits 2.5 s for the PC to re-enumerate the
port before printing.

## 7. The microSD card

The card test failed with "no FAT16/FAT32 partition found". Reading sector 0
of the card directly showed why: it is a 64 GB SDXC card, formatted exFAT,
which is the factory default for cards above 32 GB. The standard Arduino SD
library only reads FAT16 and FAT32.

Rather than ask every reader to reformat their card, we switched the example
to SdFat by Bill Greiman, which reads FAT16, FAT32 and exFAT. One trap on the
way: in the Arduino SD library, `SD.begin(clock, cs)` expects the clock in
hertz. Passing the old `SPI_HALF_SPEED` constant there quietly asks for 1 Hz.

## 8. The freeze

With SdFat in place, the card mounted, listed its files and appended to an
existing log file. Then, the moment the sketch created a new file, the board
froze. USB died completely, so even the button-free upload could not reach
it, and I had to use BOOT and RESET to recover.

A dead USB port tells you something. The USB stack runs from interrupts, so if
USB stops answering, the CPU is no longer handling interrupts at all. A plain
endless loop in the sketch would not do that. A lockup of the core would.

We added a fault catcher that saves the crash address in the TAMP backup
registers and reboots into DFU. It never fired, and that was the clue. On a
Cortex-M33, the startup code can set a hardware stack limit in the MSPLIM
register. The STM32duino startup code for the U3 does exactly that, using
`_Min_Stack_Size` from ST's linker script, and that value is 0x400. One
kilobyte. When the stack grows past it, the chip raises a fault, then needs
stack space to handle the fault, finds none, and locks up. Our fault catcher
never got a chance to run.

Creating a file with SdFat goes through a deeper call chain than appending,
and one function keeps a 512-byte sector buffer on the stack. That pushed it
over 1 KB. Any sketch with a few hundred bytes of local arrays would freeze
the same way.

The fix is one line in the board's linker script: `_Min_Stack_Size = 0x4000`,
16 KB out of 256 KB of RAM. With that in place, the same create, write,
delete and recreate sequence ran without a problem, and the button-free
upload still reached the board afterwards. Other STM32U3 boards in
STM32duino 3.1.0 still have the 1 KB limit.

Tip: if a Cortex-M33 board freezes so hard that USB dies and your fault
handler never runs, look at the stack limit first.

## 9. Drawbacks of uploading without the BOOT button

The button-free upload is handy, and I use it every day during development.
It does have costs, which is why it is an option and not the default upload
method. You should know them before relying on it.

**It wears the option bytes.** Every upload writes the option bytes twice:
once to enter the bootloader and once to leave it. Flash is rated for about
10,000 write cycles, so that is roughly 5,000 uploads before you reach the
rated limit. For normal development that is a lot of uploads. If you upload
hundreds of times a day for months, switch to the BOOT button method.

**The reset cause changes.** After a button-free upload, the sketch reports
"option byte load" as the reset reason, not "power-on" or "reset pin". If your
sketch acts on the reset cause, keep that in mind.

**Any program that opens the port at 1200 baud resets the board.** That is
how the trick works, and it is the same on every native-USB Arduino board.
Do not use 1200 baud for real data.

**It needs a healthy sketch with USB serial.** The sketch itself has to see
the 1200 baud touch. It cannot if it has crashed, is in Stop or Shutdown
mode, has called `Serial.end()`, was built with USB set to HID or None, or
was not built with this board package. In all of those cases, use BOOT and
RESET. A sketch that defines its own `HAL_SYSTICK_Callback()` also replaces
the hook that watches for the touch.

**An interrupted upload leaves the board in DFU.** If the cable is pulled or
the PC crashes after the board has entered the bootloader, the boot bits stay
cleared and the board keeps starting in DFU. Nothing is lost: upload again
and everything is set back. `docs/uploading.md` also shows how to fix it by
hand with dfu-util.

**Power loss during the option byte write is the one real risk.** The write
takes a few milliseconds per upload. If power drops in exactly that window,
the option bytes can end up in a bad state. You would then repair them with
STM32CubeProgrammer, over DFU with the BOOT button or over SWD on the K5
header. It never happened during testing, but it can happen in theory.

In short: use the button-free upload for daily work, keep the BOOT button for
recovery, and switch to the BOOT button for very heavy upload use.

## Status at the end of bring-up

| Part | Status |
|---|---|
| LEDs, USB serial, button-free upload | Working |
| Clock switching 96 to 12 MHz, MSI locked to the 32.768 kHz crystal | Working |
| Sleep, Stop, Shutdown with RTC wake-up | Working |
| SMPS core supply (`Board.useSMPS()`) | Switches both ways; current saving not measured yet |
| microSD (exFAT, 64 GB) | Working |
| Grove I2C power switch and scan | Working, no device connected yet |
| mikroBUS socket | All signals exercised with an empty socket; no Click board tested yet |
| User1 / User2 buttons | Not tested yet on v1.1 |

The open items are a test of the User1 and User2 buttons on v1.1, a current
measurement with the SMPS on, and a real Click board in the mikroBUS socket. These are the
next things I plan to test.
