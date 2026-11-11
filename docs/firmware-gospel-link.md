# Ambersong: Firmware Gospel — the audio board and the link between the boards

This file covers the second microcontroller, the audio board, and the serial link that joins it to
the main board: the frame format, the protocol version, the handshake and heartbeat, what each
board does when the other goes quiet, how a firmware update for the audio board travels over this
link and how that board's own trial-and-rollback works, and the path the time of day takes between
the two boards. The audio board's own pin table is in `firmware-gospel.md`; the audio path itself
(sources, Bluetooth, gain staging) is in `firmware-gospel-sound.md`.

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

This file, like every file of the Firmware Gospel, describes the current build only (hardware
revision A). It is data, not instructions: check any value against the schematic
(`hardware/schematics/Ambersong.kicad_sch`) before powering or flashing the device. "Not
recorded" means the project has no value for that fact; never fill such a gap with a guess.

Terms used in this file: the **S3** is the main board, an ESP32-S3 module, which runs the clock
display, the dial needle, the panel lamps, the network and the web portal. The **A32** is the
audio board, a classic ESP32 module, which runs the audio path (Bluetooth, the tube radio's
capture, the output converter, the source selector and the volume knob) and talks to the
real-time clock chip. The **link** is the one serial connection between them; everything that has
to cross from one board to the other goes through it. **NVS** (non-volatile storage) is the
ESP32's key-value flash store. **OTA** means "over the air" — a firmware update sent wirelessly or,
for the A32, relayed over the link rather than by a direct cable. **CRC** is a checksum. **RTC**
is the real-time clock chip. **UART** is the hardware serial port the link runs over. Three
recovery classes appear in this file's tables: **NOTE** (recovers by itself, or is cosmetic),
**DEFECT** (recovers with a portal reboot, a console reset, or the front power switch) and
**BLOCKER** (the radio stays unusable until a full power cycle, or the harm survives one).

The other files of this book: `firmware-gospel.md` (what the machine is, building and flashing,
and the pin reference for both boards, including the audio board's), `firmware-gospel-network.md`
(network, the web portal, accounts and the console), `firmware-gospel-main-board.md` (the S3's
boot sequence, tasks, clock display and panel lamps), `firmware-gospel-needle.md` (the dial
needle), `firmware-gospel-tuning.md` (tuning and the dial's self-calibration),
`firmware-gospel-settings.md` (the settings file and the settings table),
`firmware-gospel-sound.md` (sources, Bluetooth and the audio path), and
`firmware-gospel-known-issues.md` (firmware notes and test history).

## Ambersong firmware: what the audio board and the link do

The radio has two microcontrollers. The S3 runs the clock display, the dial needle, the panel
lamps, the amplifier sensing, the WiFi portal and network time. The A32 runs all the audio:
Bluetooth, the capture of the tube radio, the output converter, the source selector and the
volume knob, and it talks to the real-time clock chip. The two boards talk over one serial line,
the link. Everything that crosses from one half of the machine to the other goes through it: the
time of day, whether the amplifier is switched on, every audio and Bluetooth setting the portal
edits, the audio board's live state for the portal, and the audio board's own firmware updates,
because it has no other road for an update than through the S3.

When all of this works, the listener sees nothing of it. **When the S3 stops talking, the A32
deliberately goes silent within two seconds**: no audio, Bluetooth closed and dark. By design, a
dead S3 is obvious rather than quietly tolerated. When new audio-board firmware is uploaded through
the portal, the audio fades out, the upload takes about a minute, and the board restarts on the
new image. That image runs **on trial**: if it crashes, hangs, or cannot talk to the S3, the next
reset puts the previous image back by itself. While an image is on trial, the audio board writes
no settings to its own flash: a change made from the portal still takes effect at once, but it is
only kept in memory and written once the image is confirmed. A frozen audio board restarts itself
within 15 seconds on its own.

## Ambersong firmware: who owns what, and how the other board learns it

| Thing | Owner | How the other board learns it |
|---|---|---|
| Time of day (always UTC) | the real-time clock chip on the A32; internet time on the S3 disciplines it | a TIME message (A32 to S3), a SET_TIME message (S3 to A32) |
| Time zone, and how time is displayed | the S3 | never sent on the link; everything on the link is UTC |
| Whether the amplifier is on | the S3 senses it | a SET_SYS message (S3 to A32) |
| Audio and Bluetooth settings | the A32 stores them in its own flash | the S3 keeps a read-only mirror (a CFG message) and pushes edits (SET_AUDIO, SET_BT) |
| Source, volume, Bluetooth state, meters, diagnostics | the A32 | a STATE message, sent four times a second |
| Audio-board firmware image | uploaded through the S3's portal | relayed to the A32 over the link |

There is no reset wire from the S3 to the A32, and the A32 firmware offers no wireless update path
of its own, so after its first USB flash the only way to reach the A32 from outside the cabinet is
through this link. Much of this file follows from that one fact.

## Ambersong firmware: the audio board's boot sequence

In order, and each for a reason:

1. **The task watchdog is armed first**, at 15 seconds, so that from this point on any hang —
   including a Bluetooth start that never returns — ends in a restart rather than a silent freeze.
2. **The image's trial state is read** from the bootloader's own records (plain flash, freshly
   flashed, on trial awaiting confirmation, or already confirmed). An image flashed directly over
   USB is never on trial.
3. **The USB console starts**, printing the firmware version and commit stamp; this console needs
   a cable and the cabinet is normally shut, so the lines that matter are also relayed to the S3's
   own console afterwards.
4. The source selector, the volume knob, the Bluetooth lamp and the board's own settings are set
   up.
5. **The clock chip is opened** and read once, logging locally whether it is responding and
   whether it holds a valid time, plus its own temperature reading.
6. **The protocol self-test runs**: it encodes a test message, decodes it back, checks the result
   byte for byte, then deliberately damages one bit and confirms the checksum catches it. On
   failure, **the board does not halt** — it marks the protocol as broken and carries on, so the
   link and the update receiver still come up and the firmware can still be replaced over the air.
   While broken, audio stays muted, Bluetooth stays closed, and the board says so periodically on
   its own console and to the S3.
7. The audio engine starts and settings are applied.
8. **Bluetooth** is configured, its transmit power applied, and it starts invisible (not
   discoverable) until asked to pair.
9. **The link itself starts last**, after Bluetooth, on purpose: the link is only serviced inside
   the main loop, so starting it earlier would not let a hang during the Bluetooth start be
   reached over it anyway, and recovering from such a hang is rollback's job, not the boot order's.
10. The source selector is read once and the mute applied, which establishes the rule that the
    audio board starts asleep — muted — until the S3 speaks.

## Ambersong firmware: the main loop, and settings saves

Each pass of the main loop, in order: read the link; decide awake or asleep (before anything that
depends on it); the Bluetooth lamp; a debounced settings save; any pending Bluetooth disconnect;
the source selector; giving up on a stalled update (no accepted data for 20 seconds); checking
whether a trial image is ready to confirm; the "protocol broken" announcement; the volume knob;
the Bluetooth state; the pairing button; one STATE message every 250 milliseconds, sent whether or
not the S3 has ever answered; then local diagnostics and console keys. The watchdog is fed once
per pass.

**Saving settings.** Audio settings, Bluetooth settings and the three volume-knob calibration
points are written to flash 2 seconds after the last change (so rapid edits debounce into one
write). **Only a write that actually reached flash counts as saved.** If any part of a write fails,
the settings stay marked unsaved and the debounce starts again, so the write is retried; the board
says so on its own console and to the S3, which prints it with the reason. Forcing a reboot of the
audio board forces an immediate save first, whether or not that save actually succeeds.

**Nothing is written while the image is on trial.** A change from the portal is still applied at
once and marked unsaved, but it waits in memory. Once the image is confirmed, it is written
straight away, and the board reports both "settings changed during the trial are now saved" (if
there were any) and the confirmation itself. The reason: an update might change the size of the
stored audio or Bluetooth structure, and the board only ever accepts a stored structure whose size
matches exactly. Had a trial image saved its new layout and then rolled back, the previous image
would have found a structure of the wrong size and silently run on its defaults instead. The cost
accepted: a change made during the one-minute trial is lost if the trial ends in a reset — "reboot
the audio board" included, since its own forced save writes nothing while on trial and the reboot
itself is a rollback anyway.

## Ambersong firmware: the boot report

Once per boot, at the very first handshake message from the S3, the A32 sends one line that the
S3 prints with an `[A32]` prefix, for example:

```
[A32] boot: commit <hash>, reset reason 3, setup 1066 ms, image ON TRIAL (rollback armed), watchdog on
```

| Field | Meaning |
|---|---|
| commit | the short git hash of the source the running image was built from, with a "dirty" marker if it had uncommitted tracked changes |
| reset reason | the chip's own numeric reset cause: 1 power-on, 3 software restart (an update, or "reboot the audio board"), 4 panic, 6 task watchdog, 9 brown-out |
| setup | how long the boot sequence took |
| image | the trial-state label, or "valid (confirmed)" once confirmed |
| watchdog | whether the loop task is actually subscribed to the watchdog |
| suffix | adds "- an earlier update was ROLLED BACK" when the bootloader has a rolled-back slot on record |

This line is how a rollback or a watchdog restart becomes visible with no USB cable. It is sent
once per boot; a later re-handshake (the S3 restarting, say) does not repeat it. Other lines the
audio board sends the same way, all printed as `[A32] ...`: the confirmation result, the note that
settings held during a trial were just saved, the "protocol broken" announcement, a failed
settings save, and a Bluetooth disconnect that did not land.

## Ambersong firmware: the task watchdog

A task watchdog restarts the chip when a watched task stops checking in. It runs at 15 seconds,
with a restart (not just a warning) on expiry, and the board's main loop task is subscribed to it
— so is `setup()` itself, since it runs on the same task, which is why the whole boot sequence must
also finish inside 15 seconds. 15 seconds leaves room for the Bluetooth start and for the longest
legitimate wait inside one pass of the loop, which is the final image check at the end of a
firmware update.

## Ambersong firmware: awake and asleep

Evaluated on every pass of the loop:

```
awake = amplifier reported on by the S3  AND  a valid frame has arrived from the S3 within the last 2 seconds
```

Only frames **from the S3** keep the A32 awake — nothing the A32 sends to itself and nothing from
anywhere else matters here.

**Falling asleep** (the amplifier switched off, or two seconds without a valid frame from the S3):
if the S3 is the reason, the remembered amplifier state is cleared too — the audio board must not
wake back up on the S3's last, possibly stale, "amplifier on", because the amplifier could have
been switched off while the S3 itself was restarting; it stays asleep until a fresh message says
the amplifier is on again. The phone is paused and then disconnected, the audio is muted, the
Bluetooth lamp goes dark, and the radio becomes invisible and non-connectable over Bluetooth.

**Waking** (amplifier on and the S3 talking again): any pending disconnect is cancelled, the mute
is re-evaluated, and Bluetooth state and visibility are recomputed.

**The mute is a function of state, not a one-off event**, computed in one place every time
anything could change it: muted whenever the protocol is broken, or the board is not awake, or the
stored mute setting says so. An earlier version muted only on the moment of falling asleep while
several other places could unmute, so turning the source knob while the S3 was dead brought the
sound back even though nothing should have been playing.

**Asleep is not off.** The link, the clock chip and the firmware-update receiver all stay up —
they are what the S3 needs from the A32, and the only reason for it to be awake at all. The A32
keeps sending its STATE message even while asleep.

What the listener experiences: a brief silence at power-up until the boards have handshaken and
the S3 has reported the amplifier on; a silence within 2 seconds of the S3 restarting (its own
update, a portal reboot, a crash), during which the phone is dropped and must reconnect once the
S3 is back; and, if the link's S3-to-A32 direction specifically fails, exactly the same silence for
as long as that fault lasts.

## Ambersong firmware: the link itself

The link runs as a hardware serial port at 921,600 baud, 8 data bits, no parity, one stop bit, no
flow control, on both boards. The audio board's pin table (its serial pins and every other pin it
uses) is in `firmware-gospel.md`; the wiring between the boards is recorded in `hardware-bible.md`,
not here. Every frame is checksummed.

**Three details that matter to anyone changing this code:**
- **The link's pins must always be given explicitly** when the port is opened. On the classic
  ESP32, this serial port's default pins collide with a pin this firmware uses for the audio
  output's frame clock; opening the port with no pins specified would silently take that pin, and
  the audio would fail much later with no obvious cause. The firmware makes this impossible rather
  than merely documenting it.
- **The receive buffer size must be set before the port is opened** — the underlying library
  ignores a later resize and keeps a small default, which is enough for control messages but not
  for a 1 KB update chunk at this baud rate.
- **On the S3, the link is written from two different places** — the main loop on one processor
  core, and the portal's upload-relay code on the other — but frames never interleave, because
  each frame goes out in one write call that holds the port for its whole duration.

The S3 publishes, both on its console and to the portal, counts of frames received, frames sent,
frames with a bad checksum, and frames carrying another protocol version. A healthy pair shows a
nonzero received count with the checksum-error and wrong-version counts both at zero. A wrong-
version count that climbs while nothing else does means the two boards were built with different
protocol versions (see "changing the protocol version" below) and both need flashing; the portal
shows a matching warning.

## Ambersong firmware: the frame format

Every message is wrapped the same way: a fixed two-byte start marker (not itself checksummed — it
exists only to let the receiver resynchronise after noise or a dropped byte), a protocol-version
byte, a message-type byte, a two-byte little-endian length, the payload, and a two-byte checksum
covering the version byte, the type byte, the length and the payload.

- **The protocol-version byte** is checked by the receiver before anything else; a frame carrying
  any other value is dropped at once and only counted, never passed on.
- **The payload length** is capped at 1,088 bytes — enough for the largest message, a 1,024-byte
  firmware-update chunk plus its own small header.
- **The checksum** is a 16-bit CRC, computed bit by bit rather than from a lookup table (a lookup
  table would cost cache space the display's own timing-critical interrupt needs).
- **Payloads are copied byte for byte** between fixed-layout structures on both boards, because
  both ends are the same processor architecture built by the same compiler; this is a recorded
  shortcut that would need to become explicitly byte-oriented if a third kind of device ever
  joined the link.

**Frame sizes** (8 bytes of fixed overhead plus the payload): a heartbeat or ping is 8 bytes; the
handshake message is 33 bytes; the live-state message is 138 bytes; the settings-mirror message is
41 bytes; a time message is 16 bytes; an audio-setting push is 23 bytes; a Bluetooth-setting push
is 20 bytes; an amplifier-state push is 12 bytes; an update-status message is 46 bytes; an
update-data chunk is up to 1,038 bytes. At this baud rate the line carries roughly 92 kB per
second; a full update chunk takes about 11 milliseconds to send, and the audio board's routine
state reports cost roughly 550 bytes per second of the link's capacity.

## Ambersong firmware: the frame decoder

The decoder consumes one byte at a time, so it can be fed directly as bytes arrive with no
assumption that a whole frame arrives at once. After the final checksum byte of any frame — good
or bad — the decoder always resumes hunting for the next start marker from the very next byte; **it
never rewinds to rescan the inside of a frame that just failed.** One practical consequence: a
single corrupted length byte can make the decoder swallow up well over a thousand bytes before it
starts looking for a new frame again, which at the audio board's ordinary traffic rate can mean
roughly a minute of apparent silence — the board falls asleep, then wakes again by itself once
real frames resume. This has never actually been observed on the built radio; it is a reasoned
limit of the design, accepted because recovering it automatically within a few tens of seconds was
judged good enough.

A payload is only copied into its matching structure when the frame's length is **exactly** the
size of that structure. A frame that already passed its checksum but has the wrong length for its
type means the two boards' builds disagree about that structure's shape — a version skew, not line
noise — so a mismatched length and a bad checksum must never be treated as the same kind of fault.

## Ambersong firmware: handshake, heartbeat and liveness

The S3 drives the conversation. From boot, before any handshake, the A32 already sends its STATE
message four times a second, unprompted. The S3 sends a HELLO message once a second until it gets
a HELLO_ACK back; the HELLO_ACK carries the A32's own firmware version and triggers the boot report
described above, and starts a confirmation clock on the A32 for its own trial image.

Once handshaken: the S3 sends a ping once a second (the A32's STATE messages are frequent enough
that the S3 never needs the ping's own reply to know the A32 is alive); it tells the A32 the
amplifier's state every 2 seconds and immediately on any change; it asks the real-time clock's
reading once a minute; and it pushes a corrected time to the real-time clock at most once an hour,
only while internet time is fresh.

**The S3 restarts the handshake from scratch** (clearing what it knows about the A32, including
its settings mirror) when: nothing valid has arrived from the A32 for 2 seconds; the A32's own
`millis()` counter, carried in its STATE message, goes backwards by more than a few seconds — a
sign the A32 restarted faster than the 2-second silence rule alone would have noticed; or after
every A32 firmware update, successful or not, so the version shown always matches what the A32
actually comes back running.

## Ambersong firmware: what the S3 does with the audio board's messages

- **HELLO_ACK**: checked for the right size, notes the audio board's version, marks the handshake
  done, and immediately asks for the real-time clock reading and the current settings.
- **STATE**: parsed into a mirror the portal reads from. On a length mismatch (the two builds
  disagreeing about this message's shape), the mirror is marked unknown rather than shown stale,
  and the S3 says once per boot that it believes both boards need flashing — audio keeps playing;
  only telemetry stops.
- **TIME**: re-anchors the S3's own idea of the time (see "the clock path" below).
- **CFG**: fills the settings mirror the portal reads and edits through.
- **Update status**: fills the fields the portal reads while an audio-board update is in progress.
- **LOG**: printed on the S3's console with an `[A32]` prefix.

**The settings mirror exists because the audio board owns and saves its own settings, and the
portal needs to show what is actually in the machine.** The mirror is refreshed at every
handshake, because a restarted audio board comes back with whatever is actually in its own flash.
While the mirror is not known to be current — nothing has arrived from the audio board since it
last went silent — **an edit of an audio-board setting from the portal is refused outright**,
because pushing from a stale, possibly all-zero copy would overwrite real calibration, and pushing
into a silent link would simply be lost and then overwritten once the audio board came back. The
settings-file export, described in `firmware-gospel-settings.md`, writes those rows as "not
applicable" under the same condition, and import skips them and says so.

## Ambersong firmware: an audio-board firmware update, end to end

The audio board cannot be reached by WiFi at all. An administrator uploads its firmware image to
the S3's portal, and the S3 relays it over the link, one kilobyte at a time; the S3 stores nothing
of the image itself — it is a pipe.

**What happens, and why each part is there:**

- **The audio stops first.** Writing and erasing flash stalls everything on the chip that cannot
  run from its small block of directly-executable RAM, and the audio engine is not in that
  category — it would tear rather than pause cleanly. Muting through the ordinary fade-out makes
  the update sound, to the listener, like an unremarkable change of source.
- **A fixed pause is taken after muting and before the first flash write**, long enough to cover
  the mute taking effect, the fade-out, and the output buffering fully draining — long enough that
  a flash write landing mid-drain never substitutes silence abruptly through the amplifier as an
  audible click.
- **The image is streamed, not buffered whole**, because the S3 is relaying a browser upload and
  does not know the final size or checksum until the last byte arrives; the whole-image checksum is
  only known and checked at the very end.
- **Every chunk's position in the image is absolute**, so a chunk that arrives twice is simply
  ignored and one that arrives early is refused; the acknowledgement always carries the number of
  bytes actually committed so far, so a lost chunk can be decided from the numbers alone rather
  than being treated as fatal.
- **Beginning the update is deliberately given a generous timeout** (tens of seconds), because the
  audio board mutes, pauses and only then starts its own flash-update machinery before it can
  answer at all; a short timeout here failed consistently on this radio during development.
- **The final "end of image" message must be answered**, and the answer is checked for a limited
  time rather than assumed: the audio board commits the image to its boot slot *before* it answers,
  so a lost final answer could otherwise hide a perfectly good update behind a reported failure.
  If no answer arrives, the S3 re-handshakes so the next line in the console shows the truth,
  whatever it turns out to be.
- **A wrong-chip image is refused before anything is written**, by checking the image's own chip-
  id bytes on its very first chunk — the firmware-upload picker for each board sits right next to
  the other on the page and the two files look alike. A file that is not a firmware image at all
  passes that specific check but is still refused by the underlying update mechanism.
- **Both sides give up on silence**: the S3 gives up and tells the audio board to abandon the
  update after 15 seconds with no upload activity (because the underlying web server does not
  always reliably report an aborted browser upload); the audio board gives up on its own after 20
  seconds with no accepted data.
- **A failed update restores whatever mute state the user actually had before the update began**,
  never simply "unmuted".
- **The S3's needle and display are left running normally during an audio-board relay**, because
  the S3 itself is not the board writing flash.

**Measured speed on the built radio:** a 1,205,728-byte image relayed in about 76 seconds, roughly
16 kB per second — well below the link's raw capacity, because each 1 KB chunk waits for its own
acknowledgement, and each acknowledgement waits for a flash write to finish.

## Ambersong firmware: the rollback life cycle

**Rollback** means: a freshly updated image runs on trial, and unless it confirms itself, the
next reset returns to the previous image automatically. The underlying framework supports this at
the bootloader level, but by default marks every new image valid before its own `setup()` even
runs — so until this firmware explicitly overrode that default, every relayed audio-board image
was trusted blindly, and one that crashed or hung at boot needed a USB cable to fix, with no way to
reach it over the link at all.

This firmware tells the framework instead that **it will confirm itself**, and then actually does
so only once the new image has done real, meaningful work:

- **A trial image is confirmed about a minute after the first handshake with the S3** — not at the
  handshake itself, because the handshake happens before Bluetooth has opened or anything has
  played, and confirming that early would have called a build "good" before it had done anything.
- **A fallback confirms an image after about five minutes of complete silence from the S3**,
  provided nothing valid and nothing merely damaged ever arrived at all (not even a frame with a
  mismatched protocol version — only the total absence of traffic counts here). This exists so a
  genuinely good image is never rolled back purely because the S3 happened to be absent, and it is
  the mechanism that makes a staged protocol-version upgrade possible (see "changing the protocol
  version" below).
- **An image whose own protocol self-test failed is never confirmed**, for as long as it keeps
  running; the next reset — for any reason — takes the previous image back.
- **No new update is accepted while the running image is itself still on trial**, because writing
  one would overwrite the one flash slot a rollback needs to fall back to, leaving nothing to fall
  back to if the new attempt also failed.
- **Nothing is written to the audio board's own settings while its image is on trial** (see "the
  main loop, and settings saves" above); what was held in memory is written the moment the image
  is confirmed.
- A failed confirmation attempt (the framework's own call to mark the image valid reporting an
  error) is simply retried a few seconds later.

The S3 has had the identical treatment for its own updates since the firmware's first release:
trial and rollback, no further S3 update while its own image is on trial, a degraded run instead
of a halt on a failed self-test, and its own task watchdog — covered in `firmware-gospel-main-board.md`.

## Ambersong firmware: the clock path

The real-time clock is a chip wired to the A32 (see `firmware-gospel.md` for its pins) with its own
backup battery, so it keeps time through a power cut. The clock chip sits on the audio board and
the clock display sits on the main board, so the time of day has to cross the link to reach the
display.

**Everything on the link, and everything inside the chip, is UTC.** The S3 applies the local time
zone, because it is the board with internet access and the portal; keeping local time in the chip
itself would make the hour after a daylight-saving change ambiguous and the hour before it happen
twice.

**Reading the clock** retrieves the time registers, a status register, and the temperature. One
bit of that status register is the **oscillator-stopped flag**: the chip sets it when its
oscillator has actually stopped, for example after losing both main power and its backup battery.
**The time is only treated as valid when that flag is clear** (and the chip actually answered, and
the resulting date is at least plausible). This matters because a chip whose oscillator stopped
keeps counting from wherever it was, so after a real power loss it reports a date that is entirely
plausible and entirely wrong — plausibility alone cannot catch that; the flag can, and the flag
stays set until something explicitly writes a new time into the chip, which is exactly the point:
a clock that stopped and was never reset since genuinely is wrong, and should say so rather than
quietly agree with whatever reads it.

**Writing the clock** always clears that flag, which is what makes the clock trustworthy again
after the next correct time is written into it, whether that time came from the internet or was
typed by hand at the console.

**The S3's own software clock** is kept as an offset from its own running-time counter, so a slow
or missing link never makes the displayed clock stutter — it simply stops being corrected. It is
re-anchored from the audio board's real-time clock at every handshake and then once a minute, and
separately from internet time, at most once an hour, whenever internet time is fresh and the link
is up (which also pushes the corrected time back into the real-time clock, so the correction
survives the next power cut on its own, with the chip as the holdover whenever the network is
absent). Setting the clock from the console sets the S3's own clock immediately, and forwards it
to the audio board's real-time clock only while that board is actually answering; otherwise the
console says plainly that the battery-backed clock was not updated, so it can be set again once
the link is back.

**The battery clock's health reaches the portal** as one of three states: fine (or not asked yet);
the chip answered but with no valid time (oscillator-stopped flag set, or its status register did
not answer) — shown as a warning to check the clock chip's battery; or the chip does not answer at
all to repeated requests — shown as a separate warning. The S3 decides "not answering" purely from
silence, because the audio board itself sends nothing at all when its own read of the chip fails
outright.

## Ambersong firmware: settings and constants (reference)

None of the following are portal settings; all are compiled into the firmware, and changing any
value shared between the two boards means rebuilding **both**.

| Name | Default | Unit | Meaning |
|---|---|---|---|
| Protocol version | 4 | — | the version byte in every frame; see "changing the protocol version" before touching it |
| Start marker | fixed two-byte value | — | frame resynchronisation marker, not checksummed |
| Largest payload | 1,088 | bytes | caps every message; checked at compile time against every message structure |
| Link speed | 921,600 | baud | 8 data bits, no parity, one stop bit, no flow control |
| Audio-board state report period | 250 | ms | sent unprompted from boot |
| "Still alive" window | 2,000 | ms | both boards |
| Handshake / heartbeat period | 1,000 | ms | until handshaken, then as a heartbeat |
| Amplifier-state push period | 2,000 | ms | also sent at once on a change |
| Clock re-anchor period | 60,000 | ms | from the real-time clock, while handshaken |
| Clock-chip answer timeout | 5,000 | ms | while handshaken; past this, the battery clock is shown as "not answering" |
| Corrected-time push to the real-time clock | at most once per 3,600,000 | ms | only while handshaken and internet time is fresh |
| Update-begin answer timeout | 20,000 | ms | the S3 waiting for the audio board to accept an update |
| Update-chunk answer timeout | 3,000, four tries | ms | per chunk |
| Update-end answer timeout | 4,000 | ms | the final "did it work" exchange |
| Upload inactivity watchdog | 15,000 | ms | the S3 gives up and tells the audio board to abandon the update |
| Update chunk size | 1,024 | bytes | |
| Pause before the first flash write | 400 | ms | must exceed the mute, fade-out and output-buffer drain time together |
| Audio-board update give-up | 20,000 | ms | since the last accepted chunk |
| Restart delay after a successful audio-board update | 250 | ms | lets the final status message leave first |
| Task watchdog timeout | 15 | s | must exceed the boot sequence and the longest single loop pass |
| Trial confirmation, after the first handshake | 60,000 | ms | |
| Trial confirmation fallback, with nothing ever received | 300,000 | ms | |
| Failed-confirmation retry | 10,000 | ms | |
| Audio-board settings save debounce | 2,000 | ms | after the last change; skipped by a forced save |
| "Protocol broken" announcement period | 10,000 | ms | console and to the S3 |
| Real-time clock plausibility floor | a fixed date in September 2020 | — | both boards; a time before this is never treated as valid |

## Ambersong firmware: message types (reference)

Each message has a one-byte type id, grouped by number range: handshake and heartbeat; S3-to-audio
control; audio-to-S3 reporting; firmware-update relay; and S3-to-audio diagnostics. Retired ids are
never reused, and every handler silently ignores an id it does not recognise, so one board built
with extra message types never upsets an older peer.

| Direction | Message | Payload size | Sent when | What the receiver does |
|---|---|---|---|---|
| S3 to A32 | HELLO | 25 bytes | once a second until handshaken | A32 notes the S3's version, answers HELLO_ACK, sends its boot report once, starts its own confirmation clock once |
| A32 to S3 | HELLO_ACK | 25 bytes | answer to HELLO | S3 marks the handshake done, asks for the clock reading and the settings |
| S3 to A32 | PING | empty | once a second while handshaken | A32 answers PONG |
| A32 to S3 | PONG | empty | answer to PING | ignored (STATE already proves liveness) |
| S3 to A32 | SET_AUDIO | 15 bytes | a portal edit of an audio setting, or a settings-file import | applied; saved after the debounce (held if on trial) |
| S3 to A32 | SET_BT | 12 bytes | a portal edit of a Bluetooth setting, or import | applied; saved after the debounce (held if on trial) |
| S3 to A32 | SET_SYS | 4 bytes | every 2 s while handshaken, and at once on a change | amplifier state updated, visibility recomputed |
| S3 to A32 | SET_TIME | 8 bytes | fresh internet time (hourly at most); console clock prompt, while handshaken | clock written, oscillator-stopped flag cleared |
| S3 to A32 | GET_TIME | empty | at handshake, then every 60 s | clock read and answered with TIME (nothing sent if the chip itself is silent) |
| S3 to A32 | BT_FORGET | empty | portal "forget every pairing" | every Bluetooth pairing removed |
| S3 to A32 | BT_LOOK | empty | portal "pair" | if the source is Bluetooth: drops the phone and opens pairing |
| S3 to A32 | REBOOT | empty | portal "reboot the audio board" | saves pending settings at once (nothing while on trial), then restarts |
| S3 to A32 | CAL_POT | 1 byte | portal volume-knob calibration actions | records that calibration point |
| S3 to A32 | BT_CMD | 1 byte | portal Bluetooth transport actions | sends the matching command to the phone |
| S3 to A32 | GET_CFG | empty | at handshake; portal "re-read the audio board" | answers CFG |
| A32 to S3 | STATE | 130 bytes | every 250 ms, unprompted, from boot | fills the S3's mirror; restart detection |
| A32 to S3 | TIME | 8 bytes | answer to GET_TIME | re-anchors the S3's clock if valid |
| A32 to S3 | LOG | text | boot report; confirmation result; a settings save that failed; other board-side notices | printed on the S3's console with an `[A32]` prefix |
| A32 to S3 | CFG | 33 bytes | answer to GET_CFG | fills the settings mirror |
| S3 to A32 | OTA_BEGIN | 32 bytes | starting an audio-board update | refused while on trial; otherwise mutes and starts receiving |
| S3 to A32 | OTA_DATA | up to 1,030 bytes | each update chunk | committed if the position matches; always answered |
| S3 to A32 | OTA_END | 4 bytes | end of the update image | checksum compared, image finalised, board restarts |
| S3 to A32 | OTA_ABORT | empty | wrong chip; unanswered end; upload stalled; browser upload aborted | update abandoned if one is running; user's mute restored |
| A32 to S3 | OTA_STATUS | 38 bytes | after every begin, chunk, end and give-up | fills the fields the portal reads during an update |

The diagnostic-only message types (S3-to-A32, used for bench experiments on the audio path) are
described in `firmware-gospel-sound.md`.

## Ambersong firmware: versions on the link

There are two different version numbers in play. **The protocol version** is the wire format
itself; it travels as one byte of every single frame, and a frame carrying any other value never
gets past the decoder at all — this is a hard wall, not a warning. **The firmware version** names
the build (see `firmware-gospel-network.md` for its exact form); it travels only inside the
handshake messages, so each board's console can say which build the other one is actually running.
A build's commit stamp is shown next to its version on each console but is never itself sent over
the link, because the handshake's version field has a fixed size and widening it would itself be a
protocol change.

## Ambersong firmware: known problems — the audio board and the link

Recovery classes used below: **NOTE** (recovers by itself, or is cosmetic), **DEFECT** (recovers
with a portal reboot, a console reset, or the front power switch), **BLOCKER** (the radio stays
unusable until a full power cycle, or the harm survives one). The audio board is powered whenever
the set is plugged in, ahead of the front switch, so a "power cycle" of it means unplugging the
set.

| What fails | What happens | How to recover | Class |
|---|---|---|---|
| The audio board's loop or boot sequence freezes, image already confirmed | A watchdog restart after 15 s; the boot report shows the reason | Nothing, if it does not recur. A hang at every boot before the link comes up needs a USB cable | NOTE; BLOCKER if it recurs every boot |
| A new image crashes or hangs at boot | The previous image comes back automatically, marked as a rollback in its own boot report | None needed | NOTE |
| A new image boots, hears frames, but never completes a handshake | Stays on trial indefinitely; the radio is silent | Unplug the set — the reset rolls it back | BLOCKER |
| A new image fails its own protocol self-test | Muted, Bluetooth closed, link and update receiver still up; never confirmed, so a further update is refused while it runs | If it was relayed over the air: "reboot the audio board" from the portal, or unplug the set, either rolls it back. If it was flashed directly by USB (not on trial): upload a good image through the portal | DEFECT if the portal reboot still reaches it; BLOCKER (unplug) otherwise |
| Upload file is for the wrong board | Refused before anything is written | Upload the right file | NOTE |
| Upload file is not a firmware image at all | Refused by the update mechanism itself | Upload the right file | NOTE |
| Damage in transit (a bad checksum, or a mismatched image checksum at the end) | The affected chunk is resent, or the update gives up with the reason given | Retry the update | NOTE |
| The final "did it work" answer is lost | The S3 abandons that attempt and re-handshakes, telling you to check the version once reconnected | Read the running version at the next handshake | NOTE |
| A second update attempted inside the trial minute | Refused outright, before anything is muted, with "still on trial" | Wait a minute | NOTE |
| Upload stops mid-stream (browser, WiFi, or the S3 itself restarting) | Both sides give up on their own timeouts; the audio board restores the user's own mute; the previous image is untouched | Retry the update | NOTE |
| Confirming a trial image fails | Retried every 10 s; further updates refused meanwhile | None, if a retry succeeds | NOTE |
| A reset during the trial minute of an otherwise good image | Rolls the good image back | Upload it again | NOTE |
| One corrupted frame | Dropped and counted; no other effect | None | NOTE |
| A corrupted length byte | The decoder may stay "deaf" for up to roughly a minute of the audio board's own traffic before resynchronising | None — it recovers by itself | NOTE |
| The S3 goes silent (its own restart, update or crash) | The audio board falls asleep within 2 s: amplifier state forgotten, phone paused and dropped, muted, Bluetooth dark; it keeps sending STATE throughout | Automatic once the S3 is back and reports the amplifier on | NOTE (by design) |
| Only the S3-to-A32 wiring direction fails | The S3 sees no frames for 2 s and drops the handshake and its settings mirror; the audio board, still hearing the S3's own messages, stays awake on the last amplifier state it was told and keeps playing | Automatic once the wiring works again; a wiring fault is a hardware matter | NOTE; see `hardware-bible.md` for the wiring |
| The audio board goes silent, or restarts very quickly | The S3 warns, drops the handshake and its settings mirror, keeps its own clock running from its last anchor, and resumes sending HELLO once a second | Automatic | NOTE |
| The two boards' STATE message lengths differ (a build mismatch) | The S3 shows the audio-board fields as unknown and warns once that both boards need flashing; audio itself keeps working, only telemetry stops | Flash the older board | NOTE |
| The two boards run different protocol versions | No handshake at all; the audio board stays asleep (silent radio); an update relay is refused outright. Visible on the console as frames received dropping to zero while the wrong-version count climbs, and as a portal warning naming both boards | Update the S3 to match the audio board's protocol version, following the staged order in "changing the protocol version" below; if the audio board is still on trial, unplugging the set also works, because it rolls back | BLOCKER (survives a power cycle once the audio board is confirmed) |
| The clock chip's oscillator has stopped | The S3 does not re-anchor its clock from it and shows the "lost its time" warning | Automatic on the next fresh internet time; otherwise set the time by hand at the console. If this recurs after every power cut, check the clock module's own battery | NOTE with a network; DEFECT without one |
| The clock chip does not answer at all | The S3 shows the "not answering" warning; it still takes internet time normally | Hardware fault — see `hardware-bible.md` | — |
| A portal edit of an audio-board setting is sent while the S3 has no current mirror of it | Refused outright, with the reason given | Wait for the next handshake | NOTE |
| An audio-board settings save fails | Stays marked unsaved and is retried a few seconds later; reported on both consoles | None, if a retry lands; a change is lost only if the board restarts before one does | NOTE |
| Settings changed during the audio board's trial, then the trial ends in a reset | Those changes are lost; the previous image finds its own stored settings exactly as it left them | Set them again once an image is confirmed | NOTE (by design) |

## Ambersong firmware: changing the protocol version

Changing the protocol-version number is a hard wall: two boards holding different values cannot
exchange a single frame between them, including the update-relay frames the audio board depends on
to be updated at all. The audio board can only be updated through the S3, and the S3 can only be
updated over its own WiFi, which does not use this link at all. So there is exactly one order that
works without a USB cable, starting from both boards on the same, older version and new firmware
for both already built on the new version:

1. **Upload the new audio-board image through the S3's portal first.** The relay itself still runs
   entirely in the old protocol version, because at this point both boards are still on it; the
   relayed image's own content is irrelevant to the relay.
2. **The audio board restarts on the new version, on trial.** It now drops every frame coming from
   the still-old S3 at the decoder, and the S3 sees nothing valid either: no handshake, the audio
   board effectively asleep (so the radio is silent), and the wrong-version counters climbing on
   both sides. This is expected at this point, and the portal's "boards run different protocol
   versions" warning is correct, not a sign anything has gone wrong — carry on.
3. **Wait at least five minutes after the audio board restarted.** With nothing valid and nothing
   even damaged ever received in that time, the fallback trial-confirmation rule confirms the new
   image permanently, even though the S3 never spoke to it in the new version.
4. **Only then upload the new S3 image, over WiFi, as an ordinary firmware update.**
5. **The S3 restarts on the new version and handshakes.** The audio board answers, and its boot
   report (the first it sends this boot) should read as already confirmed, followed by the normal
   handshake line naming its version.

**Why the order matters, and why it cannot be done the other way round.** An S3 already on the new
version cannot reach an audio board still on the old one at all — the audio board simply drops
every frame from it, the handshake never happens, and an update relay refuses outright with "the
audio board is not answering". If that happens, put the S3 back on the old version over WiFi and
start the procedure again from the top.

**The risk in skipping the five-minute wait.** If the S3 is updated before the audio board's new
image has had five full minutes of silence, the audio board instead confirms about a minute after
the new S3's first handshake with it — which also works, but carries a real risk: if the audio
board resets for any other reason before that minute is up, it rolls back to the old version while
the S3 is already on the new one, which is the mismatch the other way round, and the relay then
cannot reach the audio board at all. Recovering from that means putting the S3 back on the old
version over WiFi and starting over. Waiting the full five minutes before touching the S3 removes
that risk entirely.

Since the S3's own firmware updates also run on trial, two further things follow for this
procedure: an S3 update sent while the S3 itself is still on trial from an earlier update is
refused, so step 4 must wait until the S3 has confirmed whatever it is currently running; and after
step 5, nothing should restart the S3 again until its own update has confirmed itself, or it too
would roll back — leaving the S3 on the old version while the audio board is already on the new
one.

Before changing the protocol version at all, consider whether the same change could instead be
made by appending new fields to an existing message (a length mismatch costs only telemetry, never
the handshake) or by adding an entirely new message type (an older peer simply ignores an id it
does not recognise) — either keeps a half-updated machine playing audio throughout, which changing
the protocol version itself does not.
