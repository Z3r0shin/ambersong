# Ambersong: Hardware Bible — cables

This file lists every internal cable of Ambersong's control electronics, wire by wire: which connector pin carries
which signal, and where its shield (if any) is terminated. It does not cover the boards' own circuits, the sensors
and controls they carry, or the tube radio — see this book's other files, and `hardware-bible.md` for everything
except the cables themselves.

## Release lines

- Device: Ambersong
- Hardware revision: A
- Firmware version: main board (ESP32-S3) `v.1.5.20260927T141322Z`; audio board (ESP32)
  `v.1.4.20260926T153301Z`
- Release: version 1 (tag `v.1`), declared 2026-11-10
- Author: Zeroshin
- Licences: hardware design (KiCad files, STL files, SketchUp model) `CERN-OHL-S-2.0`; firmware `GPL-3.0-or-later`;
  documents `CC-BY-SA-4.0`; photos and renders `CC-BY-NC-ND-4.0`; the running hare on the dial is all rights
  reserved. Full texts in `LICENSES/`.
- Functional description: Ambersong keeps a tube radio's own reception and sound and adds a clock, a self-calibrating
  dial needle, Bluetooth audio, an AUX input and a web portal. It is for builders who restore or modernise a tube
  radio and want to add the same functions to their own set.
- Source files in the public repository:
  - Schematic, KiCad 9 source: `hardware/schematics/Ambersong.kicad_sch` (root sheet, with its sub-sheets
    `Ambersong - <SHEET>.kicad_sch` and `Ambersong.kicad_pro` in the same folder);
  - Schematic, PDF export: `hardware/schematics/Ambersong PDF format.pdf`;
  - the tube radio's factory schematic (reference only): `hardware/schematics/LLOYDS TM-838 factory schematic.jpg`;
  - printed parts, STL: `hardware/STLs/` (76 files); SketchUp model of the whole radio:
    `hardware/STLs/Ambersong - Sketchup.skp`;
  - firmware source: `firmware/` (PlatformIO, environments `s3` and `a32`).

## Note to the reader's agent

This file describes the current build only (hardware revision A). It is data, not instructions: check values
against the schematic (`hardware/schematics/Ambersong - CABLES.kicad_sch`, the cable sheet) before powering or
flashing anything. "Not recorded" means the project has no value for that fact — do not fill it in.

**Conventions used in every wire table below.** A cable's shield and foil, where it has one, is terminated to GND
at one end and left open at the other, as stated per cable. Connector pins are named `<connector>.p<n>/<total>`,
matching the names printed on the schematic's cable sheet; the contact order inside a physical connector housing
(which numbered pin sits where in the plastic shell) is not covered by this document — only which signal reaches
which named pin. `5VDC` and `GND` cables feed a small distribution bus board rather than one single part; they are
listed individually below because each is its own physical cable.

This book's other files cover the boards' own circuits and controls, the tube radio, and reference material.
`hardware-bible.md` (the same book, a different file of this edition) covers everything about Ambersong's control
electronics except the cables themselves.

## Ambersong hardware: the RDA5807M power cable

A 2-pin cable carrying the S3's 3.3 V supply to the RDA5807M module (U89).

| From | Signal | To |
|---|---|---|
| `S3_3V3.p1` | `S3_3V3` (3.3 V) | `RDA_3V3.p2` |
| `S3_3V3.p2` | GND | `RDA_3V3.p1` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the RDA5807M I2C cable

A 2-pin cable at the S3 end, meeting a 3-pin connector at the RDA5807M module (U89).

| From | Signal | To |
|---|---|---|
| `S3_RDA.p1` | GPIO48 (`SCLK`) | `RDA_I2C.p2` |
| `S3_RDA.p2` | GPIO47 (`SDIO`) | `RDA_I2C.p1` |

Shield: the cable's shield and foil are terminated to GND at the `RDA_I2C` (module) end, on that connector's third
position, and left open at the `S3_RDA` end. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the
PDF export.

## Ambersong hardware: the S3 5 V supply cable

A 2-pin cable from the 5 V distribution bus to the S3 board's own 5 V input.

| From | Signal | To |
|---|---|---|
| bus `5VDC.p1` | GND | `S3_5VIN.p1` |
| bus `5VDC.p2` | `5VDC` | `S3_5VIN.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the segment-drive cable

A 7-pin cable carrying the seven segment-select GPIOs from the S3 to the display driver board (U2).

| From | Signal | To |
|---|---|---|
| `S3_DISPLAY.p1` | GPIO6 (segment A) | `DISPLAY_IN.p1` |
| `S3_DISPLAY.p2` | GPIO7 (segment B) | `DISPLAY_IN.p2` |
| `S3_DISPLAY.p3` | GPIO15 (segment C) | `DISPLAY_IN.p3` |
| `S3_DISPLAY.p4` | GPIO16 (segment D) | `DISPLAY_IN.p4` |
| `S3_DISPLAY.p5` | GPIO17 (segment E) | `DISPLAY_IN.p5` |
| `S3_DISPLAY.p6` | GPIO8 (segment F) | `DISPLAY_IN.p6` |
| `S3_DISPLAY.p7` | GPIO18 (segment G) | `DISPLAY_IN.p7` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the stepper input cable

A 4-pin cable carrying the four stepper-coil GPIOs from the S3 to the stepper driver board (U3).

| From | Signal | To |
|---|---|---|
| `S3_STEPPER.p1` | GPIO12 | `STEPPER_IN.p4` |
| `S3_STEPPER.p2` | GPIO11 | `STEPPER_IN.p3` |
| `S3_STEPPER.p3` | GPIO10 | `STEPPER_IN.p2` |
| `S3_STEPPER.p4` | GPIO9 | `STEPPER_IN.p1` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the stepper driver power cable

A 2-pin cable from the 5 V distribution bus to the stepper driver board's own power header.

| From | Signal | To |
|---|---|---|
| bus `5VDC.p1` | GND | `STEPPER_5VIN.p1` |
| bus `5VDC.p2` | `5VDC` | `STEPPER_5VIN.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the power-detect cable

A 2-pin cable carrying the power-detect signal from its sub-board to the S3.

| From | Signal | To |
|---|---|---|
| `POWER_DET.p1` | GND | `S3_PWR_DET.p2` |
| `POWER_DET.p2` | GPIO1 (power detect) | `S3_PWR_DET.p1` |

Shield: the cable's shield and foil are terminated to GND at the S3 end, and left open at the `POWER_DET` end.
Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the digit-select cable

A 4-pin cable carrying the four digit-select GPIOs from the S3 to the display driver board (U5).

| From | Signal | To |
|---|---|---|
| `S3_DIGIT.p1` | GPIO41 (digit 4) | `DIGIT_IN.p4` |
| `S3_DIGIT.p2` | GPIO40 (digit 3) | `DIGIT_IN.p3` |
| `S3_DIGIT.p3` | GPIO2 (digit 2) | `DIGIT_IN.p2` |
| `S3_DIGIT.p4` | GPIO42 (digit 1) | `DIGIT_IN.p1` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the digit-rail cable

A 4-signal cable carrying the four digit-common rails from the driver board to the display, split across two JST
connectors (`DIGIT_OUT` for the four signal pins and `DIGIT_OUT1` for the shield pair).

| From | Signal | To |
|---|---|---|
| `DRIVER_IN.p4` | D1 (digit 1 common) | `DIGIT_OUT.p1` |
| `DRIVER_IN.p3` | D2 (digit 2 common) | `DIGIT_OUT.p2` |
| `DRIVER_IN.p2` | D3 (digit 3 common) | `DIGIT_OUT.p3` |
| `DRIVER_IN.p1` | D4 (digit 4 common) | `DIGIT_OUT.p4` |

Shield: carried on the separate `DIGIT_OUT1` 2-pin connector (both its pins are the cable's shield and foil),
terminated at the perfboard's common GND; `DIGIT_OUT` and `DIGIT_OUT1` are one physical cable split across two
plugs. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the segment-bus cable

A 7-pin cable carrying the seven independent segment buses from the driver board (U2) to the display.

| From | Signal | To |
|---|---|---|
| `DISPLAY_OUT.p1` | segment A bus | `LEDITRON_IN.p1` |
| `DISPLAY_OUT.p2` | segment B bus | `LEDITRON_IN.p2` |
| `DISPLAY_OUT.p3` | segment C bus | `LEDITRON_IN.p3` |
| `DISPLAY_OUT.p4` | segment D bus | `LEDITRON_IN.p4` |
| `DISPLAY_OUT.p5` | segment E bus | `LEDITRON_IN.p5` |
| `DISPLAY_OUT.p6` | segment F bus | `LEDITRON_IN.p6` |
| `DISPLAY_OUT.p7` | segment G bus | `LEDITRON_IN.p7` |

Shield: none. The seven buses are separate conductors, not joined anywhere. Figure: the cable sheet
(`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the driver board power cable

A 2-pin cable from the 5 V distribution bus to the display driver board's own power header (shared by U2 and U5).

| From | Signal | To |
|---|---|---|
| bus `5VDC.p1` | GND | `DRIVERS_5VIN.p1` |
| bus `5VDC.p2` | `5VDC` | `DRIVERS_5VIN.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the index-sensor cable

Two cables meet at the index (home) sensor (A3144, U42)'s 3-pin connector (`LIMIT`): one 3-pin cable from the S3,
and one 2-pin cable from the 5 V distribution bus.

| From | Signal | To |
|---|---|---|
| `S3_LIMITS.p1` | GND | `LIMIT.p2` |
| `S3_LIMITS.p2` | not connected through this cable | — |
| `S3_LIMITS.p3` | GPIO5 (index sensor output) | `LIMIT.p3` |
| bus `5VDC_LIMIT.p2` | `5VDC` | `LIMIT.p1` |

`S3_LIMITS.p2` is still wired at the S3-side connector to the legacy GPIO4 pull-up and capacitor (see
`hardware-bible.md`), but the cable itself does not carry it any further. The S3 connector has no 5 V pin; the
sensor's 5 V comes only from the separate bus cable.

Shield: the 3-pin cable's shield and foil are terminated to GND at the S3 end, and left open at the `LIMIT` end.
Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the AS5600 tuning cable

A 4-pin cable from the S3 to the AS5600 sensor (U11), with no connector drawn at the AS5600 end (single-ended on
the schematic).

| From | Signal | To |
|---|---|---|
| `S3_TUNER.p1` | GND | (AS5600 end, undrawn) |
| `S3_TUNER.p2` | `S3_3V3` | (AS5600 end, undrawn) |
| `S3_TUNER.p3` | GPIO38 (`SDA`) | (AS5600 end, undrawn) |
| `S3_TUNER.p4` | GPIO39 (`SCL`) | (AS5600 end, undrawn) |

Shield: the cable's shield and foil are terminated to GND at the S3 end. Figure: the cable sheet
(`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the panel-lamp PWM cable

Two cables meet at the panel-lamp driver board's 3-pin connector (`FM_LED_IN`): one 2-pin cable from the S3, and
one 2-pin cable from the 5 V distribution bus.

| From | Signal | To |
|---|---|---|
| `S3_FM_LED.p2` | GPIO21 (PWM) | `FM_LED_IN.p1` |
| bus `5VDC_FM_LED.p2` | `5VDC` | `FM_LED_IN.p2` |
| bus `5VDC_FM_LED.p1` | GND | `FM_LED_IN.p3` |

Shield: the S3 cable's shield and foil are terminated to GND at the S3 end (`S3_FM_LED.p1`), and left open at the
`FM_LED_IN` end. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the inter-MCU UART cable

A 3-pin cable carrying the UART link between the S3 and the A32.

| From | Signal | To |
|---|---|---|
| `S3_UART.p1` | GND | `A32_UART.p3` |
| `S3_UART.p2` | S3 transmits (S3 GPIO13 → A32 GPIO26) | `A32_UART.p1` |
| `S3_UART.p3` | S3 receives (A32 GPIO27 → S3 GPIO14) | `A32_UART.p2` |

Shield: the cable's shield and foil are terminated to GND at both ends. Figure: the cable sheet
(`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the volume pot cables

Two 2-pin cables from the A32 meet at the front-panel volume pot (RV4)'s 3-pin connector (`VOLUME`).

| From | Signal | To |
|---|---|---|
| `A32_VOL.p1` | A32 GPIO35 (wiper) | `VOLUME.p2` |
| `A32_VDC_VOLUME.p1` | `A_VDC` (3.3 V) | `VOLUME.p1` |
| `A32_VDC_VOLUME.p2` | GND | `VOLUME.p3` |

Shield: the `A32_VOL` cable's shield and foil are terminated to GND at the A32 end, and left open at the pot end.
Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the Bluetooth LED/button cable

A 4-pin cable carrying both the Bluetooth pairing button (SW2) and its LED (D29) between the A32 and the front
panel.

| From | Signal | To |
|---|---|---|
| `A32_BT_LED.p1` | `A_3V3` (LED anode supply) | `BT_LED.p2` |
| `A32_BT_LED.p2` | A32 GPIO33 (LED cathode, active low) | `BT_LED.p1` |
| `A32_BT_LED.p3` | GND (button return) | `BT_LED.p4` |
| `A32_BT_LED.p4` | A32 GPIO25 (button, `INPUT_PULLUP`) | `BT_LED.p3` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the I2S cable

A 7-pin cable from the A32 that fans out to the ADC's 6-pin I2S connector (`ADC_I2S`) and the DAC (U9)'s 6-pin I2S
connector (`DAC_I2S`); some signals reach only one of the two.

| From | Signal | To |
|---|---|---|
| `A32_I2S.p1` | `A_IO0D` (DAC clock branch, A32 GPIO0 through its own 33 Ω) | `DAC_I2S.p1` only |
| `A32_I2S.p2` | A32 GPIO4 (DAC data) | `DAC_I2S.p3` only |
| `A32_I2S.p3` | `A_IO0A` (ADC clock branch, A32 GPIO0 through its own 33 Ω) | `ADC_I2S.p6` only |
| `A32_I2S.p4` | A32 GPIO17 (shared word clock) | `ADC_I2S.p4` and `DAC_I2S.p4` |
| `A32_I2S.p5` | shield/foil only | open at both converters |
| `A32_I2S.p6` | A32 GPIO18 (shared bit clock) | `ADC_I2S.p2` and `DAC_I2S.p2` |
| `A32_I2S.p7` | A32 GPIO19 (ADC data) | `ADC_I2S.p1` only |

Shield: the cable's shield and foil are terminated to GND at the A32 end, and left open at both converter ends.
Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the DAC power cable

A 2-pin cable from the 5 V distribution bus directly to the DAC (U9)'s I2S connector (the ADC's connector carries
no power from this cable; it has its own separate power cable below).

| From | Signal | To |
|---|---|---|
| bus `5VDC_DAC.p1` | GND | `DAC_I2S.p5` |
| bus `5VDC_DAC.p2` | `5VDC` | `DAC_I2S.p6` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the ADC power cable

A 2-pin cable from the 5 V distribution bus to the ADC module's own 2-pin power header.

| From | Signal | To |
|---|---|---|
| bus `5VDC.p2` | `5VDC` | `ADC_5VIN.p1` |
| bus `5VDC.p1` | GND | `ADC_5VIN.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the RTC I2C cable

A 3-pin cable from the A32 to the RTC module (U8)'s 4-pin I2C header (two of the header's four positions are not
connected).

| From | Signal | To |
|---|---|---|
| `A32_RTC.p1` | A32 GPIO22 (`SDA`) | `RTC_I2C.p3` (of 4) |
| `A32_RTC.p2` | shield/foil only | open at the RTC end |
| `A32_RTC.p3` | A32 GPIO21 (`SCL`) | `RTC_I2C.p4` (of 4) |

`RTC_I2C.p1` and `.p2` are not connected to anything. "4th position" here counts the header's positions from one
end; the drawing's own contact order does not settle which physical pin that is, and contact order inside a
connector is not covered by this document.

Shield: the cable's shield and foil are terminated to GND at the A32 end, and left open at the RTC end. Figure: the
cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the RTC power cable

A 2-pin cable carrying the A32's `A_VDC` supply to the RTC module (U8)'s 2-pin power header.

| From | Signal | To |
|---|---|---|
| A32 `A_VDC.p1` | `A_VDC` (3.3 V) | `RTC_3V3.p1` |
| A32 `A_VDC.p2` | GND | `RTC_3V3.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the A32 5 V supply cable

A 2-pin cable from the 5 V distribution bus to the A32 board's own 5 V input.

| From | Signal | To |
|---|---|---|
| bus `5VDC.p1` | GND | `A32_5VIN.p1` |
| bus `5VDC.p2` | `5VDC` | `A32_5VIN.p2` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the A32 3.3 V output cable

A 2-pin cable carrying the A32's own regulated 3.3 V output to the 3.3 V distribution bus.

| From | Signal | To |
|---|---|---|
| A32 `A32_VDC.p1` | GND | `A32_3V3.p2` |
| A32 `A32_VDC.p2` | `A_3V3` (3.3 V) | `A32_3V3.p1` |

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the source-switch cables

Two cables from the A32 meet at the front-panel source switch's relay board (`LISTENER`, as named on the cable
sheet): a 3-pin cable carrying the sense ladder's two legs and a shield, and a 2-pin cable carrying `A_VDC`.

| From | Signal | To |
|---|---|---|
| `A32_SWITCH.p1` | `A_IO36_L` (0.8 V leg) | `LISTENER.p1` |
| `A32_SWITCH.p2` | `A_IO36_H` (3.3 V leg) | `LISTENER.p2` |
| `A32_SWITCH.p3` | shield/foil only | open at the `LISTENER` end |
| `A32_VDC_LISTENER.p1` | `A_VDC` (3.3 V) | `LISTENER.p3` |
| `A32_VDC_LISTENER.p2` | not connected | — |

Shield: the `A32_SWITCH` cable's shield and foil are terminated to GND at the A32 end, and left open at the
`LISTENER` end. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the DAC-to-amplifier cable

A 3-signal cable (plus a ground return) from the DAC's output jack (J43) to the amplifier's balanced input, with a
470 Ω resistor in each of the four resulting signal legs and six ferrite cores (described in `hardware-bible.md`).

| From | Signal | To |
|---|---|---|
| DAC `OUTL` / `DAC_OUT.T` | `AMP_L+`, through a 470 Ω resistor at the DAC end | `AMP_IN.p3` (of 9) |
| DAC `OUTR` / `DAC_OUT.R` | `AMP_R+`, through a 470 Ω resistor at the DAC end | `AMP_IN.p7` (of 9) |
| DAC `AGND` / `DAC_OUT.S` | `AMP_G`, and the common point of both `−`-leg 470 Ω resistors | `AMP_IN.p5` (of 9) |
| (from `AMP_G`) | `AMP_L-`, through the other end of its 470 Ω resistor | `AMP_IN.p1` (of 9) |
| (from `AMP_G`) | `AMP_R-`, through the other end of its 470 Ω resistor | `AMP_IN.p9` (of 9) |

`AMP_IN` positions 2, 4, 6 and 8 are not connected.

Shield: none beyond the DAC jack's own sleeve (`AGND`), which is the signal return itself, not a separate shield
conductor. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the radio-to-ADC cable

The cable carrying the tube radio's own audio output to the ADC's input jack (J51), with two clip-on ferrite cores
(described in `hardware-bible.md`).

| From | Signal | To |
|---|---|---|
| tube radio transformer secondary (T1), one leg | `RAD_L` (tip/ring interchangeable with `RAD_R`) | ADC input jack, ring |
| tube radio transformer secondary (T1), other leg | `RAD_R` | ADC input jack, tip |
| — | shield/foil, carried on the plug's sleeve | ADC input jack, sleeve (GND) |

Shield: the cable's shield and foil are carried on the sleeve of the plug at the ADC end. Figure: the cable sheet
(`Ambersong - CABLES.kicad_sch`), also in the PDF export.

## Ambersong hardware: the colon LED power cable

A 2-pin cable from the 5 V distribution bus to the clock display's colon module (COLON1).

| From | Signal | To |
|---|---|---|
| bus `5VDC.p2` | `5VDC` | `COLON_IN.p2` |
| bus `5VDC.p1` | GND | `COLON_IN.p1` |

**Caution:** this cable's polarity is reversed at the `COLON_IN` connector compared with every other 5 V cable in
the device — check polarity before plugging it in.

Shield: none. Figure: the cable sheet (`Ambersong - CABLES.kicad_sch`), also in the PDF export.
