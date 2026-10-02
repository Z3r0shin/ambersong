# Appendix C. What this book does not cover, and open points

This book states only what is known about the fitted machine. This appendix gathers, in one place, what
it leaves out on purpose and what is not known. Read it before you plan a repair or a change, or before
you rely on a detail the book does not give. Each open point is stated where it matters, in the chapter
named. It is listed here so that you can see the gaps at a glance.

## C.1 Outside the book's scope

The table lists what the book leaves out on purpose.

| Not covered | Notes |
|---|---|
| **The firmware** | What the two boards run is in the companion book, the Ambersong Firmware Gospel. |
| **The amplifier's own circuit** | Chapter 12 says what the amplifier is and how it connects. No drawing shows its boards. |
| **Wire colours** | Except the amplifier supply's four wires (chapter 5, section 5.6). |
| **Connector details** | Which side of a connector is pin 1, the crimp and connector housings, and the order of the contacts inside a connector. Pin numbers in this book are the cable sheet's (chapter 10, section 10.1). |
| **Cable lengths and routing** | The only length facts: every cable inside the cabinet is under 14 in, and the DAC-to-amplifier cable is under 12 in (chapter 10, section 10.21). |
| **Screws of the printed plates** | Mentioned, not detailed (chapter 3). |
| **Where each module sits in the cabinet** | Beyond "each board in its own printed case, or on a printed carrying plate, screwed to the cabinet" (chapter 3, section 3.1). The amplifier's aluminium back plate, with the mains inlet, is at the back of the cabinet, and the antenna connector is on the back panel; where the amplifier's input panel sits is not recorded. |
| **The tube radio's original documents** | The factory schematic and the SAMS service data are scans, and the SAMS data is not included. Their values reach this book only through the radio drawing's specification fields and notes: the "Original" columns of chapter 13. |
| **The real-time clock's datasheet** | The fitted clock is a DS3231 module of the ZS-042 style. Its datasheet limits are not included in this book, so Appendix A has no entry for it (chapter 4, section 4.7). |

## C.2 Open points

The open points are grouped by chapter. In each table, a row names a topic and says what is not known
about it.

### The boards (chapter 4)

| Topic | What is not known |
|---|---|
| Module makers | Neither module's maker is identified. The main board module's FCC grantee code (2AD6X: the part of its FCC radio-approval ID that names the maker) differs from Espressif's (2AC7Z). The audio board's code (2BB77) is not identified with a maker. Whether either module matches any Espressif module datasheet is not established. |
| Flash chips | The makers behind the flash manufacturer IDs 0x68 (main board) and 0x5e (audio board) are not identified. |
| Main board against the DevKitC-1 | Compatible in firmware and pin-compatible for every pin the machine uses. The pins it does not use have not been checked. |
| Audio board against the DevKitC V4 | Whether it matches, its header order included, is not established. |
| Firmware text that disagrees with this book | Four lines of the firmware's pin definitions disagree with the hardware; the firmware is the one to change. A comment reads "680 ohm on the 5 V rail": the panel-lamp resistors are 120 Ω (chapter 9, section 9.1). A comment reads "4.7k pull-ups to S3_3V3": the angle sensor's pull-ups are 10 kΩ (chapter 4, section 4.4). The define `A32_PCM5102_XSMT 16` makes the audio board drive GPIO16 high as the DAC's soft mute, but GPIO16 is not wired and XSMT is pulled up on the DAC module itself, so it drives nothing (chapter 6, section 6.5). The define `S3_LIMIT_RIGHT 4` is used nowhere, and GPIO4 goes nowhere beyond its own pull-up and capacitor (chapter 4, section 4.4). |

### Power and grounds (chapter 5)

| Topic | What is not known |
|---|---|
| DC ground to Earth | The DC ground reaches Earth through the amplifier's ground (a washer on its Bass pot, chapter 5, section 5.3). By which path the `GND` buses reach the amplifier's ground is not recorded. "Almost 0 Ω" is the author's figure; the instrument and when it was read are not recorded. |
| Bus positions | Which 5 V bus socket each power cable uses; which of the clock module, the volume knob and the source switch plug into 3.3 V bus sockets; which of the 3.3 V jacks are bus positions and which sit on the audio board (chapter 10). |
| Amplifier supply wires | Their colours are the author's reading, not checked inside the supply. |
| Box-fan harness | Which board its two plugs sit on, and how the wires end at the supply and at the fan. |
| Mains wiring | The wires' gauges and colours. |
| Mains fuse | No fuse is recorded on the machine's mains side; the tube radio has its own 0.75 A fuse (chapter 13, section 13.10). Whether the mains inlet or the supply modules carry fuses inside is not recorded. |

### Audio (chapters 6, 9 and 12)

| Topic | What is not known |
|---|---|
| ADC's PDWN and FSYNC | As drawn, they are joined on a `P_3V3` label that goes nowhere else. The converter runs, so PDWN is not held low; the module's own wiring is not recorded (chapter 6, section 6.3). |
| DAC data pull-down | Where the 100 kΩ to ground sits along the DAC data line (chapter 4, section 4.5). |
| AUX and RCA | The AUX jack reaches the amplifier's input panel, but the run is not drawn. Nothing is recorded on the panel's RCA pair (chapter 6, section 6.7). |
| DAC-to-amplifier cable | Where along the cable the two minus legs' 470 Ω resistors sit (chapter 10, section 10.21). |
| Radio-to-ADC cable | Whether it is soldered or plugged at the output transformer's secondary (chapter 10, section 10.22). |
| Amplifier input | What is known rests on one meter reading, taken once; nothing inside the amplifier was examined (chapter 12, section 12.4). |

### Clock display (chapter 7)

| Topic | What is not known |
|---|---|
| Digit order | The order (GPIO42, GPIO2, GPIO40, GPIO41 for digits 1 to 4) is the one drawn in chapter 7, section 7.4, and chapter 10, section 10.8, and the one the display shows. Which physical board or cable puts the digits in this order is not established; the author believes it is the digit driver board. |
| Digit sockets | Whether the display sub-sheet's digit outputs are the sockets the digits plug into (chapter 10, section 10.34). |
| Colon cable | Where its reversed polarity physically sits (chapter 10, section 10.23). |
| Digit-rail cable | What its shield and foil do at the display end (chapter 10, section 10.27). |

### Dial and sensors (chapter 8)

| Topic | What is not known |
|---|---|
| Index magnet | Which pole faces the Hall switch. |
| Angle sensor cable | The connector at the sensor end, and its pin order (chapter 10, section 10.10). |
| Calibration receiver's antenna plug | Which pins of the 2 × 3 grid carry the antenna wire (chapter 10, section 10.35). |
| Needle motor lead | Anything beyond the socket's five pins (chapter 10, section 10.32). |

### FM antenna (chapter 11)

| Topic | What is not known |
|---|---|
| Performance | No SWR, no field strength and no reception comparison has been measured. |
| Antenna coax inside | The cable from the antenna connector, on the back panel, to the radio's antenna input (chapter 10, section 10.35). |
| Build details | The details of section 11.3 come from the build notes and were not confirmed one by one. |

### The tube radio (chapter 13)

| Topic | What is not known |
|---|---|
| Lugs | Which physical lug carries each logical terminal number of the coils and the mode switch (section 13.1.3). |
| The ratio detector's tertiary winding (L14) | Where its other end goes (section 13.4). |
| Original specifications | Not recorded for the oscillator cathode resistor (R33) and its bypass capacitor (C50) (section 13.3). |
| The antenna shield capacitor (C51) | Where it came from (section 13.2). |
| Fitted part types and ratings | Not recorded for several parts: among them the FM detector diodes (M3, M4), the mains fuse's type and voltage (M1), the rectifier's part number and rating (X1), and some capacitors' dielectrics. Each stage's parts table says which (sections 13.2 to 13.10). |
| Power switch | Which lug of the power and tone switch (S2) carries the power contact (section 13.10), which of its positions have power on, and where the switch sits (it is not on the front panel). |
