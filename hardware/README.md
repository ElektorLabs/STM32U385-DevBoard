# Hardware

KiCad project for the Elektor STM32U385 Dev Board (260660), board revision
v1.1.

| Path | What |
|---|---|
| `STM32U385_DevBoard.kicad_pro` | KiCad project: open this |
| `STM32U385_DevBoard.kicad_sch` | Schematic |
| `STM32U385_DevBoard.kicad_pcb` | PCB layout |
| `stm32u385-uno-schematic.pdf` | Schematic as PDF |
| `MySym/`, `MyLib.pretty/`, `DevBoard4.pretty/` | Project symbols and footprints |
| `3D Models/` | STEP models for the 3D view |
| `Data_Sheets/` | Datasheets of the main parts |
| `plots/` | Assembly drawing (PDF) |
| `production/` | Fabrication ZIP: Gerbers, drill and placement files |

v1.1 moves the buttons: RESET and BOOT sit on the left edge next to USB-C,
USER1 and USER2 at the bottom left next to the barrel jack, so they are
easier to press. The MCU pins are unchanged, so the same board package and
library work on v1.0 and v1.1. [docs/pin-guide.md](../docs/pin-guide.md) and
the [pinout drawing](../docs/pinout.png) list every pin and connector.
