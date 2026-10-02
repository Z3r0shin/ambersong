# 12. The amplifier and speakers

Every sound the machine makes leaves through one amplifier and its two speakers. The DAC's output (the
RADIO and BT sources) and the AUX input both arrive at the amplifier's input panel, which adds them
together.
This chapter says what the amplifier and the speakers are, what the input panel does, and what has been
measured there. It describes this speaker set; adapt it to yours.

**The amplifier's own circuit is not covered.** No drawing shows its boards.

## 12.1 What the amplifier and speakers are

The amplifier and speakers came from a **Kramer Tavor 5-O** powered speaker pair, part number
60-00009010: active left, passive right, 2 × 30 W RMS, 4 Ω. The active speaker held the amplifier; the
passive one did not. The table lists the parts.

| Part | What it is |
|---|---|
| **Power stage** | a **TDA7265** |
| **Input board** | three op-amp chips: a **TL074**, a **TL072** and a **C4558** |
| **Supply** | the amplifier's own isolated mains switching supply, measured at **±19.33 V with no load**. Two TO-220 regulators (a common three-pin power package) make ±12 V for the op-amps. It reaches the amplifier on four wires, the only wires in the machine whose colours are recorded (chapter 5, section 5.6). It also feeds the box fan and the amplifier power sensor (chapter 5, section 5.6; chapter 9, section 9.5). |
| **Auto mode** | a mode of the original amplifier; it has been removed. Nothing more about it is recorded. |
| **Volume control** | a B10k dual-gang potentiometer (section 12.2) |
| **Speakers** | on each side, a **5.25 in polypropylene woofer** and a **0.5 in Mylar dome tweeter**, with a first-order passive crossover at **8 kHz** |

**Passive radiators, not ports.** The original speaker cabinets were ported. In this machine the
speakers work with **passive radiators**, added by the author, and are not ported. A passive radiator
is a cone with no magnet or coil of its own. The air in the enclosure moves it, where a port would let
that air out through a tube. Its printed parts (a holder, two protectors, a membrane weight and a TPU
membrane) are listed in Appendix D.

**The earth plate.** Of the original cabinets, only the active speaker's aluminium back plate remains.
It is at the back of the cabinet. The machine's mains inlet came on this plate. The plate carries the
earth star point, and the amplifier's ground is joined to it. Chapter 5, section 5.3, explains that joint
and what it means for the DC ground.

## 12.2 The volume control

The amplifier's original dual-gang volume potentiometer has been replaced with a **B10k dual-gang
potentiometer**: two potentiometers on one shaft, one per channel. Its two gangs are separate, as the
original's were. It has a power switch, which is not
used.

- It sits **on the amplifier's circuit board** and is reached **only from the back**.
- It **does not reach silence** at its minimum.
- It is **not** the front volume knob. The front knob is read digitally by the audio board and acts on
  RADIO and BT only (chapter 9, section 9.4). Chapter 6, section 6.8, sets out all three volume
  controls in the chain.

## 12.3 The input panel

The input panel is a small circuit board carrying the amplifier's inputs. The cable sheet's `AMP_`
signals (chapter 10) land on its pins. Where the panel sits in the cabinet is not recorded.

**It adds up its inputs; it does not choose between them.** If two or three inputs carry sound at the
same time, the amplifier plays their sum.

A **balanced** input takes each channel on two wires, a plus and a minus, and amplifies the difference
between them. The table lists the panel's three inputs, their contacts, and whether each is grounded with nothing
plugged in.

| Input | Contacts | With nothing plugged in |
|---|---|---|
| **Balanced** | L+, L−, ground, R−, R+ | not grounded |
| **AUX** | a TRS jack (tip, ring, sleeve) | grounded |
| **RCA pair** | two RCA jacks | grounded |

The DAC reaches the panel through the balanced input. Its cable puts a 470 Ω resistor in each of its four
signal legs and carries six ferrite cores (chapter 6, section 6.6; chapter 10, section 10.21).

The AUX jack reaches the amplifier's input panel, which sums it with the other inputs; the run is not
drawn. Where the AUX jack sits in the cabinet is not recorded; it is not among the front-panel controls
(chapter 3, section 3.2). AUX never passes through the audio board. Nothing is recorded on the RCA pair
(chapter 6, section 6.7).

## 12.4 What was measured at the panel and on the cable

These readings show how the amplifier's balanced input behaves at DC, which matters if you chase a
noise or a fault on the DAC-to-amplifier link. All the readings below are resistance, taken with a
multimeter, one reading each.

**At the amplifier's balanced input**, with the DAC-to-amplifier cable unplugged at the amplifier:

| Between | Reading |
|---|---|
| L− and `AMP_G` (ground) | open: 3 MΩ and rising, as a capacitor charged |
| R− and `AMP_G` | open: 3 MΩ and rising, as a capacitor charged |

**On the DAC-to-amplifier cable**, unplugged. These readings were taken before the 470 Ω resistors on
the two plus legs (L+ and R+) were fitted:

| Between | Reading |
|---|---|
| L− and ground | 470 Ω |
| R− and ground | 470 Ω |
| L− and R− | a little under 1 kΩ |
| L+ and ground | 4.2 MΩ |
| R+ and ground | 4.2 MΩ |

**What this supports, and no more.** At the amplifier's connector, neither minus leg has a DC path to the
amplifier's ground. That is what a differential, AC-coupled input looks like on a meter. Such an input
amplifies the difference between its two legs, through capacitors that pass audio but block DC. It is one
meter reading, taken once; nothing inside the amplifier was examined.

The frying noise once heard at the amplifier, and the parts later added to the DAC-to-amplifier cable,
are in chapter 6, section 6.6.

## 12.5 When something is wrong

The table lists what you may notice and where to look first.

| You see | Check |
|---|---|
| No sound from any source, with the front power switch on | The amplifier supply: +20 V and −20 V on its four wires (chapter 5, section 5.6). Then the amplifier's own volume control, at the back. |
| The amplifier is never fully silent | Normal: its volume control does not reach silence at its minimum. |
| Two sources heard at once | Normal: the input panel adds its inputs. Unplug whatever is playing into AUX (chapter 6, section 6.7). |
| Radio and Bluetooth silent, AUX plays | Not the amplifier: the DAC side (chapter 6, section 6.9). |
| A frying noise at the amplifier | The DAC-to-amplifier cable, its four 470 Ω resistors and its ferrites (chapter 6, section 6.9). |
