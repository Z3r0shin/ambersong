# 2. Safety before you open it

Ambersong runs on mains. It holds a tube radio built in the old "AC/DC" way: no mains transformer, and
its metal chassis wired straight to the mains. This short chapter lists every point where mains or high
voltage is present, and when. Chapter 5 explains the same wiring in detail.

> **Danger — unplug the cord.** The front power switch does **not** remove mains from the cabinet. The
> only way to remove mains is to pull the cord out of the mains inlet. Do it before you open the back
> panel or the front panel.

## 2.1 Where the mains is, and when

The table lists each point that carries mains or high voltage, and what must be on for it to be live.

| Point | Live when | Notes |
|---|---|---|
| **Mains inlet** (IEC C14) and its wiring | the cord is plugged in | Live, neutral and earth. |
| **5 V power supply**, mains side | the cord is plugged in | It is wired ahead of the front power switch, so it never switches off. |
| **Front power switch**, input side | the cord is plugged in | The switch breaks the live wire only. |
| **Amplifier ±20 V supply**, mains side | the front switch is on | Its neutral wire stays connected with the switch off. |
| **Mains line filter**, in its own cage | the front switch is on | Its neutral wire stays connected with the switch off. |
| **Tube radio**: its mains terminals, its own power switch | the front switch is on | |
| **Tube radio**: its 0.75 A fuse, its heater string, and everything after its own power switch | the front switch **and** the radio's own power switch are on | The seven tube heaters are wired in series from the fused live to the chassis, which is on neutral. |
| **Tube radio's high-voltage supply (B+)** | the radio is on | Made from the mains by a half-wave rectifier inside the radio (chapter 13). |
| **Tube radio's chassis** (`CHASSIS`) | always wired to mains neutral | See section 2.3. |

**B+** is the radio's high-voltage supply for its tubes. The radio makes it by rectifying the mains
directly. The chain is a 28 Ω surge resistor and a silicon rectifier, then three 47 µF 315 V capacitors
with two dropping resistors between them. No bleeder resistor is drawn across these capacitors. The numbers in
the names of its points (`B130`, `B115_A`) are the original service data's nominal figures, not measured
voltages.

## 2.2 The front power switch does not make it safe

The front power switch is a single-pole toggle. It breaks **only the live wire**, and only to two
things: the amplifier's ±20 V supply and the tube radio (through the line filter). Three things follow:

- The **5 V power supply** stays on. Both microcontroller boards, the clock and everything digital keep
  running.
- The **neutral wire** is never switched. It stays connected to the amplifier supply, the line filter and
  the tube radio's chassis.
- **Only unplugging the cord removes mains** from the cabinet.

## 2.3 The radio chassis is on mains neutral

**The tube radio has no isolation transformer.** Its circuit common, `CHASSIS` — the metal chassis and
every part connected to it — is wired to mains neutral through the line filter. It sits near 0 V only
while the wall socket's live and neutral are the right way round, and **nothing in the machine checks
that**. If they are reversed, the chassis is on the live wire, and it stays there with the front switch
off, because the neutral wire is never switched.

`CHASSIS` is never joined to Earth, to the Faraday cages, to the DC ground, or to the amplifier supply
return. The machine uses four ground names. Earth, the DC ground and the amplifier supply return are all
joined; the radio chassis is joined to none of them:

| Name | What it is |
|---|---|
| **Earth** (`Earth`) | the mains protective earth; the Faraday cages and the antenna connector's shell are on it |
| **DC ground** (`GND`) | the return of both boards and everything on 5 V and 3.3 V; reaches the amplifier's ground and so **on Earth** at almost 0 Ω. By which path it reaches the amplifier's ground is not recorded |
| **Amplifier supply return** (`20V_GND`) | the 0 V output of the amplifier's ±20 V supply; its two middle black wires go to the amplifier's ground, which sits on the earth star point of the amplifier's back plate, so it too is **on Earth** |
| **Radio chassis** (`CHASSIS`) | the tube radio's circuit common, **on mains neutral** |

Chapter 5, section 5.3, says what each one is joined to.

**The DC ground is on Earth**, at almost 0 Ω, through the amplifier's ground (a washer on its Bass pot,
chapter 5, section 5.3). By which path the DC ground reaches the amplifier's ground is not recorded.
Anything you connect to the DC ground is connected to Earth.

## 2.4 Safety-rated parts

A few capacitors in the tube radio, on the mains side or on the antenna input, are safety-rated types.
The table lists them, with the safety class of each.

| Part | Where | Fitted |
|---|---|---|
| The antenna coupling capacitor (C3) | between the antenna's centre conductor and the radio's FM input | 100 pF, class X1Y1 |
| The antenna shield capacitor (C51) | between the antenna coax shield, which is on Earth, and the chassis: the shield's only path to the chassis | 0.001 µF, class X1Y2 |
| The mains bypass capacitor (C44) | between the fused live and the chassis | 0.01 µF, class X1Y2 |
| Three of the four heater-string bypass capacitors (C47, C49, C52) | from points on the heater string to the chassis | 0.0022 µF each, class X1Y2. The fourth (C48) is the original 0.002 µF ceramic, not a safety type. |

**Fuses.** No fuse is recorded on the machine's mains side; the tube radio has its own 0.75 A fuse. That
fuse sits in the radio's switched live, between its own power switch and the rest of the set. Its type and
voltage rating are not recorded.

## 2.5 The low-voltage side

The rest of the machine runs at low voltage. This is what each part of it is referenced to.

- The **amplifier supply** gives +20 V and −20 V (±19.33 V measured with no load) around its own return,
  `20V_GND`. It is isolated from the mains and held off the amplifier's back plate by plastic screws and
  standoffs.
- The **5 V** and **3.3 V** buses, and both boards, are on the DC ground, which is on Earth (section 2.3).

> **Caution — two power sources on one board.** Each board has a USB-C port on the back panel, carried
> straight through to the board, power wire included. The boards are also powered from the 5 V bus on
> their 5V pin whenever the cord is plugged in. Espressif's guide for the ESP32-S3-DevKitC-1, the board
> the main board is compatible with, calls the USB port, the 5V pin and the 3V3 pin "three mutually
> exclusive ways to provide power to the board". Espressif's guide for its original ESP32 DevKitC V4 board
> says the same, and adds that power must come from one and only one of them, "otherwise the board and/or
> the power supply source can be damaged". Whether the fitted boards match that V4 board is not
> established. These are the guides' rules; the author gives no rule of his own. Plugging a USB cable into a
> back-panel port while the cord is in gives the board two of these inputs at once.
> Chapter 4, section 4.6, describes the ports.
