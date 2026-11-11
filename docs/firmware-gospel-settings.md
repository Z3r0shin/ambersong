# Ambersong: Firmware Gospel — settings

This file covers every adjustable value the firmware keeps across a power cut: how the two boards
store, read, bound, write, mirror and migrate their settings; the settings file that backs them up
and restores them; the complete reference of every setting with its default, range, unit, meaning
and where it is saved; and how to add a new setting safely.

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
needle, panel lights, the tuning receiver, WiFi and the web portal. It owns every setting about
the display, the lights and the needle. **A32** is the audio board (a classic ESP32): the digital
audio converters, Bluetooth, and the volume knob. It owns every setting about sound and Bluetooth.
The **Leditron** is the four-digit LED-filament clock display on the front panel. **NVS**
(Non-Volatile Storage) is the key-value store, backed by flash, that the ESP-IDF framework
provides on both boards; the Arduino `Preferences` class wraps it, grouping keys under a
**namespace** and storing a number, a string, or a **blob** (a byte array). A **struct** is a
fixed-layout block of fields in memory; both boards keep most of their settings as one struct each,
saved as one blob. The **portal** is the S3's web interface; the **console** is its (and the
A32's) serial command line, reachable over USB and, for the S3, also from the portal's own console
tab.

This is one file of the Firmware Gospel. The others: `firmware-gospel.md` (what the firmware
does, the pins it uses, building, flashing, updating and recovery); `firmware-gospel-main-board.md`
(the S3's boot sequence, its tasks, the clock display, the panel lamps, and time-keeping);
`firmware-gospel-needle.md` (the dial needle: its motion, homing and calibration);
`firmware-gospel-tuning.md` (tuning and the dial's self-calibration against the tube set's own
local oscillator); `firmware-gospel-network.md` (the WiFi network, the web portal and the
console); `firmware-gospel-link.md` (the A32 and the link between the two boards);
`firmware-gospel-sound.md` (sound sources, Bluetooth, and the audio pop investigation);
`firmware-gospel-known-issues.md` (the test tools, and every known issue small enough to live
with).

---

## Ambersong firmware: what the settings system does

A **setting** is any adjustable value that must survive a power cut: the volume law, the gains,
the clock and panel brightness, the needle's motion profile, its soft limits and calibrations, the
tuning curve, the Bluetooth policy, the LED levels. The machine has two microcontrollers and each
one keeps its own settings. The S3 owns everything about the display, the lights and the needle.
The A32 owns everything about sound and Bluetooth.

A person meets settings in four places. The **web portal** shows every value as a slider, box,
checkbox or drop-down on five tabs (Audio, Display, Lights, Needle, Bluetooth); a guest may move
only volume, mute and the five brightness levels, while an administrator may move everything. A
change applies at once and is written to flash about two seconds later, once the needle is
standing still. Just after a firmware update of either board, the board that was updated writes
nothing until its new firmware has confirmed itself, roughly a minute later; any change made in
that window waits in memory until then. The portal's System tab can **download** every setting as
one plain-text file (`ambersong.txt`) and **upload** such a file back; this file is the backup and
the only undo available. The **USB consoles** of both boards (bench use only — the cabinet is shut
in normal use) change a few values by single keys, and the **front volume knob** sets the A32's
volume directly.

## Ambersong firmware: one table describes every setting

Every portal-visible setting is one row of a single table in the firmware's source, declared with
a shared row type. Each row carries:

| field | meaning |
|---|---|
| key | The stable machine name. The settings file and the API both use it. It must never change. |
| label, unit | What a person reads on the portal. |
| tab | Which portal tab the row belongs to (Audio, Display, Lights, Needle, Bluetooth; the System tab has no rows of its own). Row order is both screen order and file order. |
| store | How the bytes are stored: unsigned or signed 8-, 16- or 32-bit integer, or a 32-bit float. It must match the C type of the field it points at; nothing in the firmware checks this. |
| kind | How it is presented: a plain number, a boolean, a floating-point number, or a choice from a fixed list (the choices are one comma-separated string; the stored value is the index into it). |
| lo, hi, step | The slider's range **and the hard clamp applied to the stored value**, in the units shown to a person. |
| scale | Stored value × scale = shown value. Gains are stored as tenths of a dB, the volume law as its exponent × 10, the intermediate frequency in 50 kHz units — all so that the value travels as a plain integer, safely, across the link between the two boards, while the person still reads, for example, "+8.0 dB". |
| ptr | The address of the live field, inside the S3's own settings structure or inside the S3's copy of the A32's settings. |
| owner | Which board applies it: the S3 itself, the A32's audio settings, or the A32's Bluetooth settings. |
| admin | Whether only an administrator may change it. |

From this one table the firmware generates the schema the browser renders, the current values
shown on the portal, the bounds applied when a value is set, and both directions of the settings
file. The portal page itself knows nothing about any particular setting; the browser caches the
schema against a tag built from the firmware version and the administrator/guest role, so it is
fetched again only after a firmware change or a change of role.

There are 58 rows in total: 38 owned by the S3, 10 by the A32's audio side, 10 by the A32's
Bluetooth side. A few S3 values are not rows, because they are not "one scalar with one range": the
tuning samples, the fixed-feature list, and the pre-calibration fallback line (see
`firmware-gospel-tuning.md`). These are written into the settings file by hand, as described below.
The learned WiFi transmit power is neither a row nor in the file at all: it is kept in the settings
structure and changed only from the S3 console (see `firmware-gospel-network.md`).

## Ambersong firmware: reading, bounding and writing one value

- **Reading a value** copies the stored bytes with a byte-for-byte copy, converts to a floating
  point number, and multiplies by the row's scale. The firmware never simply casts and dereferences
  the pointer, because the A32's settings travel in a packed structure (no padding, byte alignment)
  and some of its 16-bit fields sit at odd byte addresses.
- **Bounding a value** is the only place any value is clamped; both the portal and the settings
  file go through it. It refuses a value that is not a finite number (a "not a number" value
  passes every ordinary comparison, and the text "nan" parses as one), forces a boolean to 0 or 1,
  clamps a choice to its first or last option, and clamps everything else to its row's range. It
  clamps a value; it never refuses an out-of-range number outright.
- **Setting a value**: bound it; refuse an A32 row outright if the A32 has never answered since
  boot (see "How the S3 mirrors the A32's settings" below); convert to the stored unit; compare with
  the old stored bytes and write only if different. If the row is one of the tuning calibration's
  two tuner-end fields, setting it also marks that end as measured (typing a tuner end counts as
  measuring it). The change is then applied: an S3 row re-applies every S3 setting at once; an A32
  audio row sends the whole audio structure to the A32; an A32 Bluetooth row sends the whole
  Bluetooth structure. Nothing in this step writes to flash by itself.
- **The portal's "set" endpoint** requires login; answers "not found" for an unknown key; refuses a
  guest who touches an administrator-only row; refuses an A32 row, with the message "the audio
  board is not answering - its settings cannot be changed now", while the S3 holds no copy of the
  A32's settings; requires the submitted value to be wholly a number (blanks trimmed, one decimal
  comma read as the point, anything else refused). It then sets the value, reads it back, and
  answers with the value actually stored, flagged as "clamped" if it differs from what was asked by
  more than 0.0005, plus a warning carrying the tuning curve's state whenever a tuner-end field was
  the one changed. The portal page then snaps its widget to the value actually returned.
- **Applying settings** pushes every S3 value into the module that uses it: display brightness
  target, panel levels and timings, needle speeds, its second-order response, its envelope, its
  dwell times, its microstepping, its measuring speed, its soft limits, its index band, the tuning
  calibration (refitting the tuning curve), the dial face, the receiver's fixed-feature list, and
  the WiFi transmit power. It then **writes back** the soft limits that the needle module actually
  accepted, so the stored settings always equal what the needle is actually using. This step runs
  dozens of times in a single session, from both of the S3's processor cores, so it must stay cheap
  and change nothing if run twice in a row with no new input.

## Ambersong firmware: where settings live

Both boards keep settings in NVS. Neither board keeps settings in a filesystem. Both builds use
stock Arduino partition tables, and NVS sits at the same flash address on both, so a change to the
partition table does not disturb stored settings.

| board | namespace | key | holds | part of the settings system? |
|---|---|---|---|---|
| S3 | `amb3` | `cfg` | The whole S3 settings structure, 220 bytes, as one blob. | Yes. |
| S3 | `net` | `ssid`, `pass`, `ntp`, `tz` | The house WiFi network name and passphrase, the NTP server, the time-zone string. | No. Changed from the portal's Network card; the S3 console's network key sets the network name and passphrase only. Never in the settings file. |
| S3 | `auth` | `users` | The portal accounts: names, salted and hashed passwords, administrator flags. | No. Changed from the portal's account cards. Never in the settings file. |
| A32 | `amb` | `audio` | The audio settings structure (15 bytes). | Yes. |
| A32 | `amb` | `bt` | The Bluetooth settings structure (12 bytes). | Yes. |
| A32 | `amb` | `potMin`, `potMid`, `potMax` | The volume knob's own calibration, one unsigned 16-bit value each. | Yes, but not rows. |

The needle's remembered position after a software restart lives in a part of memory that survives
a reset but not a power cut; it is not a setting (see `firmware-gospel-needle.md`). The compiled
WiFi defaults include a specific time zone for the build this book describes — change it for your
own build (see `firmware-gospel-network.md`).

## Ambersong firmware: the S3's settings structure: versioned and append-only

The S3's settings structure begins with a header — a fixed magic number and a version number —
followed by every field, each with its own default as a compile-time initializer. The rules
governing it are absolute:

- Never reorder or remove a field. Append only.
- Every append bumps the settings version and adds a migration branch.
- An older stored blob is copied into the current structure so its values survive, and the new
  fields keep their defaults.

**Loading settings, in order:**

1. No stored blob at all: defaults are used, and are written to flash on the first change.
2. Read the smaller of the stored length and the current structure's size into a zeroed buffer.
   The stored length decides only how much may be read — never which version the blob actually is.
3. Fewer than 100 bytes (the size of the very first layout): a warning is logged, and defaults are
   used.
4. The magic number is missing: the blob is the original layout, from before the header existed
   (its first two bytes were the two brightness values). It is copied field by field.
5. The version matches the current one exactly: a straight copy, and "settings loaded" is logged.
6. An older version, but one the firmware still recognises: a **prefix copy**, up to the byte
   offset of the first field that version did not yet have. This is deliberately not simply the
   stored blob's length, because a later version's new field can land inside an older structure's
   own trailing padding.
7. The firmware's own magic number, but any other, unrecognised version: this firmware is *older*
   than the one that saved the blob. Defaults are used in memory, and the **downgrade lock** is set
   (see below).

Every migration also increments the internal "something changed" counter, so the migrated blob is
written back in the current format about two seconds later, once the needle is standing still.
Each migration logs one line describing what its new fields start as.

**Version history.**

| version | what it added |
|---|---|
| V1 | The original layout, with no header at all. 38 of its 39 fields map straight across to later versions; the one that does not (a search speed for the index sensor) was a deleted setting that never actually mattered, and is dropped on migration. |
| V2 | The magic-number and version header; the layout that first shipped with the web portal. |
| V3 | The readout hold time, placed inside V2's own trailing padding (still 100 bytes). |
| V4 | The printed dial's low and high stops — the first append that actually grew the structure. |
| V5 | The hand offset, the sample mask and table, and which tuner ends have been measured. A V4 blob whose tuner-end values differ from their defaults is taken as proof that both ends were measured by hand, and is given credit for both. |
| V6 | The learned WiFi transmit power, and the fixed-feature mask and list. |
| V7 (current) | The tube set's intermediate frequency, defaulting to 212 (10.60 MHz). |

The move from V6 to V7 changes behaviour on the very first boot after the update: every computed
station frequency moves down by 0.1 MHz, because the firmware had assumed a 10.70 MHz
intermediate frequency and now uses the tube set's actually fitted 10.60 MHz (see the Hardware
Bible and `firmware-gospel-tuning.md`).

**After loading, two one-time purges run at boot:**

- A stored soft-limit pair that does not actually bracket the needle's index position cannot be
  real; it is replaced by a provisional pair (±300 half-steps) and saved, and the console asks for
  a re-home, a new band calibration, and new stops. The index-band values themselves are kept, since
  they are relative to the index and unaffected.
- A fixed-feature count above 16, or any listed feature outside the receiver's tuning window,
  empties the whole list and saves the empty list — a truncated list is not treated as a list at
  all.

## Ambersong firmware: boot order for settings

**S3:** the task watchdog and the update-image check run first of all (see `firmware-gospel.md`);
then settings load (with any migration); then the two boot purges; then a self-test of the link to
the A32 (its failure no longer halts the boot — the boot continues, but the new image is then never
confirmed); then the display, the panel, the needle and the receiver all start; the needle's
calibration is set from the tuner ends *before* the saved shaft angle is used to seed it, so the
saved angle can pick the correct turn; settings are then applied once; the link to the A32 begins
(its own mirror arrives later, after the handshake); settings are applied a second time, after the
display has come up at its standby brightness; then homing, the network, and the web portal start.

**A32:** before anything else, it asks the bootloader whether the image now running is still on
trial after an update, and keeps the answer for later. Settings then load: defaults first; the
volume knob's three calibration points; then the audio and Bluetooth structures, each read only if
its stored length exactly matches the current structure's size (there is no migration on this
board at all); then mute is forced off regardless of what was stored. The loaded values are then
pushed into the audio engine and the LED driver. The very first reading of the volume knob, in the
main loop, then sets the actual volume.

## Ambersong firmware: the S3's write path

- **"Something changed"** is recorded as a timestamp, together with an atomic counter that is
  incremented on every change. Every writer calls this *after* making its change.
- **"Last saved"** is a second counter, set to the value the last successful write actually copied.
- **"Unsaved"** is true whenever the two counters differ. This is deliberately derived from the two
  counters rather than kept as its own flag, so that a change landing on either processor core
  during a write is never marked as saved by that write.
- **Writes are safe** only when the needle's velocity is exactly zero, no needle calibration is
  running, and no S3 firmware update is in progress. A flash write disables the flash cache
  entirely, and everything not already running from fast internal memory stops — the needle's own
  step generator included. Measured during a settings write without this guard: about 383
  microseconds of step jitter and a display timing error of about 1.3 milliseconds, against about
  49 and 8 microseconds with the guard in place.
- **The actual write** is the only function that ever writes the S3's settings blob. It: refuses
  outright if the downgrade lock is set; refuses silently while this S3 image is still on trial
  after an update (nothing is written until the image confirms itself — see below); allows only one
  writer at a time, since the portal's own reboot path and the main loop run on different cores and
  could otherwise both try to open the same storage handle; reads the "something changed" counter
  *before* copying any data; copies the whole settings structure into a fixed-size snapshot; opens
  the storage namespace and writes the blob, checking both results; on failure, stays marked dirty,
  restarts its two-second wait so the write is retried, and logs a warning that the flash write
  failed; on success, updates "last saved" to the counter value read before the copy, and logs that
  settings were saved.
- **Nothing is saved while a newly updated S3 image is on trial.** An image written over the air
  boots on trial and is rolled back by the next reset unless it confirms itself first, roughly 70
  seconds after boot (see `firmware-gospel.md`). During that time every change — a migration
  included — stays only in memory, still counted as unsaved. Once the image confirms itself, the
  ordinary two-second debounce writes it on the very next safe pass of the main loop. This means a
  rolled-back image never encounters a blob written by the image that replaced it; the cost is that
  a change made during that trial minute is lost if power is lost during it.
- **The two-second debounce** runs on every pass of the main loop: if something is unsaved, and at
  least 2000 ms have passed since the last change, and it is currently safe to write, the actual
  write runs. The two-second delay exists because flash traffic is the one load that visibly
  disturbs the display, and a slider dragged across its whole range would otherwise trigger a write
  on every single step.
- **A forced, immediate flush** ("Save settings now", also used internally): returns true at once
  if nothing is unsaved; returns false if the downgrade lock is set, if the image is still on
  trial, or if it is not currently safe to write; otherwise it writes repeatedly, up to 40 attempts
  25 ms apart if the other core is mid-write, until nothing is left unsaved, a real flash failure
  occurs, or the attempts run out. Every caller of this function treats "true" as meaning
  "nothing is unsaved" — nothing weaker.
- **The reason for the last refusal** is kept as one of five fixed messages: the stored settings
  come from a newer firmware; this firmware is still on trial and settings are written once it is
  confirmed, "about a minute"; the needle is moving, or a calibration or an update is running; the
  flash write itself failed; or the settings kept changing while being written. This is shown to
  the portal whenever a save is refused.

Worked example, a portal slider:

```
t = 0.0 s   a setting is changed to 140   applied at once, recorded as changed
t = 0.4 s   the same slider moves again, to 150   recorded as changed again
t = 2.4 s   the main loop sees it is unsaved and 2 s have passed, but the needle is
            still moving -> the write waits
t = 3.1 s   the needle stops -> the write runs and succeeds -> "settings saved."
```

**Writes that happen without being asked:**

- The main loop saves the tuning shaft's last known angle every 10 s, when it has changed, and only
  while the encoder is actually answering.
- The main loop copies the WiFi driver's own learned transmit power into the settings structure
  whenever the two differ (see `firmware-gospel-network.md` for how that power is learned).
- The automatic tuning sampler stores calibration samples, refreshing a nearby one or, once all
  nine automatic slots are full, replacing the most crowded one (see `firmware-gospel-tuning.md`).
  It never touches the hand marks or the hand offset.
- The index-band calibration stores the measured band and shifts the soft limits into the new
  frame of reference.
- The two one-time boot purges.

**The three actions that force an immediate save:**

| action | what it does |
|---|---|
| Portal "Save settings now" | Forces an immediate flush; answers "saved", or shows the refusal reason as an error. |
| Portal "Reboot the main board" | While the S3 image is still on trial: restarts at once and answers that it is rebooting because this firmware was still on trial, so the previous firmware returns; nothing was saved during the trial, so there is nothing to wait for, and this button remains the one way to undo a bad update. Otherwise: forces an immediate flush; if that is refused and the settings are not locked, the needle is stopped and the save retried every 100 ms, up to 15 times; if it is still refused, the needle resumes and the reboot itself is refused with the reason. While the settings are locked the reboot proceeds anyway, since nothing would have been written regardless. |
| End of an S3 firmware update | Clears the "update in progress" flag first (otherwise a write could never be judged safe), forces an immediate flush, and reboots even if that flush failed — such a failure is logged on the USB console only. The new image then boots on trial. An upload sent while the currently running image is itself still on trial is refused before any of this happens: nothing is flushed and nothing restarts. |

## Ambersong firmware: the downgrade lock

When settings load and find the firmware's own magic number but a version it does not recognise,
this firmware is older than the one that wrote the stored blob. The downgrade lock is then set.
While it is set:

- The machine runs entirely on **defaults** in memory: provisional soft limits, the default tuning
  line, default brightness, and so on.
- Every write is refused, so the newer blob already on flash is never overwritten.
- **Downloading the settings file is refused**, with a message, and the portal page shows that
  message instead of navigating to the file. A file built from defaults would look complete, and
  uploading it back would both lift the lock and overwrite the real calibration with those
  defaults.
- The portal shows a "SETTINGS LOCKED - newer version stored" indicator.
- "Save settings now" answers with the lock's own message.

There are two ways out. **Flash the newer firmware again**, which reads its own blob normally. Or
**upload a complete settings file**: every one of the 38 S3 rows present and accepted, a tuning
block that commits as a whole (the sample mask present, the sample lines matching that mask, the
hand offset readable), and a fixed-feature block that commits as a whole (the fixed-feature count
present, every slot up to that count present). A32 rows, the two fallback-line values, and which
tuner ends were measured are not required. Keys this firmware does not recognise — from a newer
firmware's own file — are named but do not block the unlock. On success, the console logs that a
complete settings file was uploaded and that the newer stored settings will now be replaced, and
the next save writes the current version over the newer blob. Otherwise the portal reports exactly
how many of the 38 settings arrived, and that no tuning marks or fixed features were received, and
that nothing was saved; whatever values did arrive are still applied in memory even so.

A single changed value cannot lift the lock by itself, because while the lock holds, the whole
settings structure is defaults: lifting the lock on one changed value would otherwise write
defaults for everything else over the newer blob, two seconds later.

## Ambersong firmware: the A32's own storage

- **Defaults** are filled in first, for both the audio and the Bluetooth structures; the values
  themselves are in the complete reference below.
- **Loading**: defaults first; then the volume knob's three calibration points (minimum default
  60, centre default 0 meaning "not measured", maximum default 3990); then the audio and Bluetooth
  blobs, each loaded only if its stored length exactly equals its structure's current size. Any
  change to either structure's layout therefore means that whole structure resets to its defaults —
  there is no migration on this board. Finally, mute is always forced to 0: mute is never restored
  across a boot.
- **"Something changed"** sets a dirty flag and a timestamp.
- **Flushing**, on every pass of the main loop: when dirty, and either forced or at least 2000 ms
  since the last change, both blobs and the three volume-knob keys are written, and **every
  result is checked** — the storage handle opening, and each write's byte count. Only once every
  one of them succeeds is the dirty flag cleared and "settings saved" logged. On any failure the
  flag stays set, the two-second wait restarts so the write retries, and a warning is logged on the
  A32's own USB console and relayed to the S3 as a log line. This board has **no motion guard** at
  all, since it drives no stepper motor.
- **Nothing is saved while a newly updated A32 image is on trial**, the same rule as the S3's.
  Changes made meanwhile — from the portal, a pot calibration, or the A32's own console — are
  applied at once and wait, still dirty, in memory. The image confirms itself about a minute after
  the S3's first handshake message, or after five minutes if the S3 never spoke at all, and never
  while the proto self-test has failed. On confirmation, if anything is still dirty, a flush runs at
  once; once that write succeeds, the A32 reports to the S3 that settings changed during the trial
  have now been saved, alongside its own confirmation message. If that write instead fails, the
  ordinary failure path applies — the warning, and a retry two seconds later. The hold is silent
  while it lasts: the portal still answers "ok" to an edit of an A32 row, because the S3 keeps no
  flag of its own for the A32's trial state; it only ever sees that state as text in the A32's own
  boot report (see `firmware-gospel-link.md`). So an A32 image that changes the size of either
  structure cannot save its new layout before it has proved itself, and a rollback always finds
  blobs the previous image can still read. The cost: a change made during the A32's own trial
  minute is lost if power is lost, or the A32 itself restarts, before it confirms.
- **A commanded restart of the A32** flushes first, if that is possible; if the flush fails, the
  board restarts anyway and the change is lost. During an A32 trial that flush writes nothing by
  design — a restart during a trial is itself a rollback, so there is nothing worth keeping across
  it.
- **Writers**: the portal's audio and Bluetooth set actions, a pot calibration, and the A32's own
  console keys.
- **The volume knob** is read every 50 ms (the median of five readings), mapped through its
  three-point calibration to a 0–255 range, and written straight into the live volume **without**
  touching the settings system at all. The very first reading after every A32 boot always applies;
  later readings apply only once the raw value has moved by at least 24 counts. So a volume set
  from the portal holds only until the knob is next moved, and the knob's own position always wins
  at the next boot. The volume reaches flash only when something else triggers a save.
- **Bounds applied on the A32 itself**: the volume law is clamped to 1.0–4.0 and the balance to
  −100–100 when applied; gains are clamped to −60 to +30 dB when converted from tenths of a dB.
  Fade times are not clamped at this stage (a zero behaves as 1 ms); the S3 is the only thing that
  ever sends these values across the link, and it clamps them first.
- During an A32 firmware update, mute is forced on and restored afterwards to whatever it had been.

## Ambersong firmware: how the S3 mirrors the A32's settings

The A32 **owns and saves** its own settings. The S3 keeps a **mirror** of them — the audio
structure, the Bluetooth structure, and the volume-knob calibration together — so that the portal
can show what is actually running on the machine, rather than only what it last asked for.

```
  S3                                              A32
  HELLO  ------------------------------------------>
         <------------------------------------------  HELLO_ACK (its firmware version)
  GET_CFG ----------------------------------------->
         <------------------------------------------  CFG (audio, Bluetooth, pot calibration)
  mirror = CFG ; "the mirror is known" = true
         <------------------------------------------  STATE, every 250 ms
  mirror's volume and mute = STATE's                 (the knob moves the volume; nothing
                                                       else would tell the S3)
  portal: a gain is changed
  mirror updated
  SET_AUDIO = the whole audio structure ------------>  applied, and saved two seconds later
```

- At every handshake, the S3 requests the A32's current settings and copies the reply into the
  mirror. This way a peer that has just rebooted brings **its own** values back, rather than the
  S3's mirror overwriting them. When the A32's own reported uptime goes backwards, the S3 notices a
  restart the link itself missed, and handshakes again, which refreshes the mirror.
- The portal's Audio tab also has a **Re-read the A32** button, which requests the current settings
  on demand.
- Changing an A32 row edits one field of the mirror and sends the **whole** structure it belongs
  to. The A32 copies it, applies it, and saves it two seconds later — or, while its own image is on
  trial, once that image confirms itself.
- **The mirror is never written before it has actually been read.** Until the very first reply
  arrives, the mirror holds all zeros; setting any A32 row is refused outright while that is the
  case — pushing one in that window would have sent volume 0, every gain at 0, every LED off, and
  "not connectable" to the A32, which would then have saved all of it. The portal answers such an
  attempt with a refusal: "the audio board is not answering - its settings cannot be changed now".
- **A silent A32 is forgotten.** When the link, having once handshaken, stops hearing the A32 at
  all, the S3 clears "the mirror is known" along with the rest of its A32 state, and logs a
  warning. From that point the same guards apply again: portal edits of A32 rows are refused, a
  download writes "n/a" for them, and an upload names them as "not sent". The next handshake fetches
  the settings afresh. Before this was added, the mirror stayed marked "known" for the rest of the
  boot: edits made while the A32 was silent went only into the mirror and were silently overwritten
  once it came back, and a download in the meantime wrote the stale copy instead of saying so.
- The settings file follows the same rule: while the mirror is not known, export writes "n/a" for
  every A32 row, and import names A32 rows "not sent" (see below).
- **Boards built against different protocol versions** no longer look like a simply dead
  connection: the S3 counts every frame it drops for that reason and shows the count on the
  console, in its own state, and — while no handshake stands at all — as a portal warning telling
  the owner to flash both boards (see `firmware-gospel-link.md`).
- The volume knob's own calibration is not a row: the portal's three "Pot" buttons on the Audio tab
  send a dedicated calibration message, and the A32 measures the knob itself, because the reading
  and the thing being calibrated must not live on opposite sides of the link.

## Ambersong firmware: concurrency

The S3 changes settings from two separate tasks:

| core | task | writes |
|---|---|---|
| 0 | the portal task | the "set" endpoint, the settings-file upload, every portal action that changes a setting (tuning marks and offsets, needle limits and nudges, tuner-end captures, clearing the fixed-feature list, "save now"), and the reboot and update-end saves. Each of these writes the S3's own settings or the A32 mirror and re-applies settings, from core 0. |
| 1 | the main loop | the periodic flush, saving the last shaft angle, copying the learned WiFi power, tuning samples, band-calibration results, console keys, and the mirror update from the A32's own messages (its settings reply, and volume/mute from its periodic state message). |

What actually protects the data: the two counters (a change landing on either core during a write
is never marked saved by that write); the single-writer flag inside the write function itself; the
snapshot (the blob actually written is one single, consistent copy); calling "something changed"
only *after* the change, everywhere; and every multi-field unit (the tuning marks with their mask
and offset, the fixed-feature list) being parsed into local variables and committed as one step.

What is **not** protected, by deliberate choice: there is no lock around the settings structure
itself. The races that remain are only microseconds wide: an upload half-applied at the very
instant a background save takes its snapshot, followed by a power cut within about two seconds; a
hand mark and an automatic tuning sample landing in the same microsecond; or a soft-limit edit
racing a concurrent application of settings.

## Ambersong firmware: the settings file — format and export

The portal's System tab serves every setting as a plain-text file attachment, `ambersong.txt`,
administrator only. The format is plain text, one `key=value` line per setting, with `#` starting
a comment line. It stays readable even when neither this firmware nor a web browser exists to read
it.

```
# Ambersong settings
# firmware v.1.20260925T184108 (build stamp and commit; see `firmware-gospel.md`)
# settings version 7
# saved 2026-09-25T18:45:00 (only when the clock is valid; local time)
# the audio board had not answered - its settings are n/a (only while its mirror is unknown)
volume=40                                       one line per setting, in table order
...                                             (58 lines)
gainRadio=8.000                                 floating-point and scaled rows: 3 decimals
ifOffset=10.600
...                                             every other row: a plain integer
connectable=1                                   A32 rows read "n/a" while the mirror is unknown
bandLow=881
bandHigh=1079
tuneOffset10=0
tuneUsed=7                                      ALWAYS written, even when 0
tunerEndsSet=3
tuneMark0=-817,913                              <shaft count>,<frequency x10>, one per set bit
tuneMark1=120,985
tuneMark2=1043,1073
spurUsed=2                                      ALWAYS written, even when 0
spur0=790                                       LO frequency x10, one per slot in use
spur1=818
# potcal 61..1974..3988                         only while the mirror is known; never imported
```

(The values above are illustrative, not a real backup.)

- **The tuning and fixed-feature counts are always written, even when they are zero.** Importing
  hinges the "replace the marks" and "replace the fixed-feature list" steps on seeing these two
  keys at all, so that a backup taken before either feature was ever used can correctly undo later
  marking. Without the zero line, such a file would look identical to one from before those
  features existed, and importing it would leave the current marks untouched instead of clearing
  them.
- **A32 rows read "n/a" whenever the A32's mirror is not known.** A file built from the mirror's raw
  zeros, uploaded later, would push a volume of 0, every gain at 0, every LED off, and "not
  connectable" to the A32 — which would then save all of it.
- **Not in the file at all:** the header fields themselves (shown only as comments); the last saved
  shaft angle (live state, not a setting); the index-band homing edge (always zero by definition);
  the learned WiFi transmit power (console-only, see `firmware-gospel-network.md`); an unused field
  left over in the A32's Bluetooth structure; the WiFi and time settings; the portal accounts; the
  volume-knob calibration (shown only as a comment); and any experiment toggle (see "Deliberately
  not settings" below).

## Ambersong firmware: the settings file — import

The portal's System tab posts the uploaded text to the settings-import endpoint, administrator
only, with a 20-second timeout.

**An upload is a merge, never a replace.** A key absent from the file keeps its current stored
value untouched. The only exceptions are the two blocks below, each of which replaces its whole
stored list as one unit, but only once it has been fully and correctly received.

**Per line:** the line is trimmed; empty lines, and lines that *start* with `#`, are skipped (a
trailing `# comment` after a real value makes that value unreadable, so comments must be whole
lines); the line is split at its first `=`; a line without one is skipped. Then:

| key | rule | on a bad value |
|---|---|---|
| which tuner ends measured | whole integer 0–3, applied at once | refused, and named |
| fallback line, low / high | whole integer 500–2000 (×0.1 MHz), applied at once | refused and named (refused outright, not clamped) |
| hand offset | whole integer −200–200 (×0.1 MHz, so ±20 MHz), held with the marks | the whole tuning block is refused, with a note that the hand-offset line could not be read and the stored calibration was kept |
| sample mask | a whole number, in decimal or `0x` hexadecimal, 0–0xFFFF, held | ignored with no note of its own, and every mark line after it is then skipped; if the file also has a hand-offset line (as every exported file does), the block is refused with a misleading note that announces "0 and 0 parsed"; without one, nothing is said at all |
| one sample slot | only accepted after the sample-mask line; the slot number must be a whole decimal 0–11 (a label like "mark A" is not accepted, since that is how the console's own dump labels the hand slots, and a looser parse once read such a label as slot 0, silently handing one mark's numbers to another's slot while the mask still appeared to agree); value is `<shaft position>,<frequency>`, position within ±1,000,000, frequency 870–1085; held | skipped silently, and the mask check then refuses the whole block |
| the two retired keys (the old WiFi-power file line, and an unused A32 field) | skipped whatever the value, not counted, not named — so a file saved by an older firmware still uploads cleanly | — |
| fixed-feature count | integer 0–16, held | spoils the fixed-feature block |
| one fixed-feature slot | only accepted after the fixed-feature count; slot number 0–15, value 772–972 (×0.1 MHz, the receiver's LO window), no duplicate slot; held | spoils the fixed-feature block |
| any other key | looked up in the settings table | see the next list |

Any key starting with the sample-slot prefix or the fixed-feature prefix is always treated as part
of its own block, never as an unrecognised key.

**For a table row**, in this order: an unknown key is refused; the value `n/a` (in any case) is
noted as "n/a in the file - kept as they are"; an A32 row while the mirror is unknown is noted as
"not sent - the audio board is not answering"; a value that is not wholly a number (one decimal
comma allowed) is refused; otherwise the value is bounded, and if bounding changed it by more than
0.0005 it is noted as clamped and stored at the bound instead. Every row is written straight into
the live structure or the mirror, with no message printed line by line.

**At the end of the whole file, in this order:**

1. **The tuning block** commits only if the sample mask was actually seen, the parsed sample slots'
   bits exactly equal that mask, and any hand-offset line present was readable. Only then do all 12
   slots, the mask, and the offset (if given) replace the stored ones together. Otherwise the whole
   block is refused, naming how many marks the file announced against how many actually parsed, and
   the stored calibration is kept. A file with no sample-mask line at all (older than the version
   that introduced it) leaves the stored marks untouched.
2. **The fixed-feature block** commits only if the count was seen, no line spoiled it, every slot
   up to that count actually arrived, and no two entries sit closer than 0.3 MHz (closer than that,
   the matching logic would treat them as one). Otherwise the block is refused and the stored list
   is kept.
3. **Tuner-end inference**, for an older file that carries a tuner-end value but no explicit record
   of which ends were measured: for each end, a value equal to the structure's own compiled default
   is read as "not measured"; anything else is read as "measured". This is a convention of the file
   format, not a claim about the actual shaft — a measured end that happened to equal the default
   exactly would be misread as unmeasured. This step runs before the curve is refitted.
4. The downgrade lock is checked.
5. If any S3 value actually changed, settings are applied and the change is recorded. If the soft
   limits that resulted differ from what the file asked for (because they did not bracket the
   index), that is noted too, naming both the refused pair and the pair actually kept.
6. **The fit gets the final say.** Unless the tuning block was refused, and whenever marks are
   stored, if the resulting curve is refused or falls back to a straight line, that state is added
   to the reply. The tuner-end values alone can cause this, since they set the fit's own domain.
7. Four named lists are appended, in this order, so block-level refusals are never crowded out:
   refused keys (unreadable or unrecognised), clamped keys, keys read as "n/a" and left untouched,
   and keys read as "not sent" because the audio board was not answering. Each list names up to five
   keys, each cut to 20 characters, then "...".
8. If any A32 row actually landed, the whole audio and/or Bluetooth structure is sent to the A32
   once, at the very end.

The reply counts every accepted line, plus one for each block that actually committed. The portal
shows a plain count on full success, or the same count together with the warning text whenever
there is anything to warn about; the page then shows only the warning, not the count, and reloads
the current values.

**Worked example.** With the A32 answering and nothing locked, uploading:

```
volume=999
posMax=abc
wn=10,5
nosuchkey=1
tuneUsed=7
tuneMark0=-817,913
tuneMark1=120,985
```

gives: the volume clamped to 255 (noted as clamped) and sent to the A32 once at the end; the soft
limit line refused outright; the natural-frequency line accepted as 10.5; the unknown key refused;
the sample mask announces three marks but only two parse, so the whole tuning block is refused and
the stored marks are kept. The reply counts 2 settings applied, with a warning naming the tuning
block's refusal (3 announced, 2 parsed, calibration kept), the two refused keys, and the one
clamped key. The natural-frequency setting is saved about two seconds later, once the needle is
still.

## Ambersong firmware: the complete settings reference

Legend. **Owner:** S3; A32 audio structure; A32 Bluetooth structure. **Saved in:** the S3's `cfg`
blob; the A32's `audio` or `bt` blobs; the A32's individual pot-calibration keys. **Changed by:**
portal (guest-visible); portal (administrator only); the settings file; the S3 console; the A32
console; the front volume knob; a named portal action button; the firmware itself, automatically.
Every row below is exported in the settings file and can be imported. Ranges are the clamp applied
when the value is set, unless stated otherwise.

### Audio tab (all owned by the A32)

| key | default | unit (stored as) | range | meaning | changed by |
|---|---|---|---|---|---|
| volume | 40 | 0–255 linear fader (u8) | 0–255 | Master volume; the volume law itself is applied on the A32. The knob's first reading after each A32 boot replaces it. | Portal (guest), file, A32 console `+`/`-` (10 per press; `+` also unmutes), front knob |
| muted | 1 in the defaults, forced to 0 at every load | bool (u8) | 0/1 | Software mute (writes silence into the DAC stream). Saved, but never restored across a boot. | Portal (guest), file, A32 console `m`; forced on during an A32 update and restored after |
| taper | 2.5 (stored 25) | gamma (u8, ×10) | 1.0–4.0, step 0.1 | Volume law: gain = (volume/255)^gamma; 2.5 puts half of the fader's travel at about −15 dB. | Portal (admin), file, A32 console `t`/`T` (0.1 per press) |
| balance | 0 | −100..+100 (i16) | −100–100 | Negative leans left. | Portal (admin), file |
| gainRadio | +8.0 dB (stored 80) | dB (i16, tenths) | −20..+30, step 0.5 | Makeup gain on the tube radio's own input. | Portal (admin), file |
| gainBt | −7.0 dB (stored −70) | dB (i16, tenths) | −20..+30, step 0.5 | Bluetooth gain, set to match the radio's own loudness within about 1 dB. | Portal (admin), file |
| monoSum | 1 | bool (u8) | 0/1 | Sum left and right on the radio input (about 3 dB less noise). | Portal (admin), file |
| muteOnChange | 1 | bool (u8) | 0/1 | Fade down and back up across a change of source. | Portal (admin), file |
| fadeInMs | 1000 | ms (u16) | 0–5000, step 50 | Fade-up time on arriving at a source. | Portal (admin), file |
| fadeOutMs | 150 | ms (u16) | 0–5000, step 10 | Fade-down time on leaving a source. | Portal (admin), file |

Why these defaults: the volume and balance defaults date from an earlier arrangement that used the
amplifier's own volume control, which has since been replaced and now sits on the amplifier's own
board, reached only from the back (see `hardware-bible.md`). The front panel's volume knob is the
A32's own potentiometer, not the amplifier's (see `hardware-bible.md`). The defaults were left as
they were: the volume is overwritten by the knob's first reading at boot regardless, and balance
remains a plain left/right trim. Balance does nothing on the AUX input, which never passes through
the digital converter at all (see `hardware-bible.md`). The radio gain of +8.0 dB was measured with
the tube radio's own volume control at about one eighth of its travel, its normal listening
position; the previous +15 dB clipped the gain stage at every volume. The Bluetooth gain of −7.0 dB
was measured the same way: a pink-noise reference at −14 dBFS RMS (a typical streaming loudness),
played from a computer at full volume, arrived at about −15 dBFS; −7 dB brings it within about 1 dB
of the radio, so a change of source does not also change the loudness. The fade times are
deliberately asymmetric: arriving at a source should be gentle, about a second, never a jump; leaving
should be quick, because the listener has just operated the selector. The mute-on-change setting
exists because the source selector is a centre-off switch (see `hardware-bible.md`): its centre
position reads electrically as AUX, so a change from RADIO to Bluetooth briefly passes through AUX.
The fade covers that passage.

### Display and Lights tabs

| key | default | unit (stored as) | range | meaning | owner | saved in | changed by |
|---|---|---|---|---|---|---|---|
| brightOn | 255 | 0–255 (u8) | 0–255 | Clock brightness while the amp is powered. | S3 | S3 blob | Portal (guest), file, S3 console `-` and `+`/`=` (15 per press) |
| brightOff | 110 | 0–255 (u8) | 0–255 | Clock brightness with the amp off. | S3 | S3 blob | Portal (guest), file |
| hour12 | 0 | bool (u8) | 0/1 | 12-hour clock. | S3 | S3 blob | Portal (admin), file |
| blankLeadZero | 1 | bool (u8) | 0/1 | Blank the hour's leading zero. | S3 | S3 blob | Portal (admin), file |
| dispDwellMs | 250 | ms (u16) | 0–5000, step 50 | Stillness required before the clock brightness starts to move. | S3 | S3 blob | Portal (admin), file |
| dispFadeMs | 1500 | ms (u16) | 0–10000, step 50 | Eased travel time between the two brightness levels. | S3 | S3 blob | Portal (admin), file |
| showTuning | 0 | choice (u8) | 0–2 | 0 = clock always; 1 = tuning readout always; 2 = tuning readout while tuning, then back to the clock. | S3 | S3 blob | Portal (admin), file, S3 console `f`/`F` (cycles) |
| tuneHoldMs | 4000 | ms (u16) | 500–20000, step 250 | In mode 2, how long the readout stays after the dial last moved. | S3 | S3 blob | Portal (admin), file |
| panelTuning | 255 | 0–255 (u8) | 0–255 | Panel light level while tuning. | S3 | S3 blob | Portal (guest), file |
| panelIdle | 128 | 0–255 (u8) | 0–255 | Panel level, radio idle. | S3 | S3 blob | Portal (guest), file |
| panelOther | 96 | 0–255 (u8) | 0–255 | Panel level on AUX or Bluetooth. | S3 | S3 blob | Portal (guest), file |
| panelIdleMs | 5000 | ms (u16) | 0–60000, step 250 | Time without tuning before "idle". | S3 | S3 blob | Portal (admin), file |
| panelFadeMs | 900 | ms (u16) | 0–5000, step 50 | Eased panel fade. | S3 | S3 blob | Portal (admin), file |
| panelDwellMs | 250 | ms (u16) | 0–5000, step 50 | Stillness required before the panel moves. | S3 | S3 blob | Portal (admin), file |
| btLedOff | 255 | 0–255 (u8) | 0–255 | Bluetooth LED brightness scale in the OFF state. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedOn | 255 | 0–255 (u8) | 0–255 | ... state ON, ready. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedLook | 255 | 0–255 (u8) | 0–255 | ... state LOOK, looking for a phone. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedStdby | 255 | 0–255 (u8) | 0–255 | ... state STDBY, standby. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedFound | 255 | 0–255 (u8) | 0–255 | ... state FOUND. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedCon | 255 | 0–255 (u8) | 0–255 | ... state CON, connected. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| btLedLink | 255 | 0–255 (u8) | 0–255 | ... state LINK, explicit pairing. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |

The off-state brightness is not simply lower by intent: the display's colon is lit by its own
fixed circuit and cannot dim along with the digits (see `hardware-bible.md`).

### Needle and Bluetooth tabs

"hs" is a half-step of the needle's stepper motor. "Counts" are the tuning shaft's accumulated
AS5600 encoder counts, carried across whole turns (see `firmware-gospel-tuning.md`). Every needle
row is administrator only. The motion defaults were arrived at on a bench test rig; they are this
build's own settings, not a reference value the firmware departs from (see
`firmware-gospel-needle.md` for how each was chosen).

| key | default | unit (stored as) | range | meaning | owner | saved in | changed by |
|---|---|---|---|---|---|---|---|
| upVmax | 1100 | hs/s (u16) | 100–4000, step 50 | Top speed of every second-order move, in either direction. | S3 | S3 blob | Portal (admin), file |
| upAccel | 18000 | hs/s² (u16) | 1000–60000, step 500 | Acceleration of the same moves. | S3 | S3 blob | Portal (admin), file |
| dnVmax | 1700 | hs/s (u16) | 100–4000, step 50 | Top speed of the decay: the fall and the park. | S3 | S3 blob | Portal (admin), file |
| dnAccel | 18000 | hs/s² (u16) | 1000–60000, step 500 | Used only by the park's own arrival test; the decay law itself has no acceleration limit. | S3 | S3 blob | Portal (admin), file |
| wn | 9.0 | rad/s (f32) | 1–21, step 0.5 | Natural frequency of the second-order ring-down. | S3 | S3 blob | Portal (admin), file |
| zeta | 0.50 | none (f32) | 0.1–2.0, step 0.05 | Damping of the ring-down (overshoot). | S3 | S3 blob | Portal (admin), file |
| riseMs | 100 | ms (u16) | 10–3000, step 10 | Fall envelope, rise. | S3 | S3 blob | Portal (admin), file |
| fallMs | 1000 | ms (u16) | 10–5000, step 10 | Fall envelope, fall. | S3 | S3 blob | Portal (admin), file |
| dwellUpMs | 100 | ms (u16) | 0–3000, step 25 | Hesitation before rising. | S3 | S3 blob | Portal (admin), file |
| dwellDnMs | 1000 | ms (u16) | 0–3000, step 25 | Hesitation before falling. | S3 | S3 blob | Portal (admin), file |
| microFast | 16 | 1/n (u8) | 1–32 | Microstep divisor for sweeps, parks, homing, and for the needle sitting parked (not moving). | S3 | S3 blob | Portal (admin), file |
| microSlow | 32 | 1/n (u8) | 1–32 | Microstep divisor while actually tracking the tuner knob. | S3 | S3 blob | Portal (admin), file |
| reapHsps | 60 | hs/s (u16) | 10–500, step 10 | Creep speed of the index-band calibration; also the portal's own jog speed. Not the homing re-approach. | S3 | S3 blob | Portal (admin), file |
| posMin | −300 | hs from the index (i32) | −3000–3000 | Low soft limit. The pair must bracket the index (0) and not cross; a value that would not is refused outright and the previous pair is kept. The row's own range is deliberately wide, so a wrong value is refused by that check rather than silently stored as a bound. Reset to −300..+300 at boot if found corrupt. | S3 | S3 blob | Portal (admin), file, S3 console `m`, portal actions (limit, nudge both at once), automatic (band calibration; boot purge) |
| posMax | +300 | hs from the index (i32) | −3000–3000 | High soft limit. Same rules. | S3 | S3 blob | Portal (admin), file, S3 console `M`, portal actions, automatic |
| idxOffFwd | 0 | hs (i32) | −500–500 | Index band: sensor OFF edge going up, relative to the index. A measurement, not a preference. | S3 | S3 blob | Automatic (band calibration), portal (admin), file |
| idxOnRev | 0 | hs (i32) | −500–500 | Index band: ON edge going down. | S3 | S3 blob | as above |
| idxOffRev | 0 | hs (i32) | −500–500 | Index band: OFF edge going down. | S3 | S3 blob | as above |
| sweepOn | 0 | bool (u8) | 0/1 | A full end-to-end needle sweep each time the amp comes on. Homing itself ends with a full sweep regardless of this setting. | S3 | S3 blob | Portal (admin), file |
| dialLow | 879 | ×0.1 MHz (u16) | 500–2000 | Frequency printed at the needle's low stop. | S3 | S3 blob | Portal (admin), file |
| dialHigh | 1079 | ×0.1 MHz (u16) | 500–2000 | Frequency printed at the needle's high stop. | S3 | S3 blob | Portal (admin), file |
| calLow | 0 (placeholder) | counts (i32) | −20000–20000 | Tuning shaft position at its low mechanical end. Typing it marks that end as measured. | S3 | S3 blob | Portal (admin), file, S3 console `c`, portal action |
| calHigh | 6023 (placeholder) | counts (i32) | −20000–20000 | Tuning shaft position at its high end. Typing it marks that end as measured. | S3 | S3 blob | Portal (admin), file, S3 console `C`, portal action |
| ifOffset | 10.60 MHz (stored as 212) | MHz (i16, 50 kHz units) | 10.00–11.50, step 0.05 | The tube set's own intermediate frequency; turns the receiver's reading of the LO into a station frequency (see `firmware-gospel-tuning.md`). | S3 | S3 blob | Portal (admin), file |
| connectable | 1 | bool (u8) | 0/1 | Let a previously paired phone reconnect. Even at 1, the A32 is only actually connectable while awake (amp on, S3 talking) and the source is Bluetooth, so a phone cannot latch onto a radio whose amp is off overnight (see `firmware-gospel-sound.md`). | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| lookTimeoutS | 90 | s (u16) | 15–600, step 5 | Pairing window, despite the name: the explicit pairing state falls back to standby after this. The ordinary "looking" state has its own fixed 20 s, which is not a setting. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |
| pauseOnLeave | 1 | bool (u8) | 0/1 | Send a "pause" to the phone before the disconnect that follows leaving Bluetooth. | A32 (Bluetooth) | A32 `bt` blob | Portal (admin), file |

Why these defaults: the top speed of 1100 hs/s, rather than a higher one, reflects a measured
limit of this particular needle mechanism — moving up through the index faster than about
1100 hs/s, it was found to stop mechanically following while the firmware kept counting steps as
if it had (seen reliably at 1500, often at 1200–1400, never at 1100). This is a physical value of
this machine, held here by the firmware (see `firmware-gospel-needle.md`); the row's own range
still allows up to 4000, for a different mechanism. The microstep divisors exist because the
microsteps themselves are made entirely by the firmware, driving a simple stepper driver with no
microstepping of its own (see `firmware-gospel-needle.md`); the row caps the divisor at 32. The
soft-limit defaults of ±300 hs assume the needle's real travel extends further than that on each
side of the index, and that the index band itself is narrower than about 600 hs, so a sweep between
the provisional limits is guaranteed to cross it; these too are physical values of this machine.
The dial-stop defaults reflect what is actually printed on the dial face, 87.9 MHz at the low stop
and 107.9 MHz at the high stop. The tuner-end placeholders 0 and 6023 are the old compiled values
meaning "not measured", not a claim about the shaft. The intermediate-frequency default of
10.60 MHz is the tube set's own fitted FM IF (see `hardware-bible-tube-radio.md`).

The System tab itself has no rows; its page draws its own cards there instead (the settings file,
the network, the firmware, and the machine).

### Hand-written keys (present only in the file, not table rows; all owned by the S3, saved in the S3's blob)

| key | default | format and import rule | meaning | changed by |
|---|---|---|---|---|
| bandLow | 881 (88.1 MHz) | integer 500–2000, else refused | The pre-calibration fallback line, low end (×0.1 MHz). Nothing reads it once two samples exist. | File only |
| bandHigh | 1079 (107.9 MHz) | as above | Fallback line, high end. | File only |
| tuneOffset10 | 0 | integer −200–200 (×0.1 MHz, so ±20 MHz); commits with the marks | The hand offset (the dial correction), sliding the whole tuning curve. Never erased automatically. | Portal nudge action (0.1–5 MHz per press, ±20 MHz total); reset to 0 only by the reset action; file |
| tuneUsed | 0 | integer, decimal or `0x`; always exported | Which of the 12 sample slots hold a sample. | Hand-mark actions, the automatic sampler, drop, clear, S3 console `V`; file |
| tuneMark0 .. tuneMark11 | empty | `<shaft count, ±1,000,000>,<frequency ×10, 870–1085>` | The tuning samples: slots 0–2 are hand marks A, B, C; slots 3–11 are automatic samples. Raw shaft counts, so a sample keeps its meaning even after the tuner ends are re-measured. | as above |
| tunerEndsSet | 0 | integer 0–3, else refused; inferred for older files | Bit 0 = low end measured, bit 1 = high end measured. Only 3 lets the fit use the actually measured travel. | S3 console `c`/`C`, portal actions, typing a tuner end directly, file, the V4 migration (a V4 file or blob read by V5 code: `tunerEndsSet` is inferred from `calLow`/`calHigh`) |
| spurUsed | 0 | integer 0–16; always exported | How many fixed features are stored. Empty by default, on purpose: this list is only ever earned by measurement. | S3 console `o` (add the strongest candidate), `O` (clear), portal clear action, automatic boot purge; file |
| spur0 .. spur15 | empty | integer 772–972 (LO frequency ×10), no two closer than 3 | The fixed features: cabinet-internal peaks excluded from the hunt for the tube set's own LO. | as above |

The fixed-feature range is the receiver's LO window, 77.2–97.2 MHz, because fixed features are
learned at LO frequencies — the window is the printed dial minus the design intermediate frequency
(see `firmware-gospel-tuning.md`).

### Stored, but neither a portal row nor in the file

| item | owner, saved in | default | meaning | changed by |
|---|---|---|---|---|
| the header fields | S3 blob | fixed magic number; version 7 | The blob's own header (the version appears only as a comment in the file). | the firmware |
| last saved shaft angle | S3 blob | 0 | Seeds the shaft's turn choice at boot. | Automatic, every 10 s when changed, only from a live encoder |
| index homing edge | S3 blob | 0 | Always zero by definition. An attempt to import this key is refused as unrecognised. | the band calibration (always writes 0) |
| learned WiFi transmit power (`wifiTxQ`) | S3 blob | 8 (2.0 dBm) | The join ladder's current rung (see `firmware-gospel-network.md`). Printed by the S3 console dump only; not in the portal and not in the settings file. | Automatic, S3 console (cycles a fixed list of power levels) |
| unused A32 Bluetooth field | inside the A32's `bt` blob | 0 | A field nothing reads any more: the A32 never actually attempts to reconnect on its own. Kept only so the wire layout and the stored blob's size do not change. An old file's line for it is ignored. | nothing |
| volume-knob calibration (three raw readings) | A32, individual keys | about 60, 0 (not measured), about 3990 | Raw readings of the knob at its minimum, centre and maximum; a three-point map to 0–255. Shown in the file only as a comment. If the measured maximum is not at least 65 above the minimum, the defaults are used instead. | Portal "Pot" buttons, A32 console |
| house network credentials and time settings | S3, separate namespace | empty; a public time server; a compiled time zone | Never exported. | Portal Network card, S3 console (network name and passphrase only) |
| portal accounts | S3, separate namespace | one administrator account with a compiled default password, which the portal forces you to change | Never exported. | Portal account cards |

The volume-knob defaults (about 60 and about 3990) are the fallback raw readings of this
particular machine's own knob at its two ends; they apply only until the knob is actually
calibrated (its wiring is in `hardware-bible.md`).

### Deliberately not settings

A number of portal **actions** change behaviour only until the next reboot, and are never saved at
all: disabling the volume knob, and a handful of audio and display experiments. The reasoning
given for this, in the firmware's own source, is that an experiment that survives a reboot is one
whose eventual result gets credited to something else entirely, perhaps much later; if an
experiment turns out to be the actual fix, it is promoted to a real setting then, with its own
reason attached. One such experiment, tried for a day during the investigation described in
`firmware-gospel-sound.md`, is retired; it sent the radio to full volume and is not kept. The audio
experiments themselves are covered in `firmware-gospel-sound.md`, and the display test action in
`firmware-gospel-main-board.md`.

### Constants of the settings system

| name | value | meaning |
|---|---|---|
| settings magic number | fixed value | Marks a blob as having a header (version 2 onward). |
| current settings version | 7 | The S3's current layout. |
| S3 blob size | 220 bytes | |
| sample slots | 12 | 3 by hand, 9 automatic (see `firmware-gospel-tuning.md`). |
| fixed-feature slots | 16 | Frozen into the layout; the receiver module's own copy must hold exactly as many, checked at build time. |
| provisional soft limits | ±300 hs | Default and boot-purge soft limits. |
| S3 save debounce | 2000 ms | Quiet time after the last change, before a write. |
| last-angle save period | 10 s | |
| forced-flush retries | 40, 25 ms apart when the other core is writing | |
| reboot save retries | 15, 100 ms apart | |
| A32 save debounce | 2000 ms | |
| A32 trial hold ends | 60 s after the first handshake, or 5 min if the S3 never spoke | Held changes are written at confirmation. |
| knob deadband | 24 raw counts | |
| settings-file upload timeout | 20 s | |
| import note length | 512 bytes; each named list up to 5 keys, 20 characters each | |
| wire-message payload limit | 1088 bytes | Checked at build time for every structure sent across the link. |

## Ambersong firmware: why the settings system is built this way

> Each decision below was right for this build; a different machine may call for a different
> answer.

**One table describes every setting, and generates the schema, the widgets, the clamp and the
file.** Four hand-kept copies of the same information drift apart from each other over time; one
table instead makes adding any knob or slider cheap enough to actually do. A hand-written page
control and handler for every individual setting was tried first and rejected.

**Portal roles are split by the damage a change can do, not by secrecy.** A guest sees every
value, but may move only volume, mute and the five brightness levels — all obvious, and all
reversible. Hiding values from a guest entirely was considered and rejected.

**The firmware itself bounds every value; the browser's own limits are only a suggestion.** One
bounding function serves both the portal and the file, and refuses a "not a number" value outright.
A soft limit of, say, 30000 half-steps would drive the needle straight into its mechanical stop if
nothing clamped it. A second, separate copy of the same rule once guarded only the file path, which
let the portal itself store an invalid choice value. Validating only in the page, or keeping two
copies of the rule, were both tried and rejected.

**Numbers are parsed whole; one decimal comma is accepted as the decimal point; anything else is
refused.** The earlier parser read "107,3" as 107 and an empty field as 0, and the clamp then
quietly stored a bound value instead — an intermediate frequency of 10.00 MHz, or a soft limit
sitting exactly on the index. Using the standard library's own lenient number parsing was tried
first and rejected.

**Refused and clamped values are named, in their own lists, placed last, and sent in a separate
warning field, so the page can never paint a partial apply as a plain success.** Folding refusals
into the ordinary success message was tried and rejected.

**The S3's settings structure is versioned and append-only, migrated on load, and its version is
read from its own header — never guessed from its size.** An earlier scheme discarded everything on
any layout change at all, which is tolerable on a bench rig and not acceptable on a machine someone
has spent an evening calibrating. Size alone could not have told versions apart in any case: the
first three versions were all exactly 100 bytes. Dispatching on size, or requiring an exact-size
read, were both tried and rejected — an exact-size read would have discarded every blob from the
fourth version onward.

**Tuning samples are stored, not curve coefficients, and are refitted every time settings are
applied.** Raw samples describe themselves and can be refitted with a better model at any time with
no change to the stored layout, and no floating-point curve coefficient ever has to go into flash.
Storing coefficients instead was tried and rejected.

**Writes wait two seconds after the last change, and never happen while the needle is moving, a
calibration is running, or an update is being written.** A flash write stops everything outside
fast internal memory; measured step jitter during an unguarded write was about 383 microseconds,
against 49 microseconds with the guard in place. Writing on every single change was tried and
rejected.

**"Unsaved" is derived from two counters rather than kept as a flag of its own; only one writer
runs at a time; the blob written is always a single consistent snapshot.** A simple stored flag,
cleared around the write, lost changes that happened to land during that write. A plain dirty flag
was tried and rejected.

**"Save now" always tells the truth, and a reboot refuses to proceed over an unsaved change.**
Answering "saved" and then immediately rebooting over a save that had actually been refused was a
real path to losing data. A save that returned nothing meaningful, and a reboot that simply ignored
the save's result, were both tried and rejected.

**Nothing is written to flash while a newly updated S3 image is still on trial.** "Save now"
explains why it is held, and "Reboot the main board" is allowed to proceed at once during the
trial. An image that changes the settings layout migrates at boot and used to save that new layout
within seconds; if it was then rolled back, the previous image found a blob newer than it
recognised and locked itself onto defaults (the downgrade lock, above). Holding every write until
the image confirms itself closes this path by design, and a reboot during the trial is itself a
rollback with nothing waiting to be saved anyway, so refusing it would only remove the one button
that undoes a bad update. Simply documenting the trap for whoever next bumps the settings version
was tried first and rejected. Proven on this radio: a portal reboot while on trial correctly
answered that it was rolling back to the previous firmware, the board came back on the previous
image, and nothing had been saved during the trial; once re-flashed, it confirmed itself normally
about a minute later.

**The A32 writes nothing either while a newly updated A32 image is on trial; changes wait in
memory and are written the moment the image confirms itself, and the A32 says so over the link.**
The S3's own hold had not originally been carried over to the A32. An A32 update that changed the
size of either of its structures would otherwise have saved its new layout during the trial; rolled
back, the previous image would then have found a structure of the wrong size and simply run on its
own defaults (this board has no migration at all). A restart during the trial is itself a rollback,
so nothing held back across it is worth keeping. Advising an export of the settings file before any
update that changes an A32 structure, instead of holding the write, was tried first and rejected.
Proven on this radio: with an A32 update on trial, a Bluetooth LED level changed through the portal
was answered "ok" but the A32 wrote nothing until about a minute after the handshake, when it
reported that settings changed during the trial had now been saved, alongside its own confirmation
message.

**A settings blob from a newer firmware locks every write; export is refused while locked; only a
complete file lifts the lock.** Before the lock existed, the very first automatic save wrote
defaults straight over the newer blob, and a single-line upload did the same thing. Writing
defaults back, or lifting the lock on any upload at all, were both tried and rejected. A downgrade
is not expected in ordinary use, so the download refusal is meant as a one-line guard, not a
recovery path in its own right.

**The A32 owns and saves its own settings; the S3 only mirrors them, refreshed at every handshake,
and never pushes a mirror it has not yet actually read.** A rebooted A32 must keep its own stored
values regardless of what the S3 last asked for, and an unread mirror is simply all zeros. Making
the S3 the single store for both boards was considered and rejected.

**When the A32 falls silent, the S3 forgets its copy of the A32's settings until the next
handshake; portal edits of A32 rows are refused in the meantime.** The copy used to be treated as
known for the rest of the S3's own boot: edits made while the A32 was silent went only into the
mirror, were silently overwritten once it came back, and the portal answered "ok" regardless; a
download in that window wrote the stale copy instead of saying it was unavailable.

**The A32 only counts a save as successful once every write actually reached flash, and retries a
failure.** Its own dirty flag used to be cleared before the write happened, so every result was
simply ignored — a failed write still said "saved", and the change was gone at the next reboot. The
S3 had already been fixed the same way.

**When the A32 has not answered, the file carries "n/a" for its rows, and import names them as
not sent.** A file of zeros, uploaded later, would silence the radio and switch off every LED, and
the A32 would save all of it.

**The A32 never restores mute across a boot; it does restore volume, but the knob's first reading
always wins over it.** A saved mute once made the set boot silent with nothing on the front panel
to explain why.

**The A32 saves before a commanded restart, except while its own image is on trial.** Restarting
the audio board within two seconds of a change once lost that change outright. During a trial the
restart is itself a rollback, and the trial hold already writes nothing.

**The tuning marks, their mask and the hand offset commit from a file as a single unit; the
tuning and fixed-feature counts are always exported, even at zero.** Wiping the stored marks on
seeing the very first sample-mask line let a truncated upload erase the whole calibration along
with its only backup; an offset belongs to the curve it was correcting and must travel with it.

**The user's own hand offset is never erased automatically: no sample, hand mark, drop or clear
touches it.** Only the dedicated reset action, or a settings-file restore, ever changes it besides
the nudge action itself. It is the user's own calibration, made by ear; whether it still fits is
the user's own judgement to make (see `firmware-gospel-tuning.md`). Zeroing it on every sample, and
later zeroing it only on a requested sample, a new hand mark, a drop or a clear, were both tried and
rejected.

**The index homing edge is not a row.** It is zero by definition, and once, when it was editable,
an edited value moved both soft limits physically into a mechanical stop.

**The three other index-band values are rows, so the settings file carries them.** A restore onto
blank flash once left them at zero, and every crossing of the index afterwards read as a slip.

**The fallback line's two values stopped being portal rows, but still stay in the file.** Nothing
reads them once two tuning marks actually exist, and an editable "tuner reaches" field was
previously ignored outright; but with no marks stored at all, they are the live mapping, so a
backup must still carry them.

**The learned WiFi transmit power is treated as learned state: stored, stepped only from the S3
console, and printed by the console dump — but never a portal row and never a line in the settings
file.** It is what the join ladder itself discovered, not something anyone actually sets (see
`firmware-gospel-network.md`). As a scaled portal row it once showed one value in the file while
the console dump showed another for the same underlying number, and a value that only reached the
stored structure was then silently overwritten by the driver's own value anyway. Making it a portal
row, and later pushing a file's own value straight to the driver on import, were both tried and
rejected.

**The unused Bluetooth-reconnect row is gone; the field itself stays in the A32's structure,
unread.** Nothing in the firmware ever reads it: the A32 always disables automatic reconnection on
its own. A control that does nothing is worse than no control; the field stays only so the wire
layout and the stored blob's size do not change. An old file's line for it is ignored.

**The intermediate frequency is a setting, stored in 50 kHz units.** Measured readings of 10.60 MHz
(five times) and 10.65 MHz (once) cannot be written in tenths of a MHz at all, and the IF itself
could move again if the IF coils are ever realigned. The value itself is the tube set's own fitted
IF (see `hardware-bible-tube-radio.md` and `firmware-gospel-tuning.md`).

**The settings file is plain text, one `key=value` line per setting.** The file must be
recoverable by hand, and readable in ten years by something that is not this firmware at all. A
restore is the only undo available, so the backup must be able to outlive the tools that wrote it.

**Experiments are actions, and are never saved.**

**Credentials and portal accounts are never in the settings file, and a restore never restores
accounts.** The file itself is a backup that gets copied, pasted and kept anywhere at all; the
house network's own passphrase and the portal's password hashes stay on the machine alone. Losing
the accounts on a restore was judged the correct trade-off (a dedicated S3 console key erases every
account and restores the default administrator — see `firmware-gospel-network.md`).

**There is no lock around the settings structure across the two processor cores.** The races that
remain are only microseconds wide, and a full lock would have to touch every single writer for a
risk this small.

## Ambersong firmware: settings failures and recovery

Classes use the recovery ladder: **BLOCKER** = unusable until a full power cycle, or harm that
survives one; **DEFECT** = recovers with a portal reboot, a console reset, or the front switch;
**NOTE** = recovers by itself, or is cosmetic.

| what fails | what the firmware sees | what it does | how to recover | class |
|---|---|---|---|---|
| S3 flash write fails | the write itself fails | Stays unsaved, retries after 2 s, logs a warning; Save and Reboot both report it. | Automatic. | NOTE |
| A32 flash write fails | the write, or one write's byte count, is short | Stays unsaved, retries after 2 s, with a warning on its own console relayed to the S3's. A forced save just before a commanded restart is not retried — the board restarts anyway. Never exercised on the radio itself. | Automatic, except just before a restart. | NOTE |
| Power lost within 2 s of a change | | The change is lost; the previous value is intact. | Make the change again. | NOTE |
| Power lost during the trial minute after an S3 update | | Nothing was written during the trial, so every change made in it is lost; flash still holds the settings from before the update. | Make the changes again. | NOTE |
| Power lost during a write itself | | Relies on the underlying flash storage keeping the previous value when a write is interrupted; this has never actually been tested here, and whether it is always true is a property of that storage layer, not settled by this firmware's own code. | | NOTE |
| Power lost just after a background save caught a half-applied upload | | Half of the upload is on flash (an accepted, narrow race). | Upload the file again. | BLOCKER, accepted |
| Newer settings already on flash (a downgrade) | the firmware's own magic number, but an unrecognised version | The downgrade lock: defaults in memory, nothing written, download refused, an indicator shown. | Flash the newer firmware again, or upload a complete settings file. | DEFECT |
| Stored blob shorter than 100 bytes | a length check | Warning logged, defaults used, no lock; the defaults are saved on the first change. | Upload a settings file. | BLOCKER (never actually seen) |
| Header corrupted so the magic number differs | the magic-number check fails | Read as the original, header-less layout, marked dirty, and saved back in the current format — the resulting motion profile would be garbage. Whether the underlying flash storage would reject such a corrupt blob before this even happens is a property of that storage layer, never tested here. | Upload a settings file. | BLOCKER (theoretical) |
| Corrupt soft-limit pair at boot | the pair does not bracket the index | Reset to ±300 and saved; the console asks for recalibration. | Re-home, run a band calibration, set the stops again. | BLOCKER (guarded: the needle stays off its stops, but the limits must be captured again) |
| Corrupt fixed-feature list at boot | count or an entry out of range | Emptied and saved. | Re-learn with the set switched off. | DEFECT |
| A file with valid but simply wrong values uploaded | nothing | Applied and saved; this is the user's own act. | Upload a good file. | DEFECT |
| A truncated or damaged file uploaded | missing or malformed lines | Whatever arrived is merged in; the tuning and fixed-feature blocks are refused unless they are each complete; every refusal is named. | Upload the file again. | NOTE |
| A soft-limit pair in a file that does not bracket the index | the geometry check refuses it | The old pair is kept, and the reply says so. | Correct the file. | NOTE |
| A32 silent since the S3 booted | the mirror is not known | Portal refuses A32 rows ("the audio board is not answering ..."); export writes "n/a"; import names them "not sent". Never exercised on the radio itself. | Restore the link; the mirror arrives at the next handshake. | NOTE |
| A32 falls silent after having already answered | the link stops hearing it; the mirror is cleared | As the row above. Never exercised on the radio itself. | Restore the link; the next handshake refreshes the mirror. | NOTE |
| A32 restarts | a new handshake, or its uptime going backwards | Handshakes again; the mirror is refreshed from the A32's own flash. | Automatic. | NOTE |
| An A32 update changes either of its structures' size | the stored blob's length no longer matches | That whole structure resets to defaults, saved on the next change (after an over-the-air update, only once the new image confirms itself). The volume-knob calibration survives. | Upload a settings file exported before the update. | BLOCKER by design; avoidable by exporting first |
| Boards built from mismatched versions of the same wire-protocol version (a half-flashed pair) | a message is rejected on its length | The A32 ignores set-audio and set-Bluetooth messages; the S3 never receives a settings reply (the "n/a" path); the radio keeps playing on the A32's own stored settings. | Flash both boards. | BLOCKER (a function is lost until both are flashed) |
| Boards on different wire-protocol versions | every frame dropped and counted | No handshake happens at all; the A32 stays asleep and the radio is silent; a warning names it. | Flash both boards (see `firmware-gospel-link.md`). | BLOCKER (survives a power cycle once the A32 is confirmed) |
| Reboot pressed with an unsaved change while the needle is moving | the save is refused | Needle stopped, save retried for about 1.5 s; reboot refused with the reason if still unsaved; needle resumes. | Try again. | NOTE |
| S3 update ends with an unsaved change that cannot be written | the forced flush fails | Reboots anyway; a warning on the USB console only. | Make the change again. | NOTE |
| An S3 update that bumped the settings version is rolled back | nothing: the new image wrote nothing during its own trial | The previous image reads its own blob normally; no lock at all. | - | NOTE |
| An A32 update that changed either structure's size is rolled back | nothing: the new image wrote nothing during its own trial | The previous image reads its own blobs normally. | - | NOTE |
| Power lost, or the A32 restarted, during the A32's own trial minute | | Nothing was written during the trial, so every A32 change made in it is lost (a restart on trial is also itself a rollback); flash still holds the A32 settings from before the update. | Make the changes again. | NOTE |
| Flash wear | | Writes happen only on an actual change, two seconds later, never during motion. The most frequent writer is the last-angle save, at most once every 10 s and only after the dial actually moved. At a handful of writes per listening session, wear is not judged a practical concern — an estimate, not a measurement. | - | NOTE |

## Ambersong firmware: settings approaches tried and rejected

These were dead ends on this machine. They are not necessarily dead ends on anyone else's build.

- **Rejecting a blob outright on any size change at all.** Here, any layout change silently
  replaced a fully calibrated machine's settings with defaults. Dropped for versioned migration.
- **Deciding the version purely from the blob's own size.** The first three versions were all
  exactly 100 bytes, so three separate tests collapsed into one, and the oldest branch caught
  everything. Found by review, not by an actual failure. Dropped for dispatching on the header.
- **Reading only a blob of exactly the current structure's size.** Would have discarded every
  migratable blob from the fourth version onward. Dropped for "read up to our own size, whatever
  arrives".
- **Prefix copies sized only by the stored blob's length.** A new field can land inside an older
  structure's own trailing padding. Dropped for cuts based on the new field's actual byte offset.
- **A stored "dirty" flag, cleared both before and after the write.** Changes that happened to land
  during the write were marked saved, and lost. Dropped for the two-counter scheme.
- **A save that returned nothing meaningful and always printed "saved" regardless.** Dropped.
- **Purging corrupt soft limits from inside the settings-apply function itself.** It could fire
  mid-edit and overwrite a value that had just been typed. Moved to run once, at boot, instead.
- **Soft-limit ranges of −3000..0 and 0..3000.** These assumed a centred index; clamping instead of
  refusing an out-of-range pair made the high limit silently unsettable while it happened to be
  negative. Dropped for one wide range, validated by the geometry check instead.
- **Two separate copies of the bounding rule**, the second of which guarded only the file path.
  Dropped for the single shared bounding function.
- **The fallback line's two values as editable "tuner reaches" rows.** The fit ignored them
  outright, so a value typed there was simply discarded. Replaced by the curve's own computed
  reach, shown read-only.
- **The index homing edge as an editable row.** An edit moved both soft limits straight into a
  mechanical stop.
- **The learned WiFi power as a portal row, scaled by a quarter.** Two different spellings of the
  same underlying key existed at once; a pasted console value would have been read as four times
  the actual power.
- **The learned WiFi power kept in the settings file, and pushed to the driver on import.** It was
  learned state nobody actually sets by hand; taken out of both the portal and the file, and kept
  on the console alone.
- **The unused Bluetooth-reconnect row** ("chase the phone, not advised"). Stored, shown, exported
  and imported, and read by nothing at all. Removed as a row; the underlying structure field
  stays.
- **Two S3 console keys for an unbounded brightness adjustment.** One of them had no ceiling at
  all and bypassed its row's own bounds entirely; removed, since the portal row already does the
  same job within its proper range.
- **The A32 mirror counted as "known" for the whole rest of the boot** once it had first answered.
  A silent A32 then took portal edits into the mirror only, and a download wrote the stale copy
  regardless. Dropped for forgetting the mirror the moment the A32 falls silent.
- **The A32 clearing its own dirty flag before writing, and ignoring every write's actual result.**
  A failed write still said "saved". Dropped for the S3's own rule, checking every result.
- **Saving during an S3 trial, with only a documented trap for whoever next bumps the settings
  version.** A rollback after such a save left the previous image locked onto defaults. Dropped for
  holding every write until the image confirms itself.
- **Saving during an A32 trial, with only the advice to export a settings file before any update
  that changes an A32 structure.** A rollback after such a save would have left the previous image
  running that whole structure on its defaults. Dropped for the same hold as the S3's own.
- **Zeroing the hand offset on a hand mark, a drop, a clear, or a requested sample.** Each of these
  erased a correction the user had deliberately set by ear, unasked. Dropped for the explicit reset
  action.
- **Wiping the stored marks the moment the sample-mask line was seen, and applying the hand offset
  at once rather than holding it.** A truncated file erased the whole calibration outright, or
  slid an offset onto a curve it was never measured against.
- **Using the standard library's own lenient integer parser for hand-parsed keys.** It answers 0 for
  outright garbage, and 0 is the single most destructive value for the sample mask, the
  fixed-feature count, and which tuner ends are measured.
- **One single bit covering both tuner ends.** Measuring one end declared both measured, and the
  fit then ran over a domain far wider than the shaft's own actual travel.
- **Exporting raw zeros for a silent A32**, rather than "n/a". Dropped once "n/a" was added.
- **Lifting the downgrade lock on any upload at all.** Dropped for the complete-file rule.
- **Saving the A32's own mute state.** The set once booted completely silent, with nothing on the
  front panel to explain why.
- **Showing refusals inside the success message itself**, painted in the success colour. Dropped
  for the separate warning field.

## Ambersong firmware: known limits of the settings system

**Never exercised on the radio itself:** the "n/a" export and the refusal of A32 rows (both need
the A32 actually silent); the downgrade lock (no downgrade is expected in ordinary use), and with
it the complete-file unlock, the "not sent" note, and the locked-download refusal; and an A32 save
failure. **Exercised:** export; upload (including, once, pushing the learned WiFi power through a
file, before that was removed); every migration up to the current version; the S3's own trial hold
(a portal reboot while on trial correctly rolled back with nothing saved); and the A32's own trial
hold (a change made during a trial was written only at confirmation, with its own log line). **Not
exercised:** an A32 rollback with a change still held in memory, and a failed write at the moment
of the A32's own confirmation.

**Known limits in the code:**

1. The A32 has no migration at all: any change to either of its two structures resets that whole
   structure's stored settings to defaults on the very first boot of the new image. Since the A32
   trial hold was added, a rollback no longer adds to this — but a forward update still does, so
   export a settings file before any update that changes an A32 structure, and upload it again
   afterwards.
2. **Rollback.** Neither board writes its settings while its own image is on trial, so a rollback
   of either board always finds blobs the previous image can still read.
3. A change made during either board's own trial minute exists only in memory until that image
   confirms itself; a power cut during that minute loses it. On the A32, so does any restart during
   that minute, since a restart on trial is itself a rollback. The A32's hold says nothing while it
   lasts — the portal still answers "ok", and nothing visibly marks the change as waiting; only the
   log line at confirmation shows that it actually happened.
4. The A32's own forced save before a commanded restart is not retried: if that write fails, the
   board restarts regardless and the change is lost.
5. Changes made on the A32's own console (the volume law, a pot calibration) do not refresh the
   S3's mirror by themselves; the next portal edit of any audio row will send the mirror's older
   value straight back. The "Re-read the A32" button refreshes it. Bench use only.
6. The knob-set volume is never itself touched by the settings system, so it only ever reaches
   flash when something else triggers a save, and the knob's own first reading overrides it again
   at every boot regardless.
7. An unreadable sample-mask line is ignored with no note of its own, so every mark line after it
   is then silently skipped. An exported file then produces a misleading refusal naming "0 and 0
   parsed", because its own hand-offset line is what actually triggers that block's check; a file
   without that line gets no word about it at all. The two retired keys are skipped without any
   note at all, by design.
8. Merge, never replace: an upload cannot return any single setting to its default by simply
   leaving it out of the file (except through the two whole-block counters).
9. The file carries no checksum at all; its version line is only a comment, and is never checked on
   import. Old files still work, key by key.
10. The forced, immediate flush does not re-check that a write is currently safe between its own
    retries; a write could in principle start just as the needle begins moving. The window for this
    is extremely small.
11. The portal page itself has no "unsaved" indicator at all; its own state carries the downgrade
    lock, but not whether anything is currently unsaved.
12. No lock exists around the settings structure across the two processor cores — accepted, as
    described above.
13. The console's own full settings dump is only partly pasteable back into a file: its first
    lines each carry several key/value pairs on one line, which the importer refuses outright; the
    tuning, intermediate-frequency and fixed-feature lines do match the file's own format. Its
    learned-WiFi-power line is simply ignored by the importer.
14. Two of the S3's own console keys bypass the shared bounding function entirely: the clock
    brightness keys have their own separate floor and ceiling.
15. The top-speed row's own range (up to 4000 hs/s) allows speeds that have been measured, on this
    particular mechanism, to stall the needle as it passes through the index.
16. When the portal receives a warning, it shows only the warning text, not the accompanying "N
    settings applied" count.

## Ambersong firmware: changing the settings system

### Invariants

- The settings structure is append-only. Every append bumps the settings version and adds a prefix
  migration. The version always comes from the stored header; the stored length only bounds how
  much is read.
- Every row's declared storage type must match the exact C type of the field it points at. All
  access goes through a byte-for-byte copy, because the A32's own structures are packed.
- The shared bounding function is the only bounding rule anywhere. Never add a second copy inside
  a caller.
- "Something changed" must be called *after* every change to a setting that has to persist, from
  either processor core. A change without it is lost the moment power is cut.
- The write function is the only writer of the S3's own blob. Nothing writes during motion, a
  calibration, or an update.
- A32 rows are never pushed before the mirror is actually known; never export raw zeros for them.
- A multi-field calibration must commit as one whole unit or not at all. The two whole-block
  counters are exported even when they are zero.
- Refusals must reach the portal page through the dedicated warning channel, never inside the
  success message.
- Hand-parsed keys use full-consumption number parsing (the whole field must be consumed), never
  the standard library's lenient parser.
- Applying settings must stay cheap and change nothing if it happens to run twice in a row, and
  must write back whatever soft limits were actually accepted.

### Traps

- The original layout is recognised by the **absence** of the magic number. Do not add a size test
  on top of that.
- A new field may land inside an older structure's own trailing padding: the prefix cut must be
  based on that new field's actual byte offset, never on the stored blob's length.
- The fixed-feature slot count (16) is frozen into the layout. The receiver module's own copy must
  equal it exactly; the build itself refuses to compile otherwise.
- The main loop copies the WiFi driver's own transmit power into the stored setting on every pass.
  Any code path that sets that stored value must also push it to the driver itself, or it is simply
  reverted again on the very next pass.
- A retired key must stay in the importer's own skip list, or every file saved by an older firmware
  reports it as refused.
- The two tuner-end values are both portal rows **and** feed the curve fit directly. Setting them
  marks the corresponding end as measured; a file import infers the same thing. The old compiled
  placeholder values mean "not measured".
- The two soft-limit rows are validated by the needle's own geometry check, never by the row's own
  range alone.
- The portal task runs on core 0; anything that busy-waits there freezes the whole portal.
- The console's own settings dump is written by hand; it is not generated from the settings table.
- In the settings file, the sample-mask line must come before its own sample lines, and the
  fixed-feature count before its own fixed-feature lines.
- **A version bump and a rollback.** This trap is closed by design on the S3: an image on trial
  writes nothing at all, so a migrated blob only reaches flash once the new image has confirmed
  itself, and a rolled-back image always finds its own expected layout. Keep the trial check first
  in the write function; any other path that writes the S3's blob would reopen this trap. The A32
  has the same hold since its own trial feature was added: keep its trial check ahead of its own
  write, never write its namespace from anywhere else, and keep its flush inside the confirmation
  step — otherwise changes held during a trial are only ever written by the *next* unrelated
  change.

### How to add an S3 setting

1. Decide what it actually is. A value that persists, is one single scalar, and has one range
   becomes a row. An experiment becomes an action instead (see "Deliberately not settings" above).
   A list, a learned value, or several fields that must commit together, becomes a hand-written key
   instead (step 8 below).
2. Append the new field at the very **end** of the settings structure, with its own default as the
   initializer. That default is exactly what every existing machine receives on its first boot with
   the new firmware. Never insert, reorder, widen or remove a field. (To grow the fixed-feature
   list itself, append a second array instead of widening the first.)
2. Bump the settings version.
4. Add a migration branch for the previous version, alongside the existing ones: compute the byte
   offset of the new field, copy up to the smaller of that offset and what was actually read,
   restore the header, mark everything as changed, and log one line describing the new field's
   starting value. The older branches need no change at all — their own cuts simply stop earlier,
   so the new field keeps its default there too. Never dispatch on size.
5. Add a row to the settings table: a key that will never change again, a label, a unit, which tab
   it belongs to, a storage type matching the field's exact C type, its kind, its range and step in
   the units shown to a person, its scale, the address of the field itself, the S3 as its owner, and
   whether it is administrator-only (true unless the value is both obvious and reversible). The
   row's position in the table is also its position on screen and in the file.
6. Consume it inside the settings-apply function: push it into whichever module actually uses it.
   Setting any S3 row already calls that function automatically.
7. Add it to the console's own settings dump, by hand.
8. For a hand-written key instead of a row: write it inside the export function, after the ordinary
   rows; parse it inside the import function, before the ordinary row lookup, using full-consumption
   parsing; decide whether a bad value should be refused and named, or simply ignored; for a
   multi-field unit, parse into local variables and commit them all at the very end; write its own
   count key even when it is zero; add it to the console dump exactly as the file would write it.
9. Decide how the downgrade lock should treat it. A new row is automatically required by the
   complete-file check, since that check covers every S3-owned row. A hand-written key is required
   only if it is explicitly added to that same check. A file saved by the previous version will
   never have the new row, and so can never unlock a lock on its own.
10. Nothing in the portal page itself needs to change: the schema is generated automatically, and
    its own cache tag changes with the firmware version.
11. Build both firmware environments and test the change (see "How to test a change" below).

### How to add an A32 setting

1. Append the new field at the end of whichever wire structure it belongs to (the audio structure
   or the Bluetooth structure). This changes a structure that travels **across the link itself**: a
   board running the other build will reject every message built from it on length alone, so both
   boards must be flashed together. A build-time check verifies both structures still fit the
   wire-message payload limit.
2. Do **not** bump the wire-protocol version for a field that was only appended. The rule, written
   directly above the version constant in the firmware's own source, is that a structure which only
   changes size needs no bump at all, because the message layer itself refuses a payload of the
   wrong length — so a mismatched pair simply loses that one message type, while the link as a
   whole stays up. Bump the protocol version only for a change the length check itself cannot see
   (the same size, but a different layout or meaning). A bump drops every frame of the other
   version outright, which costs the whole handshake — and, with it, the relayed route an A32
   update normally travels by — and shows up as a dropped-frame count on the console and as a
   portal warning. Update the A32 first, through the S3, while the two sides still agree.
3. The A32's own stored blob for that structure no longer matches its size, so **every** setting in
   it resets to its own default on the first boot of the new image. Download a settings file before
   the update and upload it again afterwards; A32 rows are only pushed once the mirror has actually
   been read. If the update was sent over the air, the upload is applied at once but written only
   once the new A32 image confirms itself, about a minute after its first handshake; should it be
   rolled back instead, the previous image finds its own blobs completely untouched.
4. Give the new field a default inside the A32's own defaults function, and apply it inside the
   A32's own apply function. Add whatever clamp the A32 itself needs when applying it.
5. Add a row to the settings table pointing at the new field inside the mirror, owned by the A32's
   audio or Bluetooth side as appropriate, with a storage type matching the field exactly.

### How to test a change

Without the real machine, where possible:

1. Build both firmware environments.
2. Round trip: download `ambersong.txt`, then upload it unchanged; expect the full count applied,
   and no warning at all.
3. Refusals: upload a file containing an invalid soft limit, an unrecognised key, an out-of-range
   volume, and a natural-frequency value with a decimal comma; expect the first two refused, the
   volume clamped, and the decimal-comma value accepted correctly.
4. Atomic blocks: a sample-mask line claiming three marks with only two actual sample lines present
   should refuse the whole tuning block, naming "3 announced, 2 parsed", with the stored marks left
   unchanged. The same check applies to the fixed-feature count.
5. Soft limits: a low limit set above the index should be refused as "soft limits refused".
6. Save path: change a needle setting while the needle is moving and press Save; expect the motion
   refusal; once it stops, expect "saved". The console should log the save about two seconds after
   the change.
7. Migration: flash the new build over a board already holding the previous version; expect a
   "migrated" log line, and compare a full settings dump before and after. Over the air, expect no
   "settings saved" log line at all until the console reports the image confirmed.
8. Trial hold: right after an S3 update sent over the air, change a setting and press Save; expect
   a message that nothing is saved yet because the firmware is still on trial. A portal reboot
   right after that should answer that it is rolling back to the previous firmware.
9. A32 trial hold: right after an A32 update sent over the air, change an A32 row (a Bluetooth LED
   level is a harmless choice) from the portal; expect "ok", no "settings saved" line on the A32's
   own USB console if one is connected, and, about a minute after the handshake, a log line
   reporting that settings changed during the trial have now been saved, next to the image's own
   confirmation message. Put the value back afterwards.
10. Retired keys: upload a file carrying both of the two retired keys with values; expect no
    refusal and no change from either.
11. Downgrade lock (bench board only): flash an older build, by cable, over a newer stored blob;
    expect the locked indicator, a refused download, a partial upload that leaves the lock in
    place, and a complete upload that lifts it.
12. A32 silent: keep the link down at S3 boot, or cut it after the handshake; expect "n/a" in the
    export, "not sent" on upload, and a refusal on any portal edit of an audio row.

On the real machine: identify the S3 by its unique board ID before any flash, never flash while
anyone is listening to or watching the radio, and keep the firmware toolchain pinned exactly as
recorded (see `firmware-gospel.md`).
