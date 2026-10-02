# About this book

Ambersong is a LLOYDS TM-838N tube (valve) radio from the early 1960s, rebuilt into a new cabinet with
two small computers inside it. This book is the **Firmware Gospel**: the account of the software those
two computers run. It says what the software does, how it does it, and why each choice was made. With
it, someone who has never seen the machine can understand it, recover it, build something like it, and
change it safely.

## What this book covers

**The firmware of the two boards, and nothing else.** What is physically in the machine — the boards,
the parts, the wiring, the values — is the subject of the companion book, the **Hardware Bible**. This
book points to it as "Hardware Bible, chapter 8", and states in place any hardware fact it needs.

**One exception.** The firmware holds the physical values of this particular mechanism, so this book
carries them, in chapter 5. They are the needle's travel, its index switch, the motor's speed limits,
the tuner's travel and the printed dial.

**The code is what runs.** A comment in the code states what its author meant. Where a comment and the
code disagree, the code is what the radio does.

**The versions described.** This book describes the main board's firmware at release **v.1.0.5** and
the audio board's at release **v.1.0.4**. The two boards are updated separately, so their numbers need
not match. Chapter 3 shows where to read the version each board is running.

## How the chapters are laid out

**This page and chapters 1 and 2 are the way in:** this page; a rescue chapter for when something is
wrong; and the whole machine at a glance. **Chapters 3 to 10** each cover one part of the firmware. Each opens with
what you see or hear, readable on its own, then explains how it works and lists its settings. Each
then ends with the same tail, in this order, where there is something to say:

| Section | What it holds |
|---|---|
| **When it goes wrong** | A table: what you see, what happened, what the firmware does, what you do. |
| **Design choices** | One line each: the choice, then why. |
| **Tried and rejected** | One line each: approaches tried on this machine and dropped. |
| **Known limits** | What the firmware does not do, and paths never exercised on the real radio. |
| **Changing it safely** | What must stay true, the traps, and how to test a change. |

**The grey bands.** Some passages sit in a grey band headed **For firmware changes**. They hold what
only someone changing the firmware needs: code maps, function and file names, internal workings, the
*Tried and rejected* lists and every *Changing it safely* section. To repair or use the radio, you can
skip every band.

**Chapter 11** is the history and what the bench tests proved. **Chapter 12** opens with an index of
known issues by symptom. It then gathers the issues that were left as they are, grouped by area. For
each, it gives what you would see, why it was left, and a starting point for a fix. The appendices are
reference tables.

| Ch. | Title |
|---|---|
| — | About this book |
| 1 | Rescue: when the radio misbehaves |
| 2 | The machine at a glance |
| 3 | Building, flashing and updating |
| 4 | The main board: start-up, clock, lamps and time |
| 5 | The dial needle |
| 6 | Tuning and the dial's self-calibration |
| 7 | Settings |
| 8 | Network, web portal and console |
| 9 | The audio board and the link |
| 10 | Sound and Bluetooth |
| 11 | History and bench tests |
| 12 | Known issues, ideas and untested paths |
| A | Settings reference |
| B | Network and portal constants, HTTP endpoints |
| C | Portal actions |
| D | Console keys, both boards |
| E | Portal map: tabs, cards and buttons |
| F | Link reference: messages and payloads |
| G | Audio constants and experiments |
| H | The pop hunt in detail |
| I | Pin use by the firmware |

## The recovery ladder

This book sorts every fault by what it costs to recover from it:

| Class | What it costs |
|---|---|
| **NOTE** | Nothing: it heals by itself, or it is only cosmetic. |
| **DEFECT** | A portal reboot, a console reset, or the front power switch. |
| **BLOCKER** | The radio is unusable until a full power cycle (pulling the plug), or the harm survives one: the needle driven into a mechanical stop; a calibration or the settings corrupted and saved; a wrong calibration sample stored; a board that will not start, or that only the USB cable can reach. |

Chapter 1 gives the steps of the ladder in the order to try them, cheapest first.

## Conventions

- **The two boards.** The **S3** is the main board, an ESP32-S3: the clock display, the needle, the
  panel lamps, WiFi and the web portal. The **A32** is the audio board, a classic ESP32: the sound,
  Bluetooth and the battery-backed clock. Chapter 2 explains why there are two.
- **Exact words.** Text in `code` is exactly what you type or read: a console key, a setting's name, a
  message the radio prints. Portal buttons, tabs and pills are in **bold**, spelt as on the page.
  Console keys are case-sensitive. The key tables write a pair in two ways. Two keys side by side,
  `s` `S`, do the same thing. Two keys with a slash, `n` / `N`, are two different commands, described
  in the same order: "Jog −20 / +20" means `n` jogs −20 and `N` jogs +20. Keys in separate rows, such as
  `y` and `Y`, are different commands too.
- **Version stamps.** Every build is stamped `v.1.YYYYMMDDTHHMMSS (hash)`: the release number, the time
  it was built, and the git commit it was built from. `-dirty` is added if the code had uncommitted
  changes. Chapter 3 explains the stamp.
- **"The author"** is the person who built this machine.

## Your mileage may vary

Every *Tried and rejected* list records what was tried **here**, what happened **here**, and why it was
dropped **here**. It never says "this does not work". A retired approach was a dead end in this
particular machine, not necessarily in yours. Likewise, every design choice in this book is a choice
made for this radio, not a law. If you are building your own, question both.
