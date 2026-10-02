# Appendix A. Datasheet facts used in this machine

This appendix gathers the makers' figures that matter for this machine: pinouts, limits, addresses and
ratings, part by part. Only the facts the machine uses are here. You need it when you replace a part,
check a reading against a limit, or adapt the design to other parts. The chapters tell you what each
part does; abbreviations are explained in the glossary.

Every figure is the **maker's**, for the catalogue part. It is not a measurement of the part fitted here.
Sometimes the maker's document describes a related part rather than the fitted one: a module the boards
may not carry, or a reference development board the fitted board may not match. The entry then says so.
There are no entries for the real-time clock module: its datasheet is not included in this book
(Appendix C).

## A.1 Main board: ESP32-S3

The fitted module is marked **ESP32-S3-N16R8**. The chip read itself as an ESP32-S3 (QFN56), revision
v0.2, with 8 MB of PSRAM inside (chapter 4, section 4.1). The module figures below are from Espressif's
**ESP32-S3-WROOM-1** module datasheet. That the fitted module is a WROOM-1 is not established.

The table gives each fact, the maker's figure, and why it matters on this machine.

| Fact | Maker's figure | Why it matters here |
|---|---|---|
| Memory, variant N16R8 | 16 MB flash (quad SPI) and 8 MB PSRAM (octal SPI); operating temperature −40 to 65 °C | the fitted module's marking |
| GPIO35, GPIO36, GPIO37 | On modules with octal PSRAM, connected to the PSRAM and **not available for other uses**. Espressif's ESP32-S3-DevKitC-1 guide says the same for its boards. | not used here (chapter 4, section 4.2) |
| GPIO47, GPIO48 voltage | 1.8 V only on modules whose chip is an ESP32-S3R16V. The chip datasheet gives the ESP32-S3R8 a 3.3 V flash supply (VDD_SPI). | They carry the FM calibration receiver's bus. That they work at the normal 3.3 V on this board is an inference from these two figures; the datasheet does not state it directly. |
| Supply (VDD33) | 3.0 to 3.6 V, 3.3 V typical; absolute maximum 3.6 V | |
| Strapping pins | **GPIO0** (weak pull-up) and **GPIO46** (weak pull-down) set the boot mode; **GPIO45** (weak pull-down) sets the flash supply voltage; **GPIO3** (floating) sets the JTAG signal source. They are read at reset. GPIO1 is not a strapping pin. | none of the four is used; GPIO1 reads the amplifier power sensor |
| GPIO19, GPIO20 | USB D− and D+, connected by default to the chip's USB Serial/JTAG controller. Reconfiguring them as ordinary GPIOs disables USB-JTAG. | the main board's USB port |
| GPIO39 to GPIO42 | the JTAG interface: MTCK (GPIO39), MTDO (GPIO40), MTDI (GPIO41), MTMS (GPIO42) | used here for the angle sensor's clock and three digit selects |
| UART0 | TXD0 on GPIO43, RXD0 on GPIO44 | not used |
| RGB LED (DevKitC-1 guide) | on GPIO48 on the first version of the DevKitC-1 board, on GPIO38 on version 1.1 | The fitted board's RGB LED is on GPIO48, which also carries the calibration receiver's clock. |
| Power inputs (DevKitC-1 guide) | the USB ports, the 5V pin and the 3V3 pin are "three mutually exclusive ways to provide power to the board" | see the caution in chapter 2, section 2.5 |

## A.2 Audio board: ESP32

The audio board is meant to carry an **ESP32-WROOM-32E** module; whether it does is not established.
Its chip read itself as an
ESP32-D0WD-V3. The module figures are from Espressif's **ESP32-WROOM-32** datasheet. The board figures are
from Espressif's **ESP32-DevKitC V4** guide. Whether the fitted board matches that board is not
established.

The table is laid out as in section A.1.

| Fact | Maker's figure | Why it matters here |
|---|---|---|
| GPIO6 to GPIO11 | connected to the module's SPI flash; not recommended for other uses | not connected here |
| Strapping pins | **GPIO0** (pull-up) and **GPIO2** (pull-down) set the boot mode; **GPIO12** (MTDI, pull-down) selects the flash supply voltage at reset; **GPIO15** (MTDO, pull-up); **GPIO5** (pull-up) | GPIO0 carries the converters' master clock |
| GPIO34 to GPIO39 | **input only**: no output driver and no internal pull-up or pull-down (ESP32 chip datasheet). On the module: GPIO36 is ADC1 channel 0, GPIO39 channel 3, GPIO34 channel 6, GPIO35 channel 7. | the volume knob (GPIO35) and the source switch (GPIO36) |
| Supply (VDD33) | 3.0 to 3.6 V, 3.3 V typical; absolute maximum −0.3 to 3.6 V | |
| GPIO16, GPIO17 (DevKitC V4) | available only on boards with ESP32-WROOM or ESP32-SOLO-1 modules; boards with ESP32-WROVER modules reserve them | GPIO17 is the word clock here |
| Capacitor C15 (DevKitC V4) | On earlier DevKitC V4 boards, C15 may make the board boot into download mode, and may disturb a clock output on GPIO0. | GPIO0 is the master clock output here |
| Power inputs (DevKitC V4) | Micro USB, the 5V pin and the 3V3 pin are three mutually exclusive ways to power the board; power must come from one and only one of them, "otherwise the board and/or the power supply source can be damaged" | see chapter 2, section 2.5 |

## A.3 The ADC: PCM1802 (Texas Instruments)

The ADC turns the tube radio's audio into digital samples for the audio board (chapter 6). The table gives
the maker's figures for its supplies and setting pins.

| Fact | Maker's figure |
|---|---|
| Supplies | VCC (pin 5): analogue, 5 V. VDD (pin 14): digital, 3.3 V. |
| Setting pins | PDWN (pin 7), BYPAS (pin 8), OSR (pin 16), FMT0, FMT1, MODE0 and MODE1 are Schmitt-trigger inputs with an internal pull-down of about 50 kΩ, and are 5 V tolerant. |
| PDWN | power-down, active low: the chip runs only while PDWN is high |
| Format | FMT1 low and FMT0 high: I2S, 24-bit |
| Interface mode | MODE1 and MODE0 both low: slave mode (system clock 256, 384, 512 or 768 times the sample rate) |
| Slave mode | BCK, LRCK and FSYNC are inputs. The chip sends its data only while FSYNC is high. |
| OSR | low: oversampling ×64; high: ×128 |
| BYPAS | low: normal mode, with the high-pass filter cutting DC; high: filter bypassed |
| SCKI (pin 15) | the system clock input, 256, 384, 512 or 768 times the sample rate; Schmitt-trigger, 5 V tolerant |

How each setting pin is tied on this machine's module is in chapter 6, section 6.3.

## A.4 The DAC: PCM5102A (Texas Instruments)

The DAC turns the audio board's digital audio back into analogue audio for the amplifier (chapter 6). The
table gives the maker's figures for its pins, supplies and modes.

| Fact | Maker's figure |
|---|---|
| Pins | OUTL 6, OUTR 7, AVDD 8, AGND 9, DEMP 10, FLT 11, SCK 12, BCK 13, DIN 14, LRCK 15, FMT 16, XSMT 17, DVDD 20 |
| Digital inputs | failsafe LVCMOS Schmitt-trigger inputs |
| Supplies | AVDD: 3.3 V. DVDD: 1.8 V or 3.3 V. Absolute maximum 3.9 V on AVDD, CPVDD and DVDD. |
| XSMT | soft mute when low, un-mute when high. Where it is not needed, it can be tied straight to AVDD. |
| FMT | low: I2S; high: left-justified |
| FLT | low: normal-latency filter; high: low-latency filter |
| DEMP | de-emphasis for 44.1 kHz audio: off when low, on when high |
| Zero-data mute | After **1024 word-clock periods of zero data** (21 ms at 48 kHz), on both channels by default, the chip puts its output into full analogue mute. |
| Master clock and PLL | The internal PLL is turned off as soon as an external system clock (SCK) is supplied. If BCK and LRCK start correctly while SCK stays at ground for 16 word-clock periods, the PLL starts and makes its own system clock from BCK. |

## A.5 Amplifier power sensor and box-fan regulator

**PC817 optocoupler (Sharp).** An optocoupler passes a signal across by light: an LED on one side lights a
phototransistor on the other, with no wire between them.

| Fact | Maker's figure |
|---|---|
| Pins (DIP-4) | 1 anode, 2 cathode, 3 emitter, 4 collector |
| Absolute maximum, input (LED) | forward current 50 mA; peak forward current 1 A; reverse voltage 6 V; power dissipation 70 mW |
| Absolute maximum, output | collector–emitter 80 V; emitter–collector 6 V; collector current 50 mA |
| Current transfer ratio (CTR) | 50 to 400 % at 5 mA LED current and 5 V collector–emitter, 25 °C; 50 % minimum. The CTR is the output current as a share of the LED current. |

**LM7812 12 V regulator (Texas Instruments).** It makes the box fan's 12 V from the amplifier supply's
+20 V (chapter 5, section 5.6).

| Fact | Maker's figure |
|---|---|
| Pins (TO-220) | 1 input, 2 ground, 3 output |
| Output | 11.5 to 12.5 V at 25 °C; 11.4 to 12.6 V over an input of 14.5 to 27 V, 5 mA to 1 A output, at most 15 W dissipated. The maker specifies it at 19 V in. |

The amplifier supply that feeds it measured 19.33 V with no load (chapter 5, section 5.6).

## A.6 Display and motor drivers

**ULN2003A (Texas Instruments).** The chip holds seven Darlington transistor pairs, each able to pull a
load to ground. It is used on the display driver board and on the needle motor driver.

| Fact | Maker's figure |
|---|---|
| Inputs | 1B to 7B on pins 1 to 7 |
| Outputs | 1C to 7C on pins 16 to 10 (open collector: each can only pull low) |
| E (pin 8) | common emitter of all channels, usually tied to ground |
| COM (pin 9) | common cathode of the built-in flyback diodes; required for inductive loads |
| Input resistor | 2.7 kΩ in series with each base, so the inputs work straight from 5 V or 3.3 V logic |
| Absolute maximum | collector–emitter 50 V; input 30 V; peak collector current 500 mA; all referred to E |

**FQP27P06 P-channel MOSFET (onsemi).** These are the digit switches of the clock display (chapter 7).

| Fact | Maker's figure |
|---|---|
| Drain–source voltage | −60 V |
| Drain current, continuous | −27 A (case at 25 °C) |
| Gate–source voltage | ±25 V maximum |
| Gate threshold | −2.0 to −4.0 V |
| On-resistance | 70 mΩ maximum at −10 V gate–source, −13.5 A |
| Package and pins | TO-220, pins G, D, S |

## A.7 Connectors

**JST XH.** These are the 2-pin sockets of the 5 V and 3.3 V buses. They are identified only by the
drawing's text "JST XH".

| Fact | Maker's figure |
|---|---|
| Pitch | 2.5 mm |
| Current | 3 A AC/DC, with 22 AWG wire |
| Voltage | 250 V AC/DC |
| Temperature | −25 to +85 °C |
| Wire | 30 to 22 AWG (0.05 to 0.33 mm²); insulation 0.9 to 1.9 mm outside diameter |

**IEC C14 mains inlet.** No datasheet names the fitted inlet. One maker's C14 inlet, the Schurter
6100-4, is rated 10 A, protection class I.

## A.8 Needle, tuning and lamp parts

**AS5600 magnetic angle sensor** (the tuning shaft).

| Fact | Maker's figure |
|---|---|
| I2C address | 0x36 (7-bit) |
| DIR pin | on ground: the reading increases clockwise; on the supply: counter-clockwise |
| SDA, SCL | "consider external pull-up" |
| Supply | VDD3V3 absolute maximum 4.0 V. For 3.3 V operation, the VDD5V and VDD3V3 pins must be tied together. Whether the fitted sensor's board ties them is not recorded. |

**RDA5807M FM receiver** (the FM calibration receiver).

| Fact | Maker's figure |
|---|---|
| Pins | ground 1, 3 and 8; FM_IN 2; SCLK 4 (serial clock); SDIO 5; RCLK 6 (32.768 kHz crystal or reference clock input); VDD 7; ROUT and LOUT 9 and 10 |
| Supply | 1.8 to 3.3 V, 3.0 V typical, through an internal regulator |
| Control | I2C only |
| Address | 0010000 binary (**0x10**). This revision of the datasheet documents no other address. **The fitted chip answers at 0x11** (observed on the running machine, not a datasheet figure). |
| Bypass | the maker's reference design puts a 22 nF supply bypass capacitor close to pin 7 |

The decoupling fitted on this machine's module is in chapter 8, section 8.5.

**IRL540N MOSFET** (the panel-lamp switch).

| Fact | Maker's figure |
|---|---|
| Gate | logic-level gate drive |
| Gate threshold | 1.0 to 2.0 V |
| On-resistance | 0.044 Ω at 10 V gate, 18 A; 0.053 Ω at 5.0 V, 18 A; 0.063 Ω at 4.0 V, 15 A |

**A3144 Hall-effect switch** (the needle index).

| Fact | Maker's figure |
|---|---|
| Supply | 4.5 to 24 V, with a built-in regulator |
| Output | open collector, sinks up to 25 mA; needs a pull-up for logic circuits |
| Pins | viewed from the branded side: 1 supply, 2 ground, 3 output |
| Switching | the output (pin 3) goes low when the field at the sensor rises above the operate point, and high again when it falls below the release point |
| Availability | **discontinued**; the maker points A3144 users to the **A1104** |

## A.9 Tubes

The tubes' bases, heater ratings and pins, and the 50C5's maximum ratings, are in chapter 13
(sections 13.1.4 and 13.8.2). These figures are published tube data. Some of it comes from reference
sites rather than from the tubes' makers.

**50C5 typical class-A operation.** The 50C5 is the radio's output tube (chapter 13, section 13.8). The
table gives two typical operating points: the voltages on its plate, control grid and screen grid, the
plate current, the load and the output power.

| Plate voltage | Grid voltage | Screen voltage | Plate current | Load | Output power |
|---|---|---|---|---|---|
| 110 V | −7.5 V | 110 V | 50 mA | 2.5 kΩ | 1.9 W |
| 120 V | −8 V | 110 V | 49 mA | 2.5 kΩ | 2.3 W |
