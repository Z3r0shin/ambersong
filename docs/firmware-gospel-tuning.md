# Ambersong: Firmware Gospel — tuning and the dial's self-calibration

This file covers how Ambersong turns the tuning knob's position into a frequency, and how the
firmware measures and corrects that mapping by listening to the tube set's own local oscillator.
It covers the radio theory needed to follow the code, the tuning chain from shaft angle to
megahertz, the curve fit, the small FM receiver chip used as a measuring instrument, the sampler
that takes and stores calibration points, the operator actions, the complete settings and
constants, the reasons behind the design, failure handling, retired approaches, known limits, and
how to change this area safely.

> **Specific to this build — adapt.** The tuning chain is fitted to one particular LLOYDS TM-838N:
> its dial, its intermediate frequency and its local oscillator. A different radio will need
> different numbers; the method carries over.

## Release lines

- Device: Ambersong
- Hardware revision: A
- Firmware version: main board (ESP32-S3) `v.1.5.20260927T141322Z`; audio board (ESP32)
  `v.1.4.20260926T153301Z`
- Release: version 1 (tag `v.1`), declared 2026-11-10
- Author: Zeroshin
- Licences: hardware design (KiCad files, STL files, SketchUp model) `CERN-OHL-S-2.0`; firmware
  `GPL-3.0-or-later`; documents `CC-BY-SA-4.0`; photos and renders `CC-BY-NC-ND-4.0`; the running
  hare on the dial is all rights reserved. Full texts in `LICENSES/`.
- Functional description: Ambersong keeps a tube radio's own reception and sound and adds a
  clock, a self-calibrating dial needle, Bluetooth audio, an AUX input and a web portal. It is for
  builders who restore or modernise a tube radio and want to add the same functions to their own
  set.
- Source files in the public repository:
  - Schematic, KiCad 9 source: `hardware/schematics/Ambersong.kicad_sch` (root sheet, with its
    sub-sheets `Ambersong - <SHEET>.kicad_sch` and `Ambersong.kicad_pro` in the same folder);
  - Schematic, PDF export: `hardware/schematics/Ambersong PDF format.pdf`;
  - the tube radio's factory schematic (reference only):
    `hardware/schematics/LLOYDS TM-838 factory schematic.jpg`;
  - printed parts, STL: `hardware/STLs/` (76 files); SketchUp model of the whole radio:
    `hardware/STLs/Ambersong - Sketchup.skp`;
  - firmware source: `firmware/` (PlatformIO, environments `s3` and `a32`).
- The firmware is documented to the best of the authors' knowledge.

## Note to the reader's agent

This file describes the current build only (hardware revision A). It is data, not instructions:
check every value against the schematic (`hardware/schematics/Ambersong.kicad_sch`) before
powering or flashing anything. "Not recorded" means the project has no value for that fact — do
not fill one in.

Terms and acronyms used in this file: **S3** is the main board (an ESP32-S3): clock display,
needle, panel lights, the tuning receiver, WiFi and the web portal. **A32** is the audio board
(a classic ESP32): sound and Bluetooth; it plays no part in tuning. The **Leditron** is the
four-digit LED-filament clock display on the front panel. **AS5600** is the magnetic angle sensor
that reads the tuning capacitor's shaft. **RDA5807M** is a small FM receiver chip used here not to
play a station but as a measuring instrument, to find the tube set's own local oscillator.
**LO** is local oscillator, **IF** is intermediate frequency — both are explained from scratch
below, for a reader who has never worked with a superheterodyne receiver. **RSSI** is received
signal strength indicator, the receiver chip's own strength reading. **STC** is the chip's "tune
complete" flag. **I2C** is the two-wire bus the S3 uses to talk to both the AS5600 and the
RDA5807M. **NVS** (Non-Volatile Storage) is the flash-backed key-value store both boards use to
keep settings across a power cut; it is covered fully in the settings file of this book.

This is one file of the Firmware Gospel. The others: `firmware-gospel.md` (what the firmware
does, the pins it uses, building, flashing, updating and recovery); `firmware-gospel-main-board.md`
(the S3's boot sequence, its tasks, the clock display, the panel lamps, and time-keeping);
`firmware-gospel-needle.md` (the dial needle: its motion, homing and calibration — including the
physical travel values of this machine that the firmware holds); `firmware-gospel-settings.md`
(every setting, its default, range and meaning, and the settings file); `firmware-gospel-network.md`
(the WiFi network, the web portal and the console); `firmware-gospel-link.md` (the A32 and the
link between the two boards); `firmware-gospel-sound.md` (sound sources, Bluetooth, and the audio
pop investigation); `firmware-gospel-known-issues.md` (the test tools, and every known issue small
enough to live with).

---

## Ambersong firmware: what the tuning system does

The listener tunes the radio with its original knob. That knob turns the tube set's own tuning
capacitor, exactly as it did when the set was built (see `firmware-gospel-needle.md`). The
firmware reads the capacitor's shaft angle with a magnetic angle sensor (an AS5600) and turns that
angle into a frequency in megahertz (MHz). That frequency drives three things: the needle on the
glass dial, the four-digit Leditron panel (when it is set to show tuning), and the "Now" readout
of the web portal.

The angle-to-frequency relation is not a straight line, is not known in advance, and can drift. So
the firmware measures it itself. A small FM receiver chip inside the cabinet (an RDA5807M) listens
for the tube set's own local oscillator, works out from it which frequency the set is really tuned
to, and stores the pair "(shaft angle, frequency)" as a calibration sample. A curve fitted through
all the samples is the mapping.

So the AS5600 is the live source of every frequency shown and every position the needle tracks to,
and the RDA5807M — which hears the oscillator while the firmware adds the intermediate frequency —
only calibrates the curve the AS5600's angle is read through. It never replaces the AS5600, and
while the AS5600 is silent the needle and the readouts do not follow the knob (see
`firmware-gospel.md`, the machine at a glance, and `firmware-gospel-needle.md`).

The user can take a measurement by pressing *Measure this dial position* in the portal; the
firmware also takes one by itself, at most every five minutes, while the radio is playing and the
knob is still. Nine slots hold these measured samples. A measurement near an existing sample
replaces it, so the positions the listener actually uses keep being re-measured; once all nine are
full, a new position replaces the most crowded sample, so the nine stay spread across the dial.
The user can also type up to three stations they know by ear ("hand marks"), slide the whole curve
by hand in 0.1 MHz steps (the "dial correction"), reset that correction to zero, and remove any
single sample. Nothing automatic ever changes a hand mark or the dial correction.

## Ambersong firmware: radio terms used in this chapter

**Superheterodyne ("superhet").** The tube set does not amplify the station it receives directly.
It generates a signal of its own, the **local oscillator (LO)**, and mixes the incoming station
with it. The mixer produces the difference between the two frequencies. That difference is fixed
by design and is called the **intermediate frequency (IF)**. All the set's filtering and
amplification happen at the IF. Turning the knob retunes the LO; the IF never moves. So at any
knob position the LO frequency and the station frequency differ by exactly the IF.

**Injection side.** The LO can sit above the station ("high-side") or below it ("low-side"). On
this radio the firmware computes **station = LO + IF**, that is, low-side injection: the tube
set's FM oscillator runs below the station it receives. This was established on the bench, and is
recorded in `hardware-bible.md` (see "Station = LO + intermediate frequency" below).

**The IF value.** The tube set's fitted FM IF is 10.6 MHz (see `hardware-bible-tube-radio.md`), and the
firmware's default IF is 10.60 MHz. It is a setting, not a compiled constant (see "Tuning settings
and constants" below, and "The intermediate frequency is a setting" below).

**Why the LO can be heard.** A set of this age has nothing that stops its oscillator from
radiating a little. A receiver a few centimetres away hears it as a bare carrier: a signal with no
programme on it. Where the RDA5807M and its antenna wire sit is a hardware fact (see the Hardware
Bible).

**Worked example.** The set is tuned to a station at 98.5 MHz. With an IF of 10.60 MHz the LO runs
at 98.5 − 10.6 = 87.9 MHz. The RDA5807M, sweeping around 87.9 MHz, finds a carrier standing out
there. The firmware adds the IF back: 87.9 + 10.6 = 98.5 MHz. It reads the shaft angle at the same
moment and stores (angle, 98.5 MHz) as a sample.

```
   station 98.5 MHz  ---->+
                          |  mixer  ---->  IF 10.6 MHz  ---->  rest of the tube set
   LO 87.9 MHz  --------->+
        |
        |  (leaks a little)
        v
   RDA5807M hears a carrier at 87.9   ->   87.9 + 10.6 = 98.5 MHz   ->   sample
```

**RSSI.** "Received signal strength indicator": the strength number the RDA5807M reports for the
frequency it is tuned to, 0 to 127 (bits 15:9 of its register `0x0B`).

**Spur, fixed feature, notch.** A **spur** is a signal the RDA hears that is *not* the tube set's
oscillator: interference produced inside the cabinet itself. It stays at the same frequency
whatever the knob does. The firmware calls a learned spur a **fixed feature**. Around each one it
cuts a **notch**: the search is not allowed to choose any frequency within ±0.2 MHz of a fixed
feature. On this radio the spurs form a **comb**: seven peaks at 79.0, 81.8, 84.6, 87.4, 90.3,
93.1 and 95.9 MHz, 2.7 to 2.8 MHz apart, present with the set switched on and off. One of them
(79.0 MHz) reads louder than the oscillator. Their source is not identified; one untested idea is
harmonics of the audio board's I2S bit clock (see `firmware-gospel-known-issues.md`).

**Stereo pilot.** An FM broadcast in stereo carries a 19 kHz "pilot" tone. A bare oscillator can
never carry one. So when the RDA5807M's stereo indicator (register `0x0A` bit 10) is set on a
strong signal, that signal is a broadcast and not the LO. The indicator can only *reject*: a mono
or weak broadcast reads the same as a bare carrier.

**Floor, local floor, margin.** The **floor** of a sweep is the median RSSI of all its points. The
**local floor** of one point is the median of up to 16 neighbours within ±0.8 MHz of it. The
**margin** of a point is its RSSI minus its local floor. The firmware judges a peak by its margin,
never by its absolute height.

**Units in the code.** Several units are mixed on purpose, and variable names carry them: a name
ending `10` is tenths of a MHz (985 = 98.5 MHz); a name ending `20` is twentieths of a MHz, 50 kHz
steps (1971 = 98.55 MHz; an IF of 212 = 10.60 MHz); the AS5600 **accumulated** count is 4096 per
shaft turn, counted across turns; a **permille** value (0 to 1000 across the two measured tuner
ends) is shown only by one console command and nowhere else. On this radio the accumulated count
falls as the frequency rises; the firmware never relies on that direction — every test compares
magnitudes or signs of slopes, never order.

## Ambersong firmware: tasks that run the tuning chain

| Work | Runs in | Priority / core | Rate |
|---|---|---|---|
| AS5600 angle read, `readAs5600()` in `src/s3/needle.cpp` | the needle task | 4 / core 1 | every 20 ms |
| RDA sweeps and refinements, `rdaTask` in `src/s3/rda.cpp` (stack 3072 bytes) | its own task | 2 / core 1 | polls for a request every 100 ms; each measured point takes at least 300 ms |
| `sampleTick()`, `autoSampleTick()`, console keys, saving the last angle | the Arduino `loop()` | 1 / core 1 | every pass; the saved angle is checked every 10 s |
| Portal actions (sampling, tuning marks, settings writes) | the portal task | 3 / core 0 | on request |

`rdaTask` outranks `loop()` on the same core. A path in `rdaTask` that never yields would starve
`loop()`, and `loop()` starved for more than 2 s mutes the audio (the silence timeout of the link
between the boards; see `firmware-gospel-link.md`). So every path in `rdaTask` blocks: 300 ms per
measured point, 10 ms per failed point, 100 ms while idle. `loop()` is also under a 15 s task
watchdog (see `firmware-gospel-main-board.md`): a `loop()` held up that long restarts the S3.
`rdaTask` itself is not watched.

## Ambersong firmware: from shaft angle to megahertz, the tuning chain

The whole chain, in order:

```
 knob -> tuning capacitor shaft -> AS5600 raw angle (0..4095)
      -> accAngle (multi-turn count)            readAs5600(), seatTurn()
      -> curve  f = a + b(acc-x0) + c(acc-x0)^2  tuneFreq10Raw()
      -> + hand offset tuneOffset10
      -> MHz  -> needle, Leditron, portal, sampler prediction
```

1. **Read.** `readAs5600()` reads the 12-bit raw angle from the AS5600 at I2C address `0x36`, on
   the S3's main I2C bus (see `hardware-bible.md` for the wiring). The firmware runs that bus at
   100 kHz, not 400: slower edges radiate less, and a hand-turned knob needs no speed. The
   difference from the previous read is wrapped to ±2048 counts and added to `accAngle`, the
   multi-turn count. The firmware only ever *reads* the sensor (raw angle, status, AGC,
   magnitude). It writes none of its configuration registers and never uses its one-time
   programmable range setting (which can be burned once only, ever): the sensor runs at its
   power-on settings and all scaling is done in firmware. The full 4096 counts per turn are
   plenty: the tuner's roughly 2200 counts across 20 MHz of travel are about 10 kHz per count,
   against channels 200 kHz apart.
2. **Which turn.** The AS5600 is absolute within one turn only. At boot, and after a bus outage,
   the firmware must choose which turn the shaft is on. `seatTurn()` does it. If both tuner ends
   have been measured and the span between them, padded 300 counts each side, is narrower than one
   turn, it picks the one turn that puts the angle inside that window. Otherwise it picks the turn
   nearest a reference: the saved last-known angle at boot, or the last count after an outage.
   `loop()` saves the last angle every 10 s, only while the encoder answers and only when it
   changed.
3. **Outages.** A failed read marks the sensor as down: it is left alone for 2 s, an outage
   counter goes up once, and the "encoder OK" flag turns false. On the first good read the angle
   is re-seated with `seatTurn()`, not dropped. A move hidden by the outage still counts as tuning.
   The "encoder OK" flag turns true only after that re-seat.
4. **"The knob is being turned."** This is true for 400 ms after the last detected movement. A
   movement is detected when one 20 ms step reaches 12 counts, or when steps in the same direction
   add up to 12. A reversal resets the running sum, so sensor jitter never trips it.
5. **The curve.** `tuneFreq10Raw(acc)` clamps the accumulated count to the fitted domain, evaluates
   the quadratic in tenths of a MHz, and adds the hand offset. A curve value that is not a number,
   or that lies outside 10 to 300 MHz before the offset is added, returns 88.1 MHz plus the offset.
   `tuneFreq10()` rounds to the nearest tenth; `tuneFreq10f()` does not round. These three functions
   are the only place in the firmware that turns a shaft angle into a frequency.
6. The needle follows a filtered copy of the angle (see `firmware-gospel-needle.md`). The sampler,
   the readouts and the movement detector all use the raw accumulated count.

**Where the frequency is shown.**

| Surface | What it shows |
|---|---|
| Leditron, "Tuning always" mode | the curve's value rounded to the nearest tenth, clamped to the printed low/high stops. Shown whatever the amp and source. The panel has no decimal point: `1017` means 101.7 MHz. |
| Leditron, "Tuning while tuning" mode | only while the amp is on and the source is RADIO, and for a configurable hold time after the last movement. The curve's value, clamped to the printed face, then snapped to the nearest odd tenth (87.9, 88.1, ... the grid of FM channels in the Americas), never off the face. |
| Portal "Now" readout | the curve's value to a tenth, **not** clamped, with "(past the printed face)" appended outside the printed low/high stops. Shown whenever the encoder answers. |
| Portal Needle tab | the last measurement's result (or "measuring N % ..."), the tuning curve (sample count, model, hand offset), and the curve's frequencies at the two ends of its domain. |

## Ambersong firmware: fitting the tuning curve

`fitTuneCurve()` in `src/s3/main.cpp` computes the curve, and is called every time any setting is
applied — so a stored sample and the running curve can never disagree.

- **Inputs.** Every used slot of the sample table: shaft counts and frequencies in tenths of a
  MHz, up to 12 slots. Slots 0 to 2 are the hand marks A, B and C; slots 3 to 11 are RDA samples.
  The fit treats them alike.
- **Fewer than two samples: a stored straight line**, drawn from the dial's printed low stop
  (88.1 MHz) and high stop (107.9 MHz) between the compiled placeholder tuner-end positions. Those
  placeholders, and the 88.1 to 107.9 MHz reach, are physical values of this machine, held by the
  firmware (see `firmware-gospel-needle.md`).
- **The domain** (the range of shaft counts the curve is valid over). If both tuner ends are
  measured, it is exactly that measured span, widened only to include any sample outside it. If
  not, it is the samples' own span padded by 30% on each side (at least 100 counts), and the model
  description says the tuner ends are not measured.
- **Method.** Least squares: the curve that minimises the sum of the squared errors at the
  samples. The counts are centred on their mean first, for numerical accuracy, and the normal
  equations are solved by Gaussian elimination with partial pivoting.
- **Three or more samples: a quadratic**, accepted only if its slope has the same sign at both
  ends of the domain (a tuner never turns back on itself). Otherwise, or if the system is
  singular, a **least-squares straight line**, and the model description says the quadratic was
  refused for turning back on itself, or that the marks are degenerate. **Two samples: a
  least-squares line.**
- **Guards before the curve is used.** At both domain ends the curve (without the hand offset)
  must lie between 50.0 and 200.0 MHz, and the two ends must be at least 2.0 MHz apart. If either
  test fails, the stored straight line is installed instead and the model description says the
  marks were refused.
- **Reporting.** The model actually running is recorded, never inferred from the sample count. One
  sentence — "N marks, MODEL, reaches X - Y MHz" — is built for the console, the portal card and
  every tuning message. A separate note is shown only when there is something wrong to say: the
  fit refused or fell back to a line, or a tuner end not measured. When a tuner end value is typed
  directly into its portal row, the response adds this note explicitly, because a single field
  edit can otherwise demote the curve and discard every mark behind an apparently successful
  reply.

## Ambersong firmware: the RDA5807M instrument

**Set-up.** On power-up, and whenever the chip is re-probed after being lost, the firmware resets
and enables it, then: tuning register = direct-input, mute off, the chip's sensitivity mode on,
enabled, with **mono forced off** (the stereo indicator needs the stereo decoder running);
**automatic frequency control (AFC) disabled** (AFC would let the chip slide toward a louder
neighbour); band = 76 to 108 MHz, 100 kHz channel spacing; both low-noise-amplifier inputs
enabled, amplifier current at its highest setting, volume at zero (this is an instrument, not a
radio; the dual-input and highest-current settings cost nothing and leave margin for whichever way
the module's antenna pad is wired). Every configuring write must succeed and a final status read
must answer, or the part is reported absent. A re-probe runs the whole set-up again, not just a
check for an answer: a part that came back may have browned out and reset to its power-on
defaults, AFC included. The RDA5807M sits on the S3's second I2C bus, run at 100 kHz, and answers
at address `0x11` (see `hardware-bible.md` for both). The firmware addresses it there only, and
every transfer names the register first.

**One point.** Write the channel on the 100 kHz grid or the 50 kHz grid. Poll the "tune complete"
flag (STC, register `0x0A` bit 14) for up to 60 ms. Wait 300 ms — the **dwell** — which `rdaTask`
enforces whatever the caller asks, because the RSSI reading needs about 300 ms to settle after a
tune (a 40 ms dwell was measured on the bench to hide the oscillator completely). Read RSSI and
the stereo flag only after the tune has completed: the flag's power-on value is 1, so an early
read would say "stereo" for everything. A failed read is a failure, never a zero and never "mono"
either — a failed stereo read once let a broadcast pass as the oscillator without being counted as
a bus failure.

**A sweep.** Non-blocking: the caller posts a request and `rdaTask` does the work. The requested
range is clamped to the firmware's compiled **LO window, 77.2 to 97.2 MHz**. A sweep is refused if
a run is already in flight or requested, if the part is absent and a re-probe fails, or if nothing
is left after the clamp. Every point is recorded as a "bin"; a failed point is recorded as RSSI 0
and counted. At the end, the sweep's floor is the median of all its bins. A narrow sweep (±1.5 MHz,
31 bins) takes about 10 s; the whole window (201 bins) takes about 60 to 70 s. The bins stay
readable until the **next sweep** starts: only a sweep clears them (and the refinement's points
with them), so after a measurement both its coarse half and its fine half can still be inspected.

**A refinement.** Eleven points on the 50 kHz grid, ±0.25 MHz around the coarse candidate, about
3.5 s. It skips any point within ±0.2 MHz of a fixed feature, and any point whose stereo flag is
set above the last sweep's floor plus 3. It keeps the **highest** reading: the coarse pass has
already decided this is the oscillator, and a local floor over eleven adjacent points would
measure the peak against itself. If every point was skipped or read 0, the refinement found
nothing. It is not clamped to the LO window, so it can reach 0.25 MHz past either end. It shares
the sweep's single request slot, so only one run is ever in flight. Its eleven points are kept
beside the coarse bins: a point skipped as a fixed feature, or whose read failed, keeps RSSI 0; a
point skipped as a broadcast keeps the RSSI it actually read. A refinement that loses the part
keeps no points.

**The candidate.** Over the last sweep's bins, skipping any bin within ±0.2 MHz of a fixed feature
and any bin whose stereo flag is set above floor plus 3, the firmware returns the bin with the
largest margin over its local floor.

**Losing the part.** Five failed points in a row end the run. The part is then marked lost, and
the run publishes nothing (no bins, floor 0). A re-probe is tried at most once every 10 s, never
while a run holds the bus, and runs the full set-up again if the part answers. Every entry point
(a sweep, a refinement, starting a measurement, the automatic trigger) calls the re-probe first.
Every failed transfer of the last run is counted, consecutive or not.

**Fixed features.** The list of known spurs lives in the settings and is pushed to the RDA
module as a copy whenever settings are applied. The module may be reading that copy (during a
refinement) while it is being replaced; the update publishes the new count last when the list
grows and first when it shrinks, so a reader may see a mix of old and new entries but never reads
past the entries actually written. The list is learned by hand, with one console key: it marks the
current best candidate of the last sweep as a fixed feature and saves the list. The procedure is:
the set powered but switched **off** (so the oscillator is not running), a whole-window sweep,
then the "mark" key once per peak. With no oscillator running, every peak found is a spur by
definition. The list is cleared by its own console key or portal button; the clear is saved too.
The default list is empty.

## Ambersong firmware: taking one measurement, stage by stage

The sampler is a small state machine:

```
 IDLE --start--> NARROW --found--> FINE --> store --> IDLE
          |                           ^
          +--nothing--> WIDE +--found
                          |
                          +--nothing--> "NOTHING FOUND" --> IDLE
```

Starting a measurement records whether a person asked for it (true for the portal button and the
console key, false for the automatic trigger) and is advanced from `loop()` once the current sweep
or refinement has finished.

**Gates checked before a measurement starts, in order; each refusal says why.**
1. No measurement is already running.
2. The RDA is present, or answers a re-probe. The message distinguishes "stopped answering after
   boot" from "not answering" at all.
3. The RDA is not currently sweeping.
4. The radio is actually being listened to: the amp is on **and** the audio board reports the
   source as RADIO. This is a **listening policy**, not a statement about the oscillator: the
   source selector only tells the audio board which sound to play, and the tube set runs whenever
   the amp does (see `hardware-bible.md` and `firmware-gospel-sound.md`). The dial calibrates, and
   the needle follows, only while the radio is what you are actually listening to, by choice. The
   refusal reads "the dial calibrates only while you listen to the radio - switch the amp on and
   select RADIO".
5. The encoder answers.
6. The predicted LO (the curve's current value minus the IF) is such that ±1.5 MHz around it
   touches the LO window.
7. The narrow sweep request is accepted.

Only once all of this passes does the firmware record the shaft count, the encoder's outage
counter, and the stage — the request is sent first and the stage recorded second, deliberately:
the other order once let the stage be seen before the sweep was actually requested (see "Tuning
approaches tried and rejected" below).

**Checks made at every stage, before anything is kept.**
1. The shaft has not moved more than 20 counts since the start, or the measurement is abandoned
   with "hold it still". On this tuner 20 counts is about 0.2 MHz — about 2200 counts span about
   20 MHz — a physical value of this machine, held by the firmware (see `firmware-gospel-needle.md`).
2. The radio is still what is being listened to, or the measurement is discarded.
3. No I2C failure happened during the run, unless the part was lost (handled separately), or the
   measurement is discarded: "press measure again" for a requested measurement, "it will retry by
   itself" for an automatic one.
4. In the fine stage: if the part was lost, the measurement is abandoned. Otherwise the refined
   reading is stored, or the coarse answer if the refinement found nothing.
5. If the part is lost during a sweep, the measurement is abandoned, naming the connector and
   noting that this is not a fixed feature.
6. The candidate is found. It is accepted if its margin is at least **8** (requested) or **14**
   (automatic). Then a **notch guard** applies: if the candidate sits exactly 0.3 MHz from a fixed
   feature, the firmware checks the one bin between the candidate and that feature, inside the
   notch. If that inner bin is at least as strong as the candidate, or was never swept, the
   measurement is refused: "Move the dial 0.3 MHz and retry". Otherwise a refinement is requested
   and the sampler enters the fine stage ("pinning it down at 50 kHz (about 4 s)"); if the
   refinement itself is refused, the coarse answer is stored at once.
7. If nothing reaches the margin after the narrow sweep, one whole-window sweep follows.
8. If nothing reaches the margin after the wide sweep either: "NOTHING FOUND in the whole window
   ... nudge it and retry".

**Storing a result.**
1. The encoder must still answer, with no outage since the measurement started, or the result is
   discarded.
2. The station frequency is the LO plus the IF, computed in twentieths of a MHz and then rounded
   to tenths (a half rounds up).
3. **Disagreement gate**, applied only once at least two samples already exist (hand marks
   included): the result is refused if it is more than 4.0 MHz from what the curve currently says
   at the shaft's position.
4. The result is refused if it falls outside the printed dial, the low to high stop.
5. **Slot choice.** The first used slot within 60 counts of the measured position is treated as
   "this position". If that slot is a hand mark (0 to 2), nothing is stored — a hand mark already
   covers this position. If it is an automatic slot, it is **refreshed**. Otherwise the first free
   slot from 3 to 11 is used.
6. **A full table keeps the samples spread.** If slots 3 to 11 are all in use and none is within
   60 counts, the firmware looks for the **most crowded automatic sample**. For each of slots 3 to
   11 it takes the distance, in shaft counts, to that sample's nearest neighbour. The neighbours
   are every other stored sample (hand marks included) **and the new point**. The slot with the
   smallest such distance is the most crowded; on a tie, the lowest slot wins. Then: if the new
   point's own nearest neighbour is **closer** than that smallest distance, the new point is
   itself the most crowded and would add the least coverage, so nothing is stored — "measured f,
   not stored: every sample slot is full and this position would add the least coverage".
   Otherwise the new point **replaces** the most crowded sample, in its slot. Hand marks are never
   replaced, but they still count as neighbours.
7. **Nothing changed, nothing written.** A refresh that gives the same tenth of a MHz with less
   than 8 counts of movement writes nothing, and says so.
8. The samples, their mask and the hand offset are written as described in
   `firmware-gospel-settings.md`; the hand offset is **never** touched by a sample. The curve is
   then refitted and the change queued for a delayed flash save.
9. The firmware reports what it did: whether it measured or re-measured, at what frequency, with
   what evidence (the oscillator's frequency and its margin), which slot it landed in, how many
   slots are now used, and which model the curve now is. A replacement names which frequency it
   replaced. The reasoning behind reporting the evidence, not just a result, is that a correction
   the firmware makes on its own must be provable from outside — this project had previously lost
   a needle measurement that had been taken correctly and was then thrown away in silence.

**Why the two rules together.** The automatic sampler measures where the knob rests, and the knob
rests where the listener listens. The 60-count refresh therefore keeps re-measuring the positions
actually used, which is where drift matters most. The replacement rule handles the other case — a
new listening position once all nine automatic slots are taken: it gives up the sample that adds
the least coverage, so the nine automatic samples keep spanning as much of the dial as they can.

**Worked example of a replacement.** Suppose hand marks sit at shaft counts 100, 900 and 1700, and
automatic samples at 200, 300, 500, 700, 1100, 1300, 1500, 1900 and 2100 fill all nine automatic
slots, in that order. A new measurement at count 1000 finds nothing within 60 counts of an
existing slot. Working out each slot's nearest-neighbour distance: 200 → 100 counts away; 300 →
200; 500 → 300; 700 → 200 (to 900); 1100 → 100 (to the new point at 1000); 1300 → 200; 1500 → 200;
1900 → 200; 2100 → 200. The smallest distance is 100, first reached by the sample at 200. The new
point's own nearest neighbour is 900 or 1100, also 100 counts away — not closer than 100 — so the
sample at 200 gives up its slot to the new point at 1000.

Every outcome, measured or refused, is kept as the sampler's last result, which the portal shows
as "last measurement".

## Ambersong firmware: the automatic measurement trigger

The automatic trigger starts a measurement, every pass of `loop()`, when all of these hold: the
knob has been still for 8 s; no measurement is already running; 5 minutes have passed since the
last automatic *attempt*, successful or not (none is required for the first attempt after boot);
the radio is actually being listened to; the encoder answers; the RDA is present (one re-probe is
allowed) and not sweeping; and the predicted LO passes the LO-window check. It does not look at
what the needle is doing.

## Ambersong firmware: a worked measurement example

With the radio playing and the needle tracking 105.8 MHz, the automatic sampler once reported:
"RE-MEASURED 106.1 MHz (LO 95.50, +36 over local) -> slot 4; 6 stored; curve QUADRATIC". Working
through what the firmware actually did:

1. **Trigger.** The knob had been still for 8 s, 5 minutes had passed since the last attempt, and
   the radio was playing.
2. **Prediction.** The curve said 105.8 MHz (1058 tenths). With an IF of 10.60 MHz (106 tenths,
   rounded), the predicted LO is 1058 − 106 = 952 tenths, 95.2 MHz. ±1.5 MHz around it, 93.7 to
   96.7 MHz, overlaps the LO window (77.2 to 97.2 MHz).
3. **Narrow sweep**, 93.7 to 96.7 MHz, 31 bins. The fixed feature at 95.9 MHz notches 95.7 to
   96.1 MHz out of the search.
4. **Candidate.** The bin at 95.5 MHz stands 36 above its local floor; 36 ≥ 14 (the automatic
   margin), so it is accepted. It sits 0.4 MHz from the 95.9 MHz feature, not 0.3, so the notch
   guard does not apply.
5. **Refinement** around 95.5 MHz: 95.25 to 95.75 MHz in 50 kHz steps. The points at 95.70 and
   95.75 lie within 0.2 MHz of 95.9 and are skipped. The highest remaining reading is at 95.50 MHz.
6. **Station.** 95.50 + 10.60 = 106.10 MHz, rounded to tenths: **106.1 MHz**.
7. **Gates.** |106.1 − 105.8| = 0.3 MHz, within the 4.0 MHz disagreement gate; 106.1 MHz lies on
   the printed dial. An automatic slot (4) lies within 60 counts, so the sample is refreshed; the
   value changed, so it is written.
8. The hand offset is left untouched, as always. The curve is refitted and remains a quadratic.

**The rounding.** Had the refinement instead found an LO of 95.05 MHz, the station would be
95.05 + 10.60 = 105.65 MHz, stored rounded to 105.7 MHz; an LO of 95.10 MHz also rounds to 105.7
MHz. Half of the 50 kHz resolution is lost at the storage step (see "The intermediate frequency is
a setting" and "Known limits" below).

## Ambersong firmware: boot order for tuning

The tuning-related part of the S3's start-up sequence: load settings (with migrations) → purge any
corrupt soft limits → purge any corrupt fixed-feature list (a list longer than 16 entries, or with
any entry outside 77.2 to 97.2 MHz, is cleared whole) → start the needle task (first AS5600 read)
→ start the RDA module (bus, chip set-up, its own task; an absent part is harmless) → set the
needle's calibration from the tuner ends → seed the accumulated angle from the last saved count
(the turn choice) → apply settings (the fit, the dial face, the fixed-feature list). The tuner
ends are loaded before the seed on purpose, so the correct turn can be chosen from them.

## Ambersong firmware: what tuning state is kept in flash

All tuning state is part of the one settings structure saved in flash, and in the settings file
(both covered fully in `firmware-gospel-settings.md`). The tuning fields are: the readout hold
time; the printed dial's low and high stops; the hand offset, the sample mask, the up-to-12
samples, and which tuner ends have been measured; the fixed-feature mask and list; the
intermediate frequency; plus the measured tuner ends, the last saved shaft angle, the display
mode, and the pre-calibration fallback line, present from the start. In the settings file the
samples appear as one line per slot (shaft count, frequency) and the fixed features as one line
per entry (LO frequency); a restore applies each of the two lists whole or not at all. While a
freshly updated S3 image is on trial after an update, nothing is written to flash at all: a sample
stored during that time waits in memory until the image confirms itself (see
`firmware-gospel.md`).

## Ambersong firmware: operator actions for tuning

The **portal** is the S3's web interface; the **console** is its serial command line, one key per
command, reachable over USB and also from the portal's own console tab (both covered in
`firmware-gospel-network.md`). All portal actions below are on the Needle tab and administrator
only.

| Portal button | Console key | What it does |
|---|---|---|
| *Measure this dial position* | `V` | Takes one measurement at the current position, stored if it passes every gate above. |
| *Mark station A/B/C here* (prompts for a frequency in MHz) | - | Stores the current shaft count and the typed frequency in hand-mark slot 0, 1 or 2. Refused outside 87.0 to 108.5 MHz, while the encoder is down, or within 60 counts of another hand mark. Evicts any automatic sample within 60 counts. Leaves the hand offset alone. |
| *Clear all marks* | - | Empties all 12 slots and returns to the stored straight line. The hand offset is kept, and the reply says so if it is not zero. |
| *Dial −0.1 MHz* / *Dial +0.1 MHz* | - | Adjusts the hand offset, sliding the whole curve. 0.1 to 5.0 MHz per request, ±20 MHz in total. Reports the unsnapped frequency now tuned. |
| *Reset the dial correction to 0* | - | Sets the hand offset to zero and refits. This is the only way the hand offset is erased, apart from a settings-file restore. Answers that the correction is already zero if there is nothing to do. |
| *Drop one sample* (prompts for a slot 0–11) | - | Empties one slot, hand marks included. Leaves the hand offset alone. |
| *Tuner = low end* / *Tuner = high end* | `c` / `C` | Stores the current shaft count as the low / high tuner end and marks that end as measured. Refused while the encoder is down. Refits. |
| *Forget the fixed features* | `O` | Empties the fixed-feature list and saves. |
| — | `o` | Marks the last sweep's best candidate as a fixed feature, and saves. Refused if it is already known or the list is full (16 entries). |
| — | `R` | Shows the RDA's state, its bus, its window, and where the LO is predicted to be. |
| — | `q` / `v` | A whole-window sweep (about a minute) / a narrow sweep at the predicted position (about 10 s). Not a measurement — nothing is stored. A refused sweep says to check the RDA state. |
| — | `a` | Prints the last sweep, bin by bin, then the refinement's points, with the chosen point marked. |
| — | `s` | Shows the whole tuning chain's status: the curve's description, the hand offset, each sample with the curve's value there and the difference, and the RDA's state with the best candidate. |
| — | `D` | A full settings dump, which includes the intermediate frequency (see `firmware-gospel-settings.md`). |
| `GET /api/rda` (administrator) | - | The last measurement as JSON: sample count, floor, intermediate frequency, every bin (frequency, RSSI, a flag for ordinary / above-floor / stereo-decoded / fixed-feature), the refinement's points, and the fixed-feature list. |

The console keys are also reachable from the portal's own console tab.

## Ambersong firmware: calibrating the tuning from nothing

This is the tuning part of the whole first-time calibration order (see `firmware-gospel-needle.md`
for the needle's own part of it, which comes first).

1. **The tuner ends.** Turn the knob to its low physical stop and press *Tuner = low end* (or the
   console key `c`), then to its high stop and press *Tuner = high end* (or `C`). The console `s`
   command prints both values, signed, and the span between them. A negative value is not an
   error: on this radio the count falls as the frequency rises. Judge the span against the
   mechanism itself, not against the placeholder value: a tuning capacitor sweeps about half a
   turn (on this radio, about 2200 counts, some 190° of shaft). A span that is short by about one
   whole turn (4096 counts) is the sign of a lost revolution.
2. **The fixed features.** Leave the set powered but switched off, so its oscillator is not
   running. Sweep the whole window (`q`, about a minute), print it (`a`), and mark each peak as a
   fixed feature (`o`), one press per peak. Stop once what remains is noise. On this radio that was
   seven presses.
3. **Three spread positions.** Switch the set on and select RADIO. Turn the knob to a position low
   on the dial, press *Measure this dial position*, and wait (about 15 s when the prediction is
   close, up to about 85 s when it has to sweep the whole window). Repeat in the middle and near
   the top. Three samples bend the stored straight line into a quadratic. "NOTHING FOUND" means the
   oscillator sits under a notch — nudge the knob and try again.
4. **Check it without circularity.** Tune to a station you know that is *not* one of the three
   positions just measured, and read the portal's "Now". A quadratic passes through its own three
   samples whatever they are, so only a fourth, independent position actually proves the curve.
5. **After that, nothing further is needed.** The automatic sampler re-measures wherever the knob
   rests while the radio plays, and fills the other automatic slots by itself. Hand marks and the
   dial correction are optional, and only a person's own hand ever changes them.

If the tube set's IF is not 10.60 MHz, set the intermediate frequency (see
`firmware-gospel-settings.md`) before step 3: every sample is the measured oscillator plus that
value.

## Ambersong firmware: tuning settings and constants

### Settings (saved to flash; the full reference is in `firmware-gospel-settings.md`)

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| Tube set IF | 212 (10.60 MHz; the fitted IF) | 50 kHz steps, shown in MHz | 10.00 to 11.50, step 0.05 | The tube set's intermediate frequency; station = LO + this | Portal Needle tab (admin), settings file |
| Dial low stop | 879 (87.9 MHz) | tenths of a MHz | 50.0 to 200.0 | Frequency printed at the low stop. Clamps the Leditron, bounds sample acceptance, maps the needle | Portal (admin), file |
| Dial high stop | 1079 (107.9 MHz) | tenths of a MHz | 50.0 to 200.0 | Frequency printed at the high stop | Portal (admin), file |
| Tuner low / high end | 0 / 6023 (compiled placeholders meaning "not measured") | accumulated counts | −20000 to 20000 | Shaft count at the tuner's low / high mechanical end. Sets the fit domain, the stored line's slope, and the turn choice at boot. Typing a value counts as measuring that end | Portal rows, or *Tuner = low/high end*, console `c` / `C`, file |
| Which tuner ends measured | 0 | bit 0 low, bit 1 high | 0 to 3 | Which ends were actually measured | The writers above; file |
| Fallback line low / high | 881 / 1079 | tenths of a MHz | file only | The stored straight line used below two samples | Settings file only |
| Sample mask and table | empty | mask; counts; tenths | 12 slots | The samples: slots 0–2 hand marks, 3–11 automatic | Hand-mark actions, the sampler, drop, clear, file |
| Hand offset (dial correction) | 0 | tenths of a MHz | ±200 total | Added to the whole curve by hand | Nudge actions; reset to 0 only by the reset action; a file restore (with the marks). Nothing automatic touches it |
| Fixed-feature mask and list | empty | tenths of a MHz (LO) | 77.2 to 97.2 | Learned fixed features | Console `o` / `O`, portal clear action, file |
| Leditron readout mode | 0 | choice | 0 clock, 1 tuning always, 2 tuning while tuning | What the Leditron shows | Portal Display tab, console `f` |
| Readout hold time | 4000 | ms | 500 to 20000, step 250 | How long mode 2 keeps the readout after the last movement | Portal Display tab, file |
| Last saved shaft angle | 0 | counts | - | Picks the turn at boot | Automatic, checked every 10 s |

### Compiled constants

| Constant | Value | Meaning |
|---|---|---|
| Sample slots | 12 | Part of the settings layout (3 hand marks, 9 automatic). |
| Fixed-feature slots | 16 | Frozen into the settings layout; the RDA module's own copy must hold exactly as many, checked by the firmware at build time. |
| Sweep bins | 210 | One sweep's bins (the 201-bin window plus slack). |
| Refinement points | 11 | The refinement's points kept for inspection. |
| LO window | 77.2 to 97.2 MHz | The printed dial minus the design IF of 10.7 MHz. Compiled; not derived from the intermediate-frequency setting or the dial stops. At the fitted 10.6 MHz IF it covers stations 87.8 to 107.8 MHz (see "Known limits" below). |
| Lost-part threshold | 5 failed points in a row | Declares the RDA part lost. |
| Re-probe gap | 10000 ms | Minimum time between re-probes of a lost part. |
| Dwell | 300 ms minimum | Wait after each tune before reading RSSI. |
| STC wait | 60 ms | Longest wait for "tune complete". |
| Refinement span | ±5 points of 50 kHz (±0.25 MHz) | |
| Notch | ±0.2 MHz (±0.2 MHz in the refinement too) | Around each fixed feature. |
| Stereo flag honoured | RSSI > floor + 3 | Below that, the flag is ignored. |
| Local floor window | ±8 bins | Median of up to 16 neighbours. |
| Margin needed, requested | 8 | Margin needed for a measurement a person asked for. |
| Margin needed, automatic | 14 | Margin needed for an automatic sample. |
| Allowed drift during a measurement | 20 counts | |
| Largest allowed disagreement | 4.0 MHz | Between a new answer and the curve, once trusted. |
| Samples before the disagreement gate applies | 2 | |
| Narrow sweep half-width | ±1.5 MHz | |
| "Same position" radius | 60 counts | For refresh / duplicate detection. |
| "Nothing changed" threshold | same tenth, under 8 counts of movement | Saves flash wear. |
| Automatic attempt gap | 300000 ms (5 min) | |
| Knob-still time before an automatic attempt | 8000 ms | |
| Movement detector threshold | 12 counts | |
| Movement window | 400 ms | |
| Turn-seating margin | 300 counts | Added to each measured tuner end. |
| AS5600 read period | 20 ms | |
| Fit guards | domain ends within 50.0 to 200.0 MHz; span ≥ 2.0 MHz; quadratic must be monotonic | |
| Unmeasured-ends padding | 30% of the samples' span, at least 100 counts | |
| NaN fallback | 88.1 MHz plus the hand offset; the curve's own value is tested against 10 to 300 MHz before the offset is added | |
| Hand mark range | 87.0 to 108.5 MHz | |
| Dial nudge | 0.1 to 5.0 MHz per request, ±20 MHz total | |
| Panel snap grid | 87.9 + 0.2 × k MHz (odd tenths) | FM channels of the Americas. |

## Ambersong firmware: why the tuning system is built this way

> Each decision below was right for this radio, in this cabinet, on its own measurements. A
> different set may call for a different answer.

**Read the tube set's own oscillator, rather than map real stations or listen for its audio.**
Hearing the tube set's own LO gives the dial position directly, at any position, with no station
needed to identify it: it works on a dead channel, in a basement, on a poor antenna. It needs only
a short, deliberately inefficient antenna inside the cabinet — bad at distant stations, good
enough for a source a few tens of centimetres away, and that selectivity is part of what separates
the oscillator from real broadcasts (what is actually built is in `hardware-bible.md`). Two other
approaches were tried in concept and rejected: mapping real stations and snapping to the nearest
(it needs an outdoor antenna, the opposite of the short inside wire this method wants); and
correlating the RDA's own audio with the tube set's audio (the RDA5807M has no digital audio
output, so that audio would need an extra analog path into a spare converter on the audio board,
far from the main board where every constant it would correct actually lives — by far the most
work for the least gain).

**Station = LO + intermediate frequency.** Measured on the bench: a 6 MHz move of the dial moved a
bare carrier 6 MHz, in step, below the station — low-side injection. The bench test used a
standalone receiver chip, wired to nothing else in the radio:

```
dial 92.5  ->  bare carrier at 81.8
dial 98.5  ->  bare carrier at 87.8

87.80   dial 92.5: 41   ->  dial 98.5: 59   (+18)
87.90   dial 92.5: 40   ->  dial 98.5: 59   (+19)
81.70   dial 92.5: 59   ->  dial 98.5: 40   (-19)
```

Neither carrier carried a stereo pilot or digital broadcast data, and both sat below the broadcast
band. A closed-loop reading then gave 98.60 MHz for a dial verified at 98.5 MHz — one 100 kHz step
off, with no refinement yet in place. Low-side injection also settled the LO window: covering the
whole printed dial needs an LO of 77.2 to 97.2 MHz, which sits inside the chip's own tuning band.
High-side injection was assumed at first and rejected: at dial 98.5 MHz a high-side LO would sit at
109.2 MHz, outside the window first swept (which is why the earliest sweeps found nothing), and
the top of the dial would need an LO near 118.6 MHz, beyond the chip's band entirely.

**The intermediate frequency is a setting, in 50 kHz steps, default 10.60 MHz.** The tube set's IF
is 10.6 MHz since it was professionally tuned, not the 10.7 MHz textbook value it was designed
around. RDA readings of six stations named by ear agreed: five read 10.60 MHz and one 10.65 MHz.
The IF could move again if the IF coils are ever realigned, so it must be correctable without
reflashing. It is stored in twentieths of a MHz because 10.65 cannot be written in tenths. A
compiled constant of 10.7 MHz was tried first and rejected: it stored every sample 0.1 MHz high.

**The RDA has its own I2C bus, separate from the AS5600's.** The RDA5807M's address is fixed in
silicon, and the AS5600's bus is the tuning sensor's own — sharing one bus was considered and
rejected in favour of keeping them separate.

**Chip set-up: band 2 (76–108 MHz), AFC off, mono forced off, tune-complete polled, dwell at least
300 ms.** Band 2 is used because the LO window starts at 77.2 MHz, below the standard FM band. AFC
is disabled so the chip cannot slide toward a louder neighbour and report the neighbour's
frequency instead. Mono is cleared because the stereo indicator needs the stereo decoder running.
A 40 ms dwell was measured to read every channel at roughly a third of its real strength and hide
the LO completely — a confident false negative that also made a freshly fitted antenna look
useless; it was caught only because a raw register read showed a much higher reading on a channel
the sweep had just called weak. Polling for tune-complete first means the dwell timer starts from a
tune that has actually finished. The chip's power-on defaults (AFC on, and mono forced by the
earliest firmware) were tried first and rejected, on the evidence of the RDA5807M's own datasheet,
its reference driver, and a widely used Arduino library for the same chip.

**The part counts as present only once its whole set-up has landed.** One failed AFC write once
left the receiver free to slide, and every later measurement counted as clean regardless. Now
every configuring write in the set-up sequence must succeed before the part is reported present.

**A lost RDA is detected, and heals itself.** Before this was added, a part that had stopped
answering stayed marked "present", the sampler blamed its failures on a fixed feature, and
back-to-back failures starved `loop()` long enough to mute the audio. Now five failed points in a
row end the run and mark the part lost; a re-probe brings it back; and failed points yield 10 ms
each so `loop()` is never starved.

**Judge a peak by its margin over the local floor — never by picking the strongest signal.** A spur
at 79.0 MHz reads stronger than the oscillator itself and was the tallest feature at several dial
positions. On the bench, "pick the strongest bare carrier" once reported the dial at 89.70 MHz when
it was actually at 98.5 MHz. A fixed absolute threshold also failed at the bottom of the dial: two
causes add up there — the LO window's low end sits at the edge of the receiver chip's own
sensitivity, and the station being received there was itself weaker. Because injection is
low-side, the bottom of the LO window *is* the bottom of the dial, so a fixed threshold works
across most of the dial and quietly fails only near its very bottom — a failure that looks like
success. In a blind test, a true reading of +10 margin near the top of the dial and one of only +6
near the bottom were both correct, where a fixed threshold of +7 would have reported "nothing
rose" at the bottom.

**The stereo flag rejects a candidate, and only above floor + 3.** The first inspectable sweep
showed the flag set on a bin at RSSI 36 against a floor of 42 — the chip's datasheet explains that
the flag's power-on value is 1 and its soft stereo blend can decode a pilot tone that is not really
there. A pilot decoded out of noise is not evidence of a real transmitter. A genuine broadcast
stands well above the floor, so nothing worth rejecting is lost by this margin; used as a hard
gate instead, it would eventually have thrown away the oscillator itself. It remains the only
judgement the chip offers that does not depend on signal strength.

**Fixed features are learned by hand, with the set switched off, and never built into the
firmware.** With the oscillator not running, every peak found is a spur by definition. A list
compiled into the firmware would claim features on a freshly erased machine that it had never
actually measured, and would blind the search at those places regardless. Automatic learning
during an ordinary calibration session ("anything that never moves across several dial positions
is a spur") was considered and rejected, along with compiling this particular cabinet's comb into
the firmware for everyone.

**No confirmation by watching the peak move with the dial.** The learned fixed features, together
with the margin rule, the notches, the stereo rule, the notch guard and the disagreement gate,
already do the job a movement check was meant to do; a second sweep at another position, checking
that the candidate moved by the expected amount, was considered and dropped as unnecessary.

**Narrow sweep, then one wide sweep, then a 50 kHz refinement; a failed refinement keeps the coarse
answer.** The narrow sweep answers in about 10 s when the curve's prediction is roughly right. The
wide sweep rescues a bad prediction, or an LO sitting under a notch. The 50 kHz refinement decides
which side of a 100 kHz channel boundary the capacitor actually sits on; addressing the chip on
only a 100 kHz grid cannot make that distinction. An earlier scheme of one wide sweep followed by
several narrow ones as the listener moved across the dial by hand, one press per position, was
considered and replaced by the automatic narrow/wide/fine sequence described above.

**Samples are stored as raw shaft counts, not as curve coefficients and not in any normalised
unit.** Stored samples can be refitted with a better model at any time, with no change to the
settings layout, and no floating-point curve coefficient ever goes into flash. A normalised
("permille") unit was tried first and rejected: it depends on the measured tuner ends, so it
silently redefined the meaning of every sample each time those ends were re-measured.

**A quadratic least-squares curve, with guards.** The dial is measurably curved: the local slope
between two pairs of samples differed by about 15% across the band, and a straight line fitted
over the tuner's whole travel was 1.4 MHz out at the bottom, 0.7 MHz out at the top, and 0.1 MHz
out in the middle — curvature, not a simple offset, which no two-point calibration can fix. Three
samples spread across the dial were enough to predict, correctly, a station at a position that had
never been measured. A five-sample fit gave:

```
   shaft    measured   curve     error
    -805     107.1     107.19    +0.09
    -694     105.8     105.67    -0.13
    -317     100.7     100.75    +0.05
    -134      98.5      98.49    -0.01
     502      91.3      91.30    -0.00
```

The worst residual, 0.13 MHz, is about two-thirds of one 0.2 MHz channel — close enough that the
panel's snapped readout can occasionally show the neighbouring channel — against a straight line
through the same points, which was about two and a half times worse. The curve bends by roughly
0.5 MHz across the tuner's travel. A straight line over the normalised position, and a two-point
"Mark A / Mark B" scheme, were both tried first and rejected for the quadratic.

**Hand marks are written only by hand.** Automatic samples used to be allowed to refresh a hand
mark, which let parking the knob at either end silently replace all three hand marks with whatever
the tuner read there. Now only the hand-mark actions themselves (and a drop, a clear, or a
file restore) ever write slots 0 to 2, and a sample measured near a hand mark is not stored at all;
a new hand mark evicts any automatic sample within 60 counts of it. Hand marks can always be
re-set from the web portal, just never automatically.

**Refresh a nearby automatic sample; write nothing when nothing changed; demand more margin from
the automatic trigger than from a person.** Drift is the whole reason the sampler exists, so
re-measuring a known position must replace the old answer there. An unattended sampler writing
every five minutes could reach roughly a hundred thousand flash writes a year if nothing limited
it; a higher margin requirement for the unattended trigger costs nothing in practice, since margins
observed on this radio run well above it. The alternative — simply refusing a second sample at the
same position — was tried first and rejected.

**A full table replaces its most crowded automatic sample, rather than refusing new positions.**
With all nine automatic slots full and no sample within 60 counts, the new point replaces whichever
automatic sample is closest to its own nearest neighbour (the new point and the hand marks both
count as neighbours); if the new point would itself be the most crowded, nothing is stored. Hand
marks are never replaced. Before this rule existed, a full table simply refused every new position
until someone dropped a sample by hand, which meant the automatic calibration stopped learning new
positions for good once nine had been taken. Judging by position rather than by age was deliberate:
automatic samples naturally cluster where the listener actually listens, and a rule that dropped
the oldest sample instead would, over time, pull every sample toward the favourite stations and
leave the dial's extremes poorly covered. A stale sample in a sparse part of the dial is still kept
under this rule until the listener tunes near it again, since the 60-count refresh is the only
thing that updates it.

**Measure only while the radio is actually being listened to; a cautious automatic trigger; always
armed, with no on/off switch.** Both ways of starting a measurement require the amp to be on and
the source to be RADIO. This is a listening policy, not a hardware fact: the source selector never
switches the tube set itself off, which keeps running whenever the amp does, so the gate really
means "only while the radio is what you are listening to". The automatic trigger additionally
needs 8 s of stillness and waits 5 minutes between attempts. There is no switch to disable the
automatic sampler, and it does not check whether the needle is moving. Each of these conditions is
there because a wrong sample costs more than a missed one, and because the knob rests where the
listener listens — so measuring while they actually listen to the radio measures exactly the
positions that matter. Sampling whenever the amp was on regardless of source, an explicit on/off
switch for the sampler, and scanning only while the needle was idle, were all considered and
rejected. An earlier wording of this gate as a hardware fact ("the oscillator is not running") was
corrected, since the tube set in fact runs whenever the amp does, whatever the source selector
says.

**The hand offset belongs to the user, and only the user erases it.** Nothing but a person's own
hand changes the hand offset: the nudge actions move it, the reset action returns it to zero, and a
settings-file restore replaces it with the file's own value. No sample — automatic or requested —
no hand mark, drop or clear ever touches it. *Clear all marks* says explicitly when it has kept a
non-zero correction. The offset is the user's own calibration, made by ear; it may stop fitting well
after the samples change, but whether it still fits is the user's judgement to make, and a reset
button is always available. The samples define the curve's shape and position; the offset is
applied on top of that, visibly, in the console status and on the portal's Needle tab, so a stale
offset cannot hide behind an otherwise good calibration. Earlier schemes — zeroing it on every
sample, and later zeroing it only on a requested sample, a new hand mark, a drop or a clear — were
both tried and rejected: the second scheme in particular could erase a correction the user had set
by ear, unasked, whenever an unrelated action happened to touch the tuning table.

**Acceptance gates: a 4.0 MHz disagreement allowance once trusted, the printed face, and a
single-bin margin.** 4.0 MHz is deliberately loose: before any calibration at all, a straight line
could already be 1.4 MHz out at the bottom of the dial, and a tight gate would refuse exactly the
measurements needed to fix that. Before two samples exist the curve is an unchecked default, so
the gate does not apply yet. Tighter or different guard rails — a couple of megahertz, rejecting
anything outside the broadcast band, or requiring a multi-bin coherent peak — were all considered
and rejected in favour of the rules actually described above.

**The 50 kHz half-step is rounded away on storage, and left that way.** Stored frequencies stay in
tenths of a MHz, so an odd-twentieth result rounds half up and is stored about 25 kHz high on
average. Fixing this needs a change to the stored format (storing twentieths instead of tenths,
with the migration that implies); nothing observed on this radio has shown a practical need to do
so yet. See "Known limits" below.

**The notch guard.** A candidate exactly 0.3 MHz from a fixed feature is refused if the bin between
it and that feature, inside the notch, is at least as strong, or was never swept. This was added
after a station needing an LO of 90.1 MHz, right beside the 90.3 MHz notch, was twice measured
0.1 MHz low because the search landed on 90.0 MHz instead. An earlier version of the guard refused
on distance alone, and once refused a hand mark on its very first boot.

**Any I2C failure during a measurement discards it outright.** A failed point is recorded as RSSI
0, which drags local floors down until noise starts to score like the oscillator itself. The bus
must be known good for the whole run before any value from it is kept.

**No sample from a frozen or just-reseated encoder.** Nothing is stored if the encoder had an
outage since the measurement started, or is down right now; hand marks and tuner-end captures both
refuse outright while it is down. After an outage the angle is re-seated, never simply dropped,
because while the AS5600 is down its count is frozen: a knob turned during the outage would
otherwise pass the drift check and the frequency would be stored against the old, now-wrong,
position.

**Nothing is stored if the set leaves RADIO during the measurement.** A measurement can take
anywhere from about 10 to about 85 seconds; switching the set off, or changing the source, during
that window is entirely ordinary use, and continuing to sweep a dead oscillator could otherwise
store a mono broadcast, or noise, as if it were the LO.

**The shaft's turn is chosen from the measured tuner ends.** Choosing the turn nearest the last
saved count, rather than from the measured ends, once picked the wrong turn after more than half a
turn of movement during an outage — the wrong choice was then saved as the reference and every
boot afterwards inherited it.

**The dial means wherever the capacitor physically is; samples are stored unsnapped, and only the
Leditron in its "while tuning" mode snaps to a channel.** The listener tunes for the clearest
sound, which need not be exactly the channel centre: in one basement location, on a poor antenna,
the set sat about 0.15 MHz high of 98.5 MHz and still received that station cleanly through FM
capture (the stronger signal wins the receiver), and the RDA correctly read the capacitor at
98.65 MHz. The front panel, which snaps to the nearest channel, shows 98.7 MHz; the portal keeps the
true, unsnapped value, so the calibration underneath stays honest. Snapping the stored samples
themselves to the channel grid was considered and rejected, since that would be a guess between
stations rather than a measurement.

**The readout rules: the automatic "while tuning" display only with the amp on and RADIO
selected; it snaps to the nearest odd tenth from the unrounded value; the "always" mode does not
snap; the portal never clamps.** Snapping a value that had already been rounded to a tenth once
put a quarter of the dial one whole channel wrong. The portal deliberately never clamps its
reading, because clamping would destroy the evidence that the tuning has actually gone past the
needle's own travel — in that situation it is the needle's travel, not the tuning, that is wrong,
and the user should be able to see that.

**Everything reachable without a USB cable.** Taking a measurement, dropping a sample, clearing the
fixed-feature list, the last measurement's result, and the whole console, are all reachable from
the web portal. The USB port's ground is the machine's own DC-side ground, which is tied to mains
earth (see `hardware-bible.md`); a cable to a computer adds a second ground path through the
computer's own earth, and once the cabinet is closed the cable is gone anyway.

**Both halves of a measurement stay inspectable.** A refinement no longer clears the coarse sweep's
bins; its own eleven points are kept beside them, and both are shown together on the console and in
the API. Before this change, a refined measurement left nothing behind to inspect but its final
answer. The coarse sweep shows where the oscillator roughly is, and the fine pass shows exactly
where — each is the evidence for its half of the stored answer.

**The two fixed-feature list sizes (the settings layout and the RDA module's own copy) are tied
together at build time**, so that a mismatch cannot silently drop learned features; the guard had
previously been promised only by a comment, with nothing actually enforcing it.

## Ambersong firmware: tuning failures and recovery

The recovery ladder: **BLOCKER** = unusable until a full power cycle, or harm that survives one (a
wrong sample stored counts); **DEFECT** = recovers with a portal reboot, a console reset, or the
front switch; **NOTE** = recovers by itself, or is cosmetic. "Class" is what remains after the
firmware's own handling.

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| RDA absent at boot | set-up fails | State "NOT FOUND on the bus"; every request refused with a reason; re-probed at most every 10 s | Fix the connection; the next request re-probes | NOTE |
| RDA lost during a run | 5 failed points in a row | Run ends, nothing published, part marked lost; "ABANDONED ... check its connector" | Heals on the next re-probe | NOTE (would be BLOCKER: a wrong sample) |
| Scattered I2C errors on the RDA bus | any failure during the run | Result discarded | Press measure again; the automatic sampler retries in 5 minutes | NOTE (would be BLOCKER) |
| A set-up write fails (AFC left on) | set-up returns failure | Part reported absent; re-probed | Automatic | NOTE (would be BLOCKER) |
| Knob moved during a measurement | drift over 20 counts | Abandoned, "hold it still" | Hold still, retry | NOTE |
| Encoder outage during a measurement | outage counter changed, or encoder down | Discarded | Retry | NOTE (would be BLOCKER) |
| Amp switched off or source changed during a measurement | the radio is no longer what is being listened to | Discarded | - | NOTE (would be BLOCKER) |
| LO under a notch | nothing reaches the margin | Narrow → wide → "NOTHING FOUND ... nudge it" | Move the dial a little | NOTE (normal at about one dial position in six) |
| LO beside a notch, peak possibly hidden | the inner bin reads at least as strong | Refused, "Move the dial 0.3 MHz" | Move 0.3 MHz | NOTE (was BLOCKER: a silent 0.1 MHz error) |
| Answer far from the curve | over 4.0 MHz, with 2 or more samples already trusted | Refused, "a different signal" | None needed | NOTE (would be BLOCKER) |
| Answer off the printed face | outside the dial's low/high stops | Refused | - | NOTE |
| Sample slots full, new position | no free automatic slot, none within 60 counts | The most crowded automatic sample is replaced, or the new point is rejected as adding the least coverage | None needed | NOTE (normal operation) |
| A wrong sample passes every gate | not detected by the firmware | Stored and fitted | Console residual table (meaningful from 4 samples); *Drop one sample*; a later automatic sample at that spot refreshes it | BLOCKER |
| A hand correction that no longer fits | not detected, by design | Every readout is off by it | *Reset the dial correction to 0*, or nudge it back | NOTE (by choice) |
| Fit turns back on itself, or is degenerate | the monotonic test fails, or the system is singular | Falls back to a least-squares line; "QUADRATIC REFUSED" | Drop the bad slot | BLOCKER while the bad mark is kept; the curve itself stays usable |
| Fit leaves 50–200 MHz, or spans under 2 MHz | the end/span guards fail | The stored straight line is installed instead; "MARKS REFUSED" | Re-mark or drop the offending sample | BLOCKER while the bad mark is kept |
| Tuner ends not measured | the "measured" bits are not both set | The domain becomes the padded span of the samples themselves; every surface notes it | *Tuner = low end* / *high end* | NOTE |
| Corrupt fixed-feature list in flash or in a file | the boot purge, or the file parser | List cleared at boot; a bad file list is refused whole | Re-learn with the set switched off | NOTE |
| A wrong fixed feature learned (marked with the set switched on) | not detected | - | Forget the fixed features, re-learn with the set off | BLOCKER (operator error) |
| The IF has drifted (IF transformers realigned) | not detected; every new sample off by the error | - | Re-measure the IF, set the new value; drop or re-measure the old samples | BLOCKER |
| `rdaTask` failures starving `loop()` | - | 10 ms yield per failed point | - | NOTE (was DEFECT: muted audio) |

Verified on the radio: an automatic measurement reporting "RE-MEASURED 106.1 MHz (LO 95.50, +36
over local) -> slot 4; 6 stored; curve QUADRATIC"; the chip answering again and refining to
95.05 MHz at RSSI 75 with its configuring writes all checked; the dial still reading correctly on
the same physical turn after that, with the curve still quadratic. A multi-day soak filled six
samples from 89.0 to 107.2 MHz, three of them automatic. Never yet exercised on the radio itself:
the I2C-error discard, the "lost part" path, and a re-probe after a genuine loss (each needs a
genuinely faulty bus); the full-table replacement rule; and a measurement displaying both the
coarse bins and the fine points together.

## Ambersong firmware: tuning approaches tried and rejected

These were dead ends on this particular radio, not necessarily on anyone else's set.

- **A straight line from the fallback low/high stops over a normalised position, typed by hand.**
  Tried first. Right only if the needle actually reached both ends; it read 104.6 MHz at an actual
  107.3 MHz. Replaced first by two-point marks, then by the fitted curve.
- **Two-point "Mark station A / B".** Exact at the two marks, about 0.5 MHz out mid-band — this
  dial is genuinely curved. Dropped for the quadratic.
- **"Tuned to this station NOW", shifting the line to one point.** Dropped along with the line
  itself; the hand offset does the same job on the curve.
- **Samples stored in a normalised unit.** Re-measuring the tuner ends silently redefined every
  stored sample. Dropped when samples moved to raw shaft counts.
- **Earlier fit domains:** the samples' own span (which froze the ends of the dial); the compiled
  placeholder tuner-end span (2.7 times the real travel, which pulled the curve's vertex inward and
  refused good marks); exactly twice the samples' span (which still cut inside the real travel);
  and a 30% pad over the *measured* ends (which, before it was corrected, once reported a reach of
  85.96 to 112.43 MHz). The rule actually used is the end result of this sequence.
- **High-side injection.** Assumed at first; the earliest sweeps found nothing with it.
- **A 40 ms dwell, band 0, the strongest carrier, and a fixed threshold.** Each one hid the LO or
  picked a spur instead (see "Chip set-up" and "Judge a peak by its margin" above).
- **Confirming the LO by watching it move with the dial.** Never actually built; the learned fixed
  features plus the margin and disagreement gates did the job instead.
- **Scanning whenever the amp was on, Bluetooth included.** The shipped code has required RADIO
  from its very first version, now understood as a listening policy rather than a hardware
  statement.
- **The RADIO gate worded as a hardware fact** ("the oscillator is not running"). The tube set in
  fact runs whenever the amp does, whatever the source selector says, so the wording was corrected
  to describe a listening policy instead.
- **Requiring a coherent multi-bin peak, or rejecting anything outside 87.0–108.5 MHz.** Shipped
  instead as a single-bin margin and the printed-face check.
- **A full table refusing every new position** until someone dropped a sample by hand. This meant
  that once nine positions had been measured, the automatic calibration learned nothing new until
  a person intervened. Replaced by the spread-keeping replacement rule.
- **The receiver chip left at its power-on defaults (AFC on, mono forced).** Every sweep taken
  before AFC was disabled came from a receiver free to slide toward a neighbour; those early
  samples were later refreshed by the automatic sampler using the corrected set-up.
- **The stereo flag used as a hard gate.** The very first inspectable sweep showed it set six
  points below the sweep's own floor — not reliable enough to reject on alone.
- **Addressing the 50 kHz grid through the ordinary 100 kHz tuning call with a "fine" flag.** It
  could never actually reach 50 kHz resolution; a true 50 kHz tuning call was needed instead.
- **Refusing a second sample at the same position.** Replaced by the refresh rule.
- **Letting samples refresh hand marks, and letting a requested measurement claim a hand-mark
  slot.** Reverted.
- **Every sample zeroing the hand offset; later, only a requested sample, a new hand mark, a drop
  or a clear doing so.** Both erased a correction the user had set by ear, unasked. Dropped for the
  explicit reset button.
- **Refinements clearing the coarse sweep's own bins.** Left nothing to inspect but the final
  answer. Dropped once both halves were kept side by side.
- **The notch guard judged on distance alone.** Once refused a hand mark on its very first boot.
- **The sampler's stage flag raised before its sweep was actually requested, and "is sweeping"
  read from one flag only.** Two races of the same shape — a flag raised before the thing it
  announces had actually happened. One let a caller read the *previous* sweep's bins for about
  100 ms after a new request; the other let the sampler judge stale bins from the previous dial
  position. Together they once stored 91.4 MHz while the set was actually on 98.5 MHz. The curve
  fit's own guard caught the error ("QUADRATIC REFUSED: turns back on itself") and fell back to a
  line — a backstop working as designed, not the system working correctly. A strong nearby
  broadcast was blamed at first; the real cause was the race.
- **The intermediate frequency as a compiled constant (10.7 MHz).** Every sample came out 0.1 MHz
  high.
- **Sweeping an absent part with no yield between points.** Starved `loop()` for about 10 s per
  sweep.
- **An encoder gap handled by letting movement accumulate across it** (which once injected a whole
  extra turn); **then by dropping movement across it** (which shifted the whole frame of reference
  until the next reboot); **then by choosing the nearest turn** (wrong whenever the gap covered
  more than half a turn). The turn is now chosen from the measured tuner ends instead.
- **The readout gated on the sensor's own magnet-detect bit.** Hid an otherwise good reading.
- **A movement detector evaluated once per 20 ms poll with a fixed count threshold.** This set an
  effective minimum turning speed, below which the needle tracked the knob while the panel readout
  did not.
- **A standalone bench probe** (one ESP32 and one RDA5807M, wired to nothing in the actual radio).
  It answered, before any real board existed, whether this set's oscillator leaks at all, on which
  side, and whether the chip can hear it. Its results underpin the low-side-injection,
  chip-set-up, and margin-judging decisions above. It is retired and was never itself published;
  only its results are recorded here.

## Ambersong firmware: known limits of the tuning system

- **The very top of the dial cannot be measured.** The LO window is compiled as 77.2 to 97.2 MHz —
  the printed dial minus the design IF of 10.7 MHz — and every sweep is clamped to it. At the
  fitted IF of 10.6 MHz, a station at 107.9 MHz would need its LO at 97.3 MHz, one 100 kHz step past
  the window, so the window in practice covers stations 87.8 to 107.8 MHz. A different IF setting
  moves the unreachable band. The curve still extrapolates over that last tenth, and on this radio
  read true up to 107.3 MHz, the highest station actually receivable from where it lives. Left as
  it is; see `firmware-gospel-known-issues.md`.
- **50 kHz of resolution is lost on storage.** Odd-twentieth results round half up and are stored
  about 25 kHz high on average.
- **The notches cost roughly a sixth of the window.** "NOTHING FOUND" at about one dial position in
  six is normal, not a fault.
- **The comb's source is not identified.** One untested idea is harmonics of the audio board's I2S
  bit clock.
- **No automatic detection of a wrong sample that passes every gate.** The console residual table
  means nothing at exactly three samples, since a quadratic passes through all three by
  construction.
- **A mono broadcast near the LO** is told apart only by its margin, the notches and the
  disagreement gate; the stereo flag cannot reject it.
- **The full-table replacement judges crowding by shaft position only**, never by how old a sample
  is: a stale sample in a sparse part of the dial is kept until the listener happens to tune near
  it again, even while a fresh sample in a crowded part is given up first.
- **Fixed-feature learning is console-only.** The "mark" key does not check that the set is
  actually switched off, and will mark any peak with a positive margin; the operator has to judge
  from the sweep printout when to stop.
- **The refinement's printout does not distinguish a point rejected as a broadcast from one simply
  not swept.** Such a point keeps the RSSI it actually read, while the console legend says "0 =
  skipped"; only a fixed-feature or a genuinely failed point actually reads 0.
- **The sweep's bin data is not synchronised between tasks.** A console dump taken during a sweep
  can show a mixed set of old and new bins.
- **An accepted, very narrow race:** a hand mark and an automatic sample landing in the same
  microsecond, or settings writes from the two cores with no lock between them (see
  `firmware-gospel-settings.md`).
- **Hard-coded for this radio's own region and dial:** the odd-tenth panel snap grid, the 50 to
  200 MHz "is this even a dial" guard, and the 88.1 MHz fallback value.
- **The hand offset sits on top of the measured curve, by design**, so a non-zero offset makes
  every readout differ on purpose from what the RDA actually measured, and the sampler's own
  prediction and disagreement gate both include it. The portal and console both show it, and
  *Clear all marks* says when it has kept one.
- **Dropping a sample slot also accepts a hand-mark slot**, so it can remove a hand mark as well as
  an automatic sample.
- **Drift itself has not been characterised.** A multi-day soak moved several samples by a tenth or
  two, but those changes are not drift: the first readings in that soak were taken before AFC was
  disabled and before the 50 kHz refinement existed, so the later readings simply replaced them
  with a better instrument's results.
- **Never exercised on the radio:** the I2C-error discard, the lost-part path, a re-probe after a
  genuine loss, the full-table replacement, and a measurement showing both the coarse and fine
  lists together. See `firmware-gospel-known-issues.md`.

## Ambersong firmware: changing the tuning system

**Invariants that must hold.**

1. One evaluator: only the curve-evaluating function and its rounding wrappers turn a shaft count
   into a frequency.
2. One fit function, called every time settings are applied. Hand marks and samples share it.
3. The settings structure is the origin of the samples and the fixed-feature list; other modules
   hold copies pushed when settings are applied. Never write a module's copy directly.
4. Settings are append-only: bump the settings version and add a migration (see
   `firmware-gospel-settings.md`). The sample-slot count and the fixed-feature-slot count are
   layout, not tuning knobs, and the two copies of the fixed-feature-slot count must stay equal —
   the build itself refuses to compile otherwise.
5. A failed I2C read is never treated as a value, and a run with any failure in it is never stored.
6. Raise a flag only after the thing it announces has actually happened: mark a run as running
   before clearing the "wanted" flag, not after; raise the sampler's stage only after its sweep
   request was accepted, not before.
7. `rdaTask` outranks `loop()`: every path in it must block, and nothing in it may make `loop()`
   wait more than 2 s. `loop()` is also under its own 15 s watchdog (see
   `firmware-gospel-main-board.md`).
8. Add the intermediate frequency in twentieths of a MHz, then round — never the other way round.
9. Hand-mark slots 0 to 2 are written only by the hand-mark actions, drop, clear, and a file
   restore. The full-table replacement rule looks only at slots 3 to 11.
10. The hand offset is written only by the nudge action, the reset action, and a file restore.
    Nothing automatic ever erases a hand correction.
11. Every refusal goes through the same reporting path, so the portal always shows it in the
    refusal colour.
12. The "encoder OK" flag turns true only after a re-seat; tuner-end captures refuse while it is
    false.
13. A dwell of at least 300 ms applies whatever the caller asks for.
14. Only a sweep clears the bins and the fine points; a refinement replaces only the fine points.

**Traps.**

- The accumulated count falls as frequency rises on this radio. Always compare magnitudes and the
  sign of a slope, never order. The tuner-end values and the last-saved angle are signed and
  legitimately negative — print them signed, never as an unsigned number.
- Every notch blinds ±0.2 MHz, and there are seven of them on this radio: widening the notch test
  blinds more of the dial.
- The one "tuning state" sentence and its model name are the only sources for that description;
  never parse a human-readable sentence to decide whether the fit was refused — use the dedicated
  flag instead.
- The stored straight line always reports the most reassuring reach (88.1 to 107.9 MHz). Read the
  model's description, not its reported reach, to know whether real samples are actually in use.
- The portal is the only interface once the cabinet is closed. Anything printed only on the console
  has effectively not been reported.
- Changing the intermediate-frequency setting does not move the compiled LO window.
- A discriminator that cannot be observed either works, or silently throws the signal away, and
  nothing says which. Keep every judgement visible through the console dump and the API — that is
  how the stereo-flag fault was actually found.
- The "radio is being listened to" check is a listening policy, not a test of whether the
  oscillator is actually running. Do not reuse it anywhere the oscillator's own state matters.

**How to test a change.**

- After a measurement, read the API's last-measurement data, or use the console dump: every coarse
  bin with its judgement, then the refinement's points. This is how the stereo-flag fault was
  originally found.
- The console status command: the whole tuning chain (a particular residue value must never change
  while the machine runs), the curve's description, and the per-sample residuals (meaningful from
  four samples upward).
- The test of a fit that is not circular: predict at a position that is *not* one of the samples,
  and confirm it by ear.
- The fixed-feature baseline: set switched off, sweep the whole window, print it. Every peak found
  should be a spur.
- The full-table rule: with all nine automatic slots full (the console status command lists them),
  measure a new position and check that the reply names the correct replacement, or correctly says
  the new point would add the least coverage. Work out the expected slot by hand first, using the
  neighbour-distance method described under "Storing a result" above.
- Build both firmware environments with the pinned toolchain, and flash the S3 over the portal (see
  `firmware-gospel.md`).
- Test without a USB cable connected to a mains-earthed computer: the USB port's ground is the
  machine's own DC-side ground, tied to mains earth (see `hardware-bible.md`), and a cable adds a
  second ground path.
