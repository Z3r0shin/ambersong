# Ambersong — Hardware Bible

## 0. About this document

**What Ambersong is.** A vintage vacuum tube radio, a LLOYDS TM-838N AM/SW/FM set, connected with ESP32 audio and
control. Two ESP32 boards sit beside the tube chassis. The main MCU, an ESP32-S3, drives the LED clock display, the
dial-needle stepper and its sensors, the RDA5807M tuning calibrator and the panel lamps. The audio MCU, an ESP32 called
"A32" throughout, digitises the tube radio's output through a PCM1802 ADC, feeds a PCM5102A DAC to an external amplifier,
and reads the front-panel source switch (RADIO / BT / AUX), the Bluetooth button and LED, a volume pot and a DS3231 RTC.

**What this document covers.** The hardware: what joins what in the machine and in the tube radio inside it. For the
tube radio it also gives the fitted values and the original specifications held in the sheet's hidden Spec fields. The FM
antenna as built is §29; what the amp and its speakers are, and the amp's input panel, are §30; the physical build (the
cabinet, access, the front panel, the tuning and needle mechanics, the Faraday cages) is §31, mentioned rather than detailed.
This is not a step-by-step build guide: it does not cover wire colours (except the amp supply's four wires, §30), pin-1
sides, crimp/JST housings, individual cable lengths or routing, the contact order inside a connector, or the amp's circuit. Firmware is documented in `FIRMWARE-GOSPEL.md`. This document takes only five pin facts from the
firmware, listed below.

**How to read it with the Gospel and the sheets.** The KiCad sheets in `hardware/schematics/` are visual aids. The author
uses KiCad to show a human what was built, not to make a PCB, and some drawings place a part where it reads best rather
than where it sits (§22 says where). This document states the fitted hardware. Where a sheet and this document differ,
this document is right, and §25 lists the known drawing errors: all are corrected in the published sheets except two
typos. `FIRMWARE-GOSPEL.md` describes what the firmware does with this hardware. A firmware comment or define that
describes wiring is not a hardware record: where one disagrees with this document, the comment is stale (§26 lists the
known ones).

**Order of truth.** For the machine: the fitted hardware first, then the main sheet, then the sub-sheets. For the tube
radio: the fitted hardware first, then the factory schematic (`hardware/schematics/LLOYDS TM-838 factory schematic.jpg`),
then the SAMS Photofact for the set (a published, copyrighted service document, not included here). The tube radio sheet
reflects the fitted hardware, except for a few details noted on the sheet itself. Datasheet facts (§24) are quoted and
marked, and they describe the datasheet part, which is not always the fitted one.

**Pin facts taken from the firmware.** Five facts come from the firmware, not from the drawings, because on these points
the firmware is exact: the digit order, the digit MOSFET part, the Bluetooth button and LED pins, the volume pot wiper
pin, and the positions of the front DPDT source switch. The lines are quoted as they read in the firmware on 2026-09-12, before v.1.0 (paths under `firmware/`). Some of their comments have since been reworded or removed; the code on these lines and every pin number are unchanged.

- Digit order; line 42 is a comment asking for re-confirmation, written before the order was confirmed:
  - `include/pins.h` — `//  exactly one digit lights at a time. To be re-confirmed on the next run.`
  - `include/pins.h` — `#define S3_DIGIT_1         42      // was 2`
  - `include/pins.h` — `#define S3_DIGIT_2          2      // was 42, confirmed: this pin lights digit 2`
  - `include/pins.h` — `#define S3_DIGIT_3         40      // was 41`
  - `include/pins.h` — `#define S3_DIGIT_4         41      // was 40, confirmed: this pin lights digit 4`
- Digit MOSFET part FQP27P06; a comment:
  - `include/pins.h` — `//  S3 -> digit-driver ULN2003 input -> 1k -> FQP27P06 gate (high side).`
- BT button GPIO25 and BT LED GPIO33, both active low:
  - `include/pins.h` — `#define A32_BT_BUTTON      25      // ACTIVE LOW, ground-referenced, INPUT_PULLUP`
  - `include/pins.h` — `#define A32_BT_LED         33      // sinks; anode on 3V3, so drive LOW to light`
  - `src/a32/main.cpp` — `  pinMode(A32_BT_BUTTON, INPUT_PULLUP);      // ACTIVE LOW, measured`
  - `src/a32/main.cpp` — `  bool down = (digitalRead(A32_BT_BUTTON) == LOW);`
  - `src/a32/btled.cpp` — `  ledcWrite(LED_CH, 255 - v);         // ACTIVE LOW`
  - `src/a32/btled.cpp` — `  ledcAttachPin(A32_BT_LED, LED_CH);`
- Volume pot wiper GPIO35:
  - `include/pins.h` — `#define A32_VOLUME_POT     35`
  - `src/a32/main.cpp` — `  for (int i = 0; i < 5; i++) { v[i] = analogRead(A32_VOLUME_POT); delayMicroseconds(150); }`
  - `src/a32/main.cpp` — `    uint16_t r = analogRead(A32_VOLUME_POT);`
- Source select on GPIO36: below 424 counts AUX, 2471 and up RADIO, between BT:
  - `include/pins.h` — `#define A32_MODE_ADC       36`
  - `src/a32/main.cpp` — `  for (int i = 0; i < 5; i++) { v[i] = analogRead(A32_MODE_ADC); delayMicroseconds(200); }`
  - `src/a32/main.cpp` — `  raw = v[2];`
  - `include/pins.h` — `#define A32_MODE_THRESH_AUX_BT     424   // raw below this          -> AUX`
  - `include/pins.h` — `#define A32_MODE_THRESH_BT_RADIO  2471   // raw above this          -> RADIO`
  - `src/a32/main.cpp` — `  if (raw <  A32_MODE_THRESH_AUX_BT)   return SRC_AUX;`
  - `src/a32/main.cpp` — `  if (raw >= A32_MODE_THRESH_BT_RADIO) return SRC_RADIO;`
  - `src/a32/main.cpp` — `  return SRC_BT;`
- The ladder, and the level each source reads, in comments that are exact for the switch positions (which leg gives which level follows from the drawn 22 kΩ and 68 kΩ):
  - `include/pins.h` — `//  The ladder itself is exactly as drawn - A_VDC through the switch, one leg`
  - `include/pins.h` — `//  direct and one through 68k, joining a single 22k to GND, then 1k + 0.1uF at`
  - `include/pins.h` — `//  the pin. Levels land at 3.3 V / 0.807 V / 0 V as designed.`
  - `include/pins.h` — `//        measured   AUX = 0        BT = 848 (0.81 V)   RADIO = 4095 (3.3 V)`

**How it was made.** A script read the saved `.kicad_sch` source text (never the netlist, never a picture for text) and
computed connectivity. Every item was then looked up again in the source and checked against the author's knowledge of
the fitted hardware.

**Where the drawings are wrong.** §25 lists the drawing errors found while this document was made, and how each was
corrected. If a drawing disagrees with this document, check §25 first.

**Conventions.**
- Coordinates are sheet millimetres. The author often types a value into a part's Reference field, so a part may be
  named by its value plus its position: `10kΩ@21.59,140.97`.
- Pins: `#1` is a pin number; `VCC`, `SCK`, `A`, `K` are pin names. The MCU symbols name their pins by GPIO number
  (U7 pin `4` is GPIO4). `p2/3` is the 2nd of 3 pins counted top-to-bottom in the library symbol, used when a symbol
  has neither name nor number (the JST connectors, the PC817). `p`-order is not a moulded contact number.
- A label name (`GPIO4`, `A_IO22`) is how the drawings refer to one conductor. A net can carry two names, a global
  label and a local label joined by a wire; both name that conductor.
- `A_VDC` = `A_3V3`: the same rail, joined by the bus. `S3_3V3` is the S3's 3.3 V. `5VDC_S` = `5VDC`.
- `CABLE S/FTP` on a connector pin: the cable's shield and foil are terminated there, to GND on the board side, and
  left open at the cable's other end, unless noted otherwise.
- Source lines name the drawing (the main sheet, the cable sheet, or `sheet <title>`; the files are listed below);
  "the author" where the fact rests on the author's knowledge of the fitted hardware rather than on a drawing; and SCn
  for a drawing correction in §25. "Label names across all drawings" means the label name was looked up on every
  drawing. Line numbers inside the `.kicad_sch` files and net numbers are left out, because every save moves them.
- `block B47`, `note B29`, `drawn blocks B6 and B7`: the rectangles drawn on a sheet, numbered B1, B2, … by position — top edge
  first, rounded to the millimetre, then left edge — as read when this document was made (a later save may renumber them); a
  text note carries the number of the smallest rectangle around it. The numbers are not printed on the drawings. The ones
  this document cites, by corners in sheet mm: the main sheet B29 (12.70,38.10)-(71.75,43.18), B30 (12.70,43.18)-(71.75,48.26), B31 (12.70,48.26)-(71.75,53.34), B47 (69.22,93.98)-(107.31,125.73); sheet HERE (the star-point buses) B5 (130.18,76.83)-(139.06,119.38), B6 (159.38,76.83)-(168.28,119.38), B7 (173.35,76.83)-(182.25,119.38).
  `G1` to `G9` name this document's nine Ambersong groups (§4 to §12); `T1` to `T9` name the tube radio groups (§13 to §21).
- Tube radio: a tube is drawn as units of one tube (its signal sections in their circuit, its heater on the heater string);
  S1A–S1G are seven drawn units of one mode switch; the terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not
  physical lugs. `CHASSIS` is the radio's circuit common, on mains neutral. In the polarized capacitors C1A–C1C and C2,
  pin #1 is the + terminal (read from the symbol embedded in the sheet).

**The drawings**, in `hardware/schematics/` (as saved on 2026-09-23):

| Named in this document | File |
|---|---|
| the main sheet | `Ambersong.kicad_sch` |
| the cable sheet | `Ambersong - CABLES.kicad_sch` |
| sheet 7 SEGMENTS 24H CLOCK DISPLAY | `Ambersong - 7 SEGMENTS 24H CLOCK DISPLAY.kicad_sch` |
| sheet AUDIO MCU ESP32-WROOM-32 | `Ambersong - AUDIO MCU ESP32-WROOM-32.kicad_sch` |
| sheet BLUETOOTH PAIR BUTTON & FRONT PANEL BLUETOOTH LED & DPDT ON-OFF-ON AUDIO SWITCH | `Ambersong - BLUETOOTH PAIR BUTTON & FRONT PANEL BLUETOOTH LED & DPDT ON-OFF-ON AUDIO SWITCH.kicad_sch` |
| sheet DS3231 RTC | `Ambersong - DS3231 RTC.kicad_sch` |
| the tube radio sheet | `Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch` |
| sheet MAIN MCU ESP32-S3 N16R8 | `Ambersong - MAIN MCU ESP32-S3 N16R8.kicad_sch` |
| sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER | `Ambersong - P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER.kicad_sch` |
| sheet PANEL RADIO LED PWM CONTROL | `Ambersong - PANEL RADIO LED PWM CONTROL.kicad_sch` |
| sheet PCM1802 ADC | `Ambersong - PCM1802 ADC & TUBE RADIO AUDIO JACK IN INTO PCM1802.kicad_sch` |
| sheet PCM5102A DAC | `Ambersong - PCM5102A DAC.kicad_sch` |
| sheet HERE (the star-point buses) | `Ambersong - STAR POINT VDC & GND JST XH BUSES.kicad_sch` (its box on the main sheet reads HERE) |
| sheet Power Detect | `Ambersong - POWER DETECT.kicad_sch` |
| sheet RDA5807M | `Ambersong - RDA5807M.kicad_sch` |
| sheet STEPPER DRIVER | `Ambersong - STEPPER DRIVER.kicad_sch` |
| sheet STEPPER LIMIT | `Ambersong - STEPPER LIMIT.kicad_sch` |
| sheet USB-C CONNECTORS | `Ambersong - USB-C CONNECTORS.kicad_sch` |
| sheet BOX FAN | `Ambersong - BOX FAN.kicad_sch` |

## 1. The two MCU boards

> Board markings as read by the author. Chip data is esptool output, read over USB on 2026-09-13. Each query reset its
> board into the ROM bootloader and back (`Hard resetting via RTS pin...`). Read-only: nothing was written. The boards'
> MAC addresses are masked here.

### Main MCU — ESP32-S3 devkit

Both boards are devkits. Module tin: "ESP32-S3-N16R8 FCC ID: 2AD6X-ESP32S3N16R8 IC: 29018-ESP32S3N16R8 CMIT ID: 2022DP2107". Board: "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3". The board's RGB LED is on GPIO48.

**USB:** `USB VID:PID=303A:1001 SER=xx:xx:xx:xx:xx:xx` on COM9 (`pio device list`, 2026-09-13).

**esptool.py v4.9.0, `--port COM9 flash_id`, exit 0:**
```
esptool.py v4.9.0
Serial port COM9
Connecting...
Detecting chip type... ESP32-S3
Chip is ESP32-S3 (QFN56) (revision v0.2)
Features: WiFi, BLE, Embedded PSRAM 8MB (AP_3v3)
Crystal is 40MHz
USB mode: USB-Serial/JTAG
MAC: xx:xx:xx:xx:xx:xx
Uploading stub...
Running stub...
Stub running...
Manufacturer: 68
Device: 4018
Detected flash size: 16MB
Flash type set in eFuse: quad (4 data lines)
Flash voltage set by eFuse to 3.3V
Hard resetting via RTS pin...
```

### Audio MCU (A32) — ESP32 devkit

The A32 board is marked "FCC IDL 2BB77-ESP32-32X". It is meant to carry an ESP32-WROOM-32E module.

**USB:** `USB VID:PID=10C4:EA60 SER=0001 LOCATION=1-5.4.4`, "Silicon Labs CP210x USB to UART Bridge", on COM8 (`pio device list`, 2026-09-13).

**esptool.py v4.9.0, `--port COM8 flash_id`, exit 0:**
```
esptool.py v4.9.0
Serial port COM8
Connecting....
Detecting chip type... Unsupported detection protocol, switching and trying again...
Connecting......
Detecting chip type... ESP32
Chip is ESP32-D0WD-V3 (revision v3.1)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: xx:xx:xx:xx:xx:xx
Uploading stub...
Running stub...
Stub running...
Manufacturer: 5e
Device: 4016
Detected flash size: 4MB
Flash voltage set by a strapping pin to 3.3V
Hard resetting via RTS pin...
```

### Not yet established

- Whether the S3 module and the A32 module match any Espressif module datasheet. Espressif's FCC grantee code is 2AC7Z (per an esp32.com thread); the S3 module's 2AD6X is a different code; the A32's 2BB77 has not been looked up. Each datasheet fact in this document says what it describes: the chips as read above, or an Espressif module or devkit document.
- The flash chip makers behind manufacturer IDs 0x68 and 0x5e — not looked up.

## 2. Pin map — Main MCU (ESP32-S3), U7 on the main sheet

One row per module pin as drawn on the main sheet, in drawn order, with the author's statements about the fitted hardware applied.

| Pin | Signal | Goes to | Source |
|---|---|---|---|
| 3V3 (ord 1, left, 30.48,138.43) | S3_3V3 | C23 0.1uF and C25 10uF (return to U7 GND ord 2); 10k pull-ups on GPIO4/GPIO5; by label: U89.VDD with C28 (RDA5807M, main), U11.VCC (AS5600, main), S3_TUNER.p2 and S3_3V3 connector (S3 sub-sheet and cable sheet), RDA_3V3.p2 (RDA5807M sub-sheet and cable sheet) | the main sheet; label names across all drawings; SC16 |
| GND (ord 2, right, 54.61,138.43) | unnamed net | C23.#2, C25.#2 only | the main sheet; the author; SC16 |
| 3V3 (ord 3, left, 30.48,140.97) | no-connect | nothing | the main sheet |
| 43 | no-connect | nothing | the main sheet |
| RST | no-connect | nothing | the main sheet |
| 44 | no-connect | nothing | the main sheet |
| 4 | GPIO4 | 10kΩ to S3_3V3 and 0.1uF to GND on the main sheet; no other GPIO4 label on any drawing; S3 sub-sheet: U70 LIMITS header p2, jack p2 no-connect. Still wired to the LIMITS JST female, goes nowhere else | the main sheet; label names across all drawings; sheet MAIN MCU ESP32-S3 N16R8; the author |
| 1 | GPIO1 | 10kΩ pull-up; U6 PC817 p2/4 (collector) (main); Power-Detect sub-sheet U83.p2/4 (collector), U84.p2, POWER_DET.p2; cable sheet POWER_DET.p2 and S3_PWR_DET.p1; S3 sub-sheet S3_PWR_DET.p1 | the main sheet; label names across all drawings |
| 5 | GPIO5 | 10kΩ pull-up and 0.1uF; U42.OUT A3144 (main); cable sheet LIMIT.p3 and S3_LIMITS.p3; STEPPER LIMIT sub-sheet U90.p1, LIMIT.p3; S3 sub-sheet S3_LIMITS.p3 | the main sheet; label names across all drawings; the author |
| 2 | GPIO2 (digit 2 per SC4) | U5.I2 (main); cable sheet DIGIT_IN.p2, S3_DIGIT.p3; P-MOSFET sub-sheet U29.p2, DIGIT_IN.p2; S3 sub-sheet S3_DIGIT.p4 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings; SC4; the author |
| 6 | GPIO6 | U2.I1 (main); cable sheet DISPLAY_IN.p1, S3_DISPLAY.p1; P-MOSFET sub-sheet U24.p1; S3 sub-sheet S3_DISPLAY.p1 | the main sheet; label names across all drawings |
| 42 | GPIO42 (digit 1 per SC4) | U5.I1 (main); cable sheet DIGIT_IN.p1, S3_DIGIT.p4; P-MOSFET sub-sheet U29.p1, DIGIT_IN.p1; S3 sub-sheet S3_DIGIT.p3 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings; SC4; the author |
| 7 | GPIO7 | U2.I2 (main); cable sheet DISPLAY_IN.p2, S3_DISPLAY.p2; P-MOSFET sub-sheet U24.p2; S3 sub-sheet S3_DISPLAY.p2 | the main sheet; label names across all drawings |
| 41 | GPIO41 (digit 4 per SC4) | U5.I4 (main); cable sheet DIGIT_IN.p4, S3_DIGIT.p1; P-MOSFET sub-sheet U29.p4, DIGIT_IN.p4; S3 sub-sheet S3_DIGIT.p2 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings; SC4; the author |
| 15 | GPIO15 | U2.I3 (main); cable sheet DISPLAY_IN.p3, S3_DISPLAY.p3; P-MOSFET sub-sheet U24.p3; S3 sub-sheet S3_DISPLAY.p3 | the main sheet; label names across all drawings |
| 40 | GPIO40 (digit 3 per SC4) | U5.I3 (main); cable sheet DIGIT_IN.p3, S3_DIGIT.p2; P-MOSFET sub-sheet U29.p3, DIGIT_IN.p3; S3 sub-sheet S3_DIGIT.p1 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings; SC4; the author |
| 16 | GPIO16 | U2.I4 (main); cable sheet DISPLAY_IN.p4, S3_DISPLAY.p4; P-MOSFET sub-sheet U24.p4; S3 sub-sheet S3_DISPLAY.p4 | the main sheet; label names across all drawings |
| 39 | pin node → 220Ω → GPIO39 | 10kΩ pull-up to S3_3V3 at the pin, then 220Ω@63.50,158.75, then U11.SCL AS5600 (main); cable sheet S3_TUNER.p4; S3 sub-sheet S3_TUNER.p4 | the main sheet; label names across all drawings; the author |
| 17 | GPIO17 | U2.I5 (main); cable sheet DISPLAY_IN.p5, S3_DISPLAY.p5; P-MOSFET sub-sheet U24.p5; S3 sub-sheet S3_DISPLAY.p5 | the main sheet; label names across all drawings |
| 38 | pin node → 220Ω → GPIO38 | 10kΩ pull-up to S3_3V3 at the pin, then 220Ω@58.42,166.37, then U11.SDA AS5600 (main); cable sheet S3_TUNER.p3; S3 sub-sheet S3_TUNER.p3 | the main sheet; label names across all drawings; the author |
| 18 | GPIO18 | U2.I7 (main); cable sheet DISPLAY_IN.p7 and S3_DISPLAY.p7; P-MOSFET sub-sheet U24.p7; S3 sub-sheet S3_DISPLAY.p6 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings |
| 37 | no-connect | nothing | the main sheet |
| 8 | GPIO8 | U2.I6 (main); cable sheet DISPLAY_IN.p6 and S3_DISPLAY.p6; P-MOSFET sub-sheet U24.p6; S3 sub-sheet S3_DISPLAY.p7 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings |
| 36 | no-connect | nothing | the main sheet |
| 3 | no-connect | nothing | the main sheet |
| 35 | no-connect | nothing | the main sheet |
| 46 | no-connect | nothing | the main sheet |
| 0 | no-connect | nothing | the main sheet |
| 9 | GPIO9 | U3.I1 (main); cable sheet S3_STEPPER.p4, STEPPER_IN.p1; STEPPER DRIVER sub-sheet U38.p1; S3 sub-sheet S3_STEPPER.p4 | the main sheet; label names across all drawings |
| 45 | no-connect | nothing | the main sheet |
| 10 | GPIO10 | U3.I2 (main); cable sheet S3_STEPPER.p3, STEPPER_IN.p2; STEPPER DRIVER sub-sheet U38.p2; S3 sub-sheet S3_STEPPER.p3 | the main sheet; label names across all drawings |
| 48 | GPIO48 | U89.SCLK RDA5807M (main); RDA5807M sub-sheet RDA_I2C.p2, U92.p2; cable sheet RDA_I2C.p2 and S3_RDA.p1; S3 sub-sheet S3_RDA.p2 (the cable sheet orders these contacts differently; contact order is out of scope); also the S3 board's RGB LED | the main sheet; label names across all drawings; the author |
| 11 | GPIO11 | U3.I3 (main); cable sheet S3_STEPPER.p2, STEPPER_IN.p3; STEPPER DRIVER sub-sheet U38.p3; S3 sub-sheet S3_STEPPER.p2 | the main sheet; label names across all drawings |
| 47 | GPIO47 | U89.SDIO RDA5807M (main); RDA5807M sub-sheet RDA_I2C.p1, U92.p3; cable sheet RDA_I2C.p1 and S3_RDA.p2; S3 sub-sheet S3_RDA.p1 (the cable sheet orders these contacts differently; contact order is out of scope) | the main sheet; label names across all drawings; the author |
| 12 | GPIO12 | U3.I4 (main); cable sheet S3_STEPPER.p1, STEPPER_IN.p4; STEPPER DRIVER sub-sheet U38.p4; S3 sub-sheet S3_STEPPER.p1 | the main sheet; label names across all drawings |
| 21 | GPIO21 | 100Ω.#2 in block B47 (main); PANEL RADIO LED PWM sub-sheet 100Ω.#2, U82.p3, RLPC_IN.p1; cable sheet FM_LED_IN.p1 and S3_FM_LED.p2; S3 sub-sheet S3_FM_LED.p2. GPIO21 lands on Q1's gate, through the drawn 100 Ω | the main sheet; label names across all drawings; the author |
| 13 | S3_to_A32 | U10 pin 26 (main); AUDIO MCU sub-sheet U74.26, U80.p1, A32_UART.p1; cable sheet A32_UART.p1 and S3_UART.p2; S3 sub-sheet S3_UART.p2 | the main sheet; label names across all drawings |
| 20 | no-connect on the drawing | GPIO20 is the ESP32-S3's USB_D+ (datasheet, §24). The back-panel USB-C reaches the S3's native USB port (SC24). | the main sheet; SC24 |
| 14 | A32_to_S3 | U10 pin 27 (main); AUDIO MCU sub-sheet U74.27, U80.p2, A32_UART.p2; cable sheet A32_UART.p2 and S3_UART.p3; S3 sub-sheet S3_UART.p3 | the main sheet; label names across all drawings |
| 19 | no-connect on the drawing | GPIO19 is the ESP32-S3's USB_D− (datasheet, §24). The back-panel USB-C reaches the S3's native USB port (SC24). | the main sheet; SC24 |
| 5V (ord 41, left, 30.48,189.23) | 5VDC | 100uF@19.05,191.77 and 0.1uF@22.86,191.77 (return to U7 GND ord 43); the 5VDC rail (power buses); S3 sub-sheet S3_5VIN.p2; cable sheet S3_5VIN.p2 | the main sheet; label names across all drawings; SC16 |
| GND (ord 42, right, 54.61,189.23) | GND | global GND label | the main sheet |
| GND (ord 43, left, 30.48,191.77) | unnamed net | 100uF.#2, 0.1uF@22.86,191.77.#2 only | the main sheet |
| GND (ord 44, right, 54.61,191.77) | no-connect | nothing | the main sheet |

## 3. Pin map — Audio MCU (A32), U10 on the main sheet

One row per module pin as drawn on the main sheet, in drawn order, with the author's statements about the fitted hardware applied.

| Pin | Signal | Goes to | Source |
|---|---|---|---|
| 3V3 | A_3V3 | C20 0.1µF, C21 10µF, C22 10µF (returns to U10 GND ord 2); A_3V3 bus: sheet HERE (the star-point buses) (U52-U56, A32_VDC, A32_3V3, same net as A_VDC); A32 sub A32_3V3 p1 and A32_BT_LED p1; BT sub A32_BT_LED.p2/4 | the main sheet; label names across all drawings; the author; SC17 |
| GND (ord 2, right top) | unnamed net | C20/C21/C22 returns (main); sub: GND label, C54/C55/C56 returns + A32_3V3 cable GND | the main sheet; sheet AUDIO MCU ESP32-WROOM-32 |
| EN | no-connect | — | the main sheet |
| 23 | no-connect | — | the main sheet |
| 36 | A_IO36 via 1 kΩ (A_IO36_H on sub) | 0.1µF to GND at the pin; 1kΩ to the ladder node: 22kΩ to GND, 68kΩ to A_IO36_L, A32_SWITCH p2/3 (THIS A32_SWITCH.p2/3 -> LISTENER.p2/3; BT sub LISTENER/U43/U44 p2/3) to the front DPDT switch; main 22kΩ.#1, 68kΩ.#1, DPDT_ON_OFF_ON.A | the main sheet; label names across all drawings; the author |
| 22 | A_IO22 | U8.SDA (main); RTC module SDA U16/RTC_I2C p3/4; A32_RTC cable; THIS A32_RTC.p1/3, RTC_I2C.p3/4 | the main sheet; label names across all drawings |
| 39 | no-connect | — as drawn; not joined to GPIO34 or GPIO36 on the board now (§26) | the main sheet; the author |
| 1 | no-connect | — | the main sheet |
| 34 | no-connect | — as drawn; not joined to GPIO39 or GPIO36 on the board now (§26) | the main sheet; the author |
| 3 | no-connect | — | the main sheet |
| 35 | A_IO35 (drawn A_IO34 until 2026-09-23; SC6 done) | RV4.2 volume pot wiper (main); A32_VOL cable p1/2 (sub); THIS VOLUME.p2/3 | the main sheet; label names across all drawings; the author; SC6; firmware (§0) |
| 21 | A_IO21 | U8.SCL (main); RTC module SCL U16/RTC_I2C p4/4 as drawn; A32_RTC cable; THIS A32_RTC.p3/3, RTC_I2C.p4/4 as drawn. The RTC_I2C 4th position is not connected (SCV6 done: the 2026-09-23 save draws RTC_I2C with two of its four positions connected, which is right) | the main sheet; label names across all drawings; the author |
| 32 | no-connect | — | the main sheet |
| GND (ord 14, right) | no-connect | — on the main sheet; the sub-sheet puts a local GND label on the pin (since 2026-09-23) | the main sheet; sheet AUDIO MCU ESP32-WROOM-32 |
| 33 | A_IO33 | BT LED D29 cathode (main, anode A_VDC); A32_BT_LED p2/4; THIS BT_LED.p1/4; BT sub LED − (SC3) | the main sheet; label names across all drawings; the author; SC3; firmware (§0) |
| 19 | A_IO19 | PCM1802.DOUT (main); A32_I2S p7/7; THIS ADC_I2S.p1/6; PCM1802 sub U23.p1/6 | the main sheet; label names across all drawings |
| 25 | A_IO25 | BT pair button SW2.1 (main; SW2 pin 2 on GND per SC3); A32_BT_LED p4/4; THIS BT_LED.p3/4; BT sub button (SC3) | the main sheet; label names across all drawings; the author; SC3; firmware (§0) |
| 18 | A_IO18 | PCM1802.BCK and U9 PCM5102A.BCK (main); A32_I2S p6/7; THIS ADC_I2S.p2/6, DAC_I2S.p2/6 | the main sheet; label names across all drawings |
| 26 | S3_to_A32 | S3 U7.13 (main) / U63.13 (S3 sub); A32_UART p1/3; THIS S3_UART.p2/3 | the main sheet; label names across all drawings |
| 5 | no-connect | — | the main sheet |
| 27 | A32_to_S3 | S3 U7.14 (main) / U63.14 (S3 sub); A32_UART p2/3; THIS S3_UART.p3/3 | the main sheet; label names across all drawings |
| 17 | A_IO17 | PCM1802.LRCK and U9.LRCK (main); A32_I2S p4/7; THIS ADC_I2S.p4/6, DAC_I2S.p4/6 | the main sheet; label names across all drawings |
| 14 | no-connect | — | the main sheet |
| 16 | no-connect (unused) | — | the main sheet; the author (GPIO16 unused) |
| 12 | no-connect | — | the main sheet |
| 4 | A_IO4 | U9 PCM5102A.DIN and 100kΩ@116.84,24.13.#1 (main); A32_I2S p2/7; THIS DAC_I2S.p3/6; DAC sub U13.p4/6 | the main sheet; label names across all drawings |
| GND (ord 27, left) | GND | 5V decoupling returns; A32_5VIN cable GND (sub) | the main sheet; sheet AUDIO MCU ESP32-WROOM-32 |
| 0 | GPIO0 node → A_IO0A (ADC clock) and A_IO0D (DAC clock) | Two 33 Ω at the A32 pin, one per converter. Main sheet: 33Ω@109.22,175.26 → A_IO0A → PCM1802.SCK, 33Ω@109.22,180.34 → A_IO0D → U9 PCM5102A.SCK (SC8 done). A32 sub-sheet: 33Ω@167.64,102.87 → A_IO0A → A32_I2S.p3/7, 33Ω@167.64,107.95 → A_IO0D → A32_I2S.p1/7. Cable sheet: A32_I2S.p3/7 = A_IO0A = ADC_I2S.p6/6; A32_I2S.p1/7 = A_IO0D = DAC_I2S.p1/6. | the main sheet; sheet AUDIO MCU ESP32-WROOM-32; the cable sheet; label names across all drawings; the author; SC7; SC8 |
| 13 | no-connect | — | the main sheet |
| 2 | no-connect | — | the main sheet |
| 9 | no-connect | — | the main sheet |
| 15 | no-connect | — | the main sheet |
| 10 | no-connect | — | the main sheet |
| 8 | no-connect | — | the main sheet |
| 11 | no-connect | — | the main sheet |
| 7 | no-connect | — | the main sheet |
| 5V | 5VDC | 0.1µF + 100µF to GND; 5VDC rail via A32_5VIN cable (THIS A32_5VIN.p2/2; sheet HERE (the star-point buses)) | the main sheet; label names across all drawings |
| 6 | no-connect | — | the main sheet |

## 4. Main MCU (ESP32-S3) and the back-panel USB-C

### On the main sheet

- **S3_3V3** — The S3 board's upper 3V3 pin drives the S3_3V3 rail, decoupled by C23 0.1 µF and C25 10 µF, and feeds the GPIO4 and GPIO5 pull-ups.
  - members: U7.3V3 (ord 1, upper left), C23.#1 (0.1uF), C25.#1 (10uF), 10kΩ@21.59,140.97.#1, 10kΩ@27.94,143.51.#1, global label S3_3V3 (22.86,135.89)
  - source: the main sheet; SC16
- **unnamed net** — C23/C25 return to U7's top-right GND pin. The net has no GND label.
  - members: C23.#2, C25.#2, U7.GND (ord 2, top right, 54.61,138.43)
  - source: the main sheet (U7 pin 2/44)
- **S3_3V3** — Supply end of the GPIO1 pull-up.
  - members: 10kΩ@57.15,143.51.#1, global label S3_3V3 (58.42,140.97)
  - source: the main sheet
- **S3_3V3** — Supply end of the GPIO38/GPIO39 pull-ups (10 kΩ).
  - members: 10kΩ@58.42,161.29.#1, 10kΩ@60.96,160.02.#1, global label S3_3V3 (66.04,161.29)
  - source: the main sheet; the author
- **GPIO1** — GPIO1 has a 10 kΩ pull-up to S3_3V3. By label it reaches U6 (PC817) pin p2/4 on the main sheet.
  - members: U7.1, 10kΩ@57.15,143.51.#2, global label GPIO1
  - source: the main sheet; label names across all drawings
- **GPIO4** — GPIO4 has a 10 kΩ pull-up and 0.1 µF to GND. No other drawing carries a GPIO4 label. It is still wired to the LIMITS JST female with these parts, and goes nowhere else.
  - members: U7.4, 10kΩ@21.59,140.97.#2, 0.1uF@19.05,146.05.#1, global label GPIO4 (19.05,142.24)
  - source: the main sheet; label names across all drawings; the author
- **GPIO5** — GPIO5 has a 10 kΩ pull-up and 0.1 µF to GND. By label it reaches U42.OUT (A3144 Hall switch), carried on the LIMITS harness.
  - members: U7.5, 10kΩ@27.94,143.51.#2, 0.1uF@19.02,151.13.#2, global label GPIO5 (19.05,154.94)
  - source: the main sheet; label names across all drawings; the author
- **GND** — Ground return of the GPIO4 and GPIO5 capacitors.
  - members: 0.1uF@19.05,146.05.#2, 0.1uF@19.02,151.13.#1, global label GND (15.24,147.32)
  - source: the main sheet
- **GND** — The only U7 GND pin drawn onto a GND label.
  - members: U7.GND (ord 42, right, 54.61,189.23), global label GND (57.15,189.23)
  - source: the main sheet
- **5VDC** — The S3 board's 5V pin takes 5VDC, with 100 µF and 0.1 µF across it.
  - members: U7.5V, 100uF@19.05,191.77.#1, 0.1uF@22.86,191.77.#1, global label 5VDC (16.51,186.69)
  - source: the main sheet; SC16
- **unnamed net** — The 5V capacitors return to U7's bottom-left GND pin. The net has no GND label.
  - members: U7.GND (ord 43, bottom left, 30.48,191.77), 100uF@19.05,191.77.#2, 0.1uF@22.86,191.77.#2
  - source: the main sheet (U7 pin 43/44)
- **unnamed net** — Pin 39, then the 10 kΩ pull-up junction, then the 220 Ω series resistor.
  - members: U7.39, 10kΩ@60.96,160.02.#2, 220Ω@63.50,158.75.#2
  - source: the main sheet
- **GPIO39** — Cable side of the 220 Ω. By label it reaches U11.SCL (AS5600).
  - members: 220Ω@63.50,158.75.#1, global label GPIO39
  - source: the main sheet; label names across all drawings
- **unnamed net** — Pin 38, then the 10 kΩ pull-up junction, then the 220 Ω series resistor.
  - members: U7.38, 10kΩ@58.42,161.29.#2, 220Ω@58.42,166.37.#1
  - source: the main sheet
- **GPIO38** — Cable side of the 220 Ω. By label it reaches U11.SDA (AS5600).
  - members: 220Ω@58.42,166.37.#2, global label GPIO38
  - source: the main sheet; label names across all drawings
- **GPIO2 / GPIO42 / GPIO41 / GPIO40** — Digit-select outputs: GPIO42 → digit 1, GPIO2 → digit 2, GPIO40 → digit 3, GPIO41 → digit 4 (SC4). The main sheet draws this order (SC4 done).
  - members: U7.2 label GPIO2, U7.42 label GPIO42, U7.41 label GPIO41, U7.40 label GPIO40
  - source: the main sheet; SC4
- **GPIO6, 7, 15, 16, 17, 8, 18** — Segment outputs by global label to the segment ULN2003A U2.
  - members: U7.6 → U2.I1, U7.7 → U2.I2, U7.15 → U2.I3, U7.16 → U2.I4, U7.17 → U2.I5, U7.8 → U2.I6, U7.18 → U2.I7
  - source: the main sheet; label names across all drawings
- **GPIO9, 10, 11, 12** — Stepper outputs by global label to U3 (ULN2003A_1).
  - members: U7.9 → U3.I1, U7.10 → U3.I2, U7.11 → U3.I3, U7.12 → U3.I4
  - source: the main sheet; label names across all drawings
- **S3_to_A32 / A32_to_S3** — Inter-MCU UART: S3 GPIO13 to A32 pin 26, A32 pin 27 to S3 GPIO14.
  - members: U7.13 → U10.26, U7.14 → U10.27
  - source: the main sheet; label names across all drawings
- **GPIO48 / GPIO47** — RDA5807M bus with no external pull-ups. GPIO48 also controls the S3 board's RGB LED (it was off before, and is bright now).
  - members: U7.48 → U89.SCLK, U7.47 → U89.SDIO
  - source: the main sheet; label names across all drawings; the author
- **GPIO21** — Panel-lamp PWM output. It lands on Q1's gate; the 100 Ω on the way is drawn.
  - members: U7.21 → 100Ω.#2 (block B47)
  - source: the main sheet; label names across all drawings; the author
- **U7 no-connect** — No-connect X on each; nothing attached.
  - members: 3V3 (ord 3), RST, 43, 44, 37, 36, 35, 3, 46, 0, 45, 20, 19, GND (ord 44)
  - source: the main sheet

### On sheet: MAIN MCU ESP32-S3 N16R8

- **U63 power** — Since the 2026-09-23 save (SC16 done) the sub-sheet draws U63's power pins connected: both 3V3 pins on the local label 3V3 (= S3_3V3), decoupled by C45 0.1uF and C53 10uF, which return to U63's GND ord 2; the 5V pin on the local label 5V (= 5VDC); the other three GND pins on local GND labels. The main sheet X's U7's lower 3V3 pin (ord 3) and GND ord 44; this sheet puts both on labels.
  - members: U63.3V3 (ord 1), C45.#1, C53.#1, U63.3V3 (ord 3) on 3V3; U63.GND (ord 2), C45.#2, C53.#2 on GND; U63.5V (ord 41) on 5V; U63.GND (ord 42, 43, 44) on GND
  - source: sheet MAIN MCU ESP32-S3 N16R8; the author; SC16
- **3V3 = S3_3V3** — The local label 3V3 carries U63's two 3V3 pins with C45 and C53, and feeds all five pull-ups (the global name S3_3V3 is drawn only on the TUNER pull-ups' conductor and the S3_3V3 connector), the TUNER harness supply and the 2-pin S3_3V3 connector.
  - members: U63.3V3 (ord 1), U63.3V3 (ord 3), C45.#1, C53.#1, 10kΩ@133.35,73.66.#1, 10kΩ@130.81,74.93.#1, 10kΩ@167.64,73.66.#1, 10kΩ@170.18,96.52.#1, 10kΩ@165.10,96.52.#1, U66.p3, S3_TUNER.p2, S3_3V3.p1, U73.p1
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **5V = 5VDC / GND** — 5VIN connector (pin 1 GND, pin 2 5VDC) with 100 µF + 0.1 µF; S3_3V3 connector (pin 1 3V3, pin 2 GND).
  - members: U63.5V, S3_5VIN.p2, U72.p2, 0.1uF@146.05,130.81.#1, 100uF.#1, both cap returns (local GND), U63.GND (all four), S3_5VIN.p1, U72.p1, S3_3V3.p2, U73.p2
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **CABLE S/FTP = GND** — LIMITS harness ground, also the GPIO4/GPIO5 capacitor return. Shield and foil are terminated to GND at the board and open at the far end.
  - members: S3_LIMITS.p1, U70.p1, 0.1uF@128.27,72.39.#1, 0.1uF@121.92,74.93.#1
  - source: sheet MAIN MCU ESP32-S3 N16R8; the author
- **GPIO4 node** — GPIO4 with pull-up and cap reaches LIMITS header pin 2; the jack pin 2 is no-connect. It is wired to the LIMITS JST female, and goes nowhere else.
  - members: U63.4, 10kΩ@133.35,73.66.#2, 0.1uF@128.27,72.39.#2, U70.p2, S3_LIMITS.p2 (no-connect)
  - source: sheet MAIN MCU ESP32-S3 N16R8; the author
- **GPIO5** — GPIO5 with pull-up and cap leaves on LIMITS pin 3.
  - members: U63.5, 10kΩ@130.81,74.93.#2, 0.1uF@121.92,74.93.#2, U70.p3, S3_LIMITS.p3
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **GPIO1 / GND** — Power-detect harness: pin 1 GPIO1, pin 2 GND and shield.
  - members: U63.1, 10kΩ@167.64,73.66.#2, U64.p2, S3_PWR_DET.p1, S3_PWR_DET.p2, U64.p1 (CABLE S/FTP = GND)
  - source: sheet MAIN MCU ESP32-S3 N16R8; the author
- **DIGIT harness** — Digit-select harness as drawn. Digit order truth is SC4.
  - members: S3_DIGIT.p1 GPIO40 / U67.p4, S3_DIGIT.p2 GPIO41 / U67.p3, S3_DIGIT.p3 GPIO42 / U67.p2, S3_DIGIT.p4 GPIO2 / U67.p1
  - source: sheet MAIN MCU ESP32-S3 N16R8; SC4
- **DISPLAY harness** — Segment harness as drawn on this sheet.
  - members: S3_DISPLAY.p1 GPIO6, p2 GPIO7, p3 GPIO15, p4 GPIO16, p5 GPIO17, p6 GPIO18, p7 GPIO8
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **STEPPER harness** — Stepper harness.
  - members: S3_STEPPER.p1 GPIO12, p2 GPIO11, p3 GPIO10, p4 GPIO9
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **UART harness** — Inter-MCU UART harness.
  - members: S3_UART.p1 GND / CABLE S/FTP, S3_UART.p2 S3_to_A32 (U63.13), S3_UART.p3 A32_to_S3 (U63.14)
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **TUNER harness** — AS5600 harness with 10 kΩ pull-ups and 220 Ω series resistors at the S3 end.
  - members: U63.39 → 10k junction → 220Ω@167.64,92.71 → GPIO39 → S3_TUNER.p4, U63.38 → 10k junction → 220Ω@172.72,95.25 → GPIO38 → S3_TUNER.p3, S3_TUNER.p2 S3_3V3, S3_TUNER.p1 GND / CABLE S/FTP
  - source: sheet MAIN MCU ESP32-S3 N16R8; the author
- **RDA harness** — RDA5807M I2C pair only, no ground or supply conductor.
  - members: S3_RDA.p1 GPIO47 / U91.p2, S3_RDA.p2 GPIO48 / U91.p1
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **FM LED harness** — Panel-lamp PWM harness.
  - members: S3_FM_LED.p2 GPIO21 / U62.p1, S3_FM_LED.p1 GND (local) + CABLE S/FTP / U62.p2
  - source: sheet MAIN MCU ESP32-S3 N16R8
- **U63 no-connect** — Same X'd signal pins as the main sheet.
  - members: RST, 43, 44, 37, 36, 35, 3, 46, 0, 45, 20, 19
  - source: sheet MAIN MCU ESP32-S3 N16R8

### On sheet: USB-C CONNECTORS

- **P2 (MAIN)** — Back-panel USB-C for the S3: carried through, VBUS and GND included, to the S3 board's native USB port.
  - members: P2.VBUS, P2.D-, P2.D+ and P2.GND (ord 4), each labelled 'TO MAIN MCU ESP32-S3'S USB-C' (a visual aide, not one conductor); P2's other three GND pins (A12, B1, B12) wired together, unlabelled
  - source: sheet USB-C CONNECTORS; the author; SC24
- **P1 (AUDIO)** — Back-panel USB-C for the A32: carried through, VBUS and GND included, to the audio MCU board's USB-C.
  - members: P1.VBUS, P1.D-, P1.D+ and P1.GND (ord 4), each labelled 'TO AUDIO MCU ESP32-WROOM USB-C' (a visual aide, not one conductor); P1's other three GND pins (A12, B1, B12) wired together, unlabelled
  - source: sheet USB-C CONNECTORS; the author; SC24

## 5. Audio MCU (ESP32, "A32") and the DS3231 RTC

### On the main sheet

- **A_3V3** — A32 3V3 pin on the A_3V3 bus, decoupled by 0.1 µF + 10 µF + 10 µF
  - members: U10.3V3, C20.#1 (0.1uF), C21.#1 (10uF), C22.#1 (10uF), label A_3V3(g)
  - source: the main sheet; the author; SC17
- **unnamed net** — 3V3 decoupling caps return to the top-right GND pin
  - members: U10.GND (ord 2), C20.#2, C21.#2, C22.#2
  - source: the main sheet
- **5VDC** — A32 5V pin on 5VDC, 0.1 µF + 100 µF
  - members: U10.5V, 0.1uF@76.20,180.34.#1, 100uF@80.01,180.34.#1, label 5VDC(g)
  - source: the main sheet
- **GND** — Left GND pin and 5V cap returns
  - members: U10.GND (ord 27), 0.1uF@76.20,180.34.#2, 100uF@80.01,180.34.#2, label GND(g)
  - source: the main sheet
- **GPIO36 input** — Front DPDT audio switch input: pin, 0.1 µF to GND at the pin, 1 kΩ to the ladder node. This is the **selector signal only**: on AUX the audio itself never reaches the A32 (§22).
  - members: U10.36, 1kΩ@80.01,147.32 (series), 0.1 µF to GND on the pin side, A_IO36 ladder node: 22kΩ.#1, 68kΩ.#1, DPDT_ON_OFF_ON.A
  - source: the main sheet; the author
- **A_IO35 (drawn A_IO34 until 2026-09-23; SC6 done)** — Volume pot wiper to GPIO35
  - members: U10.35, RV4.2
  - source: the main sheet; the author; SC6; firmware (§0)
- **A_IO33** — BT LED cathode to GPIO33 (active low)
  - members: U10.33, D29.K
  - source: label names across all drawings; the author; SC3; firmware (§0)
- **A_IO25** — BT pair button to GPIO25 (active low; SW2 pin 2 on GND)
  - members: U10.25, SW2.1
  - source: label names across all drawings; the author; SC3; firmware (§0)
- **S3_to_A32** — UART S3 GPIO13 -> A32 GPIO26
  - members: U10.26, U7.13
  - source: label names across all drawings
- **A32_to_S3** — UART A32 GPIO27 -> S3 GPIO14
  - members: U10.27, U7.14
  - source: label names across all drawings
- **A_IO22** — RTC SDA
  - members: U10.22, U8.SDA
  - source: label names across all drawings
- **A_IO21** — RTC SCL
  - members: U10.21, U8.SCL
  - source: label names across all drawings
- **A_IO19** — ADC data in
  - members: U10.19, PCM1802.DOUT
  - source: label names across all drawings
- **A_IO18** — Shared I2S bit clock
  - members: U10.18, PCM1802.BCK, U9.BCK
  - source: label names across all drawings
- **A_IO17** — Shared I2S word clock
  - members: U10.17, PCM1802.LRCK, U9.LRCK
  - source: label names across all drawings
- **A_IO4** — DAC data out
  - members: U10.4, U9.DIN, 100kΩ@116.84,24.13.#1
  - source: label names across all drawings
- **GPIO0 clock (A_IO0A, A_IO0D)** — SCK clock from GPIO0, one 33 Ω per converter branch at the A32: the branch named A_IO0A reaches the PCM1802 SCK, the branch named A_IO0D the PCM5102A SCK
  - members: U10.0, 33Ω@109.22,175.26 → A_IO0A → PCM1802.SCK, 33Ω@109.22,180.34 → A_IO0D → U9 (PCM5102A).SCK
  - source: the main sheet; the author; SC8
- **A_VDC** — RTC supply on A_VDC (= A_3V3 on sheet HERE (the star-point buses))
  - members: U8.VCC, label A_VDC(g); as drawn, the DS3231M symbol's eight hidden GND pins (#5–#12) also lie on this wire, and are disregarded because they are hidden — the fitted RTC is a DS3231 ZS-042-style module
  - source: the main sheet; label names across all drawings
- **GND** — RTC ground
  - members: U8.GND (visible), label GND(g)
  - source: the main sheet
- **no-connect** — Unused U10 pins carry X
  - members: U10.EN, 23, 39, 1, 34, 3, 32, GND ord 14, 5, 14, 16, 12, 13, 2, 9, 15, 10, 8, 11, 7, 6
  - source: the main sheet; the author (GPIO16 unused)

### On sheet: AUDIO MCU ESP32-WROOM-32

- **5VDC** — 5 V in on the A32_5VIN cable
  - members: U74.5V, 0.1uF@128.27,118.11.#1, 100uF.#1, U81.p1/2, A32_5VIN.p2/2
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **GND** — 5 V input ground
  - members: U74.GND ord 27, 0.1uF@128.27,118.11.#2, 100uF.#2, U81.p2/2, A32_5VIN.p1/2
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_3V3** — A32 3V3 pin feeds A_3V3 out on the +-3V3 cable, decoupled by C54 0.1uF, C55 10uF and C56 10uF (since the 2026-09-23 save; SC17 done)
  - members: U74.3V3, C54.#1 (0.1uF), C55.#1 (10uF), C56.#1 (10uF), U75.p1/2, A32_3V3.p1/2
  - source: sheet AUDIO MCU ESP32-WROOM-32; SC17
- **GND** — 3V3 cable ground, and the returns of C54, C55 and C56
  - members: U74.GND ord 2, C54.#2, C55.#2, C56.#2, U75.p2/2, A32_3V3.p2/2
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_3V3** — 3V3 to the BT LED/button cable
  - members: U79.p4/4, A32_BT_LED.p1/4
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO33** — BT LED (active low)
  - members: U74.33, U79.p3/4, A32_BT_LED.p2/4
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author; firmware (§0)
- **GND** — BT cable ground (labels only)
  - members: U79.p2/4, A32_BT_LED.p3/4
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO25** — BT pair button (active low)
  - members: U74.25, U79.p1/4, A32_BT_LED.p4/4
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author; firmware (§0)
- **S3_to_A32** — UART RX
  - members: U74.26, U80.p1/3, A32_UART.p1/3
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A32_to_S3** — UART TX
  - members: U74.27, U80.p2/3, A32_UART.p2/3
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **GND + CABLE S/FTP** — UART cable GND and shield/foil terminated at board side
  - members: U80.p3/3, A32_UART.p3/3
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **GND + CABLE S/FTP** — Switch cable ground/shield; ladder and filter returns
  - members: U78.p3/3, A32_SWITCH.p1/3, 22kΩ.#1, 0.1uF@135.89,77.47.#1
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **A_IO36_H** — Ladder node (H = 3.3 V level)
  - members: U78.p2/3, A32_SWITCH.p2/3, 22kΩ.#2, 68kΩ.#1, 1kΩ.#2
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **A_IO36_L** — L leg through 68 kΩ (0.8 V level)
  - members: U78.p1/3, A32_SWITCH.p3/3, 68kΩ.#2
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **GPIO36 pin node** — Pin node with 0.1 µF at the pin
  - members: U74.36, 1kΩ.#1, 0.1uF@135.89,77.47.#2
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **A_IO21** — RTC SCL out
  - members: U74.21, U76.p3/3, A32_RTC.p1/3
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **GND + CABLE S/FTP** — RTC cable ground/shield
  - members: U76.p2/3, A32_RTC.p2/3
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **A_IO22** — RTC SDA out
  - members: U74.22, U76.p1/3, A32_RTC.p3/3
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO19** — I2S ADC data
  - members: U74.19, U77.p1/7, A32_I2S.p7/7
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO18** — I2S BCK
  - members: U74.18, U77.p2/7, A32_I2S.p6/7
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **GND + CABLE S/FTP** — I2S cable ground/shield
  - members: U77.p3/7, A32_I2S.p5/7
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **A_IO17** — I2S LRCK
  - members: U74.17, U77.p4/7, A32_I2S.p4/7
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO0A — GPIO0 clock branch to the ADC (through 33Ω@167.64,102.87)** — One of the two GPIO0 clock branches, each with its own 33 Ω, both at the A32's pin. Since the 2026-09-23 save this branch is named A_IO0A, the name the PCM1802 sheet puts on the ADC's SCK (ADC_I2S.p6/6, U23.p6/6).
  - members: 33Ω@167.64,102.87.#2, U77.p5/7, A32_I2S.p3/7
  - source: the author; SC7; SC8; sheet AUDIO MCU ESP32-WROOM-32
- **A_IO4** — I2S DAC data
  - members: U74.4, U77.p6/7, A32_I2S.p2/7
  - source: sheet AUDIO MCU ESP32-WROOM-32
- **A_IO0D — GPIO0 clock branch to the DAC (through 33Ω@167.64,107.95)** — The second GPIO0 clock branch. Since the 2026-09-23 save it is named A_IO0D, the name the PCM5102A sheet puts on the DAC's SCK (U13.p6/6, U14.p1/6).
  - members: 33Ω@167.64,107.95.#2, U77.p7/7, A32_I2S.p1/7
  - source: sheet AUDIO MCU ESP32-WROOM-32; SC8
- **GPIO0 node** — GPIO0 splits into two 33 Ω at the pin
  - members: U74.0, 33Ω@167.64,107.95.#1, 33Ω@167.64,102.87.#1
  - source: sheet AUDIO MCU ESP32-WROOM-32; SC8
- **A_IO35** — Volume pot wiper to GPIO35
  - members: U74.35, U45.p2/2, A32_VOL.p1/2
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author; SC6; firmware (§0)
- **GND + CABLE S/FTP** — Volume cable ground/shield
  - members: U45.p1/2, A32_VOL.p2/2
  - source: sheet AUDIO MCU ESP32-WROOM-32; the author
- **no-connect** — Unused pins with X. GND ord 14 is not one of them: since the 2026-09-23 save it carries a local GND label at the pin, and nothing else is on its wire.
  - members: U74.EN, 23, 39, 1, 34, 3, 32, 5, 14, 16, 12, 13, 2, 9, 15, 10, 8, 11, 7, 6
  - source: sheet AUDIO MCU ESP32-WROOM-32; SC17

### On sheet: DS3231 RTC (file renamed from DS3132 2026-09-23)

- **A_VDC = VCC** — RTC module supply from A_VDC
  - members: RTC_3V3.p1/2, U15.p2/2
  - source: sheet DS3231 RTC; the author
- **GND** — Supply header ground
  - members: RTC_3V3.p2/2, U15.p1/2
  - source: sheet DS3231 RTC
- **RTC_I2C 4th position — not connected** — The RTC_I2C 4th position is not connected. As drawn, RTC_I2C and U16 put p1/4 and p2/4 on no-connect, p3/4 on A_IO22 (SDA) and p4/4 on A_IO21 (SCL) (SCV6 done: the 2026-09-23 save draws RTC_I2C with two of its four positions connected, which is right).
  - members: RTC_I2C.p1/4 and p2/4, U16.p1/4 and p2/4: all drawn no-connect
  - source: the author; sheet DS3231 RTC
- **A_IO22 = SDA** — Module SDA to A32 GPIO22
  - members: RTC_I2C.p3/4, U16.p3/4
  - source: sheet DS3231 RTC
- **A_IO21 = SCL** — Module SCL to A32 GPIO21, drawn on RTC_I2C p4/4 and U16 p4/4; p1/4 and p2/4 are drawn no-connect (the 2026-09-23 save, which is right: it is a 4-pin JST with only 2 pins connected). The RTC_I2C 4th position is not connected; "4th position" counts the positions from one end, and the drawing's contact order does not settle which one that is (contact order inside a connector is not covered).
  - members: RTC_I2C.p4/4, U16.p4/4
  - source: sheet DS3231 RTC; the author
- **no-connect (as drawn)** — As drawn, p2/4 of the RTC I2C header is also no-connect; see the 4th-position entry above (SCV6 done 2026-09-23).
  - members: RTC_I2C.p2/4, U16.p2/4
  - source: sheet DS3231 RTC; the author

## 6. Audio converters: PCM1802 ADC, tube radio input, radio transformer output, PCM5102A DAC

### On the main sheet

- **5VDC (PCM1802 supply)** — The PCM1802 module's 5V pin is on 5VDC, with 100 µF and 0.1 µF (both fitted) to GND.
  - members: PCM1802.5V, 100uF@78.74,50.80.#1, 0.1uF@78.74,54.61.#1, global label 5VDC
  - source: the main sheet; the author
- **GND (PCM1802 supply caps)** — The other side of both PCM1802 supply caps is GND.
  - members: 100uF@78.74,50.80.#2, 0.1uF@78.74,54.61.#2, global label GND
  - source: the main sheet
- **GND (PCM1802 straps)** — MODE1, MODE0, BYPAS, OSR and FMT1 are tied to the module's GND.
  - members: PCM1802.GND, PCM1802.MODE1, PCM1802.MODE0, PCM1802.BYPAS, PCM1802.OSR, PCM1802.FMT1, global label GND
  - source: the main sheet
- **(unnamed) FMT0** — FMT0 is wired to the module's own P_3V3 pin, and to nothing else.
  - members: PCM1802.FMT0, PCM1802.P_3V3
  - source: the main sheet
- **P_3V3 (label)** — PDW and FSY are joined to a global label P_3V3 that appears on no other drawing and is not wired to the module's P_3V3 pin.
  - members: PCM1802.PDW, PCM1802.FSY, global label P_3V3
  - source: the main sheet; label names across all drawings
- **A_IO17** — LRCK of both converters is A32 GPIO17.
  - members: PCM1802.LRCK, U9.LRCK (#15), U10 GPIO17
  - source: label names across all drawings
- **A_IO18** — BCK of both converters is A32 GPIO18.
  - members: PCM1802.BCK, U9.BCK (#13), U10 GPIO18
  - source: label names across all drawings
- **A_IO19** — The ADC's data output goes to A32 GPIO19.
  - members: PCM1802.DOUT, U10 GPIO19
  - source: label names across all drawings
- **J51 — the radio jack (drawn as J42 before the correction)** — Cable side: sleeve on CABLE S/FTP, ring = RAD_L, tip = RAD_R (tip and ring interchangeable). Output side: sleeve and ring on GND, tip into the 4.7 kΩ. The radio output is mono.
  - members: J51.#S (ord1, output side) = GND, J51.#R (ord2, output side) = GND, J51.#S (ord3, cable side) = CABLE S/FTP, J51.#T (ord4, output side) → 4.7kΩ@92.71,90.17, J51.#R (ord5, cable side) = RAD_L, J51.#T (ord6, cable side) = RAD_R
  - source: the main sheet; the author
- **the attenuator node (LIN, and RIN in the hardware)** — The tip leg goes through the 4.7 kΩ to this node; a 1 kΩ and the 0.001 µF CBB22 capacitor C59 run from it to GND. It feeds the PCM1802's LIN by label, and its RIN in the hardware; the main sheet joins RIN to it by name since 2026-09-23 (SCH1 done).
  - members: 4.7kΩ@92.71,90.17.#2 ← J51.#T (ord4); 4.7kΩ@92.71,90.17.#1, 1kΩ@95.25,85.09.#2, C59.#1, label LIN → PCM1802.LIN; in the hardware also PCM1802.RIN
  - source: the main sheet; the author
- **GND (radio input)** — J51's output-side sleeve and ring, the 1 kΩ and C59 are on GND; so are the radio cable's shield and foil.
  - members: J51.#S (ord1), J51.#R (ord2), 1kΩ@95.25,85.09.#1, C59.#2, global label GND
  - source: the main sheet; the author
- **CABLE S/FTP (radio cable)** — J51's cable-side sleeve carries the radio cable's shield and foil.
  - members: J51.#S (ord3)
  - source: the main sheet; the author
- **RAD_L** — One end of the radio transformer secondary, with the 7.5 Ω 5 W load across it (marked 5W7Ω5J; drawn reference text 7.5Ω, Value 5W7.5Ω; the ±5% is irrelevant), to J51's cable-side ring (tip and ring interchangeable).
  - members: T1.SA, 7.5Ω.#1, J51.#R (ord5, cable side)
  - source: the main sheet; the author
- **RAD_R** — The other end of the transformer secondary (the 7.5 Ω 5 W load across the two) to J51's cable-side tip.
  - members: T1.SB, 7.5Ω.#2, J51.#T (ord6, cable side)
  - source: the main sheet; the author
- **50C5** — T1's primary is the tube radio's 50C5 output stage (the tube radio's internals: §13–§21).
  - members: T1.AA, T1.AB
  - source: the author
- **A_IO4** — DAC data input is A32 GPIO4, with a 100 kΩ (fitted) to GND.
  - members: U9.DIN (#14), 100kΩ@116.84,24.13.#1, U10 GPIO4
  - source: the main sheet; label names across all drawings; the author
- **GND (DIN pull-down)** — The DIN pull-down's other end is GND.
  - members: 100kΩ@116.84,24.13.#2, global label GND
  - source: the main sheet
- **5VDC (PCM5102A supply)** — The DAC module's VIN is on 5VDC with 100 µF, 10 µF ceramic and 0.1 µF, all fitted.
  - members: U9.VIN, C27.#1 (100 µF), C24.#1 (10 µF ceramic), C26.#1 (0.1 µF)
  - source: the main sheet; the author
- **GND (PCM5102A)** — The DAC module's GND and its supply caps are on GND.
  - members: U9.GND, C24.#2, C26.#2, C27.#2
  - source: the main sheet
- **GND (DAC straps)** — FLT, DEMP and FMT are tied to GND (normal-latency filter, de-emphasis off, I2S).
  - members: U9.FLT (#11), U9.DEMP (#10), U9.FMT (#16)
  - source: the main sheet
- **no-connect (XSMT is pulled up on the module)** — Nothing outside the module reaches XSMT or the module's A3V3 pin, and GPIO16 is unused. On the module XSMT is pulled up to the module's own 3.3 V, so the DAC is un-muted without any connection here.
  - members: U9.XSMT (#17), U9.A3V3, U10 GPIO16
  - source: the main sheet; the author; SC7
- **AMP_R+ / AMP_R- / AMP_L+ / AMP_L- (the four DAC-to-amp 470 Ω, drawn as a visual aide)** — The main sheet draws the four 470 Ω on U9's output pins. The main sheet is a visual aide that shows the resistances applied to the pins, not their exact emplacement. Where they are is the cable sheet's, and §6 "On sheet: THIS (cable) via SC13" and §12 carry it.
  - members: 470Ω@116.84,48.26 (#2 U9.OUTR (#7), #1 AMP_R+); 470Ω@116.84,55.88 (#2 U9.AGND (#9), #1 AMP_R-); 470Ω@116.84,60.96 (#1 U9.OUTL (#6), #2 AMP_L+); 470Ω@116.84,63.50 (#1 U9.AGND (#9), #2 AMP_L-); AMP_G on both drawn U9.AGND pins
  - source: the main sheet; the author

### On sheet: AUDIO MCU + THIS (cable) + PCM1802 ADC

- **clock at the ADC (A_IO0A)** — One of the two GPIO0 clock branches, each with its own 33 Ω, both at the A32's pin. Since the 2026-09-23 save the ADC's branch is named A_IO0A on every drawing: at the A32, GPIO0 → 33Ω@167.64,102.87 → A_IO0A → A32_I2S.p3/7; on the cable sheet A32_I2S.p3/7 and ADC_I2S.p6/6; on the PCM1802 sheet ADC_I2S.p6/6 and U23.p6/6 (SCK); on the main sheet 33Ω@109.22,175.26 → A_IO0A → PCM1802.SCK.
  - members: ADC_I2S.p6/6, U23.p6/6 (SCK), PCM1802.SCK
  - source: sheet AUDIO MCU ESP32-WROOM-32; the cable sheet; sheet PCM1802 ADC; SC8; SC7; the author

### On sheet: AUDIO MCU + THIS (cable) + PCM5102A DAC

- **clock at the DAC (A_IO0D)** — GPIO0 of the A32 feeds both the PCM1802 SCK and the PCM5102A SCK, each through its own 33 Ω, both at the A32's pin. Since the 2026-09-23 save the DAC's branch is named A_IO0D on every drawing: at the A32, GPIO0 → 33Ω@167.64,107.95 → A_IO0D → A32_I2S.p1/7; on the cable sheet A32_I2S.p1/7 and DAC_I2S.p1/6; on the PCM5102A sheet U13.p6/6 and U14.p1/6 (SCK); on the main sheet 33Ω@109.22,180.34 → A_IO0D → U9.SCK (#12).
  - members: DAC_I2S.p1/6, U13.p6/6 / U14.p1/6 (SCK), U9.SCK (#12)
  - source: the author; SC7; SC8

### On sheet: PCM1802 ADC

- **5VDC = 5VIN / GND** — The ADC module's 2-pin power header: + = 5VDC (left), - = GND (right).
  - members: U21.p1/2, ADC_5VIN.p1/2 (5VDC), U21.p2/2, ADC_5VIN.p2/2 (GND)
  - source: sheet PCM1802 ADC
- **I2S header** — The ADC's 6-pin I2S header, left to right as drawn.
  - members: U23/ADC_I2S p1 DOUT=A_IO19, p2 BCK=A_IO18, p3 no-connect, p4 LRCK=A_IO17, p5 no-connect, p6 SCK=A_IO0A
  - source: sheet PCM1802 ADC

### On sheet: PCM1802 ADC + THIS (cable)

- **RADIO_TO_ADC (= J42, drawn as J51 on the main sheet)** — Cable side: RAD_L on ring, RAD_R on tip, sleeve on CABLE S/FTP. Output side: ring and tip both labelled ATTENUATOR (into the main sheet's radio input), sleeve on GND.
  - members: RADIO_TO_ADC.#R (ord5) / J50.#R = RAD_L, RADIO_TO_ADC.#T (ord6) / J50.#T = RAD_R, RADIO_TO_ADC.#S (ord3) / J50.#S = CABLE S/FTP, RADIO_TO_ADC.#R (ord2) and #T (ord4) = ATTENUATOR, RADIO_TO_ADC.#S (ord1) = GND
  - source: sheet PCM1802 ADC, the cable sheet; the author
- **Ferrite cores on this cable (not drawn)** — Two clip-on cores, one near the tube radio transformer T1 end, one near the ADC end; the cable passes through each once. Measured: 10 mm OD, 3.5 mm ID, 3.5 mm walls, 20 mm long (the figures do not close: 3.5 + 3.5 + 3.5 is 10.5 against a 10 mm OD, so one is rounded). Material as supplied with the cores, with no maker, part number or grade: rectangle ratio 20, coercivity 16 A/m, remanence 200 mT, Curie temperature 130 °C, density 4.9 g/cm³, "Soft Magnetic".
  - source: the author

### On sheet: PCM5102A DAC

- **header** — The DAC's 6-pin header, left to right as drawn (no pull-down or caps drawn on this sheet; those are on the main sheet and fitted).
  - members: U13/U14 SCK=A_IO0D, BCK=A_IO18, DIN=A_IO4, LCK=A_IO17, GND, VIN=5VDC
  - source: sheet PCM5102A DAC; the author
- **module output jack J43, and plug J40 (DAC_OUT)** — The DAC module's 3-pole (TRS) output jack J43. Since the 2026-09-23 save the plug J40, Value DAC_OUT, is drawn on the same three nets. The cable sheet's plug J49 carries the same Value, DAC_OUT; no label name joins J40's poles to J49's, so the drawings do not say whether they are one plug.
  - members: J43.#S = J40.#S = AGND, J43.#R = J40.#R = OUTR, J43.#T = J40.#T = OUTL
  - source: sheet PCM5102A DAC; the author

### On sheet: THIS (cable) via SC13

- **AMP_L+** — DAC left out -> plug tip -> a 470 Ω at the DAC end of the cable -> amp input AMP_L+.
  - members: J43.#T (OUTL), J49.#T (plug DAC_OUT), 470Ω@212.09,40.64, AMP_IN.p3/9
  - source: the cable sheet; sheet PCM5102A DAC; the author; SC13
- **AMP_R+** — DAC right out -> plug ring -> a 470 Ω at the DAC end of the cable -> AMP_R+.
  - members: J43.#R (OUTR), J49.#R, 470Ω@212.09,30.48, AMP_IN.p7/9
  - source: the cable sheet; sheet PCM5102A DAC; the author; SC13
- **AMP_G** — DAC analog ground -> plug sleeve -> AMP_G, and to one end of both in-cable 470 Ω (the two on the − legs; all four signal legs carry a 470 Ω, the two others in series with AMP_L+ and AMP_R+ at the DAC end).
  - members: J43.#S (AGND), J49.#S, 470Ω@214.63,33.02.#1, 470Ω@214.63,38.10.#1, AMP_IN.p5/9
  - source: the cable sheet; sheet PCM5102A DAC; SC13
- **Ferrite cores on this cable (not drawn)** — Six cores, three at each end: one HDMI-type sleeve (17.4 mm OD, 9.7 mm ID, 28.5 mm long, the same type as the antenna feedpoint cores of §29), one small toroid (16 mm OD, 3 mm wall, 8 mm long) and one large toroid (22 mm OD, 4 mm wall, 8 mm long). The cable passes through each once; it is under 12 inches long, so no turns. The toroids are salvaged and unidentified: no maker, part number, grade or magnetic figure is recorded, and their material is not the clip-ons'.
  - source: the author
- **AMP_R-** — AMP_G through 470 Ω to AMP_R-.
  - members: 470Ω@214.63,33.02.#2, AMP_IN.p9/9
  - source: the cable sheet; SC13
- **AMP_L-** — AMP_G through 470 Ω to AMP_L-.
  - members: 470Ω@214.63,38.10.#2, AMP_IN.p1/9
  - source: the cable sheet; SC13

## 7. Volume pot, amp PSU 20 VDC, power detect, box and cage fans

*Specific to this build — adapt: the fans, the box and the amplifier's ±20 V supply suit this cabinet and this amplifier.*

### On the main sheet

- **+20V** — The amp PSU module's +20V output feeds the box-fan LM7812 input, with its 0.1uF and 100uF input caps, and the power-detect opto's 4.7kΩ LED resistor. The two drawn nets are joined by the +20V label.
  - members: U40.VI (#1), C14.#1, C15.#1, global label +20V @(132.08,130.81), U1.+20V, 4.7kΩ@264.16,171.45.#2, global label +20V @(260.35,168.91)
  - source: the main sheet
- **-20V** — The amp PSU -20V output goes only to a -20V label; no other drawing carries -20V.
  - members: U1.-20V, global label -20V @(255.27,179.07)
  - source: the main sheet; label names across all drawings
- **20V_GND** — The amp PSU's two 20V_GND pins are joined and return the PC817 input side (p3/4).
  - members: U1.20V_GND (ord 2/7), U1.20V_GND (ord 3/7), U6.p3/4 (LED cathode), global label 20V_GND @(255.27,173.99)
  - source: the main sheet
- **20V_GND (box-fan / LM7812 return; drawn unnamed as)** — The LM7812 ground, the low side of all four regulator caps, box fan M1's - and one end of D34 share one return. The drawing leaves it unnamed; it is 20V_GND.
  - members: U40.GND (#2), C13.#1, C14.#2, C15.#2, C16.#2, M1.-, D34.p1/2
  - source: the main sheet; the author
- **unnamed net 12 V box-fan rail** — The LM7812 output feeds box fan M1 +, with its output caps C13 0.1uF and C16 100uF and the other end of D34.
  - members: U40.VO (#3), C13.#2, C16.#1, M1.+, D34.p2/2
  - source: the main sheet
- **5VDC** — Cage fan M2 + is wired to the 5VDC bus.
  - members: M2.+, global label 5VDC @(134.62,144.78)
  - source: the main sheet
- **GND (cage fan)** — Cage fan M2 - is on GND.
  - members: M2.-, global label GND on pin @(125.73,144.78)
  - source: the main sheet
- **AC_L** — The amp PSU AC+ input is on AC_L, the switched side of the front-panel power toggle SW1; SW1.B is on AC_L_IN with J30.L and U4.AC+.
  - members: U1.AC+, SW1.A (by name,)
  - source: label names across all drawings; the main sheet
- **AC_N** — The amp PSU AC- input shares AC_N with the AC inlet J30 and the 5V PSU U4.
  - members: U1.AC-, J30.N (by name,), U4.AC- (by name,)
  - source: label names across all drawings
- **Earth** — The amp PSU EARTH pin is on the Earth node with the AC inlet earth, the Faraday-cage label and the antenna jack's shell.
  - members: U1.EARTH (power:Earth symbol on pin), J30.E (power:Earth), FARADAY_CAGE label net, J10 shell 'Ext' (per SC30)
  - source: the main sheet; SC30
- **unnamed net** — The 4.7kΩ resistor's pin 1 touches PC817 p1/4 (LED anode) directly, pin to pin.
  - members: 4.7kΩ@264.16,171.45.#1, U6.p1/4 (LED anode)
  - source: the main sheet
- **GPIO1** — The PC817 output p2/4 is on GPIO1, the S3's pin 1, which is pulled up to S3_3V3 through 10kΩ@57.15,143.51.
  - members: U6.p2/4 (collector) (label on pin), U7.1 (S3 pin '1', by name), 10kΩ@57.15,143.51.#2 (by name)
  - source: the main sheet
- **S3_3V3 (pull-up)** — The GPIO1 pull-up's other end is on S3_3V3.
  - members: 10kΩ@57.15,143.51.#1
  - source: the main sheet
- **GND (opto)** — PC817 p4/4 (emitter) is on GND.
  - members: U6.p4/4 (emitter) (label on pin)
  - source: the main sheet
- **A_IO35 (drawn A_IO34 until 2026-09-23; SC6 done)** — The volume pot RV4's wiper goes to the A32's GPIO35.
  - members: RV4.2 (wiper), U10 pin 35 (A32, by label)
  - source: the main sheet; SC6; the author; firmware (§0)
- **A_VDC (= A_3V3)** — Pot end 1 is on A_VDC, which sheet HERE (the star-point buses) names A_3V3 as well; on the main sheet A_3V3 is the A32's 3V3 pin.
  - members: RV4.1
  - source: the main sheet; sheet HERE (the star-point buses); label names across all drawings
- **GND (pot)** — Pot end 3 is on GND.
  - members: RV4.3
  - source: the main sheet

### On sheet: Power Detect

- **+20V** — The sub-sheet LED resistor is fed from +20V.
  - members: 4.7kΩ.#2 (label on pin)
  - source: sheet Power Detect
- **unnamed net** — The resistor touches PC817 p1/4 (LED anode) directly.
  - members: 4.7kΩ.#1, U83.p1/4 (LED anode)
  - source: sheet Power Detect
- **20V_GND** — PC817 p3/4 (LED cathode) is wired to 20V_GND.
  - members: U83.p3/4 (LED cathode)
  - source: sheet Power Detect
- **GPIO1** — PC817 p2/4 (collector) runs through the U84 JST to the POWER_DET cable end, on GPIO1.
  - members: U83.p2/4 (collector), U84.p2/2, POWER_DET.p2/2
  - source: sheet Power Detect
- **GND** — PC817 p4/4 (emitter) runs through the U84 JST to the POWER_DET cable end, on GND.
  - members: U83.p4/4 (emitter), U84.p1/2, POWER_DET.p1/2
  - source: sheet Power Detect

### On sheet: BOX FAN (the 12V box-fan harness; file renamed from VOLUME POT 2026-09-23)

- **+20V = VI** — Box-fan harness supply in: +20V, locally named VI.
  - members: FAN_IN.p1/2, U88.p1/2
  - source: sheet BOX FAN; the author
- **20V_GND = FAN_- = GND** — Box-fan harness return: 20V_GND, the fan's -, locally named GND.
  - members: FAN_IN.p2/2, U88.p2/2, FAN_OUT.p1/2, U87.p2/2
  - source: sheet BOX FAN; the author
- **FAN_+ = VO** — Box-fan harness output to the fan's +, locally named VO.
  - members: FAN_OUT.p2/2, U87.p1/2
  - source: sheet BOX FAN; the author

## 8. The Leditron clock display (four digits and the colon)

*Specific to this build — adapt: the display (digits of discrete segment LEDs, and the colon) is made for this front panel.*

- **Digit numbering** — Digit 1 is the rightmost digit (units of minutes); digits 2, 3 and 4 run leftwards from it. GPIO42 → digit 1, GPIO2 → 2, GPIO40 → 3, GPIO41 → 4 (SC4).

### On the main sheet

- **Colon** — 5VDC → 33 Ω → colon LED → variable resistor (stuck at 1.5 kΩ) → GND, as the 7-segment sub-sheet draws it; the variable sits at the colon module. The main sheet draws R17 (variable, 1.5 kΩ) and the 33 Ω both on the 5 V side of the LED; that placement is intentional (SC12 done).
  - members: 5VDC, 33 Ω, COLON1 (colon LED), variable resistor stuck at 1.5 kΩ, GND
  - source: the author; SC12
- **unnamed net (digit 1 common anode)** — All seven segment anodes of LEDITRON DIGIT 1 join on header J1 pin 8.
  - members: D1.A, D2.A, D3.A, D4.A, D5.A, D6.A, D7.A, J1.Pin_8
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg A cathode to J1 pin 1.
  - members: D1.K, J1.Pin_1
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg B cathode to J1 pin 2.
  - members: D3.K, J1.Pin_2
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg C cathode to J1 pin 3.
  - members: D5.K, J1.Pin_3
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg D cathode to J1 pin 4.
  - members: D6.K, J1.Pin_4
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg E cathode to J1 pin 5.
  - members: D4.K, J1.Pin_5
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg F cathode to J1 pin 6.
  - members: D2.K, J1.Pin_6
  - source: the main sheet
- **unnamed nets** — Digit 1 Seg G cathode to J1 pin 7.
  - members: D7.K, J1.Pin_7
  - source: the main sheet
- **unnamed net (digit 2 common anode)** — All seven segment anodes of LEDITRON DIGIT 2 join on header J8 pin 8.
  - members: D22.A, D23.A, D24.A, D25.A, D26.A, D27.A, D28.A, J8.Pin_8
  - source: the main sheet
- **unnamed nets** — Digit 2 cathodes, one net each: Seg A..G to J8 pins 1..7 (A=D24, B=D27, C=D28, D=D26, E=D23, F=D22, G=D25).
  - members: D24.K–J8.Pin_1, D27.K–J8.Pin_2, D28.K–J8.Pin_3, D26.K–J8.Pin_4, D23.K–J8.Pin_5, D22.K–J8.Pin_6, D25.K–J8.Pin_7
  - source: the main sheet
- **unnamed net (digit 3 common anode)** — All seven segment anodes of LEDITRON DIGIT 3 join on header J4 pin 8.
  - members: D15.A, D16.A, D17.A, D18.A, D19.A, D20.A, D21.A, J4.Pin_8
  - source: the main sheet
- **unnamed nets** — Digit 3 cathodes, one net each: Seg A..G to J4 pins 1..7 (A=D17, B=D20, C=D21, D=D19, E=D16, F=D15, G=D18).
  - members: D17.K–J4.Pin_1, D20.K–J4.Pin_2, D21.K–J4.Pin_3, D19.K–J4.Pin_4, D16.K–J4.Pin_5, D15.K–J4.Pin_6, D18.K–J4.Pin_7
  - source: the main sheet
- **unnamed net (digit 4 common anode)** — All seven segment anodes of LEDITRON DIGIT 4 join on header J3 pin 8.
  - members: D8.A, D9.A, D10.A, D11.A, D12.A, D13.A, D14.A, J3.Pin_8
  - source: the main sheet
- **unnamed nets** — Digit 4 cathodes, one net each: Seg A..G to J3 pins 1..7 (A=D10, B=D13, C=D14, D=D12, E=D9, F=D8, G=D11).
  - members: D10.K–J3.Pin_1, D13.K–J3.Pin_2, D14.K–J3.Pin_3, D12.K–J3.Pin_4, D9.K–J3.Pin_5, D8.K–J3.Pin_6, D11.K–J3.Pin_7
  - source: the main sheet
- **Header/socket pairs (drawn mating, not a file net)** — Each digit's LED-side header is drawn plugged into a bus-side socket at the same origin. Pin n reaches pin n only through the connector mating; the file does not join them.
  - members: J1 Conn_01x08_Pin over J5 Conn_01x08_Socket @158.75,68.58, J8 over J7 @191.77,68.58, J4 over J6 @233.68,68.58, J3 over J2 @266.70,68.58
  - source: the main sheet; Ambersong.kicad_sch '(at 5.08 7.62 180)' and '(at -5.08 7.62 0)'
- **SEG_A_BUS (by label name)** — Segment A bus: socket pin 1 of all four digits, linked by global label to the bus-driver 200 Ω.
  - members: J5.Pin_1, J7.Pin_1, J6.Pin_1, J2.Pin_1, 200Ω@270.51,101.60.#1
  - source: the main sheet; label names across all drawings
- **SEG_B_BUS (by label name)** — Segment B bus: socket pin 2 of all four digits, linked to the bus-driver 200 Ω.
  - members: J5.Pin_2, J7.Pin_2, J6.Pin_2, J2.Pin_2, 200Ω@264.16,104.14.#1
  - source: the main sheet; label names across all drawings
- **SEG_C_BUS (by label name)** — Segment C bus: socket pin 3 of all four digits, linked to the bus-driver 200 Ω.
  - members: J5.Pin_3, J7.Pin_3, J6.Pin_3, J2.Pin_3, 200Ω@256.54,106.68.#1
  - source: the main sheet; label names across all drawings
- **SEG_D_BUS (by label name)** — Segment D bus: socket pin 4 of all four digits, linked to the bus-driver 200 Ω.
  - members: J5.Pin_4, J7.Pin_4, J6.Pin_4, J2.Pin_4, 200Ω@247.65,109.22.#1
  - source: the main sheet; label names across all drawings
- **SEG_E_BUS (by label name)** — Segment E bus: socket pin 5 of all four digits, linked to the bus-driver 200 Ω.
  - members: J5.Pin_5, J7.Pin_5, J6.Pin_5, J2.Pin_5, 200Ω@238.76,111.76.#1
  - source: the main sheet; label names across all drawings
- **SEG_F_BUS (by label name)** — Segment F bus: socket pin 6 of all four digits, linked to the bus-driver 200 Ω.
  - members: J5.Pin_6, J7.Pin_6, J6.Pin_6, J2.Pin_6, 200Ω@229.87,114.30.#1
  - source: the main sheet; label names across all drawings
- **SEG_G_BUS (by label name)** — Segment G bus: socket pin 7 of all four digits, linked to the bus-driver 200 Ω. Like the other six, it is its own bus (SC29).
  - members: J5.Pin_7, J7.Pin_7, J6.Pin_7, J2.Pin_7, 200Ω@220.98,116.84.#1
  - source: the main sheet; label names across all drawings; SC29
- **D1 (by label name)** — Label D1 joins socket J5 pin 8 and the first digit P-MOSFET's drain; GPIO42 selects it (SC4), drawn through U5.I1.
  - members: J5.Pin_8, P-Mosfet.D
  - source: the main sheet; label names across all drawings; SC4
- **D2 (by label name)** — Label D2 joins socket J7 pin 8 and PFet1's drain; GPIO2 selects it (SC4), drawn through U5.I2.
  - members: J7.Pin_8, PFet1.D
  - source: the main sheet; label names across all drawings; SC4
- **D3 (by label name)** — Label D3 joins socket J6 pin 8 and PFet2's drain; GPIO40 selects it (SC4), drawn through U5.I3.
  - members: J6.Pin_8, PFet2.D
  - source: the main sheet; label names across all drawings; SC4
- **D4 (by label name)** — Label D4 joins socket J2 pin 8 and PFet3's drain; GPIO41 selects it (SC4), drawn through U5.I4.
  - members: J2.Pin_8, PFet3.D
  - source: the main sheet; label names across all drawings; SC4

### On sheet: 7 SEGMENTS 24H CLOCK DISPLAY

- **5V = 5VDC** — Colon harness supply contact to the 33 Ω.
  - members: COLON_IN.p2/2, U34.p1/2, 33Ω.#1, 5V(l), 5VDC(g)
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **unnamed nets** — 33 Ω to colon output connector U35, first contact.
  - members: 33Ω.#2, U35.p1/2
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **GND** — Colon harness GND contact to the variable resistor.
  - members: COLON_IN.p1/2, U34.p2/2, R99.#1 (variable resistor glued at 1.5 kΩ; R1 until 2026-09-23, SCE2), GND(l), GND(g)
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY; the author
- **unnamed nets** — Variable resistor to colon output connector U35, second contact.
  - members: R99.#2, U35.p2/2
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **Note beside COLON_IN (verbatim)** — "Caution! This 5VDC cable's polarity is / reversed at the COLON_IN side compared to others!".
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY; the author
- **SEG_A_BUS (aliases IN SEG A, D1..D4 SEG A)** — Segment A: first contact of the 7-way input and of all four 8-way digit outputs.
  - members: LEDITRON_IN.p1/7, U12.p1/7, U18.p1/8, U17.p1/8, U19.p1/8, U32.p1/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_B_BUS** — Segment B: second contact of input and all four digit outputs.
  - members: LEDITRON_IN.p2/7, U12.p2/7, U18.p2/8, U17.p2/8, U19.p2/8, U32.p2/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_C_BUS** — Segment C: third contact.
  - members: LEDITRON_IN.p3/7, U12.p3/7, U18.p3/8, U17.p3/8, U19.p3/8, U32.p3/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_D_BUS** — Segment D: fourth contact.
  - members: LEDITRON_IN.p4/7, U12.p4/7, U18.p4/8, U17.p4/8, U19.p4/8, U32.p4/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_E_BUS** — Segment E: fifth contact.
  - members: LEDITRON_IN.p5/7, U12.p5/7, U18.p5/8, U17.p5/8, U19.p5/8, U32.p5/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_F_BUS** — Segment F: sixth contact.
  - members: LEDITRON_IN.p6/7, U12.p6/7, U18.p6/8, U17.p6/8, U19.p6/8, U32.p6/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **SEG_G_BUS** — Segment G: seventh contact.
  - members: LEDITRON_IN.p7/7, U12.p7/7, U18.p7/8, U17.p7/8, U19.p7/8, U32.p7/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **D1** — Digit 1 common: fourth ordinal of the 4-way driver input to contact 8 of output '1'.
  - members: DRIVER_IN.p4/4, U33.p4/4, U18.p8/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **D2** — Digit 2 common to contact 8 of output '2'.
  - members: DRIVER_IN.p3/4, U33.p3/4, U17.p8/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **D3** — Digit 3 common to contact 8 of output '3'.
  - members: DRIVER_IN.p2/4, U33.p2/4, U19.p8/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY
- **D4** — Digit 4 common: first ordinal of the driver input to contact 8 of output '4'.
  - members: DRIVER_IN.p1/4, U33.p1/4, U32.p8/8
  - source: sheet 7 SEGMENTS 24H CLOCK DISPLAY

## 9. Display drivers: digit select (P-MOSFETs) and segment sinks

### On the main sheet

- **digit 1 select** — GPIO42 → digit 1 (SC4): label GPIO42 → U5.I1 → U5.O1 = PFET1 → 1 kΩ → P-Mosfet gate; its drain is D1.
  - members: label GPIO42, U5.I1 (#1), U5.O1 (#16) = PFET1, 1kΩ@153.67,135.89, P-Mosfet (FQP27P06) gate, P-Mosfet.D = D1
  - source: SC4; the main sheet
- **digit 2 select** — GPIO2 → digit 2 (SC4): label GPIO2 → U5.I2 → PFET2 → PFet1; drain D2.
  - members: label GPIO2, U5.I2 (#2), U5.O2 (#15) = PFET2, 1kΩ@189.23,135.89, PFet1 (FQP27P06) gate, PFet1.D = D2
  - source: SC4; the main sheet
- **digit 3 select** — GPIO40 → digit 3 (SC4): label GPIO40 → U5.I3 → PFET3 → PFet2; drain D3.
  - members: label GPIO40, U5.I3 (#3), U5.O3 (#14) = PFET3, 1kΩ@224.79,135.89, PFet2 (FQP27P06) gate, PFet2.D = D3
  - source: SC4; the main sheet
- **digit 4 select** — GPIO41 → digit 4 (SC4): label GPIO41 → U5.I4 → PFET4 → PFet3; drain D4.
  - members: label GPIO41, U5.I4 (#4), U5.O4 (#13) = PFET4, 1kΩ@261.62,135.89, PFet3 (FQP27P06) gate, PFet3.D = D4
  - source: SC4; the main sheet
- **PFET1 / PFET2 / PFET3 / PFET4** — Each U5 output reaches its gate resistor by global label name only (no wire).
  - members: U5.O1 ↔ 1kΩ@153.67,135.89.#2, U5.O2 ↔ 1kΩ@189.23,135.89.#2, U5.O3 ↔ 1kΩ@224.79,135.89.#2, U5.O4 ↔ 1kΩ@261.62,135.89.#2
  - source: label names across all drawings; the main sheet
- **unnamed net (gates)** — Each P-FET gate is fed through 1 kΩ and pulled to 5VDC (its source) by 100 kΩ.
  - members: 1kΩ@153.67.#1, 100kΩ@160.02,138.43.#1, P-Mosfet.G, 1kΩ@189.23.#1, 100kΩ@195.58,138.43.#1, PFet1.G, 1kΩ@224.79.#1, 100kΩ@231.14,138.43.#1, PFet2.G, 1kΩ@261.62.#1, 100kΩ@267.97,138.43.#1, PFet3.G
  - source: the main sheet
- **5VDC (digit sources)** — Each digit's source, gate pull-up and one side (#1) of its 0.1 µF and 10 µF are on 5VDC.
  - members: P-Mosfet.S, 100kΩ@160.02.#2, 0.1uF@160.02,146.05.#1, 10uF@165.10,146.05.#1, PFet1.S, 100kΩ@195.58.#2, 0.1uF@195.58.#1, 10uF@200.66.#1, PFet2.S, 100kΩ@231.14.#2, 0.1uF@231.14.#1, 10uF@236.22.#1, PFet3.S, 100kΩ@267.97,138.43.#2, 0.1uF@267.97.#1, 10uF@273.05.#1
  - source: the main sheet
- **GND (digit caps)** — Each digit block's two capacitors return to GND.
  - members: 0.1uF@160.02.#2, 10uF@165.10.#2, 0.1uF@195.58.#2, 10uF@200.66.#2, 0.1uF@231.14.#2, 10uF@236.22.#2, 0.1uF@267.97.#2, 10uF@273.05.#2
  - source: the main sheet
- **D1 / D2 / D3 / D4** — The four P-FET drains are the digit outputs D1-D4.
  - members: P-Mosfet.D, PFet1.D, PFet2.D, PFet3.D
  - source: the main sheet
- **U5 power and unused pins** — Digit driver ground on GND; COM and channels 5-7 no-connect.
  - members: U5.GND (#8) → GND, U5.COM (#9) NC, U5.I5-I7, O5-O7 NC
  - source: the main sheet
- **segment inputs** — Seven S3 GPIOs drive U2 inputs 1-7 (segments A-G).
  - members: GPIO6 → U2.I1, GPIO7 → U2.I2, GPIO15 → U2.I3, GPIO16 → U2.I4, GPIO17 → U2.I5, GPIO8 → U2.I6, GPIO18 → U2.I7
  - source: the main sheet
- **U2 power and COM** — Segment driver ground on GND; COM no-connect.
  - members: U2.GND (#8) → GND, U2.COM (#9) NC
  - source: the main sheet
- **U2 outputs to first resistor** — Each U2 output feeds a 10 Ω.
  - members: U2.O1 – 10Ω@260.35,101.60.#2, U2.O2 – 10Ω@252.73,104.14.#2, U2.O3 – 10Ω@243.84,106.68.#2, U2.O4 – 10Ω@234.95,109.22.#2, U2.O5 – 10Ω@226.06,111.76.#2, U2.O6 – 10Ω@217.17,114.30.#2, U2.O7 – 10Ω@213.36,116.84.#2 (, pin on pin)
  - source: the main sheet
- **channel mid-nodes A-F** — On each channel A–F the 10 Ω and 200 Ω are in series, and the mid-node has its own 100 kΩ to 5VDC: the seven channels are separate, one 100 kΩ per segment (SC29 done).
  - members: 10Ω@260.35.#1 – 200Ω@270.51.#2 – 100kΩ@267.97,119.38.#1, 10Ω@252.73.#1 – 200Ω@264.16.#2 – 100kΩ@260.35,119.38.#1, 10Ω@243.84.#1 – 200Ω@256.54.#2 – 100kΩ@252.73,119.38.#1, 10Ω@234.95.#1 – 200Ω@247.65.#2 – 100kΩ@243.84,119.38.#1, 10Ω@226.06.#1 – 200Ω@238.76.#2 – 100kΩ@234.95,119.38.#1, 10Ω@217.17.#1 – 200Ω@229.87.#2 – 100kΩ@226.06,119.38.#1
  - source: the main sheet; the author; SC29
- **channel G mid-node** — Channel G mid-node with its 100 kΩ pull-up to 5VDC.
  - members: 10Ω@213.36,116.84.#1, 200Ω@220.98,116.84.#2, 100kΩ@217.17,119.38.#1
  - source: the main sheet
- **SEG_A_BUS..SEG_G_BUS** — Each 200 Ω output end is its segment bus. There are seven separate buses (SC29).
  - members: 200Ω@270.51.#1 = SEG_A_BUS, 200Ω@264.16.#1 = SEG_B_BUS, 200Ω@256.54.#1 = SEG_C_BUS, 200Ω@247.65.#1 = SEG_D_BUS, 200Ω@238.76.#1 = SEG_E_BUS, 200Ω@229.87.#1 = SEG_F_BUS, 200Ω@220.98.#1 = SEG_G_BUS
  - source: the main sheet; SC29
- **5VDC (segment pull-up row)** — The bottom ends of all seven segment-section 100 kΩ are on 5VDC.
  - members: 100kΩ@217.17/226.06/234.95/243.84/252.73/260.35/267.97 ,119.38 .#2, 5VDC global label (275.59,121.92)
  - source: the main sheet
- **sheet box** — The sub-sheet box carries no sheet pins.
  - members: P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER box (153.03,90.17) 131.4x59.7, 0 pins
  - source: the main sheet

### On sheet: P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER

- **DISPLAY IN (segment inputs)** — The 7-pin JST DISPLAY_IN jack mates with header U24 and brings the seven segment GPIOs onto the driver board.
  - members: U24.p1/7–DISPLAY_IN.p1/7 GPIO6 = SEG DRIVER I1, p2 GPIO7 = I2, p3 GPIO15 = I3, p4 GPIO16 = I4, p5 GPIO17 = I5, p6 GPIO8 = I6, p7 GPIO18 = I7
  - source: sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER
- **DISPLAY OUT (segment buses)** — Header U20 and jack DISPLAY_OUT carry the seven separate segment buses out to the display.
  - members: U20.p1/7–DISPLAY_OUT.p1/7 SEG DRIVER O1 = SEG_A_BUS, p2 O2 = SEG_B_BUS, p3 O3 = SEG_C_BUS, p4 O4 = SEG_D_BUS, p5 O5 = SEG_E_BUS, p6 O6 = SEG_F_BUS, p7 O7 = SEG_G_BUS
  - source: sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER
- **DIGIT IN** — The 4-pin DIGIT_IN cable brings the four digit-select GPIOs, in the SC4 order.
  - members: U29.p1/4–DIGIT_IN.p1/4 = P_MOSFET I1 ← GPIO42 (digit 1), p2 = I2 ← GPIO2 (digit 2), p3 = I3 ← GPIO40 (digit 3), p4 = I4 ← GPIO41 (digit 4)
  - source: SC4; sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER
- **DIGIT OUT** — The four drain outputs leave on the 4-pin DIGIT_OUT jack; header pin order is reversed relative to the jack's.
  - members: U26.p4/4–DIGIT_OUT@184.15,102.87.p1/4 = P_MOSFET O1 = D1, U26.p3/4–p2/4 = O2 = D2, U26.p2/4–p3/4 = O3 = D3, U26.p1/4–p4/4 = O4 = D4
  - source: sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER
- **GND X2** — The GND×2 pair is the cable shield + foil terminating at the perfboard's common GND. DIGIT_OUT and DIGIT_OUT1 are one cable split by two JSTs.
  - members: U22.p1/2–DIGIT_OUT@184.15,77.47.p1/2 GND, U22.p2/2–DIGIT_OUT@184.15,77.47.p2/2 GND
  - source: the author; sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER
- **VDC IN** — The driver board's 5 V and ground come in on the 2-pin DRIVERS_5VIN jack.
  - members: U28.p1/2–DRIVERS_5VIN.p1/2 GND, U28.p2/2–DRIVERS_5VIN.p2/2 5VDC
  - source: sheet P-MOSFET DIGITS & P-MOSFET DIGIT DRIVER & 7 SEGMENTS BUS DRIVER

## 10. Power distribution, mains side, earth and antenna

### On the main sheet

- **AC_L_IN** — Mains live from the IEC C14 inlet. It feeds the front-panel toggle's inlet side and, by the same name, the 5 V PSU's AC+. As drawn, the 5 V PSU sits ahead of the toggle.
  - members: J30.L (#1), SW1.B (#2), U4.AC+
  - source: the main sheet; label names across all drawings
- **AC_L** — Switched live from the front-panel power toggle SW1, to the amp PSU.
  - members: SW1.A (#1), U1.AC+ (AMP PSU, other group)
  - source: the main sheet; label names across all drawings
- **AC_N** — Mains neutral from the inlet, to the 5 V PSU and the amp PSU.
  - members: J30.N (#2), U4.AC-, U1.AC- (AMP PSU, other group)
  - source: label names across all drawings
- **Earth** — Earth: the inlet's E pin, the amp PSU earth, the radio's Faraday cage and the antenna connector's shell. Earth is a global power name, so these are one net. A Faraday cage sits over the AC line filter (between the tube radio's mains input and Ambersong's AC input, after the front power switch), bonded to the same earth; it is not drawn.
  - members: J30.E (#3) via #PWR03, U1.EARTH via #PWR04 (other group), FARADAY_CAGE label via #PWR01, J10.Ext (#2, coax shell)
  - source: the main sheet, with its Earth power symbol description; SC30
- **Earth and GND are joined in the machine, not on the drawings** — The amp's ground reaches the earth star point on its back aluminium plate through a washer on its Bass pot, so the DC-side GND is on mains earth at almost 0 Ω (§30.4).
  - source: the author
- **ANT** — Antenna signal on the coax connector's centre pin. The name ANT appears on no other drawing, so its destination is not drawn; the antenna (any FM/VHF antenna works, and the author made one for this) is §29, as built and installed. J10 is deliberately just a coax connector: it was F-type and is now a BNC male (the type does not matter).
  - members: J10.In (#1, coax centre)
  - source: SC30; label names across all drawings
- **FARADAY_CAGE = Earth** — The Faraday cage over the tube radio is bonded to Earth. Note B30 also says the cage over the AS5600 goes to EARTH (text only, no drawn symbol).
  - members: global label FARADAY_CAGE (134.62,162.56), #PWR01 Earth (128.27,167.64)
  - source: the main sheet
- **5VDC (= 5VDC_S)** — The 5 V PSU output feeds the 5 V star-point bus, 15 positions, drawn on the bus sub-sheet (box HERE) only: the main sheet no longer draws the bus. On the drawings 5VDC_S is on U4.5VDC alone; it is 5VDC, 5VDC_S being only the pre-bus name.
  - members: U4.5VDC (5VDC_S); by name, every 5VDC label on every drawing, the bus sub-sheet's 15 5 VDC positions included (On sheet: HERE (the star-point buses), below)
  - source: the main sheet; sheet HERE (the star-point buses); label names across all drawings; the author; SC15
- **GND (= PSU_GND)** — All the grounds are joined at a single star point, represented by the JST 2-pin buses (note B29). PSU_GND is the pre-bus name for GND.
  - members: U4.GND (PSU_GND); by name, every GND label on every drawing, the bus sub-sheet's GND pins included (On sheet: HERE (the star-point buses), below)
  - source: the main sheet; sheet HERE (the star-point buses); the author
- **A_VDC / A_3V3 bus** — The 5-position 3.3 VDC JST 2-pin bus, drawn on the bus sub-sheet (box HERE) only since the main sheet's drawing of the buses was removed; supplied from the A32's 3V3 pin with its three decoupling caps on the main sheet.
  - members: A_3V3 by name: U10.3V3 with C20 0.1 µF, C21 10 µF, C22 10 µF (main sheet); bus sub-sheet U52–U56 p1/2 with A32_VDC.p1/2 and A32_3V3.p1/2 (A_3V3 = A_VDC), U52–U56 p2/2 on GND
  - source: the main sheet; sheet HERE (the star-point buses); label names across all drawings; SC17; the author
- **SW1 placement** — SW1 (SPST, block 'FRONT PANEL / POWER TOGGLE') sits in the live leg between AC_L_IN and AC_L.
  - members: SW1.B on AC_L_IN, SW1.A on AC_L
  - source: the main sheet

### On sheet: HERE (the star-point buses)

- **A_3V3 = A_VDC** — The + pin of all five 3.3 VDC connectors is one wire. It joins the A32_VDC jack and the A32_3V3 jack, so A_VDC and A_3V3 are drawn as one net here.
  - members: U56.p1/2, U55.p1/2, U54.p1/2, U53.p1/2, U52.p1/2, A32_VDC.p1/2 (label A_VDC), A32_3V3.p1/2 (label A_3V3)
  - source: sheet HERE (the star-point buses)
- **GND** — A wired ground covering the 3.3 VDC column, both A32 jacks and the first 5 VDC column.
  - members: U56..U52 p2/2, A32_VDC.p2/2, A32_3V3.p2/2, U61..U57 p1/2
  - source: sheet HERE (the star-point buses)
- **5VDC** — The + pins of the first 5 VDC column (drawn block B5).
  - members: U61.p2/2, U60.p2/2, U59.p2/2, U58.p2/2, U57.p2/2
  - source: sheet HERE (the star-point buses)
- **GND** — A wired ground for the second and third 5 VDC columns (drawn blocks B6, B7). It joins by the name GND.
  - members: U51..U47 p1/2, U46.p1/2, U31.p1/2, U30.p1/2, U27.p1/2, U25.p1/2
  - source: sheet HERE (the star-point buses)
- **5VDC** — The + pins of the columns in drawn blocks B6 and B7. It joins by the name 5VDC. With block B5, that makes 15 5 VDC positions.
  - members: U51..U47 p2/2, U46.p2/2, U31.p2/2, U30.p2/2, U27.p2/2, U25.p2/2
  - source: sheet HERE (the star-point buses); SC15

## 11. Stepper and index sensor, AS5600, RDA5807M, panel lamps, BT button/LED, audio source switch

*Specific to this build — adapt: the dial-needle drive (the stepper, its index sensor, the AS5600 and its magnet), the panel lamps and the front-panel controls suit this cabinet and this dial.*

### On the main sheet

- **GPIO9 / GPIO10 / GPIO11 / GPIO12** — S3 GPIO9, 10, 11, 12 drive ULN2003A inputs I1, I2, I3, I4 (linked by global label names).
  - members: U3.I1 (#1), U3.I2 (#2), U3.I3 (#3), U3.I4 (#4), U7.9, U7.10, U7.11, U7.12
  - source: the main sheet
- **unnamed net** — ULN2003A outputs O1-O4 go to stepper socket J9 pins 1-4 ('To 28BYJ-48'). J9 is the connector that comes with the common ULN2003A boards.
  - members: U3.O1 (#16)-J9.Pin_1, U3.O2 (#15)-J9.Pin_2, U3.O3 (#14)-J9.Pin_3, U3.O4 (#13)-J9.Pin_4
  - source: the main sheet
- **5VDC** — ULN2003A COM and the motor common (J9 pin 5) are on 5VDC, with 0.1 µF and 47 µF to GND.
  - members: U3.COM (#9), J9.Pin_5, 0.1uF@142.24,101.60.#1, 47uF.#1
  - source: the main sheet
- **GND** — Stepper driver decoupling returns and ULN2003A GND pin to GND.
  - members: 0.1uF@142.24,101.60.#2, 47uF.#2, U3.GND (#8)
  - source: the main sheet
- **5VDC / GND** — A3144 Hall sensor U42 on 5VDC with C17 0.1 µF across its supply.
  - members: U42.VCC, C17.#1, U42.GND, C17.#2
  - source: the main sheet
- **GPIO5** — Hall output to S3 GPIO5, pulled up to S3_3V3 by 10 kΩ, 0.1 µF to GND at the S3.
  - members: U42.OUT, U7.5, 10kΩ@27.94,143.51 (to S3_3V3), 0.1uF@19.02,151.13 (to GND)
  - source: the main sheet; label names across all drawings
- **S3_3V3 / GND** — AS5600 module on S3_3V3; GND; DIR tied to GND. Its magnet is a 6 × 3 mm disc neodymium magnet, less than 0.5 mm from the IC.
  - members: U11.VCC, U11.GND, U11.DIR
  - source: the main sheet
- **GPIO38 (AS5600 SDA)** — AS5600 SDA → 220 Ω → S3 GPIO38, with 10 kΩ pull-up to S3_3V3 at the pin.
  - members: U11.SDA, 220Ω@58.42,166.37, U7.38, 10kΩ@58.42,161.29 to S3_3V3
  - source: the main sheet; the author
- **GPIO39 (AS5600 SCL)** — AS5600 SCL → 220 Ω → S3 GPIO39, with 10 kΩ pull-up to S3_3V3 at the pin.
  - members: U11.SCL, 220Ω@63.50,158.75, U7.39, 10kΩ@60.96,160.02 to S3_3V3
  - source: the main sheet; the author
- **GPIO47 / GPIO48** — RDA5807M SDIO on S3 GPIO47, SCLK on GPIO48; no external pull-ups.
  - members: U89.SDIO (#5), U7.47, U89.SCLK (#4), U7.48
  - source: the main sheet; the author
- **30CM_ANT** — RDA5807M ANT carries only the local label 30CM_ANT: a 30 cm length of wire inside the tube radio's Faraday cage, bent in a U over the FM oscillator section. The RDA module reaches that wire through U71, a 2×3 pin grid that lets someone making repairs unplug the module from the antenna (drawn on sheet RDA5807M).
  - members: U89.ANT
  - source: the main sheet
- **S3_3V3 / GND — RDA supply** — RDA5807M VDD on S3_3V3, GND on GND; 100 µF electrolytic, 10 µF and 0.1 µF ceramic each directly across VDD–GND.
  - members: U89.VDD (#7), U89.GND (#1), C28 100uF, C29 0.1uF, C30 10uF
  - source: the main sheet; SC14
- **RDA no-connects** — RCLK, IO1, IO2, ROUT, LOUT have no-connect X.
  - members: U89.RCLK (#6), U89.IO1, U89.IO2, U89.ROUT (#9), U89.LOUT (#10)
  - source: the main sheet
- **5VDC** — Panel lamp LED anodes (LED 1-4) on 5VDC.
  - members: D30.A, D31.A, D32.A, D33.A
  - source: the main sheet
- **unnamed nets** — Each LED cathode goes through its own 120 Ω.
  - members: D30.K-120Ω@95.25,101.60.#2, D31.K-120Ω@95.25,106.68.#2, D32.K-120Ω@95.25,111.76.#2, D33.K-120Ω@95.25,116.84.#2
  - source: the main sheet; the author
- **unnamed net (drain)** — The four 120 Ω resistors join on the IRL540N drain.
  - members: Q1.D (#1), 120Ω@95.25,101.60.#1, 120Ω@95.25,106.68.#1, 120Ω@95.25,111.76.#1, 120Ω@95.25,116.84.#1
  - source: the main sheet; the author
- **unnamed net (gate) / GPIO21** — S3 GPIO21 → 100 Ω → Q1 gate; 100 kΩ and 220 pF from gate to GND.
  - members: Q1.G (#2), 100Ω.#1, 100kΩ@86.36,121.92.#2, 220pF.#1, 100Ω.#2 = GPIO21, U7.21
  - source: the main sheet; the author
- **GND** — Q1 source, gate pull-down and gate cap to GND.
  - members: Q1.S (#3), 100kΩ@86.36,121.92.#1, 220pF.#2
  - source: the main sheet; the author
- **A_IO25 / GND — BT pair button** — Pair button SW2 between A32 GPIO25 and GND (active low, INPUT_PULLUP).
  - members: SW2.1, U10.25, SW2.2 → GND
  - source: the main sheet; the author; SC3; firmware (§0)
- **A_VDC / A_IO33** — Front-panel BT LED D29: anode on A_VDC, cathode on A32 GPIO33 (drive low to light); no series resistor drawn.
  - members: D29.A, D29.K, U10.33
  - source: the main sheet; the author; SC3; firmware (§0)
- **A_VDC / A_IO36 / GND** — DPDT pins 2 and 5 on A_VDC; pin 1 straight to the A_IO36 node; pin 6 through 68 kΩ to it; 22 kΩ from the node to GND; pins 3 and 4 no-connect. The 22 k / 68 k are physically on the A32 board. Positions, from the firmware: the direct leg (pin 1) puts 3.3 V on GPIO36 = RADIO; the 68 kΩ leg (pin 6) puts 0.81 V = BT; the centre, off, leaves the 22 kΩ at 0 V = AUX. The A32 reads below 424 counts as AUX, 2471 and up as RADIO, and between as BT.
  - members: DPDT_ON_OFF_ON.B (#2), DPDT_ON_OFF_ON.B (#5), DPDT_ON_OFF_ON.A (#1), DPDT_ON_OFF_ON.C (#6)-68kΩ, 22kΩ to GND, DPDT C (#3) nc, DPDT A (#4) nc
  - source: the main sheet; the author; firmware (§0)
- **A_IO36 at the A32** — A_IO36 node → 1 kΩ → A32 GPIO36; the 0.1 µF to GND sits on the pin (A32) side.
  - members: 1kΩ@80.01,147.32, U10.36, 0.1uF@74.93,144.78
  - source: the main sheet; the author
- **no-connects (stepper, AS5600)** — ULN2003A channels 5-7 and AS5600 OUT carry no-connect X.
  - members: U3.I5, U3.I6, U3.I7, U3.O5, U3.O6, U3.O7, U11.OUT
  - source: the main sheet

### On sheet: STEPPER DRIVER

- **GPIO9-12 = ULN IN1-4** — 4-wire cable into the driver board's INPUT header, IN1-IN4 in GPIO order.
  - members: STEPPER_IN.p1/4-U38.p1/4 GPIO9, STEPPER_IN.p2/4-U38.p2/4 GPIO10, STEPPER_IN.p3/4-U38.p3/4 GPIO11, STEPPER_IN.p4/4-U38.p4/4 GPIO12
  - source: sheet STEPPER DRIVER
- **ULN OUT1-4 / ULN COM** — TO STEPPER header pins 1-4 carry OUT1-OUT4 (named coil 4..1); pin 5 is COM.
  - members: U39.p1/5 ULN OUT1 = STEPPER COIL 4, U39.p2/5 ULN OUT2 = STEPPER COIL 3, U39.p3/5 ULN OUT3 = STEPPER COIL 2, U39.p4/5 ULN OUT4 = STEPPER COIL 1, U39.p5/5 ULN COM = STEPPER COM
  - source: sheet STEPPER DRIVER
- **5VDC = ULN2003A COM / GND = ULN2003A GND** — Driver board power header: '+' 5VDC, '−' GND.
  - members: STEPPER_5VIN.p2/2, U37.p2/2, STEPPER_5VIN.p1/2, U37.p1/2
  - source: sheet STEPPER DRIVER

### On sheet: STEPPER LIMIT

- **5VDC = VCC / GND / GPIO5 = OUT** — Sensor cable: conductor 1 5VDC to VCC, 2 GND, 3 sensor OUT to GPIO5.
  - members: LIMIT.p1/3-U90.p3/3, LIMIT.p2/3-U90.p2/3, LIMIT.p3/3-U90.p1/3
  - source: sheet STEPPER LIMIT

### On sheet: RDA5807M

- **GPIO47 = SDA / GPIO48 = SCL** — I2C cable conductor 1 SDA (GPIO47), 2 SCL (GPIO48).
  - members: RDA_I2C.p1/3-U92.p3/3, RDA_I2C.p2/3-U92.p2/3
  - source: sheet RDA5807M
- **CABLE S/FTP (labelled GND at module header)** — Third conductor = cable shield + foil, terminated to GND at the board side and open at the cable's other end (S3_RDA at the S3 is 2-pin).
  - members: RDA_I2C.p3/3, U92.p1/3
  - source: sheet RDA5807M; label names across all drawings; the author
- **S3_3V3 = VDD / GND** — 3V3 cable '+' S3_3V3, '−' GND; C18 100 µF (polarized), C31 10 µF and C19 0.1 µF across the supply at the module (SC14 done 2026-09-23).
  - members: RDA_3V3.p2/2-U93.p1/2, C18 100uF (polarized), C31 10uF, C19 0.1uF, RDA_3V3.p1/2-U93.p2/2
  - source: sheet RDA5807M; SC14
- **30CM_ANT / U71 — the antenna plug** — U71 is a 2×3 pin grid that physically connects the RDA5807M module to the antenna, so someone making repairs can unplug the module from the antenna there. Its symbol is drawn without pins. The label 30CM_ANT (the same as the main sheet's 30CM_ANT) is drawn reaching no pin.
  - members: U71 (Mine:2x3_Pin_GRID, drawn without pins), label 30CM_ANT (reaches no pin)
  - source: sheet RDA5807M; the author

### On sheet: PANEL RADIO LED PWM CONTROL

- **GPIO21 / 5VDC / GND** — PWM board input: GPIO21 via 100 Ω to the MOSFET gate, 5VDC out through 5VOUT to the LED anodes, GND = source.
  - members: RLPC_IN.p1/3-U82.p3/3 GPIO21 → 100Ω → gate, RLPC_IN.p2/3-U82.p2/3 5VDC → U85.p1/2, U85.p2/2 → D46-D49 anodes, RLPC_IN.p3/3-U82.p1/3 GND = S
  - source: sheet PANEL RADIO LED PWM CONTROL; SC33
- **LEDS RTN** — Each LED cathode returns through LEDS RTN to its own 120 Ω; the four resistors join on the MOSFET drain.
  - members: U86.p1/4-D46.K, U86.p2/4-D47.K, U86.p3/4-D48.K, U86.p4/4-D49.K, four LED resistors (120Ω) → drain
  - source: sheet PANEL RADIO LED PWM CONTROL; SC2; SC33

### On sheet: BLUETOOTH PAIR BUTTON & FRONT PANEL BLUETOOTH LED & DPDT ON-OFF-ON AUDIO SWITCH

- **A_IO25 / A_3V3 / A_IO33 / GND** — Pair button between GPIO25 and GND; LED anode on A_3V3 (= A_VDC), cathode on GPIO33.
  - members: SW4 → A_IO25, SW4 other side → GND (A32_BT_LED.p4/4, U36.p4/4), LED + → A_3V3 (A32_BT_LED.p2/4, U36.p2/4), LED − (cathode) → A_IO33
  - source: sheet BLUETOOTH PAIR BUTTON & FRONT PANEL BLUETOOTH LED & DPDT ON-OFF-ON AUDIO SWITCH; label names across all drawings; the author; SC3; firmware (§0)
- **A_VDC = 3V3 IN / A_IO36_H / A_IO36_L** — Three-wire switch cable through the IN/LISTENER/OUT board to the front DPDT switch; H is the leg that puts 3.3 V on the line, L the 0.8 V leg.
  - members: LISTENER.p1/3-U43.p1/3-U44.p1/3 A_VDC, LISTENER.p2/3-U43.p2/3-U44.p2/3 A_IO36_H, LISTENER.p3/3-U43.p3/3-U44.p3/3 A_IO36_L
  - source: sheet BLUETOOTH PAIR BUTTON & FRONT PANEL BLUETOOTH LED & DPDT ON-OFF-ON AUDIO SWITCH; the author

## 12. The cable harnesses (cable sheet "THIS")

The cable sheet is a visual aid: each harness is drawn as it looks when seen as drawn.

### On the main sheet

- **(none)** — G9 on the main sheet is only the note B31 with the sheet box 'THIS' over its blank. It has no parts, nets or sheet pins, so the cable sheet links to the main sheet only by label names.
  - members: THIS sheet symbol (0 sheet pins)
  - source: the main sheet

### On sheet: THIS

- **S3_3V3** — Cable S3_3V3 to RDA_3V3 carries the S3's 3.3 V to the RDA5807M.
  - members: S3_3V3.p1/2, RDA_3V3.p2/2
  - source: the cable sheet
- **GND** — Same cable, GND conductor.
  - members: S3_3V3.p2/2, RDA_3V3.p1/2
  - source: the cable sheet
- **GPIO48** — RDA5807M I2C cable, GPIO48.
  - members: S3_RDA.p1/2, RDA_I2C.p2/3
  - source: the cable sheet
- **GPIO47** — RDA5807M I2C cable, GPIO47.
  - members: S3_RDA.p2/2, RDA_I2C.p1/3
  - source: the cable sheet
- **CABLE S/FTP (RDA I2C cable)** — The RDA I2C cable's shield and foil end at RDA_I2C to GND on that board, and are open at the S3_RDA end.
  - members: RDA_I2C.p3/3
  - source: the cable sheet; the author
- **5VDC** — S3 5 V cable from a 5 V bus jack.
  - members: S3_5VIN.p2/2, 5VDC@86.36,43.18.p2/2
  - source: the cable sheet
- **GND** — S3 5 V cable, GND.
  - members: S3_5VIN.p1/2, 5VDC@86.36,43.18.p1/2
  - source: the cable sheet
- **GPIO6** — Segment-drive cable, GPIO6.
  - members: S3_DISPLAY.p1/7, DISPLAY_IN.p1/7
  - source: the cable sheet
- **GPIO7** — Segment-drive cable, GPIO7.
  - members: S3_DISPLAY.p2/7, DISPLAY_IN.p2/7
  - source: the cable sheet
- **GPIO15** — Segment-drive cable, GPIO15.
  - members: S3_DISPLAY.p3/7, DISPLAY_IN.p3/7
  - source: the cable sheet
- **GPIO16** — Segment-drive cable, GPIO16.
  - members: S3_DISPLAY.p4/7, DISPLAY_IN.p4/7
  - source: the cable sheet
- **GPIO17** — Segment-drive cable, GPIO17.
  - members: S3_DISPLAY.p5/7, DISPLAY_IN.p5/7
  - source: the cable sheet
- **GPIO8** — Segment-drive cable, GPIO8.
  - members: S3_DISPLAY.p6/7, DISPLAY_IN.p6/7
  - source: the cable sheet
- **GPIO18** — Segment-drive cable, GPIO18. The cable has no GND or shield conductor.
  - members: S3_DISPLAY.p7/7, DISPLAY_IN.p7/7
  - source: the cable sheet
- **GPIO12** — Stepper input cable, GPIO12.
  - members: S3_STEPPER.p1/4, STEPPER_IN.p4/4
  - source: the cable sheet
- **GPIO11** — Stepper input cable, GPIO11.
  - members: S3_STEPPER.p2/4, STEPPER_IN.p3/4
  - source: the cable sheet
- **GPIO10** — Stepper input cable, GPIO10.
  - members: S3_STEPPER.p3/4, STEPPER_IN.p2/4
  - source: the cable sheet
- **GPIO9** — Stepper input cable, GPIO9. No GND or shield.
  - members: S3_STEPPER.p4/4, STEPPER_IN.p1/4
  - source: the cable sheet
- **GPIO1** — Amp power-detect cable, GPIO1.
  - members: S3_PWR_DET.p1/2, POWER_DET.p2/2
  - source: the cable sheet
- **GND + CABLE S/FTP (power-detect cable)** — GND conductor end to end. The shield and foil are also terminated at the S3 end.
  - members: S3_PWR_DET.p2/2, POWER_DET.p1/2
  - source: the cable sheet; the author
- **GPIO41** — Digit-select cable, GPIO41 (digit 4 per SC4).
  - members: S3_DIGIT.p1/4, DIGIT_IN.p4/4
  - source: the cable sheet; SC4
- **GPIO40** — Digit-select cable, GPIO40 (digit 3 per SC4).
  - members: S3_DIGIT.p2/4, DIGIT_IN.p3/4
  - source: the cable sheet; SC4
- **GPIO2** — Digit-select cable, GPIO2 (digit 2 per SC4).
  - members: S3_DIGIT.p3/4, DIGIT_IN.p2/4
  - source: the cable sheet; SC4
- **GPIO42** — Digit-select cable, GPIO42 (digit 1 per SC4).
  - members: S3_DIGIT.p4/4, DIGIT_IN.p1/4
  - source: the cable sheet; SC4
- **GPIO5** — LIMITS harness, GPIO5, now the A3144 Hall sensor output (retrofit).
  - members: S3_LIMITS.p3/3, LIMIT.p3/3
  - source: the cable sheet; the author
- **GND + CABLE S/FTP (LIMITS harness)** — GND conductor. The shield and foil are also terminated at the S3 end.
  - members: S3_LIMITS.p1/3, LIMIT.p2/3
  - source: the cable sheet; the author
- **5VDC** — The sensor end of the LIMITS harness takes 5 V from a bus jack. The S3 connector has no 5 V pin.
  - members: LIMIT.p1/3, 5VDC_LIMIT.p2/2
  - source: the cable sheet
- **(no-connect)** — Drawn no-connect on the cable. GPIO4 is still wired at the S3-side JST with its 10 kΩ pull-up and 0.1 µF, but goes nowhere else.
  - members: S3_LIMITS.p2/3
  - source: the cable sheet; the author
- **GND + CABLE S/FTP (S3_TUNER, single-ended)** — AS5600 cable GND at the S3 end, with the shield and foil terminated there. No far-end connector is drawn.
  - members: S3_TUNER.p1/4
  - source: the cable sheet; the author
- **S3_3V3** — AS5600 cable, 3.3 V from the S3.
  - members: S3_TUNER.p2/4
  - source: the cable sheet
- **GPIO38** — AS5600 cable, GPIO38.
  - members: S3_TUNER.p3/4
  - source: the cable sheet
- **GPIO39** — AS5600 cable, GPIO39.
  - members: S3_TUNER.p4/4
  - source: the cable sheet
- **GPIO21** — Panel-lamp PWM cable, GPIO21, to the PWM board.
  - members: S3_FM_LED.p2/2, FM_LED_IN.p1/3
  - source: the cable sheet
- **CABLE S/FTP (panel-lamp cable)** — Shield and foil terminated at the S3 end to GND, open at the FM_LED_IN end.
  - members: S3_FM_LED.p1/2
  - source: the cable sheet; the author
- **5VDC** — The PWM board takes 5 V from a bus jack.
  - members: FM_LED_IN.p2/3, 5VDC_FM_LED.p2/2
  - source: the cable sheet
- **GND** — The PWM board takes GND from the same bus jack.
  - members: FM_LED_IN.p3/3, 5VDC_FM_LED.p1/2
  - source: the cable sheet
- **S3_to_A32** — UART from S3 GPIO13 to A32 GPIO26 (names on main-sheet U7.13 / U10.26).
  - members: S3_UART.p2/3, A32_UART.p1/3
  - source: the cable sheet; label names across all drawings
- **A32_to_S3** — UART from A32 GPIO27 to S3 GPIO14 (U10.27 / U7.14).
  - members: S3_UART.p3/3, A32_UART.p2/3
  - source: the cable sheet; label names across all drawings
- **GND + CABLE S/FTP (UART cable)** — GND conductor, with shield and foil terminated at both ends (both ends are marked).
  - members: S3_UART.p1/3, A32_UART.p3/3
  - source: the cable sheet; the author
- **A_IO35** — Volume pot wiper to A32 GPIO35.
  - members: A32_VOL.p1/2, VOLUME.p2/3
  - source: the cable sheet; SC6
- **CABLE S/FTP (volume cable)** — Shield and foil terminated at the A32 end to GND, open at the pot end.
  - members: A32_VOL.p2/2
  - source: the cable sheet; the author
- **A_VDC** — The pot's top end is on the A32's A_VDC.
  - members: A32_VDC_VOLUME.p1/2, VOLUME.p1/3
  - source: the cable sheet
- **GND** — The pot's bottom end is on GND.
  - members: A32_VDC_VOLUME.p2/2, VOLUME.p3/3
  - source: the cable sheet
- **A_3V3** — Front-panel BT LED anode supply (3.3 V).
  - members: A32_BT_LED.p1/4, BT_LED.p2/4
  - source: the cable sheet; the author; firmware (§0)
- **A_IO33** — BT LED cathode on GPIO33, active low.
  - members: A32_BT_LED.p2/4, BT_LED.p1/4
  - source: the cable sheet; the author; firmware (§0)
- **GND** — GND to the front panel, the BT pair button's return (button ground-referenced).
  - members: A32_BT_LED.p3/4, BT_LED.p4/4
  - source: the cable sheet; SC3
- **A_IO25** — BT pair button on GPIO25, INPUT_PULLUP, pressed = LOW.
  - members: A32_BT_LED.p4/4, BT_LED.p3/4
  - source: the cable sheet; the author; firmware (§0)
- **A_IO0A (clock at the ADC)** — One of the two GPIO0 clock branches, each with its own 33 Ω, both at the A32's pin. The cable sheet carries it from A32_I2S.p3/7 to ADC_I2S.p6/6; at the A32 it is the branch through 33Ω@167.64,102.87.
  - members: A32_I2S.p3/7, ADC_I2S.p6/6
  - source: the cable sheet; sheet AUDIO MCU ESP32-WROOM-32; the author; SC7; SC8
- **A_IO0D (clock at the DAC)** — The other GPIO0 clock branch, with its own 33 Ω at the A32's pin. The cable sheet carries it from A32_I2S.p1/7 to DAC_I2S.p1/6; at the A32 it is the branch through 33Ω@167.64,107.95.
  - members: A32_I2S.p1/7, DAC_I2S.p1/6
  - source: the cable sheet; sheet AUDIO MCU ESP32-WROOM-32; the author; SC7; SC8
- **A_IO4** — DAC data (GPIO4) to the PCM5102A only.
  - members: A32_I2S.p2/7, DAC_I2S.p3/6
  - source: the cable sheet
- **A_IO17** — Shared LRCK to both converters.
  - members: A32_I2S.p4/7, ADC_I2S.p4/6, DAC_I2S.p4/6
  - source: the cable sheet
- **CABLE S/FTP (I2S harness)** — Harness shield and foil terminated at the A32 end to GND, open at the converter ends.
  - members: A32_I2S.p5/7
  - source: the cable sheet; the author
- **A_IO18** — Shared BCK to both converters.
  - members: A32_I2S.p6/7, ADC_I2S.p2/6, DAC_I2S.p2/6
  - source: the cable sheet
- **A_IO19** — ADC data out (GPIO19) from the PCM1802.
  - members: A32_I2S.p7/7, ADC_I2S.p1/6
  - source: the cable sheet
- **GND** — DAC connector GND from a bus jack inside the harness outline.
  - members: DAC_I2S.p5/6, 5VDC_DAC.p1/2
  - source: the cable sheet
- **5VDC** — DAC connector 5 V from the same bus jack. The ADC connector carries no power.
  - members: DAC_I2S.p6/6, 5VDC_DAC.p2/2
  - source: the cable sheet
- **A_VDC** — RTC module supply from the A32's A_VDC.
  - members: A32_VDC@105.41,130.81.p1/2, RTC_3V3.p1/2
  - source: the cable sheet
- **GND** — RTC module GND.
  - members: A32_VDC@105.41,130.81.p2/2, RTC_3V3.p2/2
  - source: the cable sheet
- **A_IO22** — RTC I2C, GPIO22.
  - members: A32_RTC.p1/3, RTC_I2C.p3/4
  - source: the cable sheet
- **A_IO21** — RTC I2C, GPIO21, drawn on RTC_I2C p4/4; p1/4 and p2/4 are drawn no-connect (the 2026-09-23 save, which is right). The RTC_I2C 4th position is not connected; "4th position" counts the positions from one end, and the drawing's contact order does not settle which one that is (contact order inside a connector is not covered).
  - members: A32_RTC.p3/3, RTC_I2C.p4/4
  - source: the cable sheet; the author
- **CABLE S/FTP (RTC I2C cable)** — Shield and foil terminated at the A32 end to GND, open at the RTC end (RTC_I2C p1/4 and p2/4 are drawn no-connect).
  - members: A32_RTC.p2/3
  - source: the cable sheet; the author
- **5VDC** — A32 5 V cable from a bus jack.
  - members: 5VDC@105.41,158.75.p2/2, A32_5VIN.p2/2
  - source: the cable sheet
- **GND** — A32 5 V cable, GND.
  - members: 5VDC@105.41,158.75.p1/2, A32_5VIN.p1/2
  - source: the cable sheet
- **A_3V3** — A32 3.3 V cable (connector named A32_VDC, pin labelled A_3V3).
  - members: A32_VDC@105.41,170.18.p2/2, A32_3V3.p1/2
  - source: the cable sheet
- **GND** — A32 3.3 V cable, GND.
  - members: A32_VDC@105.41,170.18.p1/2, A32_3V3.p2/2
  - source: the cable sheet
- **A_IO36_L** — Source-select leg L: the front DPDT switch puts the GPIO36 line to 0.8 V. Ladder resistors on the A32 board.
  - members: A32_SWITCH.p1/3, LISTENER.p1/3
  - source: the cable sheet; the author
- **A_IO36_H** — Source-select leg H: the switch puts the line to 3.3 V.
  - members: A32_SWITCH.p2/3, LISTENER.p2/3
  - source: the cable sheet; the author
- **A_VDC** — A_VDC to the switch. The jack's other pin is no-connect, so no GND goes to the switch.
  - members: A32_VDC_LISTENER.p1/2, LISTENER.p3/3
  - source: the cable sheet
- **CABLE S/FTP (source-select cable)** — Shield and foil terminated at the A32 end to GND, open at the LISTENER end.
  - members: A32_SWITCH.p3/3
  - source: the cable sheet; the author
- **AMP_G** — DAC_OUT plug sleeve to amp input AMP_G, and the common end of both in-cable 470 Ω (the two on the − legs; all four signal legs carry a 470 Ω, the two others in series with AMP_L+ and AMP_R+ at the DAC end).
  - members: J49.#S, 470Ω@214.63,33.02.#1, 470Ω@214.63,38.10.#1, AMP_IN.p5/9
  - source: the cable sheet; SC13
- **AMP_R+** — DAC_OUT ring through a 470 Ω at the DAC end of the cable to AMP_R+.
  - members: J49.#R, 470Ω@212.09,30.48, AMP_IN.p7/9
  - source: the cable sheet; the author; SC13
- **AMP_L+** — DAC_OUT tip through a 470 Ω at the DAC end of the cable to AMP_L+.
  - members: J49.#T, 470Ω@212.09,40.64, AMP_IN.p3/9
  - source: the cable sheet; the author; SC13
- **Ferrite cores on this cable (not drawn)** — Six cores, three at each end: one HDMI-type sleeve (17.4 mm OD, 9.7 mm ID, 28.5 mm long, the same type as the antenna feedpoint cores of §29), one small toroid (16 mm OD, 3 mm wall, 8 mm long) and one large toroid (22 mm OD, 4 mm wall, 8 mm long). The cable passes through each once; it is under 12 inches long, so no turns. The toroids are salvaged and unidentified: no maker, part number, grade or magnetic figure is recorded, and their material is not the clip-ons'.
  - source: the author
- **AMP_R-** — AMP_R− is AMP_G through a 470 Ω in the cable.
  - members: 470Ω@214.63,33.02.#2, AMP_IN.p9/9
  - source: the cable sheet; SC13
- **AMP_L-** — AMP_L− is AMP_G through the other 470 Ω in the cable.
  - members: 470Ω@214.63,38.10.#2, AMP_IN.p1/9
  - source: the cable sheet; SC13
- **(no-connect)** — AMP_IN even positions unused.
  - members: AMP_IN.p2/9, AMP_IN.p4/9, AMP_IN.p6/9, AMP_IN.p8/9
  - source: the cable sheet
- **Ferrite cores on this cable (not drawn)** — Two clip-on cores, one near the tube radio transformer T1 end, one near the ADC end; the cable passes through each once. Measured: 10 mm OD, 3.5 mm ID, 3.5 mm walls, 20 mm long (the figures do not close: 3.5 + 3.5 + 3.5 is 10.5 against a 10 mm OD, so one is rounded). Material as supplied with the cores, with no maker, part number or grade: rectangle ratio 20, coercivity 16 A/m, remanence 200 mT, Curie temperature 130 °C, density 4.9 g/cm³, "Soft Magnetic".
  - source: the author
- **CABLE S/FTP (radio cable)** — The sleeve of the RADIO_TO_ADC plug J50 carries the cable's shield and foil.
  - members: J50.#S
  - source: the cable sheet; the author
- **RAD_L** — One transformer output side (tip/ring interchangeable).
  - members: J50.#R
  - source: the cable sheet; the author
- **RAD_R** — The other transformer output side.
  - members: J50.#T
  - source: the cable sheet; the author
- **5VDC** — Colon LED 5 V cable.
  - members: COLON_IN.p2/2, 5VDC@250.19,71.12.p2/2
  - source: the cable sheet
- **GND** — Colon LED cable GND.
  - members: COLON_IN.p1/2, 5VDC@250.19,71.12.p1/2
  - source: the cable sheet
- **5VDC** — Stepper driver 5 V cable.
  - members: STEPPER_5VIN.p2/2, 5VDC@250.19,83.82.p2/2
  - source: the cable sheet
- **GND** — Stepper driver cable GND.
  - members: STEPPER_5VIN.p1/2, 5VDC@250.19,83.82.p1/2
  - source: the cable sheet
- **5VDC** — ADC 5 V cable (ADC_5VIN.p1/2 is on 5VDC).
  - members: ADC_5VIN.p1/2, 5VDC@250.19,95.25.p2/2
  - source: the cable sheet
- **GND** — ADC cable GND.
  - members: ADC_5VIN.p2/2, 5VDC@250.19,95.25.p1/2
  - source: the cable sheet
- **5VDC** — Driver board 5 V cable.
  - members: DRIVERS_5VIN.p2/2, 5VDC@250.19,107.95.p2/2
  - source: the cable sheet
- **GND** — Driver board cable GND.
  - members: DRIVERS_5VIN.p1/2, 5VDC@250.19,107.95.p1/2
  - source: the cable sheet
- **D1** — Digit rail D1 (main sheet: P-Mosfet drain, J5.Pin_8).
  - members: DRIVER_IN.p4/4, DIGIT_OUT.p1/4
  - source: the cable sheet; label names across all drawings
- **D2** — Digit rail D2 (PFet1 drain, J7.Pin_8).
  - members: DRIVER_IN.p3/4, DIGIT_OUT.p2/4
  - source: the cable sheet; label names across all drawings
- **D3** — Digit rail D3 (PFet2 drain, J6.Pin_8).
  - members: DRIVER_IN.p2/4, DIGIT_OUT.p3/4
  - source: the cable sheet; label names across all drawings
- **D4** — Digit rail D4 (PFet3 drain, J2.Pin_8).
  - members: DRIVER_IN.p1/4, DIGIT_OUT.p4/4
  - source: the cable sheet; label names across all drawings
- **CABLE S/FTP x2 (digit-rail cable)** — DIGIT_OUT and DIGIT_OUT1 are one cable split over two JSTs. The 2-pin pair is the cable shield and foil, ending at the perfboard's common GND.
  - members: DIGIT_OUT1.p1/2, DIGIT_OUT1.p2/2
  - source: the cable sheet; the author
- **SEG_A_BUS** — Segment bus A to the display.
  - members: DISPLAY_OUT.p1/7, LEDITRON_IN.p1/7
  - source: the cable sheet
- **SEG_B_BUS** — Segment bus B.
  - members: DISPLAY_OUT.p2/7, LEDITRON_IN.p2/7
  - source: the cable sheet
- **SEG_C_BUS** — Segment bus C.
  - members: DISPLAY_OUT.p3/7, LEDITRON_IN.p3/7
  - source: the cable sheet
- **SEG_D_BUS** — Segment bus D.
  - members: DISPLAY_OUT.p4/7, LEDITRON_IN.p4/7
  - source: the cable sheet
- **SEG_E_BUS** — Segment bus E.
  - members: DISPLAY_OUT.p5/7, LEDITRON_IN.p5/7
  - source: the cable sheet
- **SEG_F_BUS** — Segment bus F.
  - members: DISPLAY_OUT.p6/7, LEDITRON_IN.p6/7
  - source: the cable sheet
- **SEG_G_BUS** — Segment bus G. The seven buses are separate conductors (SC29 done: the main sheet's leftover merge is gone).
  - members: DISPLAY_OUT.p7/7, LEDITRON_IN.p7/7
  - source: the cable sheet; SC29
- **(no-connect)** — Remaining connector positions drawn no-connect.
  - members: 5VDC_LIMIT.p1/2, A32_VDC_LISTENER.p2/2, ADC_I2S.p3/6, ADC_I2S.p5/6, RTC_I2C.p1/4, RTC_I2C.p2/4
  - source: the cable sheet

## 13. Tube radio — FM RF amplifier and mixer (V1, 12DT8)

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in FM RF / MIXER on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| V1@33.02,31.75 | 12DT8 twin triode, unit 1 of 3: the FM RF amplifier triode (plate #1, grid #2, cathode #3). Source 'Factory TM-838N JPG; SAMS pin/reference crosswalk', Source_ref V1, Section '12DT8 FM RF amp + mixer'. No Spec fields. | 12DT8 | fitted | — | the tube radio sheet; the author (pins verified on the chassis); 'High-Mu Twin Triode' per nj7p.org |
| V1@100.33,31.75 | 12DT8, unit 2 of 3: the FM mixer triode (plate #6, grid #7, cathode #8). Same Source, Source_ref and Section as unit 1. | 12DT8 | fitted | — | the tube radio sheet; the author |
| V1@27.94,185.42 | 12DT8, unit 3 of 3: heater pins 5 (H_V2_IN) and 4 (H12), shield pin 9 (CHASSIS). Drawn in SERIES HEATER STRING - ALL SEVEN TUBES (T8). | 12DT8 | fitted | — | the tube radio sheet; the author |
| J53 | Coaxial connector, the FM antenna input (drawn description 'Coaxial Cable'; library symbol Connector:Conn_Coaxial): centre In #1 on FM_ANT, shield Ext #2 on COAX_SHIELD_PE. It has no Spec, Source, Source_ref or Section. Its Reference and Value are shown on the drawing since 2026-09-23 (SCE4, done). The antenna itself, as built, is §29. | RG179 Coax | fitted | — | the tube radio sheet; antenna note the tube radio sheet; SCE4; the author |
| C3 | Series coupling capacitor from FM_ANT (#2) to RF_IN (#1); Spec Function 'FM antenna input coupling' | 100pF | fitted | **X1Y1** rated; no voltage given | the tube radio sheet; the author |
| L2 | Inductor from RF_IN (#1) to CHASSIS (#2); Spec Function 'FM antenna choke' | 1uH (23 turns) | fitted | — | the tube radio sheet; the author |
| R2 | Resistor from RF_K (#1) to RF_IN (#2), side by side with C4; Spec Function 'FM RF cathode bias' | 70Ω | fitted | — | the tube radio sheet; the author |
| C4 | Capacitor from RF_IN (#1) to RF_K (#2); Spec Function 'FM RF cathode bypass' | 0.002uF | fitted | — | the tube radio sheet; the author |
| C51 | Capacitor from COAX_SHIELD_PE (#2) to CHASSIS (#1), the shield's only path to CHASSIS. No Spec, Source, Source_ref or Section. | 0.001uF | fitted | X1Y2 rated; no voltage given | the tube radio sheet; the author |
| L3 | RF plate choke from RF_PLATE (#1) to FM_B (#2); Spec Function 'FM RF plate choke' | 3uH | fitted | — | the tube radio sheet; the author |
| C5 | Bypass capacitor from FM_B (#2) to CHASSIS (#1); Spec Function 'FM RF supply bypass' | 0.002uF | fitted | — | the tube radio sheet; the author |
| C6 | Coupling capacitor from RF_PLATE (#2) to FM_MIX_GRID (#1); Spec Function 'FM RF-to-mixer coupling' | 100pF | fitted | — | the tube radio sheet; the author |
| L4 | Tuned RF tank coil from FM_MIX_GRID (#1) to CHASSIS (#2); Spec Function 'FM RF tank'. No inductance in any source. | FM RF Tank | fitted | — | the tube radio sheet; the author |
| C7 | Fixed tank capacitor from FM_MIX_GRID (#2) to CHASSIS (#1); Spec Function 'FM RF fixed tank capacitance' | 5pF | fitted | — | the tube radio sheet; the author |
| C8 | Trimmer capacitor from FM_MIX_GRID (#2) to CHASSIS (#1); Spec Function 'FM RF trimmer'; no numeric value in any source. #1/#2 are symbol pins. | RF Trim | fitted | — | the tube radio sheet; the author |
| VC1 | FM RF section of the ganged tuning capacitor (one assembly with VC2, VC3 and VC4; SAMS assembly M2), from FM_MIX_GRID (#2) to CHASSIS (#1); Spec Function 'FM RF tuning section' | TUNING RF | fitted | — | the tube radio sheet; note the tube radio sheet; the author |
| R3 | Resistor from FM_MIX_K (#1) to CHASSIS (#2), side by side with C9; Spec Function 'FM mixer cathode bias' | 1kΩ | fitted | — | the tube radio sheet; the author |
| C9 | Capacitor from FM_MIX_K (#2) to CHASSIS (#1); Spec Function 'FM mixer cathode bypass' | 0.01uF | fitted | — | the tube radio sheet; the author |

### Connections

#### FM RF / MIXER

- **FM_ANT** — The antenna coax centre pin goes to C3 only. No other net on the sheet is named FM_ANT.
  - members: J53.In (#1), C3.#2
  - source: the tube radio sheet; label names across all drawings
- **RF_IN** — The signal passes through C3 to the RF_IN node. From there L2 goes to CHASSIS, and R2 and C4 go side by side to the RF cathode.
  - members: C3.#1, L2.#1, C4.#1, R2.#2
  - source: the tube radio sheet
- **RF_K** — V1 pin 3, the RF amplifier cathode, is on R2 and C4, which return to RF_IN. The pin is verified on the chassis.
  - members: V1@33.02,31.75.K (#3), C4.#2, R2.#1
  - source: the tube radio sheet; the author
- **CHASSIS** — V1 pin 2, the RF amplifier grid, is on CHASSIS. The pin is verified on the chassis.
  - members: V1@33.02,31.75.G (#2)
  - source: the tube radio sheet; the author
- **RF_PLATE** — V1 pin 1, the RF amplifier plate, is fed through choke L3 and coupled to the mixer grid by C6. The pin is verified on the chassis.
  - members: V1@33.02,31.75.P (#1), L3.#1, C6.#2
  - source: the tube radio sheet; the author
- **FM_B (by label name)** — The FM B+ rail feeds L3 and is bypassed to CHASSIS by C5. The same label reaches R18, R4, R8 and the FM and AFC contacts of mode-switch unit S1D, whose common (#14) is on B115_A. S1D's contact identifiers are logical, not physical lugs.
  - members: L3.#2, C5.#2, R18.#1 (10.7 MHz FM IF PATH), R4.#1 (10.7 MHz FM IF PATH), R8.#2 (AFC / LOCAL OSCILLATOR), S1D.FM (#7) (MODE SUPPLY AND INDICATOR LAMPS), S1D.AFC (#8) (MODE SUPPLY AND INDICATOR LAMPS)
  - source: the tube radio sheet; the author
- **CHASSIS** — C5's other end is on CHASSIS.
  - members: C5.#1
  - source: the tube radio sheet
- **FM_MIX_GRID (by label name)** — V1 pin 7, the mixer grid, sits on the tuned RF tank (L4, C7, C8, VC1). It takes the RF amplifier output through C6. By label it also meets C16 (2 pF), whose other pin is on FM_OSC_TANK. The pin is verified on the chassis.
  - members: C6.#1, L4.#1, C7.#2, C8.#2, VC1.#2, V1@100.33,31.75.G (#7), C16.#2 (AFC / LOCAL OSCILLATOR)
  - source: the tube radio sheet; the author
- **CHASSIS** — The cold ends of the tank parts L4, C7, C8 and VC1 are on CHASSIS.
  - members: C7.#1, L4.#2, C8.#1, VC1.#1
  - source: the tube radio sheet
- **FM_MIX_K** — V1 pin 8, the mixer cathode, goes to CHASSIS through R3 and C9 side by side. The pin is verified on the chassis.
  - members: V1@100.33,31.75.K (#8), R3.#1, C9.#2
  - source: the tube radio sheet; the author
- **CHASSIS** — The CHASSIS ends of R3 and C9.
  - members: R3.#2, C9.#1
  - source: the tube radio sheet
- **FM_MIX_PLATE (by label name)** — V1 pin 6, the mixer plate, reaches primary terminal P of the first FM IF transformer L10, by label only. L10's other primary terminal (B) is on FM_IF1_B with C10 #1 and R4 #2. L10's terminal letters are logical, not physical lugs. The V1 pin is verified on the chassis.
  - members: V1@100.33,31.75.P (#6), L10.PRI (#P) (10.7 MHz FM IF PATH)
  - source: the tube radio sheet; the author
- **COAX_SHIELD_PE** — The antenna coax shield goes to C51. COAX_SHIELD_PE is on Earth, which this sheet does not draw. It reaches CHASSIS only through C51.
  - members: J53.Ext (#2), C51.#2
  - source: the tube radio sheet; label names across all drawings; the author
- **CHASSIS** — The CHASSIS ends of L2 and C51.
  - members: L2.#2, C51.#1
  - source: the tube radio sheet
- **CHASSIS (global label; its net includes AC2 on J4.#1)** — CHASSIS is one conductor across the whole tube radio, joined by its global label, and is the same net as AC2 on J4 pin 1. CHASSIS is on neutral: J4 takes AC_N through the external EMI filter, with no isolation transformer. CHASSIS is never bonded to Earth or the Faraday cages. V1's shield pin 9 is on CHASSIS, verified on the chassis. V1's heater pin 4 is on H12, not CHASSIS: on the heater string the two 12DT8s are interchanged, and V2's heater pin 4 is the string's CHASSIS end.
  - members: C5.#1, C7.#1, L4.#2, C8.#1, VC1.#1, C9.#1, R3.#2, L2.#2, C51.#1, V1@33.02,31.75.G (this group), V1@27.94,185.42.SH (#9), J4.#1 (its net also named AC2), and every other CHASSIS pin of the tube radio sheet
  - source: the tube radio sheet; the author
- **H_V1_IN (by label name)** — V2's heater pin 5 is on H_V1_IN, with heater choke L15 (other end on H12) and 0.0022 µF bypass C49 (other end on CHASSIS). V2 is the last tube in the string, before chassis. Verified on the chassis. The rest of the string belongs to T8.
  - members: V2@52.07,185.42.H (#5), L15.#2, C49.#1
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (15.75,15.24): "FM RF / MIXER" — The section title. (source: the tube radio sheet)
- (46.48,44.45): "Antenna was "upgraded" to a differential antenna :  / center to RF, shield to rejection. / Was previously a simple wire." — The FM antenna feed is now the coax connector J53 in place of the original simple wire. The centre goes to the RF input (J53 In → FM_ANT → C3 → RF_IN). The shield goes to COAX_SHIELD_PE, which is on Earth and reaches CHASSIS only through C51. The note describes a modification the drawing shows; it does not say the drawing departs from the hardware. No source or ruling says what 'rejection' refers to. (source: the tube radio sheet; the author)
- (52.32,49.28): "VC1 and VC2 are mechanically ganged with VC3/VC4. / Factory AFC series C11 and oscillator feedback differ from SAMS. / Unspecified coil values and factory tube section-to-pin mapping need chassis verification." — Line 1: VC1-VC4 are one ganged tuning assembly. Line 2 concerns the AFC / LOCAL OSCILLATOR section: the fitted C11 is a shunt from AFC_C to chassis, and the FM oscillator as drawn is the actual hardware, its cathode on the R33 / C50 network. Line 3: the tube section-to-pin mapping (V1 RF 1/2/3, mixer 6/7/8, heaters 4/5, shield 9) has now been checked on the chassis. The coil values are still unspecified; L4 has no inductance in any source. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **R2** fitted `70Ω` — The fitted 70 Ω matches the original factory value. SAMS lists 68 Ω, with 70 Ω as an alternate. Original: Spec Factory resistance (Ω): 70 | Spec SAMS resistance (Ω): 68 | Spec Differences: Factory 70 Ω; SAMS 68 Ω (70 Ω alternate). (source: the tube radio sheet; the author)
- **C5** fitted `0.002uF` — The fitted 0.002 µF matches the factory drawing and the SAMS parts table. The SAMS schematic shows 0.01 µF, with 2000 pF as an alternate. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 10000 | Spec SAMS table (pF): 2000 | Spec Factory / SAMS differences: Factory 2000 pF; SAMS schematic 10000 pF (2000 alternate); table 2000 pF. (source: the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **C3** — row: Capacitors: C3; Function: FM antenna input coupling; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: External antenna coupling on line-operated chassis: original DC-rated substitutes do not establish a modern safety class or insulation design.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L2** — row: Coils and other parts: L2; Category: Magnetic component; Function: FM antenna choke; Factory specification: 1 µH; SAMS additional specification: 23 turns; Catalog check ID: No verified substitute; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **R2** — row: Resistors: R2; Function: FM RF cathode bias; Factory resistance (Ω): 70; SAMS resistance (Ω): 68; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Factory 70 Ω; SAMS 68 Ω (70 Ω alternate).; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C4** — row: Capacitors: C4; Function: FM RF cathode bypass; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L3** — row: Coils and other parts: L3; Category: Magnetic component; Function: FM RF plate choke; Factory specification: 3 µH; SAMS additional specification: 3 µH; DCR about 0.9 Ω; Catalog check ID: K-RFC; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C5** — row: Capacitors: C5; Function: FM RF supply bypass; Factory value (pF): 2000; SAMS schematic (pF): 10000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 2000 pF; SAMS schematic 10000 pF (2000 alternate); table 2000 pF.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C6** — row: Capacitors: C6; Function: FM RF-to-mixer coupling; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L4** — row: Coils and other parts: L4; Category: Magnetic component; Function: FM RF tank; Factory specification: Tuned coil; no inductance stated; SAMS additional specification: Value/Q not stated; Catalog check ID: No verified substitute; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C7** — row: Capacitors: C7; Function: FM RF fixed tank capacitance; Factory value (pF): 5; SAMS schematic (pF): 5; SAMS table (pF): 5; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C8** — row: Capacitors: C8; Function: FM RF trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Range, Q, dielectric and mechanical dimensions unspecified. Associated with tuning assembly M2.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **VC1** — row: Capacitors: VC1; Function: FM RF tuning section; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Part of M2; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Mechanically ganged variable capacitor section; Check ID: No catalog substitute; Factory / SAMS differences: VC labels newly assigned; sections belong to tuning assembly M2, not four independent controls.; Selection / missing specifications: Match min/max capacitance, tracking law, shaft/gang geometry and trimmer arrangement; no numerical range supplied.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R3** — row: Resistors: R3; Function: FM mixer cathode bias; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C9** — row: Capacitors: C9; Function: FM mixer cathode bypass; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table

## 14. Tube radio — FM local oscillator and AFC (V2, 12DT8)

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in AFC / LOCAL OSCILLATOR on the tube radio sheet. The local oscillator runs below the station: station = oscillator + IF (low-side injection). The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| V2@140.97,83.82 | 12DT8 VHF double triode (radiomuseum.org), unit 1: FM local-oscillator triode, P #1, G #2, K #3; Section '12DT8 FM oscillator + AFC'; Source_ref V2 | 12DT8 | fitted | — | the tube radio sheet; the author (pins checked on chassis); the author |
| V2@63.50,83.82 | 12DT8 unit 2: AFC (control) triode, P #6, G #7, K #8 | 12DT8 | fitted | — | the tube radio sheet; the author |
| V2@52.07,185.42 | 12DT8 unit 3: heater H #5 (H_V1_IN), H #4 (CHASSIS), shield SH #9 (CHASSIS); drawn on the series heater string (T8) | 12DT8 | fitted | — | the tube radio sheet; the author |
| R5 | Fixed resistor, AFC control feed, AFC_CTL to AFC_C | 250kΩ | fitted | — | the tube radio sheet; the author |
| C11 | Unpolarized capacitor, AFC shunt from AFC_C to CHASSIS (the hardware, and always was) | 0.002uF | fitted | — | the tube radio sheet; the author |
| R6 | Fixed resistor, AFC grid series resistor, AFC_C to AFC_GRID | 100Ω | fitted | — | the tube radio sheet; the author |
| C12 | Unpolarized capacitor, AFC control bypass, AFC_CTL to CHASSIS | 0.01uF | fitted | — | the tube radio sheet; the author |
| R33 | Fixed resistor, FM oscillator cathode bias, from the oscillator cathode node (unnamed; V2 pin 3, C50 #1) to CHASSIS; its hidden Spec fields are R9's row, not its own | 600Ω | fitted | — | the tube radio sheet; the author |
| C50 | Unpolarized capacitor, FM oscillator cathode bypass, from the oscillator cathode node (unnamed; V2 pin 3, R33 #2) to CHASSIS; its hidden Spec fields are C19's row, not its own | 0.01uF | fitted | — | the tube radio sheet; the author |
| R7 | Fixed resistor, AFC cathode bias, AFC_K to CHASSIS | 600Ω | fitted | — | the tube radio sheet; the author |
| C13 | Unpolarized capacitor, AFC cathode bypass, AFC_K to CHASSIS (parallel with R7) | 0.01uF | fitted | — | the tube radio sheet; the author |
| L1 | Inductor, RF choke (Spec Function 'AFC RF choke'), AFC_PLATE to AFC_SUPPLY | 3uH | fitted | — | the tube radio sheet; the author |
| R8 | Fixed resistor, AFC plate feed, FM_B to AFC_SUPPLY | 1kΩ | fitted | — | the tube radio sheet; the author |
| C14 | Unpolarized capacitor, AFC plate-supply bypass, AFC_SUPPLY to CHASSIS | 0.002uF | fitted | — | the tube radio sheet; the author |
| C15 | Unpolarized capacitor, AFC-to-oscillator coupling, AFC_PLATE to FM_OSC_TANK | 10pF | fitted | — | the tube radio sheet; the author |
| C16 | Unpolarized capacitor, RF/oscillator coupling, FM_OSC_TANK to FM_MIX_GRID | 2pF | fitted | — | the tube radio sheet; the author |
| C17 | Unpolarized capacitor, FM oscillator fixed tank capacitance, FM_OSC_TANK to CHASSIS | 5pF | fitted | — | the tube radio sheet; the author |
| C18 | Adjustable trimmer capacitor, FM oscillator trimmer, FM_OSC_TANK to CHASSIS; no capacitance range in any source | OSC Trim | fitted | — | the tube radio sheet; the author |
| VC2 | Variable capacitor, FM oscillator tuning section, FM_OSC_TANK to CHASSIS; one section of the ganged tuning assembly with VC1, VC3, VC4 (SAMS M2); no capacitance range in any source | TUNING OSC | fitted | — | the tube radio sheet; the author |
| L5 | FM oscillator coil, two coupled windings: logical terminals 1-2 tank winding (FM_OSC_TANK to CHASSIS), 3-4 feedback winding (FM_OSC_FB to CHASSIS); no inductance in any source; terminal numbers are logical, not physical lugs | FM OSC Coupled Coils | fitted | — | the tube radio sheet; the author |
| C19 | Unpolarized capacitor, FM oscillator grid feedback, FM_OSC_FB to FM_OSC_GRID | 50pF | fitted | — | the tube radio sheet; the author |
| R9 | Fixed resistor, FM oscillator grid leak, across C19 (FM_OSC_FB to FM_OSC_GRID) | 20kΩ | fitted | — | the tube radio sheet; the author |
| R10 | Fixed resistor, FM oscillator plate feed, AFC_SUPPLY to the oscillator plate net | 30kΩ | fitted | — | the tube radio sheet; the author |
| C20 | Unpolarized capacitor, FM oscillator plate coupling, oscillator plate net to FM_OSC_TANK | 20pF | fitted | — | the tube radio sheet; the author |

### Connections

#### AFC / LOCAL OSCILLATOR

- **AFC_CTL** — The AFC control line comes from FM_DET_RAW through R23 3MΩ and enters R5. C12 bypasses it to chassis. Mode-switch unit S1A ('AFC disable') shorts it to chassis in FM and is open in FM-AFC. S1A's terminal numbers are logical.
  - members: C12.#2, R5.#1, R23.#2 (3MΩ, Spec Function 'AFC feed'; R23.#1 on FM_DET_RAW), S1A.FM #3 (logical; S1A.COM #13 on CHASSIS)
  - source: the tube radio sheet; the author
- **AFC_C** — R5 250kΩ, R6 100Ω and the shunt capacitor C11 0.002uF meet here; C11's other end is on CHASSIS.
  - members: R5.#2, R6.#1, C11.#2 (C11.#1 on CHASSIS)
  - source: the tube radio sheet; the author
- **AFC_GRID** — The AFC triode grid (pin 7) connects only to R6, so its path from AFC_CTL runs R5, AFC_C, R6, with C11 shunting AFC_C to chassis. (The sheet once drew C11 in series in that path, which would have left the grid with no DC path; C11 is a shunt, and always was.)
  - members: V2@63.50,83.82 G #7, R6.#2
  - source: the tube radio sheet; the author
- **AFC_K** — The AFC cathode (pin 8) goes to R7 600Ω and C13 0.01uF, both of whose other ends are on CHASSIS.
  - members: V2@63.50,83.82 K #8, R7.#1, C13.#2
  - source: the tube radio sheet; the author
- **AFC_PLATE** — The AFC plate (pin 6) is fed through choke L1 3uH from AFC_SUPPLY and couples to the oscillator tank through C15 10pF.
  - members: V2@63.50,83.82 P #6, L1.#1, C15.#2
  - source: the tube radio sheet; the author
- **AFC_SUPPLY** — The local supply after R8 1kΩ feeds L1 (AFC plate) and R10 (oscillator plate), with C14 0.002uF to chassis. The name appears on no other net on any drawing.
  - members: R10.#1, R8.#1, L1.#2, C14.#2
  - source: the tube radio sheet; label names across all drawings
- **FM_B (by label name)** — FM B+ reaches this group only through R8. Mode-switch unit S1D ('B+ mode selection') joins it to B115_A through its FM (#7) and AFC (#8) contacts; S1D.COM #14 is on B115_A. The terminal numbers are logical.
  - members: R8.#2, L3.#2 (3uH, FM RF plate choke), C5.#2 (0.002uF, FM RF supply bypass), R18.#1 (1kΩ, ratio-detector supply decoupling), R4.#1 (1kΩ, first FM IF supply decoupling), S1D.AFC #8 (logical), S1D.FM #7 (logical)
  - source: the tube radio sheet; the author
- **FM_OSC_PLATE — FM oscillator plate (labelled since 2026-09-23, SCE1 done)** — The oscillator plate (pin 1) is fed from AFC_SUPPLY through R10 30kΩ and coupled to the tank through C20 20pF. The net is drawn with seven wires and carries the label FM_OSC_PLATE since 2026-09-23.
  - members: V2@140.97,83.82 P #1, R10.#2, C20.#2
  - source: the tube radio sheet; the author; SCE1
- **FM_OSC_TANK** — The oscillator tank's hot side holds L5's tank winding, C17 5pF, trimmer C18 and tuning section VC2, whose other ends are on chassis. C15 brings in the AFC plate, C20 the oscillator plate, and C16 2pF couples out to the FM mixer grid.
  - members: L5.1 (logical), C17.#2, C18.#2, VC2.#2, C15.#1, C16.#1, C20.#1
  - source: the tube radio sheet; the author
- **FM_MIX_GRID** — C16 injects the oscillator onto the FM mixer grid, V1 pin 7.
  - members: C16.#2, V1@100.33,31.75 G #7 (mixer triode), C7.#2, L4.#1, C6.#1, C8.#2, VC1.#2
  - source: the tube radio sheet; the author
- **FM_OSC_FB** — L5's feedback winding goes to C19 50pF and R9 20kΩ, which are in parallel.
  - members: L5.3 (logical), C19.#2, R9.#1
  - source: the tube radio sheet; the author
- **FM_OSC_GRID** — The oscillator grid (pin 2) is fed from the feedback winding through C19, with R9 across C19.
  - members: V2@140.97,83.82 G #2, C19.#1, R9.#2
  - source: the tube radio sheet; the author
- **the oscillator cathode node (unnamed)** — V2's oscillator cathode (pin 3) meets R33 600Ω and C50 0.01uF; the other end of each is on CHASSIS.
  - members: V2@140.97,83.82 K #3, R33.#2, C50.#1
  - source: the tube radio sheet; the author
- **CHASSIS (one net by name with every CHASSIS net, including AC2 on J4.#1)** — This is the group's circuit common: the AFC bypasses and cathode network, the tank capacitors, one end of each L5 winding, and the oscillator cathode network R33 / C50. CHASSIS sits on mains neutral (J4 from AC_N through the external EMI filter, no isolation transformer) and is never bonded to Earth or the Faraday cages.
  - members: C12.#1, C13.#1, R7.#2, C14.#1, C17.#1, C18.#1, VC2.#1, L5.2 (logical), L5.4 (logical), R33.#1, C50.#2
  - source: the tube radio sheet; the author
- **H_V2_IN** — Heater current reaches V1 pin 5 through choke L16 from H24, with C48 bypassing to chassis. The string order, checked on chassis, is V7, V5, V6, V4, V3, L16, V1, L15, V2.
  - members: V1@27.94,185.42 H #5, C48.#1 (0.002uF; C48.#2 on CHASSIS), L16.#2 (RF Heater Choke; L16.#1 on H24)
  - source: the tube radio sheet; the author (C48 is 0.002 µF, the original three-lead part)
- **H12** — From V1 pin 4 the heater string continues through choke L15, with C52 bypassing H12 to chassis, to V2's heater.
  - members: V1@27.94,185.42 H #4, L15.#1 (RF Heater Choke; L15.#2 on H_V1_IN)
  - source: the tube radio sheet; label names across all drawings; the author
- **CHASSIS** — V2 pin 9, named SH on the sheet, goes to chassis.
  - members: V2@52.07,185.42 SH #9
  - source: the tube radio sheet; the author

### Near this section (not a connection)

- Ambersong's RDA5807M antenna wire lies over this section: the RDA's 30CM_ANT is a 30 cm length of wire inside the tube radio's Faraday cage, bent in a U over the FM oscillator section.

### Notes written on the tube radio sheet (verbatim)

- (52.83,56.39), section title: "AFC / LOCAL OSCILLATOR" — Title of the section holding both V2 triodes and their parts. The section is two rectangles: (48.26,53.34)-(150.5,98.42) and (13.34,69.85)-(48.26,98.42). (source: the tube radio sheet)
- (52.32,49.28), above the AFC / LOCAL OSCILLATOR rectangle (top edge y=53.34): "VC1 and VC2 are mechanically ganged with VC3/VC4.
Factory AFC series C11 and oscillator feedback differ from SAMS.
Unspecified coil values and factory tube section-to-pin mapping need chassis verification." — VC2 turns with VC1, VC3 and VC4 as one tuning assembly. The fitted C11 is a shunt from AFC_C to chassis, so on this point the note's "factory AFC series C11" describes the factory drawing, not the radio; the oscillator feedback as drawn is certain. The tube section-to-pin mapping has been checked on the chassis, so that part of the note is answered. The coil values remain unspecified: no source or ruling gives L5's inductance. (source: the tube radio sheet; the author)
- (120.14,173.99), inside SW / AM LOCAL OSCILLATOR (T6), outside this section: "All S1 units rotate together: SW / AM / FM / FM-AFC.
VC1, VC2, VC3 and VC4 are one ganged tuning assembly.
Logical switch and coil terminals require a physical lug crosscheck." — S1A–S1G are units of one four-position mode switch, and VC1–VC4 are one tuning assembly. The coil and switch terminal identifiers (L5; S1A, S1D here) are logical, not physical lugs. No lug mapping is recorded. (source: the tube radio sheet; the author; §22)
- (184.66,116.33), ratio detector area, outside this section (third line bears on this group): "CHASSIS is at the midpoint of R20 / R22.
FM_DET_RAW is the C34 / C35 midpoint and L14 tertiary.
S1A grounds AFC_CTL only in FM without AFC." — In the FM position S1A (Section 'AFC disable') shorts AFC_CTL to CHASSIS. In FM-AFC that contact is open, and AFC_CTL is fed from FM_DET_RAW through R23 3MΩ. (source: the tube radio sheet)

### Fitted value against the original specification

- **C11** fitted `0.002uF (shunt, actual)` — The fitted 0.002uF capacitor, a shunt from AFC_C to chassis, is the factory part. SAMS shows 1000 pF as a shunt, and the Spec's substitute list is for that SAMS part. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 1000 | Spec SAMS table (pF): 1000 | Spec Factory / SAMS differences: Factory 2000.0 pF; SAMS schematic 1000.0 pF; table 1000 pF. Factory SERIES AFC coupling; SAMS SHUNT. Substitute list is for the 1000 pF SAMS part. (source: the tube radio sheet; the author)
- **C12** fitted `0.01uF` — The fitted value matches the factory. SAMS gives one tenth of it. Original: Spec Factory value (pF): 10000 | Spec SAMS schematic (pF): 1000 | Spec SAMS table (pF): 1000 | Spec Factory / SAMS differences: Factory 10000.0 pF; SAMS schematic 1000.0 pF; table 1000 pF. (source: the tube radio sheet; the author)
- **C13** fitted `0.01uF` — The fitted value matches the factory and the SAMS table. The SAMS schematic prints 2000 pF with 10000 as an alternate. Original: Spec Factory value (pF): 10000 | Spec SAMS schematic (pF): 2000 | Spec SAMS table (pF): 10000 | Spec Factory / SAMS differences: Factory 10000 pF; SAMS schematic 2000 pF (10000 alternate); table 10000 pF. (source: the tube radio sheet; the author)
- **C20** fitted `20pF (oscillator plate to tank)` — The fitted part is the factory's 20 pF plate-to-tank coupling capacitor. SAMS's C20 is a different part, a 2000 pF bypass, and its substitutes do not apply. The original dielectric, tolerance and voltage are unknown. Original: Spec Function: FM oscillator plate coupling (factory) / bypass (SAMS) | Spec Factory value (pF): 20 | Spec SAMS schematic (pF): 2000 | Spec SAMS table (pF): 2000 | Spec Factory / SAMS differences: Factory 20.0 pF; SAMS schematic 2000.0 pF; table 2000 pF. Different circuit function. The SAMS 2000 pF bypass substitutes do not validate the factory 20 pF coupling part. | Spec Selection / missing specifications: Factory: 20 pF RF coupling part. Its dielectric, tolerance and voltage rating remain unknown; investigate original markings. (source: the tube radio sheet; the author)
- **R10** fitted `30kΩ (AFC_SUPPLY to oscillator plate)` — The fitted 30 kΩ matches the factory; SAMS gives 10 kΩ. The Spec attaches 'with different supply connection' to the factory 30 kΩ. Original: Spec Factory resistance (Ω): 30000 | Spec SAMS resistance (Ω): 10000 | Spec Differences: Factory 30 kΩ with different supply connection; SAMS 10 kΩ. (source: the tube radio sheet; the author)
- **L5** fitted `FM OSC Coupled Coils (two coupled windings, actual)` — The fitted coil has the factory's two coupled windings; SAMS shows a single tapped winding. Neither source gives an inductance. Original: Spec Factory specification: Coupled two-winding oscillator; no inductance stated | Spec SAMS additional specification: Tapped single oscillator winding; value not stated | Spec Differences / context: Factory/SAMS winding topology differs. Do not substitute a tapped oscillator for coupled windings by name alone. (source: the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **R5** — row: Resistors: R5; Function: AFC control feed; Factory resistance (Ω): 250000; SAMS resistance (Ω): 250000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C11** — row: Capacitors: C11; Function: AFC control coupling (factory) / shunt (SAMS); Factory value (pF): 2000; SAMS schematic (pF): 1000; SAMS table (pF): 1000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 2000.0 pF; SAMS schematic 1000.0 pF; table 1000 pF. Factory SERIES AFC coupling; SAMS SHUNT. Substitute list is for the 1000 pF SAMS part.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R6** — row: Resistors: R6; Function: AFC grid series resistor; Factory resistance (Ω): 100; SAMS resistance (Ω): 100; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C12** — row: Capacitors: C12; Function: AFC control bypass; Factory value (pF): 10000; SAMS schematic (pF): 1000; SAMS table (pF): 1000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 10000.0 pF; SAMS schematic 1000.0 pF; table 1000 pF.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R7** — row: Resistors: R7; Function: AFC cathode bias; Factory resistance (Ω): 600; SAMS resistance (Ω): 600; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C13** — row: Capacitors: C13; Function: AFC cathode bypass; Factory value (pF): 10000; SAMS schematic (pF): 2000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 10000 pF; SAMS schematic 2000 pF (10000 alternate); table 10000 pF.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L1** — row: Coils and other parts: L1; Category: Magnetic component; Function: AFC RF choke; Factory specification: 3 µH; SAMS additional specification: 3 µH; DCR about 0.9 Ω; Catalog check ID: K-RFC; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **R8** — row: Resistors: R8; Function: AFC plate feed; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C14** — row: Capacitors: C14; Function: AFC plate-supply bypass; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C15** — row: Capacitors: C15; Function: AFC-to-oscillator coupling; Factory value (pF): 10; SAMS schematic (pF): 10; SAMS table (pF): 10; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C16** — row: Capacitors: C16; Function: RF/oscillator coupling; Factory value (pF): 2; SAMS schematic (pF): 2; SAMS table (pF): 2; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: NP0 ceramic; Check ID: K-NP0; Factory / SAMS differences: Both drawings 2 pF; several listed substitutes, including Sprague, are 2.2 pF.; Selection / missing specifications: Prefer verified 2 pF low-loss NP0 equivalent for the factory value. 2.2 pF is historical substitution evidence, not an exact value match.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C17** — row: Capacitors: C17; Function: FM oscillator fixed tank capacitance; Factory value (pF): 5; SAMS schematic (pF): 5; SAMS table (pF): 5; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C18** — row: Capacitors: C18; Function: FM oscillator trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Range, Q, dielectric and mechanical dimensions unspecified. Associated with tuning assembly M2.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **VC2** — row: Capacitors: VC2; Function: FM oscillator tuning section; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Part of M2; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Mechanically ganged variable capacitor section; Check ID: No catalog substitute; Factory / SAMS differences: VC labels newly assigned; sections belong to tuning assembly M2, not four independent controls.; Selection / missing specifications: Match min/max capacitance, tracking law, shaft/gang geometry and trimmer arrangement; no numerical range supplied.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L5** — row: Coils and other parts: L5; Category: Magnetic component; Function: FM oscillator; Factory specification: Coupled two-winding oscillator; no inductance stated; SAMS additional specification: Tapped single oscillator winding; value not stated; Catalog check ID: No verified substitute; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: Factory/SAMS winding topology differs. Do not substitute a tapped oscillator for coupled windings by name alone.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C19** — row: Capacitors: C19; Function: FM oscillator grid feedback; Factory value (pF): 50; SAMS schematic (pF): 50; SAMS table (pF): 50; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R9** — row: Resistors: R9; Function: FM oscillator grid leak; Factory resistance (Ω): 20000; SAMS resistance (Ω): 20000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R10** — row: Resistors: R10; Function: FM oscillator plate feed; Factory resistance (Ω): 30000; SAMS resistance (Ω): 10000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Factory 30 kΩ with different supply connection; SAMS 10 kΩ.; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C20** — row: Capacitors: C20; Function: FM oscillator plate coupling (factory) / bypass (SAMS); Factory value (pF): 20; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 20.0 pF; SAMS schematic 2000.0 pF; table 2000 pF. Different circuit function. The SAMS 2000 pF bypass substitutes do not validate the factory 20 pF coupling part.; Selection / missing specifications: Factory: 20 pF RF coupling part. Its dielectric, tolerance and voltage rating remain unknown; investigate original markings.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table

## 15. Tube radio — 10.7 MHz FM IF path (V4 first IF, V5 limiter, 12BA6)

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in 10.7 MHz FM IF PATH on the tube radio sheet. **The fitted IF is 10.6 MHz** since the FM path was professionally tuned; the 10.7 MHz in the section title and in the L10 / L12 Section fields is the design value drawn on the sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| C10 | Capacitor, first FM IF supply bypass (Spec Function), from FM_IF1_B to CHASSIS | 0.01uF | fitted | — | the tube radio sheet; the author |
| C28 | Capacitor, first IF cathode bypass (Spec Function), from IF1_K to CHASSIS, in parallel with R12 | 0.002uF | fitted | — | the tube radio sheet; the author |
| C30 | Capacitor, FM limiter grid-return bypass (Spec Function), from IF2_SEC_RETURN to CHASSIS, in parallel with R14 | 50pF | fitted | — | the tube radio sheet; the author |
| C33 | Capacitor, ratio-detector plate-supply bypass (Spec Function), from FM_DET_B to CHASSIS | 0.01uF | fitted | — | the tube radio sheet; the author |
| L10 | First FM IF transformer (Spec Function), custom symbol Radio:IF, Section 'FM IF A - 10.7MHz'; primary P/B, secondary G/E, can SH. Terminal identifiers are logical, not lugs. Internal tuning capacitors drawn inside the can; 'Values are not supplied by either source.' | FM IF A - 10.7MHz | fitted | — | the tube radio sheet; the author |
| L12 | Second FM IF transformer (Spec Function), Radio:IF (cached as IF_3), Section 'FM IF B - 10.7MHz'; primary P/B, secondary G/E, can SH. Terminal identifiers logical. | FM IF B - 10.7MHz | fitted | — | the tube radio sheet; the author |
| L14 | FM ratio-detector transformer (Spec Function), custom symbol Radio:RATIO, Section 'FM ratio detector'; primary 3/4, secondary 1/2, tertiary 5, can SH. Terminal identifiers logical. | FM ratio detector | fitted | — | the tube radio sheet; the author |
| R12 | Resistor, first IF cathode bias (Spec Function), from IF1_K to CHASSIS | 70Ω | fitted | — | the tube radio sheet; the author |
| R14 | Resistor, FM limiter grid return (Spec Function), from IF2_SEC_RETURN to CHASSIS | 250kΩ | fitted | — | the tube radio sheet; the author |
| R18 | Resistor, ratio-detector supply decoupling (Spec Function), from FM_B to FM_DET_B | 1kΩ | fitted | — | the tube radio sheet; the author |
| R4 | Resistor, first FM IF supply decoupling (Spec Function), from FM_B to FM_IF1_B | 1kΩ | fitted | — | the tube radio sheet; the author |
| V4 | 12BA6 pentode, Section '12BA6 first IF'. Signal unit 1 at (162.56,31.75): P #5, G2 #6, G1 #1, G3 #2, K #7. Heater unit 2 at (73.66,172.72) in the series heater string: H #3, #4. Pins and heater order checked on chassis. | 12BA6 | fitted | — | the tube radio sheet; the author |
| V5 | 12BA6 pentode, Section '12BA6 limiter'. Signal unit 1 at (224.79,31.75): P #5, G2 #6, G1 #1, G3 #2, K #7. Heater unit 2 at (48.26,172.72): H #3, #4. Pins and heater order checked on chassis. | 12BA6 | fitted | — | the tube radio sheet; the author |

### Connections

#### 10.7 MHz FM IF PATH

- **FM_MIX_PLATE** — The FM mixer plate feeds the P end of L10's primary. The two pieces are joined by the label name. L10's terminal letters are logical.
  - members: L10.PRI (#P), V1@100.33,31.75.P (#6, 12DT8 unit 2, FM RF / MIXER)
  - source: the tube radio sheet; the author
- **FM_IF1_B** — The supply end of L10's primary, fed from FM_B through R4 1kΩ and bypassed to CHASSIS by C10 0.01uF. B is a logical identifier.
  - members: L10.PRI (#B), R4.#2, C10.#1
  - source: the tube radio sheet; the author
- **FM_B** — The FM-mode B+ rail. It is on S1D's FM and AFC throws; S1D's common (#14) is on B115_A and its SW (#5) and AM (#6) throws are on AM_B. In this group it feeds R4 and R18. S1D terminal numbers are logical.
  - members: R4.#1, R18.#1, L3.#2 (FM RF / MIXER), C5.#2 (FM RF / MIXER), R8.#2 (AFC / LOCAL OSCILLATOR), S1D.FM (#7), S1D.AFC (#8)
  - source: the tube radio sheet; the author
- **IF1_GRID** — The G end of L10's secondary drives V4's control grid (pin 1). Tube pins were checked on chassis; coil letters are logical.
  - members: V4@162.56,31.75.G1 (#1), L10.SEC (#G)
  - source: the tube radio sheet; the author
- **IF1_SEC_RETURN** — The E end of L10's secondary is in series with L11's secondary. L11's other secondary end (#E) is on AVC, so V4's grid DC path runs through L10's secondary, then L11's secondary, to AVC. Coil letters are logical.
  - members: L10.SEC (#E), L11.SEC (#G) (AM IF A - 455kHz)
  - source: the tube radio sheet; the author
- **IF1_K** — V4's cathode (pin 7) goes to CHASSIS through R12 70Ω in parallel with C28 0.002uF.
  - members: V4@162.56,31.75.K (#7), R12.#1, C28.#2
  - source: the tube radio sheet; the author
- **IF1_PLATE** — V4's plate (pin 5) drives the P end of L12's primary.
  - members: V4@162.56,31.75.P (#5), L12.PRI (#P)
  - source: the tube radio sheet; the author
- **IF_CHAIN_MID** — The B end of L12's primary continues to the B end of L13's primary, putting both primaries in series in V4's plate circuit. The two pieces are joined by label only, with no wires.
  - members: L12.PRI (#B), L13.PRI (#B) (AM IF B - 455kHz)
  - source: the tube radio sheet; the author
- **AM_IF2_B** — V4's screen (pin 6) and the P end of L13's primary share this node. R13 1kΩ feeds it from B115_A, and C29 0.01uF bypasses it to CHASSIS. V4's plate supply is L12's primary, then L13's primary, then this node, then R13, then B115_A. None of it is on FM_B.
  - members: V4@162.56,31.75.G2 (#6), L13.PRI (#P), R13.#2, C29.#2
  - source: the tube radio sheet
- **IF2_GRID** — The G end of L12's secondary drives V5's control grid (pin 1).
  - members: L12.SEC (#G), V5@224.79,31.75.G1 (#1)
  - source: the tube radio sheet; the author
- **IF2_SEC_RETURN** — The E end of L12's secondary returns to CHASSIS through R14 250kΩ in parallel with C30 50pF. V5's grid return therefore runs through L12's secondary and then R14 in parallel with C30.
  - members: L12.SEC (#E), R14.#1, C30.#2
  - source: the tube radio sheet; the author
- **IF2_PLATE** — V5's plate (pin 5) drives terminal 3 of L14's primary.
  - members: V5@224.79,31.75.P (#5), L14.PRI (#3)
  - source: the tube radio sheet; the author
- **FM_DET_B** — V5's screen (pin 6) and terminal 4 of L14's primary are fed from FM_B through R18 1kΩ and bypassed to CHASSIS by C33 0.01uF. All four pieces are joined by the label name.
  - members: V5@224.79,31.75.G2 (#6), L14.PRI (#4), R18.#2, C33.#2
  - source: the tube radio sheet; the author
- **DET_SEC_H** — L14's secondary end 1 goes to R19 1kΩ, the upper ratio-detector series resistor (its Spec Function).
  - members: L14.SEC (#1), R19.#1 (RATIO DETECTOR / AFC)
  - source: the tube radio sheet; the author
- **DET_SEC_L** — L14's secondary end 2 goes to R21 1kΩ, the lower ratio-detector series resistor (its Spec Function).
  - members: L14.SEC (#2), R21.#1 (RATIO DETECTOR / AFC)
  - source: the tube radio sheet; the author
- **FM_DET_RAW** — L14's tertiary output (terminal 5) is the C34/C35 midpoint, as the sheet note at (184.66,116.33) says. C36, R23 and R24 of the ratio detector are drawn on the same net.
  - members: L14.TERT (#5), C34.#1, C35.#2, C36.#2, R23.#1, R24.#1
  - source: the tube radio sheet; the author
- **CHASSIS (also named AC2)** — The radio's circuit common, joined sheet-wide by the CHASSIS label, which shares a net with AC2 on J4.#1. In this group it takes 12 pins: the three can shields, V4's suppressor, V5's suppressor and cathode, and the returns of R12, C28, R14, C30, C10 and C33. CHASSIS is on neutral: J4 takes AC_N through the external EMI filter, with no isolation transformer. CHASSIS is never bonded to Earth or the Faraday cages, and COAX_SHIELD_PE reaches it only through C51.
  - members: C10.#2, C30.#1, C33.#1, L10.CAN (#SH), L12.CAN (#SH), L14.CAN (#SH), R12.#2, C28.#1, R14.#2, V4@162.56,31.75.G3 (#2), V5@224.79,31.75.G3 (#2), V5@224.79,31.75.K (#7)
  - source: the tube radio sheet; the author
- **H36** — V4's heater pin 3 is in series with V3's (12BE6) heater pin 4. C47 0.0022uF (Heater-string RF bypass) goes from this node to CHASSIS. The order was checked on chassis; C47 0.0022uF is fitted.
  - members: V4@73.66,172.72.H (#3), V3@86.36,172.72.H (#4), C47.#1
  - source: the tube radio sheet; the author
- **H48** — V4's heater pin 4 is in series with V6's (12AV6) heater pin 3.
  - members: V4@73.66,172.72.H (#4), V6@60.96,172.72.H (#3)
  - source: the tube radio sheet; the author
- **H60** — V5's heater pin 3 is in series with V6's heater pin 4.
  - members: V5@48.26,172.72.H (#3), V6@60.96,172.72.H (#4)
  - source: the tube radio sheet; the author
- **H72** — V5's heater pin 4 is in series with V7's (50C5) heater pin 3. The string, checked on chassis, runs V7, V5, V6, V4, V3, L16, V1, L15, V2. Voltages in heater net names are SAMS nominal identifiers.
  - members: V5@48.26,172.72.H (#4), V7@35.56,172.72.H (#3)
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (123.19,14.48), 10.7 MHz FM IF PATH: "10.7 MHz FM IF PATH" — Section title for this group. (source: the tube radio sheet)
- (178.05,57.91), below the 455 kHz AM IF AND AM DETECTOR title; applies to the IF can symbols: "IF transformers include their internal tuning capacitors.
Values are not supplied by either source.
See the custom symbol (capacitors added inside the can)." — The capacitors drawn inside the IF can symbols (L10, L12 and, in the same style, L14) are the cans' built-in tuning capacitors, not separate parts. Neither the factory drawing nor SAMS gives their values, and they are not otherwise recorded. (source: the tube radio sheet)
- (184.66,116.33), RATIO DETECTOR / AFC; names L14's tertiary: "CHASSIS is at the midpoint of R20 / R22.
FM_DET_RAW is the C34 / C35 midpoint and L14 tertiary.
S1A grounds AFC_CTL only in FM without AFC." — L14's tertiary output (terminal 5, FM_DET_RAW) is the same node as the C34/C35 midpoint of the ratio detector. (source: the tube radio sheet)
- (120.14,173.99), inside SW / AM LOCAL OSCILLATOR (T6); bears on L10, L12, L14 terminal identifiers: "All S1 units rotate together: SW / AM / FM / FM-AFC.
VC1, VC2, VC3 and VC4 are one ganged tuning assembly.
Logical switch and coil terminals require a physical lug crosscheck." — Answered: coil and switch terminal identifiers (L5, L6, L8-L14, S1A-S1G) are logical, not physical lugs. For this group that covers L10/L12's P, B, G, E, SH and L14's 1-5 and SH. The physical lug identity is not recorded. (source: the tube radio sheet; the author)
- (52.32,49.28), FM RF / MIXER: "VC1 and VC2 are mechanically ganged with VC3/VC4.
Factory AFC series C11 and oscillator feedback differ from SAMS.
Unspecified coil values and factory tube section-to-pin mapping need chassis verification." — For this group: the tube pin mapping (V4, V5) was checked on the chassis. Coil values that no source gives, such as the IF cans' inductance and tuning capacitance, remain unspecified. (source: the tube radio sheet; the author)
- (37.59,161.29), SERIES HEATER STRING - ALL SEVEN TUBES: "SERIES HEATER STRING - ALL SEVEN TUBES" — Section title where the heater units of V4 and V5 are drawn. (source: the tube radio sheet)
- (39.62,222.76), ORIGINAL TRANSFORMERLESS AC SUPPLY; bears on CHASSIS and the heater net names: "CHASSIS is circuit common, directly mains referenced in these source drawings.
AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet.
Voltages in net names are SAMS nominal identifiers, not guaranteed measured values.
L18 and L17 were removed by accident, replaced with the external EMI filter." — CHASSIS, which takes 12 pins in this group, is the radio's circuit common. In the radio it is on neutral: J4 takes AC_N through the external EMI filter, with no isolation transformer. It is never bonded to Earth or the Faraday cages, and the coax shield reaches it only through C51. Numbers in heater net names such as H36 are nominal. L17 and L18 are not in the radio and are not drawn as parts on the sheet. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **R12** fitted `70Ω` — The fitted 70Ω matches the original factory value. SAMS originally gave 68 Ω and listed 70 Ω as an alternate. The sheet's value is the fitted state. No wattage, tolerance or working voltage is recorded, original or fitted. Original: Spec Factory resistance (Ω): 70 | Spec SAMS resistance (Ω): 68 | Spec SAMS stated power (W): Not specified | Spec Original tolerance: Not specified | Spec Original working voltage: Not specified | Spec Differences: Factory 70 Ω; SAMS 68 Ω (70 Ω alternate). (source: the tube radio sheet; the author)
- **C28** fitted `0.002uF` — The fitted 0.002uF (2 nF) matches the original factory value. The SAMS schematic and parts table both gave 10 nF, five times larger. The sheet's value is the fitted state. No voltage rating is recorded. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 10000 | Spec SAMS table (pF): 10000 | Spec SAMS stated VDC: Not specified | Spec Factory / SAMS differences: Factory 2000.0 pF; SAMS schematic 10000.0 pF; table 10000 pF. (source: the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **C10** — row: Capacitors: C10; Function: First FM IF supply bypass; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C28** — row: Capacitors: C28; Function: First IF cathode bypass; Factory value (pF): 2000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Factory 2000.0 pF; SAMS schematic 10000.0 pF; table 10000 pF.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C30** — row: Capacitors: C30; Function: FM limiter grid-return bypass; Factory value (pF): 50; SAMS schematic (pF): 50; SAMS table (pF): 50; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Both drawings and table rating 50 pF. SAMS prints Centralab DD-501 here, but DD-500 for the other 50 pF part C19. Suspected replacement-number error; DD-501 not approved.; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C33** — row: Capacitors: C33; Function: Ratio-detector plate-supply bypass; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L10** — row: Coils and other parts: L10; Category: Magnetic component; Function: First FM IF transformer; Factory specification: FM IF (A); SAMS additional specification: 10.7 MHz; primary/secondary DCR 0.8 Ω / 0.8 Ω; Catalog check ID: OEM number only; Selection / missing specifications: Need tuned frequency, winding coupling/impedance, pinout, internal capacitor values, Q/bandwidth and mechanical fit. DCR is a diagnostic value, not impedance or inductance.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L12** — row: Coils and other parts: L12; Category: Magnetic component; Function: Second FM IF transformer; Factory specification: FM IF (B); SAMS additional specification: 10.7 MHz; primary/secondary DCR 0.6 Ω / 0.4 Ω; Catalog check ID: OEM number only; Selection / missing specifications: Need tuned frequency, winding coupling/impedance, pinout, internal capacitor values, Q/bandwidth and mechanical fit. DCR is a diagnostic value, not impedance or inductance.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L14** — row: Coils and other parts: L14; Category: Magnetic component; Function: FM ratio-detector transformer; Factory specification: Ratio detector, tertiary winding; SAMS additional specification: 10.7 MHz; primary DCR 1.2 Ω, secondary halves 0.4 Ω each, tertiary 0.5 Ω; Catalog check ID: OEM number only; Selection / missing specifications: Need tuned frequency, winding coupling/impedance, pinout, internal capacitor values, Q/bandwidth and mechanical fit. DCR is a diagnostic value, not impedance or inductance.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **R12** — row: Resistors: R12; Function: First IF cathode bias; Factory resistance (Ω): 70; SAMS resistance (Ω): 68; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Factory 70 Ω; SAMS 68 Ω (70 Ω alternate).; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R14** — row: Resistors: R14; Function: FM limiter grid return; Factory resistance (Ω): 250000; SAMS resistance (Ω): 250000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R18** — row: Resistors: R18; Function: Ratio-detector supply decoupling; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R4** — row: Resistors: R4; Function: First FM IF supply decoupling; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic

## 16. Tube radio — FM ratio detector and AFC control

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in RATIO DETECTOR / AFC on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| C2 | Polarized (electrolytic) capacitor across the ratio-detector rails; Spec function 'FM ratio-detector stabilizing capacitor'. + (pin #1) on DET_BOTTOM, − (pin #2) on DET_TOP. | 10uF | fitted | — | the tube radio sheet; note "C2 is electrolytic."; polarity Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch; the author (polarity as fitted) |
| C34 | Unpolarized capacitor, ratio-detector upper balancing capacitor, DET_TOP to FM_DET_RAW (dielectric not stated as fitted) | 200pF | fitted | — | the tube radio sheet; the author |
| C35 | Unpolarized capacitor, ratio-detector lower balancing capacitor, FM_DET_RAW to DET_BOTTOM (dielectric not stated as fitted) | 200pF | fitted | — | the tube radio sheet; the author |
| C36 | Unpolarized capacitor, FM detector output RF bypass, FM_DET_RAW to CHASSIS (dielectric not stated as fitted) | 200pF | fitted | — | the tube radio sheet; the author |
| C37 | Unpolarized capacitor, FM audio de-emphasis/filter, FM_AUDIO to CHASSIS (dielectric not stated as fitted) | 0.001uF | fitted | — | the tube radio sheet; the author |
| M3 | Upper FM ratio-detector diode (type not stated beyond the Value); K (#1) on DET_D1_A (R19 side), A (#2) on DET_TOP | FM Diode | fitted | — | the tube radio sheet; the author |
| M4 | Lower FM ratio-detector diode (type not stated beyond the Value); K (#1) on DET_BOTTOM, A (#2) on DET_D2_A (R21 side) | FM Diode | fitted | — | the tube radio sheet; the author |
| R19 | Fixed resistor, upper ratio-detector series resistor, DET_SEC_H (L14 SEC #1) to DET_D1_A (M3 K) | 1kΩ | fitted | — | the tube radio sheet; the author |
| R20 | Fixed resistor, upper ratio-detector DC load, DET_TOP to CHASSIS | 10kΩ | fitted | — | the tube radio sheet; the author |
| R21 | Fixed resistor, lower ratio-detector series resistor, DET_SEC_L (L14 SEC #2) to DET_D2_A (M4 A) | 1kΩ | fitted | — | the tube radio sheet; the author |
| R22 | Fixed resistor, lower ratio-detector DC load, CHASSIS to DET_BOTTOM | 10kΩ | fitted | — | the tube radio sheet; the author |
| R23 | Fixed resistor, AFC feed, FM_DET_RAW to AFC_CTL | 3MΩ | fitted | — | the tube radio sheet; the author |
| R24 | Fixed resistor, FM de-emphasis/filter resistor, FM_DET_RAW to FM_AUDIO | 50kΩ | fitted | — | the tube radio sheet; the author |
| S1A | Unit of mode switch S1 (SW / AM / FM / FM-AFC selector, SAMS M5), Section 'AFC disable': COM #13 on CHASSIS; FM #3 on AFC_CTL; SW #1, AM #2, AFC #4 no-connect. Terminal identifiers logical. Symbol Mine:TM-838M_UNIT1 is the M-for-N typo (SCD4: closed, not corrected). | Mode selector M5 | fitted | — | the tube radio sheet; the author; SCD4; §22 |
| S1B | Unit of mode switch S1, Section 'Audio mode': COM #14 on VOLUME_IN; SW #5 and AM #6 on AM_AUDIO; FM #7 and AFC #8 on FM_AUDIO. Terminal identifiers logical. Symbol typo SCD4 (closed, not corrected). | Mode selector M5 | fitted | — | the tube radio sheet; the author (S1B contact 5 on AM_AUDIO); SCD4 |

### Connections

#### RATIO DETECTOR / AFC

- **DET_SEC_H** — One end of the ratio-detector transformer L14's secondary goes to the upper 1 kΩ series resistor R19. L14's terminal #1 is a logical identifier, not a physical lug.
  - members: L14.SEC (#1) [10.7 MHz FM IF PATH, T3], R19.#1
  - source: the tube radio sheet; the author
- **DET_SEC_L** — The other end of L14's secondary goes to the lower 1 kΩ series resistor R21. Terminal #2 is logical.
  - members: L14.SEC (#2) [T3], R21.#1
  - source: the tube radio sheet; the author
- **DET_D1_A** — R19's far end goes to the cathode of the upper diode M3. This orientation and resistor order are the actual hardware.
  - members: R19.#2, M3.K (#1)
  - source: the tube radio sheet; the author
- **DET_D2_A** — R21's far end goes to the anode of the lower diode M4. This orientation and resistor order are the actual hardware.
  - members: R21.#2, M4.A (#2)
  - source: the tube radio sheet; the author
- **DET_TOP** — The upper detector rail: M3's anode, the top of C34, the top of R20, and C2's negative terminal.
  - members: M3.A (#2), C34.#2, R20.#1, C2.#2 (negative terminal)
  - source: the tube radio sheet; Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch; the author
- **DET_BOTTOM** — The lower detector rail: M4's cathode, the bottom of C35, the bottom of R22, and C2's positive terminal.
  - members: M4.K (#1), C35.#1, R22.#2, C2.#1 (positive terminal)
  - source: the tube radio sheet; Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch; the author
- **CHASSIS** — The midpoint of the two 10 kΩ DC loads is on CHASSIS.
  - members: R20.#2, R22.#1
  - source: the tube radio sheet
- **FM_DET_RAW** — The detector output: the C34/C35 midpoint joined to L14's tertiary winding (terminal #5, logical). It feeds C36 to CHASSIS, R24 toward FM_AUDIO and R23 toward AFC_CTL.
  - members: C34.#1, C35.#2, L14.TERT (#5) [T3], C36.#2, R24.#1, R23.#1
  - source: the tube radio sheet; the author
- **CHASSIS** — The returns of the RF bypass C36 and the de-emphasis capacitor C37 are on CHASSIS.
  - members: C36.#1, C37.#1
  - source: the tube radio sheet
- **FM_AUDIO** — The de-emphasised FM audio at the R24/C37 junction goes to S1B's FM and FM-AFC contacts. Nothing else on any drawing carries FM_AUDIO. S1B contact numbers are logical.
  - members: R24.#2, C37.#2, S1B.FM (#7), S1B.AFC (#8)
  - source: the tube radio sheet; label names across all drawings; the author
- **AFC_CTL** — The AFC control line runs from R23 to C12 (other pin on CHASSIS) and R5 in the AFC / local oscillator section. S1A's FM contact is on it. S1A's common is on CHASSIS, so AFC_CTL is grounded in the FM position and left open in SW, AM and FM-AFC.
  - members: R23.#2, S1A.FM (#3), C12.#2 [AFC / LOCAL OSCILLATOR, T2], R5.#1 [AFC / LOCAL OSCILLATOR, T2]
  - source: the tube radio sheet
- **CHASSIS** — S1A's common is on CHASSIS. Contact #13 is logical.
  - members: S1A.COM (#13, label attached to the pin)
  - source: the tube radio sheet; the author
- **not connected (each contact alone)** — S1A's SW, AM and FM-AFC contacts are not connected. S1B has no unconnected contacts.
  - members: S1A.SW (#1) no-connect, S1A.AM (#2) no-connect, S1A.AFC (#4) no-connect
  - source: the tube radio sheet
- **AM_AUDIO** — The AM detector audio from the 455 kHz AM IF / detector section reaches S1B's SW and AM contacts. Contact 5 (SW) on AM_AUDIO is the actual hardware.
  - members: S1B.SW (#5), S1B.AM (#6), R15.#2 [T5], R16.#2 [T5], C32.#1 [T5], R17.#1 [T5]
  - source: the tube radio sheet; the author
- **VOLUME_IN** — S1B's common feeds the top of the 500 kΩ volume control R1: AM_AUDIO in SW and AM, FM_AUDIO in FM and FM-AFC. R1 is this part's reference only: the colon's 1.5 kΩ variable resistor is R99 since 2026-09-23 (SCE2, done).
  - members: S1B.COM (#14, label attached to the pin), R1.1 [AUDIO AMPLIFIER, T7; 500kΩ volume potentiometer]
  - source: the tube radio sheet; SCE2
- **CHASSIS** — CHASSIS is the radio's circuit common. In the device it is on mains neutral: J4 from neutral (AC_N) through the external EMI filter, no isolation transformer. It is never bonded to Earth or the Faraday cages. COAX_SHIELD_PE is on Earth and reaches CHASSIS only through C51.
  - members: this group: R20.#2, R22.#1, C36.#1, C37.#1, S1A.COM, sheet-wide: every other CHASSIS pin of the tube radio sheet, including J4.#1, whose net is also named AC2
  - source: the tube radio sheet; the author
- **(switch S1 mechanics)** — S1A and S1B are units of the one four-position mode switch S1 (SW / AM / FM / FM-AFC); all units rotate together. S1A: common 13 to contact 3 in FM, open in FM-AFC. S1B: common 14; contacts 5/6 AM, 7/8 FM. Terminal numbers are logical.
  - members: S1A, S1B, S1C–S1G elsewhere
  - source: the tube radio sheet; the author; §22

### Notes written on the tube radio sheet (verbatim)

- (153.67,96.01), RATIO DETECTOR / AFC: "RATIO DETECTOR / AFC" — Section title. (source: the tube radio sheet)
- (184.66,116.33), RATIO DETECTOR / AFC: "CHASSIS is at the midpoint of R20 / R22.
FM_DET_RAW is the C34 / C35 midpoint and L14 tertiary.
S1A grounds AFC_CTL only in FM without AFC." — A reading aid that agrees with the drawn nets. CHASSIS holds R20.#2 and R22.#1. FM_DET_RAW holds C34.#1, C35.#2 and L14.TERT. S1A's FM contact is on AFC_CTL, and its SW, AM and FM-AFC contacts are no-connect. (source: the tube radio sheet)
- (149.10,116.59), RATIO DETECTOR / AFC (narrow left strip): "Factory diodes: upper cathode faces the transformer;
lower cathode faces the load. Opposite to SAMS.
C2 is electrolytic." — M3 and M4 are drawn in the factory orientation, opposite to SAMS, and C2 is electrolytic. The drawn diode orientation, the 1 kΩ order and C2's polarity are the actual hardware: M3 K toward R19/L14 with A on DET_TOP; M4 A toward R21/L14 with K on DET_BOTTOM; C2 + on DET_BOTTOM. (source: the tube radio sheet; the author)
- (120.14,173.99), outside this section: inside SW / AM LOCAL OSCILLATOR (T6); bears on S1A/S1B: "All S1 units rotate together: SW / AM / FM / FM-AFC.
VC1, VC2, VC3 and VC4 are one ganged tuning assembly.
Logical switch and coil terminals require a physical lug crosscheck." — S1A and S1B are units of one four-position mode switch. The lug crosscheck is answered: the switch and coil terminal identifiers are logical, not physical lugs. (source: the tube radio sheet; the author)
- (39.62,222.76), outside this group, inside ORIGINAL TRANSFORMERLESS AC SUPPLY (T9): "CHASSIS is circuit common, directly mains referenced in these source drawings.
AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet.
Voltages in net names are SAMS nominal identifiers, not guaranteed measured values.
L18 and L17 were removed by accident, replaced with the external EMI filter." — In the device, CHASSIS is on neutral, with J4 from AC_N through the external EMI filter and no isolation transformer. CHASSIS is never bonded to Earth or the cages. So this group's CHASSIS returns are mains-referenced. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **C2** fitted `10uF, + (pin #1) on DET_BOTTOM` — The capacitance matches all three original sources (10000000 pF). The polarity differs from the SAMS statement, and the drawn polarity is what is fitted. The 50 VDC is SAMS's original rating. No fitted voltage rating is recorded. Original: Spec Factory / SAMS differences: Factory polarity unresolved; SAMS positive to upper detector rail. Diode orientations differ. | Spec Selection / missing specifications: 10 µF, ≥50 VDC per SAMS. Confirm polarity on actual detector circuit before fitting. | Spec SAMS stated VDC: 50 (source: the tube radio sheet; the author)
- **M3** fitted `FM Diode; K (#1) toward R19 / L14, A (#2) on DET_TOP` — Neither original gives a type. The fitted orientation and resistor placement are as drawn (factory orientation, opposite to SAMS). Original: Spec Factory specification: Discrete diode, type unmarked | Spec SAMS additional specification: Diode; ratio detector. No part number supplied. | Spec Differences / context: Factory and SAMS diode orientation/resistor placement differ. Confirm actual polarity. (source: the tube radio sheet; the author)
- **M4** fitted `FM Diode; K (#1) on DET_BOTTOM, A (#2) toward R21 / L14` — Neither original gives a type. The fitted orientation and resistor placement are as drawn (factory orientation, opposite to SAMS). Original: Spec Factory specification: Discrete diode, type unmarked | Spec SAMS additional specification: Diode; ratio detector. No part number supplied. | Spec Differences / context: Factory and SAMS diode orientation/resistor placement differ. Confirm actual polarity. (source: the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **C2** — row: Capacitors: C2; Function: FM ratio-detector stabilizing capacitor; Factory value (pF): 10000000; SAMS schematic (pF): 10000000; SAMS table (pF): 10000000; SAMS stated VDC: 50; SAMS stated tolerance: Not specified; Type supported by substitute: Polarized electrolytic; Check ID: K-DET; Factory / SAMS differences: Factory polarity unresolved; SAMS positive to upper detector rail. Diode orientations differ.; Selection / missing specifications: 10 µF, ≥50 VDC per SAMS. Confirm polarity on actual detector circuit before fitting.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C34** — row: Capacitors: C34; Function: Ratio-detector upper balancing capacitor; Factory value (pF): 200; SAMS schematic (pF): 200; SAMS table (pF): 200; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C35** — row: Capacitors: C35; Function: Ratio-detector lower balancing capacitor; Factory value (pF): 200; SAMS schematic (pF): 200; SAMS table (pF): 200; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C36** — row: Capacitors: C36; Function: FM detector output RF bypass; Factory value (pF): 200; SAMS schematic (pF): 200; SAMS table (pF): 200; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C37** — row: Capacitors: C37; Function: FM audio de-emphasis/filter; Factory value (pF): 1000; SAMS schematic (pF): 1000; SAMS table (pF): 1000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: 1000 pF (1 nF), not 10 nF. Match de-emphasis/filter time constant; original tolerance unspecified.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **M3** — row: Coils and other parts: M3; Category: Detector diode; Function: FM ratio detector; Factory specification: Discrete diode, type unmarked; SAMS additional specification: Diode; ratio detector. No part number supplied.; Catalog check ID: No verified substitute; Selection / missing specifications: Semiconductor material, forward curve, leakage, capacitance, reverse voltage and matching unspecified. No evidence for a particular 1N34/1N60 replacement.; Differences / context: Factory and SAMS diode orientation/resistor placement differ. Confirm actual polarity.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **M4** — row: Coils and other parts: M4; Category: Detector diode; Function: FM ratio detector; Factory specification: Discrete diode, type unmarked; SAMS additional specification: Diode; ratio detector. No part number supplied.; Catalog check ID: No verified substitute; Selection / missing specifications: Semiconductor material, forward curve, leakage, capacitance, reverse voltage and matching unspecified. No evidence for a particular 1N34/1N60 replacement.; Differences / context: Factory and SAMS diode orientation/resistor placement differ. Confirm actual polarity.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **R19** — row: Resistors: R19; Function: Upper ratio-detector series resistor; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R20** — row: Resistors: R20; Function: Upper ratio-detector DC load; Factory resistance (Ω): 10000; SAMS resistance (Ω): 10000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R21** — row: Resistors: R21; Function: Lower ratio-detector series resistor; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R22** — row: Resistors: R22; Function: Lower ratio-detector DC load; Factory resistance (Ω): 10000; SAMS resistance (Ω): 10000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R23** — row: Resistors: R23; Function: AFC feed; Factory resistance (Ω): 3000000; SAMS resistance (Ω): 3000000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R24** — row: Resistors: R24; Function: FM de-emphasis/filter resistor; Factory resistance (Ω): 50000; SAMS resistance (Ω): 50000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **S1A** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **S1B** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables

## 17. Tube radio — 455 kHz AM IF and AM detector

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in 455 kHz AM IF AND AM DETECTOR on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| L11 | First AM IF transformer (Spec Function), a 455 kHz IF can (Radio:IF symbol) with primary terminals B and P, secondary terminals G and E, and a can terminal SH. All terminal identifiers are logical, not physical lugs. The symbol draws the can's internal tuning capacitors; neither source gives their values (sheet note at 178.05,57.91). | AM IF A - 455kHz | fitted | — | the tube radio sheet; the author |
| L13 | Second AM IF transformer (Spec Function), a 455 kHz IF can (Radio:IF symbol) with primary terminals B and P, secondary terminals G and E, and a can terminal SH. All logical identifiers. Internal tuning capacitors drawn in the symbol, values not supplied by either source. Its secondary feeds the AM detector diodes of V6. | AM IF B - 455kHz | fitted | — | the tube radio sheet; the author |
| C29 | Unpolarized capacitor, Spec Function 'Second AM IF / screen supply bypass' | 0.01uF | fitted | — | the tube radio sheet; the author |
| C31 | Unpolarized capacitor, Spec Function 'AM detector IF bypass' | 100pF | fitted | — | the tube radio sheet; the author |
| C32 | Unpolarized capacitor, Spec Function 'AM audio IF bypass' | 100pF | fitted | — | the tube radio sheet; the author |
| R13 | Fixed resistor, Spec Function 'AM IF/screen decoupling' | 1kΩ | fitted | — | the tube radio sheet; the author |
| R15 | Fixed resistor, Spec Function 'AM detector load/filter' | 50kΩ | fitted | — | the tube radio sheet; the author |
| R16 | Fixed resistor, Spec Function 'AM detector DC return' | 1MΩ | fitted | — | the tube radio sheet; the author |
| R17 | Fixed resistor, Spec Function 'AM AVC feed' | 2MΩ | fitted | — | the tube radio sheet; the author |

### Connections

#### 455 kHz AM IF AND AM DETECTOR

- **AM_PLATE** — The 12BE6 converter's plate, pin 5, goes to L11 primary terminal B. The terminal letter is logical; the tube pin was checked on the chassis.
  - members: L11 PRI B (logical identifier), V3 12BE6 pin 5 (P), unit V3@124.46,115.57
  - source: the tube radio sheet; the author
- **AM_B** — L11 primary terminal P and the 12BE6 screen grids (pin 6) share AM_B. S1D, whose common is on B115_A, puts B115_A onto AM_B in the SW and AM positions. No bypass capacitor is on this net: the sheet stands against SAMS's .002.
  - members: L11 PRI P (logical identifier), V3 12BE6 pin 6 (G2/G4), S1D contact AM #6 (logical identifier), S1D contact SW #5 (logical identifier)
  - source: the tube radio sheet; the author
- **IF1_SEC_RETURN** — L11's secondary terminal G joins L10's secondary terminal E. L10's other secondary terminal G is on IF1_GRID with V4 pin 1, so the AM IF signal reaches the 12BA6 V4 grid through L10's secondary, in series.
  - members: L11 SEC G (logical identifier), L10 SEC E (logical identifier)
  - source: the tube radio sheet; the author
- **IF1_GRID** — V4's control grid, pin 1, is on the far end of L10's secondary.
  - members: V4 12BA6 pin 1 (G1), L10 SEC G (logical identifier)
  - source: the tube radio sheet; the author
- **AVC** — The AVC line joins L11 secondary terminal E, the far end of R17 from the detector output, the AM antenna trimmer C23A, the bypass C24 (other pin on CHASSIS) and the loopstick L7.
  - members: L11 SEC E (logical identifier), R17 #2, C23A #1 (AM RF Trim), C24 #1 (0.05uF; C24 #2 on CHASSIS), L7 #1 (Loopstick)
  - source: the tube radio sheet; the author
- **IF_CHAIN_MID** — L13 primary terminal B joins L12 primary terminal B. L12's other primary terminal P is on IF1_PLATE with V4 pin 5, so V4's plate load is L12's primary and then L13's primary, in series.
  - members: L13 PRI B (logical identifier), L12 PRI B (logical identifier)
  - source: the tube radio sheet; the author
- **IF1_PLATE** — V4's plate, pin 5, is on the far end of L12's primary.
  - members: V4 12BA6 pin 5 (P), L12 PRI P (logical identifier)
  - source: the tube radio sheet; the author
- **AM_IF2_B** — The supply end of L13's primary and V4's screen grid (pin 6) share AM_IF2_B. R13 1 kΩ feeds it from B115_A; C29 0.01 µF bypasses it to CHASSIS.
  - members: L13 PRI P (logical identifier), R13 #2, C29 #2, V4 12BA6 pin 6 (G2)
  - source: the tube radio sheet; the author
- **B115_A** — R13 takes its supply from B115_A. B115_A is also on R31's output end (R31's other end is on B130), on C1C, on R26, on the S1D common and on the 50C5 screen, pin 6. C1C is one of three separate Rubycon 47 µF 315 V capacitors. B115_A is a SAMS nominal name, not a measured voltage (sheet note at 39.62,222.76).
  - members: R13 #1, R26 #2 (250kΩ audio triode plate load), R31 #2 (300Ω 1W second B+ filter dropper), C1C #1 (47uF B+ filter section), S1D COM #14 (logical identifier), V7 50C5 pin 6 (G2)
  - source: the tube radio sheet; the author
- **AM_DIODE** — L13 secondary terminal G feeds both diode plates of the 12AV6, pins 5 and 6. The tube's cathode, pin 2, is on CHASSIS (net).
  - members: L13 SEC G (logical identifier), V6 12AV6 pin 5 (D1), V6 12AV6 pin 6
  - source: the tube radio sheet; the author
- **AM_DET_LOAD** — The other end of L13's secondary (terminal E) is wired to C31 100 pF (other pin to CHASSIS) and to R15 50 kΩ (other pin to AM_AUDIO).
  - members: L13 SEC E (logical identifier), by wire, C31 #2, R15 #1
  - source: the tube radio sheet; the author
- **AM_AUDIO** — The AM detector output. R15 arrives from the detector load, R16 1 MΩ and C32 100 pF go to CHASSIS, and R17 2 MΩ goes to AVC. S1B puts AM_AUDIO onto its common (VOLUME_IN) in the SW and AM positions.
  - members: R15 #2, R16 #2, C32 #1, R17 #1, S1B contact AM #6 (logical identifier), S1B contact SW #5 (logical identifier)
  - source: the tube radio sheet; the author
- **VOLUME_IN** — The common of S1B goes to pin 1 of the volume control R1, the end opposite pin 3, which is on CHASSIS. R1 is this part's reference only; the colon's resistor is R99 since 2026-09-23 (SCE2, done).
  - members: S1B COM #14 (logical identifier), R1 pin 1 (500kΩ volume control)
  - source: the tube radio sheet; the author; SCE2
- **FM_AUDIO** — In the FM and FM-AFC positions, S1B's common takes FM_AUDIO instead of AM_AUDIO. All S1 units rotate together.
  - members: S1B FM #7 (logical identifier), S1B AFC #8 (logical identifier), C37 #2, R24 #2
  - source: the tube radio sheet; the author
- **FM_B** — In the FM and FM-AFC positions, S1D sends its B115_A common to FM_B instead of AM_B.
  - members: S1D FM #7 (logical identifier), S1D AFC #8 (logical identifier), L3 #2, C5 #2, R18 #1, R4 #1, R8 #2
  - source: the tube radio sheet; the author
- **CHASSIS** — In this group, CHASSIS takes C29, C31, C32, R16 and both can terminals. CHASSIS is the circuit common. It is on neutral: J4 comes from AC_N through the external EMI filter, with no isolation transformer. It is never bonded to Earth or to the Faraday cages; COAX_SHIELD_PE is on Earth and reaches CHASSIS only through C51. The drawn neon bulbs NE1–NE3 are absent from the device.
  - members: C29 #1, C31 #1, C32 #2, R16 #1, L11 CAN SH (logical identifier), L13 CAN SH (logical identifier), elsewhere on the sheet (joined by the label name): J4.#1, whose net is also named AC2, and every other CHASSIS pin of the tube radio sheet
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (151.89,55.12), section 455 kHz AM IF AND AM DETECTOR: "455 kHz AM IF AND AM DETECTOR" — The section title. (source: the tube radio sheet)
- (178.05,57.91), section 455 kHz AM IF AND AM DETECTOR, above L11 and L13: "IF transformers include their internal tuning capacitors. / Values are not supplied by either source. / See the custom symbol (capacitors added inside the can)." — The capacitors drawn inside L11 and L13 are the cans' own tuning capacitors, not separate parts. Neither the factory drawing nor SAMS gives their values. (source: the tube radio sheet)
- (245.36,59.44), section 455 kHz AM IF AND AM DETECTOR, right of L13: "AM_DIODE goes to V6 pins 5 and 6. / The complete 12AV6 is on the audio sheet; / its triode and diodes share cathode pin 2." — The AM detector is the pair of diodes in the 12AV6 V6 (pins 5 and 6), reached through the AM_DIODE label; the triode and the diodes share cathode pin 2. On this sheet the 12AV6 signal unit is in the AUDIO AMPLIFIER section and its heater unit on the heater string. The note's 'audio sheet' wording stays, and the tube pins were checked on the chassis. (source: the tube radio sheet; the author)
- (120.14,173.99), outside T5, inside SW / AM LOCAL OSCILLATOR (T6); bears on S1B, S1D and the L11/L13 terminal letters: "All S1 units rotate together: SW / AM / FM / FM-AFC. / VC1, VC2, VC3 and VC4 are one ganged tuning assembly. / Logical switch and coil terminals require a physical lug crosscheck." — S1B and S1D, which switch AM_AUDIO and AM_B, are units of one four-position mode switch. The crosscheck is answered: the coil and switch terminal identifiers (L5, L6, L8–L14, S1A–S1G) are logical, not physical lugs. (source: the tube radio sheet; the author)
- (39.62,222.76), outside T5 (ORIGINAL TRANSFORMERLESS AC SUPPLY); bears on CHASSIS and B115_A: "CHASSIS is circuit common, directly mains referenced in these source drawings. / AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet. / Voltages in net names are SAMS nominal identifiers, not guaranteed measured values. / L18 and L17 were removed by accident, replaced with the external EMI filter." — CHASSIS is the sheet's common return. B115_A is a SAMS nominal name, not a measured voltage. L17 and L18 are not in the hardware. In the radio: J3 from the switched live (AC_L), J4 from neutral (AC_N) through the external EMI filter, no isolation transformer, CHASSIS on neutral; COAX_SHIELD_PE on Earth, reaching CHASSIS only through C51; CHASSIS never bonded to Earth or the cages. (source: the tube radio sheet; the author)
- (83.57,129.03), outside T5 (AM / SW INPUT AND CONVERTER): "There is no SW/AM antenna in the final build." — The final build has no SW/AM antenna on the converter that drives L11. The note belongs to T6. (source: the tube radio sheet)

### Fitted value against the original specification

- **L11** fitted `AM IF A - 455kHz` — No conflict. The Value carries the factory name and the SAMS frequency but not the SAMS winding resistances (22 Ω / 22 Ω). The Spec calls DCR 'a diagnostic value, not impedance or inductance'. The Spec is the original specification, not a fitted measurement. Original: Spec Factory specification: AM IF (A) | Spec SAMS additional specification: 455 kHz; primary/secondary DCR 22 Ω / 22 Ω (source: the tube radio sheet)
- **L13** fitted `AM IF B - 455kHz` — No conflict. The Value carries the factory name and the SAMS frequency but not the SAMS winding resistances (22 Ω / 20 Ω). Original: Spec Factory specification: AM IF (B) | Spec SAMS additional specification: 455 kHz; primary/secondary DCR 22 Ω / 20 Ω (source: the tube radio sheet)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **L11** — row: Coils and other parts: L11; Category: Magnetic component; Function: First AM IF transformer; Factory specification: AM IF (A); SAMS additional specification: 455 kHz; primary/secondary DCR 22 Ω / 22 Ω; Catalog check ID: OEM number only; Selection / missing specifications: Need tuned frequency, winding coupling/impedance, pinout, internal capacitor values, Q/bandwidth and mechanical fit. DCR is a diagnostic value, not impedance or inductance.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L13** — row: Coils and other parts: L13; Category: Magnetic component; Function: Second AM IF transformer; Factory specification: AM IF (B); SAMS additional specification: 455 kHz; primary/secondary DCR 22 Ω / 20 Ω; Catalog check ID: OEM number only; Selection / missing specifications: Need tuned frequency, winding coupling/impedance, pinout, internal capacitor values, Q/bandwidth and mechanical fit. DCR is a diagnostic value, not impedance or inductance.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C29** — row: Capacitors: C29; Function: Second AM IF / screen supply bypass; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C31** — row: Capacitors: C31; Function: AM detector IF bypass; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C32** — row: Capacitors: C32; Function: AM audio IF bypass; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R13** — row: Resistors: R13; Function: AM IF/screen decoupling; Factory resistance (Ω): 1000; SAMS resistance (Ω): 1000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R15** — row: Resistors: R15; Function: AM detector load/filter; Factory resistance (Ω): 50000; SAMS resistance (Ω): 50000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R16** — row: Resistors: R16; Function: AM detector DC return; Factory resistance (Ω): 1000000; SAMS resistance (Ω): 1000000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R17** — row: Resistors: R17; Function: AM AVC feed; Factory resistance (Ω): 2000000; SAMS resistance (Ω): 2000000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic

## 18. Tube radio — AM / SW input, converter (V3, 12BE6) and SW / AM local oscillator

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in AM / SW INPUT AND CONVERTER; SW / AM LOCAL OSCILLATOR on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| C21 | Fixed capacitor, SW antenna input coupling (Spec Function), in series between SW_ANT and L6's primary (SW_PRI) | 450pF | fitted | — | the tube radio sheet; the author |
| C22A | Adjustable trimmer, SW antenna trimmer (Spec Function), SW_RF to CHASSIS; no source gives a capacitance | SW RF Trim | fitted | — | the tube radio sheet; the author |
| C23A | Adjustable trimmer, AM antenna trimmer (Spec Function), AVC to AM_RF across L7; no source gives a capacitance | AM RF Trim | fitted | — | the tube radio sheet; the author |
| VC3 | Tuning-gang section, AM/SW RF tuning section (Spec Function), AM_GRID to CHASSIS; one of the VC1–VC4 gang (sheet note) | TUNING AM RF | fitted | — | the tube radio sheet; the author |
| C24 | Fixed capacitor, AM AVC return bypass (Spec Function), AVC to CHASSIS | 0.05uF | fitted | — | the tube radio sheet; the author |
| C25 | Fixed capacitor, AM/SW oscillator feedback (Spec Function), AM_OSC_GRID to AM_OSC_TUNE | 100pF | fitted | — | the tube radio sheet; the author |
| R11 | Fixed resistor, AM/SW oscillator grid leak (Spec Function), AM_OSC_GRID to CHASSIS | 20kΩ | fitted | — | the tube radio sheet; the author |
| J2 | One-pin terminal, SW antenna connection (Spec Function), on SW_ANT | SW ANT | absent from the device | — | the tube radio sheet; the author |
| L6 | Two-winding SW antenna matching transformer (Spec Function): PRI #1 on SW_PRI, PRI #2 on CHASSIS, SEC #3 on SW_RF, SEC #4 on CHASSIS; identifiers are logical | SW antenna coil | fitted | — | the tube radio sheet; the author |
| L7 | AM ferrite loopstick (Spec Function): #1 on AVC, #2 on AM_RF | Loopstick | fitted | — | the tube radio sheet; the author |
| S1C | Unit of the one mode switch S1 (Section 'RF selection'): COM #15 on AM_GRID, SW #9 on SW_RF, AM #10 on AM_RF, FM #11 and AFC #12 no-connect; identifiers logical; library name TM-838M is the SCD4 typo (closed, not corrected) | Mode selector M5 | fitted | — | the tube radio sheet; the author; SCD4 |
| V3 | 12BE6 pentagrid converter. Unit 1 (Section '12BE6 converter'): P #5 AM_PLATE, G2/G4 #6 AM_B, G3 #7 AM_GRID, G1 #1 AM_OSC_GRID, K #2 AM_OSC_K. Unit 2 (heater): H #4 on H36, H #3 on H24. Pins and heater order checked on the chassis. | 12BE6 | fitted | — | the tube radio sheet; the author |
| VC4 | Tuning-gang section, AM/SW oscillator tuning section (Spec Function), AM_OSC_TUNE to CHASSIS; one of the VC1–VC4 gang | TUNING AM OSC | fitted | — | the tube radio sheet; the author |
| C26 | Fixed capacitor, SW oscillator padder (Spec Function), SW_OSC_PAD to SW_OSC_TOP | 0.003uF | fitted | — | the tube radio sheet; the author |
| C27 | Fixed capacitor, AM oscillator padder (Spec Function), AM_OSC_PAD to AM_OSC_TOP | 350pF | fitted | — | the tube radio sheet; the author |
| C22B | Adjustable trimmer, SW oscillator trimmer (Spec Function), SW_OSC_TOP to CHASSIS; no source gives a capacitance | SW Oscillator Trim | fitted | — | the tube radio sheet; the author |
| C23B | Adjustable trimmer, AM oscillator trimmer (Spec Function), AM_OSC_PAD to CHASSIS; no source gives a capacitance | AM Oscillator Trim | fitted | — | the tube radio sheet; the author |
| L9 | Tapped SW oscillator coil: TOP #1 on SW_OSC_TOP, TAP #2 on SW_OSC_TAP, BOTTOM #4 on CHASSIS; identifiers logical | SW oscillator coil | fitted | — | the tube radio sheet; the author |
| L8 | Tapped AM oscillator coil: TOP #1 on AM_OSC_TOP, TAP #4 on AM_OSC_TAP, BOTTOM #3 on CHASSIS; identifiers logical | AM oscillator coil | fitted | — | the tube radio sheet; the author |
| S1F | Unit of the one mode switch S1 (Section 'Oscillator tank'): COM #15 on AM_OSC_TUNE, SW #9 on SW_OSC_PAD, AM #10 on AM_OSC_PAD, FM #11 and AFC #12 no-connect; identifiers logical | Mode selector M5 | fitted | — | the tube radio sheet; the author; SCD4 |
| S1G | Unit of the one mode switch S1 (Section 'Oscillator feedback'): COM #13 on AM_OSC_K, SW #1 on SW_OSC_TAP, AM #2 on AM_OSC_TAP, FM #3 and AFC #4 no-connect; identifiers logical | Mode selector M5 | fitted | — | the tube radio sheet; the author; SCD4 |

### Connections

#### AM / SW INPUT AND CONVERTER

- **SW_ANT** — C21's antenna-side lead is on SW_ANT with the SW ANT terminal J2. J2 is not in the radio, and the sheet draws nothing else on SW_ANT.
  - members: C21 #2, J2 #1 (J2 absent from the device)
  - source: the tube radio sheet; the author
- **SW_PRI** — C21's other lead goes to one end of L6's primary.
  - members: C21 #1, L6 PRI #1 (logical identifier)
  - source: the tube radio sheet; the author
- **CHASSIS (L6 primary return)** — The other end of L6's primary is on CHASSIS.
  - members: L6 PRI #2 (logical identifier)
  - source: the tube radio sheet; the author
- **SW_RF** — One end of L6's secondary, the trimmer C22A and S1C's SW contact are one node. The other ends of L6's secondary (SEC #4) and of C22A (#1) are on CHASSIS.
  - members: L6 SEC #3 (logical), C22A #2, S1C SW #9 (logical)
  - source: the tube radio sheet; the author
- **AM_RF** — One end of the loopstick L7, one side of trimmer C23A and S1C's AM contact are one node.
  - members: L7 #2, C23A #2, S1C AM #10 (logical)
  - source: the tube radio sheet; the author
- **AVC** — The loopstick's other end and C23A's other side are on the AVC line, which also reaches L11's secondary and R17. C24 (#2 on CHASSIS) bypasses AVC to CHASSIS.
  - members: L7 #1, C23A #1, C24 #1, L11 SEC (#E) (outside this group; logical), R17 #2 (outside this group; R17 #1 on AM_AUDIO)
  - source: the tube radio sheet; the author
- **AM_GRID** — S1C's common, the gang section VC3 (#1 on CHASSIS) and V3's signal grid, pin 7, are one node. Going by S1C's pin names, it takes SW_RF in SW and AM_RF in AM. In FM and FM-AFC, S1C's contacts #11 and #12 are no-connect.
  - members: S1C COM #15 (logical), V3 G3 #7, VC3 #2
  - source: the tube radio sheet; the author
- **AM_OSC_GRID** — V3's oscillator grid, pin 1, goes to the 20 kΩ R11 (#2 on CHASSIS) and to the 100 pF C25.
  - members: V3 G1 #1, R11 #1, C25 #1
  - source: the tube radio sheet; the author
- **AM_PLATE** — V3's plate, pin 5, goes by label to one end of the primary of the first AM IF transformer L11.
  - members: V3 P #5, L11 PRI (#B) (outside this group; logical)
  - source: the tube radio sheet; the author
- **AM_B** — V3's screen grids, pin 6, join the other end of L11's primary and S1D's SW and AM contacts. S1D's common (#14) is on B115_A. There is no capacitor on this line.
  - members: V3 G2/G4 #6, L11 PRI (#P) (outside this group; logical), S1D SW (#5) (outside this group; logical), S1D AM (#6) (outside this group; logical)
  - source: the tube radio sheet; the author

#### SW / AM LOCAL OSCILLATOR

- **SW_OSC_PAD** — S1F's SW contact goes to the 0.003 µF padder C26.
  - members: S1F SW #9 (logical), C26 #2
  - source: the tube radio sheet; the author
- **SW_OSC_TOP** — C26, the trimmer C22B (#1 on CHASSIS) and the top of the SW oscillator coil L9 are one node.
  - members: C26 #1, C22B #2, L9 TOP #1 (logical)
  - source: the tube radio sheet; the author
- **AM_OSC_PAD** — S1F's AM contact, the 350 pF padder C27 and the trimmer C23B (#1 on CHASSIS) are one node.
  - members: S1F AM #10 (logical), C27 #2, C23B #2
  - source: the tube radio sheet; the author
- **AM_OSC_TOP** — C27 goes to the top of the AM oscillator coil L8.
  - members: C27 #1, L8 TOP #1 (logical)
  - source: the tube radio sheet; the author
- **SW_OSC_TAP** — S1G's SW contact goes to L9's tap.
  - members: S1G SW #1 (logical), L9 TAP #2 (logical)
  - source: the tube radio sheet; the author
- **AM_OSC_TAP** — S1G's AM contact goes to L8's tap.
  - members: S1G AM #2 (logical), L8 TAP #4 (logical)
  - source: the tube radio sheet; the author
- **CHASSIS (coil bottoms)** — The bottoms of both oscillator coils are on CHASSIS.
  - members: L9 BOTTOM #4 (logical), L8 BOTTOM #3 (logical)
  - source: the tube radio sheet; the author

#### Across its sections

- **AM_OSC_TUNE** — C25's far side, the oscillator gang section VC4 (#1 on CHASSIS) and S1F's common are one node.
  - members: C25 #2, VC4 #2, S1F COM #15 (logical)
  - source: the tube radio sheet; the author
- **AM_OSC_K** — V3's cathode, pin 2, goes only to S1G's common. S1G's FM #3 and AFC #4 contacts are no-connect.
  - members: V3 K #2, S1G COM #13 (logical)
  - source: the tube radio sheet; the author
- **H36** — V3's heater pin 4 is in series with V4's heater pin 3, and C47 bypasses that node to CHASSIS.
  - members: V3 H #4, V4 H #3 (12BA6 heater unit), C47 #1 (0.0022 µF; #2 on CHASSIS)
  - source: the tube radio sheet; the author
- **H24** — V3's heater pin 3 goes through the RF heater choke L16 toward V2's heater.
  - members: V3 H #3, L16 #1 (RF Heater Choke; #2 on H_V2_IN)
  - source: the tube radio sheet; the author
- **CHASSIS** — Every CHASSIS label on the sheet is one conductor, which the sheet also names AC2 at J4. CHASSIS is on mains neutral through J4 and the external EMI filter, with no isolation transformer. It is never bonded to Earth or the Faraday cages.
  - members: C22A #1, C22B #1, C23B #1, C24 #2, L6 PRI #2, L6 SEC #4, L8 BOTTOM #3, L9 BOTTOM #4, R11 #2, VC3 #1, VC4 #1
  - source: the tube radio sheet; the author
- **no-connects** — The FM and FM-AFC contacts of S1C, S1F and S1G carry no-connect flags. All identifiers are logical.
  - members: S1C FM #11, S1C AFC #12, S1F FM #11, S1F AFC #12, S1G FM #3, S1G AFC #4
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (15.75,100.84): "AM / SW INPUT AND CONVERTER" — Section title. (source: the tube radio sheet)
- (83.57,129.03), inside AM / SW INPUT AND CONVERTER: "There is no SW/AM antenna in the final build." — Of the drawn antenna-input parts, only the SW ANT terminal J2 is absent from the device. C21, L6, C22A, the loopstick L7 and C23A are fitted. (source: the tube radio sheet; the author)
- (46.48,133.60): "SW / AM LOCAL OSCILLATOR" — Section title. (source: the tube radio sheet)
- (120.14,173.99), SW / AM LOCAL OSCILLATOR rectangle (115.57,159.38)–(165.1,178.44), near S1G: "All S1 units rotate together: SW / AM / FM / FM-AFC.
VC1, VC2, VC3 and VC4 are one ganged tuning assembly.
Logical switch and coil terminals require a physical lug crosscheck." — S1A–S1G are one four-position switch, and VC1–VC4 are one gang. The third sentence is answered: the coil and switch terminal identifiers (L5, L6, L8–L14, S1A–S1G) are logical, not physical lugs. No source maps them to lugs. (source: the tube radio sheet; the author)
- S1C Transcription_note (hidden property), part at (30.48,151.13): "SAMS M5 section 2 common 15; 9=SW, 10=AM; 11 (FM) and 12 (FM-AFC) not connected. Logical throws only." — S1C's identifiers are SAMS M5 section 2 markings: common 15, 9 in SW, 10 in AM, 11 and 12 unused. They are logical identifiers, not physical lugs. (source: the tube radio sheet; the author)
- S1F Transcription_note (hidden property), part at (64.77,153.67): "SAMS M5 section 3 common 15; SW=9, AM=10; inactive FM contacts." — S1F's identifiers are SAMS M5 section 3 markings: common 15, 9 in SW, 10 in AM, FM contacts inactive. They are logical identifiers. (source: the tube radio sheet; the author)
- S1G Transcription_note (hidden property), part at (137.16,162.56): "SAMS M5 section 3 common 13; SW=1, AM=2; inactive FM contacts." — S1G's identifiers are SAMS M5 section 3 markings: common 13, 1 in SW, 2 in AM, FM contacts inactive. They are logical identifiers. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **C24** fitted `0.05uF` — No difference in capacitance. The 50 V is SAMS's original rating. No fitted rating is recorded. Original: Spec Factory value (pF): 50000 / Spec SAMS schematic (pF): 50000 / Spec SAMS table (pF): 50000 / Spec SAMS stated VDC: 50 (source: the tube radio sheet)
- **C26** fitted `0.003uF` — No difference in capacitance (3000 pF). The ±5% is SAMS's original tolerance. The silvered mica type and 500 VDC are given for the listed substitute. No source records the fitted part's type or rating. Original: Spec SAMS stated tolerance: ±5% / Spec Type supported by substitute: Silvered mica / Spec Selection / missing specifications: 3000 pF ±5%, stable low-loss RF capacitor. 500 VDC is established for the listed mica substitute. (source: the tube radio sheet)
- **C27** fitted `350pF` — No difference. Some listed substitutes are not 350 pF. Original: Spec Factory / SAMS differences: Both drawings 350 pF. Sprague 5GA-T35 is 350 pF; Aerovox DI-330 and other listed numbers differ. (source: the tube radio sheet)
- **C21** fitted `450pF` — No difference. Listed Sprague and several other substitutes are 470 pF. Original: Spec Factory / SAMS differences: Both drawings 450 pF; listed Sprague and several other substitutes are 470 pF. (source: the tube radio sheet)
- **C22A, C22B, C23A, C23B** fitted `SW RF Trim / SW Oscillator Trim / AM RF Trim / AM Oscillator Trim` — Each Value is a function name. No source gives a capacitance for these trimmers. Original: Spec Factory value (pF): Not specified / Spec SAMS schematic (pF): Not specified / Spec SAMS table (pF): Not specified (source: the tube radio sheet)
- **VC3** fitted `TUNING AM RF` — The Value says AM, but the Spec calls it the AM/SW section, part of tuning assembly M2. No capacitance range is supplied. Original: Spec Function: AM/SW RF tuning section / Spec SAMS table (pF): Part of M2 (source: the tube radio sheet)
- **VC4** fitted `TUNING AM OSC` — The Value says AM, but the Spec calls it the AM/SW oscillator section, part of M2. No capacitance range is supplied. Original: Spec Function: AM/SW oscillator tuning section / Spec SAMS table (pF): Part of M2 (source: the tube radio sheet)
- **L6** fitted `SW antenna coil` — The Value is a name. SAMS gives the original winding resistances and band. Original: Spec SAMS additional specification: DCR 0.3 Ω each winding; 3.5–9 MHz receiver band (source: the tube radio sheet)
- **L7** fitted `Loopstick` — The Value is a name. SAMS gives the original winding resistance and band. Original: Spec SAMS additional specification: Winding DCR 0.6 Ω; 540–1620 kHz receiver band (source: the tube radio sheet)
- **L8** fitted `AM oscillator coil` — The Value is a name. SAMS gives the original section resistances. The drawn terminals are logical. Original: Spec SAMS additional specification: Winding DCR 4.6 Ω and 0.9 Ω sections (source: the tube radio sheet; the author)
- **L9** fitted `SW oscillator coil` — The Value is a name. SAMS gives the original section resistances. Original: Spec SAMS additional specification: Winding DCR 0.3 Ω and 0.1 Ω sections (source: the tube radio sheet)
- **S1C, S1F, S1G** fitted `Mode selector M5` — The Value uses SAMS's designator M5 for the switch whose reference is S1. The seven S1 units are one four-position switch. Original: Spec SAMS additional specification: Four positions; three physical sections/wafer terminal guides. / Spec Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches. (source: the tube radio sheet; §22)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **C21** — row: Capacitors: C21; Function: SW antenna input coupling; Factory value (pF): 450; SAMS schematic (pF): 450; SAMS table (pF): 450; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Both drawings 450 pF; listed Sprague and several other substitutes are 470 pF.; Selection / missing specifications: External antenna coupling on line-operated chassis: original DC-rated substitutes do not establish a modern safety class or insulation design.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C22A** — row: Capacitors: C22A; Function: SW antenna trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Capacitance range, dielectric, Q, travel and mounting unspecified. Part of band alignment; do not select by appearance.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C23A** — row: Capacitors: C23A; Function: AM antenna trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Capacitance range, dielectric, Q, travel and mounting unspecified. Part of band alignment; do not select by appearance.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **VC3** — row: Capacitors: VC3; Function: AM/SW RF tuning section; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Part of M2; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Mechanically ganged variable capacitor section; Check ID: No catalog substitute; Factory / SAMS differences: VC labels newly assigned; sections belong to tuning assembly M2, not four independent controls.; Selection / missing specifications: Match min/max capacitance, tracking law, shaft/gang geometry and trimmer arrangement; no numerical range supplied.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C24** — row: Capacitors: C24; Function: AM AVC return bypass; Factory value (pF): 50000; SAMS schematic (pF): 50000; SAMS table (pF): 50000; SAMS stated VDC: 50; SAMS stated tolerance: Not specified; Type supported by substitute: Miniature high-K ceramic; Check ID: K-AVC; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C25** — row: Capacitors: C25; Function: AM/SW oscillator feedback; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R11** — row: Resistors: R11; Function: AM/SW oscillator grid leak; Factory resistance (Ω): 20000; SAMS resistance (Ω): 20000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **J2** — row: Coils and other parts: J2; Category: Terminal; Function: SW antenna connection; Factory specification: Antenna terminal with series coupling capacitor; SAMS additional specification: Terminal; no electrical/mechanical rating supplied.; Catalog check ID: No verified substitute; Selection / missing specifications: Match insulated terminal arrangement and antenna coupling design. J references newly assigned.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L6** — row: Coils and other parts: L6; Category: Magnetic component; Function: SW antenna matching transformer; Factory specification: Two-winding SW matching coil; SAMS additional specification: DCR 0.3 Ω each winding; 3.5–9 MHz receiver band; Catalog check ID: No verified substitute; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L7** — row: Coils and other parts: L7; Category: Magnetic component; Function: AM ferrite loopstick; Factory specification: Ferrite bar with antenna winding; SAMS additional specification: Winding DCR 0.6 Ω; 540–1620 kHz receiver band; Catalog check ID: K-LOOP; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **S1C** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **VC4** — row: Capacitors: VC4; Function: AM/SW oscillator tuning section; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Part of M2; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Mechanically ganged variable capacitor section; Check ID: No catalog substitute; Factory / SAMS differences: VC labels newly assigned; sections belong to tuning assembly M2, not four independent controls.; Selection / missing specifications: Match min/max capacitance, tracking law, shaft/gang geometry and trimmer arrangement; no numerical range supplied.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C26** — row: Capacitors: C26; Function: SW oscillator padder; Factory value (pF): 3000; SAMS schematic (pF): 3000; SAMS table (pF): 3000; SAMS stated VDC: Not specified; SAMS stated tolerance: ±5%; Type supported by substitute: Silvered mica; Check ID: K-MICA; Factory / SAMS differences: Values agree; Selection / missing specifications: 3000 pF ±5%, stable low-loss RF capacitor. 500 VDC is established for the listed mica substitute.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C27** — row: Capacitors: C27; Function: AM oscillator padder; Factory value (pF): 350; SAMS schematic (pF): 350; SAMS table (pF): 350; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Both drawings 350 pF. Sprague 5GA-T35 is 350 pF; Aerovox DI-330 and other listed numbers differ.; Selection / missing specifications: Retain 350 pF for original specification. Oscillator padder affects tracking; do not silently round to 330 pF.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C22B** — row: Capacitors: C22B; Function: SW oscillator trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Capacitance range, dielectric, Q, travel and mounting unspecified. Part of band alignment; do not select by appearance.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C23B** — row: Capacitors: C23B; Function: AM oscillator trimmer; Factory value (pF): Not specified; SAMS schematic (pF): Not specified; SAMS table (pF): Not specified; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Adjustable trimmer; Check ID: No catalog substitute; Factory / SAMS differences: Values agree; Selection / missing specifications: Capacitance range, dielectric, Q, travel and mounting unspecified. Part of band alignment; do not select by appearance.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **L9** — row: Coils and other parts: L9; Category: Magnetic component; Function: SW oscillator; Factory specification: Tapped oscillator coil; SAMS additional specification: Winding DCR 0.3 Ω and 0.1 Ω sections; Catalog check ID: No verified substitute; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L8** — row: Coils and other parts: L8; Category: Magnetic component; Function: AM oscillator; Factory specification: Tapped oscillator coil; SAMS additional specification: Winding DCR 4.6 Ω and 0.9 Ω sections; Catalog check ID: K-AMOSC; Selection / missing specifications: Inductance range, Q, self-resonance, current rating, wire gauge, core, geometry and mounting not fully specified.; Differences / context: SAMS: disregard extra tap on Merit/Miller/Stancor substitutes. Refer to physical terminal guide; redraw uses logical terminals.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **S1F** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **S1G** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables

## 19. Tube radio — Audio amplifier (V6 12AV6, V7 50C5), tone switch, original output transformer and speakers

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in AUDIO AMPLIFIER; TONE SWITCH; ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| V6@210.82,140.97 (unit 1) + V6@60.96,172.72 (unit 2) | 12AV6 duplex-diode triode. Signal unit in AUDIO AMPLIFIER: triode grid G #1 on AUDIO_GRID, plate P #7 on AUDIO_PLATE, cathode K #2 on CHASSIS, diode plates D1 #5 and D2 #6 on AM_DIODE. Heater unit on the series heater string: H #3 on H48, H #4 on H60. Pin assignments as drawn checked on the chassis. | 12AV6 | fitted | — | the tube radio sheet; the author |
| V7@265.43,139.70 (unit 1) + V7@35.56,172.72 (unit 2) | 50C5 beam power output tube. Signal unit in AUDIO AMPLIFIER: plate P #7 on OUTPUT_PLATE, screen G2 #6 on B115_A, control grid G1 #2 on OUTPUT_GRID, cathode K #1 on OUTPUT_K. Nothing on pin 5. Heater unit: H #4 on AC_FUSE_OUT, H #3 on H72. Drawn as fitted; pins checked on the chassis. | 50C5 | fitted | — | the tube radio sheet; the author |
| R1 (tube radio) | Volume control potentiometer: pin 1 (top) on VOLUME_IN, pin 2 (wiper) on VOLUME_W, pin 3 (bottom) on CHASSIS. R1 is this part's reference only: the colon's 1.5kΩ variable resistor is R99 since 2026-09-23 (SCE2, done). | 500kΩ | fitted | — | the tube radio sheet; the author; SCE2 |
| C38 | Coupling capacitor, volume wiper (VOLUME_W) to 12AV6 grid (AUDIO_GRID) | 0.01uF | fitted | — | the tube radio sheet; the author |
| R25 | 12AV6 grid leak, AUDIO_GRID to CHASSIS | 2MΩ | fitted | — | the tube radio sheet; the author |
| R26 | 12AV6 plate load, B115_A to AUDIO_PLATE | 250kΩ | fitted | — | the tube radio sheet; the author |
| C39 | Capacitor from AUDIO_PLATE to CHASSIS (Spec Function 'Audio plate HF shunt') | 100pF | fitted | — | the tube radio sheet; the author |
| C42 | Coupling capacitor, AUDIO_PLATE to OUTPUT_GRID | 0.01uF | fitted | — | the tube radio sheet; the author |
| R27 | 50C5 grid leak, OUTPUT_GRID to CHASSIS | 500kΩ | fitted | — | the tube radio sheet; the author |
| R28 | 50C5 cathode resistor, OUTPUT_K to CHASSIS | 150Ω 1W | fitted | — | the tube radio sheet; the author |
| C40 | Tone capacitor, first section: AUDIO_PLATE to TONE_LOW | 0.002uF | fitted | — | the tube radio sheet; the author |
| C41 | Tone capacitor, second section: TONE_LOW to TONE_MID | 0.002uF | fitted | — | the tube radio sheet; the author |
| S2@198.12,185.42 (unit 1) + S2@36.83,212.09 (unit 2) | Power/tone selector M6, one physical switch drawn as two units. Tone unit (TONE SWITCH): COM #C on CHASSIS, HIGH #H no-connect, MID #M on TONE_MID, LOW #L on TONE_LOW (letters as drawn; whether they are logical is not recorded). Power unit (AC supply): IN #AC1 on AC1 with J3.#1, OUT #AC2 touching fuse M1.#1 pin to pin; its note says the physical lug is unconfirmed. | Power-tone M6 | fitted | — | the tube radio sheet; the author |
| T1 (tube radio, Radio:OUTPUT) | Audio output transformer: two-terminal primary, no tap, P on OUTPUT_PLATE and B on B130. Secondary S1/S2 drawn in its original form (SP_H/SP_L). The actual secondary wiring is the main sheet's T1 (SA on RAD_L, SB on RAD_R, the 7.5 Ω 5 W load across). The main sheet's T1 is the same transformer. | Output transformer | fitted | — | the tube radio sheet; the main sheet; the author; §22 |
| C43 | Capacitor across T1's primary, OUTPUT_PLATE to B130 | 0.0047uF | fitted | — | the tube radio sheet; the author |
| SP1 | Loudspeaker, drawn on T1's secondary in series with SP2 (SP_H to SP_MID) in the section the sheet titles ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS | 4 x 6 / 3-4 ohm | not recorded | — | the tube radio sheet; the author |
| SP2 | Loudspeaker, drawn in series with SP1 (SP_MID to SP_L) in the ORIGINAL section | 4 x 6 / 3-4 ohm | not recorded | — | the tube radio sheet; the author |

### Connections

#### AUDIO AMPLIFIER

- **VOLUME_IN (+, by label name)** — The top of the volume control is joined by label name to COM of S1B, the mode switch's 'Audio mode' unit. S1B's SW #5 and AM #6 throws are on AM_AUDIO (with R15.#2, R16.#2, C32.#1, R17.#1); its FM #7 and AFC #8 throws are on FM_AUDIO (with C37.#2, R24.#2). S1B terminal identifiers are logical, not physical lugs.
  - members: R1.1, S1B.COM (#14)
  - source: the tube radio sheet; label names across all drawings; the author
- **VOLUME_W** — The volume control's wiper feeds coupling capacitor C38.
  - members: R1.2 (wiper), C38.#2
  - source: the tube radio sheet
- **AUDIO_GRID** — The 12AV6 triode grid, pin 1, takes the audio from C38 and returns to chassis through R25.
  - members: V6@210.82,140.97.G (#1), C38.#1, R25.#1
  - source: the tube radio sheet; the author
- **AUDIO_PLATE (+, by label name)** — The 12AV6 plate, pin 7, has its load R26, C39 to chassis, coupling capacitor C42 to the 50C5 grid, and tone capacitor C40.
  - members: V6@210.82,140.97.P (#7), R26.#1, C39.#1, C42.#2, C40.#2
  - source: the tube radio sheet; the author
- **B115_A (by label name)** — The 12AV6 plate load and the 50C5 screen, pin 6, are fed from B115_A. That net also has R13 (AM IF/screen decoupling), the second B+ filter dropper R31, filter capacitor C1C (one of three separate Rubycon 47 µF 315 V capacitors) and S1D's common. The name is a SAMS nominal identifier, not a measured voltage.
  - members: R26.#2, V7@265.43,139.70.G2 (#6), R13.#1, R31.#2, C1C.#1, S1D.COM
  - source: label names across all drawings; the tube radio sheet; the author
- **AM_DIODE (by label name)** — Both 12AV6 diode plates, pins 5 and 6, are tied by labels on the pins to one secondary terminal of AM IF transformer L13. L13's other secondary terminal is on AM_DET_LOAD with C31.#2 and R15.#1. The diodes share cathode pin 2 with the triode. L13's terminal names are logical.
  - members: V6@210.82,140.97.D1 (#5), V6@210.82,140.97.D2 (#6), L13.SEC (logical terminal)
  - source: label names across all drawings; the tube radio sheet; the author
- **OUTPUT_GRID** — The 50C5 control grid, pin 2, takes the audio through C42 and returns to chassis through R27. Nothing is on pin 5.
  - members: V7@265.43,139.70.G1 (#2), C42.#1, R27.#1
  - source: the tube radio sheet; the author
- **OUTPUT_K** — The 50C5 cathode, pin 1, goes to chassis through R28 (150Ω 1W). Nothing else is on this net.
  - members: V7@265.43,139.70.K (#1), R28.#1
  - source: the tube radio sheet; the author

#### TONE SWITCH

- **TONE_LOW (by label name)** — C40's far end and one end of C41 go to S2's LOW throw. With S2's common on chassis, LOW puts C40 alone from the 12AV6 plate to chassis ('L=2n').
  - members: C40.#1, C41.#2, S2@198.12,185.42.LOW (#L)
  - source: the tube radio sheet
- **TONE_MID (+, by label name)** — C41's other end goes to S2's MID throw: MID puts C40 and C41 in series from the plate to chassis ('M=1n (two 2n in series)').
  - members: C41.#1, S2@198.12,185.42.MID (#M)
  - source: the tube radio sheet
- **unnamed net** — S2's HIGH throw has a no-connect flag, so in HIGH no tone capacitor is switched in ('H=open'). It is the only flagged pin in T7; no T7 pin is left unconnected without a flag.
  - members: S2@198.12,185.42.HIGH (#H) — no-connect
  - source: the tube radio sheet

#### ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS

- **B130 (+, by label name)** — T1 primary terminal B and the other end of C43 go to B130, between the first (R30) and second (R31) B+ filter droppers, at filter capacitor C1B (a Rubycon 47 µF 315 V). C43 (0.0047 µF) sits directly across the two-terminal primary. B130 is a SAMS nominal name.
  - members: T1.PRI (#B), C43.#2, R30.#2, R31.#1, C1B.#1
  - source: the tube radio sheet; the author
- **SP_H** — As drawn in the section the sheet calls ORIGINAL: secondary terminal S1 to SP1 pin 1. The sheet note says the secondaries' actual form is the main sheet's. SP1 is not stated as fitted or absent.
  - members: T1.SEC (#S1), SP1.#1
  - source: the tube radio sheet; the author
- **SP_MID** — As drawn in the ORIGINAL section: SP1 and SP2 in series. Not stated as fitted or absent.
  - members: SP1.#2, SP2.#1
  - source: the tube radio sheet; the author
- **SP_L** — As drawn in the ORIGINAL section: secondary terminal S2 to SP2 pin 2. The actual secondary form is the main sheet's.
  - members: T1.SEC (#S2), SP2.#2
  - source: the tube radio sheet; the author

#### Across its sections

- **OUTPUT_PLATE (+, by label name)** — The 50C5 plate, pin 7, goes to output transformer primary terminal P and to one end of C43.
  - members: V7@265.43,139.70.P (#7), T1.PRI (#P), C43.#1
  - source: the tube radio sheet; the author
- **CHASSIS (all CHASSIS nets; J4.#1's net also named AC2)** — T7 returns to CHASSIS at the bottom of the volume control, both grid leaks, C39, the 12AV6 cathode, the 50C5 cathode resistor and the tone switch common. CHASSIS is the same conductor as AC2 on J4 and is used only on the tube radio sheet. CHASSIS is on neutral: J4 takes the neutral (AC_N) through the external EMI filter, with no isolation transformer. CHASSIS is never bonded to Earth or the Faraday cages; the coax shield (COAX_SHIELD_PE) is on Earth and reaches CHASSIS only through C51.
  - members: R1.3, R25.#2, C39.#2, V6@210.82,140.97.K (#2), R27.#2, R28.#2, S2@198.12,185.42.COM (#C), J4.#1 (AC2 = CHASSIS), and every other CHASSIS pin of the tube radio sheet
  - source: label names across all drawings; the tube radio sheet; the author
- **H48** — 12AV6 heater pin 3 is in series with a V4 heater pin. The string order as drawn (V7, V5, V6, V4, V3, L16, V1, L15, V2) was checked on the chassis; the two 12DT8s were found interchanged on 2026-09-17.
  - members: V6@60.96,172.72.H (#3), V4@73.66,172.72.H
  - source: the tube radio sheet; the author
- **H60** — 12AV6 heater pin 4 is in series with a V5 heater pin.
  - members: V6@60.96,172.72.H (#4), V5@48.26,172.72.H
  - source: the tube radio sheet; the author
- **H72** — 50C5 heater pin 3 is in series with V5's other heater pin.
  - members: V7@35.56,172.72.H (#3), V5@48.26,172.72.H
  - source: the tube radio sheet; the author
- **AC_FUSE_OUT (by label name)** — 50C5 heater pin 4, the first tube in the heater string, is on the fuse output node with C44 (X1Y2 rated), R29 and R32. R32 is fitted; R29 is in the AC supply section, which is drawn as fitted.
  - members: V7@35.56,172.72.H (#4), M1.#2, C44.#1, R29.#1, R32.#1
  - source: the tube radio sheet; the author
- **AC1** — The power pole of the same physical switch S2 takes AC1 from J3 ('AC conductor LIVE'), which receives the switched live (AC_L) through the external EMI filter.
  - members: S2@36.83,212.09.IN (#AC1), J3.#1
  - source: the tube radio sheet; the author
- **unnamed net** — The power pole's output touches fuse M1 pin 1 directly, pin to pin, with no wire. The part note says the physical lug is unconfirmed.
  - members: S2@36.83,212.09.OUT (#AC2), M1.#1
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (179.83,193.80), TONE SWITCH: "Tone in BOTH sources: H=open; M=1n (two 2n in series); L=2n. / SAMS M6: 13=common; 2=OFF; 3=H; 4=M; 5=L." — Factory and SAMS agree on the tone network: HIGH switches nothing in, MID puts C40 and C41 (2 nF each) in series, LOW switches C40 alone. The drawn connectivity matches. The second line gives the SAMS lug numbers of switch M6; the drawn symbol uses letters C/H/M/L, and no record maps those letters to lugs. (source: the tube radio sheet)
- (222.50,194.06), ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS: "The output transformer's secondaries are shown here in their original form. / Their actual form is now what is represented in the Main Sheet." — The secondary wiring to SP1/SP2 on this sheet is the original circuit. The actual secondary is the main sheet's T1: SA on RAD_L, SB on RAD_R, with the 7.5 Ω 5 W load across them, to J51's ring and tip (interchangeable). The primary is as drawn on this sheet, two terminals, no tap. SP1/SP2 are not stated as fitted or absent. (source: the tube radio sheet; the main sheet; the author)
- (221.49,166.37), section title: "ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS" — Section title for T1, C43, SP1 and SP2. The section's note limits the stated difference from hardware to the secondaries. (source: the tube radio sheet)
- (170.43,127.00), section title: "AUDIO AMPLIFIER" — Section title for the 12AV6 audio stage and the 50C5 output stage. (source: the tube radio sheet)
- (166.88,166.37), section title: "TONE SWITCH" — Section title for S2 unit 1 and tone capacitors C40, C41. (source: the tube radio sheet)
- T1 (248.92,182.88), part property Transcription_note: "Factory: two-terminal primary. Do not assume actual transformer matches without tracing." — Answered: the fitted T1 matches the sheet — no tap, just a primary and a secondary. (source: the tube radio sheet; the author)
- (245.36,59.44), AM IF / detector area, outside T7's sections: "AM_DIODE goes to V6 pins 5 and 6. / The complete 12AV6 is on the audio sheet; / its triode and diodes share cathode pin 2." — Points to V6's diode plates, pins 5 and 6, in this sheet's AUDIO AMPLIFIER section; triode and diodes share cathode pin 2, which is on CHASSIS. The 'audio sheet' wording stays as written. Pins as drawn were checked on the chassis. (source: the tube radio sheet; the author)
- (39.62,222.76), ORIGINAL TRANSFORMERLESS AC SUPPLY, outside T7's sections (bears on T7's CHASSIS, B115_A, B130): "CHASSIS is circuit common, directly mains referenced in these source drawings. / AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet. / Voltages in net names are SAMS nominal identifiers, not guaranteed measured values. / L18 and L17 were removed by accident, replaced with the external EMI filter." — For T7: B115_A and B130 are SAMS nominal names, not measured voltages, and CHASSIS is the common return. In the radio there is no isolation transformer; CHASSIS is on neutral (J4 from AC_N, J3 from switched live AC_L, both through the external EMI filter) and is never bonded to Earth or the Faraday cages. L17 and L18 are gone from the hardware (a T9 matter). (source: the tube radio sheet; the author)
- (52.32,49.28), FM area, outside T7's sections (bears on V6/V7 pin mapping): "VC1 and VC2 are mechanically ganged with VC3/VC4. / Factory AFC series C11 and oscillator feedback differ from SAMS. / Unspecified coil values and factory tube section-to-pin mapping need chassis verification." — The tube section-to-pin verification this note asks for is done: pins and heater order as drawn were checked on the chassis. Only the last line bears on T7. (source: the tube radio sheet; the author; §22)

### Fitted value against the original specification

- **C43** fitted `0.0047uF` — Fitted 4.7 nF; the factory and SAMS originals were 5 nF, rated 450 VDC by SAMS. No fitted voltage rating is recorded. Original: Spec Factory value (pF): 5000 | Spec SAMS schematic (pF): 5000 | Spec SAMS table (pF): 5000 | Spec SAMS stated VDC: 450 | Spec Selection / missing specifications: 5000 pF, ≥450 VDC per SAMS. Also assess repetitive pulse and transient rating. 4700 pF fitted value is a separate chassis observation. (source: the tube radio sheet; the author)
- **T1 (tube radio)** fitted `Output transformer (fitted: two-terminal primary, no tap)` — The fitted transformer matches the factory form (primary and secondary, no tap). SAMS's tapped primary, its unused section and its DCR figures describe the SAMS original, not the fitted part. The Value carries no electrical data. Original: Spec Factory specification: Single-ended, two-terminal primary; two series speakers | Spec SAMS additional specification: OEM 38.11.1; 7000 Ω total primary, tap at 2500 Ω; 6–8 Ω secondary. DCR: active primary 176 Ω, unused section 146 Ω, secondary 0.9 Ω. | Spec Differences / context: Factory has no unused primary section. Do not apply full-primary 7 kΩ blindly to the SAMS active tap. (source: the tube radio sheet; the author)

### Where the tube radio meets Ambersong

- T1 secondary (drawn in original form on the tube sheet: S1 on SP_H, S2 on SP_L) ↔ T1 (Device:Transformer_Audio, 'TUBE RADIO TRANSFORMER AUDIO OUTPUT', 112.57,84.90, block 'RADIO TRANSFO / AUDIO OUTPUT'): SA on RAD_L, SB on RAD_R, with part '7.5Ω' (Value '5W7.5Ω') across them, #1 on RAD_L and #2 on RAD_R: The same output transformer is drawn on both sheets. Its actual secondary is the main sheet's: the two secondary leads carry RAD_L and RAD_R with the 7.5 Ω 5 W load across them (marked 5W7Ω5J; it replaced an 8 Ω 6 W part). Which lead is which does not matter: tip and ring are interchangeable. (source: the tube radio sheet; the main sheet; the author; §22)
- (reached through the main-sheet T1 secondary) ↔ RAD_L to J51.#R (ord 5, cable side) and RAD_R to J51.#T (ord 6, cable side), J51 in block 'TUBE RADIO AUDIO JACK IN / INTO PCM1802'; RAD_L/RAD_R also on the cable sheet J50.#R/#T and the PCM1802 ADC sheet RADIO_TO_ADC.#R/#T: The transformer's secondary signal leaves on RAD_L and RAD_R to the radio jack's ring and tip (interchangeable) and on through the cable to the ADC sheet's RADIO_TO_ADC. (source: the main sheet; label names across all drawings; the author)
- T1 primary: P on OUTPUT_PLATE (V7 pin 7, C43.#1); B on B130 (C43.#2, R30.#2, R31.#1, C1B.#1); two terminals, no tap ↔ T1 AA and AB both on the global label '50C5' at (107.49,84.90); '50C5' is used on no other sheet: The main sheet's '50C5' label on T1's primary stands for the tube radio's 50C5 output stage. The primary's real connections are the tube sheet's: 50C5 plate to P, B130 to B, C43 across. (source: the main sheet; label names across all drawings; the tube radio sheet; the author; §22)
- The whole tube radio sheet, including every T7 label ↔ Sub-sheet box drawn "Lloyd's TM-838M Tube Radio" -> Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch at (119.38,151.13), pins 0: The tube radio sheet has no sheet pins, and no T7 label name appears on any other sheet, so T7 has no KiCad electrical connection to the Ambersong sheets; the link is T1 (sheet note and shared reference). The box reads TM-838N since 2026-09-23 (SCD4, main-sheet row, done). (source: the main sheet; label names across all drawings (T7 labels, tube sheet only); SCD4)
- CHASSIS (T7's common return: volume pot bottom, grid leaks, C39, 12AV6 cathode, R28, tone switch common), same conductor as AC2 on J4 ↔ Mains: J3 fed from the switched live (AC_L) and J4 from the neutral (AC_N), both through the external EMI filter; Earth and the Faraday cages: There is no isolation transformer, so T7's CHASSIS is on mains neutral. CHASSIS is never bonded to Earth or the Faraday cages; the coax shield COAX_SHIELD_PE is on Earth and reaches CHASSIS only through C51. (source: label names across all drawings; the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **R1** — row: Resistors: R1; Function: Volume control; Factory resistance (Ω): 500000; SAMS resistance (Ω): 500000; SAMS stated power (W): 1/2 W or less (controls heading); Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Potentiometer; audio taper inferred from substitutes; Catalog check ID: K-POT; Differences: Values agree; Selection / missing specifications: 500 kΩ audio-taper replacement supported by A47 family/suffix; 1/2 W candidate. Measure shaft, flat, bushing and rotation; no power switch on this control in drawings.; Source: Factory JPG; SAMS pp.2,7 schematic; p.5 controls/power resistors
- **C38** — row: Capacitors: C38; Function: Volume wiper-to-audio grid coupling; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: 450; SAMS stated tolerance: Not specified; Type supported by substitute: Film/paper substitute; other listed types differ; Check ID: K-FILM; Factory / SAMS differences: Values agree; Selection / missing specifications: 10000 pF, ≥450 VDC per SAMS; low-leakage coupling capacitor. Listed film substitute is 600 VDC ±10%.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R25** — row: Resistors: R25; Function: Audio triode grid leak; Factory resistance (Ω): 2000000; SAMS resistance (Ω): 2000000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R26** — row: Resistors: R26; Function: Audio triode plate load; Factory resistance (Ω): 250000; SAMS resistance (Ω): 250000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C39** — row: Capacitors: C39; Function: Audio plate HF shunt; Factory value (pF): 100; SAMS schematic (pF): 100; SAMS table (pF): 100; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: General-application ceramic disc; Check ID: K-GA; Factory / SAMS differences: Values agree; Selection / missing specifications: Match nominal value and appropriate RF behavior. Original voltage, tolerance and dimensions are not established by a substitute rating.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C42** — row: Capacitors: C42; Function: Audio plate-to-output grid coupling; Factory value (pF): 10000; SAMS schematic (pF): 10000; SAMS table (pF): 10000; SAMS stated VDC: 450; SAMS stated tolerance: Not specified; Type supported by substitute: Film/paper substitute; other listed types differ; Check ID: K-FILM; Factory / SAMS differences: Values agree; Selection / missing specifications: 10000 pF, ≥450 VDC per SAMS; low-leakage coupling capacitor. Listed film substitute is 600 VDC ±10%.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R27** — row: Resistors: R27; Function: Output grid leak; Factory resistance (Ω): 500000; SAMS resistance (Ω): 500000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Preserve resistance and low parasitic inductance in RF circuits. Original wattage, tolerance, working voltage and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **R28** — row: Resistors: R28; Function: Output cathode bias; Factory resistance (Ω): 150; SAMS resistance (Ω): 150; SAMS stated power (W): 1; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Power/surge resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Use at least the stated wattage with adequate thermal clearance and working/pulse voltage. Original tolerance and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C40** — row: Capacitors: C40; Function: Tone capacitor, first section; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: 2000 pF each. Medium uses two in SERIES (1000 pF equivalent), low one 2000 pF. Match both value and voltage stress.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C41** — row: Capacitors: C41; Function: Tone capacitor, second section; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: 2000 pF each. Medium uses two in SERIES (1000 pF equivalent), low one 2000 pF. Match both value and voltage stress.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **S2** — row: Coils and other parts: M6; Category: Switch; Function: Power and tone selector; Factory specification: OFF / H / M / L, linked power contact; SAMS additional specification: Separate AC power pole and tone-selection contacts.; Catalog check ID: No verified substitute; Selection / missing specifications: Need mains-rated contact/insulation capability, correct contact sequence, shaft and mounting. No part number or contact rating listed.; Differences / context: Prior KiCad reference S2. Medium tone puts C40/C41 in series; low uses C40 only.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **T1** — row: Coils and other parts: T1; Category: Transformer; Function: Audio output; Factory specification: Single-ended, two-terminal primary; two series speakers; SAMS additional specification: OEM 38.11.1; 7000 Ω total primary, tap at 2500 Ω; 6–8 Ω secondary. DCR: active primary 176 Ω, unused section 146 Ω, secondary 0.9 Ω.; Catalog check ID: K-OT; Selection / missing specifications: Preserve active-section impedance ratio, DC-bias capability, audio power and insulation; identify actual winding and mounting.; Differences / context: Factory has no unused primary section. Do not apply full-primary 7 kΩ blindly to the SAMS active tap.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C43** — row: Capacitors: C43; Function: Output transformer primary shunt; Factory value (pF): 5000; SAMS schematic (pF): 5000; SAMS table (pF): 5000; SAMS stated VDC: 450; SAMS stated tolerance: Not specified; Type supported by substitute: Film/paper substitute; other listed types differ; Check ID: K-FILM; Factory / SAMS differences: Values agree; Selection / missing specifications: 5000 pF, ≥450 VDC per SAMS. Also assess repetitive pulse and transient rating. 4700 pF fitted value is a separate chassis observation.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **SP1** — row: Coils and other parts: SP1; Category: Speaker; Function: Audio loudspeaker; Factory specification: Two speakers wired in series; SAMS additional specification: 4 × 6 inch; permanent magnet; 3–4 Ω EACH. Pair 6–8 Ω.; Catalog check ID: No verified substitute; Selection / missing specifications: Match impedance, size, mounting centers/depth, cone clearance and polarity. Power handling and sensitivity not specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **SP2** — row: Coils and other parts: SP2; Category: Speaker; Function: Audio loudspeaker; Factory specification: Two speakers wired in series; SAMS additional specification: 4 × 6 inch; permanent magnet; 3–4 Ω EACH. Pair 6–8 Ω.; Catalog check ID: No verified substitute; Selection / missing specifications: Match impedance, size, mounting centers/depth, cone clearance and polarity. Power handling and sensitivity not specified.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables

## 20. Tube radio — Series heater string (all seven tubes), mode supply and indicator lamps

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in SERIES HEATER STRING - ALL SEVEN TUBES; MODE SUPPLY AND INDICATOR LAMPS on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| V7@35.56,172.72 | Heater unit (unit 2) of the 50C5 output tube V7; first heater in the series string, fed from AC_FUSE_OUT; signal unit V7@265.43,139.70 in AUDIO AMPLIFIER (T7) | 50C5 | fitted | — | the tube radio sheet; the author (pins and order checked on chassis); the author |
| V5@48.26,172.72 | Heater unit (unit 2) of the 12BA6 limiter V5; second in the string; signal unit in 10.7 MHz FM IF PATH (T3) | 12BA6 | fitted | — | the tube radio sheet; the author |
| V6@60.96,172.72 | Heater unit (unit 2) of the 12AV6 detector + audio tube V6; third in the string; signal unit in AUDIO AMPLIFIER (T7) | 12AV6 | fitted | — | the tube radio sheet; the author |
| V4@73.66,172.72 | Heater unit (unit 2) of the 12BA6 first IF V4; fourth in the string; signal unit in 10.7 MHz FM IF PATH (T3) | 12BA6 | fitted | — | the tube radio sheet; the author |
| V3@86.36,172.72 | Heater unit (unit 2) of the 12BE6 converter V3; fifth in the string; signal unit in AM / SW INPUT AND CONVERTER (T6) | 12BE6 | fitted | — | the tube radio sheet; the author |
| L16 | Series RF heater choke between the 12BE6 heater (H24) and the V1 12DT8 heater (H_V2_IN); description 'Inductor, small symbol, RF Choke, 23 turns'; Spec Function 'FM AFC heater choke' | RF Heater Choke | fitted | — | the tube radio sheet; the author |
| V1@27.94,185.42 | Heater and internal-shield unit (unit 3) of the 12DT8 FM RF amp + mixer tube V1; sixth in the string, heater pin 4 on H12; shield pin 9 on CHASSIS; signal units in FM RF / MIXER (T1) | 12DT8 | fitted | — | the tube radio sheet; the author |
| L15 | Series RF heater choke between the V1 heater (H12) and the V2 heater (H_V1_IN); description 'Inductor, small symbol, RF Choke, 23 turns'; Spec Function 'FM RF heater choke' | RF Heater Choke | fitted | — | the tube radio sheet; the author |
| V2@52.07,185.42 | Heater and internal-shield unit (unit 3) of the 12DT8 FM oscillator + AFC tube V2; last in the string, pin 4 the CHASSIS end; shield pin 9 on CHASSIS; signal units in AFC / LOCAL OSCILLATOR (T2) | 12DT8 | fitted | — | the tube radio sheet; the author |
| C47 | Bypass capacitor from heater node H36 to CHASSIS; Spec Function 'Heater-string RF bypass'; description reads 'X1Y2 Ceramic Unpolarized Safety capacitor, small symbol' | 0.0022uF | fitted (0.0022 µF, replacing the 0.002 µF originals) | — | the tube radio sheet; the author |
| C48 | Bypass capacitor from H_V2_IN (L16 #2 / V1 heater pin 5) to CHASSIS; Spec Function 'FM AFC/oscillator heater bypass'; description reads 'Three Lead Ceramic Unpolarized capacitor, small symbol' | 0.002uF | fitted — the original three-lead part, never replaced; the sheet draws 0.002uF since 2026-09-23 (SCH2, done) | — | the author; the tube radio sheet; §22 |
| C52 | Second bypass capacitor at the L15 heater choke, from H12 (L15 #1 / V1 heater pin 4) to CHASSIS; the pair C52 / C49 sits one each side of L15. This is the part that was replaced, the one that had been taken for C48; an X1Y2 safety capacitor was fitted. | 0.0022uF | fitted — an X1Y2 safety capacitor | — | the tube radio sheet; the author |
| C49 | Bypass capacitor from H_V1_IN (L15 #2 / V2 heater pin 5) to CHASSIS; Spec Function 'FM RF/mixer heater bypass'; description reads 'X1Y2 Ceramic Unpolarized Safety capacitor, small symbol' | 0.0022uF | fitted (0.0022 µF, replacing the 0.002 µF originals) | — | the tube radio sheet; the author |
| S1D | B+ mode-selection unit of the single four-position mode switch S1 (SAMS M5; SW / AM / FM / FM-AFC). COM feeds AM_B in SW and AM, FM_B in FM and FM-AFC. Terminal identifiers are logical, not physical lugs. | Mode selector M5 | fitted | — | the tube radio sheet; the author; §22 |
| S1E | Mode-lamp unit of the same mode switch S1. COM (NEON_FEED) goes to NEON_SW in SW, NEON_AM in AM, NEON_FM in FM and FM-AFC. Terminal identifiers are logical. | Mode selector M5 | fitted (the S1E contacts) | — | the tube radio sheet; the author; §22 |
| R32 | Neon lamp current limiter from AC_FUSE_OUT to NEON_FEED | 50kΩ | fitted | — | the tube radio sheet; the author; §22 |
| NE1 | SW mode neon indicator lamp, NEON_SW to CHASSIS | SW Neon | absent from the device | — | the tube radio sheet; the author |
| NE2 | AM mode neon indicator lamp, NEON_AM to CHASSIS | AM Neon | absent from the device | — | the tube radio sheet; the author |
| NE3 | FM mode neon indicator lamp (FM and FM-AFC positions), NEON_FM to CHASSIS | FM Neon | absent from the device | — | the tube radio sheet; the author |

### Connections

#### SERIES HEATER STRING - ALL SEVEN TUBES

- **H72** — The 50C5 heater pin 3 joins the 12BA6 limiter V5 heater pin 4.
  - members: V7@35.56,172.72 H #3, V5@48.26,172.72 H #4
  - source: the tube radio sheet; the author
- **H60** — V5 heater pin 3 joins the 12AV6 V6 heater pin 4.
  - members: V5@48.26,172.72 H #3, V6@60.96,172.72 H #4
  - source: the tube radio sheet; the author
- **H48** — V6 heater pin 3 joins the 12BA6 first IF V4 heater pin 4.
  - members: V6@60.96,172.72 H #3, V4@73.66,172.72 H #4
  - source: the tube radio sheet; the author
- **H36** — V4 heater pin 3 joins the 12BE6 V3 heater pin 4. C47 (0.0022 µF) bypasses this node to CHASSIS. Two drawn nets are joined by the label H36.
  - members: V4@73.66,172.72 H #3, V3@86.36,172.72 H #4, C47 #1
  - source: the tube radio sheet; the author
- **H24** — V3 heater pin 3 goes to RF heater choke L16. Two drawn nets are joined by the label H24.
  - members: V3@86.36,172.72 H #3, L16 #1
  - source: the tube radio sheet; the author
- **H_V2_IN** — L16's other end feeds the 12DT8 V1 heater pin 5. C48 (0.002 µF) bypasses this node (V1's pin-5 side) to CHASSIS.
  - members: L16 #2, C48 #1, V1@27.94,185.42 H #5
  - source: the tube radio sheet; the author; §22
- **H12** — V1 heater pin 4 goes to RF heater choke L15, and C52 0.0022 µF (X1Y2) bypasses this node to CHASSIS.
  - members: V1@27.94,185.42 H #4, L15 #1, C52 #1
  - source: the tube radio sheet; the author
- **H_V1_IN** — L15's other end feeds the 12DT8 V2 heater pin 5. C49 (0.0022 µF) bypasses this node to CHASSIS.
  - members: L15 #2, C49 #1, V2@52.07,185.42 H #5
  - source: the tube radio sheet; the author

#### MODE SUPPLY AND INDICATOR LAMPS

- **NEON_FEED** — The limited feed from R32 goes to the common of mode-lamp unit S1E. Both are fitted.
  - members: R32 #2, S1E COM (#15, logical identifier)
  - source: the tube radio sheet; the author
- **NEON_SW** — In the SW position S1E connects NEON_FEED to NEON_SW, drawn to NE1. The S1E contact is fitted; the NE1 bulb is absent.
  - members: S1E SW (#9, logical), NE1 #1 (bulb absent)
  - source: the tube radio sheet; the author
- **NEON_AM** — In the AM position S1E connects NEON_FEED to NEON_AM, drawn to NE2. The S1E contact is fitted; the NE2 bulb is absent.
  - members: S1E AM (#10, logical), NE2 #1 (bulb absent)
  - source: the tube radio sheet; the author
- **NEON_FM** — In the FM and FM-AFC positions S1E connects NEON_FEED to NEON_FM, drawn to NE3. The S1E contacts are fitted; the NE3 bulb is absent.
  - members: S1E FM (#11, logical), S1E AFC (#12, logical), NE3 #1 (bulb absent)
  - source: the tube radio sheet; the author
- **B115_A** — The B115_A rail (R31 / C1C node) feeds the common of mode unit S1D, and directly R13, R26 and the 50C5 screen.
  - members: S1D COM (#14, logical), R31 #2 (300Ω 1W second B+ filter dropper, outside group), C1C #1 (47uF B+ filter section; one of three Rubycon 47 µF 315 V, outside group), R13 #1 (1kΩ AM IF/screen decoupling, outside group), R26 #2 (250kΩ audio triode plate load, outside group), V7@265.43,139.70 G2 (#6, 50C5 screen, AUDIO AMPLIFIER T7)
  - source: the tube radio sheet; the author
- **AM_B** — In the SW and AM positions S1D connects B115_A to AM_B, which feeds the first AM IF transformer primary and the 12BE6 screen grids.
  - members: S1D SW (#5, logical), S1D AM (#6, logical), L11 PRI (#P, logical; 'AM IF A - 455kHz', outside group), V3@124.46,115.57 G2/G4 (#6, 12BE6 screens, T6)
  - source: the tube radio sheet; the author
- **FM_B** — In the FM and FM-AFC positions S1D connects B115_A to FM_B, which feeds the FM RF plate choke and bypass, the first FM IF decoupling, the AFC plate feed and the ratio-detector decoupling.
  - members: S1D FM (#7, logical), S1D AFC (#8, logical), L3 #2 (3uH FM RF plate choke, outside group), C5 #2 (0.002uF FM RF supply bypass, outside group), R4 #1 (1kΩ first FM IF supply decoupling, outside group), R8 #2 (1kΩ AFC plate feed, outside group), R18 #1 (1kΩ ratio-detector supply decoupling, outside group)
  - source: the tube radio sheet; the author
- **(mode switch mechanics)** — S1D and S1E are two of the seven drawn units of one mode switch S1. All units rotate together through SW / AM / FM / FM-AFC. Every S1 terminal number is a logical identifier, not a physical lug.
  - members: S1D, S1E, S1A, S1B, S1C, S1F, S1G (elsewhere)
  - source: the tube radio sheet; the author; §22

#### Across its sections

- **AC_FUSE_OUT** — The fused output of M1 feeds the top of the heater string (50C5 V7 heater pin 4), the neon limiter R32, R29 and C44. M1's other end meets S2 unit 2 (Power switch) OUT pin to pin. S2 unit 2 IN is on AC1 with J3 'AC conductor LIVE'. In the radio J3 takes the switched live (AC_L) through the external EMI filter, with no isolation transformer.
  - members: V7@35.56,172.72 H #4, R32 #1, M1 #2 (0.75A fuse, outside group), R29 #1 (28Ω 3W rectifier surge limiter, outside group), C44 #1 (0.01uF switched-line-to-chassis bypass, X1Y2 rated, outside group)
  - source: the tube radio sheet; the author
- **CHASSIS (same net as AC2)** — The bottom of the heater string (V2 heater pin 4, since the two 12DT8s are interchanged there), both 12DT8 shield pins 9 and the three heater bypass capacitors return to CHASSIS. The lamps' returns are drawn there too, but the bulbs are absent. CHASSIS is the sheet's circuit common, drawn as one net with AC2 / J4. In the radio J4 takes the neutral (AC_N) through the external EMI filter, with no isolation transformer, so CHASSIS is on neutral. COAX_SHIELD_PE is on Earth and reaches CHASSIS only through C51. CHASSIS is never bonded to Earth or the Faraday cages.
  - members: V2@52.07,185.42 H #4, V1@27.94,185.42 SH #9, V2@52.07,185.42 SH #9, C47 #2, C48 #2, C49 #2, C52 #2, NE1 #2 (drawn; bulb absent), NE2 #2 (drawn; bulb absent), NE3 #2 (drawn; bulb absent), J4 #1 'AC conductor NEUTRAL' (via AC2), plus the sheet's other CHASSIS members
  - source: the tube radio sheet; the author

### Notes written on the tube radio sheet (verbatim)

- (37.59,161.29), title of SERIES HEATER STRING - ALL SEVEN TUBES: "SERIES HEATER STRING - ALL SEVEN TUBES" — Section title: all seven tubes' heaters are drawn here in one series chain. The order and the heater pins as drawn were checked on the chassis. (source: the tube radio sheet; the author)
- (125.73,180.85), title of MODE SUPPLY AND INDICATOR LAMPS: "MODE SUPPLY AND INDICATOR LAMPS" — Section title: B+ mode selection (S1D) and the mode-lamp circuit (R32, S1E, NE1–NE3). (source: the tube radio sheet)
- (139.19,224.79), MODE SUPPLY AND INDICATOR LAMPS, below NE1–NE3 (' / ' marks the line break): "Indicator lights are absent from the final device, they're  / shown here as a representation of the original schematics." — Of the lamp parts, only the neon bulbs NE1, NE2 and NE3 are absent from the radio; R32 and the S1E contacts are fitted. (source: the tube radio sheet; the author)
- (120.14,173.99), inside SW / AM LOCAL OSCILLATOR (T6), just above the MODE SUPPLY AND INDICATOR LAMPS rectangles; outside this group's sections: "All S1 units rotate together: SW / AM / FM / FM-AFC. / VC1, VC2, VC3 and VC4 are one ganged tuning assembly. / Logical switch and coil terminals require a physical lug crosscheck." — S1D and S1E belong to one four-position mode switch whose units move together. VC1–VC4 are one ganged tuning assembly. The switch and coil terminal identifiers are logical, not physical lugs. No physical lug mapping is recorded. (source: the tube radio sheet; the author)
- (39.62,222.76), inside ORIGINAL TRANSFORMERLESS AC SUPPLY (T9); outside this group's sections: "CHASSIS is circuit common, directly mains referenced in these source drawings. / AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet. / Voltages in net names are SAMS nominal identifiers, not guaranteed measured values. / L18 and L17 were removed by accident, replaced with the external EMI filter." — CHASSIS, where this group returns, is the sheet's circuit common. In the radio J3 takes the switched live (AC_L) and J4 the neutral (AC_N), through the external EMI filter, with no isolation transformer, so CHASSIS is on neutral and never bonded to Earth or the Faraday cages. Voltages in net names are SAMS nominal identifiers, not measurements; the note does not say which nets it means. L17 and L18 are gone, replaced by the external EMI filter; the AC supply section is drawn as fitted. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **C47** fitted `0.0022uF` — Fitted 0.0022 µF (2200 pF). Both original drawings give 2000 pF, and the originals were replaced by 0.0022 µF parts. The Spec gives no voltage rating. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 2000 | Spec SAMS table (pF): 2000 | Spec SAMS stated VDC: Not specified | Spec Type supported by substitute: High-K bypass ceramic | Spec Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence. (source: the tube radio sheet; the author)
- **C48** fitted `0.002uF` — Fitted 0.002 µF, the original three-lead part, not replaced; both original drawings give 2000 pF. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 2000 | Spec SAMS table (pF): 2000 | Spec SAMS stated VDC: Not specified | Spec Type supported by substitute: High-K bypass ceramic | Spec Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence. (source: the tube radio sheet; the author)
- **C49** fitted `0.0022uF` — Fitted 0.0022 µF; both original drawings give 2000 pF; replaced by 0.0022 µF parts. Original: Spec Factory value (pF): 2000 | Spec SAMS schematic (pF): 2000 | Spec SAMS table (pF): 2000 | Spec SAMS stated VDC: Not specified | Spec Type supported by substitute: High-K bypass ceramic | Spec Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence. (source: the tube radio sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **L16** — row: Coils and other parts: L16; Category: Magnetic component; Function: FM AFC heater choke; Factory specification: Series heater RF choke; SAMS additional specification: 23 turns; Catalog check ID: No verified substitute; Selection / missing specifications: 23 turns alone cannot define a replacement. Need winding diameter, length, wire gauge, core, current and insulation rating.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **L15** — row: Coils and other parts: L15; Category: Magnetic component; Function: FM RF heater choke; Factory specification: Series heater RF choke; SAMS additional specification: 23 turns; Catalog check ID: No verified substitute; Selection / missing specifications: 23 turns alone cannot define a replacement. Need winding diameter, length, wire gauge, core, current and insulation rating.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C47** — row: Capacitors: C47; Function: Heater-string RF bypass; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C48** — row: Capacitors: C48; Function: FM AFC/oscillator heater bypass; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C52** — row: Capacitors: C52; Function: Heater bypass
- **C49** — row: Capacitors: C49; Function: FM RF/mixer heater bypass; Factory value (pF): 2000; SAMS schematic (pF): 2000; SAMS table (pF): 2000; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: High-K bypass ceramic; Check ID: K-HK; Factory / SAMS differences: Values agree; Selection / missing specifications: Drawings: 2000 pF. Owner reports 2200 pF originals/fitted parts. Match RF bypass construction and layout; safety marking alone does not prove RF equivalence.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **S1D** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **S1E** — row: Coils and other parts: M5; Category: Switch; Function: SW / AM / FM / FM-AFC selector; Factory specification: Mechanically linked band, supply, audio, AFC and neon contacts; SAMS additional specification: Four positions; three physical sections/wafer terminal guides.; Catalog check ID: No verified substitute; Selection / missing specifications: Match complete contact truth table, shaft/detents, insulation and current ratings. No commercial substitute listed.; Differences / context: Prior KiCad reference S1 has seven functional units; those are not seven physical switches.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **R32** — row: Resistors: R32; Function: Neon lamp current limiter; Factory resistance (Ω): 50000; SAMS resistance (Ω): 50000; SAMS stated power (W): Not specified; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Fixed resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: 50 kΩ series lamp limiter. Original wattage not printed; establish lamp voltage/current and resistor dissipation before purchase.; Source: Factory JPG; SAMS pp.2,7 schematic
- **NE1** — row: Coils and other parts: NE1; Category: Indicator; Function: Mode lamp SW; Factory specification: N.L. neon lamp symbol; SAMS additional specification: Indicator light with shared R32 50 kΩ series limiter.; Catalog check ID: No verified substitute; Selection / missing specifications: Original part, striking/sustaining voltage, operating current, dimensions and brightness unknown. No lamp model supplied.; Differences / context: NE references newly assigned; three lamps appear in both originals.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **NE2** — row: Coils and other parts: NE2; Category: Indicator; Function: Mode lamp AM; Factory specification: N.L. neon lamp symbol; SAMS additional specification: Indicator light with shared R32 50 kΩ series limiter.; Catalog check ID: No verified substitute; Selection / missing specifications: Original part, striking/sustaining voltage, operating current, dimensions and brightness unknown. No lamp model supplied.; Differences / context: NE references newly assigned; three lamps appear in both originals.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **NE3** — row: Coils and other parts: NE3; Category: Indicator; Function: Mode lamp FM; Factory specification: N.L. neon lamp symbol; SAMS additional specification: Indicator light with shared R32 50 kΩ series limiter.; Catalog check ID: No verified substitute; Selection / missing specifications: Original part, striking/sustaining voltage, operating current, dimensions and brightness unknown. No lamp model supplied.; Differences / context: NE references newly assigned; three lamps appear in both originals.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables

## 21. Tube radio — Original transformerless AC supply, and where the tube radio meets Ambersong (main sheet)

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted; a reader's set will differ.*

Drawn in ORIGINAL TRANSFORMERLESS AC SUPPLY on the tube radio sheet. The sheet reflects the actual hardware except where its own notes
say otherwise. A part's Value is its fitted state; its hidden Spec fields hold the ORIGINAL factory/SAMS specifications
for it, with notes on substitutes and selection. They are not the fitted state, though some notes mention it, for example
C43, C46, C47–C49 and M1. The terminal identifiers of L5, L6, L8–L14 and S1A–S1G are logical, not physical lugs.

### Parts

| Part | What it is | Value (fitted) | In the device | Rating | Source |
|---|---|---|---|---|---|
| J3 | Mains input terminal of the radio, live conductor (net AC1). In the radio it takes the switched live AC_L through the external EMI filter. | AC conductor LIVE | fitted | — | the tube radio sheet; the author |
| J4 | Mains input terminal of the radio, neutral conductor. Its net carries both AC2 and CHASSIS, so CHASSIS is on neutral; it takes AC_N through the external EMI filter, with no isolation transformer. | AC conductor NEUTRAL | fitted | — | the tube radio sheet; the author |
| S2@36.83,212.09 | Unit 2 of power/tone switch S2: the power contact, IN (#AC1) on AC1 and OUT (#AC2) on the fuse. Unit 1 is in TONE SWITCH. The physical lug is unconfirmed (Transcription_note). | Power-tone M6 | fitted | — | the tube radio sheet; the author |
| M1 | Mains fuse in the switched live, between S2's output and AC_FUSE_OUT (position as fitted) | 0.75A | fitted | — | the tube radio sheet; the author |
| C44 | Capacitor from the fused live (AC_FUSE_OUT) to CHASSIS; replaces SAMS C45, which does not exist in the radio | 0.01uF | fitted | X1Y2 rated | the tube radio sheet; the author |
| R29 | Rectifier surge-limiting resistor, AC_FUSE_OUT to RECT_A | 28Ω 3W | fitted | — | the tube radio sheet; the author |
| X1 | Half-wave silicon rectifier for B+: anode A (#2) on RECT_A, cathode K (#1) on B_RAW; no part number recorded | Silicon Rectifier (SCE3, done 2026-09-23) | fitted | — | the tube radio sheet; SCE3; the author |
| C46 | Capacitor across rectifier X1 (#1 on B_RAW, #2 on RECT_A) | 0.0047uF | fitted | — | the tube radio sheet; the author |
| C1A | B+ reservoir electrolytic on B_RAW (#1), #2 to CHASSIS; one of three separate capacitors | 47uF | fitted | Rubycon 47 µF 315 V | the tube radio sheet; the author |
| R30 | First B+ filter dropping resistor, B_RAW to B130 | 150Ω 5W | fitted | — | the tube radio sheet; the author |
| C1B | B+ filter electrolytic on B130 (#1), #2 to CHASSIS; one of three separate capacitors | 47uF | fitted | Rubycon 47 µF 315 V | the tube radio sheet; the author |
| R31 | Second B+ filter dropping resistor, B130 to B115_A | 300Ω 1W | fitted | — | the tube radio sheet; the author |
| C1C | B+ filter electrolytic on B115_A (#1), #2 to CHASSIS; one of three separate capacitors | 47uF | fitted | Rubycon 47 µF 315 V | the tube radio sheet; the author |

### Connections

#### ORIGINAL TRANSFORMERLESS AC SUPPLY

- **AC1** — The live input terminal goes to the input of S2's power contact. In the radio, J3 is fed from the switched live AC_L through the external EMI filter, which is not drawn. S2's pin identifiers are the symbol's; the physical lug is unconfirmed.
  - members: J3.#1, S2@36.83,212.09.IN (#AC1)
  - source: the tube radio sheet; the author
- **unnamed net** — The power contact's output goes straight into the fuse. The pins touch with no wire, and the file counts that as a connection. OUT is not on the AC2 net.
  - members: S2@36.83,212.09.OUT (#AC2), M1.#1
  - source: the tube radio sheet; the author
- **AC_FUSE_OUT** — The switched, fused live, in three drawn pieces joined by name. It feeds R29, C44 to chassis, the neon-lamp limiter R32 and 50C5 heater pin 4, the first tube of the series heater string (V7, V5, V6, V4, V3, L16, V1, L15, V2; the two 12DT8s interchanged against an earlier reading).
  - members: M1.#2, C44.#1, R29.#1, R32.#1 (MODE SUPPLY AND INDICATOR LAMPS), V7@35.56,172.72.H (#4) (50C5 heater unit, SERIES HEATER STRING - ALL SEVEN TUBES)
  - source: the tube radio sheet; the author
- **RECT_A** — The surge-limited AC reaches the rectifier's anode and one end of C46.
  - members: R29.#2, X1.A (#2), C46.#2
  - source: the tube radio sheet
- **B_RAW** — The rectifier cathode: the reservoir C1A (47 µF 315 V), the other end of C46 (so C46 is across X1), and the first dropper R30. The name appears only here.
  - members: X1.K (#1), C46.#1, C1A.#1, R30.#1
  - source: the tube radio sheet; label names across all drawings; the author
- **B130** — The first filtered B+ node: filter C1B, one terminal of output transformer T1's two-terminal primary, and C43 (0.0047 µF). Two pieces joined by name.
  - members: R30.#2, R31.#1, C1B.#1, T1.PRI (#B) (ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS), C43.#2 (ORIGINAL OUTPUT TRANSFORMER/SERIES SPEAKERS)
  - source: the tube radio sheet; the author
- **B115_A** — The second filtered B+ node: filter C1C, R13, R26, the common of mode-switch unit S1D, and the 50C5 screen grid (pin 6). Five drawn pieces joined by the label name. S1D's terminal identifier COM is logical, not a physical lug.
  - members: R31.#2, C1C.#1, R13.#1 (455 kHz AM IF AND AM DETECTOR), R26.#2 (AUDIO AMPLIFIER), S1D.COM (MODE SUPPLY AND INDICATOR LAMPS), V7@265.43,139.70.G2 (#6) (AUDIO AMPLIFIER)
  - source: the tube radio sheet; the author
- **CHASSIS (= AC2 on J4's net)** — The neutral terminal J4 carries both names AC2 and CHASSIS, so neutral is the radio's circuit common. By name it joins every CHASSIS return on the sheet (78 pins). In this section those are C44 and C1A, C1B and C1C. In the radio CHASSIS is on neutral through the external EMI filter, with no isolation transformer, and is never bonded to Earth or the Faraday cages. The sheet has no Earth symbol. Coil and switch terminal identifiers (L5, L6, L8–L14, S1A) are logical. The NE1–NE3 bulbs are absent from the device.
  - members: J4.#1, C44.#2, C1A.#2, C1B.#2, C1C.#2, C10.#2, C11.#1, C12.#1, C13.#1, R7.#2, C14.#1, C18.#1, C17.#1, VC2.#1, C22A.#1, C22B.#1, C23B.#1, C24.#2, C28.#1, C29.#1, C30.#1, C31.#1, C32.#2, C33.#1, C36.#1, C37.#1, C39.#2, C47.#2, C48.#2, C49.#2, C5.#1, C50.#2, R33.#1, C52.#2, C7.#1, L4.#2, C8.#1, VC1.#1, C9.#1, R3.#2, L10.CAN, L11.CAN, L12.CAN, L13.CAN, L14.CAN, L2.#2, C51.#1, L5.2, L5.4, L6.PRI, L6.SEC, L8.BOTTOM, L9.BOTTOM, NE1.#2, NE2.#2, NE3.#2, R1.3, R11.#2, R12.#2, R14.#2, R16.#1, R20.#2, R22.#1, R25.#2, R27.#2, R28.#2, S1A.COM, S2@198.12,185.42.COM, V2@52.07,185.42.H, V1@27.94,185.42.SH, V1@33.02,31.75.G, V2@52.07,185.42.SH, V4@162.56,31.75.G3, V5@224.79,31.75.G3, V5@224.79,31.75.K, V6@210.82,140.97.K, VC3.#1, VC4.#1
  - source: the tube radio sheet; the author
- **S2 unit 1 contacts** — The tone side of the same switch, drawn in TONE SWITCH (group T7). HIGH is marked no-connect.
  - members: S2@198.12,185.42.HIGH (#H): no-connect, S2@198.12,185.42.COM (#C): CHASSIS, S2@198.12,185.42.MID (#M): TONE_MID, S2@198.12,185.42.LOW (#L): TONE_LOW
  - source: the tube radio sheet

### Notes written on the tube radio sheet (verbatim)

- (50.29,202.18) section title: "ORIGINAL TRANSFORMERLESS AC SUPPLY" — Names the radio's own transformerless mains supply. Despite 'ORIGINAL', the whole section as drawn is the actual hardware, fuse position included: the section was corrected to match the radio. (source: the tube radio sheet; the author)
- (39.62,222.76) note, line 1: "CHASSIS is circuit common, directly mains referenced in these source drawings." — In the radio too, CHASSIS is the circuit common on mains neutral: J4 takes neutral through the external EMI filter, with no isolation transformer. (source: the tube radio sheet; the author)
- (39.62,222.76) note, line 2: "AC terminals are the radio input; external EMI filter, isolation and modern audio electronics are outside this sheet." — J3/J4 are the sheet's boundary. The external EMI filter exists and sits between Ambersong's AC_L/AC_N and J3/J4, under an earthed Faraday cage. There is no isolation transformer. The modern audio electronics are on the main sheet. (source: the tube radio sheet; the author)
- (39.62,222.76) note, line 3: "Voltages in net names are SAMS nominal identifiers, not guaranteed measured values." — The note does not say which net names it means. (source: the tube radio sheet)
- (39.62,222.76) note, line 4: "L18 and L17 were removed by accident, replaced with the external EMI filter." — L17 and L18 are not in the radio and are not drawn. The external EMI filter took their place. No source checked says what L17 and L18 were. (source: the tube radio sheet; §22)
- S2@36.83,212.09 (unit 2), hidden property Transcription_note: "Hardware: switch output to fuse per explicit owner connection report; physical lug unconfirmed." — The switch-output-to-fuse connection is the fitted hardware, part of the section drawn as actual. Which physical lug of S2 carries it is not recorded, and S2 is not among the parts whose terminal identifiers are recorded as logical. (source: the tube radio sheet; the author)

### Fitted value against the original specification

- **C1A** fitted `47uF` — The original was a 40 µF 150 V section of one common-negative can. Fitted is a separate Rubycon 47 µF 315 V capacitor, one of three. Original: Spec Factory value (pF): 40000000 | Spec SAMS schematic (pF): 40000000 | Spec SAMS table (pF): 40000000 | Spec SAMS stated VDC: 150 | Spec Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF. (source: the tube radio sheet; the author)
- **C1B** fitted `47uF` — The original was a 40 µF 150 V can section. Fitted is a separate Rubycon 47 µF 315 V capacitor. Original: Spec Factory value (pF): 40000000 | Spec SAMS schematic (pF): 40000000 | Spec SAMS table (pF): 40000000 | Spec SAMS stated VDC: 150 | Spec Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF. (source: the tube radio sheet; the author)
- **C1C** fitted `47uF` — The original was a 40 µF 150 V can section. Fitted is a separate Rubycon 47 µF 315 V capacitor. Original: Spec Factory value (pF): 40000000 | Spec SAMS schematic (pF): 40000000 | Spec SAMS table (pF): 40000000 | Spec SAMS stated VDC: 150 | Spec Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF. (source: the tube radio sheet; the author)
- **C46** fitted `0.0047uF` — The original was 5000 pF, 450 V per SAMS. Fitted is 0.0047 µF; no fitted voltage rating is recorded. Original: Spec Factory value (pF): 5000 | Spec SAMS schematic (pF): 5000 | Spec SAMS table (pF): 5000 | Spec SAMS stated VDC: 450 | Spec Selection / missing specifications: 5000 pF, ≥450 VDC per SAMS. Also assess repetitive pulse and transient rating. 4700 pF fitted value is a separate chassis observation. (source: the tube radio sheet; the author)
- **M1** fitted `0.75A` — The factory drawing has no fuse. SAMS gives 0.75 A type GJV in a different position. The radio has a 0.75 A fuse between the power switch and AC_FUSE_OUT; its type and voltage are not recorded. Original: Spec Factory specification: Not drawn | Spec SAMS additional specification: 0.75 A; type GJV; omitted in some versions | Spec Selection / missing specifications: Use matching speed, voltage, interrupting capacity and package. Fuse location in owner chassis differs from SAMS. (source: the tube radio sheet; the author)
- **X1** fitted `Silicon Rectifier` — Same device class. The Value read "Silicone" until 2026-09-23 (SCE3, done). No fitted part number or rating is recorded, and 400 V / 0.5 A is only a selection guideline. Original: Spec Factory specification: Silicon rectifier | Spec SAMS additional specification: Measured output current 0.090 A. Device rating not printed. | Spec Selection / missing specifications: Match at least a verified 400 V / 0.5 A historical class, with appropriate surge, heat and leakage ratings. Higher ratings alone do not guarantee identical B+. (source: the tube radio sheet; SCE3)

### Where the tube radio meets Ambersong

- J3.#1 (net AC1, 'AC conductor LIVE') and J4.#1 (net AC2 = CHASSIS, 'AC conductor NEUTRAL') ↔ AC_L (front toggle SW1 pin A; also amp PSU U1.AC+) and AC_N (J30.N; also U1.AC- and U4.AC-). The inlet J30's L is AC_L_IN, which goes to SW1 pin B and the always-on 5 V PSU U4.AC+.: J3 takes the switched live AC_L and J4 takes neutral AC_N, through the external EMI filter, with no isolation transformer, so the radio's CHASSIS is on neutral. The front toggle switches the radio. The EMI filter sits under a Faraday cage bonded to earth. None of this is drawn: no wire, sheet pin or shared label joins the sheets. (source: the author; the main sheet; label names across all drawings)
- CHASSIS (circuit common on neutral) ↔ Earth: FARADAY_CAGE label on Earth inside the 'FARADAY CAGE / OVER RADIO' box (reaches no pin); J30.E; U1.EARTH; note 'The faraday cages over the whole of the tube radio and / over the AS5600 are both connected to EARTH': The Faraday cage over the radio is on Earth. CHASSIS is never bonded to Earth or to the Faraday cages. (source: the author; the main sheet)
- COAX_SHIELD_PE: J53.Ext and C51.#2; C51 0.001uF, #1 on CHASSIS ↔ Earth: The coax shield is on Earth and reaches CHASSIS only through C51, which is X1Y2 rated. The Earth connection of COAX_SHIELD_PE is not drawn on either sheet. (source: the author; the tube radio sheet)
- J53 RG179 Coax: In on FM_ANT (with C3.#2), Ext on COAX_SHIELD_PE ↔ J10 coax connector, block ANTENNA: centre 'In' = ANT, shell 'Ext' = Earth (SC30, drawn that way since 2026-09-23): Both sheets draw a coax antenna connector. No cable or label joining J10 to J53 is drawn or ruled. The FM antenna, as built, is §29. (source: SC30; the author; the tube radio sheet; the main sheet; label names across all drawings)
- T1 output transformer: two-terminal primary (no tap) between OUTPUT_PLATE (V7 plate) and B130; secondaries drawn in original form on SP_H/SP_L. Note: 'Their actual form is now what is represented in the Main Sheet.' ↔ T1 'TUBE RADIO TRANSFORMER AUDIO OUTPUT': AA and AB on the label 50C5 (the radio's output stage); SA on RAD_L, SB on RAD_R; the 7.5 Ω 5 W load (marked 5W7Ω5J; drawn reference text 7.5Ω) across RAD_L–RAD_R; RAD_L to J51 #R (ord 5/6), RAD_R to J51 #T (ord 6/6): One physical transformer, drawn on both sheets. The primary belongs to the radio's 50C5 output stage. The actual secondary is the main sheet's: its two leads carry RAD_L and RAD_R, loaded by the 7.5 Ω 5 W part, to jack J51, where tip and ring are interchangeable. What lies after J51 on the radio input is §6's mono feed into the PCM1802. (source: the author; §22; the tube radio sheet; the main sheet)
- The whole tube radio sheet (file Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch) ↔ Sub-sheet box at (119.38,151.13), 18.4x21.6, pins 0, named 'Lloyd's TM-838N Tube Radio' since 2026-09-23 (SCD4, main-sheet row, done): The box has no sheet pins, and no label name is shared between the tube sheet and any other drawing. None of the meeting points above is drawn as a connection: the AC and earth points are the author's statement of the fitted wiring, T1 is one transformer by the tube sheet's own note, and no cable or label joining J10 to J53 is drawn or recorded. (source: the main sheet; label names across all drawings (script scan: 0 shared names); SCD4; the author; the tube radio sheet)
- None (no tube-sheet net) ↔ U89 RDA5807M ANT on local label 30CM_ANT, in the RDA5807M box next to the tube radio box: 30CM_ANT is a 30 cm wire inside the tube radio's Faraday cage, bent in a U over the FM oscillator section. No connection to the tube sheet is drawn. (source: the main sheet; the author)

### Original specifications (the hidden Spec fields, verbatim)

Copied from the sheet's hidden fields, which hold the ORIGINAL factory/SAMS specifications for each part and notes on
substitutes and selection, compiled by the author in a component list that is not published. They are not the fitted
state, though some notes mention it (for example C43, C46, C47–C49 and M1). Parts with no Spec fields have no line here.

- **S2** — row: Coils and other parts: M6; Category: Switch; Function: Power and tone selector; Factory specification: OFF / H / M / L, linked power contact; SAMS additional specification: Separate AC power pole and tone-selection contacts.; Catalog check ID: No verified substitute; Selection / missing specifications: Need mains-rated contact/insulation capability, correct contact sequence, shaft and mounting. No part number or contact rating listed.; Differences / context: Prior KiCad reference S2. Medium tone puts C40/C41 in series; low uses C40 only.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **M1** — row: Coils and other parts: M1; Category: Fuse; Function: AC input protection; Factory specification: Not drawn; SAMS additional specification: 0.75 A; type GJV; omitted in some versions; Catalog check ID: K-FUSE; Selection / missing specifications: Use matching speed, voltage, interrupting capacity and package. Fuse location in owner chassis differs from SAMS.; Differences / context: No additional source conflict identified; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C44** — row: Capacitors: C44-F; Function: Factory switched-line-to-chassis bypass; Factory value (pF): 10000; SAMS schematic (pF): See C45; SAMS table (pF): See C45; SAMS stated VDC: Not specified; SAMS stated tolerance: Not specified; Type supported by substitute: Not specified; Check ID: No catalog substitute; Factory / SAMS differences: New inventory suffix -F. Earlier KiCad Factory/Chassis files call this C44. SAMS C44 is a different location.; Selection / missing specifications: Factory original: 0.01 µF. Original voltage/tolerance/safety class not specified. SAMS C45 400 V is not proof of the rating of this factory part.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R29** — row: Resistors: R29; Function: Rectifier surge limiter; Factory resistance (Ω): 28; SAMS resistance (Ω): 28; SAMS stated power (W): 3; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Power/surge resistor; Catalog check ID: K-MR2; Differences: Values agree; Selection / missing specifications: Use at least the stated wattage with adequate thermal clearance and working/pulse voltage. Original tolerance and temperature coefficient unlisted. IRC MR 2 is a configurable 10 W wirewound assembly, not a fixed 28 Ω part. Keep original 28 Ω / 3 W specification.; Source: Factory JPG; SAMS pp.2,7 schematic; p.5 controls/power resistors
- **X1** — row: Coils and other parts: X1; Category: Rectifier; Function: Half-wave B+ supply; Factory specification: Silicon rectifier; SAMS additional specification: Measured output current 0.090 A. Device rating not printed.; Catalog check ID: K-X1A; K-X1B; K-X1C; Selection / missing specifications: Match at least a verified 400 V / 0.5 A historical class, with appropriate surge, heat and leakage ratings. Higher ratings alone do not guarantee identical B+.; Differences / context: RCA 1N2861 has a conflicting 105 VAC capacitor-input rating.; Source: Factory JPG; SAMS pp.2,7 schematic / p.5 parts tables
- **C46** — row: Capacitors: C46; Function: Silicon rectifier shunt; Factory value (pF): 5000; SAMS schematic (pF): 5000; SAMS table (pF): 5000; SAMS stated VDC: 450; SAMS stated tolerance: Not specified; Type supported by substitute: Film/paper substitute; other listed types differ; Check ID: K-FILM; Factory / SAMS differences: Values agree; Selection / missing specifications: 5000 pF, ≥450 VDC per SAMS. Also assess repetitive pulse and transient rating. 4700 pF fitted value is a separate chassis observation.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **C1A** — row: Capacitors: C1A; Function: B+ reservoir; Factory value (pF): 40000000; SAMS schematic (pF): 40000000; SAMS table (pF): 40000000; SAMS stated VDC: 150; SAMS stated tolerance: Not specified; Type supported by substitute: Polarized electrolytic, common-negative can; Check ID: K-CAN; Factory / SAMS differences: Values agree; Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R30** — row: Resistors: R30; Function: First B+ filter dropper; Factory resistance (Ω): 150; SAMS resistance (Ω): 150; SAMS stated power (W): 2; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Power/surge resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Use at least the stated wattage with adequate thermal clearance and working/pulse voltage. Original tolerance and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C1B** — row: Capacitors: C1B; Function: B+ filter section; Factory value (pF): 40000000; SAMS schematic (pF): 40000000; SAMS table (pF): 40000000; SAMS stated VDC: 150; SAMS stated tolerance: Not specified; Type supported by substitute: Polarized electrolytic, common-negative can; Check ID: K-CAN; Factory / SAMS differences: Values agree; Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table
- **R31** — row: Resistors: R31; Function: Second B+ filter dropper; Factory resistance (Ω): 300; SAMS resistance (Ω): 300; SAMS stated power (W): 1; Original tolerance: Not specified; Original working voltage: Not specified; Type / taper: Power/surge resistor; Catalog check ID: No substitutes listed; Differences: Values agree; Selection / missing specifications: Use at least the stated wattage with adequate thermal clearance and working/pulse voltage. Original tolerance and temperature coefficient unlisted.; Source: Factory JPG; SAMS pp.2,7 schematic
- **C1C** — row: Capacitors: C1C; Function: B+ filter section; Factory value (pF): 40000000; SAMS schematic (pF): 40000000; SAMS table (pF): 40000000; SAMS stated VDC: 150; SAMS stated tolerance: Not specified; Type supported by substitute: Polarized electrolytic, common-negative can; Check ID: K-CAN; Factory / SAMS differences: Values agree; Selection / missing specifications: One section of ONE can. Match polarity, ripple current, temperature and startup voltage. 47 µF is a proposed change, not the original 40 µF.; Source: Factory JPG; SAMS printed pp.2,7 schematic and p.4 parts table

## 22. Facts the drawings do not show

These are the author's statements about the fitted hardware, and they outrank every drawing. Where a line names a part
that the statement itself does not name, the part reference comes from the drawings.

### Ambersong

- GPIO4: pull-up 10 kΩ + 0.1 µF still fitted on the S3 side, wired to the LIMITS JST female, goes nowhere else; the LIMITS harness now carries the A3144 on GPIO5 (a retrofit on the same harness).
- J42 / RADIO_TO_ADC: one transformer output side on tip, the other on ring (interchangeable); all sleeves are GND and/or CABLE S/FTP; RADIO_TO_ADC is J42 (drawn as J51 on the main sheet); its output, labelled ATTENUATOR, feeds the attenuator in the main sheet's "TUBE RADIO AUDIO JACK IN INTO PCM1802" box.
- GPIO36 filter: the 0.1 µF is on the pin side (A32 side). Main sheet's ladder-side placement is deliberate visual aide.
- 22 kΩ / 68 kΩ ladder resistors are on the A32 board (A32 sub-sheet). Main sheet's placement in the DPDT block is deliberate visual aide.
- LISTENER board goes to the front DPDT switch; A_IO36_H / _L denote the level the switch puts on the line: 3.3 V (H) or 0.8 V (L).
- Box-fan return = LM7812 return = 20V_GND (implied on the main sheet).
- DAC supply: C24 10 µF ceramic, C26 0.1 µF, C27 100 µF — all fitted; the 10 µF ceramic is actual.
- Parts drawn only on the main sheet are fitted.
- RTC: DS3231, ZS-042-style module, with an AT24C32 EEPROM on the same module at I2C address 0x57, and a CR2032 backup cell (most consumer DS3231 modules of this kind carry one; it depends on the module used). RTC_I2C: the drawings connect p3/4 (SDA) and p4/4 (SCL) and mark p1/4 and p2/4 no-connect, which is right: it is a 4-pin JST with only 2 pins connected. The statement that its "4th position is not connected" counts the positions from one end, and contact order inside a connector is not covered here.
- 5VDC_S = 5VDC (pre-bus vs post-bus naming); PSU_GND likewise.
- A_VDC = A_3V3: the same rail, joined by the bus.
- GND×2 pair = cable shield + foil terminating at the perfboard's common GND; DIGIT_OUT and DIGIT_OUT1 are one cable split by two JSTs.
- CABLE S/FTP on a pin = shield + foil terminated there, to GND on the board side, open at the cable's other end, unless noted otherwise; when GND and CABLE S/FTP both appear, both are terminated.
- Unlabelled rectangles over modules = the 3D-printed shells that hold/cover them.
- T1 primary "50C5" = the radio's output stage.
- Off-page TL074 circuit = a draft of the amp input board, drawn to help visualise it: disregard.
- RDA5807M I2C: no external pull-ups.
- Panel lamps (verified on hardware): LED resistors on Q1 drain, GPIO21 on gate (through the drawn 100 Ω), GND on source. They light and dim with the PWM. The four LED resistors are 120 Ω, chosen for maximum brightness control.
- Clock: GPIO0 of the A32 feeds both the PCM1802 SCK and the PCM5102A SCK, each through its own 33 Ω at the A32's pin.
- Colon, main sheet: R17 (variable, 1.5 kΩ) and the 33 Ω both drawn on the 5 V side of the LED is intentional visual aide; the truth is the sub-sheet's order (5 V → 33 Ω → colon → variable → GND). The variable resistor is glued at 1.5 kΩ.
- RDA5807M decoupling: only the 100 µF is polarized; the 10 µF and 0.1 µF are ceramic.
- I2S clock contacts: both A32_I2S clock contacts carry GPIO0; they are not paired to a converter. The names A_IO0A and A_IO0D are that same GPIO0, named differently so that the sheets read clearly.
- Volume pot at the A32 end: one cable carries A_IO35 + CABLE S/FTP; another carries GND + A_VDC.
- Polarized capacitors are correctly polarized as drawn. Capacitors not drawn as polarized are film or ceramic.
- PC817 (power detect): +20V → 4.7 kΩ → LED anode; LED cathode → 20V_GND; collector → GPIO1; emitter → GND.
- J42: on both ends, in all the sheets, GND and/or CABLE S/FTP are only on the sleeve of the TRS.
- J10 is deliberately "a coax connector": it was F-type and is now a BNC male; the type does not matter.
- The antenna on ANT (any FM/VHF antenna works; the author made one for this) is §29, as built and installed.
- A Faraday cage over the AC LINE FILTER, between the tube radio's mains input and Ambersong's AC input (after the front power switch), is bonded to the same earth. Not drawn.
- 30CM_ANT is a 30 cm length of wire inside the tube radio's Faraday cage, bent in a U over the FM oscillator section.
- GPIO48 controls the S3 board's onboard RGB LED: it was off before, and is bright now.
- The AS5600's magnet is a 6 × 3 mm disc neodymium magnet, less than 0.5 mm from the IC.
- J9 is the connector that comes with the common ULN2003A boards.
- Front DPDT audio switch positions, from the firmware, which is exact for this switch (§0): the direct leg puts 3.3 V on GPIO36 = RADIO; the 68 kΩ leg puts 0.81 V = BT; the centre, off, gives 0 V = AUX. The switch is electrically stable and the hardware is set as such.
- U71 on the RDA5807M sub-sheet is a 2×3 pin grid that physically connects the RDA module to the antenna, so someone making repairs can unplug the module from the antenna there. The sub-sheet label 30CM_ANT is the same as the main sheet's 30CM_ANT.
- Contact order inside a connector is not covered by this document.
- The DS3231M symbol's eight hidden GND pins, lying on the main sheet's A_VDC wire, are disregarded: they are hidden.
- A label name used on two different wires of one sheet stays as drawn, with no correction: the BT sheet's "GPIO" on LISTENER H and L, the ADC sheet's "ATTENUATOR" on RADIO_TO_ADC ring and tip. On the USB-C sheet one "TO … USB-C" name sits on all four of each plug's VBUS, D−, D+ and GND; that is a visual aide only, not a short: it shows that everything passes from the external connector to the associated MCU's USB-C connector. This document describes each of those wires separately.
- Where the SC4 digit-order swap physically sits is not established; the author believes it is at the digit driver module.

### The tube radio

*Specific to this build — adapt: this is one LLOYDS TM-838N as fitted and repaired; a reader's set will differ.*

- Order of truth for the tube radio: actual hardware, then the factory drawing, then SAMS; the KiCad sheet reflects the actual hardware except details noted on the sheet itself.
- A part's Value is its fitted state; its hidden Spec fields hold the original factory/SAMS specifications, with notes on substitutes and selection.
- The FM oscillator as drawn is the fitted circuit.
- C43 and C46 are 0.0047 µF; C1A, C1B and C1C are the 47 µF parts. C47 and C49 are 0.0022 µF, replacing the 0.002 µF originals; C48 is 0.002 µF, the original three-lead part.
- The tube pin assignments and the heater string order as drawn were checked on the chassis.
- Coil and switch terminal identifiers (L5, L6, L8–L14, S1A–S1G) are logical, not physical lugs.
- T1 has a two-terminal primary and a secondary, no tap.
- Of the drawn lamp and antenna-input parts, only the neon bulbs NE1–NE3 and J2 are absent from the device; R32, the S1E contacts, C21, L6, C22A, L7 and C23A are fitted. What SP1 and SP2 are is not known, so whether they are in the device is not recorded.
- Mains and earth: J3 takes the switched live (AC_L) and J4 the neutral (AC_N), through the external EMI filter, with no isolation transformer, so CHASSIS is on neutral; the coax shield (COAX_SHIELD_PE) is on Earth and reaches CHASSIS only through C51; CHASSIS is never bonded to Earth or the Faraday cages.
- C1A–C1C are three Rubycon 47 µF 315 V capacitors; C44 and C51 are X1Y2 rated; **C3 is X1Y1**.
- C11 is a shunt from AFC_C to chassis in the radio, and always was; the sheet once drew it in series by mistake. C2's polarity is as drawn.
- On the series heater string the two 12DT8s are interchanged against an earlier reading: the string runs V7, V5, V6, V4, V3, L16, V1, L15, V2, and V2 is the tube whose heater returns to chassis. Everything else about V1 and V2 stays as drawn.
- C48 is the original three-lead 0.002 µF ceramic and is still fitted; the part that was replaced is C52, which had been taken for C48, and C52 is now a 0.0022 µF X1Y2 safety capacitor. C49 is unchanged.
- V1 and V2 (both 12DT8) and V6 (12AV6) are new old stock tubes, fitted by Radio Hosvep on Park Ave. Their types and connections are unchanged.
- L10, L12 and L14 were tuned by Radio Hosvep on Park Ave.; only the FM path (and whatever touches it) was tuned, not the AM/SW path. Their connections and identifiers are unchanged; no source records what they were tuned to.
- **The FM IF is tuned to 10.6 MHz, not 10.7 MHz**, since L10, L12 and L14 were professionally tuned. Why is not known; the tuner, an 80-plus-year-old technician who has tuned radios all his life, is trusted on it, and his setting stands. The IF has not been measured independently, and whether the ratio detector and the AFC were re-centred with it is not recorded.
- The tube set's FM local oscillator runs one IF below the station (low-side injection), consistent on two separate recordings.
- **AUX audio is analogue and bypasses the A32 entirely.** AUX does not go through the DAC; on AUX the A32's I2S output is silent. Its jack reaches the amp's input panel, which sums its inputs; no drawing shows that run, and no A32 setting can change AUX loudness or mute it. The firmware agrees and writes zeros on AUX: `src/a32/audio.cpp`, "AUX: the analogue path bypasses us entirely and the amp sums its three / inputs, so our only job is to be silent." That line states what the firmware does, not what is wired.
- The tube radio's own volume control R1 sits at about one eighth of its travel in normal use: above that the sound is too loud downstream, not clipped. There are three volume controls in the chain: the tube radio's R1, the A32's and the amp's.
- C5, C11, C14 and C48 are three-lead ceramic capacitors: the middle lead is wired to chassis, and the two outer leads have 0 Ω between them. The sheet draws each as a two-terminal capacitor, which is representative.
- The radio feed into the ADC is mono: RAD_L to GND with the cable's shield and foil, RAD_R through the 4.7 kΩ to the LIN+RIN node, and a 0.001 µF CBB22 with a 1 kΩ from that node to GND.
- The 50C5 is drawn as fitted, with nothing on pin 5.
- The AC supply is drawn as fitted: S2 → M1 → AC_FUSE_OUT, C44 to chassis; there is no C45; L17 and L18 were removed and replaced by the external EMI filter (sheet note).
- The sheet's notes that point to "the audio sheet" and "the Main Sheet" are left as written: this sheet is usually one of a larger set.

## 23. Notes written on the main sheet (verbatim)

> SOME NOTES TIDEDLIDOTES

> There are two volume knobs, one is in the back at the / amp's analog and the other is at the front, digital only.

> USB-C connections  from back of box goes to their / respective MCU's USB-C connector.

> All the "AMP_" prefixes on labels go their respective pins / on the small PCB where AUX inputs go.

> The AMP PSU "20V_GND" is shown for completeness only; / the actual path goes to the AMP's -20V and +20V inputs.

- **Note on the colon LED cable (verbatim)** — "Caution! This 5VDC cable's polarity is reversed at the COLON_IN side compared to others!" It is drawn around COLON_IN and 5VDC@250.19,71.12.
  - source: the cable sheet; the author
> All the grounds are joined at a single starpoint, / represented by the JST 2PINS buses      

> The faraday cages over the whole of the tube radio and / over the AS5600 are both connected to EARTH

> Where possible and adequate, all cables are twisted pairs, / shielded and/or foiled. Representations are in      sheet.

## 24. Datasheet facts

Quoted from manufacturer documents. **Datasheet, not the author
and not the drawing.** Each fact says what it describes; where that is not the fitted part, how far it applies is
not established. The DS3231M facts are left out: that is the symbol's chip, and the fitted RTC is a DS3231
ZS-042-style module whose datasheet could not be re-opened.

### Main MCU (ESP32-S3) and the back-panel USB-C

- **WROOM-1 v1.8: N16R8 = 16 MB Quad flash + 8 MB Octal PSRAM, -40~65** — describes the WROOM-1 module named by the S3 symbol value — the fitted module is marked ESP32-S3-N16R8.
  - evidence: https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf (text shows 'Version 1.8'). Whitespace-normalised match found for 'Table 1-1. ESP32-S3-WROOM-1 Series Comparison' and 'ESP32-S3-WROOM-1-N16R8 16 MB (Quad SPI) 8 MB (Octal SPI) –40 ~65'.
- **WROOM-1 v1.8: IO35-37 taken by octal PSRAM on R8** — describes the WROOM-1 module named by the S3 symbol value — the fitted module is marked ESP32-S3-N16R8.
  - evidence: Found: 'For modules with Octal SPI PSRAM, i.e., modules embedded with ESP32-S3R8 or ESP32-S3R16V, pins IO35, IO36, and IO37 are connected to the Octal SPI PSRAM and are not available for other uses.'
- **WROOM-1 v1.8: GPIO47/48 at 1.8 V only on R16V(A); VDD_SPI 1.8 V only for N16R16VA** — describes the WROOM-1 module named by the S3 symbol value — the fitted module is marked ESP32-S3-N16R8.
  - evidence: Found: 'For modules embedded with ESP32-S3R16V, as the VDD_SPI voltage of the ESP32-S3R16V chip is set to 1.8 V, the working voltage for GPIO47 and GPIO48 is also 1.8 V, which is different from other GPIOs.' and 'Please note that the VDD_SPI voltage is 1.8 V for ESP32-S3-WROOM-1-N16R16VA and ESP32-S3-WROOM-1U-N16R16VA only.' That on N16R8 GPIO47/48 are at normal I/O voltage is an inference; the SoC table row gives ESP32-S3R8 VDD_SPI 3.3 V.
- **SoC v2.2 Table 1-1: ESP32-S3R8 VDD_SPI 3.3 V** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: https://documentation.espressif.com/esp32-s3_datasheet_en.pdf ('Version 2.2'). The PDF text on page index 12 reads 'ESP32-S3R8 — 8 MB (Octal SPI) ⚶40 ∼65 °C 3.3 V v0.1/v0.2'. pypdf extracts the minus sign as '⚶'; everything else matches. 'Table 1-1. ESP32-S3 Series Comparison' found.
- **WROOM-2 v1.7: embeds S3R8V/S3R16V only; all variants octal flash; no N16R8 without V** — describes the WROOM-2 module named by the S3 symbol library — not the fitted module.
  - evidence: https://documentation.espressif.com/esp32-s3-wroom-2_datasheet_en.pdf ('Version 1.7'). Found 'ESP32-S3-WROOM-2 comes with a PCB antenna. It has ESP32-S3R8V or ESP32-S3R16V SoC embedded.' Found the rows 'ESP32-S3-WROOM-2-N32R16V 32 MB (Octal SPI) 16 MB (Octal SPI)', 'ESP32-S3-WROOM-2-N16R8V (EOL) 16 MB (Octal SPI) 8 MB (Octal SPI)', 'ESP32-S3-WROOM-2-N32R8V (EOL) 32 MB (Octal SPI)'. The only ordering codes in the document are N16R8V, N32R16V, N32R8V.
- **WROOM-2 v1.7: GPIO47/48 at 1.8 V** — describes the WROOM-2 module named by the S3 symbol library — not the fitted module.
  - evidence: Found: 'As the VDD_SPI voltage of the ESP32-S3R8V and ESP32-S3R16V chips has been set to 1.8 V, the working voltage for GPIO47 and GPIO48 would also be 1.8 V, which is different from other GPIOs.'
- **WROOM-2 v1.7: pins 28-30 NC (no IO35-37 pads)** — describes the WROOM-2 module named by the S3 symbol library — not the fitted module.
  - evidence: Found: 'IO0 27 I/O/T RTC_GPIO0,GPIO0', 'NC 28 - NC', 'NC 29 - NC', 'NC 30 - NC', 'IO38 31 I/O/T GPIO38, FSPIWP, SUBSPIWP'. The U7 symbol has pins 35, 36, 37 (the main sheet).
- **WROOM-1 v1.8: 41 pins, GND/3V3/EN/IOs/EPAD, no 5V or RST** — describes the WROOM-1 module named by the S3 symbol value — the fitted module is marked ESP32-S3-N16R8.
  - evidence: Found 'The module has 41 pins.', 'GND 1 P GND', '3V3 2 P Power supply', 'EN 3 I High: on, enables the chip.', 'GND 40 P GND', 'EPAD 41 P GND'. No line in the pin table starts with 5V or RST.
- **WROOM-1 and WROOM-2: EN must not float** — describes both the ESP32-S3-WROOM-1 datasheet (the module named by the S3 symbol value) and the WROOM-2 datasheet (the module named by the S3 symbol library); the fitted module is marked ESP32-S3-N16R8.
  - evidence: Found in both PDFs: 'High: on, enables the chip. Low: off, the chip powers off. Note: Do not leave the EN pin floating.'
- **WROOM-1 and WROOM-2: VDD33 3.0-3.6 V, abs max 3.6 V** — describes both the ESP32-S3-WROOM-1 datasheet (the module named by the S3 symbol value) and the WROOM-2 datasheet (the module named by the S3 symbol library); the fitted module is marked ESP32-S3-N16R8.
  - evidence: Found in both: 'VDD33 Power supply voltage –0.3 3.6 V' and 'VDD33 Power supply voltage 3.0 3.3 3.6 V'. W1 also has 'Table 6-1. Absolute Maximum Ratings' and 'Table 6-2. Recommended Operating Conditions'.
- **Strapping pins GPIO0 (weak pull-up), GPIO3 (floating), GPIO45/46 (weak pull-down)** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: W1 found: 'Chip boot mode – Strapping pin: GPIO0 and GPIO46', 'VDD_SPI voltage – Strapping pin: GPIO45', 'JTAG signal source – Strapping pin: GPIO3', 'Table 4-1. Default Configuration of Strapping Pins', 'GPIO0 Weak pull-up 1', 'GPIO3 Floating –', 'GPIO45 Weak pull-down 0', 'GPIO46 Weak pull-down 0'. 'GPIO0 Weak pull-up 1' also found in W2 and the SoC datasheet.
- **GPIO19 = USB_D-, GPIO20 = USB_D+, default USB Serial/JTAG** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: W1 found: 'IO19 13 I/O/T RTC_GPIO19, GPIO19, U1RTS, ADC2_CH8, CLK_OUT2,USB_D-' and 'IO20 14 I/O/T RTC_GPIO20, GPIO20, U1CTS, ADC2_CH9, CLK_OUT1,USB_D+'. SoC found: 'USB_D+/- – by default, connected to the USB Serial/JTAG Controller.'
- **GPIO39-42 are MTCK/MTDO/MTDI/MTMS JTAG** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: W1 found: 'IO39 32 I/O/T MTCK, GPIO39, CLK_OUT3, SUBSPICS1', 'IO40 33 I/O/T MTDO, GPIO40, CLK_OUT2', 'IO41 34 I/O/T MTDI, GPIO41, CLK_OUT1', 'IO42 35 I/O/T MTMS, GPIO42'. SoC found: 'GPIO39, GPIO40, GPIO41, GPIO42: JTAG interface.'
- **UART0 TXD0 GPIO43, RXD0 GPIO44** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: W1 found: 'RXD0 36 I/O/T U0RXD, GPIO44, CLK_OUT2' and 'TXD0 37 I/O/T U0TXD, GPIO43, CLK_OUT1'.
- **Internal weak pull-up/down 45 kΩ** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: W1 found: 'RPU Internal weak pull-up resistor — 45 — kΩ' and 'RPD Internal weak pull-down resistor — 45 — kΩ'.
- **DevKitC-1 v1.1: J1/J3 rows match U7/U63 columns; J1 pin 3 RST = EN; J1 pin 21 5V** — describes an Espressif DevKitC reference board (its user guide) — the fitted S3 board is compatible with it in firmware and pin-compatible for every pin the device uses; the pins it does not use are not checked (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html. J1 rows: ['1','3V3','P','3.3 V power supply'], ['2','3V3','P','3.3 V power supply'], ['3','RST','I','EN'], ['21','5V','P','5 V power supply'], ['22','G','G','Ground']. J3 rows: ['1','G','G','Ground'], ['2','TX','I/O/T','U0TXD, GPIO43, CLK_OUT1'], ['3','RX','I/O/T','U0RXD, GPIO44, CLK_OUT2'], ['10','38','I/O/T','GPIO38, FSPIWP, SUBSPIWP, RGB LED']. The full name order matches (see the U7 pin-column check).
- **DevKitC-1: RGB LED on GPIO48 (initial) vs GPIO38 (v1.1)** — describes an Espressif DevKitC reference board (its user guide) — the fitted S3 board is compatible with it in firmware and pin-compatible for every pin the device uses; the pins it does not use are not checked (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Found: 'Both the initial and v1.1 versions of ESP32-S3-DevKitC-1 are available on the market. The main difference lies in the GPIO assignment for the RGB LED: the initial version uses GPIO48, whereas v1.1 uses GPIO38.' On the fitted board the RGB LED is on GPIO48 (§1).
- **DevKitC-1: three mutually exclusive power inputs** — describes an Espressif DevKitC reference board (its user guide) — the fitted S3 board is compatible with it in firmware and pin-compatible for every pin the device uses; the pins it does not use are not checked (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Found: 'There are three mutually exclusive ways to provide power to the board', 'USB-to-UART Port and ESP32-S3 USB Port (either one or both), default power supply (recommended)', '5V and G (GND) pins', '3V3 and G (GND) pins'.
- **DevKitC-1: USB-to-UART Micro-USB port and native USB port** — describes an Espressif DevKitC reference board (its user guide) — the fitted S3 board is compatible with it in firmware and pin-compatible for every pin the device uses; the pins it does not use are not checked (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Found: 'A Micro-USB port used for power supply to the board, for flashing applications to the chip, as well as for communication with the chip via the on-board USB-to-UART bridge.' and 'ESP32-S3 full-speed USB OTG interface, compliant with the USB 1.1 specification. The interface is used for power supply to the board, for flashing applications to the chip, for communication with the chip using USB 1.1 protocols, as well as for JTAG debugging.'
- **DevKitC-1: GPIO35-37 unavailable on octal WROOM-1 or WROOM-2 boards** — describes an Espressif DevKitC reference board (its user guide) — the fitted S3 board is compatible with it in firmware and pin-compatible for every pin the device uses; the pins it does not use are not checked (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Found: 'For boards with Octal SPI flash/PSRAM memory embedded ESP32-S3-WROOM-1/1U modules, and boards with ESP32-S3-WROOM-2 modules, the pins GPIO35, GPIO36 and GPIO37 are used for the internal communication between ESP32-S3 and SPI flash/PSRAM memory, thus not available for external use.'
- **ESP-IDF: GPIO19/20 USB-JTAG by default** — describes the ESP32-S3 chip (read over serial: ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB).
  - evidence: https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html. Found: 'USB-JTAG: GPIO19 and GPIO20 are used by USB-JTAG by default. If they are reconfigured to operate as normal GPIOs, USB-JTAG functionality will be disabled.'

### Audio MCU (ESP32, "A32") and the DS3231 RTC

- **WROOM-32 GPIO6-11 on SPI flash** — describes the ESP32-WROOM-32 module — the A32 module is meant to be an ESP32-WROOM-32E.
  - evidence: https://documentation.espressif.com/esp32-wroom-32_datasheet_en.pdf, "Datasheet Version 3.7". Text: "2 Pins SCK/CLK, SDO/SD0, SDI/SD1, SHD/SD2, SWP/SD3 and SCS/CMD, namely, GPIO6 to GPIO11 on the ESP32-D0WDQ6 chip are connected to the SPI flash integrated on the module and are not recommended for other uses." (page footer 10). The main sheet and sheet AUDIO MCU ESP32-WROOM-32 put 6-11 on no-connect.
- **WROOM-32 strapping pins and defaults; MTDI=IO12, MTDO=IO15** — describes the ESP32-WROOM-32 module — the A32 module is meant to be an ESP32-WROOM-32E.
  - evidence: Same PDF, raw text: "Chip boot mode" / "Strapping pin: GPIO0 and GPIO2"; "Table 3: Default Configuration of Strapping Pins" / "GPIO0 Pull-up 1" / "GPIO2 Pull-down 0" / "MTDI Pull-down 0" / "MTDO Pull-up 1" / "GPIO5 Pull-up 1"; "MTDI is used to select the VDD_SDIO power supply voltage at reset:"; "IO12 14 I/O GPIO12, ADC2_CH5, TOUCH5, RTC_GPIO15, MTDI"; "IO15 23 I/O GPIO15, ADC2_CH3, TOUCH3, MTDO"
- **WROOM-32 GPIO36/39/34/35 type I, ADC1** — describes the ESP32-WROOM-32 module — the A32 module is meant to be an ESP32-WROOM-32E.
  - evidence: Same PDF, raw text: "SENSOR_VP 4 I GPIO36, ADC1_CH0, RTC_GPIO0" / "SENSOR_VN 5 I GPIO39, ADC1_CH3, RTC_GPIO3" / "IO34 6 I GPIO34, ADC1_CH6, RTC_GPIO4" / "IO35 7 I GPIO35, ADC1_CH7, RTC_GPIO5"; "1 P: power supply; I: input; O: output."
- **WROOM-32 VDD33 3.0/3.3/3.6 V, abs max -0.3..3.6** — describes the ESP32-WROOM-32 module — the A32 module is meant to be an ESP32-WROOM-32E.
  - evidence: Same PDF: "VDD33 Power supply voltage 3.0 3.3 3.6 V" (Table 13); "VDD33 Power supply voltage �0.3 3.6 V" (Table 12; the minus sign extracts as an unknown glyph)
- **DevKitC V4 three mutually exclusive power options** — describes an Espressif DevKitC reference board (its user guide) — whether the fitted boards match it is not established (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html: "There are three mutually exclusive ways to provide power to the board: Micro USB port, default power supply; 5V and GND header pins; 3V3 and GND header pins." / "The power supply must be provided using one and only one of the options above, otherwise the board and/or the power supply source can be damaged."
- **DevKitC V4 J2/J3 rows 3V3, GND, 5V, TX, RX, IO0** — describes an Espressif DevKitC reference board (its user guide) — whether the fitted boards match it is not established (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Same page: "1 | 3V3 | P | 3.3 V power supply", "14 | GND | G | Ground", "19 | 5V | P | 5 V power supply"; J3 "1 | GND | G | Ground", "4 | TX | I/O | GPIO1, U0TXD", "5 | RX | I/O | GPIO3, U0RXD", "7 | GND | G | Ground", "14 | IO0 | I/O | GPIO0, ADC2_CH1, TOUCH_CH1, Boot"
- **DevKitC GPIO16/17 WROOM only; C15 on GPIO0 clock** — describes an Espressif DevKitC reference board (its user guide) — whether the fitted boards match it is not established (the S3 board is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; the A32 is marked "FCC IDL 2BB77-ESP32-32X", board or module not stated).
  - evidence: Same page: "The pins GPIO16 and GPIO17 are available for use only on the boards with the modules ESP32-WROOM and ESP32-SOLO-1. The boards with ESP32-WROVER modules have the pins reserved for internal use." / "The component C15 may cause the following issues on earlier ESP32-DevKitC V4 boards: The board may boot into Download mode; If you output clock on GPIO0, C15 may impact the signal."

### Audio converters: PCM1802 ADC, tube radio input, radio transformer output, PCM5102A DAC

- **PCM1802 VCC 5 V analog / VDD 3.3 V digital** — describes the named part.
  - evidence: https://www.ti.com/lit/gpn/PCM1802 (SLES023D, revised December 2016), p3: "VCC 5 — Analog power supply, 5 V" and "VDD 14 — Digital power supply, 3.3 V".
- **PCM1802 PDWN/BYPAS/OSR/FMT0/FMT1/MODE0/MODE1 internal pulldown, 5 V tolerant; PDWN active-low** — describes the named part.
  - evidence: SLES023D p3: "PDWN 7 I Power-down control, active-low (2)", "(2) Schmitt-Trigger input with internal pulldown (50 kΩ typically), 5-V tolerant"; p5: "(5) Applies to PDWN, BYPAS, OSR, FMT0, FMT1, MODE0, MODE1 (Schmitt-trigger input, with 50-kΩ typical pulldown resistor) pins."
- **PCM1802 Table 6 FMT1=0 FMT0=1 = I2S 24-bit** — describes the named part.
  - evidence: SLES023D p16: "Table 6. Data Format" / "1 0 1 I2S, 24-bit". That FMT0 is high on this board is an inference: FMT0 is wired only to the module pin P_3V3 (the main sheet).
- **PCM1802 Table 9 MODE1=MODE0=0 slave mode** — describes the named part.
  - evidence: SLES023D p21: "Table 9. Interface Mode" / "0 0 Slave mode (256 fS, 384 fS, 512 fS, 768 fS)"
- **PCM1802 FSYNC enables data output while HIGH in slave mode** — describes the named part.
  - evidence: SLES023D p21: "In slave mode, BCK, LRCK, and FSYNC work as input pins. FSYNC enables the BCK signal, and the device can" / "shift out the converted data while FSYNC is HIGH."
- **PCM1802 OSR input (low x64); BYPAS low = HPF** — describes the named part.
  - evidence: SLES023D p3: "OSR 16 I Oversampling ratio select. Low: ×64 fS; High: ×128 fS"; "BYPAS 8 I HPF bypass control. Low: normal mode (DC cut); High: bypass mode (through) (2)"
- **PCM1802 SCKI system clock input, 5 V tolerant** — describes the named part.
  - evidence: SLES023D p3: "SCKI 15 I System clock input; 256 fS, 384 fS, 512 fS, or 768 fS", "(3) Schmitt-Trigger input, 5-V tolerant"; p5: "(3) Applies to PDWN, BYPAS, SCKI, OSR, FMT0, FMT1, MODE0, MODE1 (Schmitt-trigger input, 5-V tolerant) pins."
- **PCM5102A XSMT soft mute; may tie to AVDD when unused** — describes the named part.
  - evidence: https://www.ti.com/lit/gpn/PCM5102A (SLAS859C, revised May 2015), p5: "XSMT 17 I Soft mute control (1): Soft mute (Low) / soft un-mute (High)"; p16: "In systems where XSMT is not required, it can be directly connected to AVDD."
- **PCM5102A zero-data detect: full analog mute after 1024 LRCKs of zero data on both channels (21 ms at 48 kHz)** — describes the named part.
  - evidence: https://www.ti.com/lit/gpn/PCM5102A (SLAS859C, revised May 2015), §9.3.2.3 Zero Data Detect: "When the device detects continuous zero data, it enters a full analog mute condition. The PCM510xA counts zero data over 1024 LRCKs (21ms @ 48kHz) before setting analog mute."; "By default, Both L-ch and R-ch have to be zero data for zero data detection to begin the muting process".
- **PCM5102A FMT/FLT/DEMP low meanings** — describes the named part.
  - evidence: SLAS859C p5: "DEMP 10 I De-emphasis control for 44.1-kHz sampling rate (1): Off (Low) / On (High)", "FLT 11 I Filter select : Normal latency (Low) / Low latency (High)", "FMT 16 I Audio format selection : I2S (Low) / Left-justified (High)"
- **PCM5102A pin numbers OUTL 6, OUTR 7, AGND 9, SCK 12, BCK 13, DIN 14, LRCK 15; failsafe Schmitt inputs** — describes the named part.
  - evidence: SLAS859C p5: "AGND 9 — Analog ground", "BCK 13 I Audio data bit clock input (1)", "DIN 14 I Audio data input (1)", "LRCK 15 I Audio data word clock input (1)", "OUTL 6 O", "OUTR 7 O", "SCK 12 I System clock input (1)", "(1) Failsafe LVCMOS Schmitt trigger input"; U9 numbers match the main sheet
- **PCM5102A internal PLL off with external SCK; PLL starts if SCK at ground 16 LRCK periods** — describes the named part.
  - evidence: SLAS859C p25: "The internal PLL is disabled as soon as an external SCK is supplied." / "The device starts up expecting an external SCK input, but if BCK and LRCK start correctly while SCK remains at ground level for 16 successive LRCK periods, then the internal PLL starts, automatically generating an internal SCK from the BCK reference."
- **PCM5102A AVDD 3.3 V, DVDD 1.8/3.3 V, abs max 3.9 V** — describes the named part.
  - evidence: SLAS859C p5: "AVDD 8 P Analog power supply, 3.3 V", "DVDD 20 P Digital power supply, 1.8 V or 3.3 V"; p6 8.1 Absolute Maximum Ratings: "AVDD, CPVDD, DVDD –0.3 3.9" under "Supply voltage"

### Volume pot, amp PSU 20 VDC, power detect, box and cage fans

- **PC817 Sharp DIP-4 pinout: 1 Anode, 2 Cathode, 3 Emitter, 4 Collector** — describes the named part.
  - evidence: Sharp PC817XxNSZ1B_e.pdf p2: "Internal Connection Diagram
Sheet No.: OP18002EN
2
1
1 Anode
2 Cathode
3 Emitter
4 Collector"
- **PC817 absolute maximum ratings (IF 50 mA, IFM 1 A, VR 6 V, P 70 mW, VCEO 80 V, VECO 6 V, IC 50 mA)** — describes the named part.
  - evidence: Sharp PC817XxNSZ1B_e.pdf p4: "Forward current IF 50 mA
*1 Peak forward current IFM 1 A
Reverse voltage VR 6 V
Power dissipation P 70 mW
Output
Collector-emitter voltage VCEO 80 V
Emitter-collector voltage VECO 6 V
Collector current IC 50 mA"
- **PC817 CTR 50-400% at IF=5 mA, VCE=5 V; min 50%** — describes the named part.
  - evidence: Sharp PC817XxNSZ1B_e.pdf p1: "Collector-emitter voltage is 80V.
CTR is 50% to 400% (at IF=5mA,VCE=5V,Ta=25℃)" and "(CTR : MIN. 50% at IF=5 mA, VCE=5V ,Ta=25℃)"
- **LM7812 TO-220 pins: 1 input, 2 ground, 3 output** — describes the named part.
  - evidence: TI lm7800.pdf p3: "LM7805, LM7812, and LM7815 NDE Package
3-Pin TO-220
Top View ... Pin Functions
PIN
I/O DESCRIPTION
NAME NO.
INPUT 1 I Input voltage pin
GND 2 I/O Ground pin
OUTPUT 3 O Output voltage pin"; matches the main sheet "VI (#1)" and the main sheet "GND (#2)", "VO (#3)"
- **LM7812 characteristics at VI = 19 V; 11.4-12.6 V over 14.5 V ≤ VIN ≤ 27 V, PD ≤ 15 W, 5 mA ≤ IO ≤ 1 A** — describes the named part.
  - evidence: TI lm7800.pdf p7: "6.7 LM340 / LM7812 Electrical Characteristics,
VO = 12 V, VI = 19 V
... VO Output voltage
TJ = 25°C, 5 mA ≤ IO ≤ 1 A 11.5 12 12.5 V
PD ≤ 15 W, 5 mA ≤ IO ≤ 1 A
14.5 V ≤ VIN ≤ 27 V
11.4 12.6
V"
- **ESP32 GPIO34-39 input-only, no output driver, no internal pulls; VDET_1=GPIO34, VDET_2=GPIO35** — describes the named part.
  - evidence: Espressif esp32_datasheet_en.pdf p61: "GPIO pins 34-39 are input-only . These pins do not feature an output driver or internal pull-
up/pull-down circuitry . The pin names are: SENSOR_ VP (GPIO36), SENSOR_CAPP (GPIO37),
SENSOR_CAPN (GPIO38), SENSOR_ VN (GPIO39), VDET_1 (GPIO34), VDET_2 (GPIO35).". Under SC6 the wiper's pin is GPIO35. That the wiper therefore needs no pull and cannot be driven is an inference.
- **ESP32 GPIO34 = ADC1_CH6/RTC_GPIO4, GPIO35 = ADC1_CH7/RTC_GPIO5, type I** — describes the named part.
  - evidence: esp32_datasheet_en.pdf p14: "VDET_1 10 I GPIO34, ADC1_CH6, RTC_GPIO4
VDET_2 11 I GPIO35, ADC1_CH7 , RTC_GPIO5"
- **ESP32-S3 strapping pins GPIO0, GPIO3, GPIO45, GPIO46; GPIO1 not strapping** — describes the named part.
  - evidence: Espressif esp32-s3_datasheet_en.pdf p26: "* GPIO0, GPIO3, GPIO45, GPIO46: Strapping pins."
- **ESP32-S3 GPIO1 is chip pin 6, RTC_GPIO1, TOUCH1, ADC1_CH0** — describes the named part.
  - evidence: esp32-s3_datasheet_en.pdf p24: "T able 2-8. Analog Functions ... 6 RTC_GPIO1 TOUCH1 ADC1_CH0"; p16: "6 GPIO1 IO VDD3P3_RTC IE IE IO MUX RTC IO MUX Analog"

### Display drivers: digit select (P-MOSFETs) and segment sinks

- **ULN2003A pin table 1B-7B pins 1-7 inputs, 1C-7C pins 16-10 outputs** — describes the named part.
  - evidence: https://www.ti.com/lit/ds/symlink/uln2003a.pdf (SLRS027T, rev. March 2025), pdftotext -raw: "1B 1" / "I Channel 1 through 7 Darlington base input" ... "7B 7" / "1C 16" / "O Channel 1 through 7 Darlington collector output" ... "7C 10". The word 'sink' is not in the quote.
- **ULN2003A COM pin 9** — describes the named part.
  - evidence: Same PDF: "COM 9 -- Common cathode node for flyback diodes (required for inductive loads)"
- **ULN2003A E pin 8** — describes the named part.
  - evidence: Same PDF: "E 8 -- Common emitter shared by all channels (typically tied to ground)"
- **ULN2003A 2.7k series base resistor** — describes the named part.
  - evidence: Same PDF: "The ULx2003A" / "devices have a 2.7k series base resistor for each" / "Darlington pair for operation directly with TTL or 5V" / "CMOS devices."
- **ULN2003A abs max 50 V / 30 V / 500 mA, referred to E** — describes the named part.
  - evidence: Same PDF: "Collector-emitter voltage 50 V"; "Input voltage(2) 30 V"; "Peak collector current, See Figure 5-4 and Figure 5-5 500 mA"; "(2) All voltage values are with respect to the emitter/substrate terminal E, unless otherwise noted."
- **FQP27P06 VDSS -60 V, ID -27 A, VGSS ±25 V** — describes the named part.
  - evidence: https://www.onsemi.com/pdf/datasheet/fqp27p06-d.pdf (Rev. 4, Oct 2024), pdftotext -raw: "VDSS Drain-Source Voltage -60 V"; "ID Drain Current - Continuous (TC = 25�C) -27 A"; "VGSS Gate-Source Voltage �25 V"; "IGSSF Gate-Body Leakage Current, Forward VGS = -25 V, VDS = 0 V - - -100 nA"; "IGSSR Gate-Body Leakage Current, Reverse VGS = 25 V, VDS = 0 V - - 100 nA"
- **FQP27P06 VGS(th) -2.0..-4.0 V; RDS(on) 70 mΩ max @ -10 V** — describes the named part.
  - evidence: Same PDF, raw mode (also present in layout mode): "VGS(th) Gate Threshold Voltage VDS = VGS, ID = -250 mA -2.0 - -4.0 V"; "-27 A, - 60 V, RDS(on) = 70 mW (Max.) @ VGS = -10 V, ID = -13.5 A". The row is present in both raw and layout mode.
- **FQP27P06 package 'GDS TO-220-3LD'** — describes the named part.
  - evidence: Same PDF raw: "GDS" / "TO-220-3LD" / "CASE 340AT"

### Power distribution, mains side, earth and antenna

- **JST XH: 'Current rating 3 A AC/DC (AWG #22)'** — describes the named part.
  - evidence: https://www.jst-mfg.com/product/detail_e.php?series=277 (raw page, tags stripped): 'Current rating 3 A AC/DC (AWG #22) Voltage rating 250 V AC/DC Temperature range -25 ℃ to +85 ℃'. The fitted bus connectors are not identified beyond the drawing text 'JST XH' (the main sheet).
- **JST XH: 'Pitch 2.5 mm ... Voltage rating 250 V AC/DC'** — describes the named part.
  - evidence: Same page, raw: 'Pitch 2.5 mm Circuit 1, 2, 3, ...' and 'Voltage rating 250 V AC/DC'. Both phrases are verbatim; the '...' in the heading joins two separate spec rows.
- **JST XH: 'Conductor size AWG # 30 , # 28 , # 26 , # 24 , # 22 0.05 mm 2 to 0.33 mm 2'** — describes the named part.
  - evidence: Same page, raw: 'Conductor size AWG # 30 , # 28 , # 26 , # 24 , # 22 0.05 mm 2 to 0.33 mm 2 Insulation O.D. φ 0.9 mm to φ 1.9'.
- **Schurter 6100-4: C14 is 10 A, protection class I, suitable per IEC 61140** — describes the Schurter 6100-4, a C14 inlet — no source names the fitted inlet (the drawing has a generic IEC 60320 C14 receptacle).
  - evidence: https://www.schurter.com/en/datasheet/6100.4125 (raw page, tags stripped): 'C14 acc. to IEC 60320-1, UL 60320-1, CSA C22.2 no. 60320-1 (for cold conditions) pin-temperature 70 °C, 10 A, Protection Class I' and 'Suitable for appliances with protection class I acc. to IEC 61140'. The heading joins the two with ' / '.

### Stepper and index sensor, AS5600, RDA5807M, panel lamps, BT button/LED, audio source switch

- **AS5600 7-bit address 0x36** — describes the named part.
  - evidence: Seeed-hosted AS5600 PDF; its text: "The host MCU (master) initiates data transfers. The 7-bit slave / address of the AS5600 is 0x36 (0110110 in binary)."
- **AS5600 DIR polarity** — describes the named part.
  - evidence: the AS5600 PDF "8 DIR Digital input Direction polarity (GND = values increase clockwise, / VDD = values increase counterclockwise)"
- **AS5600 SDA/SCL consider external pull-up** — describes the named part.
  - evidence: the AS5600 PDF "6 SDA Digital input/output I²C Data (consider external pull-up)"; "7 SCL Digital input I²C Clock (consider external pull-up)"
- **AS5600 VDD3V3 abs max 4.0 V; tie VDD5V and VDD3V3 in 3.3 V operation** — describes the named part.
  - evidence: the AS5600 PDF "VDD3V3 DC Supply Voltage at" / "VDD3V3 pin -0.3 4.0 V"; "In 3.3V operation, the VDD5V and VDD3V3 pins must be tied / together."
- **AS5600 OUT analog/PWM; PGO internal pull-up** — describes the named part.
  - evidence: the AS5600 PDF "3 OUT Analog/digital output Analog/PWM output"; "5 PGO Digital input Program option (internal pull-up, connected to / GND = Programming Option B)"
- **RDA5807M Table 7-1 pins** — describes the named part.
  - evidence: Adafruit-hosted RDA5807M PDF; the RDA5807M PDF "Table 7-1 RDA5807M Pins Description" / "GND 1,3,8 Ground. Connect to ground plane" / "FM_IN 2 LNA dual input port." / "RCLK 6" with "32.768KHz crystal oscillator and" on its line and "reference clock input" on the next (layout text) / "SDIO 5" / "SCLK 4 Clock input for serial control bus" / "VDD 7 Power supply" / "ROUT,LOUT 9,10"
- **RDA5807M supply 1.8-3.3 V, VDD row** — describes the named part.
  - evidence: the RDA5807M PDF "The RDA5807M integrated one LDO which / supplies power to the chip. The external supply / voltage range is 1.8-3.3 V."; "VDD Supply Voltage 1.8 3.0 3.3 V"
- **RDA5807M I2C only, chip address 0010000b; 0x11 random-access mode not in this revision** — describes the named part.
  - evidence: the RDA5807M PDF "The RDA5807M only supports I2C control / interface."; "a 7-bit chip address (0010000b) and a R/W bit."; grep for "0x11|random|sequential" in the RDA5807M PDF found no match.
- **The fitted RDA5807M acknowledges at I2C address 0x11** — the running machine, not the quoted datasheet: the firmware addresses it at 0x11 and it answers at every boot (observed 2026-09-25). The datasheet revision above documents only 0x10.
- **RDA5807M C2 22 nF near pin 7** — describes the named part.
  - evidence: the RDA5807M PDF "5. Place C2 Close to 5807M pin7."; "C2 22nF Power Supply Bypass Capacitor Murata"
- **ULN2003A pinout 1B-7B pins 1-7, 1C-7C 16-10, COM 9, E 8** — describes the named part.
  - evidence: TI uln2003a.pdf (pdftotext text layer: TI SLRS027T); the ULN2003A PDF "1B 1 16" / "1C" ... "7B 7 10" / "7C" / "E 8 9" / "COM" (raw text); "Figure 4-1. D, N, NS, and PW Package 16-Pin SOIC, PDIP, SO, and TSSOP Top View"
- **ULN2003A COM common cathode; E common emitter** — describes the named part.
  - evidence: the ULN2003A PDF "Common cathode node for flyback diodes (required for inductive loads)"; "Common emitter shared by all channels (typically tied to ground)"
- **ULN2003A 2.7 kΩ base resistor, 3.3/5 V logic, open collector** — describes the named part.
  - evidence: the ULN2003A PDF "All units / feature a common emitter and open collector outputs. ... The ULN2003A device has a series base resistor to each Darlington pair, / thus allowing operation directly with TTL or CMOS operating at supply voltages of 5 V or 3.3 V."; "The ULx2003A / devices have a 2.7k series base resistor for each"
- **IRL540N logic-level, VGS(th) 1.0-2.0 V, RDS(on) at 5.0/4.0 V** — describes the named part.
  - evidence: redrok IRL540N PDF; the IRL540N PDF "Logic-Level Gate Drive"; "RDS(on) Static Drain-to-Source On-Resistance ... 0.053 VGS = 5.0V, ID = 18A"; "0.063 VGS = 4.0V, ID = 15A"; "0.044 VGS = 10V, ID = 18A"; "VGS(th) Gate Threshold Voltage 1.0 ... 2.0 V VDS = VGS, ID = 250µA"
- **A3144 4.5-24 V, open-collector 25 mA, needs pull-up** — describes the named part.
  - evidence: elecrow A3141-2-3-4 PDF; the A3144 PDF "Each device includes a voltage regulator for operation with supply / voltages of 4.5 to 24 volts, ... and an open-collector output to sink / up to 25 mA. With suitable output pull up, they can be used with / bipolar or CMOS logic circuits."
- **A3144 pinning 1 supply 2 ground 3 output; output low above BOP** — describes the named part.
  - evidence: the A3144 PDF "1 2 3"; "SUPPLY GROUND OUTPUT"; "Pinning is shown viewed from branded side."; "The output of these devices (pin 3) switches low when the magnetic field / at the Hall element exceeds the operate point threshold (BOP). ... When the magnetic field is reduced to below the / release point threshold (BRP), the device output goes high."
- **A3144 discontinued, substitute A1104** — describes the named part.
  - evidence: the A3144 PDF "Discontinued Product"; "for the A3141, refer to the A1101"; "for the A3144, refer to the A1104"

### The cable harnesses (cable sheet "THIS")

- **JST XH: 3 A AC/DC (AWG #22)** — describes the named part.
  - evidence: https://www.jst-mfg.com/product/detail_e.php?series=277 gives 'Current Rating: "3 A AC/DC (AWG #22)"'.
- **JST XH conductor sizes AWG 30-22** — describes the named part.
  - evidence: Same page: 'Conductor Size (applicable wire): "AWG # 30 , # 28 , # 26 , # 24 , # 22"'.
- **JST XH 2.5 mm pitch, 250 V AC/DC** — describes the named part.
  - evidence: Same page: 'Pitch: "2.5 mm"' and 'Voltage Rating: "250 V AC/DC"'.

### Tube radio — T1: FM RF amplifier and mixer (V1, 12DT8)

The tube radio facts are quoted from the page each evidence line names; not all of those pages are manufacturer documents.

- **12DT8: Heater indirectly heated, 12.6 V at 0.15 A** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12dt8.html — "Vf 12.6 Volts / If 0.15 Ampere / Indirect / Specified voltage AND current AC/DC"
- **12DT8: 9-pin miniature (noval, B9A) envelope/base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12dt8.html — "Noval, 9 pin miniature (USA pico-9) B9A"
- **12DT8: High-mu twin triode, small-button 9-pin, EIA base 9DE, heater 12.6 V / 0.15 A** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://nj7p.org/Tubes/SQL/Tube_query.php?Type=12DT8 — "High-Mu Twin Triode"; "Small Button 9-Pin"; "EIA Base ..................................... 9DE"; "Heater Voltage ............................... 12.6 V"; "Heater Current ................................ 0.15 A"
- **12DT8: Differs from the 12AT7 in heater and heater-cathode ratings, interelectrode capacitances and basing arrangement, so a 12AT7 pinout cannot be assumed** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://nj7p.org/Tubes/SQL/Tube_query.php?Type=12DT8 — "Except for heater and heater-cathode ratings, interelectrode capacitances, and basing arrangement, these types are identical with 12AT7."
- **12DT8: Heater 12.6 V, 0.15 A (TDSL table); the pinout is shown as an image only** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://tdsl.duncanamps.com/show.php?des=12DT8 — table cells "Vh V" / "12.6" and "Ih A" / "0.15"; "12DT8 Pinout" "![pinout](basing/9de.gif)"

### Tube radio — T2: FM local oscillator and AFC (V2, 12DT8)

- **12DT8 (V2): Heater 12.6 V at 0.15 A, indirectly heated, voltage and current both specified, AC/DC** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12dt8.html — raw page line 340: 'Vf 12.6 Volts / If 0.15 Ampere / Indirect / ... Specified voltage AND current AC/DC'
- **12DT8 (V2): Noval 9-pin miniature base (B9A); VHF double triode** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12dt8.html — raw page line 326: 'Noval, 9 pin miniature (USA pico-9) B9A'; line 260: 'Double Triode &nbsp; VHF&nbsp;'

### Tube radio — T3: 10.7 MHz FM IF path (V4 first IF, V5 limiter, 12BA6)

- **12BA6 (V4, V5): Remote-cutoff RF pentode, basing 7BK** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.nostalgiaair.org/tubes/12BA6.htm — page text (HTML tags stripped): "Type 12BA6 Remote Cutoff RF Pentode MECHANICAL DATA Basing 7BK"
- **12BA6 (V4, V5): Heater rated 12.6 V, 0.15 A** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.nostalgiaair.org/tubes/12BA6.htm — "HEATER CHARACTERISTICS Heater Voltage 12.6 Volts Heater Current 0.15 A"
- **7BK basing (12BA6 per NostalgiaAir): Pin 1 control grid, 2 suppressor grid and internal shield, 3 heater, 4 heater, 5 plate, 6 screen grid, 7 cathode (wiring side of socket). This agrees with the drawn V4/V5 pins: G1 #1, G3 #2, H #3/#4, P #5, G2 #6, K #7.** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bustedgear.com/res_Tube_pinouts.pdf — text layer: "PIN DIAGRAM: 7BK" / "WIRING SIDE OF SOCKET IS SHOWN" / "1 Control Grid" / "2 Suppressor Grid, IS" / "3 Heater" / "4 Heater" / "5 Plate" / "6 Screen Grid" / "7 Cathode" / "IS = Internal Shield (Electrostatic)." (the page names 18GD6A, 12AU6, 6AU6 and is headed SHARP-CUTOFF PENTODE)

### Tube radio — T5: 455 kHz AM IF and AM detector

- **12AV6 (V6): Pins 5 and 6 are the two diode plates; with the cathode on pin 2 they act as a dual diode (hobbyist build article, not a manufacturer datasheet)** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bristolwatch.com/radio/index.htm (page title '12AV6 Vacuum Tube AM Radio – Classic DIY Build with LM386 Amplifier'): 'The two "plates" pin 5 and 6 with the cathode (pin 2) act as a dual diode.'
- **12AV6 (V6): The heater is on pins 3 and 4 and needs 12.6 V** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bristolwatch.com/radio/index.htm: 'Pins 3 and 4 on the tube are the filament and require 12.6 volts AC or DC.'
- **12AV6 (V6): Heater 12.6 V, 0.15 A, indirectly heated; B7G miniature 7-pin base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12av6.html: 'Vf' '12.6 Volts'; 'If' '0.15 Ampere'; 'Indirect'; 'Specified voltage AND current' 'AC/DC'; 'Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zk)'
- **6BA6 (the r-type page does not name the 12BA6): Basing: 1 g1, 2 g3 and shield, 3 h, 4 h, 5 anode, 6 g2, 7 k** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://r-type.org/exhib/aai0837.htm (page for the '6BA6', also 'EF93'): '1 g1' '2 g3,s' '3 h' '4 h' '5 a' '6 g2' '7 k'. The text '12BA6' does not appear on the page.
- **12BA6 (V4): Heater 12.6 V, 0.15 A, indirectly heated; B7G miniature 7-pin base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12ba6.html: 'Vf' '12.6 Volts'; 'If' '0.15 Ampere'; 'Indirect'; 'Specified voltage AND current AC/DC'; 'Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zia)'
- **6BE6 / 12BE6 (V3): 6BE6 basing B7G: 1 g1, 2 k, 3 h, 4 h, 5 a, 6 g2,g4, 7 g3; the same page names the 12BE6 as the 12.6 V heater version** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: http://www.r-type.org/exhib/aaa0011.htm: 'B7G | 1 | 2 | 3 | 4 | 5 | 6 | 7' / 'g1 | k | h | h | a | g2,g4 | g3'; 'See also 12BE6 for the 12.6 Volt heater version.'
- **12BE6 (V3): Heater 12.6 V, 0.15 A, indirectly heated; B7G miniature 7-pin base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12be6.html: 'Vf 12.6 Volts / If 0.15 Ampere / Indirect / Specified voltage AND current AC/DC'; 'Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zeo)'

### Tube radio — T6: AM / SW input, converter (V3, 12BE6) and SW / AM local oscillator

- **12BE6 (V3): B7G pin functions: pin 1 g1, pin 2 k, pins 3 and 4 heater, pin 5 anode, pin 6 g2 and g4, pin 7 g3. This matches the drawn V3.** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.r-type.org/exhib/aaa0820.htm — "Pin Connections B7G 1 2 3 4 5 6 7 g1 k h h a g2,g4 g3"
- **12BE6 (V3): Heater 12.6 V at 0.15 A, indirectly heated** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12be6.html — "Filament Vf 12.6 Volts / If 0.15 Ampere / Indirect / Specified voltage AND current AC/DC"
- **12BE6 (V3): Miniature 7-pin B7G base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12be6.html — "Base Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zeo)"
- **12BE6 (V3): Heater 12.6 V / 0.15 A (second source, flattened table: Vh then Ah)** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.r-type.org/exhib/aaa0820.htm — "Heptode Vh Ah Va Vs Vg mAa mAs ra gm Vosc 12.6 0.15 250 100 -1.5 3 7.1 1M 0.475 10"
- **12BE6 (V3): Pin 1 is the local-oscillator grid (a restorer's forum post, not a manufacturer datasheet)** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/forum/tele_tone_50c5_grid_bias_from_12be6.html — "the 50C5 grid was biased with a long white wire to a 470k resistor soldered to the local oscillator grid at pin1 of the 12BE6 pentagrid converter."

### Tube radio — T7: Audio amplifier (V6 12AV6, V7 50C5), tone switch, original output transformer and speakers

- **12AV6: Heater 12.6 V, 0.15 A, indirectly heated, voltage and current specified (the page does not say 'series working')** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12av6.html — 'Vf 12.6 Volts / If 0.15 Ampere / Indirect / Specified voltage AND current AC/DC'
- **12AV6: Miniature 7-pin B7G base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12av6.html — 'Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zk)'
- **12AV6: Heater (filament) on pins 3 and 4** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bristolwatch.com/radio/index.htm — 'Pins 3 and 4 on the tube are the filament and require 12.6 volts AC or DC.'
- **12AV6: Diode plates on pins 5 and 6 work with the cathode on pin 2** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bristolwatch.com/radio/index.htm — 'The two "plates" pin 5 and 6 with the cathode (pin 2) act as a dual diode.'
- **12AV6: The triode has only a control grid besides plate and indirectly heated cathode (no grid or plate pin number given on this page)** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.bristolwatch.com/radio/index.htm — 'In the case of the 12AV6 it has only a control grid in addition to the plate and indirectly heated cathode.'
- **50C5: Beam power amplifier; heater 50 V at 0.15 A; EIA base 7CV; data source RC-29** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html — 'Beam Power Amplifier' / 'Data Source. . . . . . . . . RC-29' / 'Heater or Filament Voltage . 50 volts' / 'Heater or Filament Current . 0.15 amperes' / 'EIA Base . . . . . . . . . . 7CV'
- **50C5: Heater current specified for series working; miniature 7-pin B7G base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_50c5.html — 'Vf 50 Volts / If 0.15 Ampere / Indirect / Specified current AC/DC ~ = (Series working)' and 'Miniatur-7-Pin-Base B7G, USA 1940 (Codex=Zh)'
- **50C5: Maximum ratings: plate dissipation 7 W, screen dissipation 1.4 W, plate 150 V, screen 130 V** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html — 'Plate Dissipation . . . . . 7 watts' / 'Screen Dissipation. . . . . 1.4 watts' / 'Maximum Plate Voltage . . . 150 volts' / 'Maximum Screen Voltage. . . 130 volts'
- **50C5: Typical class A operation: Ep 120 V, Eg −8 V, Es 110 V, Ip 49 mA, load 2.5 kΩ, output 2.3 W (the page also gives a 110 V row: 50 mA, 2.5 kΩ, 1.9 W)** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html — 'Description Ep Eg EgMax Es Is IsMax Ip IpMax Rp TC AF Zl PO' / 'Class A Amp 110 -7.5 - 110 4 8.5 50 - 14K 7500 49 2.5K 1.9' / 'Class A Amp 120 -8 8 110 4 8.5 49 50 10K 7500 - 2.5K 2.3'

### Tube radio — T8: Series heater string (all seven tubes), mode supply and indicator lamps

- **50C5: Heater 50 V at 0.15 A, specified for series (AC/DC) working** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_50c5.html — "Vf 50 Volts / If 0.15 Ampere / Indirect / | Specified current AC/DC ~ = (Series working)"
- **50C5: Heater on pins 3 and 4 (pin 1 Cath G3, 2 and 5 Grid-1, 6 Grid-2, 7 Anode); heater 50 V, 0.15 A** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: http://www.oddmix.com/tube7/50c5.html — "Heater: 50 V, 0.15 A" and "PIN # | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Electrodes | Cath G3 | Grid-1 | Heater | Heater | Grid-1 | Grid-2 | Anode"
- **50C5: EIA basing 7CV** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html — "EIA Base . . . . . . . . . . 7CV"
- **12BA6: Heater 12.6 V at 0.15 A on pins 3 and 4** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: http://www.oddmix.com/tube7/12ba6.html — "Heater: 12.6 V, 0.15 A" and "PIN # | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Electrode | Grid-1 | Grid-3 | Heater | Heater | Anode | Grid-2 | Cathode"
- **12AV6: Heater 12.6 V at 0.15 A on pins 3 and 4** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: http://www.oddmix.com/tube7/12av6.html — "Heater: 12.6 V, 0.15 A" and "PIN # | 1 | 2 | 3 | 4 | 5 | 6 | 7 | Electrodes | Grid-1 | Cathode | Heater | Heater | Anode 1D | Anode 2D | Anode T"
- **12BE6: Heater 12.6 V at 0.15 A, AC/DC** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12be6.html — "Vf 12.6 Volts / If 0.15 Ampere / Indirect / | Specified voltage AND current AC/DC"
- **12BE6: EIA base 7CH; filament 12.6 V, 150 mA** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: http://electrontubestore.com/index.php?main_page=product_info&products_id=283&language=fr — "EIA Base: 7CH | Filament Voltage: 12.6V | Filament Current: 150mA"
- **12DT8: Heater 12.6 V at 0.15 A, AC/DC; Noval 9-pin miniature base** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_12dt8.html — "Noval, 9 pin miniature (USA pico-9) B9A | Filament | Vf 12.6 Volts / If 0.15 Ampere / Indirect / | Specified voltage AND current AC/DC"
- **12DT8: Two triode units isolated by an internal shield with its own base-pin terminal; intended as FM RF amplifier and combined oscillator-mixer** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.amplifiedparts.com/products/vacuum-tube-12dt8-triode-dual — "The 12DT8 is a general-purpose high-mu twin triode of the 9-pin miniature type intended for use as an rf amplifier and as a combined oscillator-mixer in fm tuners." / "In the 12DT8, the two units are effectively isolated from each other by an internal shield having a separate base-pin terminal."
- **series heater string (AC/DC sets): AC/DC designs for 110–117 V usually used a 150 mA heater current** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://en.wikipedia.org/wiki/All_American_Five — "AC/DC designs for 110–117V usually used 150 mA heater current." (a non-breaking-space span sits between 150 and mA in the raw HTML)

### Tube radio — T9: Original transformerless AC supply, and where the tube radio meets Ambersong (main sheet)

- **50C5 (V7): Heater rating 50.0 V at 0.15 A** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.r-type.org/exhib/abo0115.htm — '50C5 - 50.0 V @ 0.15 A' (thin spaces before V and A in the page)
- **50C5 (V7): Heater 50 V, 0.15 A; EIA base 7CV** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html — 'Heater or Filament Voltage . 50 volts' / 'Heater or Filament Current . 0.15 amperes' / 'EIA Base . . . . . . . . . . 7CV'
- **50C5 (V7): Indirectly heated, for series (AC/DC) heater operation at a specified current** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.radiomuseum.org/tubes/tube_50c5.html — 'Vf 50 Volts / If 0.15 Ampere / Indirect / Specified current AC/DC ~ = (Series working)'
- **50C5 (V7): Socket pins, base B7G: 1 k,g3; 2 g1; 3 h; 4 h; 5 g1; 6 g2; 7 a. On the sheet, V7's heater is on pins #4 (AC_FUSE_OUT) and #3, and G2 is #6 on B115_A.** — describes the named tube or part type, not a measurement of the fitted one.
  - evidence: https://www.r-type.org/exhib/abo0115.htm — table text 'Pin Connections B7G 1 2 3 4 5 6 7 k,g3 g1 h h g1 g2 a'

## 25. Known drawing errors and their corrections

The drawing errors found while this document was made, what the drawing showed, and what reflects the fitted hardware.
The published sheets carry the corrections (saved on 2026-09-23): every row below is done, except
two left uncorrected by choice — SC35 (c) and the tube-radio row of SCD4 — because they read as the typos they are. If an
older copy of a sheet shows the "as drawn" state, this document states the truth.

The second SCD4 row and SCE1–SCE4 concern the tube radio sheet.

### The record

Coordinates are sheet millimetres; handles are the drawn reference text.

| # | Sheet | What is wrong (as drawn) | What to do to reflect truth | Status |
|---|---|---|---|---|
| SC2 | PANEL RADIO LED PWM CONTROL sub-sheet | The four LED resistors read **680Ω**. | Change all four to **120Ω**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the four LED resistors at 120Ω |
| SC3 | Main sheet | BT pair button **SW2** (129.54,187.96): pin 2 on **A_VDC**. | Put SW2 pin 2 on **GND** (active low, ground-referenced). LED D29 is already right. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows SW2 pin 2 on GND, pin 1 on A_IO25 |
| SC3 | BT sub-sheet | Button **SW4** pin 1 on **A_IO33**; LED connector p1 ("LED −") on **A_IO25** — GPIO25 and GPIO33 swapped. | Button on **A_IO25** (other side GND); LED − (cathode) on **A_IO33**; LED + on A_3V3. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows SW4 on A_IO25 and GND; LED connector p1 on A_IO33, p2 on A_3V3 |
| SC4 | Main + P-MOSFET DIGITS sub-sheet | Digit select reads GPIO2 → digit 1, GPIO42 → 2, GPIO41 → 3, GPIO40 → 4. | Make the chain read **GPIO42 → digit 1, GPIO2 → digit 2, GPIO40 → digit 3, GPIO41 → digit 4**, wherever the swap physically is. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows GPIO42 → digit 1, GPIO2 → 2, GPIO40 → 3, GPIO41 → 4 on the main sheet |
| SC5 | Main sheet | The four digit MOSFETs (P-Mosfet, PFet1–3) have value **FPQ27P06**, and the four notes at y 138.68 read FPQ27P06. | Change all eight to **FQP27P06**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows no FPQ27P06 left; the four digit MOSFETs read FQP27P06 |
| SC6 | Main sheet | Pot wiper label **A_IO34** on A32 U10 pin 35 (83.82,154.94) and RV4 pin 2 (189.23,157.48). | Rename both to **A_IO35**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows U10 pin 35 and RV4 pin 2 on A_IO35 |
| SC6 | AUDIO MCU sub-sheet | U74 pin 35 (138.43,87.63) has **no wire**; the A32_VOL cable conductor A_IO35 reaches only the connector pins A32_VOL p1/2 and U45 p2/2, no U74 pin. | Wire U74 **pin 35 to A_IO35**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows U74 pin 35 on local labels "35", joining A32_VOL p1 and U45 p2 on A_IO35 |
| SC7 | AUDIO MCU sub-sheet + cable sheet | I2S cable conductor A32_I2S p3/7 labelled **A_IO16**. | Rename to the GPIO0 clock branch. | **DONE** — the 18:38 save shows A_IO0 on both drawings (since named A_IO0A and A_IO0D) |
| SC8 | Main sheet | The two 33 Ω clock resistors drawn beside the converters: 33Ω@83.82,60.96 (PCM1802 SCK), 33Ω@118.11,31.75 (PCM5102A SCK). | Draw both **at the A32's GPIO0 pin**, one per converter. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the two 33Ω on A_IO0A and A_IO0D, the GPIO0 branches |
| SC12 | Main sheet | Colon block read 1.5kΩ (fixed) + **330Ω**. | **33Ω**, and the 1.5 kΩ drawn as a variable resistor. | **DONE.** The saved sheet shows R17 (variable, 1.5kΩ) and 33Ω@213.36,46.99, both on the 5 V side of the LED; that placement is intentional visual aide (the truth is the sub-sheet's order). |
| SC12 | 7 SEGMENTS 24H CLOCK DISPLAY sub-sheet | Variable resistor **R1** carried **no value** ('~'). | Give R1 its value: **1.5 kΩ**. | **DONE** — the 19:08 save shows R1 = 1.5kΩ (since renamed R99, SCE2) |
| SC13 | Main sheet | 470Ω@116.84,54.61 and 470Ω@118.11,63.50 at the PCM5102A, AMP_ labels straight off U9, **no plug**. | Draw the DAC → amp link **as the cable sheet does**: plug DAC_OUT (J49), then the two 470 Ω **in the cable** (AMP_G → AMP_R− and AMP_G → AMP_L−), to AMP_IN. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows four 470Ω on AMP_L+, AMP_R+, AMP_L−, AMP_R−; the main sheet shows the resistances on the pins, as a visual aide |
| SC13 | PCM5102A DAC sub-sheet | **No 470 Ω**; plug J40's three wires reach no other pin or label, and their far ends touch nothing. | Same as above, matching the cable sheet. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the 470Ω drawn on the cable sheet only, and J40 drawn connected |
| SC14 | Main sheet | RDA5807M: **C28 100µF** runs VDD → middle node; C29 0.1µF + C30 10µF run middle node → GND (C28 **in series**). | **All three directly across the RDA's VDD–GND**: 100 µF electrolytic, 10 µF and 0.1 µF ceramic. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows C28, C29, C30 all directly across S3_3V3 and GND |
| SC14 | RDA5807M sub-sheet | Only C18 10µF (drawn polarized) and C19 0.1µF; **no 100 µF**. | Add the **100 µF electrolytic** across the supply; the 10 µF is ceramic, not polarized. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows 100µF, 10µF and 0.1µF across the supply |
| SC15 | Main sheet | POWER BUSES note "**12X** 5VDC JST 2PINS BUS" with **12** pairs. | **15** positions: "15X" and 15 pairs. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the bus drawing removed from the main sheet, and the bus sheet (box now named HERE) with 15 positions |
| SC16 | MAIN MCU (S3) sub-sheet | U63's power pins (3V3 ×2, 5V, GND ×4) **all unconnected**; no C23/C25. | Match the main sheet: 5V pin on 5VDC with 100 µF + 0.1 µF; upper 3V3 pin drives S3_3V3 with C23 0.1 µF + C25 10 µF; GND pins as main. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows U63's 3V3, 5V and GND pins connected, with C45 0.1µF and C53 10µF |
| SC17 | AUDIO MCU (A32) sub-sheet | U74's **3V3 pin no-connect**; no capacitors. | Match the main sheet: 3V3 pin feeds A_3V3 with C20 0.1 µF + C21 10 µF + C22 10 µF. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows U74's 3V3 pin connected, with C54 0.1µF, C55 10µF, C56 10µF |
| SC19 | Main sheet box + file | Box **"VOLUME POT"** (177.16,151.13) over RV4 opens the **box fan's harness** (`Ambersong - VOLUME POT.kicad_sch`). | Move/rename the box (and the file) to the **12V BOX FAN** block; the pot has no sub-sheet. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the box over the fan block named BOX FAN, opening `Ambersong - BOX FAN.kicad_sch` |
| SC24 | USB-C CONNECTORS sub-sheet | On P2 and P1, **VBUS and GND carry no-connect X**. | Show **VBUS and GND carried through** to each MCU board's USB-C; the S3's goes to its native USB port. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows no no-connect marks left on P1 or P2 |
| SC29 | Main sheet | 7 SEGMENTS BUS DRIVER: leftover wires and junction dots (y 116.84, x 227.33 / 236.22 / 245.11 / 254.00 / 261.62 / 267.97) join the A–F mid-nodes and their six 100 kΩ onto SEG_G_BUS. | **Remove the leftovers**: seven separate segment buses; **one 100 kΩ per segment to 5VDC**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows SEG_G_BUS holding only its own 200Ω and Pin_7 of J2, J5, J6, J7; the A–F channels separate |
| SC30 | Main sheet | Antenna **J10**: centre "In" → **Earth**, shell "Ext" → **ANT**. | **Centre "In" → ANT**, **shell "Ext" → Earth**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows J10 In on ANT, Ext on Earth |
| SC33 | PANEL RADIO LED PWM CONTROL sub-sheet | LED resistors on label **"G"**; GPIO21's 100 Ω on **"D"**. | Swap: LED resistors on **D**, GPIO21's 100 Ω (with 100 kΩ and 220 pF) on **G**; S = GND. Main sheet Q1 is right. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the LED resistors on D; GPIO21's 100Ω, 100kΩ and 220pF on G |
| SC35 | Files + main sheet + symbol | (a) Box **"RDA5807M"** opens `Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch`. (b) Main block title **"RDA5907M TUNING CALIBRATOR"**. (c) S3 symbol from library **ESP32-S3-WROOM-2**, value "ESP32-S3-WROOM-1_N16R8"; fitted module marked **ESP32-S3-N16R8**. (d) RTC block title, box and file read **"DS3132"**. (e) S3 sub-sheet file named **`Ambersong - MAIN MCU ESP32-S# N16R8.kicad_sch`**. | (a) Rename the file to match the RDA5807M. (b) "RDA5807M". (c) A symbol/value matching the fitted ESP32-S3-N16R8. (d) "DS3231". (e) Rename the file to "…ESP32-S3 N16R8". | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows (a) the box RDA5807M opening `Ambersong - RDA5807M.kicad_sch`; (b) the title RDA5807M TUNING CALIBRATOR; (d) the RTC box, title and file DS3231 (`Ambersong - DS3231 RTC.kicad_sch`); (e) the S3 file `Ambersong - MAIN MCU ESP32-S3 N16R8.kicad_sch`. **(c) CLOSED, NOT CORRECTED, by choice:** U7 and U63 still use the library symbol PCM_Espressif:ESP32-S3-WROOM-2, value ESP32-S3-WROOM-1_N16R8; it reads as the typo it is |
| SCV6 | DS3132 RTC sub-sheet + cable sheet | RTC_I2C (with U16 on the RTC sub-sheet) draws **p1/4 and p2/4 no-connect**, p3/4 on A_IO22 (SDA) and **p4/4 on A_IO21 (SCL)**. The 4th position is not connected. | Correct the RTC_I2C connector on the drawings so that its **4th position is not connected**. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows RTC_I2C with two of its four positions connected, which is right |
| SCD4 | Main sheet | The sheet box for `Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch` is named **"Lloyd's TM-838M Tube Radio"**. | Rename the box **"Lloyd's TM-838N Tube Radio"** (a typo). | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the tube radio box reading Lloyd's TM-838N Tube Radio |
| SCD4 | Tube radio sheet | The mode-switch units S1A–S1G use the library symbol **Mine:TM-838M_UNIT1** (M). | Name the symbol TM-838N, like the file and the Source property that S1A and other parts carry ("Factory TM-838N JPG; SAMS pin/reference crosswalk"): the same M-for-N typo as the box. | **CLOSED, NOT CORRECTED, by choice:** S1A–S1G still use the library symbol Mine:TM-838M_UNIT1; it reads as the typo it is |
| SCE1 | Tube radio sheet | The FM oscillator plate net — V2 pin 1 (P), C20 #2, R10 #2, joined by five wires — has **no label**. The connection itself is complete. | Put a label on that net. The net lost an FM_OSC_PLATE label in a rework, and KiCad's "wire_dangling" warnings on two of its wires clear with a label (tried on a copy). | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the FM oscillator plate net labelled FM_OSC_PLATE |
| SCE2 | Tube radio sheet + 7 SEGMENTS 24H CLOCK DISPLAY sub-sheet | Two different parts share the reference **R1**: the tube radio's 500kΩ volume potentiometer (Device:R_Potentiometer_Small) and the colon's 1.5kΩ variable resistor (Device:R_Variable_US). | Give one of them another reference. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the colon variable resistor renamed R99 (1.5kΩ); R1 is the tube radio's only |
| SCE3 | Tube radio sheet | X1's value reads **"Silicone Rectifier"**. | **"Silicon Rectifier"** (its own Spec: "Silicon rectifier"). | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows X1 reading Silicon Rectifier |
| SCH3 | Main sheet | The load across the transformer secondary has Value **`5W7Ω`** in the 2026-09-20 12:04:05 save, which reads 7 Ω. | Give it the part's marking, **5W7Ω5J** — 5 W, 7.5 Ω, ±5%. | **DONE 2026-09-26** — the 2026-09-23 16:15:10 save shows reference text 7.5Ω and Value 5W7.5Ω; the ±5% is irrelevant |
| SCH2 | Tube radio sheet | C48 and C52 read **0.0022uF**. | Both were first given as **0.002 µF** ceramics: C48 is the original three-legged part, C52 the one that was fitted. C52 was later settled as drawn, 0.0022 µF X1Y2. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows C48 at 0.002uF (three-lead), and C52 at 0.0022uF, which is right: C52 is the fitted X1Y2 part |
| SCH1 | Main sheet | The attenuator node reaches **LIN only**; PCM1802 RIN reaches no other pin. | Join **RIN to the LIN node**: in the radio the mono feed goes to the LIN+RIN node. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows the attenuator node carrying both LIN and RIN, joined by name to the PCM1802's LIN and RIN |
| SCE4 | Tube radio sheet | J53's Reference and Value are **hidden** in the 22:04 save. | Show J53's Reference and Value. | **DONE 2026-09-23** — the 2026-09-23 16:06:42 save shows J53's Reference and Value no longer hidden |

### The same, in plain words

> A readable copy of the record above: the same items, not more corrections. If the two ever differ, the record is right.

#### Main sheet — `Ambersong.kicad_sch`
- [x] **SC3** — The BT pair button's second leg goes to 3.3 V (A_VDC). It goes to GND. **Done 2026-09-23.**
- [x] **SC4** — The four digit-select GPIOs are in the wrong order. Truth: GPIO42 → digit 1, GPIO2 → digit 2, GPIO40 → digit 3, GPIO41 → digit 4. **Done 2026-09-23.**
- [x] **SC5** — The digit MOSFETs are spelled FPQ27P06 (4 values + 4 notes). The part is FQP27P06. **Done 2026-09-23.**
- [x] **SC6** — The volume pot wiper label reads A_IO34 (twice). It is GPIO35, so A_IO35. **Done 2026-09-23.**
- [x] **SC8** — The two 33 Ω clock resistors are drawn next to the ADC and the DAC. They sit at the A32's GPIO0. **Done 2026-09-23.**
- [x] **SC12** — Colon: 330 Ω corrected to 33 Ω, and the 1.5 kΩ drawn as a variable (R17). **Done.** Both resistors drawn on the 5 V side is intentional visual aide; the truth is the sub-sheet's order.
- [x] **SC13** — DAC → amp: the two 470 Ω are drawn at the DAC, with no plug. They are inside the cable, after a plug, as on the cable sheet. **Done 2026-09-23.**
- [x] **SC14** — RDA5807M: the 100 µF is drawn in series with the other two caps. All three go straight across its supply. **Done 2026-09-23.**
- [x] **SC15** — The 5 V bus says 12 positions. It has 15. **Done 2026-09-23.**
- [x] **SC19** — The "VOLUME POT" sheet box opens the box-fan harness. Move/rename it to the box-fan block. **Done 2026-09-23.**
- [x] **SC29** — 7-segment bus driver: leftover wires and dots join the A–F mid-nodes and their six 100 kΩ onto SEG_G_BUS. Remove them; the seven channels are separate, one 100 kΩ per segment to 5VDC. **Done 2026-09-23.**
- [x] **SC30** — Antenna jack: centre and shell are swapped. Centre = antenna, shell = earth. **Done 2026-09-23.**
- [x] **SCH1** — The radio attenuator node reaches the ADC's LIN only. Join RIN to that node: the radio feed is mono into LIN+RIN. **Done 2026-09-23.**
- [x] **SCE4** — J53's reference and value are hidden. **Done 2026-09-23.**
- [x] **SCH3** — The load across the radio transformer secondary reads 5W7Ω. Its marking is 5W7Ω5J: 5 W, 7.5 Ω, ±5%. **Done 2026-09-26:** it reads 5W7.5Ω; the ±5% is irrelevant.
- [x] **SC35** — Names: title "RDA5907M" → RDA5807M; the RDA sub-sheet's file was named after the tube radio (the 2026-09-14 21:35 save shows it renamed); the S3 symbol is a WROOM-2; "DS3132" → DS3231. **Done 2026-09-23.** (a), (b) and (d); **(c), the S3 symbol, closed not corrected, by choice.**
- [x] **SCD4** — The tube radio's sheet box reads "Lloyd's TM-838M Tube Radio". It is TM-838N. **Done 2026-09-23.**

#### S3 sub-sheet — MAIN MCU
- [x] **SC16** — The module's power pins are unconnected and C23/C25 are missing. Draw them as on the main sheet. **Done 2026-09-23.**
- [x] **SC35** — The file name contains "S#"; rename it "…ESP32-S3 N16R8". **Done 2026-09-23.**

#### A32 sub-sheet — AUDIO MCU
- [x] **SC6** — Pin 35 has no wire. Connect it to the volume-pot conductor A_IO35. **Done 2026-09-23.**
- [x] **SC7** — The conductor named A_IO16 is GPIO0's clock branch. **Done (the 18:38 save shows A_IO0).**
- [x] **SC17** — The 3V3 pin is marked no-connect and has no caps. It feeds A_3V3 with 0.1 µF + 10 µF + 10 µF. **Done 2026-09-23.**

#### BT button / LED / DPDT sub-sheet
- [x] **SC3** — Button and LED GPIOs are swapped. Button on GPIO25, LED cathode on GPIO33. **Done 2026-09-23.**

#### Panel radio LED PWM sub-sheet
- [x] **SC2** — The LED resistors said 680 Ω; they are 120 Ω, and the saved sheet shows 120 Ω. **Done 2026-09-23.**
- [x] **SC33** — Labels D and G were swapped; LED resistors go on D (drain), GPIO21 on G (gate), and the saved sheet shows them so. **Done 2026-09-23.**

#### 7-segment clock display sub-sheet
- [x] **SC12** — The variable resistor R1 had no value; it is stuck at 1.5 kΩ. **Done (the 19:08 save shows R1 = 1.5kΩ).**
- [x] **SCE2** — R1 is also the tube radio's volume pot. One of the two needs another reference. **Done 2026-09-23.**

#### PCM5102A DAC sub-sheet
- [x] **SC13** — No 470 Ω shown, and plug J40 (DAC_OUT) reaches no other pin or label. Draw it as the cable sheet does: plug DAC_OUT, then the two 470 Ω in the cable, to AMP_IN. **Done 2026-09-23.**

#### RDA5807M sub-sheet — file `Ambersong - RDA5807M.kicad_sch`
- [x] **SC14** — The 100 µF across the supply was missing. The saved sheet shows 100 µF, 10 µF and 0.1 µF across the supply. **Done 2026-09-23.**

#### P-MOSFET digits / drivers sub-sheet
- [x] **SC4** — Same digit-order fix as the main sheet. **Done 2026-09-23.**

#### USB-C sub-sheet
- [x] **SC24** — VBUS and GND are marked no-connect. They are carried through. **Done 2026-09-23.**

#### DS3231 RTC sub-sheet (file renamed; SC35 (d))
- [x] **SCV6** — The RTC I2C connector: its 4th position is not connected. As drawn, in the symbol's pin order, pins 1 and 2 are unconnected, pin 3 is SDA and pin 4 is SCL. Correct the connector. **Done 2026-09-23.**

#### Cable sheet — THIS (`Ambersong - CABLES.kicad_sch`)
- [x] **SC7** — The A_IO16 conductor renamed. **Done (the 18:38 save shows A_IO0).**
- [x] **SCV6** — Same RTC_I2C correction as on the RTC sub-sheet. **Done 2026-09-23.**

#### Tube radio sheet — `Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch`
- [x] **SCD4** — The mode-switch symbol is named Mine:TM-838M_UNIT1; it is the TM-838N. **Closed, not corrected, by choice:** it reads as the typo it is.
- [x] **SCE1** — The FM oscillator plate net has no label. Its wires still connect. **Done 2026-09-23.**
- [x] **SCE3** — X1 reads "Silicone Rectifier"; it is silicon. **Done 2026-09-23.**
- [x] **SCH2** — The heater bypasses C48 and C52 read 0.0022 µF. C48 is 0.002 µF, the original three-lead ceramic. **Done 2026-09-23.** C52 is 0.0022 µF, as drawn: the fitted X1Y2 part.

## 26. Not covered, open points and known differences

- **The amp's circuit** is not covered: §30 says what the amp is and how it connects, and no drawing here shows its boards.
- **Build details beyond §31:** wire colours (except the amp supply's four, §30), pin-1 sides, crimp/JST housings, individual
  cable lengths and cable routing, the screws of the PLA plates, and the contact order inside a connector. The printed parts
  are §31.7.
- **The tube radio's speakers SP1/SP2**, drawn in the section the sheet calls ORIGINAL, are not recorded as fitted or absent.
- **The original documents** (the factory schematic and the SAMS schematic and parts tables) are scans. Their values reach
  this document only through the tube radio sheet's hidden Spec fields and text notes.
- **RTC datasheet facts** are not included (see §24).
- **GPIO48** is both the S3 board's RGB LED and the RDA5807M's SCLK (main sheet). GPIO48 controls that LED; it was off before and is bright now.
- **DevKitC-1 power inputs.** Espressif's DevKitC-1 guide says the USB ports, the 5V pin and the 3V3 pin are "three mutually
  exclusive ways to provide power to the board". In this machine VBUS reaches the back panel while 5VDC feeds the 5V pin. The
  fitted S3 board is marked "YD-ESP32-23 2022-v1.3". It is compatible with the DevKitC-1 in firmware and pin-compatible for
  every pin the device uses; the pins it does not use are not checked. The power-input rule above is the guide's.
- **S3 module FCC grantee code** 2AD6X differs from Espressif's (2AC7Z, per an esp32.com forum thread; a thread about 2AD6X
  itself could not be re-found); the module maker is unidentified.
- **Firmware text that disagrees with this document**, quoted from the firmware source (the firmware's to change). Two are
  comments: "680 ohm on the 5 V rail" (include/pins.h; the panel-lamp LED resistors are 120 Ω) and "4.7k pull-ups to
  S3_3V3" (include/pins.h; the AS5600 pull-ups are 10 kΩ). Two are defines: A32_PCM5102_XSMT 16 (include/pins.h),
  which src/a32/audio.cpp sets as an output and drives HIGH, while GPIO16 is unused and XSMT is pulled up on the DAC
  module itself, so that code drives nothing; and S3_LIMIT_RIGHT 4 (include/pins.h), used nowhere in src/ or include/,
  while GPIO4 goes nowhere else.
- **A32 GPIO34, GPIO39 and GPIO36.** An earlier record, and pins.h as it read on 2026-09-12, say they were
  measured continuous on this board. They are not joined now; the drawings show GPIO34 and GPIO39 no-connect and GPIO36 on
  the source-select ladder. The pins.h comment is the firmware's to change.
- **C11's factory original.** An earlier reading of the two original drawings gave C11 as a shunt in both. C11's own Spec
  field reads "Factory SERIES AFC coupling; SAMS SHUNT.", and the tube radio sheet's note at (52.32,49.28) reads "Factory
  AFC series C11 and oscillator feedback differ from SAMS." They disagree about the factory drawing. The fitted C11 is a
  shunt from AFC_C to chassis, and always was; what the factory drawing shows is still open.

## 29. The FM antenna, as built and installed (2026-09-20)

*Specific to this build — adapt: the dimensions, mounting and aim suit this house and this location.*

The author's own figures are in 29.1 and 29.2. Section 29.3 gives further details from the build notes, which the author
reviewed (he corrected the conductor spacing, the earth lug and the boom profile) but did not confirm one by one.

### 29.1 The aerial

- **A three-element Yagi**, horizontal, on the roof.
- **Director:** plain aluminium U-channel, 3.2 mm thick, 12.6 mm high, 25.5 mm wide, **133.8 cm** long.
- **Reflector:** the same U-channel material, **150.9 cm** long.
- **Driven element:** **1/2 in type L copper tube**, 139 cm per element, folded: the ends are soldered with 90-degree
  tubing joining both tubes, **about 1390 mm tip to tip**, the two conductors **50 mm** apart centre to centre.
- **Boom:** **3030 aluminium extrusion, 1170 mm**.
- **Nothing touches the boom electrically:** the director and reflector touch neither the boom nor anything else; only the folded
  driver is wired to the coax; the driver itself reaches the boom only through a non-conductive clamp.

### 29.2 Feedline, earth and mounting

- **Feedline:** coax, **at least 100 ft at present**, to be about **50 ft total** at its final place, bulkhead to the radio.
- **Earth:** **8 AWG stranded copper, ferruled, to the Hydro-Québec meter box**, landed **on a lug on the meter base,
  beside one an electrician made for the external wiring**.
- **Grounding block:** fitted outside, out of the weather; its exact position does not matter.
- **Mast:** the same 3030 extrusion as the boom, **4 ft high**, fixed with continuous aluminium extrusion on the roof; the mast is
  centred on the boom.
- **The boom is continuous metal through the mast to the roof metal.** **The metal roof is electrically connected to
  the electrical mast, and so to earth**, so the boom and the coax shield's 8 AWG run to the meter-base lug reach
  the same electrode system.
- **Height and aim:** about **4 ft over its base and 1 ft over the tip of the roof**, clear of roof reflections; the slope faces
  away from Mount Royal and the aerial points **as near Mount Royal as it can**.
- **Guys:** Dacron lines, **two from each end of the boom**; as fitted, the antenna cannot move short of a very big accident.
- **The radio end** is not in this section. It is §13 and §21 — J53, C3, L2 and COAX_SHIELD_PE, the shield reaching
  CHASSIS only through C51. A temporary braid-to-chassis bond made on 2026-09-04 does not change that.

### 29.3 Further details from the build notes

Not confirmed one by one by the author; weaker than 29.1 and 29.2.

- Element positions along the boom: **0 / 55 / 110 cm** — reflector, driven, director.
- Both U-channels mounted opening downwards; the three element centres coplanar within 5 mm.
- **Feed gap 20 mm**, cut in one tube only.
- Mounting, top down: nut, Belleville washer, neoprene washer (the metal-roofing-screw type), G10 washer, the element in an ASA
  bushing, an ASA plate continuous from that bushing, then the boom through a 90-degree clamp. The driven element is mounted the
  same way. **No 1 MΩ static bleed resistors** are fitted from the parasitics to the boom.
- **Feedpoint:** an F-81 bulkhead in the ASA clamp; the shell on one side of the 20 mm gap, the centre pin on the other. **Six of
  the salvaged HDMI ferrite sleeves** (17.4 mm OD, 9.7 mm ID, 28.5 mm long) are on the feedpoint as a common-mode choke.
- **Feedline RG-6.** **Grounding block: Perfect Vision PVGB1HFWS**, UL listed, 3 GHz, F-type, single, with a weather boot — a
  plain block, with **no gas discharge tube**.

### 29.4 What follows, and what is not recorded

- **Reasoned, from 29.1–29.3:** the boom is metal through the mast to the roof metal, while the elements sit on
  insulating bushings, so the elements are isolated from that grounded structure. The coax shield's earth is the 8 AWG run to the
  meter-base lug, not a separate boom wire.
- **Not recorded:** what the antenna measures — no SWR, no field strength, no reception comparison.
- **No drawing shows the antenna.** No `.kicad_sch` draws it. The main sheet's ANTENNA block (J10) and the tube
  radio's J53 are the radio end.

## 30. The amp, its speakers and its input panel

*Specific to this speaker set; adapt it to yours.*

**Scope.** What the amp and its speakers are, the panel, and what has been measured at it. **The amp's circuit is not
covered**: no drawing here shows its boards.

### 30.1 What the panel does

- **It sums its inputs; it does not select between them.** If the AUX, the RCA and the balanced input all carry sound at
  the same time, the output at the amp is their sum.
- **Three inputs:** a TRS jack (AUX), a balanced input (L+, L−, GND, R−, R+) and an RCA pair.
- **The AUX and RCA jacks are grounded when nothing is plugged into them.** The balanced input is not.
- **The amp's own volume control does not reach silence at its minimum.**
- The DAC reaches this panel through the balanced input, by the cable of §6 and §12, which now carries a 470 Ω in each of its
  four signal legs and six ferrite cores. AUX reaches the same panel as analogue audio, never through the A32 (§22).
- **Observed (2026-09-20 to 2026-09-22):** a frying noise heard at the amp went away when the DAC-to-amp cable was unplugged
  at the amp, and almost went away when it was unplugged at the DAC end. After the clip-on ferrites went onto the
  radio-to-ADC cable, no pops were audible, before the 470 Ω pair and the HDMI ferrites were added to the DAC-to-amp cable.

### 30.2 What was measured at the panel and on the cable

- **Measured 2026-09-21**, multimeter, resistance, one reading each, with the DAC-to-amp cable unplugged at the amp: **L− to
  AMP_G and R− to AMP_G both read open**, 3 MΩ and rising as a capacitor charged.
- **Measured 2026-09-21 on the DAC-to-amp cable**, unplugged, before the 470 Ω pair on L+ and R+ went in: L− to GND 470 Ω,
  R− to GND 470 Ω, L− to R− a little under 1 kΩ, L+ to GND 4.2 MΩ, R+ to GND 4.2 MΩ.
- **What that supports and no more:** at the amp connector each − leg has no DC path to the amp's ground, which is what a
  differential, AC-coupled input looks like on a meter. The reading is one meter reading on one day; nothing inside the amp was
  seen.

### 30.3 What the amp and speakers are

- **Source:** the amp and speakers came from a **Kramer Tavor 5-O** powered speaker pair, P/N 60-00009010: active left, passive
  right, 2 × 30 W RMS, 4 Ω.
- **Speakers:** each side has a 5.25 in polypropylene woofer and a 0.5 in Mylar dome tweeter, with a first-order passive crossover
  at 8 kHz. The original cabinets were ported; this device uses **passive radiators**, added by the author, and is not ported.
  Of the original cabinets only the active one's back aluminium plate remains — the earth plate of §30.4.
- **The four ICs:** a **TDA7265** power stage, and a **TL074**, a **TL072** and a **C4558** on the input board.
- **Supply:** an isolated offline switching supply, measured at ±19.33 V with no load; two TO-220 regulators make ±12 V for the
  op-amps. Its outputs reach the amp on four wires: **red +20 V, the two middle black wires GND, yellow −20 V** — the
  author's reading, not verified inside the supply. The same supply feeds the box fan and the power detect (§7).
- **Auto mode** has been removed.
- **Volume pot:** the amp's original dual-gang volume pot has been replaced with a **B10k dual-gang pot** with a power switch
  that is not used; its two gangs are separate, as the original's were. It is on the amp's PCB and is reached only from the
  back. An earlier setup took the original pot off the PCB and cabled it to the front panel; that setup is gone. The front
  panel's volume knob is not this pot (§31.3).

### 30.4 The amp's ground and the earth plate (2026-09-23)

- **The amp's GND is earthed through its Bass knob:** a washer on the Bass pot bonds it to the back aluminium plate, which carries
  the earth star point. This is the original design of the Tavor speaker/amp system.
- **So the DC-side ground is on mains earth at almost 0 Ω:** the ground of the S3, the A32, the amp and the rest of the DC side.
  No drawing shows this joint, and by which path the §10 GND buses reach the amp's ground is not stated.
- **The amp's volume and treble pots do not touch that plate** electrically.
- **The amp's ±20 V PSU is insulated from that plate** by plastic screws and standoffs.
- The tube radio's CHASSIS is not part of this: it stays on neutral and is never bonded to Earth (§22).
- "Almost 0 Ω" is the author's figure; the instrument used is not recorded.
- **An earlier record disagrees:** a safety check on 2026-08-26 recorded "System GND -> earth — OPEN, megohms — pass", and an
  earlier design rule kept every GND off anything earthed. The statement above is later (2026-09-23). This document nowhere
  states that GND is isolated from earth; the only isolation it states is CHASSIS from Earth.

---

## 31. The physical build (2026-09-27)

*Specific to this build — adapt: a reader's cabinet, panels and mounts will differ.*

**Scope.** Where things are and how they are held, so that someone can open the box, find a part, and repair or rebuild it.
Mentioned, not detailed: no screw details for the PLA plates, no cable routing, no individual cable lengths.

### 31.1 Before opening it — mains

- **The tube radio has no isolation transformer. CHASSIS, its circuit common, is on mains neutral** (J4 from AC_N through the
  external EMI filter), and is never bonded to Earth or the Faraday cages (§21, §22). CHASSIS is at neutral only while the
  outlet's live and neutral are the right way round; nothing in the device checks that.
- **The power switch SW1 does not remove mains from the box.** The inlet's live (AC_L_IN) goes to SW1 and also, before SW1, to the
  always-on 5 V PSU U4 (§10). Only unplugging the inlet removes mains.
- The tube radio's B+ is made from the mains by a half-wave rectifier (X1, §21); it is present whenever the radio is on.
- The DC side's GND is on mains earth through the amp's back plate (§30.4). The tube radio's CHASSIS is not.

### 31.2 The cabinet and how to open it

- **A wooden cabinet** holds everything. The tube radio's original case has been stripped; its **original chassis** is in the cabinet.
- **Back:** an inverted-T ½ in plywood panel behind the radio, held by ten countersunk 3 mm screws.
- **Front panel:** held from the inside by five 3 mm screws.
- **Tube radio:** held by two #8 ½ in wood screws through its tabs, one each side.
- **Everything else:** almost every board and mechanical part is on a small perfboard or module in its own PLA case, or on a PLA
  carrying plate, screwed to the cabinet with #8 screws.
- **Cables:** every cable inside is under 14 in, long enough to service the box with panels open and nothing unplugged, and each
  is marked at both ends with an evident name or its name on the cable sheet.
- **Cooling:** box fan M1 (§7, on the main sheet) sits at the top of the back panel and blows air out; air comes in through two
  small louvred PLA vents at the other two ends of the inverted T. Cage fan M2 (§7, on 5VDC) is screwed to the PLA Faraday
  cage and blows air into it.

### 31.3 The front panel

The tuning knob; the volume knob; the source switch; the power switch; the BT LED, which is also the BT button; the Leditron
clock display (four seven-segment digits and the colon, §8); the four FM panel LEDs (§11); and the FM panel, where the needle
shows the rough FM position against a blackened brass plate. The source switch and the power switch are fixed in the
wooden front panel itself, each by its own nut and then plenty of hot glue.

The volume knob is RV4, the A32's volume pot (§7), not the amp's; the amp's own volume pot is reached only from the back
(§30.3). The PLA shaft extension of §31.4 is for tuning only.

### 31.4 Tuning, the needle and the index

- **Tuning:** the tube radio's original tuning knob is turned from the front through a PLA shaft extension; that knob drives the
  dial cord, which turns the tuning capacitor's shaft through its wheel. The cord drives nothing else. Clockwise raises the
  frequency.
- **AS5600 (§11):** mounted beside the tuning capacitor's shaft, its board perpendicular to the shaft, the shaft's axis through
  the IC's centre. A PLA shaft extends the capacitor's shaft almost to the IC; the magnet (6 × 3 mm disc neodymium, less than
  0.5 mm from the IC) is epoxied to its end. **The magnet must be diametrically magnetised.**
- **Needle:** the tube radio's original brass needle, on a PLA mount on the stepper's shaft (28BYJ-48, §11). The stepper alone
  drives it; it has never been on the dial cord. What the stepper is told to do is in `FIRMWARE-GOSPEL.md`.
- **Index:** a small arm on the opposite side of the same PLA mount carries a square neodymium magnet, about 3 × 3 mm and 0.2 mm
  thick, which passes over the A3144 Hall switch (§11, GPIO5). The A3144 is on a small perfboard in a PLA case. Which pole faces
  the A3144 is not recorded.
- The old limit-switch input GPIO4 keeps its pull-up and goes nowhere (§22).

### 31.5 The Faraday cages

- **Over the tube radio:** a PLA box lined with aluminium tape, the tape covered with Kapton tape, well clear of the tube radio.
  Earth reaches the aluminium through a stranded green wire held against it by copper tape with conductive adhesive.
- **Over the AS5600:** built the same way; the AS5600 is mounted to the cage's base, clear of the aluminium and copper tape.
- **Over the AC line filter (§10):** Kapton tape over the circuit, then aluminium tape, then Kapton tape again; earthed the same
  way as the others.
- All three are on Earth (§10, §23).

### 31.6 Connectors

- The mains inlet J30 is a standard **IEC C14**; it is the inlet that came on the Tavor's aluminium back plate (§30.3).
- The cable sheet defines the important and less evident cables (§12). Some connector types are used once only, so that one
  cable cannot be plugged where it would break something — for example the AC input to the front power switch.

### 31.7 Printed parts

Every printed part of the current build is an STL in **`hardware/STLs/`**, named as below. All are PLA except the passive
radiator's membrane, which is TPU. 76 files. They are the author's parts for this cabinet and these boards: adapt them to yours.

- **Speakers (§30.3)**
  - `AUDIO - Passive Radiator - Holder.stl` — passive radiator: holder
  - `AUDIO - Passive Radiator - Inside Protector.stl` — passive radiator: inside protector
  - `AUDIO - Passive Radiator - Membrane Weight.stl` — passive radiator: membrane weight
  - `AUDIO - Passive Radiator - Outside Protector.stl` — passive radiator: outside protector
  - `AUDIO - Passive Radiator - TPU Membrane.stl` — passive radiator: membrane, **printed in TPU**
  - `AUDIO - Speakers Holder and Seal.stl` — speaker holder and seal
- **Back panel (§31.2)**
  - `BACK - Dual USB-C Ports.stl` — the two back-panel USB-C ports (§4)
  - `BACK - Electronics - BNC Antenna Interface.stl` — the antenna connector J10, a BNC (§10, §22)
  - `BACK - Fan Exhaust.stl` — box fan M1's exhaust
  - `BACK - Fan Intake.stl` — the louvred air intakes
  - `BACK - Panel Anchors Bottom.stl` — back panel anchors, bottom
  - `BACK - Panel Anchors Inner Corners.stl` — back panel anchors, inner corners
  - `BACK - Panel Anchors Outer Corners.stl` — back panel anchors, outer corners
  - `BACK - Panel Anchors.stl` — back panel anchors
- **Front panel (§31.3, §31.4)**
  - `FRONT - FM Dial - Back.stl` — FM dial back; it is also the stepper motor's mount
  - `FRONT - FM Dial - Cover.stl` — FM dial cover
  - `FRONT - FM Dial - Spacer.stl` — FM dial spacer
  - `FRONT - FM Dial - Stepper Needle and Magnet Holder.stl` — the mount on the stepper's shaft carrying the needle and the index magnet
  - `FRONT - Panel Anchors - Insidemost.stl` — front panel anchors, insidemost
  - `FRONT - Panel Anchors - Outsidemost.stl` — front panel anchors, outsidemost
  - `FRONT - Tuner Pot Knob.stl` — the front tuning knob
  - `FRONT - Tuning Pot Extender PLA Shaft.stl` — the tuning shaft extension
  - `FRONT - Tuning Shaft Bearing Seat.stl` — the tuning shaft's bearing seat
  - `FRONT - Volume Pot Knob.stl` — the front volume knob, on RV4
- **Board cases and holders (inside)**
  - `IN - Electronics - 5V BUS case Bottom.stl` — 5 V bus case, bottom
  - `IN - Electronics - 5V BUS case Top.stl` — 5 V bus case, top
  - `IN - Electronics - 5V PSU Case Bottom.stl` — 5 V PSU case, bottom
  - `IN - Electronics - 5V PSU Case Top.stl` — 5 V PSU case, top
  - `IN - Electronics - 5V+3V3 BUS case Bottom.stl` — 5 V + 3.3 V bus case, bottom
  - `IN - Electronics - 5V+3V3 BUS case Top.stl` — 5 V + 3.3 V bus case, top
  - `IN - Electronics - AC Line Filter Case.stl` — AC line filter case (§31.5)
  - `IN - Electronics - AS5600 Case - Bottom.stl` — the AS5600's cage, bottom (§31.5)
  - `IN - Electronics - AS5600 Case - Connecting Rod.stl` — the PLA shaft that carries the AS5600's magnet (§31.4)
  - `IN - Electronics - AS5600 Case - Top.stl` — the AS5600's cage, top
  - `IN - Electronics - Audio Listener Case BOTTOM.stl` — the source switch's IN/LISTENER/OUT board case, bottom (§11)
  - `IN - Electronics - Audio Listener Case TOP.stl` — the source switch's IN/LISTENER/OUT board case, top
  - `IN - Electronics - Audio MCU Case BOTTOM.stl` — A32 case, bottom
  - `IN - Electronics - Audio MCU Case TOP.stl` — A32 case, top
  - `IN - Electronics - Backing Panel 1.stl` — a panel that holds several of the cases
  - `IN - Electronics - Bluetooth LED Holder.stl` — BT LED holder
  - `IN - Electronics - Bluetooth LED Pusher.stl` — BT LED pusher (the LED is also the button)
  - `IN - Electronics - Box Fan LM7812 Case Bottom.stl` — box fan LM7812 case, bottom (§7)
  - `IN - Electronics - Box Fan LM7812 Case Top.stl` — box fan LM7812 case, top
  - `IN - Electronics - DS3231 RTC Case BOTTOM.stl` — DS3231 RTC case, bottom
  - `IN - Electronics - DS3231 RTC Case TOP.stl` — DS3231 RTC case, top
  - `IN - Electronics - Digit Drivers Case BOTTOM.stl` — digit drivers case, bottom (§9)
  - `IN - Electronics - Digit Drivers Case TOP.stl` — digit drivers case, top
  - `IN - Electronics - Front Panel FM LEDs Case BOTTOM.stl` — FM panel LEDs case, bottom (§11)
  - `IN - Electronics - Front Panel FM LEDs Case TOP.stl` — FM panel LEDs case, top
  - `IN - Electronics - Hall Sensor Stepper Limit Case.stl` — the A3144's case (§31.4)
  - `IN - Electronics - LEDITRON Back.stl` — Leditron display back
  - `IN - Electronics - LEDITRON Bus Case Bottom.stl` — Leditron bus case, bottom
  - `IN - Electronics - LEDITRON Bus Case TOP.stl` — Leditron bus case, top
  - `IN - Electronics - PCM1802 ADC Case BOTTOM.stl` — PCM1802 ADC case, bottom
  - `IN - Electronics - PCM1802 ADC Case TOP.stl` — PCM1802 ADC case, top
  - `IN - Electronics - PCM5102A DAC Case Bottom.stl` — PCM5102A DAC case, bottom
  - `IN - Electronics - PCM5102A DAC Case TOP.stl` — PCM5102A DAC case, top
  - `IN - Electronics - Power Detect Case BOTTOM.stl` — power detect case, bottom (§7)
  - `IN - Electronics - Power Detect Case TOP.stl` — power detect case, top
  - `IN - Electronics - RDA5807M - Bottom.stl` — RDA5807M case, bottom
  - `IN - Electronics - RDA5807M - Cage Holder.stl` — RDA5807M cage holder
  - `IN - Electronics - RDA5807M - Middle Bottom.stl` — RDA5807M case, middle bottom
  - `IN - Electronics - RDA5807M - Middle TOP.stl` — RDA5807M case, middle top
  - `IN - Electronics - RDA5807M - TOP.stl` — RDA5807M case, top
  - `IN - Electronics - S3 Main MCU Backing.stl` — S3 backing
  - `IN - Electronics - S3 Main MCU Case BOTTOM.stl` — S3 case, bottom
  - `IN - Electronics - S3 Main MCU Case TOP.stl` — S3 case, top
  - `IN - Electronics - Stepper Driver ULN2003A Case - Bottom.stl` — stepper driver (ULN2003A) case, bottom (§11)
  - `IN - Electronics - Stepper Driver ULN2003A Case - Top.stl` — stepper driver (ULN2003A) case, top
- **Tube radio (§31.5)**
  - `IN - Tube Radio - Faraday Cage 1.stl` — the tube radio's Faraday cage, part 1; cage fan M2's mount is part of the cage
  - `IN - Tube Radio - Faraday Cage 2.stl` — the tube radio's Faraday cage, part 2
- **Leditron display (§8)**
  - `LEDITRON - Colon - Blanket.stl` — colon blanket
  - `LEDITRON - Colon.stl` — colon
  - `LEDITRON - Digit.stl` — digit
  - `LEDITRON - Front Case.stl` — front case
  - `LEDITRON - Glass Spacer.stl` — glass spacer

Not printed: the source and power switches (in the wooden front panel), the IEC inlet (on the Tavor's back plate), and
the amp's ±20 V PSU standoffs, which are plastic screws and standoffs (§30.4).
