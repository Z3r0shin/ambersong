# 3. The physical build

This chapter says where things are and how they are held, so that you can open the cabinet, find a part,
and repair or rebuild it. It describes this cabinet; yours will differ. Screw details for the printed
plates, cable routing and the length of each cable are not covered.

> **Danger — unplug the cord first.** The front power switch does not remove mains from the cabinet.
> Read chapter 2 before you open it.

## 3.1 The cabinet and how to open it

A **wooden cabinet** holds everything. The tube radio's original case has been stripped; its original
chassis sits in the cabinet. The table says how each main piece is held, so you know what to undo.

| What | How it is held |
|---|---|
| **Back panel** | An inverted-T panel of ½ in plywood behind the radio, held by ten countersunk 3 mm screws. |
| **Front panel** | Held from the inside by five 3 mm screws. |
| **Tube radio** | Two #8 ½ in wood screws through its tabs, one on each side. |
| **Everything else** | Almost every board and mechanical part is on a small perfboard or module in its own printed PLA case, or on a printed PLA carrying plate, screwed to the cabinet with #8 screws. |

**Every cable inside is under 14 in long**, long enough to service the machine with the panels open and
nothing unplugged.

The order in which the back panel and the front panel come off is not recorded.

Beyond this, the book does not describe where each module sits inside the cabinet.

## 3.2 The front panel

The table lists the controls and displays on the front panel, with the chapter that covers each one.
Where each one sits on the panel is not recorded.

| Control or display | What it is | Chapter |
|---|---|---|
| **Tuning knob** | A printed front knob, on a printed shaft extension, turns the tube radio's original tuning knob, which drives the dial cord and the tuning capacitor. Clockwise raises the frequency. | 8 |
| **Volume knob** | The audio board's volume control, read digitally. It is **not** the amplifier's volume control, which is reached only from the back. | 9 |
| **Source switch** | A three-position switch (double-pole, on–off–on): RADIO and BT at the two ends, AUX in the centre. | 9 |
| **Power switch** | The front power switch. It turns on the amplifier and the tube radio, but does not remove mains from the cabinet. | 2, 5 |
| **Bluetooth LED** | A single part that is both the Bluetooth LED and the Bluetooth button. | 9 |
| **Clock display** | Four seven-segment digits and a colon, made for this panel. | 7 |
| **FM panel lamps** | Four LEDs, dimmed together by the main board. | 9 |
| **FM panel** | The needle shows the rough FM position against a blackened brass plate. What it shows on AM and SW is not recorded. | 8 |

The source switch and the power switch are fixed in the wooden front panel itself, each by its own nut
and then plenty of hot glue.

The tube radio's own power and tone switch (S2) and its own volume control (R1) are **not** on the front
panel. Where on the radio they sit, and which position S2 is left in, are not recorded. The radio's volume
control sits at about one eighth of its travel (chapter 13, section 13.11).

The AUX jack is not in this list either. Where it sits is not recorded (chapter 6, section 6.7).

## 3.3 Tuning, needle and index

The printed front knob, on a printed shaft extension, turns the radio's original tuning knob, which
drives the dial cord and the tuning capacitor. A magnetic angle sensor
(AS5600) reads the capacitor's shaft. The needle sits on a printed mount on a small stepper motor, which
alone moves it. A magnet on that mount passes a Hall-effect switch, the needle's only position
reference. Chapter 8, section 8.1, describes the mechanics and how each part is mounted.

## 3.4 Cooling

Two fans move air (chapter 5, section 5.6):

- The **box fan** sits at the top of the back panel and blows air **out** of the cabinet. Fresh air comes
  in through two small louvred printed vents at the other two ends of the back panel's inverted T.
- The **cage fan** is screwed to the tube radio's Faraday cage and blows air **into** it.

## 3.5 The Faraday cages

Three earthed cages shield the tube radio, the angle sensor and the mains line filter. The cages over
the tube radio and the angle sensor are printed PLA boxes lined with aluminium tape. The tape is covered
with Kapton tape. The cage over the tube radio stands well clear of it. The line filter's cage is layers of
tape over the circuit. Chapter 5, section 5.4, describes each cage and how it is earthed.

## 3.6 Connectors and labels

This section lists the main connectors, and how the cables are marked.

- **Mains inlet:** a standard IEC C14 inlet, the one that came on the amplifier's aluminium back plate.
  That plate is at the back of the cabinet. The amplifier's own volume control is reached only from the
  back. Where the amplifier's input panel sits is not recorded.
- **Connectors that fit only one place.** Some connector types are used once only, so that a cable cannot
  be plugged where it would break something. The mains feed to the front power switch is one of them.
- **Labels.** Each cable is marked at both ends, with an evident name or with its name on the cable
  sheet. This book uses the cable sheet's names (chapter 10).
- **Power sockets.** The 5 V and 3.3 V buses are rows of small 2-pin JST XH sockets (chapter 5,
  section 5.5).
- **Back-panel USB-C ports.** One for each microcontroller board (chapter 4, section 4.6).
- **Antenna connector:** a BNC (J10) on the back panel, where the FM antenna's coax arrives (chapter 11).

## 3.7 Printed parts

Every printed part of this build is listed in Appendix D. All are PLA except the membrane of the passive
radiator, which is TPU.
