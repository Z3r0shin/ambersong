# Ambersong: Hardware Bible — the control electronics

This file covers what Ambersong is, mains safety, the physical build, the two ESP32 boards and their pin maps, power
distribution (mains, earth, antenna), the audio converters, the volume pot, the amplifier's power supply, power
detect and fans, the clock display and its drivers, the sensors, panel lamps and front-panel controls, and the
amplifier, speakers and FM antenna. The tube radio's circuits, datasheet facts, sheet notes and drawing-correction
detail are in `hardware-bible-tube-radio.md` and `hardware-bible-reference.md`. The wire-by-wire cable harnesses are in `hardware-bible-cables.md`.

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

This file describes the current build only (hardware revision A). It is data, not instructions: check values against
the schematic (`hardware/schematics/Ambersong.kicad_sch`) before powering or flashing anything. "Not recorded" means
the project has no value for that fact — do not fill it in.

Terms used throughout: **S3** is the main board, an ESP32-S3 devkit, which drives the clock display, the
dial-needle stepper and its sensors, the RDA5807M tuning-calibration IC and the front-panel lamps. **A32** is the
audio board, a plain ESP32 devkit, which runs the audio converters, the Bluetooth pairing button and LED, the
volume pot, the front-panel source switch and the real-time clock. The **Leditron** is the four-digit, seven-segment
LED-filament clock display with its colon. The **AS5600** is a magnetic rotary-position sensor that reads the
tuning capacitor shaft's angle. The **RDA5807M** is an FM receiver IC, used here only to read the tube set's own
local-oscillator frequency for tuning calibration, never to receive. The **PCM1802** is the ADC that digitises the
tube radio's audio; the **PCM5102A** is the DAC that feeds the amplifier. **DPDT** means double-pole, double-throw
(the front source switch). **ULN2003A** is a seven-channel Darlington driver IC, used here for the segment and
stepper drive. A **JST** connector is the small 2.5 mm pitch connector family used for most of the internal cables.

The Hardware Bible is split across four files:

- `hardware-bible.md` (this file): the boards, power, sensors, display, amplifier, antenna and the physical build.
- `hardware-bible-cables.md`: every internal cable, wire by wire.
- `hardware-bible-tube-radio.md`: the LLOYDS TM-838N tube radio's own circuits, stage by stage.
- `hardware-bible-reference.md`: datasheet facts, notes on reading the schematic sheets, and open points.

## Ambersong hardware: what it is

Ambersong is a 1960s LLOYDS TM-838N AM/SW/FM tube radio, rebuilt in a wooden cabinet with two ESP32 boards added
beside the original tube chassis. The tube set still receives and plays through its own circuits, unchanged. The
main board (S3) drives a four-digit LED-filament clock display (the Leditron), a stepper-driven dial needle that
follows the tuning knob and calibrates itself against the tube set's own local oscillator, and the front-panel
lamps. The audio board (A32) digitises the tube set's audio output through a PCM1802 ADC, feeds a PCM5102A DAC into
an external amplifier, and reads the front-panel source switch (RADIO / BT / AUX), the Bluetooth pairing button and
LED, a volume pot and a DS3231 real-time clock. Ambersong adds Bluetooth audio, an AUX input and a web portal with
over-the-air firmware updates, without changing the tube set's own reception or sound.

## Ambersong hardware: mains safety

- **The tube radio has no isolation transformer.** Its circuit common, `CHASSIS`, is on mains neutral (fed from
  `AC_N` through an external EMI filter). `CHASSIS` is never bonded to Earth or to any of the Faraday cages. This
  holds only while the wall outlet's live and neutral are the correct way round — nothing in the device checks
  that.
- **The front-panel power switch (SW1) does not remove mains from the box.** The inlet's live (`AC_L_IN`) reaches the
  switch, and also reaches the always-on 5 V power supply directly, ahead of the switch. Only unplugging the mains
  inlet removes mains power from the device.
- The tube radio's high-voltage B+ supply is made from the mains by a half-wave rectifier inside the tube radio's
  own supply; it is present whenever the radio is switched on.
- The DC side's ground (`GND`, shared by the S3, the A32 and the rest of the control electronics) is bonded to
  mains earth through the amplifier's own earthing path (see "Ambersong hardware: the amplifier, speakers and input
  panel"). The tube radio's `CHASSIS` is not part of that bond.

## Ambersong hardware: the physical build

### The cabinet and how to open it

A wooden cabinet holds everything. The tube radio's original case has been stripped; its original chassis sits in
the cabinet. The back is an inverted-T half-inch plywood panel behind the radio, held by ten countersunk 3 mm
screws. The front panel is held from the inside by five 3 mm screws. The tube radio itself is held by two #8
half-inch wood screws through its tabs, one on each side. Almost every other board or mechanical part sits on a
small perfboard or module in its own 3D-printed (PLA) case, or on a PLA carrying plate, screwed to the cabinet with
#8 screws.

Every internal cable is under 14 inches, long enough to service the box with the panels open and nothing unplugged,
and each is marked at both ends with an evident name or its name on the cable sheet of the schematic.

Cooling: a box fan sits at the top of the back panel and blows air out; air comes in through two small louvred PLA
vents at the other two ends of the inverted T. A second, smaller cage fan is screwed to the PLA Faraday cage over
the tube radio and blows air into it.

### The front panel

The front panel carries: the tuning knob; the volume knob; the source switch; the power switch; the Bluetooth LED
(which doubles as the Bluetooth pairing button); the Leditron clock display (four seven-segment digits and a
colon); the four FM panel lamps; and the FM dial window, where the needle shows the rough FM position against a
blackened brass plate. The source switch and the power switch are fixed directly in the wooden front panel, each
by its own nut and then plenty of hot glue.

The volume knob turns the A32's volume pot — the front-panel control — not the amplifier's own volume pot, which is
reached only from the back of the amplifier (see "Ambersong hardware: the amplifier, speakers and input panel").
The printed shaft extension described below is for tuning only.

### Tuning, the needle and the index

The tube radio's original tuning knob is turned from the front through a printed (PLA) shaft extension. That knob
drives the original dial cord, which turns the tuning capacitor's shaft through its wheel; the cord drives nothing
else. Turning clockwise raises the frequency.

The AS5600 sensor is mounted beside the tuning capacitor's shaft, its board perpendicular to the shaft, with the
shaft's axis passing through the sensor IC's centre. A printed shaft extends the capacitor's shaft almost to the
IC; a 6 × 3 mm disc neodymium magnet is epoxied to the end of that extension, less than 0.5 mm from the IC. The
magnet must be diametrically magnetised.

The needle is the tube radio's original brass needle, mounted on a printed holder on the stepper motor's shaft (a
28BYJ-48 geared stepper). The stepper alone drives the needle; it has never been connected to the dial cord. What
the stepper is told to do is firmware behaviour, described in `firmware-gospel.md`.

A small arm on the opposite side of the same printed needle holder carries a square neodymium magnet, about
3 × 3 mm and 0.2 mm thick, which passes over an A3144 Hall-effect switch as the needle sweeps — this is the index
(home) sensor. The A3144 sits on a small perfboard in its own PLA case. Which pole of the index magnet faces the
A3144 is not recorded.

The old limit-switch input (GPIO4 on the S3) keeps its pull-up resistor and capacitor fitted but is wired to a dead
end: see "Ambersong hardware: pin map, main MCU board (S3)".

### The Faraday cages

There are three Faraday cages, all bonded to Earth:

- **Over the tube radio:** a PLA box lined with aluminium tape, the tape covered with Kapton tape, kept well clear
  of the tube radio itself. Earth reaches the aluminium through a stranded green wire, held against it by copper
  tape with conductive adhesive.
- **Over the AS5600:** built the same way; the AS5600 is mounted to the cage's base, clear of both the aluminium
  and the copper tape.
- **Over the mains EMI filter:** Kapton tape over the circuit, then aluminium tape, then Kapton tape again; earthed
  the same way as the other two.

### Connectors

The mains inlet is a standard IEC C14 connector — the inlet that came fitted to the amplifier's own aluminium back
plate (see "Ambersong hardware: the amplifier, speakers and input panel"). Some connector types inside the box are
used once only, so that a cable cannot be plugged into the wrong socket and break something — for example, the AC
feed to the front power switch.

### Printed parts

Every printed part in the current build is an STL file in `hardware/STLs/`, 76 files in all. All are PLA except the
passive radiator's membrane, which is TPU. They are sized for this cabinet and these boards — a builder adapting the
design should expect to resize or redesign them.

**Speakers**

| File | What |
|---|---|
| `AUDIO - Passive Radiator - Holder.stl` | passive radiator holder |
| `AUDIO - Passive Radiator - Inside Protector.stl` | passive radiator, inside protector |
| `AUDIO - Passive Radiator - Membrane Weight.stl` | passive radiator, membrane weight |
| `AUDIO - Passive Radiator - Outside Protector.stl` | passive radiator, outside protector |
| `AUDIO - Passive Radiator - TPU Membrane.stl` | passive radiator membrane, printed in TPU |
| `AUDIO - Speakers Holder and Seal.stl` | speaker holder and seal |

**Back panel**

| File | What |
|---|---|
| `BACK - Dual USB-C Ports.stl` | the two back-panel USB-C ports |
| `BACK - Electronics - BNC Antenna Interface.stl` | the antenna connector (a BNC) |
| `BACK - Fan Exhaust.stl` | box fan exhaust |
| `BACK - Fan Intake.stl` | louvred air intakes |
| `BACK - Panel Anchors Bottom.stl` | back panel anchors, bottom |
| `BACK - Panel Anchors Inner Corners.stl` | back panel anchors, inner corners |
| `BACK - Panel Anchors Outer Corners.stl` | back panel anchors, outer corners |
| `BACK - Panel Anchors.stl` | back panel anchors |

**Front panel**

| File | What |
|---|---|
| `FRONT - FM Dial - Back.stl` | FM dial back; also the stepper motor's mount |
| `FRONT - FM Dial - Cover.stl` | FM dial cover |
| `FRONT - FM Dial - Spacer.stl` | FM dial spacer |
| `FRONT - FM Dial - Stepper Needle and Magnet Holder.stl` | mount on the stepper's shaft carrying the needle and the index magnet |
| `FRONT - Panel Anchors - Insidemost.stl` | front panel anchors, insidemost |
| `FRONT - Panel Anchors - Outsidemost.stl` | front panel anchors, outsidemost |
| `FRONT - Tuner Pot Knob.stl` | front tuning knob |
| `FRONT - Tuning Pot Extender PLA Shaft.stl` | tuning shaft extension |
| `FRONT - Tuning Shaft Bearing Seat.stl` | tuning shaft's bearing seat |
| `FRONT - Volume Pot Knob.stl` | front volume knob, on the A32's volume pot |

**Board cases and holders (inside the cabinet)**

| File | What |
|---|---|
| `IN - Electronics - 5V BUS case Bottom.stl` / `Top.stl` | 5 V bus case |
| `IN - Electronics - 5V PSU Case Bottom.stl` / `Top.stl` | 5 V power supply case |
| `IN - Electronics - 5V+3V3 BUS case Bottom.stl` / `Top.stl` | 5 V + 3.3 V bus case |
| `IN - Electronics - AC Line Filter Case.stl` | mains EMI filter case (Faraday cage) |
| `IN - Electronics - AS5600 Case - Bottom.stl` / `Top.stl` | AS5600's cage |
| `IN - Electronics - AS5600 Case - Connecting Rod.stl` | the printed shaft that carries the AS5600's magnet |
| `IN - Electronics - Audio Listener Case BOTTOM.stl` / `TOP.stl` | the source switch's relay board case |
| `IN - Electronics - Audio MCU Case BOTTOM.stl` / `TOP.stl` | A32 case |
| `IN - Electronics - Backing Panel 1.stl` | a panel that holds several of the cases |
| `IN - Electronics - Bluetooth LED Holder.stl` | BT LED holder |
| `IN - Electronics - Bluetooth LED Pusher.stl` | BT LED pusher (the LED is also the button) |
| `IN - Electronics - Box Fan LM7812 Case Bottom.stl` / `Top.stl` | box-fan 12 V regulator case |
| `IN - Electronics - DS3231 RTC Case BOTTOM.stl` / `TOP.stl` | DS3231 RTC case |
| `IN - Electronics - Digit Drivers Case BOTTOM.stl` / `TOP.stl` | digit driver board case |
| `IN - Electronics - Front Panel FM LEDs Case BOTTOM.stl` / `TOP.stl` | FM panel lamp driver case |
| `IN - Electronics - Hall Sensor Stepper Limit Case.stl` | the A3144 index sensor's case |
| `IN - Electronics - LEDITRON Back.stl` | Leditron display back |
| `IN - Electronics - LEDITRON Bus Case Bottom.stl` / `TOP.stl` | Leditron bus case |
| `IN - Electronics - PCM1802 ADC Case BOTTOM.stl` / `TOP.stl` | PCM1802 ADC case |
| `IN - Electronics - PCM5102A DAC Case Bottom.stl` / `TOP.stl` | PCM5102A DAC case |
| `IN - Electronics - Power Detect Case BOTTOM.stl` / `TOP.stl` | power-detect case |
| `IN - Electronics - RDA5807M - Bottom.stl` / `Middle Bottom.stl` / `Middle TOP.stl` / `TOP.stl` | RDA5807M case (four parts) |
| `IN - Electronics - RDA5807M - Cage Holder.stl` | RDA5807M cage holder |
| `IN - Electronics - S3 Main MCU Backing.stl` | S3 backing |
| `IN - Electronics - S3 Main MCU Case BOTTOM.stl` / `TOP.stl` | S3 case |
| `IN - Electronics - Stepper Driver ULN2003A Case - Bottom.stl` / `Top.stl` | stepper driver case |

**Tube radio**

| File | What |
|---|---|
| `IN - Tube Radio - Faraday Cage 1.stl` | tube radio Faraday cage, part 1; the cage fan's mount is part of this piece |
| `IN - Tube Radio - Faraday Cage 2.stl` | tube radio Faraday cage, part 2 |

**Leditron display**

| File | What |
|---|---|
| `LEDITRON - Colon - Blanket.stl` | colon blanket |
| `LEDITRON - Colon.stl` | colon |
| `LEDITRON - Digit.stl` | digit |
| `LEDITRON - Front Case.stl` | front case |
| `LEDITRON - Glass Spacer.stl` | glass spacer |

Not printed: the source and power switches (mounted in the wooden front panel itself), the IEC mains inlet (fitted
to the amplifier's own back plate), and the amplifier's ±20 V power supply standoffs, which are plastic screws and
standoffs.

## Ambersong hardware: the two MCU boards

Board markings were read directly off the hardware; chip data is `esptool.py` output, read over USB with each board
reset into its ROM bootloader and back — read-only, nothing written. MAC addresses are never recorded in a public
document.

### Main board — ESP32-S3 devkit (S3)

Module marking: "ESP32-S3-N16R8 FCC ID: 2AD6X-ESP32S3N16R8 IC: 29018-ESP32S3N16R8 CMIT ID: 2022DP2107". Board
marking: "ESP32-S3 N16R8 YD-ESP32-23 2022-v1.3". The board's own RGB LED is on GPIO48 (shared with the RDA5807M's
clock line — see "Ambersong hardware: sensors, panel lamps and controls"); it lights brightly now, where it was off
before the RDA5807M was wired to that pin.

`esptool.py v4.9.0 --port <port> flash_id`, exit 0: chip is ESP32-S3 (QFN56, revision v0.2); features WiFi, BLE,
embedded PSRAM 8 MB (AP_3v3); 40 MHz crystal; USB mode USB-Serial/JTAG; flash manufacturer ID `68`, device ID
`4018`, detected size 16 MB, quad (4-line) mode, voltage set by eFuse to 3.3 V.

### Audio board — ESP32 devkit (A32)

Board marking: "FCC IDL 2BB77-ESP32-32X". It is meant to carry an ESP32-WROOM-32E module, and reaches its USB port
through an onboard Silicon Labs CP210x USB-to-UART bridge.

`esptool.py v4.9.0 --port <port> flash_id`, exit 0: chip is ESP32-D0WD-V3 (revision v3.1); features WiFi, BT, dual
core, 240 MHz, VRef calibration in eFuse, no coding scheme; 40 MHz crystal; flash manufacturer ID `5e`, device ID
`4016`, detected size 4 MB, voltage set by a strapping pin to 3.3 V.

**Not recorded:** whether the S3 or A32 module matches any Espressif module datasheet exactly — the S3 module's FCC
code (2AD6X) and the A32 module's (2BB77) have not been checked against Espressif's own grantee code; the makers
behind flash manufacturer IDs `0x68` and `0x5e` are not identified.

## Ambersong hardware: pin map, main MCU board (S3)

One row per module pin, in the board's own pin order.

| Pin | Signal | Goes to |
|---|---|---|
| 3V3 (upper) | `S3_3V3` | Decoupled by 0.1 µF + 10 µF; feeds the GPIO4/GPIO5 pull-ups, the RDA5807M supply, the AS5600 supply, the AS5600 cable's 3.3 V conductor, and the S3_3V3 bus connector |
| GND (upper) | (unnamed, returns the upper 3V3 decoupling) | — |
| 3V3 (lower) | not connected | — |
| 43, RST, 44 | not connected | — |
| GPIO4 | `GPIO4` | 10 kΩ pull-up to `S3_3V3` and 0.1 µF to GND; wired to the LIMITS cable's JST, but goes nowhere beyond it (a dead leg left from an earlier design) |
| GPIO1 | `GPIO1` (power detect) | 10 kΩ pull-up to `S3_3V3`; reads the power-detect opto-isolator's collector — see "Ambersong hardware: volume pot, amplifier supply, power detect and fans" |
| GPIO5 | `GPIO5` (index sensor) | 10 kΩ pull-up to `S3_3V3`, 0.1 µF to GND; reads the A3144 Hall sensor's output |
| GPIO2 | digit 2 select | Drives the digit-select driver's input 2 |
| GPIO6 | segment A | Drives the segment driver's input 1 |
| GPIO42 | digit 1 select | Drives the digit-select driver's input 1 |
| GPIO7 | segment B | Drives the segment driver's input 2 |
| GPIO41 | digit 4 select | Drives the digit-select driver's input 4 |
| GPIO15 | segment C | Drives the segment driver's input 3 |
| GPIO40 | digit 3 select | Drives the digit-select driver's input 3 |
| GPIO16 | segment D | Drives the segment driver's input 4 |
| GPIO39 | AS5600 `SCL` | 10 kΩ pull-up to `S3_3V3` at the pin, then 220 Ω in series, to the AS5600's `SCL` |
| GPIO17 | segment E | Drives the segment driver's input 5 |
| GPIO38 | AS5600 `SDA` | 10 kΩ pull-up to `S3_3V3` at the pin, then 220 Ω in series, to the AS5600's `SDA` |
| GPIO18 | segment G | Drives the segment driver's input 7 |
| 37 | not connected | — |
| GPIO8 | segment F | Drives the segment driver's input 6 |
| 36, 3, 35, 46, 0 | not connected | — |
| GPIO9 | stepper coil | Drives the stepper driver's input 1 |
| 45 | not connected | — |
| GPIO10 | stepper coil | Drives the stepper driver's input 2 |
| GPIO48 | RDA5807M `SCLK` | Also drives the S3 board's own onboard RGB LED |
| GPIO11 | stepper coil | Drives the stepper driver's input 3 |
| GPIO47 | RDA5807M `SDIO` | — |
| GPIO12 | stepper coil | Drives the stepper driver's input 4 |
| GPIO21 | panel-lamp PWM | Through a 100 Ω resistor to the panel-lamp MOSFET's gate |
| GPIO13 | UART to A32 | S3 transmits, A32 receives, on A32's GPIO26 |
| GPIO20 | not drawn as connected | This is the ESP32-S3's native `USB_D+`; the back-panel USB-C connector reaches the S3's own USB port |
| GPIO14 | UART from A32 | A32 transmits (its GPIO27), S3 receives |
| GPIO19 | not drawn as connected | The ESP32-S3's native `USB_D-`, reached the same way as GPIO20 |
| 5V | `5VDC` | Decoupled by 100 µF + 0.1 µF; feeds from the 5 V bus |
| GND (ord 42) | GND | global GND label |
| GND (ord 43) | (unnamed net) | returns only the 5 V pin's local decoupling (100 µF + 0.1 µF); no GND label on the main sheet |
| GND (ord 44) | no-connect on the main sheet | — |

**The main sheet and the "MAIN MCU ESP32-S3 N16R8" sub-sheet disagree on ord 43 and ord 44**: the main sheet draws
ord 43 as an unnamed net (local decoupling return only) and ord 44 as no-connect, while the sub-sheet instead ties
both of these to GND labels.

GPIO9–GPIO12 feed a ULN2003A (U3; its own driver IC, separate from the segment and digit drivers) whose outputs and
common (tied to `5VDC`, decoupled by 0.1 µF + 47 µF) drive the stepper motor. GPIO6, 7, 15, 16, 17, 8, 18 feed a
second ULN2003A (U2) for the seven display segments.

## Ambersong hardware: pin map, audio MCU board (A32)

One row per module pin, in the board's own pin order. `A_3V3` and `A_VDC` are the same 3.3 V rail; `A_IOnn` names
mirror the GPIO number.

| Pin | Signal | Goes to |
|---|---|---|
| 3V3 | `A_3V3` | Decoupled by 0.1 µF + 10 µF + 10 µF; feeds the 3.3 V bus, the BT LED's anode |
| GND (upper) | (unnamed, returns the 3V3 decoupling) | — |
| EN, 23 | not connected | — |
| GPIO36 | source-switch sense | 0.1 µF to GND at the pin, then 1 kΩ in series to the source-switch ladder node (see "Ambersong hardware: sensors, panel lamps and controls") |
| GPIO22 | RTC `SDA` | — |
| 39, 1, 34, 3 | not connected (GPIO34 and GPIO39 are not joined to GPIO36 or to each other on this board) | — |
| GPIO35 | volume-pot wiper | Reads the front-panel volume pot's wiper |
| GPIO21 | RTC `SCL` | — |
| 32 | not connected | — |
| GND (right) | GND (local label only) | — |
| GPIO33 | BT LED cathode | Active low: drive the pin low to light the LED |
| GPIO19 | ADC data in | PCM1802 `DOUT` |
| GPIO25 | BT pairing button | `INPUT_PULLUP`; pressed = LOW |
| GPIO18 | shared I2S bit clock | PCM1802 `BCK` and PCM5102A `BCK` |
| GPIO26 | UART from S3 | S3 transmits (its GPIO13), A32 receives |
| 5 | not connected | — |
| GPIO27 | UART to S3 | A32 transmits, S3 receives on its GPIO14 |
| GPIO17 | shared I2S word clock | PCM1802 `LRCK` and PCM5102A `LRCK` |
| 14, 16, 12 | not connected (GPIO16 is unused) | — |
| GPIO4 | DAC data out | PCM5102A `DIN`, with a 100 kΩ pull-down at the A32 pin |
| GND (left) | GND | Returns the 5 V decoupling |
| GPIO0 | shared I2S bit clock source | Splits at the pin into two separate 33 Ω series resistors: one feeds the PCM1802's `SCK`, the other the PCM5102A's `SCK` — both carry the same GPIO0 signal, named `A_IO0A` and `A_IO0D` on the drawings only to keep the two branches readable |
| 13, 2, 9, 15, 10, 8, 11, 7, 6 | not connected | — |
| 5V | `5VDC` | Decoupled by 0.1 µF + 100 µF |

**The real-time clock** is a DS3231 (U8), on a ZS-042-style breakout module. The fitted module also carries an AT24C32
EEPROM at I2C address `0x57` and a CR2032 backup cell; most modules of this kind carry both, but not all. The module's I2C header has four
positions, of which only two are wired: `SDA` and `SCL` (see "Ambersong hardware: the RTC I2C cable" in
`hardware-bible-cables.md`).

## Ambersong hardware: power distribution, mains, earth and the antenna connector

Mains live from the IEC inlet (J30, `AC_L_IN`) feeds the front-panel power toggle (SW1) and, directly, the always-on
5 V power supply (U4; see "Ambersong hardware: mains safety"). The toggle's switched output (`AC_L`) feeds the
amplifier's own power supply (U1). Mains neutral (`AC_N`) is shared by the inlet, the 5 V supply and the amplifier's
supply.

**Earth** is one node joining the mains inlet's earth pin, the amplifier power supply's earth pin, all three
Faraday cages, and the antenna connector's outer shell. A Faraday cage sits over the mains EMI filter, between the
tube radio's mains input and Ambersong's own AC input (after the front power switch), bonded to the same earth.

The antenna connector (J10; a coax connector, originally F-type, now a BNC male — the type does not matter) carries the
antenna signal on its centre pin; its far end is the FM antenna described under "Ambersong hardware: the FM
antenna".

The 5 V power supply's output feeds a 5 V distribution bus of 15 JST 2-pin positions (with a matching ground bus),
built on its own small board, from which every 5 V consumer in the device is fed. The A32's own 3.3 V output
(`A_3V3` / `A_VDC`, the same rail) similarly feeds a 5-position 3.3 V distribution bus with its own matching ground
bus, from which the AS5600, the RTC and the volume pot are fed. Both buses are built as simple screw/solder
distribution boards, not drawn as a single part.

## Ambersong hardware: audio converters

### The ADC (PCM1802) — digitising the tube radio's own audio

The PCM1802 module runs from `5VDC`, decoupled by 100 µF + 0.1 µF. On the module, `MODE1`, `MODE0`, `BYPAS`, `OSR`
and `FMT1` are tied to the module's own GND; `FMT0` is tied to the module's own 3.3 V pin and to nothing else; `PDW`
and `FSY` are tied together but reach no other part of the circuit. `LRCK` and `BCK` are shared with the DAC
(A32 GPIO17 and GPIO18); `DOUT` goes to A32 GPIO19; `SCK` is fed from A32 GPIO0 through its own 33 Ω series
resistor (see the A32 pin map).

The radio feed comes in on a jack (J51, the current reference — a drawing correction renamed it from J42): the
sleeve and the ring are both grounded at this jack, and the tip runs through a 4.7 kΩ resistor to an attenuator
node, which is also loaded by a 1 kΩ resistor and a 0.001 µF film (CBB22) capacitor to GND. That node feeds the
PCM1802's `LIN` by the drawing, and, in the actual hardware, its `RIN` as well — the radio feed is mono and drives
both ADC channels. The transformer (T1) secondary that supplies this jack (one leg on the tip, the other on the
ring — the two are interchangeable) also carries a 7.5 Ω, 5 W load resistor across it; the transformer's primary is
the tube radio's own 50C5 output-stage winding.

Two clip-on ferrite cores sit on the radio-to-ADC cable, one near the transformer end and one near the ADC end,
each passed through once. Measured: about 10 mm OD, 3.5 mm ID, 3.5 mm wall, 20 mm long (the wall and ID figures
don't quite add to the OD, so one of the three is rounded). Material as supplied, with no maker, part number or
grade recorded: rectangular-loop ratio 20, coercivity 16 A/m, remanence 200 mT, Curie temperature 130 °C, density
4.9 g/cm³, described only as "soft magnetic".

### The DAC (PCM5102A) — feeding the amplifier

The PCM5102A module (U9) runs from `5VDC`, decoupled by 100 µF electrolytic + 10 µF ceramic + 0.1 µF. On the module,
`FLT`, `DEMP` and `FMT` are tied to GND (normal-latency filter, de-emphasis off, I2S format). `XSMT` is pulled up
to the module's own 3.3 V on the module itself, so the DAC is permanently un-muted with no external connection
needed; the module's `A3V3` pin is not used externally. `DIN` is fed from A32 GPIO4, with a 100 kΩ pull-down at the
A32 pin. `LRCK` and `BCK` are shared with the ADC. `SCK` is fed from the same A32 GPIO0 as the ADC's clock, through
its own separate 33 Ω series resistor (named `A_IO0D` on the drawings).

The DAC's own output jack (J43) carries three signals — `OUTL`, `OUTR`, `AGND` — which reach the amplifier through the
cable described in "Ambersong hardware: the DAC-to-amplifier cable" (in `hardware-bible-cables.md`): each of the four resulting
legs (`AMP_L+`, `AMP_L-`, `AMP_R+`, `AMP_R-`) carries its own 470 Ω resistor in the cable, with `AGND` as the common
return for the two `−` legs. Six ferrite cores sit on this cable, three at each end: one HDMI-type sleeve core
(17.4 mm OD, 9.7 mm ID, 28.5 mm long — the same type used at the antenna's feedpoint, see "Ambersong hardware: the
FM antenna") and two salvaged, unidentified toroids (16 mm and 22 mm OD, 3–4 mm wall, 8 mm long); the cable is
under 12 inches, so it passes through each core once, with no turns.

## Ambersong hardware: volume pot, amplifier supply, power detect and fans

*Specific to this build — a reader adapting the design should expect to size the fans, the enclosure and the
amplifier's ±20 V supply to their own cabinet and amplifier.*

### The front-panel volume pot

The front-panel volume knob turns a B10K potentiometer (RV4). Its wiper feeds the A32's GPIO35; one end is on `A_VDC`
(3.3 V), the other on GND.

### The amplifier's power supply, and power detect

The amplifier's own power-supply module (U1) takes switched mains (`AC_L`, downstream of the front power toggle)
and neutral (`AC_N`), and earths its case through the shared Earth node. It outputs `+20V`, `-20V` and a common
`20V_GND` return, carried to the amplifier on four wires: red `+20V`, two middle black `GND` wires, yellow `-20V`
(the builder's own reading of the wire colours, not independently verified inside the supply). The same supply
also feeds the box fan and the power-detect circuit below.

`+20V` feeds, in the control electronics: the box-fan 12 V regulator's input (through its own small cable, with
0.1 µF + 100 µF decoupling at the regulator), and a 4.7 kΩ resistor that is the LED series resistor of a
power-detect opto-isolator (a PC817, U6). That opto's LED cathode returns to `20V_GND`; its output side has the
collector pulled up to `S3_3V3` through 10 kΩ and read on the S3's GPIO1, with the emitter on GND. In other words,
GPIO1 reads "high" only while the amplifier's `+20V` rail — and so the amplifier's own power supply — is energised.
This power-detect circuit is duplicated once more, built as its own small sub-board reached by its own cable (see
"Ambersong hardware: the power-detect cable"), but it is electrically the same signal.

### Fans

A small 12 V regulator (U40, fed from `+20V`, returning to `20V_GND`) powers the box fan (M1), which exhausts air
at the top of the back panel; a flyback diode (D34) sits across the fan. A second, smaller fan is wired directly to the main
`5VDC`/`GND` rails (not through the 20 V supply or the 12 V regulator) and is screwed to the Faraday cage over the
tube radio, blowing air into it.

## Ambersong hardware: the clock display

*Specific to this build — a reader adapting the design should expect to size the display for their own front
panel.*

The Leditron display has four seven-segment digits and a colon. Digit 1 is the rightmost digit (units of minutes);
digits 2, 3 and 4 run leftwards from it. The S3 selects a digit for lighting with one GPIO each: GPIO42 → digit 1,
GPIO2 → digit 2, GPIO40 → digit 3, GPIO41 → digit 4. **Where this GPIO-to-digit-position mapping is physically made
is not recorded**; it is believed to be inside the digit-driver module (see "Ambersong hardware: display drivers").

Each digit is a common-anode block of seven LEDs (one per segment), with the seven cathodes bussed, segment by
segment, across all four digits, to the shared segment driver (see "Ambersong hardware: display drivers"):

| Digit | Common anode (digit-select input) | Seg A | Seg B | Seg C | Seg D | Seg E | Seg F | Seg G |
|---|---|---|---|---|---|---|---|---|
| 1 (rightmost, GPIO42) | digit-select driver output 1 | D1 | D3 | D5 | D6 | D4 | D2 | D7 |
| 2 (GPIO2) | digit-select driver output 2 | D24 | D27 | D28 | D26 | D23 | D22 | D25 |
| 3 (GPIO40) | digit-select driver output 3 | D17 | D20 | D21 | D19 | D16 | D15 | D18 |
| 4 (leftmost, GPIO41) | digit-select driver output 4 | D10 | D13 | D14 | D12 | D9 | D8 | D11 |

Each digit module plugs into a matching socket on the shared segment bus via an 8-pin header (7 segment cathodes
plus the common anode); the pin numbering only reaches its partner through that plug, not through any other
conductor.

The colon runs as its own simple string: `5VDC` → a 33 Ω resistor → the colon LED (COLON1) → a variable resistor
(R99, glued in place at 1.5 kΩ) → GND. **Caution:** the colon's own 5 V supply cable is wired with reversed polarity at the
colon's connector compared with every other 5 V cable in the device — check polarity before plugging it in.

## Ambersong hardware: display drivers

### Digit select (P-channel MOSFETs)

Each digit-select GPIO (GPIO42, GPIO2, GPIO40, GPIO41) drives one input of a seven-channel open-drain driver IC
(U5), whose corresponding output feeds, through a 1 kΩ gate resistor, the gate of one P-channel MOSFET (FQP27P06,
one per digit: "P-Mosfet", "PFet1", "PFet2" and "PFet3" on the schematic, for digits 1–4 respectively). Each
MOSFET's source is on `5VDC`; its gate also carries a 100 kΩ pull-up to `5VDC`. Each digit's
`5VDC` supply point is decoupled locally by 0.1 µF + 10 µF to GND. Each MOSFET's drain is that digit's common-anode
rail (D1–D4 in the table above).

### Segment sinks

The seven segment GPIOs (GPIO6, 7, 15, 16, 17, 8, 18, for segments A–G) each drive one input of a second
seven-channel driver IC (U2). Each output feeds, in series, a 10 Ω resistor and then a 200 Ω resistor to that segment's
own bus; the mid-point between the two resistors also carries its own 100 kΩ pull-up to `5VDC`. The seven segment
buses (A–G) are electrically separate from each other, each with its own 100 kΩ pull-up — there is no shared
segment bus.

The digit-select driver and the segment driver both return their own GND pin to the shared GND, and leave their
unused channels unconnected.

This driver circuitry lives on its own small perfboard, in its own PLA case, reached from the S3 and from the
display by the cables described in `hardware-bible-cables.md` (the `DISPLAY_IN`/`DISPLAY_OUT`,
`DIGIT_IN`/`DIGIT_OUT`, and `DRIVERS_5VIN` cables).

## Ambersong hardware: sensors, panel lamps and controls

*Specific to this build — a reader adapting the design should expect to size the dial-needle drive, the panel
lamps and the front-panel controls for their own cabinet and dial.*

### Stepper motor and index sensor

The stepper motor (a 28BYJ-48 geared stepper, driven through its own ULN2003A board, U3) has its four coil inputs
driven from S3 GPIO9–GPIO12; the ULN2003A's common pin is tied to `5VDC`, decoupled locally by 0.1 µF + 47 µF.

The index (home) sensor is an A3144 Hall-effect switch (U42), running from `5VDC`/GND with 0.1 µF decoupling. Its output
feeds S3 GPIO5, pulled up to `S3_3V3` by 10 kΩ, with 0.1 µF to GND at the S3 pin.

### AS5600 tuning-angle sensor

The AS5600 module (U11) runs from `S3_3V3`/GND, with its `DIR` pin tied to GND. Its `SDA` and `SCL` each reach the S3
through their own 220 Ω series resistor and 10 kΩ pull-up to `S3_3V3` (GPIO38 for `SDA`, GPIO39 for `SCL`). Its
magnet and mounting are described under "Ambersong hardware: the physical build".

### RDA5807M tuning-calibration IC

The RDA5807M module (U89) runs from `S3_3V3`/GND, decoupled by 100 µF electrolytic + 0.1 µF ceramic + 10 µF ceramic
directly across its supply pins. Its `SDIO` reaches S3 GPIO47 and its `SCLK` reaches S3 GPIO48, with no external
pull-up resistors on either line. Its antenna pin connects, through a 2×3 pin grid connector (U71) that lets the
module be unplugged for repairs, to a 30 cm length of wire inside the tube radio's own Faraday cage, bent in a U shape
over the tube set's FM oscillator section — this lets the RDA5807M read the tube set's own local-oscillator
frequency without an antenna of its own. Its `RCLK`, `IO1`, `IO2`, `ROUT` and `LOUT` pins are not used.

### Front-panel lamps

Four LEDs (D30–D33) light the FM dial panel. Their anodes are on `5VDC`; each cathode runs through its own 120 Ω
resistor to the shared drain of an IRL540N N-channel MOSFET (Q1). The MOSFET's gate is driven from S3 GPIO21 through a 100 Ω series
resistor, with a 100 kΩ pull-down and a 220 pF capacitor from gate to source; the source is on GND. GPIO21 dims the
lamps by PWM.

### Bluetooth pairing button and LED

The front-panel Bluetooth control is one physical part that is both an LED and a button. The button (SW2) sits
between A32 GPIO25 and GND, read as `INPUT_PULLUP` (pressed = LOW). The LED (D29)'s anode is on `A_VDC`; its
cathode is on A32 GPIO33, active low — drive the pin low to light it.

### Front-panel audio source switch

A front-panel DPDT switch selects RADIO, BT or AUX by putting one of three voltage levels onto A32 GPIO36. The
ladder resistors (22 kΩ and 68 kΩ) are on the A32 board itself; a 1 kΩ series resistor and a 0.1 µF filter capacitor
sit at the A32 pin. With the switch in the RADIO position, the direct leg puts 3.3 V on the line; in the BT
position, the 68 kΩ leg puts about 0.81 V on the line; in the centre (AUX) position, the line is pulled to 0 V by
the 22 kΩ resistor alone. The firmware reads below 424 counts as AUX, 2471 and above as RADIO, and anything between
as BT.

**On AUX, the audio itself never reaches the A32.** AUX is wired as an analogue path straight into the amplifier's
own input panel, which sums its inputs; the A32's only role on AUX is to stay silent on its own output (see
"Ambersong hardware: the amplifier, speakers and input panel"). No A32 setting can change AUX's loudness or mute
it.

## Ambersong hardware: the amplifier, speakers and input panel

*Specific to this speaker and amplifier set — a reader adapting the design should expect to use their own.*

**Scope.** What the amplifier and its speakers are, the input panel, and what has been measured at it. The
amplifier's own internal circuit is not covered here: no schematic sheet in this project shows its boards.

### What the input panel does

The panel sums its inputs; it does not select between them — if AUX, the RCA input and the balanced input all
carry sound at the same time, the amplifier's output is their sum. It has three inputs: a TRS jack (AUX), a
balanced input (`L+`, `L-`, `GND`, `R-`, `R+`) and an RCA pair. The AUX and RCA jacks are grounded when nothing is
plugged into them; the balanced input is not. The amplifier's own volume control does not reach silence at its
minimum setting.

The DAC reaches this panel through the balanced input, by the cable described above (with its 470 Ω resistors and
ferrite cores). AUX reaches the same panel as analogue audio, never through the A32.

**Observed (2026-09-20 to 2026-09-22):** a frying noise heard at the amplifier went away when the DAC-to-amplifier
cable was unplugged at the amplifier end, and almost went away when it was unplugged at the DAC end instead. After
clip-on ferrites were added to the radio-to-ADC cable, no pops were audible even before the 470 Ω resistor pair and
the HDMI-type ferrites were added to the DAC-to-amplifier cable.

### What was measured at the panel and on the cable

**Measured 2026-09-21** (multimeter, resistance, one reading each), with the DAC-to-amplifier cable unplugged at
the amplifier: `L-` to `AMP_G` and `R-` to `AMP_G` both read open on a multimeter (resistance), 3 MΩ and rising as
if charging a capacitor.

**Measured 2026-09-21** on the same cable, unplugged, before its 470 Ω resistor pair on `L+`/`R+` was fitted: `L-`
to GND read 470 Ω, `R-` to GND read 470 Ω, `L-` to `R-` read a little under 1 kΩ, and `L+` to GND and `R+` to GND
both read 4.2 MΩ.

These readings are consistent with each `−` leg having no DC path to the amplifier's ground at its input connector
— what a differential, AC-coupled input looks like on a meter. They are single readings taken on one occasion;
nothing inside the amplifier itself was inspected.

### What the amplifier and speakers are

The amplifier and speakers came from a Kramer Tavor 5-O powered speaker pair (P/N 60-00009010): one side active,
one passive, 2 × 30 W RMS, 4 Ω. Each side has a 5.25 in polypropylene woofer and a 0.5 in Mylar-dome tweeter, with a
first-order passive crossover at 8 kHz. The original cabinets were ported; this build instead uses passive
radiators, and is not ported. Of the original cabinets, only the active side's back aluminium plate remains — the
earth plate described below.

The amplifier board carries four ICs: a TDA7265 power stage, and a TL074, a TL072 and a C4558 on its input board.
Its power supply is an isolated offline switching supply, measured at ±19.33 V with no load, with two TO-220
regulators making ±12 V for the op-amps; it reaches the amplifier board on four wires (red `+20 V`, two middle
black `GND`, yellow `-20 V` — the builder's own reading of the wire colours, not independently verified inside the
supply). The same supply also feeds the box fan and the power-detect circuit (see "Ambersong hardware: volume pot,
amplifier supply, power detect and fans"). The amplifier's "auto" standby mode has been removed from this build.

The amplifier's original dual-gang volume pot has been replaced with a B10k dual-gang pot (its power-switch
section is not used); its two gangs stay separate, as the original's were. It sits on the amplifier's own PCB and
is reached only from the back — it is not the front-panel volume knob (see "Ambersong hardware: the physical
build").

### The amplifier's ground and the earth plate

The amplifier's GND is earthed through its own Bass control: a washer on the Bass potentiometer bonds it to the
amplifier's back aluminium plate, which carries the device's earth star point. This is the Kramer Tavor's own
original design, unmodified. **Not recorded:** no drawing shows this ground-to-plate joint, and the path by which
the device's GND buses reach the amplifier's ground is not stated. As a result, the whole DC side's ground — the
S3, the A32, the amplifier and everything else on the DC rails — sits on mains earth at very close to 0 Ω; the
instrument and the time window of this reading are not recorded. The amplifier's volume and treble pots
do not touch that plate electrically, and the amplifier's ±20 V power supply is insulated from it by plastic screws
and standoffs. The tube radio's own `CHASSIS` is not part of this bond: it stays on neutral and is never bonded to
Earth (see "Ambersong hardware: mains safety").

An earlier measurement disagrees with "very close to 0 Ω": a safety check once recorded the system ground as open
to earth, in the megohms, and an earlier design rule kept every ground conductor off anything earthed. The
near-0-Ω figure above is the later, current statement. The only isolation this document states for the present
build is between the tube radio's `CHASSIS` and Earth — not between GND and Earth.

## Ambersong hardware: the FM antenna

*Specific to this build — a reader adapting the design should expect to size, mount and aim an antenna for their
own house and location.*

### The aerial

A three-element Yagi antenna, mounted horizontally on the roof.

- **Director:** plain aluminium U-channel, 3.2 mm thick, 12.6 mm high, 25.5 mm wide, 133.8 cm long.
- **Reflector:** the same U-channel material, 150.9 cm long.
- **Driven element:** 1/2 in type L copper tube, 139 cm per element, folded — the ends are soldered with 90-degree
  tubing joints — about 1390 mm tip to tip, with the two conductors 50 mm apart, centre to centre.
- **Boom:** 3030 aluminium extrusion, 1170 mm long.
- Nothing touches the boom electrically except through its clamps: the director and reflector touch neither the
  boom nor anything else; only the folded driven element is wired to the coax, and it reaches the boom only through
  a non-conductive clamp.

### Feedline, earth and mounting

The feedline is coax, at least 100 ft at present, intended to end up at about 50 ft total, bulkhead to the radio.
The earth is 8 AWG stranded copper, ferruled, run from a grounding block to the Hydro-Québec meter box, landed on a
lug on the meter base, beside one an electrician made for the external wiring. The grounding block is fitted
outside, out of the weather; its exact position does not matter.

The mast is the same 3030 aluminium extrusion as the boom, 4 ft high, fixed with continuous aluminium extrusion on
the roof, centred on the boom. The boom is continuous metal through the mast to the roof metal, and the metal roof
is itself electrically connected to the mast and so to earth — so the boom and the coax shield's 8 AWG earth run
reach the same electrode system, even though they are separate conductors.

Height and aim: about 4 ft over the antenna's own base and 1 ft over the tip of the roof, clear of roof
reflections; the roof slope faces away from the target direction and the aerial points as directly at it as it
can. The antenna is guyed with Dacron lines, two from each end of the boom, firmly enough that it cannot move short
of a major accident.

The radio end of this run — the antenna jack, and the components between it and the tube radio's tuner — is
covered in the book's tube-radio file, not here.

### Further build detail (reviewed but not individually confirmed)

- Element positions along the boom: 0 / 55 / 110 cm — reflector, driven element, director. Both U-channels are
  mounted opening downward; the three element centres are coplanar within 5 mm.
- Feed gap: 20 mm, cut in one tube only.
- Mounting, top down: nut, Belleville washer, neoprene washer (the metal-roofing-screw type), G10 washer, the
  element in an ASA bushing, an ASA plate continuous from that bushing, then the boom through a 90-degree clamp.
  The driven element is mounted the same way. No 1 MΩ static-bleed resistors are fitted from the parasitic elements
  to the boom.
- Feedpoint: an F-81 bulkhead connector in the ASA clamp, its shell on one side of the 20 mm gap and its centre pin
  on the other. Six salvaged HDMI-type ferrite sleeves (17.4 mm OD, 9.7 mm ID, 28.5 mm long — the same type used on
  the DAC-to-amplifier cable) sit on the feedpoint as a common-mode choke.
- Feedline: RG-6. Grounding block: a Perfect Vision PVGB1HFWS, UL listed, 3 GHz, F-type, single, with a weather
  boot — a plain block with no gas-discharge tube.

### What follows, and what is not recorded

The boom is metal all the way through the mast to the roof metal, while the three elements sit on insulating
bushings, so the elements themselves are isolated from that grounded structure; the coax shield's earth path is
the 8 AWG run to the meter-base lug, not a separate boom-earth wire. **Not recorded:** any measurement of the
antenna itself — no SWR, no field-strength reading, no reception comparison against another antenna.
