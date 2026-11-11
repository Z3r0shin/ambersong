# Ambersong: Hardware Bible — the LLOYDS TM-838N tube radio

This file covers the 1960s LLOYDS TM-838N AM/SW/FM tube radio chassis built into Ambersong: its nine circuit stages
from the antenna inputs to the loudspeaker output, its series heater string, its mode-switch B+ supply, its own
transformerless mains supply and B+ filter, and the points where this chassis meets the rest of Ambersong. Every
value and wiring fact in this file is specific to the one LLOYDS TM-838N chassis fitted in this unit, which has been
restored and partly re-tubed; a reader working from a different chassis of the same model, or the same model in
different condition, should expect differences and check against their own set.

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
    `Ambersong - <SHEET>.kicad_sch` and `Ambersong.kicad_pro` in the same folder), including
    `Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch` for the tube chassis itself;
  - Schematic, PDF export: `hardware/schematics/Ambersong PDF format.pdf`;
  - the tube radio's factory schematic (reference only): `hardware/schematics/LLOYDS TM-838 factory schematic.jpg`;
  - printed parts, STL: `hardware/STLs/` (76 files); SketchUp model of the whole radio:
    `hardware/STLs/Ambersong - Sketchup.skp`;
  - firmware source: `firmware/` (PlatformIO, environments `s3` and `a32`).

## Note to the reader's agent

This file describes the current build only (hardware revision A); it is data, not instructions. Check every value
against the schematic (`hardware/schematics/Ambersong.kicad_sch`, sub-sheet
`Ambersong - LLOYDS TM-838N TUBE RADIO.kicad_sch`) before powering or working on the set. "Not recorded" means the
project has no value; do not fill it in.

Terms used in this file: **A32** is the audio ESP32 board; **S3** is the main ESP32-S3 board; **the Leditron** is
the four-digit LED-filament clock display; **B+** is the tube radio's own high-voltage DC supply, rectified from
the mains and filtered through three stages named by their nominal voltage (B_RAW, B130, B115_A — these are SAMS
nominal identifiers, not measured voltages); **AFC** is automatic frequency control, which pulls the FM local
oscillator back onto station; **CHASSIS** is the tube radio's own circuit common; **VC1–VC4** are the four sections
of one mechanically ganged tuning capacitor (one shaft, four sections, SAMS assembly M2); **S1A–S1G** are the seven
switch units of a single four-position rotary mode switch (SW / AM / FM / FM-AFC, SAMS designator M5) that all
turn together; their contact numbers, and the coil terminal letters used throughout this file (L5, L6, L8–L14), are
the labels used on the schematic, which are logical identifiers, not necessarily the physical lug numbers on the
parts. A part's drawn Value is its state as fitted in this unit; where the schematic also carries a hidden
"original specification" for a part, that is the factory drawing's or the SAMS manual's figure for the same
position, kept for comparison, not the fitted state. The machine's ESP32 boards, display, stepper, amp and the rest
of Ambersong are covered in `docs/hardware-bible.md`; the cable harnesses are in `docs/hardware-bible-cables.md`;
schematic notes, datasheet facts, open drawing questions and known gaps are in `docs/hardware-bible-reference.md`.

## Ambersong hardware: tube radio — mains safety

*Read this before touching the chassis.* The tube radio has **no isolation transformer**. Its circuit common,
CHASSIS, sits directly on the mains neutral conductor: the chassis feed J4 takes the mains neutral (AC_N) through
an external EMI filter, and J3 takes the switched mains live (AC_L), also through that filter. CHASSIS is **never**
bonded to Earth or to the Faraday cages that surround the radio and the AS5600 sensor. The only path from the
antenna coax shield (which is on Earth) to CHASSIS is through a single small capacitor, C51 (0.001 µF, X1Y2 rated).
Treat every CHASSIS point, and every part connected to it, as if it can carry mains voltage, whether or not the
radio is switched on at the front panel, because switching happens after the mains inlet. A Faraday cage also
covers the AC line filter between the tube radio's mains input and Ambersong's own mains input (after the front
power switch); it is bonded to the same earth as the other cages, and is not drawn on any schematic sheet.

## Ambersong hardware: tube radio — FM RF amplifier and mixer (V1, 12DT8)

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

V1 is a 12DT8 twin triode. Its first triode section is the FM RF amplifier; its second is the FM mixer; its third
"section" is not a signal triode at all but the tube's heater and internal shield pin, which belong to the series
heater string (below). The antenna feed was changed from the factory's simple wire to a coaxial connector, J53:
centre conductor to the amplifier input, shield to the ground-return path described below.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| V1 (triode 1) | FM RF amplifier triode: plate pin 1, grid pin 2, cathode pin 3 | 12DT8 | not changed |
| V1 (triode 2) | FM mixer triode: plate pin 6, grid pin 7, cathode pin 8 | 12DT8 | not changed |
| J53 | FM antenna coax input; centre to the amplifier input, shield to the ground-return capacitor | RG179 coax | not specified by either original source — part of the antenna upgrade |
| C3 | Antenna input coupling | 100 pF | factory and SAMS agree, 100 pF |
| L2 | Antenna choke to chassis | 1 µH, 23 turns | SAMS adds the turn count only |
| R2 | RF cathode bias | 70 Ω | factory 70 Ω; SAMS lists 68 Ω, with 70 Ω noted as an alternate |
| C4 | RF cathode bypass | 0.002 µF | factory and SAMS agree, 2000 pF |
| C51 | Antenna shield's only path to chassis; X1Y2 rated | 0.001 µF | not in either original source — part of the antenna upgrade |
| L3 | RF plate choke | 3 µH | factory and SAMS agree, 3 µH (SAMS adds ≈0.9 Ω winding resistance) |
| C5 | RF supply bypass | 0.002 µF | factory 2000 pF and the SAMS parts table agree; the SAMS schematic prints 10000 pF |
| C6 | RF-to-mixer coupling | 100 pF | factory and SAMS agree, 100 pF |
| L4 | RF tank coil | FM RF tank (fitted, no inductance given) | neither source gives an inductance or Q |
| C7 | RF tank fixed capacitor | 5 pF | factory and SAMS agree, 5 pF |
| C8 | RF tank trimmer | adjustable, no capacitance given | neither source gives a range |
| VC1 | FM RF section of the 4-gang tuning capacitor (VC1–VC4 on one shaft, SAMS assembly M2) | tuning section, no range given | section of M2; no numeric range in either source |
| R3 | Mixer cathode bias | 1 kΩ | factory and SAMS agree, 1 kΩ |
| C9 | Mixer cathode bypass | 0.01 µF | factory and SAMS agree, 10000 pF |

**Signal path.** The antenna's centre conductor feeds C3, which couples into a node carrying L2 (choke to chassis)
and the cathode-bias pair R2/C4. From there the RF amplifier's cathode (pin 3) returns through R2/C4; its grid
(pin 2) is grounded to chassis; its plate (pin 1) is fed B+ through choke L3 (bypassed to chassis by C5) and
couples through C6 into the tuned RF tank — L4, C7, the trimmer C8 and the tuning-gang section VC1, all returning
to chassis — at the mixer's grid (pin 7). The same tank point also receives a 2 pF feed from the local oscillator
(described in the next section). The mixer's cathode (pin 8) returns through R3/C9 to chassis; its plate (pin 6)
feeds, by schematic label only, into the primary of the first FM IF transformer (the next section's L10). The B+
rail that feeds this stage, labelled FM_B, is switched onto the chassis's filtered B+ by the mode switch (unit
S1D, in its FM and FM-AFC positions); the same B+ rail also reaches the AFC/oscillator stage and the first FM IF
stage. The antenna shield returns to chassis only through C51; that return point is otherwise on Earth, never
joined to chassis directly.

## Ambersong hardware: tube radio — FM local oscillator and AFC (V2, 12DT8)

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

V2 is the second 12DT8. One triode section is the FM local oscillator; the other is the AFC (automatic frequency
control) triode; the third section is again the heater/shield pin, part of the heater string. The oscillator runs
below the station frequency (station = oscillator + IF, low-side injection), confirmed on two separate recordings
of the set in use. The oscillator circuit is drawn exactly as fitted; an older schematic note questioning it is
answered — nothing here is a drawing error.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| V2 (triode 1) | FM local-oscillator triode: plate pin 1, grid pin 2, cathode pin 3 | 12DT8 | not changed |
| V2 (triode 2) | AFC (control) triode: plate pin 6, grid pin 7, cathode pin 8 | 12DT8 | not changed |
| R5 | AFC control feed | 250 kΩ | factory and SAMS agree |
| C11 | AFC shunt to chassis — **a shunt in this radio, and always was**; the schematic once drew it as a series part by mistake | 0.002 µF | factory 2000 pF (drawn as series coupling); SAMS 1000 pF (drawn as a shunt). Whether the factory original was really series or shunt is not settled — see `docs/hardware-bible-reference.md` |
| R6 | AFC grid series resistor | 100 Ω | factory and SAMS agree |
| C12 | AFC control bypass | 0.01 µF | factory 10000 pF; SAMS 1000 pF |
| R33 | FM oscillator cathode bias | 600 Ω | not separately specified; shares its schematic data with R9 below |
| C50 | FM oscillator cathode bypass | 0.01 µF | not separately specified; shares its schematic data with C19 below |
| R7 | AFC cathode bias | 600 Ω | factory and SAMS agree |
| C13 | AFC cathode bypass | 0.01 µF | factory 10000 pF and the SAMS table agree; the SAMS schematic prints 2000 pF |
| L1 | AFC plate RF choke | 3 µH | factory and SAMS agree, 3 µH |
| R8 | AFC plate feed | 1 kΩ | factory and SAMS agree |
| C14 | AFC plate-supply bypass | 0.002 µF | factory and SAMS agree, 2000 pF |
| C15 | AFC-to-oscillator coupling | 10 pF | factory and SAMS agree |
| C16 | RF/oscillator coupling, into the mixer grid | 2 pF | factory and SAMS agree, 2 pF |
| C17 | Oscillator tank fixed capacitance | 5 pF | factory and SAMS agree |
| C18 | Oscillator trimmer | adjustable, no capacitance given | neither source gives a range |
| VC2 | FM oscillator section of the 4-gang tuning capacitor | tuning section, no range given | section of M2; no numeric range in either source |
| L5 | FM oscillator coil, two coupled windings (tank and feedback) | coupled coils, no inductance given | factory: coupled two-winding oscillator. SAMS instead describes a single tapped winding — **do not substitute a tapped coil for the fitted coupled-winding coil by name alone** |
| C19 | Oscillator grid feedback | 50 pF | factory and SAMS agree |
| R9 | Oscillator grid leak, across C19 | 20 kΩ | factory and SAMS agree |
| R10 | Oscillator plate feed | 30 kΩ | factory 30 kΩ "with a different supply connection"; SAMS 10 kΩ |
| C20 | Oscillator plate coupling | 20 pF | factory 20 pF coupling capacitor; SAMS's part at this position is a different 2000 pF bypass — the two serve different circuit functions, so SAMS's substitute advice for its part does not apply to the fitted 20 pF part |

**Signal path.** The oscillator plate (pin 1) is fed from the AFC supply rail through R10 and couples through C20
into the oscillator tank, which also carries the fixed capacitor C17, the trimmer C18 and the tuning-gang section
VC2, all returning to chassis; the AFC plate (through choke L1 and coupling C15) also feeds into this same tank.
From the tank, C16 (2 pF) couples the oscillator signal out to the FM mixer grid described in the previous section.
The oscillator's feedback winding on L5 drives the oscillator grid (pin 2) through C19, with grid-leak R9 across
it; the oscillator cathode (pin 3) returns through R33/C50 to chassis. On the AFC side, the control line AFC_CTL
arrives from the ratio detector (next section but one) through a 3 MΩ resistor, is filtered by R5/C11/R6 into the
AFC grid (pin 7), and the AFC triode's plate (pin 6), fed through L1 from the local B+ rail (itself fed through R8
from the FM_B rail), couples through C15 into the oscillator tank — this is how the detected frequency error pulls
the oscillator back onto station. In the plain FM position (not FM-AFC), mode-switch unit S1A grounds the AFC
control line directly to chassis, disabling the correction; in FM-AFC that contact opens and the correction line is
live. The heater current for this stage's tube continues the series string described below.

## Ambersong hardware: tube radio — 10.7 MHz FM IF path (V4 first IF, V5 limiter, 12BA6 × 2)

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

**The fitted FM IF is 10.6 MHz, not 10.7 MHz.** The three FM IF transformers (L10, L12, L14 below) were
professionally retuned by Radio Hosvep on Park Ave.; only the FM path was touched, not the AM/SW path. 10.7 MHz is
the design value printed in the section title and on the transformer labels, and is not the fitted tuning. Why the
tuner chose 10.6 MHz is not known; the IF has not been measured independently since, and his setting stands as the
current build state.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| V4 | First FM IF pentode (12BA6): plate pin 5, screen pin 6, control grid pin 1, suppressor pin 2, cathode pin 7 | 12BA6 | not changed |
| V5 | FM limiter pentode (12BA6), same pin layout as V4 | 12BA6 | not changed |
| C10 | First FM IF supply bypass | 0.01 µF | factory and SAMS agree, 10000 pF |
| R12 | First IF cathode bias | 70 Ω | factory 70 Ω; SAMS lists 68 Ω, 70 Ω noted as an alternate |
| C28 | First IF cathode bypass | 0.002 µF | factory 2000 pF; SAMS (both schematic and table) 10000 pF — five times the fitted value |
| L10 | First FM IF transformer (10.7 MHz design value; fitted tuning 10.6 MHz); primary/secondary/can, internal tuning capacitors built into the can | FM IF A | neither source gives the can's internal capacitor values; SAMS gives winding resistances only (≈0.8 Ω each winding) |
| R14 | FM limiter grid return | 250 kΩ | factory and SAMS agree |
| C30 | FM limiter grid-return bypass | 50 pF | factory and SAMS agree, 50 pF |
| L12 | Second FM IF transformer, same design/fitted frequency note as L10 | FM IF B | neither source gives the can's internal capacitor values; SAMS gives winding resistances only (≈0.6 Ω / 0.4 Ω) |
| R18 | Ratio-detector supply decoupling | 1 kΩ | factory and SAMS agree |
| C33 | Ratio-detector plate-supply bypass | 0.01 µF | factory and SAMS agree |
| L14 | FM ratio-detector transformer: primary, secondary, tertiary, can; internal tuning capacitors built into the can | FM ratio detector | neither source gives the can's internal capacitor values; SAMS gives winding resistances only |
| R4 | First FM IF supply decoupling | 1 kΩ | factory and SAMS agree |
| R13, C29 | Second FM IF screen supply feed and bypass (R13 1 kΩ, C29 0.01 µF) | 1 kΩ / 0.01 µF | factory and SAMS agree |

**Signal path.** V4's plate (pin 5) feeds L10's primary from the FM mixer's output; L10's secondary drives V4's own
control grid through a return path shared with the AM IF chain (see the next section). V4's cathode (pin 7) returns
through R12/C28 to chassis; its plate load continues through L12's primary (in series with L13's primary from the
AM chain — the two share a mid-point node, but L13's own primary load comes only from the AM IF rail, never from
FM_B); its screen (pin 6) is fed from the general B+ rail through R13, bypassed by C29. L12's secondary drives V5
(the limiter) at its control grid, with the grid return through R14 in parallel with C30; V5's plate (pin 5) feeds
L14's primary. V5's screen (pin 6) is fed from the FM_B rail through R18, bypassed by C33 — this is the ratio
detector's own B+ feed, separate from the plain AM/FM B+ switching that reaches the rest of this stage. L14's
secondary and tertiary windings feed the ratio detector and the AFC line, covered in the next section. Heater
current for both tubes continues the series string below; V4's suppressor and V5's suppressor and cathode return
to chassis, which (as throughout this chassis) sits on mains neutral — see the mains-safety note above.

## Ambersong hardware: tube radio — FM ratio detector and AFC control

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

The FM ratio detector recovers the audio from the limiter stage and also produces the AFC control voltage. Its two
diodes are drawn in the **factory orientation**, which is opposite to the SAMS manual's, and this is the fitted
state: the upper diode's cathode faces the transformer, the lower diode's cathode faces the load. The stabilising
capacitor C2 is electrolytic, and its drawn polarity is the fitted state.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| C2 | Ratio-detector stabilising capacitor (electrolytic); + on the lower detector rail | 10 µF | capacitance agrees with both originals (10 µF); SAMS states the opposite polarity (+ to the upper rail) and a 50 V rating — the fitted polarity is as drawn, opposite to SAMS; no fitted voltage rating is recorded |
| C34 | Upper balancing capacitor | 200 pF | factory and SAMS agree |
| C35 | Lower balancing capacitor | 200 pF | factory and SAMS agree |
| C36 | Detector output RF bypass to chassis | 200 pF | factory and SAMS agree |
| C37 | FM audio de-emphasis/filter capacitor | 0.001 µF | factory and SAMS agree, 1000 pF |
| M3 | Upper ratio-detector diode (type not stated by either source) | cathode toward the IF transformer, anode on the upper rail | factory orientation; opposite to SAMS — confirm actual polarity before replacing |
| M4 | Lower ratio-detector diode (type not stated by either source) | cathode on the lower rail, anode toward the IF transformer | factory orientation; opposite to SAMS — confirm actual polarity before replacing |
| R19 | Upper series resistor, to M3 | 1 kΩ | factory and SAMS agree |
| R20 | Upper DC load, to chassis | 10 kΩ | factory and SAMS agree |
| R21 | Lower series resistor, to M4 | 1 kΩ | factory and SAMS agree |
| R22 | Lower DC load, to chassis | 10 kΩ | factory and SAMS agree |
| R23 | AFC feed, to the AFC control line | 3 MΩ | factory and SAMS agree |
| R24 | FM de-emphasis/filter resistor | 50 kΩ | factory and SAMS agree |
| S1A | Mode-switch unit, "AFC disable": grounds the AFC control line to chassis in the plain FM position; open in FM-AFC, SW and AM | one unit of the single mode switch | SAMS designator M5; four positions, three physical wafer sections — the schematic's seven S1 units (S1A–S1G) are logical sections of this one switch, not seven separate switches |
| S1B | Mode-switch unit, "audio mode": selects AM or FM detected audio onto the volume control | one unit of the single mode switch | same switch as S1A |

**Signal path.** L14's secondary feeds R19 and R21 (1 kΩ each) into the two diodes M3/M4; the diodes' outer ends
(the upper and lower detector rails) carry C34/C35, the two 10 kΩ loads R20/R22 (whose midpoint is chassis) and
C2's two terminals. The detector output node — shared by the C34/C35 midpoint and L14's tertiary winding — feeds
C36 to chassis, R24 (then C37) toward the recovered FM audio, and R23 toward the AFC control line described in the
oscillator section above. The recovered FM audio reaches mode-switch unit S1B, which in the FM and FM-AFC
positions puts it onto the volume control; in SW and AM, S1B instead selects the AM detector's audio (next
section). S1A grounds the AFC control line only in the plain FM position; it is open in FM-AFC (where the AFC
loop is active), and also open — with no AFC effect either way — in SW and AM.

## Ambersong hardware: tube radio — 455 kHz AM IF and AM detector

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

The AM detector is a pair of diodes inside the 12AV6 (V6), the same tube whose triode section is the first audio
amplifier (covered two sections below); the diodes and the triode share one cathode pin. **No conflict was found
between the factory drawing and SAMS for any part in this stage.**

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| L11 | First AM IF transformer (455 kHz); primary, secondary, can, internal tuning capacitors built in | AM IF A | SAMS gives only winding resistances (≈22 Ω each winding); neither source gives the can's internal capacitor values |
| L13 | Second AM IF transformer (455 kHz), feeding the AM detector diodes in V6 | AM IF B | SAMS gives only winding resistances (≈22 Ω / 20 Ω) |
| C29 | Second AM IF / screen supply bypass | 0.01 µF | factory and SAMS agree |
| C31 | AM detector IF bypass | 100 pF | factory and SAMS agree |
| C32 | AM audio IF bypass | 100 pF | factory and SAMS agree |
| R13 | AM IF / screen decoupling | 1 kΩ | factory and SAMS agree |
| R15 | AM detector load / filter | 50 kΩ | factory and SAMS agree |
| R16 | AM detector DC return | 1 MΩ | factory and SAMS agree |
| R17 | AM AVC (automatic volume control) feed | 2 MΩ | factory and SAMS agree |

**Signal path.** The AM/SW converter's plate (V3, covered next) feeds L11's primary; the mode switch (unit S1D)
puts the general B+ onto L11's primary and the converter's screen grids whenever SW or AM is selected. L11's
secondary feeds back into the FM IF path's first transformer secondary in series, reaching V4's control grid — the
same path the FM IF signal uses, since the two stages share that grid. Further along, R17 (2 MΩ) carries the AM
AVC line out to the AM/SW front end (AVC return, covered in the next section), and L13's primary continues in
series from V4's plate load (see the FM IF section). L13's secondary drives both diode plates of V6 (pins 5 and 6);
their common cathode (pin 2) is on chassis. The detector load side of L13's secondary feeds C31 to chassis and
R15 toward the recovered AM audio node, where R16 (1 MΩ) and C32 (100 pF) also return to chassis and R17 carries
the AVC feedback. The recovered AM audio reaches mode-switch unit S1B's SW and AM contacts, the same switch
described in the ratio-detector section, which puts it on the volume control in those two positions. **Note: the
drawn neon mode-indicator bulbs (NE1–NE3, covered under "mode supply and indicator lamps" below) are absent from
this chassis** — the sheet shows them only as a representation of the original factory circuit. **There is no
SW/AM antenna fitted** (see the next section): only the FM antenna, J53, is connected.

## Ambersong hardware: tube radio — AM/SW input, converter (V3, 12BE6) and SW/AM local oscillator

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

V3 is a 12BE6 pentagrid converter: it combines the AM/SW RF input with the SW/AM local oscillator to produce the
455 kHz IF signal fed to the previous section. **The SW antenna terminal (J2) is not fitted in this chassis**; every
other part of the AM/SW front end and oscillator described here is fitted.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| V3 | 12BE6 pentagrid converter: plate pin 5, screen pins 6, signal grid pin 7, oscillator grid pin 1, cathode pin 2 | 12BE6 | not changed |
| J2 | SW antenna terminal | **absent from this chassis** | not applicable |
| C21 | SW antenna input coupling | 450 pF | factory and SAMS agree; note that several commonly listed historical substitute parts for this position are 470 pF, not 450 pF |
| L6 | SW antenna matching transformer (two windings) | SW antenna coil | SAMS gives winding resistance only (≈0.3 Ω each winding, 3.5–9 MHz band) |
| C22A | SW antenna trimmer | adjustable, no capacitance given | neither source gives a range |
| L7 | AM ferrite loopstick antenna | AM loopstick | SAMS gives winding resistance only (≈0.6 Ω, 540–1620 kHz band) |
| C23A | AM antenna trimmer | adjustable, no capacitance given | neither source gives a range |
| VC3 | AM/SW RF section of the 4-gang tuning capacitor | tuning section, no range given | section of M2; no numeric range in either source |
| C24 | AM AVC return bypass | 0.05 µF | factory and SAMS agree, 50000 pF, 50 V stated by SAMS |
| R11 | AM/SW oscillator grid leak | 20 kΩ | factory and SAMS agree |
| C25 | AM/SW oscillator feedback | 100 pF | factory and SAMS agree |
| VC4 | AM/SW oscillator section of the 4-gang tuning capacitor | tuning section, no range given | section of M2; no numeric range in either source |
| C26 | SW oscillator padder | 0.003 µF | factory and SAMS agree, 3000 pF, ±5% |
| L9 | SW oscillator coil, tapped winding | SW oscillator coil | SAMS gives winding resistances only (≈0.3 Ω / 0.1 Ω sections) |
| C22B | SW oscillator trimmer | adjustable, no capacitance given | neither source gives a range |
| C27 | AM oscillator padder | 350 pF | factory and SAMS agree, 350 pF — some commonly listed historical substitutes for this position are a different value; keep 350 pF |
| L8 | AM oscillator coil, tapped winding | AM oscillator coil | SAMS gives winding resistances only (≈4.6 Ω / 0.9 Ω sections) |
| C23B | AM oscillator trimmer | adjustable, no capacitance given | neither source gives a range |
| S1C | Mode-switch unit, "RF selection": puts SW_RF or AM_RF onto the converter's signal grid; no connection in FM / FM-AFC | one unit of the single mode switch | SAMS designator M5 (see the ratio-detector section for the switch as a whole) |
| S1F | Mode-switch unit, "oscillator tank": puts the SW or AM oscillator padder/trimmer onto the oscillator tuning-gang section; no connection in FM / FM-AFC | one unit of the single mode switch | same switch |
| S1G | Mode-switch unit, "oscillator feedback": puts the SW or AM oscillator coil tap onto the converter's cathode; no connection in FM / FM-AFC | one unit of the single mode switch | same switch |

**Signal path.** In the SW position, the antenna feed runs C21 → L6 primary (returning to chassis) → L6 secondary,
trimmed by C22A, to mode-switch unit S1C's SW contact. In the AM position, the ferrite loopstick L7, trimmed by
C23A, feeds S1C's AM contact instead; the loopstick's other end is the AVC line, also reaching R17 (previous
section) and the bypass C24. S1C's common feeds the converter's signal grid (pin 7) together with the RF
tuning-gang section VC3. The converter's oscillator grid (pin 1) is fed through R11/C25 from the oscillator tank
node, which is the common of mode-switch unit S1F and the oscillator tuning-gang section VC4; S1F's SW and AM
contacts carry the two padders C26/C27, which feed the tops of the oscillator coils L9 (SW) and L8 (AM). Those
coils' taps feed mode-switch unit S1G, whose common returns to the converter's cathode (pin 2); the coils' bottoms
return to chassis. The converter's plate (pin 5) and screen (pin 6) feed, by label, into the AM IF transformer and
B+ switching described in the previous section. In FM and FM-AFC, S1C, S1F and S1G are all open (no-connect), so
none of this front end is live.

## Ambersong hardware: tube radio — audio amplifier, tone control and output transformer (V6 12AV6, V7 50C5)

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

V6 (12AV6) is both the AM detector (previous sections) and the first audio amplifier triode; V7 (50C5) is the
output tube. **The output transformer T1's secondary is drawn in its original factory form in this section, but
that is not how it is actually wired**: the fitted secondary wiring is on the main Ambersong sheet, described at
the end of this section. The transformer's primary (plate to B+) is as drawn here, with no tap.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| V6 | 12AV6: triode grid pin 1, triode plate pin 7, shared cathode pin 2, diode plates pins 5/6 (the AM detector, previous section) | 12AV6 | not changed; one of three new-old-stock tubes fitted by Radio Hosvep on Park Ave. (with V1, V2) |
| V7 | 50C5 beam power output tube: plate pin 7, screen pin 6, control grid pin 2, cathode pin 1; nothing wired to pin 5 | 50C5 | not changed |
| R1 (tube radio volume control) | Volume potentiometer, wiper to the audio amplifier grid (note: reference R1 is this part only — the clock's colon dimmer, also once R1, was renamed R99) | 500 kΩ | factory and SAMS agree; SAMS adds audio taper, ≤½ W |
| C38 | Volume-wiper to triode-grid coupling | 0.01 µF | factory and SAMS agree, 10000 pF; SAMS states 450 V |
| R25 | Triode grid leak | 2 MΩ | factory and SAMS agree |
| R26 | Triode plate load | 250 kΩ | factory and SAMS agree |
| C39 | Triode plate HF shunt to chassis | 100 pF | factory and SAMS agree |
| C42 | Triode-plate to output-grid coupling | 0.01 µF | factory and SAMS agree, 10000 pF; SAMS states 450 V |
| R27 | Output-tube grid leak | 500 kΩ | factory and SAMS agree |
| R28 | Output-tube cathode bias | 150 Ω, 1 W | factory and SAMS agree |
| C40 | Tone capacitor, first section | 0.002 µF | factory and SAMS agree, 2000 pF |
| C41 | Tone capacitor, second section | 0.002 µF | factory and SAMS agree, 2000 pF |
| S2 (tone unit) | Power/tone selector, one physical switch with a power pole (next section) and a tone pole: HIGH switches no capacitor in, MID puts C40 and C41 in series (≈1000 pF), LOW switches C40 alone (2000 pF) | fitted as described | factory and SAMS agree on this tone network's behaviour |
| T1 | Audio output transformer: two-terminal primary (plate to B+), no tap | fitted, no tap | factory: single-ended, no tap. SAMS describes a tapped primary (7 kΩ total, tap at 2.5 kΩ) with an unused section — **the fitted transformer has no such unused section; do not apply SAMS's full 7 kΩ primary figure to it** |
| C43 | Output transformer primary shunt | 0.0047 µF | factory and SAMS agree, 5000 pF (fitted value is the next size down, 4700 pF); SAMS states 450 V |
| SP1, SP2 | Original factory loudspeakers, drawn in series on the transformer secondary in this stage's factory-form diagram | 4 × 6 inch, 3–4 Ω each, ≈6–8 Ω in series | **not recorded** whether SP1/SP2 are still physically present in the cabinet — in this build the transformer secondary instead feeds the audio chain described below, so no speaker is loaded by it either way |

**Signal path.** The AM or FM detected audio (selected by mode-switch unit S1B, previous sections) reaches the top
of volume control R1; its wiper feeds the audio triode's grid (V6 pin 1) through coupling capacitor C38, with grid
leak R25 to chassis. The triode's plate (pin 7) carries load resistor R26 (fed from the chassis's B+ rail named
B115_A), HF shunt C39, the first tone capacitor C40, and coupling capacitor C42 into the output tube's grid (V7
pin 2), with grid leak R27 to chassis. The tone network's second capacitor C41 and the tone-selector pole of switch
S2 sit between the first tone capacitor and chassis, giving the HIGH/MID/LOW behaviour in the parts table above.
The output tube's cathode (pin 1) returns through R28 to chassis; its screen (pin 6) is fed from B115_A directly.
Its plate (pin 7) drives the output transformer's primary, with C43 across the primary and the other primary
terminal on the B+ node named B130. **The transformer's secondary, drawn here in its original form (feeding SP1
and SP2 in series), is not the fitted wiring**: in this build the secondary's two leads instead reach a 7.5 Ω,
5 W load (part marking 5W7Ω5J) on the main Ambersong sheet, and from there a jack whose tip and ring are
interchangeable feeds the cable run into the PCM1802 ADC described in `docs/hardware-bible.md`. Heater current for
both V6 and V7 continues the series string below.

## Ambersong hardware: tube radio — series heater string (all seven tubes)

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

All seven tube heaters are wired in one series chain, fed from the fused, switched mains live. **The fitted heater
order was checked on the chassis and runs: V7 (50C5) → V5 (12BA6, limiter) → V6 (12AV6) → V4 (12BA6, first IF) →
V3 (12BE6) → L16 (RF choke) → V1 (12DT8, RF amp/mixer) → L15 (RF choke) → V2 (12DT8, oscillator/AFC), which returns
to chassis.** This order interchanges the two 12DT8s (V1, V2) against an earlier reading of the schematic: V2, not
V1, is the tube whose heater pin returns to chassis.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| L16 | RF heater choke, between V3's and V1's heater | RF heater choke, 23 turns | neither source gives enough to pick a replacement (turns alone do not define one) |
| L15 | RF heater choke, between V1's and V2's heater | RF heater choke, 23 turns | same as L16 |
| C47 | Heater-string RF bypass at the V3/V4 junction | 0.0022 µF | both original drawings give 2000 pF; the fitted part is a 2200 pF replacement |
| C48 | Heater bypass at V1's pin-5 side; **the original three-lead ceramic part, never replaced** | 0.002 µF | factory and SAMS agree, 2000 pF — this is the only one of the four heater bypasses still at the original value and construction |
| C52 | Heater bypass at V1's pin-4 side (an X1Y2 safety capacitor); this is the part that had at one point been mistaken for C48 | 0.0022 µF | not separately specified by either original |
| C49 | Heater bypass at V2's pin-5 side | 0.0022 µF | both original drawings give 2000 pF; the fitted part is a 2200 pF replacement |

**Signal path.** Starting from the fused live rail (AC_FUSE_OUT, described in the mains-supply section below): V7
pin 4 → V5 pin 4 → (V5 pin 3) → V6 pin 4 → (V6 pin 3) → V4 pin 4, with C47 bypassing that junction to chassis → (V4
pin 3) → V3 pin 4 → (V3 pin 3) → choke L16 → V1 pin 5, with C48 bypassing that junction to chassis → (V1 pin 4) →
choke L15 → V2 pin 5, with C49 bypassing that junction to chassis → (V2 pin 4) → chassis. Both 12DT8s' internal
shield pins (pin 9 on each) also return directly to chassis. Because chassis sits on mains neutral (see the
mains-safety note above), every point along this string is at a mains-referenced potential, not an isolated low
voltage, even though the heaters themselves run on AC/DC series current rather than a stepped-down heater winding.

## Ambersong hardware: tube radio — mode-switch B+ supply and indicator lamps

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

Besides switching signal paths (described throughout the earlier sections), two more units of the same single
mode switch handle the B+ routing between AM/SW and FM, and the (unused) mode indicator lamps.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| S1D | Mode-switch unit, "B+ mode selection": puts the filtered B+ rail B115_A onto the AM_B rail in SW/AM, or onto the FM_B rail in FM/FM-AFC | one unit of the single mode switch | SAMS designator M5 (see the ratio-detector section for the switch as a whole) |
| S1E | Mode-switch unit, "mode lamp": would feed the current-limited lamp supply to whichever lamp matches the selected band | one unit of the single mode switch; its contacts are fitted even though the lamps are not | same switch |
| R32 | Neon lamp current limiter, fed from the fused heater-string rail | 50 kΩ | factory and SAMS agree; neither gives a wattage — establish the lamp's actual voltage/current before buying a replacement resistor |
| NE1, NE2, NE3 | SW, AM and FM mode-indicator neon lamps | **absent from this chassis** — the schematic shows them only as a representation of the original factory circuit | neither source gives a lamp model, striking voltage or current |

**Signal path.** S1D's common is the filtered B+ rail B115_A (see the mains-supply section below); in SW and AM it
connects that rail to AM_B, which feeds the AM IF transformer and the AM/SW converter's screen grids (as described
in those sections); in FM and FM-AFC it connects B115_A to FM_B instead, feeding the FM RF, FM oscillator/AFC and
first FM IF stages. S1E's common receives the current-limited lamp supply from R32 and would route it to whichever
of NE1/NE2/NE3 matches the selected band — but since none of the three bulbs is actually fitted, this part of the
switch has no visible effect in the running machine.

## Ambersong hardware: tube radio — mains supply and B+

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

This is the chassis's own **transformerless** AC-to-DC supply: a half-wave rectifier and a three-stage resistor/
capacitor filter, producing the B+ rail that feeds every tube's plate and screen circuits described above. Read
the mains-safety note at the top of this file before working on any part of this supply.

| Part | Function | Fitted value | Original specification |
|---|---|---|---|
| J3 | Mains input, switched live (fed from Ambersong's front-panel switch through the external EMI filter) | fitted | not applicable |
| J4 | Mains input, neutral; its net is the same conductor as chassis | fitted | not applicable |
| S2 (power unit) | Power switch, physically one part with the tone switch described earlier | fitted; the exact physical lug used is not recorded | not applicable |
| M1 | Mains fuse in the switched live | 0.75 A | factory drawing has no fuse shown at all; SAMS gives 0.75 A, type GJV, at a different position in its own drawing — this chassis's fuse position and the fitted part's type/voltage rating beyond 0.75 A are not otherwise recorded |
| C44 | Fused-live to chassis bypass, X1Y2 rated | 0.01 µF | factory gives 0.01 µF at this function; SAMS's nearest same-function part is in a different position in its drawing, so the two are not a direct comparison |
| R29 | Rectifier surge-limiting resistor | 28 Ω, 3 W | factory and SAMS agree |
| X1 | Half-wave silicon rectifier | silicon rectifier (no part number recorded) | factory: silicon rectifier. SAMS: measured output current 0.090 A, no device rating printed — as a selection guideline only, a verified historical class around 400 V / 0.5 A with adequate surge rating is suggested; it is not a measured or fitted rating |
| C46 | Capacitor across the rectifier | 0.0047 µF | factory and SAMS agree, 5000 pF (fitted value is the next size down, 4700 pF); SAMS states 450 V |
| C1A | B+ reservoir capacitor, on the raw rectified rail | 47 µF | factory and SAMS describe one 40 µF, 150 V section of a common-negative multi-section can; the fitted part is instead one of three separate 47 µF, 315 V capacitors (Rubycon) |
| R30 | First B+ filter dropping resistor | 150 Ω, 5 W | factory and SAMS agree on resistance; SAMS states 2 W, below the fitted 5 W |
| C1B | First filtered B+ node capacitor | 47 µF | same change as C1A: a separate 47 µF, 315 V Rubycon capacitor, replacing a 40 µF, 150 V can section |
| R31 | Second B+ filter dropping resistor | 300 Ω, 1 W | factory and SAMS agree |
| C1C | Second filtered B+ node capacitor | 47 µF | same change as C1A/C1B: a separate 47 µF, 315 V Rubycon capacitor, replacing a 40 µF, 150 V can section |

**Signal path.** The switched live reaches the power switch, then fuse M1, then the node named AC_FUSE_OUT, which
feeds the heater string (previous section), the lamp limiter R32, the surge resistor R29 and the bypass C44 (to
chassis). From R29 the AC reaches rectifier X1 (with C46 across it); the rectifier's output is the raw B+ rail,
filtered first by reservoir C1A, then dropped through R30 to the node named B130 (which feeds the output
transformer's primary, as described in the audio-amplifier section), then filtered by C1B, then dropped again
through R31 to the node named B115_A (which feeds the mode switch's B+ routing, the second audio triode's plate
load, the output tube's screen, and the AM IF/screen decoupling, as described in the relevant earlier sections),
then filtered by C1C. "B_RAW", "B130" and "B115_A" are SAMS's own nominal names for these nodes, not measured
voltages. This whole supply, and every B+ point it feeds, is referenced to chassis — which, as stated at the top
of this file, sits directly on the mains neutral conductor with no isolation transformer anywhere in the chain.

## Ambersong hardware: where the tube radio meets the rest of Ambersong

*Specific to this build: this is the one LLOYDS TM-838N chassis fitted in this unit; a different set will differ.*

The tube radio's own schematic sheet has no electrical connection, in KiCad terms, to any other Ambersong sheet: no
net label and no drawn wire is shared between them. Every meeting point between the two is a physical fact stated
in the project's records, not a drawn connection. A builder working from the schematic alone would not see any of
these links; they are given here instead.

- **Audio output.** The output transformer T1 (described in the audio-amplifier section above) is one physical
  part drawn on both the tube radio's own sheet and the main Ambersong sheet. Its primary is the radio's own 50C5
  output stage, as described above. Its secondary — drawn in its obsolete, original form on the tube radio's own
  sheet — is actually wired, on the main Ambersong sheet, to a 7.5 Ω, 5 W load (part marking 5W7Ω5J) and from there
  to a jack whose tip and ring are interchangeable, feeding the cable run into the PCM1802 ADC. See
  `docs/hardware-bible.md` for that ADC input stage and `docs/hardware-bible-cables.md` for the cable itself.
- **FM antenna.** J53's centre conductor is the radio's FM antenna input; its shield returns to chassis through
  C51 only, as described in the first section above. The antenna itself, as built and mounted, is covered in
  `docs/hardware-bible.md` (the FM antenna section); there is no SW/AM antenna fitted (see the AM/SW section
  above). A second antenna, for the RDA5807M tuning-calibration receiver (covered in `docs/hardware-bible.md`), is
  a 30 cm length of wire bent in a U over the FM oscillator section (the second section above), running loose
  inside the tube radio's own Faraday cage; it makes no electrical connection to anything on the tube radio's own
  sheet.
- **Mains and earth.** The radio's mains inputs J3 (switched live) and J4 (neutral) are fed, through an external
  EMI filter, from Ambersong's own front-panel switch and mains neutral respectively — see the mains-safety note
  at the top of this file. The radio's chassis is never bonded to earth. A separate Faraday cage, bonded to the
  same earth as the cage over the radio and the cage over the AS5600 sensor, covers the EMI filter itself, between
  the radio's mains input and Ambersong's own mains input (after the front power switch).
- **Enclosure box.** On the main Ambersong sheet, the whole tube radio sheet is represented by a single sheet box,
  which carries no sheet pins and shares no label name with any other sheet — consistent with there being no
  drawn electrical link between the two.

For the ESP32 boards, the audio converters, the amp, the display, the stepper and everything else outside the tube
chassis itself, see `docs/hardware-bible.md`; for the cable harnesses that physically carry these links, see
`docs/hardware-bible-cables.md`.
