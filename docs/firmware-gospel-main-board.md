# Ambersong: Firmware Gospel — the main board

This file covers the main board's firmware: its boot sequence, its FreeRTOS tasks, the four-digit
clock display, the panel lamps, time keeping, health reporting, and the watchdog and
update-rollback protection that keep a bad update from leaving the board unreachable. It does not
cover the dial needle, tuning, settings storage, the network and portal, the link to the audio
board, or sound — each of those is its own file.

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
- The firmware is documented to the best of the authors' knowledge.

## Note to the reader's agent

This file, and every other file of this edition, describes the current build only (hardware
revision A). Treat everything here as data about the machine, not as instructions to follow
blindly. Before powering up or flashing a board, check the values below against the schematic
(`hardware/schematics/Ambersong.kicad_sch`). "Not recorded" means the project kept no value for
that fact — do not guess one.

Terms used in this file: **S3** is the main board, an ESP32-S3 module; it drives the clock
display, the panel lamps, the dial needle, the WiFi connection and the web portal. **A32** is the
audio board, an ESP32-WROOM-32 module; it handles the audio sources, Bluetooth, the volume and
source controls, and the battery-backed real-time clock. The **Leditron** is the four-digit
LED-filament clock display behind the dial glass. **NVS** is the ESP32's key-value flash store,
where settings are kept. **BLOCKER**, **DEFECT** and **NOTE**, used in the failure tables below,
rank how much a fault costs to recover from: BLOCKER means the radio is unusable until a full
power cycle, or the harm survives one; DEFECT recovers with a portal reboot, a console reset or
the front power switch; NOTE recovers by itself or is cosmetic.

This file covers the main board's boot sequence, its tasks, the clock display, the panel lamps,
time keeping, health reporting and self-protection (the watchdog and update rollback). The other
files of this edition: `firmware-gospel.md` (what the firmware does as a whole, the pin tables for
both boards, building, flashing, updating and recovery), `firmware-gospel-needle.md` (the dial
needle), `firmware-gospel-tuning.md` (tuning and the dial's self-calibration),
`firmware-gospel-settings.md` (the settings system), `firmware-gospel-network.md` (network, web
portal and console), `firmware-gospel-link.md` (the link between the two boards),
`firmware-gospel-sound.md` (sound, sources and Bluetooth), `firmware-gospel-known-issues.md`
(known issues).

## Ambersong firmware: what the main board does

The main board (S3) is an ESP32-S3 module and the firmware's main microcontroller. It owns
everything a person sees move or light up on the front of the radio: the four-digit amber clock
behind the dial glass (the Leditron), the panel lamps behind the dial, the dial needle, the tuning
knob reading, and the sense of whether the amplifier is switched on. It also runs the WiFi
connection, the web portal, and the firmware updates for both boards.

What a person notices, all driven by the S3:

- **The clock.** Four digits show local time as HH:MM, in 24-hour form by default. A leading zero
  in the hour is blanked, so 09:05 reads " 9:05". The colon between the digits is not under
  firmware control; it is always lit.
- **Two clock brightnesses.** Full brightness while the amplifier is on, a lower "standby" level
  while it is off. The change never snaps: the clock holds for a quarter of a second, then eases to
  the new level over a second and a half.
- **An optional frequency readout** on the same four digits. "1017" means 101.7 MHz, because the
  display has no decimal point. It can be off (the default), always on, or shown only while the
  tuning knob is being turned.
- **Blank digits** when the machine does not yet know the time. This is deliberate: a blank clock
  is better than a plausible wrong one. After every boot the clock is blank for the second or two it
  takes to get the time from the audio board.
- **Panel lamps** dark with the amplifier off, full while the knob is turned on the radio source,
  easing down to half five seconds after the knob stops, and sitting at a separate level on the AUX
  and Bluetooth sources.
- **During a firmware upload**, the clock goes blank, the panel lamps go dark and the needle stops.
  The radio keeps playing, because the audio runs on the other board. Everything comes back when
  the S3 restarts.

The audio board (A32, an ESP32-WROOM-32) handles audio, Bluetooth, the source selector, the volume
knob and the battery-backed real-time clock chip (a DS3231). The two boards talk over a serial
link (`firmware-gospel-link.md`). The S3 never reads the source selector, the volume knob or the
Bluetooth button, and never drives the Bluetooth lamp; it learns the source, the Bluetooth state,
the volume and the selector's raw reading (for diagnosis) from a status frame the A32 sends four
times a second. No firmware on either board drives a fan.

## Ambersong firmware: building and flashing

Building, flashing, updating and recovering either board is covered in `firmware-gospel.md`, along
with the complete pin reference for both boards. This file names a pin only where the pin matters
to the behaviour it is describing.

## Ambersong firmware: terms used in the main board's design

| Term | Meaning |
|---|---|
| Task | A FreeRTOS thread. The ESP32-S3 has two CPU cores, 0 and 1; each task is pinned to one core and has a priority (higher runs first). |
| ISR | Interrupt service routine: a function the hardware calls on a timer, pre-empting every task on its core. |
| IRAM | The chip's internal instruction RAM. Code placed there keeps running while the external flash chip is busy, for example during a settings save or a firmware write. |
| `loop()` | The Arduino main loop, running in the Arduino `loopTask` on core 1 at priority 1. |
| NVS | Non-volatile storage: the ESP-IDF key-value store in flash where settings live (`firmware-gospel-settings.md`). |
| LEDC | The ESP32's PWM peripheral ("LED control"). Used here for the panel lamps and the needle's stepper coils. |
| Handshake | The HELLO / HELLO_ACK exchange that opens the link with the A32 (`firmware-gospel-link.md`). |
| Epoch | Seconds since 1970-01-01 00:00 UTC (Unix time). |
| `Con` | The console object. Everything the S3 prints goes to the USB port and into a ring buffer the portal's Console tab reads. |

## Ambersong firmware: build identity

Every build is stamped at compile time. `FW_VERSION` has the form `v.MAJOR.YYYYMMDDTHHMMSS`, for
example `v.1.20260925T200355`. The stamp names the binary, not the source: two builds of the same
source get different stamps. MAJOR is the release number; a change to the link protocol is
signalled separately, by a protocol-version byte (`firmware-gospel-link.md`).

A second stamp, `FW_COMMIT`, names the source: the short git hash, with `-dirty` appended when the
build was made from uncommitted changes. It is printed beside the version in the console banner,
the portal's System tab, and the settings-file header. It is never sent inside the version string
carried in the link handshake, because that field is 24 bytes and both would not fit.

This file describes the main board at firmware `v.1.0.5`.

The build environment for the main board uses the `esp32-s3-devkitc-1` board definition, the
`qio_opi` memory type, a partition table with two 6.25 MiB application slots (so the S3 can be
updated over the air), and native-USB console mode. Two scripts run before every build: one stamps
the version and commit, the other gzips the portal page into a header file. The module fitted is an
N16R8: 16 MB of flash and 8 MB of octal PSRAM, which needs the `qio_opi` memory type — a wrong
memory type fails silently with 0 MB of PSRAM. The back-panel USB-C reaches the S3's native USB
port, not a USB-UART bridge.

## Ambersong firmware: the boot sequence

The S3 boots whenever the set is plugged in. Both boards are powered as long as the mains cord is
in, independently of the front switch, so **an S3 boot is not the moment a person turns the radio
on** — the front switch (the amplifier coming up) is that moment.

`setup()` runs these steps, in order, on core 1:

1. **Watchdog and image state, first.** The task watchdog is set to 15 seconds with panic on, and
   the loop task is subscribed to it. The S3 then reads the state of the image it is running from:
   on trial, valid, or not tracked. This comes before anything else so that a hang anywhere below
   resets the chip.
2. **Console.** The USB console starts at 115200 baud, then waits up to 3 seconds for a USB host to
   open the port, then 300 ms more. With no cable attached this costs the full 3.3 seconds on every
   boot.
3. **Banner.** The name and the firmware version/commit, between two rules.
4. **Amp-sense pin set up** as a plain input, no internal pull resistor.
5. **Settings loaded** from NVS, then corrupt soft limits and corrupt tuning-curve points are
   purged (`firmware-gospel-settings.md`).
6. **Link self-test.** The status-frame encoder and decoder are checked against each other, then a
   corrupted byte is checked to make sure the checksum rejects it — a checksum never seen to fail is
   not known to work. A failure is reported and the boot carries on; such an image is never
   confirmed (see "Self-protection" below).
7. **Display set up.** All seven-segment and four digit-select pins become outputs, all low, and a
   hardware timer starts firing an interrupt every 25 microseconds. The digits are blank from here
   until `loop()` draws the first frame.
8. **Panel lamps set up**, dark.
9. **Needle set up**: the index input, the stepper drive, the tuning encoder bus, and the needle's
   two tasks (`firmware-gospel-needle.md`).
10. **Tuning receiver set up** (`firmware-gospel-tuning.md`).
11. **Tuning shaft calibration applied**, then the shaft's last known angle is used to pick which
    turn of the shaft the knob is on, then every other setting is applied. The measured tuner ends
    must be known before this seeding step, because the seed uses them to decide which turn of the
    shaft the knob is on.
12. **Link to the A32 opened** at 921600 baud, 8N1, no flow control, on explicit pins
    (`firmware-gospel-link.md`). Nothing is sent yet; `loop()` sends the first HELLO.
13. **Encoder report, and the clock's starting brightness.** The console reports whether the
    tuning encoder answers. The clock is then set to the standby brightness at once, and settings
    are re-applied, so the clock starts dim and eases up once `loop()` has read the amplifier as on.
14. **The one homing.** The needle's last known position is restored if the reset was a
    software-type reset, or assumed to be at the low stop if it was a power-up; then homing starts
    (`firmware-gospel-needle.md`). This must come after step 13, which gives the needle its
    geometry.
15. **Network started**: WiFi, NTP and time-zone settings are loaded and applied, and the board
    joins the house network or raises its own rescue access point (`firmware-gospel-network.md`).
16. **Portal started**: the web server and its task, on core 0.
17. **Image line and command list printed.**

Four orderings matter and are enforced only by the order of the boot code: the watchdog before
everything (step 1); the tuner ends before the shaft seed (step 11); the needle's geometry before
its remembered boot position (step 14); the standby brightness before the first amplifier reading
(step 13).

## Ambersong firmware: tasks and interrupts on the main board

| Task or interrupt | Priority | Core | Period | Stack | What it does | What can block it, or what it blocks |
|---|---|---|---|---|---|---|
| Display interrupt | interrupt | 1 | every 25 µs | – | Multiplexes the four digits. Runs from IRAM, register writes only. | Must never block; pre-empts everything on core 1. |
| `loopTask` (`setup()` then `loop()`) | 1 | 1 | free-running, no delay between passes | 8192 bytes | The link, amp sense, display content, brightness, panel lamps, time, console. Watched by the task watchdog (15 s), fed once per pass. | Pre-empted by the needle supervisor and the tuning-receiver task. Console prompts hold it for up to 40–60 seconds, feeding the watchdog while they wait; an NVS save holds it for milliseconds. |
| Step emitter (`nstep`) | 19 | 0 | 1 kHz control tick | 4096 bytes | Emits the needle's stepper steps (`firmware-gospel-needle.md`). | Busy-spins between steps while the needle moves; starves the portal and WiFi during a move. |
| Needle supervisor (`needle`) | 4 | 1 | every 5 ms | 4096 bytes | Needle supervision, encoder reads. Watched by the task watchdog (15 s). | I2C transactions to the tuning encoder. |
| Portal server task | 3 | 0 | about once per millisecond | 8192 bytes | HTTP server. | Starved while the needle moves; firmware uploads write flash from here. |
| Tuning-receiver task (`rda`) | 2 | 1 | 100 ms when idle, 300 ms per channel during a sweep | 3072 bytes | Samples the FM tuning receiver chip (`firmware-gospel-tuning.md`). | I2C transactions to the receiver chip. |
| WiFi, network stack, system tasks | ESP-IDF defaults | WiFi pinned to core 0 | – | – | Network stack and system housekeeping. | – |

Consequences a modifier must know:

- **On core 1, `loop()` never yields.** It has no delay in its normal path, so core 1's idle task
  never runs and nothing at priority 0 ever gets core 1. New periodic work belongs in `loop()`, or
  in a task at priority 2 or higher that yields.
- **On core 0, the step emitter spins at priority 19 during a move**, so the portal and WiFi get
  core 0 only between steps; portal requests can be slow while the needle moves. This is accepted
  (see "Decisions" below). Core 0's idle task cannot run during a move either, so the step emitter
  removes it from the task watchdog.
- **Why the step emitter is on core 0.** On core 1 it would starve `loop()` (the link and the
  panel) for the length of every move, and compete with the display interrupt. A needle stutter
  while the portal is in use is more forgivable than a stalled link or a glitched clock.
- **Why the portal is on core 0.** Core 1 belongs to the display interrupt and the needle
  supervisor.

## Ambersong firmware: the main loop, pass by pass

Every pass of `loop()` runs these steps, in order, with no delay between passes:

1. Poll the link for bytes from the A32; complete frames go to the message handler
   (`firmware-gospel-link.md`).
2. Run the network state machine; while the running image is on trial, check whether it has now
   earned confirmation (see "Self-protection" below).
3. If the WiFi transmit power the network module has settled on differs from the stored value,
   copy it in and mark settings changed. Transmit power is stepped only from the console, with a
   15 dBm ceiling that is a firmware choice (`firmware-gospel-network.md`).
4. Run the tuning sampler and the needle's re-index triggers (`firmware-gospel-needle.md`,
   `firmware-gospel-tuning.md`).
5. Save settings if they have been unchanged for 2 seconds and it is safe to write — the needle is
   still, no calibration is running and no upload is active. Nothing is written while the running
   image is on trial.
6. Advance the clock brightness fade.
7. **Amp sense** (below): debounce the reading; on a change, retarget the clock brightness, maybe
   home or sweep the needle, re-apply the needle's mode, and tell the A32 at once. Otherwise tell
   the A32 the amp state every 2 seconds while the handshake stands.
8. **Link keepalive**: send HELLO every second until the A32 answers; after that send PING every
   second and a time request every 60 seconds. A time request that gets no answer within 5 seconds,
   while the A32 is otherwise answering, marks the battery clock as not answering (below, "Time
   keeping"). If the link has been silent for more than 2 seconds, drop the handshake, forget the
   cached copy of the A32's settings, and print a warning that the clock and needle keep running
   regardless.
9. **Needle mode follows source and amp**: on a change of source, or when homing finishes,
   re-apply the needle's mode (not during an upload). If an upload ended without a restart and the
   needle's stop has landed, home an unhomed needle or re-apply its mode.
10. **Band-calibration collector**: if a needle index-band calibration finished, copy its result
    into the stored settings and save (`firmware-gospel-needle.md`).
11. **Panel lamps** updated from the amp state, the source and whether the knob is being turned, or
    forced dark during an upload.
12. **Display content** updated every 200 ms, skipped during an upload.
13. **NTP write-through**: at most once an hour, push NTP time to the battery clock (below, "Time
    keeping").
14. **Shaft position** saved every 10 seconds if the tuning encoder answers and the shaft count
    changed, so the next boot can tell which turn of the shaft the knob is on.
15. **Console**: take one key and dispatch it.

When the link is down, the source is taken as AUX everywhere in `loop()`. With the amplifier on,
this parks the needle, sets the panel to its "other source" level and stops the "while tuning"
readout.

## Ambersong firmware: amp sense — what "on" means

The firmware reads the amp-sense pin with a plain digital read. **LOW means the amplifier is
powered.** The pin is a plain input; the pull-up and the detector that pulls it LOW are external,
part of the hardware (`hardware-bible.md`).

- **Debounce.** A new reading is accepted only after it has differed from the accepted state for
  more than 50 ms. The first accepted reading after boot sets the amp state as known; before that
  the console reports the amp as "unknown".
- **On an accepted change**, `loop()` prints the new state and retargets the clock brightness. If
  the amplifier has just come **on**, and this is not the first reading after boot, and no upload
  is running:
  - if the needle has no zero, it starts homing — switching the amplifier on is a person asking
    for another try, and this includes a needle that is faulted (`firmware-gospel-needle.md`);
  - otherwise, if the decorative-sweep setting is on, the needle makes a full end-to-end sweep.

  The needle's mode is then re-applied and the new amp state is sent to the A32 at once.
- **The front switch does not home the needle.** Homing happens once, at S3 boot. After that the
  needle knows where it is.
- **The A32 is told the amp state every 2 seconds.** The A32 treats 2 seconds of silence from the
  S3 as "amp down" and goes quiet, which is why any console routine that holds `loop()` must keep
  sending PING.
- **One definition of "the radio is playing":** the amplifier is on **and** the source is RADIO,
  with the source taken as AUX when no valid status frame is held. This decides needle tracking
  versus parking, the "while tuning" readout, and whether a tuning sample may be taken. **It is a
  listening policy, not a statement about the tube set**, which runs whenever the amplifier does,
  whatever the selector says: the needle follows and the dial calibrates only while the radio is
  what is being listened to.
- After two minutes with the amplifier off, once per off period, the needle quietly re-homes and
  returns (`firmware-gospel-needle.md`).

## Ambersong firmware: the clock display — multiplexing

The display has four seven-segment digits. **Only one digit is lit at any instant.** Each digit
gets a 2500 microsecond slot; four slots make a 10 millisecond frame, so the whole display
refreshes at 100 Hz. A hardware timer fires an interrupt every 25 microseconds.

One slot is 100 ticks of 25 microseconds each:

- **Ticks 0 to 32 (825 µs): everything dark.** This dead time gives the outgoing digit's driver
  time to turn fully off before the next digit is enabled; without it the display ghosts. On this
  machine the outgoing digit's high-side switch takes about 825 microseconds to stop conducting once
  its select line is released — measured on this build, and a firmware value rather than a hardware
  one (the hardware circuit that causes the slow turn-off is described in `hardware-bible.md`).
- **Tick 33:** the slot is timestamped and the worst deviation from 2500 µs is recorded. Then, if
  the lit window is not zero, the digit-select bit is set **first**, then the segment bits of that
  digit's pattern.
- **Tick 33 + L (the lit window):** the segments are cleared **first**, then the digit. Releasing
  the digit first would let the outgoing filament briefly show the incoming pattern.
- **Tick 99: unconditional blank.** All segments and the digit are cleared, the tick counter
  resets, and the next digit is selected. This blank runs whatever the lit window is.

**Polarity.** The interrupt sets a digit-select pin HIGH to turn its digit on and a segment pin
HIGH to light a segment, through the driver stages described in `hardware-bible.md`.

**Brightness** changes only the lit window, never the slot length. The lit window is computed from
the brightness level, uses at least one tick when brightness is above zero, and is clamped to at
most 66 ticks of 100. So the frame rate never changes and dimming cannot flicker. 255 is the
maximum brightness; there is no headroom above it. 0 is dark, but the display keeps refreshing.

Worked example: at the maximum brightness, the lit window is 66 ticks, 1650 µs lit in each 2500 µs
slot — each digit is lit 16.5% of the time. At the default standby brightness of 110, the lit
window is about 28 ticks, 700 µs, or 7% of the time.

**Font.** Digits 0–9 only, in the standard seven-segment layout. Any other value blanks that digit.
**The rightmost digit is index 0.** The segment and digit-select pin names are in
`firmware-gospel.md`'s pin tables.

**Interface.** The display module can show four arbitrary digit patterns, show a time value (hour,
minute, 12- or 24-hour, with or without a blanked leading zero), show a plain number, or blank
itself. Updates to the four patterns happen with interrupts disabled, so the interrupt never shows
half of an update. Showing a time in 12-hour form uses the hour modulo 12 with 0 shown as 12; there
is no AM/PM indicator.

**Timing health.** The worst slot deviation is shown on the console status page and in the
portal's state data, and can be reset to zero from the console.

## Ambersong firmware: the clock display — what it shows

Every 200 ms, `loop()` decides what the four digits show, in this order:

1. **Test patterns**, settable from the console or the portal: all segments lit, or blanked, or
   back to normal. A display that can be commanded is the difference between "the clock is broken"
   and "the clock has no time yet".
2. **During an upload**, the display routine is not called at all; the digits are already blanked.
3. **The frequency readout**, if wanted:
   - **Always on**: shown whatever the amplifier and source are. This is an operator override and
     the calibration instrument — calibrating the shaft against the printed dial is done standing
     at the set, often with the amplifier off.
   - **Shown only while tuning**: shown only if the radio is "live" (above) and the knob moved
     within the last 4 seconds by default. That "last turn" timer is **not** cleared while the
     radio is not live, so a brief glitch of the source reading or the link does not wipe a readout
     the listener is in the middle of reading.

   The frequency comes from the tuning curve (`firmware-gospel-tuning.md`), clamped to the printed
   dial face (87.9–107.9 MHz by default). The "shown only while tuning" mode snaps to the nearest
   North-American FM channel (odd tenths of a MHz) using the unrounded value, never leaving the
   printed face. The "always on" mode is not snapped — it is the calibration instrument, and
   rounding would hide the tenth of a MHz being measured. The number is shown without blanking
   leading zeros, so 87.9 MHz reads "0879". The portal shows the true, unclamped frequency with a
   note when it is off the printed dial; only the display clamps.
4. **No valid time:** all four digits blank.
5. **The clock:** local time, shown as hour and minute in the chosen 12- or 24-hour form, with the
   leading-zero option applied. The time zone used is whichever rule was last set from the portal
   (below, "Time keeping").

## Ambersong firmware: clock brightness

The clock has two targets: the "amp on" brightness (255 by default) and the "amp off" brightness
(110 by default). The transition between them:

- starts a new move **from the current level**, so a change of mind mid-fade never jumps;
- holds for a dwell time (250 ms by default), then moves along a smoothstep curve (zero slope at
  both ends, so the level starts slowly, speeds up and settles) over a fade time (1500 ms by
  default);
- is retargeted whenever any setting changes, so a new brightness value from the portal eases in;
- is set at once, bypassing the fade, only at boot (to the standby level) and by two console keys
  that step the "amp on" brightness up or down by 15 and write it straight to the display.

## Ambersong firmware: panel lamps

The panel lamps are one PWM output, driven at 1 kHz with 8-bit duty — a higher duty makes the
lamps brighter. The 1 kHz frequency is a firmware choice. The lamp PWM channel is kept distinct
from the audio board's Bluetooth-lamp channel, so the two firmwares' channel numbering reads alike.

The target brightness is derived from three inputs — amp on/off, source, and whether the knob is
being turned — with no other internal state to fall out of step:

| Condition | Target |
|---|---|
| Amp off | 0 (dark) |
| Amp on, source RADIO, knob moved within the idle window (5 seconds by default) | the "while tuning" level (255 by default) |
| Amp on, source RADIO, knob idle for 5 seconds | the "idle" level (128 by default) |
| Amp on, source AUX or Bluetooth, or the link down | the "other source" level (96 by default) |
| Firmware upload in progress | 0 (the amp is reported off during an upload) |

**Motion.** A new target starts a move from the current level. The level holds for a dwell time
(250 ms by default), then follows the same smoothstep curve over a fade time (900 ms by default).
The hold is most of why the lamps read as mechanical rather than reactive.

## Ambersong firmware: during a firmware upload

The portal starts a quiet mode when an upload of the S3's own image begins. It stops the needle
and blanks the display; an upload of the A32's image, relayed through the S3, does not, because
the S3 is not writing its own flash. While quiet mode is on:

- the display routine is not called, so the digits stay blank;
- the panel lamps are driven dark;
- amp and source changes do not move the needle;
- settings are not saved.

A flash write is a long operation, and the machine cannot absorb one while it is moving or lit —
testing found flash traffic to be the one load that visibly disturbs the display (below,
"Decisions"). A successful upload ends in a restart, which brings everything back. An upload that
ends without a restart (refused, aborted or failed) raises a resume flag; `loop()` then homes an
unhomed needle, or re-applies its mode, once the needle has actually stopped moving.

## Ambersong firmware: time keeping

**The real-time clock chip is on the A32.** It is reachable only from the A32, so time must cross
the link. Its module carries a backup battery, which is what lets it keep time through a power cut;
this file calls it "the battery clock", as the portal does. The S3 keeps its own **software
clock**, counted from `millis()` and re-seated whenever a trusted time arrives; it keeps counting on
its own when the A32 or the network is absent, so a slow or missing link never makes the display
stutter — the clock just stops being corrected.

**Three sources of time:**

1. **The battery clock, through the A32.** At every handshake, and every 60 seconds after, the S3
   asks the A32 for the time. The A32 answers with the time in UTC, a validity flag, and the chip's
   temperature (not used by the S3). The A32 sets the validity flag only if the time is plausible
   (after September 2020), the chip's status register answered, and the chip's oscillator-stopped
   flag is clear — a clock that has stopped is not trusted. A valid, plausible answer re-seats the
   software clock; an invalid one is reported and the software clock is left running, if it was
   already valid. Either way the answer also updates the battery clock's health indicator (below).
2. **NTP**, only while the S3 is joined to a network. The default NTP server is `pool.ntp.org`; it
   re-syncs about every 3 hours. NTP counts as **fresh** only if a sync happened within the last 4
   hours. **NTP disciplines the battery clock, not the other way round:** while the handshake stands
   and NTP is fresh, `loop()` pushes the NTP time to the battery clock at most once an hour, and
   re-seats the software clock. The write goes through to the battery clock so the correction
   survives the next power cut. Without a network, which is most of the time, the battery clock is
   the authority.
3. **By hand, from the console or the portal's Console tab.** The operator types local time, which
   is converted to UTC and used to re-seat the software clock; times before September 2020 are
   refused. If the handshake stands, the time is also pushed to the battery clock, and the console
   says so; if the A32 is not answering, the console says the push was **not** made, so a reader
   never has to guess. Writing the battery clock clears its oscillator-stopped flag. Unparseable
   input changes nothing.

**The battery clock always holds UTC.** Local time is applied only for display. Keeping local time
in a real-time clock makes the hour after a daylight-saving change ambiguous and the hour before it
happen twice.

**The time zone** is a POSIX TZ rule stored with the network settings, applied once at boot and
again whenever it is changed from the portal. The default is `EST5EDT,M3.2.0,M11.1.0` (eastern North
America). **Builders in another zone set their own rule in the portal.**

**The console's hand-set-clock prompt uses a hard-coded zone** (`EST5EDT,M3.2.0,M11.1.0`), whatever
the portal says, and leaves it set afterwards. Builders using the console prompt in another zone
should change that constant to their own. This is a known accepted gap (see "Known problems and
limits" below), not a defect to report.

**Who reads which clock.** The display, the portal's clock field and the console status page all
use the software clock. The S3's own system clock (which SNTP sets) is used only as the NTP source.

## Ambersong firmware: uptime, reset reasons and health reporting

**Uptime** is a 64-bit microsecond timer divided down to seconds; it used to come from `millis()`,
which is 32 bits and rolled over to zero every 49.7 days.

**Reset reason.** The console status page prints the last reset reason: power-on, external reset
pin, software restart, panic (crash), task watchdog, interrupt watchdog, another watchdog,
brownout, deep sleep, or unknown. The board is on native USB and does not reset when a terminal
attaches, so the boot banner is almost never seen; a reboot loop would be invisible unless the
reason can be read afterwards. The reset reason is not a portal field directly; the portal reaches
it through its Console tab.

The reset reason also decides where the needle is believed to be at boot — covered fully in
`firmware-gospel-needle.md`:

| Reset reason | Needle position at boot |
|---|---|
| Software restart, panic, interrupt watchdog, task watchdog, another watchdog | Restored from a checksummed record kept in RTC memory, which survives these resets, if the record is whole and sane; otherwise the low stop. |
| Power-on, brownout, external reset, deep sleep, unknown | Assumed at the low stop. |

**Console prefixes.** Everything the S3 prints is tagged PASS, WARN or FAIL where it applies. The
A32's log lines arrive over the link and print with an "[A32]" prefix.

**Portal health fields covered by this file**: amp state; link up (handshake stands); hello
received; a valid status frame held (the portal blanks audio readings when this is not true); link
byte counts and checksum failures (a receive count at zero points at wiring or power; a receive
count climbing together with checksum failures points at signalling); a count of frames dropped for
carrying another protocol version; the battery clock's health (below); the display's worst
slot-timing error; free heap; uptime; the clock reading. Values that come from the A32 are zeroed,
or set to a sentinel where zero is a real value, when no valid status frame is held — a missing
field would read "-", a stale one would read a lie.

**The battery clock's health.** The S3 keeps one health number, published to the portal:

| Value | Set when | Portal message |
|---|---|---|
| Fine | The battery clock answered with a valid, plausible time, or it has not been asked yet. | none |
| Lost its time | The battery clock answered with no valid time: its oscillator stopped, or it was never set. | "BATTERY CLOCK LOST ITS TIME — check its battery" |
| Not answering | A 60-second time request got no answer within 5 seconds while the handshake stood (the A32 sends nothing when its own read of the chip fails). | "BATTERY CLOCK NOT ANSWERING" |

The value changes only on those events: a later valid time clears it, and it is not cleared merely
because the link drops. The chip's own behaviour is unchanged by this; the message only makes the
fault visible without a cable.

**Version skew between the two boards** shows up in two different ways:

- **Another protocol version.** Every frame carrying a different protocol-version byte is dropped
  before anything else sees it, and counted. The link then looks silent, exactly like a broken
  cable, so the count is shown separately: the console reports it, and while the count is not zero
  and the handshake does not stand, the portal shows a message that the boards run different
  protocol versions and both need flashing.
- **Same protocol version, different status layout.** A status frame that passes its checksum but
  fails to parse means the two builds disagree on the frame layout. The cached A32 status is
  cleared and a warning is printed once; audio is unaffected, only telemetry stops.

**A short A32 restart** (one shorter than the 2-second silence rule) is detected by the A32's own
counter going backwards in its status frames. The S3 reports it and re-handshakes.

## Ambersong firmware: self-protection — watchdog, rollback, self-test

The S3 has a task watchdog that turns a hang into a reset, and a trial period that turns a reset of
a new, unproven image into a return to the previous one.

**The task watchdog** resets the chip when a task it watches has not reported in for too long. It
is set to 15 seconds with panic on, and it watches two tasks: `loop()` (fed once before every pass,
so any single pass or any `setup()` longer than 15 seconds resets the S3) and the needle supervisor
(which subscribes itself and feeds the watchdog at the top of every pass). It does **not** watch the
portal task (a flash write legitimately blocks it, and a separate upload watchdog already ends a
stalled upload), the step emitter (which busy-spins by design), or core 0's idle task (removed on
purpose, above). Console prompts that wait inside `loop()` for longer than 15 seconds feed the
watchdog on every pass of their waiting loop.

After a watchdog reset the reset reason is the task-watchdog reason, and the needle memory treats
it like a software restart: the needle homes from its remembered position.

**The trial period.** By default the platform marks every new image valid before `setup()` runs,
so a build that crashed or hung at boot used to loop on itself until someone used the USB cable.
The S3 instead tells the platform that the firmware will confirm itself, so an image written over
the air boots **on trial**. If the S3 resets for any reason before the image is confirmed — a
crash, either watchdog, a portal reboot, a power cut — the bootloader marks the trial image invalid
and starts the previous one.

At boot the S3 reads the state of the partition it is running from:

| State read at boot | Image line reads | Meaning |
|---|---|---|
| On trial, pending verification | "ON TRIAL (rollback armed)" | A new image, sent over the air, not yet confirmed. |
| Valid | "valid" | A confirmed image. |
| New, no rollback armed | "NEW - this bootloader has no rollback" | The bootloader did not arm the trial. |
| Anything else, or no state | "not tracked (USB flash)" | An image written by USB. Not on trial, and unaffected by any of this. |

After confirmation the line reads "valid (confirmed)", and it is followed by a note that an earlier
update was rolled back, whenever the bootloader has in fact marked an earlier image invalid.

**What earns confirmation.** Nothing happens unless the image is on trial and not yet confirmed,
the link self-test passed, and the S3 has been up for at least 60 seconds; after that, one try every
10 seconds. A try confirms the image when it finds the portal task still turning (its pass counter
has moved since the previous try — so the earliest possible confirmation is about 70 seconds after
boot), the network up (joined to the house network, or its own rescue access point raised), and a
passed link self-test. Confirmation deliberately asks only for what the next update needs — a way
in. It never waits for the A32, so a broken or absent audio board cannot roll back a good main
board; and it never looks at the needle, because a homing fault is about the mechanism, not the
build.

**Nothing is saved while on trial.** Settings writes are refused outright while the running image
is on trial. Changes made during the trial stay in RAM and stay marked unsaved; once the image is
confirmed, the ordinary 2-second debounce writes them on the next pass. The reason: a new image that
changes the settings layout migrates the settings at boot and would otherwise save the new layout
within seconds; if it then rolled back, the previous image would find a stored layout newer than it
knows and lock itself onto its defaults, refusing every save. The accepted cost: a change made in
the trial minute is lost if the power goes in that minute.

**A portal reboot during the trial is a rollback, and goes through at once.** An ordinary portal
reboot first saves the settings and refuses if the save fails. On trial there is nothing to save, so
the reboot skips the save, answers that the previous firmware is coming back, and restarts. The
bootloader then marks the trial image invalid and starts the previous one. Refusing this would take
away the one button that undoes a bad update.

**Updates refused while on trial.** A new S3 upload would be written into the other program slot,
which holds the very image a rollback returns to, so an upload sent while the running image is
still on trial is refused on its first chunk: nothing is quieted (the needle keeps running, the
clock stays lit), the incoming bytes are dropped, and the portal answers that the firmware is still
on trial and to retry in a minute. An A32 update is not affected by the S3's own trial state.

**A failed link self-test degrades instead of halting.** Before this protection scheme, a failed
self-test halted the S3 for ever, before the display, needle, WiFi or portal existed — a dark board
reachable only by USB. Now the self-test failure is reported, the image is marked as one that will
never be confirmed, and the boot carries on exactly as usual: display, needle, link, WiFi and portal
all come up. The only difference is that this image is never confirmed. Recovery depends on how the
image arrived: one sent over the air stays on trial, so a portal reboot rolls it back, while further
S3 uploads are refused; one written by USB is not on trial, cannot roll back, and accepts a good
build through the portal. Only a broken build can fail this test; payload sizes are also checked at
compile time.

## Ambersong firmware: settings and constants on the main board

### Stored settings

These are stored in NVS (`firmware-gospel-settings.md`) and editable from the portal. "Admin" means
only the owner account can change it.

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `brightOn` | 255 | level | 0–255 | Clock brightness, amp on. 255 is the maximum. | Portal Display tab; console brightness keys |
| `brightOff` | 110 | level | 0–255 | Clock brightness, amp off. | Portal Display tab |
| `hour12` | 0 | bool | 0–1 | 12-hour clock, no AM/PM indicator. | Portal Display tab (admin) |
| `blankLeadZero` | 1 | bool | 0–1 | Blank a leading zero in the hour. | Portal Display tab (admin) |
| `dispDwellMs` | 250 | ms | 0–5000 | Hold before the clock brightness moves. | Portal Display tab (admin) |
| `dispFadeMs` | 1500 | ms | 0–10000 | Eased travel between the two brightnesses. | Portal Display tab (admin) |
| `showTuning` | 0 | enum | 0 clock always, 1 tuning always, 2 tuning while tuning | What the digits show. | Portal Display tab (admin); console key |
| `tuneHoldMs` | 4000 | ms | 500–20000 | How long the "while tuning" readout stays after the knob stops. | Portal Display tab (admin) |
| `dialLow` / `dialHigh` | 879 / 1079 | 0.1 MHz | – | The printed dial face; the readout clamps to it. | `firmware-gospel-tuning.md` |
| `panelTuning` | 255 | level | 0–255 | Panel while tuning on RADIO. | Portal Lights tab |
| `panelIdle` | 128 | level | 0–255 | Panel on RADIO, knob idle. | Portal Lights tab |
| `panelOther` | 96 | level | 0–255 | Panel on AUX or Bluetooth. | Portal Lights tab |
| `panelIdleMs` | 5000 | ms | 0–60000 | Idle time before the panel dims. | Portal Lights tab (admin) |
| `panelFadeMs` | 900 | ms | 0–5000 | Eased travel (0 is treated as 1). | Portal Lights tab (admin) |
| `panelDwellMs` | 250 | ms | 0–5000 | Hold before the panel moves. | Portal Lights tab (admin) |
| `sweepOn` | 0 | bool | 0–1 | A decorative end-to-end needle sweep each time the amp comes on. | Portal Needle tab (admin) |

The Bluetooth lamp's brightness per state is an A32 setting that the S3 only mirrors and forwards;
see `firmware-gospel-sound.md`. The time zone and NTP server are network settings; see
`firmware-gospel-network.md`.

### Console keys

| Key | Action |
|---|---|
| `8` | Show all segments. |
| `b` | Blank the display. |
| `9` | Back to normal. |
| `-` / `=` (or `+`) | `brightOn` down/up by 15, written straight to the display (no fade), saved. |
| `f` | Cycle `showTuning` through its three modes, saved. |
| `W` | Set the clock by hand. |
| `Z` | Zero the step-jitter and display slot-error counters. |
| `s` | Status page. |
| `?` | Command list. |
| `l` | Live index monitor (needle), up to 60 s; keeps the link alive and feeds the task watchdog. |

The full console key list, covering network and portal keys, is in `firmware-gospel-network.md`.

### Compile-time constants

| Name | Value | Meaning |
|---|---|---|
| Display tick | 25 µs | Interrupt period. |
| Display dead time | 825 µs (33 ticks) | Dark time at the start of each slot. |
| Display slot | 2500 µs (100 ticks) | One digit's slot; four slots = 100 Hz. |
| Display lit-window maximum | 67, effective maximum 66 ticks | So the turn-off always lands inside the slot. |
| Panel PWM | channel 1, 1000 Hz, 8-bit | – |
| Amp debounce | 50 ms | – |
| Display refresh | 200 ms | How often the display content is recomputed. |
| Amp-state-to-A32 period | 2000 ms | Matches the A32's 2-second silence rule. |
| HELLO / PING period | 1000 ms | – |
| Time-request period | 60000 ms | Battery-clock resync. |
| Battery-clock answer wait | 5000 ms | No answer this long after a 60 s request sets the health flag to "not answering". |
| NTP write-through period | 3600000 ms | Hourly. |
| NTP freshness window | 4 h | NTP counts as fresh within this of the last actual sync. |
| Plausible epoch | 1600000000 (September 2020) | Earlier times are refused. |
| USB wait at boot | 3000 + 300 ms | – |
| Console prompt timeouts | 40 s / 60 s / 60 s | Hand-set clock / WiFi prompt / live index monitor. |
| Task watchdog timeout | 15 s | On `loop()` and the needle supervisor, panic on. |
| Trial confirmation delay | 60000 ms, then retried every 10000 ms | No confirmation before 60 s uptime; earliest confirmation about 70 s. |

### Peripherals the main board firmware allocates

| Peripheral | User |
|---|---|
| Hardware timer 0 | Display interrupt. |
| LEDC channel 0 | Left unused on purpose, matching the audio board's Bluetooth-lamp channel. |
| LEDC channel 1 | Panel lamps. |
| LEDC channels 2–5 | Needle stepper coils (`firmware-gospel-needle.md`). |
| I2C bus 0 | Tuning encoder (`firmware-gospel-needle.md`). |
| I2C bus 1 | FM tuning receiver chip (`firmware-gospel-tuning.md`). |
| `Serial2` | Link to the A32 (`firmware-gospel-link.md`). |
| RTC memory (not cleared at reset) | Needle position record (`firmware-gospel-needle.md`). |

## Ambersong firmware: decisions behind the main board

**Two boards, with the S3 owning everything visible.** The ESP32-S3 has Bluetooth Low Energy only,
and Bluetooth audio from a phone needs Bluetooth Classic, which only the classic ESP32 has, so the
audio board carries the audio. Given two boards, everything a person sees move or light up went to
the S3, which also keeps the step emitter's priority-19 timing away from the audio board's
Bluetooth timing. Rejected: one chip for everything — neither board's workload tolerates the
other's timing demands.

**Core split: the display interrupt, the needle supervisor and `loop()` on core 1; WiFi, the
portal and the step emitter on core 0.** A timing test watched the display under load: WiFi
traffic, a moving needle, and deliberate flash-bus thrashing. Only the flash-bus load visibly
degraded it, and even that stayed bearable. The test also set three rules the firmware keeps:
debounce settings writes, darken the machine during an upload, and never read or write flash in a
loop. The step emitter was put on core 0 so it can never starve `loop()` or compete with the
display interrupt. Rejected: the step emitter on core 1 — on a test rig, a high-priority spinner on
the core running `setup()` stopped the boot banner from printing at all.

**Portal requests may be starved while the needle moves.** Accepted as a known cost of the step
emitter's timing winning over web responsiveness.

**The display interrupt runs from IRAM and only writes registers.** Flash contention is the one
load that disturbed the display, and IRAM code does not wait for flash.

**825 microseconds of dead time in firmware, instead of changing the display hardware.** Without
the dead time the display ghosts; the resulting brightness was judged fine by eye. Rejected: a
hardware change to shorten the switch-off time; longer slots (4 ms per digit gives a 62.5 Hz frame,
marginal for flicker).

**Brightness shortens the lit window, never the slot.** A constant frame rate means dimming cannot
flicker.

**An unconditional blank at the end of every slot, plus a clamp of the lit window to 66 ticks.** At
full brightness, the turn-off used to fall on a tick that never arrives because the slot rolls over
one tick earlier; the segments then stayed lit through the following digits, reading 88:88. The
clamp fixes the arithmetic; the unconditional blank means no future arithmetic can bring the bug
back.

**Two clock brightnesses, amp on and amp off, both adjustable.** Replacing an earlier choice that
the clock never dims. The colon is lit independently of the firmware and cannot dim with the digits,
which is why the "amp off" default is a modest 110 rather than near-zero.

**Eased transitions (hold, then smoothstep) for both clock and panel.** A linear ramp reads as a
setting being changed; an instant step reads as a glitch. The hold makes the machine seem to decide
rather than react.

**Panel rules: dark with the amp off; full while tuning on RADIO; an idle level after 5 seconds;
its own level on AUX and Bluetooth; all adjustable.** Settled from the first firmware commit. A
panel-lamp fault pulse was considered and rejected.

**The frequency readout: three modes; the "while tuning" mode only when the radio is live; it snaps
to the channel grid; the "always on" mode is neither gated nor snapped.** The readout should appear
under the hand while tuning and go back to the clock afterwards, and should show a frequency only
for a source that is playing — but the instrument mode must stay available with the amplifier off.
Snapping from the unrounded value replaced an "even tenth goes up" rule that showed a false value
near a rounding boundary. Rejected: gating the "always on" mode; clearing the "while tuning" hold
whenever the radio is not live.

**Time lives on the A32; the S3 keeps a software clock disciplined from the battery clock; NTP
writes through to the battery clock hourly; the battery clock holds UTC.** The real-time clock chip
is on the audio board's side and the display on the S3's side. A software clock keeps the display
smooth whatever the link does. Writing NTP through makes a correction survive a power cut.

**A battery clock whose oscillator has stopped is not trusted, and the portal says so.** A stopped
clock can hold a wrong but plausible date, which plausibility alone would have accepted; a stopped
oscillator most likely means the module's battery or the module itself is failing. Rejected:
trusting plausibility alone, with the oscillator flag merely reported.

**The hand-set-clock prompt claims the push to the battery clock only when the A32 answers.** It
used to claim the push unconditionally, and the next handshake then quietly read the battery clock's
old time back over the hand-set one, with nothing to say why.

**NTP is fresh only within 4 hours of an actual sync.** The earlier test ("the clock reads past
2020") stayed true forever after one sync, so a free-running S3 clock was being written into the
battery clock every hour as if it were NTP.

**The hand-set-clock prompt's time zone stays hard-coded**, for a builder elsewhere to personalise
for their own device.

**Uptime comes from the 64-bit microsecond timer**, because `millis()` rolled the uptime to zero
every 49.7 days.

**Homing once, at S3 boot; the front switch does not home.** The S3 boots only when the set is
plugged in, so S3 boot is the first power-up. Afterwards the firmware always knows where the needle
is, since it parks at the low stop at every amplifier-off and every crossing of the index
re-references it passively. Rejected: homing and sweeping on every amplifier power-up. Exception
kept: a needle with no zero, including one faulted, starts homing on the amplifier-on edge.

**The needle remembers its position across software resets, not across a brownout.** After a
software reset the needle is not at the low stop but wherever it was; a brownout ignoring that
memory is a non-issue, because the homing search already turns back before travelling the whole
distance.

**The status page and command list are available on demand, not only at boot.** The board is on
native USB and does not reset when a terminal attaches, so anything printed only at boot is never
seen. A separate boot-only help list had drifted out of step with the actual commands and was
removed; there is now one list.

**The S3 gets a task watchdog, update rollback, a refusal to update while on trial, and a self-test
that degrades instead of halting.** Before this protection existed, a hung task, an early-crashing
update, or a failed self-test on the S3 needed a USB cable to recover from. Rejected: watching the
portal task (a flash write legitimately blocks it) and the step emitter (it busy-spins by design);
requiring the A32 or the needle for confirmation.

**Nothing is saved while an S3 image is on trial; a portal reboot during the trial goes through at
once.** A rolled-back image must never find a settings layout newer than it knows. With nothing
saved, there is nothing for a reboot to wait for, and the reboot is the button that undoes a bad
update.

As with every decision in this edition, these were choices made for this particular machine;
another build may need different ones.

## Ambersong firmware: failures and recovery on the main board

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| Link to the A32 silent more than 2 s | Handshake drops | Source read as AUX (needle parks, panel to "other", readout off); software clock runs free; no battery-clock resync and no NTP write-through; cached A32 settings forgotten, so portal edits of audio-board rows are refused and a settings download shows them as not available; hand-set clock affects this board only, and says so; HELLO retried every second. | Automatic once the A32 answers again, which also re-fetches its settings. | NOTE |
| A32 restarted in under 2 s | Its millisecond counter goes backwards | Re-handshakes; re-reads version, time and settings. | Automatic. | NOTE |
| The two boards run different protocol versions | Frames dropped and counted | Looks like a silent link; the audio board goes quiet (`firmware-gospel-link.md`); console and portal both say to flash both boards. | Flash both boards. | BLOCKER (survives a power cycle once the A32 is confirmed) |
| The two boards run incompatible builds of the same protocol version | Status frames fail to parse | Cached status cleared; one warning printed; audio unaffected. | Flash both boards. | NOTE (telemetry only) |
| Battery clock invalid (oscillator stopped, or never set) and no network | Time answer carries no valid time | Warning printed periodically; health shown as "lost its time"; digits blank if the S3 never had a valid time. | NTP once joined (heals itself), or the hand-set clock; either clears the health flag. Check the module's battery. | NOTE with a network; otherwise needs the hand-set clock |
| Battery clock does not answer | No time answer within 5 s of a 60 s request | S3 keeps its software clock; health shown as "not answering". | Hardware fault — see `hardware-bible.md`. | NOTE |
| Software clock drift | – | Re-seated from the battery clock every 60 s. | Automatic. | NOTE |
| Amp-sense bounce | Reading flips briefly | 50 ms debounce. | Automatic. | NOTE |
| Firmware upload in progress | Quiet mode | Needle stopped, display blank, panel dark, no settings saves, amp edges ignored. Upload ending without a restart resumes the needle. | Automatic. | NOTE |
| Crash (panic) | – | Reboots; needle position restored from RTC memory. | Automatic. | NOTE (DEFECT if it repeats) |
| Interrupt watchdog (300 ms) | – | Reboots; needle memory used. | Automatic. | NOTE |
| `loop()` or the needle supervisor hangs | Task watchdog expires after 15 s | Reboots; needle memory used; if the image was on trial, the bootloader returns to the previous one. | Automatic. | NOTE |
| Portal task or the WiFi stack hangs | Nothing watches it | The radio, clock and needle keep working; the portal is unreachable. | Power cycle (a USB console still works). | DEFECT by what it actually costs (only the portal is lost); the ladder's own wording would call it BLOCKER |
| New S3 image crashes or hangs before confirmation | Reset while still on trial | Bootloader returns to the previous image, and the image line says so. | Automatic; upload a fixed image. | NOTE |
| New S3 image runs but its portal task stalls, or no network comes up | Confirmation conditions never met | Never confirmed; stays on trial; further S3 uploads refused. | Any reset returns the previous image; with the portal unreachable, a power cycle. | BLOCKER, though not USB-only |
| S3 upload sent while the image is on trial | – | Upload refused, nothing quieted, a message to retry in a minute. | Wait for confirmation, then upload again. | NOTE |
| Settings changed while the image is on trial | – | Held in RAM, written once confirmed; Save says why it is held. A power cut in the trial minute loses them. | Automatic at confirmation; after a power cut, set them again. | NOTE |
| Portal reboot while the image is on trial | – | Restarts at once without saving; the bootloader returns to the previous image. | By design — this is how a trial image is undone. | NOTE |
| New S3 image runs healthy for a minute but is wrong | Confirmed | Nothing. | Upload a good image through the portal. | DEFECT |
| Link self-test fails at boot | – | Reported; boots as usual (display, needle, link, WiFi, portal); never confirmed. | Sent over the air: a portal reboot rolls it back. Written by USB: upload a good image through the portal. | DEFECT |
| Brownout | Brownout detector | Resets; needle assumed at the low stop (a non-issue here). | Automatic. | NOTE |
| A console prompt holds `loop()` | – | No amp sense update, but PING is still sent, so the A32 does not treat it as silence. | Ends when the prompt ends. | NOTE |
| Console brightness keys pressed with the amp off | – | The clock shows the "amp on" brightness until the next retarget. | Toggle the amp, or change any setting. | NOTE |
| Hand-set clock used while the A32 is not answering | – | Sets the S3's clock only; says the battery clock was not updated. The next handshake may bring the battery clock's old time back. | Set it again once the link is back. | NOTE |

## Ambersong firmware: known problems and limits on the main board

- **About a 3.3-second boot delay without a USB host**, waiting for the console port.
- **The clock is blank for a moment after every S3 boot**, until the A32's first time message
  arrives, and stays blank if neither the battery clock nor NTP has a valid time.
- **NTP cannot reach the display while the A32 link is down.** The write-through requires the
  handshake, and nothing else re-seats the software clock from NTP.
- **The software clock is exact only between resyncs.** It counts from the last battery-clock or
  NTP set and can lag by up to about a second; the display shows minutes, so this is not visible.
- **The hand-set-clock prompt uses a hard-coded zone and leaves it in force.** After using it on a
  set configured for another zone, the display shows eastern North American time until the zone is
  saved again from the portal or the S3 restarts.
- **A hand-set clock typed while the A32 is silent sets this board only**, and says so. If the
  battery clock holds a valid time, the next handshake puts that time back on the display.
- **A settings change made in the minute an S3 image is on trial is lost if the power goes in that
  minute.** Accepted.
- **12-hour mode has no AM/PM indicator**, because the display's font has digits only.
- **The readout below 100 MHz shows a leading zero** ("0879").
- **The "always on" readout mode is not gated on the tuning encoder answering**; with a dead
  encoder the display shows a frequency from the last count.
- **The two brightness console keys bypass the fade** and write straight to the display, so the
  clock can sit at the wrong level or step back briefly before easing.
- **Portal requests can be starved while the needle moves.** Accepted.
- **With the link down, the panel treats the source as AUX**, so with the amp on it sits at the
  "other source" level.
- **The battery clock's temperature arrives in every time message and is not used.**
- **Not yet verified on this build:** a failed self-test at boot; a hang of the needle supervisor
  task; an image that runs but never earns confirmation; a failed confirmation write; a hung portal
  task; the battery-clock health messages in a real fault; the protocol-version message; a refused
  edit of an audio-board setting while the A32 is silent.
- **The rescue access point is an open network**, with no passphrase to change
  (`firmware-gospel-network.md`).

## Ambersong firmware: maintaining the main board

**Invariants that must hold:**

1. The display interrupt stays in IRAM, writes registers only, touches no flash, and calls nothing
   that can block. Keep the display setup inside `setup()` so its timer interrupt is allocated on
   core 1.
2. Never shorten the display's dead time. Keep the turn-off strictly inside the slot. Keep the
   unconditional blank at the last tick. Only one digit select may ever be set.
3. All seven segment pins must stay below GPIO 32, because the interrupt clears segments through
   the low output register only. Digit pins may be anywhere.
4. `loop()` must not block. A routine that must wait inside `loop()` has to keep sending the
   keepalive ping, or the A32 decides the amp is down. It must also return within 15 seconds, or
   feed the task watchdog while it waits, or the watchdog resets the S3. Anything the portal task
   calls must not block or write flash.
5. Core 1 below priority 2 belongs to `loop()`, which never yields; a new task at priority 0 on
   core 1 will never run. Put periodic work in `loop()`, or give the task priority 2 or more and
   make it yield. Tasks on core 0 must yield at least briefly; the step emitter alone spins.
6. No NVS writes while the needle moves, a calibration runs or an upload is active, and none while
   the image is on trial. An NVS write disables the flash cache, which stalls every piece of code
   not in IRAM — a measured effect, not a theoretical one (see "Decisions" in
   `firmware-gospel-needle.md`). A new stored field is appended, never inserted, with the settings
   version bumped and a migration case added (`firmware-gospel-settings.md`).
7. Push settings to modules only through the one settings-apply function, which also writes back
   what a module accepted, so the stored settings always hold what is running.
8. Keep the panel lamp and the needle's stepper coils on separate LEDC timers from each other; do
   not put anything else on the panel's timer at a different frequency.
9. Keep one definition of "the radio is playing" and one definition of the current frequency; do
   not rebuild either locally.
10. The battery clock stores UTC. Local time is applied only at display time, with the time-zone
    rule from the network module.
11. A pin moves in the pin table and nowhere else (`firmware-gospel.md`); what it is wired to is
    the Hardware Bible's business.
12. The toolchain is pinned (see `firmware-gospel.md`). Identify the board before flashing it over
    USB; never let the uploader pick a port by itself when several boards of the same kind are
    connected.

**Traps that have bitten this project:**

- The turn-off on a tick that never happens (the 88:88 display).
- A busy-spinning task on a core whose idle task is watched: a boot loop that looked like a power
  problem.
- The native-USB console does not reset on attach, so anything printed only at boot is never seen.
- A serial tool that toggles DTR/RTS resets the board; "uptime 1 second on every reading" was the
  reading tool resetting it.
- A word typed in the portal's console used to run as a string of one-key commands; now one key
  per send, unless a prompt wants a whole line.
- The display's lit-window variable starts one tick past its own clamp until the first brightness
  is set at boot; harmless only because the frame is blank then. Do not show digits before a
  brightness has been set.

**How to test a change, without instruments:**

- The three display test patterns: all segments, blank, normal.
- Full brightness must not show 88:88 (the regression test for the tick-rollover bug).
- Clear the slot-timing counter, then watch the worst-slot-error field under load (WiFi traffic, a
  needle sweep, a settings save).
- Toggle the amp: the clock should hold for 250 ms, then ease for 1.5 s; the panel should follow
  its table.
- Cycle the readout modes; turn the knob with the amp on and the source on RADIO to see the "while
  tuning" mode.
- Use the hand-set-clock prompt; check the status page and the portal clock.
- Read the full status page for everything at once: amp, source, needle, panel level and target,
  display brightness and slot error, time, link counters, last reset and uptime.
- After an S3 upload, the image line should read "ON TRIAL (rollback armed)"; about 70 to 90
  seconds after the restart it should read confirmed. A second S3 upload inside that time should be
  refused, and Save should answer that settings are held. A change to the watchdog or the trial
  deserves the rollback tests: a portal reboot during the trial, and a deliberately hanging image
  never committed to the repository.
