# About this book

This book describes the hardware of Ambersong: a vintage tube radio, rebuilt in a wooden cabinet with
two small computer boards, a digital clock, a motor-driven dial needle and a modern amplifier. It says
what every part is, where it sits when that is known, and what it is joined to. With it you should be
able to repair, debug, change or rebuild a machine like this one without any other document.

## What this book covers

- **The machine:** the two microcontroller boards, the power and the grounds, the audio chain, the clock
  display, the dial needle and its sensors, the front-panel controls and lamps, and every cable.
- **The tube radio inside it,** a LLOYDS TM-838N, stage by stage. Its parts are given as fitted, with the
  original factory and service-data values beside them for reference.
- **The FM antenna** as built and installed, **the amplifier and its speakers**, and **the physical build**:
  the cabinet, the front panel, the tuning and needle mechanics, and the Faraday cages.
- **Datasheet limits** for the parts used here (Appendix A). They describe the catalogue part, not a
  measurement of the one fitted.

## What it does not cover

- **The firmware.** What the two boards run, what you see on the radio, and how to update them are in the
  companion book, the **Ambersong Firmware Gospel**. This book states the wire and the part; the Gospel
  states what the code does with them.
- **The amplifier's own circuit.** Chapter 12 says what the amplifier is and how it connects, but not
  what is on its boards.
- **Build details:** wire colours (except the amplifier supply's four wires, chapter 5), which side of a
  connector is pin 1, crimp and connector housings, the length of each cable (the only length facts: every
  cable inside is under 14 in, and the DAC-to-amplifier cable is under 12 in), cable routing, the screws of the printed plates, and the order of the
  contacts inside a connector.

Appendix C lists every open point and every gap.

## How to use it

**Read chapter 2 before you open the cabinet.** Mains is present inside even with the front switch off,
and the tube radio's metal chassis is wired to mains neutral.

Then find your job in the table below; it names the chapter to start at.

| If you want to… | Start at |
|---|---|
| know what the machine is and how the parts fit together | chapter 1 |
| open the cabinet safely | chapters 2 and 3 |
| find what a GPIO pin does | chapter 4 |
| trace a power or ground problem | chapter 5 |
| trace a sound problem | chapters 6 and 12 |
| fix the clock display | chapter 7 |
| fix the dial needle or tuning sensors | chapter 8 |
| fix a front-panel control or lamp | chapter 9 |
| repair or remake a cable | chapter 10 |
| service the tube radio | chapter 13 |

Most chapters end with a *When something is wrong* table. It holds only what this book knows: the
wiring, and what the author has stated about the fitted parts.

Much of this book describes **one** machine: this cabinet, this radio, this amplifier and these boards.
If you build your own, adapt it.

**The drawings.** The author drew the machine in KiCad to show a person what was built, not to make a
circuit board. Some drawings place a part where it reads best rather than where it sits. You do not need
them to use this book. If you do read them, this book is right where the two differ, and Appendix B
lists the places where a drawing can mislead.

**The firmware's own text.** A few comments and names in the firmware describe wiring that is not what
was built. Where one disagrees with this book, the hardware stated here is right and the firmware text is
stale. Each known case is noted where its part is described (chapters 4, 6 and 9).

## Notation

A few conventions run through the whole book. Learn them once here.

**The two boards.** The book calls them by what they do:

| In this book | The board | Also written |
|---|---|---|
| **main board** | the ESP32-S3 development board: clock display, needle, tuning sensors, panel lamps | S3 |
| **audio board** | the ESP32 development board: sound, Bluetooth, front controls, real-time clock | A32 |

**Signal names** are written in `code font`, exactly as on the drawings and the cable sheet: `GPIO5`,
`A_IO22`, `S3_to_A32`. On the main board a signal is usually named after its pin (`GPIO38`); on the audio
board it carries an `A_` prefix (`A_IO19` is the audio board's GPIO19). In the text, an audio-board pin
is written with its board named, as in "the audio board's GPIO25".

**The four grounds.** The machine has four ground names, kept different on purpose. Three of them, Earth,
the DC ground and the amplifier supply return, are all joined. The fourth, the radio chassis, is joined to
none of them: it sits on mains neutral. Chapter 5, section 5.3, explains each one and how they are
joined.

| Name in this book | Written as |
|---|---|
| Earth | `Earth` |
| DC ground | `GND` |
| Amplifier supply return | `20V_GND` |
| Radio chassis | `CHASSIS` |

**Rails with two names.** Some conductors carry two names on the drawings. The two names are the same
wire. The last row is the one rail that is easy to confuse with them: it has one name and is a separate
wire.

| Name | Same as | What it is |
|---|---|---|
| `5VDC_S` | `5VDC` | the 5 V supply's output; `5VDC_S` is its name before the bus |
| `PSU_GND` | `GND` | the 5 V supply's ground; `PSU_GND` is its name before the bus |
| `A_VDC` | `A_3V3` | the 3.3 V bus, fed from the audio board's 3.3 V output |
| `S3_3V3` | — | the main board's own 3.3 V output, a separate rail |

**`CABLE S/FTP`** on a connector pin means the cable's shield and foil end there. Unless a cable's page
says otherwise, they are joined to `GND` at the board end and left unconnected at the far end
(chapter 10).

**Pin numbers on cables** are the cable sheet's, counted along the drawn connector. They are not the
numbers moulded on a real connector.

**Parts** are named by what they are. Where you need a reference to find a part on a drawing, it follows
in brackets: "the safety capacitor between the antenna shield and the chassis (C51)". The tube radio's
references clash with the main drawing's, so chapter 13 always gives a name with them.

**Values** are as fitted. Capacitors drawn as polarised are fitted the right way round as drawn; the
others are film or ceramic.
