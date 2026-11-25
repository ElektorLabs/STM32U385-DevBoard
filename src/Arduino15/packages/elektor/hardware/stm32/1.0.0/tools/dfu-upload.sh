#!/bin/sh
# Flash a sketch into an STM32U385 through its ROM USB-DFU bootloader.
#   $1  sketch .bin file
#   $2  directory holding dfu-util (may be empty or an unexpanded {...})
#   $3  -v for verbose output
# The Arduino IDE has already done the 1200-baud touch if that upload method
# is selected, so the board should now be rebooting into DFU (0483:df11).

BIN="$1"
DFU_DIR="$2"
VERBOSE="$3"

# Where to find dfu-util, first match wins:
#   1. Arduino's packaged dfu-util tool ($2)
#   2. tools/win/dfu-util.exe next to this script (Windows, copied in by hand)
#   3. dfu-util on PATH
SCRIPT_DIR=$(dirname "$0")
DFU=dfu-util
if [ -n "$DFU_DIR" ] && [ -x "$DFU_DIR/dfu-util" ]; then
  DFU="$DFU_DIR/dfu-util"
elif [ -n "$DFU_DIR" ] && [ -x "$DFU_DIR/dfu-util.exe" ]; then
  DFU="$DFU_DIR/dfu-util.exe"
elif [ -f "$SCRIPT_DIR/win/dfu-util.exe" ]; then
  DFU="$SCRIPT_DIR/win/dfu-util.exe"
fi
if ! command -v "$DFU" >/dev/null 2>&1; then
  echo "dfu-util not found. Install it (Linux: sudo apt install dfu-util," >&2
  echo "macOS: brew install dfu-util, Windows: copy dfu-util.exe and" >&2
  echo "libusb-1.0.dll into $SCRIPT_DIR/win/)." >&2
  exit 1
fi

if [ ! -f "$BIN" ]; then
  echo "Sketch binary not found: $BIN" >&2
  exit 1
fi

echo "Waiting for the STM32 DFU bootloader (0483:df11)..."
i=0
while ! "$DFU" -l 2>/dev/null | grep -q "0483:df11"; do
  i=$((i + 1))
  if [ "$i" -ge 40 ]; then
    echo "No DFU device found after 10 s." >&2
    echo "Hold BOOT, press RESET, release BOOT, then upload again." >&2
    echo "Linux: check the udev rule in tools/linux/ is installed." >&2
    exit 1
  fi
  sleep 0.25
done

TMP=$(mktemp -d 2>/dev/null || { mkdir -p "/tmp/u385dfu.$$" && echo "/tmp/u385dfu.$$"; })
LOG="$TMP/dfu.log"
OB="$TMP/ob.bin"
cleanup() { rm -rf "$TMP"; }

run_dfu() {
  "$DFU" "$@" >>"$LOG" 2>&1
}

# Read the 64-byte option-byte block (DFU alt 1, starts with FLASH_OPTR).
# The sketch's auto-reset clears nSWBOOT0 and nBOOT0 (OPTR bits 26, 27 =
# byte 3, mask 0x0C) so the chip boots the ROM bootloader. Those bits must be
# set again after flashing, or the chip would keep booting into DFU.
AUTO_ENTERED=0
if run_dfu -a 1 -d 0483:df11 -s 0x40022040:64 -U "$OB" && [ -s "$OB" ]; then
  B3=$(od -An -tu1 -j3 -N1 "$OB" | tr -d ' ')
  if [ -n "$B3" ] && [ $((B3 & 12)) -ne 12 ]; then
    AUTO_ENTERED=1
  fi
fi

if [ "$AUTO_ENTERED" = "1" ]; then
  # 1. flash the sketch without leaving DFU
  run_dfu -a 0 -d 0483:df11 -s 0x08000000 -D "$BIN"
  RC=$?
  if grep -q "File downloaded successfully" "$LOG"; then
    # 2. set nSWBOOT0 and nBOOT0 again (only those two bits) and write the block
    #    back. The bootloader reloads the option bytes, which resets the chip
    #    and starts the new sketch.
    NEW3=$((B3 | 12))
    OCT=$(printf '%03o' "$NEW3")
    {
      dd if="$OB" bs=1 count=3 2>/dev/null
      printf "\\$OCT"
      dd if="$OB" bs=1 skip=4 2>/dev/null
    } >"$OB.fixed"
    run_dfu -a 1 -d 0483:df11 -s 0x40022040 -D "$OB.fixed"
    OBWRITE_OK=0
    grep -q "Downloading element to address = 0x40022040" "$LOG" && OBWRITE_OK=1
  fi
else
  run_dfu -a 0 -d 0483:df11 -s 0x08000000:leave -D "$BIN"
  RC=$?
fi

if [ "$VERBOSE" = "-v" ]; then
  cat "$LOG"
else
  grep -i "found dfu\|downloading element\|downloaded successfully\|cannot\|error" "$LOG" \
    | grep -v "get_status\|Error during download" | sort -u
fi

# DfuSe resets the chip on 'leave' or on the option-byte reload, so the last
# status query fails even though everything worked. Trust the success lines.
if ! grep -q "File downloaded successfully" "$LOG"; then
  cleanup
  echo "Upload failed (dfu-util exit code $RC)." >&2
  exit 1
fi
if [ "$AUTO_ENTERED" = "1" ] && [ "$OBWRITE_OK" != "1" ]; then
  cleanup
  echo "Sketch written, but restoring the boot option bytes failed." >&2
  echo "The board will stay in DFU mode; upload again to retry." >&2
  exit 1
fi
cleanup
echo "Upload done, sketch is starting."
exit 0
