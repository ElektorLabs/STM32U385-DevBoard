#!/usr/bin/env python3
"""Generate docs/pinout.svg for the Elektor STM32U385 Dev Board (260660).

Arduino-style layout: the board is turned 90 degrees clockwise (USB at the
top, digital header on the right, power and analog header on the left) and
every header pin gets a row of pill-shaped labels.

    python3 docs/make_pinout.py .      (run from the repository root)
"""
import base64, io, sys
from PIL import Image

REPO = sys.argv[1]

# docs/board-top.png is the v1.1 render (1340 x 1023, white margin around the
# board). Every position below is in pixels of that file. The board is cropped
# to CROP, scaled to a 1110 px wide frame and turned clockwise; a frame point
# (x, y) moves to (LH_ - y, x). For a new render, crop and re-measure here.
CROP = (159, 93, 1185, 887)             # board edges, incl. USB-C and jack
S = 1110 / (CROP[2] - CROP[0])
LW_, LH_ = 1110, round((CROP[3] - CROP[1]) * S)
IW, IH = LH_, LW_
def rot(x, y):
    """Render pixel -> position in the turned frame."""
    fx, fy = (x - CROP[0]) * S, (y - CROP[1]) * S
    return (LH_ - fy, fx)

src = Image.open(f"{REPO}/docs/board-top.png").convert("RGB").crop(CROP).rotate(-90, expand=True)
buf = io.BytesIO(); src.save(buf, "PNG", optimize=True)
img_b64 = base64.b64encode(buf.getvalue()).decode()

OX, OY = 330, 330
W, H = OX + IW + 370, OY + IH + 300

C = {"ard": "#212121", "mcu": "#5388c6", "fn": "#e8a33d", "pwr": "#e30613",
     "gnd": "#6b6b6b", "ana": "#2e9e5b", "nc": "#bdbdbd", "line": "#212121",
     "spi": "#9b59b6", "i2c": "#16a2a2", "uart": "#d35400"}
FONT = "Arial, Helvetica, sans-serif"
out = []
add = out.append

def text_w(t, fs=14):
    return 0.62 * fs * len(t) + 22

def pill(x, y, text, fill, anchor="left", fs=14, minw=0):
    """Pill with its vertical centre at y. anchor=left: starts at x and grows
    right; anchor=right: ends at x and grows left. Returns the far edge."""
    w = max(text_w(text, fs), minw); h = 28
    x0 = x if anchor == "left" else x - w
    add(f'<rect x="{x0:.1f}" y="{y-h/2:.1f}" width="{w:.1f}" height="{h}" rx="{h/2}" fill="{fill}"/>')
    add(f'<text x="{x0+w/2:.1f}" y="{y+1:.1f}" fill="#fff" font-size="{fs}" font-weight="bold" '
        f'text-anchor="middle" dominant-baseline="middle">{text}</text>')
    return x0 + w if anchor == "left" else x0

def fn_colour(f):
    if f.startswith("SPI"): return C["spi"]
    if f.startswith("I2C"): return C["i2c"]
    if f.startswith("Serial1"): return C["uart"]
    return C["fn"]

def kind(ard):
    if ard == "GND": return "gnd"
    if ard in ("3V3", "5V", "VIN", "IOREF"): return "pwr"
    if ard.startswith("A") and ard[1:].isdigit(): return "ana"
    if ard == "NC": return "nc"
    return "ard"

# Header pins, pin centres measured on the render: (x, Arduino, MCU, functions)
DIGITAL_Y = 130  # top edge -> right side after turning
DIGITAL = [
    (446, "SCL", "PB6", ["I2C SCL"]), (484, "SDA", "PB7", ["I2C SDA"]),
    (521.5, "AREF", "", ["VDDA via JP2"]), (559.5, "GND", "", []),
    (597, "D13", "PA5", ["SPI SCK"]), (635, "D12", "PA6", ["SPI MISO"]),
    (673, "D11", "PA7", ["SPI MOSI"]), (711, "D10", "PA15", ["SPI SS"]),
    (748.5, "D9", "PA8", ["MCO clock out"]), (786, "D8", "PC7", []),
    (839, "D7", "PC6", []), (877, "D6", "PB10", ["PWM", "mikroBUS PWM"]),
    (915, "D5", "PB15", []), (953, "D4", "PB14", []),
    (990.5, "D3", "PB13", []), (1028.5, "D2", "PC9", ["mikroBUS INT"]),
    (1066, "D1", "PA9", ["Serial1 TX"]), (1104, "D0", "PA10", ["Serial1 RX"]),
]
POWER_Y = 850    # bottom edge -> left side after turning
POWER_ANALOG = [
    (579.5, "NC", "", []), (617.5, "IOREF", "", ["3.3 V"]),
    (655, "RESET", "NRST", []), (693, "3V3", "", []),
    (730.5, "5V", "", []), (768.5, "GND", "", []), (806.5, "GND", "", []),
    (844, "VIN", "", ["max. 18 V"]),
    (919.5, "A0", "PA0", ["mikroBUS AN"]), (957.5, "A1", "PA1", []),
    (995.5, "A2", "PA2", []), (1033, "A3", "PC1", []),
    (1071, "A4", "PA3", []), (1108.5, "A5", "PB1", []),
]
NAME_W, MCU_W, GAP = 66, 66, 6

add(f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" '
    f'viewBox="0 0 {W} {H}" width="{W}" height="{H}" font-family="{FONT}">')
add(f'<rect width="{W}" height="{H}" fill="#ffffff"/>')
add(f'<text x="{W/2}" y="48" text-anchor="middle" font-size="32" font-weight="bold" fill="#212121">'
    f'Elektor STM32U385 Dev Board v1.1</text>')
add(f'<text x="{W/2}" y="80" text-anchor="middle" font-size="17" fill="#555">'
    f'STM32U385RGT6Q · Cortex-M33 · all I/O 3.3 V · board names for code: docs/pin-guide.md</text>')
add(f'<image x="{OX}" y="{OY}" width="{IW}" height="{IH}" xlink:href="data:image/png;base64,{img_b64}"/>')

# Right side: digital header
for x, ard, mcu, fns in DIGITAL:
    px, py = rot(x, DIGITAL_Y); px += OX; py += OY
    start = OX + IW + 18
    add(f'<line x1="{px}" y1="{py}" x2="{start}" y2="{py}" stroke="{C["line"]}" stroke-width="1.5"/>')
    add(f'<circle cx="{px}" cy="{py}" r="4" fill="{C["line"]}"/>')
    cx = pill(start, py, ard, C[kind(ard)], "left", minw=NAME_W) + GAP
    if mcu: cx = pill(cx, py, mcu, C["mcu"], "left", minw=MCU_W) + GAP
    else: cx += MCU_W + GAP
    for f in fns: cx = pill(cx, py, f, fn_colour(f), "left", 13) + GAP

# Left side: power and analog header
for x, ard, mcu, fns in POWER_ANALOG:
    px, py = rot(x, POWER_Y); px += OX; py += OY
    start = OX - 18
    add(f'<line x1="{px}" y1="{py}" x2="{start}" y2="{py}" stroke="{C["line"]}" stroke-width="1.5"/>')
    add(f'<circle cx="{px}" cy="{py}" r="4" fill="{C["line"]}"/>')
    cx = pill(start, py, ard, C[kind(ard)], "right", 13 if len(ard) > 4 else 14, minw=NAME_W) - GAP
    if mcu: cx = pill(cx, py, mcu, C["mcu"], "right", minw=MCU_W) - GAP
    else: cx -= MCU_W + GAP
    for f in fns:
        col = C["pwr"] if f.startswith("max") or f.startswith("3.3") else fn_colour(f)
        cx = pill(cx, py, f, col, "right", 13) - GAP

def box(bx, by, bw, lines):
    lh = 21; bh = 12 + lh * len(lines)
    add(f'<rect x="{bx}" y="{by}" width="{bw}" height="{bh}" rx="12" fill="#ffffff" stroke="#212121" stroke-width="1.5"/>')
    for i, (t, col, bold) in enumerate(lines):
        add(f'<text x="{bx+12}" y="{by+24+i*lh}" font-size="15" font-weight="{"bold" if bold else "normal"}" fill="{col}">{t}</text>')
    return bh

def leader(sx, sy, target):
    tx, ty = rot(*target); tx += OX; ty += OY
    add(f'<line x1="{sx}" y1="{sy}" x2="{tx}" y2="{ty}" stroke="{C["line"]}" stroke-width="1.5"/>')
    add(f'<circle cx="{tx}" cy="{ty}" r="7" fill="none" stroke="{C["pwr"]}" stroke-width="3"/>')

K, B, F = C["ard"], C["mcu"], "#b06d00"

# ---- above the board (power input side) --------------------------------------
bw, by = 268, 110
bx = OX - 20
h = box(bx, by, bw, [("VIN barrel jack K2", K, True), ("to the BQ25185 charger", K, False),
                     ("max. 18 V", C["pwr"], False)])
leader(bx + bw / 2, by + h, (268, 738))
bx = OX + 285
h = box(bx, by, bw, [("BATT K3: 1S LiPo", K, True), ("BQ25185 charger", K, False),
                     ("red CHG LED3, green PWR LED4", K, False)])
leader(bx + bw / 2, by + h, (220, 590))
bx = OX + 590
h = box(bx, by, bw, [("USB-C K1", K, True), ("power, USB serial, DFU upload", K, False),
                     ("PA11 D−  ·  PA12 D+", B, False)])
leader(bx + bw / 4, by + h, (220, 365))   # left of the LED2 leader

# ---- right column above the digital pins ---------------------------------------
# On the turned board RESET sits right above LED1, so straight leaders from one
# column would cross the buttons. Elbow leaders run left, then straight down to
# the part. The boxes are ordered by their part's x position, left to right, so
# no two leaders cross, and the vertical runs pass between the buttons.
def elbow(sx, sy, target):
    tx, ty = rot(*target); tx += OX; ty += OY
    add(f'<polyline points="{sx},{sy} {tx:.1f},{sy} {tx:.1f},{ty:.1f}" fill="none" stroke="{C["line"]}" stroke-width="1.5"/>')
    add(f'<circle cx="{tx:.1f}" cy="{ty:.1f}" r="7" fill="none" stroke="{C["pwr"]}" stroke-width="3"/>')

RX, RW = OX + IW + 20, 330
y = 200                                  # below the USB-C box
for lines, target, kind_ in [
    ([("LED2 (two-colour)", K, True), ("PC12 LED2_A  ·  PB4 LED2_B", B, False)], (372, 268), "elbow"),
    ([("BOOT (SW3) → PH3 / BOOT0", K, True), ("hold BOOT, tap RESET = DFU", K, False)], (228, 238), "elbow"),
    ([("LED1 (two-colour)", K, True), ("PC10 LED1_A, LED_BUILTIN", B, False), ("PC11 LED1_B", B, False)], (372, 197), "elbow"),
    ([("RESET (SW4) → NRST", K, True)], (228, 150), "elbow"),
    ([("SWD / JTAG K5", K, True), ("PA13 SWDIO · PA14 SWCLK · PB3 SWO", B, False)], (532, 240), "straight"),
]:
    h = box(RX, y, RW, lines)
    (elbow if kind_ == "elbow" else leader)(RX, y + h / 2, target)
    y += h + 12

# ---- left column above the power pins -----------------------------------------
h = box(20, 470, 285, [("USER1 (SW2) → PB5", K, True), ("BUTTON_USER1, USER_BTN", B, False),
                       ("USER2 (SW1) → PB2", K, True), ("BUTTON_USER2", B, False),
                       ("HIGH = pressed", K, False)])
leader(305, 470 + h / 2, (488, 820))
leader(305, 470 + h / 2, (488, 735))

# ---- below the board -------------------------------------------------------------
by = OY + IH + 40
col_w = 282
bx = OX - 120
h = box(bx, by, col_w + 40, [("mikroBUS socket (Click boards)", K, True),
                          ("J5: AN PA0 · RST PC3 · CS PB0", B, False),
                          ("SCK PA5 · MISO PA6 · MOSI PA7", B, False),
                          ("3V3 · GND", B, False),
                          ("J6: PWM PB10 · INT PC9", B, False),
                          ("RX PA10 · TX PA9 · SCL PB6", B, False),
                          ("SDA PB7 · 5V · GND", B, False)])
leader(bx + 200, by, (945, 510))          # mikroBUS logo between J5 and J6
bx = OX + 230
h = box(bx, by, col_w, [("Grove UART K6 (Serial1)", K, True), ("PA10 RX · PA9 TX", B, False),
                        ("check pin order first", F, False)])
leader(bx + col_w / 2, by, (1115, 640))
h2 = box(bx, by + h + 14, col_w, [("microSD K7 (on the back)", K, True), ("SPI1 · SD_CS PC8", B, False),
                                  ("SD_DETECT PD2, LOW = card", B, False)])
bx = OX + 530
h = box(bx, by, col_w, [("Grove I2C K4 (Wire)", K, True), ("PB7 SDA · PB6 SCL", B, False),
                        ("3V3 switched by PA4", B, False)])
leader(bx + col_w / 2, by, (1115, 415))
h2 = box(bx, by + h + 14, col_w, [("Qwiic K8 (Wire, 3.3 V)", K, True), ("PB7 SDA · PB6 SCL", B, False)])
leader(bx + col_w, by + h + 14 + h2 / 2, (1100, 238))
bx = OX + 830
box(bx, by, 340, [("Serial = USB", K, True), ("Serial1 = PA10 RX / PA9 TX", K, True),
                  ("Wire = PB7 SDA / PB6 SCL", K, True), ("SPI = PA5 / PA6 / PA7", K, True),
                  ("Not connected: PB8 PC0 PC2 PC13", "#777", False)])

# ---- legend ----------------------------------------------------------------------
items = [("Arduino pin", C["ard"]), ("MCU pin", C["mcu"]), ("Power", C["pwr"]),
         ("Ground", C["gnd"]), ("Analog", C["ana"]), ("SPI", C["spi"]), ("I2C", C["i2c"]),
         ("UART", C["uart"]), ("Other function", C["fn"])]
ly = H - 40
total = sum(text_w(n, 15) + 18 for n, _ in items)
lx = (W - total) / 2
for name, col in items:
    lx = pill(lx, ly, name, col, "left", 15) + 18
add('</svg>')

open(f"{REPO}/docs/pinout.svg", "w").write("\n".join(out))
print("written", W, H)
