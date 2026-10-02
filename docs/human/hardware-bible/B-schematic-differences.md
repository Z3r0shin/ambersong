# Appendix B. Where the schematics differ from this book

You do not need the KiCad drawings to use this book. This appendix is for readers who also open them:
read it before you compare a drawing with the machine or with this book. The author drew the machine to
show a person what was built, not to make a circuit board. So a drawing sometimes places a part where it
reads best rather than where it sits. **Where a drawing and this book differ, this book is right.**

## B.1 Drawing errors still in the published sheets

Two known errors are left in the drawings on purpose. Both are wrong names on library symbols, and
neither changes any wiring. The table gives what the drawing shows and what is in the machine.

| On the drawing | In the machine |
|---|---|
| The main board's module is drawn with Espressif's **ESP32-S3-WROOM-2** library symbol, with the value "ESP32-S3-WROOM-1_N16R8". | The fitted module is marked **ESP32-S3-N16R8** (chapter 4, section 4.1). |
| The tube radio's mode-switch units (S1A to S1G) use a library symbol named **TM-838M_UNIT1**. | The set is a **TM-838N**. |

**Do not take the WROOM-2's datasheet for this board.** It describes the module named by the symbol,
not the fitted one. That module:

- carries only the ESP32-S3R8V or ESP32-S3R16V chip, with octal flash on every variant;
- runs GPIO47 and GPIO48 at 1.8 V;
- has its pins 28 to 30 not connected, so it has no IO35 to IO37 pads, while the drawn symbol has pins
  35, 36 and 37.

Appendix A gives the figures that do apply.

## B.2 Parts drawn where they read best

On the main sheet, these parts are drawn for clarity, not where they sit. The sub-sheet or the cable
sheet shows the real place. The table gives where each is drawn and where it really is.

| On the main sheet | In the machine |
|---|---|
| The source switch's 22 kΩ and 68 kΩ resistors, drawn in the front switch's block | on the audio board (chapter 9, section 9.3) |
| The 0.1 µF filter capacitor on GPIO36, drawn at the ladder side of the 1 kΩ | on the pin side, at the audio board |
| The colon's 33 Ω and its 1.5 kΩ variable resistor, both drawn on the 5 V side of the colon LED | 5 V → 33 Ω → colon → variable resistor → ground, as the display sub-sheet draws it. The variable resistor is glued at 1.5 kΩ (chapter 7, section 7.6). |
| The four 470 Ω resistors of the DAC-to-amplifier link, drawn on the DAC's output pins | in the cable, as the cable sheet draws them: the two plus legs' at the DAC end, the two minus legs' in the cable, at an end not recorded (chapter 6, section 6.6; chapter 10, section 10.21) |

## B.3 Reading notes

**Names and symbols**

- **Rails with two names** are one conductor: `5VDC_S` and `5VDC`; `PSU_GND` and `GND`; `A_VDC` and
  `A_3V3` (About this book, *Notation*).
- **`A_IO0A` and `A_IO0D`** are both the audio board's GPIO0, named twice so each converter's wiring
  reads clearly. The two clock contacts of the audio harness both carry GPIO0; neither belongs to one
  converter.
- **One label on several wires.** On the Bluetooth and source-switch sheet, "GPIO" names both the H and
  the L leg. On the ADC sheet, "ATTENUATOR" names both the ring and the tip of the radio input. On the
  USB-C sheet, one "TO … USB-C" name sits on all four wires of each port (VBUS, D−, D+ and ground). These
  are visual aids, not shorts. Each is a separate wire, and this book describes each one separately.
- **`GND` twice on one connector** (the digit-rail cable's 2-pin connector) is the cable's shield and
  foil, ending on the driver board's common ground (chapter 10, section 10.27).
- **`20V_GND`** is drawn on the main sheet for completeness only. The sheet's note beside it reads: "The
  AMP PSU '20V_GND' is shown for completeness only; the actual path goes to the AMP's -20V and +20V
  inputs." The amplifier supply's four wires, as recorded, are in chapter 5, section 5.6, and chapter 10,
  section 10.30.
- **The real-time clock** is drawn with a DS3231M symbol. The fitted clock is a DS3231 module of the
  ZS-042 style (chapter 4, section 4.7). The symbol's eight ground pins are hidden on the drawing,
  and lie on the main sheet's `A_VDC` wire. They belong to the drawn symbol only and are disregarded:
  they are not a connection between 3.3 V and ground. The module's ground comes on its power cable
  (chapter 10, section 10.16).
- **The clock module's I2C connector** is drawn with pins 1 and 2 not connected, pin 3 on SDA and pin 4
  on SCL. That is right: a 4-pin socket with two pins used.

**References**

- **The radio input jack at the ADC** (`RADIO_TO_ADC`) is J51, as on the main sheet; the ADC's own sheet
  numbers it J42. This book uses J51.
- **That jack's output side.** The ADC's sheet labels both its ring and its tip ATTENUATOR. As fitted,
  the ring is on ground with the sleeve, and only the tip feeds the 4.7 kΩ (chapter 6, section 6.4).
- **The DAC's output plug**, named `DAC_OUT` on both, is J49 on the cable sheet and J40 on the DAC sheet.
- **The same reference on two sheets** can mean two different parts: C13 to C30, J2 to J4, R17 and M1,
  for example, are used both on the main sheet and in the tube radio. Chapter 13, section 13.1.7,
  explains how this book keeps them apart.

**What to ignore**

- **The TL074 circuit off the page** of the main sheet is a draft of the amplifier's input board, drawn
  to help visualise it. Disregard it.
- **Unlabelled rectangles over modules** are the printed shells that hold or cover them.

**What a drawing does and does not tell you**

- **Parts drawn only on the main sheet are fitted.**
- **Capacitors drawn as polarised** are fitted the right way round as drawn; the others are film or
  ceramic. In the tube radio's polarised capacitors (C1A to C1C and C2), pin 1 is the + terminal.
- **Pin numbers on cable connectors** count along the drawn symbol. They are not contact numbers, and a
  sub-sheet sometimes orders a connector's pins differently from the cable sheet. This book prints the
  cable sheet's order. Neither order says anything about the real connector (chapter 10, section 10.1).

**The tube radio sheet**

- **The FM IF is 10.6 MHz.** The sheet's "10.7 MHz FM IF Path" section title and the fields of the first
  two FM IF transformers (L10, L12) read 10.7 MHz, the design value; the third, the ratio-detector
  transformer (L14), is drawn in its own section but its field reads 10.7 MHz too. The fitted IF is
  10.6 MHz for all three (L10, L12 and L14), set when Radio Hosvep on Park Ave. professionally tuned the
  FM path (chapter 13, section 13.1.1).
- **Terminal numbers are logical.** The identifiers on the coils L5, L6 and L8 to L14 and on the mode
  switch's units S1A to S1G name connections, not physical lugs.
- **One part, several units.** A tube is drawn as separate units: its signal sections in their circuits,
  its heater on the heater string. S1A to S1G are seven drawn units of one mode switch.
- **Three-lead capacitors** (C5, C11, C14, C48) are drawn as two-terminal capacitors. On the part, the
  middle lead goes to the chassis and the two outer leads read 0 Ω to each other (chapter 13, section 13.1.3).
- **"50C5" on the output transformer's primary** means the radio's output stage.
- **The speakers SP1 and SP2** are the radio's original speakers as the factory drew them. In this
  machine the radio's output feeds the 7.5 Ω load and the ADC, not speakers.
- **Drawn but absent:** the neon lamps NE1 to NE3 and the external SW antenna terminal (J2)
  are not in this radio.
- **Values and specifications.** A part's value on the sheet is its fitted value. Its hidden
  specification fields hold the original factory and SAMS values; chapter 13 prints both.
- **Notes that point elsewhere.** The sheet's notes that send you to "the audio sheet" or "the Main
  Sheet" are left as written, because the sheet is usually one of a larger set.
