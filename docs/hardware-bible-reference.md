# Ambersong: Hardware Bible — schematic notes, datasheet facts and open points

This file collects the material that supports the rest of the Hardware Bible rather than describing one circuit
stage on its own: the handwritten notes carried on the main Ambersong schematic sheet, manufacturer datasheet facts
about the parts used throughout the build, the two schematic naming quirks that remain by choice, and the points
the Bible does not cover or cannot yet answer. It has no parts list or wiring of its own.

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

This file describes the current build only (hardware revision A); it is data, not instructions. Check every value
against the schematic (`hardware/schematics/Ambersong.kicad_sch`) before powering or flashing. "Not recorded" means
the project has no value; do not fill it in.

Terms used in this file: **A32** is the audio ESP32 board; **S3** is the main ESP32-S3 board. The parts, wiring and
tables for the ESP32 boards, display, stepper, amp and the rest of the machine are in `docs/hardware-bible.md`; the
tube radio chassis (parts, wiring, heater string, mode supply, mains supply and B+) is in
`docs/hardware-bible-tube-radio.md`; the cable harnesses are in `docs/hardware-bible-cables.md`. This file is the
place for schematic notes, datasheet evidence and anything not covered or not yet settled; it has no wiring tables
of its own, so it points to those other files by name rather than repeating their content.

## Ambersong hardware: notes written on the main schematic sheet

These are the handwritten notes on the main Ambersong sheet (as distinct from the tube radio sheet's own notes,
which are given alongside the relevant stage in `docs/hardware-bible-tube-radio.md`). Most restate a fact that is
already given, as a wiring table, in `docs/hardware-bible.md` or `docs/hardware-bible-cables.md`; they are kept
here as the sheet's own words.

- "There are two volume knobs, one is in the back at the amp's analog and the other is at the front, digital
  only." — one volume control is the amp's own analogue knob; the other is a digital control read by the A32.
- "USB-C connections from back of box goes to their respective MCU's USB-C connector." — the back-panel USB-C
  connectors pass straight through to the matching MCU board's own USB-C port; see `docs/hardware-bible.md` for
  the wiring.
- "All the AMP_ prefixes on labels go their respective pins on the small PCB where AUX inputs go." — every signal
  name starting `AMP_` ends at the small board that carries the amplifier's AUX inputs.
- "The AMP PSU 20V_GND is shown for completeness only; the actual path goes to the AMP's -20V and +20V inputs." —
  the amp's power-supply ground label is drawn for clarity; the real return path is through the amp's own ±20 V
  input pins.
- "Caution! This 5VDC cable's polarity is reversed at the COLON_IN side compared to others!" — written on the cable
  sheet, next to the colon LED's power cable; see `docs/hardware-bible-cables.md` for the wiring this warns about.
- "All the grounds are joined at a single starpoint, represented by the JST 2PINS buses" — every ground point in
  the machine returns through the 5 V ground bus, drawn as a row of two-pin JST connectors.
- "The faraday cages over the whole of the tube radio and over the AS5600 are both connected to EARTH" — both
  cages are bonded to earth; see `docs/hardware-bible-tube-radio.md` for how the tube radio's own circuit common
  relates to that earth (it is never bonded to it).
- "Where possible and adequate, all cables are twisted pairs, shielded and/or foiled. Representations are in
  sheet." — the cable sheet's own drawings show which cables are twisted, shielded or foiled; see
  `docs/hardware-bible-cables.md`.

## Ambersong hardware: schematic naming quirks left uncorrected by choice

Two places in the schematic carry an old typo that is left exactly as it is, because it plainly reads as the typo
it is and correcting it would not change anything electrical:

- On the main sheet, both ESP32-S3 symbols (U7 and U63) use the KiCad library symbol
  `PCM_Espressif:ESP32-S3-WROOM-2`, with their Value field reading `ESP32-S3-WROOM-1_N16R8`. The fitted module is
  marked ESP32-S3-N16R8 (the WROOM-1 series, not WROOM-2); only the symbol's library name and its Value field
  disagree with each other, not the drawn pinout.
- On the tube radio sheet, the mode-switch units S1A–S1G use the KiCad library symbol `Mine:TM-838M_UNIT1` — an
  M-for-N typo, since the chassis is a TM-838N, matching every other part's `Source` field on that sheet.

Every other naming or wiring mismatch once found while drawing the schematic has since been corrected in the
schematic itself, so the current schematic carries no further open errors of that kind.

## Ambersong hardware: datasheet facts — main MCU (ESP32-S3) and the back-panel USB-C

The fitted S3 module is marked "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3"; chip identity read over serial is
"ESP32-S3 QFN56 rev v0.2, embedded PSRAM 8 MB". Facts below describe either that exact module/chip, or a reference
document (the WROOM-2 datasheet, an Espressif DevKitC-1 reference board) kept for comparison; each line says which.

- **ESP32-S3-WROOM-1-N16R8: 16 MB Quad flash, 8 MB Octal PSRAM, −40…65 °C** — the fitted module's own part number.
  Evidence: espressif ESP32-S3-WROOM-1/1U datasheet (v1.8) — "ESP32-S3-WROOM-1-N16R8 16 MB (Quad SPI) 8 MB (Octal
  SPI) –40 ~65".
- **IO35–IO37 are taken internally by the Octal PSRAM on an R8 module and are not available for other uses.**
  Evidence: same datasheet.
- **GPIO47/48 run at 1.8 V only on an R16V(A) module; on this N16R8 module they are normal I/O voltage (an
  inference, not stated directly for N16R8).** Evidence: same datasheet.
- **The bare ESP32-S3 chip (not the module): VDD_SPI 3.3 V on the R8 variant.** Evidence: ESP32-S3 datasheet
  (v2.2), Table 1-1.
- **WROOM-2 module (reference only, not fitted): embeds R8V/R16V variants only, all with octal flash; there is no
  WROOM-2 ordering code matching this build's N16R8 without a trailing V.** Evidence: ESP32-S3-WROOM-2 datasheet
  (v1.7).
- **WROOM-2: GPIO47/48 run at 1.8 V on both its variants.** Evidence: same datasheet.
- **WROOM-2: pins 28–30 are no-connect; it has no IO35–37 pads.** Evidence: same datasheet (contrast: the fitted
  module's symbol on the main sheet does use pins 35–37).
- **WROOM-1: 41 pins total (GND, 3V3, EN, GPIOs, exposed pad); no 5 V pin and no RST pin on the module itself.**
  Evidence: ESP32-S3-WROOM-1/1U datasheet.
- **EN must never be left floating, on either WROOM-1 or WROOM-2.** Evidence: both datasheets, same wording.
- **VDD33 operating range 3.0–3.6 V, absolute maximum 3.6 V, on either WROOM-1 or WROOM-2.** Evidence: both
  datasheets.
- **Strapping pins: GPIO0 weak pull-up, GPIO3 floating, GPIO45/46 weak pull-down (chip defaults).** Evidence:
  ESP32-S3 datasheet.
- **GPIO19 = USB_D−, GPIO20 = USB_D+, both default to the USB Serial/JTAG controller.** Evidence: ESP32-S3
  datasheet; confirmed again in the ESP-IDF GPIO reference, which adds that reconfiguring them as plain GPIOs
  disables USB-JTAG.
- **GPIO39–42 are the JTAG signals MTCK/MTDO/MTDI/MTMS.** Evidence: ESP32-S3 datasheet.
- **UART0: TXD0 on GPIO43, RXD0 on GPIO44.** Evidence: ESP32-S3-WROOM-1/1U datasheet.
- **Internal weak pull-up/pull-down resistors are about 45 kΩ.** Evidence: ESP32-S3-WROOM-1/1U datasheet.
- **Espressif DevKitC-1 reference board (not the fitted board, but pin-compatible with it in firmware for every
  pin this build uses): header rows match the fitted module's pins, with header pin 3 = EN and header pin 21 =
  5 V.** Evidence: Espressif ESP32-S3-DevKitC-1 user guide, v1.1.
- **DevKitC-1: the on-board RGB LED is on GPIO48 in the initial board revision, GPIO38 in revision v1.1. This
  build's fitted board uses GPIO48.** Evidence: same user guide.
- **DevKitC-1: three mutually exclusive power inputs — USB, the 5 V header pin, or the 3V3 header pin; never more
  than one at a time.** Evidence: same user guide.
- **DevKitC-1: a Micro-USB UART bridge port and a native USB OTG port, both usable for power, flashing and (OTG
  only) JTAG.** Evidence: same user guide.
- **DevKitC-1: on an octal-flash/PSRAM WROOM-1 or WROOM-2 board, GPIO35–37 are reserved internally and unavailable
  externally.** Evidence: same user guide.

## Ambersong hardware: datasheet facts — audio MCU (A32, ESP32-WROOM-32) and the DS3231 RTC

The A32 module is meant to be an ESP32-WROOM-32E; its marking reads "FCC IDL 2BB77-ESP32-32X" (board or module
identity not otherwise stated). The RTC datasheet is left out of this file: the fitted RTC is a DS3231 ZS-042-style
module whose own datasheet could not be re-opened; see `docs/hardware-bible.md` for what is fitted.

- **GPIO6–11 are connected to the module's own SPI flash and are not recommended for other use.** Evidence:
  ESP32-WROOM-32 datasheet (v3.7).
- **Strapping pins GPIO0 (pull-up) and GPIO2 (pull-down); MTDI = GPIO12, MTDO = GPIO15.** Evidence: same datasheet.
- **GPIO36/39/34/35 are input-only (type I), all on ADC1.** Evidence: same datasheet.
- **VDD33 operating range 3.0/3.3/3.6 V, absolute maximum −0.3…3.6 V.** Evidence: same datasheet.
- **Espressif DevKitC V4 reference board (not confirmed to match the fitted board): three mutually exclusive power
  inputs (USB, 5 V header, 3V3 header).** Evidence: Espressif ESP32-DevKitC user guide.
- **DevKitC V4: header rows 3V3, GND, 5V, TX, RX, IO0 at stated pin numbers.** Evidence: same user guide.
- **DevKitC V4: GPIO16/17 are only usable on WROOM/SOLO-1 modules, not on WROVER modules; a component (C15) can
  interfere with boot mode or with a clock signal output on GPIO0 on earlier board revisions.** Evidence: same
  user guide.

## Ambersong hardware: datasheet facts — audio converters (PCM1802 ADC, PCM5102A DAC)

- **PCM1802: analogue supply (VCC) 5 V, digital supply (VDD) 3.3 V.** Evidence: TI PCM1802 datasheet (SLES023D).
- **PCM1802: PDWN, BYPAS, OSR, FMT0, FMT1, MODE0 and MODE1 each have an internal ≈50 kΩ pulldown and are 5 V
  tolerant; PDWN is active-low.** Evidence: same datasheet.
- **PCM1802: with FMT1 low and FMT0 high, the data format is I2S, 24-bit (Table 6). That FMT0 is wired high on
  this build is an inference: FMT0 is tied only to the module's own 3.3 V pin.** Evidence: same datasheet.
- **PCM1802: with MODE1 and MODE0 both low, the part runs in slave mode (Table 9).** Evidence: same datasheet.
- **PCM1802: in slave mode, FSYNC enables the BCK signal and the part shifts out converted data while FSYNC is
  high.** Evidence: same datasheet.
- **PCM1802: OSR low selects ×64 fS oversampling; BYPAS low selects the normal (DC-cut) high-pass filter mode.**
  Evidence: same datasheet.
- **PCM1802: SCKI (system clock input) is 5 V tolerant.** Evidence: same datasheet.
- **PCM5102A: XSMT is the soft-mute control; where it is not needed it may be tied directly to AVDD.** Evidence:
  TI PCM5102A datasheet (SLAS859C).
- **PCM5102A: after 1024 LRCK periods of continuous zero data on both channels (21 ms at 48 kHz), the part enters
  a full analogue mute.** Evidence: same datasheet.
- **PCM5102A: DEMP, FLT and FMT are all active-low control inputs (de-emphasis, filter latency, I2S vs
  left-justified format).** Evidence: same datasheet.
- **PCM5102A: pin numbers OUTL 6, OUTR 7, AGND 9, SCK 12, BCK 13, DIN 14, LRCK 15; all digital inputs are failsafe
  Schmitt-trigger LVCMOS.** Evidence: same datasheet — these pin numbers match the part as drawn on the main
  sheet.
- **PCM5102A: the internal PLL is disabled whenever an external SCK is supplied; if SCK instead sits at ground for
  16 LRCK periods while BCK and LRCK run correctly, the internal PLL starts and generates SCK itself.** Evidence:
  same datasheet.
- **PCM5102A: AVDD 3.3 V, DVDD 1.8 or 3.3 V, absolute maximum supply voltage 3.9 V.** Evidence: same datasheet.

## Ambersong hardware: datasheet facts — volume pot, amp PSU, power detect, box and cage fans

- **PC817 (Sharp): DIP-4 pinout, pin 1 anode, pin 2 cathode, pin 3 emitter, pin 4 collector.** Evidence: Sharp
  PC817XxNSZ1B datasheet.
- **PC817: absolute maximum ratings — forward current 50 mA (peak 1 A), reverse voltage 6 V, power dissipation
  70 mW, collector-emitter voltage 80 V, emitter-collector voltage 6 V, collector current 50 mA.** Evidence: same
  datasheet.
- **PC817: current transfer ratio 50–400% at IF = 5 mA, VCE = 5 V, 25 °C; minimum 50%.** Evidence: same datasheet.
- **LM7812 (TO-220): pin 1 input, pin 2 ground, pin 3 output.** Evidence: TI LM78xx datasheet — matches the main
  sheet's own pin labelling.
- **LM7812: output 11.4–12.6 V over 14.5 V ≤ VIN ≤ 27 V, power dissipation ≤ 15 W, 5 mA ≤ IO ≤ 1 A (characteristics
  measured at VI = 19 V).** Evidence: same datasheet.
- **ESP32 GPIO34–39 are input-only, with no output driver and no internal pull-up/pull-down; GPIO34 = VDET_1,
  GPIO35 = VDET_2.** Evidence: Espressif ESP32 datasheet. Under this build's wiring, the volume-pot wiper is on
  GPIO35; that it therefore needs no pull resistor and cannot be driven as an output is an inference from this
  fact, not a separate datasheet statement.
- **ESP32 GPIO34 = ADC1_CH6/RTC_GPIO4; GPIO35 = ADC1_CH7/RTC_GPIO5; both type I (input only).** Evidence: same
  datasheet.
- **ESP32-S3 strapping pins are GPIO0, GPIO3, GPIO45 and GPIO46; GPIO1 is not a strapping pin.** Evidence: ESP32-S3
  datasheet.
- **ESP32-S3 GPIO1 is chip pin 6: RTC_GPIO1, TOUCH1, ADC1_CH0.** Evidence: same datasheet.

## Ambersong hardware: datasheet facts — display drivers (digit-select P-MOSFETs and segment sinks)

- **ULN2003A: inputs 1B–7B on pins 1–7; outputs 1C–7C on pins 16 down to 10.** Evidence: TI ULN2003A datasheet
  (SLRS027T).
- **ULN2003A: pin 9 (COM) is the common cathode node for the flyback diodes, used with inductive loads.**
  Evidence: same datasheet.
- **ULN2003A: pin 8 (E) is the common emitter, typically tied to ground.** Evidence: same datasheet.
- **ULN2003A: each channel has its own 2.7 kΩ series base resistor, for direct drive from 5 V TTL or CMOS logic.**
  Evidence: same datasheet.
- **ULN2003A: absolute maximum collector-emitter voltage 50 V, input voltage 30 V, peak collector current 500 mA,
  all referred to the emitter/substrate pin.** Evidence: same datasheet.
- **FQP27P06: VDSS −60 V, continuous drain current −27 A, VGSS ±25 V.** Evidence: onsemi FQP27P06 datasheet
  (rev. 4).
- **FQP27P06: VGS(th) −2.0 to −4.0 V; RDS(on) 70 mΩ maximum at VGS = −10 V.** Evidence: same datasheet.
- **FQP27P06: package is a 3-lead TO-220 (gate, drain, source).** Evidence: same datasheet.

## Ambersong hardware: datasheet facts — power distribution, mains side, earth and antenna

- **JST XH connector series: current rating 3 A AC/DC on AWG 22 wire, 2.5 mm pitch, 250 V AC/DC voltage rating,
  accepts AWG 22–30 conductors.** Evidence: JST XH series page. The fitted bus connectors are identified on the
  schematic only as "JST XH", without a specific part number.
- **Schurter 6100.4125 (a C14 mains inlet, not confirmed as the fitted part): rated 10 A, Protection Class I,
  suitable per IEC 61140.** Evidence: Schurter 6100.4125 datasheet. The schematic draws a generic IEC 60320 C14
  receptacle with no source naming the exact fitted part.

## Ambersong hardware: datasheet facts — stepper, index sensor, AS5600, RDA5807M, panel lamps, BT button/LED, audio switch

- **AS5600: 7-bit I2C address 0x36.** Evidence: AS5600 datasheet.
- **AS5600: DIR pin sets rotation polarity (GND = values increase clockwise, VDD = counterclockwise).** Evidence:
  same datasheet.
- **AS5600: SDA and SCL should each have an external pull-up.** Evidence: same datasheet.
- **AS5600: VDD3V3 absolute maximum 4.0 V; in 3.3 V operation, VDD5V and VDD3V3 must be tied together.** Evidence:
  same datasheet.
- **AS5600: OUT is an analogue/PWM output; PGO has an internal pull-up (tying it to GND selects programming
  option B).** Evidence: same datasheet.
- **RDA5807M: pin assignments — GND on pins 1, 3, 8; FM_IN (antenna) on pin 2; RCLK (32.768 kHz crystal input) on
  pin 6; SDIO on pin 5; SCLK on pin 4; VDD on pin 7; ROUT/LOUT on pins 9/10.** Evidence: RDA5807M datasheet.
- **RDA5807M: internal LDO; external supply range 1.8–3.3 V.** Evidence: same datasheet.
- **RDA5807M: I2C only, with a 7-bit chip address of 0010000b (0x10) in the datasheet revision checked; that
  revision has no mention of a second, random-access address.** Evidence: same datasheet.
- **The fitted RDA5807M answers at I2C address 0x11, not the 0x10 the checked datasheet revision documents; it has
  answered there at every boot observed.** This is a fact about the running machine, not the datasheet.
- **RDA5807M: a 22 nF bypass capacitor (its own C2) should sit close to pin 7.** Evidence: same datasheet.
- **IRL540N: logic-level gate drive; VGS(th) 1.0–2.0 V; RDS(on) 53 mΩ at VGS = 5 V, ID = 18 A (63 mΩ at 4 V, 15 A;
  44 mΩ at 10 V, 18 A).** Evidence: IRL540N datasheet.
- **A3144: operates from 4.5–24 V; open-collector output, sinks up to 25 mA, needs an external pull-up.**
  Evidence: A3144 datasheet.
- **A3144: pin 1 supply, pin 2 ground, pin 3 output (viewed from the branded side); output goes low once the
  field exceeds the operate threshold, high again below the release threshold.** Evidence: same datasheet.
- **A3144: discontinued by its maker; its own suggested substitute is the A1104.** Evidence: same datasheet.

## Ambersong hardware: datasheet facts — cable harnesses

- **JST XH: 3 A AC/DC current rating on AWG 22 wire; accepts AWG 22–30; 2.5 mm pitch; 250 V AC/DC voltage
  rating.** Evidence: JST XH series page (the same series as in the power-distribution facts above).

## Ambersong hardware: datasheet facts — the tube radio's valves

These facts describe the named tube type in general, from tube-data pages (not all are manufacturer datasheets),
not a measurement of the specific tube fitted in this chassis. See `docs/hardware-bible-tube-radio.md` for which
stage each tube belongs to and what is actually wired to each pin.

- **12DT8: heater 12.6 V at 0.15 A, indirectly heated, specified for both voltage and current, AC/DC; 9-pin
  miniature (Noval/B9A) base; a high-mu twin triode meant for RF amplifier and combined oscillator/mixer duty, with
  its two triode sections isolated by an internal shield carried out to its own base pin.** Evidence:
  radiomuseum.org, nj7p.org, tdsl.duncanamps.com, amplifiedparts.com (consistent across all four).
- **12DT8: differs from the otherwise similar 12AT7 in heater and heater-cathode ratings, interelectrode
  capacitances and basing — a 12AT7 pinout must not be assumed for a 12DT8.** Evidence: nj7p.org.
- **12BA6: remote-cutoff RF pentode, 7-pin base (7BK); heater 12.6 V at 0.15 A.** Evidence: nostalgiaair.org.
- **7BK base pin-out (as used by the 12BA6): pin 1 control grid, pin 2 suppressor grid/internal shield, pins 3–4
  heater, pin 5 plate, pin 6 screen grid, pin 7 cathode — matching this build's drawn V4/V5 pin assignments.**
  Evidence: bustedgear.com tube pinout chart.
- **12AV6: the two diode plates are pins 5 and 6, sharing cathode pin 2 with the triode, acting as a dual diode;
  the heater is on pins 3 and 4 and needs 12.6 V; 7-pin miniature (B7G) base; the triode has only a control grid
  besides plate and cathode.** Evidence: bristolwatch.com (a hobbyist build article, not a manufacturer datasheet)
  and radiomuseum.org.
- **12BE6: pentagrid converter, 7-pin (B7G) base, pin 1 oscillator grid, pin 2 cathode, pins 3–4 heater, pin 5
  anode, pin 6 screen (shared with grid 4), pin 7 grid 3; heater 12.6 V at 0.15 A — matching this build's drawn
  V3.** Evidence: r-type.org and radiomuseum.org.
- **50C5: beam power amplifier, 7-pin (B7G) base, EIA basing 7CV; heater 50 V at 0.15 A, specified for series
  (AC/DC) operation; maximum plate dissipation 7 W, screen dissipation 1.4 W, plate voltage 150 V, screen voltage
  130 V; one source's typical class-A figures: plate 120 V, grid −8 V, screen 110 V, plate current 49 mA, load
  2.5 kΩ, output power 2.3 W.** Evidence: qsl.net/wd4nka tube manual data and radiomuseum.org.
- **50C5: one pin table gives heater on pins 3–4 with cathode/grid-3 on pin 1, grid 1 on pins 2 and 5, grid 2 on
  pin 6, anode on pin 7 — consistent with this build's drawn V7, whose heater is on pins 3–4 and whose screen
  (grid 2) is pin 6.** Evidence: oddmix.com and r-type.org tube pin charts.
- **All-American-Five style AC/DC heater strings commonly used a 150 mA heater current for sets built for
  110–117 V mains.** Evidence: Wikipedia, "All American Five" — general background on the heater-string design
  class this chassis belongs to, not a measurement of this chassis.

## Ambersong hardware: datasheet and reference links

The documents the datasheet facts in this file were checked against, one row per document. The values quoted in the
sections above come from these versions.

| Document | Link |
|---|---|
| ESP32-S3-WROOM-1 / -1U module datasheet (Espressif, version 1.8) | <https://documentation.espressif.com/esp32-s3-wroom-1_wroom-1u_datasheet_en.pdf> |
| ESP32-S3 chip datasheet (Espressif, version 2.2) | <https://documentation.espressif.com/esp32-s3_datasheet_en.pdf> |
| ESP32-S3-WROOM-2 module datasheet (Espressif, version 1.7) | <https://documentation.espressif.com/esp32-s3-wroom-2_datasheet_en.pdf> |
| ESP32-S3-DevKitC-1 user guide v1.1 (Espressif), header pin tables | <https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32s3/esp32-s3-devkitc-1/user_guide_v1.1.html> |
| ESP-IDF GPIO reference for the ESP32-S3 (USB-JTAG on GPIO19 and GPIO20) | <https://docs.espressif.com/projects/esp-idf/en/stable/esp32s3/api-reference/peripherals/gpio.html> |
| ESP32-WROOM-32 module datasheet (Espressif, version 3.7) | <https://documentation.espressif.com/esp32-wroom-32_datasheet_en.pdf> |
| ESP32-DevKitC user guide (Espressif), power options | <https://docs.espressif.com/projects/esp-dev-kits/en/latest/esp32/esp32-devkitc/user_guide.html> |
| PCM1802 ADC datasheet (Texas Instruments, SLES023D) | <https://www.ti.com/lit/gpn/PCM1802> |
| PCM5102A DAC datasheet (Texas Instruments, SLAS859C) | <https://www.ti.com/lit/gpn/PCM5102A> |
| ULN2003A Darlington array datasheet (Texas Instruments, SLRS027T) | <https://www.ti.com/lit/ds/symlink/uln2003a.pdf> |
| FQP27P06 P-MOSFET datasheet (onsemi, rev. 4) | <https://www.onsemi.com/pdf/datasheet/fqp27p06-d.pdf> |
| JST XH connector series (JST) | <https://www.jst-mfg.com/product/detail_e.php?series=277> |
| A typical IEC 60320 C14 inlet, Schurter 6100-4 (not the fitted part, which is not identified) | <https://www.schurter.com/en/datasheet/6100.4125> |
| 12DT8 tube data | <https://www.radiomuseum.org/tubes/tube_12dt8.html> |
| 12DT8 tube data (base 9DE) | <https://nj7p.org/Tubes/SQL/Tube_query.php?Type=12DT8> |
| 12BA6 tube data (base 7BK) | <https://www.nostalgiaair.org/tubes/12BA6.htm> |
| 12BA6 tube data | <https://www.radiomuseum.org/tubes/tube_12ba6.html> |
| 12BE6 tube data | <https://www.radiomuseum.org/tubes/tube_12be6.html> |
| 12BE6 pin connections | <https://www.r-type.org/exhib/aaa0820.htm> |
| 12AV6 tube data | <https://www.radiomuseum.org/tubes/tube_12av6.html> |
| 50C5 tube data | <https://www.radiomuseum.org/tubes/tube_50c5.html> |
| 50C5 ratings and typical operation (RC-29 data) | <https://www.qsl.net/wd4nka/TOOLS/Manual/50c5.html> |
| Tube base pinout diagrams (7BK and others) | <https://www.bustedgear.com/res_Tube_pinouts.pdf> |

## Ambersong hardware: open points and known gaps

- **The amp's own circuit is not covered by this Bible.** `docs/hardware-bible.md` gives what the amp is, its
  speakers and its input panel; no drawing in this project shows the amp's own internal boards.
- **Mains side, not recorded:** the always-on 5 V supply module (U4) and its isolation, and the fuse, breaker and
  wire gauge of the box's own mains wiring (standard parts); the EMI filter's part, rating and terminals (it is
  drawn on no sheet); the voltage and current rating of the front-panel power switch (SW1).
- **Also not recorded:** the cable from the antenna connector (J10) to the tube radio's antenna input (J53), and
  the gauge of the internal cables.
- **Build detail beyond what the Bible's build-and-enclosure material covers is not recorded**: wire colours
  (other than the amp supply's four), which side is "pin 1" on a connector, crimp and housing types, individual
  cable lengths, cable routing, the fastening screws of the printed plates, and the contact order inside a
  connector. The printed parts themselves are listed in `docs/hardware-bible.md`.
- **Whether the tube radio's original factory loudspeakers (SP1, SP2) are still physically present in the cabinet
  is not recorded.** They are on no circuit in this build either way: see
  `docs/hardware-bible-tube-radio.md`.
- **The tube radio's two original reference documents — the factory schematic and the SAMS schematic and parts
  tables — are scans.** Their values reach this project only through the notes and hidden data carried on the tube
  radio's own KiCad sheet; the scans themselves are not published.
- **The fitted DS3231 RTC module's own datasheet facts are not included**: the fitted part is a DS3231 ZS-042-style
  module whose datasheet could not be re-opened, as against the bare DS3231 chip's own datasheet.
- **GPIO48 on the S3 board has two uses on the same schematic**: it drives the S3 board's own on-board RGB LED,
  and it is also the RDA5807M's SCLK line. The LED was off before and is bright now.
- **The fitted S3 board's power-input rule follows the Espressif DevKitC-1 reference guide**, which states that
  the USB ports, the 5 V header pin and the 3V3 header pin are three mutually exclusive ways to power the board.
  The fitted S3 board is marked "YD-ESP32-23 2022-v1.3"; it is compatible with the DevKitC-1 in firmware, and
  pin-compatible for every pin this build actually uses, but the pins this build does not use have not been
  checked against the reference board.
- **The fitted S3 module's FCC grantee code (2AD6X) differs from Espressif's own code (2AC7Z, per a forum thread);
  a thread specifically about 2AD6X itself could not be found again, so the module's actual maker is not
  identified.**
- **Firmware text that disagreed with this Bible: not recorded for this release.** The Bible lists four lines of
  `include/pins.h` as disagreeing with the hardware (a "680 ohm on the 5 V rail" comment, a "4.7k pull-ups to S3_3V3"
  comment, the define `A32_PCM5102_XSMT 16`, the define `S3_LIMIT_RIGHT 4`). The published firmware no longer has any
  of them: its `include/pins.h` records that `A32_PCM5102_XSMT` and `S3_LIMIT_RIGHT` were deleted. The hardware
  facts behind them stand: the panel-lamp resistors are 120 Ω, the AS5600 pull-ups are 10 kΩ, GPIO16 of the A32
  and GPIO4 of the S3 are not wired to anything, and the DAC module pulls XSMT up itself.
- **An earlier record of the A32's GPIO34, GPIO39 and GPIO36 said they were measured continuous on this board.**
  They are not joined to anything now: the current schematic shows GPIO34 and GPIO39 as no-connect and GPIO36 on
  the audio-source-select ladder. The firmware's own comment describing them as continuous is the firmware's to
  correct.
- **Two documents disagree about the tube radio's factory-original C11, and this file does not pick between
  them.** C11's own hidden specification field on the tube radio sheet reads "Factory SERIES AFC coupling; SAMS
  SHUNT." The tube radio sheet's handwritten note at the AFC/local-oscillator section instead reads "Factory AFC
  series C11 and oscillator feedback differ from SAMS." Both describe the factory original as different from the
  SAMS manual, but they do not agree with each other about what the factory drawing itself shows. Neither point is
  about the fitted hardware: C11 is fitted as a shunt from the AFC line to chassis, and (per
  `docs/hardware-bible-tube-radio.md`) always was.
