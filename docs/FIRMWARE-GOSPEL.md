# Ambersong — Firmware Gospel

Firmware v.1.0.5 (S3) and v.1.0.4 (A32)

# 0. How to read this Gospel

## 0.1 What this is

Ambersong is a LLOYDS TM-838N valve radio from the early 1960s, rebuilt into a new
cabinet (Bible §31.2) with two small computers inside it. This document is the **Firmware Gospel**: the account
of the software those two computers run — what it does, how it does it, and **why** each choice
was made — written so that someone who has never seen the project can understand it, recover it,
build something similar, and change it safely.

It has a twin. The **Hardware Bible** (`HARDWARE-BIBLE.md`) records what is physically there:
boards, parts, wiring, values. This Gospel records what the firmware does with it. The two do not
state the same fact twice: where the firmware depends on the hardware, this document points to the
Bible by section, "(Bible §11)". The firmware is meant to hold no hardware assumption of its own;
the one exception is the physical values of this particular mechanism (the needle, the dial and
the tuner), which this Gospel carries (§5.9). Code comments state intent; where a comment and the
code disagree, the code is what runs.

## 0.3 Your mileage may vary

Every **Graveyard** section (N.6) records what was tried **here**, what happened **here**, and why
it was dropped **here** — never "this does not work". The retired approaches were dead ends in this
particular setup, not necessarily in someone else's, and every recorded decision is a choice made
for this machine, not a law. If you are building your own, question both.

## 0.4 How the chapters are built

Chapters 0 to 2 are the way in: this page, a rescue page for when something is wrong (chapter 1),
and the whole machine at a glance (chapter 2). Chapters 3 to 10 each cover one part of the firmware
in the same eight sections:

| Section | What it holds |
|---|---|
| N.1 What it does | What you see or hear. Readable alone. |
| N.2 How it works | The design, tasks, data flow, state machines, worked examples. The longest part. |
| N.3 Settings and constants | Every value, with default, unit, range and who can change it. |
| N.4 Decisions | Each choice: what, why, what was rejected, when, and the evidence. |
| N.5 Failures and recovery | What can go wrong, what the firmware does, how you recover, and its class (§0.5). |
| N.6 Graveyard | Retired approaches (§0.3). |
| N.7 Limits and firmware notes | Known limits, paths never exercised on the real radio, open items. |
| N.8 Changing this area | Invariants that must hold, traps, how to test a change. |

Chapter 11 describes the test tools and what they proved. Chapter 12 collects the **firmware
notes**: every known issue small enough to live with, recorded so that if one ever becomes more
than a note there is a starting point.

| Ch. | Title |
|---|---|
| 0 | How to read this Gospel |
| 1 | If the radio misbehaves |
| 2 | The machine at a glance |
| 3 | Building, flashing and versions |
| 4 | The S3: boot, tasks, clock display, panel lamps, time |
| 5 | The dial needle |
| 6 | Tuning and the dial's self-calibration |
| 7 | Settings |
| 8 | Network, web portal and console |
| 9 | The A32 and the link between the boards |
| 10 | Sound: sources, Bluetooth, and the pop hunt |
| 11 | The test tools |
| 12 | Firmware notes |

## 0.5 The recovery ladder

Failures are classed by what it costs to recover from them. Every failure table uses this ladder:

| Class | Meaning |
|---|---|
| **BLOCKER** | The radio is unusable until a full power cycle, or the harm survives one: the needle driven into a stop, a calibration or the settings corrupted and saved, a wrong sample stored, a board bricked or reachable only by USB. |
| **DEFECT** | Recovers with a portal reboot, a console reset, or the front power switch. |
| **NOTE** | Recovers by itself, or is cosmetic. |

## 0.6 Conventions

- **The two computers.** The **S3** is the main board (ESP32-S3): clock, needle, lamps, WiFi and
  the portal. The **A32** is the audio board (ESP32): sound, Bluetooth and the real-time clock.
- **Code references** name the function and the file — `homingTick()` in `src/s3/needle.cpp` —
  never a line number, because line numbers move.
- **Decisions** carry the date they were made, for example (2026-09-25).
- **"The author"** is the person who built this machine.
- **Versions.** A build is stamped `v.1.YYYYMMDDTHHMMSS (hash)`: the release number, the build
  time, and the git commit it was built from, with `-dirty` if the code had uncommitted changes
  (§3). The release described here is the S3 at git tag `v.1.0.5` and the A32 at
  tag `v.1.0.4`.

---

# 1. If the radio misbehaves

This page is for the moment something is wrong and you remember nothing about how the machine
works. Find your symptom and try the steps in order, cheapest first. Stop as soon as the radio
behaves. Each paragraph ends with a pointer to where the full story is told.

**First, the machine.** It has two small computers. The **S3** runs the clock, the dial
needle, the panel lamps and the WiFi web page (the **portal**). The **A32** runs the sound, Bluetooth
and the battery-backed clock chip. Both are powered whenever the set is plugged in, and the front
switch only turns the amplifier on and off, so it restarts neither computer (Bible §10, §21). A
"full power cycle" therefore means pulling the plug (§4.2.3, §9.5).

**The recovery ladder, cheapest first:** wait (most faults heal by themselves); press a portal
button; reboot one board from the portal; switch the front switch off and on; unplug the set; and
last, the USB cable. **Before you unplug,** switch the amplifier off and let the needle fall to the
low end of the dial, because a power-up assumes it is there. **Before you change any calibration,**
download the settings file from the System tab: it is the only undo (§4.2.14, §5.5, §7.1).

## 1.1 A console without a cable

The S3 has a text console: it prints what it is doing, and every key you type is a command. Sign in
to the portal as the administrator and open the **Console** tab; no cable is needed. Lines from the
A32 arrive there prefixed `[A32]`. Send **one key at a time**: every character is its own command,
so the portal refuses anything longer, except while a prompt is waiting for a line (§8.2.9).

| Key | What it prints |
|---|---|
| `s` | The status: amplifier, source, needle, index, encoder, tuning chain, portal counters, the link to the A32 (`link : alive` or `SILENT`, frames received, checksum errors, frames of the wrong protocol version), the last reset reason, uptime, and the `image` line (update trial and rollback). It ends with the command list. |
| `i` | The network: joined or "OWN ACCESS POINT", name, address, signal, portal sessions, network time fresh or stale, access-point clients, and what the WiFi driver really holds. |
| `?` | The list of every console key (all of them are in §8.11). |

Attaching does not restart the S3, so its start-up banner is never seen; press `s` (§3.2.7).

## 1.2 The portal will not open

1. **Try both addresses**, the house address and `http://ambersong.local/`. A page that says
   "no answer from the radio." retries by itself after 2, 4 and 8 s, then every 15 s (§8.2.7).
2. **Wait for the needle.** The portal is starved while the needle moves (§8.2.1).
3. **Stop all traffic.** About twenty to forty quick connections exhaust the radio's connection
   records; it then stops answering while ping still works. Close every tab open on the radio,
   wait one to two minutes, and try once (§8.2.7, §8.5).
4. **Look for the rescue access point.** When the S3 cannot reach the house network it raises its
   own WiFi network, **`Ambersong`**, and serves the same portal at **`http://192.168.4.1/`**.
   It comes up by itself: at once, and for good, on a board with no house network stored; 20 s
   after a failed attempt to join; 20 s after the house link drops. **It asks for no password**:
   join it, and on most phones the portal's sign-in page opens by itself. If it does not, open
   `http://192.168.4.1/`. At most two devices can join. A blank board transmits at the lowest
   power, so stand close. The network is open, so what you type there crosses the air unencrypted
   until the radio goes home (§8.2.2, §8.2.3, §8.2.6, §8.5).
5. **Signing in fails.** Sessions end after 5 idle minutes. After five wrong passwords a device is
   locked out for 1 minute, then 2, 4 and so on up to 64. A lost owner password cannot be reset
   from the portal: the USB console key `~` erases **all** accounts and restores the shipped
   placeholder account (§8.2.6, §8.5).
6. **Forcing the access point.** The radio has no configuration buttons. From a portal that still
   answers, press **Raise the rescue access point** (System tab), or send `Y`. It holds for 10
   minutes, longer while someone is connected, then the S3 goes home by itself (§8.1, §8.2.3).
7. **Nothing works, but the radio plays.** Nothing watches the portal task or the WiFi stack, so a
   hang there leaves the radio, clock and needle working and the portal dead. Unplug the set. If
   no access point appears and the house network is not joined either, see §1.11 (§4.5, §8.5).

## 1.3 Getting back to the house network

- **After a forced access point:** wait out the 10 minutes, or disconnect your phone from
  `Ambersong`. The S3 retries the house network every 2 minutes, but **never while someone
  is connected to its access point**. After "WiFi off for 2 minutes", it returns by itself (§8.2.3).
- **After a new router or password:** on the access point, System tab, Network card, type the WiFi
  name and password (blank keeps the stored password) and press **Save network**. The button stays
  disabled until the form has been filled from the radio. Three seconds later the S3 drops the
  access point and joins; put your phone back on the house network (§8.2.2, §8.2.8).
- **Without a browser:** console `y`, then the name, a space and the password on one line. The
  prompt echoes the password into the console log (§8.2.9).

## 1.4 The needle says FAULT, or points to the wrong place

**First, check that it is really wrong.** The needle rests at the low end when the amplifier is
off, when the source is not RADIO, and when the link to the A32 is down (the S3 then takes the
source as AUX). It stays pinned at an end when the knob is past the printed scale. It stops, with
the clock dark, during an S3 update. Two minutes after the amplifier goes off it quietly re-checks
its reference point (§4.2.6, §5.1).

**The pills** at the top of the portal:

- **NEEDLE OFF - re-index pending (try n/3):** it lost steps. It re-indexes by itself once the knob
  has been still for 3 s. **NEEDLE RE-INDEXING:** that repair is running. Do nothing (§5.2.14).
- **NEEDLE FAULT:** three automatic repairs failed, or a homing started by hand or at boot failed.
  The motor is released and the needle stays put. The reason is on the console (§5.2.9, §5.5).
- **NEEDLE HUNTING:** it keeps stepping without going anywhere. Only a warning; **STOP** (Needle
  tab) or console `x` stops it (§5.2.21).

**Clearing a FAULT, cheapest first:** **Home** (or **Re-index**) on the Needle tab; console `H`; or
the amplifier off and on, since the front switch starts a homing when the needle has no reference
point. If homing fails again with "index never found in either direction" or a "sensor never ..."
message, the console suggests checking the index switch, its magnet, its pull-up and the motor:
that is hardware (Bible §11, §31.4) (§4.2.7, §5.5).

**Off by the same amount at every station, with no pill.** Press **Re-index**. If it is still off,
compare the frequency on the Now tab with what you hear. If the frequency is right, the face
mapping is off: **Needle Limit ◀ 20 / 20 ▶** slides where the needle points for every frequency.
If the frequency is wrong, see §1.5 (§5.2.11, §5.2.16).

## 1.5 The frequency on the dial is wrong

The knob's angle becomes megahertz through a curve fitted to up to twelve samples: three **hand
marks** that you type, and nine **RDA samples** that a receiver chip inside the cabinet (an
RDA5807M) measures by hearing the tube set's own local oscillator. A wrong sample, once stored, is
saved, so download the settings file first. All the buttons are on the Needle tab (§6.1, §6.2.5,
§6.5).

1. **Measure.** Switch the amplifier on, select RADIO, hold the knob still and press **Measure this
   dial position**. The result is stored only if it passes every check; a refusal says why ("Move
   the dial 0.3 MHz", "NOTHING FOUND ... nudge it", "the dial calibrates only while you listen to
   the radio ..."). When all nine RDA slots are full, a new sample replaces the most crowded one, or
   is not stored if it "would add the least coverage"; hand marks are never replaced. The machine
   also measures by itself, at most every five minutes, while the radio plays and the knob has been
   still for 8 s (§6.2.8, §6.2.12).
2. **Mark or slide.** Tune to a station you know and press **Mark station A here** (or B, C), then
   type its frequency. **Dial −0.1 MHz** / **Dial +0.1 MHz** move every reading together. That hand
   correction is never erased by itself: **Reset the dial correction to 0** is the only way back
   (§6.2.12).
3. **Remove one bad sample.** Console `s` lists every sample with its residual (its gap from the
   curve), meaningful from four samples on. Press **Drop one sample** with its slot (§6.5).
4. **"tuner ends NOT measured"** in the tuning sentence: knob fully down, **Tuner = low end**; knob
   fully up, **Tuner = high end** (§6.2.5, §6.2.12).
5. **Every new sample is off by the same amount since the tube set was realigned:** its IF changed.
   Set **Tube set IF**, then drop or re-measure the old samples. To start over, **Clear all marks**
   returns to the stored straight line; it keeps the hand correction (§6.2.12, §6.5).

## 1.6 No sound, the wrong source, or Bluetooth misbehaving

**No sound, cheapest first.** On the Now tab: is the amplifier on? The A32 plays nothing while it is
off. Is the source the one you expect? **AUX never passes through the A32**, so the A32 is silent on
AUX by design. Check mute and volume; a volume set from the portal holds until the front knob moves.
Is the **link** dot green? The A32 goes quiet within 2 s of losing the S3 (§1.7). During an A32
update the sound stops for about a minute. The experiment buttons on the System tab are never saved,
and **Reboot the audio board** undoes them (§9.2.6, §10.1, §10.2.7, §10.3.3).

**Still silent:** if the console shows `[A32] proto self-test FAILED - muted, BT closed, awaiting
OTA` every 10 s, see §1.8. Otherwise press **Reboot the audio board**; it clears a stuck audio task
(§10.5).

**The wrong source.** Only the A32 reads the front selector, every 50 ms; the S3 learns the source
from it four times a second, and shows AUX while the link is down (§4.2.6, §10.2.8).

**Bluetooth is not visible.** A phone can connect only on the BT source, with the amplifier on and
the A32 awake. A phone that paired before must reconnect **from the phone**; the radio never calls a
phone. A new phone needs the pairing window: the front pair button (on BT only) or **Pair** on the
Now tab. It lasts 90 s (`lookTimeoutS`), with the blue lamp blinking regularly, and it first drops
any phone already connected. With `connectable` off (Bluetooth tab), Bluetooth is closed (§10.2.10,
§10.5).

**Bluetooth is visible when it should not be.** The radio should appear in a phone's list of new
devices only during the pairing window; anything else is a defect. **Reboot the audio board**
rewrites the visibility at start-up; **Forget every pairing** removes all stored phones (§10.2.10).

## 1.7 The console says "A32 silent"

`[WARN] A32 silent. Clock and needle keep running.` means the S3 has heard nothing valid from the
A32 for 2 s. The clock keeps time and the needle parks. The S3 calls the A32 every second and prints
`[PASS] A32 up: proto vN, firmware ...` when it answers. Most silences heal themselves (§4.5,
§9.2.10).

- **Read the link line** of `s`: `link : SILENT rx N crc N wrong-version N`. A **wrong-version**
  count above 0, with the pill **BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both**, means the
  two programs speak different protocol versions: update both boards. Otherwise a receive count
  that does not move points at wiring or power (Bible §12), and receive and checksum counts
  climbing together point at signal quality (§9.2.7, §9.7).
- **Right after an A32 update:** a new program that never completes the handshake stays on trial;
  unplug the set and it rolls back (§9.5).
- **AUDIO BOARD SENDS NO STATE**, or "Flash BOTH MCUs" on the console: the builds are incompatible.
  The sound is unaffected; update the older board (§4.2.14, §9.5).
- **Reboot the audio board** travels over the link and cannot reach a silent A32. A frozen A32
  restarts itself within 15 s (§9.1, §9.2.5).

## 1.8 An update went wrong

**Every new program runs on trial.** After an over-the-air update a board confirms its new program
only after about a minute of healthy running: the S3 70 to 90 s after start-up, once its network
and portal are up (`image confirmed (a minute of running, network and portal up).`); the A32 a
minute after its first handshake with the S3. If the board resets for any reason before that, a
crash or a 15 s hang included, it goes back to its previous program by itself (§3.2.9, §4.2.15,
§9.2.13).

| What you see | What it means | What to do |
|---|---|---|
| The page reported failure | The restart can beat the answer | Read "running" on the System tab (S3), or `[PASS] A32 up: ...` on the console (A32) (§3.2.10) |
| HTTP 409 "this firmware is still on trial - retry in a minute", or "A32: still on trial - retry in 1 min" | That board is in its trial minute | Wait for `image confirmed`, upload again (§8.2.10, §9.2.13) |
| `- an earlier update was ROLLED BACK` ending the `image` line of `s`, or the `[A32] boot:` line | The new program failed its trial; the board runs its previous one | Read the reset reason, fix the build, upload again (§3.2.9) |
| `[FAIL] proto self-test - this image will NOT be confirmed` | A broken S3 build, running | Sent over the air: **Reboot the main board** rolls it back. Flashed by USB: upload a good build (§4.2.15) |
| The new program misbehaves, still on trial | Not yet confirmed | Reboot that board from the portal: it rolls back. During the trial **Reboot the main board** goes through at once, even over an unsaved change, and the previous program comes back (§4.2.15, §9.2.13) |
| "NOT saved yet - this firmware is on trial after an update ..." | The S3 writes no settings during its trial | Changes apply at once and are written once the program is confirmed, about a minute later (§7.2) |
| The new program misbehaves after `image confirmed` | It is permanent now | Upload a good one through the portal (§4.5) |

An interrupted upload changes nothing: the S3 gives up after 15 s without data, the A32 after 20 s,
and the old program stays. Other messages ("the A32 is not answering", "did not confirm", a wrong
chip) are explained in §3.5. Because the S3 saves nothing during its trial, a rollback finds the
settings exactly as the previous program left them. A change made during the trial is lost with
it (§3.2.9, §7.2).

## 1.9 The clock shows the wrong time

- **Blank digits:** no valid time. Normal for a second or two after the S3 starts. If it lasts, the
  clock chip has no valid time and there is no network time: join the house network, or set it with
  console `W` (local time, `YYYY-MM-DD HH:MM:SS`) (§4.2.9, §4.5).
- **`1017` or `0879`:** a frequency (101.7, 87.9 MHz), chosen by `showTuning` on the Display tab.
  **`8888`**, or dark while playing: a display test; press **Normal** on the Display tab, or `9`
  (§4.2.9).
- **Off by whole hours:** the time zone is a POSIX rule on the Network card; the default is eastern
  North America, `EST5EDT,M3.2.0,M11.1.0`, so set your own. The `W` prompt always converts with that
  eastern rule and leaves it in force until the rule is applied again (a restart, or saving the
  Network card) (§4.2.13).
- **BATTERY CLOCK LOST ITS TIME - check its battery** (pill): the clock chip answered with no valid
  time, which most likely means its backup battery or the module is failing. Set the time (network
  or `W`); the pill clears at the next good reading. **BATTERY CLOCK NOT ANSWERING**: the A32 is
  up but the clock chip did not answer (Bible §5) (§9.2.14).
- **Off by minutes:** the clock chip keeps the time, and network time corrects it hourly while the
  **time** dot is green. Without a network, use `W`. **12- or 24-hour:** `hour12`, Display tab
  (§4.2.13, §7.3.2).

## 1.10 Settings look lost, or "SETTINGS LOCKED"

- **SETTINGS LOCKED - newer version stored:** this program is older than the one that saved the
  settings. It runs on defaults, writes nothing and refuses the download. Flash the newer program
  again, or upload a **complete** settings file taken before (§7.2.7).
- **Audio settings back to defaults after an A32 update:** the update changed their layout. Upload
  a settings file exported before the update (§7.5).
- **Audio rows show `n/a`, or edits are refused with "the audio board is not answering - its
  settings cannot be changed now":** the S3 holds no current copy of the A32's settings, because
  the A32 has not answered since the S3 started or has gone silent. Wait for the handshake, or press
  **Re-read the A32** (Audio tab) (§7.5, §9.2.11, §10.2.12).
- **Needle limits back at ±300:** the stored pair was corrupt and was purged. Home, calibrate the
  index and set the limits again (§5.2.16, §7.5).
- **Mute off after a restart** is by design. **Accounts and the WiFi network** are not in the
  settings file, and a restore does not touch them (§8.2.6, §10.2.7).

## 1.11 When you need the USB cable

Few faults need it: no access point and no house network; a lost owner password (`~`); a confirmed
A32 program that hangs at every start before the link comes up; too little free memory on the A32
for an update (§8.5, §9.5, §10.5).

**Where:** each board has its own USB-C socket on the back panel (Bible §4). **Safety:** read Bible
§10, §21 and §30.4 first. The USB ground is the machine's DC-side ground, which is on earth (Bible
§30.4), not the tube chassis. **Identify the board
by its MAC address before you write to it**, and never let PlatformIO choose the port. The S3's
console is its native USB port at 115200 baud (§3.2.6, §3.2.7, §8.2.9).

## 1.12 What the lights and pills mean

| Where | What you see | Meaning |
|---|---|---|
| Portal pill `link` | green dot | The S3 and the A32 have shaken hands (§4.2.14) |
| Portal pill `needle` | green dot | The needle has found its reference point (§5.2.23) |
| Portal pill `time` | green dot | Network time synced within the last 4 hours (§8.2.5) |
| Portal pills | `--:--`; network name and dBm, or `access point` | No valid time; house network, or the rescue access point (§4.2.13, §8.9) |
| Red pills | NEEDLE OFF / RE-INDEXING / FAULT / HUNTING | See §1.4 |
| Red pills | AUDIO BOARD SENDS NO STATE; SETTINGS LOCKED; UPDATING | §1.7; §1.10; an upload is running (§8.9) |
| Red pill | BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both | The two programs do not match; update both (§1.7) |
| Red pills | BATTERY CLOCK LOST ITS TIME - check its battery; BATTERY CLOCK NOT ANSWERING | See §1.9 |
| Clock | blank | No valid time, or an S3 update is writing (§4.2.9, §4.2.12) |
| Panel lamps | dark | Amplifier off, or an S3 update is writing (§4.2.11) |
| Panel lamps | full / half / low | RADIO, knob moving / RADIO, knob still 5 s / AUX or BT (§4.2.11) |
| Blue lamp | dark | Not on BT, or the A32 is asleep (§10.2.10) |
| Blue lamp | fast breath, then slow breath | BT source, waiting for a known phone (§10.2.10) |
| Blue lamp | regular blink | Pairing window open: visible to new phones (§10.2.10) |
| Blue lamp | double flash, then steady bright | A phone connected (§10.2.11) |
| Blue lamp | steady dim | Hung up from the portal (§10.2.10) |

---

# 2. The machine at a glance

This chapter is the map. Read it once before the chapters that follow: it names the parts, shows where
each piece of firmware runs and how information moves between them, lists every source file, and
tells you which chapter to open for what. It states nothing that a later chapter does not explain in
full, and each paragraph ends with a pointer to that place.

## 2.1 What the machine is

Ambersong is a cabinet radio built around a tube radio, the LLOYDS TM-838N, which still
does the receiving (Bible §0, §13 to §21). Its original tuning knob still turns the tube set's own
tuning capacitor. Around it, two small computers add what the old set never had: a four-digit amber
clock behind the dial glass (the **Leditron**), a dial needle driven by a stepper motor instead of a
cord, panel lamps that follow what the listener does, Bluetooth music from a phone, a battery-backed
clock, and a password-protected web page (the **portal**) that shows and changes everything. The
radio has no configuration buttons; once the cabinet is closed, the portal is the only way in. What
is physically fitted and wired is the business of the Hardware Bible, not of this Gospel (§4.1,
§5.1, §6.1, §8.1).

What a listener meets, from the front:

- **The front switch** turns the amplifier on and off. It does not start the computers, which run
  whenever the set is plugged in (Bible §10, §21) (§4.2.3).
- **The source selector** picks RADIO (the tube set, digitised and played back), BT (a phone) or AUX
  (an input that never passes through the A32; the amplifier sums its inputs, Bible §22, §30.1)
  (§10.1, §10.2.2).
- **The tuning knob** tunes the tube set; the needle follows it electrically (§2.1.1, §5.1).
- **The volume knob** is read as a position sensor and sets the digital volume (§10.2.7).
- **The pair button and the blue lamp** open and show the Bluetooth pairing window (§10.2.10,
  §10.2.11).

### 2.1.1 How the needle and the tuning are tied together

Nothing mechanical joins the tuning knob to the needle (Bible §31.4). The knob turns the tube set's
ganged tuning capacitor (Bible §13, §14); the needle is moved by a stepper motor (Bible §11). The
needle follows the firmware, and the firmware is the only link between the two. Two sensors on the
S3 play two different parts:

- **The AS5600 is the live measure.** This magnetic angle sensor reads the angle of the tuning
  capacitor's shaft (Bible §11, §31.4), every 20 ms. It tells the firmware that the tuning changed, and
  where the shaft is. Every frequency the machine shows (on the Leditron and on the portal), and
  every position the needle takes while it follows the tuning, comes from that angle, through the
  calibration curve (§6.2.4).
- **The RDA5807M calibrates the curve.** This small FM receiver chip listens to the tube set's
  local oscillator. The firmware adds the set's intermediate frequency (IF), 10.6 MHz (Bible §22),
  to the oscillator's frequency to get the station (6.4.2), and stores the pair (shaft angle,
  station) as a calibration sample. The curve is fitted through the samples, so the measurements
  calibrate the AS5600's positions (§6.2.5). A measurement changes the curve; it is never shown as
  the frequency and never moves the needle by itself.
- **If the AS5600 stops answering, nothing takes its place.** The shaft count freezes: the needle
  stops following the knob and holds its last target, the portal's "Now" frequency goes blank, a
  Leditron set to show tuning all the time keeps the last value, and no measurement is started or
  stored. When the sensor answers again, the angle is placed back on the right turn and everything
  follows again (§5.2.17, §6.2.4).

Chapter 5 covers the needle, and chapter 6 the curve and how the RDA5807M measures (§6.1).

## 2.2 Two boards, and why two

| | **The S3** | **The A32** |
|---|---|---|
| Chip | ESP32-S3 (Bible §1) | classic ESP32, an ESP32-WROOM-32 (Bible §1) |
| Owns | the Leditron, the panel lamps, the needle and its motor, the tuning-angle sensor (AS5600), the RDA5807M receiver used as a dial instrument, amplifier sensing, WiFi, the portal, network time, firmware updates for both boards | all the audio: the tube set's audio in (ADC), the audio out (DAC), Bluetooth, the source selector, the volume knob, the pair button and blue lamp, the DS3231 clock chip |
| Sources | `src/s3/` | `src/a32/` |
| Keeps in its flash | settings for the display, lamps, needle and tuning; the WiFi network; the portal accounts | settings for the audio and Bluetooth; the volume knob's calibration |

**Why two.** Phone music needs Bluetooth *Classic* (the A2DP profile), and the ESP32-S3 does not
have it. So the audio runs on a classic ESP32, and everything else on the S3 (§10.1). The split also
keeps two strict timings apart: on the A32, Bluetooth audio with the classic ESP32's legacy I2S
driver; on the S3, a needle step emitter at priority 19 that must not be starved. Neither tolerates
the other's timing (§4.4).

**The consequences.** The A32 has no road to the outside except through the S3: no WiFi portal of
its own, and no reset wire from the S3 (Bible §2, §3). Its firmware updates are relayed by the S3
over the link between them. The clock chip sits on the A32 (Bible §5), so the time of day crosses
the link too (§9.1, §9.2.1).

**One project, two programs.** A single PlatformIO project builds both. The pin map (`pins.h`) and
the protocol between the boards (`proto.h`, `link.h`) are one set of files compiled into both
programs, so the two cannot disagree about them by accident (§3.2.1).

**Each board looks after itself.** Both run a 15 s task watchdog that turns a hang into a restart.
Both run a program freshly updated over the air "on trial", and return to the previous one by
themselves if it resets before it has proved itself. And each has its own rule for the other's silence: the A32 goes
quiet (no sound, Bluetooth closed) within 2 s of hearing nothing from the S3, while the S3 keeps its
clock and needle running and keeps calling (§4.2.15, §9.2.5, §9.2.6, §9.2.13).

## 2.3 What runs where

```
   browser (portal page, Console tab)
        |  HTTP, port 80, over the house WiFi
        |  or the rescue AP "Ambersong" at 192.168.4.1
        |  (open; a captive portal while it is up)
        v
  +- S3 (ESP32-S3) --------------------------------+
  | core 0                                         |
  |   WiFi and TCP/IP stack; SNTP time             |
  |   portal (prio 3): HTTP server, accounts,      |
  |     OTA endpoints, web console                 |
  |   nstep (prio 19): step emitter, 1 kHz tick    |
  | core 1                                         |
  |   display interrupt every 25 us (Leditron)     |
  |   loopTask (prio 1): link, amp sense, clock,   |
  |     brightness, panel lamps, time, console,    |
  |     RDA and re-index triggers                  |
  |   needle (prio 4): supervisor every 5 ms,      |
  |     AS5600 angle every 20 ms                   |
  |   rda (prio 2): RDA5807M sweeps                |
  | peripherals: Leditron, panel lamps (PWM),      |
  |   stepper drive (4 PWM), index switch,         |
  |   AS5600, RDA5807M, amp sense, USB console     |
  +------------------------------------------------+
        |
        |  the link: one UART, 921600 baud, framed, CRC16-checked, protocol v4
        |    S3 -> A32 : HELLO, PING, SET_SYS (amp on/off), GET_TIME, SET_TIME,
        |                settings, firmware updates (relayed)
        |    A32 -> S3 : HELLO_ACK, STATE every 250 ms, TIME, CFG, LOG
        v
  +- A32 (classic ESP32) --------------------------+
  | core 0                                         |
  |   Bluedroid: Bluetooth, SBC decoding           | <-- phone (Bluetooth
  |     -> ring buffer (BT samples)                |     Classic, A2DP)
  | core 1                                         |
  |   audio task (prio 6), one block at a time:    |
  |     RADIO: ADC -> I2S in                       |
  |     BT:    ring buffer                         |
  |     AUX:   zeros                               |
  |     -> gain, fade, volume -> I2S out -> DAC    |
  |   loop (prio 1): link, sleep and wake,         |
  |     selector and volume knob every 50 ms,      |
  |     Bluetooth state and blue lamp,             |
  |     STATE every 250 ms, update receiver        |
  | peripherals: DS3231 clock chip, selector,      |
  |   volume knob, pair button, blue lamp          |
  +------------------------------------------------+
        |
        v
     DAC -> amplifier <- AUX input (never through the A32)          (Bible §30)

   tube set's local oscillator ~~ leaks a little ~~> RDA5807M on the S3   (Bible §11, §14, §29)
```

The S3's timing-critical work lives on core 1 (the display interrupt, `loop()`, the needle
supervisor); WiFi, the portal and the needle's step emitter share core 0. The step emitter spins at
priority 19 while the needle moves, so the portal is slow during a move; this is known and accepted
(§4.2.4, §5.2.2, §8.2.1).

On the A32, Bluetooth owns core 0 and the audio task runs on core 1 at priority 6, above `loop()`,
with no delay of its own: it is paced by the audio hardware. The audio path is set up once at boot
and never reinstalled; the Bluetooth library hands decoded samples to the firmware instead of
driving the audio output itself (§10.2.3, §10.2.5).

The tube set's local oscillator is the one radio signal that matters to the firmware: the RDA5807M
listens for it. The oscillator runs below the station (Bible §14, §22), so
the station the tube set is tuned to is that oscillator plus the set's intermediate frequency. The
IF is 10.6 MHz (Bible §22), and the firmware's IF setting defaults to 10.60 MHz (§6.2.1).

## 2.4 How information moves

**Tuning: knob to needle and to megahertz.**

```
 tuning knob -> capacitor shaft -> AS5600 angle (0..4095 per turn)
   -> multi-turn count (needle task, every 20 ms)
   -> tuning curve: count -> frequency, plus the hand offset
        |-> Leditron (when set to show tuning), portal "Now"
        '-> needle: printed face dialLow..dialHigh mapped onto soft limits posMin..posMax
              -> supervisor plans a move -> step emitter -> drive (4 PWM duties) -> stepper
```

The needle has no end switches. One index switch part-way along the travel is its only reference;
it "homes" on it once at boot and corrects its step count at every crossing. The curve is fitted to
up to twelve samples: three hand marks typed by hand and nine measured by the RDA5807M, which
takes one by itself at most every five minutes while you listen to the radio and the knob is
still. Once the nine are full, a new measurement replaces the most crowded automatic one, so the
samples stay spread across the dial; hand marks are never replaced. A hand correction (the
portal's Dial ±0.1 MHz) slides the whole curve and is never erased automatically (§5.1, §5.2.1,
§6.2.4, §6.2.8).

**The front switch.** The S3 reads the amplifier's state, debounced over 50 ms. A change sets the
clock brightness (full or standby), the panel lamps, and the needle's job: it tracks the knob when
the radio is "live" (amplifier on and source RADIO), and otherwise falls to the low end and parks.
The S3 tells the A32 at once, and again every 2 s. The A32 is awake only while the amplifier is on
**and** the S3 is talking (§4.2.7, §9.2.6).

**The source.** Only the A32 reads the selector, every 50 ms. It fades the old source out (150 ms)
and the new one in (1 s), opens or closes Bluetooth, and reports the source to the S3 in its STATE
frame four times a second. The S3 uses that report for the needle, the lamps and the tuning readout.
While the link is down, the S3 takes the source as AUX (§4.2.6, §10.2.8, §10.2.10).

**Time.** The DS3231 on the A32 holds UTC. The S3 asks for it at every handshake and every 60 s and
keeps its own software clock between answers, so a slow link never makes the display stutter. When
the S3 is on a network, SNTP corrects the S3 and, once an hour, the DS3231 through the link. The time
zone is applied only for display, on the S3. If the DS3231 answers with no valid time, or does not
answer, a portal pill says so (§4.2.13, §9.2.14).

**Settings.** Each board stores its own in flash (NVS). The S3 keeps a read-only mirror of the A32's
settings so the portal can show them; it drops that copy when the A32 goes silent, and refuses
edits to the A32's settings until the next handshake. A change on the portal applies at once and is
written about two seconds later, once the needle is still; an A32-owned change travels over the link
and the A32 saves it. Neither board writes its settings while a freshly updated program is on trial:
changes wait in RAM and are written once the program is confirmed, so a program that rolls back never
finds a settings layout newer than it knows (S3 since v.1.0.1, A32 since v.1.0.4). On both boards only
a write that reached flash counts as saved. The whole set can be downloaded and uploaded as one text file, which is the backup.
WiFi and the portal accounts are stored apart and are not in that file (§7.1, §7.2.3, §9.2.11).

**Updates.** The browser uploads a program to the S3. For the S3 itself, the S3 writes it into its
other program slot and restarts; the needle stops and the clock goes dark meanwhile, while the sound
carries on. For the A32, the S3 relays the file over the link in 1 KB acknowledged frames, and the
sound stops while the A32 writes. Either board then runs the new program on trial until it confirms
it, about a minute later; a reboot from the portal during that minute brings the previous program
back. The two programs must share one protocol version; if they do not, a portal pill says so
(§3.2.9, §9.2.12, §9.2.13).

**The console.** Everything the S3 prints goes both to its USB port and into a 16 KB ring that the
portal's Console tab reads; keys typed there are fed back as if typed on USB. The A32's messages
reach the same console through the link, prefixed `[A32]` (§8.2.9, §9.2.4).

## 2.5 The source tree

The firmware lives in `firmware/`. This Gospel describes the S3 at the git tag `v.1.0.5`
(2026-09-27), which changed only the S3, and the A32 at the tag `v.1.0.4`
(2026-09-26), which changed only the A32. The release line began with `v.1.0` on
2026-09-25; `v.1.0.1` and `v.1.0.2` followed the same evening, and `v.1.0.3`
the next morning (§3.2.10). Besides a README and a `.gitignore`, these are all of its files; `src/s3/` builds
into the S3's program and `src/a32/` into the A32's, while `include/` is compiled into both.

| File | What it holds | Read |
|---|---|---|
| `platformio.ini` | The two build environments, `s3` and `a32`; the pinned platform and the pinned Bluetooth library | §3.2.1, §3.2.2 |
| `include/pins.h` | Every pin number either firmware uses | §4.3.5 |
| `include/proto.h` | The protocol between the boards: `PROTO_VERSION`, message types, payload structures, the frame encoder and decoder | §9.2.8, §9.3 |
| `include/link.h` | The `Link` class over the UART, the 2 s liveness rule, and `protoSelfTest()` | §9.2.7 |
| `src/s3/main.cpp` | The S3's `setup()` and `loop()`: the settings struct and its storage, the tuning-curve fit, the handler for the A32's frames, the A32 update relay, the portal's live state, the RDA and re-index triggers, the trial and watchdog, the console keys and prompts | §4.2.5 |
| `src/s3/needle.cpp`, `needle.h` | The needle's two tasks: homing, tracking, parking, sweeps, index sensing, band calibration, AS5600 reads, the memory kept across a software reset | §5.2.2 |
| `src/s3/drive.cpp`, `drive.h` | Turns a needle position into four PWM duties | §5.2.4 |
| `src/s3/display.cpp`, `display.h` | The Leditron's multiplexing interrupt | §4.2.8 |
| `src/s3/panel.cpp`, `panel.h` | The panel lamps' PWM and fades | §4.2.11 |
| `src/s3/rda.cpp`, `rda.h` | The RDA5807M as a dial instrument, and its task | §6.2.6 |
| `src/s3/net.cpp`, `net.h` | WiFi state machine, rescue access point and its captive-portal DNS server, transmit-power ladder, SNTP, mDNS | §8.2.3 |
| `src/s3/portal.cpp`, `portal.h` | The HTTP server task: accounts, sessions, every endpoint, firmware uploads | §8.2.6, §8.9 |
| `src/s3/console.cpp`, `console.h` | `Con`: the console tee to USB and to the ring the portal reads | §8.2.9 |
| `src/s3/settings_api.h` | The settings-table row type and the calls the portal uses, so the portal never touches the settings struct | §7.2.1 |
| `src/s3/settings_table.h` | The table of every setting, the portal's actions (`doAction()`), the settings-file export and import | §7.2.1, §8.10 |
| `src/s3/page_gz.h` | The portal page, gzipped; produced from `data/portal.html` at every S3 build (rewritten only when the page changed) and kept in git; never edit it by hand | §8.2.8 |
| `src/a32/main.cpp` | The A32's `setup()` and `loop()`: the link handler, sleep and wake, the selector and volume knob, the Bluetooth policy and states, its settings, the update receiver, trial and watchdog, the boot report | §9.2.2, §10.2.10 |
| `src/a32/audio.cpp`, `audio.h` | The audio engine: I2S installed once, the audio task, gains, volume, fades, mute, the Bluetooth ring | §10.2.3 to §10.2.7 |
| `src/a32/btled.cpp`, `btled.h` | The blue lamp's patterns | §10.2.11 |
| `src/a32/rtc.h` | Reading and writing the DS3231 | §9.2.14 |
| `data/portal.html` | The portal page's source: one file, inline style and script | §8.2.8 |
| `scripts/version.py` | Stamps each build with `FW_VERSION` and `FW_COMMIT` | §3.2.3, §3.2.10 |
| `scripts/page.py` | Checks the page's script, gzips the page and writes `page_gz.h` | §8.2.8 |

## 2.6 Words used everywhere

| Word | Meaning |
|---|---|
| **S3**, **A32** | The main board (ESP32-S3) and the audio board (classic ESP32) (§2.2) |
| **the link** | The serial line between the two boards (§9.2.7) |
| **portal** | The S3's web page; **Console tab**, its copy of the S3's console (§8.1) |
| **console** | The S3's text console: every key is one command (§8.11) |
| **rescue access point** | The S3's own WiFi network, `Ambersong`, raised when it cannot reach the house network; open (no password), and a captive portal: most phones open the sign-in page by themselves (§8.2.3) |
| **amp on**, **radio live** | The amplifier is powered; amplifier on and source RADIO (§4.2.7) |
| **homing**, **index**, **re-index** | Finding the needle's reference switch; the switch; homing again to repair lost steps (§5.2.9, §5.2.14) |
| **hs** | Half-steps of the needle's motor, the unit of needle position (§5.2.3) |
| **soft limits** | `posMin` and `posMax`, the ends of the needle's travel, captured by hand (§5.2.16) |
| **hand mark**, **RDA sample** | A typed (angle, frequency) pair; a measured one (§6.2.5) |
| **asleep**, **awake** | The A32's two states: silent and closed to phones, or playing (§9.2.6) |
| **on trial**, **confirmed**, **rolled back** | A new program not yet proved; proved and permanent; thrown away by the bootloader for the previous one (§4.2.15, §9.2.13) |
| **NVS** | The ESP32's key-value store in flash, where settings live (§7.2.3) |

## 2.7 Where to read what

| If you want to... | Read |
|---|---|
| Fix a misbehaving radio tonight | Chapter 1 |
| Build both programs, flash them, update them over the air, read the versions | Chapter 3 |
| Understand the S3's boot, its tasks, the clock display, the panel lamps, time keeping, the watchdog and rollback | Chapter 4 |
| Change how the needle moves, homes or recovers | Chapter 5 |
| Find the physical values of this machine that the firmware holds: needle travel, index, motor speed limits, tuner travel, the printed dial | §5.9 |
| Understand how the knob's angle becomes a frequency, and how the dial calibrates itself | Chapter 6 |
| Add or change a setting, or understand the settings file | Chapter 7 |
| Work on WiFi, the portal, its security, or the console | Chapter 8 |
| Understand the A32's boot, the link protocol, the A32 update relay and its rollback | Chapter 9 |
| Work on the sound, the sources, Bluetooth, or read the story of the pops | Chapter 10 |
| Know what the test tools proved | Chapter 11 |
| Find the small known issues left as they are, and starting points for new work | Chapter 12 |
| Know what is soldered, wired or fitted | The Hardware Bible |

Each of chapters 3 to 10 has the same shape: what it does, how it works, settings and constants,
decisions, failures and recovery, the graveyard of retired approaches, limits, and how to change the
area safely. The decisions and retired approaches record what worked and what did not **on this
radio**; they are not verdicts for every build (§0.3).

---

# 3. Building, flashing and versions

## 3.1 What it does

The firmware is one PlatformIO project that produces two programs: one for the main
microcontroller (an ESP32-S3, called "the S3" throughout this Gospel) and one for the audio
microcontroller (a classic ESP32, called "the A32"). This chapter tells you how to build both
from a clean PC, how to put them on the boards the first time over a USB cable, and how to
update them afterwards without a cable, through the machine's own web page (the "portal").
It also explains the version numbers the firmware carries, so that you can always tell which
program is running on which board, and which source code it was built from.

A newcomer should remember five things from this chapter. The toolchain is pinned and must
stay pinned. The only library the firmware uses is pinned to one exact commit. A board is
identified by its MAC address before anything is written to it. The success of a build or
an update is judged by what the machine reports afterwards, never by what a tool printed on
the way. And a new program sent over the air runs "on trial" for its first minute or so: if
it hangs or resets before it has proved itself, the board goes back to the program it had
before, by itself. On both boards the settings are not saved during that trial (they are
written once the new program is confirmed), and on the main board a reboot from the portal
during the trial is the way to take the new program back out.

## 3.2 How it works

### 3.2.1 One project, two boards

`firmware/platformio.ini` defines two build environments. An *environment* is PlatformIO's
name for one complete build recipe: target board, compiler flags, libraries and sources.

| | `s3` | `a32` |
|---|---|---|
| Board definition | `esp32-s3-devkitc-1` | `esp32dev` |
| Sources compiled | `src/s3/` | `src/a32/` |
| Shared headers | `include/pins.h`, `include/proto.h`, `include/link.h` | the same three files |
| Identity define | `-DAMB_S3=1` | `-DAMB_A32=1` |
| Other defines | `-DBOARD_HAS_PSRAM`, `-DARDUINO_USB_CDC_ON_BOOT=1` | none |
| Partition table | `default_16MB.csv` | `min_spiffs.csv` |
| Libraries | none beyond the Arduino core | ESP32-A2DP, pinned to one commit |
| Pre-build scripts | `scripts/version.py`, `scripts/page.py` | `scripts/version.py` |

Both environments also get `-DCORE_DEBUG_LEVEL=3` (the Arduino core's warning-level logging),
a serial monitor at 115,200 baud, and the monitor filters `esp32_exception_decoder` (turns a
crash address into a function name) and `time` (timestamps each line).

The `build_src_filter` line decides which folder of sources goes into which program. The
`include/` folder goes into both. That is the point of the single project: the pin map and the
inter-MCU protocol are one file compiled into both programs, so the two boards cannot disagree
about them by accident.

The S3's memory settings deserve a word. `board_build.arduino.memory_type = qio_opi`,
`flash_mode = qio`, `psram_type = opi` and `flash_size = 16MB` tell the core how the S3's
flash and its external RAM (PSRAM) are connected: 16 MB of quad-SPI flash and 8 MB of octal
PSRAM (Bible §1, §24). Getting `memory_type` wrong fails silently: the board boots normally
and reports 0 MB of PSRAM. `ARDUINO_USB_CDC_ON_BOOT=1` sends the S3's console to the chip's
own USB port, which is where the console cable goes (Bible §1, §2). If that ever changes, the
define must become 0 or the console goes nowhere visible. The A32 build is laid out for a
4 MB flash chip, because `min_spiffs.csv` lays out 4 MB (Bible §1).

The `s3` environment is built as Espressif's ESP32-S3-DevKitC-1 with those memory overrides,
and the board fitted here runs it: it is compatible with the DevKitC-1 in firmware and
pin-compatible for every pin the device uses (Bible §24). The firmware relies only on the pins in
`include/pins.h` and the memory type above.

### 3.2.2 The two pins: the platform and the library

**The platform.** `platform = espressif32@7.0.1`. This PlatformIO platform release brings
Arduino core 2.0.17, built on ESP-IDF 4.4.7 (Espressif's own framework, underneath Arduino).
The next major Arduino core, 3.x on ESP-IDF 5, retires the "legacy" I2S driver (I2S is the
serial audio bus between the A32 and its converters). Without that driver, the ESP32-A2DP
library routes its audio through a second library, AudioTools, instead of driving I2S
directly. The A32 runs three things at once on one chip: a Bluetooth audio receiver, audio
capture from the radio, and audio output. Its whole audio design rests on the legacy driver:
one full-duplex I2S driver, installed once and never reconfigured, with the Bluetooth
library given an output object that has no route to I2S at all (§10.2.3 and §10.2.4 explain
why). Core 2.0.17 is the most-travelled road for that combination. Official PlatformIO
support for Espressif stopped at 7.0.1 (May 2024), so the pin costs nothing in practice. If
core 3.x is ever truly needed, the escape hatch is one line: point `platform` at the
community fork `https://github.com/pioarduino/platform-espressif32`, and then expect to
redo the A32's audio engine.

**The library.** The A32 depends on exactly one library, ESP32-A2DP by Phil Schatzmann, which
turns the classic ESP32 into a Bluetooth A2DP audio receiver ("sink"). It is not in the
PlatformIO registry under a short name, so it is fetched from source with git:

```
lib_deps = https://github.com/pschatzmann/ESP32-A2DP.git#3245602afc494f9e62160a0cfb2af864af45a37f
```

That hash is the exact commit that had been running on the machine since the start
(dated 2026-08-12 upstream, library version 1.8.11). Before the pin, a fresh clone or a
deleted `.pio` folder fetched whatever the library's main branch held that day. That
mattered because the A32 overrides one of the library's internal hooks so that only the
A32's own code decides whether the radio is visible to Bluetooth scans (`PolicyA2dpSink` in
`src/a32/main.cpp`; §10.2.10 has the details). An override of internals is
exactly what a silent library update breaks.

### 3.2.3 The two build scripts

Both scripts run before compilation (`pre:` in `extra_scripts`). Note that the `s3`
environment lists `version.py` again, because an environment's own `extra_scripts` replaces
the common one instead of adding to it.

**`scripts/version.py` (both boards)** defines two C strings for the compiler.

- `FW_VERSION`, the version stamp: `v.<MAJOR>.<YYYYMMDDTHHMMSS>`, taken from the build PC's
  clock (`datetime.now()`, local time, no time zone in the string). Example:
  `v.1.20260926T101500`. MAJOR is a number typed into the script by hand (`MAJOR = 1`). It
  is `1` from the v.1.0 release on.
- `FW_COMMIT`, the source stamp: the 7-character short git hash of `HEAD`
  (`git rev-parse --short=7 HEAD`), followed by `-dirty` if any file that git tracks had
  uncommitted changes at build time (`git status --porcelain --untracked-files=no` is not
  empty). Files git does not track do not count. If git is not available, or the folder is
  not a repository, the value is `nogit`. The script prints both at build time:
  `Ambersong firmware version: v.1.<stamp> (<hash>)`.

The stamp is taken at build time, not from a file, because the question that matters in the
field is "which binary is this?", not "which source revision?". Two builds of the same source
get different stamps on purpose. The S3 can update the A32, so the two boards *will* run
builds from different moments, and the start-up handshake between them has to be able to say
which is which. The commit hash then answers the second question, "which source?", and
`-dirty` warns you when the hash alone would name code that is not what was built.

The two strings are kept apart for a reason. The version string travels between the boards
inside the link's handshake message, in a fixed 24-byte field (`PROTO_VERSION_LEN` in
`include/proto.h`). `v.1.` plus a 15-character timestamp is 19 characters and fits. The
version and the hash together would not fit, and widening the field would be a protocol
change. So `FW_COMMIT` is shown next to the version wherever a human reads it, and never
travels on the link as part of the version. (The A32's commit reaches the S3 only inside the
text of its boot report, 3.2.9.)

If `version.py` did not run at all, both `main.cpp` files fall back to
`FW_VERSION "v.0.unstamped"` and `FW_COMMIT "nogit"`. Seeing
`unstamped` on a running board means it was built outside PlatformIO's normal flow.

**`scripts/page.py` (S3 only)** turns the portal's single web page into C data.

1. It reads `data/portal.html`.
2. It checks that the page's one `<script>` block parses as JavaScript, using the Python
   package `esprima`. If the script does not parse, **the build fails**. If `esprima` is not
   installed, the script prints `[WARN] esprima not installed - the portal's script was NOT
   checked.` and the build carries on.
3. It compresses the page with gzip at level 9, with the gzip timestamp forced to zero
   (`mtime=0`).
4. It computes an ETag (a cache identifier that browsers send back to ask "has this
   changed?"): the first 16 hexadecimal digits of the SHA-256 of the compressed bytes.
5. It writes `src/s3/page_gz.h`, holding `PAGE_GZ[]` (the compressed page, in program
   memory), `PAGE_GZ_LEN` and `PAGE_ETAG`. It rewrites the file only when its text changed,
   so an untouched page does not force a rebuild.

`src/s3/page_gz.h` is generated, but it is tracked in git so that the tree is always complete.
Never edit it by hand: edit `data/portal.html` and rebuild. At v.1.0.3 the page
is 35,886 bytes of HTML and 13,322 bytes compressed (37 %).

Why each step exists:

- *Compression at build time.* The page used to be sent uncompressed on every load, by a web
  server that handles one connection at a time. Measured on the machine, a load took 0.7 to
  3.6 seconds. Every browser accepts `Content-Encoding: gzip`. The ESP32's ROM has a reliable
  decompressor and no compressor, so compressing on each request would only move the cost.
- *`mtime=0`.* An unchanged page then gives byte-identical compressed data, so the ETag stays
  the same across rebuilds, and a reload costs one "304 Not Modified" with an empty body.
- *The JavaScript check.* A duplicated `const` declaration once reached the machine and made
  the whole script fail to parse, so the portal showed a dead page. The author found it on the
  hardware. A syntax error is the one class of bug a machine can catch for free.
- *No filesystem.* The page lives inside the program, so there is nothing to upload
  separately and nothing that can fall out of step with the firmware serving it. The
  `spiffs` partitions below exist only because the stock tables contain them; this firmware
  never mounts them.

### 3.2.4 Partitions, and what survives a reflash

A partition table divides the flash chip into named regions. Both tables are stock files from
the Arduino core; the firmware does not ship its own.

S3, `default_16MB.csv` (16 MB flash):

| Name | Type / subtype | Offset | Size | Used for |
|---|---|---|---|---|
| nvs | data / nvs | 0x9000 | 0x5000 (20 KB) | settings, calibrations, network, accounts |
| otadata | data / ota | 0xE000 | 0x2000 | which app slot boots |
| app0 | app / ota_0 | 0x10000 | 0x640000 (6.25 MiB) | program slot 0 |
| app1 | app / ota_1 | 0x650000 | 0x640000 (6.25 MiB) | program slot 1 |
| spiffs | data / spiffs | 0xC90000 | 0x360000 | unused |
| coredump | data / coredump | 0xFF0000 | 0x10000 | crash dump area |

A32, `min_spiffs.csv` (4 MB flash):

| Name | Type / subtype | Offset | Size | Used for |
|---|---|---|---|---|
| nvs | data / nvs | 0x9000 | 0x5000 | audio and Bluetooth settings |
| otadata | data / ota | 0xE000 | 0x2000 | which app slot boots |
| app0 | app / ota_0 | 0x10000 | 0x1E0000 (1.875 MiB) | program slot 0 |
| app1 | app / ota_1 | 0x1F0000 | 0x1E0000 (1.875 MiB) | program slot 1 |
| spiffs | data / spiffs | 0x3D0000 | 0x20000 | unused |
| coredump | data / coredump | 0x3F0000 | 0x10000 | crash dump area |

Two program slots are what make an over-the-air (OTA) update possible. The running program
writes the new image into the slot it is not running from, then tells the bootloader to boot
the other slot next time. At v.1.0.5, the S3 image is 999,568 bytes (about 15 % of
its slot); at v.1.0.4, the A32 image is 1,218,608 bytes (about 62 % of its slot).

NVS ("non-volatile storage") is the ESP-IDF key-value store. The firmware uses these
namespaces:

| Board | Namespace | Holds |
|---|---|---|
| S3 | `amb3` | The settings blob: a versioned, append-only structure (magic `0xA838`, `SETTINGS_VERSION 7`). Chapter 7 covers it (§7.2.4). It also holds the learned WiFi transmit power (`wifiTxQ`), which is not written to the settings file. |
| S3 | `net` | WiFi network name and password, NTP server, time zone. |
| S3 | `auth` | Portal accounts, stored as salted SHA-256 hashes. |
| A32 | `amb` | Audio and Bluetooth settings. |

The needle's position memory across a *software* restart is not in NVS. It lives in RAM that
a restart does not clear (`RTC_NOINIT_ATTR`, `gMem` in `src/s3/needle.cpp`); a power cycle
clears it.

A normal PlatformIO USB upload writes the bootloader, the partition table, a fresh `otadata`
(so the board boots slot 0) and the program into slot 0. It does not touch NVS. So
calibrations, accounts and WiFi settings survive a USB reflash. A full chip erase
(`pio run -t erase`, or `esptool.py erase_flash`) wipes all of them. Changing a partition
table can only be done over USB, never over the air, because the running program cannot
rewrite the table it is running under.

### 3.2.5 Building from a clean PC, step by step

These steps were written on Windows; they work the same on Linux or macOS with the obvious
path changes.

1. **Install git.** PlatformIO uses it to fetch the pinned A2DP library, and `version.py`
   uses it to read the commit hash.
2. **Install PlatformIO.** Either VS Code with the PlatformIO IDE extension, or PlatformIO
   Core on its own. The `pio` command may not be on your `PATH`; on Windows it lives at
   `%USERPROFILE%\.platformio\penv\Scripts\pio.exe`.
3. **Install `esprima` into PlatformIO's own Python** (recommended). PlatformIO runs its own
   Python, so install the package there:
   `%USERPROFILE%\.platformio\penv\Scripts\pip install esprima`. Without it the portal's
   JavaScript is not checked (see 3.2.3).
4. **Get the `firmware/` folder**, ideally as a git clone so that the commit stamp works.
   Nothing outside `firmware/` is needed to build.
5. **Build both programs**, from inside `firmware/`:

   ```
   pio run -e s3        # the main board
   pio run -e a32       # the audio board
   ```

   The first build downloads the pinned platform, Arduino core 2.0.17, the Xtensa compilers
   and the pinned A2DP commit. That takes a while and needs internet access. Later builds are
   offline.
6. **Check the exit code, not the output.** `0` means built; anything else means not built,
   whatever the last lines on screen say. In PowerShell read `$LASTEXITCODE`; in a POSIX
   shell, `$?`. The project learned this the hard way: a filter that searched the output for
   `SUCCESS` once hid a linker failure, and twice a pre-build script failed before printing
   anything at all.
7. **Never let two builds use the same `.pio` folder at once.** Two builds compiling into
   one `.pio` produced the linker failure just mentioned.
8. **Find the outputs** in `.pio/build/s3/` and `.pio/build/a32/`. `firmware.bin` is the
   program, the file you upload through the portal. `bootloader.bin` and `partitions.bin` go
   onto the board only on a USB upload.

### 3.2.6 Identify the board before you write to it

**Rule: never flash a board without first identifying it by its MAC address.** Every ESP32 has
a factory-burned MAC address, unique to the chip. Keep a written list of your own boards' MAC
addresses, and before every write compare the target against that list.

Why: a workbench often has several ESP32 boards plugged in, and boards of the same kind share
the same USB vendor and product IDs. PlatformIO's automatic port detection will happily pick
the wrong one. So never let PlatformIO choose the port. Pass `--upload-port` every time.

How to read a board's MAC:

- **A board on the chip's own USB port** (the S3 here). `pio device list` shows the MAC
  directly as the serial number, `SER=xx:xx:xx:xx:xx:xx`.
- **A board behind a USB-to-UART bridge chip** (the A32 here, on a CP210x bridge, Bible §1).
  The serial number that `pio device list` shows belongs to the bridge, not the ESP32. Ask
  the chip itself:

  ```
  esptool.py --port COMx read_mac
  ```

  (`flash_id` also prints the MAC, together with the flash size.) `esptool.py` ships with
  PlatformIO as the package `tool-esptoolpy`. Reading the MAC resets the board into its ROM
  bootloader and back; it writes nothing.

The same rule applies to over-the-air updates. The portal is reached by a network address,
and an address is not an identity. The author identified the S3 before every OTA by
finding its MAC in the PC's ARP table (`arp -a`) against the
address the browser was about to use.

A second rule travels with the first: **never flash a machine while someone is listening to
it or looking at it.** A new program pushed into a machine under observation spoils the
observation, and the person may not know why the behaviour changed.

### 3.2.7 The first flash, by USB

A blank board needs one USB flash. After that, both boards can be updated over the air
through the portal.

Safety first. The tube radio in this machine is mains-referenced (Bible §10, §21). Read those
sections before you connect any cable from the machine to a mains-powered PC. The portal's
console and OTA exist partly so that no cable is needed after the first flash.

```
pio device list                                     # find your S3 by SER=<its MAC>
pio run -e s3  -t upload --upload-port COMx         # the port you found, never automatic
esptool.py --port COMy read_mac                     # confirm the A32 is the board you mean
pio run -e a32 -t upload --upload-port COMy
pio device monitor -p COMx -b 115200                # the S3's console
```

Practical notes:

- On this machine both boards entered download mode by themselves; no button presses were
  needed.
- The S3's console runs on its native USB, which does not reset the chip when a monitor
  opens. You therefore miss the start-up banner. The console key `s` prints the status and
  the command list for that reason. To see the banner, reset the board first (for example
  `esptool.py --after hard_reset`), then open the monitor.
- A serial tool that opens the port with DTR and RTS asserted (the default of Python's
  `pyserial`) resets the board. It once made every reading show "uptime 1 s".
- The S3 has no wire to reset the A32 (Bible §2, §3). After its first USB flash, the A32's
  only update route is the relay through the S3 (3.2.9).

### 3.2.8 The first boot

Chapters 4 to 10 describe each subsystem's own start-up. Here is what a builder meets, in
order, on a factory-fresh pair of boards.

The S3 runs `setup()` in `src/s3/main.cpp` in this order: the 15-second task watchdog and
the reading of its own image state (on trial, valid, or not tracked), the console (waits up
to 3 s for a USB host), a banner with the firmware version and commit
(`firmware v.1.<stamp> (<hash>)`), the amp-sense input, `settingsLoad()`, the purge of
corrupt stored limits and spurs, `protoSelfTest()` (the link's framing self-test; a failure is
reported and the boot carries on, §4.2.15), then the display, the panel lamps, the needle and
the RDA5807M, the stored calibration and last angle, the link to the A32 on its UART, the
encoder report, the needle's start-up position and homing, `Net::begin()`, `Portal::begin()`,
the image line (`image      : <state>, watchdog on`) and the command list. Chapter 4 (§4.2.3)
walks through each step.

What you will see:

1. **No stored settings.** The compiled defaults apply. Chapter 7 lists them (§7.3).
2. **No stored WiFi.** The S3 raises its rescue access point at once: network name
   `Ambersong`, on channel 1. It is an open network with no password (since v.1.0.5).
   Join it; on most phones the portal's sign-in page opens by itself (a captive portal, §8.2.3).
   Otherwise open `http://192.168.4.1/`, or `http://ambersong.local/` (mDNS). Set your home network from the portal's network page, or with the console key `y`.
   If a stored network cannot be joined within 20 s, the rescue access point comes up and the
   home network is retried every two minutes. The transmit power starts at 2 dBm and climbs a
   ladder (2, 5, 7, 8.5, 11 and 15 dBm) on failed joins, keeping whichever step worked. The 15 dBm ceiling is a
   firmware choice: measured here on 2026-09-24, nothing could see the board at 20 dBm and
   everything could at 15. The learned power is not in the portal or the settings file; only
   the console changes it by hand. Chapter 8 has the details (§8.2.2 to §8.2.4).
3. **The placeholder portal account.** The firmware ships with a public placeholder account
   (`DEFAULT_USER` / `DEFAULT_PASS` in `src/s3/portal.cpp`). While the stored account still
   matches it, the portal refuses everything except changing the password (and both OTA
   endpoints refuse too): `requireLogin()` lets through only `/api/boot`, `/api/state`,
   `/api/passwd` and `/api/logout`, and answers every other request with 403 `mustchg`; the
   page shows only the change-password form. Change the password first. The console key `~` erases all accounts
   and restores the placeholder. An admin can type it in the portal's console; if nobody can
   log in any more, the USB cable is the only way to type it. That is by design: there is no
   front-panel reset for a forgotten password.
4. **The clock.** The real-time clock keeps time; NTP corrects it once the S3 is online. The
   console key `W` sets it by hand, but that prompt interprets your input in a hard-coded
   North American Eastern time zone (`EST5EDT`), regardless of the portal's time-zone
   setting. The code carries a note for builders elsewhere to change it; it stays as it is in
   this build. A new real-time clock module may come up with no valid
   time: the portal then shows `BATTERY CLOCK LOST ITS TIME - check its battery` until the
   clock reports a valid time again (it is asked every minute; chapter 4).
5. **Needle calibration.** The needle has no end-stop switches. Calibrate in this order, as
   chapter 5 explains: the band calibration (`k`) with the needle on its index
   sensor, and save; the soft limits (`m`, `M`); the tuner ends (`c`, `C`), which since
   v.1.0 the firmware also uses at every boot to pick the encoder's turn; then the dial
   marks.
6. **The dial's self-calibration.** With the tube set powered but switched off, sweep and
   mark its fixed spurs (`q`, `o`). Switch it on, and measure three positions spread across
   the dial (`V`, or "Measure this dial position" in the portal). From then on the radio keeps
   its own calibration up to date. Chapter 6 explains the method.

The A32 at first boot arms a 15-second loop watchdog as the first line of `setup()`, runs its
own framing self-test (a failure mutes it and keeps it off Bluetooth, but the board stays
reachable for an update), loads its settings, reads the clock and waits for the S3's HELLO
message. An image written by USB is not "on trial" on either board (3.2.9): the S3's image
line then reads `not tracked (USB flash)`, and both boards save settings normally from their
first boot.

### 3.2.9 Updating over the air, through the portal

The portal's **System** tab has two file pickers, one per board. Choose the matching
`firmware.bin` and upload. Behind them are two HTTP endpoints:

| Endpoint | Handlers in `src/s3/portal.cpp` | Target |
|---|---|---|
| `POST /api/ota/s3` | `hOtaUpload`, `hOtaEnd` | the S3 itself |
| `POST /api/ota/a32` | `hOtaA32Upload`, `hOtaA32End` | the A32, relayed by the S3 |

Both take a multipart file upload of `firmware.bin`. To script an update, log in with
`POST /api/login` (form fields `u` and `p`; the answer sets a session cookie), then post the
file to the endpoint with that cookie. Keep credentials in environment variables, never in the
script.

**Gates common to both endpoints.**

- *Admin only.* A non-admin session gets `403 admin only`.
- *Not with the placeholder password.* The shipped account gets `403 change the default
  password first`. These two OTA handlers check this themselves; they are the only handlers
  that do not go through `requireLogin()`.
- *The chip check.* Bytes 12 and 13 of an ESP32 image header hold the chip ID: `0x0009` for an
  ESP32-S3, `0x0000` for a classic ESP32. On the first chunk, each endpoint refuses an image
  built for the other chip, before anything is written. The two pickers sit side by side and
  the two files have the same name, so this mistake is easy to make.
- *The upload watchdog.* If no chunk arrives for 15 s, `otaWatchdog()` aborts the update and
  puts the S3 back to work. The Arduino `WebServer` does not reliably report a connection that
  died mid-upload, and a dead transfer once left the machine muted for four minutes.

**Updating the S3.** First, `hOtaUpload` asks `s3ImageOnTrial()` whether the S3's own running
image is still on trial (see below). If it is, the update is refused: nothing is parked or
darkened, the bytes that keep arriving are dropped, the console prints
`portal: S3 update refused - this image is still on trial.`, and at the end `hOtaEnd` answers
HTTP 409 with `{"e":"this firmware is still on trial - retry in a minute"}`. The browser still
sends the whole file before that answer comes back; the portal page then shows the message.
Otherwise `hOtaUpload` streams the file into the idle program slot
(`Update.begin(UPDATE_SIZE_UNKNOWN)`). While it writes, `portalOtaQuiet(true)` parks the
needle, blanks the display and darkens the panel lamps. Writing flash suspends the processor's
flash cache, which would make those animations stutter; fifteen dark seconds read better than
fifteen ugly ones. The audio keeps playing, because it lives on the A32. On success the S3
first clears its "OTA in progress" flag, then saves any pending settings
(`settingsFlushNow()`; a failed save is reported as a warning), answers
`200 rebooting into the new firmware`, waits 300 ms and restarts. If an upload fails and the
S3 does not restart, the needle state that the upload interrupted is restored.

Do not trust the HTTP answer of an update that restarts the board. Several transfers reported
failure and had in fact landed. Confirm by reading the version after the restart (3.2.10).

**The S3's trial period.** A new S3 image written over the air boots "on trial"
(`ON TRIAL (rollback armed)` in the console's image line). The S3 confirms it once it has run
for at least 60 seconds and, at a check made every 10 seconds after that, finds three things:
the portal task still turning, the home network joined or the rescue access point up, and a
passed link self-test. The first check only takes a reading, so the earliest confirmation
comes about 70 seconds after boot. The console then prints
`image confirmed (a minute of running, network and portal up).` The A32 is not needed for
this, and neither is the needle. Until the image is confirmed, the S3 refuses another S3
update (the 409 above), so a second upload cannot overwrite the image a rollback would return
to. If the trial image resets for any reason before it is confirmed, the bootloader starts the
previous image instead. A task watchdog of 15 s watches `loop()` and the needle's supervisor
task (not the portal task, not the step emitter), so a hang becomes a reset, and a reset of a
trial image becomes a rollback. A failed link self-test at boot no longer stops the S3: the
S3 reports it and boots normally, but such an image is never confirmed. An image written by
USB is not on trial. Chapter 4 describes all of this in full (§4.2.15).

**No settings are saved during the S3's trial** (v.1.0.1, 2026-09-25). While
`s3ImageOnTrial()` is true, `settingsWrite()` in `src/s3/main.cpp`, the only function that
writes the settings blob (`amb3`), writes nothing. Changes are kept in RAM, marked unsaved,
and the normal two-second save writes them once the image is confirmed. (The WiFi network and
the portal accounts live in their own namespaces, `net` and `auth`, and are not held: they
save at once, trial or not.) The portal's
Save answers `NOT saved yet - this firmware is on trial after an update; settings are written
once it is confirmed (about a minute)`, and the console prints `save held - this firmware is
on trial; settings are written once it is confirmed.` The reason: a new image may change the
settings layout (`SETTINGS_VERSION`, §7.2.4). It migrates the stored settings in RAM when it
boots. If it wrote them straight away and were then rolled back, the previous image would
find a layout newer than it knows and lock its settings (§7.2.7). With nothing written until
confirmation, a rolled-back image always finds the settings exactly as it left them. The cost:
a change made during the trial minute is lost if the trial ends in a reset (a rollback or a
power cut).

**A portal reboot during the trial is a rollback.** `hReboot()` in `src/s3/portal.cpp` does
not try to save first while the image is on trial, because there is nothing it may save. It
answers `rebooting - this firmware was still on trial, so the previous firmware comes back`
and restarts at once. The bootloader then starts the previous image. So the portal's Reboot
button is the way to take back an S3 update that misbehaves in its first minute. Proven live
on 2026-09-25 with v.1.0.1: the S3 came back on the v.1.0 image with `ROLLED BACK` in its image
line.

**Updating the A32.** The file goes to the S3 over WiFi, and the S3 relays it to the A32 over
the inter-board serial link (921,600 baud, framed and CRC-checked; §9.2.8 describes the
framing, and §9.2.12 the update from the A32's side). The functions are `a32OtaBegin()`, `a32OtaChunk()` and `a32OtaEnd()` in
`src/s3/main.cpp`, and the `MSG_OTA_*` cases in the A32's message handler in
`src/a32/main.cpp`.

1. *Begin.* The S3 refuses (`the A32 is not answering`) if the A32 has not said HELLO. The A32
   refuses with `still on trial - retry in 1 min` if its own current image is still on trial.
   Otherwise the A32 remembers the user's mute setting, mutes, waits 400 ms for the fade and
   for its audio buffers to drain, and opens the idle slot (`Update.begin()`, which erases
   nothing; the slot is erased a sector at a time as the data is written). The S3 waits up to
   20 s for the answer: 5 s failed consistently on 2026-09-01 (the update never started and
   nothing was written), and waiting costs nothing on this one step. The S3 does not park its
   needle or display, because it is not writing its own flash; the sound stops because the A32
   is.
2. *Data.* The S3 cuts the upload into 1 KB frames, each carrying its offset and length. The
   A32 acknowledges each frame with the number of bytes it has actually written. The S3
   moves on when the acknowledgement equals offset plus length, resends when it equals the
   offset (a duplicate or a late acknowledgement lands here harmlessly), and aborts on
   anything else. Each frame gets 4 attempts of 3 s. The A32 ignores a frame that does not
   start exactly where it is.
3. *End.* The S3 sends the CRC32 of the whole image (neither the size nor the checksum is
   known at the start, because the upload is streamed). The A32 checks it, finishes the update,
   answers `ok, rebooting` and restarts 250 ms later. The S3 waits 4 s for that answer. If it
   does not come, the portal says `the A32 did not confirm the update - check its version once
   it reconnects`: the A32 commits before it answers, so a lost answer can hide a good update.
4. After the end, the S3 forgets the A32's HELLO, so the next handshake shows the new version.
5. If frames stop arriving for 20 s, the A32 gives up by itself (`the S3 stopped sending`)
   and restores the user's mute setting.

The relay is slow on purpose: about 16 KB/s (1.2 MB took 76 s on 2026-08-31), because each
frame waits for its acknowledgement and each acknowledgement waits for a flash write. A
transfer that cannot tell whether it arrived is not worth having faster.

**The A32's trial period.** The A32 defines `verifyRollbackLater()` to return true, which
tells the Arduino core not to confirm a new image before `setup()`. A relayed image therefore
boots on trial. `confirmTick()` confirms it (`esp_ota_mark_app_valid_cancel_rollback()`) one
minute after the first handshake with the S3, or after five minutes if nothing at all has
arrived from the S3 (the S3 is absent). An image that hears bytes on the link but never
completes a handshake stays on trial, so the next reset returns to the previous image. So does
an image whose own link self-test failed: `confirmTick()` never confirms it (v.1.0,
2026-09-25), as on the S3. A failed confirmation is retried every 10 s. The 15-second loop watchdog turns a frozen
`loop()`, or a Bluetooth start that never returns, into a reset, and a reset of a trial image
into a rollback. Why one minute and not the handshake itself: the handshake arrives about a
second after boot, before Bluetooth has started or any audio has played, so an image that
crashed there used to be confirmed already.

**No settings are saved during the A32's trial** (v.1.0.4, 2026-09-26). The A32 has
the same hold as the S3. While its image is on trial and not confirmed, `settingsFlush()` in
`src/a32/main.cpp`, the only function that writes the A32's settings (NVS namespace `amb`),
writes nothing. A change sent from the portal is applied at once and kept in RAM. The moment the
image is confirmed, `confirmImage()` writes it and sends the line `settings changed during the
trial are now saved` to the S3's console. The reason is the same as on the S3: the A32 keeps a
stored settings structure only if its size matches its own, so a trial image that changed
`ProtoAudio` or `ProtoBtCfg` and saved, then was rolled back, would have left the previous image
running that structure on its defaults. The cost is the same too: a change made during the trial
is lost if the trial ends in a reset, including "reboot the audio board" (chapter 9, 9.2.3).

Proven live on 2026-09-26 with v.1.0.4 on trial: the Bluetooth lamp's `btLedOff` was changed
from 255 to 254 in the portal during the trial (answered ok); the A32 wrote nothing until 11:35:40, when
it logged `settings changed during the trial are now saved` together with `image confirmed (a
minute of running with the S3)`. The value was put back at 11:36:36.

**The A32's boot report.** Once per boot the A32 sends a line to the S3's console:
`boot: commit <hash>, reset reason N, setup X ms, image <state>, watchdog on|OFF`, with
` - an earlier update was ROLLED BACK` appended when the bootloader has marked a previous
image invalid (`bootReport()` in `src/a32/main.cpp`). The reset reason is ESP-IDF's
`esp_reset_reason()` number; 6 is the task watchdog. The S3's console is readable in the
portal, so a rollback or a watchdog reset is visible without a cable.

**Walk-through: updating both boards over the air.** This is the whole round trip, as it
was done on the radio for every release from v.1.0 (2026-09-25) to v.1.0.5 (2026-09-27), with
no USB cable connected. (v.1.0.1 and v.1.0.5 changed only the S3 and v.1.0.4 only the A32; for
those, only the board that changed was updated.)

1. Build both environments and check both exit codes (3.2.5). Note the commit you built
   from; if the build printed `-dirty`, you are about to send code that no commit names.
2. Log in to the portal as the admin and open the **System** tab. Its "running" line shows
   the S3's current `v.1.<stamp> (<hash>)`.
3. Choose the S3's `.pio/build/s3/firmware.bin` in the main board's picker and upload. The
   clock goes blank, the panel lamps go dark and the needle stops; the sound keeps playing.
   When the image is written the page reports `rebooting into the new firmware`, and the
   S3 restarts 300 ms later.
4. Reload the page (log in again if it asks). Check that the "running" line shows the new
   stamp and hash. In the Console
   tab, `s` ends with `image      : ON TRIAL (rollback armed), watchdog on`.
5. **The trial minute.** Leave the S3 alone. If you upload another S3 image now, it is
   refused: the page shows `this firmware is still on trial - retry in a minute` (HTTP 409)
   and nothing is written. Settings you change now are held in RAM, not saved (3.2.9). If the
   new image misbehaves, press Reboot in the portal: the page answers `rebooting - this
   firmware was still on trial, so the previous firmware comes back`, and the S3 returns to
   the image it had before, with its settings as they were. Otherwise, about 70 to 90 seconds
   after the restart the console prints `image confirmed (a minute of running, network and
   portal up).` From then on the image is permanent, held settings are written, and a new S3
   upload is accepted.
6. Choose the A32's `.pio/build/a32/firmware.bin` in the audio board's picker and upload.
   The sound stops; the S3's clock, lamps and needle carry on. The relay takes over a minute
   for a full image. The page reports `<N> bytes sent, the A32 is rebooting`.
7. In the S3's console, watch for the A32's boot report
   (`boot: commit <hash>, ... image ON TRIAL (rollback armed), watchdog on`), then
   `[PASS] A32 up: proto v4, firmware v.1.<stamp>` at the new handshake, and, a minute
   later, `image confirmed (a minute of running with the S3)`. An A32 upload started inside
   that minute is refused with `still on trial - retry in 1 min`. Audio-board settings changed
   in that minute are held in RAM; if there were any, `settings changed during the trial are now
   saved` appears next to the confirmation.
8. If either board ever comes back with `- an earlier update was ROLLED BACK` in its image
   line or boot report, the new image failed its trial and the board is running the one it
   had before. Read the reset reason (`s` on the S3; the boot report on the A32), fix the
   build, and upload again.

**A settings layout change is safe over the air, on both boards.** Before v.1.0.1, an S3 image
that raised `SETTINGS_VERSION` wrote its migrated settings within seconds of its first boot,
while still on trial; had it then been rolled back, the previous image would have found a layout
newer than it knows and locked its settings (§7.2.7). Before v.1.0.4 the A32 had the same trap in
its own form: a trial image that changed `ProtoAudio` or `ProtoBtCfg` and saved, then rolled back,
left the previous image running that structure on defaults. Both traps are closed by design: no
settings are written on either board until its image is confirmed, and a confirmed image is never
rolled back to the previous one. No special step is needed. Downloading a settings file before any
update is still a good habit.

### 3.2.10 Versions, and where to read them

The firmware carries four independent numbers. Do not confuse them.

| Number | Where it is set | What it means | Changes when |
|---|---|---|---|
| `FW_VERSION`, e.g. `v.1.20260926T101500` | `scripts/version.py` | which binary: MAJOR plus the build time | every build |
| `FW_COMMIT`, e.g. `a1b2c3d` or `a1b2c3d-dirty` | `scripts/version.py` | which source it was built from | every commit, or uncommitted changes |
| `PROTO_VERSION` (4 at v.1.0.3, v.1.0.4 and v.1.0.5) | `include/proto.h` | wire compatibility between the two boards | a protocol change that breaks compatibility |
| `SETTINGS_VERSION` (7 at v.1.0.3 and v.1.0.5) | `src/s3/main.cpp` | the layout of the stored settings | a settings layout change (append-only, with a migration) |

**MAJOR** means only "the author's release". It is `1` from the v.1.0 release, and it changes
only when the author declares a new release. A protocol break is `PROTO_VERSION`'s job, not
MAJOR's.

**Tags.** The release, and each set of builds put on the radio after it, is marked in the
firmware repository by an annotated git tag. The tag names a commit; the version stamp still
carries only MAJOR, so every build below reads `v.1.<stamp>`.

| Tag | Date | S3 build on the radio | A32 build on the radio |
|---|---|---|---|
| `v.1.0` | 2026-09-25 | `v.1.20260925T200355` | `v.1.20260925T200411` |
| `v.1.0.1` | 2026-09-25 | `v.1.20260925T211316` | unchanged: `v.1.20260925T200411` |
| `v.1.0.2` | 2026-09-25 | `v.1.20260925T222141` | `v.1.20260925T222159` |
| `v.1.0.3` | 2026-09-26 | `v.1.20260926T103447` | `v.1.20260926T103503` |
| `v.1.0.4` | 2026-09-26 | unchanged: `v.1.20260926T103447` | `v.1.20260926T113302` |
| `v.1.0.5` | 2026-09-27 | `v.1.20260927T101323` | unchanged: `v.1.20260926T113302` |

The running pair is the S3 at `v.1.0.5` (`v.1.20260927T101323`) and the A32 at
`v.1.0.4` (`v.1.20260926T113302`), both sent over the air and confirmed.
`v.1.0.1` and `v.1.0.5` changed only the S3 and `v.1.0.4` only the A32, so for those tags the two boards ran
builds from different commits; that is normal, and the handshake is what tells them apart.

When the two boards shake hands, each sends its `PROTO_VERSION` and `FW_VERSION`. The link's
framer drops any frame whose protocol number is not its own before the message handler sees
it, and counts it. So boards running different protocol versions never complete a handshake
at all. The count is what shows it (2026-09-25): the `link` line of the console's
`s` status shows a non-zero `wrong-version` count followed by `<- the boards run different
protocol versions: flash both`, the portal shows the pill `BOARDS RUN DIFFERENT PROTOCOL
VERSIONS - flash both` while the link is down, and `GET /api/state` carries the count as
`linkver`. (A `[WARN] PROTOCOL MISMATCH` line after a
handshake is still in the code, but it cannot fire, because such a handshake never arrives.)
Adding fields at the end of the telemetry structure `ProtoState` was deliberately done
without a protocol bump (2026-09-12): an older peer's frame then has the wrong length
and is rejected, so a half-updated machine loses its telemetry but keeps its audio.

Where to read each board's version:

| Board | Where |
|---|---|
| S3 | the portal's System tab, line "running": the schema field `fw`, `v.1.<stamp> (<hash>)` (`GET /api/schema`, or inside `GET /api/boot` whenever the browser's cached schema is out of date, which it always is after an update); the console banner, `firmware v.1.<stamp> (<hash>)`; the second line of a downloaded settings file, `# firmware v.1.<stamp> (<hash>)` (the first is `# Ambersong settings`) |
| A32 | only in the S3's console (visible in the portal): `[PASS] A32 up: proto vN, firmware <version>` at each handshake, and the A32's boot report, `boot: commit <hash>, ...`. No portal JSON field carries the A32's version. |

The portal's cached settings schema is keyed on `FW_VERSION` plus the admin bit
(`schemaTag` in `src/s3/portal.cpp`), so every new S3 build makes browsers fetch a fresh
schema.

This Gospel describes the code at the commits it names; a newer commit may differ from it.

## 3.3 Settings and constants

Nothing in this chapter is a user setting. These are the build and update constants.

| Name | Value | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `platform` | `espressif32@7.0.1` | — | pinned | PlatformIO platform; brings Arduino core 2.0.17 / ESP-IDF 4.4.7 | `platformio.ini`, on purpose only |
| ESP32-A2DP | commit `3245602afc494f9e62160a0cfb2af864af45a37f` (v1.8.11) | — | pinned | the A32's only library | `platformio.ini` `lib_deps`, on purpose only |
| `MAJOR` | 1 | — | integer | the author's release number | `scripts/version.py`, by hand |
| `FW_VERSION` | `v.MAJOR.YYYYMMDDTHHMMSS` | — | ≤ 23 characters | which binary | `version.py`, every build |
| `FW_COMMIT` | 7-character short hash, plus `-dirty`; or `nogit` | — | — | which source | `version.py`, every build |
| `PROTO_VERSION` | 4 | — | integer | link compatibility | `include/proto.h`, by hand |
| `PROTO_VERSION_LEN` | 24 | bytes | fixed | the version field in the handshake | `include/proto.h` (changing it is a protocol change) |
| `PROTO_MAX_PAYLOAD` | 1088 | bytes | fixed | largest frame payload: a 1024-byte OTA chunk plus header | `include/proto.h` |
| Page compression | gzip level 9, `mtime=0` | — | — | the portal page in program memory | `scripts/page.py` |
| OTA upload watchdog | 15 | s | — | abort an upload with no chunk for this long | `otaWatchdog()`, `src/s3/portal.cpp` |
| S3 restart delay after OTA | 300 | ms | — | time for the HTTP answer to leave | `hOtaEnd()` |
| A32 BEGIN answer wait | 20 | s | — | the A32 preparing its idle slot | `a32OtaBegin()`, `src/s3/main.cpp` |
| A32 fade wait before writing | 400 | ms | — | mute fade plus audio buffer drain | A32 `MSG_OTA_BEGIN` handler |
| Relay frame | 1024 | bytes | — | OTA data per frame | `src/s3/main.cpp` |
| Relay attempts per frame | 4 × 3 | s | — | resend budget | `src/s3/main.cpp` |
| A32 END answer wait | 4 | s | — | `ok, rebooting` | `a32OtaEnd()` |
| A32 relay give-up | 20 | s | — | no frame for this long ends the A32's update | A32 `loop()` |
| A32 restart after END | 250 | ms | — | — | A32 `MSG_OTA_END` handler |
| A32 trial confirmation | 60 after handshake, or 300 with no S3 traffic | s | — | when a relayed image becomes permanent | `confirmTick()`, `src/a32/main.cpp` |
| A32 loop watchdog | 15 | s | — | frozen `loop()` resets the chip | `WDT_TIMEOUT_S`, `src/a32/main.cpp` |
| S3 trial confirmation | not before 60, then a check every 10; earliest about 70 | s | — | when an S3 image becomes permanent (portal turning, network up, self-test passed) | `CONFIRM_AFTER_MS`, `confirmTick()`, `src/s3/main.cpp` |
| S3 task watchdog | 15, on `loop()` and the needle supervisor | s | — | hang becomes reset (panic on) | `WDT_TIMEOUT_S`, `src/s3/main.cpp` |
| S3 settings writes while on trial | none | — | — | changes wait in RAM until the image is confirmed | `settingsWrite()`, `src/s3/main.cpp` |
| S3 portal reboot while on trial | at once, no save attempted (200 ms for the answer) | — | — | a reboot of a trial image is a rollback | `hReboot()`, `src/s3/portal.cpp` |
| A32 settings writes while on trial | none | — | — | changes wait in RAM; written by `confirmImage()` at confirmation (v.1.0.4) | `settingsFlush()`, `src/a32/main.cpp` |

## 3.4 Decisions

**Decision.** Pin the platform to `espressif32@7.0.1` (Arduino core 2.0.17).
Why. The A32's audio design needs the legacy I2S driver, and ESP32-A2DP drives I2S directly
only on core 2.x. On core 3.x the library routes through AudioTools.
Rejected. Core 3.x through the community pioarduino fork: kept only as an escape hatch.
When and evidence. 2026-08-26, while both boards were still blank, before the repository
existed; stated in the header of `platformio.ini`. It proved itself on 2026-08-28, when the
Bluetooth receiver passed its first bench test on this core. The toolchain is a standing pin:
do not update it.

**Decision.** Pin ESP32-A2DP to commit `3245602`.
Why. The A32's Bluetooth-visibility rule overrides the library's internals (2026-09-24),
and an unpinned dependency changed whenever the `.pio` folder was rebuilt.
Rejected. Following the library's main branch.
When and evidence. 2026-09-25.

**Decision.** One PlatformIO project with two environments and a shared `include/`.
Why. The pin map and the protocol must be the same file for both boards by construction. During
bring-up, a pin map kept in more than one place drifted apart five times.
Rejected. Two separate projects.
When and evidence. 2026-08-30/31, before the repository; the repository starts with it
(2026-09-01).

**Decision.** Stamp the version at build time from the build PC's clock.
Why. The useful question is "which binary", and the pair will run builds from different
moments because the S3 updates the A32.
Rejected. A version number kept in a file.
When and evidence. Present from the first commit (2026-09-01). The format
`v.X.YYYYMMDDTHHMMSS` is the author's convention, used unchanged.

**Decision.** MAJOR goes to 1, means only the author's release, and a separate commit stamp is
added; the release is tagged `v.1.0`.
Why. Before this, the stamp could not name its source (a build was matched to its commit by
comparing times), and MAJOR had two stated meanings (a protocol break, and "finished"). The
protocol already has its own number.
Rejected. Putting the hash inside the version string: it would not fit the 24-byte handshake
field, and widening the field is a protocol change.
When and evidence. 2026-09-25 (`MAJOR = 1` and `FW_COMMIT` in `scripts/version.py`,
shown in the banner, the portal, the settings file and the A32's boot report); tag `v.1.0` on
2026-09-25. History: until then MAJOR was held at 0, by a rule
recorded in `version.py` as "v.1 is when it is done - fully calibrated, OTA confirmed, and every
outstanding problem resolved"; v.1 was declared on 2026-09-25. (`PROTO_VERSION` had meanwhile
gone from 2 to 4 with no change of MAJOR.)

**Decision.** Serve the portal page gzipped from program memory, compressed at build time,
with a stable ETag, and fail the build if its JavaScript does not parse.
Why. Page loads took 0.7 to 3.6 s uncompressed on a one-connection server; a broken script
once shipped a dead page that the author found on the hardware.
Rejected. Compressing per request (the ROM has no compressor); a filesystem partition for the
page (a second thing to upload and to keep in step).
When and evidence. Before the repository (2026-08-31); present since 2026-09-01.

**Decision.** Give the A32 two program slots (`min_spiffs.csv`) instead of the bench suite's
single 3 MB slot (`huge_app.csv`).
Why. Without a second slot there is no over-the-air update, and the A32 is inside the cabinet.
Rejected. `huge_app.csv`, which has no OTA partition at all.
When and evidence. 2026-08-30/31, when the firmware project was created; the change needed one
USB flash, and NVS at 0x9000 in both tables kept the stored settings.

**Decision.** Relay the A32's update through the S3, one acknowledged 1 KB frame at a time,
with a whole-image CRC32 at the end, and require the A32's final answer.
Why. The A32 has no network of its own in this design and no reset wire from the S3; a
transfer that cannot tell whether it arrived is worse than a slow one.
Rejected. Unacknowledged streaming; treating a silent end as success (fixed on
2026-09-24).
When and evidence. Implemented and proven over the air 2026-08-31, before the repository;
END confirmation added 2026-09-24.

**Decision.** Check the chip ID in the image header before writing anything.
Why. The two pickers sit side by side and both files are called `firmware.bin`.
Rejected. Trusting the picker.
When and evidence. Present at the first commit, 2026-09-01.

**Decision.** The A32 confirms a relayed image only after a minute of running with the S3 (or
five minutes with no S3 traffic at all), refuses a new update while on trial, and runs a
15-second loop watchdog.
Why. An update that cannot boot must not need the USB cable, and there is no reset wire from
the S3. The watchdog turns a hang into the reset that triggers the rollback.
Rejected. Confirming at the handshake (it lands before Bluetooth starts); a five-minute
fallback while bytes arrive but no handshake completes (a broken link would then be
confirmed).
When and evidence. 2026-09-25 (rollback and watchdog); refined on
2026-09-25 (one minute after the handshake; refuse updates on trial); at v.1.0 (a
build whose link self-test failed is never confirmed). Proven live the same day: an image
booted "ON TRIAL (rollback armed)", a second update sent at once was refused, and the image
was confirmed 60 s after the handshake. At the v.1.0 release a deliberately hanging image was
relayed to the A32; its watchdog reset it and it came back on the previous image (3.7).
The A32 was given a 64-bit uptime at the same time. The refusal of an update while on trial, and
the timings (one minute after the handshake, five minutes with no S3 traffic, a retry every 10 s),
belong to the same decision.

**Decision.** The S3 gets the same treatment: a trial period, refusal of S3 updates while on
trial, a failed self-test that degrades instead of halting, and a 15 s task watchdog on
`loop()` and the needle supervisor.
Why. Before v.1.0, the Arduino core marked every new S3 image valid before `setup()` ran, so an
image that crashed, hung or could not serve the portal needed the USB cable, and a failed link
self-test halted the S3 forever.
Rejected. Watching the portal task and the step emitter with the watchdog (a flash write
legitimately blocks the portal task, and the emitter busy-spins by design); requiring the A32
or the needle for confirmation (a broken audio board or a homing fault must not roll back a
good main board).
When and evidence. 2026-09-25. Proven live the same day, by OTA with no USB cable
connected: the image booted on trial, a second S3 upload during the trial got HTTP 409, the
image was confirmed; then a deliberately hanging image was reset by the watchdog and rolled
back (3.7).

**Decision.** Write no settings while an S3 image is on trial, and let a portal reboot during
the trial go through at once.
Why. A new image that changed the settings layout wrote the migrated layout within seconds of
its first boot. A rollback after that left the previous image in the downgrade lock, and the
only defence was a procedure a person had to remember (keep a settings file, change nothing
until confirmation). A reboot that first tried to save would have had nothing it could save,
and refusing it would take away the one button that undoes a bad update.
Rejected. Writing during the trial and relying on the person to keep a settings file as the
key that unlocks the old image.
When and evidence. v.1.0.1, 2026-09-25 (`settingsWrite()` and `settingsForceFlush()` in
`src/s3/main.cpp`, `hReboot()` in `src/s3/portal.cpp`). Proven live the same evening: a portal
reboot of v.1.0.1 during its trial brought the S3 back on v.1.0 with `ROLLED BACK` in its
image line; v.1.0.1 was then sent again and confirmed.

**Decision.** Write no settings while an A32 image is on trial either; write what was held the
moment the image is confirmed, and say so on the link.
Why. The A32 keeps a stored settings structure only when its size matches, so a trial image that
changed `ProtoAudio` or `ProtoBtCfg` and saved, then was rolled back, left the previous image on
defaults for that structure. The S3 had been protected since v.1.0.1; the A32 had not.
Rejected. Advising a settings export before any update that changes an A32 structure (the rule at
v.1.0.3).
When and evidence. 2026-09-26, tag `v.1.0.4` (A32 only; `settingsFlush()` and
`confirmImage()` in `src/a32/main.cpp`). Proven live the same morning: a change made during the
trial was written only at confirmation (3.2.9).

**Decision.** Identify every board by its MAC before writing to it, pass the port explicitly,
and never flash a machine under observation.
Why. Several boards of the same kind share USB IDs on the bench; an update during a listening
or measuring session spoils it.
Rejected. PlatformIO's automatic port detection.
When and evidence. A rule since the bring-up (2026-08); applied to every OTA of 2026-09-24/25
through the ARP table.

## 3.5 Failures and recovery

| What fails | What the firmware sees | What it does | How to recover | Ladder class |
|---|---|---|---|---|
| Build: portal script does not parse | `page.py` raises | the S3 build fails | fix `data/portal.html` | — (build time) |
| Build: `esprima` missing | import fails | warns, builds without the check | install `esprima` into PlatformIO's Python | — (build time) |
| Wrong image in a picker | chip ID in the header | refuses before writing | pick the other file | NOTE |
| Upload connection dies | no chunk for 15 s | `otaWatchdog()` aborts, S3 resumes | nothing; retry | NOTE |
| S3 update fails without restart | `Update` error | reports it, restores the interrupted needle state | retry | NOTE |
| S3 update reports failure but landed | the restart cut the answer | — | read `/api/boot` `fw` after the restart | NOTE |
| S3 upload while the running S3 image is on trial | `s3ImageOnTrial()` true | drops the bytes, quiets nothing; HTTP 409 `this firmware is still on trial - retry in a minute` | wait for `image confirmed`, then upload again | NOTE |
| New S3 image crashes or hangs before it is confirmed | a panic, or the 15 s task watchdog on `loop()` or the needle supervisor | the chip resets; the bootloader starts the previous image; the image line says `- an earlier update was ROLLED BACK`, `s` shows the reset reason | nothing; fix the build and upload again | NOTE (proven live 2026-09-25) |
| New S3 image runs, but its portal task stops turning or no network comes up | `confirmTick()` never sees all three conditions | the image is never confirmed and stays on trial; S3 uploads are refused | any reset returns to the previous image; with the portal unreachable, that means a full power cycle, because the front power switch neither resets the S3 nor removes its power (Bible §10, §21) | BLOCKER, but no longer USB-only (never exercised) |
| New S3 image misbehaves during its trial, portal reachable | the person sees it | nothing by itself | press Reboot in the portal: it answers `rebooting - this firmware was still on trial, so the previous firmware comes back` and restarts at once; the previous image boots | DEFECT (proven live 2026-09-25) |
| S3 link self-test fails at boot | `protoSelfTest()` false | `[FAIL] proto self-test - this image will NOT be confirmed`; the boot carries on (display, needle, link, WiFi, portal) | an image sent over the air: a portal reboot rolls it back (S3 uploads are refused while it is on trial). An image written by USB is not on trial: upload a good build through the portal | DEFECT (never exercised) |
| Settings changed during the S3's trial, then the trial ends in a reset (rollback or power cut) | nothing was written during the trial | the changes are gone; the stored settings are those from before the update, in the layout the previous image knows | set them again once an image is confirmed | NOTE (by design) |
| A32 update started while its image is on trial | `gImgOnTrial` and not confirmed | refuses: `still on trial - retry in 1 min` | wait a minute | NOTE |
| A32 not yet said HELLO | `peerHello` false | S3 refuses to begin: `the A32 is not answering` | wait for the handshake | NOTE |
| Relay stops mid-stream | no frame for 20 s on the A32 | the A32 gives up and restores the user's mute; the S3 sends no abort | retry | NOTE |
| A32's final answer lost | no `ok, rebooting` in 4 s | portal reports "did not confirm" | read the A32's version in the console after it reconnects | NOTE |
| Relayed A32 image crashes or hangs at boot | watchdog or crash reset while on trial | the bootloader starts the previous image; the boot report says `ROLLED BACK` | nothing | NOTE |
| Relayed A32 image runs but never completes a handshake | bytes arrive, no HELLO | stays on trial, never confirmed | a reset returns the previous image; the S3 cannot reset the A32 without the link, so a full power cycle: the front power switch does not remove the A32's power (Bible §10, §21), and the firmware only puts the A32 to a soft sleep | BLOCKER (never exercised) |
| A32 confirmation write fails | `esp_ota_mark_app_valid_cancel_rollback()` error | retries every 10 s; updates refused meanwhile | nothing | NOTE |
| Audio-board settings changed during the A32's trial, then the trial ends in a reset (rollback, "reboot the audio board", unplugging) | nothing was written during the trial | the changes are gone; the previous image finds its stored settings as it left them | set them again once an image is confirmed | NOTE (by design) |
| Deliberate downgrade (by USB, or over the air once a newer image was confirmed) to firmware that does not know the stored settings version | unknown `SETTINGS_VERSION` | defaults run, settings locked until a complete settings file is uploaded | upload a settings file (§7.2.7) | DEFECT (never exercised) |

## 3.6 Graveyard

These are approaches that were tried here and dropped here. They are not verdicts on the
approach in general; your setup may differ.

- **One 3 MB program slot on the A32 (`huge_app.csv`).** Used by the bench suite. It left no
  room for an over-the-air update, and the A32 sits inside the cabinet. Dropped when the
  firmware project was created.
- **An unpinned ESP32-A2DP.** Worked for a month. Dropped when the Bluetooth-visibility rule
  came to depend on the library's internals (2026-09-25).
- **Sending the portal page uncompressed.** Page loads of 0.7 to 3.6 s on this server. Replaced
  by build-time gzip.
- **Waiting 5 s for the A32 to answer BEGIN.** Failed consistently here on 2026-09-01: the
  update never started (nothing was written). Now 20 s.
- **A 250 ms fade wait before the A32 writes flash.** Measured as too little margin once the
  audio buffers were counted; now 400 ms (2026-09-12).
- **Trusting the HTTP answer of an update.** Several updates reported failure and had landed.
  The version read after the restart is the proof.
- **Treating the relay's silent end as success.** A lost final answer hid whether the A32 had
  taken the update, and the S3 went on showing the old A32 version (2026-09-24).
- **Confirming an A32 image at the first handshake.** The handshake lands about a second after
  boot, before Bluetooth starts, so an image that crashed later was already confirmed
  (replaced on 2026-09-25).
- **Judging a build by searching its output for `SUCCESS`.** Hid a linker failure. Exit codes
  only.
- **A version stamp without a commit.** Mapping a running build to its source meant comparing
  the build time with commit times (the last S3 build was stamped 28 s before its commit).
  Replaced by `FW_COMMIT` (2026-09-25).
- **Letting the Arduino core confirm every new S3 image.** Here the core marked each new image
  valid before `setup()` ran, so an S3 build that crashed or hung at boot looped on itself and
  only the USB cable could fix it. Replaced by the S3's own trial and confirmation
  (2026-09-25).
- **Saving settings during the S3's trial, with a settings file kept as the key.** At v.1.0 a
  trial image saved like any other, so an image that changed the settings layout wrote it
  within seconds, and its rollback would have locked the previous image's settings. The
  defence was a procedure: keep a settings file, change nothing until confirmation. Replaced by
  holding every write until confirmation (v.1.0.1, 2026-09-25), which needs no procedure.
- **Saving settings during the A32's trial.** Lasted until v.1.0.4. A trial image that changed an
  A32 settings structure would have written it, and a rollback would have run that structure on
  defaults; the defence was advice (export a settings file first). Replaced by the same hold as the
  S3's (v.1.0.4, 2026-09-26).

## 3.7 Limits and firmware notes

- The A32's version is not in any portal JSON field. It is readable only in the S3's console
  (which the portal shows), at each handshake and in the A32's boot report (§12.4.6).
- The first real rollbacks on either board happened on 2026-09-25, at the v.1.0 release, all
  over the air with the USB cables unplugged. On each board a deliberately hanging image
  (`loop()` spinning forever, built from uncommitted code and never committed) was sent as an
  update. The S3's task watchdog reset it and it came back on v.1.0 with
  `image      : valid, watchdog on - an earlier update was ROLLED BACK` and
  `last reset : *** TASK WATCHDOG ***`; the needle homed from its remembered position. The
  A32 came back on v.1.0 with `boot: commit <hash>, reset reason 6, ... image valid,
  watchdog on - an earlier update was ROLLED BACK` (6 is the task watchdog). Before that, both
  boards had booted a real update on trial, refused a second update during the trial, and
  confirmed it.
- The third real rollback, on 2026-09-25 with v.1.0.1, was a portal reboot during the S3's
  trial: the page answered `rebooting - this firmware was still on trial, so the previous
  firmware comes back`, and the S3 came back on v.1.0 with `ROLLED BACK` in its image line.
  Every release since (v.1.0.1 to v.1.0.5) was sent over the air, booted on trial and was
  confirmed on each board it changed. With v.1.0.4 (A32 only, 2026-09-26) the A32's settings hold
  was seen working: a change made during the trial was written only at confirmation.
- Never exercised on the radio: an S3 image that runs but never earns confirmation (portal or
  network down), a failed link self-test on either board, a confirmation write that fails, and
  a reset during either board's trial with a setting changed in it (§12.6.6).
- A relay that fails in the middle sends the A32 no abort; it stays muted until its own 20 s
  give-up (§12.4.7).
- The version stamp uses the build PC's local time without a time zone (§12.6.4).
- A partition-table change always needs a USB cable.
- The downgrade lock (a settings version this firmware does not know locks the settings) has
  never been exercised; the author does not downgrade. Since v.1.0.1 a rollback can no longer
  cause it; only a deliberate downgrade can (§12.3.16).

## 3.8 Changing this area

Invariants that must hold:

- `platform` stays `espressif32@7.0.1` and the A2DP commit stays pinned. If you move either,
  do it on purpose, rebuild both boards, and re-test the A32's audio (radio, Bluetooth and
  AUX) and its Bluetooth visibility in every mode.
- `FW_VERSION` must fit in `PROTO_VERSION_LEN - 1` = 23 characters. The commit hash never goes
  into it.
- A change that breaks the link's compatibility bumps `PROTO_VERSION`. Appending fields to
  `ProtoState` does not. MAJOR is not touched for either.
- NVS stays at 0x9000 in both tables, so stored settings survive a partition change.
- Edit `data/portal.html`, never `src/s3/page_gz.h`.
- Nothing secret goes into the source: WiFi credentials and accounts live only in NVS.
- Both OTA handlers keep their explicit admin and placeholder-password checks, and the chip
  check on the first chunk.
- Any change to a trial period keeps two properties: an image is confirmed only after it has
  shown that it runs (not merely that it booted), and an update is refused while the running
  image is still on trial. On the S3, confirmation must never wait for the A32 or the needle,
  and must never be given to an image whose link self-test failed; the same holds for the
  A32's self-test.
- The S3's `verifyRollbackLater()` keeps returning true, and the task watchdog stays the first
  thing `setup()` does, so that a hang anywhere in the boot is caught.
- Nothing reaches the S3's flash settings while its image is on trial: every write goes
  through `settingsWrite()`, which refuses on trial. A new path that writes the `amb3`
  namespace directly would reopen the rollback trap of 3.2.9.
- The portal's Reboot never waits for, or refuses on, a save while the S3 is on trial.
- The same holds on the A32: nothing reaches its stored settings (namespace `amb`) while its
  image is on trial. Every write goes through `settingsFlush()`, which refuses on trial, and
  `confirmImage()` writes what was held.

Traps:

- `extra_scripts` in an environment replaces the common list. Keep `version.py` listed in the
  `s3` environment.
- `pio device list` shows a USB bridge's serial number, not the ESP32's MAC. Use `esptool.py
  read_mac` for boards behind a bridge.
- The S3's native USB console does not reset the board when a monitor opens. The banner is
  gone by the time you look.
- `-dirty` looks only at tracked files. A new, untracked source file that is compiled in does
  not mark the build dirty. Commit before building a release.
- A setting changed during the S3's trial minute is only in RAM. The portal's Save says so.
  It is written once the image is confirmed, and lost if the trial ends in a reset.
- A second S3 upload sent inside the trial minute is refused with HTTP 409 only after the whole
  file has been sent. Wait for `image confirmed` first.

How to test a change here:

1. Build both environments and check both exit codes.
2. Update the S3 over the air. Confirm with the System tab's "running" line (the schema field
   `fw`: version and commit), not the upload's answer. The console's image line must read
   `ON TRIAL (rollback armed), watchdog on`.
3. Try a second S3 update inside the trial; it must be refused with HTTP 409. Change a
   setting and press Save in the portal; it must answer that the settings are held until
   confirmation. Then wait for
   `image confirmed (a minute of running, network and portal up).`
   If you changed anything about the trial, also check the reboot rollback: send an image,
   press Reboot during its trial, and confirm that the S3 comes back on the previous commit
   with `ROLLED BACK` in its image line.
4. Update the A32 over the air. Read the S3 console for the boot report
   (`image ON TRIAL (rollback armed)`, `watchdog on`), the new handshake version, and
   `image confirmed (a minute of running with the S3)` about a minute later.
5. Try a second A32 update inside that minute; it must be refused. Change an audio-board
   setting in that minute (the 2026-09-26 test used `btLedOff`, 255 to 254 and back afterwards):
   at the confirmation the console must show `[A32] settings changed during the trial are now
   saved`.
6. If you changed anything about the trial or the watchdog, repeat the rollback test of 3.7
   with a deliberately hanging image, and never commit that image.
7. Download a settings file and check that its second line, `# firmware ...`, names the new
   build and commit.

---

# 4. The S3: boot, tasks, clock display, panel lamps, time

## 4.1 What it does

The S3 is the main microcontroller of Ambersong. It is an ESP32-S3 module. It owns everything a
person sees move or light up on the front of the radio. It drives the four-digit amber clock behind the
dial glass (called "the Leditron" in this project). It dims the panel lamps behind the dial. It moves the
dial needle and reads the tuning knob. It senses whether the amplifier is switched on. It runs the WiFi
connection, the web portal and the firmware updates for both microcontrollers.

What a person notices, all of it driven by the S3:

- **The clock.** Four digits show local time as HH:MM, in 24-hour form by default. A leading zero in the
  hour is blanked, so 09:05 reads " 9:05". The colon between the digits is not under firmware control; it
  is always lit.
- **Two clock brightnesses.** The clock is at full brightness while the amplifier is on, and at a lower
  "standby" level while it is off. The change never snaps: the clock holds for a quarter of a second, then
  eases to the new level over a second and a half.
- **An optional frequency readout** on the same four digits. "1017" means 101.7 MHz, because the display
  has no decimal point. It can be off (the default), always on, or shown only while the tuning knob is
  being turned.
- **Blank digits** when the machine does not yet know the time. This is deliberate: a blank clock is
  better than a plausible wrong one. After every start of the S3 the clock is blank for the second or two
  it takes to get the time from the other microcontroller.
- **Panel lamps** that are dark with the amplifier off, come up to full while the knob is turned on the
  radio source, ease down to half five seconds after the knob stops, and sit at a separate level on the
  AUX and Bluetooth sources.
- **During a firmware upload** the clock goes blank, the panel lamps go dark and the needle stops. The
  radio keeps playing, because the audio lives on the other microcontroller. Everything comes back when
  the S3 restarts.

The other microcontroller is the **A32**, an ESP32-WROOM-32. It handles audio, Bluetooth, the source
selector, the volume knob and the battery-backed real-time clock chip (a DS3231). The two talk over a
serial link (chapter 9). The S3 never reads the source selector, the volume knob or the Bluetooth button,
and never drives the Bluetooth lamp. It learns the source, the Bluetooth state, the volume and the
selector's raw reading (for diagnosis) from a status frame the A32 sends four times a second. No firmware on either board drives a fan.

## 4.2 How it works

### 4.2.1 Terms used in this chapter

| Term | Meaning |
|---|---|
| **Task** | A FreeRTOS thread. The ESP32-S3 has two CPU cores, 0 and 1; each task is pinned to one core and has a priority (higher runs first). |
| **ISR** | Interrupt service routine: a function the hardware calls on a timer, pre-empting every task on its core. |
| **IRAM** | The chip's internal instruction RAM. Code placed there keeps running while the external flash chip is busy (for example during a settings save or a firmware write). |
| **`loop()`** | The Arduino main loop. It runs in the Arduino `loopTask`, on core 1, at priority 1. |
| **NVS** | Non-volatile storage: the ESP-IDF key-value store in flash where settings live (chapter 7). |
| **LEDC** | The ESP32's PWM peripheral ("LED control"). Used here for the panel lamps and the stepper coils. |
| **Handshake** | The HELLO / HELLO_ACK exchange that opens the link with the A32 (chapter 9). `peerHello` is true while it stands. |
| **Epoch** | Seconds since 1970-01-01 00:00 UTC (Unix time). |
| **`Con`** | The console object (src/s3/console.h). Everything the S3 prints goes to the USB port and into a ring buffer the portal's Console tab reads; keys come from both (chapter 8). |

### 4.2.2 Build identity

Every build is stamped at compile time by `scripts/version.py`. `FW_VERSION` has the form
`v.MAJOR.YYYYMMDDTHHMMSS`, for example `v.1.20260925T200355`. The stamp names the binary, not the source:
two builds of the same source get different stamps on purpose. The S3 flashes the A32, so the two boards
will sometimes run different builds, and the handshake has to say which is which. If the script did not
run, main.cpp falls back to `v.0.unstamped`.

MAJOR is the author's release number and nothing else. It is 1 from the v.1.0 release on. A change to the
link protocol is signalled by `PROTO_VERSION` (chapter 9), not by MAJOR.

A second stamp, `FW_COMMIT`, names the source: the short git hash, with `-dirty` appended when the build
was made from uncommitted changes to tracked files, or `nogit` when git was not available. It is printed
beside the version in the console banner (`firmware v.1.<stamp> (<hash>)`), the portal (the schema field
`fw`, the System tab's "running" line), the settings-file header (`# firmware v.1.<stamp> (<hash>)`) and
the A32's boot report (`boot: commit <hash>, ...`). It is never sent inside the version string, because
that string travels in a 24-byte field of the handshake and both would not fit. The release is tagged
`v.1.0` in git, on 2026-09-25; chapter 3 (§3.2.3, §3.2.10) has the details. This chapter
describes the S3 at tag `v.1.0.5` (2026-09-27); `v.1.0.4` changed only the A32, and
`v.1.0.5` changed only the S3's rescue access point (chapter 8), so nothing in this chapter differs
from `v.1.0.3` (2026-09-26).

Before v.1.0, MAJOR stayed at 0 until the machine was finished.

The build environment `[env:s3]` in `platformio.ini` uses the `esp32-s3-devkitc-1` board definition,
`memory_type = qio_opi`, the `default_16MB.csv` partition table (two 6.25 MiB (0x640000-byte) application slots, so the S3
can be updated over the air), `-DARDUINO_USB_CDC_ON_BOOT=1` so the console is on the chip's native USB
port, `-DBOARD_HAS_PSRAM`, `CORE_DEBUG_LEVEL=3`, and `-DAMB_S3=1` (the A32 build sets `-DAMB_A32=1`;
no source file tests either flag at this version). `build_src_filter =
+<s3/>` compiles only `src/s3/`, and two scripts run before every S3 build: `scripts/version.py`
(the stamps above) and `scripts/page.py`, which gzips the portal page into `src/s3/page_gz.h`
(§8.2.8). The module is an N16R8, 16 MB of flash and 8 MB of octal PSRAM
(Bible §1, §24), which needs `qio_opi`; a wrong memory type fails silently with 0 MB of PSRAM. The
back-panel USB-C reaches the S3's native USB port, not a USB-UART bridge (Bible §1, §2). The board is
built with the DevKitC-1 board definition and runs; the board is compatible with the DevKitC-1 in
firmware and pin-compatible for every pin the device uses (Bible §24). The toolchain is pinned to
`espressif32@7.0.1`, which is Arduino-ESP32 2.0.17 on ESP-IDF 4.4.7. Chapter 3 covers the rest.

### 4.2.3 The boot sequence

The S3 boots whenever the set is plugged in. Both microcontrollers are powered as long as the mains
cord is in, so **an S3 boot is not the moment a person turns the radio on.** The front switch (the
amplifier coming up) is that moment. Both MCUs are powered whenever the set is plugged in, independently
of the front switch (Bible §10, §21).

`setup()` in src/s3/main.cpp runs these steps, in this order, on core 1:

1. **Watchdog and image state, first.** `esp_task_wdt_init(15 s, panic on)`, then `enableLoopWDT()`,
   which subscribes the `loopTask` (and so `loop()`) to the task watchdog. Then the S3 reads the state
   of the image it is running from: on trial, valid, or not tracked (4.2.15). This comes before
   anything else so that a hang anywhere below resets the chip.
2. **Console.** `Con.begin(115200)`. It then waits up to 3 s for a USB host to open the port, then
   300 ms more. With no cable attached this costs the full 3.3 s on every boot. The loop task is
   already watched, and the Arduino core feeds the watchdog only between `loop()` passes, so the whole
   of `setup()` must finish within 15 s.
3. **Banner.** The name, and `firmware <FW_VERSION> (<FW_COMMIT>)`, between two rules of `=`. Nothing
   else: the old "STEP 3 - display, needle, panel, link" line, left over from bring-up, was removed
   (2026-09-25).
4. **Amp sense pin.** `pinMode(S3_AMP_SENSE, INPUT)`: a plain input, no internal pull resistor.
5. **Settings.** `settingsLoad()` (NVS namespace `amb3`, key `cfg`), then `purgeCorruptLimits()` and
   `purgeCorruptSpurs()` (chapter 7).
6. **Link self-test.** `protoSelfTest(Con)` from include/link.h encodes a status frame, decodes it,
   compares the two, then corrupts one byte and checks that the checksum rejects it: a checksum never
   seen to fail is not known to work. A failure is reported and the boot carries on; such an image is
   never confirmed (4.2.15).
7. **Display.** `Display::begin()` configures the seven segment pins and four digit pins as outputs,
   all low, and starts hardware timer 0 with its interrupt every 25 µs (4.2.8). Because `setup()` runs on
   core 1, the interrupt is allocated on core 1, away from WiFi on core 0. The digits are blank from here
   until `loop()` draws the first frame.
8. **Panel lamps.** `Panel::begin()`: LEDC channel 1, 1 kHz, 8 bits, duty 0 (dark).
9. **Needle.** `Needle::begin()` sets up the index input, the stepper drive, the tuning encoder bus, and
   creates the tasks `nstep` and `needle` (chapter 5). The `needle` task subscribes itself to the task
   watchdog when it starts, which works because step 1 has already set the watchdog up.
10. **Tuning receiver.** `Rda::begin()` sets up the RDA5807M bus and creates the `rda` task. It is harmless
    if the module does not answer (chapter 6).
11. **Tuning shaft.** `Needle::setCalibration(...)`, **then** `Needle::seedAccumulator(cfg.lastAngle)`,
    then `applySettings()`. The measured tuner ends must be known before the seed, because the seed uses
    them to decide which turn of the shaft the knob is on (2026-09-25; chapter 5).
12. **Link.** `gLink.onMessage(onMessage)`, then `gLink.begin(Serial2, S3_UART_RX, S3_UART_TX)` at
    921600 baud, 8N1, no flow control (chapter 9). That speed is the firmware's own choice, and it is
    proven on every frame by the checksum. `Link::begin()` has no form without explicit pins, so the
    serial port's default pins can never be taken by accident. Nothing is sent yet; `loop()` sends the
    first HELLO.
13. **Encoder report, and the clock's starting brightness.** It prints whether the AS5600 encoder answers,
    whether it sees its magnet, and its automatic gain (AGC) reading (chapter 5).
    Then `brightnessNow(cfg.brightOff)` sets the clock to the **standby** level at once, and
    `applySettings()` runs again. The clock therefore starts dim and eases up once `loop()` has read the
    amplifier as on.
14. **The one homing.** `Needle::bootPosition()`, then `Needle::startHoming()`. `bootPosition()` looks at
    why the chip reset. After a software-type reset it restores the needle position kept in RTC memory;
    after a power-up it assumes the needle is at the low stop (4.2.14; chapter 5). This must come after
    step 13's `applySettings()`, which gives the needle its geometry.
15. **Network.** `Net::begin()` loads the WiFi, NTP and time-zone settings (their own NVS namespace,
    `net`), applies the time zone once
    (`setenv("TZ", ...)`, `tzset()`), and starts joining the house network or raises the rescue access
    point (chapter 8).
16. **Portal.** `Portal::begin()` starts the web server and its task on core 0 (chapter 8).
17. **Image line and command list.** `imageReport()` prints
    `image      : <state>, watchdog on|OFF`, with ` - an earlier update was ROLLED BACK` when the
    bootloader has thrown an earlier image away (4.2.15). Then `commandList()` prints the console keys.

Four orderings matter and are enforced only by the order of the code: the watchdog before everything
(step 1); tuner ends before the shaft seed (step 11); geometry before the boot position (step 14);
standby brightness before the first amplifier reading (step 13).

### 4.2.4 Tasks and interrupts

| Name | Priority | Core | Period | Stack | What it does | What may block it, or what it may block |
|---|---|---|---|---|---|---|
| `onTick` (display ISR), `Display::begin()` in src/s3/display.cpp | interrupt | 1 | every 25 µs | – | Multiplexes the four digits (4.2.8). IRAM, register writes only. | Must never block. Pre-empts everything on core 1. |
| `loopTask` (`setup()` then `loop()`), Arduino core | 1 | 1 | free-running, no delay between passes | 8192 | The link, amp sense, display content, brightness, panel lamps, time, console (4.2.6). Watched by the task watchdog (15 s), fed by the core once per pass. | Pre-empted by `needle` and `rda`. Console prompts hold it (`W` up to 40 s, `y` up to 60 s, `l` up to 60 s) and feed the watchdog while they wait; `i` can hold it for seconds; an NVS save holds it for milliseconds. |
| `nstep` (`stepTask`), src/s3/needle.cpp | 19 | 0 | 1 kHz control tick | 4096 | Emits the stepper steps (chapter 5). | Busy-spins between steps while the needle moves; `vTaskDelay(1)` when idle. Starves `portal` and WiFi during a move. |
| `needle` (`needleTask`), src/s3/needle.cpp | 4 | 1 | every 5 ms | 4096 | Needle supervisor, encoder reads (chapter 5). Watched by the task watchdog (15 s), fed at the top of every pass. | I2C transactions to the encoder. |
| `portal` (`serverTask`), src/s3/portal.cpp | 3 | 0 | one `vTaskDelay(1)` per pass (1 ms) | 8192 | HTTP server (chapter 8). | Starved while the needle moves. Firmware uploads write flash from here. |
| `rda` (`rdaTask`), src/s3/rda.cpp | 2 | 1 | 100 ms when idle; 300 ms per channel during a sweep | 3072 | RDA5807M sampler (chapter 6). | I2C transactions to the RDA5807M. |
| WiFi, lwIP (runs the SNTP callback), esp_timer, IDLE0, IDLE1 | ESP-IDF defaults | WiFi pinned to 0 | – | ESP-IDF | Network stack and system. | – |

Consequences a modifier must know:

- **On core 1, `loop()` never yields.** It has no delay in its normal path. So the idle task of core 1
  (IDLE1) never runs, and nothing at priority 0 ever gets core 1. A task at priority 1 on core 1 shares it
  with `loop()` only by time slicing. Periodic work belongs in `loop()` or in a task at priority 2 or
  higher that yields. The task watchdog does not check IDLE1 in this build's configuration, so this does
  not cause resets.
- **On core 0, `nstep` spins at priority 19 during a move**, so `portal` (3) and WiFi get core 0 only
  between steps. Portal requests can be slow while the needle moves; this is known and accepted
  (4.4). IDLE0 cannot run during a move either, so `stepTask` removes IDLE0 from the task watchdog
  (`esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0))`). Before v.1.0 this meant the task watchdog
  watched nothing on the S3. Since v.1.0 it watches `loop()` and the `needle` task, and still
  not IDLE0, `nstep` or `portal` (4.2.15).
- **Why the step emitter is on core 0.** On core 1 it would starve `loop()` (the link and the panel) for
  the length of every sweep, and it would compete with the display interrupt. A needle stutter while the
  portal is in use is more forgivable than a stalled link or a glitched clock.
- **Why the portal is on core 0.** Core 1 belongs to the display interrupt and the needle.

### 4.2.5 A map of src/s3/main.cpp

main.cpp is long (about 3400 lines). This is its layout, in file order.

| Section (main functions) | What it does | Chapter |
|---|---|---|
| Header comment, includes, `FW_VERSION` / `FW_COMMIT` fallbacks | – | 4 |
| Settings constants and structs: `SETTINGS_MAGIC`, `SETTINGS_VERSION`, `SettingsV1`, `SettingsV2`, `Settings` | The one stored settings structure and its history. | 7 (display and panel fields in 4.3) |
| Globals: `cfg`, `gTouchGen` / `gSavedGen`, `gLink`, `a32` (mirror of the A32's status), `a32cfg` (mirror of the A32's settings), `haveState`, `peerHello`, `ampOn`, `ampKnown`, `epochAtSync`, `millisAtSync`, `timeValid` | Shared state of the S3. | 4, 7, 9 |
| `dispTest`, `brightnessTarget()`, `brightnessNow()`, `updateBrightness()` | Eased clock brightness. | 4 |
| `refuse()`, `captureLimit()`, `purgeCorruptLimits()` | Refusal flag; soft-limit capture. | 5, 7 |
| Tuning curve: `setTuneModel()`, `fitTuneCurve()`, `solveLS()`, `purgeCorruptSpurs()` and helpers | Maps shaft angle to frequency. | 6 |
| `applySettings()` | Pushes `cfg` into every module: clock brightness target, panel levels and timings, needle motion and geometry, tuning curve, dial face, RDA spur list, WiFi transmit power. | 4 (hook), all |
| `settingsLoad()`, `settingsDump()`, `settingsTouch()`, `settingsWrite()`, `writeSafe()`, `settingsFlush()` and siblings | Load, migrate and save settings. | 7 |
| `nowEpoch()` | The software clock. | 4 |
| `otaWaitAck()`, `otaSendFrame()`, `a32OtaBegin/Chunk/End/SetError/Abort()` | Relays an A32 firmware upload over the link. | 9 |
| `#include "settings_table.h"` | The settings table, actions, settings-file round trip. | 7, 8 |
| `onMessage()` | Handles every frame from the A32: HELLO_ACK, STATE, TIME, CFG, OTA_STATUS, LOG. | 4 (TIME, STATE), 9 |
| `srcName()`, `btName()`, `portalRdaJson()`, `portalStateJson()` | The JSON the portal polls once a second. | 8 (health fields in 4.2.14) |
| `verifyRollbackLater()`, `s3ImageOnTrial()`, `wdtFeed()`, `imageReport()`, `confirmTick()` (banner "ROLLBACK AND THE WATCHDOG, THE A32'S TREATMENT") | The image's trial, its confirmation, and the task watchdog. | 4 (4.2.15) |
| `otaQuiet`, `otaResume`, `portalNeedleResume()`, `portalOtaQuiet()` | Dark and still during an upload; resume if it ends without a restart. | 4, 8 |
| `radioLive()`, `applyNeedleMode()` | "The tuner is what is playing"; track or park the needle. | 4 (hook), 5 |
| RDA sampler and re-index triggers: `sampleStart()`, `sampleStore()`, `sampleTick()`, `autoSampleTick()`, `needleRecoverTick()`, `idleIndexTick()` | Measures the dial with the RDA5807M; re-index triggers. | 5, 6 |
| `updateDisplay()` | Decides what the four digits show. | 4 |
| `drainLine()`, `setWifiInteractive()` (`y`), `setClockInteractive()` (`W`), `limitMonitor()` (`l`), `commandList()`, `status()` (`s`) | Console prompts and the status page. | 8; `W` and `s` here |
| `setup()`, `loop()` | Boot and the main loop, with the console key dispatcher at its end. | 4 |

### 4.2.6 `loop()`, pass by pass

Every pass runs these steps in this order. There is no delay between passes.

1. `gLink.poll()`: receive bytes from the A32; complete frames go to `onMessage()` (chapter 9).
2. `Net::loop()`: the WiFi state machine and SNTP bookkeeping (chapter 8). Then `confirmTick(millis())`:
   while the running image is on trial, the check that confirms it (4.2.15).
3. If `Net::txPower()` differs from `cfg.wifiTxQ`, copy it into `cfg` and mark settings changed. The
   WiFi transmit power is learned by the network module and stepped only from the console; it is not a
   portal row and not in the settings file, and its 15 dBm ceiling is a firmware choice (chapter 8).
4. `sampleTick()`, `autoSampleTick()`, `needleRecoverTick()`, `idleIndexTick()`: the RDA sampler and the
   needle's re-index triggers (chapters 5, 6).
5. `settingsFlush()`: save settings if they have been unchanged for 2 s and it is safe to write
   (`writeSafe()`: the needle is still, no calibration is running and no upload is active; 4.8,
   invariant 6). Nothing is written while the running image is on trial (4.2.15; chapter 7).
6. `updateBrightness()`: advance the clock brightness fade (4.2.10).
7. **Amp sense** (4.2.7): debounce; on a change, retarget the clock brightness, maybe home or sweep the
   needle, re-apply the needle mode, and tell the A32 at once. Otherwise tell the A32 the amp state every
   2 s while the handshake stands.
8. **Link keepalive**: send HELLO every 1 s until the A32 answers; after that send PING every 1 s and
   GET_TIME every 60 s. A GET_TIME that gets no answer within 5 s while the A32 is otherwise answering
   marks the battery clock as not answering (4.2.14). If the link has been silent for more than 2 s, drop
   the handshake (`peerHello = false`, `haveState = false`), forget the copy of the A32's settings
   (`haveCfg = false`, chapter 7) and print "[WARN] A32 silent. Clock and needle keep running."
9. **Needle mode follows source and amp**: on a change of source, or when homing finishes, call
   `applyNeedleMode()` (not during an upload). If an upload ended without a restart and the needle's
   stop has landed, home an unhomed needle or re-apply the mode (chapter 5).
10. **Band-calibration collector**: `Needle::calTakeBand()`; if a calibration finished, copy its result
    into `cfg`, shift the soft limits with the frame and save (chapter 5).
11. **Panel lamps**: `Panel::update(ampOn, source, Needle::tuningActive())`, or
    `Panel::update(false, SRC_AUX, false)` (dark) during an upload.
12. **Display content**: `updateDisplay()` every 200 ms, skipped during an upload.
13. **NTP write-through**: at most once an hour, push NTP time to the DS3231 (4.2.13).
14. **Shaft position**: every 10 s, if the encoder answers and the shaft count changed, store it in
    `cfg.lastAngle`, so the next boot can tell which turn of the shaft the knob is on. Only a live
    count is stored: before the encoder's first good reading the count is a placeholder, and saving it
    would seat the next boot on the wrong turn (2026-09-25; chapter 5).
15. **Console**: take one key from `Con` and dispatch it (4.3.2; chapter 8).

When the link is down, the source is taken as AUX everywhere in `loop()` (`haveState ? a32.source :
SRC_AUX`). With the amp on, this parks the needle, sets the panel to its "other source" level and stops the
"while tuning" readout.

### 4.2.7 Amp sense: what "on" means

The firmware reads `S3_AMP_SENSE` with `digitalRead()`. **LOW means the amplifier is powered.** The pin
is a plain input; the pull-up and the detector that pulls it LOW are external (Bible §4, §7 "Power
Detect").

- **Debounce.** A new reading is accepted only after it has differed from the accepted state for more
  than 50 ms. The first accepted reading after boot sets `ampKnown`; before that the console reports the
  amp as "unknown".
- **On an accepted change**, `loop()` prints "amp ON" or "amp OFF" and retargets the clock brightness
  (`brightOn` or `brightOff`). If the amp has just come **on**, and this is not the first reading after
  boot, and no upload is running:
  - if the needle has no zero, it starts homing ("the front switch is the natural place to try
    again"). That includes a needle in FAULT: switching the amplifier on is a person asking for another
    try (chapter 5);
  - otherwise, if `cfg.sweepOn` is set, the needle makes a decorative end-to-end sweep
    (`Needle::sweepRange(false)`).

  Then `applyNeedleMode()` runs and the new amp state is sent to the A32 at once (`MSG_SET_SYS`).
- **The front switch does not home the needle.** Homing happens once, at S3 boot. After that the needle
  knows where it is (chapter 5).
- **The A32 is told the amp state every 2 s.** The A32 treats 2 s without hearing from the S3 as "amp
  down" and goes quiet (chapter 9). This is why any console routine that holds `loop()` must keep
  sending PING, as `limitMonitor()` does.
- **One definition of "the radio is playing":** `radioLive()` returns `ampOn && source == SRC_RADIO`,
  with the source taken as AUX when no valid status frame is held. It decides needle tracking versus
  parking, the "while tuning" readout, and whether an RDA sample may run. **It is a listening policy,
  not a statement about the tube set.** The selector only tells the A32 which sound to play; the tube
  set runs whenever the amplifier does, whatever the selector says (Bible §5, §21). The needle follows
  and the dial calibrates only while the radio is what you are listening to, by choice.
- After two minutes with the amp off, once per off period, the needle quietly re-homes and returns
  (`idleIndexTick()`, chapter 5).

### 4.2.8 The Leditron: multiplexing

The display has four seven-segment digits. **Only one digit is lit at any instant.** Each digit gets a
2500 µs slot; four slots make a 10 ms frame, so the whole display refreshes at 100 Hz. Hardware timer 0
fires the interrupt `onTick()` every 25 µs (a prescaler of 80 on the 80 MHz clock gives 1 µs ticks; the
alarm reloads at 25). 25 µs is the greatest common divisor of 825 and 1675, so both phases below fall on
whole ticks.

One slot is 100 ticks:

```
tick  0 ........ 32 | 33 ...... 33+L | ........ 99
      dark (825 us)   lit (L ticks)    dark
      dead time       digit, then      rest of slot;
      never shortened segments ON      tick 99: blank everything,
                                       next digit
```

- **Ticks 0 to 32 (825 µs): everything dark.** This is the dead time. It gives the outgoing digit's
  switch time to turn fully off before the next digit is enabled; without it the display ghosts. On this
  machine the outgoing digit's high-side switch takes about 825 µs to stop conducting once its select
  is released. That figure was measured here on 2026-08-27, and it is the firmware's own value: the
  Bible records the circuit that causes the slow turn-off (Bible §9), not the timing. Another build of
  the display must measure its own.
- **Tick 33:** timestamp the slot with `esp_timer_get_time()` and record the worst deviation from
  2500 µs. Then, if the lit window is not zero, set the digit-select bit(s) **first**, then the segment
  bits of that digit's pattern.
- **Tick 33 + L:** clear the segments **first**, then the digit. Releasing the digit first would let the
  outgoing filament briefly show the incoming pattern.
- **Tick 99: unconditional blank.** Clear all segments and the digit, reset the tick counter, and move to
  the next digit (`(curDigit + 1) & 3`). This blank runs whatever L is.

**Polarity.** The ISR sets a digit-select pin HIGH to turn its digit on and a segment pin HIGH to light a
segment, through the driver stages (Bible §9).

**Register writes.** `Display::begin()` precomputes one bit mask per segment, a mask of all segments
(`segAllMask`), and for each digit a mask in the low output register (GPIO 0–31) and one in the high
output register (GPIO 32 and up). The ISR then does only a handful of writes to the set and clear
registers (`GPIO_OUT_W1TS/W1TC`, `GPIO_OUT1_W1TS/W1TC`). All segment pins are below GPIO 32; three of the
four digit pins are above it.

**Brightness** changes only the lit window L, never the slot. `setBrightness(b)` computes
`L = b × 67 / 255`, uses at least 1 tick when b > 0, and clamps L to at most 66. So the frame rate never
changes and dimming cannot flicker. 255 is the maximum; there is no headroom above it. 0 is dark, but the
display keeps refreshing.

Worked example: at 255, L = 66 ticks = 1650 µs lit in each 2500 µs slot, so each digit is lit 16.5% of the
time. At the standby default of 110, L = 110 × 67 / 255 = 28 ticks = 700 µs, or 7% of the time.

**Font.** Digits 0–9 only, in the standard layout (bit 0 = segment A at the top, then B, C, D, E, F,
bit 6 = G in the centre). Any other value, and -1, blanks that digit. **Index 0 is the rightmost digit.**
Segment pins `S3_SEG_A`..`S3_SEG_G` light segments A..G in the standard layout (Bible §8, §9). Digit
selects `S3_DIGIT_1`..`4` are the rightmost to the leftmost digit (Bible §2, §4, §8).

**Interface** (src/s3/display.h): `begin()`, `setBrightness()` / `brightness()`, `showDigits(int8_t[4])`,
`showTime(hh, mm, hour12, blankLeadingZero)`, `showNumber(v, blankLeadingZeros)`, `blank()`,
`worstSlotErrorUs()`, `resetSlotStats()`. `showDigits()` copies the four patterns with interrupts
disabled, so the ISR never shows half of an update. `showTime()` in 12-hour form uses `hh % 12` with 0
shown as 12; there is no AM/PM indicator.

**Timing health.** The worst slot deviation is shown by the console `s` ("display : brightness N worst
slot error N us") and in the portal JSON field `slot`. `Z` zeroes it.

### 4.2.9 The Leditron: what it shows

`updateDisplay()` in main.cpp runs every 200 ms from `loop()` and decides, in this order:

1. **Test patterns.** `dispTest == 1` shows 8888 (console `8`, portal action `disp.test`);
   `dispTest == 2` blanks the display (console `b`); `9` returns to normal. A display you can command is
   the difference between "the clock is broken" and "the clock has no time yet".
2. **During an upload** `updateDisplay()` is not called at all; `portalOtaQuiet(true)` has already blanked
   the digits (4.2.12).
3. **The frequency readout**, if wanted:
   - `showTuning == 1` ("tuning always"): always, whatever the amp and source. This is an operator
     override and the calibration instrument; calibrating the shaft against the printed dial is done
     standing at the set, often with the amp off.
   - `showTuning == 2` ("tuning while tuning"): only if `radioLive()` and the knob moved (as
     `Needle::tuningActive()` reports, chapter 5) within the last `tuneHoldMs` (4 s by default). The
     "last turn" time is **not** cleared while the radio is not live. A 250 ms glitch of the source
     selector or a 2 s link hiccup would otherwise wipe a readout the listener is in the middle of reading.

   The value comes from `Needle::tuneFreq10f()`, the single definition of the tuning curve (chapter 6),
   clamped to the printed dial face `dialLow..dialHigh` (87.9–107.9 MHz by default). Then:
   - mode 2 snaps to the nearest North-American FM channel. Channels sit on odd tenths of a MHz
     (87.9 + 0.2 k). The snap uses the unrounded value and never leaves the printed face;
   - mode 1 is not snapped. It is the instrument, and rounding would hide the tenth of a MHz being
     measured.

   The number is shown with `showNumber(f, false)`, so leading zeros are **not** blanked: 87.9 MHz reads
   "0879". The portal shows the true, unclamped frequency with "(past the printed face)" when it is off
   the dial; only the Leditron clamps.
4. **No valid time:** all four digits blank.
5. **The clock:** local time from `nowEpoch()` through `localtime_r()`, shown with
   `showTime(hour, minute, cfg.hour12, cfg.blankLeadZero)`. The time zone is whatever the `TZ`
   environment variable holds, set once by `Net::begin()` from the stored rule (4.2.13).

### 4.2.10 Clock brightness

The clock has two targets: `brightOn` (255 by default) while the amp is on, `brightOff` (110) while it is
off. The easing is in main.cpp:

- `brightnessTarget(t)` starts a new move **from the current level**, so a change of mind mid-fade never
  jumps.
- `updateBrightness()`, called every `loop()` pass, waits `dispDwellMs` (250 ms), then moves along a
  smoothstep curve, `k = p²(3 − 2p)`, over `dispFadeMs` (1500 ms). Smoothstep has zero slope at both
  ends, so the level starts slowly, speeds up and settles.
- `applySettings()` retargets whenever any setting changes, so a new `brightOn` from the portal eases in.
- `brightnessNow(v)` sets a level at once. It is used only at boot (4.2.3, step 13).

The console keys `-` and `=` (or `+`) change `brightOn` by 15 and write it straight to the display,
bypassing the fade (4.7).

### 4.2.11 Panel lamps

The panel lamps are one PWM output, `S3_PANEL_PWM`, on LEDC channel 1 at 1 kHz with 8-bit duty
(src/s3/panel.cpp). A higher duty makes the panel lamps brighter (Bible §11, "Panel radio LED PWM
control"; §22). The 1 kHz frequency is a firmware choice. Channel 0 is left unused on the S3 because the A32 uses channel 0 for its Bluetooth lamp; keeping them
distinct makes the two firmwares read alike.

`Panel::update(ampOn, source, tuning)` is called on every `loop()` pass. The target level is derived from
those three inputs alone, so there is no internal state to fall out of step:

| Condition | Target |
|---|---|
| Amp off | 0 (dark) |
| Amp on, source RADIO, knob moved within `panelIdleMs` (5 s) | `panelTuning` (255) |
| Amp on, source RADIO, knob idle for 5 s | `panelIdle` (128) |
| Amp on, source AUX or Bluetooth (or link down) | `panelOther` (96) |
| Firmware upload in progress | 0 (`loop()` passes amp off) |

`tuning` is `Needle::tuningActive()`. Its deadband, in needle.cpp, must be wider than the encoder's own
jitter, or the lamps flash to full with nobody touching the knob (chapter 5).

**Motion.** A new target starts a move from the current level. The level holds for `panelDwellMs`
(250 ms), then follows the same smoothstep curve over `panelFadeMs` (900 ms). The hold is what makes the
machine seem to decide rather than react. `Panel::level()` and `Panel::target()` are shown by the console
`s` ("panel : current -> target").

**Interface** (src/s3/panel.h): `begin()`; `update(ampOn, source, tuning)`; the setters
`setLevels(whileTuning, radioIdle, otherSource)`, `setIdleDelayMs()`, `setFadeMs()` (0 becomes 1) and
`setDwellMs()`, which only `applySettings()` calls, with the `panel*` settings (4.3.1); and the readers
`level()` and `target()`.

### 4.2.12 During a firmware upload

The portal calls `portalOtaQuiet(true)` when an upload to either microcontroller starts (chapter 8). It
stops the needle and blanks the display. While `otaQuiet` is set:

- `updateDisplay()` is not called, so the digits stay blank;
- the panel lamps are driven dark;
- amp and source changes do not move the needle;
- settings are not saved (chapter 7).

The reason is that a firmware write is a long flash operation, and the machine cannot absorb one while it
is moving or lit. The display timing test of 2026-08-28 (4.4) found flash traffic to be the one load
that disturbs the display, and concluded that an upload should park and darken the machine rather than
let it look broken. A successful upload ends in a restart, which brings everything back. An upload that
ends without a restart (refused, aborted, failed) raises `otaResume`; `loop()` then homes an unhomed
needle or re-applies its mode, but only once the stop has landed (chapter 5).

### 4.2.13 Time keeping

**The real-time clock is on the A32.** The DS3231 is reachable only from the A32, so time must cross the
link (Bible §5). Its module carries a backup battery (Bible §22), which is
what lets it keep time through a power cut; in this chapter it is called "the battery clock", as the
portal calls it. The S3 keeps its own **software clock**:

```
nowEpoch() = epochAtSync + (millis() - millisAtSync) / 1000     (0 while !timeValid)
```

It keeps counting from `millis()` when the A32 or the network is absent. A slow or missing link never
makes the display stutter; the clock just stops being corrected.

**Three sources of time**, and how they reach the display:

```
  NTP server --(SNTP, every 3 h)--> S3 system clock --(hourly, if fresh)--+--> MSG_SET_TIME --> DS3231 (UTC)
                                                                          |
                                                                          +--> re-seat software clock
  DS3231 --(MSG_GET_TIME at handshake and every 60 s; MSG_TIME)--> re-seat software clock
  console W --(typed local time -> UTC)--> MSG_SET_TIME --> DS3231, and re-seat software clock

  software clock --localtime_r() with TZ--> display (every 200 ms), portal clock, console s
```

1. **The DS3231, through the A32.** At every handshake, and every 60 s after, the S3 sends
   `MSG_GET_TIME`. The A32 answers `MSG_TIME` with the time in UTC (`unixUtc`), a validity flag
   (`valid`) and the chip's temperature in quarter degrees (`tempC4`). Since
   2026-09-24 the A32 sets the flag only if the time is plausible (after 1600000000, that is September
   2020), the chip's status register answered, and the chip's oscillator-stopped flag (OSF) is clear: a
   clock that has stopped is not trusted (chapter 9). On the S3, `onMessage()` accepts a valid, plausible
   time and re-seats the software clock (`epochAtSync`, `millisAtSync`, `timeValid = true`). An invalid one
   prints "[WARN] the DS3231 has no valid time yet." and leaves the software clock alone, still running if
   it was valid. Either way the answer also sets the battery clock's health (4.2.14). The DS3231's
   temperature, also in `MSG_TIME`, is not used by the S3.
2. **NTP**, only while the S3 is joined to a network. `Net` calls `configTzTime(tz, server,
   "time.nist.gov")`. The stored server defaults to `pool.ntp.org`. SNTP re-syncs every 3 h (the core's
   `CONFIG_LWIP_SNTP_UPDATE_DELAY`). NTP counts as **fresh** only if the SNTP sync callback fired within
   the last 4 h (`NTP_FRESH_MS` in src/s3/net.cpp: the 3 h interval plus an hour's grace)
   (2026-09-24). **NTP disciplines the DS3231, not the other way round:** while the handshake stands and
   NTP is fresh, `loop()` sends `MSG_SET_TIME` with the NTP time at most once an hour (at once the first
   time), and re-seats the software clock. The console prints "clock disciplined from NTP." The write goes
   through to the DS3231 so the correction survives the next power cut. Without a network, which is most of
   the time, the DS3231 is the authority.
3. **By hand, the console `W`** (also from the portal's Console tab). It prompts for local time as
   `YYYY-MM-DD HH:MM:SS`, converts it to UTC with `mktime()`, refuses anything before 1600000000 and
   re-seats the software clock. Then it looks at the link. If the handshake stands, it sends
   `MSG_SET_TIME` and prints "clock set, and pushed to the DS3231." If the A32 is not answering, it sends
   nothing and says so: "clock set on this board only - the audio board is not answering, so the
   battery clock was NOT updated. Set it again once the link is back." (2026-09-25). Before
   that it claimed the push whether or not anyone received it. Writing the DS3231 clears its OSF flag.
   Unparseable input changes nothing. The prompt waits at most 40 s.

**The DS3231 always holds UTC.** Local time is applied only for display. Keeping local time in a
real-time clock makes the hour after a daylight-saving change ambiguous and the hour before it happen
twice.

**The time zone** is a POSIX TZ rule stored with the network settings and applied once by `Net::begin()`
(`applyTz()`), and again whenever it is changed from the portal (`Net::setNtp()`). The default is
`EST5EDT,M3.2.0,M11.1.0` (eastern North America: UTC−5, daylight time from the second Sunday of March to
the first Sunday of November). **Builders in another zone: set your own rule in the portal.**

**The `W` prompt's zone is hard-coded.** `setClockInteractive()` sets `TZ` to `EST5EDT,M3.2.0,M11.1.0`
before converting, whatever the portal says, and leaves it set afterwards. A code note there reads
"BUILDERS, PERSONALISE THIS". This was left as it is on purpose (4.4).

**Who reads which clock.** The display, the portal's `clock` field ("HH:MM" local, or "--:--") and the
console `s` all use `nowEpoch()`. The S3's system clock (`time()`), which SNTP sets, is used only as the
NTP source (`Net::ntpEpoch()`).

### 4.2.14 Uptime, reset reasons and health reporting

**Uptime** is `esp_timer_get_time()` (a 64-bit microsecond count since boot) divided by 10⁶. The portal
shows it as `up` ("Nd HH:MM:SS"); the console `s` shows "uptime N s". It used to come from `millis()`,
which is 32 bits and rolled over to zero every 49.7 days (2026-09-25).

**Reset reason.** The console `s` prints the last reset reason from `esp_reset_reason()`: POWERON, EXT
(reset pin), SW (software restart), PANIC (crash), TASK WATCHDOG, INTERRUPT WATCHDOG, OTHER WATCHDOG,
BROWNOUT, deep sleep, or unknown. The board is on native USB and does not reset when a terminal attaches,
so the boot banner is almost never seen; a reboot loop would be invisible unless the reason can be read
afterwards. The reason is not a portal field; the portal reaches it through its Console tab.

The reset reason also decides where the needle is believed to be at boot (`bootPosition()` in
src/s3/needle.cpp, chapter 5):

| Reset reason | Needle position at boot |
|---|---|
| SW, PANIC, INT_WDT, TASK_WDT, WDT (software-type) | Restored from a checksummed record in RTC memory, which survives these resets, if the record is whole and sane; otherwise the low stop. |
| POWERON, BROWNOUT, EXT, deep sleep, unknown | Assumed at the low stop. |

That the needle sits at the low stop after a power-up, and holds its place unpowered, are physical
values of this machine held by the firmware (§5.9).

**Console prefixes.** Everything the S3 says goes through `Con` with `[PASS]`, `[WARN]` and `[FAIL]`
prefixes where they apply. The A32's log lines arrive as `MSG_LOG` and print as "[A32] ...".

**Portal health fields in this area** (from `portalStateJson()`): `amp`; `link` (handshake stands);
`hello`; `astate` (a valid status frame is held; the page blanks audio readings when it is 0); `linkrx`,
`linktx`, `linkcrc` (rx at 0 points at wiring or power, rx climbing with crc climbing points at
signalling); `linkver` (frames dropped for carrying another protocol version, below); `rtc` (the battery
clock's health, below); `slot` (display worst slot error); `heap`; `up`; `clock`. Values from the A32 are
zeroed, or set to a sentinel where 0 is a real value, when no valid status frame is held: a missing key
would make the page read "-", and a stale one would make it read a lie. Needle, encoder and settings
fields belong to chapters 5 and 7.

**The battery clock's health** (2026-09-25). The S3 keeps one number, `gRtcBad`, published as
the state field `rtc`:

| `rtc` | Set when | Portal pill |
|---|---|---|
| 0 | Fine, or not asked yet. Set by every `MSG_TIME` that carries a valid, plausible time. | none |
| 1 | The DS3231 answered with no valid time: its oscillator stopped, or it was never set. Set by a `MSG_TIME` with valid = 0 or an implausible time. | "BATTERY CLOCK LOST ITS TIME - check its battery" |
| 2 | The DS3231 did not answer at all. Set when a 60-second `MSG_GET_TIME` gets no `MSG_TIME` within 5 s while the handshake stands (the A32 sends nothing when its own read of the chip fails). | "BATTERY CLOCK NOT ANSWERING" |

The value changes only on those events: a later valid time clears it to 0, and it is not cleared when
the link drops. The behaviour of the clock itself is unchanged by this; the pill only makes the fault
visible without a cable.

**Version skew.** Two kinds exist, and they look different.

- **Another protocol version.** The framer drops every frame whose version byte is not this build's
  `PROTO_VERSION` before anything else sees it, and counts it (`badVer`). The link then looks silent,
  exactly like a broken cable, so the count is shown (2026-09-25): the console `s` prints
  `link       : SILENT  rx N  crc N  wrong-version N`, followed by `<- the boards run different
  protocol versions: flash both` whenever the count is not zero; the state field `linkver` carries the count;
  and while it is not zero and the handshake does not stand, the portal shows the pill "BOARDS RUN
  DIFFERENT PROTOCOL VERSIONS - flash both".
- **Same protocol version, different status layout.** A status frame that passes its checksum but fails
  to parse (wrong length) means the two builds disagree on the frame. `onMessage()` then clears
  `haveState` and prints once: "the A32's STATE frame does not match this build. Flash BOTH MCUs."

**A short A32 restart.** An A32 restart shorter than the 2 s silence rule is detected by the A32's own
millisecond counter going backwards in its status frames (with a guard against that counter's own
49.7-day wrap). The S3 prints "[WARN] the A32 restarted - handshaking again." and re-handshakes
(2026-09-24).

### 4.2.15 Self-protection: watchdog, rollback, self-test

From v.1.0 (2026-09-25) the S3 has the same recovery treatment as the A32 (§9.2.5, §9.2.13): a
task watchdog that turns a hang into a reset, and a trial period that turns a reset of a new, unproven
image into a return to the previous one. The code is in src/s3/main.cpp, under the banner "ROLLBACK AND
THE WATCHDOG, THE A32'S TREATMENT", and in `hOtaUpload()` / `hOtaEnd()` in src/s3/portal.cpp.

**The task watchdog.** The ESP-IDF task watchdog resets the chip when a task it watches has not reported
in ("fed" it) for too long. The first lines of `setup()` set it to 15 s with panic on
(`esp_task_wdt_init(WDT_TIMEOUT_S, true)`), then call `enableLoopWDT()`. It watches two tasks:

- `loopTask`, that is `setup()` and then `loop()`. The pinned Arduino core feeds it once before every
  `loop()` pass, so any single pass that takes longer than 15 s resets the S3, and so does a `setup()`
  that takes longer than 15 s in total.
- the needle supervisor task `needle` (`needleTask()` in src/s3/needle.cpp). It subscribes itself
  (`esp_task_wdt_add(NULL)`) when it starts and feeds the watchdog at the top of every pass. Every branch
  of a pass sleeps 5 to 20 ms, so 15 s trips only on a real hang (chapter 5).

It does **not** watch the `portal` task (a flash write legitimately blocks it, and `otaWatchdog()` in
src/s3/portal.cpp already ends a stalled upload), the step emitter `nstep` (which busy-spins by design), or
IDLE0 (removed by `stepTask`, 4.2.4). IDLE1 is not checked in this build's configuration. `gLoopWdtOn`
records whether the subscription of `loop()` worked; the image line reports it as `watchdog on` or
`watchdog OFF`.

**Console prompts feed the watchdog.** Three console routines wait inside `loop()` for longer than 15 s,
and each calls `wdtFeed()` on every pass of its waiting loop: the WiFi prompt `y` (`setWifiInteractive()`,
60 s after the last key typed), the clock prompt `W` (`setClockInteractive()`, 40 s after the last key
typed) and the live index monitor `l` (`limitMonitor()`, 60 s). The `i` report's WiFi scan blocks
`loop()` for a few seconds without feeding it, which is well under 15 s. Any new routine that waits inside
`loop()` must do the same (4.8).

**After a watchdog reset** the reset reason is `ESP_RST_TASK_WDT`; the console `s` shows
`last reset : *** TASK WATCHDOG ***`. The needle memory treats it like a software restart: the needle
homes from its remembered position (4.2.14; §5.2.18).

**The trial period.** The bootloader of the pinned core is built with application rollback. By default
the core's `initArduino()` marks every new image valid before `setup()`, so an S3 build that crashed or
hung at boot used to loop on itself until someone used the USB cable. The S3 now defines
`verifyRollbackLater()` to return true, which tells the core "this firmware confirms itself". An image
written over the air therefore boots **on trial** (`ESP_OTA_IMG_PENDING_VERIFY`). If the S3 resets for
any reason before the image is confirmed (a crash, the task watchdog, the interrupt watchdog, a portal
reboot, a power cut), the bootloader marks the trial image invalid and starts the previous one.

At the top of `setup()` the S3 reads the state of the partition it is running from and keeps it for the
image line:

| State read at boot | Image line says | Meaning |
|---|---|---|
| `ESP_OTA_IMG_PENDING_VERIFY` | `ON TRIAL (rollback armed)` | A new image, sent over the air, not yet confirmed. |
| `ESP_OTA_IMG_VALID` | `valid` | A confirmed image. |
| `ESP_OTA_IMG_NEW` | `NEW - this bootloader has no rollback` | The bootloader did not arm the trial. |
| anything else, or no state | `not tracked (USB flash)` | An image written by USB. It is not on trial and is unaffected. |

After a confirmation the line says `valid (confirmed)`. `imageReport()` prints the line at the end of
`setup()` and in the console `s`: `image      : <state>, watchdog on|OFF`, followed by
` - an earlier update was ROLLED BACK` whenever the bootloader has marked an earlier image invalid
(`esp_ota_get_last_invalid_partition()`).

**What earns confirmation.** `confirmTick(now)` runs on every `loop()` pass, right after `Net::loop()`.
It does nothing unless the image is on trial and not yet confirmed, the link self-test passed, and the S3
has been up for at least 60 s (`CONFIRM_AFTER_MS`). After that it tries once every 10 s. Each try reads
the portal task's pass counter (`Portal::loops()`). The image is confirmed on a try that finds:

1. the portal task still turning: its counter has moved since the previous try (the first try only
   records it, so the earliest confirmation is about 70 s after boot);
2. the network up: `Net::connected()`, which is true when the S3 is joined to the house network or its
   rescue access point is up;
3. a passed link self-test (step 6 of 4.2.3).

It then calls `esp_ota_mark_app_valid_cancel_rollback()` and prints
`image confirmed (a minute of running, network and portal up).` If that call fails, it prints
`image confirm FAILED (err N) - retrying` and tries again 10 s later; until it succeeds, a reset still
rolls back and S3 uploads are still refused. Confirmation deliberately asks only for what the next update
needs: a way in. It never waits for the A32, so a broken or absent audio board cannot roll back a good
main board, and it never looks at the needle, because a homing fault is about the mechanism, not the
build.

**Nothing is saved while on trial** (v.1.0.1, 2026-09-25). `settingsWrite()` in src/s3/main.cpp
returns without writing while `s3ImageOnTrial()` is true. Changes made during the trial stay in RAM and
stay marked unsaved; once `confirmTick()` confirms the image, the ordinary 2 s debounce writes them on
the next `loop()` pass. The reason: a new image that changes the settings layout migrates the settings
at boot and would save the new layout within seconds. If it then rolled back, the previous image would
find a stored layout newer than it knows, lock itself onto its defaults and refuse every save (chapter
7). The cost is small and accepted: a change made in the trial minute is lost if the power goes in that
minute. The portal's Save answers "NOT saved yet - this firmware is on trial after an update; settings
are written once it is confirmed (about a minute)", and the console prints `save held - this firmware
is on trial; settings are written once it is confirmed.`

**A portal reboot during the trial is a rollback, and goes through at once** (v.1.0.1). An ordinary
portal reboot first saves the settings and refuses if the save fails (chapter 8). On trial there is
nothing to save, by the rule above, so `hReboot()` in src/s3/portal.cpp skips the save, answers
"rebooting - this firmware was still on trial, so the previous firmware comes back", and restarts. The
bootloader then marks the trial image invalid and starts the previous one. Refusing would take away the
one button that undoes a bad update.

**Updates refused while on trial.** `s3ImageOnTrial()` (declared in src/s3/settings_api.h) is true while
the image is on trial and not confirmed. A new S3 upload would be written into the other program slot,
which holds the very image a rollback returns to, so `hOtaUpload()` refuses it on the first chunk: it
quiets nothing (the needle keeps running, the clock stays lit), drops the bytes that keep arriving, and
prints `portal: S3 update refused - this image is still on trial.` `hOtaEnd()` then answers HTTP 409 with
`{"e":"this firmware is still on trial - retry in a minute"}`, and the portal page shows that message.
An A32 update is not affected by the S3's trial.

**A failed link self-test degrades instead of halting.** Before v.1.0 a failed `protoSelfTest()` halted
the S3 for ever, before the display, needle, WiFi or portal existed: a dark board reachable only by USB.
Now `setup()` sets `gProtoBroken`, prints `[FAIL] proto self-test - this image will NOT be confirmed;`
and `reboot to roll back, or upload a good build.`, and carries on exactly as usual: the display, the
needle, the link to the A32 (it is started as on any other boot), WiFi and the portal all come up. The
only difference is that `confirmTick()` never confirms this image. Recovery depends on how the image
arrived. One sent over the air stays on trial, so a portal reboot rolls it back, while S3 uploads are
refused. One written by USB is not on trial, cannot roll back, and accepts a good build through the
portal. Only a broken build can fail this test; the payload sizes are also checked at compile time
(`static_assert` in include/proto.h).

**Proven on the radio, 2026-09-25**, all over the air with the USB cable unplugged. At v.1.0 the S3
booted `ON TRIAL (rollback armed), watchdog on`; a second S3 upload during the trial got HTTP 409 with the
message above; the image was confirmed (seen by an uptime of 90 s). Then a deliberately hanging image
(`loop()` spinning for ever; built from uncommitted code, stamp `<hash>-dirty`, never committed) was
sent. The task watchdog reset the S3, and the bootloader returned to v.1.0: `image : valid, watchdog on
- an earlier update was ROLLED BACK`, `last reset : *** TASK WATCHDOG ***`, and the needle
`remembered at -1154 ... homing from there`, then found the index. It was the first real rollback on the
S3.

**Proven again, 2026-09-25, on v.1.0.1:** with the new image on trial, a portal reboot answered
"rebooting - this firmware was still on trial, so the previous firmware comes back"; the S3 came back on
v.1.0 with `- an earlier update was ROLLED BACK`, and nothing had been saved during the trial.
v.1.0.1 was then sent again and confirmed itself after a minute. v.1.0.2 and v.1.0.3
were each sent over the air, ran on trial, and confirmed.

## 4.3 Settings and constants

### 4.3.1 Stored settings in this area

These are fields of `Settings` in main.cpp, stored in NVS (chapter 7) and editable from the portal
through `gSet[]` in src/s3/settings_table.h. "Admin" means only the owner account can change it.

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `brightOn` | 255 | level | 0–255 | Clock brightness, amp on. 255 is the maximum. | Portal Display tab; console `-` `=` |
| `brightOff` | 110 | level | 0–255 | Clock brightness, amp off. | Portal Display tab |
| `hour12` | 0 | bool | 0–1 | 12-hour clock, no AM/PM indicator. | Portal Display tab (admin) |
| `blankLeadZero` | 1 | bool | 0–1 | Blank a leading zero in the hour. | Portal Display tab (admin) |
| `dispDwellMs` | 250 | ms | 0–5000 | Hold before the clock brightness moves. | Portal Display tab (admin) |
| `dispFadeMs` | 1500 | ms | 0–10000 | Eased travel between the two brightnesses. | Portal Display tab (admin) |
| `showTuning` | 0 | enum | 0 clock always, 1 tuning always, 2 tuning while tuning | What the digits show. | Portal Display tab (admin); console `f` |
| `tuneHoldMs` | 4000 | ms | 500–20000 | How long the mode-2 readout stays after the knob stops. | Portal Display tab (admin) |
| `dialLow` / `dialHigh` | 879 / 1079 | 0.1 MHz | – | The printed dial face; the readout clamps to it. | Chapter 6 |
| `panelTuning` | 255 | level | 0–255 | Panel while tuning on RADIO. | Portal Lights tab |
| `panelIdle` | 128 | level | 0–255 | Panel on RADIO, knob idle. | Portal Lights tab |
| `panelOther` | 96 | level | 0–255 | Panel on AUX or Bluetooth. | Portal Lights tab |
| `panelIdleMs` | 5000 | ms | 0–60000 | Idle time before the panel dims to `panelIdle`. | Portal Lights tab (admin) |
| `panelFadeMs` | 900 | ms | 0–5000 | Eased travel (0 is treated as 1). | Portal Lights tab (admin) |
| `panelDwellMs` | 250 | ms | 0–5000 | Hold before the panel moves. | Portal Lights tab (admin) |
| `sweepOn` | 0 | bool | 0–1 | Non-zero: a decorative end-to-end needle sweep each time the amp comes on. | Portal Needle tab (admin) |

The Bluetooth lamp brightness per state (`btLedOff` ... `btLedLink`, Lights tab) are A32 settings that the
S3 only mirrors and forwards; see §10.2.11.

The time zone and NTP server are network settings, stored separately (chapter 8).

### 4.3.2 Console keys in this area

| Key | Action |
|---|---|
| `8` | Show 8888 (all segments). |
| `b` | Blank the display. |
| `9` | Back to normal. |
| `-` / `=` (`+`) | `brightOn` −15 / +15, written straight to the display (no fade), saved. |
| `f` | Cycle `showTuning` 0 → 1 → 2, saved. |
| `W` | Set the clock by hand (local time, zone hard-coded, 4.2.13). Pushed to the DS3231 only while the A32 answers, and it says which. |
| `Z` | Zero the step-jitter and display slot-error counters. |
| `s` | Status page, including the link's `wrong-version` count (4.2.14), the last reset reason and the image line (4.2.15), ending with the command list. |
| `?` | Command list. |
| `l` | Live index monitor for up to 60 s; keeps the link alive and feeds the task watchdog (chapter 5). |

The full key list belongs to chapter 8.

### 4.3.3 Compile-time constants in this area

| Name | Value | Where | Meaning |
|---|---|---|---|
| `TICK_US` | 25 µs | display.cpp | ISR period. |
| `DEAD_TICKS` | 33 (825 µs) | display.cpp | Dark time at the start of each slot; measured on this machine (4.2.8). |
| `SLOT_TICKS` | 100 (2500 µs) | display.cpp | One digit's slot; four slots = 100 Hz. |
| `LIT_MAX` | 67; effective maximum 66 | display.cpp | Longest lit window; `setBrightness()` clamps to 66 so the turn-off lands inside the slot. |
| Display timer | hardware timer 0, prescaler 80 | display.cpp `begin()` | 1 µs ticks. |
| `FONT` | 0x3F 0x06 0x5B 0x4F 0x66 0x6D 0x7D 0x07 0x7F 0x6F | display.cpp | Digits 0–9, bit 0 = A ... bit 6 = G. |
| `PANEL_CH`, frequency, resolution | 1, 1000 Hz, 8 bit | panel.cpp | Panel PWM. |
| Amp debounce | 50 ms | `loop()` | – |
| Display refresh | 200 ms | `loop()` | `updateDisplay()` cadence. |
| `MSG_SET_SYS` period | 2000 ms | `loop()` | Matches the A32's 2 s silence rule. |
| HELLO / PING period | 1000 ms | `loop()` | – |
| `MSG_GET_TIME` period | 60000 ms | `loop()` | DS3231 resync. |
| Battery-clock answer wait | 5000 ms | `loop()` | No `MSG_TIME` this long after a 60 s `MSG_GET_TIME` sets `rtc` to 2 (4.2.14). |
| NTP write-through period | 3600000 ms | `loop()` | Hourly. |
| `NTP_FRESH_MS` | 4 h | net.cpp | NTP counts as fresh within this of the last SNTP sync. |
| Plausible epoch | 1600000000 | `onMessage()`, `setClockInteractive()` | Earlier times are refused. |
| USB wait at boot | 3000 + 300 ms | `setup()` | – |
| `W` / `y` / `l` timeouts | 40 s / 60 s / 60 s | main.cpp | Console prompts. |
| `WDT_TIMEOUT_S` | 15 s | main.cpp | Task watchdog timeout on `loop()` and the `needle` task, panic on (4.2.15). |
| `CONFIRM_AFTER_MS` | 60000 ms | main.cpp | No trial confirmation before this uptime (4.2.15). |
| Confirmation retry | 10000 ms | `confirmTick()` | One confirmation try every 10 s after that; earliest confirmation about 70 s. |

Framework settings this chapter relies on (ESP-IDF configuration of the pinned Arduino core, not set by
this project): `loop()` on core 1 with an 8192-byte stack; a 1000 Hz FreeRTOS tick; WiFi pinned to
core 0; interrupt watchdog 300 ms on both cores; brownout detector on; print-and-reboot on panic;
bootloader application rollback enabled; SNTP interval 3 h.

### 4.3.4 Peripherals the S3 firmware allocates

| Peripheral | User |
|---|---|
| Hardware timer 0 | Display ISR. |
| LEDC channel 0 | Unused on purpose (the A32 uses channel 0 for its Bluetooth lamp). Note: in this Arduino core channels 0 and 1 share LEDC timer 0; anything put on channel 0 at another frequency would change the panel's. |
| LEDC channel 1 (timer 0) | Panel lamps. |
| LEDC channels 2–5 (timers 1–2) | Stepper coils (chapter 5). |
| `Wire` (I2C 0) | Tuning encoder (chapter 5). |
| `Wire1` (I2C 1) | RDA5807M (chapter 6). |
| `Serial2` | Link to the A32 (chapter 9). |
| RTC memory (not initialised at reset) | Needle position record (chapter 5). |

### 4.3.5 Pins: what the firmware does with each (include/pins.h)

`include/pins.h` is the firmware's pin authority: it says which pin the code drives or reads. It is
**not** a record of what the pin is wired to; that is the Hardware Bible, §2 for the S3 and §3 for the
A32. Every pin below reaches the function it is named for, as the Bible records row by row (Bible §2 for
the S3, §3 for the A32); polarities and other hardware facts the code relies on cite their Bible
sections on their rows. A pin moves in pins.h and nowhere else.

**S3 defines**

| Define | Value | Used in | What the firmware does with it |
|---|---|---|---|
| `S3_AMP_SENSE` | 1 | s3/main.cpp | Plain `INPUT`. Read every pass; LOW = amp powered; 50 ms debounce. External pull-up (Bible §4, §7). |
| `S3_SEG_A` .. `S3_SEG_E` | 6, 7, 15, 16, 17 | s3/display.cpp | Outputs in the low output register. Driven HIGH during a digit's lit window when that segment is in the pattern; HIGH lights the segment (Bible §9). |
| `S3_SEG_F` | 8 | s3/display.cpp | As above, segment F. |
| `S3_SEG_G` | 18 | s3/display.cpp | As above, segment G. |
| `S3_DIGIT_1` | 42 | s3/display.cpp | Digit select, rightmost digit (units of minutes; Bible §8). HIGH = digit on (Bible §9). High output register. |
| `S3_DIGIT_2` | 2 | s3/display.cpp | Digit select, tens of minutes. Low output register. |
| `S3_DIGIT_3` | 40 | s3/display.cpp | Digit select, units of hours. High output register. |
| `S3_DIGIT_4` | 41 | s3/display.cpp | Digit select, leftmost digit (tens of hours). High output register. |
| `S3_STEPPER_I1` .. `I4` | 9, 10, 11, 12 | s3/drive.cpp | Stepper phase outputs on LEDC channels 2–5 at 20 kHz (chapter 5). |
| `S3_INDEX_HALL` | 5 | s3/needle.cpp | Plain `INPUT`; LOW = index magnet present; external pull-up (Bible §11, §24; chapter 5). |
| `S3_UART_TX` / `S3_UART_RX` | 13 / 14 | s3/main.cpp | `Serial2` to the A32 at 921600 baud, pins given explicitly (Bible §4; chapter 9). |
| `S3_PANEL_PWM` | 21 | s3/panel.cpp | LEDC channel 1, 1 kHz, 8-bit PWM; higher duty = brighter (Bible §11). |
| `S3_AS5600_SDA` / `SCL` | 38 / 39 | s3/needle.cpp | `Wire` I2C bus for the tuning encoder (chapter 5). |
| `S3_I2C_HZ` | 100000 | s3/needle.cpp, s3/rda.cpp | Clock of both S3 I2C buses. |
| `AS5600_ADDR` | 0x36 | s3/needle.cpp | Encoder I2C address. |
| `S3_RDA_SDA` / `SCL` | 47 / 48 | s3/rda.cpp; printed by console `R` | `Wire1` I2C bus for the RDA5807M (chapter 6). |
| `RDA5807_ADDR` | 0x11 | nothing | Unused; rda.cpp has its own constant. The fitted RDA5807M answers at 0x11 (Bible §24; chapter 6). |

GPIO 4 is not used by the firmware; pins.h calls it "the only spare". No S3 pin drives a fan, the colon,
the Bluetooth lamp or anything on the A32's side.

The RDA5807M has an I2C bus of its own on purpose: the encoder's bus is kept to the one sensor the
tuning path depends on, and the RDA5807M's address is fixed in its silicon, with no select pin.

**Pins the S3 firmware leaves alone**, as pins.h lists them: GPIO 35, 36 and 37 (taken by the octal
PSRAM of an R8 module, per the module's datasheet); GPIO 0, 3, 45 and 46 (strapping pins, read by the
chip at reset); GPIO 19 and 20 (native USB, the console); GPIO 43 and 44 (the chip's first UART,
kept free). A new function must take a pin outside these, and outside those already used.

**History of the S3 defines.** Segments F and G were swapped in pins.h on 2026-08-27 (F was 18, G was
8), and digit selects 1 and 2, and 3 and 4, were swapped the same day; in both cases the author drove
each pin and noted which bar or digit lit. The two end-switch inputs `S3_LIMIT_LEFT` and `S3_LIMIT_RIGHT` were replaced by the one index
switch on 2026-08-30 and their defines deleted on 2026-09-20; nothing in the firmware used
them by then, and `S3_LIMIT_LEFT` only aliased the index pin, which was a trap.

**A32 defines** (same file, used only by the A32 build; detailed in chapters 9 and 10)

| Define | Value | Used in | What the firmware does with it |
|---|---|---|---|
| `A32_PCM1802_MCLK` | 0 | a32/audio.cpp | I2S master clock out. Forced: the classic ESP32 can put the I2S master clock only on GPIO 0, 1 or 3, and 1 and 3 are its console. |
| `A32_PCM1802_DOUT` | 19 | a32/audio.cpp | I2S data in. |
| `A32_PCM5102_DIN` | 4 | a32/audio.cpp | I2S data out. |
| `A32_I2S_LRCK` / `A32_I2S_BCK` | 17 / 18 | a32/audio.cpp | Shared I2S frame and bit clocks. |
| `A32_RTC_SDA` / `A32_RTC_SCL` | 22 / 21 | a32/rtc.h | I2C bus for the DS3231. The Bible records no external pull-ups on it (Bible §5), so the bus runs on the ESP32's internal pull-ups, enabled by `Wire.begin()`, plus whatever the module carries; it answers at every boot. |
| `A32_I2C_HZ` | 100000 | a32/rtc.h | That bus's clock. |
| `DS3231_ADDR`, `AT24C32_ADDR` | 0x68, 0x57 | nothing | Unused; rtc.h has its own address constant. |
| `A32_UART_RX` / `A32_UART_TX` | 26 / 27 | a32/main.cpp | `Serial2` to the S3, pins given explicitly (the defaults would collide with `A32_I2S_LRCK`). |
| `A32_MODE_ADC` | 36 | a32/main.cpp | ADC read of the source selector. |
| `A32_VOLUME_POT` | 35 | a32/main.cpp | ADC read of the volume knob. |
| `A32_MODE_THRESH_AUX_BT` / `_BT_RADIO` | 424 / 2471 | a32/main.cpp; printed by the S3's `s` | Source thresholds: raw below 424 = AUX, 2471 and above = RADIO, between = BT (Bible §0, §11). The selector's centre position reads 0 V through 22 k, so it reads AUX (Bible §11, §22). |
| `A32_BT_BUTTON` | 25 | a32/main.cpp | `INPUT_PULLUP`; LOW = pressed (Bible §11). No capacitor is fitted; the firmware debounces it, counting a press on release after more than 30 ms held (chapter 10). |
| `A32_BT_LED` | 33 | a32/btled.cpp | LEDC PWM, 2 kHz, 8 bit, active LOW: the lamp's anode is on the A32's supply and the pin sinks it (Bible §11). The firmware drives it fully on for 300 ms of every 600 ms in LINK, at the top of every LOOK breath and in both FOUND flashes (chapter 10). |

The define `A32_PCM5102_XSMT` (GPIO 16) was deleted on 2026-09-23: it drove a pad that
nothing was connected to, and the A32's mute is in software only (chapter 10). pins.h also lists the
A32's pins to leave alone, GPIO 2, 5, 12 and 15 (strapping) and 6 to 11 (the flash chip); warns that
34 to 39 are inputs only, with no internal pulls (the selector and volume inputs use two of them);
names 13, 14, 23 and 32 as unused; and notes that GPIO 16 and 17 are free only on a module
without PSRAM, since a PSRAM module takes them and GPIO 17 is the I2S frame clock.

## 4.4 Decisions

**Two microcontrollers, with the S3 owning everything visible.**
Why: the two boards are hardware (Bible §1). The ESP32-S3 has Bluetooth Low Energy only, and Bluetooth
audio from a phone (A2DP) needs Bluetooth Classic, which only the classic ESP32 has; so the A32 carries
the audio. Given two boards, everything a person sees move or light up went to the S3, which also keeps
the priority-19 step emitter away from the A32's audio timing. In short: two microcontrollers, because
one could not do it. The A32 runs A2DP, which on the classic ESP32 means the legacy I2S driver, while
the S3 runs a step emitter that must not be starved; neither tolerates the other's timing.
Rejected: one chip for everything.
When and evidence: the architecture since the first firmware commit (2026-09-01).

**Core split: display interrupt, needle supervisor and `loop()` on core 1; WiFi, the web server and the
step emitter on core 0.**
Why: a timing test on 2026-08-28 answered the question by measurement. A bench build (Phase G of the
bring-up) ran the display interrupt exactly as it runs now, timestamped every digit change and counted
how far each strayed from 2500 µs. The author watched the display under each load: WiFi with four concurrent multi-megabyte HTTP streams,
"imperceptible"; the same plus a stepper moving back and forth, "imperceptible"; deliberate
flash-bus thrashing, the only load that visibly degraded it, and "still bearable". The author's
tolerance proved to be above the 1000 µs band the tool flagged, so that band is conservative. The
test also set three rules the firmware keeps: debounce settings writes; darken the machine during an
upload (4.2.12); never read or write flash in a loop. The step emitter was later
put on core 0 so it can never starve `loop()` or compete with the display interrupt.
Rejected: the emitter on core 1 (on the needle test rig, a high-priority spinner on the core running
`setup()` stopped the boot banner from printing).
When and evidence: 2026-08-28 measurement; needle.cpp comment above `stepTask()`.

**Portal requests may be starved while the needle moves.**
Why: the step emitter's timing wins over web responsiveness.
When and evidence: 2026-09-09, accepted as a known cost.

**The display ISR lives in IRAM and only writes registers.**
Why: flash contention is the one load that disturbed the display, and IRAM code does not wait for flash.
When and evidence: display.cpp header of `onTick()`.

**825 µs of dead time in firmware, instead of changing the display hardware.**
Why: without the dead time the display ghosts. The author judged the resulting brightness fine by eye.
Rejected: a hardware change to shorten the switch-off time; longer slots (4 ms per digit gives a 62.5 Hz
frame, marginal for flicker).
When and evidence: 2026-08-27, before the firmware repository existed. Worth revisiting only if the
display is later judged too dim.

**Brightness shortens the lit window, never the slot.**
Why: a constant frame rate means dimming cannot flicker.
When and evidence: `setBrightness()` in display.cpp.

**An unconditional blank at the end of every slot, plus a clamp of the lit window to 66 ticks.**
Why: on 2026-08-28, at brightness 255 the turn-off fell on tick 100, which never arrives because the slot
rolls over at 99. The segments stayed lit through the following digits and the display read 88:88. The
author had found a workaround (240 of 255) before the cause was known. The clamp fixes the arithmetic; the
unconditional blank means no future arithmetic can bring the bug back.
When and evidence: 2026-08-28; the comment in `onTick()`.

**Two clock brightnesses, amp on and amp off, both adjustable.**
Why: the author's specification of 2026-08-28, replacing a decision of 2026-08-20 that the clock never
dims. The colon is lit independently of the firmware and cannot dim with the digits (Bible §8), which is
why `brightOff` defaults to a modest 110.
When and evidence: 2026-08-28; `Settings` comment in main.cpp.

**The colon's brightness is not a firmware matter.**
When and evidence: 2026-09-09, closed as a non-issue.

**Eased transitions (hold, then smoothstep) for both clock and panel.**
Why: a linear ramp reads as a setting being changed; an instant step reads as a glitch. The hold makes
the machine seem to decide rather than react.
Rejected: linear ramps; instant steps.
When and evidence: panel.cpp and main.cpp easing comments. The intent, as panel.cpp records it: the
lamps change not rapidly and instantaneously, but like a capacitor emptying. For the clock, a comment in
main.cpp records the same intent: powering down should not snap the clock to its standby level; it
should let go like something with a reservoir behind it. The code has carried both since the first
commit (2026-09-01).

**Panel rules: dark with the amp off; full while tuning on RADIO; an idle level after 5 s; its own level
on AUX and Bluetooth; all adjustable.**
When and evidence: panel.h "OWNER'S RULES", there since the first commit (2026-09-01). A
panel-lamp fault pulse was decided against on 2026-09-09.

**The frequency readout: three modes; mode 2 only when the radio is live; mode 2 snaps to the channel
grid; mode 1 is neither gated nor snapped.**
Why: the readout should appear under the hand while tuning and go back to the clock (2026-08-31). It
should show a frequency only for a source that is playing (2026-09-03), but
the instrument mode must stay available with the amp off. Snapping from the unrounded value replaced an
"even tenth goes up" rule, which showed 91.5 for a true 91.36.
Rejected: gating mode 1; clearing the mode-2 hold whenever the radio is not live.
When and evidence: 2026-08-31, 2026-09-03; `updateDisplay()`. From 91.3, turning the knob up
should go to 91.5, not 91.4.

**Time lives on the A32; the S3 keeps a software clock it disciplines from the DS3231; NTP writes through
to the DS3231 hourly; the DS3231 holds UTC.**
Why: the real-time clock chip is on the A32's side and the display on the S3's. A software clock keeps the
display smooth whatever the link does. Writing NTP through makes a correction survive a power cut.
When and evidence: main.cpp header and `loop()`.

**A DS3231 whose oscillator stopped is not trusted, and the portal says so.**
Why: a stopped clock can hold a wrong but plausible date, which plausibility alone accepted. A stopped
oscillator most likely means the module's battery or the module itself is failing, so the fault is
shown on the portal as a pill, and so is a DS3231 that does not answer at all (4.2.14).
Rejected: plausibility only, with the oscillator flag merely reported.
When and evidence: 2026-09-24 (the trust rule); 2026-09-25 (the pills).

**`W` claims the push to the DS3231 only when the A32 answers.**
Why: it used to print "pushed to the DS3231" with the link down; the next handshake then read the
DS3231's old time back over the hand-set one, with nothing to say why.
When and evidence: 2026-09-25.

**NTP is fresh only within 4 h of an actual SNTP sync.**
Why: the old test ("the clock reads past 2020") stayed true for ever after one sync, so a free-running S3
clock was written into the DS3231 every hour as if it were NTP.
When and evidence: 2026-09-24.

**The `W` prompt's time zone stays hard-coded, with a note for builders.**
Why: a builder elsewhere personalises it for their own device; the code is not changed here.
When and evidence: 2026-09-25.

**Uptime from the 64-bit microsecond timer.**
Why: `millis()` rolled the uptime to zero every 49.7 days.
When and evidence: 2026-09-25.

**Homing once, at S3 boot; the front switch does not home.**
Why: the S3 boots only when the set is plugged in, so S3 boot is the first power-up. `sweepOn` became
purely decorative.
Rejected: homing and sweeping on every amp power-up.
Exception kept: a needle with no zero, including one in FAULT, starts homing on the amplifier-on edge
(chapter 5, D5).
When and evidence: 2026-08-31 (chapter 5 has the details). The intent: the first calibration happens
only at first power-up. Afterwards the firmware always knows where the needle is, since it is at the
lowest point at every Off, and every time the needle passes the index sensor it re-references
passively, so as not to lose steps.

**The needle remembers its position across software resets, not across a brownout.**
Why: after a software reset the needle is not at the low stop but wherever it was. A brownout ignoring
that memory is a non-issue: the homing search already goes only up to a certain point before it turns
the other way.
When and evidence: 2026-09-25 (chapter 5).

**The status page and command list are available on demand, not only at boot.**
Why: the board is on native USB and does not reset when a terminal attaches, so anything printed only at
boot is never seen. A separate boot-only help list had drifted from the dispatcher and was deleted.
When and evidence: `commandList()` and the note below it.

**The `y` prompt may hold `loop()` for up to 60 s.**
Why: realistically it does no meaningful harm.
When and evidence: 2026-09-24.

**The S3 gets a task watchdog, OTA rollback, a refusal to update while on trial, and a self-test that
degrades instead of halting.**
Why: before v.1.0 a hung task, an early-crashing update or a failed self-test on the S3 needed a USB
cable. The A32 had received the same treatment on 2026-09-25.
Rejected: watching the portal task (a flash write legitimately blocks it) and the step emitter (it
busy-spins by design); requiring the A32 or the needle for confirmation.
When and evidence: 2026-09-25; proven live the same day, including a real rollback after a
watchdog reset (4.2.15).

**Nothing is saved while an S3 image is on trial; a portal reboot during the trial goes through at once.**
Why: a rolled-back image must never find a settings layout newer than it knows. With nothing saved,
there is nothing for a reboot to wait for, and the reboot is the button that undoes a bad update.
Rejected: saving during the trial (the new layout could be written within seconds of boot).
When and evidence: v.1.0.1, 2026-09-25; proven live the same evening (4.2.15).

As with every decision in this Gospel, these were choices for this machine in this house; your mileage
may vary.

## 4.5 Failures and recovery

Classes follow the recovery ladder of §0.5 (BLOCKER, DEFECT, NOTE).

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| A32 link silent more than 2 s | `gLink.peerAlive()` false | Drops the handshake; source read as AUX (needle parks, panel to "other", mode-2 readout off); software clock runs on `millis()`; no DS3231 resync and no NTP write-through (both need the handshake); forgets its copy of the A32's settings, so portal edits of audio-board rows get HTTP 409 "the audio board is not answering - its settings cannot be changed now" and a settings download writes n/a for them (chapter 7); `W` sets this board only and says so; HELLO every 1 s; console "[WARN] A32 silent. Clock and needle keep running." | Automatic on the A32's HELLO_ACK, which also fetches its settings again. | NOTE |
| A32 restarted in under 2 s | Its millisecond counter goes backwards | Re-handshakes; re-reads version, time and settings. | Automatic. | NOTE |
| The boards run different protocol versions | Frames dropped by the framer, counted as `badVer` | Looks like a silent link; `s` shows `wrong-version N` and "flash both"; portal pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both". | Flash both boards. | NOTE (audio unaffected) |
| The boards run incompatible builds of the same protocol version | Status frames fail to parse | `haveState` cleared; one warning "Flash BOTH MCUs"; audio unaffected. | Flash both boards. | NOTE (telemetry only) |
| DS3231 invalid (oscillator stopped, or never set) and no NTP | `MSG_TIME` with valid = 0 | "[WARN] the DS3231 has no valid time yet." every minute; `rtc` = 1 and the pill "BATTERY CLOCK LOST ITS TIME - check its battery"; digits blank if the S3 never had a valid time. | NTP once joined (heals itself), or `W` by hand; either write clears the flag. Check the module's battery. | NOTE with a network; otherwise needs `W` |
| DS3231 does not answer | No `MSG_TIME` within 5 s of a 60 s `MSG_GET_TIME` (the A32 logs its own read failure) | S3 keeps its software clock; `rtc` = 2 and the pill "BATTERY CLOCK NOT ANSWERING". | Hardware (Bible §5). | NOTE |
| Software clock drift | – | Re-seated from the DS3231 every 60 s. | Automatic. | NOTE |
| Amp-sense bounce | Reading flips briefly | 50 ms debounce. | Automatic. | NOTE |
| Firmware upload in progress | `otaQuiet` | Needle stopped, display blank, panel dark, no settings saves, amp edges ignored. Upload ending without restart → needle resumed. | Automatic. | NOTE |
| Crash (panic) | – | ESP-IDF prints and reboots; needle position restored from RTC memory; `s` shows PANIC. | Automatic. | NOTE (DEFECT if it repeats) |
| Interrupt watchdog (300 ms) | – | Reboot; needle memory used. | Automatic. | NOTE |
| `loop()` or the `needle` task hangs | Task watchdog expires after 15 s | Reboot; needle memory used; `s` shows TASK WATCHDOG. If the image was on trial, the bootloader returns to the previous one. | Automatic. | NOTE |
| `portal` task or the WiFi stack hangs | Nothing watches it | The radio, clock and needle keep working; the portal is unreachable. | Power cycle (a USB console still works). | DEFECT: the ladder keeps BLOCKER for a radio that is unusable until a power cycle, and here only the portal is lost (never seen) |
| New S3 image crashes or hangs before it is confirmed | Reset while `ESP_OTA_IMG_PENDING_VERIFY` | Bootloader returns to the previous image; the image line adds `- an earlier update was ROLLED BACK`. | Automatic; upload a fixed image. | NOTE (proven live 2026-09-25) |
| New S3 image runs but its portal task stalls or no network comes up | `confirmTick()` never sees its conditions | Never confirmed; stays on trial; S3 uploads refused (HTTP 409). | Any reset returns the previous image; with the portal unreachable, a power cycle. | BLOCKER, but not USB-only (never exercised) |
| S3 upload sent while the image is on trial | `s3ImageOnTrial()` | Upload refused, nothing quieted, HTTP 409 "this firmware is still on trial - retry in a minute". | Wait for `image confirmed`, upload again. | NOTE |
| Settings changed while the image is on trial | `s3ImageOnTrial()` in `settingsWrite()` | Held in RAM, written once the image is confirmed; Save says why it is held. A power cut in the trial minute loses them. | Automatic at confirmation; after a power cut, set them again. | NOTE (proven live 2026-09-25) |
| Portal reboot while the image is on trial | `s3ImageOnTrial()` in `hReboot()` | Restarts at once without saving; the bootloader returns to the previous image. | By design: this is how a trial image is undone. | NOTE (proven live 2026-09-25) |
| New S3 image runs healthy for a minute but is wrong | Confirmed | Nothing. | Upload a good image through the portal. | DEFECT |
| Link self-test fails at boot | `protoSelfTest()` false | Reports it and boots as usual (display, needle, link, WiFi, portal); the image is never confirmed. | Sent over the air: a portal reboot rolls it back. Written by USB: upload a good image through the portal. | DEFECT (never exercised) |
| Brownout | Brownout detector | Reset; needle assumed at the low stop (a non-issue here). | Automatic. | NOTE |
| A console prompt holds `loop()` (`W` 40 s, `y` 60 s) | – | No amp sense, no PING: the A32 treats the silence as "amp down" after 2 s. The prompts only receive on the link. | Ends when the prompt ends. The prompts feed the task watchdog while they wait (`wdtFeed()`), so they never trip it (4.2.15). | NOTE (accepted for `y`) |
| Console `-` / `=` with the amp off | – | The clock shows `brightOn` until the next brightness retarget (4.7). | Toggle the amp or change any setting. | NOTE |
| `W` typed while the A32 is not answering | `peerHello` false | Sets the S3's clock only; says the battery clock was not updated. The next handshake may bring the DS3231's old time back. | Type `W` again once the link is back. | NOTE |

## 4.6 Graveyard

These were dead ends on this machine, in this house. They may not be dead ends for yours.

- **"The clock never dims"** (2026-08-20). Tried: one brightness, so the always-on colon matched the
  digits. Here: the author wanted a dimmer clock with the amp off. Replaced on 2026-08-28 by two levels.
- **Shortening the dead time with a hardware change.** Considered on 2026-08-27. Here: the firmware dead
  time gave a brightness the author judged fine, with no rework of a built board. Kept as the answer if the
  display is ever judged too dim.
- **Longer digit slots** to recover lit time. Here: 4 ms per digit drops the frame to 62.5 Hz, judged
  marginal for flicker. Not built.
- **Letting firmware dim the colon** by moving it to a free driver channel. Here: left on the table, then
  closed on 2026-09-09 as a non-issue.
- **"Brightness 240 is the maximum."** Here: this was a workaround for the tick-100 bug (4.4), not a
  limit. Removed with the fix.
- **Forcing the time zone on every redraw.** Here: it would have overridden the zone set in the portal.
  Now set once by the network module.
- **Uptime from `millis()`.** Here: rolled over every 49.7 days. Replaced by the 64-bit timer on 2026-09-25.
- **"NTP is fresh if the clock reads past 2020."** Here: true for ever after one sync; a free-running
  clock was written into the DS3231 hourly. Replaced by the SNTP sync callback on 2026-09-24.
- **Trusting the DS3231 on plausibility alone.** Here: a stopped clock kept a wrong but plausible date
  and was adopted. Replaced by requiring the oscillator-stopped flag clear on 2026-09-24.
- **A boot-only help list.** Here: nobody sees the boot banner on native USB, and the list drifted from
  the dispatcher. Deleted; one list, printed by `s` and `?`.
- **Homing, and sweeping, on every amp power-up.** Here: a search and a full sweep every time the front
  switch was touched. Replaced on 2026-08-31 by one homing at boot (chapter 5).
- **Assuming the needle is at the low stop after every reset.** Here: after a software restart with the
  radio playing above the index, the search ran into the high stop. Replaced by the needle memory
  (2026-09-25; chapter 5).
- **Clearing the mode-2 readout hold when the radio is not live.** Here: a 250 ms selector glitch or a
  2 s link hiccup wiped a readout being read. Removed.
- **Halting the S3 for ever on a failed self-test.** Here: a board that halts before WiFi can only be
  fixed by USB. Replaced by degrading (2026-09-25).
- **Letting the core confirm every new image before `setup()`.** Here: an S3 build that crashed or hung
  at boot looped on itself until the USB cable was used. Replaced by the S3's own trial and
  confirmation (2026-09-25).
- **Saving settings during the trial.** Here: a new image could write a new settings layout within
  seconds of boot, and a rollback would then leave the previous image locked out of its own settings.
  Replaced by holding every write until confirmation (v.1.0.1).
- **`W` claiming the push to the DS3231 whatever the link.** Here: with the A32 silent the claim was
  false, and the next handshake quietly brought the old time back. Replaced by checking the handshake
  and saying which (2026-09-25).
- **Looking silent on a protocol mismatch.** Here: a half-updated pair read "SILENT, rx 0", exactly like
  a broken cable. Replaced by counting and showing the dropped frames (2026-09-25).

## 4.7 Limits and firmware notes

- **~3.3 s boot delay without a USB host**, waiting for the console port (§12.7.1).
- **The clock is blank for a moment after every S3 boot**, until the A32's first `MSG_TIME` arrives, and
  stays blank if neither the DS3231 nor NTP has a valid time.
- **NTP cannot reach the display while the A32 link is down.** The write-through requires the handshake,
  and nothing else re-seats the software clock from NTP (§12.4.2).
- **The software clock is exact only between resyncs.** It counts `millis()` from the last DS3231 or NTP
  set. It is re-seated at a whole second, so it can lag the DS3231 by up to about a second; the display
  shows minutes, so this is not visible. `millis()` subtraction survives one wrap, so only an A32 silence
  longer than 49.7 days would break it.
- **`W` uses a hard-coded zone and leaves it in force.** After a `W` on a set configured for another zone,
  the display shows eastern North American time until the zone is saved again from the portal or the S3
  restarts. Left as a note only (4.4, §12.6.2).
- **A `W` typed while the A32 is silent sets this board only**, and says so (4.2.13). If the DS3231
  holds a valid time, the next handshake puts that time back on the display.
- **A settings change made in the minute an S3 image is on trial is lost if the power goes in that
  minute** (4.2.15). Accepted.
- **12-hour mode has no AM/PM indicator.** The font has digits only, so the display cannot show letters or
  error codes.
- **The readout below 100 MHz shows a leading zero** ("0879") (§12.7.3).
- **Mode 1 of the readout is not gated on the encoder answering**; with a dead encoder the Leditron
  shows a frequency from the last count (§12.7.2).
- **`-` / `=` bypass the fade** and write `brightOn` straight to the display, so the clock can sit at the
  wrong level or step back briefly before easing (§12.7.4).
- **Portal requests can be starved while the needle moves** (accepted, 4.4, §12.3.3).
- **The `y` prompt holds `loop()` for up to 60 s**, so the A32 goes quiet meanwhile, and it echoes what
  is typed, the WiFi passphrase included, into the console ring the portal reads (accepted,
  4.4; chapter 8).
- **With the link down, the panel treats the source as AUX**, so with the amp on it sits at `panelOther`.
- **The DS3231 temperature** arrives in every `MSG_TIME` and is not used (§12.4.14).
- **Exercised on the radio, 2026-09-25:** an S3 image hung on purpose after an upload was reset by the
  task watchdog and rolled back (4.2.15); a portal reboot on trial rolled back to the previous image,
  with nothing saved during the trial; the `rtc` and `linkver` fields read 0 on a healthy pair.
- **Never exercised on the radio:** a failed self-test at boot; a hang of the `needle` task; an image that
  runs but never earns confirmation; a failed confirmation write; a hung portal task (§12.6.6); the
  battery-clock pills in a real fault; the protocol-version pill (§12.4.17); the HTTP 409 on an
  audio-board edit while the A32 is silent (§12.3.16).
- **Rescue access point:** an open network since v.1.0.5, with no passphrase to change (chapter 8,
  §12.6.2).

## 4.8 Changing this area

**Invariants that must hold**

1. **The display ISR stays `IRAM_ATTR`, writes registers only, touches no flash and calls nothing but
   `esp_timer_get_time()`.** Keep `Display::begin()` in `setup()` so the timer interrupt is allocated on
   core 1.
2. **Never shorten `DEAD_TICKS`. Keep the turn-off strictly inside the slot. Keep the unconditional blank
   at tick 99. Only one digit select may ever be set.**
3. **All seven segment pins must stay below GPIO 32.** The ISR clears segments through the low output
   register only (`segAllMask`, built with `1UL << pin`). Digit pins may be anywhere; `begin()` splits
   them between the two registers.
4. **`loop()` must not block.** A routine that must wait inside `loop()` has to keep sending `MSG_PING`
   (as `limitMonitor()` does), or the A32 decides the amp is down. It must also return within 15 s or call
   `wdtFeed()` while it waits, as the `y`, `W` and `l` routines do, or the task watchdog resets the S3. `portalStateJson()` and anything the portal task calls must
   not block or write flash.
5. **Core 1 below priority 2 belongs to `loop()`**, which never yields. A new task at priority 0 on
   core 1 will never run. Put periodic work in `loop()`, or give the task priority 2 or more and make it
   yield. Tasks on core 0 must yield at least `vTaskDelay(1)`: the step emitter spins at 19.
6. **No NVS writes while the needle moves, a calibration runs or an upload is active, and none while
   the image is on trial.** Change `cfg`, then call `settingsTouch()` after the change; let
   `settingsFlush()` write (chapter 7). Never add a write path that bypasses `settingsWrite()`. The
   reason is measured: an NVS write disables the flash cache, which stalls every piece of code not in
   IRAM, the step emitter included. During the first settings migration, on 2026-08-31, the step jitter
   reached 383 µs and the display's worst slot error 1257 µs; with writes kept away from motion they
   fell to 49 µs and 8 µs. The display interrupt survives only because it lives in IRAM. A new stored
   field is appended, never inserted, with `SETTINGS_VERSION` bumped and a migration case added (§7.8.3).
7. **Push settings to modules only through `applySettings()`.** It also writes back what a module
   accepted (the needle's soft limits, for example), so `cfg` always holds what is running.
8. **LEDC:** channel 1 is the panel, on LEDC timer 0; channels 2–5 are the stepper. In the pinned
   Arduino core a channel's timer is `(channel / 2) % 4`, so channels 0 and 1 share timer 0. Do not use
   channel 0 on the S3 at another frequency: it would change the panel's. Hardware timer 0 belongs to
   the display.
9. **One definition of "the radio is playing"** (`radioLive()`) and **one definition of the frequency**
   (`Needle::tuneFreq10f()` / `tuneFreq10()`). Do not rebuild either locally; six copies of the mapping
   once existed.
10. **The DS3231 stores UTC.** Local time is applied only at display, with the `TZ` rule from the network
    module.
11. **A pin moves in pins.h and nowhere else**, and what it is wired to is the Hardware Bible's business.
12. **The toolchain is pinned** (`espressif32@7.0.1`). Identify the board before flashing it over USB;
    never let the uploader pick a port by itself when several ESP32-S3 boards are connected.

**Traps that have bitten this project**

- The turn-off on tick 100 that never happened (the 88:88 display).
- A busy-spinning task on a core whose idle task is watched: a boot loop that looked like a power
  problem (the step emitter, 2026-08-31).
- The native-USB console does not reset on attach, so anything printed only at boot is never seen.
- A serial tool that toggles DTR/RTS resets the board; "uptime 1 s on every reading" was the reading tool
  resetting it.
- A word typed in the portal's console used to run as a string of one-key commands. Now one key per send,
  unless a prompt wants a whole line (`Con.lineWanted`, chapter 8).
- `litTicks` starts at `LIT_MAX` (67) in display.cpp, one tick past the clamp, until the first
  `setBrightness()` at boot step 13. It is harmless only because the frame is blank then and tick 99
  blanks anyway. Do not show digits before a brightness has been set.

**How to test a change, without instruments**

- `8`, `b`, `9`: all segments, blank, normal.
- Brightness 255 must not show 88:88 (the tick-100 regression test).
- `Z`, then watch `slot` in the portal or `s` under load (WiFi traffic, a sweep with `r`, a settings
  save). After the settings-save guard the worst slot error measured 8 µs (2026-08-31); the 2026-08-28
  timing test treated anything under 100 µs as invisible. `Z` exists so a clean measurement can be
  taken after a one-off event such as a boot-time save. The bench tool behind that test is not
  published and no longer builds (§12.6.3); the `slot` figure is the instrument now.
- Toggle the amp: the clock should hold 250 ms, then ease for 1.5 s; the panel should follow its table.
- `f` to cycle readout modes; turn the knob with the amp on and the source on RADIO for mode 2.
- `W` to set the clock; check `s` "time" and the portal clock.
- `s` for everything at once: amp, source, needle, panel level and target, display brightness and slot
  error, time, link counters, last reset and uptime.
- After an S3 upload, the image line reads `ON TRIAL (rollback armed), watchdog on`; about 70 to 90 s
  after the restart the console prints `image confirmed (a minute of running, network and portal up).`
  A second S3 upload inside that time is refused with HTTP 409, and Save answers that settings are held.
  A change to the watchdog or the trial deserves the rollback tests of 4.2.15: a portal reboot during
  the trial, and a deliberately hanging image, never committed.

---

# 5. The dial needle

> **Specific to this build — adapt.** The needle, its stepper, gearing, index switch and travel are
> this cabinet's mechanism; yours will differ. The methods carry over; the numbers (§5.9) do not.

## 5.1 What it does

**How the needle and the tuning are tied together.** The needle follows the
firmware, and nothing else. Nothing mechanical joins the two (Bible §31.4): the
tuning knob turns the tube set's ganged tuning capacitor (Bible §13, §14), the needle
is moved by a stepper motor (Bible §11), and the firmware is the only link between
them. A magnetic angle sensor on the capacitor's shaft (an AS5600, Bible §11, §31.4)
is the live measure: read every 20 ms, it tells the main
MCU (the ESP32-S3, "the S3") that the tuning changed and where the shaft is, and every
position the needle takes while it follows the tuning comes from that angle through
the calibration curve. A small FM receiver chip (an RDA5807M) calibrates that curve:
it listens to the tube set's local oscillator, the firmware adds the intermediate
frequency (10.6 MHz, Bible §22) to get the station, and the pair (shaft angle,
station) is stored as a sample the curve is fitted through. The RDA5807M never stands
in for the AS5600: if the sensor stops answering, the needle holds its last target
until it answers again (§5.2.17). Chapter 6 covers the calibration in full (§6.1,
§6.2.4, §6.2.5), and §2.1.1 sums it up.

The S3 turns the shaft angle into a frequency and drives the stepper so that the
dial needle points at that frequency on the printed glass. What matters most is
repeatability. The same knob position must always put the
needle in the same place.

What the listener sees:

1. **The set is plugged in.** The S3 boots and the needle finds its reference point
   (it "homes"). Then it makes one "power-up flourish": a run to the low end of the
   dial, a run to the high end, then straight to where it belongs - the station if
   the amplifier is on and the source is RADIO, otherwise the low end. On
   2026-08-31, with the firmware of that day, boot to "homed and tracking" took about
   12 s.
2. **The front switch goes on with the source on RADIO.** The needle leaves the low
   end and swings to the station like a meter movement: a small overshoot and a
   short settle. If the `sweepOn` setting is on, a full end-to-end sweep comes first.
3. **Tuning.** The needle follows the knob continuously. It ignores changes smaller
   than two motor half-steps, so it does not chase sensor noise.
4. **The amplifier goes off, or the source is not RADIO.** After one second of
   stillness the needle falls to the low end with a fast start and a long, slowing
   finish, like a drive dying away.
5. **Two minutes after the amplifier goes off.** The needle quietly re-checks its
   reference point and returns to the low end, so the next listening session starts
   from a verified zero.
6. **Tuned past the printed scale.** The needle stays pinned at that end of the
   scale.
7. **During a firmware upload to the S3.** The needle stops where it is and the
   clock display goes dark. The radio keeps playing, because audio runs on the other
   MCU. After the restart the needle remembers where it was and homes from there.

The needle has no end-stop switches (Bible §31.4). One Hall-effect switch (the "index") sits
part-way along the travel and is the only position reference. Everything else is
kept by counting steps, and corrected every time the needle passes the index.
Three firmware mechanisms keep the needle off the mechanical stops in its place:
soft limits checked before every step, a step budget on every homing phase, and a
known starting belief for the homing search. On this dial the index sits near
102.5 MHz (§5.9), so an evening spent below that frequency never crosses it; that is
why the firmware also checks the index on its own when the set is switched off.

## 5.2 How it works

### 5.2.1 The chain, end to end

```
 tuning knob -> capacitor shaft -> AS5600 angle (0..4095 per turn)
                                        |
                           readAs5600(): multi-turn count "accAngle"
                                        |  low-pass filter (needle only)
                           tuning curve: count -> frequency (tenths of MHz)
                                        |
                           trackTarget(): frequency -> needle position
                                        |  (printed face: dialLow..dialHigh
                                        |   onto posMin..posMax)
                           needle task: plan a move (profile, microstep)
                                        |
                           step emitter: one step at a time, soft limits
                                        |
                           drive layer: position -> 4 PWM duties -> ULN2003
                                        |
                                  stepper -> needle
```

The tuning curve (count to frequency) belongs to chapter 6. This chapter
only consumes it through `tuneFreq10f()` and `tuneFreq10Raw()` in
`src/s3/needle.cpp`.

What is wired where - the stepper, its driver, the index switch and the AS5600 -
is in the Hardware Bible (Bible §11, and the S3 pin map in Bible §2). The pin
numbers the firmware drives are the `S3_STEPPER_I1..I4`, `S3_INDEX_HALL`,
`S3_AS5600_SDA`/`SCL` defines in `include/pins.h`.

### 5.2.2 Files, layers and tasks

| layer | file | job |
|---|---|---|
| policy | `src/s3/main.cpp` | when to home, track, park or sweep; the recovery and idle-check timers; limit capture; band-result collection; saving the shaft count; OTA pause and resume; console keys |
| needle | `src/s3/needle.cpp`, `needle.h` | two tasks: the supervisor and the step emitter; homing, tracking, parking, sweeps, index sensing, calibration, AS5600 reads, RTC memory |
| drive | `src/s3/drive.cpp`, `drive.h` | turns a position into four PWM duties. Nothing above this layer knows what driver chip is used |
| portal actions | `src/s3/settings_table.h` (`doAction()`) | the Needle tab's buttons |
| OTA hooks | `src/s3/portal.cpp` | stops the needle for an upload; resumes it if the upload does not end in a restart |

The needle runs on two FreeRTOS tasks, both created in `Needle::begin()`:

| task | name | core | priority | stack | rhythm |
|---|---|---|---|---|---|
| step emitter | `nstep` | 0 | 19 | 4096 | 1 kHz control tick (`CTRL_US` = 1000 us). Busy-waits between steps while moving; yields one tick when idle or when the next event is more than 2 ms away |
| supervisor | `needle` | 1 | 4 | 4096 | one pass every 5 ms (`vTaskDelay(5 ms)`); 20 ms while a bring-up tool holds the needle. Reads the AS5600 angle every 20 ms and its status every 25th read (every 500 ms) |

Two other tasks call into the needle: the Arduino `loop()` (core 1, priority 1),
which runs the policy and the console, and the web portal task (`portal`, core 0,
priority 3), which runs the Needle tab's buttons.

**Why the emitter is on core 0.** Core 1 carries the clock display's 25 us timer
interrupt and `loop()`, which services the link to the audio MCU. The emitter
busy-waits between steps, so on core 1 it would starve `loop()` for the length of
every sweep. On core 0 it shares time with WiFi and the portal instead, so the portal
can be slow while the needle moves. See `stepTask()` in `src/s3/needle.cpp`.

**Why core 0's idle task is removed from the task watchdog.** The emitter starves
core 0's idle task on purpose while it moves. With the idle task still watched, the
task watchdog reset the chip, in a boot loop that looked like a power problem.
`stepTask()` therefore calls `esp_task_wdt_delete()` on core 0's idle task. The cost
is that nothing catches a permanently hung task on core 0. If that ever matters, watch
the emitter task itself; do not put the idle task back. The bench rig had warned of
exactly this boot loop; when the emitter was moved to core 0, the removal was at first
left behind on the other core (2026-08-31).

**The supervisor is watched.** Since v.1.0 (2026-09-25) the task watchdog
watches the supervisor task, `needle`, with a 15 s timeout (§4.2.15). `needleTask()`
subscribes itself (`esp_task_wdt_add(NULL)`) when it starts and feeds the watchdog
(`esp_task_wdt_reset()`) at the top of every pass. Every branch of a pass sleeps 5 to
20 ms, so the watchdog trips only when the supervisor has truly stopped turning. A
stopped supervisor would leave the needle wherever it was and deaf to every request;
the watchdog restarts the S3 instead, and the needle memory carries its place across
the restart (5.2.18). The step emitter `nstep` is still not watched, because it
busy-spins by design; neither is the portal task.

### 5.2.3 Units and the coordinate frame

- **Half-step (hs).** The unit of every geometry number: soft limits, index band,
  budgets, drift. It is one half-step of the stepper, counted from the homed zero.
- **Fine unit.** 256 fine units = 1 half-step (`FINE_PER_HALFSTEP`), and 2048 fine =
  one electrical revolution of the motor, which is 8 half-steps (`FINE_PER_EREV`).
  The low 11 bits of a fine position are therefore the electrical angle, so the step
  path needs only a mask and table lookups, no division.
- **Speeds** are in half-steps per second (hsps) and accelerations in hsps² in the
  settings. Internally they are fine units per second (`hspsToFine()`).
- **Position `gPos` and target `gTarget`** are 32-bit signed fine positions. They are
  32-bit on purpose: the Xtensa CPU has no atomic 64-bit load, and these are read on
  one core while written on the other. The whole travel is about ±410 000 fine, so
  32 bits has room to spare. Arithmetic that multiplies a position casts to 64 bits
  where it happens (2026-09-24).
- **Zero** is the index switch's **forward ON edge**: the point where it switches on
  when the needle approaches it moving in the + direction.
- **+ is toward high FM.** On this machine, advancing the motor's electrical phase
  moves the needle toward low FM. That inversion lives only in `Drive::setInvert(true)`,
  called from `Needle::begin()`. It flips the electrical angle, not the position, so
  everything above the drive layer can say "position increases toward high FM" and
  mean it. The direction is a physical value of this machine, held by the firmware
  (§5.9).
- **Encoder counts.** The AS5600 gives 0..4095 per shaft turn. `accAngle` is a
  multi-turn running count. `calLow`/`calHigh` are that count at the tuner's two
  mechanical ends.
- **Frequency** is in tenths of a MHz, e.g. 1079 = 107.9 MHz.

The published position (`Needle::position()`, the portal's `pos`) is in **whole
half-steps**. A movement smaller than one half-step does not show in it.

### 5.2.4 The drive layer

`src/s3/drive.cpp` turns a fine position into four PWM duty cycles, one per input of
the Darlington driver.

- **PWM.** LEDC channels 2, 3, 4 and 5 at 20 kHz, above hearing. `Drive::begin()`
  asks for 11-bit resolution and steps down one bit at a time to 8 bits until
  `ledcSetup()` accepts. If nothing is accepted it prints
  `[FAIL] LEDC would not accept 20 kHz at ANY resolution`, and the four outputs do
  nothing. What was achieved is kept (`Drive::pwmHz()`, `Drive::bits()`) and shown by
  the console's `s`, which prints `*** LEDC REFUSED ***` when nothing was accepted.
  11 bits at 20 kHz only just fits the 80 MHz clock the PWM runs from. The negotiation
  exists because of a real failure on 2026-08-31: `ledcSetup()` refused 11 bits,
  returned 0, and the emitter stepped faithfully while the motor never moved. The
  lesson: check what a configuration call returns. Channel 1 is the S3's panel lighting and channel 0 the audio
  MCU's Bluetooth LED; the numbers are kept distinct across both firmwares.
- **Why PWM at all.** It is the only way to microstep a unipolar motor through a
  plain Darlington array. Microstepping means placing the rotor between the motor's
  natural step positions by giving the coils partial currents. Plain half-stepping
  chatters visibly at the low speeds a needle uses: its two-coils-on positions pull
  41 % harder than its one-coil-on positions. At high speed the advantage vanishes -
  the winding cannot follow the sine fast enough, so the current degenerates back
  into half-steps while the emitter pays many times the step rate. The bench rig therefore used plain
  half-steps when it wanted top speed; the needle never needs that speed.
- **The waveform.** A 2048-entry table holds max(sin, 0), scaled 0..4096. The four
  outputs are IN1 = max(cos, 0), IN2 = max(sin, 0), IN3 = max(−cos, 0),
  IN4 = max(−sin, 0). A unipolar half-winding conducts one way only, so a signed
  sinusoid becomes two rectified ones on opposite half-windings. At 45 degrees IN1 and
  IN2 each sit at 0.707: the current vector has constant magnitude, which is why
  microstepping is smoother than half-stepping. The wiring of the four inputs to the
  driver and the motor header is in the Bible (§11). Which inputs pair into one phase
  (IN1 with IN3, IN2 with IN4, in that turning order) is a physical value of this
  machine, held by the firmware (§5.9).
- **Modes.** `DRV_WAVE`, `DRV_FULL`, `DRV_HALF` and `DRV_MICRO`. Wave, full and half
  all read one 8-entry half-step table; the mode picks the step size and the lattice
  offset (full steps sit half a step over). In `DRV_MICRO` the divisor is rounded
  **down** to a power of two, at most 64, so every microstep is a whole number of
  fine units. `Drive::snap()` puts a position on the current mode's lattice.
- **`Drive::apply()`** writes a channel only when its duty changed. **`Drive::release()`**
  sets all four duties to 0, leaving the coils slack.
- **Duty.** `Drive::setDutyPct(100)` is called in `Needle::begin()`: full duty. The
  motor's supply is in the Bible (§11); that the motor is a 5 V part, so full duty on
  that supply is safe, is a physical value of this machine, held by the firmware
  (§5.9). The duty is kept as a parameter because it is the only thing that would
  stand between a 5 V motor and a higher supply.

### 5.2.5 The step emitter

`stepTask()` in `src/s3/needle.cpp` is the only code that emits a step in normal
operation. Each loop:

1. **Fold in any pending correction.** `c = atomic_exchange(gCorrectHs, 0)`, then
   `gPos += c × 256`. This is how the supervisor changes the position without
   writing `gPos` itself (see 5.2.20).
2. **Stand aside for bring-up tools.** If a bring-up tool holds the drive
   (`gTestMode`), sleep one tick and skip the rest.
3. **Every 1 ms control tick:** if a stop or abort request is pending, velocity = 0
   and the move is done. Otherwise, if a constant-velocity ("manual") speed is set,
   use it. Otherwise run the motion profile (5.2.6). When a move has been finished
   for 400 ms, release the coils: holding costs current and heat, and the needle has
   no load to hold. That the mechanism stays where it is with the coils unpowered is
   a physical value of this machine, held by the firmware (§5.9).
4. **While moving:** the step interval is one step unit divided by the speed, clamped
   to at least 20 us (a ceiling of 50 000 steps per second) and at most 1 s. The
   deadline is **pulled in every pass** as the profile accelerates. Without that
   line, a profile leaving rest slowly schedules its second step from its slowest
   velocity and waits: on the bench rig this gave one step, then a 25 s wait while
   reporting 2000 hsps.
5. **At the deadline:** if `stepAllowed(dir)` agrees, `gPos` moves one step unit,
   `Drive::apply()` writes the coils and the step counter `gSteps` goes up. If not,
   the needle is at a soft limit: the move stops there. The worst lateness is kept
   (`jitterMaxUs()`, the portal's `jit`). A late emitter never bursts to catch up; it
   re-synchronises.

**The count cannot drift.** Every step moves `gPos` by exactly one whole step unit.
There is no fractional accumulator anywhere in the step path, so a million steps out
and a million back land on the number they started from. Any difference between the
count and the needle is therefore the motor, never the arithmetic.

**Soft limits.** `stepAllowed()` refuses a + step at or above `posMax` and a − step at
or below `posMin`, but only while the limits are armed (`gLimitsArmed`). They are
disarmed while homing, because zero is not yet known.

### 5.2.6 Motion profiles

The motion was settled on a separate bench rig (5.4, D2) and moved into the
firmware. The rig's results were the settings themselves, not a standard the code
is held to: the values in 5.3 are the rule. `runProfile()` runs one of two laws, chosen per move by `gDownLeg`.

**Second-order (`gDownLeg` false).** A damped mass on a spring driven through an
actuator with limits - the equation of a moving-coil meter:

```
a = wn² × error − 2 × zeta × wn × v      then  |a| <= accel,  |v| <= vmax
```

`zeta` below 1 overshoots and settles; it is the overshoot knob. `wn` is the
ring-down knob. Measured on the rig, and not intuitive: raising `wn` **reduces**
overshoot, because with the speed clamped a stiffer system arrives with less
momentum. The law only brakes once the error is small, and it does not know the
motor cannot brake harder than `accel`. Braking must start before the stopping
distance, which gives a **gain ceiling**: `wn < 4 × zeta × accel / vmax`. At the
defaults (0.50, 18000, 1100) the ceiling is 32.7 and `wn` is 9. `Needle::begin()`
prints `[WARN] wn ... exceeds the gain ceiling ... - expect overshoot and hunting.` if
`wn` is over the ceiling. The limits used are `upVmax`/`upAccel`. Over the ceiling
the failure is deceptive: on the bench rig, `wn` 12 against a ceiling of 8.4 overshot
11 degrees and hunted, and it looked like "this profile does not move the motor",
because a bare motor asked to reverse at 2000 hsps only buzzes. Recompute the ceiling
whenever `upVmax` or `upAccel` changes.

**Decay (`gDownLeg` true).** A planned, feed-forward fall: the drive "dying away".
The speed follows `(1 − e^(−t/ta)) × e^(−t/td)`, with rise time `ta` = `riseMs` and fall
time `td` = `fallMs`, scaled by `planDecay()` so that its integral is exactly the
distance. If the peak speed would exceed `dnVmax`, `td` is stretched rather than the
peak clipped, because clipping would leave the needle short. A small position-error
term (gain 20 per second, capped at 30 % of `dnVmax`) keeps the needle on the plan.
The decay law does not use an acceleration limit; `dnAccel` enters only the arrival
test below.

**Dwell.** A pause before a commanded move starts: `dwellUpMs` (100 ms) before a
second-order move, `dwellDnMs` (1000 ms) before a decay move. The pause is most of
why a mechanism reads as mechanical. **No dwell while TRACKING**: tracking issues a
new move for every new target, and the pause made the needle seem to refuse to move
until the knob had gone some distance. The author read it as a threshold; it was a
timer, restarted by every new target (2026-09-02).

**Arrival.** A move is finished when the needle is within **one step unit** of the
target **and** its speed is below `max(1.5 × √(2 × accel × unit), 20 hsps)`. Both
rules came from lost sessions on the rig: a fixed 20 hsps threshold gave a
one-microstep limit cycle (a 2 s move took 21.4 s to settle), and a half-unit
tolerance can never be met once the position is off the lattice.

**Which profile runs when** (`needleTask()`, `homingTick()`):

| move | law | microstep | dwell |
|---|---|---|---|
| homing: search, backoff hops, re-approach hops | second-order | unchanged (whatever was last set) | 100 ms before each hop |
| sweep legs (both directions) | second-order | `microFast` (1/16) | 100 ms |
| tracking (both directions) | second-order | `microSlow` (1/32) | none |
| park at `posMin` | decay | `microFast` (1/16) | 1000 ms |
| band calibration creep | constant speed `reapHsps` | `microFast` | none |
| jogs (bring-up tools) | constant speed, emitter suspended | half-step or `microFast` | none |

So "up" and "down" in the setting names are names of the two laws, not directions.
The second-order limits (`upVmax` 1100) govern every profiled move except the park.
The decay limits govern only the park.

**Microstep changes only at rest.** `useMicro()` returns without doing anything if a
move is running. Each division has its own lattice; changing it mid-move leaves a
target that whole steps can never reach, and the needle jiggles forever. The rig
measured that 1/32 at 1700 hsps needs 54 400 steps per second, which the 20 us floor
clamps to about 92 % of the command; that is why moves use 1/16 and slow motion 1/32.

### 5.2.7 The supervisor, one pass

`needleTask()` does this every 5 ms:

1. `esp_task_wdt_reset()` - feed the task watchdog (5.2.2), then `serviceRequests()` -
   carry out a posted stop or calibration abort (5.2.20).
2. `huntTick()` - the hunting detector (5.2.21).
3. `sampleIndex()` - the one read of the index switch for motion purposes (5.2.8).
4. The band check (5.2.13).
5. `memRecord()` - update the needle's memory in RTC RAM (5.2.18).
6. Every 20 ms, `readAs5600()`; every 25th time, `readStatus()`.
7. If a bring-up tool holds the needle: sleep 20 ms and start again.
8. If a band calibration is running: `calTick()`, sleep 5 ms, start again.
9. By state: homing states go to `homingTick()`. SWEEP runs the sweep sequence, or
   abandons it on a slip. TRACKING plans a new move if the target differs from the
   position by 2 hs or more. PARKED re-parks if the position is 2 hs or more from
   `posMin`.
10. The edge log (console `e`) and the crossing re-reference (5.2.12).
11. Sleep 5 ms.

### 5.2.8 Reading the index: debounce and back-dating

The index switch is read in exactly one place, `sampleIndex()`, once per supervisor
pass and before anything branches on state. Homing, the crossing check and the band
calibration all read the result - the debounced level `idxState()` and a one-shot
edge flag `idxEdgeIsNew` with its snapshot `idxEdge` - and never the pin. The pin is
read as active LOW (`indexRaw()`: LOW = ON): the switch output is LOW while the magnet
is over it and pulled up otherwise (Bible §11, §24, §31.4).

**Debounce: 4 agreeing polls (`IDX_DEBOUNCE_K`).** A new level is believed only when
four consecutive 5 ms polls agree. The number is derived, not picked:

- To reject glitches of 10 ms or more: `(K − 1) × 5 ms > 10 ms`, so K ≥ 4.
- To still see a fast crossing: the narrowest ON region (reverse ON to reverse OFF,
  94 hs) crossed at the fastest speed near it (1700 hsps, the park) lasts 55.3 ms,
  about 11 polls. Seeing both edges needs 2K polls, so K ≤ 5.

K = 4 leaves about three polls of margin. The 94 hs width is a physical value of this
machine, held by the firmware (§5.9).

**Back-dated, not delayed.** When the raw level first disagrees with the confirmed
one, `sampleIndex()` records the position, the speed and the time since the previous
poll at that moment. If the run of agreeing polls reaches K, the edge is reported at
**that first snapshot**, not at the poll that confirmed it. The debounce therefore
costs three polls of delay before the edge is known, but adds no error to where the
edge is recorded. What remains is one poll interval of sampling uncertainty: the
edge happened somewhere between the previous poll and this one. The recorded
interval is clamped to 20 ms, so a scheduling hiccup is not taken for a large lag.

`Needle::begin()` seeds the debounce with a live read, so a magnet already over the
switch at power-up reads ON from the first poll, with no false edge. Homing can start
within 15 ms of the supervisor starting, and must see the truth from its first look
(2026-09-23).

### 5.2.9 Homing

**States** (`Needle::State`): IDLE, SEEK_INDEX, BACKOFF, REAPPROACH, SWEEP, TRACKING,
PARKED, FAULT.

```
  boot, H, portal Home, amp-on while unhomed, track()/park() from IDLE unhomed
          |
     startHoming()   (refused while a calibration or bring-up tool holds the needle)
          |   already ON the switch? --> record a sighting, go to BACKOFF
          v
   SEEK_INDEX  one second-order move of up to 2336 hs in the chosen direction
          |  switch ON        -> record a sighting -> BACKOFF
          |  budget spent     -> [WARN], reverse once; spent again -> FAULT
          v
   BACKOFF     while ON: hop -20 hs; once OFF: one -60 hs clearance move
          |  more than 400 hs from the phase start -> FAULT
          v
   REAPPROACH  +10 hs hops until a NEW confirmed ON edge
          |  hand the new zero to the emitter (gCorrectHs), wait for the fold,
          |  then: homed, limits armed, clue forgotten
          |  more than 300 hs from the phase start -> FAULT
          v
   automatic re-index?  -> gAfter (no sweep)
   any other home       -> SWEEP: posMin, then posMax (repeat if asked) -> gAfter

   gAfter = TRACKING or PARKED, set by track()/park(); PARKED at boot
   stop() -> IDLE from anywhere;  a jog or coil test -> IDLE
   FAULT: nothing driven, coils released, reason in faultReason()
```

**Direction.** `chooseHomeDir()` searches toward the index clue if one is on record
(5.2.14). Otherwise it goes by the sign of the position: positive searches down,
zero or negative searches up. At a cold boot `assumeAtLowStop()` has set the position
to `posMin`, which is negative, so the first search goes up (5.2.18).

**Budgets.** Every phase has its own step budget, because the soft limits are
disarmed while zero is unknown and nothing physical stops the needle:

| phase | budget | derived from |
|---|---|---|
| search (per direction) | 2336 hs (`HOME_BUDGET_HS`) | the measured travel, 1869 hs, plus 25 % |
| backoff | 400 hs (`HOME_BACKOFF_BUDGET_HS`) | about twice the worst case (band edge plus 60 clearance plus one 20 hop) |
| re-approach | 300 hs (`HOME_REAPPROACH_BUDGET_HS`) | about 2.4 × the ~127 hs needed (60 clearance plus the ~67-79 hs direction offset) |

The travel (1869 hs, with the index 1195 hs above the low stop and 674 hs below the
high stop) and the offset between the ON regions seen moving + and moving − (about
79 hs on the last calibration) are physical values of this machine, held by the
firmware (§5.9).

In the normal case the search budget is never spent: SEEK_INDEX stops the moment the
switch confirms ON. From the low stop the first pass travels about 1195 hs. When the
first direction's budget is spent, the console prints `[WARN] index not found in the
expected direction - the needle was not where the frame believed. Trying the other
way.` and the search reverses once (the reverse is remembered in `gTriedOther`). That
line was printed by every portal reboot made with the needle above the index until
the needle memory of 5.2.18 existed: each of those searches had run up toward the
high stop first.

**Backoff.** While the switch is ON, BACKOFF moves −20 hs at a time. Once it reads
OFF, it moves a further −60 hs of clearance. The final approach therefore always
comes from below.

**Re-approach, and what `reapHsps` really drives.** REAPPROACH moves **+10 hs at a
time, each hop an ordinary second-order move** under the `upVmax`/`upAccel` limits,
with the 100 ms up-dwell before each hop, until a newly confirmed ON edge. The speed
setting `reapHsps` is **not** used here, despite its name and its portal label,
"Measuring pass speed". It drives exactly two things: the speed of the band
calibration's four creeps (5.2.15), which is the measuring pass the label means, and
the speed of the portal's jog buttons (5.2.23). A 10 hs hop never approaches
`upVmax`; the second-order law itself keeps it slow. Not known: the effective speed
of the re-approach. The code sets no figure for it (it follows from the profile of
5.2.6, the 10 hs hop and the 100 ms dwell), and it has not been measured on the
radio. The label is left as it is.

**Zero.** On the new ON edge, `homingTick()` computes the shift that makes the
back-dated edge position read 0 (`pOnFwd`, which is always 0) and hands it to the
emitter through `gCorrectHs`. It then sets `gZeroPending` and **waits** until the
emitter has folded the shift in (`gCorrectHs` reads 0 again) before it marks the
needle homed, arms the soft limits, forgets the index clue, records
`gMeasAtHome = gMeas`, clears the fault reason and plans the sweep. Before this wait
existed, the crossing check scored the zero edge against the old frame ("-1226
half-steps ... Something slipped") and the sweep had the frame jump 1226 hs under it
mid-move (2026-09-23).

**FAULT.** `homeFault()` handles a spent budget. For a hand or boot home it goes
straight to FAULT: nothing is driven, the coils are released, and the reason
("index never found in either direction", "sensor never released during backoff",
"sensor never returned during the re-approach") is kept in `faultReason()` for
the portal (`fault` in the state JSON, the NEEDLE FAULT pill). For an automatic
re-index it retries instead (5.2.14).

**Leaving FAULT.** FAULT drives nothing on its own. A person gets the needle out of
it: the portal's Home or Re-index, the console `H` (or `x` then `T`), or switching
the amplifier on. Each of these starts a fresh home, because a needle in FAULT has no
zero (5.2.10, 5.2.14). `T` and `P` alone refuse in FAULT and say so.

**Refusals.** `startHoming()` returns `nullptr` if homing started, otherwise a reason
string, which it also prints: it refuses while a band calibration or a bring-up tool
(held coil, jog) holds the needle. Starting a home clears the old fault reason, the
drift evidence (`gDrift`, `gDriftPending`), the homed flag and a repeating sweep. It
keeps the index clue. It does not check whether a home is already running: a second
`H`, portal Home or amp-on edge during a home restarts homing from where the needle
is, with a fresh budget.

### 5.2.10 The sweep, and where the needle goes afterwards

After any home that is not an automatic re-index, the needle enters SWEEP:
`useMicro(false)` (1/16), a second-order move to `posMin`, then to `posMax`, then
`st = gAfter`. With the repeating sweep (`sweepRange(true)`, console `r`, portal
"Sweep repeatedly") it goes back to `posMin` and continues until stopped.

`gAfter` is where the needle goes when the busy phase ends. `track()` and `park()`
set it. If homing, a sweep or a calibration is running, they only set `gAfter` and
leave the state alone; if the needle is homed they switch at once; if it is IDLE and
unhomed they start a home; otherwise (FAULT, or unhomed and not IDLE) they return
false. The portal and the console say so when they return false.

`applyNeedleMode()` in `src/s3/main.cpp` calls `track()` when the radio is live (amp
on AND source RADIO, `radioLive()`) and `park()` otherwise. That gate is a listening
policy, not a statement about the tube set, which runs whenever the amplifier does:
the needle follows the dial while the radio is what you are listening to (§4.2.7). It does nothing while an
S3 upload runs or while the needle is unhomed. `loop()` calls it when the source
changes, when the amplifier changes, when the homed flag changes, after a band
calibration and after an upload that did not end in a restart.

On an amplifier-on edge `loop()` also calls `startHoming()` if the needle is not
homed, or `sweepRange(false)` if `sweepOn` is set. **The boot home always ends in a
sweep; `sweepOn` adds one on each amplifier-on edge.** "Not homed" includes FAULT, so
every amplifier-on edge sends a faulted needle homing again (up to 2336 hs each way).
This is kept on purpose (5.4, D5).

### 5.2.11 Tracking and parking

**Where to point.** `trackTarget()` maps frequency to position, linearly, from
`dialLow..dialHigh` (the frequencies **printed** at the two needle stops) onto
`posMin..posMax`, clamped at both ends. It uses the **unrounded** frequency. Three
mappings are kept separate on purpose: count to per-mille of the capacitor's travel
(`calLow`/`calHigh`), count to frequency (the tuning curve), frequency to needle (the
printed face).

Worked example, with the soft limits and dial values recorded on this machine in
September 2026 (`posMin` −1156, `posMax` +414, `dialLow` 87.9, `dialHigh` 107.9 MHz -
calibration data, not firmware defaults): the face spans 1570 hs over 200 tenths,
7.85 hs per 0.1 MHz. At 100.0 MHz the target is −1156 + (1000 − 879) × 1570 / 200 =
−1156 + 950 = −206.

**A filtered shaft.** The needle follows `gTrackAcc`, a low-pass of the shaft count:
each 20 ms sample moves it 35 % of the way (`TRACK_ACC_ALPHA` = 0.35). A real turn
arrives in about 100 ms; a one-sample spike is cut to about a third. Everything else
(the tuning-activity detector, the tuning-sample instrument, every readout) uses the
raw count (2026-09-24).

**Deadband.** TRACKING plans a move only when the target differs from the position
by 2 hs or more (`TRACK_DEADBAND_HS`). A move is a whole profile (accelerate,
decelerate, settle); one per half-step of encoder noise is audible. PARKED uses the
same 2 hs deadband against `posMin`, because a park that settles one half-step short
would otherwise be re-commanded forever.

**Tracking moves** are second-order at 1/32 with no dwell. **Parking** is a decay
move to `posMin` at 1/16 after a one-second dwell. `posMin` is both the low soft
limit and the park position.

### 5.2.12 Passive re-referencing at every crossing

Every confirmed ON edge seen while homed is a free measurement of where the needle
really is. In `needleTask()`:

1. **Subtract the sampling lag.** The edge happened somewhere in the poll interval
   before the run's first sample, so on average half an interval earlier:
   `lag = speed × interval / 2`. At 1100 hsps and a 5 ms interval that is about 3 hs.
2. **Score only a directional edge.** The switch turns on at different places
   moving up and moving down (its hysteresis). An edge moving + (speed above
   1 fine/s) is compared with `pOnFwd` (0). An edge moving − is compared with `pOnRev`,
   but only once the band calibration has measured it. An edge at rest, or moving −
   with no band measured, is counted (`gCross`) and nothing is concluded.
3. **Drift = where it was seen − where it should be.** `gMeas` counts scored edges.
4. **|drift| ≤ 40 hs (`CORRECT_MAX_HS`): absorbed.** `gCorrectHs = −drift`, folded by
   the emitter. The slip flag is cleared and the index clue retired: the frame has
   just been shown right.
5. **|drift| > 40 hs: not believed.** The number never enters the frame. It becomes a
   clue to where the index is (`believeIndexAt(expected + drift)`), `gDriftPending`
   is raised, and the console prints a warning. Recovery follows (5.2.14).

Nothing is corrected while a calibration runs, because the calibration is measuring
these same edges. Crossings during a jog are not scored either: a jog runs with the
emitter suspended and the supervisor skips everything after its bring-up branch.

40 is just under half the ~79 hs gap between the forward and reverse ON edges - the
one systematic error that could masquerade as drift - so a crossing read in the
wrong direction can never be absorbed. The 79 hs gap is a physical value of this
machine, held by the firmware (§5.9). The ladder built on it is 5.4 D8.

### 5.2.13 The band check

The crossing check scores ON edges only. It cannot see steps lost **after** a correct
ON edge while the magnet is still over the switch - which is what happened on
2026-09-24 (5.4 D3). So every pass, while homed, band-calibrated, not calibrating,
not in a bring-up tool and not homing: if the debounced switch reads ON while the
position is outside the measured band `[min(offRev, onFwd) − m, max(offFwd, onRev) + m]`,
the frame is wrong. The margin is `m = 30 hs + 40 ms of travel at the current speed`,
because the debounced level lags the needle. A fixed 30 hs was tried first: going up
at 1100 hsps it fired at 131, 34 hs past the OFF edge at 97 - pure lag, and it
re-indexed the needle at every boot until the speed term was added (2026-09-24).
It is raised once per excursion: the
clue is set to the current position, `gDriftPending` is raised, a warning is printed,
and recovery follows. It does not run before the band has been calibrated: with all
four band values at 0, every ordinary crossing would read as a slip and re-index in
a loop with no exit (2026-09-24).

Example: with the band recorded on this machine on 2026-09-03 (onFwd 0, offFwd 97,
onRev 79, offRev −15) and the needle at rest, the switch reading ON anywhere outside
−45..127 raises it. At 1700 hsps the margin grows to 98 hs.

### 5.2.14 Recovery: the re-index ladder, the idle check and the index clue

**The ladder** (2026-09-08):

| drift at a scored crossing | what happens |
|---|---|
| ≤ 40 hs | absorbed silently |
| > 40 hs, or the band check fires | slip flag raised; once the knob has been still for 3 s, a re-index |
| a re-index fails | the soft limits are re-armed, the old frame is kept, and it tries again - up to 3 attempts |
| the third attempt fails | FAULT, with "three re-index attempts failed. Something is mechanically wrong" |

`needleRecoverTick()` in `src/s3/main.cpp` starts the re-index: a slip is outstanding,
no upload is running, no recovery is running, fewer than 3 attempts have failed, the
knob has been still (`tuningActive()` false) for 3 s (`REIDX_STILL_MS`), and no
calibration runs. It does not require the amplifier to be on.

`startReindex()` refuses during a calibration, a bring-up tool, homing or the sweep.
Its body, `beginReindex()`, is an ordinary home with `gAutoHome` set: it ends in
`gAfter` with no sweep, and a failure is counted instead of faulting. On a failed
attempt `homeFault()` re-arms the limits and restores the homed flag and the slip
flag, so the trigger comes round again. A wrong frame is then still bounded: the
limits are the right distance apart, only in the wrong place.

**No zero, no re-index** (v.1.0.1, 2026-09-25). A re-index is a recovery of a
frame that exists. When the needle has no zero - it is in FAULT, or it was stopped
before a home finished - `startReindex()` does not call `beginReindex()`. It prints
`needle: no zero to re-index from - homing instead.` and runs a plain home through
`startHoming()`. That home ends in the sweep like any other, and if it fails it goes
straight back to FAULT. In practice only the portal's Re-index button reaches this
branch: the automatic triggers all require a homed needle. The portal still answers
"re-indexing".

Before this fix, the Re-index button on a needle in FAULT from a failed hand or boot
home ran a real re-index. If that failed too, `homeFault()` treated it as the first
failed automatic attempt and handed back "the frame we had": it marked a frame that
had never been measured as homed, armed the soft limits in it and raised the slip
flag. The needle then tracked or parked in that unverified frame, limits and all,
until the automatic retries ran out and FAULT came back.

**A slip during the sweep** does not wait. The SWEEP branch sees `gDriftPending` and
calls `sweepSlipped()`, which abandons the sweep and calls `beginReindex()` at once
(2026-09-24).

**The routine index check.** `idleIndexTick()` in `src/s3/main.cpp`: once the
amplifier has been known to be off for 2 minutes (`IDLE_CHECK_MS`), once per off
period, with the needle homed and no recovery, calibration, tuning measurement or
upload running, it calls `startReindex(true)`. The off period's check counts as done
only if the re-index actually started. A park that starts below the index never
crosses it, so without this an open-loop needle could not learn of lost steps between
listening sessions (2026-09-23).

**The index clue.** `gIndexBelieved` holds where the index was last seen, in the
frame the position is counting in, and `gIdxEvid` says whether that holds a real
observation. `believeIndexAt()` records a sighting; `forgetIndexClue()` clears it.
It is **replaced** by any newer sighting: a slip at a crossing, the band check, or the
switch found ON by a homing search. It is **cleared** only when the frame is made or
shown true: REAPPROACH declaring zero, the band calibration shifting the frame, or
an absorbed crossing. Nothing that merely starts or stops a home touches it. So a
re-index cut short by a stop, an upload or a restart still leaves the next home
pointed at the index (2026-09-25).

### 5.2.15 The band calibration

The four edges of the index's ON region - on and off, moving up and moving down -
are measured by `calStartBand()` / `calTick()` / `calCreep()` / `calFinishBand()`.
It is a state machine advanced once per supervisor pass, not a blocking routine,
because it is a portal button and a blocking call would freeze the portal.

**Preconditions.** Homed, no slip outstanding, no homing or sweep running, and the
needle sitting ON the switch. Otherwise it refuses and says why.

**One pass** is four constant-speed creeps at `reapHsps` (60 hsps), each with a 600 hs
budget:

| step | direction | until | records |
|---|---|---|---|
| 0 | + | OFF | `offFwd` |
| 1 | − | ON | `onRev` |
| 2 | − | OFF | `offRev` |
| 3 | + | ON | `onFwd` |

**Why constant speed, not a profile.** Profiles are for going somewhere known. A
creep moves until the switch says stop and cannot know the distance in advance; a
profiled move would mean a fresh dwell and acceleration ramp every few steps. So the
emitter has a constant-speed mode (`manualRun()`, the "manual" speed of 5.2.5) that
overrides the profile entirely; the jogs use it too. At 60 hsps the sampling lag of
5.2.12 is about 0.3 hs, which is why these edges can serve as the reference that
faster crossings are scored against.

Positions are the back-dated edge snapshots, the same ones homing uses. The portal
runs 3 passes, the console `k` 5 (1 to 12 accepted). The console prints every pass and
then each edge's mean and spread.

**The soft limits stay armed.** A creep that reaches a limit fails the calibration
("stopped at a soft limit - the band runs past it; aborted") rather than pushing
(2026-09-24).

**The result.** `calFinishBand()` shifts the frame so the mean `onFwd` is 0, stores
the other three relative to it, marks the needle homed, clears the slip evidence and
the clue, records `gMeasAtHome = gMeas` (a frame shift makes every earlier crossing
evidence about a frame that no longer exists, exactly as a new home does), and raises
a one-shot result. `loop()` collects it with `calTakeBand()` - a
snapshot, because any `applySettings()` in between would zero the live values - and
writes the four values into the settings. It **moves both soft limits by the same
shift**, because they are physical marks. If the moved pair no longer brackets 0, it
falls back to ±300. Then it applies, saves, and calls `applyNeedleMode()`.
`calFinishBand()` writes `gPos` directly from the needle task (see 5.7).

Why the collector matters: until 2026-09-02 nothing copied the band into
the settings. The result lived only in RAM, and the very next `applySettings()` -
triggered by the next step of the procedure, capturing a limit - put all four values
back to 0 while `gPos` kept the shift. Every limit captured afterwards was out by the
shift, about 640 hs on this machine. That is how the stored pair −1837..−117 of D16
came about.

`bandCalibrated()` is true when any of `offFwd`, `onRev`, `offRev` is non-zero. After a
real calibration `onFwd` is 0 and the other three are not.

The last result recorded on this machine: onFwd 0, offFwd 97, onRev 79, offRev −15
(2026-09-03).

### 5.2.16 Soft limits: capture, nudge, purge

- **The rule.** `posMin ≤ 0 ≤ posMax` and `posMin < posMax`. The index is a mid-travel
  mark, so any real pair brackets 0. `setGeometry()` refuses a pair that fails, and
  `applySettings()` writes back what was accepted, so the settings always hold what the
  needle runs.
- **Defaults: ±300** (`PROVISIONAL_LIMIT_HS` in `src/s3/main.cpp`). Deliberately narrow:
  with no end switches, a default must be a lower bound on the travel. It also has a
  lower bound: the post-home sweep at ±300 must cross the index band, or limit capture
  can never be satisfied (2026-09-03). Both bounds come from this machine's
  travel and band (§5.9).
- **Purge at load.** `purgeCorruptLimits()` runs once after the settings load. A stored
  pair that is crossed or does not bracket 0 is reset to ±300 and saved. The band values
  are kept: they are relative to the index.
- **Capture.** `captureLimit()` (console `m`/`M`, portal "Set low/high limit here" - one
  shared copy) stores the current position as `posMin` or `posMax`. It refuses unless the
  needle is homed, no calibration runs, the band is calibrated, the index has been
  crossed **and scored** since the home (`driftMeasured()`: `gMeas > gMeasAtHome`; a jog
  does not count), and no slip is outstanding. Then it applies and checks that the value
  took.
- **Nudge.** The portal's "Needle Limit ◀ 20 / 20 ▶" (`needle.nudge`) moves **both**
  limits by the same amount (±1 to 200 hs). It slides where the needle points for every
  frequency. It does not move the needle and it is not a clearance tool. It refuses a
  result that would not bracket 0. On 2026-09-23 the author pressed it four times
  expecting the needle to move; the buttons were relabelled "Needle Limit" to say what
  they move (2026-09-23).
- **Band carry**: see 5.2.15.

### 5.2.17 The AS5600, as the needle sees it

`readAs5600()` in `src/s3/needle.cpp` reads the raw angle at 50 Hz and adds the
difference, wrapped to ±2048, to `accAngle`. `readStatus()` keeps the raw status byte
(magnet detected, too weak, too strong), the AGC value and the field magnitude
separately, because "no magnet" collapses three conditions that need different
fixes. The sensor answers at I2C address 0x36 on the `S3_AS5600_SDA`/`SCL` pins
(Bible §11, §24); the 100 kHz bus clock is a firmware choice.

**Bus failure.** On an I2C error the reader backs off to one retry every 2 s,
silences the Arduino HAL logger for the length of the outage only, and counts the
outage (`encoderGaps()`). While the bus is down the shaft count is frozen and the
needle holds its last target. `i2cOk()` goes false (`i2c` 0 in the state JSON).

**Re-seating after a gap.** After any gap the count is not accumulated across the gap.
The fresh angle is placed on a turn by `seatTurn()`:

- With **both tuner ends measured** (`tunerEndsSet` = both, passed through
  `setCalibration()`), the tuner's whole travel lies inside one 4096-count turn, so
  exactly one turn puts the angle inside `[ends − 300, ends + 300]` (`SEAT_MARGIN`). That
  turn is taken, whatever moved during the outage.
- Otherwise (ends unknown, the measured window 4096 counts or wider, or the angle outside
  it) the turn **nearest the last count** is taken. That is exact for movement under
  half a turn.

A movement hidden by the outage counts as tuning (the still-timers restart), and the
console prints how far the shaft moved (2026-09-02; 2026-09-25).
That the tuner's travel is less than one shaft turn (about 2200 counts; 2197 measured on 2026-09-03) and that its
ends repeat within 300 counts are physical values of this machine, held by the
firmware (§5.9). The code checks the first at run time: a measured span too wide for
one turn falls back to the nearest turn.

**At boot.** `setup()` passes the measured ends first, then calls
`seedAccumulator(cfg.lastAngle)`, which seats the first reading on the right turn.
`loop()` saves the live count as `lastAngle` every 10 s when it changed and the encoder
is answering; a placeholder count from before the first good read is never saved.

**Tuning activity.** `tuningActive()` is true for 400 ms after the knob moved.
Movement is detected by adding up same-direction changes until they reach 12 counts
(`TUNE_DEADBAND`); a reversal resets the sum. A slow turn therefore still counts, and
jitter does not. It drives the display's tuning readout and the "knob is still"
timers, including the 3 s re-index wait.

### 5.2.18 The needle's memory across a software reset

A software reset does not move the needle; a power cut happens with the needle
wherever it last was. `bootPosition()` uses this.

**The record.** A small structure in `RTC_NOINIT_ATTR` memory, which survives a
software reset: a magic number `MEM_MAGIC` ("NM01", 0x4E4D3031), the position in fine
units, the homed flag, the clue's valid flag, the clue position, and a checksum
(FNV-1a over the fields, never over the structure's bytes).

**Writing.** `memRecord()`, on the needle task only, every pass in which anything
changed: magic = 0 first, then the fields, then the checksum, then the magic last. All
members are `volatile`, so the stores keep that order. A reset in the middle leaves
magic = 0. Nothing is recorded while a new zero is in flight (`gZeroPending`), and
nothing before the boot frame has been set (`gMemArmed`): the needle task starts in
`begin()`, before `setup()` reaches `bootPosition()`, and would otherwise overwrite the
record with the cold position 0.

**Reading.** `bootPosition()` reads the record once and invalidates it before doing
anything else, so it is never used twice. It uses it only when the reset reason is one
that leaves the needle where it was: `ESP_RST_SW` (portal reboot, OTA, console),
`ESP_RST_PANIC`, `ESP_RST_INT_WDT`, `ESP_RST_TASK_WDT` or `ESP_RST_WDT`. It checks the
magic, the checksum, that both flags are 0 or 1, and that both positions are within
±6000 hs (`MEM_POS_SANE_HS`). If all hold, it seeds the position (snapped to the current
microstep lattice), restores the clue, and prints
`needle: remembered at N (software restart) - homing from there.`

Otherwise - power-on, brownout, the reset pin, deep sleep, unknown, or a missing or
torn record - it calls `assumeAtLowStop()`, which sets the position to `posMin`. That
value is **a direction hint, not a position**: it makes the position negative so the
search goes up. It is wrong by up to the whole travel, and it is harmless only because
`applyNeedleMode()` does nothing while the needle is unhomed. That the needle is at
the low stop at power-on (the firmware parked it when the amplifier went off, and it
does not move while unpowered), and that a software reset leaves the rotor where it
was to within a detent, are physical values of this machine, held by the firmware
(§5.9).

**A watchdog reset counts as a software restart.** The task watchdog added in v.1.0
(§4.2.15) resets the S3 with `ESP_RST_TASK_WDT`, which is on the list above, so a hang of
`loop()` or of the supervisor ends in a restart that homes from the remembered position.
Seen on the radio on 2026-09-25, when a deliberately hanging image was reset by the
watchdog and rolled back: `remembered at -1154 ... homing from there`, and the index was found.

**Homing always runs afterwards.** The record is a better starting belief for the
search direction, not a substitute for finding zero. It is kept in fine units so the
drive's electrical phase matches where the rotor actually sits.

Verified on the radio on 2026-09-25: a portal reboot while tracking printed
"remembered at 301 (software restart) - homing from there", found the index and returned
to 301; the position was also remembered across an OTA restart.

### 5.2.19 Boot order

`setup()` in `src/s3/main.cpp`, as far as it concerns the needle:

0. The task watchdog is set up first of all (§4.2.3, §4.2.15), so the supervisor can
   subscribe to it when `Needle::begin()` starts it.
1. `settingsLoad()`, then `purgeCorruptLimits()`.
2. `protoSelfTest()` - a failure is reported and the boot carries on, so the needle is
   started and homed as usual; only the image's confirmation is withheld (§4.2.15).
   Before v.1.0 a failure halted `setup()` for ever, before the needle was started.
3. `Display::begin()`, `Panel::begin()`, then **`Needle::begin()`**: index pin as input,
   debounce seeded from a live read, `Drive::begin()`, invert on, duty 100 %, MICRO at
   `microFast`; the I2C bus started; first AS5600 status and angle reads; the gain-ceiling
   warning (against the compiled defaults - the stored settings are not applied yet); the
   `nstep` and `needle` tasks started.
4. `Rda::begin()`.
5. `Needle::setCalibration(calLow, calHigh, both ends measured)`, then
   `Needle::seedAccumulator(cfg.lastAngle)` - ends first, because the seed uses them.
6. `applySettings()` (twice, around the start of the inter-MCU link). This pushes every
   needle setting: limits, profiles, microstep, geometry (with write-back), index band,
   tuner ends, tuning curve, dial.
7. **`Needle::bootPosition()`** - the RTC record or `assumeAtLowStop()`.
8. **`Needle::startHoming()`** - the one home.
9. `Net::begin()`, `Portal::begin()`.

When homing finishes, `loop()` sees the homed flag change and calls
`applyNeedleMode()`, which sets `gAfter`; the sweep is not interrupted.

### 5.2.20 Stop, abort, and who writes the position

**Stop and abort are requests.** `stop()` and `calAbort()` can be called from the
console (`loop()`, core 1), from portal handlers and from the upload hook (portal task,
core 0). They only set a request bit (`gReq`) and zero the speed. The emitter stops
stepping at once: it checks the bit every control tick and before every step. The needle
task does the rest at the top of its next pass, within 5 ms, in `serviceRequests()`:
abort a calibration, clear the manual speed, end a recovery, end a repeating sweep, set
target = position, release the coils, release a held coil test (restoring MICRO at `microFast`),
state = IDLE. The bits are cleared only after the work. `manualRun()` refuses to set a
speed while a request is pending. `stopPending()` lets `main.cpp` wait for a stop to land
before starting the needle again. A running jog (`jogRaw()`) is not interrupted by a
stop (2026-09-24; 2026-09-25).

**Who writes `gPos`.** The emitter owns `gPos`. The supervisor hands corrections over
through `gCorrectHs`: one 32-bit word, one writer, one reader that only adds it in,
folded with an atomic exchange (2026-09-23). A new zero must wait for the fold
(5.2.9). Known exceptions that still write `gPos` directly are listed in 5.7.

### 5.2.21 Hunting detection

"Hunting" is a needle that keeps stepping without going anywhere. `huntTick()` checks
every 4 s (`HUNT_WINDOW_MS`): if at least 120 steps were emitted (`HUNT_STEPS_MIN`) and
the net movement is under 8 hs (`HUNT_NETMOVE_MAX`), `hunting()` is true. Following
the knob moves both numbers together, so only travel that cancels itself trips it; it
stayed quiet across 41 samples of a settled needle when it was added. It needs no
theory of the cause: a hunt has one signature, whatever causes it. What it cannot
see: a needle stuck and not moving, a slow one-way error, or a wrong but steady
reading. The console
prints `[WARN] the needle is HUNTING` on the rising edge and `the needle has settled` on
the falling edge; the portal shows the NEEDLE HUNTING pill (`hunt`). Nothing is driven
differently. It is a sense, not a fix, and it names no cause. The count is of emitted
steps at the current microstep division: 120 steps is 3.75 hs at 1/32 and 7.5 hs at
1/16. It exists because the wobble of 2026-09-10 was found by the author watching the
driver board's LEDs while the telemetry, truthfully, reported a stationary needle: both
positions it alternated between rounded to the same half-step (2026-09-10).

### 5.2.22 Firmware uploads and reboots: pause and resume

A flash write disables the flash cache and stalls every task running from flash,
including the emitter. So for an upload of the S3's own image:

- **Start.** `hOtaUpload()` in `src/s3/portal.cpp` calls `portalOtaQuiet(true)`, which
  calls `Needle::stop()` and blanks the display. While it is on, `applyNeedleMode()`,
  the amplifier edge, the recovery tick and the idle check do nothing.
- **Success.** The image is written and the S3 restarts. The needle memory makes the
  boot home start toward the index (5.2.18).
- **No restart.** A refused image (wrong chip id), an aborted upload, a failed write, or
  15 s with no upload activity (`otaWatchdog()`) calls `portalOtaQuiet(false)`, which
  raises `otaResume`. `loop()` acts on it only once the stop has landed
  (`!Needle::stopPending()`): it starts homing if the needle is unhomed, not recovering
  and not faulted, otherwise it calls `applyNeedleMode()` (2026-09-25).
- **A refused reboot.** The portal's Reboot first saves settings. If the save is refused
  because the needle is moving, `hReboot()` stops the needle and retries for 1.5 s. If
  the save still fails, the reboot is refused and `portalNeedleResume()` raises the same
  resume.
- **A reboot while the S3 image is on trial** skips the save and the needle stop
  altogether and restarts at once, because nothing is saved during the trial and the
  reboot is what rolls the image back (§4.2.15; v.1.0.1, 2026-09-25). The needle memory
  carries the position across it like any software restart.
- `writeSafe()` in `src/s3/main.cpp` refuses every settings write while the needle's
  speed is not zero, a calibration runs or an upload is active; `settingsWrite()`
  also writes nothing while the image is on trial (§4.2.15).

An upload relayed to the audio MCU does not stop the needle.

**An upload refused because the image is on trial.** Since v.1.0 a new S3 image runs
on trial until it is confirmed, about a minute after boot, and an S3 upload sent in
that time is refused (§4.2.15). The refusal is decided on the first chunk, before
`portalOtaQuiet()` is called, so the needle is never stopped for it: it keeps tracking
or parking as if nothing had happened, and there is nothing to resume.

**A rollback.** A trial image that crashes or trips the task watchdog restarts with a
reset reason the needle memory accepts (`ESP_RST_PANIC`, `ESP_RST_TASK_WDT`, ...), and
the bootloader starts the previous image, which reads the same RTC record. Proven on
the radio on 2026-09-25: after a deliberately hanging image was reset by the watchdog
and rolled back, the previous image printed `remembered at -1154 ... homing from there`
and found the index. The record is read by whichever image boots, so an image with a
different memory layout must change `MEM_MAGIC`.

### 5.2.23 The needle on the portal and the console

**Portal state** (the state JSON the page polls; names in `src/s3/main.cpp`): `pos`,
`tgt` (half-steps), `nstate` (state name), `homed`, `drift`, `driftpend`, `driftmeas`,
`calbusy`, `calpct`, `calmsg`, `fault`, `reidx`, `reidxtry`, `hunt`, `idx` (the index
switch, live, raw), `jit`, `i2c`, `magnet`.

**Pills** (status line): a `needle` dot lit when homed; "NEEDLE OFF - re-index pending
(try n/3)" while a slip is outstanding; "NEEDLE FAULT"; "NEEDLE RE-INDEXING";
"NEEDLE HUNTING".

**Needle tab buttons** (admin only; `doAction()` in `src/s3/settings_table.h`):

| button | action | calls | notes |
|---|---|---|---|
| Home | `needle.home` | `startHoming()` | refusal shown |
| Re-index | `needle.reindex` | `startReindex()` | refused during calibration, bring-up tool, homing or sweep; with no zero (FAULT, or a stopped home) it runs a plain home instead (5.2.14) |
| Sweep once / Sweep repeatedly | `needle.sweep` 0/1 | `sweepRange()` | refused unless homed |
| Track the tuner | `needle.track` | `track()` | refused when dropped |
| STOP | `needle.stop` | `stop()` | |
| ◀ 100, ◀ 10, 10 ▶, 100 ▶ | `needle.jog` | `jogRaw(n, reapHsps, MICRO)` | clamped to ±200 hs, said so if clamped; runs on the portal task and blocks it for the length of the jog |
| Set low / high limit here | `needle.limitLow/High` | `captureLimit()` | 5.2.16 |
| Calibrate the index | `needle.calBand` 3 | `calStartBand(3)` | 5.2.15 |
| Abort calibration | `needle.calAbort` | `calAbort()` | |
| Tuner = low end / high end | `needle.tuneLow/High` | stores `calLow`/`calHigh` | refused if the encoder is not answering |
| Needle Limit ◀ 20 / 20 ▶ | `needle.nudge` ∓20 | moves both limits | 5.2.16 |

There is no park button. The tab also holds the tuning-curve actions (chapter 6). The
tab's status card shows the position, the state (with the fault reason), the index
switch live, and the calibration's progress and message. The Now page shows the
position, the state and the index switch too.

**Console keys** (USB serial or the portal's Console tab; `loop()` in `src/s3/main.cpp`):

| key | does |
|---|---|
| `H` | home |
| `T` / `P` | track / park (say "NOT tracking/parking" when dropped) |
| `x` | stop (also aborts a calibration and ends a recovery) |
| `r` | repeating full sweep, until `x` |
| `k` | band calibration, 5 passes (needle on the switch first) |
| `A` | abort a calibration |
| `m` / `M` | capture the low / high soft limit here |
| `n` / `N` | jog −20 / +20 hs at 60 hsps, microstepped |
| `j` / `J` | jog +200 / −200 hs at 200 hsps, half-step mode |
| `u` / `U` | jog +200 / −200 hs at 200 hsps, microstepped |
| `1`..`4` / `0` | hold one driver input at full duty / release |
| `e` | index edge log on or off (every edge, with back-dated position and direction) |
| `l` | live index monitor, raw pin, up to one minute (holds `loop()` while it runs, feeding the task watchdog) |
| `c` / `C` | store the tuner's low / high end |
| `g` / `G` | print that the end-stop calibration was removed and that `m` / `M` replace it. Kept because old instructions still name these keys, and silence would be indistinguishable from a key that worked |
| `Z` | clear the jitter counters |
| `s` | status, including position, target, state, drift, crossings, jitter, steps, speed, PWM, and the AS5600 chain with the `(accumulator − raw) & 4095` residue, which must stay constant; then the command list, because the S3's console does not reset when a monitor attaches and the boot banner is never seen |
| `D` | dump all settings |

The keys `,` and `.`, which stepped `upVmax` by 100, were removed (2026-09-25):
`.` had no ceiling, and the portal's `upVmax` row does the same job
inside its bounds.

The `l` monitor runs inside `loop()` for up to a minute, longer than the 15 s task
watchdog on `loop()` allows. `limitMonitor()` therefore feeds the watchdog
(`wdtFeed()`) on every pass, beside the PING that keeps the link alive, and it stops
on any key other than Enter or after 60 s.

## 5.3 Settings and constants

### Settings

All are rows on the portal's Needle tab (admin), in the downloadable settings file and in
the console dump `D`, unless marked. Stored in NVS on a 2 s debounce, never while the
needle moves and never while an S3 image is on trial (§4.2.15). Defaults are in `struct Settings` in `src/s3/main.cpp`; rows in
`src/s3/settings_table.h`. The portal range clamps the value.

| name | default | unit | range | meaning | changed by |
|---|---|---|---|---|---|
| `upVmax` | 1100 | hsps | 100..4000 | top speed of every second-order move: tracking, sweeps, homing. Portal label "Sweep up, top speed". The default is this mechanism's measured limit (§5.9) | portal, file |
| `upAccel` | 18000 | hsps² | 1000..60000 | acceleration limit of the second-order law | portal, file |
| `dnVmax` | 1700 | hsps | 100..4000 | top speed of the decay (park) move | portal, file |
| `dnAccel` | 18000 | hsps² | 1000..60000 | used only in the park's arrival test | portal, file |
| `wn` | 9.0 | rad/s | 1..21 | ring-down; keep below `4 × zeta × upAccel / upVmax` | portal, file |
| `zeta` | 0.50 | - | 0.1..2.0 | overshoot (about 5 degrees, about 300 ms tail on the rig) | portal, file |
| `riseMs` | 100 | ms | 10..3000 | decay envelope rise | portal, file |
| `fallMs` | 1000 | ms | 10..5000 | decay envelope fall | portal, file |
| `dwellUpMs` | 100 | ms | 0..3000 | pause before a second-order move (not while tracking) | portal, file |
| `dwellDnMs` | 1000 | ms | 0..3000 | pause before the park | portal, file |
| `microFast` | 16 | 1/n | 1..32 | microstep for sweeps, park, calibration, jogs; rounded down to a power of two | portal, file |
| `microSlow` | 32 | 1/n | 1..32 | microstep while tracking | portal, file |
| `reapHsps` | 60 | hsps | 10..500 | speed of the band calibration's four creeps (the "measuring pass") and of the portal jog buttons; nothing else. Not the homing re-approach, despite its name (5.2.9). Portal label "Measuring pass speed", kept as it is | portal, file |
| `posMin` | −300 | hs | −3000..3000 | low soft limit = park position = where `dialLow` is printed | `m`, portal capture, nudge, band carry, purge, portal, file |
| `posMax` | +300 | hs | −3000..3000 | high soft limit = where `dialHigh` is printed | `M`, portal capture, nudge, band carry, purge, portal, file |
| `idxOffFwd` | 0 | hs | −500..500 | index turns off, moving + | band calibration (normally only) |
| `idxOnRev` | 0 | hs | −500..500 | index turns on, moving − | band calibration |
| `idxOffRev` | 0 | hs | −500..500 | index turns off, moving − | band calibration |
| `idxOnFwd` | 0 | hs | not a row | index turns on, moving +; 0 by definition | band calibration only |
| `sweepOn` | 0 | on/off | 0..1 | a full sweep on each amplifier-on edge | portal, file |
| `dialLow` | 879 | 0.1 MHz | 500..2000 | frequency printed at the low needle stop | portal, file |
| `dialHigh` | 1079 | 0.1 MHz | 500..2000 | frequency printed at the high needle stop | portal, file |
| `calLow` / `calHigh` | 0 / 6023 (placeholders, §5.9) | counts | −20000..20000 | shaft count at the tuner's two mechanical ends | `c`/`C`, "Tuner = low/high end", portal, file (each sets its "measured" bit) |
| `lastAngle` | 0 | counts | not a row | last live shaft count; seeds the turn at boot | `loop()` every 10 s |

`dialLow`/`dialHigh` defaults describe this set's printed face, 87.9 MHz at the low
stop and 107.9 MHz at the high stop; they are physical values of this machine held
by the firmware (§5.9), and another set's builder types in their own. The ranges of
`posMin`/`posMax` are wide on purpose, because validity is checked by `setGeometry()`,
not by the range. They are signed on purpose too: until 2026-09-02 the portal's
`posMax` range was 0..3000, so a negative value was silently clamped to 0 and
answered "ok" (fixed on 2026-09-02). The three band rows exist since 2026-09-24 so
that the settings file carries them: a restore onto blank flash had left all four
band values at 0, and with them the band check and reverse-crossing scoring switched
off (5.2.12, 5.2.13). The `upVmax` row steps by 50.

### Compile-time constants

| name | value | unit | meaning |
|---|---|---|---|
| `CTRL_US` | 1000 | us | profile control tick |
| `FINE_PER_HALFSTEP` / `FINE_PER_EREV` (drive.h) | 256 / 2048 | fine | lattice unit; one electrical revolution |
| `DRV_LEDC_FREQ_HZ` / `DRV_LEDC_BITS` (drive.h) | 20000 / 11 | Hz / bits | PWM; bits negotiated down to 8 |
| LEDC channels (drive.cpp `CHAN`) | 2, 3, 4, 5 | - | stepper inputs |
| step interval clamp | 20 us .. 1 s | - | 50 000 steps/s ceiling |
| coil release delay | 400 | ms | after a move ends |
| `IDX_DEBOUNCE_K` | 4 | polls | index debounce |
| sample interval clamp | 20 000 | us | lag model cap |
| `CORRECT_MAX_HS` | 40 | hs | largest drift absorbed |
| `BAND_MARGIN_HS` | 30 | hs | band-check margin, plus 40 ms of travel |
| `TRACK_DEADBAND_HS` | 2 | hs | tracking and park deadband |
| `HOME_BUDGET_HS` | 2336 | hs | search budget per direction |
| `HOME_BACKOFF_BUDGET_HS` | 400 | hs | backoff budget |
| `HOME_REAPPROACH_BUDGET_HS` | 300 | hs | re-approach budget |
| backoff hop / clearance | 20 / 60 | hs | in `homingTick()` |
| re-approach hop | 10 | hs | in `homingTick()` |
| `AUTO_TRIES_MAX` | 3 | - | failed automatic re-indexes before FAULT |
| `BAND_BUDGET` | 600 | hs | per band-calibration creep |
| band passes | portal 3, console 5, 1..12 | - | `calStartBand()` |
| `HUNT_WINDOW_MS` / `HUNT_STEPS_MIN` / `HUNT_NETMOVE_MAX` | 4000 / 120 / 8 | ms / emitted steps / hs | hunting detector |
| `MEM_MAGIC` | 0x4E4D3031 | - | RTC record; change it if `NeedleMemory` changes |
| `MEM_POS_SANE_HS` | 6000 | hs | RTC record sanity bound |
| AS5600 poll / status | 20 ms / every 25th | - | |
| bus retry | 2000 | ms | after an I2C failure |
| `TUNE_DEADBAND` | 12 | counts | knob-movement detector |
| tuning-active hold | 400 | ms | `tuningActive()` |
| `TRACK_ACC_ALPHA` | 0.35 | per 20 ms | the needle's shaft filter |
| `SEAT_MARGIN` | 300 | counts | turn window around the tuner ends |
| decay error gain | 20 /s, cap 0.3 × `dnVmax` | - | `runProfile()` |
| `jogRaw()` speed clamp | 5..900 | hsps | |
| portal jog clamp | ±200 | hs | `needle.jog` |
| `needle.nudge` range | ±1..200 | hs | |
| `PROVISIONAL_LIMIT_HS` (main.cpp) | 300 | hs | default and purge soft limits |
| `REIDX_STILL_MS` (main.cpp) | 3000 | ms | knob still before an automatic re-index |
| `IDLE_CHECK_MS` (main.cpp) | 120 000 | ms | amplifier off before the routine index check |
| `lastAngle` save period (main.cpp) | 10 000 | ms | |
| `HALFSTEPS_PER_REV` (drive.h) | 4096 | hs | defined, used nowhere |
| run-time placeholders | `posMin/Max` ±300, dial 88.0..108.0, curve flat 88.1 MHz | - | overwritten by the first `applySettings()` before any move |

## 5.4 Decisions

These are choices made for this mechanism; your mileage may vary (§0.3). The first firmware
commit in the repository (2026-09-01) already carried the decisions dated 2026-08-30
and 2026-08-31 below; their evidence is the code.

**D1. The needle is an electrical indicator of the tuning shaft, and it follows
frequency through the printed face.**
Why: the needle is moved by the stepper and the knob turns the tuning capacitor
(Bible §11, §13, §14, §31.4), so the firmware is what ties them together (§5.1). The
capacitor is one ganged assembly shared with AM and SW (Bible §13, §18) and turns
wider than the printed FM scale, so following the capacitor's travel directly put
every station compressed and offset. Following frequency through the printed face
does not.
Rejected: `posMin + permille × travel / 1000` (following the capacitor's travel).
When: 2026-09-02.

**D2. The motion comes from the bench rig: a second-order law for every move but the
park, a decay law for the park, a dwell before commanded moves, microstep by activity.**
Why: a trapezoid profile reads as a machine; a real meter movement is a damped
second-order system that overshoots slightly and settles. The rig's recorded results,
settled on 2026-08-30: `wn` 9, `zeta` 0.50 gave about 5 degrees of overshoot and a
300 ms ring-down (475 ms at `wn` 6 / `zeta` 0.60); the fall gave a 1 s hesitation, a
peak at about 15 % of travel, a long trickle, and landed exactly. Measured step
throughput at 1700 hsps: 1/8 and 1/16 delivered 100 %, 1/32 delivered 92 % (clamped).
Worst step jitter on the rig was 5 to 10 us (measured with no motor attached).
1/32 was kept for slow motion because it is smoother exactly where smoothness shows:
the knob being followed, and the long trickle at the end of a fall. The firmware
before the rig swept at 850 hsps on a trapezoid, about 2.8 s for a full sweep; the rig
existed because that shape read as a machine and that speed was far below what the
motor could do.
Rejected on the rig (see 5.6): the trapezoid the firmware first shipped (850 hsps), an
s-curve, an exponential approach, a spring-with-friction fall, and a random "wobble".
When and evidence: 2026-08-30, settled on the rig; carried in
`src/s3/needle.h`'s banner. The rig's table is not a standard the firmware departs
from: its results became the firmware's settings, and the settings in 5.3 are the
rule. So the needle parks at 1/16 (`microFast`) and only tracking uses 1/32.

**D3. `upVmax` is 1100, not the rig's 1700.**
Why: going up through the index faster than this, the needle stopped following while
the firmware counted on - magnet still over the switch at about +88 - and every
power-up sweep landed about 400 hs (about 5 MHz) low. No firmware write moved the
position. The author swept `upVmax`: 1500 failed every time, 1200 to 1400 often, 1100
never. The band check (5.2.13) was added in the same commit to catch it if it recurs.
When: 2026-09-24. The limit is a physical
value of this machine, held by the firmware (§5.9).

**D4. No end switches: one mid-travel index, soft limits, step budgets and a starting
belief.**
Why: the author measured that this motor does not lose steps at the settled
configuration, so absolute position can be kept by counting and re-referenced at each
index crossing. On 2026-08-31 one full sweep came back to the index 17 hs out, and
repeated returns agreed within about 12 hs, gear lash included. An edge log over 30
minutes on 2026-09-02 then showed the trip points repeating within ±4 hs over dozens
of crossings; the only jump in it (about 215 hs) came at the moment the needle was
commanded into a stop. Steps are lost when the needle is driven past a physical stop,
and otherwise not.
Rejected: two end-stop switches with homing against them (5.6).
When and evidence: 2026-08-30; `src/s3/needle.h` banner. That the motor and gearbox
do not lose steps at the settled configuration is a physical value of this machine,
held by the firmware (§5.9).

**D5. Home once, at power-up. The front switch does not home.**
Why: the S3 boots only when the set is plugged in, so its boot is the first power-up.
After that the position is known and every crossing corrects it. The front switch
only decides where the needle should point, and the run from the low stop to a
station is itself a sweep, so nothing is lost to the eye. `sweepOn` became pure
theatre on the same day: a full sweep on each amplifier-on edge, on top of that run.
Before this, every touch of the switch started a search and a sweep.
Rejected: a search and full sweep every time the front switch is turned on.
Exception kept: a needle with no zero homes on the amplifier-on edge. That includes a
needle in FAULT, so every amplifier-on edge sends a faulted needle searching again.
This is kept on purpose.
When and evidence: 2026-08-31. The intent: the first calibration happens only at first
power-up. Afterwards the firmware always knows where the needle is, since it is at the
lowest point at every Off, and every time the needle passes in front of the index sensor
it re-references passively, so as not to lose steps.

**D6. Zero is one edge approached from one direction: the forward ON edge, after a
backoff below the band.**
Why: a Hall switch turns off at a weaker field than it turns on, so the ON region
shifts with the direction of travel. Approaching one edge one way cancels that, and
gearbox backlash the same way. The other three edges are still measured, so a
crossing in either direction can re-reference. Zero is defined by the slow
re-approach, not by a crossing at speed, whose lag is compensated but not removed.
Rejected: reading the index inside the step emitter for more precision at speed -
decided against on 2026-09-09 (a gain of single half-steps, at the cost of
moving the sensor read across a task boundary on a mechanism with no end switches).
Since the ladder of D8, a reading taken at speed only decides between absorbing and
re-indexing; a re-index goes and looks instead of believing the number, so extra
precision at speed would buy little.
When and evidence: 2026-08-30 (`src/s3/needle.h`), 2026-09-09.

**D7. Every crossing re-references the frame, but only a directional edge is scored.**
Why: the intent of D5 (2026-08-31). The two directions trip about 79 hs apart,
twice what may be absorbed, so a guessed direction produces a confident, large, wrong
number. The sampling lag (half a poll interval of travel) is subtracted.
Rejected: `gVel >= 0` as the direction (guessed "forward" at rest, which is after
every completed move); scoring reverse edges before the band is measured; treating
sampling lag as slip. The lag is not small at speed: read every 5 ms, a crossing at
the sweep's 1500 hsps of the time was up to 7.5 hs late, always in the direction of
travel, while the band edges it was scored against were taken at 60 hsps with 0.3 hs
of lag. Absorbed as drift, that made the frame breathe by about a needle width
depending on which way it last crossed. With the lag subtracted, the first boot read
`last drift 4` against 7 to 13 before.
When: 2026-09-03, 2026-09-24.

**D8. Absorb up to 40 hs; above that, re-index - never trust the bigger number.
Three failed attempts, then FAULT. A failed attempt re-arms the soft limits.**
Why: before 2026-09-08 a drift over 40 hs set a flag, printed "re-home with H", and did
nothing else. There was no path back inside the window, so the needle ran on against
a frame it knew was wrong - and the serial console is normally unplugged, so in
practice until the next power cycle. It was a ratchet, not an accumulation: on
2026-09-07 the needle was 394 hs out after 16 h 40 min of ordinary use (about 24 hs
an hour), every slip since the first one that exceeded 40.
Absorbing a large drift trusts a suspect number and shifts the frame by it;
re-indexing uses it only as a direction hint and takes the new zero from the switch.
The author drew that distinction. Each rung of the ladder uses a better measurement
than the one below, not a bigger dose of the same one. An automatic re-index ends
without the power-up sweep, which is theatre: it should happen quietly between one
thing and the next. A needle that slips straight back out is mechanical,
and repeating would grind. Bounded-and-wrong is better than unbounded: re-armed limits
cost accuracy, not gearbox.
Rejected: raising the 40; requiring N consecutive crossings to agree (the index is
crossed about twice per power cycle, hours apart); a separate "full search" rung
(SEEK_INDEX already tries the other way).
When and evidence: 2026-09-08 (a re-index measured at about ten seconds end to
end, needle back on the same position). A needle left wildly inaccurate, with nothing
done about it until the S3 reboots or is power-cycled, was the failure this fixes.

**D9. The band check: ON outside the measured band means the frame is wrong.**
Why: see D3; the crossing check cannot see a loss after a correct ON edge. Gated on a
calibrated band; margin grows with speed.
When: 2026-09-24.

**D10. A slip during the power-up sweep ends the sweep at once and re-indexes.**
Why: the sweep drives to limits in a frame the slip has just shown wrong.
When and evidence: 2026-09-24. No 3 s wait.

**D11. The automatic re-index waits for the hand to leave the knob: 3 s of stillness,
amplifier on or off.**
Why: keep tracking while the listener is tuning, and correct the moment they stop. The
same signal (`tuningActive()`) returns the display from the tuning readout to the
clock, so the re-index waits for exactly what the display calls "the hand has left
the knob". With the set off, nobody is looking at the dial, which is the best moment
to spend ten seconds putting it right.
When and evidence: 2026-09-08; `REIDX_STILL_MS` in `src/s3/main.cpp`.

**D12. A routine index check two minutes after the amplifier goes off.**
Why: parking never crosses the index from below, so lost steps could go unseen until
the next session.
When and evidence: 2026-09-23. Seen working on the radio
on 2026-09-24: the check found the index and parked the needle again.

**D13. The index clue is kept until a better one replaces it.**
Why: a re-index cut short by a stop used to leave no clue, and the next home fell back
to the sign of a frame the slip had just proved wrong - with the limits disarmed and a
2336 hs budget, a search that can walk into a stop.
When: 2026-09-25.

**D14. The starting belief: the low stop after power-on; the remembered position after
a software reset.**
Why: after portal reboots and uploads with the needle above the index, "believe the low
stop, search up" ran away from the index toward the high stop ("index not found in the
expected direction" at 301). A brownout is excluded on purpose: a count kept through a
supply sag is not one to steer by.
Rejected: a boot position saved to NVS (planned 2026-08-30, never built); a cold
position of 0 (it searched down into the stop the needle already sat on, 2026-08-31).
When and evidence: `assumeAtLowStop()` 2026-08-31; RTC memory 2026-09-25.

**D15. Homing budgets are properties of the mechanism, not of the calibration.**
Why: budgets derived from the soft-limit span gave, with the provisional ±300, a 750 hs
search against a needle about 1195 hs below the index: it ran out going up, tried the
other way into the low stop and faulted, on the first boot after a purge. Every phase
is budgeted: a magnet already over the switch enters at BACKOFF, which a search-only
budget never covered.
When and evidence: 2026-09-03. The numbers come from this machine's travel and band (§5.9).

**D16. Soft limits must bracket the index; defaults ±300; purge at load; carry with the
band shift.**
Why: a stored pair of −1837..−117 put the index outside the needle's range, killed
passive re-referencing and drove the park into a stop. A default with no end switches
must be a lower bound on the travel. Asking the person to re-set the limits after every
band calibration was the step every earlier corruption came from.
Rejected: ±967 defaults (an estimate about 293 hs past the measured high stop).
When: 2026-09-02 to 09-03.

**D17. The band calibration measures from the switch outward, never drives into a
stop, and keeps the soft limits armed. The end-stop calibration was removed.**
Why: the end-stop calibration checked nothing before it drove - not homed, not which
side of the index, and it disarmed the limits. Run the wrong one and it crept 4000 hs
(about 66 s) into the physical stop, then left the frame 4000 hs out. Capturing each
limit by hand against a visible position replaces it. The principle, from the author's
procedure of 2026-08-31: the operator places the needle, and the machine measures
back to the index; nothing is driven into a stop, so nothing can lose steps while
being calibrated. Every earlier attempt had driven outward against a guessed limit
and lost about 300 hs doing it. The band calibration's own
limit disarm was a leftover from when it could run unhomed.
When and evidence: end-stop calibration removed 2026-09-03; limits kept armed
2026-09-24.

**D18. The needle follows the unrounded frequency, with a 2 hs deadband, from a
filtered shaft reading; no dwell while tracking.**
Why: following the rounded tenth of a MHz made the target hop 7.85 hs back and forth
across a rounding boundary on encoder noise - the wobble of 2026-09-10, reported by the
author. A pointer on a glass dial is an analogue indicator; it has no business
snapping to tenths of a MHz. Both faults (the rounding and the missing deadband) had
been latent for weeks: they waited for the tuning to rest near a rounding boundary,
and a change to the tuning curve moved where those boundaries fall, so a change to
the frequency mapping surfaced days later as a mechanical symptom. After the fix, 68
of 71 samples of a resting needle sat at one position. One noise sample could still
move the target 2 to 3 hs, giving position flips at idle (−481/−484) and 227- and
363-step hunting bursts after boot (2026-09-23/24); hence the filter. The deadband,
narrower than the pointer is wide, removes an audible profile per half-step; the
same deadband on the PARKED branch was found by looking for the same shape rather
than waiting for it to show.
When and evidence: 2026-09-10, 2026-09-24,
2026-09-02 (dwell). There is no median filter
anywhere; the exponential filter is the rule.

**D19. The shaft's turn is chosen by the measured tuner ends.**
Why: before 2026-09-25 an outage dropped the movement it hid, so the frame stayed shifted
until the next reboot; re-seating on the nearest turn fixed that, but nearest-turn was
right only for movement under half a turn; beyond that it saved a wrong turn as
`lastAngle` and every later boot inherited it.
When: 2026-09-25.

**D20. Stop and abort are requests; the emitter alone writes the position; a new zero
waits for the fold.**
Why: called from core 0, the old in-place stop raced the calibration and homing on
core 1: an abort's zeroed creep speed was written straight back and the jog ran on to a
soft limit. Corrections written between a read and a clear were lost.
When and evidence: 2026-09-23, 2026-09-24, 2026-09-25. The writes of `gPos` that still
happen outside the emitter are a firmware note (5.7).

**D21. The emitter runs on core 0; core 0's idle task is off the task watchdog; no
flash writes while the needle moves.**
Why: see 5.2.2. An NVS write disables the flash cache and stalls the emitter; the
project measured step jitter of 383 us during a write against 49 us with the guard
(2026-08-31); the clock display's worst slot error went from 1257 us to 8 us in the
same test. Everything not in IRAM stalls during a write, and the emitter is not in
IRAM. For uploads, a dark and still needle is preferred to a stuttering one.
Portal slowness while the needle moves was accepted as a cost on 2026-09-09.
When and evidence: `stepTask()`, `writeSafe()`, `portalOtaQuiet()`.

**D22. Jogs are not bounded by the soft limits; their step count is.**
Why: the jog is how the needle is placed to set a limit. Bounded by the limits, a
needle sitting at `posMax` refused every forward press and the limits could never be
widened. The request is clamped to ±200 hs where it enters; crossing a limit is
announced. After a jog the needle stays IDLE where it was put: on 2026-08-31 every
manual nudge was undone a moment later by the supervisor re-asserting its target,
and a hand command the machine overrides is worse than no command; automatic behaviour
resumes on `H`, `T`, `P` or a source change.
When and evidence: bounded 2026-09-01, unbounded again 2026-09-02.

**D23. FAULT is left to a person.**
Why: if the firmware cannot fix it, a power cycle will not either. Nothing automatic
retries a FAULT; every way out is something a person does: Home, Re-index, `H`, or
switching the amplifier on, which is kept as one of those ways (D5). A
panel-lamp pulse to signal FAULT was proposed and left out.
When and evidence: 2026-09-09; D5's exception, 2026-09-25.

**D26. Re-index with no zero runs a plain home.**
Why: a re-index recovers a frame that exists. A failed one hands back that frame; on a
needle that never had one, it handed back an unmeasured frame and armed the soft
limits in it (5.2.14).
When and evidence: v.1.0.1, 2026-09-25. Not yet exercised on the radio.

**D27. `upVmax` is changed only through its settings row; the console `,` and `.` keys
are gone.**
Why: `.` raised `upVmax` by 100 with no ceiling, past the speed this mechanism can
follow (D3). The portal row keeps the value inside its range.
When: 2026-09-25.

**D24. Refusals say why, and failures are visible.**
Why: a refusal once shown in the success colour made the author believe limits were
captured. `startHoming()`, `calStartBand()`, `track()`, `park()`, `sweepRange()`, the
jog clamp and limit capture all return a reason. Hunting, pending slips, re-indexing
and faults are in the state JSON and the pills. The rule behind the telemetry dates
from 2026-09-10, after the wobble (D18):
every failure the machine can have must move some published field, at a resolution
that would show it. `pos` did cover the wobble, only not finely enough; eighteen
code reviews had not caught the rounding, and the author found it by watching the
driver board's LEDs. The machine's account of itself was accurate and useless at the
same time.
When: 2026-09-24/25.

**D25. `idxOnFwd` is not a settings row.**
Why: it is 0 by definition; an edited value moved both soft limits physically, into a
stop.
When and evidence: 2026-09-24.

**D28. `track()` and `park()` name where the needle goes next; they never cut a busy
phase short.**
Why: they used to set the state directly. `loop()` calls `applyNeedleMode()` on the
homed edge sooner than the 5 ms supervisor runs, so the power-up sweep was overwritten
before it started: "one trip to the low end, then straight to the station". Now they
set `gAfter` and leave a home, sweep or calibration alone (5.2.10), and since 2026-09-25
they also say whether the request will be carried out.
When: 2026-09-02, 2026-09-25.

**D29. An interrupted upload resumes the needle only after the stop has landed; a
refused reboot resumes it too.**
Why: a stop is carried out up to 5 ms after it is asked for (5.2.20). A resume issued
in that window would be undone by the stop, leaving the needle IDLE; so `loop()` waits
for `stopPending()` to clear (5.2.22). And a reboot refused because the save failed
had stopped the needle for the save and never restarted it.
When: 2026-09-25.

## 5.5 Failures and recovery

Classes follow the recovery ladder of §0.5. BLOCKER: the radio is unusable until a full
power cycle, or the harm survives one (needle driven into a stop, calibration or settings
corrupted and saved). DEFECT: recovers with a portal reset, a console reset or the front
power switch. NOTE: recovers by itself, or cosmetic.

| what fails | what the firmware sees | what it does | how to recover | class |
|---|---|---|---|---|
| a few steps lost (≤ 40 hs) | drift at a scored crossing | absorbs it | nothing | NOTE |
| many steps lost (> 40 hs) | drift at a scored crossing | clue, slip flag, "NEEDLE OFF" pill; re-index after 3 s of still knob | nothing | NOTE |
| steps lost with the magnet still over the switch | band check | same as above | nothing | NOTE |
| a slip during the power-up sweep | band check or crossing | sweep abandoned, re-index at once | nothing | NOTE |
| steps lost while parked or parking below the index | nothing until a crossing | routine check after 2 min amplifier-off | nothing | NOTE |
| an automatic re-index fails (1st, 2nd) | a spent budget | limits re-armed, old frame kept, retry | nothing | NOTE |
| the third automatic re-index fails | `gAutoTries` = 3 | FAULT, coils released, reason in `fault` | portal Home or Re-index (both run a plain home), console `H`, `x` then `T`, amplifier off then on | DEFECT (a person's job, D23) |
| a hand or boot home fails ("index never found in either direction", "sensor never released during backoff", "sensor never returned during the re-approach") | a spent budget | FAULT at once | as above; the console suggests checking the switch, its magnet, its pull-up and the motor | DEFECT |
| Re-index pressed on a needle with no zero | `startReindex()` sees no zero | runs a plain home; a failure returns to FAULT, never to a guessed frame | as above | NOTE (fixed in v.1.0.1; never exercised) |
| the first search goes the wrong way | the first budget spent | warning, one reverse pass | nothing, if the needle survives it | BLOCKER class if the needle is driven against a stop (limits are disarmed; up to 2336 hs). Made unlikely by the clue and the RTC memory, not impossible |
| power cut or brownout with the needle above the index | power-on or brownout reset | assumes the low stop, searches up first | nothing, if it survives the wrong-way pass | same as above; brownout accepted (D14) |
| torn or missing RTC record after a software reset | magic or checksum | assumes the low stop | same as above | NOTE when the needle was low; see above otherwise |
| hunting | detector | warns only | find the cause; `x` stops | NOTE |
| AS5600 stops answering | I2C error | count frozen, needle holds, retry every 2 s, re-seat on return | nothing when it returns; wiring otherwise (Bible §11) | NOTE |
| shaft turn ambiguous after a long outage or at boot | re-seat | with both tuner ends measured: the one turn inside them | measure both tuner ends | NOTE with ends measured; without them a wrong turn can be saved and inherited by every boot - BLOCKER class, cured by measuring the ends |
| crossed or non-bracketing soft limits typed in | `setGeometry()` | refused, old pair kept, console warning | enter a valid pair | NOTE |
| corrupt soft limits in flash | `purgeCorruptLimits()` | reset to ±300 and saved | home, band-calibrate, capture the limits again | guarded (would be BLOCKER) |
| a creep in the band calibration reaches a soft limit | `stepAllowed()` | calibration fails and says so | move the limit, or accept | NOTE |
| an S3 upload | upload start | needle stopped, display dark | wait | by design |
| an upload refused, aborted, failed or silent for 15 s | upload hooks, `otaWatchdog()` | resume once the stop has landed | nothing | NOTE |
| portal reboot refused because the save failed | `hReboot()` | needle resumed | retry later | NOTE |
| portal reboot while the S3 image is on trial | `hReboot()` | restarts at once, no save; the previous image comes back and homes from the remembered position | nothing | NOTE (proven live 2026-09-25) |
| a software reset, crash or watchdog mid-motion | reset reason + RTC record | homes from the remembered position | nothing | NOTE |
| LEDC accepts no resolution at 20 kHz | `Drive::begin()` | "[FAIL] LEDC would not accept 20 kHz at ANY resolution"; the needle never moves | a firmware change | BLOCKER (never seen) |
| the supervisor task (`needle`, core 1) hangs | the task watchdog, 15 s | the S3 restarts; the needle homes from its remembered position | nothing | NOTE (never seen) |
| a task on core 0 (`nstep` or `portal`) hangs for good | nothing (idle task and these tasks not watched) | nothing | power cycle | BLOCKER class; accepted cost |

## 5.6 Graveyard

These were dead ends on this machine. They may not be dead ends on yours.

1. **Two end-stop switches, homing against them** (2026-08-28). Replaced on 2026-08-30
   by one mid-travel index. The physical change is hardware (Bible §11).
2. **Plain `digitalWrite` half-stepping** (and a stepping library's pin order).
   Replaced by LEDC sine microstepping: here, half-step chatter was visible at needle
   speeds.
3. **An 850 hsps trapezoid profile.** Here it read as a machine.
4. **Rig profiles not adopted:** an s-curve (a jerk-limited chase overshot 33 degrees
   before it was re-planned in time), an exponential approach (servo-like), a
   spring-with-friction fall (it could stop short, "a tired set"), and a random
   "wobble" (measured not viable on this gearbox). They were judged on taste and on this
   motor. The rig's measurements, a 170-degree move at 2000 hsps and 6000 hsps²:

   | profile | character | overshoot | settled |
   |---|---|---|---|
   | trapezoid | constant acceleration, cruise, constant braking; reads as a machine | 0.2° | 1.62 s |
   | s-curve (planned in time) | no corner anywhere | 0.1° | 2.06 s |
   | second-order (kept) | the meter movement | 6.0° | about 4.3 s at the rig's gains of the day |
   | exponential | decisive start, soft landing, never overshoots; servo-like | 0° | 3.08 s |
   | spring-with-friction fall | a mass on a return spring against friction | 0° | 4.59 s |

   The spring fall could be made to stop short of zero (at one setting it died 19.8
   degrees short), the signature of a worn vintage set; the author preferred a fall
   that always lands. The wobble needed microstepping to show at all, and had to stay
   under the gear lash or the needle buzzed instead of breathing.
5. **Homing on every amplifier power-up.** Replaced by homing once (D5).
6. **Cold `gPos` = 0 as the boot belief.** Here it searched down into the stop the
   needle sat on. Replaced by `assumeAtLowStop()`, now itself limited to cold starts.
7. **A boot position saved to NVS.** Planned on 2026-08-30 as a switch replacement,
   never built; the RTC record does the job for software resets.
8. **Homing budgets from the soft-limit span** (D15).
9. **±967 default soft limits** (D16).
10. **The end-stop calibration** (`g`/`G`, two portal buttons), removed 2026-09-03 (D17).
11. **A seek-speed setting.** Deleted on 2026-08-31: the search only has to find the
    switch.
12. **`indexStable()`**, two reads microseconds apart, and before it an edge detector
    that froze during jogs and calibration and invented a crossing when released.
    Replaced by the K = 4 back-dated debounce (2026-09-23).
13. **Writing zero straight into `gPos` at REAPPROACH.** Replaced by a back-dated shift
    through `gCorrectHs`, then made to wait for the fold (2026-09-23).
14. **A 64-bit `gPos`.** Torn reads across cores; now 32-bit (2026-09-24).
15. **Read-then-clear of `gCorrectHs`.** Lost corrections; now an atomic exchange
    (2026-09-23).
16. **Limit-capture guards on `|lastDrift()| > 12` and on a crossing count snapped at the
    start of homing.** The first refused on a drift already corrected and its advice
    regenerated the trigger; the second was satisfied by homing's own crossing. Replaced
    by `driftPending()` and `gMeas`/`gMeasAtHome` (2026-09-02, 2026-09-03).
17. **Following the capacitor's travel, then the rounded frequency** (D1, D18).
18. **Accumulating across an AS5600 gap** (it injected a whole revolution), then
    dropping the movement, then nearest-turn re-seating; now the measured-ends turn
    (2026-09-02, 2026-09-25).
19. **Absorbing more; N agreeing crossings; a full-search rung** (D8).
20. **A jog bounded by the soft limits** (D22).
21. **The band calibration disarming the soft limits** (D17).
22. **In-place `stop()` and `calAbort()` from any core** (D20).
23. **A dwell before every tracking move** (D18).
24. **A panel-lamp pulse for FAULT** (D23).
25. **The bench rig itself.** A bench rig (a bare motor, driver and ESP32 with no
    needle) produced the motion numbers in D2. It was a testing tool to get to v.1; it is
    retired and not published, and anyone who needs one can rebuild it (5.8).
26. **A real re-index on a needle with no zero.** Here: a failed one handed back a
    frame that had never been measured and armed the limits in it. Replaced by a
    plain home (v.1.0.1, D26).
27. **The console `,` / `.` keys for `upVmax`.** Here: `.` had no ceiling. Removed
    (2026-09-25, D27).
28. **A band result kept only in RAM.** The next `applySettings()` zeroed it while the
    frame kept its shift; every limit captured after it was about 640 hs out. Replaced
    by the collector and the limit carry (2026-09-02, 5.2.15).
29. **A slip over 40 hs flagged and left alone.** A one-way ratchet: 394 hs out after
    16 h 40 min. Replaced by the re-index ladder (2026-09-08, D8).
30. **Crossing lag absorbed as drift**, from a crossing read that also took the edge
    log's print time into the recorded position. Replaced by one back-dated
    observation with the half-interval lag subtracted (2026-09-03, 2026-09-23, D7).
31. **Limits checked once per move** in the earliest bench firmware, keyed to the
    logical direction: with the motor's direction inverted, a move toward one stop
    consulted the other stop's switch and would have pushed through it (found
    2026-08-28, before it hurt). Replaced by `stepAllowed()` before every step, in
    the frame the inversion has already corrected.

## 5.7 Limits and firmware notes

**Seen working on the radio:** the portal Re-index (about 10 s, back to the same
position, 2026-09-08); the band-check re-index (2026-09-24); the routine idle check
(2026-09-24); the RTC memory after a portal reboot and after an upload (2026-09-25); the
hunting detector (2026-09-23/24).

**Seen working on the radio since v.1.0:** the RTC memory after a watchdog reset that
rolled an image back (2026-09-25, §4.2.15).

**Never exercised on the radio:** a slip over 40 hs found by a crossing (as opposed to
the band check) driving the ladder; the three-failure FAULT; any homing FAULT since the
budget fixes; the Re-index button on a needle with no zero (v.1.0.1); the RTC record
after a crash, and torn-record detection; the measured-ends turn choice after an
outage of more than half a turn; a portal stop or abort landing in the middle of a
band calibration (§12.2.8).

**Known limitations and notes:**

1. **Portal needle actions run on core 0**, writing needle state from the portal task
   while the needle task runs on core 1; only stop and abort are cross-core safe (§12.2.4).
2. **`gPos` is still written outside the emitter**: by `calFinishBand()` and
   `calStartBand()`, which break the ownership rule and could race the emitter, and by
   `useMicro()` (at rest), `assumeAtLowStop()` and `bootPosition()` (in `setup()`), and
   `coilRelease()` and `jogRaw()` (emitter parked), which are safe by construction. It
   is left as a firmware note (§12.2.1).
3. **FAULT has no local sign.** Only the portal and the console show it (D23).
4. **Homing runs with the soft limits disarmed** under a 2336 hs budget per direction. A
   wrong starting belief - a cold boot with the needle left high, a brownout, a torn
   record - can drive toward the far stop before it reverses. In practice: switch the
   amplifier off, so the needle parks, before unplugging the set.
5. **A second home during a home restarts it** from wherever the needle is, with a
   fresh budget.
6. **The portal answers "re-indexing" when Re-index runs a plain home**, and its
   refusal during homing or the sweep says "a calibration or a bring-up tool has the
   needle" (§12.2.5).
7. **The portal jog blocks the portal task** for the length of the jog, up to 20 s at
   the minimum `reapHsps` (§12.2.4).
8. **The gain-ceiling warning checks only the compiled defaults**, once at boot (§12.2.3).
9. **The boot home always ends in a full sweep**; `sweepOn` governs only the
   amplifier-on sweep.
10. **The soft-limit margin** (20 hs inside each end on this machine since 2026-09-08)
    is an open question: whether it is enough over days of amplifier on/off
    cycles. Before it, `posMin` left 19 hs between the park position and the low
    stop, and the needle drifted about 24 hs an hour. The feared loop - a park in a
    wrong frame lands past the stop, loses steps, and makes the next park worse - was
    an analysis, not an observation; the author corrected the claim that it ground
    every time. After the change: 2 hs of drift in 3 h 20 min, then 2 hs at 1 day 20 h
    with no re-index needed. Consistent with the loop being real and broken, not
    proof: nothing counted the amplifier switch-offs in those windows.
11. **`CORRECT_MAX_HS`, `IDX_DEBOUNCE_K`, the band margin, the homing budgets and the
    other values of §5.9 belong to this machine's mechanism.** A different magnet,
    switch position, travel or faster profile needs them re-measured and re-derived
    (5.8, 5.9).
12. **A settings write can land during a move's dwell or a portal jog** (the speed reads
    0 then) and make the jog stutter (§12.3.4).
13. **`HALFSTEPS_PER_REV`** is defined in `drive.h` and used nowhere (§12.2.7).
14. **The re-approach speed is not a setting** and has not been measured (5.2.9, §12.2.2).
15. **A stop during a home leaves the needle IDLE and unhomed.** It re-homes on the next
    `H`, `T`, `P`, Re-index, amplifier-on edge or upload resume.
16. **The `upVmax` row accepts up to 4000 hsps**, far past the 1100 this mechanism
    follows through the index (D3, §12.2.6).

## 5.8 Changing this area

### Invariants

1. `posMin ≤ 0 ≤ posMax` and `posMin < posMax`, always. `setGeometry()` enforces it; every
   writer goes through `applySettings()` and its write-back.
2. Only the emitter writes `gPos` in operation. Everyone else hands a correction through
   `gCorrectHs`. A new zero is not used until the emitter has folded it (`gZeroPending`).
3. The index pin is read for motion decisions only in `sampleIndex()`, once per pass.
4. Every homing phase has a constant budget derived from the mechanism, never from the
   soft limits.
5. `PROVISIONAL_LIMIT_HS` stays above about half the index band's width, so the post-home
   sweep crosses the band.
6. `CORRECT_MAX_HS` stays below half the forward/reverse ON offset.
7. The debounce must fit: `(K − 1) × 5 ms` above the glitch you reject, and `2 × K` polls
   inside the narrowest ON region at the fastest speed near the index. Raise `upVmax` or
   `dnVmax`, or change the magnet, and recompute K.
8. `wn < 4 × zeta × upAccel / upVmax`.
9. Arrival: tolerance one step unit; speed threshold scaling with √unit.
10. Never change the microstep division during a move; snap both position and target.
11. Keep the deadline pull-in in `stepTask()`.
12. At 1/32, speeds above about 1560 hsps are clamped by the 20 us step floor.
13. No flash writes while the needle moves (`writeSafe()`); uploads stop it.
14. Settings are append-only: bump `SETTINGS_VERSION` and add a migration case.
15. Change `MEM_MAGIC` whenever `NeedleMemory` changes.
16. The direction inversion lives only in `Drive::setInvert()`.
17. LEDC channels 2..5 are the stepper's, on both firmwares' numbering.
18. `assumeAtLowStop()` is a direction hint; nothing may read its position as true.
19. The index clue changes only through `believeIndexAt()` and `forgetIndexClue()`, and
    is cleared only when the frame is made or shown true.
20. Stop and abort stay requests; anything that sets a speed checks for a pending request.
21. A refusal returns a reason; never report success for a request that was dropped.
22. A re-index needs a zero. Anything that can reach `beginReindex()` on a needle that
    has none must go through `startReindex()`, which homes instead.
23. The physical values of §5.9 are this machine's. Change one only from a measurement,
    and re-derive every constant its row says depends on it.

### Traps

- "Up" and "down" in setting names are the two laws, not directions (5.2.6).
- `reapHsps` does not set the homing re-approach, whatever its name and its label
  "Measuring pass speed" suggest; it sets the band calibration's creep and the portal
  jog (5.2.9).
- `gCross` counts edges; `gMeas` counts edges a conclusion was drawn from.
- `gDrift` is history; `gDriftPending` means "still wrong".
- The hunting count is emitted steps, not half-steps.
- `pos` is published in whole half-steps; a smaller oscillation is invisible in it. Watch
  `hunt`, `jit` and the driver board's LEDs.
- A jog runs with the emitter suspended: its crossings are not scored or logged, and it
  does not satisfy the limit-capture guard.
- After a correct band calibration `idxOnFwd` is 0 and the other three are not.
- The S3 console is native USB and does not reset when a monitor attaches: anything
  printed only at boot is never seen. Use `s`.
- A stop is carried out up to 5 ms later; `state()` read right after `stop()` may still
  show the old state.
- With the provisional ±300 limits, the whole printed face maps onto 600 hs and the
  needle hugs the middle of the dial. That is not a failed calibration; capture the
  real limits.
- A home ends by sweeping away from the index, so the needle is not on the switch
  afterwards. To run the band calibration, jog it back into the band first (about
  40 hs above zero is well inside it on this machine) and watch the portal's live index
  field; the edge log prints nothing during a jog.
- A tuner end can be negative, and the "low" end can be the larger count: on
  2026-09-03 this set measured `calLow` 1024 and `calHigh` −1173 (a span of 2197),
  the count running down as frequency rose. `seatTurn()` takes the ends in either
  order.
- A gain over the ceiling (5.2.6) looks like a profile that does not move the motor.
- Two individually correct changes can be jointly worse than the bug. On 2026-09-02
  the first limit fix was caught in review: its purge fell back to the old
  ±967 defaults, and the newly working sweep would have driven the needle nearly
  300 hs into the high stop on its first power-up. Neither half showed it on its own.

### How to test a change

- **On the radio, with the console open:** `e` (every index edge with position and
  direction), `l` (the raw switch), `s` (position, target, state, drift, crossings,
  jitter, steps, speed, PWM, and the AS5600 residue that must stay constant), `r` (a
  repeating sweep, to watch `last drift` over many crossings), `Z` (clear the jitter
  counters), `D` (settings). On the portal: Re-index to exercise recovery, and the state
  fields `drift`, `driftpend`, `driftmeas`, `reidx`, `reidxtry`, `hunt`, `fault`, `jit`.
- **Motion and drive changes** were measured on a bare-motor bench rig before they
  reached the needle. That rig is retired (5.6, item 25). A new one needs only a motor,
  its driver board and an ESP32; tape a paper pointer to the shaft and make every test
  end where it began, so any pointer error is lost steps. Run every return leg at a
  rate already known to be safe, so the return can never be what lost the steps. The
  old rig's tests, worth rebuilding: a rate ladder (where it starts to buzz), an
  out-and-back at a fixed rate over many cycles (the real pull-out test), an unramped
  start (pull-in), an acceleration ladder, a many-turn run for the gear ratio, a
  thermal soak, and a backlash measurement (do that one early). Measure rates at the
  moving division: at 1/32 every rate reads about 8 % low because of the 20 us floor.
- **Build** both firmware environments with the pinned toolchain (`espressif32@7.0.1`)
  and check the build's exit code, not its output.
- **Flash** only a board you have positively identified, and not while someone is
  listening or watching the dial. Uploading through the portal is the normal route.
- **Review** needle changes adversarially: reviewers briefed to disprove, sign-off on the
  final diff, then a live boot with the console open. That practice caught most of the
  defects listed in 5.6.

## 5.9 Physical values of this machine, held by the firmware

No other device will be built exactly like this one, so this mechanism's physical values are
kept here, on the firmware side, rather than in the Hardware Bible.

The Hardware Bible records what is wired to what (Bible §0, §11) and how the mechanism
is built: the parts, how they are mounted, and what drives what (Bible §31.4). It does
not record how this particular mechanism behaves: which way the motor turns the
needle, how far the needle can travel, where the index magnet sits in that travel, how
fast the gearbox will follow. The firmware depends on all of these. They are gathered
here, because they are the values another builder will not share.

How to read the table:

- **Compiled** means the value is written into the source and changes only with a new
  build. **Setting** means it is a stored setting with a compiled default. **Measured**
  means the firmware measures it on the machine at run time and stores it; only its
  compiled default is this machine's.
- **Depends on it** names the firmware values derived from a physical value. Change the
  physical value and those must be re-derived (5.8).
- Positions are in half-steps (hs) from the homed zero, the index's forward ON edge
  (5.2.3); shaft angles are AS5600 counts, 4096 per turn (5.2.17).

### The motor and its drive

| Physical value | This machine | Where in code | Kind | How it was established | Another builder |
|---|---|---|---|---|---|
| Direction: advancing the motor's electrical phase moves the needle toward **low** FM | inverted | `gInvert = true` in src/s3/drive.cpp; `Drive::setInvert(true)` in `Needle::begin()` | compiled | Measured on this machine, and so marked in drive.cpp since the first commit (2026-09-01) | Run `u` (+200 hs): if the needle moves toward low FM with the inversion off, turn it on, and the other way round. Every position above the drive layer assumes + is toward high FM. |
| Coil pairing: IN1 and IN3 are the two halves of one phase, IN2 and IN4 of the other, in turning order IN1, IN2, IN3, IN4 | as stated | the half-step table and `PHASE[4] = {512, 0, 1536, 1024}` in `Drive::apply()`, src/s3/drive.cpp | compiled | In the code since the first commit; the needle has moved microstepped in both directions on it ever since | Hold each input with `1`..`4`, then step with `j`/`J` and `u`/`U`. A needle that buzzes, stalls or moves backward on some steps has another pairing: change the table, not the wiring. |
| The motor is a 5 V part, so full PWM duty on its supply (Bible §11) is safe | 5 V, 100 % duty | `Drive::setDutyPct(100)` in `Needle::begin()` | compiled | The motor fitted on this machine; the supply is the Bible's | If your motor's rated voltage is below its supply, lower the duty before anything moves. The bench rig worked out, but never soak-tested, the duty that gives the same coil current as 5 V continuous through this kind of driver: about 100 × 4.8 / (V − 0.2) %, so 41 % at 12 V. Check the temperature before trusting it. |
| The needle holds its place with the coils unpowered | holds | coil release 400 ms after every move, `stepTask()` | compiled | In use since the first commit; a needle that moved while released would show as drift at the next index crossing (5.2.12) | If your needle drifts or falls when released, remove the release (and accept the heat), or re-reference more often. |
| The motor and gearbox do not lose steps at the settled motion configuration | no lost steps | the whole open-loop position model | compiled | Measured by the author, 2026-08-30; a 30-minute index edge log on 2026-09-02 showed the trip points repeating within ±4 hs over dozens of crossings; steps were lost only when driven past a physical stop | Log `e` over a long `r` sweep: the index edges must repeat. If they wander, slow the profile before anything else. |
| The speed the mechanism follows going up through the index | 1100 hs/s never lost steps; 1200 to 1400 often did; 1500 always did | `upVmax` default 1100, `Settings` in src/s3/main.cpp | setting | The author's sweep of `upVmax`, 2026-09-24: at 1500 every power-up sweep landed about 400 hs (about 5 MHz) low | Sweep `upVmax` downward from high, watching `last drift` over repeated `r` sweeps, and keep a margin below the first failure. `dnVmax` (1700) governs only the park, and is the fastest the needle ever crosses the index. Depends on both: `IDX_DEBOUNCE_K` (below). |
| Gear ratio 64:1 (4096 hs per output turn) | 4096 | `HALFSTEPS_PER_REV`, src/s3/drive.h | compiled | Recorded as measured on this motor; **used nowhere**, because the firmware counts half-steps from the index and never needs the ratio | Nothing to do. Many motors of this family are about 63.68:1 rather than 64:1; the firmware does not care. |

The motion feel itself (`wn` 9, `zeta` 0.50, `upAccel` and `dnAccel` 18000, `riseMs`
100, `fallMs` 1000, the dwells, 1/16 moving and 1/32 tracking) is a matter of taste,
settled on a bench rig on 2026-08-30 (5.4, D2). It is not a physical
value, but it was judged on this motor and needle, and a lighter or heavier pointer
may want it re-tuned by eye.

### The travel, the index and the soft limits

| Physical value | This machine | Where in code | Kind | How it was established | Another builder |
|---|---|---|---|---|---|
| Travel between the two mechanical stops | 1869 hs (about 164 degrees of the motor's output shaft, at 0.088 degree per hs) | `HOME_BUDGET_HS = 2336` (1869 + 25 %), src/s3/needle.cpp | compiled | Measured on this machine before 2026-09-03; first used on 2026-09-03 | Jog from stop to stop and read `pos`. Depends on it: `HOME_BUDGET_HS`, `MEM_POS_SANE_HS` (6000: the travel plus a whole search, with room). |
| Where the index sits in the travel | 1195 hs above the low stop, 674 hs below the high stop; near 102.5 MHz on the printed dial, so listening below that never crosses it | the homing comments and budgets, src/s3/needle.cpp | compiled | Measured with the travel | Read `pos` at each stop after a home. Depends on it: the backoff and re-approach budgets (each runs one way, and neither may reach a stop), `PROVISIONAL_LIMIT_HS`. The index must be mid-travel for anything in this chapter to work. |
| The index band: the four switching edges | onFwd 0, offFwd 97, onRev 79, offRev −15 (2026-09-03) | `idxOffFwd`, `idxOnRev`, `idxOffRev` settings; `idxOnFwd` 0 by definition | measured (band calibration, 5.2.15); compiled defaults 0 | The band calibration on this machine, 2026-09-03 | Run the band calibration (portal "Calibrate the index", console `k`) with the needle on the switch. Nothing needs re-deriving by hand except the constants in the next three rows. |
| Offset between the ON edges seen moving + and moving − | about 79 hs (earlier estimates said 67) | `CORRECT_MAX_HS = 40`, src/s3/needle.cpp | compiled | From the band calibration above | Keep `CORRECT_MAX_HS` below half your offset, so a crossing read in the wrong direction is never absorbed (5.2.12). |
| Narrowest ON region (reverse ON to reverse OFF) | 94 hs | `IDX_DEBOUNCE_K = 4`, src/s3/needle.cpp | compiled | From the band calibration above, crossed at 1700 hs/s (the park): about 55 ms, 11 polls | Re-derive K: `(K − 1) × 5 ms` above the glitch you reject, and `2 × K` polls inside the narrowest region at your fastest speed near the index (5.2.8). |
| How far outside the band the switch may still read ON | 30 hs plus 40 ms of travel | `BAND_MARGIN_HS = 30`, src/s3/needle.cpp | compiled | Chosen from the debounce lag at speed plus room (2026-09-24) | Re-check against your band and speeds; too small raises false slips, too large misses real ones. |
| Homing hops and budgets | backoff hops 20 hs, clearance 60 hs, budget 400; re-approach hops 10 hs, budget 300 | `homingTick()`, `HOME_BACKOFF_BUDGET_HS`, `HOME_REAPPROACH_BUDGET_HS` | compiled | Derived from the band and the index position above (about twice and 2.4 times the worst case) | Re-derive from your band width; each budget must stay shorter than the distance to the stop it faces. |
| Provisional soft limits | ±300 hs | `PROVISIONAL_LIMIT_HS`, src/s3/main.cpp; `posMin`/`posMax` defaults | compiled default of a measured pair | Chosen under the travel on both sides (1195 and 674) and above half the band, so the post-home sweep crosses the band (2026-09-03) | Keep it below the shorter side of your travel and above half your band width. |
| The soft limits themselves | −1156 / +414 hs (September 2026) | `posMin`, `posMax` | measured (captured with `m`/`M` or the portal, 5.2.16) | Captured on this machine; moved 20 hs inward from −1176 / +434 on 2026-09-08 | Capture your own, after a home, a band calibration and one scored crossing. |
| The needle is at the low stop after a power-up | low stop | `assumeAtLowStop()` in `bootPosition()`, src/s3/needle.cpp | compiled | A design assumption since 2026-08-31: the needle is at the lowest point at every Off; true because the needle parks at `posMin` when the amplifier goes off and holds unpowered | Only a direction hint: if your set is often unplugged while playing, the first search may go the wrong way first, within its budget. |
| A software reset leaves the rotor where it was, to within a detent | yes | the RTC memory record, `memRecord()` / `bootPosition()` | compiled | Seen 2026-09-25: after a portal reboot while tracking, "remembered at 301" and back to 301 after homing; also across an upload and a watchdog rollback | Nothing to do unless your driver moves the rotor at reset. |

### The tuner and the dial

| Physical value | This machine | Where in code | Kind | How it was established | Another builder |
|---|---|---|---|---|---|
| The tuner's whole travel lies inside one shaft turn | about 2200 counts of 4096 (2197 measured 2026-09-03), for about 20 MHz | `seatTurn()`, src/s3/needle.cpp; `SMP_MAX_DRIFT = 20` (about 0.2 MHz), src/s3/main.cpp (chapter 6) | compiled; checked at run time | The tuner ends measured on this machine; relied on since 2026-09-25 | Measure both tuner ends (`c`/`C`, or "Tuner = low/high end"). If the span is a whole turn or more, the firmware falls back to the nearest turn, which is right only for movement under half a turn. |
| The tuner's ends repeat | within 300 counts | `SEAT_MARGIN = 300`, src/s3/needle.cpp | compiled | A margin chosen with the rule above, not a repeatability measurement | Keep the measured span plus twice the margin under 4096. |
| The tuner's ends | measured values in `calLow`/`calHigh`; compiled placeholders 0 / 6023 | `Settings` in src/s3/main.cpp; `calHigh = 6023` also in src/s3/needle.cpp | measured; the compiled pair is only a placeholder | 6023 is left from an older belief that the shaft turned about 1.47 times; the measured span is about 2200 (2197 on 2026-09-03, with `calLow` 1024 and `calHigh` −1173: the count ran down as frequency rose, which is allowed). The placeholder span is wider than a turn, so it also switches off the measured-ends turn choice until real ends are stored | Measure both ends. Nothing reads the placeholder as a measurement (the "measured" bits say which, chapter 7). |
| What the tuner reaches before any calibration | 88.1 MHz at the low end, 107.9 MHz at the high end | `bandLow` 881, `bandHigh` 1079, `Settings` | compiled defaults | In the code since the first commit; with the tuner ends they give the stored straight line used until marks exist (chapter 6) | Calibrate the tuning curve (chapter 6). |
| The frequencies printed at the two needle stops | 87.9 MHz low, 107.9 MHz high | `dialLow` 879, `dialHigh` 1079, `Settings` | setting | Read off this set's dial glass | Type in what your glass prints at each end. |

**What another builder does first**, in order: set the direction; check the coil
pairing and the duty; find the stall speed and set `upVmax` below it; jog to both stops
and read the travel and the index position; re-derive the budgets, `CORRECT_MAX_HS`,
`IDX_DEBOUNCE_K` and `PROVISIONAL_LIMIT_HS` if yours differ, and rebuild; home; run the
band calibration; capture the soft limits; measure the tuner ends; type in the printed
dial; then calibrate the tuning curve (chapter 6).

---

# 6. Tuning and the dial's self-calibration

> **Specific to this build — adapt.** The tuning chain is fitted to this LLOYDS TM-838N: its dial,
> its IF and its local oscillator. A reader's set will differ; the method carries over.

## 6.1 What it does

The listener tunes the radio with its original knob. That knob turns the tube set's own tuning
capacitor, exactly as it did in the 1960s (Bible §31.4). The machine reads the capacitor's shaft angle with a
magnetic angle sensor (an AS5600) and turns that angle into a frequency in megahertz (MHz). That
frequency drives three things: the needle on the glass dial, the four-digit Leditron panel (when it
is set to show tuning), and the "Now" readout of the web portal. The angle-to-frequency relation is
not a straight line, is not known in advance, and can drift. So the machine measures it itself. A
small FM receiver chip inside the cabinet (an RDA5807M) listens for the tube set's own local
oscillator, works out from it which frequency the set is really tuned to, and stores the pair
"(shaft angle, frequency)" as a calibration sample. A curve fitted through all the samples is the
mapping. So the AS5600 is the live source of every frequency shown and every position the needle
tracks to, and the RDA5807M, which hears the oscillator while the firmware adds the IF, only
calibrates the curve the AS5600's angle is read through; it never replaces the AS5600, and while
the AS5600 is silent the needle and the readouts do not follow the knob (§2.1.1, §5.1). The user
can take a measurement by pressing *Measure this dial position* in the portal;
the machine also takes one by itself, at most every five minutes, while the radio is playing and the
knob is still. Nine slots hold these measured samples. A measurement near an existing sample replaces
it, so the positions the listener uses keep being re-measured; once all nine are full, a new
position replaces the most crowded sample, so the nine stay spread across the dial. The user can
also type up to three stations they know by ear ("hand marks"), slide the whole curve by hand in
0.1 MHz steps (the "dial correction"), reset that correction to zero, and remove any single sample.
Nothing automatic ever changes a hand mark or the dial correction.

## 6.2 How it works

### 6.2.1 Radio terms, for a newcomer

**Superheterodyne ("superhet").** The tube set does not amplify the station it receives directly.
It generates a signal of its own, the **local oscillator (LO)**, and mixes the incoming station with
it. The mixer produces the difference between the two frequencies. That difference is fixed by
design and is called the **intermediate frequency (IF)**. All the set's filtering and amplification
happen at the IF. Turning the knob retunes the LO; the IF never moves. So at any knob position the
LO frequency and the station frequency differ by exactly the IF.

**Injection side.** The LO can sit above the station ("high-side") or below it ("low-side"). The
firmware computes **station = LO + IF**, that is, low-side injection: the tube set's FM oscillator
runs below the station it receives. This was found on the bench on 2026-09-04 (6.4.2), and the Bible
records it (Bible §14, §22).

**The IF value.** The tube set's fitted FM IF is 10.6 MHz (Bible §15, §22), and the firmware's
default IF is 10.60 MHz. It is a setting, not a constant (see 6.3 and decision 6.4.3).

**Why the LO can be heard.** A set of this age has nothing that stops its oscillator from radiating a
little. A receiver a few centimetres away hears it as a bare carrier: a signal with no programme on
it. Where the RDA5807M and its antenna wire sit is hardware (Bible §11).

**Worked example.** The set is tuned to a station at 98.5 MHz. With an IF of 10.60 MHz the LO runs at
98.5 − 10.6 = 87.9 MHz. The RDA5807M, sweeping around 87.9 MHz, finds a carrier standing out there.
The firmware adds the IF back: 87.9 + 10.6 = 98.5 MHz. It reads the shaft angle at the same moment
and stores (angle, 98.5 MHz) as a sample.

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
frequency it is tuned to, 0 to 127 (bits 15:9 of its register 0x0B).

**Spur, fixed feature, notch.** A **spur** is a signal the RDA hears that is *not* the tube set's
oscillator: interference produced inside the cabinet itself. It stays at the same frequency whatever
the knob does. The firmware calls a learned spur a **fixed feature**. Around each one it cuts a
**notch**: the search is not allowed to choose any frequency within ±0.2 MHz of a fixed feature. On
this radio the spurs form a **comb**: seven peaks at 79.0, 81.8, 84.6, 87.4, 90.3, 93.1 and 95.9 MHz,
2.7 to 2.8 MHz apart, present with the set switched on and off. One of them (79.0) reads louder than
the oscillator. Their source is unidentified (6.7).

**Stereo pilot.** An FM broadcast in stereo carries a 19 kHz "pilot" tone. A bare oscillator can never
carry one. So when the RDA5807M's stereo indicator (register 0x0A bit 10) is set on a strong signal,
that signal is a broadcast and not the LO. The indicator can only *reject*: a mono or weak broadcast
reads the same as a bare carrier.

**Floor, local floor, margin.** The **floor** of a sweep is the median RSSI of all its points. The
**local floor** of one point is the median of up to 16 neighbours within ±0.8 MHz of it. The
**margin** of a point is its RSSI minus its local floor. The firmware judges a peak by its margin,
never by its absolute height.

### 6.2.2 Units

Several units are mixed on purpose. Variable names carry them.

| Name ending | Unit | Example |
|---|---|---|
| `f10`, `...10` | tenths of a MHz | 985 = 98.5 MHz |
| `f20`, `...20` | twentieths of a MHz (50 kHz steps) | 1971 = 98.55 MHz; IF 212 = 10.60 MHz |
| `acc`, `tuneP[]`, `calLow`, `calHigh` | AS5600 **accumulated** counts: 4096 per shaft turn, counted across turns | |
| permille | 0 to 1000 across `calLow`..`calHigh` | shown by the console `s` only |

On this radio the accumulated count falls as the frequency rises. The firmware never relies on the
direction: every test compares magnitudes or signs of slopes, never order.

### 6.2.3 Tasks

| Work | Runs in | Priority / core | Rate |
|---|---|---|---|
| AS5600 angle read, `readAs5600()` in src/s3/needle.cpp | `needleTask` ("needle") | 4 / core 1 | every 20 ms |
| RDA sweeps and refinements, `rdaTask` in src/s3/rda.cpp (stack 3072 bytes) | its own task ("rda") | 2 / core 1 | polls for a request every 100 ms; each point takes at least 300 ms |
| `sampleTick()`, `autoSampleTick()`, console keys, `lastAngle` save | Arduino `loop()` | 1 / core 1 | every loop pass; `lastAngle` checked every 10 s |
| Portal actions (`rda.sample`, `tune.*`, settings writes) | portal task ("portal") | 3 / core 0 | on request |

`rdaTask` outranks `loop()` on the same core. A path in `rdaTask` that never blocks starves
`loop()`, and a `loop()` starved for more than 2 s mutes the audio (the A32 link's silence
timeout). So every path in `rdaTask` blocks: 300 ms per measured point, 10 ms per failed point
(`pointFailed()`), 100 ms while idle. `loop()` is also under a 15 s task watchdog (chapter 4): a
`loop()` held up that long restarts the S3. `rdaTask` itself is not watched.

### 6.2.4 From shaft angle to megahertz: the tuning chain

The whole chain, in order:

```
 knob -> tuning capacitor shaft -> AS5600 raw angle (0..4095)
      -> accAngle (multi-turn count)            readAs5600(), seatTurn()
      -> curve  f = a + b(acc-x0) + c(acc-x0)^2  tuneFreq10Raw()
      -> + hand offset tuneOffset10
      -> MHz  -> needle, Leditron, portal, sampler prediction
```

1. **Read.** `readAs5600()` reads the 12-bit raw angle from the AS5600 at I2C address 0x36, on the
   S3's `Wire` bus (Bible §11, §24). I2C is the two-wire bus the S3 uses to talk to the sensor; the
   firmware runs it at 100 kHz (`S3_I2C_HZ` in include/pins.h), not 400: slower edges radiate less,
   and a hand-turned knob needs no speed. The difference from the previous read is wrapped to ±2048
   counts and added to `accAngle`, the multi-turn count. The firmware only ever *reads* the sensor
   (raw angle, status, AGC, magnitude). It writes none of its configuration registers and never
   uses its one-time-programmable range setting (which can be burned once only, ever): the sensor
   runs at its power-on settings and all scaling is done in firmware. The full 4096 counts per turn
   are plenty: the tuner's roughly 2200 counts across 20 MHz are about 10 kHz per count, against
   channels 200 kHz apart.
2. **Which turn.** The AS5600 is absolute within one turn only. At boot and after a bus outage the
   firmware must choose the turn. `seatTurn()` does it. If both tuner ends were measured
   (`tunerEndsSet == 3`) and the span between them plus 300 counts each side (`SEAT_MARGIN`) is
   narrower than one turn, it picks the one turn that puts the angle inside that window. Otherwise
   it picks the turn nearest a reference: the saved `lastAngle` at boot, the last count after an
   outage. `loop()` saves `lastAngle` every 10 s, only while the encoder answers and only when it
   changed.
3. **Outages.** A failed read calls `busFail()`: the sensor is left alone for 2 s, the outage counter
   (`encoderGaps()`) goes up once, and `i2cOk()` turns false. On the first good read the angle is
   re-seated with `seatTurn()`, not dropped. A move hidden by the outage counts as tuning. `i2cOk()`
   turns true only after the re-seat.
4. **"The knob is being turned."** `tuningActive()` is true for 400 ms after the last detected
   movement. A movement is detected when one 20 ms step reaches 12 counts (`TUNE_DEADBAND`), or when
   steps in the same direction add up to 12. A reversal resets the sum, so sensor jitter never
   trips it.
5. **The curve.** `tuneFreq10Raw(acc)` clamps `acc` to the fitted domain, evaluates the quadratic in
   tenths of a MHz, and adds the hand offset `tuneOffset10`. A result that is not a number, or lies
   outside 10 to 300 MHz, returns 88.1 MHz plus the offset. `tuneFreq10()` rounds to the nearest
   tenth; `tuneFreq10f()` does not round. These three functions are the only place in the firmware
   that turns a shaft angle into a frequency.
6. The needle follows a filtered copy of the angle (chapter 5). The sampler, the
   readouts and `tuningActive()` all use the raw `accAngle`.

**Where the frequency is shown.**

| Surface | What it shows |
|---|---|
| Leditron, `showTuning` = 1 ("Tuning always") | `tuneFreq10f()` rounded to the nearest tenth, clamped to `dialLow`..`dialHigh`. Shown whatever the amp and source. The panel has no decimal point: 1017 means 101.7 MHz. |
| Leditron, `showTuning` = 2 ("Tuning while tuning") | Only while the amp is on and the source is RADIO, and for `tuneHoldMs` after the last movement. `tuneFreq10f()` clamped to the printed face, then snapped to the nearest odd tenth (87.9, 88.1, ... the grid of FM channels in the Americas), never off the face. `updateDisplay()` in src/s3/main.cpp. |
| Portal "Now" (`tune` in the state JSON) | `tuneFreq10()` to a tenth, **not** clamped, with "(past the printed face)" appended outside `dialLow`..`dialHigh`. Shown whenever the encoder answers. `portalStateJson()`. |
| Portal Needle tab | *last measurement* (the sampler's last sentence, or "measuring N % ..."), *tuning curve* (count, model, hand offset), *tuner reaches* (the curve's frequencies at the two ends of its domain). |

### 6.2.5 The fit

`fitTuneCurve()` in src/s3/main.cpp computes the curve. `applySettings()` calls it, so every change
to any setting refits the curve, and a stored sample and the running curve can never disagree.

- **Inputs.** Every used slot of `cfg.tuneP[12]` (shaft counts) and `cfg.tuneF[12]` (tenths of a
  MHz), selected by the bit mask `cfg.tuneUsed`. Slots 0 to 2 are the hand marks A, B and C; slots 3
  to 11 are RDA samples. The fit treats them alike.
- **Fewer than 2 samples: the stored straight line.** `installStoredLine()` draws a line from
  `bandLow` (88.1 MHz) at `calLow` to `bandHigh` (107.9 MHz) at `calHigh`. Until the tuner ends are
  measured, `calLow`/`calHigh` hold the old compiled placeholders 0 and 6023; these, and the 88.1 to
  107.9 MHz reach, are physical values of this machine, held by the firmware (§5.9).
- **The domain** (the range of shaft counts the curve is valid over). If both tuner ends are
  measured, it is exactly `calLow`..`calHigh`, widened only to include any sample outside it. If
  not, it is the samples' span padded by 30 % on each side (at least 100 counts), and the model
  sentence says "tuner ends NOT measured".
- **Method.** Least squares: the curve that minimises the sum of the squared errors at the samples.
  The counts are centred on their mean first, for numerical accuracy. The normal equations are
  solved by Gaussian elimination with partial pivoting (`solveLS()`).
- **3 or more samples: a quadratic**, accepted only if its slope has the same sign at both ends of
  the domain (a tuner never turns back). Otherwise, or if the system is singular, a
  **least-squares straight line**, and the model sentence says "QUADRATIC REFUSED: turns back on
  itself" or "the marks are degenerate". **2 samples: a least-squares line.**
- **Guards before the curve is used.** At both domain ends the curve (without the hand offset) must
  lie between 50.0 and 200.0 MHz, and the two ends must be at least 2.0 MHz apart. If either test
  fails the stored straight line is installed and the model sentence reads "MARKS REFUSED (...)".
- **Reporting.** The model actually running is recorded, never inferred from the sample count:
  `tuneModelName()`, `fitRefused()`, `fitDemoted()`. `tuneStateLine()` builds the one sentence
  ("N marks, MODEL, reaches X - Y MHz") that the console, the portal card and every tuning message
  print. `tuneTroubleNote()` (src/s3/settings_table.h) returns that sentence only when there is
  something wrong to say (the fit refused or demoted, or a tuner end not measured), and an empty
  string otherwise. When `calLow` or `calHigh` is typed into its portal row, `/api/set` adds it to
  its answer as `warn`: before, a single field edit could demote the curve and throw three marks
  away behind a green "ok".

### 6.2.6 The RDA5807M instrument (src/s3/rda.cpp)

**Set-up, `initChip()`**, called by `begin()` at boot and by `reprobe()`. Reset and enable, then:
register 0x02 = DHIZ, DMUTE, NEW_METHOD, ENABLE with **MONO cleared** (the stereo indicator needs
the stereo decoder running); 0x04 = **AFC disabled** (automatic frequency control would let the chip
slide toward a louder neighbour); 0x03 = band 2 (76 to 108 MHz), 100 kHz spacing; 0x05 = LNA on both
ports, LNA current 2.7 mA, volume 0. NEW_METHOD is a sensitivity mode of the chip, and the bench
test ran with it. The two LNA (low-noise amplifier) settings are the part's most sensitive: the
bench test ran with the defaults (the lowest current, 1.8 mA, and a single input port) and still
read the oscillator at 59 against a floor of 44, but inside the cabinet that margin might not
survive, and these cost nothing. Dual-port input works whichever way the module's antenna pad is
routed. Volume 0 because this is an instrument, not a radio. Every configuring write must succeed
and a final read of 0x0A must answer, or the part is reported absent. A re-probe runs the whole
set-up again, not just a check for an answer: a part that came back may have browned out and
reset to its power-on defaults, AFC on among them. The RDA5807M sits on the S3's second I2C bus, `Wire1`
(Bible §11), run by the firmware at 100 kHz. The fitted part answers at address 0x11 (Bible §24).
The firmware addresses it there only (`ADDR_RANDOM`), and every transfer names the register first
(`wr()` and `rd()` in src/s3/rda.cpp).

**One point.** Write the channel: `tuneTo(f10)` on the 100 kHz grid (channel = f10 − 760) or
`tuneTo20(f20)` on the 50 kHz grid (channel = f20 − 1520). Poll the "tune complete" flag (STC,
0x0A bit 14) for up to 60 ms. Wait 300 ms: the **dwell**. `rdaTask` enforces 300 ms whatever the
caller asks, because the RSSI reading needs about 300 ms to settle after a tune: measured here on the
bench on 2026-09-04, where a 40 ms dwell hid the oscillator. Read RSSI and the stereo flag. The
stereo flag is read only after the tune has completed: its power-on value is 1, so an early read
says "stereo" for everything. A failed read is a failure, never a zero, and never "mono" either (a
failed stereo read once let a broadcast pass as the oscillator without counting as a bus failure).

**A sweep, `requestSweep(lo10, hi10, step10, dwellMs)`.** Non-blocking: it posts a request and
`rdaTask` does the work. The range is clamped to the **LO window, 77.2 to 97.2 MHz**
(`Rda::LO_WINDOW_LO10` / `LO_WINDOW_HI10`, compiled constants). It is refused if a run is already
in flight or requested, if the part is absent and a re-probe fails, or if nothing is left after the
clamp. Every point is recorded as a "bin"; a failed point is recorded as RSSI 0 and counted. At the
end the sweep's floor is the median of all its bins. A narrow sweep (±1.5 MHz, 31 bins) takes about
10 s; the whole window (201 bins) about 60 to 70 s. The bins stay readable until the **next sweep**
starts: only a sweep clears them (and the refinement's points with them), so after a measurement both
its coarse half and its fine half can be inspected (2026-09-25).

**A refinement, `requestRefine(centre10)`.** Eleven points on the 50 kHz grid, ±0.25 MHz around the
coarse candidate, about 3.5 s. It skips any point within ±0.2 MHz of a fixed feature (`isSpur20()`)
and any point whose stereo flag is set above the last sweep's floor + 3. It keeps the **highest**
reading: the coarse pass has already decided this is the oscillator, and a local floor over eleven
adjacent points would measure the peak against itself. If every point was skipped or read 0,
`fineF20()` is 0 ("found nothing"). The refinement is not clamped to the LO window, so it can reach
0.25 MHz past either end. It shares the sweep's single request slot, so only one run is ever in flight.
Its eleven points are kept beside the coarse bins (`fineCount()`, `finePointF20()`,
`finePointRssi()`): a point skipped as a fixed feature, or whose read failed, keeps RSSI 0; a point
skipped as a broadcast keeps the RSSI it read. A refinement that loses the part keeps no points.

**The candidate, `bestCandidate(&margin)`.** Over the last sweep's bins, skipping any bin within
±0.2 MHz of a fixed feature (`isSpur()`) and any bin whose stereo flag is set above floor + 3, it
returns the bin with the largest margin over its local floor.

**Losing the part.** Five failed points in a row (`LOST_AFTER`) end the run. `present()` turns
false, `lost()` turns true, and the run publishes nothing (no bins, floor 0). `reprobe()` tries the
part again, at most once every 10 s (`PROBE_GAP_MS`), never while a run holds the bus, and runs the
full `initChip()` if it answers. `requestSweep()`, `requestRefine()`, `sampleStart()` and
`autoSampleTick()` all call it. `runFailures()` counts every failed transfer of the last run,
consecutive or not.

**Fixed features.** The list lives in the settings (`cfg.spur[16]`, `cfg.spurUsed`).
`applySettings()` pushes it to the RDA module with `Rda::spurSet()`; the module only holds a copy.
`rdaTask` may be reading that copy (during a refinement) while `applySettings()` replaces it, so
`spurSet()` publishes the count last when the list grows and first when it shrinks: a reader may
see a mix of old and new entries, but never reads past the entries written.
The list is learned by hand, with the console key `o`: it marks the current best candidate of the
last sweep as a fixed feature and saves the list. The procedure is: the set powered but switched
**off** (so the oscillator is not running), a whole-window sweep (`q`), then `o` once per peak. With
no oscillator, every peak is a spur. The list is cleared by the console `O` or the portal's *Forget
the fixed features* (`rda.spurClear`); the clear is saved too. The default list is empty.

### 6.2.7 One measurement, stage by stage

The sampler in src/s3/main.cpp is a small state machine:

```
 SMP_IDLE --sampleStart()--> SMP_NARROW --found--> SMP_FINE --> store --> SMP_IDLE
                                 |                     ^
                                 +--nothing--> SMP_WIDE +--found
                                                   |
                                                   +--nothing--> "NOTHING FOUND" --> SMP_IDLE
```

`sampleStart(ritual)` begins a measurement. `ritual` is true when a person asked (console `V`,
portal `rda.sample`) and false for the automatic trigger. `sampleTick()`, called from `loop()`,
advances the stage once `Rda::sweeping()` is false.

**Gates in `sampleStart()`, in order.** Each refusal says why.
1. No measurement already running.
2. The RDA is present, or answers a re-probe. The message tells "stopped answering after boot" from
   "not answering".
3. The RDA is not sweeping.
4. `radioLive()`: the amp is on **and** the A32 reports the source RADIO. This is a **listening
   policy**, not a statement about the oscillator: the source selector only tells the A32 which sound
   to play, and the tube set runs whenever the amp does (Bible §5, §21). The dial calibrates (and the
   needle follows, chapter 5) only while the radio is what you are listening to, by choice
   (6.4.18). The refusal reads "the dial calibrates only while you listen to the radio - switch the
   amp on and select RADIO".
5. The encoder answers (`i2cOk()`).
6. The predicted LO (`tuneFreq10() − ifOff10()`) is such that ±1.5 MHz around it touches the LO
   window (`loInWindow()`).
7. The narrow sweep request is accepted.

Only then does it record the shaft count (`gSmpAcc`), the encoder outage count (`gSmpGaps`) and the
stage. The request comes first and the stage second; the other order was a race between the portal
task and `loop()` (6.6).

**Checks in `sampleTick()` at every stage, before anything is kept.**
1. The shaft has not moved more than 20 counts since the start (`SMP_MAX_DRIFT`), or ABANDONED
   "hold it still". On this tuner 20 counts is about 0.2 MHz (about 2197 counts for about 20 MHz), a
   physical value of this machine, held by the firmware (§5.9).
2. `radioLive()` is still true, or DISCARDED.
3. No I2C failure during the run (`runFailures() == 0`), unless the part was lost (next checks), or
   DISCARDED: "press measure again" (on request) or "it will retry by itself" (automatic).
4. In the FINE stage: part lost → ABANDONED. Otherwise store `fineF20()`, or the coarse answer if the
   refinement found nothing.
5. Part lost in a sweep → ABANDONED, naming the connector, "not a fixed feature".
6. `bestCandidate()`. Accepted if the margin is at least **8** (on request) or **14**
   (automatic). Then the **notch guard**: if the candidate is exactly 0.3 MHz from a fixed feature
   (`besideSpur()`), it looks at the bin one step toward the feature, inside the notch. If that bin
   is at least as strong as the candidate, or was not swept, the measurement is REFUSED: "Move the
   dial 0.3 MHz and retry". Otherwise `requestRefine()` and stage FINE ("pinning it down at 50 kHz
   (about 4 s)"); if the refinement is refused, the coarse answer is stored at once.
7. Nothing good enough after the narrow sweep: one whole-window sweep, stage WIDE.
8. Nothing good enough after the wide sweep: "NOTHING FOUND in the whole window ... nudge it and
   retry".

**Storing, `sampleStore(lo20, margin)`.**
1. The encoder answers now and had no outage since the start (`encoderGaps()` unchanged), or
   DISCARDED.
2. `station20 = lo20 + cfg.ifOffset20`, then `station10 = (station20 + 1) / 2`: the IF is added in
   twentieths, then the result is rounded to tenths (a half rounds up).
3. **Disagreement gate**, only once at least 2 samples exist (`SMP_TRUST_AFTER`, hand marks
   included): refused if the answer is more than 4.0 MHz (`SMP_MAX_DISAGREE10`) from what the curve
   says at the current shaft position.
4. Refused if the answer is outside the printed face, `dialLow`..`dialHigh`.
5. **Slot choice.** The first used slot within 60 counts of the start position is "this position".
   If it is a hand mark (0 to 2), nothing is stored ("hand mark X covers this position"). If it is an
   automatic slot, it is **refreshed**. Otherwise the first free slot from 3 to 11 is used.
6. **A full table keeps the samples spread** (v.1.0.3, 2026-09-26). If slots 3 to 11 are all in use
   and none is within 60 counts, the firmware looks for the **most crowded automatic sample**. For
   each of slots 3 to 11 it takes the distance, in shaft counts, to that sample's nearest neighbour.
   The neighbours are every other stored sample, the hand marks included, **and the new point**. The
   slot with the smallest such distance is the most crowded; on a tie, the lowest slot wins. Then:
   - if the new point's own nearest neighbour (among the stored samples) is **closer** than that
     smallest distance, the new point is itself the most crowded and would add the least. Nothing is
     stored: "measured f, not stored: every sample slot is full and this position would add the
     least coverage";
   - otherwise the new point **replaces** the most crowded sample, in its slot.
   Hand marks are never replaced, but they count as neighbours.
7. **Nothing changed, nothing written.** A refresh that gives the same tenth with less than 8 counts
   of movement writes nothing, and says so: "slot 3 still reads 98.6 - nothing changed, nothing
   written".
8. Write `tuneP`, `tuneF`, `tuneUsed`. The hand offset `tuneOffset10` is **never** touched by a
   sample. Then `applySettings()` (the refit) and `settingsTouch()` (a delayed save to flash).
9. Report: "MEASURED (or RE-MEASURED) f MHz (LO x, +m over local) -> slot n; k stored; curve: model".
   A replacement reads "MEASURED (replacing the most crowded sample)", and the console adds which
   frequency it replaced. Every accepted sample reports its evidence (the oscillator frequency and
   the margin), on the console and in the portal. The design's first rule was that a correction
   the machine makes must be kept and must be provable from outside: this project had earlier lost
   a needle measurement that was taken correctly and then thrown away.

**Why the two rules together.** The automatic sampler measures where the knob rests, and the knob
rests where the listener listens. The 60-count refresh therefore keeps re-measuring the positions
actually used, which is where drift matters. The replacement rule deals with the other case, a tenth
listening position once all nine slots are taken: it gives up the sample that adds the least
coverage, so the nine automatic samples keep spanning as much of the dial as they can.

**Worked example of a replacement.** Hand marks sit at counts 100, 900 and 1700; automatic samples
at 200, 300, 500, 700, 1100, 1300, 1500, 1900 and 2100 fill slots 3 to 11, in that order. A
measurement at count 1000
finds nothing within 60 counts. Nearest neighbours: 200 → 100 (100 counts), 300 → 200 (100), 500 →
300 (200), 700 → 900 (200), 1100 → 1000, the new point (100), 1300 → 1100 (200), 1500 → 1300 (200),
1900 → 1700 (200), 2100 → 1900 (200). The smallest distance is 100, first reached by the sample at
200 (slot 3). The new point's own nearest neighbour is 900 or 1100, 100 counts away: not closer than
100, so slot 3 gives up its sample at 200 to the new point at 1000.

Every outcome goes through `smpSay()`: printed on the console and kept in `gSmpNote`, which the
portal shows as *last measurement*.

### 6.2.8 The automatic trigger

`autoSampleTick()`, every `loop()` pass, starts `sampleStart(false)` when all of these hold:
the knob has been still (`tuningActive()` false) for 8 s (`AUTO_STILL_MS`); no measurement is
running; 5 minutes have passed since the last automatic *attempt*, successful or not
(`AUTO_GAP_MS`; none is needed for the first attempt after boot); `radioLive()`; the encoder answers;
the RDA is present (one re-probe allowed) and not sweeping; the predicted LO passes `loInWindow()`.
It does not look at what the needle is doing.

### 6.2.9 A real measurement, worked through

On 2026-09-25 at 06:47 the radio was playing with the needle tracking 105.8 MHz, and the automatic
sampler reported: "RE-MEASURED 106.1 MHz (LO 95.50, +36 over local) -> slot 4; 6 stored; curve
QUADRATIC". Here is what the code did to get there. The log gives the start and the result; the
middle steps follow from the code, taking the learned fixed features to be the seven-member comb.

1. **Trigger.** Knob still for 8 s, 5 minutes since the last attempt, RADIO playing.
2. **Prediction.** The curve says 105.8 (1058 tenths). `ifOff10()` is (212 + 1) / 2 = 106. Predicted
   LO = 1058 − 106 = 952, that is 95.2 MHz. `loInWindow(952)`: 93.7 to 96.7 overlaps 77.2 to 97.2.
3. **Narrow sweep**, 93.7 to 96.7 MHz, 31 bins. The 95.9 fixed feature notches 95.7 to 96.1 out of
   the search.
4. **Candidate.** The bin at 95.5 stands 36 above its local floor. 36 ≥ 14 (automatic): accepted.
   It is 0.4 MHz from 95.9, not 0.3, so the notch guard does not apply.
5. **Refinement** around 95.5: 95.25 to 95.75 in 50 kHz steps. 95.70 and 95.75 lie within 0.2 MHz
   of 95.9 and are skipped. The highest reading is at 95.50 (1910 twentieths).
6. **Station.** 1910 + 212 = 2122 twentieths = 106.10 MHz; (2122 + 1) / 2 = 1061 → **106.1 MHz**.
7. **Gates.** |106.1 − 105.8| = 0.3 MHz ≤ 4.0. Inside 87.9 to 107.9. An automatic slot (4) lies
   within 60 counts: refresh. The value or the position changed, so it is written.
8. The hand offset is left alone (no sample ever touches it). Refit: still a quadratic.

**The rounding.** Had the refinement found LO 95.05 MHz (1901), the station would be 1901 + 212 =
2113 twentieths, 105.65 MHz, stored as (2113 + 1) / 2 = 1057, 105.7 MHz. LO 95.10 also stores 105.7.
Half of the 50 kHz resolution is lost on storage (6.4.21, 6.7).

### 6.2.10 Boot order

The tuning-related part of `setup()`: `settingsLoad()` (with migrations) → `purgeCorruptLimits()` →
`purgeCorruptSpurs()` (a list longer than 16 or with any entry outside 77.2 to 97.2 MHz is cleared
whole) → `Needle::begin()` (starts `needleTask`; first AS5600 read) → `Rda::begin()` (bus,
`initChip()`, creates `rdaTask`; an absent part is harmless) → `Needle::setCalibration()` with the
tuner ends → `Needle::seedAccumulator(cfg.lastAngle)` (the turn choice) → `applySettings()` (fit,
dial, fixed-feature list). The tuner ends are loaded before the seed on purpose, so the turn can be
chosen by them.

### 6.2.11 What is kept in flash

All tuning state is part of the one settings structure saved in the ESP32's non-volatile storage
(NVS), and in the settings file. Chapter 7 covers the format, the versions and the
migrations. The tuning fields: `tuneHoldMs` (V3); `dialLow`, `dialHigh` (V4); `tuneOffset10`,
`tuneUsed`, `tuneP[12]`, `tuneF[12]`, `tunerEndsSet` (V5); `spurUsed`, `spur[16]` (V6); `ifOffset20`
(V7); plus `calLow`, `calHigh`, `lastAngle`, `showTuning`, `bandLow`, `bandHigh` from the start. In
the settings file the samples are lines `tuneMark<i>=acc,f10` and the fixed features `spur<i>=f10`;
a restore applies each of the two lists whole or not at all. While a freshly updated S3 image is on
trial, nothing is written to flash: a sample stored then waits in RAM until the image is confirmed
(chapter 7).

### 6.2.12 Operator actions

The **portal** is the S3's web interface; the **console** is its serial command line, one key per
command, on the USB port and also reachable from the portal's Console tab (both are described in
chapter 8). All portal actions below are on the Needle tab and admin-only.

| Portal button (action) | Console key | What it does |
|---|---|---|
| *Measure this dial position* (`rda.sample`) | `V` | `sampleStart(true)`: one measurement at the current position, stored if it passes every gate. |
| *Mark station A/B/C here* (`tune.markA/B/C`), prompts for MHz | - | Stores (current shaft count, typed frequency) in slot 0, 1 or 2. Refused outside 87.0 to 108.5 MHz, while the encoder is down, or within 60 counts of another hand mark. Evicts any automatic sample within 60 counts. Leaves the hand offset alone. |
| *Clear all marks* (`tune.clear`) | - | Empties all 12 slots. Back to the stored straight line. The hand offset is kept, and the answer says so when it is not 0. |
| *Dial −0.1 MHz* / *Dial +0.1 MHz* (`tune.nudge`) | - | Adds to `tuneOffset10`, which slides the whole curve. ±0.1 to ±5.0 MHz per request, ±20 MHz in total. Reports the unsnapped frequency now tuned. |
| *Reset the dial correction to 0* (`tune.nudgeZero`) | - | Sets `tuneOffset10` to 0 and refits. The only way the hand offset is erased, apart from a settings-file restore. Answers "the dial correction is already 0" when there is nothing to do. |
| *Drop one sample (slot 0-11)* (`tune.drop`), prompts for the slot | - | Empties one slot (hand marks included). Leaves the hand offset alone. |
| *Tuner = low end* / *Tuner = high end* (`needle.tuneLow/High`) | `c` / `C` | Stores the current shaft count as `calLow` / `calHigh` and sets that end's bit in `tunerEndsSet`. Refused while the encoder is down. Refits. |
| *Forget the fixed features* (`rda.spurClear`) | `O` | Empties the fixed-feature list and saves. |
| - | `o` | Marks the last sweep's best candidate as a fixed feature and saves. Refused if already known or the list is full (16). |
| - | `R` | RDA state, bus, window, and where the LO should be. |
| - | `q` / `v` | A whole-window sweep (about a minute) / a narrow sweep at the prediction (about 10 s). Not a measurement; nothing stored. A refused sweep says "sweep refused - see the RDA state in 's' (running, absent, or lost)". |
| - | `a` | Prints the last sweep, bin by bin, then the fine pass's points ("fine pass (50 kHz; 0 = skipped)"), the chosen one marked "<<< chosen". |
| - | `s` | Status: the tuning chain, the curve sentence, the hand offset, each sample with the curve's value there and the residual (the difference), and the RDA state with the best candidate. |
| - | `D` | The settings dump, which includes the IF as `ifOffset=` (chapter 7). |
| `GET /api/rda` (admin) | - | The last measurement as JSON: `n`, `floor`, `if10`, `bins` as "f10 rssi flag;" (flag `.` ordinary, `^` above floor + 5, `S` stereo pilot decoded, `X` fixed feature), `fine` as "f20 rssi;" (the refinement's points, in twentieths of a MHz), `spurs`. |

The console keys are also reachable from the portal's Console tab.

### 6.2.13 Calibrating the tuning from nothing

Chapter 3 gives the whole first-time order; this is the tuning part of it, after the needle is
calibrated (chapter 5).

1. **The tuner ends.** Turn the knob to its low physical stop and press *Tuner = low end* (or `c`),
   then to its high stop and press *Tuner = high end* (or `C`). Console `s` prints both, signed,
   and the span. A negative value is not an error: on this radio the count falls as the frequency
   rises. Judge the span against the mechanism, not against the placeholder 6023: a tuning
   capacitor sweeps about half a turn (on this radio about 2200 counts, some 190° of shaft). A span
   that is short by about a whole turn (4096) is the sign of a lost revolution; the `residue` in
   the `s` tuning chain is the check.
2. **The fixed features.** Leave the set powered but switched off, so its oscillator is not
   running. Sweep the whole window (`q`, about a minute), print it (`a`), and press `o` once per
   peak: each press marks the strongest remaining candidate. Stop when what is left is noise. On
   this radio that was seven presses, once.
3. **Three spread positions.** Switch the set on and select RADIO. Turn the knob to a position low
   on the dial, press *Measure this dial position*, and wait (about 15 s when the prediction is
   close, up to about 85 s when it has to sweep the whole window). Repeat in the middle and near
   the top. Three samples bend the stored straight line into a quadratic. A "NOTHING FOUND" means
   the oscillator sits under a notch: nudge the knob and try again.
4. **Check it without circularity.** Tune to a station you know that is *not* one of the three
   positions, and read the portal's "Now". A quadratic passes through its three samples whatever
   they are, so only a fourth position proves the curve.
5. **After that**, nothing is needed. The automatic sampler re-measures wherever the knob rests
   while the radio plays, and fills the other automatic slots. Hand marks (A, B, C) and the dial
   correction are optional, and only a person's hand ever changes them.

If the tube set's IF is not 10.60 MHz, set *Tube set IF* before step 3: every sample is the
oscillator plus that value.

## 6.3 Settings and constants

### 6.3.1 Settings (saved)

| Name (file key) | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `ifOffset` (`cfg.ifOffset20`) | 212 (10.60 MHz; the fitted IF, Bible §22) | 50 kHz steps, shown in MHz | 10.00 to 11.50, step 0.05 | The tube set's IF; station = LO + this | Portal Needle tab "Tube set IF" (admin), settings file |
| `dialLow` | 879 (87.9 MHz, printed at the low stop; §5.9) | tenths of a MHz | 50.0 to 200.0 | Printed at the low stop. Clamps the Leditron, bounds sample acceptance, maps the needle | Portal (admin), file |
| `dialHigh` | 1079 (107.9 MHz, printed at the high stop; §5.9) | tenths of a MHz | 50.0 to 200.0 | Printed at the high stop | Portal (admin), file |
| `calLow` / `calHigh` | 0 / 6023 (the old compiled placeholders, meaning "not measured"; §5.9) | accumulated counts | −20000 to 20000 | Shaft count at the tuner's low / high mechanical end. Fit domain, stored-line slope, turn choice | Portal rows or *Tuner = low/high end*, console `c` / `C`, file. Typing a value counts as measuring that end |
| `tunerEndsSet` | 0 | bit 0 low, bit 1 high | 0 to 3 | Which ends were really measured | The writers above; file |
| `bandLow` / `bandHigh` | 881 / 1079 | tenths of a MHz | file only | The stored straight line used below 2 samples | Settings file only |
| `tuneUsed`, `tuneP[i]`, `tuneF[i]` (file `tuneMark<i>`) | empty | mask; counts; tenths | 12 slots | The samples: 0 to 2 hand marks, 3 to 11 RDA | `tune.mark*`, the sampler, `tune.drop`, `tune.clear`, file |
| `tuneOffset10` | 0 | tenths of a MHz | ±200 total | Hand offset (the dial correction) added to the whole curve | `tune.nudge`; reset to 0 only by `tune.nudgeZero`; a file restore (with the marks). Nothing automatic touches it |
| `spurUsed`, `spur[16]` | empty | tenths of a MHz (LO) | 77.2 to 97.2 | Learned fixed features | Console `o` / `O`, portal `rda.spurClear`, file |
| `showTuning` | 0 | choice | 0 clock, 1 tuning always, 2 tuning while tuning | Leditron readout mode | Portal Display tab, console `f` |
| `tuneHoldMs` | 4000 | ms | 500 to 20000, step 250 | How long mode 2 keeps the readout after the last movement | Portal Display tab, file |
| `lastAngle` | 0 | counts | - | Last live shaft count, for the turn choice at boot | Automatic, checked every 10 s |

### 6.3.2 Compiled constants

| Constant | Value | Where | Meaning |
|---|---|---|---|
| `TUNE_MARKS` | 12 | main.cpp | Sample slots. Part of the settings layout. |
| `CFG_SPURS` | 16 | main.cpp | Fixed-feature slots in the settings layout. |
| `Rda::MAX_SPURS` | 16 | rda.h | Fixed features the RDA module holds. Raised from 8 on 2026-09-07, when the seven-member comb appeared. Must equal `CFG_SPURS`; a `static_assert` in main.cpp refuses to build otherwise (2026-09-25). |
| `Rda::MAX_BINS` | 210 | rda.h | Bins of one sweep (the 201-bin window plus slack). |
| fine points | 11 | rda.cpp `FINE_POINTS` | The refinement's points kept for inspection. |
| `Rda::LO_WINDOW_LO10` / `HI10` | 772 / 972 | rda.h | The LO window, 77.2 to 97.2 MHz: a dial of 87.9 to 107.9 MHz minus the design IF of 10.7 MHz. Compiled; not derived from `ifOffset20`, `dialLow` or `dialHigh`. At the fitted 10.6 MHz it covers stations 87.8 to 107.8 MHz (6.7). |
| `Rda::IF_OFFSET20_NOMINAL` | 214 | rda.h | The design IF, 10.70 MHz. Not used by any code. |
| `LOST_AFTER` | 5 | rda.cpp | Failed points in a row that declare the part lost. |
| `PROBE_GAP_MS` | 10000 | rda.cpp | Minimum time between re-probes. |
| dwell | 300 ms minimum | rda.cpp `rdaTask` | Wait after each tune before reading RSSI. |
| STC wait | 60 ms | rda.cpp | Longest wait for "tune complete". |
| refinement | ±5 points of 50 kHz | rda.cpp | ±0.25 MHz. |
| notch | ±2 tenths; ±4 twentieths in the refinement | rda.cpp `isSpur()`, `isSpur20()` | ±0.2 MHz around each fixed feature. |
| stereo honoured | RSSI > floor + 3 | rda.cpp | Below that the flag is ignored. |
| local floor window | ±8 bins | rda.cpp `bestCandidate()` | Median of up to 16 neighbours. |
| `SMP_MIN_MARGIN` | 8 | main.cpp | Margin needed when a person asked. |
| `SMP_AUTO_MARGIN` | 14 | main.cpp | Margin needed for an automatic sample. |
| `SMP_MAX_DRIFT` | 20 counts | main.cpp | Shaft movement allowed during a measurement. |
| `SMP_MAX_DISAGREE10` | 40 (4.0 MHz) | main.cpp | Largest allowed gap between the answer and the curve. |
| `SMP_TRUST_AFTER` | 2 | main.cpp | Samples needed before the 4.0 MHz gate applies. |
| narrow half-width | 15 tenths (±1.5 MHz) | main.cpp `sampleStart()`, `loInWindow()`, console `v` | |
| "same position" | 60 counts | main.cpp `sampleStore()`, `tune.mark` | Refresh / duplicate radius. |
| "nothing changed" | same tenth and < 8 counts | main.cpp `sampleStore()` | Saves flash wear. |
| `AUTO_GAP_MS` | 300000 (5 min) | main.cpp | Between automatic attempts. |
| `AUTO_STILL_MS` | 8000 | main.cpp | Knob still before an automatic attempt. |
| `TUNE_DEADBAND` | 12 counts | needle.cpp | Movement detector threshold. |
| movement window | 400 ms | needle.cpp `tuningActive()` | |
| `SEAT_MARGIN` | 300 counts | needle.cpp | Added to each measured end for the turn choice. |
| AS5600 read period | 20 ms | needle.cpp | |
| fit guards | ends within 50.0 to 200.0 MHz; span ≥ 2.0 MHz; quadratic monotonic | main.cpp `fitTuneCurve()` | |
| unmeasured-ends pad | 30 % of the samples' span, at least 100 counts | main.cpp | |
| NaN fallback | 88.1 MHz + offset; valid range 10 to 300 MHz | needle.cpp `tuneFreq10Raw()` | |
| hand mark range | 87.0 to 108.5 MHz | settings_table.h `tune.mark` | |
| nudge | ±0.1 to ±5.0 MHz per request, ±20 MHz total | settings_table.h `tune.nudge` | |
| panel grid | 87.9 + 0.2 k MHz (odd tenths) | main.cpp `updateDisplay()` | FM channels of the Americas. |

## 6.4 Decisions

> **Your mileage may vary.** Each decision below was right for this radio, in this cabinet, on its
> measurements (§0.3).

The design of 2026-09-04 and other earlier design notes are mentioned below as history only; the code
as described here is the rule.

**6.4.1 Read the tube set's own oscillator ("Tier B").**
**Decision.** Calibrate the dial by hearing the tube set's LO with an RDA5807M inside the cabinet.
**Why.** The LO frequency *is* the dial position, at any position, with no station to identify: it
works on a dead channel, in a basement, on a poor antenna. It wants a short, deliberately
inefficient antenna inside the cabinet: bad at distant stations, good enough for a source a few
tens of centimetres away, and that selectivity is part of what separates the oscillator from the
broadcasts (what is built is Bible §11).
**Rejected.** Tier A: map real stations and snap to the nearest; it needs an outdoor antenna, the
opposite of the short inside wire Tier B wants. Tier C: correlate RDA audio with the tube set's
audio. It was the author's first idea; it is the most work for the least gain. The RDA5807M has no
digital audio output, so its audio would need an analog path into a spare converter, and the
correlation would have to run on the A32, the board that holds the tube set's audio, far from the
S3 where every constant it would correct lives.
**When and evidence.** 2026-09-03 (design); first S3 code 2026-09-05; sampler 2026-09-07. Tier B was chosen on 2026-09-03 although the drift was unmeasured, without measuring
the drift first.

**6.4.2 Station = LO + IF.**
**Decision.** The firmware adds the IF to the LO.
**Why.** Measured on the bench 2026-09-04: a 6 MHz move of the dial moved the carrier 6 MHz, in step,
below the station: low-side injection (Bible §14, §22). The bench test was a
standalone ESP32 with one RDA5807M, connected to nothing in the radio. Its record, computed with
the design IF of 10.7 MHz:

```
dial 92.5  ->  bare carrier at 81.8
dial 98.5  ->  bare carrier at 87.8

87.80   dial 92.5: 41   ->  dial 98.5: 59   (+18)
87.90   dial 92.5: 40   ->  dial 98.5: 59   (+19)
81.70   dial 92.5: 59   ->  dial 98.5: 40   (-19)
```

Neither carrier had a stereo pilot or RDS (the digital data of a broadcast), and both sat below
the broadcast band. A closed-loop read then gave 98.60 MHz for a dial verified at 98.5: one 100 kHz
step, with no refinement yet. Low-side also settled coverage: the whole printed dial needs an LO of
77.2 to 97.2 MHz, inside the chip's band.
**Rejected.** High-side (LO = station + IF), first assumed. At dial 98.5 a high-side LO would sit at
109.2 MHz, outside the window being swept, which is why the first sweeps found nothing; and the top
of the dial would have needed an LO near 118.6 MHz, beyond the chip's band.
**When and evidence.** Bench, 2026-09-04; S3 code from 2026-09-05. This is a measurement.

**6.4.3 The IF is a setting in 50 kHz steps, default 10.60 MHz.**
**Decision.** `cfg.ifOffset20`, editable in the portal, 10.00 to 11.50 MHz.
**Why.** The tube set's IF is 10.6 MHz since its professional tuning (Bible §22), not the 10.7 MHz
design value. The RDA readings of 2026-09-22 agree with it: six stations the author named by ear gave
10.60 MHz five times and 10.65 once. The IF could move again if the IF coils are realigned, so it must
be correctable without a flash. Twentieths, because 10.65 cannot be written in tenths.
**Rejected.** A compiled constant of 10.7 MHz (`Rda::IF_OFFSET10`), which stored every sample
0.1 MHz high.
**When and evidence.** 2026-09-22.

**6.4.4 The RDA has its own I2C bus.**
**Decision.** `Wire1` for the RDA, `Wire` for the AS5600 alone.
**Why.** The RDA5807M's addresses are fixed in silicon, and the AS5600's bus is the tuning sensor.
**Rejected.** Sharing the AS5600 bus.
**When and evidence.** 2026-09-05; wiring in Bible §2, §11. The pins were chosen on
2026-09-04.

**6.4.5 Chip set-up: band 2, AFC off, MONO cleared, STC polled, dwell ≥ 300 ms.**
**Decision.** As in 6.2.6.
**Why.** Band 2 because the LO window starts at 77.2 MHz, below band 0 (87 to 108). AFC off so the
chip cannot slide toward a louder neighbour and report the neighbour's frequency. MONO cleared
because the stereo indicator needs the stereo decoder. A 40 ms dwell read every channel at 12 to 23
instead of 40 to 63 and hid the LO completely: a confident false negative, which also made a
freshly fitted antenna look useless. It was caught only because a raw register read gave 49 on a
channel the sweep had just called 15. STC is polled so the dwell starts from a tune that has
really completed, and the 300 ms stays on top of it.
**Rejected.** The chip defaults (AFC on, and MONO forced by the first code).
**When and evidence.** Band 2 and the dwell: bench 2026-09-04, in the design of that day. AFC and
MONO: 2026-09-07, from the datasheet v1.1 register tables, RDA's reference
driver and the PU2CLR library.

**6.4.6 The part counts as present only when its whole set-up landed.**
**Decision.** Every configuring write in `initChip()` must succeed.
**Why.** One failed AFC write left a receiver free to slide, and every later measurement counted as
clean.
**When and evidence.** 2026-09-25.

**6.4.7 A lost RDA is detected, and heals.**
**Decision.** Five failed points in a row end the run and mark the part lost; `reprobe()` brings it
back; failed points yield 10 ms.
**Why.** Before, a part that stopped answering stayed "present", the sampler blamed a fixed feature,
and back-to-back failures starved `loop()` long enough to mute the audio.
**When and evidence.** 2026-09-24.

**6.4.8 Judge a peak by its margin over the local floor; never pick the strongest.**
**Decision.** `bestCandidate()` as in 6.2.6, inside the LO window only.
**Why.** The 79.0 MHz spur reads 63 against the LO's 56 to 59, and was the tallest feature at five
dial positions. On the bench, "the strongest bare carrier" reported the dial at 89.70 when it was at
98.5. A fixed threshold failed at the bottom of the dial. In a blind test (the author moved the dial
and said nothing) the bench read 107.3 MHz with a margin of +10 and 88.5 MHz with only +6, where a
fixed threshold of +7 reported "nothing rose"; readings at 98.5 and 107.3 had given +15 to +18.
Two causes add up at the bottom: the LO window's low end, 77.2 MHz, sits at the edge of the chip's
band, where it is least sensitive, and the station there was received more weakly. Because
injection is low-side, the bottom of the LO window *is* the bottom of the dial, so an absolute
threshold works across most of the dial and quietly fails at 87.9 to 89 MHz: a failure that looks
like working. Both blind readings were right.
**When and evidence.** Bench 2026-09-04; in the design of that day.

**6.4.9 The stereo flag rejects, and only above floor + 3.**
**Decision.** A bin with the stereo flag is skipped only if its RSSI is above the sweep's floor + 3,
in the sweep and in the refinement.
**Why.** The first sweep that could be inspected showed the flag on a bin at RSSI 36 against a floor
of 42. The datasheet explains it: the flag's power-on value is 1, and the chip's soft stereo blend
has the decoder judging a signal that is not there. A pilot decoded out of noise is not evidence of
a transmitter. A real broadcast stands well above the floor, so nothing worth rejecting is lost. As
a hard gate it would one day have thrown away the oscillator. It is still the only judgement the
chip gives that does not depend on amplitude.
**When and evidence.** 2026-09-07; the refinement's check, 2026-09-24.

**6.4.10 Fixed features are learned by hand, with the set switched off, and never compiled in.**
**Decision.** Console `o` after a sweep with the oscillator not running; list saved in settings;
default empty.
**Why.** With the oscillator off, every peak is a spur by definition. A compiled list would claim, on
a freshly erased machine, features it never measured, and blind the search at those places.
**Rejected.** Automatic learning during a calibration session ("anything that never moves across
several dial positions is a spur", the design of 2026-09-04). Also compiling this cabinet's
comb into the firmware.
**When and evidence.** 2026-09-07.

**6.4.11 No confirmation by movement.**
**Decision.** A candidate is accepted on its margin, the notches, the stereo rule, the notch guard and
the 4.0 MHz gate. The firmware never checks that the peak moved with the dial.
**Why.** The learned fixed features plus the margin did the job the movement test was meant to do.
**Rejected.** Confirming the LO by a second sweep at another position, expecting it to move by
"delta permille × local slope" (step 2 of the design of 2026-09-04).
**When and evidence.** 2026-09-07.

**6.4.12 Narrow, then wide once, then refine at 50 kHz; a failed refinement keeps the coarse answer.**
**Decision.** As in 6.2.7.
**Why.** The narrow sweep answers in about 10 s when the curve is roughly right. The wide sweep
rescues a bad prediction or an LO under a notch. 50 kHz decides which side of a channel boundary the
capacitor sits on; the old `fine` flag on `tuneTo()` could only re-address 100 kHz points, so a true
50 kHz unit (`tuneTo20()`) was needed.
**Rejected.** A calibration session of one wide sweep, then five narrow ones as the user moves
across the dial (the design of 2026-09-04). Each press measures one position.
**When and evidence.** 2026-09-07. The narrow sweep and coarse-then-fine come
from the design of 2026-09-04.

**6.4.13 Samples are stored, not coefficients, and in shaft counts, not permille.**
**Decision.** `tuneP` holds the raw accumulated count.
**Why.** Stored samples can be refitted with a better model without a migration, and no float goes
into flash. Permille depends on `calLow`/`calHigh`, so it changed the meaning of every sample each
time the tuner ends were re-measured.
**Rejected.** Samples in permille ("(permille, tenths-of-MHz)", the wording of the design of
2026-09-04).
**When and evidence.** 2026-09-04. Storing samples rather than coefficients is in the
design of 2026-09-04.

**6.4.14 A quadratic least-squares curve, with guards.**
**Decision.** Quadratic from 3 samples, line at 2, stored line below 2, with the monotonic, 50 to
200 MHz and 2.0 MHz guards and the domain rules of 6.2.5.
**Why.** Measured 2026-09-04: the dial is curved. The local slope was 0.251 tenths of a MHz per
permille between 91.3 and 98.5, and 0.288 between 98.5 and 107.3. A two-point line was exact at its
marks and about 0.5 MHz out mid-band. The straight line over the tuner's travel was 1.4 MHz out at
the bottom, 0.7 at the top and 0.1 in the middle: curvature, not offset, which no two-point
calibration can fix. Three RDA samples (91.3, 100.7, 107.4) predicted 98.5 at a position nobody had
measured, and the author confirmed it by ear. A five-sample fit on 2026-09-08:

```
   shaft    measured   curve     error
    -805     107.1     107.19    +0.09
    -694     105.8     105.67    -0.13
    -317     100.7     100.75    +0.05
    -134      98.5      98.49    -0.01
     502      91.3      91.30    -0.00
```

Worst residual 0.13 MHz, inside half a 0.2 MHz channel everywhere on the dial; a line through the
same points was 2.4 times worse. The curve bends by about 0.54 MHz across the travel.
**Rejected.** The straight line over permille; the two-point Mark A/B (6.6).
**When and evidence.** 2026-09-04; 2026-09-07. The model is in the design of
2026-09-04.

**6.4.15 Slots 0 to 2 are written by hand alone.**
**Decision.** Only `tune.markA/B/C` (and drop, clear, a file restore) write slots 0 to 2. A sample
near a hand mark is not stored at all. A new hand mark evicts automatic samples within 60 counts.
**Why.** Samples used to refresh hand marks, and parking at the ends replaced all three (88.5 →
88.1, 98.5 → 98.4, 107.3 → 107.9).
**When and evidence.** 2026-09-23; the eviction, 2026-09-24. Hand marks can always
be re-set in the web portal, just not automatically.

**6.4.16 Refresh a nearby automatic sample; write nothing when nothing changed; demand more when
nobody is watching.**
**Decision.** As in 6.2.7, with margins +8 (asked for) and +14 (automatic).
**Why.** Drift is the reason the feature exists, so re-measuring a position must replace the old
answer. An unattended sampler writing every five minutes would reach about 100,000 flash writes a
year. Observed margins on this radio ran +18 to +28, so +14 costs nothing real.
**Rejected.** Refusing a second sample at the same position.
**When and evidence.** 2026-09-07. All three fired in the 2026-09-08 to 09-10 soak.

**6.4.17 A full table replaces its most crowded automatic sample.**
**Decision.** With slots 3 to 11 full and no sample within 60 counts, the new point replaces the
automatic sample closest to a neighbour (the new point and the hand marks count as neighbours). If
the new point would itself be the most crowded, nothing is stored. Hand marks are never replaced.
The rule, with a worked example, is in 6.2.7 (Storing, step 6).
**Why.** Drift is the reason the sampler exists. With a refusal, the automatic calibration stopped
learning new positions for good once nine had been taken, until someone dropped a sample by hand. The
refresh keeps re-measuring where the listener listens; the replacement keeps the nine samples spread.
Spread, not recency: automatic samples cluster where the listener listens, and samples bunched in one
part of the dial extrapolate badly to its ends. Dropping the oldest would, over time, pull every
sample into the favourite stations.
**Rejected.** Refusing a new position with "drop one (tune.drop) to make room", the rule from
2026-09-07 until v.1.0.3.
**When and evidence.** v.1.0.3, 2026-09-26. Not yet exercised on the radio.

**6.4.18 Measure only while the radio is being listened to; a timid automatic trigger; always armed.**
**Decision.** Both entry points require `radioLive()` (amp on and source RADIO). This is a
**listening policy**: the selector does not switch the tube set, which runs whenever the amp does
(Bible §5, §21), so the gate means "only while the radio is what you are listening to". The refusal
says so: "the dial calibrates only while you listen to the radio - switch the amp on and select
RADIO". The trigger needs 8 s of stillness and 5 minutes between attempts. There is no switch to turn
the automatic sampler off, and nothing checks whether the needle is moving.
**Why.** Each clause is a reason not to measure: a wrong sample costs more than a missed one. And the
knob rests where the listener listens, so measuring while they listen to the radio measures the positions
that matter.
**Rejected.** Sampling whenever the amp is on, with any source; an on/off switch for the sampler; and
scanning only while the needle is idle (all three in the design of 2026-09-04). Also the
earlier wording of the gate as a hardware fact ("switch the set on and select RADIO - the oscillator
is not running"), which was not true of this machine.
**When and evidence.** 2026-09-07; the wording, 2026-09-25.

**6.4.19 The hand offset is the user's, and only the user erases it.**
**Decision.** Nothing but a person's own hand changes `tuneOffset10`: `tune.nudge` moves it,
`tune.nudgeZero` (*Reset the dial correction to 0*) returns it to 0, and a settings-file restore
replaces it with the file's value. No sample (automatic or asked for), hand mark, drop or clear
touches it. *Clear all marks* says when it kept a non-zero correction.
**Why.** The correction is the user's calibration, made by ear. It may no longer fit after the
samples change, but whether it still fits is the user's judgement, and there is a reset button. The
design of 2026-09-04 already said why nothing automatic may write it: two writers on one
value would mean the user's correction and the machine's silently fighting, with the loser invisible. The
samples define the curve, its shape and its position; the offset is applied on top. It is shown in
the console `s` and on the portal's Needle tab, so a stale one cannot hide behind a good calibration.
**Rejected.** Zeroing it on every sample (until 2026-09-24); then zeroing it on a sample asked
for by hand, a new hand mark, a drop and a clear (until 2026-09-25), on the argument that
the offset encodes the old curve's error.
**When and evidence.** 2026-09-24; 2026-09-25 (the reset button, and no automatic
erasure). Hand corrections are never erased automatically, only by hand.

**6.4.20 Acceptance gates: 4.0 MHz after two samples, the printed face, a single-bin margin.**
**Decision.** As in 6.2.7.
**Why.** 4.0 MHz is loose on purpose: before calibration the straight line was 1.4 MHz out at the
bottom, and a tight gate would refuse the measurements that fix it. Before two samples the curve is
an unchecked default and the gate is off.
**Rejected.** Tighter or different guard rails: "a couple of MHz", "reject a derived station outside
87.0-108.5", "require a coherent multi-bin peak" (the design of 2026-09-04).
**When and evidence.** 2026-09-07.

**6.4.21 The 50 kHz half-step is rounded away on storage, and kept that way.**
**Decision.** `tuneF` stays in tenths; odd-twentieth answers round half up (a bias of about +25 kHz).
**Why.** Changing it needs a settings-version bump; nothing on the radio showed a need. The fix is
written in `sampleStore()`: store `tuneF` in twentieths (settings version bump, migration ×2) and
fit on those.
**When and evidence.** 2026-09-25 (code note), recorded so that a later fix is easy.

**6.4.22 The 50 kHz refinement near a notch is left as it is.**
**Decision.** No special handling of a refinement that lands on a rounding boundary beside a notch.
**Why.** The 98.4 it produced records where the oscillator is at the position chosen.
**When and evidence.** 2026-09-23.

**6.4.23 The notch guard.**
**Decision.** Refuse a candidate 0.3 MHz from a fixed feature when the bin inside the notch is at
least as strong, or was not swept.
**Why.** Measured 2026-09-23: 100.7 needs LO 90.1, inside the 90.3 notch; the search took 90.0 and
stored 100.6, twice. The first version refused on distance alone and refused 98.5, a hand mark, on
its first boot.
**When and evidence.** 2026-09-23.

**6.4.24 Any I2C failure during a measurement discards it.**
**Decision.** `runFailures() > 0` → DISCARDED.
**Why.** A failed point is recorded as RSSI 0, which drags local floors down until noise scores like
the oscillator.
**When and evidence.** 2026-09-25. The bus must be known good before a value is committed.

**6.4.25 No sample from a frozen or re-seated encoder.**
**Decision.** Nothing is stored if the encoder had an outage since the start or is down now; hand
marks and tuner-end captures refuse while it is down; after an outage the angle is re-seated, not
dropped.
**Why.** While the AS5600 is down its count is frozen, so a knob turned then passes the drift check
and the frequency would be stored against the old position.
**When and evidence.** 2026-09-25.

**6.4.26 Nothing is stored if the set left RADIO during the measurement.**
**Decision.** `radioLive()` is checked again at every stage.
**Why.** A measurement lasts about 10 to 85 s; switching the set off or changing the source during one is
ordinary use, and the dead oscillator's sweep could store a mono broadcast or noise.
**When and evidence.** 2026-09-25.

**6.4.27 The shaft's turn is chosen by the measured tuner ends.**
**Decision.** `seatTurn()` as in 6.2.4.
**Why.** "Nearest to the last count" chose the wrong turn after more than half a turn of movement
during an outage, saved it as `lastAngle`, and every boot after inherited it.
**When and evidence.** 2026-09-25.

**6.4.28 The dial means where the capacitor is; samples are stored unsnapped.**
**Decision.** Samples keep the measured tenth; only the Leditron in mode 2 snaps to a channel.
**Why.** The listener tunes for the clearest sound, which need not be the channel centre. In a basement
on a poor antenna the set sat about 0.15 MHz high of 98.5 and received it through FM capture (the
stronger station wins the receiver); the RDA read the capacitor at 98.65 MHz, and was right to. The
panel, snapping to the nearest channel, shows 98.7; the portal keeps the true value, so the
calibration underneath stays honest.
**Rejected.** Snapping samples to the channel grid, which is a guess between stations.
**When and evidence.** 2026-09-07.

**6.4.29 The readout rules.**
**Decision.** The automatic readout (mode 2) only with the amp on and RADIO; mode 2 snaps to the
nearest odd tenth from the unrounded value; mode 1 does not snap; the portal never clamps.
**Why.** Snapping a value already rounded to a tenth put a quarter of the dial one channel wrong. The
portal does not clamp because clamping would destroy the evidence: past the printed face, the user
should see that the tuning went beyond the needle, not a reading pinned at the end. The comment in
`portalStateJson()` records why: when the tuning goes past the needle's travel, it is the needle
that is wrong.
**When and evidence.** 2026-09-02; 2026-09-04. From 91.3, turning the knob up
should go to 91.5, not 91.4. The amp-and-RADIO condition for mode 2 dates from 2026-09-03 (recorded
in the code).

**6.4.30 Everything reachable without the USB cable.**
**Decision.** `rda.sample`, `tune.drop`, `rda.spurClear` as portal buttons; `/api/rda`; the last
sampler outcome in the state JSON; the console through the portal.
**Why.** The USB ground is the machine's DC-side ground, which is on mains earth (Bible §30.4); a
cable to a computer adds a second ground path through the computer's earth. And once the cabinet is
shut, the cable is gone.
**When and evidence.** 2026-09-07; 2026-09-23.

**6.4.31 Both halves of a measurement stay readable.**
**Decision.** A refinement no longer clears the coarse sweep's bins, and its own eleven points are
kept beside them: console `a` prints both, `GET /api/rda` carries them as `bins` and `fine`. Only the
next sweep replaces them.
**Why.** The coarse sweep says where the oscillator is and the fine pass says exactly where; each is
the evidence for its half of the stored answer. Before, a refined measurement left nothing to inspect.
**When and evidence.** 2026-09-25. Not yet exercised on the radio (no measurement has run
since the flash with both lists shown).

**6.4.32 The two fixed-feature list sizes are tied at compile time.**
**Decision.** `static_assert(Rda::MAX_SPURS == CFG_SPURS)` in src/s3/main.cpp.
**Why.** The settings hold 16 fixed features and the RDA module holds its own copy; had the module's
list been smaller, `spurSet()` would have dropped learned features in silence. The guard had been
promised by a comment and did not exist.
**When and evidence.** 2026-09-25.

## 6.5 Failures and recovery

The ladder: BLOCKER = unusable until a full power cycle, or harm that survives one (a wrong sample
stored counts). DEFECT = recovers with a portal reboot, a console reset or the front switch. NOTE =
recovers by itself, or cosmetic. "Class" is what remains after the firmware's handling.

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| RDA absent at boot | `initChip()` fails | State "NOT FOUND on the bus"; every request refused with a reason; re-probe at most every 10 s | Fix the connection; the next request re-probes | NOTE |
| RDA lost during a run | 5 failed points in a row | Run ends, nothing published, `lost()`; "ABANDONED ... check its connector" | Same; heals on a re-probe | NOTE (would be BLOCKER: wrong sample) |
| Scattered I2C errors on the RDA bus | `runFailures() > 0` | DISCARDED | Press measure again; the automatic sampler retries in 5 min | NOTE (would be BLOCKER) |
| A set-up write fails (AFC left on) | `initChip()` returns false | Part reported absent; re-probed | Automatic | NOTE (would be BLOCKER) |
| Knob moved during a measurement | Drift > 20 counts | ABANDONED "hold it still" | Hold still, retry | NOTE |
| Encoder outage during a measurement | `encoderGaps()` changed, or `!i2cOk()` | DISCARDED | Retry | NOTE (would be BLOCKER) |
| Amp switched off or source changed during a measurement | `radioLive()` false | DISCARDED | - | NOTE (would be BLOCKER) |
| LO under a notch | Nothing reaches the margin | Narrow → wide → "NOTHING FOUND ... nudge it" | Move the dial a little | NOTE (normal at about one position in six) |
| LO beside a notch, peak maybe hidden | `besideSpur()` and the inner bin at least as strong | REFUSED "Move the dial 0.3 MHz" | Move 0.3 MHz | NOTE (was BLOCKER: 0.1 MHz wrong, silently) |
| Answer far from the curve | > 4.0 MHz, with ≥ 2 samples | REFUSED "a different signal" | None needed | NOTE (would be BLOCKER) |
| Answer off the printed face | Outside `dialLow`..`dialHigh` | REFUSED | - | NOTE |
| Sample slots full, new position | No free slot 3 to 11, none within 60 counts | The most crowded automatic sample is replaced; or, if the new point is the most crowded, "not stored ... would add the least coverage" | None needed | NOTE (normal operation) |
| A wrong sample passes every gate | Not detected | Stored and fitted | Console `s` residuals (meaningful from 4 samples); *Drop one sample*; a later automatic sample at that spot refreshes it | BLOCKER |
| A hand correction that no longer fits the samples | Not detected (nothing automatic touches it, by design) | Every readout is off by it | *Reset the dial correction to 0*, or nudge it back | NOTE (by choice) |
| Fit turns back / is degenerate | Monotonic test / singular system | Least-squares line; "QUADRATIC REFUSED" | Drop the bad slot | BLOCKER if the bad mark stays (saved); the curve stays usable |
| Fit leaves 50 to 200 MHz or spans < 2 MHz | End and span guards | Stored straight line; "MARKS REFUSED"; the toast is red | Re-mark or drop | BLOCKER while the bad mark stays (saved) |
| Tuner ends not measured | `tunerEndsSet != 3` | Domain = padded span of the samples; note on every surface | *Tuner = low end* / *high end* | NOTE |
| Corrupt fixed-feature list in flash or file | `purgeCorruptSpurs()`, the restore parser | List cleared at boot; a bad file list refused whole | Re-learn with the set off | NOTE |
| A wrong fixed feature learned (`o` with the set on notches the LO) | Not detected | - | `O` or *Forget the fixed features*, re-learn with the set off | BLOCKER (operator error, saved) |
| The IF changed (IF transformers realigned) | Not detected; every new sample off by the error | - | Re-measure the IF, set *Tube set IF*; drop or re-measure old samples | BLOCKER (saved) |
| `rdaTask` failures starving `loop()` | - | 10 ms yield per failed point; `LOST_AFTER` bound | - | NOTE (was DEFECT: audio muted) |

Verified on the radio: 2026-09-25 06:47 "RE-MEASURED 106.1 MHz (LO 95.50, +36 over local) ->
slot 4; 6 stored; curve QUADRATIC"; 12:40 to 12:47 "module ANSWERS" and "refined to 95.05 MHz, rssi
75" with the checked set-up writes; 18:42 after 2026-09-25, the dial still read 105.7 on the same turn,
QUADRATIC. The 2026-09-08 to 09-10 soak filled six samples from 89.0 to 107.2 MHz, three of them
automatic. Never exercised on the radio: the I2C-error discard, the "lost" path, and a re-probe after
a real loss (each needs a faulty bus); the full-table replacement (v.1.0.3); and a measurement showing
both the coarse bins and the fine points. The radio has run this tuning code since
v.1.0.3 (2026-09-26); v.1.0.5 did not change it.

## 6.6 Graveyard

These were dead ends in this setup, not necessarily in someone else's (§0.3). Each entry says what
happened here.

- **A straight line from `bandLow` to `bandHigh` over permille, typed by hand.** Tried first. Here it
  was right only if the needle reached both ends; the readout said 104.6 at 107.3. Replaced by
  two-point marks (2026-09-02), then by the curve (2026-09-04). `bandLow`/`bandHigh` survive only as the
  fallback below two samples, and left the portal.
- **Two-point "Mark station A / B"** (marks kept in RAM, a line extrapolated through them). Exact at
  the marks and about 0.5 MHz out mid-band on this dial, which is curved. Dropped for the quadratic
  (2026-09-04).
- **"Tuned to this station NOW" (`tune.here`)**, shifting the line to one point. Dropped with the
  line; the hand offset (`tune.nudge`) does that job on the curve.
- **Samples in permille.** Re-measuring the tuner ends silently redefined every sample here; it was
  defended by warnings on four writers and a staleness flag, all deleted when samples moved to raw
  counts (2026-09-04).
- **Earlier fit domains.** The samples' own span (froze the ends of the dial); the placeholder
  `calLow`/`calHigh` (2.7 times the real travel, which pulled the parabola's vertex inside and refused
  good marks); twice the samples' span (cut inside real travel); and a 30 % pad over measured ends
  (it reported "reaches 85.96 - 112.43 MHz"). The present rule is the third attempt.
- **High-side injection.** Assumed at first; the first sweeps here found nothing.
- **A 40 ms dwell, band 0, the strongest carrier, a fixed threshold.** Each hid the LO or picked a
  spur here (6.4.5, 6.4.8).
- **Confirming the LO by movement** (the differential sweep of the 2026-09-04 design). Never built in the
  S3; learned fixed features plus the margin and the 4.0 MHz gate did the work here.
- **Scanning whenever the amp is on, Bluetooth included.** The 2026-09-04 design; the shipped code has
  required RADIO from its first commit (2026-09-07), now as a listening policy.
- **The RADIO gate worded as a hardware fact** ("the oscillator is not running"). Here the tube set
  runs whenever the amp does, whatever the selector says, so the words were wrong; reworded as the
  listening policy it is (2026-09-25).
- **"Require a coherent multi-bin peak"**, **"reject outside 87.0 to 108.5"**. The 2026-09-04 design;
  shipped instead as a single-bin margin and the printed face.
- **A full table refusing every new position ("drop one (tune.drop) to make room")** (2026-09-07 to
  v.1.0.2). Here it meant that once nine positions had been measured, the automatic calibration
  learned no new position until someone dropped a sample by hand. Replaced by the spread-keeping
  replacement (v.1.0.3), which is close to the 2026-09-04 design's own rule ("drop the sample closest
  to a neighbour").
- **AFC at its default (on), MONO forced.** Every sweep before 2026-09-07 was taken by a receiver free
  to slide. The earliest samples were later refreshed; clean drift figures here start on 2026-09-08.
- **The stereo flag as a hard gate.** The first inspectable sweep showed it set 6 below the floor.
- **`tuneTo(f10, fine=true)` as the 50 kHz path.** It could never address 50 kHz (2026-09-07).
- **Refusing a second sample at the same position.** Replaced by the refresh (2026-09-07).
- **Samples allowed to refresh hand marks, and `V` allowed to claim slots 0 to 2.** Reverted on
  2026-09-23.
- **Every sample zeroing the hand offset.** Reverted 2026-09-24. **Then a requested sample,
  a hand mark, a drop and a clear zeroing it.** Here that erased a correction the user had set by
  ear, unasked. Dropped 2026-09-25 for the reset button.
- **Refinements clearing the sweep's bins.** Here a refined measurement left nothing to inspect but
  its final answer. Dropped 2026-09-25.
- **The notch guard on distance alone.** Refused 98.5, a hand mark, on its first boot (2026-09-23).
- **The sampler's stage raised before its sweep was requested, and `sweeping()` reading `gRunning`
  only.** Two races of one shape, a flag raised before the thing it announces. For the 100 ms
  `rdaTask` takes to notice a request, `sweeping()` said "not sweeping" and callers read the
  previous sweep's bins; and `sampleStart()`, on the portal task, raised its stage before asking,
  so `sampleTick()` in `loop()` on the other core judged stale bins from the previous dial
  position. Here they stored 91.4 MHz while the set was on 98.5 (2026-09-07). The fit's
  guard caught it ("QUADRATIC REFUSED: turns back on itself") and fell back to a line: a backstop
  working, not a plan working. A strong 88.5 broadcast lifting the local floor was blamed first;
  the race was the cause.
- **The IF as a constant, `Rda::IF_OFFSET10` = 107.** Every sample 0.1 MHz high here (2026-09-22).
- **A sweep of an absent part with no yield.** About 10 s of `loop()` starvation per sweep (2026-09-05,
  2026-09-24).
- **An encoder gap: movement accumulated across it**, which once injected a whole revolution
  (2026-09-02); **then movement dropped**, which shifted the frame until reboot (2026-09-25); **then the
  nearest turn**, wrong beyond half a turn (2026-09-25).
- **The readout gated on the magnet-detect bit.** It hid a good reading (2026-09-02).
- **A per-poll movement detector.** The 12-count deadband applied to one 20 ms poll was a rate floor
  of about 53° per second: a slow turn never registered, and the needle tracked while the panel did
  not (2026-09-02).
- **The standalone bench probe** (phase H of the bring-up: an ESP32 and one RDA5807M, wired to
  nothing in the radio). It answered, before any S3 code existed, whether this set's oscillator
  leaks, on which side, and whether the chip can hear it; its results are in 6.4.2, 6.4.5 and 6.4.8.
  It was a tool to reach v.1; it is retired and not published, and its results are recorded here.

## 6.7 Limits and firmware notes

- **The very top of the dial cannot be measured.** The LO window is compiled as 77.2 to 97.2 MHz,
  the printed dial minus the design IF of 10.7 MHz, and every sweep is clamped to it. At the fitted
  IF of 10.6 MHz a station at 107.9 MHz needs its LO at 97.3 MHz, one 100 kHz step past the window,
  so the window covers stations 87.8 to 107.8 MHz. Another IF setting moves the unreachable band. The
  curve still extrapolates over the last tenth, and the dial read true up to 107.3 MHz, the highest
  station receivable where this radio lives. 97.2 + 10.6 = 107.8 MHz is one step from the real
  107.9 MHz top of the North American band, and a peak near 107.9 would still show. Left as it is;
  a firmware note (§12.1.1).
- **50 kHz is lost on storage** (6.4.21): odd-twentieth results round half up, about +25 kHz
  (§12.1.5).
- **The notches cost about a sixth of the window.** "NOTHING FOUND" at roughly one dial position in
  six is normal.
- **The comb's source is unidentified.** One untested idea: harmonics of the A32's I2S bit clock
  (§12.1.7).
- **No automatic detection of a wrong sample** that passes every gate. The `s` residual table means
  nothing at exactly three samples (a quadratic passes through all three).
- **A mono broadcast near the LO** is told apart only by its margin, the notches and the 4.0 MHz
  gate; the stereo flag cannot reject it.
- **The full-table replacement judges crowding by shaft counts only.** It does not weigh how old a
  sample is, so a stale sample in a sparse part of the dial is kept while a fresh one in a crowded
  part is given up; a stale sample is replaced only when the listener tunes near it again (the 60-count
  refresh).
- **Fixed-feature learning is console-only** (`o`, also through the portal's Console tab). `o` does
  not check that the set is off, and marks any peak with a positive margin: the operator judges
  from `a` when to stop.
- **The fine pass's list does not mark a point rejected as a broadcast.** Such a point keeps the RSSI
  it read, while the console legend says "0 = skipped"; only fixed-feature and failed points read 0
  (§12.1.6).
- **The sweep's bin arrays are not synchronised between tasks.** A console dump during a sweep can
  read a mixed set (§12.1.2).
- **Accepted race:** a hand mark and an automatic sample landing in the same microsecond, and
  settings writes from two cores without a lock (§12.3.5).
- **Hard-coded for this radio's region and dial:** the odd-tenth panel grid (87.9 + 0.2 k), the
  50 to 200 MHz "is a dial" guard, the 88.1 MHz fallback.
- **The hand offset sits on top of the measured curve.** Nothing automatic clears it, by design,
  so a non-zero offset makes every readout differ on purpose from what the RDA measured, and
  the sampler's prediction and 4.0 MHz gate include it. The `s` residuals include it too. The portal
  state carries it (`toff`), and *Clear all marks* says when it kept one.
- **`tune.drop` also accepts slots 0 to 2**, so it removes a hand mark as well.
- **Drift itself has not been characterised.** Clean data exists only from 2026-09-08. The soak of
  2026-09-08 to 09-10 moved several samples by a tenth or two (107.4 → 107.2, 98.7 → 98.4, 105.8 →
  105.7), but those are not drift: the first readings had been taken before AFC was disabled and
  before the 50 kHz refinement existed, and the refreshes replaced them with a better instrument's.
- **Never exercised on the radio:** the I2C-error discard, the lost path, a re-probe after a real
  loss, the full-table replacement, and a measurement showing both lists (§12.1.8).

## 6.8 Changing this area

**Invariants that must hold.**
1. One evaluator: only `Needle::tuneFreq10Raw()` and its wrappers turn a shaft count into a
   frequency.
2. One fit: `fitTuneCurve()`, called from `applySettings()`. Hand marks and samples share it.
3. `cfg` is the origin of the samples and the fixed-feature list. Modules hold copies pushed by
   `applySettings()`; never write the RDA module's copy directly.
4. Settings are append-only: bump `SETTINGS_VERSION` and add a migration. `TUNE_MARKS` and
   `CFG_SPURS` are layout, not tuning knobs. `Rda::MAX_SPURS` must equal `CFG_SPURS`; the
   `static_assert` in main.cpp stops the build otherwise.
5. A failed I2C read is never a value, and a run with any failure is never stored.
6. Raise a flag only after the thing it announces: `gRunning` before `gWant` is cleared; the sampler
   stage only after the request was accepted; `sweeping()` is `gRunning || gWant`.
7. `rdaTask` outranks `loop()`: every path must block, and nothing may make `loop()` wait more than
   2 s. `loop()` is also under a 15 s task watchdog (chapter 4).
8. Add the IF in twentieths, then round. `ifOff10()` is for predictions and printouts only.
9. Slots 0 to 2 are written only by `tune.markA/B/C`, `tune.drop`, `tune.clear` and a file restore.
   The full-table replacement looks at slots 3 to 11 only.
10. The hand offset `tuneOffset10` is written only by `tune.nudge`, `tune.nudgeZero` and a file
    restore. Nothing automatic erases a hand correction.
11. Refusals go through `refuse()` or `smpSay()`, so the portal shows them in the refusal colour.
12. `i2cOk()` turns true only after the re-seat; captures refuse while it is false.
13. A dwell of at least 300 ms, whatever the caller asks.
14. Only a sweep clears the bins and the fine points; a refinement replaces only the fine points.

**Traps.**
- The count falls as frequency rises on this radio. Compare magnitudes and slope signs, never order.
  `calLow`, `calHigh`, `tuneP[]` and `lastAngle` are signed and legitimately negative: print them
  signed (until 2026-09-03 the console printed them unsigned, and a measured end of −1173 read as
  4294966123).
- Every notch blinds ±0.2 MHz, and there are seven here. Widening `isSpur()` blinds more of the dial.
- `tuneStateLine()` and `tuneModelName()` are the only sources of the tuning sentence. A refusal is
  the flag `fitRefused()`, never a word parsed from the sentence.
- The stored straight line always reports the most reassuring reach (88.1 to 107.9). Read the model
  sentence, not the reach, to know whether the samples are in use.
- The portal is the only interface once the cabinet is closed. Anything printed only on the console
  has not been reported.
- A new IF setting does not move the LO window (6.7).
- A discriminator that cannot be observed either works or silently throws the signal away, and
  nothing says which. Keep every judgement visible in `/api/rda` and `a`: that is how the stereo
  flag's fault was found.
- `radioLive()` is a listening policy, not a test of whether the oscillator runs. Do not reuse it
  where the oscillator's state matters.

**How to test a change.**
- `GET /api/rda` after a measurement, or the console `a`: every coarse bin with its judgement, then
  the fine pass's points. This is how the stereo rule's fault was found.
- The console `s`: the tuning chain (the residue `(acc − raw) & 4095` must never change while the
  machine runs), the curve sentence, and the per-sample residuals (meaningful from 4 samples).
- The test of a fit that is not circular: predict at a position that is *not* a sample, and confirm
  by ear.
- The fixed-feature baseline: the set powered but switched off, `q`, then `a`. Every peak is a spur.
- The full-table rule: with all nine automatic slots full (console `s` lists them), measure a new
  position with *Measure this dial position*; the answer names the replacement, or says the new point
  would add the least coverage. Work the expected slot out by hand first (6.2.7, Storing, step 6).
- Build both environments with the pinned toolchain, and flash the S3 over the portal (chapter 3).
- Test without a USB cable to a mains-earthed computer: the USB ground is the machine's DC-side
  ground, which is on mains earth (Bible §30.4), and a cable adds a second ground path.

---

# 7. Settings

## 7.1 What it does

A **setting** is any adjustable value that must survive a power cut: the volume law, the
gains, the clock and panel brightness, the needle's motion profile, its soft limits and
calibrations, the tuning curve, the Bluetooth policy, the LED levels. The machine has two
microcontrollers and each one keeps its own. The **S3** (the main board's ESP32-S3: clock
display, needle, panel lights, RDA5807M receiver, WiFi portal) owns everything about the
display, the lights and the needle. The **A32** (the audio board's classic ESP32: converters,
Bluetooth, volume knob) owns everything about sound and Bluetooth. A person meets settings in
four places. The **web portal** shows every value as a slider, box, checkbox or drop-down on
five tabs (Audio, Display, Lights, Needle, Bluetooth); a guest may move only volume, mute and
the five brightness levels, the owner may move everything. A change applies at once and is
written to flash about two seconds later, once the needle is standing still. Just after a
firmware update of either board, the board that was updated writes nothing until its new
firmware has confirmed itself, about a minute later; the change waits in memory until then. The portal's
System tab can **download** every setting as one plain-text file (`ambersong.txt`) and
**upload** such a file back; this file is the backup and the only undo. The **USB
consoles** of both boards (bench only, the cabinet is shut in normal use) change a few values
by single keys, and the **front volume knob** sets the A32's volume directly.

## 7.2 How it works

### 7.2.1 One table describes every value

Every portal-visible setting is one row of the array `gSet[]` in `src/s3/settings_table.h`.
The row type `SettingDesc` is declared in `src/s3/settings_api.h`:

| field | meaning |
|---|---|
| `key` | The stable machine name. The settings file and the JSON use it. It must never change. |
| `label`, `unit` | What a person reads. |
| `tab` | Which portal tab (`TAB_AUDIO` ... `TAB_BT`; `TAB_SYSTEM` has no rows). Row order is screen order and file order. |
| `store` | How the bytes are stored: `SU8`, `SU16`, `SI16`, `SI32`, `SF32` (unsigned/signed 8, 16, 32-bit integer, 32-bit float). It must match the C type of the field `ptr` points at; nothing checks this. |
| `kind` | How it is presented: `K_INT`, `K_BOOL`, `K_FLOAT`, `K_ENUM` (enum choices are one comma-separated string; the value is the index). |
| `lo`, `hi`, `step` | The slider range **and the hard clamp**, in shown units. |
| `scale` | Stored value x scale = shown value. Gains are stored as tenths of a dB (`scale 0.1`), the volume law as gamma x 10 (`0.1`), the IF in 50 kHz units (`0.05`). Integers travel safely in a struct copied byte for byte across the link; the person still reads "+8.0 dB". |
| `ptr` | The address of the live field: inside `cfg` (the S3's settings struct) or inside `a32cfg` (the S3's copy of the A32's settings, 7.2.9). |
| `owner` | Who applies it: `OWN_S3`, `OWN_A32_AUDIO`, `OWN_A32_BT`. |
| `admin` | 1 = only an administrator may change it. |

From this one table the firmware generates the JSON schema the browser renders
(`/api/schema`, `buildSchema()` in `src/s3/portal.cpp`), the current values (`/api/values`),
the bounding done by the set handler (`/api/set`), and both directions of the settings file.
The page itself knows nothing about any particular setting. The browser caches the schema
against a tag made of the firmware version and the admin bit (`schemaTag()`), so it is fetched
again only after a firmware change or a change of role.

`settings_table.h` is included once, by `src/s3/main.cpp`, after `cfg` and `a32cfg` are
defined, because the rows point straight into them. `portal.cpp` sees only
`settings_api.h` and never touches a struct.

There are 58 rows: 38 owned by the S3, 10 by the A32 audio side, 10 by the A32 Bluetooth side.
A few S3 values are not rows because they are not "one scalar with one range": the tuning
samples, the fixed-feature list and the pre-mark fallback band. They are written into the settings
file by hand (7.2.11). The learned WiFi transmit power is neither a row nor in the file: it is kept
in `cfg` and stepped from the S3 console only (7.3.5).

### 7.2.2 Reading, bounding and writing one value

- **`settingGet(i)`** copies the stored bytes with `memcpy`, converts to float and multiplies
  by `scale`. It never casts and dereferences the pointer, because `a32cfg` is a packed struct
  (no padding, alignment 1) and some 16-bit members sit at odd addresses (`bt.lookTimeoutS` is
  at offset 17).
- **`settingBound(d, v)`** is the only place a value is bounded; the portal and the file both
  call it. It refuses a value that is not finite (NaN passes every comparison, and `"nan"`
  parses), forces booleans to 0 or 1, clamps an enum to its first..last choice, and clamps
  everything else to `lo..hi`. It clamps; it does not refuse an out-of-range number.
- **`settingSet(i, shown)`**: bound the value; refuse an A32 row if the A32 has never answered
  (`haveCfg` false, 7.2.9); convert to stored units with `lroundf`; compare with the old bytes
  and write with `memcpy`; return false if nothing changed. If the row is `calLow` or
  `calHigh`, set that end's bit in `tunerEndsSet` (typing a tuner end counts as measuring it,
  one field one end). Then apply: an S3 row calls `applySettings()` and `settingsTouch()`; an
  A32 audio row sends the whole `ProtoAudio` struct (`MSG_SET_AUDIO`); an A32 Bluetooth row
  sends the whole `ProtoBtCfg` (`MSG_SET_BT`). Nothing in `settingSet` writes flash.
- **`POST /api/set`** (`hSet()` in `portal.cpp`): login required; 404 for an unknown key; 403
  when a guest touches an admin row; 409 with "the audio board is not answering - its settings
  cannot be changed now" for an A32 row while the S3 holds no copy of the A32's settings
  (`a32CfgKnown()`, 7.2.9); the value must be wholly a number (`argNumber()`: blanks
  trimmed, one decimal comma read as the point, anything else refused with 400). It calls
  `settingSet`, reads the value back, and answers `{"ok":1,"v":<actual>}`, plus `"clamped":1`
  when the stored value differs from what was asked by more than 0.0005, plus a `"warn"` with
  the tuning curve's state when the row was `calLow` or `calHigh`. The page snaps the widget to
  the returned value.
- **`applySettings()`** (`main.cpp`) pushes every S3 value into its module: display brightness
  target, panel levels and timings, needle speeds, second-order response, envelope, dwell,
  microstepping, measuring speed, soft limits, index band, tuner calibration, a refit of the
  tuning curve, the dial face, the RDA's fixed-feature list, and the WiFi transmit power. It
  then **writes back** the soft limits that `Needle::setGeometry()` actually accepted, so `cfg`
  always equals what the needle uses. It runs dozens of times a session, from both cores, so it
  must stay cheap and idempotent (running it twice changes nothing).

### 7.2.3 Where settings live

Both boards keep settings in **NVS** (Non-Volatile Storage), the key-value store that ESP-IDF
keeps in a small flash partition. The Arduino `Preferences` class wraps it: a **namespace**
groups keys, and a value can be a number, a string or a **blob** (a byte array). Neither board
keeps settings in a filesystem. Both builds use stock Arduino partition tables
(`default_16MB.csv` on the S3, `min_spiffs.csv` on the A32, set in `platformio.ini`), and NVS
sits at the same address in both, so stored settings survive a partition-table change.

| board | namespace | key | holds | part of the settings system? |
|---|---|---|---|---|
| S3 | `amb3` | `cfg` | The whole `Settings` struct, 220 bytes, one blob. | Yes. |
| S3 | `net` | `ssid`, `pass`, `ntp`, `tz` | The house WiFi network name and passphrase, the NTP server, the POSIX time-zone string. | No. Changed from the portal's Network card (`/api/net`) or the S3 console `y`. Not in the settings file. |
| S3 | `auth` | `users` | The portal accounts: names, salted SHA-256 password hashes, admin flags. | No. Changed from the portal's account cards. Not in the settings file. |
| A32 | `amb` | `audio` | The `ProtoAudio` struct (15 bytes). | Yes. |
| A32 | `amb` | `bt` | The `ProtoBtCfg` struct (12 bytes). | Yes. |
| A32 | `amb` | `potMin`, `potMid`, `potMax` | The volume knob's calibration, one unsigned 16-bit value each. | Yes, but not rows (7.3.5). |

The needle's remembered position after a software restart lives in RTC memory that survives a
reset but not a power cut; it is not a setting (chapter 5). The WiFi defaults
compiled in `src/s3/net.cpp` include a time zone for the author's location; change it for your
build.

### 7.2.4 The S3 struct: versioned, append-only, migrated

`struct Settings` in `main.cpp` begins with a header, `magic = 0xA838` and
`version = SETTINGS_VERSION` (7), followed by every field with its default as an initializer.
The rules are written above the struct and are absolute:

- Never reorder or remove a field. Append only.
- Every append bumps `SETTINGS_VERSION` and adds a migration branch.
- An older blob is copied into the current struct so its values survive and the new fields
  keep their defaults.

**Loading, `settingsLoad()`:**

1. No blob: "no stored settings - defaults in use." The defaults run and are written on the
   first change.
2. Read `min(stored length, sizeof(Settings))` bytes into a zeroed buffer. Size decides only
   how much may be read, **never which version the blob is**.
3. Fewer than 100 bytes (the size of the oldest layout): a warning, and defaults.
4. `magic` is not `0xA838`: the blob is **Version 1**, the layout from before the header
   existed (its first two bytes are the two brightness values). It is copied field by field.
5. `version == 7`: a straight copy. "settings loaded."
6. `version` 6, 5, 4 or 3: a **prefix copy** up to `offsetof(Settings, <first field that
   version did not have>)`. It is not the blob length, because a new field can sit in the old
   struct's trailing padding (V2 to V3 did exactly that). The V4 branch also infers
   `tunerEndsSet` (below).
7. `version == 2`: field by field from the kept `SettingsV2` layout.
8. Our magic but any other version: this firmware is older than the one that saved the blob.
   Defaults in RAM, and the **downgrade lock** is set (7.2.7).

Every migration increments the touch count, so the migrated blob is written back as V7 about
two seconds later, once the needle is still. Each migration prints one console line saying what
the new fields start as.

**Version history** (from the history of `SETTINGS_VERSION`):

| version | what it added | commit, date |
|---|---|---|
| V1 | The layout without a header, as it was in the machine on 2026-08-31; it holds the first real needle calibration. `SettingsV1` in `main.cpp` keeps that layout exactly; 38 of its 39 fields map across. The one that does not, `seekHsps`, was a deleted knob (the index search only has to find the sensor; the slow re-approach defines zero, so its speed never mattered, and a knob that implies it matters is worse than no knob) and is dropped on migration. | before the repository |
| V2 | The `magic`/`version` header; the layout that shipped the web portal. | before the repository |
| V3 | `tuneHoldMs`, placed in V2's trailing padding (same 100 bytes). | 2026-09-01 (first commit) |
| V4 | `dialLow`, `dialHigh`: the first append that grew the struct. | 2026-09-02 |
| V5 | `tuneOffset10`, `tuneUsed`, `tuneP[12]`, `tuneF[12]`, `tunerEndsSet`. A V4 blob whose `calLow`/`calHigh` differ from the defaults 0/6023 is taken as proof of a hand measurement and gets `tunerEndsSet = 3` (both ends). | 2026-09-04 |
| V6 | `wifiTxQ`, `spurUsed`, `spur[16]`. | 2026-09-07 |
| V7 | `ifOffset20`, default 212 (10.60 MHz). | 2026-09-22 |

The V6 to V7 migration changes behaviour on the first boot: every computed station moves
down 0.1 MHz, because the firmware used to assume a 10.70 MHz IF and now uses 10.60 MHz, the
tube set's fitted IF (Bible §15, §22).

**After loading, two purges run once** (`setup()` in `main.cpp`):

- `purgeCorruptLimits()`: a stored soft-limit pair that does not bracket the index at 0
  (`posMin > 0`, `posMax < 0`, or crossed) cannot be real. It is replaced by the provisional
  pair -300..+300 half-steps (`PROVISIONAL_LIMIT_HS`) and saved; the console asks for a
  re-home, a band calibration and new stops. The index-band values are kept, because they are
  relative to the index.
- `purgeCorruptSpurs()`: a fixed-feature count above 16, or any entry outside the RDA's LO
  window (77.2..97.2 MHz, `Rda::LO_WINDOW_LO10/HI10`), empties the list and saves the empty
  list. A truncated list is not a list.

### 7.2.5 Boot order

**S3** (`setup()` in `main.cpp`):

```
task watchdog, image state  first of all (chapter 4)
settingsLoad()            blob -> cfg (or migrate, or defaults, or lock)
purgeCorruptLimits()      once, not inside applySettings()
purgeCorruptSpurs()
proto self-test           a failure no longer halts: boot goes on, the image is never confirmed
Display, Panel, Needle, Rda begin
Needle::setCalibration(calLow, calHigh, ends known)   the tuner ends first ...
Needle::seedAccumulator(lastAngle)                    ... so the saved angle picks the right turn
applySettings()
link begins               the A32 mirror arrives later, after the handshake (7.2.9)
applySettings()           again, after the display comes up at standby brightness
homing, Net::begin(), Portal::begin()
```

**A32** (`src/a32/main.cpp`): before anything else, `setup()` asks the bootloader whether the
running image is on trial and keeps the answer in `gImgOnTrial` (9.2.13); the save path consults
it (7.2.8). Then `settingsLoad()` fills defaults, reads the three pot keys, reads
the `audio` and `bt` blobs only if each stored length equals the struct's size exactly, then
forces `muted = 0`. `settingsApply()` pushes the values into the audio engine and the LED
driver. The first volume-knob reading in `loop()` then sets the volume (7.2.8).

### 7.2.6 The write path on the S3

The names first:

- **`settingsTouch()`**: "something changed". It records `dirtyAt = millis()` and atomically
  increments the touch count `gTouchGen`. Every writer calls it **after** the change.
- **`gSavedGen`**: the touch count that the last successful write copied.
- **`settingsDirty()`** is `gTouchGen != gSavedGen`. "Unsaved" is derived from the two counts,
  not stored as a flag (2026-09-25), so a change that lands during a write, on either
  core, is never marked saved by that write.
- **`writeSafe()`** is true only when the needle's velocity is exactly 0, no needle calibration
  is running and no S3 firmware update is in progress. An NVS write disables the flash cache,
  and everything not in IRAM stops, including the step emitter. Measured 2026-08-31 during a
  migration write: step jitter 383 µs and a display slot error of 1257 µs, against 49 µs and
  8 µs with the guard.
- **`settingsWrite()`** is the only function that writes the `amb3` blob:
  1. refuse if the downgrade lock is set;
  2. refuse, silently, while this S3 image is **on trial** after an update (`s3ImageOnTrial()`,
     chapter 3): nothing is written until the image confirms itself (below);
  3. one writer at a time: an atomic exchange on a `busy` flag; the loser sets `gWriteBusy` and
     returns false ("not now"), because the portal's reboot (core 0) and `loop()` (core 1) could
     both open the same `Preferences` handle;
  4. read `gTouchGen` **before** copying;
  5. copy `cfg` into a static 220-byte snapshot;
  6. `prefs.begin("amb3")` and `putBytes("cfg", snapshot)`, checking both results;
  7. on failure: stay dirty, restart the two-second wait so it is retried, print
     "[WARN] settings NOT saved - the flash write failed.";
  8. on success: `gSavedGen = ` the count read in step 4; print "settings saved."
- **Nothing is saved during an S3 trial** (v.1.0.1, 2026-09-25). An image written
  over the air boots on trial and is rolled back by the next reset unless it confirms itself first,
  about 70 s after boot (chapter 3). During that time every change, a migration included, stays in
  RAM, still counted as unsaved. Once the image is confirmed the ordinary debounce writes it on the
  next pass of `loop()` where `writeSafe()` holds. So a rolled-back image never meets a blob written
  by the image that replaced it. The cost: a change made during the trial minute is lost if the
  power goes in that minute.
- **`settingsFlush()`** runs on every pass of `loop()`: if dirty, and at least 2000 ms since
  the last touch, and `writeSafe()`, then `settingsWrite()`. The two-second **debounce** exists
  because flash traffic is the one load that visibly disturbs the display, and a slider dragged
  across its range would otherwise write on every pixel.
- **`settingsForceFlush()`** ("Save now", exported to the portal as `settingsFlushNow()`):
  nothing dirty returns true. Locked returns false. On trial returns false. Not `writeSafe()`
  returns false. Otherwise it writes **until nothing is left**, up to 40 attempts: if the other
  core is writing, it waits 25 ms and tries again; a real flash failure returns false; still dirty
  after 40 attempts returns false. Its callers act on "true" at once, so true must mean "nothing is
  unsaved".
- **`gSaveWhy`** holds the reason for the last refusal, one of five sentences: the stored
  settings are from a newer firmware; the firmware is on trial and settings are written once it is
  confirmed ("about a minute"); the needle is moving or a calibration or update is running; the
  flash write failed; the settings kept changing while being written. `settingsSaveWhy()` exports
  it to the portal. Until 2026-09-24 every refusal gave the motion sentence, whatever
  its real cause.

Worked example, a portal slider:

```
t = 0.0 s   /api/set k=panelIdle v=140   settingSet -> cfg.panelIdle = 140,
            applySettings(), settingsTouch()                    gTouchGen 41, gSavedGen 40
t = 0.4 s   the slider moves again -> 150                        gTouchGen 42, dirtyAt reset
t = 2.4 s   loop(): dirty, 2 s since the last touch, but the needle is tracking -> wait
t = 3.1 s   the needle stops -> settingsWrite(): gen = 42, snapshot, putBytes OK
            gSavedGen = 42, "settings saved."
```

**Who writes without being asked:**

- `loop()` saves `cfg.lastAngle` (the tuning shaft's accumulated encoder count, which lets the
  next boot pick the right turn) every 10 s when it has changed, and only while the encoder
  answers.
- `loop()` copies the WiFi driver's learned transmit power into `cfg.wifiTxQ` whenever the two
  differ (chapter 8 describes the ladder that learns it).
- The RDA auto-sampler stores tuning samples in slots 3..11, refreshing a nearby one or, when all
  nine are full, replacing the most crowded (chapter 6). It never touches the hand marks or the
  hand offset.
- The index-band calibration stores the band and shifts the soft limits into the new frame.
- The two boot purges.

**The three buttons that force a save:**

| caller | what it does |
|---|---|
| Portal "Save settings now" (`sys.save` in `doAction()`) | `settingsForceFlush()`: "saved", or the `gSaveWhy` sentence painted as a refusal. |
| Portal "Reboot the main board" (`hReboot()` in `portal.cpp`) | While the S3 image is on trial: restarts at once and answers "rebooting - this firmware was still on trial, so the previous firmware comes back"; nothing was saved during the trial, so there is nothing to wait for, and the button stays the one way to undo a bad update. Otherwise: force-flush. If refused and not locked: `Needle::stop()`, retry every 100 ms up to 15 times. Still refused: the needle is resumed and the answer is HTTP 409 "not rebooting - <why>". While locked the reboot proceeds, since nothing would be written anyway. |
| End of an S3 firmware update (`hOtaEnd()`) | Clears the "update in progress" flag first (otherwise `writeSafe()` could never pass), force-flushes, and reboots even if that failed; the failure is printed on the USB console only. The new image then boots on trial (§3.2.9). An upload sent while the running image is itself still on trial is refused before any of this: nothing is flushed and nothing restarts (chapter 8). |

### 7.2.7 The downgrade lock

When `settingsLoad()` finds our magic with a version it does not know, this firmware is older
than the one that wrote the blob. It sets `gSettingsLocked`. While locked:

- The machine runs on **defaults** in RAM: provisional soft limits, default tuning line, default
  brightness, and so on.
- `settingsWrite()` refuses everything, so the newer blob on flash is never overwritten.
- **Download is refused** with HTTP 409 and a sentence, and the page shows the sentence instead
  of navigating. A file of defaults would look complete, and uploading it back would lift the
  lock and write defaults over the real calibration (2026-09-25).
- The dashboard shows a **SETTINGS LOCKED - newer version stored** pill (the state JSON carries
  `"setlock":1`).
- "Save settings now" answers with the lock sentence.

There are two ways out. **Flash the newer firmware again**, which reads its own blob normally.
Or **upload a complete settings file**: every one of the 38 S3 rows present and accepted, a
tuning block that committed (`tuneUsed` present, the mark lines matching the mask, the offset
readable) and a fixed-feature block that committed (`spurUsed` present, slots 0..N-1 exactly).
A32 rows, `bandLow`, `bandHigh` and `tunerEndsSet` are not required. Keys this
firmware does not know (a newer firmware's file) are refused and named but do not block. On
success the console prints "complete settings file uploaded - the newer stored settings will
now be replaced." and the next save writes V7 over the newer blob. Otherwise the portal note
says "SETTINGS STILL LOCKED - a complete file is needed (N of 38 settings, no tuning marks, no
fixed features); nothing was saved". The values that did arrive are still applied in RAM.

A single line cannot lift the lock because, while locked, `cfg` holds defaults: lifting the
lock on `volume=120` wrote the defaults for everything else over the newer blob two seconds
later (2026-09-24).

### 7.2.8 The A32's own storage

- **`settingsDefaults()`** fills `cfgA` (`ProtoAudio`) and `cfgB` (`ProtoBtCfg`); the values are
  in 7.3.
- **`settingsLoad()`**: defaults first; `potMin` (default 60), `potMid` (0 = not measured),
  `potMax` (3990); then the `audio` and `bt` blobs, each **only if its stored length equals the
  struct's size**. Any change to either struct therefore means that struct's defaults; there is
  no migration on this board. Last, `muted` is forced to 0: mute is never restored.
- **`settingsTouch()`** sets a dirty flag and the time.
- **`settingsFlush(force)`**, called on every `loop()` pass: when dirty and (forced, or 2000 ms
  since the last touch), write both blobs and the three pot keys and **check every result**: the
  `Preferences` open, and each write's byte count. Only when all of them succeeded is the dirty
  flag cleared and "settings saved." printed. On any failure the flag stays set, the two-second
  wait restarts so the write is retried, and "settings NOT saved - the flash write failed;
  retrying" is printed on the A32's USB console and sent to the S3 as a log line (`MSG_LOG`,
  which the S3 prints as "[A32] ..."). This is the S3's rule brought over (2026-09-25). It has **no motion guard**: the A32 drives no stepper.
- **Nothing is saved during an A32 trial** (v.1.0.4, 2026-09-26). The S3's hold
  (7.2.6), brought over. An A32 image written over the air boots on trial (9.2.13).
  `settingsFlush()` returns without writing, forced or not, while `gImgOnTrial` is set and
  `gImgConfirmed` is not. Changes made meanwhile (from the portal, a pot calibration or the A32
  console) are applied at once and wait in RAM, still dirty. `confirmImage()` confirms the image
  a minute after the S3's first HELLO, or after five minutes if the S3 never spoke, and never
  while the proto self-test has failed. On confirmation, if anything is dirty, it calls
  `settingsFlush(true)` at once; when that write succeeds it sends "settings changed during the
  trial are now saved" to the S3 as a log line, next to "image confirmed (...)". If that write
  fails, the ordinary failure path applies: the warning, and a retry two seconds later from
  `loop()`. The hold is silent while it lasts: the portal answers "ok" to an A32 row as usual,
  because the S3 keeps no flag for the A32's trial (it sees it only as text in the A32's boot
  report, 9.2.4). So an A32 image that changes `ProtoAudio` or
  `ProtoBtCfg` cannot save its new layout before it has proved itself, and a rollback always finds
  blobs the previous image can read. The cost: a change made during the A32's trial is lost if
  the power goes, or the A32 restarts, before confirmation.
- `MSG_REBOOT` ("Reboot the audio board") calls `settingsFlush(true)` before restarting (2026-09-25); if that write fails, the board restarts anyway and the change is lost. During an
  A32 trial that call writes nothing, by design: a restart on trial is a rollback, so there is
  nothing to keep.
- **Writers**: `MSG_SET_AUDIO`, `MSG_SET_BT`, `MSG_CAL_POT` (a pot calibration), and the A32
  console keys `+ - m t T p P c`.
- **The volume knob**: `updateVolumePot()` reads the knob every 50 ms (median of five), maps it
  through the three-point pot calibration to 0..255, and writes `cfgA.volume` **without
  touching** the settings. The first reading after every A32 boot always applies; later readings
  apply only when the raw value moved by at least 24 counts. So a volume set from the portal holds
  until the knob is next moved, and the knob's position wins at every boot. The volume reaches
  flash only when something else causes a save.
- **Bounds on the A32**: `Audio::setTaper` clamps the volume law to 10..40 and
  `Audio::setBalance` to -100..100 when applying. Gains and fade times are not clamped here;
  the S3 is the only remote sender and clamps first.
- During an A32 firmware update, `muted` is forced on and restored afterwards to what it was.

### 7.2.9 How the S3 proxies the A32's settings

The A32 **owns and saves** its settings. The S3 keeps a **mirror**, `ProtoCfgAll a32cfg`
(`ProtoAudio audio; ProtoBtCfg bt; uint16 potMin, potMid, potMax`), so the portal can show what
is actually in the machine rather than what it last asked for.

```
  S3                                              A32
  HELLO  ------------------------------------------>
         <------------------------------------------  HELLO_ACK (its firmware version)
  GET_CFG ----------------------------------------->
         <------------------------------------------  CFG = ProtoCfgAll (audio, bt, pot cal)
  a32cfg = CFG ; haveCfg = true
         <------------------------------------------  STATE, every 250 ms
  a32cfg.audio.volume, .muted = STATE's                (the knob moves the volume; nothing
                                                        else would tell the S3)
  portal: gainRadio = 8.5
  a32cfg.audio.gainRadio = 85
  SET_AUDIO = the whole ProtoAudio ----------------->  cfgA = it ; settingsApply() ;
                                                       settingsTouch()  -> NVS 2 s later
```

- At every handshake (`MSG_HELLO_ACK` received in `onMessage()`), the S3 sends `MSG_GET_CFG`
  and copies the reply into `a32cfg`. A peer that rebooted brings **its** values back rather
  than the S3's copy overwriting them. When the A32's uptime in a STATE frame goes backwards,
  the S3 notices a restart the link missed and handshakes again, which refreshes the mirror.
- The portal's Audio tab also has a **Re-read the A32** button (`sys.getcfg`), which sends
  `MSG_GET_CFG` on demand.
- A change to an A32 row edits one field of the mirror and sends the **whole** struct. The A32
  copies it, applies it and saves it two seconds later (or, while its image is on trial, once
  the image is confirmed: 7.2.8).
- **"Do not write a mirror we have never read."** Until the first `MSG_CFG` arrives the mirror is
  all zeros. `settingSet()` refuses every A32 row while `haveCfg` is false; a push in that window
  would have sent volume 0, gain 0, every LED off and "not connectable", and the A32 would have
  saved them. This guard exists since the first commit (2026-09-01). The portal answers such an edit
  with HTTP 409, "the audio board is not answering - its settings cannot be changed now".
- **A silent A32 is forgotten.** When the link, having handshaken, stops hearing the A32
  (`peerHello` and not `gLink.peerAlive()`, in `loop()`), the S3 clears `haveCfg` along with its
  other A32 state and prints "[WARN] A32 silent." From then on the guards above apply again: portal
  edits of A32 rows get 409, a download writes `n/a`, an upload names them "NOT SENT". The next
  handshake fetches the settings afresh (2026-09-25). Before, `haveCfg` stayed true: edits
  went only into the mirror and were overwritten when the A32 came back, and a download wrote the
  stale copy.
- The settings file follows the same rule: while `haveCfg` is false, export writes `n/a` for
  every A32 row and import names A32 rows "NOT SENT" (7.2.11, 7.2.12).
- **Boards on different protocol versions** no longer look like a dead wire: the S3 counts the
  frames it drops for their version and shows the count as "wrong-version N" in console `s`, as
  `linkver` in the state JSON, and, while no handshake stands, as a portal pill, "BOARDS RUN
  DIFFERENT PROTOCOL VERSIONS - flash both" (v.1.0.2; chapter 9).
- The volume knob's calibration is not a row. The portal's Audio tab buttons **Pot: minimum /
  centre / maximum** (`pot.min`, `pot.ctr`, `pot.max`) send `MSG_CAL_POT`, and the A32 measures
  the knob itself (median of nine readings), because the reading and the thing being calibrated
  must not live on opposite sides of a link.
- `Link::send` may be called from both S3 cores (the UART driver takes the whole buffer under
  one lock).

### 7.2.10 Concurrency

The S3 changes settings from two tasks:

| core | task | writes |
|---|---|---|
| 0 | the portal task (`serverTask` in `portal.cpp`) | `/api/set`, the file upload, the actions (`tune.mark*`, `tune.clear`, `tune.drop`, `tune.nudge`, `tune.nudgeZero`, `needle.limitLow/High`, `needle.nudge`, `needle.tuneLow/High`, `rda.spurClear`, `sys.save`), the reboot and update-end saves. Each writes `cfg` or `a32cfg` and calls `applySettings()` from core 0. |
| 1 | `loop()` | `settingsFlush()`, the `lastAngle` save, the `wifiTxQ` copy, RDA samples, band-calibration results, console keys, and `onMessage()` writing `a32cfg` (`MSG_CFG`, and volume/mute from `MSG_STATE`). |

What protects the data: the touch counts (a change on either core during a write is never
marked saved); the single-writer flag in `settingsWrite()`; the snapshot (the blob written is one
consistent copy); "touch after the change" everywhere; and the multi-field units (tuning marks
with their mask and offset; the fixed-feature list) are parsed into locals and committed in one
step.

What is **not** protected, by decision: there is no lock around `cfg` itself. The
remaining races are microseconds wide: an upload half-applied at the instant a background save
takes its snapshot, followed by a power cut within about two seconds; a hand mark and an
automatic sample in the same microsecond; a soft-limit edit against a concurrent
`applySettings()`.

### 7.2.11 The settings file: format and export

`settingsToText()` builds the file; `GET /api/settings.txt` (admin only) serves it as an
attachment named `ambersong.txt`. The format is plain text, one `key=value` per line, `#`
starts a comment line. It stays readable when neither this firmware nor a browser exists.

```
# Ambersong settings
# firmware v.1.20260925T184108 (1a2b3c4)       build stamp and commit (chapter 3)
# settings version 7
# saved 2026-09-25T18:45:00                    only when the clock is valid; local time
# the audio board had not answered - its settings are n/a      only while !haveCfg
volume=40                                       one line per gSet row, in table order
...                                             (58 lines)
gainRadio=8.000                                 SF32 rows and scaled rows: 3 decimals
ifOffset=10.600
...                                             every other row: an integer
connectable=1                                   A32 rows read "n/a" while !haveCfg
bandLow=881
bandHigh=1079
tuneOffset10=0
tuneUsed=7                                      ALWAYS written, even 0
tunerEndsSet=3
tuneMark0=-817,913                              <shaft count>,<freq x10>, one per set bit
tuneMark1=120,985
tuneMark2=1043,1073
spurUsed=2                                      ALWAYS written, even 0
spur0=790                                       LO frequency x10, slots 0..N-1
spur1=818
# potcal 61..1974..3988                         only while haveCfg; never imported
```

(The values above are illustrative.)

- **`tuneUsed` and `spurUsed` are written even when zero.** The import hangs its "replace the
  marks" and "replace the list" steps on these keys, so a backup taken before any marking can
  undo marking. Without the zero line it would be identical to a file from before those features
  existed, and would leave the marks alone.
- **`n/a` while the A32 is silent.** A file of the mirror's zeros, uploaded later, would have
  pushed volume 0, gains 0, LEDs off and "not connectable" to the A32, which saves them.
- **Not in the file:** `magic`, `version` (a comment only), `lastAngle` (live state), `idxOnFwd`
  (zero by definition), `wifiTxQ` (the learned WiFi transmit power, console only since v.1.0.2),
  the A32's unused `autoConnect` field, the WiFi and time settings, the portal accounts, the pot
  calibration (comment only), and every experiment toggle (7.3.6).

### 7.2.12 The settings file: import

The System tab's **Upload what is below** posts the text box as `text/plain` (20 s timeout) to
`POST /api/settings.txt` (admin only), which calls `settingsFromText()`.

**An upload is a merge, not a replace.** A key absent from the file keeps its current value.
The only exceptions are the two blocks below, which replace the stored marks and list as a unit
when they commit.

**Per line:** trim; skip empty lines and lines that **start** with `#` (a `# comment` after a
value makes the value unreadable); split at the first `=`; skip lines without one. Then:

| key | rule | on a bad value |
|---|---|---|
| `tunerEndsSet` | Whole integer 0..3, applied at once. | REFUSED and named. |
| `bandLow`, `bandHigh` | Whole integer 500..2000 (x0.1 MHz), applied at once. | REFUSED and named (refused, not clamped). |
| `tuneOffset10` | Whole integer -200..200 (x0.1 MHz), **held** with the marks. | The whole tuning block is refused: "TUNING CALIBRATION REFUSED - the hand-offset line is unreadable; the stored calibration was kept". |
| `tuneUsed` | Integer, decimal or `0x` hexadecimal (a console dump once printed it in hexadecimal), 0..0xFFFF, held. | Ignored with no note; every mark line after it is then skipped. |
| `tuneMark<i>` | Only after a `tuneUsed` line. `i` a whole decimal 0..11 (`tuneMarkA` is not accepted: it is what someone writes after reading the console dump, which labels the hand slots A, B, C, and a loose parse read it as slot 0, so mark A could silently receive another mark's numbers with the mask still agreeing); value `<pos>,<freq>`, pos within +/-1,000,000, freq 870..1085; held. | Skipped silently; the mask check then refuses the block. |
| `wifiTxQ`, `autoConnect` | **Retired keys** (2026-09-25): skipped whatever the value, not counted, not named, so a file saved by an older firmware uploads cleanly. | - |
| `spurUsed` | Integer 0..16, held. | Spoils the fixed-feature block. |
| `spur<i>` | Only after `spurUsed`. `i` 0..15, value 772..972 (x0.1 MHz, the LO window), no duplicate slot; held. | Spoils the fixed-feature block. |
| any other key | Looked up in `gSet[]`. | See the next list. |

Any key starting with `tuneMark` or `spur` is treated as part of its block, never as an unknown
key.

**Table rows**, in order: unknown key, REFUSED; value `n/a` (any case), noted "n/a in the file -
kept as they are"; an A32 row while `haveCfg` is false, noted "NOT SENT - the audio board is not
answering"; a value that is not wholly a number (one decimal comma allowed), REFUSED; otherwise
bounded by `settingBound()`, and if that changed it by more than 0.0005 it is noted CLAMPED and
stored at the bound. Rows are written straight into `cfg` or the mirror, with no per-line apply
and no per-line message.

**At the end of the file, in this order:**

1. **Tuning block.** Commits only if `tuneUsed` was seen, the parsed marks' bits equal the mask
   exactly, and any offset line was readable. Then all 12 slots, the mask and (if present) the
   offset replace the stored ones together. Otherwise: "TUNING MARKS REFUSED - the file announced
   N and M parsed; the stored calibration was kept". A file with no `tuneUsed` (older than V5)
   leaves the marks alone.
2. **Fixed-feature block.** Commits only if `spurUsed` was seen, no line spoiled it, slots
   0..N-1 all arrived, and no two entries are closer than 0.3 MHz (the matcher's +/-0.2 MHz would
   count them as one). Otherwise "FIXED-FEATURE LIST REFUSED - the stored one was kept".
3. **Tuner ends inference**, only for a file that carries `calLow` or `calHigh` but no
   `tunerEndsSet` (files older than V5): per end, a value equal to the struct default (0 or 6023)
   clears that end's bit and anything else sets it. This is a sentinel convention of the file, not
   a claim about the shaft: 0 and 6023 are the old compiled placeholders (§5.9), and a measured end
   that happened to equal one exactly would be read as unmeasured. Done before the refit.
4. **Downgrade lock** check (7.2.7).
5. If any S3 value changed: `applySettings()` and `settingsTouch()`. If the soft limits that came
   out differ from what the file asked (`setGeometry()` refused a pair that does not bracket the
   index): "SOFT LIMITS REFUSED (a..b does not bracket the index) - kept c..d".
6. **The fit gets a say.** Unless the tuning block was refused, and when marks are stored: if the
   resulting curve is refused or demoted, "after restore: <state>" is added. `calLow`/`calHigh`
   alone can do this, since they set the fit's domain.
7. The four named lists: "REFUSED (unreadable or unknown): ...", "CLAMPED to its range: ...",
   "n/a in the file - kept as they are: ...", "NOT SENT - the audio board is not answering: ...".
   Each names up to five keys (each cut to 20 characters) and then ", ...". They go last so the
   block refusals are never crowded out of the 512-byte note.
8. One `MSG_SET_AUDIO` and/or one `MSG_SET_BT` if any A32 row landed.

The return value counts accepted items: each accepted line, plus one per committed block. The
portal answers `{"ok":1,"m":"N settings applied"}`, or, when there is any note,
`{"ok":1,"m":...,"warn":<note>}`. The page shows the `warn` text as a warning (not in the
success colour) and then reloads the values.

**Worked example.** With the A32 answering and no lock, this upload:

```
volume=999
posMax=abc
wn=10,5
nosuchkey=1
tuneUsed=7
tuneMark0=-817,913
tuneMark1=120,985
```

gives: `volume` clamped to 255 (named CLAMPED) and sent to the A32 once at the end; `posMax`
refused; `wn` accepted as 10.5; `nosuchkey` refused; the mask announces three marks and two
parse, so the whole tuning block is refused and the stored marks stay. The answer is
`"m":"2 settings applied"` with
`"warn":"TUNING MARKS REFUSED - the file announced 3 and 2 parsed; the stored calibration was
kept; REFUSED (unreadable or unknown): posMax, nosuchkey; CLAMPED to its range: volume"`, and
the page shows the warning. `wn` is saved about two seconds later, once the needle is still.

## 7.3 Settings and constants: the complete key reference

Legend.
**Owner**: S3; A32a (A32, audio struct); A32b (A32, Bluetooth struct).
**Saved in**: `amb3/cfg` (S3 blob); `amb/audio`, `amb/bt` (A32 blobs); `amb/potMin` etc.
(A32 keys).
**Changed by**: P-G portal, a guest may change it; P-A portal, admin only; F settings-file
upload; C3 S3 console key; C32 A32 console key; Knob the front volume knob; X portal action
button (action name); Auto the firmware itself.
Every row in 7.3.1 to 7.3.3 is exported in the file and importable. Ranges are the clamp in
`settingBound()` unless stated. Defaults were read from `struct Settings` (S3) and
`settingsDefaults()` (A32).

### 7.3.1 Audio tab (all owned by the A32)

| key | default | unit (stored as) | range | meaning | owner | saved in | changed by |
|---|---|---|---|---|---|---|---|
| `volume` | 40 | 0..255 linear fader (u8) | 0..255 | Master volume; the volume law is applied on the A32. The knob's first reading after each A32 boot replaces it. | A32a | `amb/audio` | P-G, F, C32 `+`/`-` (10 per press; `+` also unmutes), Knob |
| `muted` | 1 in the defaults, forced to 0 at every load | bool (u8) | 0/1 | Software mute (writes zeros to the DAC stream). Saved but never restored. | A32a | `amb/audio` | P-G, F, C32 `m`; forced on during an A32 update and restored after |
| `taper` | 2.5 (stored 25) | gamma (u8, x10) | 1.0..4.0, step 0.1 | Volume law: gain = (volume/255)^gamma; 2.5 puts half travel at -15 dB. | A32a | `amb/audio` | P-A, F, C32 `t`/`T` (0.1 per press) |
| `balance` | 0 | -100..+100 (i16) | -100..100 | Negative leans left. | A32a | `amb/audio` | P-A, F |
| `gainRadio` | +8.0 dB (stored 80) | dB (i16, tenths) | -20..+30, step 0.5 | Makeup gain on the tube radio input. | A32a | `amb/audio` | P-A, F |
| `gainBt` | -7.0 dB (stored -70) | dB (i16, tenths) | -20..+30, step 0.5 | Bluetooth gain, set to match the radio's loudness within about 1 dB. | A32a | `amb/audio` | P-A, F |
| `monoSum` | 1 | bool (u8) | 0/1 | Sum left and right on the radio input (about 3 dB less noise). | A32a | `amb/audio` | P-A, F |
| `muteOnChange` | 1 | bool (u8) | 0/1 | Fade down and up across a change of source. | A32a | `amb/audio` | P-A, F |
| `fadeInMs` | 1000 | ms (u16) | 0..5000, step 50 | Fade-up time on arriving at a source. | A32a | `amb/audio` | P-A, F |
| `fadeOutMs` | 150 | ms (u16) | 0..5000, step 10 | Fade-down time on leaving a source. | A32a | `amb/audio` | P-A, F |

Why these defaults:
`volume` 40 and the `balance` setting date from an earlier setup of the amp's own volume pot,
which is gone: the amp's pot has been replaced and sits on the amp's board, reached only from the
back (Bible §30.3). The front panel's volume knob is the A32's pot, not the amp's (Bible §31.3).
The defaults stay as they were: `volume` is overwritten by that knob's first reading at boot
anyway, and `balance` remains a plain left/right trim. It does nothing on AUX, which never passes
through the DAC (Bible §22).
`gainRadio` +8.0 dB was measured here on 2026-09-23 with the tube radio's own volume control at
about one eighth of its travel, its normal position (Bible §22); the previous +15 dB clipped at the
gain stage at every volume.
`gainBt` -7.0 dB was measured the same way: a pink-noise reference at -14 dBFS RMS (a typical
streaming loudness), played from a computer at full volume, arrived at about -15 dBFS; -7 dB lands
it within about 1 dB of the radio, so a change of source does not change the loudness.
`fadeInMs` 1000 and `fadeOutMs` 150: arriving at a source should be gentle (about a second, not a
jump scare); leaving should be quick, because the listener has just turned
the selector.
`muteOnChange` exists because the source selector is an ON-OFF-ON switch (Bible §11): its centre
position gives 0 V through 22 kΩ and reads as AUX (Bible §11, §22), so a change from RADIO to
Bluetooth passes through AUX for a moment. The fade covers that passage.

### 7.3.2 Display and Lights tabs

| key | default | unit (stored as) | range | meaning | owner | saved in | changed by |
|---|---|---|---|---|---|---|---|
| `brightOn` | 255 | 0..255 (u8) | 0..255 | Clock brightness while the amp is powered. | S3 | `amb3/cfg` | P-G, F, C3 `-` and `+`/`=` (15 per press) |
| `brightOff` | 110 | 0..255 (u8) | 0..255 | Clock brightness with the amp off. | S3 | `amb3/cfg` | P-G, F |
| `hour12` | 0 | bool (u8) | 0/1 | 12-hour clock. | S3 | `amb3/cfg` | P-A, F |
| `blankLeadZero` | 1 | bool (u8) | 0/1 | Blank the hour's leading zero. | S3 | `amb3/cfg` | P-A, F |
| `dispDwellMs` | 250 | ms (u16) | 0..5000, step 50 | Stillness before the clock brightness starts to move. | S3 | `amb3/cfg` | P-A, F |
| `dispFadeMs` | 1500 | ms (u16) | 0..10000, step 50 | Eased (smoothstep) travel between the two brightnesses. | S3 | `amb3/cfg` | P-A, F |
| `showTuning` | 0 | enum (u8) | 0..2 | 0 clock always; 1 tuning readout always; 2 tuning readout while tuning, then back to the clock. | S3 | `amb3/cfg` | P-A, F, C3 `f`/`F` (cycles) |
| `tuneHoldMs` | 4000 | ms (u16) | 500..20000, step 250 | In mode 2, how long the readout stays after the dial last moved. | S3 | `amb3/cfg` | P-A, F |
| `panelTuning` | 255 | 0..255 (u8) | 0..255 | Panel light level while tuning. | S3 | `amb3/cfg` | P-G, F |
| `panelIdle` | 128 | 0..255 (u8) | 0..255 | Panel level, radio idle. | S3 | `amb3/cfg` | P-G, F |
| `panelOther` | 96 | 0..255 (u8) | 0..255 | Panel level on AUX or Bluetooth. | S3 | `amb3/cfg` | P-G, F |
| `panelIdleMs` | 5000 | ms (u16) | 0..60000, step 250 | Time without tuning before "idle". | S3 | `amb3/cfg` | P-A, F |
| `panelFadeMs` | 900 | ms (u16) | 0..5000, step 50 | Eased panel fade. | S3 | `amb3/cfg` | P-A, F |
| `panelDwellMs` | 250 | ms (u16) | 0..5000, step 50 | Stillness before the panel moves. | S3 | `amb3/cfg` | P-A, F |
| `btLedOff` | 255 | 0..255 (u8) | 0..255 | Bluetooth LED brightness scale in state OFF (`ledBrightness[0]`). | A32b | `amb/bt` | P-A, F |
| `btLedOn` | 255 | 0..255 (u8) | 0..255 | ... state ON, ready (`[1]`). | A32b | `amb/bt` | P-A, F |
| `btLedLook` | 255 | 0..255 (u8) | 0..255 | ... state LOOK, looking (`[2]`). | A32b | `amb/bt` | P-A, F |
| `btLedStdby` | 255 | 0..255 (u8) | 0..255 | ... state STDBY, standby (`[3]`). | A32b | `amb/bt` | P-A, F |
| `btLedFound` | 255 | 0..255 (u8) | 0..255 | ... state FOUND (`[4]`). | A32b | `amb/bt` | P-A, F |
| `btLedCon` | 255 | 0..255 (u8) | 0..255 | ... state CON, connected (`[5]`). | A32b | `amb/bt` | P-A, F |
| `btLedLink` | 255 | 0..255 (u8) | 0..255 | ... state LINK, explicit pairing (`[6]`). | A32b | `amb/bt` | P-A, F |

`brightOff` is not lower on purpose: the display colon is lit by its own fixed circuit and cannot
dim with the digits (Bible §8).

### 7.3.3 Needle and Bluetooth tabs

"hs" is a half-step of the needle's stepper motor. "Counts" are the tuning shaft's accumulated
AS5600 encoder counts (the angle, carried across whole turns). Needle rows are admin only. The
motion defaults are the settings arrived at on a bench test rig; they are this build's settings, not a reference the code departs from (chapter 5 explains each).

| key | default | unit (stored as) | range | meaning | owner | saved in | changed by |
|---|---|---|---|---|---|---|---|
| `upVmax` | 1100 | hs/s (u16) | 100..4000, step 50 | Top speed of every second-order move, in either direction (portal label "Sweep up, top speed"). | S3 | `amb3/cfg` | P-A, F |
| `upAccel` | 18000 | hs/s² (u16) | 1000..60000, step 500 | Acceleration of the same moves. | S3 | `amb3/cfg` | P-A, F |
| `dnVmax` | 1700 | hs/s (u16) | 100..4000, step 50 | Top speed of the decay: the fall and the park. | S3 | `amb3/cfg` | P-A, F |
| `dnAccel` | 18000 | hs/s² (u16) | 1000..60000, step 500 | Acceleration of the decay (chapter 5). | S3 | `amb3/cfg` | P-A, F |
| `wn` | 9.0 | rad/s (f32) | 1..21, step 0.5 | Natural frequency of the second-order ring-down. | S3 | `amb3/cfg` | P-A, F |
| `zeta` | 0.50 | none (f32) | 0.1..2.0, step 0.05 | Damping of the ring-down (overshoot). | S3 | `amb3/cfg` | P-A, F |
| `riseMs` | 100 | ms (u16) | 10..3000, step 10 | Fall envelope, rise. | S3 | `amb3/cfg` | P-A, F |
| `fallMs` | 1000 | ms (u16) | 10..5000, step 10 | Fall envelope, fall. | S3 | `amb3/cfg` | P-A, F |
| `dwellUpMs` | 100 | ms (u16) | 0..3000, step 25 | Hesitation before rising. | S3 | `amb3/cfg` | P-A, F |
| `dwellDnMs` | 1000 | ms (u16) | 0..3000, step 25 | Hesitation before falling. | S3 | `amb3/cfg` | P-A, F |
| `microFast` | 16 | 1/n (u8) | 1..32 | Microstep divisor for sweeps, parks and homing, and for the needle sitting at the park (portal label "Microstep while moving"). | S3 | `amb3/cfg` | P-A, F |
| `microSlow` | 32 | 1/n (u8) | 1..32 | Microstep divisor while tracking the tuner (portal label "Microstep at rest"). | S3 | `amb3/cfg` | P-A, F |
| `reapHsps` | 60 | hs/s (u16) | 10..500, step 10 | Creep speed of the index-band calibration (portal label "Measuring pass speed"); also the portal jog speed. Not the homing re-approach. | S3 | `amb3/cfg` | P-A, F |
| `posMin` | -300 | hs from the index (i32) | -3000..3000 | Low soft limit. The pair must bracket the index (0) and not cross; `Needle::setGeometry()` refuses any other pair and the old one is written back. The row's range is wide on purpose, so that a wrong value is refused by `setGeometry()` rather than silently stored as a bound. Purged to -300..+300 at boot if corrupt. | S3 | `amb3/cfg` | P-A, F, C3 `m`, X `needle.limitLow`, X `needle.nudge` (moves both), Auto (band calibration shifts it; boot purge) |
| `posMax` | +300 | hs from the index (i32) | -3000..3000 | High soft limit. Same rules. | S3 | `amb3/cfg` | P-A, F, C3 `M`, X `needle.limitHigh`, X `needle.nudge`, Auto |
| `idxOffFwd` | 0 | hs (i32) | -500..500 | Index band: sensor OFF edge going up, relative to the index. A measurement, not a preference. | S3 | `amb3/cfg` | Auto (band calibration: C3 `k`, X `needle.calBand`), P-A, F |
| `idxOnRev` | 0 | hs (i32) | -500..500 | Index band: ON edge going down. | S3 | `amb3/cfg` | as `idxOffFwd` |
| `idxOffRev` | 0 | hs (i32) | -500..500 | Index band: OFF edge going down. | S3 | `amb3/cfg` | as `idxOffFwd` |
| `sweepOn` | 0 | bool (u8) | 0/1 | A full end-to-end needle sweep each time the amp comes on. The homing at boot ends with a full sweep whatever this says (chapter 5). Its meaning changed on 2026-08-31, when homing became a once-per-boot event instead of an amp-on one; a stored 0 still means "no sweep". | S3 | `amb3/cfg` | P-A, F |
| `dialLow` | 879 | x0.1 MHz (u16) | 500..2000 | The frequency printed at the needle's low stop. | S3 | `amb3/cfg` | P-A, F |
| `dialHigh` | 1079 | x0.1 MHz (u16) | 500..2000 | The frequency printed at the needle's high stop. | S3 | `amb3/cfg` | P-A, F |
| `calLow` | 0 (placeholder) | counts (i32) | -20000..20000 | Tuning shaft position at its low mechanical end. Typing it sets bit 0 of `tunerEndsSet`. | S3 | `amb3/cfg` | P-A, F, C3 `c`, X `needle.tuneLow` |
| `calHigh` | 6023 (placeholder) | counts (i32) | -20000..20000 | Tuning shaft position at its high end. Typing it sets bit 1. | S3 | `amb3/cfg` | P-A, F, C3 `C`, X `needle.tuneHigh` |
| `ifOffset` | 10.60 MHz (field `ifOffset20` = 212) | MHz (i16, 50 kHz units) | 10.00..11.50, step 0.05 | The tube set's IF. Turns the RDA's reading of the set's local oscillator into a station frequency. | S3 | `amb3/cfg` | P-A, F |
| `connectable` | 1 | bool (u8) | 0/1 | Let a bonded phone reconnect. Even at 1, the A32 is connectable only while it is awake (amp on, S3 talking) and the source is Bluetooth, so a phone cannot latch on at night to a radio whose amp is off (`applyScanMode()`, chapter 10). | A32b | `amb/bt` | P-A, F |
| `lookTimeoutS` | 90 | s (u16) | 15..600, step 5 | Pairing window, despite the name: the explicit pairing state LINK falls back to STDBY after this. LOOK has its own fixed 20 s, not a setting (chapter 10). | A32b | `amb/bt` | P-A, F |
| `pauseOnLeave` | 1 | bool (u8) | 0/1 | Send a "pause" to the phone before the disconnect that follows leaving Bluetooth. | A32b | `amb/bt` | P-A, F |

Why these defaults:
`upVmax` 1100, not 1700: moving up through the index faster than about 1100 hs/s, the needle
mechanism stops following while the firmware keeps counting (measured by the author 2026-09-24:
always at 1500, often at 1200..1400, never at 1100). This is a physical value of this machine, held
by the firmware (§5.9). The range still allows up to 4000.
`microFast`/`microSlow`: the microsteps are made by the firmware itself, with PWM on the four inputs
of the ULN2003A driver (Bible §11), which has no microstepping of its own (chapter 5). The row caps
the divisor at 32.
`posMin`/`posMax` -300/+300: the provisional soft limits, which assume the needle's real travel
extends more than 300 hs on each side of the index and the index band is narrower than about
600 hs, so the provisional sweep crosses it. Physical values of this machine, held by the firmware
(§5.9).
`dialLow`/`dialHigh`: the dial face prints 87.9 MHz at the low stop and 107.9 MHz at the high stop,
a physical value of this machine, held by the firmware (§5.9).
`calLow`/`calHigh` 0/6023 are the old compiled placeholders meaning "not measured", not a claim
about the shaft (§5.9).
`ifOffset` 10.60 MHz: the tube set's fitted FM IF is 10.6 MHz (Bible §15, §22).

The **System** tab has no rows; the page draws its own cards there (settings file, network,
firmware, machine).

### 7.3.4 Hand-written keys (in the file, not table rows; all owned by the S3, saved in `amb3/cfg`)

| key | default | format and import rule | meaning | changed by |
|---|---|---|---|---|
| `bandLow` | 881 (88.1 MHz) | integer 500..2000, else REFUSED | The pre-mark fallback line, low end (x0.1 MHz). Nothing reads it once two marks exist. | F only |
| `bandHigh` | 1079 (107.9 MHz) | as `bandLow` | Fallback line, high end. | F only |
| `tuneOffset10` | 0 | integer -200..200 (x0.1 MHz, so +/-20 MHz); commits with the marks | The hand offset (the dial correction), sliding the whole tuning curve. Never erased automatically. | X `tune.nudge` (0.1..5 MHz per press, total +/-20 MHz); reset to 0 only by X `tune.nudgeZero` (*Reset the dial correction to 0*); F |
| `tuneUsed` | 0 | integer, decimal or `0x`, 0..0xFFFF; always exported | Bit i set = slot i holds a sample. | X `tune.markA/B/C`, `tune.clear`, `tune.drop`, `rda.sample`, C3 `V`, Auto (sampler); F |
| `tuneMark0` .. `tuneMark11` | empty | `<shaft count within +/-1,000,000>,<freq x10, 870..1085>` | The tuning samples. Slots 0-2 are the hand marks A, B, C; slots 3-11 are automatic RDA samples. Raw shaft counts, so a sample keeps its meaning when the tuner ends are re-measured. | as `tuneUsed` |
| `tunerEndsSet` | 0 | integer 0..3, else REFUSED; inferred for older files (7.2.12) | Bit 0 = low tuner end measured, bit 1 = high end. Only 3 lets the fit use the measured travel. | C3 `c`/`C`, X `needle.tuneLow/High`, typing `calLow`/`calHigh` (P-A), F, the V4 migration |
| `spurUsed` | 0 | integer 0..16; always exported | How many fixed features are stored. Empty by default on purpose: the list is only ever earned. | C3 `o` (add the strongest candidate), C3 `O` (clear), X `rda.spurClear`, Auto (boot purge); F |
| `spur0` .. `spur15` | empty | integer 772..972 (LO frequency x10), no two closer than 3 | The **fixed features**: peaks the cabinet itself produces, excluded when hunting the tube set's local oscillator (LO). | as `spurUsed` |

`spur*` range: the LO window, 77.2..97.2 MHz, because the fixed features are learned at LO
frequencies. The tube set's FM oscillator runs below the station (low-side injection; Bible §14, §22); the window is the printed dial minus the 10.7 MHz design IF
(chapter 6).

### 7.3.5 Stored, but neither a row nor in the file

| item | owner, saved in | default | meaning | changed by |
|---|---|---|---|---|
| `magic`, `version` | S3, `amb3/cfg` | 0xA838, 7 | The blob header (the version is printed as a comment in the file). | the firmware |
| `lastAngle` | S3, `amb3/cfg` | 0 | Last accumulated shaft count; seeds the turn at boot. | Auto, every 10 s when changed, only from a live encoder |
| `idxOnFwd` | S3, `amb3/cfg` | 0 | The homing edge of the index band, zero by definition. An import line `idxOnFwd=...` is refused as unknown. | the band calibration (always 0) |
| `wifiTxQ` | S3, `amb3/cfg` | 8 (2.0 dBm) | The learned WiFi transmit power in raw quarter-dBm: the rung of the join ladder that last worked (chapter 8). The ladder tops out at 60 (15 dBm), a firmware choice: measured here on 2026-09-24, this board radiated nothing usable at 20 dBm. Printed by the console dump `D`; not in the portal and not in the settings file since v.1.0.2 (2026-09-25); an old file's `wifiTxQ` line is ignored. | Auto (copied from `net.cpp` every `loop()` pass), C3 `B` (cycles a list of powers) |
| `autoConnect` | A32, inside the `amb/bt` blob | 0 | A field of `ProtoBtCfg` that nothing reads: the A32 always calls `set_auto_reconnect(false)`, so the radio never starts a Bluetooth connection. Kept in the struct so the wire and blob layouts do not change; no longer a row since v.1.0.2 (2026-09-25); an old file's `autoConnect` line is ignored. | nothing |
| `potMin`, `potMid`, `potMax` | A32, `amb` keys | 60, 0 (= not measured), 3990 | Raw ADC readings of the knob at its minimum, mechanical centre and maximum; a three-point map to 0..255. Exported only as the comment `# potcal`. If max is not at least 65 above min, the defaults are used. | X `pot.min`, `pot.ctr`, `pot.max` (Audio tab), C32 `p`, `c`, `P` |
| `ssid`, `pass`, `ntp`, `tz` | S3, `net` | empty, empty, a public NTP pool, a compiled time zone | The house network credentials and time settings. Never exported. | portal Network card, C3 `y` |
| `users` | S3, `auth` | one administrator account with a compiled default password that the portal forces you to change | Portal accounts, salted and hashed. Never exported. | portal account cards |

The pot defaults 60 and 3990 are the fallback readings of this machine's volume knob, about 60 raw
at one end and about 3990 at the other on the A32's ADC; they apply until the knob is calibrated
(the pot's wiring is Bible §7).

### 7.3.6 Deliberately not settings

These are portal **actions** that change behaviour until the next reboot and are never saved:
`sys.pot` (ignore the knob), `sys.dindrv`, `sys.clkdrv`, `sys.clkpair`, `sys.tail`,
`sys.zfloor`, `sys.dctest`, `sys.dcword`, `sys.bttx`, `sys.quiet`, `disp.test`. The reason is
written above `sys.zfloor` in `doAction()`: "an experiment that survives a reboot is one whose
result gets attributed to something else a week later. If it turns out to be the fix it becomes
a setting then, with a reason attached." One such experiment, `sys.sddly` (2026-09-10 to 09-11),
sent the radio to full volume here and is retired (chapter 8). Chapter 8 lists every action; chapter 10 describes the
audio experiments, and chapter 4 the display test (`disp.test`).

### 7.3.7 Constants of the settings system

| name | value | where | meaning |
|---|---|---|---|
| `SETTINGS_MAGIC` | 0xA838 | `main.cpp` | Marks a blob with a header (V2 and later). |
| `SETTINGS_VERSION` | 7 | `main.cpp` | The current S3 layout. |
| `sizeof(Settings)` | 220 bytes | `main.cpp` | The blob size. |
| `TUNE_MARKS` | 12 | `main.cpp` | Tuning sample slots (3 by hand, 9 automatic). |
| `CFG_SPURS` | 16 | `main.cpp` | Fixed-feature slots, frozen into the layout. `Rda::MAX_SPURS` must equal it (a `static_assert` checks). |
| `PROVISIONAL_LIMIT_HS` | 300 | `main.cpp` | Default and purge soft limits, +/- hs. |
| S3 save debounce | 2000 ms | `settingsFlush()` | Quiet time after the last touch. |
| `lastAngle` save period | 10 s | `loop()` | |
| force-flush retries | 40, 25 ms apart when the other core is writing | `settingsForceFlush()` | |
| reboot save retries | 15, 100 ms apart | `hReboot()` | |
| A32 save debounce | 2000 ms | `settingsFlush()` (A32) | |
| A32 trial hold ends | 60 s after the first HELLO, or 5 min if the S3 never spoke | `CONFIRM_AFTER_HELLO_MS`, `CONFIRM_AFTER_MS` (A32) | Held changes are written at confirmation (7.2.8, 9.3.1). |
| knob deadband | 24 raw counts | `POT_DEADBAND` (A32) | |
| upload timeout | 20 s | `portal.html` | |
| import note | 512 bytes; each list names 5 keys of up to 20 characters | `settings_table.h` | |
| mirror payload limit | 1088 bytes | `PROTO_MAX_PAYLOAD`, `proto.h` | Checked by `static_assert` for `ProtoAudio`, `ProtoBtCfg`, `ProtoCfgAll`. |

## 7.4 Decisions

Each decision below was right for this build; your mileage may vary (§0.3).

**Decision.** One table describes every setting and generates the schema, the widgets, the
clamp and the file.
Why. Four hand-kept copies drift apart; the table makes every knob and slider one could want
affordable.
Rejected. A hand-written HTML control and handler per setting.
When and evidence. The portal, 2026-08-31 (before the repository); `settings_api.h`.

**Decision.** Portal roles split by damage, not secrecy: a guest sees every value and may move
only volume, mute and the five brightness levels.
Why. Those are obvious and reversible; the rest changes what the machine is.
Rejected. Hiding values from guests.
When and evidence. 2026-08-31, header of `settings_table.h`.

**Decision.** The machine bounds every value; the browser's limits are only a suggestion. One
bounding function serves the portal and the file, and refuses NaN.
Why. "A posMax of 30000 would drive the needle into its stop." A second copy of the rule once
guarded only the file path, so the portal could store an enum value of 99.
Rejected. Validation in the page; two copies of the rule.
When and evidence. `settingBound()`, `settingSet()`.

**Decision.** Numbers are parsed whole; one decimal comma is accepted as the point; anything else
is refused.
Why. The old parser read "107,3" as 107 and "" as 0, and the clamp then stored a bound (an IF of
10.00 MHz, a soft limit at the index).
Rejected. `String::toFloat()` and `toInt()`.
When and evidence. 2026-09-24; `parseNumber()`, `argNumber()`.

**Decision.** Refused and clamped lines are named, in their own lists, placed last, and sent in a
separate `warn` field so the page cannot paint them as success.
Why. A partial apply reported as "41 settings applied" in green is a false reassurance.
Rejected. Folding refusals into the success message.
When and evidence. 2026-09-24; `hPutFile()`.

**Decision.** The S3 struct is versioned and append-only, migrated on load, and dispatched on its
header, never on its size.
Why. The earlier scheme discarded everything on any layout change, "right for a bench rig and
wrong for a machine somebody has spent an evening calibrating". Size could not identify a version:
V1, V2 and V3 were all 100 bytes.
Rejected. Size-based dispatch; an exact-size read gate (it would have discarded every blob from V4
on).
When and evidence. 2026-08-31 onward; `settingsLoad()`.

**Decision.** Store tuning samples, not curve coefficients, and refit on every `applySettings()`.
Why. Samples describe themselves, can be refitted by a better model without a migration, and no
float of the curve goes into flash.
Rejected. Stored coefficients.
When and evidence. 2026-09-04 (V5). Part of the tuning design of 2026-09-04: samples are
stored, not coefficients, so a better model later needs no migration.

**Decision.** Writes wait two seconds after the last change and never happen while the needle
moves, a calibration runs or an update is written.
Why. An NVS write stops everything outside IRAM; measured 383 µs of step jitter during a write
against 49 µs with the guard.
Rejected. Writing on every change.
When and evidence. 2026-08-31; `writeSafe()`.

**Decision.** "Unsaved" is derived from two counts; one writer at a time; the blob is written from
a snapshot.
Why. A stored flag cleared around the write lost changes that landed during it.
Rejected. A dirty flag.
When and evidence. 2026-09-25.

**Decision.** "Save now" tells the truth, and a reboot refuses to run over an unsaved change.
Why. "Saved" followed by "rebooting" over a refused save was a data-loss path.
Rejected. A save that returned nothing; a reboot that ignored the save.
When and evidence. 2026-09-01; 2026-09-25.

**Decision.** Nothing is written to flash while a newly updated S3 image is on trial. "Save now"
says why it is held, and "Reboot the main board" goes through at once during the trial.
Why. An image that changes the settings layout migrates at boot and used to save the new layout
within seconds; rolled back afterwards, the previous image found a blob newer than it knew and
locked itself onto defaults (the downgrade lock, 7.2.7). Holding every write until the image is
confirmed closes that path by design. A reboot during the trial is a rollback, and nothing is
waiting to be saved, so refusing it would only take away the one button that undoes a bad update.
Rejected. Documenting the trap for the person who bumps `SETTINGS_VERSION` (the rule until
v.1.0.1).
When and evidence. v.1.0.1, 2026-09-25 (tag v.1.0.1). Proven live the same evening: a portal
reboot on trial answered "rebooting - this firmware was still on trial, so the previous firmware
comes back", the S3 came back on the previous image ("ROLLED BACK"), and no settings were saved
during the trial; re-flashed, it confirmed itself after about a minute.

**Decision.** The A32 writes nothing either while a newly updated A32 image is on trial. Changes
wait in RAM and are written the moment the image confirms itself, and the A32 says so on the link.
Why. The S3's hold of v.1.0.1 had not been carried to the A32. An A32 update that changed the size
of `ProtoAudio` or `ProtoBtCfg` would have saved its new layout during the trial; rolled back, the
previous image would have found a structure of the wrong size and run it on its defaults (the A32
has no migration, 7.2.8). A restart during the trial is a rollback, so nothing held back is worth
keeping across it.
Rejected. Advising an export of the settings file before any update that changes an A32 struct
(the rule until v.1.0.4).
When and evidence. v.1.0.4, 2026-09-26 (tag v.1.0.4, A32 only). Proven live the same morning, A32
image v.1.20260926T113302 on trial: `btLedOff` was set from 255 to 254 through the portal (answered
"ok"); the A32 wrote nothing until 11:35:40, when it logged "settings changed during the trial are
now saved" together with "image confirmed (a minute of running with the S3)". `btLedOff` was then
put back to 255.

**Decision.** A blob from a newer firmware locks all writes; export is refused while locked; only a
complete file lifts the lock.
Why. Before the lock, the first automatic save wrote defaults over the newer blob; a one-line
upload then did the same.
Rejected. Writing defaults back; lifting the lock on any upload.
When and evidence. 2026-09-24, 2026-09-25. The author does not
expect to downgrade; the download refusal is a one-line guard.

**Decision.** The A32 owns and saves its settings; the S3 mirrors them, refreshed at every
handshake, and never pushes a mirror it has not read.
Why. A rebooted A32 must keep its own values; an unread mirror is all zeros.
Rejected. The S3 as the single store for both boards.
When and evidence. First commit, 2026-09-01; `ProtoCfgAll`, `settingSet()`.

**Decision.** When the A32 falls silent, the S3 forgets its copy of the A32's settings
(`haveCfg = false`) until the next handshake; portal edits of A32 rows are refused with 409.
Why. The copy used to count as known for the rest of the S3's boot: edits made while the A32 was
silent went only into the mirror, were overwritten when it came back, and the portal said "ok"; a
download wrote the stale copy instead of `n/a`.
When and evidence. 2026-09-25. Not yet exercised on the radio.

**Decision.** The A32 counts a save only when every write reached flash, and retries a failure.
Why. Its flag used to drop before the write and every result was ignored, so a failed write said
"saved" and the change was gone at the next reboot. The S3 had been fixed the same way on
2026-09-24.
When and evidence. 2026-09-25. Not yet exercised on the radio (it needs a failing flash).

**Decision.** When the A32 has not answered, the file carries `n/a` for its rows, and the import
names them.
Why. A file of zeros, uploaded later, would silence the radio and switch off every LED, and the
A32 would save it.
When and evidence. 2026-09-25. The portal sees the entries are not valid, does not
overwrite the previous ones, and states that this has been detected.

**Decision.** The A32 never restores mute; it restores volume, but the knob's first reading wins.
Why. On 2026-08-28 a saved mute made the set boot silent with nothing on the front to say why.
When and evidence. `settingsLoad()` and `updateVolumePot()` on the A32.

**Decision.** The A32 saves before a commanded restart, except while its image is on trial.
Why. "Reboot the audio board" within two seconds of a change lost it. During a trial the restart is
a rollback, and the trial hold (above) writes nothing.
When and evidence. 2026-09-25; the trial exception since v.1.0.4, 2026-09-26.

**Decision.** The tuning marks, their mask and the hand offset commit from a file as one unit;
`tuneUsed` and `spurUsed` are always exported.
Why. Wiping on the first `tuneUsed` line let a truncated upload erase the calibration with its
only backup; an offset belongs to the curve it corrected.
When and evidence. 2026-09-24.

**Decision.** The user's hand offset (the dial correction) is never erased automatically: no
sample, hand mark, drop or clear touches it. Only the portal button *Reset the dial correction to
0* (`tune.nudgeZero`) or a settings-file restore changes it besides `tune.nudge`.
Why. It is the user's calibration, made by ear; whether it still fits is the user's judgement
(chapter 6).
Rejected. Zeroing it on every sample (until 2026-09-24), then on a requested sample, a
hand mark, a drop and a clear (until 2026-09-25).
When and evidence. 2026-09-25.

**Decision.** `idxOnFwd` is not a row.
Why. It is zero by definition, and an edited value moved both soft limits physically into a stop.
When and evidence. 2026-09-24.

**Decision.** The three other index-band values are rows, so the file carries them.
Why. A restore onto blank flash left them at zero, and every index crossing then read as a slip.
When and evidence. 2026-09-24.

**Decision.** `bandLow`/`bandHigh` stopped being rows but stay in the file.
Why. Nothing reads them once two marks exist, and an editable "Tuner reaches" field was ignored;
but with no marks stored they are the live mapping, so a backup must keep them.
When and evidence. 2026-09-04.

**Decision.** The WiFi transmit power (`wifiTxQ`) is learned state: stored in raw quarter-dBm,
stepped from the S3 console (`B`) and printed by the dump `D`, but neither a portal row nor a line
of the settings file. The ceiling is 15 dBm (60 quarter-dBm), a firmware choice.
Why. It is what the join ladder discovered, not something anybody sets. As a scaled row the file
once said `2.000` while the dump said `8`, and a value that only reached `cfg` was copied back over
by the driver's. The ceiling: measured here on 2026-09-24, this board radiated nothing usable at
20 dBm.
Rejected. A portal row (before 2026-09-07); a file line pushed to the driver on import
(2026-09-24, until v.1.0.2).
When and evidence. 2026-09-07; v.1.0.2 (2026-09-25) took it out of the portal and the
file and kept it on the console. The 15 dBm ceiling is treated as a firmware choice because the
fault behind it is believed to be in firmware.

**Decision.** The `autoConnect` row is gone; the field stays in the A32's struct, unread.
Why. No code ever read it: the A32 always calls `set_auto_reconnect(false)`. A row that does
nothing is a false control. The field stays so the wire struct and the A32's stored blob keep their
size (a size change would reset every Bluetooth setting to its default, 7.2.8).
When and evidence. 2026-09-25. An old file's `autoConnect` line is ignored.

**Decision.** The IF is a setting, stored in 50 kHz units.
Why. The RDA readings of 2026-09-22 (10.60, once 10.65) cannot be written in tenths, and the IF
moves if the IF coils are realigned. The value itself is the tube set's fitted IF, 10.6 MHz (Bible
§22).
When and evidence. 2026-09-22.

**Decision.** The settings file is plain text, one `key=value` per line.
Why. The file must be recoverable by hand and readable in ten years by something that is not
this firmware (recorded in `settings_api.h`). A restore is the only undo, so
the backup must outlive the tools that wrote it.
When and evidence. 2026-08-31 (the portal), `settingsToText()` and `settingsFromText()`.

**Decision.** Experiments are actions, never saved.
When and evidence. `doAction()`.

**Decision.** Credentials and accounts are not in the settings file; a restore does not restore
accounts.
Why. The file is a backup that gets copied, pasted and kept anywhere; the house network's
passphrase and the portal's password hashes stay on the machine. Losing the accounts on a restore
was judged the correct trade (`~` on the S3 console erases every account and restores the default administrator, chapter 8).
When and evidence. `settingsToText()`; `net` and `auth` namespaces.

**Decision.** No lock around `cfg` across the two cores.
Why. The races left are microseconds wide; the refactor would touch every writer.
Rejected. A cross-core settings lock.
When and evidence. 2026-09-25; accepted as it is.

## 7.5 Failures and recovery

Classes use the recovery ladder of §0.5 (BLOCKER, DEFECT, NOTE).

| what fails | what the firmware sees | what it does | how to recover | class |
|---|---|---|---|---|
| S3 NVS write fails | `begin` or `putBytes` returns failure | Stays unsaved, retries after 2 s, console warning; Save and Reboot report it. | Automatic. | NOTE |
| A32 NVS write fails | `begin` or a write's byte count is short | Stays unsaved, retries after 2 s; "settings NOT saved - the flash write failed; retrying" on its console and relayed to the S3's. A forced save before a commanded restart is not retried: the board restarts anyway. Never exercised on the radio. | Automatic, except before a restart. | NOTE |
| Power lost within 2 s of a change | | The change is lost; the old value is intact. | Make the change again. | NOTE |
| Power lost during the trial minute after an S3 update | | Nothing was written during the trial, so every change made in it is lost; the flash still holds the settings from before the update. | Make the changes again. | NOTE |
| Power lost during a write | | Relies on ESP-IDF's NVS keeping the previous value when a write is interrupted; never tested here. Not known: whether an interrupted write always leaves the previous blob readable; that is library behaviour, not settled by this firmware's code. | | NOTE |
| Power lost just after a background save caught a half-applied upload | | Half the upload is on flash (accepted race, 7.2.10). | Upload the file again. | BLOCKER, accepted |
| Newer settings on flash (downgrade) | Our magic, unknown version | Downgrade lock: defaults in RAM, nothing written, download refused, pill shown. | Flash the newer firmware, or upload a complete file. | DEFECT |
| Stored blob shorter than 100 bytes | Length check | Warning, defaults, no lock; the defaults are saved on the first change. | Upload a settings file. | BLOCKER (never seen) |
| Header corrupted so `magic` differs | Magic mismatch in `settingsLoad()` | Read on the Version 1 layout, marked dirty and saved back as V7: the motion profile would be garbage. Not known: whether ESP-IDF's NVS checksums reject such a blob first, so that it reads as "no stored settings"; that is library behaviour, never tested here. | Upload a settings file. | BLOCKER (theoretical) |
| Corrupt soft-limit pair at boot | Pair does not bracket 0 | Purged to -300..+300 and saved; console asks for recalibration. | Re-home, band calibration, set the stops. | DEFECT |
| Corrupt fixed-feature list at boot | Count or entry out of range | Emptied and saved. | Re-learn with the set switched off (C3 `o`). | DEFECT |
| A file with valid but wrong values uploaded | Nothing | Applied and saved; it is the user's act. | Upload a good file. | DEFECT |
| A truncated or damaged file uploaded | Missing or bad lines | What arrived is merged; the tuning and fixed-feature blocks are refused unless complete; everything refused is named. | Upload the file again. | NOTE |
| A soft-limit pair in a file that does not bracket the index | `setGeometry()` refuses | The old pair is kept and the note says so. | Correct the file. | NOTE |
| A32 silent since the S3 booted | `haveCfg` false | Portal refuses A32 rows with 409, "the audio board is not answering - its settings cannot be changed now"; export writes `n/a`; import names "NOT SENT". Never exercised on the radio. | Restore the link; the mirror arrives at the handshake. | NOTE |
| A32 falls silent after it had answered | The link stops hearing it; `haveCfg` is cleared | As the row above: 409 on A32 rows, `n/a` in a download, "NOT SENT" on an upload. Never exercised on the radio. | Restore the link; the next handshake fetches the settings again. | NOTE |
| A32 restarts | New HELLO, or its uptime going backwards | Handshake again; the mirror is refreshed from the A32's flash. | Automatic. | NOTE |
| An A32 update changes `ProtoAudio` or `ProtoBtCfg` | Stored length differs from the struct | That struct's settings reset to defaults and are saved on the next change (after an update over the air, not before the new image is confirmed). The pot calibration survives. | Upload a file exported before the update. | BLOCKER by design; avoidable by exporting first |
| Boards on mismatched builds (half-flashed pair) | `ProtoFramer::as()` rejects on length; or, across a `PROTO_VERSION` change, every frame is dropped and counted | The A32 ignores `SET_AUDIO`/`SET_BT`; the S3 never gets `MSG_CFG` (the `n/a` path). A version mismatch shows as "wrong-version N" in console `s` and as the portal pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both". | Flash both boards. | DEFECT |
| Reboot pressed with an unsaved change while the needle moves | Save refused | Needle stopped, save retried for 1.5 s; reboot refused with the reason if still unsaved; needle resumed. | Try again. | NOTE |
| S3 update ends with an unsaved change that cannot be written | Force-flush false | Reboots anyway; warning on the USB console only. | Make the change again. | NOTE |
| An S3 update that bumped `SETTINGS_VERSION` is rolled back | Nothing: the new image wrote nothing during its trial | The previous image reads its own blob normally; no lock. Closed by design since v.1.0.1. | - | NOTE |
| An A32 update that changed `ProtoAudio` or `ProtoBtCfg` is rolled back | Nothing: the new image wrote nothing during its trial | The previous image reads its own blobs normally. Closed by design since v.1.0.4. | - | NOTE |
| Power lost, or the A32 restarted, during the A32's trial minute after an A32 update | | Nothing was written during the trial, so every A32 change made in it is lost (a restart on trial is also a rollback); the flash still holds the A32 settings from before the update. | Make the changes again. | NOTE |
| Flash wear | | Writes happen only on a change, two seconds after it, never during motion. The most frequent writer is `lastAngle`, at most once every 10 s and only after the dial moved. NVS spreads its entries across its pages. At a handful of writes per listening session, wear is not a practical concern (an estimate, not measured). | - | NOTE |

## 7.6 Graveyard

These were dead ends on this machine. They are not necessarily dead ends elsewhere.

- **Rejecting a blob on any size change.** Tried until 2026-08-31. Here, any layout change
  silently replaced a calibrated machine's settings with defaults. Dropped for versioned
  migration.
- **Deciding the version by the blob's size.** V1, V2 and V3 were all 100 bytes, so the three
  tests were one test and the V1 branch caught everything. Found by review, not by failure.
  Dropped for dispatch on the header.
- **Reading only a blob of exactly the current size.** It would have discarded every migratable
  blob from V4 on. Dropped for "read up to our size".
- **Prefix copies by blob length.** A new field can sit in the old struct's padding (V2 to V3).
  Dropped for `offsetof` cuts.
- **A stored "dirty" flag**, cleared before and then after the write. Here, changes that landed
  during a write were marked saved and lost. Dropped for the two counts (2026-09-24, 2026-09-25).
- **A save that returned nothing** and printed "saved" regardless. Dropped (2026-09-01).
- **Purging corrupt soft limits inside `applySettings()`.** It fired mid-edit and overwrote what
  had just been typed. Moved to boot.
- **Soft-limit ranges -3000..0 and 0..3000.** They assumed a centred index; clamping instead of
  refusing made the high limit silently unsettable while it was negative (2026-09-02). Dropped for
  one wide range and validation by `setGeometry()`.
- **Two copies of the bounding rule.** The second guarded only the file. Dropped for
  `settingBound()`.
- **`bandLow`/`bandHigh` as editable "Tuner reaches" rows.** The fit ignored them, so the author
  typed corrections into a field that was discarded. Replaced by the curve's computed reach.
- **`idxOnFwd` as a row.** An edit moved both soft limits into a stop.
- **`wifiTxQ` as a row with scale 0.25** (working tree only, before 2026-09-07). Two spellings of one
  key; a pasted dump line would have been read as four times the power.
- **`wifiTxQ` in the settings file, pushed to the driver on import** (2026-09-24 to 2026-09-25). It was
  learned state that nobody sets; it was taken out of the portal and the file and kept on the
  console.
- **The `autoConnect` row** ("Chase the phone (not advised)"). Stored, shown, exported and
  imported, and read by nothing. Removed as a row (in v.1.0.2); the struct field stays.
- **The console keys `,` and `.`** (up-leg top speed -100/+100). `.` had no ceiling and bypassed the
  row's bounds; removed (in v.1.0.2), the portal row does the job inside its range.
- **The mirror counted as known for the whole boot** once the A32 had answered. Here a silent A32
  took portal edits into the mirror only, and a download wrote the stale copy. Dropped for
  forgetting it on silence (in v.1.0.2).
- **The A32 clearing its dirty flag before writing and ignoring the results.** A failed write said
  "saved". Dropped for the S3's rule (in v.1.0.2).
- **Saving during an S3 trial**, with a documented trap for whoever bumps `SETTINGS_VERSION`. A
  rollback after such a save left the previous image locked on defaults. Dropped for holding every
  write until the image is confirmed (v.1.0.1).
- **Saving during an A32 trial**, with the advice to export a settings file before any update
  that changes an A32 struct. A rollback after such a save would have left the previous image on
  defaults for that struct. Dropped for the same hold as the S3's (v.1.0.4).
- **Zeroing the hand offset on a hand mark, a drop, a clear or a requested sample.** It erased the
  user's correction unasked. Dropped for the reset button (in v.1.0.2).
- **Wiping the marks on seeing `tuneUsed`, and applying `tuneOffset10` at once.** A truncated file
  erased the calibration, or slid an offset onto the wrong curve.
- **`String::toInt()` for hand-parsed keys.** It answers 0 for garbage, and 0 is the destructive
  value for `tuneUsed`, `spurUsed` and `tunerEndsSet`.
- **One bit for both tuner ends.** One measured end declared both measured, and the fit then ran
  over a domain far wider than the shaft.
- **Exporting zeros for a silent A32.** Dropped for `n/a` (2026-09-25).
- **Lifting the downgrade lock on any upload.** Dropped for the complete-file rule (2026-09-24).
- **Saving the A32's mute.** The set booted silent on 2026-08-28.
- **Refusals inside the success message.** Painted green. Dropped for the `warn` field.

## 7.7 Limits and firmware notes

**Never exercised on the radio.** The `n/a` export and the 409 refusal of A32 rows (they need the
A32 silent), the downgrade lock (the author does not downgrade), and with them the complete-file
unlock, the "NOT SENT" note and the locked download refusal; an A32 save failure (§12.3.16). Exercised: export
(2026-09-24), upload (the 2026-09-24 transmit-power test through the file, before `wifiTxQ` left
it), migrations up to V7, the S3's trial hold (2026-09-25: a portal reboot on trial rolled back
with nothing saved), and the A32's trial hold (2026-09-26: a change made on trial was written only
at confirmation, with its log line). Not exercised: an A32 rollback with a change held in RAM, and a
failed write at the A32's confirmation.

**Known limits in the code:**

1. The A32 has no migration: any change to `ProtoAudio` or `ProtoBtCfg` resets that struct's
   stored settings to defaults on the first boot of the new image (§12.4.3). Since v.1.0.4 a
   rollback no longer adds to this (item 2), but the forward update still does: export a settings
   file before any update that changes an A32 struct, and upload it after.
2. **Rollback.** Neither board writes settings while its own image is on trial (7.2.6, 7.2.8), so a
   rollback of either board always finds blobs the previous image can read.
3. A change made during either board's trial minute exists only in RAM until that image confirms
   itself; a power cut in that minute loses it (the accepted cost of 7.2.6 and 7.2.8). On the A32,
   so does any restart in that minute, since a restart on trial is a rollback. The A32's hold says
   nothing while it lasts: the portal answers "ok" and nothing marks the change as waiting; only
   the log line at confirmation shows it.
4. The A32's forced save before a commanded restart is not retried: if that write fails, the board
   restarts and the change is lost (§12.4.10).
5. Changes made on the A32's console (taper, pot calibration) do not refresh the S3's mirror; the
   next portal edit of any audio row sends the mirror's older taper back. "Re-read the A32"
   (`sys.getcfg`) refreshes it. Bench only (§12.4.11).
6. The knob-set volume is never touched, so it reaches flash only when something else is saved,
   and the knob's first reading overrides it at boot anyway.
7. An unreadable `tuneUsed` line is ignored without a note, so every mark line after it is skipped
   silently (§12.3.8). The retired keys `wifiTxQ` and `autoConnect` are skipped without a note, by
   design.
8. Merge, not replace: an upload cannot return a setting to its default by leaving it out (except
   the two blocks, through their count keys).
9. The file carries no checksum; its version line is a comment and is not checked on import. Old
   files work key by key.
10. `settingsForceFlush()` does not re-check `writeSafe()` between its retries; a write could start
    as the needle starts moving. The window is tiny (§12.3.4).
11. The page has no "unsaved" indicator; the state JSON carries the lock, not the dirty state
    (§12.3.6).
12. No lock around `cfg` across the cores (accepted, 7.2.10, §12.3.5).
13. The console dump `D` is only partly pasteable into a file: its first lines carry several
    `k=v` pairs each, which the importer refuses; the tuning, IF (`ifOffset=`) and fixed-feature
    lines match the file. Its `wifiTxQ` line is ignored by the importer (§12.3.9).
14. S3 console keys bypass `settingBound()`: `-` and `+` (clock brightness) have their own floor and
    ceiling (§12.7.4).
15. The `upVmax` range (up to 4000) allows speeds the author measured to stall the needle through
    the index (7.3.3, §12.2.6).
16. When the page receives a `warn`, it shows only the warning, not the "N settings applied" count
    (§12.3.7).

## 7.8 Changing this area

### 7.8.1 Invariants

- `Settings` is append-only. Every append bumps `SETTINGS_VERSION` and adds a prefix migration.
  The version comes from the header; size only bounds the read.
- Every `gSet` row's `store` code matches the C type of the field it points at. All access goes
  through `memcpy` (the A32 structs are packed).
- `settingBound()` is the only bounding rule. Never add a second copy in a caller.
- `settingsTouch()` is called **after** every change to `cfg` that must persist, from either core.
  A change without it is lost at power-off.
- `settingsWrite()` is the only writer of `amb3/cfg`. Nothing writes during motion, calibration or
  an update (`writeSafe()`).
- A32 rows are never pushed before `haveCfg`; never export zeros for them.
- A multi-field calibration commits as a unit or not at all. Count keys (`tuneUsed`, `spurUsed`)
  are exported even when zero.
- Refusals reach the page in the `warn` channel, never in the success message.
- Hand-parsed keys use full-consumption `strtol`/`strtof` (the whole field must be consumed), never
  `toInt()` or `toFloat()`.
- `applySettings()` stays idempotent and cheap, and writes back the accepted soft limits.

### 7.8.2 Traps

- Version 1 is recognised by the **absence** of the magic. Do not add a size test.
- A new field may land in trailing padding: the prefix cut must be `offsetof` the new field.
- `CFG_SPURS` (16) is frozen into the layout. `Rda::MAX_SPURS` must equal it; a `static_assert`
  in main.cpp stops the build otherwise.
- `loop()` copies the driver's transmit power into `cfg.wifiTxQ` on every pass. Any path that sets
  `wifiTxQ` must also push it to the driver (`applySettings()` does, through `Net::setTxPower()`),
  or it is reverted at once.
- A retired key must stay in the importer's skip list (`wifiTxQ`, `autoConnect`), or every old file
  reports it as REFUSED.
- `calLow`/`calHigh` are rows **and** feed the curve fit. Writing them sets `tunerEndsSet` bits
  (`settingSet`) or infers them (file). The values 0 and 6023 mean "not measured".
- `posMin`/`posMax` are validated by `Needle::setGeometry()`, not by the row range.
- The portal task runs on core 0; anything that busy-waits there freezes the portal.
- `settingsDump()` is written by hand; it is not generated from the table.
- In the file, `tuneUsed` must come before its `tuneMark` lines and `spurUsed` before its `spur`
  lines.
- **A version bump and the rollback.** This trap is closed by design on the S3: an image on trial
  writes nothing (7.2.6), so a migrated blob reaches flash only after the new image has confirmed
  itself, and a rolled-back image finds its own layout. Keep the trial check first in
  `settingsWrite()`; any other path that writes the `amb3` blob would reopen the trap. The A32 has
  the same hold since v.1.0.4 (7.2.8): keep the trial check in its `settingsFlush()` ahead of the
  write, never write the `amb` namespace from anywhere else, and keep the flush in
  `confirmImage()`, or the changes held during a trial are written only by the next change.

### 7.8.3 How to add an S3 setting

1. Decide what it is. A value that persists, is one scalar and has one range becomes a row. An
   experiment becomes an action (7.3.6). A list, a learned value, or several fields that must
   commit together becomes a hand-written key (step 8).
2. Append the field at the **end** of `struct Settings` in `src/s3/main.cpp`, with its default as
   the initializer. That default is what every existing machine gets on its first boot with the new
   firmware. Never insert, reorder, widen or remove. (To grow the fixed-feature list, append a second
   array.)
3. Bump `SETTINGS_VERSION` (7 to 8).
4. Add a migration branch for the old version in `settingsLoad()`, next to the others:

   ```cpp
   if (ver == 7) {
     size_t cut = offsetof(Settings, newField);
     if (got < cut) cut = got;
     memcpy(&cfg, blob, cut);
     cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
     gTouchGen++; dirtyAt = millis();      // boot, single-threaded
     Con.println(F("  settings MIGRATED from version 7 - <newField> starts at <default>."));
     return;
   }
   ```

   The older prefix branches need no change: their cuts stop earlier, so the new field keeps its
   default. Never dispatch on size.
5. Add a row to `gSet[]` in `settings_table.h` with `S_(...)` (or `E_(...)` for a choice): a key
   that will never change, label, unit, tab, a `store` code matching the field's exact C type, kind,
   `lo`/`hi`/`step` in shown units, `scale`, `&cfg.newField`, `OWN_S3`, and `admin = 1` unless the
   value is obvious and reversible. The row's position is its position on screen and in the file.
6. Consume it in `applySettings()`: push it to its module. `settingSet()` already calls
   `applySettings()` for every S3 row.
7. Add it to `settingsDump()` by hand.
8. For a hand-written key instead of a row: write it in `settingsToText()` after the rows; parse it
   in `settingsFromText()` before `settingFind()`, with full-consumption parsing; decide whether a
   bad value is refused (and named with `noteRefusedKey()`) or ignored; for a multi-field unit,
   parse into locals and commit at the end; write a count key even when zero; add it to the dump
   byte for byte as the file writes it.
9. Decide how the downgrade lock treats it. A new row is automatically required by the
   complete-file check (every `OWN_S3` row). A hand-written key is required only if you add it to
   that check. A file from the previous version lacks the new row and cannot unlock a lock.
10. Nothing in the page or `portal.cpp` changes: the schema is generated, and its cache tag changes
    with the firmware version.
11. Build both environments and test (7.8.5).

### 7.8.4 How to add an A32 setting

1. Append the field at the end of `ProtoAudio` or `ProtoBtCfg` in `include/proto.h`. This changes a
   **wire struct**: a board running the other build rejects `SET_AUDIO`, `SET_BT` and `CFG` frames
   on length, so flash both boards. The `static_assert`s check that the struct still fits
   `PROTO_MAX_PAYLOAD` (1088 bytes).
2. Do **not** bump `PROTO_VERSION` for an appended field. The rule is written in `include/proto.h`
   ("WHEN PROTO_VERSION CHANGES"): a struct that changes
   size needs no bump, because `ProtoFramer::as()` refuses a payload of the wrong length, so a
   mismatched pair loses only that message type and the link stays up. Bump only for a change
   `as()` cannot see (same size, different layout or meaning). A bump drops every frame of the other
   version, which costs the handshake and with it the relayed A32 update route; it then shows as
   "wrong-version N" in console `s` and as a portal pill. Update the A32 first, through the S3 while
   they still match.
3. The A32's stored `audio` or `bt` blob no longer matches the struct's size, so **every** setting in
   that struct resets to its default on the first boot (7.2.8). Download a settings file before the
   update and upload it after; the A32 rows are pushed once the mirror has been read. If the update
   came over the air, the upload is applied at once but written only when the new A32 image
   confirms itself, about a minute after its first handshake (7.2.8); should it be rolled back
   instead, the previous image finds its own blobs untouched.
4. Give it a default in `settingsDefaults()` and apply it in `settingsApply()` (both in
   `src/a32/main.cpp`). Add any clamp the A32 itself needs when applying.
5. Add a row to `gSet[]` pointing at `&a32cfg.audio.newField` or `&a32cfg.bt.newField`, owner
   `OWN_A32_AUDIO` or `OWN_A32_BT`, with a `store` code matching the field.

### 7.8.5 How to test a change

Without the real machine where possible:

1. Build both environments.
2. Round trip: download `ambersong.txt`, upload it unchanged; expect "N settings applied" and no
   warning.
3. Refusals: upload `posMax=abc`, `nosuchkey=1`, `volume=999`, `wn=10,5`; expect REFUSED (`posMax`,
   `nosuchkey`), CLAMPED (`volume`), and `wn` accepted as 10.5.
4. Atomic blocks: `tuneUsed=0x7` with only two `tuneMark` lines gives "TUNING MARKS REFUSED ...
   announced 3 and 2 parsed" and the stored marks unchanged. The same for `spurUsed`.
5. Soft limits: `posMin=100` gives "SOFT LIMITS REFUSED".
6. Save path: change a needle value while the needle moves and press Save; expect the motion
   refusal; when it stops, "saved". The console prints "settings saved." about two seconds after a
   change.
7. Migration: flash the new build over a board holding the previous version; expect the "MIGRATED"
   line, and compare `D` dumps before and after. Over the air, expect no "settings saved." until the
   console prints "image confirmed".
8. Trial hold: right after an S3 update over the air, change a setting and press Save; expect "NOT
   saved yet - this firmware is on trial ...". A portal reboot then answers "rebooting - this
   firmware was still on trial, so the previous firmware comes back".
9. A32 trial hold: right after an A32 update over the air, change an A32 row (a Bluetooth LED
   level is harmless) from the portal; expect "ok", no "settings saved." on the A32's USB console (if connected), and, a minute
   after the handshake, "[A32] settings changed during the trial are now saved" next to "[A32]
   image confirmed (a minute of running with the S3)". Put the value back afterwards.
10. Retired keys: upload a file carrying `wifiTxQ=60` and `autoConnect=1`; expect no REFUSED note and
   no change.
11. Downgrade lock (bench board only): flash an older build by cable over a newer blob; expect the
    pill, a 409 on download, a partial upload that keeps the lock, and a complete upload that lifts
    it.
12. A32 silent: keep the link down at S3 boot, or cut it after the handshake; expect `n/a` in the
    export, "NOT SENT" on upload, and a 409 on a portal edit of an audio row.

On the real machine: identify the S3 by its MAC address before any flash, never flash while
someone is listening or looking, and keep the toolchain pinned (`espressif32@7.0.1`).

---

# 8. Network, web portal and console

## 8.1 What it does

When the cabinet is closed, the web portal is the only way to reach the machine. The radio
has no configuration buttons. The main board (the ESP32-S3, called "the S3" in this book)
joins the house WiFi, serves a password-protected web page on port 80, and shows on that
page everything the machine knows: the live dashboard, every setting, every action, the
settings file, the user accounts, the network settings, a reboot button, firmware updates for
both processors, and a live copy of the S3's serial console that you can also type into. If
the S3 cannot join the house network, it raises its own WiFi network, the **rescue access
point** `Ambersong`, and serves the same portal at `http://192.168.4.1/`, while it keeps
trying to go home in the background. That network is open (no WiFi password), and it acts as a
**captive portal**: most phones that join it open the portal's sign-in page by themselves. The radio also answers at `http://ambersong.local/`.
The S3 takes its time of day from the internet (SNTP) and corrects the audio board's
battery-backed clock with it once an hour. Nothing ordinary should ever need the USB cable;
the cable remains the last way in when everything else has failed.

## 8.2 How it works

### 8.2.1 The parts and where they run

All of this chapter's code runs on the S3. The audio board (the classic ESP32, "the A32") only
appears as the far end of a relayed firmware update and as the keeper of the real-time clock.

| Part | File | Runs in | Period |
|---|---|---|---|
| Network state machine, `Net::loop()` | `src/s3/net.cpp` | the Arduino `loop()` task, core 1 | every pass of `loop()`, called right after the link poll |
| HTTP server, `serverTask()` | `src/s3/portal.cpp` | its own FreeRTOS task "portal", **core 0, priority 3, 8192-byte stack** | one `handleClient()` per pass, then `vTaskDelay(1)` |
| The page | `data/portal.html`, gzipped into `src/s3/page_gz.h` by `scripts/page.py` | the browser | polls every 5 s |
| Console tee `Con` | `src/s3/console.cpp` | any task (it is a locked ring) | on every print |
| Console dispatcher | `loop()` in `src/s3/main.cpp` | Arduino `loop()` | one character per pass |
| SNTP time callback `onSntpSync()` | `src/s3/net.cpp` | the lwIP (TCP/IP stack) task | on every time sync; it only stamps the time |
| Actions dispatcher `doAction()` | `src/s3/settings_table.h` | the portal task (called by `/api/act`) | per request |

Boot order, from `setup()` in `src/s3/main.cpp`: the console `Con` starts first (115200 baud on
the S3's native USB port), then settings, the display, the needle and the rest; then
`Net::begin()` and `Portal::begin()` are the last two things started, after homing has begun.
The console is the S3's native USB port, the socket brought to the back panel (build flag
`ARDUINO_USB_CDC_ON_BOOT=1`; Bible §2, §4).

**Why the server runs on core 0.** Core 1 belongs to the display's multiplexing interrupt and
the needle's timing. An early measurement ("phase G") showed that WiFi plus four parallel
HTTP downloads on core 0 kept the display's worst multiplex slot error under 100 µs. The
finished firmware measured 22 µs idle and 61 µs under eighteen hammering requests. The
asynchronous web server library was never tried on this machine.

**Why the server task always sleeps one tick.** A busy loop on core 0 starves that core's idle
task, and the idle-task watchdog then reboots the machine. That is how an earlier needle emitter
brought the board down on 2026-08-31. So `serverTask()` ends every pass with `vTaskDelay(1)`.

**The cost of this arrangement.** The needle's step emitter also runs on core 0, at priority 19,
and does not yield during a move. So the portal is starved while the needle moves. It was
measured unreachable 36 % of the time while the needle tracked, and one 933 kB update took
293 s. That cost was accepted (see 8.4).

**The cross-task rule.** An HTTP handler runs on the portal task, on core 0. `Net::loop()` runs on
core 1. **A handler never changes the WiFi mode itself.** When one did, it raced the network
loop's own state and tore down the very link its answer still had to travel on (found
2026-09-24). So the two portal actions that touch the radio, `net.forceAp` and `sys.quiet`, only
*ask*: `Net::requestForceAp()` and `Net::requestQuiet()` set a flag and a timestamp.
`Net::loop()` carries the request out on its own core **more than 500 ms later**, so the HTTP
answer leaves first. A network change from the portal is deferred the same way, by **3 s**
(`pendingJoin`, set in `Net::setWifi()`).

### 8.2.2 First-time setup, from a blank board

A blank board has empty NVS. NVS ("non-volatile storage") is the ESP32's key-value store in
flash; this chapter uses two of its namespaces, `net` (WiFi and time settings) and `auth`
(portal accounts). The machine's own settings live elsewhere (chapter 7).

1. **Flash both boards over USB, once.** After that every update can go over the air, the A32's
   included (chapter 3). The A32 has no reset line from the S3, so after its first flash it is
   reached only through the S3's relay (8.2.10). The S3's flash layout is `default_16MB.csv`: two 6.25 MiB (0x640000-byte)
   application slots, so it can update itself from its first flash. The S3 module has 16 MB of
   flash (Bible §1).
2. **The S3 raises its rescue access point.** `Net::begin()` loads the `net` namespace. With no
   network name stored, `startJoin()` goes straight to `startAp()`: the network
   `Ambersong` appears on channel 1. It is an **open network: joining it asks for no
   password** (since v.1.0.5; 8.4). The portal is at `http://192.168.4.1/` and
   `http://ambersong.local/`, and while the access point is up the captive portal (8.2.3)
   makes most phones open the page by themselves. With no network configured the
   retry loop never runs, so the access point stays up for as long as the board is powered.
3. **The portal creates a placeholder account.** `Portal::begin()` finds no account in the
   `auth` namespace and creates the shipped placeholder owner account (`DEFAULT_USER` and
   `DEFAULT_PASS` in `src/s3/portal.cpp`). The console prints its name and password when it does
   this, and the sign-in form's "set your account" page names them too. They are public by
   design.
4. **Sign in and replace the placeholder.** Join `Ambersong` (no password); the sign-in
   page opens by itself on most phones, and otherwise open `http://192.168.4.1/`. Sign in with the
   placeholder. The portal now refuses everything except four addresses (see
   8.2.6) and the page shows only a **"Set your account"** form. Choose a new name and a
   password of at least six characters, typed twice. Only then does the rest of the portal open.
   The form insists on a name other than the placeholder's, because an account still carrying
   the shipped name keeps half the shipped credentials; that rule is the page's, and the radio
   itself would accept a new password alone (8.2.6). After a successful change the page drops
   its cached schema (which carries the old name) and reloads.
5. **Give it the house network.** System tab, Network card: WiFi name, WiFi password, NTP
   server and timezone (a POSIX TZ string; the default is `EST5EDT,M3.2.0,M11.1.0`, Eastern
   time with North American daylight saving — set your own). **Save network** stores them in
   NVS, and **3 seconds later** the S3 drops the access point and joins the house network by
   itself.
6. **On success** the console prints the network name, the address the router gave the radio,
   the received signal and the transmit power that worked. From then on the radio is at its house
   address or at `http://ambersong.local/`.
7. **On failure** (no association within 20 s) the rescue access point comes back, the
   transmit power climbs one rung (8.2.4), and the house network is retried every 2 minutes.

Without a browser, console key `y` (8.2.9) sets the network: type the name, a space and the
password on one line.

The rest of first-time setup (homing, the index band, the dial samples) belongs to chapters 5
and 6.

**This whole path has never been run end to end on the real radio** (8.7).

### 8.2.3 The network state machine (`net.cpp`)

The state is a handful of file-level variables in `src/s3/net.cpp`: `gSta` (on the house
network), `gApUp` (our access point is up), `joining` and `joinStart`, `lastTry` (last attempt to
go home), `pendingJoin` (a scheduled rejoin), `gForced` and `forcedAt` (a forced access point),
`quietUntil` (radio silence), the request flags `gWantAp` and `gWantQuiet`, `gTxQ` (the
transmit power in quarter-dBm), and the captive portal's DNS server `gDns` with its flag
`gDnsUp` (since v.1.0.5).

Words used below: **STA** (station) is the S3 as a client of the house router. **AP** (access
point) is the S3 as its own little network. **AP+STA** is both at once, on one radio.

```
                 no network name stored
   begin() ──► startJoin() ──────────────────────► startAp() ◄───────────────┐
                    │                                  │                      │
                    │ WiFi.begin()                     │ every 2 min, if a    │
                    ▼                                  │ name is stored, no   │
               [ joining ] ── 20 s, no link ──► climb one TX rung ─┘         │
                    │                           (then startAp())             │
                    │ WL_CONNECTED                     │ nobody on the AP    │
                    ▼                                  │ and no forced hold  │
            [ on the house network ] ◄── startJoin() ◄─┘                     │
                    │                                                        │
                    │ link lost ("[WARN] wifi dropped.") ──► startJoin()     │
                    │                                                        │
   net.forceAp / Y ─┴──────────────► forceAp(): hold 10 min ────────────────┘
   sys.quiet ──────► quiet(): WiFi fully off for N s, then startJoin()
```

**`begin()`** loads the `net` namespace (`ssid`, `pass`, `ntp` default `pool.ntp.org`, `tz`
default as above), applies the timezone at once (`setenv("TZ")` and `tzset()`, so local time is
right with no network at all), registers `onWifiEvent`, calls `WiFi.persistent(false)` (the
Arduino library must not keep a second copy of the credentials), sets the hostname
`ambersong`, and calls `startJoin()`.

**`startJoin()`**:
- With no network name stored, it calls `startAp()` and stops.
- If our access point is up **and nobody is connected to it**, it takes the access point down
  for the length of the attempt, with `WiFi.softAPdisconnect(false)`. One radio cannot beacon an
  access point on channel 1 and at the same time hold the router's tight authentication round
  trip on another channel. On 2026-09-05 every retry for half an hour failed that way (the
  router was found; the authentication timed out). If somebody *is* on the access point, it
  stays up and the join runs in AP+STA, the slow way.
- It picks the **strongest** access point carrying the network's name
  (`WIFI_ALL_CHANNEL_SCAN`, `WIFI_CONNECT_AP_BY_SIGNAL`), not the first one heard. The Arduino
  default commits to the first match, which is wrong in a house with a mesh node or an extender.
- It sets the mode (`WIFI_AP_STA` if the access point stayed up, else `WIFI_STA`), re-applies the
  transmit power (every mode change resets it), calls `WiFi.begin()` and starts the 20 s clock.

**`loop()`** does, in this priority order, on every pass:
0. The captive portal's DNS server follows the access point (since v.1.0.5): while `gApUp` is
   set it is started if it is not running, then answers one waiting request
   (`processNextRequest()`); once `gApUp` is clear it is stopped. This runs first, before any of
   the steps below can return.
1. A quiet request more than 500 ms old: `quiet(seconds)`, and return.
2. A force-AP request more than 500 ms old: `forceAp()`, then print whether the WiFi driver
   really holds an access point ("access point is up." or "[WARN] the access point did NOT come
   up - the driver holds no AP.").
3. Radio silence running: return until it ends; then print "radio silence over - rejoining." and
   `startJoin()`.
4. A scheduled rejoin (`pendingJoin`) now due: drop the station if joined, stamp `lastTry`,
   `startJoin()`.
5. Joining:
   - **Connected**: `gSta = true`. If the access point was up, take it down
     (`softAPdisconnect(true)` then `WiFi.mode(WIFI_STA)`, a change from AP+STA that does not
     pass through the "no mode" state). Turn **modem sleep off** (`WiFi.setSleep(WIFI_PS_NONE)`).
     This must happen after the association: asked before `WiFi.begin()`, it is ignored, because
     connecting re-applies power save. With power save on, the radio only listens at each DTIM
     beacon, and TCP handshakes took 0.5–1.0 s and page loads 4–19 s. Print the link, start mDNS
     and SNTP.
   - **20 s without a link** (`JOIN_MS`): climb one rung of the transmit ladder, print "join failed
     at X dBm - trying Y dBm next time.", `startAp()`, stamp `lastTry`.
6. On the access point, with a network name stored, more than 2 minutes since `lastTry`
   (`RETRY_MS`), and not inside a forced hold: clear `gForced`, stamp `lastTry`. **If anyone is
   connected to the access point, skip this attempt** (a retry would tear the access point down
   under the person using it to fix the settings). Otherwise `startJoin()`.
7. Was on the house network and lost it: print "[WARN] wifi dropped." and `startJoin()`.
8. On the house network, every 5 s: mark NTP fresh if SNTP has synced (8.2.5), and **assert
   `WIFI_PS_NONE` again**. Power save came back once after an over-the-air update (pings of
   300–1000 ms, the DTIM interval), and asserting it every 5 s costs nothing.

**`startAp()`**, the rescue access point:
- `WiFi.disconnect(false, false)` (stop chasing the router but keep the radio on), 50 ms,
  `WiFi.mode(WIFI_AP_STA)`, 100 ms. **It never passes through `WIFI_MODE_NULL`.** In
  arduino-esp32 core 2.0.17, going to "no mode" calls `esp_wifi_deinit()`, and the next mode
  change re-initialises onto network-interface objects the library never destroyed
  (arduino-esp32 issue #7232). The symptom is exactly what this machine did for weeks: the AP
  start event fires, `softAP()` returns true, and nothing is on the air.
- Up to 3 attempts of `WiFi.softAP("Ambersong", no passphrase, channel 1, not hidden,
  at most 2 clients)`, 400 ms apart. The passphrase argument is `nullptr`, which makes the network
  open (since v.1.0.5). **A `true` is not believed** unless `apReallyUp()` agrees: it
  asks the driver whether its mode includes AP and whether its AP configuration holds a non-empty
  name. `softAP()`'s return cannot answer that, because the library skips the driver call when the
  new configuration is byte-identical to the old one and returns true anyway.
- `WiFi.enableSTA(false)` (AP+STA to AP, no de-initialisation on this path), then re-apply the
  transmit power.
- On total failure: "[FAIL] the access point will not start. Set the network over the cable with
  'y', or reboot."
- `esp_wifi_set_event_mask(0)`, so the driver reports probe requests (masked by default in IDF).
- It prints what the **driver** holds ("raised and the driver confirms it" or "raised but THE
  DRIVER DOES NOT CONFIRM IT"), then a line `open, no password   http://192.168.4.1/   channel 1
  bssid …` (the address, the channel and the BSSID, the access point's own hardware address), and
  starts mDNS.

**The captive portal** (since v.1.0.5). A phone that joins a WiFi network checks
whether it reaches the internet by fetching a known address (Android asks for `generate_204`,
Apple devices for `hotspot-detect.html`, Windows for `connecttest.txt`, and so on). When the
answer is not the one it expects, it concludes that the network wants a sign-in and opens that
page by itself. Two pieces make the S3 answer that way:
- **The DNS server.** `gDns` in `src/s3/net.cpp` is the Arduino `DNSServer`, started by
  `Net::loop()` as `gDns.start(53, "*", WiFi.softAPIP())`: on port 53, it answers **every** name
  with the access point's own address, 192.168.4.1. It starts on the first pass of `loop()` after
  the access point is up, and prints "captive portal: every name now points at this radio.". It is
  stopped on the first pass after `gApUp` clears (the radio went home, a join attempt took the lone
  access point down, or radio silence began). It never runs on the house network.
- **The catch-all redirect.** The portal's handler for unknown paths (`server.onNotFound` in
  `Portal::begin()`, `src/s3/portal.cpp`) answers 302. On the house network (`Net::isSta()` true)
  the redirect names `/`; otherwise it names `http://192.168.4.1/` in full (built from
  `WiFi.softAPIP()`), because the phone's check arrives under someone else's host name and a bare
  `/` would send it back to that host.

So on the access point, the phone's check goes to the S3 (DNS), asks for a path the portal does
not know, is redirected to `http://192.168.4.1/`, and the phone shows the portal's page. The
portal's own sign-in still guards every action (8.2.6).

**`onWifiEvent()`** prints `[ap] started`, `[ap] stopped`, `[ap] probe request heard, N dBm`,
`[ap] a device ASSOCIATED` and `[ap] a device left`. A count of joined devices cannot tell
"nobody tried" from "somebody tried and failed"; these lines can. Probe requests heard while a
phone scans nearby prove the S3's receiver works on channel 1.

**Two diagnostics, both printed by console `i`** (8.11). `driverReport()` reads back from the
driver what it actually holds: the mode, the transmit power as the driver reports it (flagging
low values; see §12.3.1), the country and channel plan, the access point's configuration, the
beacon interval, the PHY modes, the radio channel, and the access point's address and client
count. It exists because, for months, "`softAP()` returned true" was the only evidence ever held
that the access point transmitted. `scanReport()` runs a blocking scan of every channel and lists
every network heard, marking those that carry the configured name, with channel, signal,
security and BSSID. It separates three failures that look alike from the inside: "I cannot find
the router", "I found it and it refuses me", and "there are three and I picked the far one".

**`forceAp()`**, the rescue hatch tested on purpose (portal action `net.forceAp`, console `Y`):
cancel any pending join, set `gForced`, stamp `forcedAt` **and `lastTry`**, `startAp()`. The
retry loop then leaves the access point alone for `FORCE_HOLD_MS` = 10 minutes, and longer
while somebody is connected (the retry skips while a client is on it, and looks again every
2 minutes). Then it goes home by itself. Stamping `lastTry` is the fix of 2026-09-24:
without it the very next loop pass saw a retry overdue since boot, and the forced access point
lived about one loop tick.

**`quiet(seconds)`**, radio silence (portal action `sys.quiet`): capped at 600 s. It clears all
state, calls `WiFi.disconnect(true, false)` and `WiFi.mode(WIFI_OFF)`, and `loop()` rejoins by
itself when the time is up. It exists to hear whether the S3's own 2.4 GHz transmitter is what
breaks up the A32's Bluetooth audio. Unlike every other path in this file it does go through the
driver's de-initialisation; that is its purpose (WiFi off), and it has never failed.

**A worked example: the router is replaced.** 00:00 the link drops; "[WARN] wifi dropped.";
`startJoin()` tries for 20 s and fails. 00:20 the transmit power climbs one rung and
`Ambersong` comes up. 02:20 nobody is connected: the access point goes down and the S3
tries the house network again for 20 s; it fails, climbs another rung, and the access point is
back at about 02:40. Meanwhile a phone joins `Ambersong` at 03:00; from then on
every 2-minute retry is skipped. The sign-in page opens on the phone by itself (the captive
portal; otherwise open `http://192.168.4.1/`). The user signs in, types the new network's
name and password, and presses Save. 3 s later the S3 joins the new network, turns modem sleep
off, starts mDNS and SNTP, and the access point goes down. The rung that associated is kept.

### 8.2.4 Transmit power: the ladder

`gTxQ` is the WiFi transmit power in quarter-dBm (8 = 2.0 dBm, 60 = 15.0 dBm).

- `applyTxPower()` clamps it to 8..60 and hands it to `esp_wifi_set_max_tx_power()`. It is called
  after **every** WiFi mode change, because the driver does not carry it across one.
- **The ladder** `TX_LADDER = {8, 20, 28, 34, 44, 60}` is 2.0, 5.0, 7.0, 8.5, 11.0 and 15.0 dBm,
  ascending and wrapping back to the bottom. `ladderNext()` returns the rung above the rung nearest
  to the current value. **A failed join climbs one rung; whichever rung associates is kept.**
- **Why a ladder.** A join can fail for two opposite reasons: too much power (on this board the
  transmitter then radiated nothing at all) or too little (a distant router). From the inside the
  firmware cannot tell which, so it walks every rung. It starts at the bottom, because 2.0 dBm is
  ample for a router a few metres away. After the first success the walk normally never happens
  again.
- **The top rung is 15 dBm, and the clamp stops there.** This ceiling is a firmware choice, a
  value this firmware holds. It was measured here: on 2026-09-24 the forced access
  point at 20.0 dBm was seen by no device for 90 s; at 15.0 dBm two devices saw it within 22 s,
  and a PC joined it and signed into the portal. The ceiling is held in the firmware because the
  fault is believed to be a firmware problem.
- **The floor and default is 8 = 2.0 dBm**, also a firmware value. The board transmits on it here
  (on 2026-09-07 the station associated at 2.0 dBm when every higher rung then in use radiated
  nothing).
- **Where it is stored.** Not in the `net` namespace but in the machine's settings struct, as
  `cfg.wifiTxQ` (default 8), saved with the other settings. `applySettings()` pushes it into `Net`
  at boot. `loop()` in `src/s3/main.cpp` copies `Net::txPower()` back into `cfg` on every pass and
  marks the settings for saving, so the learned rung survives a power cut and the network code
  wins while it is hunting.
- **Console keys only.** It is not a portal setting and, since v.1.0.2, it is not in the
  settings file either. `settingsToText()` no longer writes a `wifiTxQ` line, and
  `settingsFromText()` accepts an old file's `wifiTxQ=` line and ignores it, so old backups still
  upload cleanly. Console key `B` steps it by hand (8.2.9), and the console dump `D` prints it
  (raw quarter-dBm, with a `#` line giving the dBm). It was removed from the portal on purpose and
  kept on the console.

### 8.2.5 Time: SNTP and the real-time clock

SNTP (Simple Network Time Protocol) sets the S3's system clock from a time server.

- `startNtp()` calls `configTzTime(tz, ntpServer, "time.nist.gov")` on every successful join.
  The timezone is a POSIX TZ string, so daylight saving is the C library's job.
- **"Fresh" means SNTP's own sync callback fired within the last 4 hours** (`NTP_FRESH_MS`: the
  core re-syncs every 3 hours, plus an hour's grace). Before 2026-09-24 "fresh" meant "the clock
  reads later than 2020", which stayed true forever after the first sync (2026-09-24).
- **NTP corrects the clock; the clock stays the authority.** While NTP is fresh and the link to
  the A32 is up, `loop()` in `src/s3/main.cpp` sends the time to the A32 once an hour
  (`MSG_SET_TIME`), and the A32 writes it into its battery-backed real-time clock. With no
  network, the real-time clock is the time. The real-time clock is a DS3231 module on the A32
  (Bible §5, §22), and the module has a backup battery (Bible §22).
- The timezone is applied once in `begin()` and again only by `setNtp()` (the Network card). One
  exception: the console `W` prompt uses a fixed Eastern-time rule (8.2.9 and 8.7).
- **The battery clock's health reaches the portal** (in v.1.0.2). The state field `rtc` is 0 when the
  clock is fine (or has not been asked yet), 1 when it answered with no valid time, and 2 when it
  does not answer. The page then shows a red pill, "BATTERY CLOCK LOST ITS TIME - check its
  battery" or "BATTERY CLOCK NOT ANSWERING". Chapter 9 (9.2.14) explains how the S3 decides each.

**mDNS** (multicast DNS, the `.local` name): `ambersong.local`, service `_http._tcp` on port
80, started when the S3 joins or raises its access point. `startMdns()` does nothing while its
flag `mdnsUp` is set; a lost link ("wifi dropped") and radio silence clear the flag, so the next
start runs `MDNS.begin()` again. The name keeps answering after the S3 comes home from its
rescue access point. This was tested on 2026-09-25: the S3 came home by itself from a forced
10-minute access point, and `ambersong.local` answered within 18 s and on every check after.
No restart is needed.

### 8.2.6 The portal: requests, sessions and security

The server is the Arduino core's synchronous `WebServer` on port 80. It handles **one
connection at a time**. It collects two headers, `Cookie` and `If-None-Match`. The server task
only calls `handleClient()` while `Net::connected()` is true (house network or access point up);
during radio silence it idles.

**The threat model is written in the code**: "people in the house, and a guest who is
curious". So the portal uses **plain HTTP, with no TLS** (encryption), by the author's decision.

**The rescue access point is open** (since v.1.0.5, by the author's decision, 8.4). On the house
network, the router's WiFi encryption at least covers the radio link. On the rescue access point
nothing is encrypted at all: while it is up, the portal sign-in and the house WiFi password typed
during a rescue cross the air in clear, and anyone within radio range can capture them. The
access point is up only when the house network cannot be reached or when someone raises it by
hand, and it closes when the radio gets home. The portal sign-in still guards every action on it,
exactly as on the house network.

What is enforced instead of encryption:

- **No anonymous access to any API.** Every API handler starts with `requireLogin()`, which
  answers **401 `{"e":"login"}`** and returns -1 if there is no valid session, so a forgotten
  check fails closed. Only these are anonymous: `GET /` (the page itself), `POST /api/login`,
  `POST /api/logout`, `GET /favicon.ico`, and the redirect for unknown paths.
- **The placeholder gate.** While the signed-in account is still the shipped placeholder,
  `requireLogin()` also answers **403 `{"e":"mustchg"}`** for every path except `/api/boot`,
  `/api/state`, `/api/passwd` and `/api/logout`. "Still the placeholder" is computed by
  `credsAreDefault()` from the stored record: the name equals the placeholder name **and** the
  stored hash equals the hash of the placeholder password with this account's salt. A new
  password alone therefore clears the gate (it reseeds the salt), and a rename alone does too.
  There is no "must change" flag, because the size of the `User` record is the length of the NVS
  entry, and adding a field would discard every stored account on upgrade. The gate lives in
  **one place**, `requireLogin()`; the code's reason is that "a rule enforced in fifteen places
  is a rule with fourteen chances to be forgotten". The two firmware-upload handlers do not use
  `requireLogin()` and repeat the check themselves (8.2.10). The page learns about the gate from
  `mustchg` in the live state, not from the schema, because the schema is cached against a tag
  that does not change when a password does.
- **Accounts.** Four slots (`MAX_USERS`), stored as one binary blob, key `users` in NVS
  namespace `auth`. Each is `User{ name[17], salt[8], hash[32], admin, used }`. The password is
  stored as **SHA-256 of (8-byte random salt followed by the password)**, one pass, computed with
  mbedtls; the salt comes from `esp_random()`. Passwords are hashed rather than stored because
  anyone with a USB cable can read this flash, and people reuse passwords on things that matter.
  **Slot 0 is the only administrator**; it cannot be deleted or demoted, new accounts are always
  normal, and nothing can promote one. A machine with no administrator would be a machine that
  needs a cable. Passwords are at least 6 characters; new user names are 2–16 characters, a
  rename 1–16. Anyone may change their own password (the old one is required); the administrator
  may change any other account's password without knowing the old one, which makes a guest's "I
  forgot mine" survivable without a cable. The page has no button for that; it is
  `POST /api/passwd` with the target slot `i` (8.9). From the page, a forgotten guest password is
  handled by removing the account and adding it again.
- **Sessions.** In RAM only, so a reboot signs everyone out, which is right for a machine that
  reboots whenever somebody flips its switch. Four slots (`MAX_SESS`). The token
  is 32 hexadecimal characters from `esp_random()` (128 bits). **Idle expiry is 5 minutes,
  sliding**: every request with the token restarts the 5 minutes. A fifth sign-in evicts the
  stalest session ("never refuse a login"). The age comparison uses wrap-safe subtraction of
  `millis()` values; an earlier comparison would have broken at the counter's 49-day wrap. The cookie is
  `amb=<token>; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400`; the server's 5-minute idle rule is
  what actually governs.
- **Password guessing.** Counted per client IPv4 address, four addresses tracked (`MAX_BAD`).
  From the 5th consecutive failure the address is locked out for 1 minute, then 2, 4, 8 …,
  doubling, capped at 64 minutes. While locked, login answers **429
  `{"e":"locked","s":<seconds>}`**. A success clears the record. The code's reasoning: "Five
  wrong guesses is a person who has forgotten; twenty is not." The hash comparison at login
  looks at all 32 bytes whatever happens.
- **Roles.** A normal account ("guest") sees every value and can change only volume, mute, the
  display brightness, the three panel-lamp levels, and the Bluetooth play, pause, next, previous,
  disconnect and pair actions. Everything that changes what the machine *is* (the needle and its
  calibrations, gain staging, the network, accounts, updates, the settings file, reboots) needs the
  administrator. The split is about damage, not secrecy.
- **Credentials are not in the settings file**, and restoring a settings file does not restore
  accounts. WiFi credentials live in NVS `net` only.
- **The web console is administrator-only** and has the same power as the cable, including `z`
  (erase the RF calibration) and `~` (reset the accounts). So `~` can also be sent from the web
  console while an administrator session is still open; since there is only one administrator,
  a lost administrator password in practice means the USB cable.

**Request handling details.**
- **Speculative-socket reaper.** Browsers open spare connections in advance. `WebServer` accepts
  one and then waits up to 5 s (`HTTP_MAX_DATA_WAIT`, fixed inside the library) for a request that
  never comes, while real requests queue behind it; logins took 15 s. After every
  `handleClient()`, `serverTask()` takes the connection the server is holding and closes it if it
  has been connected for **3 s** (`IDLE_SOCKET_MS`) with nothing to read. The reaper is off while
  an upload runs (`gOta`), because an upload has quiet moments. Three preconnected sockets made a
  fifteen-second login. With the reaper (then at 750 ms) a cold login went from 4.8 s to 0.12 s,
  and three parallel requests from 10, 16 and 19 s to about 1.1 s each (2026-08-31). No curl
  test could have found this, because curl opens exactly one connection.
- **Upload watchdog.** `otaWatchdog()` runs on every pass. If an upload is flagged and no upload
  callback has touched it for **15 s**, it releases the machine: for the S3's own image
  `Update.abort()` and un-park the needle and display; for the A32 relay `a32OtaAbort()`.
  `WebServer` does not reliably deliver "upload aborted" when a connection simply dies.
  This is why the portal task itself is not under the S3's 15 s task watchdog (chapter 4): a
  flash write legitimately blocks it, and the upload watchdog covers a stalled upload.
- **Numbers from requests** are parsed by `argNumber()`: the whole string must be one finite
  number; one decimal comma is read as a point ("107,3" is 107.3). Anything else is refused.
- **Unknown paths** answer **302**: to `/` on the house network, to `http://192.168.4.1/` on the
  rescue access point (the captive portal, 8.2.3). `/favicon.ico` answers **204** (no content).
- **Counters for "HTTP dead, ping alive"**: `Portal::loops()` (server task passes),
  `Portal::served()` and `Portal::stackFreeBytes()` (the task's stack high-water mark, in bytes),
  printed by console `s`. Frozen loops mean the task is starved; climbing loops mean the fault is
  in accepting connections.

### 8.2.7 The connection budget, and how the design follows from it

This is the rule that shapes everything the page does. Each HTTP connection, once closed, leaves
a TCP control block (the stack's record of a connection) in the TIME_WAIT state for about a
minute, against a pool of roughly sixteen. About twenty to forty rapid connections exhaust the
pool. The radio then stops accepting connections, **while ping stays perfect** (ping does not use
that pool), and it recovers after a minute or two of silence. A dashboard polled once a second
could never have worked: about sixty outstanding blocks against a pool of sixteen. Worse, the old
page kept polling at the same rate while its polls failed, so the pool never drained and the
fault looked permanent rather than intermittent.

The experiment that settled it (2026-08-31) was the cheapest one available: stop touching the
machine, then try exactly once. After 3 minutes of no traffic the first request answered in
1.07 s; 20 back-to-back requests all succeeded; the next 19 all died. Before that, the same
symptom had been blamed in turn on modem sleep, on an over-eager socket reaper and on core-0
starvation. Each of those was a real defect worth fixing, which is why each was convincing, and
none was the cause. At the new cadence (below) about 60 connections a minute became about 12,
and six polls measured afterwards all answered 200 in 0.35–3.1 s. So:

- **A page load is two requests**: the document (usually answered **304**, "not modified", with no
  body), then `/api/boot`, which returns the schema, the values and the live state in one reply.
- **The schema is cached in the browser.** The page keeps the last schema in
  `localStorage['amb.sc']` and offers its tag as `/api/boot?sc=<tag>`. The tag is the firmware
  version plus the administrator bit (`FW_VERSION-0` or `-1`); if it still matches, the radio
  answers `"sc":null` (about 11 kB becomes about 3 kB). Every new build changes the tag. The
  administrator bit is in the tag so that a normal account, signing in on a browser an
  administrator used, can never inherit the administrator's cached page.
- **Polling**: `/api/state` every **5 s**; after a failure the interval doubles, up to **30 s**,
  so an exhausted pool can drain; no request while the browser tab is hidden; polling stops on a
  401, so a half-typed password is not wiped by a redrawn sign-in form.
- **The console rides the state poll** (`/api/state?log=`), with no connection of its own.
- **Settings tabs** refresh `/api/values` when opened, not on a timer.
- **Every request has a deadline** of 12 s that covers the body as well as the headers (20 s for a
  settings upload; none for firmware uploads), because "headers, then stall" is what this machine
  does when it is out of control blocks.
- **The page never renders nothing.** If `/api/boot` fails, it shows "no answer from the radio."
  and retries after 2, 4, 8 and then every 15 s. An earlier page simply returned when its first
  fetch timed out, and the reported symptom, "it loads but doesn't show anything", was that line
  exactly.
- **Nothing is drawn before the state has arrived, and drawing never throws.** The first page
  drew its panes before it had fetched any state; the first paint read a field of the still-empty
  state object, threw, and everything after it (the first poll, the timer) never ran. The page
  rendered once, blank, forever. Since then `/api/boot` delivers the state with the schema, and
  the paint code tolerates any missing field. A dashboard that throws is a dashboard that stops.
- Anyone scripting against the radio must stay at about one request every few seconds.

### 8.2.8 The page: how it is built, embedded and served

- **Source**: `data/portal.html`, one self-contained file: inline CSS, one `<script>` block, no
  external assets. Never edit `src/s3/page_gz.h` by hand.
- **Build**: `scripts/page.py` is a PlatformIO `pre:` script of the `s3` environment only. On every
  build it:
  1. extracts the text between the first `<script>` and `</script>` and parses it with the
     `esprima` Python package. **A parse error fails the build** ("portal script will not parse").
     If `esprima` is not installed, it prints a warning and continues, unchecked. The reason: a
     duplicate `const` once shipped and killed the whole page on the radio;
  2. gzips the HTML at level 9 with the timestamp field set to 0, so identical HTML gives an
     identical blob;
  3. takes the ETag (the page's version label for caching) as the first 16 hexadecimal characters
     of the SHA-256 of the gzipped blob;
  4. writes `src/s3/page_gz.h` (`PAGE_ETAG`, `PAGE_GZ_LEN` and the bytes `PAGE_GZ[]` in PROGMEM,
     the flash-resident constant data), **only if its text changed**, so an untouched page does
     not force a rebuild. The file is committed to git.

  On 2026-09-25 the page was 35,109 bytes of HTML and 13,003 bytes gzipped.
- **Why gzip at build time.** The uncompressed page took 0.7–3.6 s per load on a one-connection
  server. The ESP32's ROM can inflate but not deflate, and compressing at run time would trade one
  cost for another. There is no filesystem at all: nothing to upload separately and nothing that
  can fall out of step with the firmware serving it.
- **Serving** (`hRoot()`): if the request's `If-None-Match` contains the ETag, answer **304** with
  no body. Otherwise **200** with `ETag`, `Cache-Control: no-cache, must-revalidate` (the browser
  always asks, so a firmware update is never hidden behind a stale page) and
  `Content-Encoding: gzip`, straight from flash.
- **The page is driven by the schema.** It knows no setting by name except `volume` and `muted`
  (the Now tab's music card). It draws whatever the schema lists: key, label, unit, tab, kind,
  low, high, step, read-only flag and enumeration choices. Adding a setting is one row in
  `src/s3/settings_table.h`; the same row also generates the `/api/set` handler's clamping and
  the setting's line in the downloadable and uploadable settings file (chapter 7), and its
  `scale` is why a value can travel between the boards as an integer (tenths of a dB, say) while
  a person reads "+15.0 dB". Kind 1 is a checkbox, kind 3 a drop-down, anything else a slider and a
  number box. A change posts `/api/set`; the stored value in the reply (possibly clamped) is written
  back into every widget with that key. Volume and mute follow the physical knob through the live
  state, except while their slider has focus.
- **Action buttons** are one list in the page (`ACTIONS`, per tab: label, action, value,
  administrator-only). A value of `?` means "ask": the station marks prompt for "Frequency of the
  station you are tuned to now, in MHz" (pre-filled 107.3), and the drop-sample button asks for a
  slot number. A comma typed in the answer is turned into a point. Which button calls what is
  listed in 8.12.
- **Tabs**: Now (dashboard and music), then the schema's tabs (Audio, Display, Lights, Needle,
  Bluetooth, System), then Console (administrator only), then Account.
- **Header pills**: link to the A32, needle homed, NTP fresh, the clock, the network (name and
  signal, or "access point"), the address, and red warnings when relevant (needle off, fault,
  re-indexing or hunting; audio board sends no state; settings locked; updating). Since v.1.0.2
  three more: "BATTERY CLOCK LOST ITS TIME - check its battery" (state `rtc` = 1),
  "BATTERY CLOCK NOT ANSWERING" (`rtc` = 2), and "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash
  both" (state `linkver`, the count of link frames dropped for another protocol version, is not
  zero while the boards have no handshake; chapter 9).
- **Toasts** (the message strip): an acknowledgement shows for 2.6 s in amber; a refusal or
  warning for 12 s in red. **A refusal travels on a 200 with a `warn` field** (`/api/act`,
  `/api/set` for the tuner ends, the settings upload) and is painted red. This project repeatedly
  shipped refusals that were painted as successes; this is the rule that ends it.
- **Network card**: Save stays **disabled until `GET /api/net` has filled the form**. A failed
  prefill once let one click save three empty fields and drop the house network. A blank password
  field means "keep the stored one".
- **Firmware uploads** use `XMLHttpRequest` with a progress bar and form field `f`; "Reboot the main
  board" asks for confirmation first.

### 8.2.9 The console, and the web console

**`Con`** (`class ConsoleTee`, `src/s3/console.cpp`) owns the USB port. Every byte the S3 prints
goes both to the USB port and into a **16 KB ring** (`LOG_CAP` = 16384; `gSeq` counts every byte
ever written). Every byte it reads comes first from a **256-byte injection queue** fed by the
portal (`IN_CAP`), then from USB. One spinlock guards both rings and is held only for memory
copies; the USB write happens outside it, because it can wait on the host. `write()` reports the
full length even when no USB host took anything, so a machine with only the portal attached is
not treated as failing. `console.cpp` is the only file that touches the real `Serial` port.

The reason for the web console is practical and electrical: with no cable there is nothing to
unplug from a closed cabinet, and there is no second ground connection between a PC and the
machine. The USB socket's ground is the machine's DC-side ground, which sits on mains earth
(Bible §10, §30.4), not the tube radio's chassis; a PC plugged into it adds a second path to
earth.

**Reading, from the web.** An administrator page with the Console tab open asks
`/api/state?log=<next byte number>`. The reply gains `"log":{"from":F,"next":N,"t":"..."}`: at most
**4096 bytes** per poll, the **oldest** bytes since `F` first (`copySince()`), newlines escaped,
other control characters dropped. A `from` older than the ring, or from the future (the S3
rebooted under an open page), resyncs to the oldest byte held. The page keeps the last 60,000
characters and inserts "[... older output was overwritten before it could be shown ...]" or
"[... the main board restarted ...]" when the byte count jumps. After a send, the page polls once
early, 900 ms later.

**Writing, from the web.** `POST /api/cons` with `k=<text>`. The server appends a newline and
injects it. **One key per request**, because the dispatcher reads one character per `loop()` pass
and **every character is a command**: a typed word ran as a string of commands ("status" ran `t`,
`a` and then an unbounded jog; any word with a `z` erased the RF calibration). The exception:
while a line prompt is reading (`Con.lineWanted` is true, set only by the `y` and `W` prompts),
up to **128** characters go through.

**Line prompts** block `loop()` while they read (up to 60 s for `y`, 40 s for `W`, each reset by
every character typed). While they wait, the display freezes and `Net::loop()` does not run; they
poll the link but do not send, so the A32, which treats two seconds of silence from the S3 as "amp
down", mutes. When a prompt returns, `drainLine()` discards input up to the end of the line
(50 ms window), so the tail of an over-long line can never run as one-key commands (where an
`m` or a `c` would overwrite a calibration and a `j` would jog without limits). Both prompts
feed the S3's 15 s loop watchdog (`wdtFeed()`, chapter 4) while they wait, so a prompt left open
does not restart the board. The `y`
buffer is 98 bytes: a 32-byte network name, a space and a 63-byte passphrase, plus the terminator.
**The `y` prompt echoes what is typed, the password included**, into the console ring and onto USB.

**The live index monitor `l`** prints the index sensor every ~250 ms for up to 60 s. It ignores
carriage return and line feed (the portal sends Enter after every key), stops on any other key,
and sends a ping to the A32 on every pass so the audio stays up. It feeds the loop watchdog on
every pass too. The `i` key's network scan also blocks `loop()`, for a few seconds, and is not fed:
it is well under the watchdog's 15 s.

**`commandList()`** is the single help list, printed by `?`, at the end of every `s`, and at boot.

**Not captured**: anything not written through `Con`, including the ESP-IDF framework's own log
lines. IDF's messages, brownout reports included, go to the S3's UART0, not to its native-USB
console, so silence on the USB console proves nothing about them.

### 8.2.10 Firmware updates: the S3-side endpoints

Both `/api/ota/s3` and `/api/ota/a32` use `WebServer`'s two-callback form: an upload callback
runs for every chunk (start, write, end, aborted), then an end handler answers. **These two do not
use `requireLogin()`.** Both callbacks check that the session is the administrator and is not
still the placeholder. The upload callback silently drops chunks from anyone else, and the end
handler answers **403** "admin only" or "change the default password first".

**The S3's own image** (`hOtaUpload()`, `hOtaEnd()`):
- Start: set `gOta`, call `portalOtaQuiet(true)` (needle stopped, display blanked, panel lamps
  dark), `Update.begin()`. The audio keeps playing, because it lives on the A32. A flash write
  turns the S3's cache off, and everything not in IRAM (the internal RAM that code can run from
  without the cache) stalls; the needle and display cannot absorb that. Fifteen dark seconds beat
  fifteen ugly ones. Settings saves are refused while the upload runs, for the same reason.
- First chunk of at least 14 bytes: if byte 0 is `0xE9` (the ESP image magic number) and the chip
  id in bytes 12–13 is not `0x0009` (ESP32-S3), print "[FAIL] that image is for chip id …, not the
  S3", abort and un-park. The two file pickers sit side by side and the two builds look alike.
  The main board is an ESP32-S3 and the audio board a classic ESP32 (Bible §1).
- Write each chunk; at the end `Update.end(true)`; on "aborted", abort and un-park.
- End handler: on a flash error, answer **500 `{"e":"flash"}`** and un-park. Otherwise **clear
  `gOta` before saving the settings** (the save refuses while an upload is flagged; before
  2026-09-03 it could never succeed here), flush the settings, answer
  **200 "rebooting into the new firmware"** with `Connection: close`, wait 300 ms, restart.
- An upload that ends without a reboot (refused, aborted, failed, or released by the watchdog)
  raises `otaResume`; `loop()` puts the needle back to work only after its stop has landed, and
  homes it if it was mid re-index.
- A successful update reboots, and the reboot can beat the HTTP reply, so the page may report
  failure for an update that worked. Check the version on the System tab ("running") afterwards.
- **Trial and rollback.** A freshly uploaded S3 image boots "on trial" (§3.2.9; the mechanism is
  in chapter 4). `confirmTick()` in `src/s3/main.cpp` confirms it no earlier than about 70 s after
  boot: a minute of running, the portal task still turning, the house network or the rescue access
  point up, and the link self-test passed. It does not need the A32. If the image resets before it
  is confirmed, the bootloader returns to the previous image. The console status `s` prints the
  image state on its `image` line.
- **Nothing is saved during the trial** (v.1.0.1). `settingsWrite()` in
  `src/s3/main.cpp` writes nothing while `s3ImageOnTrial()` is true; changes wait in RAM and the
  ordinary debounce writes them once the image is confirmed. A rolled-back image therefore never
  finds a settings layout newer than it knows. The cost: a change made in the trial minute is lost
  if the power goes in that minute. The portal's Save answers "NOT saved yet - this firmware is on
  trial after an update; settings are written once it is confirmed (about a minute)".
- **A portal reboot during the trial is a rollback, and it goes through at once** (v.1.0.1).
  `hReboot()` does not try to save (nothing is saved on trial), answers **200 "rebooting - this
  firmware was still on trial, so the previous firmware comes back"**, and restarts 200 ms later.
  Refusing would take away the one button that undoes a bad update. Proven on 2026-09-25: a reboot
  on trial brought back the previous build, whose `image` line read "ROLLED BACK"; the build was
  then uploaded again and confirmed.
- **No S3 upload while the running image is on trial.** At the first chunk, `hOtaUpload()` asks
  `s3ImageOnTrial()`. If the image is on trial, it prints "portal: S3 update refused - this image
  is still on trial." on the console, quiets nothing (the needle and the display carry on), and
  drops every byte that follows. `hOtaEnd()` then answers **HTTP 409
  `{"e":"this firmware is still on trial - retry in a minute"}`**, and the page shows that
  sentence. The reason: the upload would be written over the other slot, which holds the very
  image a rollback returns to.

**The A32 image, relayed** (`hOtaA32Upload()`, `hOtaA32End()`): start sets `gOta` and calls
`a32OtaBegin()`; the needle and display are **not** parked, because the S3 is not writing its own
flash. The audio stops, because the A32 is. The first chunk is checked for chip id `0x0000`
(classic ESP32); a wrong chip aborts with "that image is not for the audio board (wrong chip) -
nothing was written". Each chunk goes to `a32OtaChunk()`, the end to `a32OtaEnd()`. The end handler
answers **500** with the relay's reason, or **200 "N bytes sent, the A32 is rebooting"**. The relay
puts about 2.1 kB of frame buffers on the portal task's stack. How the relay and the A32's own
rollback work is described in chapter 9. First proven on 2026-08-31: a 1,205,728-byte image
relayed in 75.8 s, HTTP 200. That is about 16 kB/s, well below the link's line rate, because each
1 kB frame waits for the A32's acknowledgement and each acknowledgement waits for a flash write.

Since v.1.0.4 (A32 only) the A32 also writes no settings while its own new image is on
trial. An edit of an audio-board row from the portal in that minute is still answered as
accepted; the A32 holds it in RAM and writes it when the image is confirmed, logging "settings
changed during the trial are now saved" (chapter 9). Proven on 2026-09-26.

### 8.2.11 Version shown to the user

The firmware version string is `v.1.YYYYMMDDTHHMMSS`, stamped at build time by
`scripts/version.py` (chapter 3). Next to it every build carries a **commit stamp**, `FW_COMMIT`:
the source's short git hash (seven characters), with `-dirty` added when tracked files had
uncommitted changes (untracked files are ignored), or `nogit` when git was not available. The
schema's `fw` field, shown as "running" in the portal's System tab, is the two together, for
example `v.1.20260927T101323 (v.1.0.5)`. The S3's boot banner on the console prints the same pair
as `firmware v.1.<stamp> (<hash>)`, and the settings file header carries it too (chapter 7). The
commit stamp is not sent on the inter-board link. The schema cache tag (`schemaTag()`) is still
the version string plus the administrator bit; the commit is not part of it.

## 8.3 Settings and constants

### 8.3.1 Network (`src/s3/net.cpp` unless stated)

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `ssid`, `pass` (NVS `net`) | empty | text | name ≤ 32, passphrase ≤ 63 | House network | Network card, console `y` |
| `ntp` (NVS `net`) | `pool.ntp.org` | host name | — | Primary time server; `time.nist.gov` is the fixed second | Network card |
| `tz` (NVS `net`) | `EST5EDT,M3.2.0,M11.1.0` | POSIX TZ | — | Local time rule | Network card |
| `AP_SSID` | `Ambersong` | — | — | Rescue access point name | source |
| AP security | open (`nullptr` passphrase in `softAP()`) | — | — | No WiFi password on the rescue access point (since v.1.0.5; 8.2.6 for the cost) | source |
| AP channel / clients / hidden | 1 / 2 / no | — | — | "a rescue hatch, not a hotspot" | source |
| captive-portal DNS (`gDns`) | port 53, every name (`"*"`) → the AP's address | — | — | Runs only while the AP is up (since v.1.0.5) | source |
| AP address | 192.168.4.1 | — | — | The ESP32 soft-AP default | — |
| softAP attempts | 3, 400 ms apart; settle 50 ms + 100 ms | — | — | `startAp()` | source |
| `JOIN_MS` | 20,000 | ms | — | Join timeout before the AP and the next rung | source |
| `RETRY_MS` | 120,000 | ms | — | Retry the house network from the AP | source |
| `FORCE_HOLD_MS` | 600,000 | ms | — | Minimum life of a forced AP | source |
| request grace | 500 | ms | — | Delay before a portal force-AP or quiet request is carried out | source |
| `setWifi` grace | 3,000 | ms | — | Delay before rejoining after a network change | source |
| quiet | 120 (portal default) | s | 1–600 | Radio silence length | `sys.quiet` |
| `TX_LADDER` | 8, 20, 28, 34, 44, 60 | ¼ dBm | 2.0–15.0 dBm | Transmit power rungs; the 15 dBm top is a firmware choice (8.2.4) | source |
| `cfg.wifiTxQ` (settings struct, not the settings file) | 8 | ¼ dBm | 8–60 | Learned transmit power | the ladder, console `B` |
| scan / sort | all channels / by signal | — | — | Pick the strongest access point with the name | source |
| power save | `WIFI_PS_NONE` | — | — | After association and every 5 s | source |
| hostname / mDNS | `ambersong` / `ambersong.local`, `_http._tcp` 80 | — | — | Name on the network | source |
| `NTP_FRESH_MS` | 4 | h | — | NTP counts as fresh this long after a sync | source |
| NTP push to the real-time clock | 1 | h | — | While fresh (`src/s3/main.cpp`) | source |

### 8.3.2 Portal (`src/s3/portal.cpp` unless stated)

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| HTTP port | 80 | — | — | | source |
| portal task | core 0, priority 3, 8192 | bytes of stack | — | | source |
| `IDLE_SOCKET_MS` | 3,000 | ms | — | Silent-socket reaper; off during uploads | source |
| upload watchdog | 15,000 | ms | — | No upload activity for this long = no upload | source |
| `MAX_USERS` | 4 | accounts | — | Slot 0 is the administrator | source |
| `MAX_SESS` | 4 | sessions | — | Stalest evicted on a fifth sign-in | source |
| `IDLE_MS` | 5 | min | — | Sliding session idle expiry | source |
| cookie | `amb`, `HttpOnly`, `SameSite=Lax`, `Max-Age` 86400 s | — | — | | source |
| `MAX_BAD` | 4 | addresses | — | Addresses tracked for lockout | source |
| lockout | from the 5th failure: 1, 2, 4 … 64 | min | — | Doubling | source |
| password / name | ≥ 6 / 2–16 (new), 1–16 (rename) | characters | — | | source |
| log slice | 4096 | bytes per poll | — | Web console read | source |
| console send | 1 key, or ≤ 128 characters while a prompt reads a line | characters | — | Web console write | source |
| reboot save retries | 15 × 100 | ms | — | Before refusing a reboot (none while the image is on trial) | source |
| restart delays | 200 (reboot), 300 (S3 update) | ms | — | Let the answer leave | source |
| placeholder account | `DEFAULT_USER` / `DEFAULT_PASS` | — | — | Created on a blank board and by `~` | source |

### 8.3.3 Page and console

| Name | Default | Unit | Where |
|---|---|---|---|
| poll interval | 5 s, doubling on failure to 30 s, none while hidden | s | `data/portal.html` (`POLL_MIN`, `POLL_MAX`) |
| boot retry | 2, 4, 8, then 15 s | s | `data/portal.html` |
| request deadline | 12 s (settings upload 20 s, firmware upload none) | s | `data/portal.html` |
| toast | 2.6 s acknowledgement / 12 s warning | s | `data/portal.html` |
| console buffer on the page | 60,000 characters; early poll 900 ms after a send | — | `data/portal.html` |
| schema cache | `localStorage['amb.sc']`, tag `FW_VERSION-<0/1>` | — | page and `schemaTag()` |
| `LOG_CAP` / `IN_CAP` | 16,384 / 256 | bytes | `src/s3/console.cpp` |
| prompts | `y` 60 s, `W` 40 s (reset per character); `drainLine` 50 ms | — | `src/s3/main.cpp` |
| `l` monitor | 60 s at most, ~250 ms per line | — | `src/s3/main.cpp` |
| USB console | 115200 baud, native USB CDC | — | `platformio.ini`, `setup()` |

## 8.4 Decisions

**Decision.** A synchronous web server in its own task on core 0.
Why: the one arrangement measured harmless to the display. Rejected: the asynchronous server,
untested on this machine. When and evidence: 2026-08-28 to 08-31, before the firmware repository's
first commit (2026-09-01); `src/s3/portal.h`.

**Decision.** Plain HTTP, no TLS.
Why: against the stated threat model (family and curious guests on the house network), TLS on an
ESP32 costs more than it buys. Instead: no anonymous API, salted hashes, RAM sessions, lockout, no
credentials in the settings file. When: 2026-08-31, before the repository; the author's decision.

**Decision.** Roles split by damage, not secrecy.
Why: a guest should get "the music and the lights, not the machine", and see everything. When
and evidence: 2026-08-31, before the repository; `src/s3/settings_table.h`.

**Decision.** Sessions expire after 5 idle minutes, sliding.
Why: the author's requirement (so written in `src/s3/portal.cpp`). When: 2026-08-31, before the
repository.

**Decision.** No password reset on the front of the machine; the USB console `~` is the
recovery path.
Why: recorded in `src/s3/portal.h` only as "by decision - the USB socket is the recovery path".
Rejected: a front-panel reset. When: 2026-08-31,
before the repository; `src/s3/portal.h`.

**Decision.** A public placeholder account in the image, with a hard first-login gate.
Why: the firmware tree became a git repository meant to be published, so the author's real
credentials could no longer be compiled in. Rejected: a "must change" field in `User` (it would
change the NVS record size and discard every stored account); carrying the gate flag in the schema
(cached; it would never clear). When and evidence: 2026-09-03, a pre-v.1 item.

**Decision.** One choke point, `requireLogin()`, for sign-in and the placeholder gate; the two
upload handlers are the only exceptions and repeat the check.
Why: one rule in one place cannot be forgotten in the fifteenth handler. When: 2026-09-03.

**Decision.** The page is gzipped at build time, given an ETag and served from flash; no filesystem.
Why: page load time was the page's size. Rejected: compression at run time (no deflate in ROM);
a filesystem image (can fall out of step with the firmware). When: before the repository;
`scripts/page.py`.

**Decision.** The build fails if the page's script does not parse.
Why: a duplicate `const` shipped and killed the page on the radio. When: `scripts/page.py`.

**Decision.** The page is generated from one settings table.
Why: adding a knob is one row; four hand-edited places per knob would drift. The intent was
all the knobs and sliders possible. When: before the
repository.

**Decision.** The connection budget: polls every 5 s backing off to 30 s, none while hidden; a
two-request page load; the console on the state poll; favicon answered 204.
Why: the ~16-block TIME_WAIT pool (8.2.7). Rejected: once-a-second polling, by measurement; a
separate console poll. When: 2026-08-31 (polling), 2026-09-23 (console).

**Decision.** Close silent sockets after 3 s, never during an upload.
Why: speculative browser sockets made 15-second logins. Rejected: 750 ms, which killed real
requests once the link slowed and cut off the only route in. When: before the repository.

**Decision.** Modem sleep off after association, and asserted again every 5 s.
Why: measured page 9.0 s → 1.1 s, state 4.1 s → 0.3 s, TCP connect 0.5–1.0 s → 15–80 ms (a slow
handshake meant the radio was asleep between beacons). It costs about 80 mA, which does not
matter on a set powered from the mains (Bible §10). When: before the repository.

**Decision.** The radio must never become unreachable: a rescue access point plus a background
retry, and the access point reachable from the portal and fixed rather than recorded.
Why: once the cabinet is shut, the cable is gone. When and evidence: the access point and retry
before the repository; the portal button and the driver checks 2026-09-12. The rescue
access point was made a required item before v.1: a fault in it is fixed, not recorded as an
observation.

**Decision.** Never take the WiFi driver through `WIFI_MODE_NULL` while the AP or station must
survive; believe the driver, not `softAP()`'s return.
Why: arduino-esp32 issue #7232 and the library's identical-configuration shortcut. When and
evidence: `startAp()` 2026-09-12; the retry path 2026-09-25.

**Decision.** A forced access point holds 10 minutes, then goes home by itself.
Why: an unbounded hold, reachable from the portal, would strand the machine off the house network
until a power cycle. When and evidence: 2026-09-24.

**Decision.** Take a lone access point down for the length of a join attempt; keep it if someone
is on it.
Why: one radio cannot hold the router's authentication round trip while beaconing on another
channel. On 2026-09-05 every retry for half an hour failed with authentication expiry and
timeout reasons, never "no access point found". Rejected: the "channel lock" explanation, which
was checked against the ESP-IDF 4.4.7 documentation and is wrong (in AP+STA the station's channel
wins, and the access point moves to it, announcing a channel switch). When:
2026-09-07.

**Decision.** Hold the retry while anyone is connected to the access point.
Why: otherwise the settings form dies every two minutes in exactly the case the access point
exists for. When: 2026-09-01.

**Decision.** Transmit power is learned on an ascending, wrapping ladder that starts at the bottom
and tops out at 15 dBm. It is learned state: not a portal setting and not in the settings file;
only the console (`B`, `D`) reaches it.
Why: a join can fail from too much power or too little and the firmware cannot tell which. A
settings row once wrote 2.000 where the dump wrote 8, quadrupling power on a paste. At 20 dBm this
board radiated nothing on 2026-09-24, and at 15 dBm it radiated reliably; the 15 dBm ceiling is a
firmware choice. When and evidence: 2026-09-07, 2026-09-24; the settings-file
line removed in v.1.0.2.

**Decision.** HTTP handlers only request WiFi changes; `Net::loop()` acts after 500 ms (3 s for a
network change).
Why: avoid the cross-core race, and let the answer out first. Rejected: acting inside the handler
with an immediate "verified" answer (2026-09-12). When: 2026-09-24.

**Decision.** NTP is fresh only within 4 h of an SNTP sync; NTP corrects the real-time clock
hourly and the clock is the authority without a network.
Why: "later than 2020" stayed true forever and wrote a free-running clock into the real-time clock
as if it were NTP. When: 2026-09-24.

**Decision.** Timezone as a POSIX string, applied once.
Why: daylight saving handled by the C library. The display code used to force the timezone on
every redraw, which would have quietly overridden whatever the portal set. When: before the
repository.

**Decision.** mDNS is not restarted when the S3 comes home from its access point.
Why: tested rather than assumed. The live
test of 2026-09-25 showed the name answering within 18 s of the return and on every check after
(8.2.5), so nothing was changed.

**Decision.** Request numbers are parsed whole, with one decimal comma accepted, and non-finite
values are refused.
Why: "107,3" was stored as 107. And `String::toFloat()` is `atof()`,
which parses "nan"; `NaN < lo` and `NaN > hi` are both false, so a NaN passed every clamp.
`posMin=nan` would have become a soft limit that does not limit (chapter 5). The clamp is one function, `settingBound()`,
used by both `/api/set` and the settings file, because a fix that landed on one of two call
sites had already been found twice ("a rule written twice is a rule that will
be enforced once"). When: 2026-08-31 (non-finite), 2026-09-24 (the whole-string parse).

**Decision.** Refusals travel on a 200 with a `warn` field and are painted red for 12 s.
Why: refusals shown in the success colour for 2.6 s were missed, repeatedly. When: 2026-09-24 and earlier.

**Decision.** A reboot refuses rather than lose an unsaved change (409 with the reason), and a
refused reboot resumes the needle.
Why: "change something, press Reboot" lost the change. When: 2026-09-25.

**Decision.** The settings file download is refused (409) while settings are locked.
Why: the RAM then holds defaults, and a round trip would overwrite the real calibration. When:
2026-09-25.

**Decision.** One console object for two audiences, `Con`, not a redefined `Serial`.
Why: `Serial` is a core macro, and redefining it depends on include order. When: 2026-09-23
.

**Decision.** One key per web-console send, except while a prompt reads a line.
Why: typed words ran as strings of commands. When: 2026-09-24.

**Decision.** A prompt discards the unread rest of its line (`drainLine()`); the `y` buffer fits the
longest legal pair.
Why: an over-long line's tail ran as one-key commands. When: 2026-09-25.

**Decision.** The live index monitor `l` works from the web console.
When: 2026-09-25.

**Decision.** The `W` clock prompt keeps its fixed Eastern-time rule, with a note for builders.
Why: the rule belongs to this build's location; anyone building a similar device should put in
their own. It is noted for builders (8.7), not fixed. When: 2026-09-25.

**Decision.** The `y` prompt may block `loop()` for up to 60 s, mute the A32 meanwhile, and echo the
password.
Why: judged not to cause any meaningful harm in realistic use; left as it is.

**Decision.** The rescue access point is an open network, with no WiFi password.
Why: the rescue access point is for someone whose radio has lost its network; a passphrase to
find and type is one more obstacle in exactly that moment. The portal sign-in still guards every
action. The cost, accepted with it: on the open access point nothing is
encrypted, so the portal sign-in and the house WiFi password typed during a rescue cross the air
in clear while the access point is up, within radio range. The access point is up only when the
house network is unreachable or when raised by hand, and it closes when the radio gets home.
Rejected: the passphrase used until v.1.0.4, a literal (`AP_PASS`) in the published source that
every builder had to change (§12.6.2). This decision replaces the earlier choice,
which kept the access point's name and passphrase as published defaults. When and evidence:
v.1.0.5, 2026-09-27 (tag v.1.0.5); proven live the same morning (8.7).

**Decision.** The rescue access point is a captive portal: a DNS server answers every name with
the access point's address while it is up, and the catch-all redirect names that address.
Why: the same wish, carried one step further; a person rescuing the radio should not have to know
`192.168.4.1` either. The phone's own connectivity check opens the page. The DNS server runs only
with the access point, so nothing changes on the house network. When and evidence: v.1.0.5, 2026-09-27, released with the open network. A phone opening the page
by itself has not yet been seen (8.7).

**Decision.** Portal starvation while the needle moves is accepted.
Why: after v.1 nobody should need the portal while tuning. Accepted; whether to change it is
undecided.

**Decision.** Upload safety: a chip-id check on bytes 12–13, a 15 s inactivity watchdog, and the
needle and display parked only for the S3's own image.
Why: two side-by-side file pickers; dead connections do not always report an abort. When: before
2026-09-03, extended 2026-09-24/25.

**Decision.** The S3's own image runs on trial and rolls back unless it confirms itself, and no S3
upload is accepted while the running image is on trial.
Why: before v.1.0 the core marked every new image valid before `setup()`, so an S3 build that hung
or crashed at boot could only be fixed over USB, with the cabinet open. An upload during the trial
would overwrite the image a rollback returns to. When and evidence: 2026-09-25. Proven
live the same day, by over-the-air upload with the USB cables unplugged: the image booted "ON TRIAL
(rollback armed), watchdog on"; a second S3 upload sent during the trial got the 409 above; the
image confirmed itself by 90 s of uptime. A deliberately hanging test image was then reset by the
task watchdog, and the bootloader returned to the confirmed build. Chapter 4 describes the
mechanism.

**Decision.** Nothing is saved while the S3's image is on trial, and a portal reboot during the
trial goes through at once, as a rollback.
Why: a new image that changes the settings layout would save the new layout within seconds, and if
it then rolled back, the previous image would find settings newer than it knows and lock itself
onto defaults. A reboot refused over an unsaved change would take away the one button that undoes
a bad update. When and evidence: v.1.0.1, 2026-09-25; proven live the same evening (a
reboot on trial brought the previous build back, "ROLLED BACK").

**Decision.** An edit of an audio-board setting is refused with 409 while the A32 is not
answering.
Why: the S3 forgets its copy of the A32's settings (`haveCfg`) the moment the A32 goes silent.
Without the A32's answer the edit would go nowhere, read back as a clamp to 0, and be overwritten
when the A32 came back. When and evidence: v.1.0.2, 2026-09-25. Not yet exercised on the radio.

## 8.5 Failures and recovery

| What fails | What the firmware sees | What it does | How to recover | Ladder class |
|---|---|---|---|---|
| Wrong WiFi password stored | No association within 20 s | Rescue AP up; climbs one transmit rung per failure (all six, wrapping); retries every 2 min | Join `Ambersong` (open; the page opens by itself on most phones, otherwise `http://192.168.4.1/`), fix it on the Network card (type the new password; blank keeps the old), or console `y` | NOTE (heals once a person fixes the setting, no cable) |
| Router off or replaced | Link lost | "[WARN] wifi dropped.", rejoin, AP after 20 s, retry every 2 min, never while someone is on the AP | Wait, or fix it through the AP | NOTE |
| Empty network name submitted | `ssid` empty | 400 "the network name is empty"; the page cannot save before its prefill | — | NOTE |
| Access point will not start | `apReallyUp()` false after 3 attempts | "[FAIL] the access point will not start …" | USB console: `y`, `i`, `B`, `z`; or reboot | BLOCKER if nothing radiates at all (USB-only) |
| Transmitter above what the board can radiate | Join fails; AP unseen | Ladder climbs, wraps to 2.0 dBm; top rung capped at 15 dBm (8.2.4) | Wait for the ladder; USB `B` | NOTE |
| Forced AP, nobody comes | Hold expires | Goes home after 10 min | none | NOTE |
| The phone joins the AP but does not open the page by itself | Nothing (the phone decides) | The DNS server and the redirect keep answering | Open `http://192.168.4.1/` by hand | NOTE |
| Captive-portal DNS server fails to start | `gDns.start()` false; no "captive portal" line | Tries again on every pass of `Net::loop()` while the AP is up; the portal still answers at its address | Open `http://192.168.4.1/` by hand | NOTE (never seen) |
| Blank board's AP at the bottom rung | No network name, so no failed join to climb the ladder | AP stays at 2.0 dBm | Move closer; or console `y` / `B` over USB | DEFECT at worst (console) |
| Portal dead, ping alive | TIME_WAIT pool exhausted | The page backs off to 30 s so the pool drains | Stop all traffic for 1–2 min, then try once; console `s` shows `loops`/`served` | NOTE |
| Portal slow while the needle moves | Emitter at priority 19 on core 0 | Nothing (accepted) | Stop the needle, or wait | NOTE |
| Page shows "no answer" | `/api/boot` failed | Retries 2, 4, 8, 15 s | Wait | NOTE |
| Session expired | 401 | Sign-in form; polling stops | Sign in | NOTE |
| Locked out | 5th failure from one address | 429 with seconds left, 1–64 min | Wait | NOTE |
| Owner's password lost | — | Nothing on the portal can reset slot 0 | USB console `~` (erases **all** accounts, restores the placeholder) | DEFECT (needs the cable) |
| S3 upload interrupted | "aborted", or 15 s of no chunks | Abort, un-park; needle back to work once its stop lands; the old image stays in its slot | Upload again | NOTE |
| A32 upload interrupted | as above | `a32OtaAbort()` from the watchdog; mid-stream relay failures send no abort and the A32 stays muted about 20 s until its own timeout (accepted as harmless) | Upload again | NOTE |
| Wrong image picked | Chip id | Refused before writing | Pick the other file | NOTE |
| Update reported failed but worked | Reboot beat the reply | — | Check "running" on the System tab | NOTE |
| Reboot pressed with an unsaved change (image confirmed) | Save refused | Needle stopped, save retried for 1.5 s; else 409 with the reason, needle resumed | Fix the cause, press again | NOTE |
| Reboot pressed while the S3 image is on trial | `s3ImageOnTrial()` | No save attempted; "rebooting - this firmware was still on trial, so the previous firmware comes back"; restart; the bootloader returns to the previous image | Upload the new build again if it was good | NOTE (by design; proven 2026-09-25) |
| Audio-board setting edited while the A32 is silent | `haveCfg` false | 409 "the audio board is not answering - its settings cannot be changed now" | Wait for the A32 to answer (handshake) | NOTE (never exercised) |
| Settings locked (older firmware than the saved settings) | `settingsLocked()` | Download refused 409; the page says why | Flash the newer firmware | NOTE |
| A word typed in the web console | length > 1, no prompt | 400 "one key at a time" | Send single keys | NOTE (before 2026-09-24 it could erase the RF calibration) |
| Console injection queue full | 256 bytes queued | 503 "console busy" | Wait | NOTE |
| `y` or `W` prompt left open | — | `loop()` blocked up to 60 s / 40 s; display frozen, A32 mutes. The prompt feeds the loop watchdog, so it does not restart the S3 | Finish, or send a blank line | NOTE (accepted) |
| Radio silence test | — | WiFi off up to 600 s, then rejoins | Wait | NOTE |
| Proto self-test fails at boot | `protoSelfTest()` false | Console "[FAIL] proto self-test - this image will NOT be confirmed; reboot to roll back, or upload a good build." Boot carries on (display, needle, link, WiFi, portal); the image is never confirmed | Image uploaded over the air: reboot from the portal, and the previous image returns (an upload is refused while it is on trial). Image flashed by USB: it is not on trial, so upload a good build | DEFECT |
| New S3 image hangs or crashes on trial | The task watchdog (15 s) or a crash resets it before confirmation | The bootloader returns to the previous image; its `image` line adds "an earlier update was ROLLED BACK" | Nothing; upload a fixed image | NOTE (proven 2026-09-25) |
| New S3 image runs but never earns confirmation (no network, portal task stuck) | Not confirmed | Stays on trial and keeps running; a later reset rolls it back | A portal reboot if the portal answers; otherwise a full power cycle | BLOCKER (never exercised) |
| S3 upload sent while the running image is on trial | `s3ImageOnTrial()` | Bytes dropped; 409 "this firmware is still on trial - retry in a minute" | Wait for "image confirmed" (about 70 s after boot), upload again | NOTE |

## 8.6 Graveyard

These are what failed **here**, on this machine, with this toolchain. They may work elsewhere.

- **Once-a-second dashboard polling, at the same rate while failing.** Here it exhausted the TCP
  control-block pool and the radio stopped accepting connections while ping stayed perfect.
  Replaced by the 5 s / 30 s back-off.
- **Four requests per page load** (document, schema, values, state). Replaced by `/api/boot`.
- **Settings tabs refreshed every 10 s.** Replaced by a refresh when the tab is opened.
- **A page that drew before it had any state**, and **a boot that gave up silently** when its
  first fetch timed out. Here the first left the page blank for ever, and the second showed
  nothing with no retry (8.2.8).
- **A separate console poll.** Replaced by the log riding `/api/state` (2026-09-23).
- **A 750 ms silent-socket reaper.** Here, once power save came back and round trips rose past a
  second, it killed real requests and made the portal unreachable. Now 3 s, off during uploads.
- **`WiFi.setSleep(false)` before `WiFi.begin()`.** Ignored here, because connecting re-applies
  power save. Now after association, and every 5 s.
- **An uncompressed page, and a 302 for the favicon** (the browser downloaded the page twice).
- **`src/s3/page.h`**, a stale, unreferenced second copy of the portal page, deleted on
  2026-09-03.
- **A stack counter named in words** (`stackFreeWords()`) that reported bytes, so it over-read
  the portal task's headroom by four (found 2026-08-31). Now `stackFreeBytes()`.
- **The first `startAp()`**: it ignored `softAP()`'s return, called it in the same breath as the
  asynchronous mode change, and pinned no channel. Here the access point was claimed and never
  seen.
- **`WiFi.disconnect(true, false)` at the head of `startAp()`, and `softAPdisconnect(true)` on the
  retry path.** Both went through `WIFI_MODE_NULL` and de-initialisation; here the access point
  then broadcast nothing.
- **Believing `softAP()`'s return and printing "is up".** For months here, nothing was on the air.
- **`net.forceAp` carried out inside the HTTP handler with an immediate verified answer**
  (2026-09-12). Here it raced `Net::loop()` and tore down the link the answer needed (2026-09-24).
- **A forced access point that did not stamp `lastTry`.** It lived one loop tick (2026-09-24).
- **Transmit power as a typed setting, and 20 dBm as the top rung** (2026-09-07, 2026-09-24). Then
  **transmit power as a settings-file line** (`wifiTxQ=`): removed in v.1.0.2; an old
  file's line is ignored on upload.
- **Console keys `,` and `.`** (the needle's up-leg top speed −100 / +100 half-steps/s). Here `.`
  had no ceiling; removed in v.1.0.2. The portal's `upVmax` row does the job inside its
  bounds (chapter 5).
- **A drop-sample prompt that asked for a frequency.** The portal's "Drop one sample" button reused
  the station prompt; since v.1.0.2 it asks "Which sample slot to drop (0-11)?".
- **"NTP fresh" meaning "the clock is past 2020"** (2026-09-24).
- **The author's real credentials compiled into the image** (until 2026-09-03).
- **Multi-character web-console sends** (2026-09-24).
- **A second help list printed at boot**, which drifted from the dispatcher (2026-09-23).
- **`copySince()` returning the newest bytes**: it dropped the head of any burst over 4 KB
  (2026-09-24).
- **`String::toFloat()` for request numbers**: it read "107,3" as 107 and accepted "nan" (2026-09-24).
- **The `sys.sddly` portal action** (2026-09-10 to 09-11): here it sent the radio to full volume.
  Do not restore it (2026-09-12 and later).
- **The "channel lock" theory** of the AP+STA join failure: checked and wrong.
- **The "core 0 starvation" theory** of "HTTP dies, ping lives": the cause was the connection pool.

## 8.7 Limits and firmware notes

**Known limitations**
- Plain HTTP: passwords and session cookies cross the WiFi unencrypted. By decision.
- The rescue access point is open (since v.1.0.5): while it is up, the portal sign-in and a house
  WiFi password typed during a rescue cross the air in clear, readable by anyone within radio
  range. By decision (8.2.6, 8.4).
- Whether a phone opens the page by itself is the phone's choice; some do not, and then the
  address must be typed (`http://192.168.4.1/`).
- Password hashing is one SHA-256 pass with an 8-byte salt and no key stretching. NVS is not
  encrypted, so anyone with the USB cable can read the hashes.
- The lockout tracks four addresses; a fifth address takes over slot 0's record and resets it.
- There is no CSRF token; protection rests on `SameSite=Lax`.
- The cookie lives 24 h but the server forgets a session after 5 idle minutes; a reboot signs
  everyone out.
- There is only one administrator; losing its password needs the cable, and `~` erases every
  account.
- The portal is starved while the needle moves (accepted, §12.3.3).
- `y` echoes the WiFi passphrase into the console ring (readable by any administrator with the
  Console tab open) and onto USB. Accepted (§12.3.15).
- `y` and `W` block `loop()`; the A32 mutes while they wait. Accepted. They feed the loop watchdog,
  so a prompt left open does not restart the S3.
- **`W` uses a fixed `EST5EDT` rule** in `setClockInteractive()` and ignores the portal's
  timezone. Builders: put your own POSIX TZ string there, or read the one `Net` holds (noted on
  purpose, not fixed; §12.6.2).
- A change made in the minute an S3 image is on trial is lost if the power goes in that minute
  (nothing is saved on trial, 8.2.10).
- The rescue access point transmits at the learned station rung. With no network configured the
  ladder never moves, so a blank board's access point runs at 2.0 dBm. On this machine on
  2026-09-24 an access point at 2.0 dBm "was visible at 44 % but could not be joined" (2026-09-24).
  On another builder's board it may be fine at close range; untested (§12.3.14).
- While retrying from the access point, the access point disappears for up to 20 s every 2
  minutes (if nobody is connected to it). Someone trying to join at that moment must wait.
- `quiet()` still goes through `WIFI_OFF` and de-initialisation (accepted; never failed).
- IDF's own log lines never reach the web console.
- The WiFi workarounds depend on arduino-esp32 core 2.0.17 internals (the library's soft-AP
  shortcut, the mode-change branches, `HTTP_MAX_DATA_WAIT`). The toolchain is pinned to
  `espressif32@7.0.1`; a core update must re-check all of them.

**Small known issues, left as they are**
- A refused S3 upload (image on trial) also prints the updater's "No Error" line on the console
  (§12.3.11).
- The S3 upload's wrong-chip reason reaches only the console; the page shows "flash" (§12.3.11).
- Saving the Network card always resends the network name, so changing only NTP or the timezone
  also rejoins 3 s later (§12.3.12).
- The confirmation of a portal-raised access point is on the console only (§12.3.13).
- Console `B` keeps its own place in its list, so its first press after a boot sets 11.0 dBm
  whatever the current rung (§12.3.2).

**Never exercised on the real radio**
- The rescue access point coming back after a **failed** join on the current retry path
  (2026-09-25). The 2026-09-25 test exercised force, hold and return only; that day radiation was
  shown from the S3's side (driver confirmed, probe requests heard at −68 to −87 dBm), and the
  S3 came home by itself after 10.4 min. The 2026-09-24 test, on the older path, was the one where
  a PC joined the access point and signed in (§12.3.16).
- The first-login placeholder gate: it has never armed on the author's unit, whose account is not
  the placeholder. The whole first-time path in 8.2.2 has not been run end to end (§12.3.16).
- The settings download refused while locked (the author never downgrades) (§12.3.16).
- Lockout beyond its first steps, the 507 "full" user path, and session eviction with more than
  four browsers (§12.3.16).
- The 409 refusal of an audio-board setting while the A32 is silent, and the two battery-clock
  pills in a real fault (both v.1.0.2) (§12.3.16, §12.4.17).
- The captive portal from the phone's side (v.1.0.5): a phone joining the open access point and
  opening the sign-in page by itself. Not yet checked (§12.3.16).

**Exercised since v.1.0**
- mDNS after the rescue access point (2026-09-25, on v.1.0.1): the S3 was forced onto its access
  point, came home by itself 10.1 minutes later, and `ambersong.local` answered within 18 s of its
  return and on every check after (11 of 11). No restart is needed (8.2.5).
- A portal reboot while the S3 image was on trial (2026-09-25): the previous build came back.
- Both boards updated over the air to v.1.0.2 and v.1.0.3, on trial and then confirmed
  (2026-09-25 and 2026-09-26); the console `s` link line read "wrong-version 0".
- The open access point and the captive portal's DNS server, from the S3's side (2026-09-27, on
  v.1.0.5): the S3 image went over the air, ran on trial and confirmed itself. The access point was
  then forced at 10:15:57 and the S3 left the house network at 10:16:16. The console showed
  "Access point "Ambersong" raised and the driver confirms it.", "open, no password
  http://192.168.4.1/   channel 1 …" and "captive portal: every name now points at this radio.",
  and probe requests were heard. The S3 came home by itself at 10:26:01 (10.1 min), and
  `ambersong.local` answered from 10:26:19 on.

## 8.8 Changing this area

**Invariants**
1. **The radio must never become unreachable.** A failed join ends in `startAp()`; a forced access
   point ends; radio silence ends. Nothing reachable from the portal may be unbounded.
2. **Never pass the WiFi driver through `WIFI_MODE_NULL`** while the access point or the station
   must survive: no `WiFi.disconnect(true, …)` and no `softAPdisconnect(true)` from a
   single-interface mode. Only `quiet()` does it, on purpose.
3. **Believe the driver, not return values** (`apReallyUp()`, `driverReport()`).
4. **Re-apply the transmit power after every WiFi mode change** (`applyTxPower()`).
5. **HTTP handlers never change the WiFi mode or tear down the link**; they request, and
   `Net::loop()` acts after its grace.
6. **Every API handler starts with `requireLogin()`.** A new two-callback handler must repeat the
   session, administrator and placeholder checks in both callbacks.
7. **Do not add fields to `User`**: its size is the NVS record length, and every stored account
   would be discarded.
8. **Stay inside the connection budget**: no new periodic requests; ride on `/api/state`.
9. **Refusals reach the page as refusals** (`warn` on a 200, or an error status), never as a plain
   acknowledgement.
10. **Clamping and validation live in the machine**, not in the browser (`argNumber()`,
    `settingBound()`, the clamps in `doAction()`).
11. **Web console: one key per send unless `Con.lineWanted`.** A new line prompt sets `lineWanted`
    while it reads, clears it, and calls `drainLine()` after.
12. **A console loop that blocks must ping the A32** (`MSG_PING`), or the audio mutes after 2 s.
    It must also feed the loop watchdog (`wdtFeed()`) if it can wait longer than the 15 s
    timeout, or the S3 restarts. A bounded wait well under 15 s (the `i` scan) need not.
13. **Everything the S3 prints goes through `Con`**; only `console.cpp` touches `Serial`.
14. **The server task yields every pass** (`vTaskDelay(1)`).
15. **The reaper stays off while `gOta` is set**, and `gOta` is cleared before the settings flush on
    the S3 upload's success path.
16. **`mustchg` stays out of the schema** (the schema is cached by tag).
17. **While the S3 image is on trial, nothing is written and a reboot is never refused**
    (`settingsWrite()`, `hReboot()`): the reboot is how a bad update is undone.
18. **The page draws nothing before it has state, and its paint code never throws on a missing
    field** (8.2.8); a throw in the first paint stops the whole page.
19. **An edit of an audio-board row needs the A32's answer** (`a32CfgKnown()` in `hSet()`); a new
    path that pushes A32 settings must check it too.
20. **The captive portal's DNS server lives and dies with the access point** (`gDns` in
    `Net::loop()`): it must stop the moment `gApUp` clears and must never run on the house
    network, where it would answer the house's names with 192.168.4.1. A new path that raises or
    drops the access point must keep `gApUp` in step with it, because the DNS server follows
    `gApUp`.
21. **On the access point the catch-all redirect must name the access point's address in full**
    (`http://192.168.4.1/`), not `/`: the phone's check arrives under another host name.

**Traps**
- Edit `data/portal.html`, never `src/s3/page_gz.h`. The header is regenerated on every
  `pio run -e s3`; it is committed, so a page change shows as a large diff there. Commit both.
- Install `esprima` into PlatformIO's own Python (`pip install esprima` in its virtual environment),
  or the script check silently degrades to a warning and a broken page can ship. The check covers
  only the first `<script>` block; keep one.
- The page is written without a single backslash (it uses `String.fromCharCode(10)` for a
  newline): the tools used to write this code halved backslashes several times. Keep it so, and
  check escapes in C strings byte by byte (as `jsonEscLog()` was).
- The ETag only changes when the HTML does (gzip with its timestamp at 0), and the page is served
  `no-cache`, so after a page change browsers revalidate and get the new page by themselves;
  there is nothing to clear by hand.
- `WebServer::client()` returns a copy; stopping the copy works because it shares the socket.
- `i` blocks for seconds when the S3 is not on the house network (it scans). Expected.
- In AP+STA the station's channel wins and the access point moves; do not pin the station to
  channel 1.
- Silence on the USB console proves nothing about IDF's own errors (they go to UART0).

**How to test**
- **Build**: `pio run -e s3`; look for "portal script: N bytes, parses cleanly" and "portal page: X
  bytes -> Y gzipped …, etag …". Both environments should build with no new warnings.
- **Flash**: identify the S3 by its MAC address before any USB flash; never let PlatformIO pick
  the port (other S3 boards share its USB VID:PID). Find the board in `pio device list` by its
  `SER=` field and pass the port explicitly (`pio run -e s3 -t upload --upload-port <port>`).
  Before an over-the-air update, confirm the address you are sending to is this radio.
- **Wireless updates are for a machine that is working, not for one that is being worked on.**
  On 2026-08-31 three images were pushed over the air into a machine that could not be observed;
  one failed part-way and left a build whose own socket reaper then blocked the fix. Have a
  portal you trust before you let it overwrite firmware.
- **curl proves the machine; only a browser proves the page.** Two portal faults were invisible to
  curl (a page that threw before its first poll, and speculative sockets).
- **Scripts**: sign in once, keep the cookie, and stay at one request every few seconds. The
  author sent updates with a small helper script (not published) that posted `/api/login`
  (credentials from environment variables, never in the script) and then the image as a
  multipart upload to `/api/ota/s3` or `/api/ota/a32`.
- **Checks used on the radio**: an unauthenticated `GET /api/schema` gives 401; a normal account
  gets 200 for `volume` and `bt.pause` and 403 for `taper`, `needle.home` and the settings file;
  out-of-range values come back clamped by the firmware, not the browser (`posMax=99999` was
  clamped to the row's maximum, `showTuning=99` to 2); `posMin=nan` is refused and the stored
  value is unchanged; the settings file writes scaled values in shown units (`gainRadio=15.000`,
  not 150); "status" typed in the web console gives 400.
- **Rescue access point**: with someone at the radio and USB to hand (a failed access point strands
  the radio until a cable is attached), press "Raise the rescue access point"; watch the Console tab
  for "raised and the driver confirms it", "open, no password", "captive portal: every name now
  points at this radio." and "[ap] probe request heard"; join from a phone (force a rescan; no
  password is asked) and **expect the sign-in page to open by itself**; if it does not, open
  `http://192.168.4.1/`; sign in; expect it home about 10 minutes after the last client leaves.
  After it goes home, check that the DNS server stopped: a name looked up on the house network must
  not answer 192.168.4.1. Still owed: the failed-join path (store a wrong password, someone at the radio)
  and the phone opening the page by itself.
- **Transmitter questions**: `i` (driver read-back and a scan), `B` (step the power), `z` (erase the
  RF calibration and reboot).
- After any update, confirm the running version on the System tab.

## 8.9 Reference: HTTP endpoints (S3, port 80, registered in `Portal::begin()`)

"Signed in" = a valid session cookie `amb`. "Gated" = refused with 403 `{"e":"mustchg"}` while the
account is still the placeholder. "Admin" = the slot-0 account. Request bodies are form-encoded
unless stated. `{"ok":1,"m":"…"}` is the generic acknowledgement.

| Method | Path | Who | Parameters | What it does | Answers |
|---|---|---|---|---|---|
| GET | `/` | anyone | header `If-None-Match` | The gzipped page from flash | 200 (gzip, ETag, `no-cache, must-revalidate`); 304 if the ETag matches |
| GET | `/favicon.ico` | anyone | — | Nothing | 204 |
| any | unknown path | anyone | — | Redirect home; on the rescue access point this is the captive portal's redirect (8.2.3) | 302 `Location: /` on the house network; 302 `Location: http://192.168.4.1/` otherwise |
| POST | `/api/login` | anyone | `u`, `p` | Check the salted hash, create a session, set the cookie | 200 "welcome" + `Set-Cookie`; 403 `{"e":"bad"}`; 429 `{"e":"locked","s":N}` |
| POST | `/api/logout` | anyone (ends the session if the cookie is valid) | — | End this session, clear the cookie | 200 "bye" |
| GET | `/api/boot` | signed in; allowed while gated | `sc` = cached schema tag | Schema (or `null` if the tag matches), values and state in one reply `{"sc":…,"val":…,"st":…}` | 200; 401 |
| GET | `/api/schema` | signed in, gated | — | Schema only: tag, user, admin, fw, tabs, setting rows | 200; 401; 403 |
| GET | `/api/values` | signed in, gated | — | `{key: value}` for every setting row | 200; 401; 403 |
| POST | `/api/set` | signed in, gated; admin for admin rows | `k` key, `v` value | Parse the whole number (decimal comma accepted), clamp, store, apply | 200 `{"ok":1,"v":stored[,"clamped":1][,"warn":…]}` (`warn` only for `calLow`/`calHigh` when the tuning curve is in trouble); 404 `{"e":"nokey"}`; 403 `{"e":"admin"}`; 409 `{"e":"the audio board is not answering - its settings cannot be changed now"}` for an audio-board row while the A32's settings are not known (`haveCfg` false, chapter 9); 400 `{"e":"not a number"}` (also when `v` is missing) |
| POST | `/api/act` | signed in, gated; per action (8.10) | `a` action, `v` optional number | `doAction()` | 200 `{"ok":1,"m":…}`; 200 `{"ok":1,"warn":…}` for a refusal, including an unparsable `v` ("REFUSED - not a number: …"); 403 `{"e":"no"}` for an unknown or forbidden action |
| GET | `/api/state` | signed in; allowed while gated | `log` = next console byte (admin only; ignored otherwise) | Live dashboard plus `net{sta,ssid,ip,rssi,ntp}`, `sess`, `ota`, `mustchg`; with `log`, a `log{from,next,t}` slice of up to 4096 bytes | 200; 401 |
| GET | `/api/rda` | signed in, gated, admin | — | The last RDA sweep, bin by bin with the judgement made about each bin (the coarse bins are kept after a refinement), the fine pass's 11 points (`fine`, since v.1.0.2), and the fixed-feature list (chapter 6). It exists because a discriminator that cannot be observed either works or silently discards the signal, and nothing tells you which | 200; 403 text "admin only" |
| POST | `/api/cons` | signed in, gated, admin | `k` = one key, or up to 128 characters while a prompt reads a line | Inject into the console as if typed, newline appended | 200 "sent"; 403 text "admin only"; 400 `{"e":"1 to 128 characters"}`; 400 `{"e":"one key at a time - each character is its own command"}`; 503 `{"e":"console busy"}` |
| GET | `/api/settings.txt` | signed in, gated, admin | — | Download the settings file (`ambersong.txt`, `key=value`); audio-board rows read `n/a` while the A32's settings are not known (chapter 7) | 200 `text/plain` attachment; 403 text; 409 text "Settings are locked: …" |
| POST | `/api/settings.txt` | signed in, gated, admin | raw `text/plain` body | Apply a settings file | 200 `{"ok":1,"m":"N settings applied"[,"warn":…]}`; 403 `{"e":"admin"}` |
| GET | `/api/users` | signed in, gated | — | `{"me":…,"u":[{i,n,a}],"max":4}`; a normal user sees only themself | 200 |
| POST | `/api/passwd` | signed in; allowed while gated | `new`; `old` (required for your own account); `i` target slot (admin only); `name` optional rename | Change the password (new salt) and optionally the name | 200 "password changed" / "credentials changed"; 404 `{"e":"nouser"}`; 403 `{"e":"admin"}`; 403 `{"e":"oldpw"}`; 400 `{"e":"short"}`; 400 `{"e":"longname"}`; 409 `{"e":"taken"}` |
| POST | `/api/user/add` | signed in, gated, admin | `n` (2–16), `p` (≥ 6) | Add a normal user in the first free slot 1–3 | 200 "user added"; 403; 400 `{"e":"name"}` / `{"e":"short"}`; 409 `{"e":"exists"}`; 507 `{"e":"full"}` |
| POST | `/api/user/del` | signed in, gated, admin | `i` (1–3) | Delete the user and drop their sessions at once | 200 "user removed"; 403; 400 `{"e":"no"}` |
| GET | `/api/net` | signed in, gated, admin | — | `{"ssid":…,"ntp":…,"tz":…}` — never the password | 200; 403 `{"e":"admin"}` |
| POST | `/api/net` | signed in, gated, admin | `ssid`, `pass` (blank = keep), `ntp`, `tz` | If `ssid` present: refuse empty, store, rejoin in 3 s. If `ntp` present: store NTP and timezone, apply the timezone, restart SNTP if on the house network | 200 "network saved - the radio joins it in a few seconds"; 400 `{"e":"the network name is empty"}`; 403 |
| POST | `/api/reboot` | signed in, gated, admin | — | If the S3 image is on trial: restart at once, no save (the previous image comes back). Otherwise save settings; if refused (and not locked), stop the needle and retry 15 × 100 ms; restart | On trial: 200 "rebooting - this firmware was still on trial, so the previous firmware comes back". Otherwise 200 "rebooting"; 409 `{"e":"not rebooting - <why>"}` (needle resumed). Restart 200 ms after a 200; 403 |
| POST | `/api/ota/s3` | admin session, not the placeholder (checked in both callbacks) | multipart file (the page uses field `f`) | Write the S3's other application slot, reboot | 200 "rebooting into the new firmware", then restart; 500 `{"e":"flash"}`; 403 text "admin only" / "change the default password first"; 409 `{"e":"this firmware is still on trial - retry in a minute"}` while the running image is on trial (the bytes are received and dropped, nothing restarts) |
| POST | `/api/ota/a32` | same | multipart file | Relay the image to the A32 over the inter-board link | 200 "N bytes sent, the A32 is rebooting"; 500 `{"e":"<reason>"}`; 403 |

State fields added by this chapter's code (`buildState()` in `src/s3/portal.cpp`): `net.sta` (1 =
on the house network), `net.ssid` (the house network's name, or `Ambersong`), `net.ip`,
`net.rssi` (0 on the access point), `net.ntp` (1 = fresh), `sess` (active sessions), `ota` (1 =
upload in progress). `mustchg` comes from `portalStateJson()` in `src/s3/main.cpp`, as do two
fields added in v.1.0.2 that drive header pills: `rtc` (the battery clock's health: 0
fine or not asked yet, 1 answered with no valid time, 2 not answering; 8.2.5, 9.2.14) and `linkver`
(the S3's count of link frames dropped for another protocol version; 9.2.7). The other state
fields belong to other chapters.

## 8.10 Reference: `/api/act` actions (`doAction()` in `src/s3/settings_table.h`)

"Guest" actions are allowed to every signed-in account; all others need the administrator. `v` is
the optional number; absent means 0. A refusal comes back as `warn`. Most actions belong to other
chapters; they are listed so the endpoint is complete.

| Action | Who | `v` | What it does | Chapter |
|---|---|---|---|---|
| `bt.play` `bt.pause` `bt.next` `bt.prev` | guest | — | Bluetooth transport command to the A32 | 10 |
| `bt.disconnect` | guest | — | Drop the Bluetooth source | 10 |
| `bt.pair` | guest | — | Open the pairing window | 10 |
| `bt.forget` | admin | — | Drop every pairing | 10 |
| `needle.stop` | admin | — | Stop the needle | 5 |
| `needle.home` | admin | — | Home (refusal says why) | 5 |
| `needle.sweep` | admin | 0 once, non-zero repeat | Sweep the measured travel | 5 |
| `needle.reindex` | admin | — | Re-index now | 5 |
| `needle.track` | admin | — | Track the tuner | 5 |
| `needle.jog` | admin | half-steps, clamped ±200 (a clamp is reported) | Jog in micro mode | 5 |
| `needle.limitLow` `needle.limitHigh` | admin | — | Set a soft limit here | 5 |
| `needle.nudge` | admin | half-steps, ±1..200 | Move both soft limits together | 5 |
| `needle.calBand` | admin | passes (default 3) | Characterise the index band | 5 |
| `needle.calAbort` | admin | — | Abort a calibration | 5 |
| `needle.tuneLow` `needle.tuneHigh` | admin | — | Record the tuner's end here | 6 |
| `tune.markA` `tune.markB` `tune.markC` | admin | MHz, 87.0–108.5 | Hand mark at this dial position | 6 |
| `tune.nudge` | admin | MHz, ±0.1..±5.0 | Slide the tuning curve (the hand dial correction) | 6 |
| `tune.nudgeZero` | admin | — | Reset the hand dial correction to 0, the only thing that erases it (since v.1.0.2; button "Reset the dial correction to 0"). Answers "dial correction reset to 0", or "the dial correction is already 0" | 6 |
| `tune.clear` | admin | — | Clear every mark; the hand dial correction is kept (the answer says so when one is set) | 6 |
| `tune.drop` | admin | slot 0–11 (the page asks "Which sample slot to drop (0-11)?") | Drop one sample; the hand dial correction is kept | 6 |
| `rda.sample` | admin | — | Measure this dial position with the RDA5807M. Refused outside the listening policy: "the dial calibrates only while you listen to the radio - switch the amp on and select RADIO" | 6 |
| `rda.spurClear` | admin | — | Forget the fixed features | 6 |
| `pot.min` `pot.ctr` `pot.max` | admin | — | Record the volume pot's position | 10 |
| `sys.save` | admin | — | Save settings now (refusal says why) | 7 |
| `sys.rebootA32` | admin | — | Reboot the audio board | 9 |
| `sys.getcfg` | admin | — | Re-read the A32's configuration | 9 |
| `sys.bttx` | admin | 0..5 (−12..+3 dBm) | Bluetooth transmit power, down only | 10 |
| `sys.dcword` `sys.dctest` | admin | index 0..6 / 0-1 | Constant-DC diagnostic | 10 |
| `sys.pot` | admin | 0 ignore / 1 use | Ignore the volume pot (not saved) | 10 |
| `sys.dindrv` | admin | 0..3, clamped | Data-pin drive experiment (not saved) | 10 |
| `sys.clkpair` | admin | 22, 23, 32 or 33 | Clock-pin drive pair (not saved) | 10 |
| `sys.clkdrv` | admin | 2..3, clamped | Clock-pin drive (not saved) | 10 |
| `sys.tail` `sys.zfloor` | admin | 0/1 | Audio experiments (not saved) | 10 |
| **`sys.quiet`** | admin | seconds; absent or ≤ 0 = 120; capped at 600 | Request radio silence; carried out by `Net::loop()` after 500 ms. Answer: "wifi off - it will return by itself" | 8 |
| **`net.forceAp`** | admin | — | Request the rescue access point; carried out after 500 ms, holds 10 min. Answer: "raising the access point - join "Ambersong", then http://192.168.4.1/"; the driver's confirmation is on the console only | 8 |
| `disp.test` | admin | 0 normal, 1 show 8888, 2 blank | Display test | 4 |

`sys.bttx`, `sys.dcword`, `sys.dindrv`, `sys.clkpair` and `sys.clkdrv` have no button on the page;
they are reached only by posting `/api/act` directly (a script, signed in as the administrator).

Retired: `sys.sddly` (2026-09-10 to 09-11). Do not restore it (8.6).

## 8.11 Reference: console commands (S3; USB at 115200 baud, or the portal's Console tab)

Every key is one command, case-sensitive unless two cases are listed. From the web console, send
one key per line. Any key not listed is ignored silently, including the newline the portal appends.
Keys owned by other chapters are listed for completeness.

| Key | Effect | Chapter |
|---|---|---|
| `s` `S` | Status: amp, source, needle, index, drive, encoder, tuning chain, portal loops/served/stack, link (`alive`/`SILENT`, `rx`, `crc`, `wrong-version N`, with "<- the boards run different protocol versions: flash both" when N is not 0), last reset reason and uptime; ends with the command list | all |
| `?` | Command list | — |
| `i` `I` | Network: joined or "OWN ACCESS POINT", name, address, signal; portal up/down, sessions, NTP fresh/stale; access-point clients; `driverReport()` (mode, transmit power, country and channels, access-point configuration read back, beacon interval, PHY, channel, clients); and **only when not on the house network** a blocking `scanReport()` of every network heard | 8 |
| `y` | **Line prompt**: `name <space> password` (60 s, a blank line cancels); stores them and rejoins 3 s later. Echoes what is typed, password included | 8 |
| `Y` | Force the rescue access point up now; holds 10 min (longer while someone is on it), then goes home | 8 |
| `~` | Erase **all** portal accounts and restore the placeholder owner (forgotten-password recovery) | 8 |
| `z` | Erase the WiFi RF calibration in NVS (`esp_phy_erase_cal_data_in_nvs()`) and reboot (transmitter diagnosis; touches none of the user's calibrations) | 8 |
| `B` | Step the transmit power along 60, 44, 34, 28, 20, 8 (quarter-dBm, cycling; the first press after a boot gives 44), save it with the settings, print what the driver applied | 8 |
| `W` `w` | **Line prompt**: local time `YYYY-MM-DD HH:MM:SS` (40 s); converted with a fixed `EST5EDT` rule and set on the S3. Sent to the real-time clock only while the A32 is handshaken; otherwise the console says the battery clock was NOT updated | 8 |
| `l` `L` | Live index monitor (up to 60 s; Enter ignored; any other key stops; pings the A32) | 5 |
| `8` / `b` / `9` | Display test: 8888 / blank / normal | 4 |
| `-` / `=` `+` | Display brightness (radio on) down / up by 15 | 4 |
| `f` `F` | Cycle the readout: clock / TUNING / tuning while tuning | 4 |
| `H` `h` | Home the needle | 5 |
| `T` `t` | Track the tuner | 5 |
| `P` `p` | Park | 5 |
| `x` `X` | Stop the needle (does not stop a calibration) | 5 |
| `A` | Abort a running calibration (the only key that does) | 5 |
| `k` `K` | Calibrate the index band, 5 passes (needle on the sensor first; the portal button uses 3) | 5 |
| `n` / `N` | Nudge −20 / +20 half-steps, slow | 5 |
| `m` / `M` | Set soft limit LOW / HIGH here | 5 |
| `e` | Index edge logging on/off | 5 |
| `r` | Sweep the full measured travel, repeating (`x` stops) | 5 |
| `0`–`4` | Release the coils / hold one coil | 5 |
| `j` / `J` | 200 half-steps forward / reverse, half-step mode (bring-up tool) | 5 |
| `u` / `U` | 200 half-steps forward / reverse, micro-step mode | 5 |
| `g` `G` | Removed 2026-09-03; prints why and points to `m` / `M` | 5 |
| `c` / `C` | Tuner end LOW / HIGH = current encoder count (refused if the encoder does not answer) | 6 |
| `R` | RDA5807M state and where the local oscillator should be | 6 |
| `q` | RDA wide sweep (about a minute) | 6 |
| `v` | RDA narrow sweep around the prediction (about 10 s) | 6 |
| `a` | Print the last sweep (the coarse bins, kept after a refinement), then the fine pass's points at 50 kHz, with the chosen one marked | 6 |
| `o` / `O` | Mark the strongest peak as a fixed feature / forget them all | 6 |
| `V` | Measure this dial position with the RDA and store it as a sample | 6 |
| `D` | Dump all settings as text, including the IF (`ifOffset`, since v.1.0.2) and the learned transmit power (`wifiTxQ`, which the settings file no longer carries) | 7 |
| `Z` | Zero the jitter and display slot-error counters | 5, 4 |

Removed in v.1.0.2: `,` and `.` (the up-leg top speed; `.` had no ceiling). They now
fall under "any key not listed is ignored".

## 8.12 Reference: page controls and the calls they make (`data/portal.html`)

Settings widgets are not listed: every one of them posts `/api/set` with its key (8.2.8).

| Where | Control | Call |
|---|---|---|
| Sign-in form | Sign in | `POST /api/login` (`u`, `p`), then `/api/boot` |
| Set your account (placeholder only) | Save | `POST /api/passwd` (`old` = the placeholder password, `new`, `name`); drops the cached schema and reloads |
| Now, Music card | Play, Pause, ◀◀, ▶▶, Disconnect, Pair | `/api/act` `bt.play`, `bt.pause`, `bt.prev`, `bt.next`, `bt.disconnect`, `bt.pair` |
| Now, Music card | Volume slider, Mute | `/api/set` `volume`, `muted` |
| Audio, Actions (admin) | Pot: minimum / centre / maximum; Re-read the A32 | `pot.min`, `pot.ctr`, `pot.max`, `sys.getcfg` |
| Needle, Actions (admin) | Home; Re-index; Sweep once / repeatedly; Track the tuner; STOP; ◀ 100, ◀ 10, 10 ▶, 100 ▶; Set low / high limit here; Calibrate the index; Abort calibration; Tuner = low / high end | `needle.home`, `needle.reindex`, `needle.sweep` 0 / 1, `needle.track`, `needle.stop`, `needle.jog` ±100 / ±10, `needle.limitLow` / `High`, `needle.calBand` 3, `needle.calAbort`, `needle.tuneLow` / `High` |
| Needle, Actions (admin) | Mark station A / B / C here (asks for MHz); Clear all marks; Dial −0.1 / +0.1 MHz; Reset the dial correction to 0; Needle Limit ◀ 20 / 20 ▶; Measure this dial position; Drop one sample (asks for a slot); Forget the fixed features | `tune.markA` / `B` / `C`, `tune.clear`, `tune.nudge` ∓0.1, `tune.nudgeZero`, `needle.nudge` ∓20, `rda.sample`, `tune.drop`, `rda.spurClear` |
| Display, Actions (admin) | Show 8888 / Blank / Normal | `disp.test` 1 / 2 / 0 |
| Bluetooth, Actions | Pair a new device; Disconnect; Forget every pairing (admin) | `bt.pair`, `bt.disconnect`, `bt.forget` |
| System, Settings file (admin) | Download; Upload what is below (the text box) | `GET /api/settings.txt` (refused on the page with a toast while settings are locked); `POST /api/settings.txt` (raw text, 20 s deadline) |
| System, Network (admin) | Save network (disabled until the form is filled) | `GET /api/net` to fill the form; `POST /api/net` (`ssid`, `pass`, `ntp`, `tz`) |
| System, Firmware (admin) | Update the main board; Update the audio board | `XMLHttpRequest` `POST /api/ota/s3` / `/api/ota/a32`, form field `f`, with a progress bar |
| System, Machine (admin) | Save settings now; WiFi off for 2 minutes; Constant DC test ON / off; Zero-data floor ON / off; Ignore the volume pot / Volume pot back on; Sign-extended tail ON / off; Reboot the audio board; Reboot the main board (asks to confirm) | `sys.save`, `sys.quiet` 120, `sys.dctest` 1 / 0, `sys.zfloor` 1 / 0, `sys.pot` 0 / 1, `sys.tail` 1 / 0, `sys.rebootA32`, `POST /api/reboot` |
| System, Actions (admin) | Raise the rescue access point | `net.forceAp` |
| Console (admin) | Send, or Enter | `POST /api/cons` (`k`); output through `/api/state?log=` |
| Account | Change it (my password) | `POST /api/passwd` (`old`, `new`) |
| Account (admin) | Add a normal user; Remove | `POST /api/user/add` (`n`, `p`); `POST /api/user/del` (`i`) |
| Account | Sign out | `POST /api/logout` |

---

# 9. The A32 and the link between the boards

## 9.1 What it does

The radio has two microcontrollers. The **S3** (an ESP32-S3) runs the clock display, the dial needle,
the panel lamps, the amplifier sensing, the WiFi portal and network time. The **A32** (a classic
ESP32, Bible §1) runs all the audio: Bluetooth, the capture of the tube radio, the output converter,
the source selector and the volume knob. It also talks to the real-time clock chip (Bible §5). The two boards talk over one serial line, **the link**. Everything that has to
cross from one half of the machine to the other goes through it: the time of day, whether the
amplifier is switched on, every audio and Bluetooth setting the portal edits, the A32's live state
for the portal, and the A32's own firmware updates, because the A32 has no other road for an update
than through the S3. When all of this works, the listener sees nothing of it. When the S3 stops
talking, the A32 deliberately goes silent within two seconds: no audio, Bluetooth closed and dark.
By design, a dead S3 is obvious. When new A32 firmware is uploaded through the portal,
the audio fades out, the upload takes about a minute, and the A32 restarts on the new image. That
image is **on trial**: if it crashes, hangs, or cannot talk to the S3, the next reset puts the
previous image back by itself. While the image is on trial the A32 writes no settings: a change
made from the portal takes effect at once, but it is kept in memory and written to flash only
when the image is confirmed (since v.1.0.4). A frozen A32 restarts itself within 15 seconds.

## 9.2 How it works

### 9.2.1 Who owns what

| Thing | Owner | How the other board learns it |
|---|---|---|
| Time of day, UTC | the DS3231 on the A32; NTP on the S3 disciplines it | `MSG_TIME` (A32 to S3), `MSG_SET_TIME` (S3 to A32) |
| Time zone, display of time | the S3 | never sent; everything on the link is UTC |
| Amplifier on or off | the S3 senses it | `MSG_SET_SYS` (S3 to A32) |
| Audio and Bluetooth settings | the A32 stores them in its own flash | the S3 keeps a read-only **mirror** (`MSG_CFG`) and pushes edits (`MSG_SET_AUDIO`, `MSG_SET_BT`) |
| Source, volume, Bluetooth state, meters, diagnostics | the A32 | `MSG_STATE` every 250 ms |
| A32 firmware image | uploaded to the S3's portal | relayed to the A32 over the link (`MSG_OTA_*`) |

There is no reset wire from the S3 to the A32 (Bible §2, §3), and the A32 firmware offers no
wireless update path of its own, so after its first USB flash the only way to reach the A32 from
outside the cabinet is through the link. Much of this chapter follows from that one fact.

### 9.2.2 The A32 boot sequence

`setup()` in `src/a32/main.cpp` runs these steps in this order. Each one sits where it does for a
reason.

1. **The task watchdog, first.** `esp_task_wdt_init(WDT_TIMEOUT_S, true)` sets the task watchdog to
   15 s with panic (restart) on expiry, and `enableLoopWDT()` puts the Arduino loop task under it.
   `setup()` itself runs on that task, so from this line on, any hang, including a Bluetooth start
   that never returns, ends in a restart. `gLoopWdtOn` records whether the subscription took, for the
   boot report. See 9.2.5.
2. **The image state.** `esp_ota_get_running_partition()` and `esp_ota_get_state_partition()` tell
   whether this image is on trial. The result becomes `gImgOnTrial` and a label for the boot report:
   | State from the bootloader's records | Label | On trial |
   |---|---|---|
   | `ESP_OTA_IMG_PENDING_VERIFY` | `ON TRIAL (rollback armed)` | yes |
   | `ESP_OTA_IMG_VALID` | `valid` | no |
   | `ESP_OTA_IMG_NEW` | `NEW - this bootloader has no rollback` | no |
   | anything else, or no record | `not tracked (USB flash)` | no |

   An image flashed over USB is never on trial.
3. **The USB console** at 115200 baud, and a banner with `FW_VERSION` and `FW_COMMIT` (9.3.4). This console needs a cable
   and the cabinet is normally shut, so the lines that matter are also sent to the S3 later.
4. Input set-up for the selector and the knob, the Bluetooth lamp, and `settingsLoad()` (chapter 10
   describes these).
5. **The clock chip.** `Rtc::begin()` starts the I2C bus, and one `Rtc::read()` prints
   `DS3231 responding, time valid` or `time INVALID (lost power)` and the chip temperature to the
   local console.
6. **The protocol self-test.** `protoSelfTest()` in `include/link.h` encodes a `ProtoState` frame,
   feeds it back byte by byte through a decoder, checks the type, the length and a byte-exact round
   trip, then flips one bit and requires the checksum to reject the frame. ("A checksum that has
   never been seen to fail is not known to work.") On failure the A32 **does not halt**. It sets
   `protoBroken` and carries on, so the link and the update receiver still come up and the build can
   be replaced over the air. While `protoBroken` is set, audio stays muted, Bluetooth stays closed,
   and `loop()` says `proto self-test FAILED - muted, BT closed, awaiting OTA` every 10 s on the
   console and to the S3.
7. `Audio::begin()` and `settingsApply()`.
8. **Bluetooth.** The A2DP sink is configured and started, its transmit power applied, and the radio
   starts invisible (`applyScanMode(true)`).
9. **The link.** `gLink.onMessage(onMessage)` then `gLink.begin(Serial2, A32_UART_RX, A32_UART_TX)`.
   The link comes after Bluetooth; 9.4 explains why moving it earlier was dropped.
10. The first reading of the selector, `Audio::setSource()`, and `applyMute()`. This call
    establishes the rule that the A32 is asleep, and so muted, until the S3 speaks (9.2.6).
11. `gSetupMs = millis()`, kept for the boot report. On the radio, setup took 1066 ms (2026-09-25).

### 9.2.3 The A32 main loop

`loop()` in `src/a32/main.cpp` does, in order: `gLink.poll()` (read the link); `serviceWake()`
(asleep or awake, before anything that depends on it); the Bluetooth lamp; the debounced settings
save; any pending Bluetooth disconnect; the selector every 50 ms; the **update give-up** (an update
with no accepted data for 20 s is abandoned: `otaGiveUp("the S3 stopped sending")`);
**`confirmTick()`** (9.2.13); the `protoBroken` announcement; the knob; the Bluetooth state and
its scan-mode refresh; the pair button; **one `MSG_STATE` frame every 250 ms**, sent whether or
not the S3 has ever answered; then the zero-data watch's console lines and the local console
keys (chapter 10). The Arduino core feeds the watchdog once per pass of `loop()`.

**The A32's settings save.** `settingsFlush()` in `src/a32/main.cpp` writes the audio settings, the
Bluetooth settings and the three knob calibration points to NVS namespace `amb`, 2 s after the last
change (the debounce). Since v.1.0.2 only a write that reached flash counts as saved: every
`put` must report the full length written. If any part fails, the settings stay marked unsaved, the
debounce starts again (so the write is retried 2 s later), and the A32 says so on its own console and
to the S3, which prints `[A32] settings NOT saved - the flash write failed; retrying`. Before, the
"unsaved" flag dropped before the write and every result was ignored, so a failed write read as saved
and the change was gone at the next reboot. `MSG_REBOOT` forces a save and then restarts, whether or
not that save succeeded. The failure path has not been exercised on the radio.

**Nothing is written while the image is on trial** (v.1.0.4, v.1.0.4, 2026-09-26). While
`gImgOnTrial` is set and `gImgConfirmed` is not (9.2.13), `settingsFlush()` returns before it
touches flash, forced or not. A change from the portal is still applied at once and marked
unsaved; it simply waits in RAM. When `confirmImage()` confirms the image, it calls
`settingsFlush(true)` straight away, and if that write reached flash it sends
`settings changed during the trial are now saved` to the S3, which prints it as `[A32] ...`
next to `[A32] image confirmed (...)` (the settings line is sent first). If that write fails, the ordinary retry above takes
over (and says so). The reason is the rollback: an update may change the size of `ProtoAudio` or
`ProtoBtCfg`, and `settingsLoad()` accepts a stored structure only if its size matches exactly.
Had the trial image saved its new layout and then been rolled back, the previous image would have
found a structure of the wrong size and run that structure on its defaults. With nothing written
until confirmation, a rolled-back image always finds its settings as it left them. The cost: a
change made during the trial minute is lost if the trial ends in a reset. That includes "reboot
the audio board" during the trial: `MSG_REBOOT`'s forced save writes nothing on trial, and the
restart is a rollback anyway. This is the S3's v.1.0.1 hold (chapter 3, 3.2.9), brought to the
A32.

Proven on the radio on 2026-09-26, with the v.1.0.4 image on trial (11:34 to 11:36): during the trial the
Bluetooth lamp's level `btLedOff` was changed from 255 to 254 in the portal, which answered ok; the A32 wrote
nothing until 11:35:40, when it sent `image confirmed (a minute of running with the S3)` together
with `settings changed during the trial are now saved`. The level was put back to 255 at 11:36:36.

### 9.2.4 The boot report

`bootReport()` in `src/a32/main.cpp` sends one `MSG_LOG` line per A32 boot, at the **first HELLO
from the S3**. The A32's own console needs a cable; the S3's console (also shown in the portal's
Console tab) is where this line is read. The S3 prints it with an `[A32]` prefix:

```
[A32] boot: commit <hash>, reset reason 3, setup 1066 ms, image ON TRIAL (rollback armed), watchdog on
```

| Field | Source | Meaning |
|---|---|---|
| commit | `FW_COMMIT` | the source the running image was built from: the short git hash, with `-dirty` when tracked files had uncommitted changes, or `nogit` (9.3.4) |
| reset reason | `esp_reset_reason()` | the framework's reset-cause number: 1 power-on, 3 software restart (what an update or "reboot the audio board" produces), 4 panic, 6 task watchdog, 9 brown-out |
| setup | `gSetupMs` | how long `setup()` took |
| image | the label from step 2 of 9.2.2, or `valid (confirmed)` once confirmed | trial status |
| watchdog | `gLoopWdtOn` | `on` or `OFF` |
| suffix | `esp_ota_get_last_invalid_partition()` | ` - an earlier update was ROLLED BACK` when the bootloader has marked a slot invalid |

This line is how a rollback or a watchdog restart becomes visible without the USB cable. It is sent
once per A32 boot: if the S3 restarts later and handshakes again, the report is not repeated.

The commit field arrived with v.1.0 (2026-09-25). The version itself is not in this line;
it follows in the handshake (`[PASS] A32 up: ...`), so the two lines together name both the binary
and its source. A rollback proven on 2026-09-25 read, in full: `boot: commit <hash>, reset reason
6, ... image valid, watchdog on - an earlier update was ROLLED BACK`, where 6 is the task watchdog.
The report has not changed since v.1.0. After the v.1.0.2 update the same evening it read `boot:
commit <hash> ... image ON TRIAL (rollback armed) ...`, followed a minute later by `[A32] image
confirmed (a minute of running with the S3)`; v.1.0.3 and v.1.0.4 went through
the same cycle on 2026-09-26, and v.1.0.4 added, at its confirmation, `[A32] settings changed during
the trial are now saved` (9.2.3).

Other `MSG_LOG` lines the A32 sends, all printed by the S3 as `[A32] ...`: the confirmation result
(9.2.13), the settings held during the trial now saved (9.2.3), the `protoBroken` announcement every
10 s (9.2.2), a failed settings save (9.2.3), and a Bluetooth disconnect that did not land.

### 9.2.5 The task watchdog

A **task watchdog** is a timer that restarts the chip when a watched task stops checking in. The
framework already had one before 2026-09-25: 5 s, panic on, watching only the idle task of core 0.
A frozen `loop()` or a Bluetooth start that never returned went unnoticed, and the portal's "reboot
the audio board" is a message that a frozen loop cannot read. Since 2026-09-25:

- `esp_task_wdt_init(15, true)` re-initialises the existing watchdog with a 15 s timeout. This also
  changes the core-0 idle check from 5 s to 15 s, because the timeout is shared.
- `enableLoopWDT()` subscribes the Arduino loop task. The core resets the timer once per `loop()`
  pass.
- Because `setup()` runs on the same task, `setup()` must also finish in 15 s.

15 s leaves room for the Bluetooth start and for the longest legitimate wait inside one `loop()` pass,
which is the image check at the end of an update (`Update.end(true)`).

### 9.2.6 Awake and asleep: what the A32 does when the S3 goes silent

The A32 has one rule, evaluated on every pass in `serviceWake()`:

```
awake = ampOn && gLink.peerAlive()
```

- `ampOn` is the last amplifier state the S3 sent in `MSG_SET_SYS`.
- `gLink.peerAlive()` is true when a valid frame arrived from the S3 within the last 2000 ms
  (`PROTO_SILENCE_MS`).

Only frames **from the S3** keep the A32 awake: HELLO before the handshake, PING and SET_SYS after it.
`serviceWake()` acts only on the **edge**, because running the teardown on every pass would re-send a
disconnect forever.

**Falling asleep** (amp switched off, or 2 s without a valid frame from the S3):

1. Console: `ASLEEP - amp is down` or `ASLEEP - the S3 has gone quiet`.
2. If the S3 is the reason, `ampOn` is cleared. The A32 must not wake up on the S3's last,
   possibly stale, "amp on": the amp may have been switched off while the S3 was restarting. It
   stays asleep until a fresh `MSG_SET_SYS` says the amp is on.
3. The phone is paused and then disconnected (`gracefulDisconnect(BT_OFF)`), the audio muted, the
   Bluetooth lamp dark, and the radio made invisible and non-connectable.

**Waking** (amp on and the S3 talking): any disconnect still pending from the last 250 ms is
cancelled, the mute is re-evaluated, and the Bluetooth state and visibility are recomputed.

**The mute is a function of state, not an event.** Every place that could change it calls one
function:

```
applyMute()  ->  Audio::setMute(protoBroken || !awake || cfgA.muted)
```

An earlier version muted on the sleep edge while four other places unmuted, so turning the source
knob while the S3 was dead brought the sound back and the alarm vanished (fixed 2026-09-01).

**Asleep is not off.** The link, the clock chip and the update receiver stay up. They are what the S3
needs from the A32, and the only reason for it to be awake at all. The A32 keeps sending `MSG_STATE`.

What the listener sees:

| Event | What happens |
|---|---|
| Power-up | A brief silence until the S3 has handshaken and sent its first "amp on". Accepted. |
| The S3 restarts (its own update, a portal reboot, a crash) | Within 2 s: audio fades out, the phone is dropped, Bluetooth lamp dark. When the S3 is back and says the amp is on, the A32 wakes. The phone must reconnect. |
| The link cable fails in one direction | If the S3-to-A32 direction is cut, the same as an S3 restart, for as long as the fault lasts. Either way, the board whose receive direction is cut reports the link as silent on its console (`link SILENT rx 0` on the S3). |

On the S3 side, anything that stops the S3's `loop()` must keep the A32 fed. The console prompts and
the limit monitor call `gLink.poll()` while they wait, and the limit monitor also sends a PING on each
pass (`limitMonitor()` in `src/s3/main.cpp`). The WiFi prompt's `y` can still hold the S3's loop for
up to 60 s; that was left as it is on purpose (9.4).

### 9.2.7 The link: the `Link` class

`include/link.h` and `include/proto.h` are shared by both builds. `Link` is a thin wrapper around a
hardware serial port and the frame decoder.

The firmware drives the link as a UART at 921600 baud, 8 data bits, no parity, one stop bit, with no
flow control, on `Serial2` on both boards. The wiring is S3 GPIO13 (TX) to A32 GPIO26 (RX) and A32
GPIO27 (TX) to S3 GPIO14 (RX) (Bible §4, §12). The 921600 baud rate is the firmware's own choice,
and the cable carries it here: every frame is checksummed, and the counters below show a clean link
(`crc 0`).

| Member | What it does |
|---|---|
| `begin(port, rxPin, txPin, baud)` | Sets a 4096-byte receive buffer, then opens the port. **Explicit pins are required: there is no version without them.** |
| `poll()` | Reads every available byte into the decoder. On each complete, valid frame: stamps the receive time, counts it (`rxCount()`), calls the handler. Never blocks. |
| `send(type, payload, len)` | Encodes the frame into a stack buffer and writes it with one `write()` call. Refuses (and counts it) if the payload is too big. |
| `peerAlive()` | A valid frame within the last 2000 ms. |
| `notePeerHello()` | Stores the other board's version string and protocol number. |
| `rxCount()`, `txCount()`, `badCrc()`, `badVer()` | Counters: valid frames in, frames out, frames with a bad checksum, frames with another protocol version. |

Three details matter:

- **Explicit pins.** On the classic ESP32, `Serial2`'s default pins are GPIO16 and GPIO17, and this
  firmware uses GPIO17 as the I2S frame clock (`A32_I2S_LRCK` in `include/pins.h`), the pin that
  carries both converters' LRCK (Bible §3, §5). A bare `Serial2.begin(baud)` would
  silently take that pin and the audio would fail much later with no obvious cause. The trap is made
  impossible rather than documented.
- **The buffer size is set before `begin()`.** The core ignores `setRxBufferSize()` once the port is
  running and keeps its 256-byte default. 256 bytes is enough for control frames, which is why the
  first version seemed to work, but 1 kB update frames at 921600 baud overflow it.
- **Two writers on the S3.** On the S3 the link is written from `loop()` on core 1 and from the
  portal task on core 0 (the update relay). Frames do not interleave because each frame goes out in
  one `write()`, and the core's UART write holds a lock for the whole call.

The S3 publishes `linkrx`, `linktx`, `linkcrc`, `hello` (handshake done), `astate` (a parsed state
frame is in hand) and, since v.1.0.2, `linkver` (its `badVer()` count) in its status for
the portal. Its console `s` prints the same on the link line. A healthy pair reads, for example,
`link : alive  rx 489  crc 0  wrong-version 0`; during a protocol mismatch the line reads (the count
here is invented):

```
  link       : SILENT  rx 0  crc 0  wrong-version 412
               <- the boards run different protocol versions: flash both
```

(On the console the arrow text follows on the same line.) How to read it: `rx 0` with
`wrong-version 0` points at wiring or power; `rx` climbing with `crc` climbing points at signal
quality; `wrong-version` climbing means the other board is talking but was built with another `PROTO_VERSION` (9.8.1). While `linkver` is not zero and there is no
handshake, the portal shows the red pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both".
`badVer()` counts from the S3's boot, so after a mismatch has been fixed the console figure stays
above zero until the next S3 restart; the pill goes away at the handshake.

### 9.2.8 The frame

```
+------+------+-----+------+--------+--------+-------------------+--------+--------+
| 0xA5 | 0x5A | ver | type | len_lo | len_hi | payload (len)     | crc_lo | crc_hi |
+------+------+-----+------+--------+--------+-------------------+--------+--------+
 start marker  \_____________ covered by the CRC16 _____________/
```

- **Start marker** `0xA5 0x5A` (`PROTO_SOF0`, `PROTO_SOF1`). It is not covered by the checksum: it
  is a resynchronisation marker, not data.
- **ver** is `PROTO_VERSION`, currently 4. A frame with any other version byte is dropped by the
  decoder and counted in `badVer`.
- **type** is the message id (9.3.2).
- **len** is the payload length, little-endian, at most `PROTO_MAX_PAYLOAD` = 1088 bytes (a
  1024-byte update chunk plus room for its header).
- **CRC16**: poly 0x1021, initial value 0xFFFF, no final XOR, computed bit by bit over ver, type, len
  and the payload, sent little-endian. This is the variant usually called CRC-16/CCITT-FALSE; the
  ASCII string `123456789` gives 0x29B1. It is bitwise, not table-driven: "A 512-byte table would buy
  nothing and cost cache the display ISR wants."
- **Packed structs, copied verbatim.** Both ends are little-endian Xtensa built by the same
  compiler, so payload structs are `__attribute__((packed))` and copied byte for byte. This is a
  recorded shortcut; if a third kind of device ever joins the link, the encoding has to become
  byte-oriented.

`protoEncode()` in `include/proto.h` returns the frame length, or 0 and writes nothing if the frame
would not fit, so "a caller that ignores the return value still cannot emit a torn frame".

**Worked examples** (bytes in hexadecimal, as sent):

```
PING, empty payload:         A5 5A 04 03 00 00 61 17
PONG, empty payload:         A5 5A 04 04 00 00 F1 92
TIME, 8-byte payload:        A5 5A 04 21 08 00 | 80 3B B1 6A  01  64  00 00 | 17 8E
                                                 unixUtc       valid tempC4 reserved
                                                 1790000000    1     100 = 25.00 C
                                                 (2026-09-21 14:13:20 UTC)
```

Frame sizes (8 bytes of overhead plus the payload): HELLO 33, STATE 138, CFG 41, TIME 16, SET_AUDIO
23, SET_BT 20, SET_SYS 12, OTA_STATUS 46, OTA_DATA up to 1038, empty messages 8. At 921600 baud,
8N1, the line carries about 92 kB/s: a full update frame takes about 11.3 ms, and the A32's state
reports cost about 550 bytes per second.

### 9.2.9 The decoder

`ProtoFramer::feed()` in `include/proto.h` takes one byte at a time, so it can be fed straight from
the UART with no assumption that frames arrive whole.

```
          0xA5            0x5A           == PROTO_VERSION
  S_SOF0 ------> S_SOF1 ------> S_VER --------------------> S_TYPE -> S_LEN0 -> S_LEN1
    ^  ^           |  ^            |                                             |
    |  |  other    |  | 0xA5       | other: badVer++                             | len > 1088:
    |  +-----------+  +--(stay)    +--------------------> S_SOF0                 | overruns++ -> S_SOF0
    |                                                                            |
    |                              len == 0 --------------------+                v len > 0
    |                                                           v
    +---------- S_CRC1 <----------------------------------- S_CRC0 <---- S_PAYLOAD (len bytes)
      always back to S_SOF0;
      CRC good -> return true (one complete frame)
      CRC bad  -> badCrc++
```

- A second `0xA5` while waiting for `0x5A` keeps the decoder in S_SOF1: it may be the real start of
  a frame whose predecessor was cut short.
- After the last CRC byte the decoder always goes back to hunting from the **next** byte. It does not
  go back and rescan the bytes inside a frame that failed. The consequence is in 9.5.
- `as<T>()` copies the payload into a struct **only if the length is exactly `sizeof(T)`**. A
  mismatch means the two builds disagree about the struct, and a partial copy would be worse than
  none. The frame already passed its checksum, so a length mismatch is a version skew, not noise.
- Single-byte commands are checked with `f.length() == 1` (two bytes for `MSG_CLK_PAIR`) rather than
  with `as()`. `MSG_OTA_DATA` is parsed by hand at fixed offsets (9.3.3).

### 9.2.10 Handshake, heartbeat and liveness

The S3 drives the conversation from its `loop()` (the "link keepalive" block in `src/s3/main.cpp`).
The A32 answers in `onMessage()` in `src/a32/main.cpp`.

```
 S3 (loop, core 1)                                 A32 (loop)
 -----------------                                 ----------
 boot, peerHello = false                           boot, asleep: muted, Bluetooth dark
                                        <--------  STATE every 250 ms, unprompted, from boot on
                                                   (the S3 fills its mirror; haveState = true)
 while !peerHello, every 1000 ms:
   HELLO {PROTO_VERSION, S3 FW_VERSION}  -------->  notePeerHello()
                                        <--------  HELLO_ACK {PROTO_VERSION, A32 FW_VERSION}
                                                   bootReport()        (once per A32 boot)
                                        <--------  LOG "boot: reset reason ..."
                                                   gHelloAt = now      (once; starts the
                                                                        60 s confirmation clock)
 on HELLO_ACK: peerHello = true
   console "[PASS] A32 up: proto v4, firmware <A32 version>"
   GET_TIME  ---------------------------------->  Rtc::read()
   GET_CFG   ---------------------------------->  collect settings
                                        <--------  TIME {unixUtc, valid, tempC4}
                                        <--------  CFG {audio, bt, pot calibration}
                                                   (S3: mirror a32cfg, haveCfg = true)
 while peerHello:
   PING every 1000 ms   ----------------------->  PONG (the S3 ignores PONG)
   SET_SYS {ampOn} every 2000 ms, and at once
     when the debounced amp state changes ---->  ampOn = s.ampOn, visibility recomputed
                                                   serviceWake(): awake = ampOn && peerAlive()
   GET_TIME every 60 000 ms  ------------------>  TIME
   SET_TIME when NTP is fresh, at most hourly ->  Rtc::write()
```

**When the S3 starts over.** The S3 clears `peerHello` and `haveState` and goes back to sending HELLO
every second when:

1. nothing valid has arrived from the A32 for 2 s. Console: `[WARN] A32 silent. Clock and needle keep
   running.` In this case the S3 also forgets the A32's settings (`haveCfg`, 9.2.11).
2. the A32's clock went backwards in a STATE frame: the A32 restarted faster than the 2 s rule could
   notice. The test is `prevMs && prevMs < 0xF0000000 && a32Ms + 5000 < prevMs`. The upper bound
   excludes the 49.7-day wrap of `millis()`; the 5 s of slack tolerates reordering. Console:
   `[WARN] the A32 restarted - handshaking again.` This was added after the A32 update of 2026-09-24,
   when the S3 kept showing the old A32 version and the old settings mirror until the S3 itself was
   restarted (2026-09-24).
3. after every A32 update, successful or unconfirmed, so the version shown is the one the A32 comes
   back with (2026-09-24, 2026-09-25).

**Which frames keep which side alive.** The S3 counts the A32 as alive as long as frames arrive from
it, and the A32 sends STATE four times a second, so the S3 ignores PONG. The A32 counts the S3 as alive
only on frames from the S3.

**What the handshake checks.** The S3's HELLO_ACK handler refuses a HELLO_ACK of the wrong size
(`[FAIL] HELLO_ACK size - version skew`). It also compares the protocol numbers, but a frame with
another protocol version never reaches it (9.3.4). A protocol mismatch shows instead in the S3's
wrong-version counter, on the console and as a portal pill (9.2.7).

### 9.2.11 What the S3 does with the A32's frames

`onMessage()` in `src/s3/main.cpp`:

- **HELLO_ACK**: size check, note the peer, set `peerHello`, print, send GET_TIME and GET_CFG.
- **STATE**: parsed into the mirror `a32`. On success, `haveState = true`, the restart test above
  runs, and the mirror's volume and mute are copied from the state (the knob moves the volume and the
  A32 does not resend its settings when it does). On a length mismatch, `haveState = false` and, once
  per boot, `[WARN] the A32's STATE frame does not match this build. Flash BOTH MCUs. Audio is
  unaffected - only telemetry stops.` Showing the last parsed values instead would report a
  measurement that was not taken.
- **TIME**: re-anchors the S3's clock (9.2.14).
- **CFG**: fills the settings mirror `a32cfg` and sets `haveCfg`.
- **OTA_STATUS**: fills the acknowledgement fields read by the portal task during an update.
- **LOG**: printed as `[A32] <text>`.

**The settings mirror.** The A32 owns and saves its own settings; the S3 keeps a copy so the portal
can show what is really in the machine. The copy is refreshed at every handshake, because a
restarted A32 comes back with the values from its own flash. `haveCfg` means "the A32 has sent its
settings since it last went silent": the CFG frame sets it, and since v.1.0.2 the S3
clears it, with `peerHello`, when nothing valid has come from the A32 for 2 s (the other re-handshake
causes in 9.2.10 keep it; the settings are read again at the handshake anyway). While it is false:

- a portal edit of an A32-owned row is refused with **409 "the audio board is not answering - its
  settings cannot be changed now"** (`hSet()` in `src/s3/portal.cpp`, through `a32CfgKnown()`). A push
  built from an unread, all-zero copy would wipe gains, fades and lamp levels, and the A32 would save
  it; a push into a silent link would go nowhere and be overwritten when the A32 came back;
- settings export writes `n/a` for A32-owned rows, and import skips and names them (chapter 7).

The next handshake asks for the settings again (GET_CFG) and sets `haveCfg` once they arrive.

### 9.2.12 An A32 update, end to end

The A32 cannot be reached by WiFi. An admin uploads the A32 image to the S3's portal
(`POST /api/ota/a32`), and the S3 relays it over the link, one kilobyte at a time. The S3 stores
nothing: it is a pipe.

```
 Browser          S3 portal task (core 0)                  S3 loop (core 1)    A32
 -------          -----------------------                  ----------------    ---
 POST /api/ota/a32 (admin; refused while the
                    default password is unchanged)
   UPLOAD_FILE_START
                  a32OtaBegin():
                    if !peerHello: fail "the A32 is not answering"
                    BEGIN {size 0, crc 0, S3 FW_VERSION} -------------------->  on trial and not confirmed?
                                                                                  STATUS {4, "still on trial -
                                                                                          retry in 1 min"}
                                                                                otherwise:
                                                                                  remember the user's mute
                                                                                  mute; delay(400)
                                                                                  Update.begin(size unknown)
                                                                                  STATUS {1, "receiving"}
                    wait up to 20 s for STATUS  <---- (loop polls the link and sets the ack fields)
                    state 4 -> fail "A32: <detail> (err N)"
   UPLOAD_FILE_WRITE, first chunk:
                    byte 0 == 0xE9 and chip id (bytes 12-13) != 0x0000?
                      -> ABORT; "that image is not for the audio board
                         (wrong chip) - nothing was written"
   UPLOAD_FILE_WRITE, each chunk:
                  a32OtaChunk(): re-cut into 1024-byte frames
                  otaSendFrame():
                    DATA {offset, len, bytes} ------------------------------->  offset == bytes so far?
                                                                                  Update.write(), CRC32 += bytes
                                                                                (otherwise: ignored)
                                               <--------------------------------  STATUS {1, received = total}
                    up to 4 tries, 3 s each:
                      received == offset + len -> accepted, next frame
                      received == offset       -> not taken, send again
                      anything else            -> fail "A32 is at X, we are at Y"
                      4 silences               -> fail "no answer at N bytes"
   UPLOAD_FILE_END
                  a32OtaEnd(): send the last partial frame,
                    END {crc32} ------------------------------------------------>  crc32 differs? give up "crc mismatch"
                                                                                Update.end(true): image checked,
                                                                                  boot slot switched
                                                                                  (fails -> "end failed")
                                               <--------------------------------  STATUS {3, "ok, rebooting"}
                                                                                delay(250); ESP.restart()
                    wait up to 4 s for state 3 or 4 (late DATA acks skipped)
                      none: ABORT, re-handshake, "the A32 did not confirm the
                            update - check its version once it reconnects"
                      3:    console "A32 image sent: N bytes, crc X", re-handshake
 hOtaA32End(): 200 "N bytes sent, the A32 is rebooting"
               or 500 {"e": "<reason>"}
                                                          HELLO every 1 s ---->  new image boots ON TRIAL
                                                                          <----  HELLO_ACK, then the boot report
                                                                                60 s after that HELLO:
                                                                          <----  LOG "image confirmed (a minute
                                                                                     of running with the S3)"
```

The pieces, and why each is there:

- **The audio stops first.** Writing and erasing flash stalls all code that is not in internal RAM,
  and the audio engine is not; it would tear rather than pause. Muting through the normal fade-out
  makes the update sound like a source change.
- **400 ms, not 250 ms, before the first flash write** (2026-09-12). The mute is seen at the
  next audio block (at most 5.8 ms), the fade-out takes `fadeOutMs` (150 ms by default, and settable
  from the portal), and then the output DMA chain has to drain (46.4 ms for its 8 buffers): about
  200 ms in all. The wait was raised while the chain was briefly 16 buffers (92.9 ms of drain, about
  249 ms in all, so 250 ms left no margin), and it stayed at 400 ms when the chain went back to 8.
  400 ms covers a fade-out up to about 340 ms. When the wait runs short, the flash write freezes the
  audio task mid-drain and the output hardware substitutes exact zeros: a click through the
  amplifier at the start of every update.
- **The image is streamed.** The S3 is relaying an HTTP upload and does not know the length or the
  checksum until the last byte, so BEGIN carries size 0 and CRC 0 and the whole-image CRC32 arrives in
  END. Both sides compute `crc32_le()` (from the ESP32 ROM) over exactly the bytes committed. An END
  with CRC 0 skips the comparison on the A32.
- **Offsets are absolute.** A frame that arrives twice is ignored and one that arrives early is
  refused; the acknowledgement always carries the number of bytes actually committed, so a lost frame
  can be decided rather than being fatal. The acknowledgement carries no frame identity, so the S3
  reasons only in byte counts. A late acknowledgement from an earlier attempt reads "offset", which
  means "send again": harmless.
- **BEGIN waits 20 s.** Before it answers, the A32 mutes, waits its 400 ms and calls
  `Update.begin()`, which does not erase the slot (erasing happens a sector at a time inside
  `Update.write()`). Five seconds started failing consistently on 2026-09-01: the transfer never
  began, so nothing was written, but the update could not be started. Waiting costs nothing on this
  one-off step. A BEGIN that arrives while an update is already running is refused by
  `Update.begin()` ("already running"), and the A32 gives up the running update with
  `begin refused`, restoring the user's mute.
- **END must be answered** (2026-09-24). A lost END used to be reported as "image sent" while
  the A32 gave up 20 s later and kept its old firmware. And the silence is reported honestly
  (2026-09-25): the A32 commits the image *before* it answers, so a lost answer can hide a good
  update. Hence "check its version once it reconnects", and the re-handshake that shows it.
- **The END wait cannot wrap.** It reads `millis()` once per pass, because two readings could straddle
  the 4 s limit and the unsigned subtraction would wrap to about 49 days (2026-09-25).
- **The error reaches the page.** The S3 keeps the reason in `otaErr` (96 characters), including the
  A32's `Update` error code: 1 write, 2 erase, 3 read, 4 space, 5 size, 6 stream, 7 MD5, 8 magic byte,
  9 activate, 10 no partition, 11 bad argument, 12 abort. `a32OtaSetError()` lets the portal record a
  refusal decided on the S3 side (the wrong-chip check).
- **The wrong-chip check.** An ESP32 image starts with the byte 0xE9, and bytes 12-13 carry the chip id:
  0x0000 for the classic ESP32, 0x0009 for the ESP32-S3. The audio board is a classic ESP32 and the
  main board an ESP32-S3 (Bible §1). The check runs on the first chunk, after BEGIN,
  so the A32 has already muted; the ABORT restores its mute. A file that is not an ESP32 image at all
  (first byte not 0xE9) passes this check and is refused by the A32's `Update` (magic byte, error 8).
- **Both sides give up on silence.** The portal's `otaWatchdog()` sends ABORT after 15 s with no upload
  activity, because the web server does not reliably report an aborted upload. The A32 gives up after
  20 s with no accepted DATA frame. A failed update once left the machine muted for four minutes.
- **A failed update restores the user's mute**, not "unmuted" (2026-09-24). `otaMutedWas` is
  recorded at BEGIN only if no update is already running; a second BEGIN would otherwise record the
  forced mute as the user's choice.
- **The S3's needle and display are not parked** for an A32 relay: the S3 is not the chip writing
  flash.
- **Measured speed**: 1,205,728 bytes in 75.8 s, about 16 kB/s (2026-08-31). Each frame waits for its
  acknowledgement and each acknowledgement for a flash write, so the line rate is not the limit.

### 9.2.13 The rollback life cycle

**Rollback** means: a freshly updated image runs on trial, and unless it confirms itself, the
bootloader returns to the previous image at the next reset. The pinned framework builds both the
bootloader and the application with rollback support, but the Arduino core's start-up code calls a
hook, `verifyRollbackLater()`, and when that returns false (its default) it marks every trial image
valid before `setup()` runs. Until 2026-09-25, every relayed A32 image was therefore trusted blindly:
one that crashed or hung at boot looped on itself, and only a USB cable could fix it.

`src/a32/main.cpp` defines:

```
extern "C" bool verifyRollbackLater() { return true; }   // "this firmware confirms itself"
```

```
 END:    Update.end(true) -> new slot becomes the boot slot           state NEW
 reset:  bootloader: NEW -> PENDING_VERIFY, boots the new slot
 start:  verifyRollbackLater() == true -> the core leaves it on trial
 setup:  gImgOnTrial = true, label "ON TRIAL (rollback armed)"
    |
    +-- any reset before confirmation: crash, watchdog, ESP.restart(),
    |   "reboot the audio board" (MSG_REBOOT), power loss
    |     -> bootloader: PENDING_VERIFY -> ABORTED, boots the PREVIOUS image,
    |        whose boot report ends "- an earlier update was ROLLED BACK"
    |
    +-- confirmTick(now), every loop pass, at most one attempt per 10 s:
    |     never while protoBroken (the protocol self-test failed)
    |     a HELLO has been seen, and now - gHelloAt >= 60 000 ms
    |         -> confirmImage("a minute of running with the S3")
    |     no HELLO ever, now > 5 min, rxCount() == 0 and badCrc() == 0
    |         -> confirmImage("five minutes running, the S3 never spoke")
    |     otherwise: stay on trial
    v
 confirmImage(): esp_ota_mark_app_valid_cancel_rollback()
    ESP_OK -> gImgConfirmed, label "valid (confirmed)",
              settings held during the trial written now (settingsFlush(true));
              LOG "settings changed during the trial are now saved" if there were any
              LOG "image confirmed (<why>)"
    error  -> LOG "image confirm FAILED (err N) - retrying", again in 10 s
```

(Inside `confirmImage()` the held settings are written just before the confirmation line is
sent, so on the S3's console `settings changed during the trial are now saved` can come a line
before `image confirmed (...)`.)

- **A minute after the first HELLO, not at it** (2026-09-25). The handshake lands about a
  second after boot, before the image has woken, opened Bluetooth or played. The first version
  confirmed at the handshake (the same afternoon), so a build that crashed on waking was
  already "good" and looped.
- **The five-minute fallback** exists so that a good image is never thrown away just because the S3 was
  absent. It applies only when no valid frame and no damaged frame ever arrived. A build whose link is
  broken in a way that still produces bytes that look like frames (bad checksums, or valid frames that
  never include a HELLO) stays on trial, so the next reset takes the old image back. Frames carrying
  another protocol version are counted only in `badVer`, which the fallback does not look at; 9.8.1
  relies on this.
- **Never a build whose protocol self-test failed** (v.1.0, 2026-09-25). `confirmTick()` returns at
  once while `protoBroken` is set, so such a build stays on trial for as long as it runs, and the next
  reset takes the previous image back. Before v.1.0 a failed build that still handshook was confirmed
  after the minute and then stayed, muted, until a good image was uploaded. The S3 has the same rule.
- **No update while on trial.** `MSG_OTA_BEGIN` is refused while the image is on trial and not yet
  confirmed: the new image would be written over the other slot, which holds the very image a rollback
  would return to, and a second bad build would have nothing to fall back on. The refusal comes before
  anything is muted, and the S3 shows the reason: `A32: still on trial - retry in 1 min (err 0)`. The
  Arduino `Update` class writes the slot directly and does not go through the framework's own
  "refuse while pending" check, so this test in `onMessage()` is the only guard.
- **No settings written while on trial** (v.1.0.4, 2026-09-26). `settingsFlush()` holds
  every write until `confirmImage()` succeeds, which then writes what was held (9.2.3). A reset
  during the trial therefore rolls back to an image whose stored settings are exactly as it left
  them, even if the trial image used a different settings layout.
- **Seen on the radio** (2026-09-25): 17:14, the boot report showed `image ON TRIAL (rollback armed)`,
  proving the bootloader supports rollback; 18:34-18:35, a second update sent at once was refused with
  "A32: still on trial - retry in 1 min", the boot report showed `watchdog on`, and `[A32] image
  confirmed (a minute of running with the S3)` arrived 60 s after the handshake. Later that evening,
  all by over-the-air update with the USB cables unplugged, the v.1.0 builds (2026-09-25, then the tagged v.1.0)
  booted on trial, reported their commit, and confirmed. Then came the **first real rollback** on this
  board: a deliberately hanging test image (its `loop()` spins forever; built from uncommitted code,
  stamp `<hash>-dirty`) was relayed through the S3. The task watchdog reset the A32, and the
  bootloader returned to v.1.0. Its boot report read `boot: commit <hash>, reset reason 6, ...
  image valid, watchdog on - an earlier update was ROLLED BACK`.
  On 2026-09-26, 11:34 to 11:36, the v.1.0.4 image (v.1.0.4) showed the settings hold: a Bluetooth
  lamp level changed from the portal during the trial was written only at confirmation (9.2.3).

The S3 has had the same treatment since v.1.0: trial and rollback of its own updates, no
S3 update while on trial, a degraded run instead of a halt on a failed self-test, and a 15 s task
watchdog; chapter 4 describes it. It has also held its settings during its trial since v.1.0.1
; the A32 has done the same since v.1.0.4.

### 9.2.14 The clock path

The real-time clock is a DS3231 module wired to the A32, SDA on GPIO22 and SCL on GPIO21 (Bible §5,
§22). The module has a backup battery (Bible §22). The firmware addresses
the chip at I2C address 0x68 on `A32_RTC_SDA` / `A32_RTC_SCL`, at 100 kHz (`A32_I2C_HZ`, a speed the
firmware chose). The bus runs on the ESP32's internal pull-ups plus whatever the module itself
carries; the Bible records no external pull-ups on it. The clock chip sits on the audio board and the
display on the main board, so the time crosses the link.

**Everything on the link and in the chip is UTC.** The S3 applies the time zone, because it has NTP
and the portal. Local time in the chip would make the hour after a daylight-saving change ambiguous
and the hour before it happen twice.

**Reading** (`Rtc::read()` in `src/a32/rtc.h`):

1. The seven time registers from 0x00 (BCD, 24-hour, year counted from 2000).
2. The status register 0x0F; bit 7 is **OSF**, the oscillator-stopped flag, which the chip sets when
   its oscillator has stopped (for example after losing both main power and battery).
3. The temperature registers 0x11-0x12, in quarter-degrees, into `tempC4`.
4. Conversion to a Unix time with `mktime()`, with the zone pinned to `UTC0` around the call
   (`mktime()` works in local time).

`Rtc::read()` returns false only when the time read itself gets no answer. The validity rule is:

```
valid = (epoch > 1600000000) && status register answered && OSF clear
```

`reserved[0]` of the time payload carries the OSF bit itself. A status register that did not answer
counts as **not** valid: the safe direction, since the S3 keeps its own clock and only declines to
re-anchor from this one reading.

Why OSF matters: a DS3231 whose oscillator stopped keeps counting from wherever it was, so after a
power loss it reports a date that is plausible and wrong. Plausibility cannot catch that; OSF can. OSF
is sticky, and that is the point: "a clock that stopped and was never set since IS wrong". Until
2026-09-24 validity was plausibility alone and OSF was carried but never read (2026-09-24).

**Writing** (`Rtc::write()`): the time in 24-hour form with the weekday, then a read-modify-write of the
status register that **clears OSF**. That clear is what makes the time trustworthy again.

**The A32's handlers.** `MSG_GET_TIME` reads the chip and answers `MSG_TIME`; if the chip does not
answer, the A32 prints a warning on its own console and sends nothing. `MSG_SET_TIME` writes
`unixUtc` to the chip; the incoming `valid` and `tempC4` fields are ignored.

**The S3's software clock.** The S3 keeps `epochAtSync + (millis() - millisAtSync) / 1000`
(`nowEpoch()` in `src/s3/main.cpp`), so a slow or missing link never makes the display stutter; the
clock just stops being corrected. It is re-anchored:

| From | When | Condition |
|---|---|---|
| `MSG_TIME` | at every handshake, then every 60 s | `valid` and `unixUtc > 1600000000`; otherwise `[WARN] the DS3231 has no valid time yet.` |
| NTP | when NTP has a fresh answer and the link is handshaken; at most once an hour | the S3 also sends `MSG_SET_TIME`, so NTP disciplines the DS3231, not the other way round, and the correction survives the next power cut on its own |
| the console `W` prompt | on demand | always sets the S3's clock; sends `MSG_SET_TIME` only while the A32 is handshaken (since v.1.0.2). Otherwise the console says "clock set on this board only - the audio board is not answering, so the battery clock was NOT updated. Set it again once the link is back." |

The RTC is the authority whenever the network is absent. The first `MSG_SET_TIME`, from NTP or by
hand, clears OSF, and from then on the chip reports `valid` again.

**The battery clock's health, on the portal** (in v.1.0.2). The S3 keeps `gRtcBad` in
`src/s3/main.cpp` and publishes it as the state field `rtc`:

| `rtc` | Meaning | Set when | Portal pill |
|---|---|---|---|
| 0 | fine, or not asked yet | at boot; a TIME frame arrives with a valid time | none |
| 1 | the chip answered with no valid time (oscillator stopped, or no answer from its status register) | a TIME frame arrives with `valid` = 0 or a time before the plausibility floor | "BATTERY CLOCK LOST ITS TIME - check its battery" |
| 2 | the chip does not answer | a GET_TIME went unanswered for more than 5 s while the A32 is alive | "BATTERY CLOCK NOT ANSWERING" |

How the S3 decides "not answering": the A32 sends nothing at all when its read of the chip fails
(the A32's handlers, above), so silence is the only sign. Each periodic GET_TIME (every 60 s while
handshaken) stamps `gTimeAsked` if no question is already pending. Any TIME frame clears the
stamp. If the stamp is more than 5 s old while the S3 still has its handshake with the A32, `rtc`
becomes 2. Losing the handshake clears the stamp, so a silent A32 is never mistaken for a silent
clock. The GET_TIME sent at the handshake itself is not timed, so the first "not answering" can take
up to about a minute after the handshake. `rtc` keeps its last value until the next TIME frame
changes it. The pills in a real fault have not been seen on the radio.

The behaviour itself (trust the clock only when its oscillator-stopped flag is clear) was kept, with the fault made visible on the portal: if the flag is set, most probably something is
wrong with the battery clock's battery, or with the module itself.

The `W` prompt (`setClockInteractive()` in `src/s3/main.cpp`) reads the typed time in the zone
`EST5EDT,M3.2.0,M11.1.0`, hard-coded, and ignores the zone set in the portal. **Builders: put your own
POSIX TZ string there**, or make it read the portal's zone. It is left as it is on purpose: the rule belongs to this build's location, and anyone building a
similar device should put in their own.

The S3 does not use `tempC4` or the OSF copy in `reserved[0]`.

## 9.3 Settings and constants

### 9.3.1 Constants

None of these are portal settings. All are compiled in; changing one means rebuilding, and for the
ones in `include/proto.h` rebuilding **both** boards.

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `PROTO_VERSION` | 4 | - | 0-255 | the version byte in every frame; see 9.8.1 before changing it. 2 = `ProtoAudio` gained its fades; what changed at 3 and 4 was never recorded | `include/proto.h` |
| `PROTO_SOF0`, `PROTO_SOF1` | 0xA5, 0x5A | byte | - | start marker | `include/proto.h` |
| `PROTO_MAX_PAYLOAD` | 1088 | bytes | must hold every payload struct (compile-time check) | largest payload; also the decoder's buffer | `include/proto.h` |
| `PROTO_BAUD` | 921600 | baud | - | link speed, 8N1 | `include/proto.h` |
| `PROTO_STATE_MS` | 250 | ms | - | A32 state report period | `include/proto.h` |
| `PROTO_SILENCE_MS` | 2000 | ms | - | `peerAlive()` window, both boards | `include/proto.h` |
| `PROTO_VERSION_LEN` | 24 | bytes | - | version string field in HELLO and OTA_BEGIN | `include/proto.h` |
| receive buffer | 4096 | bytes | - | UART receive buffer, set before `begin()` | `Link::begin()` |
| HELLO / PING period | 1000 | ms | - | HELLO until answered, then PING | S3 `loop()` |
| SET_SYS period | 2000 | ms | - | amp state to the A32, also sent at once on a change | S3 `loop()` |
| GET_TIME period | 60 000 | ms | - | clock re-anchor from the RTC | S3 `loop()` |
| RTC answer wait | 5000 | ms | - | a periodic GET_TIME unanswered this long, while handshaken, sets `rtc` = 2 | S3 `loop()` |
| NTP push to the RTC | at most 1 per 3 600 000 | ms | - | only while handshaken and NTP is fresh | S3 `loop()` |
| restart test slack | 5000 | ms | - | A32 clock must fall back by more than this; values from 0xF0000000 up are ignored | S3 `onMessage()` |
| BEGIN answer wait | 20 000 | ms | - | S3 waits for the A32 to accept an update | `a32OtaBegin()` |
| DATA answer wait | 3000 x 4 | ms | - | per frame, four tries | `otaSendFrame()` |
| END answer wait | 4000 | ms | - | state 3 or 4 required | `a32OtaEnd()` |
| upload silence | 15 000 | ms | - | then ABORT to the A32 | `otaWatchdog()` in `src/s3/portal.cpp` |
| update frame | 1024 | bytes | - | data per `MSG_OTA_DATA` | S3 `otaBuf` |
| mute settle before flash | 400 | ms | must exceed mute + fade-out + DMA drain | wait between muting and the first flash write | A32 `MSG_OTA_BEGIN` handler |
| A32 update give-up | 20 000 | ms | - | since the last accepted DATA frame | A32 `loop()` |
| reboot delay after success | 250 | ms | - | lets the final status go out | A32 `MSG_OTA_END` handler |
| `WDT_TIMEOUT_S` | 15 | s | must exceed `setup()` and the longest `loop()` pass | task watchdog, panic on | `src/a32/main.cpp` |
| `CONFIRM_AFTER_HELLO_MS` | 60 000 | ms | - | trial confirmation, counted from the first HELLO | `src/a32/main.cpp` |
| `CONFIRM_AFTER_MS` | 300 000 | ms | - | fallback confirmation when nothing was ever received | `src/a32/main.cpp` |
| confirm retry | 10 000 | ms | - | after a failed confirmation | `confirmTick()` |
| A32 settings save debounce | 2000 | ms | - | after the last change; skipped by a forced save (`MSG_REBOOT`, confirmation) | `settingsFlush()` |
| A32 settings writes while on trial | none | - | - | changes wait in RAM; written by `confirmImage()` (v.1.0.4) | `settingsFlush()` |
| `protoBroken` announcement | 10 000 | ms | - | console and `MSG_LOG` | A32 `loop()` |
| RTC | 0x68; registers 0x00, 0x0F, 0x11; 100 kHz | - | - | see 9.2.14 | `src/a32/rtc.h`, `include/pins.h` |
| RTC plausibility floor | 1 600 000 000 | s (Unix) | - | September 2020; both boards | `rtc.h`, S3 `onMessage()` |
| A32 partitions | `min_spiffs.csv`: two 1.875 MB application slots | - | - | two slots are required for both the relay and rollback; the A32 module has 4 MB of flash (Bible §1) | `platformio.ini` |
| `FW_VERSION` | `v.1.YYYYMMDDTHHMMSS` | - | fits in 24 bytes | stamped at build time by `scripts/version.py`; says which binary | build |
| `FW_COMMIT` | short git hash, e.g. `a1b2c3d` | - | 7 characters, plus `-dirty`; or `nogit` | stamped at build time by `scripts/version.py`; says which source; never sent in the handshake | build |

### 9.3.2 Message types

Numbering blocks: 0x0n handshake, 0x1n S3-to-A32 control, 0x2n A32-to-S3 reporting, 0x3n update relay,
0x4n S3-to-A32 diagnostics (opened because the 0x1n block is full at 0x1F). The numbers are explicit
"so a mismatch between builds is diagnosable from a capture". Retired ids are never reused. Every
handler has `default: break`, so a peer ignores an id it does not know.

| Id | Name | Direction | Payload | Sent by, when | Receiver does |
|---|---|---|---|---|---|
| 0x01 | `MSG_HELLO` | S3 to A32 | `ProtoHello` (25) | S3 `loop()`, every 1 s while not handshaken | notes the S3's version, answers HELLO_ACK, sends the boot report (once per boot), starts the 60 s confirmation clock (once) |
| 0x02 | `MSG_HELLO_ACK` | A32 to S3 | `ProtoHello` (25) | answer to HELLO | `peerHello = true`, prints `[PASS] A32 up: proto vN, firmware X`, sends GET_TIME and GET_CFG |
| 0x03 | `MSG_PING` | S3 to A32 | empty | S3 every 1 s while handshaken; the S3 limit monitor on each pass | answers PONG |
| 0x04 | `MSG_PONG` | A32 to S3 | empty | answer to PING | ignored by the S3 |
| 0x10 | `MSG_SET_AUDIO` | S3 to A32 | `ProtoAudio` (15) | portal edit of an A32 audio setting; settings import (once, at the end) | applies, saves after a 2 s debounce (on trial: at confirmation) |
| 0x11 | `MSG_SET_BT` | S3 to A32 | `ProtoBtCfg` (12) | portal edit of an A32 Bluetooth setting; settings import | applies, saves after a 2 s debounce (on trial: at confirmation) |
| 0x12 | `MSG_SET_SYS` | S3 to A32 | `ProtoSys` (4) | S3 every 2 s while handshaken, and at once when the amp state changes | `ampOn = s.ampOn`, visibility recomputed |
| 0x13 | `MSG_SET_TIME` | S3 to A32 | `ProtoTime` (8) | fresh NTP (at most hourly, handshaken); console `W` (handshaken only) | `Rtc::write()`, clears OSF |
| 0x14 | `MSG_GET_TIME` | S3 to A32 | empty | at HELLO_ACK, then every 60 s | `Rtc::read()`, answers TIME (nothing if the chip is silent; the S3 then reports `rtc` = 2, 9.2.14) |
| 0x15 | `MSG_BT_FORGET` | S3 to A32 | empty | portal `bt.forget` | removes every Bluetooth pairing |
| 0x16 | `MSG_BT_LOOK` | S3 to A32 | empty | portal `bt.pair` | if the source is Bluetooth: drops the phone and opens pairing |
| 0x17 | `MSG_REBOOT` | S3 to A32 | empty | portal `sys.rebootA32` | saves pending settings at once (nothing while on trial), then restarts |
| 0x18 | `MSG_CAL_POT` | S3 to A32 | uint8: 0 MIN, 1 MAX, 2 CENTRE | portal `pot.min`, `pot.max`, `pot.ctr` | records that knob calibration point |
| 0x19 | `MSG_BT_CMD` | S3 to A32 | uint8 `BTC_*`: 0 play, 1 pause, 2 next, 3 previous, 4 disconnect | portal `bt.play`, `bt.pause`, `bt.next`, `bt.prev`, `bt.disconnect` | AVRCP command to the phone; disconnect is graceful |
| 0x1A | `MSG_GET_CFG` | S3 to A32 | empty | at HELLO_ACK; portal `sys.getcfg` | answers CFG |
| 0x1B | `MSG_TEST_DC` | S3 to A32 | uint8 0/1 | portal `sys.dctest` | constant-DC output diagnostic on or off |
| 0x1C | `MSG_ADC_CLOCK` | - | - | **retired** 2026-09-01; nothing sends it | no handler |
| 0x1D | `MSG_ZERO_FLOOR` | S3 to A32 | uint8 0/1 | portal `sys.zfloor` | zero-data floor on or off |
| 0x1E | `MSG_USE_POT` | S3 to A32 | uint8 0/1 | portal `sys.pot` | knob drives the volume, or is ignored |
| 0x20 | `MSG_STATE` | A32 to S3 | `ProtoState` (130) | A32 every 250 ms, unprompted, from boot | fills the S3 mirror, restart test |
| 0x21 | `MSG_TIME` | A32 to S3 | `ProtoTime` (8) | answer to GET_TIME | re-anchors the S3 clock if valid |
| 0x22 | `MSG_LOG` | A32 to S3 | text, no terminating zero | boot report; confirmation result; settings held during the trial now saved; `protoBroken` every 10 s; a failed settings save; a Bluetooth disconnect that did not land | printed as `[A32] <text>` |
| 0x23 | `MSG_CFG` | A32 to S3 | `ProtoCfgAll` (33) | answer to GET_CFG | fills the settings mirror, `haveCfg = true` |
| 0x30 | `MSG_OTA_BEGIN` | S3 to A32 | `ProtoOtaBegin` (32) | `a32OtaBegin()` | refuse on trial, or mute and start receiving (9.2.12) |
| 0x31 | `MSG_OTA_DATA` | S3 to A32 | 6 + len bytes (`ProtoOtaData`, parsed by hand) | `otaSendFrame()` | commit if the offset matches; always answer STATUS |
| 0x32 | `MSG_OTA_END` | S3 to A32 | uint32 CRC32 of the image | `a32OtaEnd()` | compare CRC, finish, answer, restart |
| 0x33 | `MSG_OTA_ABORT` | S3 to A32 | empty | wrong chip; unanswered END; upload aborted; portal upload silence | if an update is running: give up, restore the user's mute |
| 0x34 | `MSG_OTA_STATUS` | A32 to S3 | `ProtoOtaStatus` (38) | after every BEGIN, DATA, END and give-up | fills the S3's acknowledgement fields |
| 0x40 | `MSG_DIN_DRIVE` | S3 to A32 | uint8 0..3 | portal `sys.dindrv` | DAC data pad drive strength |
| 0x41 | `MSG_SD_DELAY` | - | - | **retired** 2026-09-11; nothing sends it | no handler |
| 0x42 | `MSG_SIGN_TAIL` | S3 to A32 | uint8 0/1 | portal `sys.tail` | sign-extended tail on or off |
| 0x43 | `MSG_CLK_DRIVE` | S3 to A32 | uint8 2..3 | portal `sys.clkdrv` | all three clock pads' drive |
| 0x44 | `MSG_DC_WORD` | S3 to A32 | uint8 index 0..6 | portal `sys.dcword` | picks one of seven fixed DC words, all at or below -42 dBFS; never a raw word; index 7 and up ignored |
| 0x45 | `MSG_BT_TX` | S3 to A32 | uint8 0..5 (`ESP_PWR_LVL_N12` .. `P3`) | portal `sys.bttx` | Bluetooth transmit power; values above P3 ignored, so the remote path can only turn it down from the default maximum |
| 0x46 | `MSG_CLK_PAIR` | S3 to A32 | uint8[2]: MCLK drive, then BCK/LRCK drive, each 2..3 | portal `sys.clkpair` | drives the two clock groups separately |

The diagnostics (0x1B, 0x1D, 0x1E, 0x40 to 0x46) belong to the pop investigation (chapter 10).
Several were added on 2026-09-12 and `MSG_CLK_PAIR` on 2026-09-22.

### 9.3.3 Payload structures

All are `__attribute__((packed))`, in `include/proto.h`. Sizes in bytes.

| Struct | Size | Fields |
|---|---|---|
| `ProtoHello` | 25 | `protoVersion` (u8), `fwVersion[24]` |
| `ProtoAudio` | 15 | volume, muted (software mute only), balance, gainRadio and gainBt (tenths of a dB), monoSum, muteOnChange, fadeInMs, fadeOutMs, taperX10 (chapter 10 explains each) |
| `ProtoBtCfg` | 12 | connectable, autoConnect, lookTimeoutS, ledBrightness[7], pauseOnLeave |
| `ProtoSys` | 4 | `ampOn` (already a plain 1 = on), reserved[3] |
| `ProtoTime` | 8 | `unixUtc` (u32, always UTC), `valid` (u8), `tempC4` (i8, quarter-degrees), reserved[2] (the A32 puts OSF in reserved[0]) |
| `ProtoCfgAll` | 33 | `ProtoAudio`, `ProtoBtCfg`, potMin, potMid, potMax |
| `ProtoState` | 130 | source, btState, volume, muted, peaks L/R, AVRCP volume, clipped, raw selector reading, peerName[24], underruns, ring fill, the zero-data watch, the volume watch, pad drives read back, DC test and word, **`a32Ms`** (the A32's `millis()` when the frame was built, used for the restart test and to place events in time), last stall, Bluetooth TX power read back, the three clock pad drives, RMS L/R |
| `ProtoOtaBegin` | 32 | size and crc32 (both sent as 0: streamed), fwVersion[24] (the S3 fills it with its **own** version) |
| `ProtoOtaData` | 1030 | offset (u32, absolute), len (u16), data[1024]; only `len` bytes of data are sent |
| `ProtoOtaStatus` | 38 | received (bytes committed), state (1 receiving, 3 ok, 4 failed; 0 idle and 2 verifying are defined but never sent), errCode (`Update.getError()`), detail[32] |

Two sets of compile-time checks in `include/proto.h` (2026-09-24):

- `static_assert(sizeof(X) <= PROTO_MAX_PAYLOAD)` for all ten structs. "A struct that does not fit is a
  fact about the SOURCE, so the build is where it belongs." Every struct is listed, because "the one
  nobody thought to check is the one that grows."
- `offsetof(ProtoOtaData, offset) == 0`, `len == 4`, `data == 6`: the A32 reads DATA at those offsets
  by hand.

### 9.3.4 Versions on the link

There are two different version numbers:

- **`PROTO_VERSION`** is the wire format. It travels as the version byte of every frame. A frame with
  another value never gets past the decoder.
- **`FW_VERSION`** names the build: `v.MAJOR.YYYYMMDDTHHMMSS`, stamped at build time. It travels in
  HELLO and HELLO_ACK so each console can say which build the other board runs. MAJOR is 1 since
  v.1.0 (2026-09-25; git tag `v.1.0`) and means only "the author's release"; a
  break in the wire format is `PROTO_VERSION`'s job.
  The git-hash stamp (`FW_COMMIT`) is shown next to the version but never sent in the handshake,
  because the handshake field is 24 bytes and widening it would itself be a protocol change.

## 9.4 Decisions

The decisions recorded here were right for this radio, in this house. They are not claimed to be
the best possible choices for every build; your mileage may vary.

**Decision.** A framed, checksummed, versioned binary protocol, not text lines.
Why. The link carries firmware images, so a corrupted byte must be detected and not installed; both
boards print to their own consoles, and a framed link cannot be confused by a stray log line; and the
S3 updates the A32, so the two will sometimes run mismatched builds.
Rejected. A printf-style text protocol.
When and evidence. Present at the first commit (2026-09-01).

**Decision.** `Link::begin()` has no form without explicit pins, and sets the receive buffer before
opening the port.
Why. `Serial2`'s default pins on the classic ESP32 include the pin this firmware uses as the audio frame
clock; the buffer size is ignored once the port is open.
Rejected. Documenting the trap and trusting callers.
When and evidence. `include/link.h`, present at the first commit (2026-09-01).

**Decision.** Packed structs, copied byte for byte.
Why. Both ends are the same architecture and compiler; the shortcut is recorded, with the condition
that ends it (a third kind of device).
Rejected. Byte-oriented serialisation.
When and evidence. `include/proto.h`.

**Decision.** Grow `ProtoState` by appending fields, without changing `PROTO_VERSION`.
Why. An appended struct makes an old peer's frame the wrong length; `as()` rejects it, and a
half-updated machine loses only its telemetry and keeps its audio. A version change would drop every
frame at the decoder, which costs the handshake, which puts the A32 to sleep by the awake rule, and the
A32's update is relayed over this same link. "The graceful failure is the correct one here, not the
loud one."
Rejected. A version change for every struct change.
When and evidence. Written 2026-09-10 with the zero-data watch; committed 2026-09-12.

**Decision.** Adding a message id needs no version change; retired ids are never reused.
Why. An older peer ignores an unknown id. An old S3 that still sends a retired id is ignored, "the safe
direction". The diagnostics block 0x4n was opened rather than putting a new command at 0x20
(`MSG_STATE`), which "would have worked" but only by luck of direction.
Rejected. Reusing 0x1C and 0x41; squeezing new ids into another block.
When and evidence. 0x1C retired 2026-09-01; 0x41 retired 2026-09-11; block 0x4n added 2026-09-12.

**Decision.** The A32 is asleep (silent, Bluetooth closed) whenever the amp is off **or the S3 is
silent**.
Why. The author wants an unreachable S3 to be obvious. The consequences are accepted and written down so
nobody "fixes" them: an S3 restart or a long S3 update silences the radio mid-listen, and every
power-up has a brief dead moment. It proved itself on its first real fault: a crimp came out of its
housing on 2026-09-02 and cut the A32-to-S3 direction; the old firmware would have played on.
Rejected. "Assume the amp is on, so a dead helper board never silences the radio."
When and evidence. 2026-09-01; the three regressions it caused fixed the same day.
The intent: if the S3 is unreachable, that must be known; the radio then does not work at all, with
no audio, no clock, no lights, nothing.

**Decision.** When the A32 falls asleep because the S3 went quiet, it forgets the amp state.
Why. The S3's last "amp on" may be stale by the time it speaks again; waking on it unmuted into an amp
that might be off.
Rejected. Waking on the last known `ampOn`.
When and evidence. 2026-09-25.

**Decision.** A failed protocol self-test on the A32 degrades (muted, Bluetooth closed, link and update
receiver up) instead of halting.
Why. The halt sat before `gLink.begin()`, so an updated image that failed it could never be reached
again without opening the cabinet. "A halt protects nothing that a mute does not: the self-test is
about the FRAMER, and a framer that is wrong in some corner may still carry an OTA, whereas a halted MCU
certainly cannot." The cheapest failure it guarded, a payload too big for a frame, became a compile
error.
Rejected. `for(;;) delay(1000);` before the link.
When and evidence. 2026-09-24.

**Decision.** A32 update rollback, and a task watchdog on the A32's loop.
Why. The bootloader supported rollback, but the core marked every image valid before `setup()`, so an
image that crashed or hung at boot needed USB. The watchdog turns a hang into the reset that rollback
needs, and restarts a frozen loop that nothing on the link can reach.
Rejected. Trusting every image; relying on the core's default watchdog, which watched only core 0's
idle task.
When and evidence. 2026-09-25, which also brought the 64-bit uptime.

**Decision.** The watchdog is 15 s, panic on, and armed as the first line of `setup()`.
Why. It must cover a Bluetooth start that never returns; 15 s leaves room for the Bluetooth start and
for the image check at the end of an update.
Rejected. The framework's 5 s default applied to the loop.
When and evidence. 2026-09-25.

**Decision.** A trial image is confirmed 60 s after the first HELLO; the 5-minute fallback applies only
when nothing valid or damaged was ever received; a failed confirmation is retried every 10 s; an update
is refused while on trial; the boot report names the watchdog state and an earlier rollback.
Why. A handshake a second after boot proves nothing about waking, Bluetooth or playback. A broken link
that still produces frames must stay on trial. Writing over the fallback image while on trial would leave
nothing to fall back to.
Rejected. Confirming at the first handshake (17:07); an unconditional 5-minute fallback.
When and evidence. 2026-09-25, 18:31; seen on the radio the same evening (9.2.13).

**Decision.** "Bring the link up before Bluetooth" in `setup()` was dropped.
Why. The link is serviced only in `loop()`, so bringing it up earlier in `setup()` would not let a hang
in the Bluetooth start be reached over it. The need behind it, recovering an image that hangs at boot, is
covered by rollback and the loop watchdog.
Rejected. Reordering `setup()`.
When. 2026-09-25.

**Decision.** The update relay: 1 kB frames each acknowledged with the committed byte count, absolute
offsets, the image streamed with its CRC32 in END, the audio muted and given 400 ms before the first
write, the S3's needle not parked.
Why. See 9.2.12.
Rejected. A 250 ms mute wait (raised in 2026-09-12); a 5 s BEGIN wait (raised to 20 s after
2026-09-01).
When and evidence. Implemented and proven 2026-08-31, before the repository's history.

**Decision.** END must be answered; the S3 handshakes again after every A32 update and whenever the
A32's clock goes backwards; an unanswered END aborts and says to check the version; the wrong-chip reason
reaches the page; the END wait reads the clock once per pass.
Why. A lost END was reported as success while the A32 kept its old firmware, and the S3 kept showing the
A32's old version.
When and evidence. 2026-09-24; refined 2026-09-25.

**Decision.** A relay that fails in mid-stream sends no ABORT; the A32 stays muted until its own 20 s
give-up.
Why. Not considered worth changing: the A32's own 20 s give-up ends it.
When. 2026-09-25.

**Decision.** The decoder does not rescan the inside of a failed frame.
Why. The consequence (9.5) was judged acceptable. It has never been seen.

**Decision.** The RTC is valid only when its oscillator-stopped flag is clear; writing the time clears it.
Why. After a power loss the chip reports a plausible, wrong date, and plausibility cannot catch it.
Rejected. Validity by plausibility alone, with OSF reported separately (the rule of 2026-08-31).
When and evidence. 2026-09-24. Kept, with the fault made
visible on the portal (the `rtc` state field and its two pills, v.1.0.2; 9.2.14): a set flag most
probably means the battery clock's battery, or the module itself, is at fault.

**Decision.** UTC everywhere below the S3; the S3 owns the time zone. NTP disciplines the RTC, and the
RTC is the holdover when the network is absent.
Why. See 9.2.14.
When and evidence. `src/a32/rtc.h`, `src/s3/main.cpp`.

**Decision.** The A32 owns and saves its settings, and only a write that reached flash counts as
saved. The S3 keeps a mirror, refreshed at every handshake and forgotten when the A32 goes silent,
and refuses to push A32 settings while it has no mirror.
Why. The portal must show what is in the machine; a push built from an unread, all-zero copy would be
saved by the A32, and a push into a silent link would be lost. A save that ignored its own result
could report "saved" and lose the change at the next reboot.
When and evidence. `src/s3/settings_table.h`. The silent-A32 rule
and the checked save came in v.1.0.2 (2026-09-25).

**Decision.** "Reboot the audio board" (`MSG_REBOOT`) saves pending settings first, skipping the
2 s debounce.
Why. A reboot within two seconds of a change used to restart before the debounced write, and the
change came back as the old value.
Rejected. Restarting at once.
When and evidence. 2026-09-25.

**Decision.** The A32 writes no settings while its image is on trial; what was held is written the
moment the image is confirmed, and the A32 says so on the link.
Why. An update that changes `ProtoAudio` or `ProtoBtCfg` would have saved its new layout during
the trial, and a rollback would then have found a structure of the wrong size and run it on
defaults. The S3 had held its own settings during its trial since v.1.0.1; the A32 had not.
Rejected. Leaving the A32 saving during its trial and advising a settings export before any update
that changes an A32 structure (the rule at v.1.0.3).
When and evidence. v.1.0.4, 2026-09-26 (A32 only). Proven live the same morning
(9.2.3).

**Decision.** The S3's WiFi prompt may hold its loop (and so put the A32 to sleep) for up to 60 s.
Why. Judged not to cause any meaningful harm in realistic use.

**Decision.** The S3 gets the same rollback and watchdog treatment (2026-09-25; chapter 4).
A build whose protocol self-test failed is never confirmed, on either board (the A32 since v.1.0,
the same day).

The failure classes in 9.5 use the recovery ladder of §0.5.

## 9.5 Failures and recovery

The A32 is powered whenever the set is plugged in: its 5 V supply sits ahead of the front switch
(Bible §10, §21), so the front switch does not restart it. A "power cycle" of the A32 therefore means
unplugging the set.

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| A32 `loop()` or `setup()` freezes, image confirmed | the loop task stops feeding the watchdog | panic restart after 15 s; boot report shows the reason | nothing, if the hang does not recur. A hang at every boot before the link comes up is a boot loop that only USB can fix | NOTE; BLOCKER if it recurs at every boot |
| A new image crashes or hangs at boot | reset while on trial | the bootloader returns to the previous image; its boot report says `an earlier update was ROLLED BACK` | none needed | NOTE |
| A new image boots, hears frames, never handshakes | stays on trial indefinitely | A32 asleep: radio silent. Nothing on the link can restart it | unplug the set: the reset rolls back | BLOCKER |
| A new image fails its protocol self-test | `protoBroken` | muted, Bluetooth closed, `proto self-test FAILED - muted, BT closed, awaiting OTA` every 10 s; link and update receiver up. Never confirmed (v.1.0), so an upload is refused while it runs | image relayed over the air: "reboot the audio board" from the portal, or unplug the set; the reset rolls back. Image flashed by USB (not on trial): upload a good image through the portal | DEFECT if the link still carries the reboot, BLOCKER (unplug) if not; never exercised |
| Upload file is not for this chip | first chunk: byte 0xE9, chip id not 0x0000 | ABORT; the A32 restores the user's mute | upload the right file | NOTE |
| Upload file is not an ESP32 image | A32 `Update` refuses it (error 8) | give-up, user's mute restored | upload the right file | NOTE |
| Damage in transit | bad CRC16 on a frame; CRC32 mismatch at END; image check fails in `Update.end()` | frame resent; or give-up with "crc mismatch" / "end failed" | retry | NOTE |
| END answer lost | no state 3 or 4 within 4 s | ABORT, re-handshake, "the A32 did not confirm the update - check its version once it reconnects" | read the version at the next handshake | NOTE |
| A second update inside the trial minute | `MSG_OTA_BEGIN` while on trial | refused before any mute: "still on trial - retry in 1 min" | wait a minute | NOTE |
| Upload stops mid-stream (browser, WiFi, S3 restart) | no DATA for 20 s on the A32; no upload activity for 15 s on the S3 | A32 gives up and restores the user's mute; the old image stays | retry | NOTE |
| Confirmation fails | `esp_ota_mark_app_valid_cancel_rollback()` returns an error | `image confirm FAILED (err N) - retrying` every 10 s; updates refused meanwhile | none, if a retry lands | NOTE |
| A reset during the trial minute of a good image | the reset | rolls back a good image | upload it again | NOTE |
| One corrupted frame | bad CRC | frame dropped, `badCrc` counts it | none | NOTE |
| A corrupted length byte (value up to 1088) | the decoder swallows up to 1090 following bytes before hunting again | on the A32, where S3 traffic is about 14 bytes per second after the handshake, that is up to about 80 s of deafness: the A32 falls asleep, then wakes by itself | none | NOTE |
| The S3 goes silent (restart, its own update, crash) | no valid frame for 2 s | asleep, amp state forgotten, phone paused and dropped, muted, Bluetooth dark; STATE still sent | automatic when the S3 returns and says the amp is on | NOTE (deliberate) |
| The A32 goes silent, or restarts quickly | S3: no valid frame for 2 s, or the A32 clock goes backwards | S3 warns, `peerHello` and `haveState` cleared (and `haveCfg` too after 2 s of silence), portal shows the A32 fields as unknown, S3 clock keeps running from its last anchor, HELLO every second | automatic | NOTE |
| Link cable faulty | one or both directions silent | as above, on the affected side | hardware (Bible §12) | - |
| `ProtoState` length differs between the builds | `as()` refuses STATE | S3 shows unknown, warns once: "Flash BOTH MCUs. Audio is unaffected - only telemetry stops." | update the older board | NOTE |
| `PROTO_VERSION` differs between the builds | every frame dropped at the decoder, both sides, counted in `badVer` | no handshake; A32 asleep (radio silent); the relay refuses ("the A32 is not answering"). Visible since v.1.0.2: the S3's console `s` shows `rx 0` with `wrong-version` climbing and "the boards run different protocol versions: flash both", and the portal shows the pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both" | update the S3 over its own WiFi to the A32's protocol (9.8.1). If the A32 is still on trial, unplugging the set also works (it rolls back) | BLOCKER (survives a power cycle once the A32 is confirmed) |
| RTC oscillator stopped (OSF set) | `valid = 0` | S3 prints `the DS3231 has no valid time yet` at each read and does not re-anchor from it; `rtc` = 1, pill "BATTERY CLOCK LOST ITS TIME - check its battery" | automatic on the next fresh NTP answer; otherwise set the time with the console `W`. If it recurs after every power cut, check the module's battery | NOTE with network; DEFECT without |
| RTC does not answer | GET_TIME gets no reply within 5 s while the A32 is alive | A32 console warning; S3 `rtc` = 2, pill "BATTERY CLOCK NOT ANSWERING" | hardware (Bible §5); the S3 still takes NTP | - |
| Settings pushed while the S3 has no mirror (never read, or the A32 went silent) | `haveCfg` false | portal edit refused, 409 "the audio board is not answering - its settings cannot be changed now"; export writes `n/a` | wait for the handshake | NOTE |
| An A32 settings save fails | a `put` to flash reports a short write | stays unsaved; `[A32] settings NOT saved - the flash write failed; retrying`; retried 2 s later | none if a retry lands; a change is lost only if the A32 restarts before one does | NOTE (never exercised) |
| Settings changed during the A32's trial, then the trial ends in a reset (rollback, "reboot the audio board", power cut) | nothing was written during the trial | the changes are gone; the previous image finds its stored settings as it left them, in the layout it knows | set them again once an image is confirmed | NOTE (by design) |
| An A32 update that changed `ProtoAudio` or `ProtoBtCfg` is rolled back | nothing was written during its trial (v.1.0.4) | the previous image loads its own settings normally | none | NOTE (before v.1.0.4: that structure fell back to defaults) |

## 9.6 Graveyard

These are approaches tried and dropped on this radio, in this house. They were dead ends here, not
necessarily everywhere.

- **A text protocol.** Rejected when the protocol was written, for the reasons in 9.4.
- **`Serial2.begin(baud)` without pins.** It took the audio frame-clock pin here and the audio failed
  later with no obvious cause. Made impossible in `Link::begin()`.
- **0xA5 0xA5 as the start marker.** Set aside for 0xA5 0x5A when the protocol was written.
- **A table-driven CRC.** Not used: the bitwise CRC is fast enough here, and the table would take cache
  the display interrupt wants.
- **Halting the A32 on a failed self-test.** Retired 2026-09-24: it made an updated image that
  failed unreachable.
- **The self-test as the only guard against oversized payloads.** Replaced by compile-time checks on
  2026-09-24.
- **`MSG_ADC_CLOCK` (0x1C).** Cut the ADC's clock pin to mute it. Retired 2026-09-01: once the
  DAC took its system clock from the same pin here, cutting it was a DAC clock error, not a mute.
- **`MSG_SD_DELAY` (0x41).** A remote write to the I2S output delay register. Added 2026-09-10, retired
  2026-09-11: here it sent the radio to full volume (of its four positions only 0 played correctly), and
  a build that still carried it was judged unsafe. Its last road, the
  A32's USB console key `y` (and `Audio::setSdOutDelay()` behind it), was removed in v.1.0.3 (v.1.0.3, 2026-09-26); only the read-back remains, in the state frame and the A32's
  status line.
- **A settings save that ignored its own result** (A32, until 2026-09-25). The "unsaved" flag
  dropped before the write, so a failed write read as saved. Now only a write that reached flash
  counts (9.2.3).
- **An A32 that saved settings during its trial** (until v.1.0.4, 2026-09-26). A trial image
  that changed a settings structure would have written its new layout, and a rollback would have
  run that structure on defaults. The defence was advice (export a settings file first). Replaced by
  holding every write until confirmation (9.2.3).
- **A settings mirror that outlived the A32's silence** (S3, until v.1.0.2). Portal edits went nowhere
  and were overwritten when the A32 came back, and a download wrote the stale copy. Now `haveCfg` is
  cleared when the A32 goes silent (9.2.11).
- **A protocol mismatch that looked like a dead wire** (until v.1.0.2). The decoder counted the
  frames it dropped (`badVer`) but nothing showed the count, so a half-updated pair read "SILENT, rx 0",
  exactly like a broken cable. Now shown on the console and the portal (9.2.7).
- **The A32 plays on when the S3 is silent** (link liveness only displayed, never acted on). Replaced
  2026-09-01 by the awake rule, at the author's request.
- **Waking on the S3's last known amp state.** Replaced 2026-09-25.
- **A 250 ms wait before the first flash write.** Here it left no margin while the output chain was
  16 buffers, and risked a click; 400 ms since 2026-09-12, kept when the chain went back
  to 8.
- **A 5 s wait for the answer to BEGIN.** Failed consistently here on 2026-09-01; 20 s since.
- **Ignoring the answer to END.** Here a lost END was reported as "image sent" while the A32 kept its old
  firmware. Replaced 2026-09-24; the "it keeps its old firmware" message was then replaced by
  the honest one on 2026-09-25.
- **A 48-character error buffer on the S3.** Cut the messages short here; 96 since 2026-09-25.
- **Restoring "unmuted" after a failed update.** Here it unmuted a set the user had muted; the user's
  own mute is restored since 2026-09-24.
- **The core's default of trusting every updated image.** Here an image that crashed at boot needed USB.
  Overridden 2026-09-25.
- **Confirming a trial image at the first handshake.** Lasted one afternoon (17:07, to
   18:31): the handshake came before Bluetooth and playback, so a crash there was already "good".
- **An unconditional 5-minute confirmation.** Restricted the same evening (2026-09-25) to "nothing heard".
- **Bringing the link up before Bluetooth.** Dropped as useless here (9.4).
- **RTC validity by plausibility alone** (2026-08-31). Here a DS3231 that had lost power reported a
  plausible wrong date and the S3 adopted it. Reversed 2026-09-24.
- **Reading the Bluetooth transmit power from the 4 Hz state builder.** Coincided here with an A32
  restart the moment Bluetooth started page-scanning (2026-09-11). The value is now read back once, when
  it is set (2026-09-12).

## 9.7 Limits and firmware notes

**Never exercised on the radio:**

- The 5-minute "S3 never spoke" confirmation (§12.4.17).
- The degraded `protoBroken` mode, and with it the rule that such a build is never confirmed
  (§12.4.17).
- A settings export while the A32 is silent (`n/a` rows), and the 409 refusal of an edit then
  (§12.3.16).
- The wrong-version counter and pill with a real mismatch (on the radio they have only read 0)
  (§12.4.17).
- The battery-clock pills in a real fault (`rtc` 1 or 2) (§12.4.17).
- A failed A32 settings save (§12.3.16).

Exercised on 2026-09-25: the trial state, the refusal while on trial, the confirmation, and one real
rollback, caused by a watchdog restart of a hanging test image. The boot report gave reset reason 6
for it. On 2026-09-25 and 2026-09-26 both boards went to v.1.0.2 and then v.1.0.3 over the air, each
on trial and then confirmed; the S3's link line read `rx 489 crc 0 wrong-version 0`. On 2026-09-26
the A32 alone went to v.1.0.4 (build `v.1.20260926T113302`), and the settings hold was
exercised: a change made during the trial was written only at confirmation (9.2.3). On 2026-09-27
the S3 alone went to v.1.0.5, on trial and then confirmed. The running pair is the S3 at
v.1.0.5 and the A32 at v.1.0.4.

**Limits and accepted risks:**

- **Any reset in the trial minute rolls a good image back:** a power cut, "reboot the audio board", or a
  crash in that minute. A settings change made in that minute goes with it, because nothing is
  written until confirmation (9.2.3).
- **A `protoBroken` image is never confirmed** (v.1.0): `confirmTick()` returns at once while
  `protoBroken` is set. An image relayed over the air therefore stays on trial, refuses any new
  update, and is rolled back by the next reset. Its own announcement ("awaiting OTA") is true only for
  an image flashed by USB, which is not on trial. Accepted (§12.4.12).
- **A `PROTO_VERSION`-skewed A32 image confirms itself after 5 minutes**, because frames with another
  version are counted only in `badVer`. 9.8.1 relies on this; the flip side is that an A32 built with the
  wrong version by mistake becomes permanent, and the S3 must then be updated to match it. The skew is
  no longer silent: the S3 shows it (9.2.7). Accepted (§12.4.1).
- **A protocol mismatch is shown only on the S3's side.** The S3 counts, shows and publishes it
  (console `s` "wrong-version N", state field `linkver`, the portal pill). The A32 counts it too
  (`badVer()`), but prints it nowhere; its USB console `s` shows only `link SILENT rx 0` (§12.4.1).
- **With the A32 absent from boot, the S3 has no clock even with NTP.** The S3 anchors its clock from
  `MSG_TIME`, the `W` prompt, or the NTP push, and the NTP push is gated on the handshake. Accepted
  (§12.4.2).
- **A change made in the portal to an A32 setting just before the A32 goes silent** can still be lost:
  the refusal starts only once 2 s of silence have cleared `haveCfg` (§12.4.8).
- **The watchdog re-initialisation also changes the core-0 idle check** from 5 s to 15 s.

**Small known issues, left as they are**
- The S3's HELLO_ACK "PROTOCOL MISMATCH" warning can never fire; the wrong-version counter does that
  job (§12.4.13).
- The A32's version appears only on the S3 console (`[PASS] A32 up: ...`), not as a portal status
  field (§12.4.6).
- The RTC year is two digits from 2000; the century bit is ignored (§12.4.15).
- "an earlier update was ROLLED BACK" stays in every boot report until the next update (§12.4.16).
- The A32's forced save before "reboot the audio board" is tried once; if that write fails, the
  board restarts and the change is lost (§12.4.10).
- **The S3's portal task shares core 0 with the needle's step task** (`nstep`, priority 19, in
  `src/s3/needle.cpp`). While the needle moves, the portal is starved, and this is the likely cause of the
  four A32 update failures of 2026-09-01, since the relay runs on that task. It is classed as a
  quality item, not a blocker; the fix (scheduling changes) has not been started. Keeping the needle at
  rest during an A32 upload avoids the contention (§12.3.3).
- **The S3 no longer halts on its own failed protocol self-test** (2026-09-25): it runs on, and its image
  is never confirmed (chapter 4).

## 9.8 Changing this area

**Invariants that must hold:**

1. `include/proto.h` and `include/link.h` are shared by both builds. Any struct change means rebuilding
   and updating **both** boards; know in advance which side loses what while they differ (9.8.1).
2. Prefer appending fields (a length skew costs only telemetry) and new message ids (ignored by old
   peers, no version change). The rule written in `include/proto.h` ("WHEN PROTO_VERSION
   CHANGES"): a new message id, no change; a struct whose size changes, no change, because `as()`
   refuses the wrong length and a mismatched peer loses only that message type; a change `as()`
   cannot see (same size, different layout or meaning), change it. Change `PROTO_VERSION` only by
   the procedure in 9.8.1.
3. Keep every payload struct in the `static_assert` list, and keep the three `offsetof` checks if you
   touch `ProtoOtaData`.
4. Never reuse a retired id (0x1C, 0x41). Put new S3-to-A32 diagnostics in 0x4n.
5. `Link::begin()` gets explicit pins; `setRxBufferSize()` comes before the port is opened.
6. The mute is a function of state: every new path that changes it goes through `applyMute()`, and
   `protoBroken || !awake` stays in it.
7. Only frames **from** the S3 keep the A32 awake. Do not block the S3's `loop()` for more than about
   1.5 s without polling the link and sending a PING.
8. The update receiver stays reachable in every degraded mode (asleep, `protoBroken`). Add no halt before
   `gLink.begin()` on the A32.
9. `verifyRollbackLater()` keeps returning true, and confirmation happens only after the new image has
   done its real work (woken, Bluetooth, audio). Do not call `esp_ota_mark_app_valid_cancel_rollback()`
   earlier. Keep the on-trial refusal in the `MSG_OTA_BEGIN` handler: it is the only thing that stops an
   update being written over the fallback image.
10. `setup()` finishes in under 15 s, and so does any single `loop()` pass (update image check, delays).
11. RTC: UTC only; `valid` requires OSF clear; `Rtc::write()` clears OSF.
12. The S3 refuses portal pushes of A32-owned settings while `haveCfg` is false, and clears `haveCfg`
    whenever it declares the A32 silent.
13. A settings save, on either board, counts as done only when the write reached flash.
14. Nothing reaches the A32's stored settings (NVS namespace `amb`) while its image is on trial:
    every write goes through `settingsFlush()`, which returns early on trial, and `confirmImage()`
    writes what was held. A new path that writes `amb` directly would reopen the rollback trap of
    9.2.3.

**Traps:**

- `as()` rejects on exact length. A struct grown on one side looks like "telemetry stopped", not like a
  wire error: the checksum passes.
- A packed struct field cannot bind to a reference; copy through locals (the STATE builder does).
- `mktime()` works in local time. Pin `TZ=UTC0` around it on the A32 (`rtc.h` does).
- The update acknowledgement has no frame identity. Reason about it only in byte counts.
- A successful relay does not mean the new image stays. It may still roll back within the minute. Check
  for `[A32] image confirmed` and the version at the re-handshake.

**How to test a change:**

- At boot, both boards print `[PASS] protocol v4, state frame 138 bytes, CRC rejects damage`.
- S3 console `s`: the link line shows alive or SILENT, frames received, CRC errors and wrong-version
  frames; a healthy pair reads `crc 0 wrong-version 0`. Portal status: `linkrx`, `linktx`, `linkcrc`,
  `linkver`, `hello`, `astate`, and `rtc` for the battery clock.
- The A32's own console (USB, on the bench): `s` shows the link, frames received, and whether it is awake
  and why.
- After any A32 update, the S3 console should show, in order: `[A32] boot: ... image ON TRIAL (rollback
  armed), watchdog on`, then `[PASS] A32 up: proto v4, firmware <new>`, then 60 s later `[A32] image
  confirmed (a minute of running with the S3)`. Sending a second update inside that minute should be
  refused. To check the settings hold, change an audio-board setting in the portal during that
  minute (the 2026-09-26 test used the Bluetooth lamp's `btLedOff`, 255 to 254, then back): the A32
  must write nothing until it confirms, then print `[A32] settings changed during the trial are
  now saved`.
- The rollback itself needs a deliberately bad image, for example one whose `loop()` spins forever (the
  2026-09-25 test) or one that crashes when it wakes. Relay it, then look for `reset reason 6` and
  `- an earlier update was ROLLED BACK` in the next boot report. Keep a USB cable at hand in case the
  bad image does something the watchdog cannot catch.
- Identify a board with certainty before flashing it, over USB or over the air.

### 9.8.1 Changing the protocol

A `PROTO_VERSION` change is a hard wall: two boards with different values cannot exchange a single frame,
including the update frames the A32 depends on. The A32 can only be updated through the S3, and the S3 can
be updated over its own WiFi, which does not use the link. So there is exactly one order that works over
the air:

```
 Start: S3 on N, A32 on N. Build both boards with N+1.

 1. Upload the A32 image (N+1) through the S3's portal.
      The relay runs entirely in protocol N (both boards are still on N); the image's
      content does not matter to it.
 2. The A32 restarts on N+1, ON TRIAL.
      It drops every frame from the S3 at the decoder (version byte N): badVer counts
      them; rxCount and badCrc stay 0. No handshake: the A32 is asleep, the radio silent.
      The S3 sees nothing valid either: "[WARN] A32 silent", HELLO every second.
      Its console `s` shows wrong-version climbing (the A32's STATE frames now
      carry N+1), and the portal shows "BOARDS RUN DIFFERENT PROTOCOL VERSIONS -
      flash both". That is expected here: carry on.
 3. Wait at least 5 minutes after the A32 restarted.
      confirmTick() finds no HELLO, rxCount() == 0 and badCrc() == 0, and confirms:
      "image confirmed (five minutes running, the S3 never spoke)". Nobody hears
      that line - the S3 drops it - but the image is now permanent.
 4. Upload the S3 image (N+1) over WiFi (the S3's own update route).
 5. The S3 restarts on N+1 and sends HELLO. The A32 answers; its boot report (the
      first it has sent this boot) should read "image valid (confirmed)", followed by
      "[PASS] A32 up: proto vN+1, firmware <new>".
```

**Why the 5-minute fallback makes this possible.** The fallback confirms an image that has heard nothing
valid and nothing damaged, and frames carrying another protocol version are counted only in `badVer`. An
A32 updated first therefore confirms itself after 5 minutes of hearing only the old S3, and a later reset
(a power cut, say) no longer takes it back to N. If the S3 is updated before those 5 minutes are up, the
A32 instead confirms 60 s after the new S3's first HELLO; that works too. The risk in that shorter path is
a reset of the A32 before it confirms: it would roll back to N while the S3 is already on N+1, the skew
would be the other way round, and the relay could not reach the A32. You would then put the S3 back on N
over WiFi and start again. Waiting the 5 minutes removes that case.

**The reverse order does not work.** An S3 on N+1 cannot reach an A32 on N: the A32 drops its frames, the
handshake never happens, and the relay refuses with "the A32 is not answering". Put the S3 back on N over
WiFi, then follow the order above.

Since v.1.0 the S3's own update also runs on trial (chapter 4). It confirms itself about 70 s after
boot once its network and portal are up, without needing the A32, so it confirms even if the A32 does
not answer. Two consequences for this procedure: an S3 update sent while the S3 is still on trial is
refused, so step 4 must wait until the S3 has confirmed its current image; and after step 5, wait for
the S3's "image confirmed" before anything restarts it, or it rolls back to N while the A32 stays on
N+1.

Before changing `PROTO_VERSION`, ask whether the change can be made by appending fields or adding message
ids instead; both keep a half-updated machine playing.

---

# 10. Sound: sources, Bluetooth, and the pop hunt

This chapter covers the audio computer, the **A32**. The A32 is a classic ESP32-WROOM-32, a
separate chip from the ESP32-S3 that runs the rest of the radio. The code lives in
`src/a32/audio.cpp`, `src/a32/audio.h`, `src/a32/btled.cpp`, `src/a32/btled.h` and the Bluetooth,
sleep and source logic in `src/a32/main.cpp`. The A32's link to the S3, its clock chip, its
watchdog and its update rollback belong to chapter 9. This chapter
touches them only where they meet the sound.

**Specific to this build — adapt.** The gains, the channel order, the volume curve and the knob
calibration here suit this tube set, this amplifier and this speaker set; adapt them to yours.

---

## 10.1 What it does

The A32 makes the sound of the radio. It has three sources, picked by the front selector:
**RADIO** (the tube radio's audio, digitised and played back), **BT** (music streamed from a phone
over Bluetooth) and **AUX** (an analogue input that never passes through the A32, so the A32 stays
silent). The A32 needs Bluetooth *Classic* for phone music, and the ESP32-S3 does not have it,
which is why this job runs on a separate chip. Turning the selector fades the old source out
quickly (150 ms) and brings the new one in gently (1 s). The front volume knob sets the level along
a curve that suits the ear. Radio and Bluetooth come out equally loud at the same knob position.
Bluetooth is invisible unless the selector is on BT and the amplifier is on. A phone that paired
before can reconnect by itself. A new phone needs the pair button, which makes the radio visible
for 90 seconds. The radio never calls a phone. A blue lamp on the front tells the Bluetooth state.
When the amplifier is off, or the S3 stops talking, the A32 "sleeps": it plays nothing, the lamp
goes dark and Bluetooth closes. The last part of this chapter tells the story of the **pops**,
loud clicks that took two weeks to tame. A firmware change to the clock pins cut them by about
150 times.

---

## 10.2 How it works

### 10.2.1 Words used in this chapter

| term | meaning |
|---|---|
| **I2S** | A serial bus for audio samples, with three or four wires. **BCK** (bit clock) ticks once per bit. **LRCK** (left-right clock, also called word select) toggles once per sample frame and says which channel the bits belong to. **Data** lines carry the bits, most significant bit first. |
| **Slot** | One channel's share of a frame. Here each frame has two 32-bit slots. |
| **MCLK** | The master clock. It is a faster reference clock that some converters need. The TI parts used here call their MCLK input **SCK**. |
| **Full duplex** | One I2S peripheral sends (to the DAC) and receives (from the ADC) at the same time, on shared BCK and LRCK. |
| **DAC / ADC** | Digital-to-analogue converter (the output) and analogue-to-digital converter (the radio input). The parts are named in the Bible (§6). |
| **DMA** | Direct memory access. The I2S driver keeps a chain of RAM buffers that the hardware plays out (or fills) by itself. The CPU only has to keep the chain topped up. DMA buffers must sit in a special part of internal RAM. |
| **APLL** | The ESP32's audio PLL, a clock generator that can make 11.2896 MHz exactly. |
| **A2DP** | The Bluetooth profile for streaming stereo music. The phone compresses the music with the **SBC** codec, and the ESP32-A2DP library decodes it to 16-bit samples (**PCM**). |
| **AVRCP** | The Bluetooth remote-control profile that goes with A2DP: play, pause, next, and the phone's volume slider. |
| **Bluedroid** | The Bluetooth stack inside the ESP-IDF. It runs its own task on core 0. |
| **Scan mode** | Whether the radio answers Bluetooth calls. **Connectable**: a phone that already knows the radio can connect. **Discoverable**: the radio shows up in a phone's list of new devices. |
| **Bond / pairing** | The stored keys that let a phone that paired once reconnect later. |
| **dBFS** | Decibels relative to digital full scale. 0 dBFS is the loudest sample possible. |
| **Q16, Q24** | Fixed-point numbers. In Q16, 65536 means 1.0. In Q24, 16,777,216 means 1.0. |
| **NVS** | The ESP32's non-volatile storage, a small key-value store in flash. |
| **LEDC** | The ESP32's LED PWM peripheral. |

### 10.2.2 The shape of the audio path

```
                    +------------------------ A32 --------------------------+
 tube radio -> ADC -+-> I2S RX --(RADIO)--+                                 |
                    |                     |                                 |
 phone --BT--> Bluedroid -> SBC decode -> ring buffer --(BT)--+             |
                    |                     |                   |             |
                    |          (AUX: zeros)                   v             |
                    |                     +--> source gain -> envelope x    |
                    |                          volume x balance -> I2S TX --+-> DAC -> amp
                    +-------------------------------------------------------+
 AUX jack ---------------------------------------------------------------------> amp
```

The amplifier adds its inputs together; it does not choose between them. So on AUX the A32's only
job is to send digital zeros to the DAC. AUX reaches the amplifier without passing through the
A32, and the amplifier's input panel sums its inputs (Bible §22, §30.1).

### 10.2.3 I2S is installed once, and never touched again

`Audio::begin()` in `src/a32/audio.cpp` installs the I2S driver once, at boot. It is never
reinstalled, reconfigured, stopped or re-pinned. Changing source is only a choice of where the next
block of samples comes from. That is ordinary code with no driver calls.

The reason is that the two audio paths want different drivers. The A2DP library, left to itself,
wants a transmit-only, 16-bit driver with no MCLK. The radio path needs full duplex, 32-bit slots
and an MCLK for the ADC. Reinstalling the driver at every source change, while Bluedroid still
holds a reference to it, is the kind of fault that crashes the chip in ways that are hard to trace.
So the firmware installs the superset once and keeps it.

The configuration, as `Audio::begin()` writes it:

| field | value | why |
|---|---|---|
| port | `I2S_NUM_0` | |
| mode | master, TX and RX | The ESP32 makes the clocks; both converters follow them as I2S slaves (Bible §6, §24). |
| sample rate | 44,100 Hz | Fixed. There is no resampler. |
| bits per sample | 32 | The ADC sends 24-bit samples in standard I2S format (Bible §24). They arrive most significant bit first at the top of a 32-bit slot, so the raw word is already at full scale. |
| channel format | `I2S_CHANNEL_FMT_RIGHT_LEFT` | |
| frame format | `I2S_COMM_FORMAT_STAND_I2S` | Standard (Philips) I2S: data starts one bit after the LRCK edge. |
| interrupt | level 1 | |
| DMA buffers | 8 buffers of 256 frames | 46.4 ms of audio per direction. See the heap budget (10.2.14). |
| `use_apll` | true | See below. |
| `tx_desc_auto_clear` | true | If the CPU falls behind, the driver sends zeros instead of replaying old audio. |
| `fixed_mclk` | 44,100 × 256 = 11.2896 MHz | BCK is 64 × 44,100 = 2.8224 MHz. |

Then it sets the pins, sets the clock-pad drive to 3 (see the pop hunt, 10.2.16) and starts the
audio task. The pins come from `pins.h`: MCLK on GPIO0, BCK on GPIO18, LRCK on GPIO17, data to the
DAC on GPIO4, data from the ADC on GPIO19 (Bible §5).

**Why the APLL stays on.** The MCLK on GPIO0 feeds both converters. The DAC switches off its own
internal clock generator when it is given an external SCK. The APLL is the only ESP32 clock that
makes 11.2896 MHz exactly, and BCK and LRCK are divided from that same clock, so the ratio SCK:LRCK
is exactly 256 by construction. Without the APLL the divider runs from a 160 MHz clock that cannot
make 44,100 Hz exactly, a worse clock for both parts. The code says: do not "try" turning it off.
Both converters' SCK inputs are fed from GPIO0 (Bible §22).

**Why MCLK is on GPIO0.** The classic ESP32 can put the I2S master clock only on GPIO0, 1 or 3.
GPIO1 and 3 are the USB console. So GPIO0 must never be taken away from the I2S peripheral: that
would stop the clock of both converters at once.

**If installation fails.** If `i2s_driver_install` or `i2s_set_pin` fails, `begin()` prints a
`[FAIL]` line on the A32's USB console and returns without starting the audio task. The radio is
then silent, and only the USB console says why.

**Channel order.** The firmware puts the **right** channel in the first I2S slot (slot 0) and the
**left** in slot 1, the reverse of the usual convention. Every frame it writes is `{right, left}`.
It does so because on this machine that order gave correct stereo. This is an adjustment for this
machine, not a fact about the parts, and anyone building another one must check it. The author knows
that the amplifier-to-speaker side is correct; the A32-and-DAC side was never tested on its own.
The channel order is a setting of this machine that needs adjusting for other devices.

A live test is pending: a left/right test track played over Bluetooth. There is no portal switch
to swap the channels; one is listed as a possible improvement (§12.8.1), beside a possible
equaliser (§12.8.2). Until then, swapping them is a firmware change in `audioTask()`, where the Bluetooth
path, the balance and the meters all take slot 0 as the right channel.

### 10.2.4 The three guards that keep the A2DP library off I2S

The ESP32-A2DP library normally owns the I2S driver. Here it must not. Three guards in `setup()` in
`src/a32/main.cpp` make that structural, and each was added after a real bug.

1. **`a2dp.set_stream_reader(a2dpStream, false)`.** The library hands the decoded samples to
   `a2dpStream()`, and the `false` tells it not to write I2S itself.
2. **`a2dp.set_output(nullOut)`**, where `NullA2dpOutput` is an output class whose every method is
   empty. The flag in guard 1 does not stop everything: the library calls the output's
   `set_sample_rate()` whether or not output is enabled, and the library's default output then
   calls `i2s_set_clk()`. The moment a phone started streaming, that call reconfigured the
   firmware's driver to 16-bit transmit-only. Bluetooth then played at full scale and garbled,
   ignoring the volume; the channels swapped; and the radio stayed dead afterwards. The empty
   output object has no way to reach I2S at all. The code dates the discovery 2026-08-28, before
   the repository's history began.
3. **`a2dp.set_volume_control(&noVol)`**, where `noVol` is the library's
   `A2DPNoVolumeControl`. The library's AVRCP volume control scales the sample buffer in place,
   one line before it calls the stream reader, and the stream-reader flag does not stop it. At a
   phone slider of 1/127 that is a divide by about 2048, with truncation. With the no-op volume
   control the library cannot touch the samples. The phone's AVRCP volume is still read as
   information (`a2dp.get_volume()`, published as `avrcpVolume`). The firmware's own curve is the
   only volume law in the machine. (In the first commit, 2026-09-01.)

The code states the discipline behind all three: "make it impossible, do not make it a rule to
remember."

The firmware uses the plain `BluetoothA2DPSink` class (through the subclass `PolicyA2dpSink`, see
10.2.10), not the "queued" variant. The plain class starts no I2S task of its own. So the stream
callback runs directly on the Bluetooth task.

### 10.2.5 Tasks, cores and buffers

| who | core | priority | what it does |
|---|---|---|---|
| audio task, `audioTask()` in `audio.cpp` | 1 | 6 | Reads or fetches one block, processes it, writes it to I2S. Stack 4096 bytes. |
| Bluetooth task (Bluedroid) | 0 | the stack's own | Decodes SBC and calls `a2dpStream()` → `Audio::pushBt()`. |
| Arduino `loop()` in `main.cpp` | 1 | 1 (Arduino default) | The link to the S3, sleep and wake, source and volume polling every 50 ms, the Bluetooth state machine, the lamp, the 250 ms state message to the S3, the USB console. |

Core 0 belongs to the Bluetooth stack. Sharing a core between SBC decoding and a real-time audio
loop invites dropouts, so the audio task sits on core 1.

**There is no `vTaskDelay` in the audio task, and there must not be one.** The pacing is free. On
RADIO, `i2s_read(..., portMAX_DELAY)` waits until a full block has been captured. On BT and AUX,
`i2s_write(..., portMAX_DELAY)` waits when the DMA chain is full. The audio task also never prints:
serial output from a priority-6 real-time task causes the dropouts it would be trying to report.
Anything worth saying is announced later from `loop()`.

**Buffers.**

- `BLOCK` = 256 frames per pass, about 5.8 ms.
- `inBuf` and `outBuf`: 256 frames × 2 slots × 4 bytes = 2 KB each, static. `btBuf`: 1 KB.
- The **Bluetooth ring**: `RING_FRAMES` = 8192 frames of two 16-bit samples, 32 KB, static. It
  absorbs about 186 ms of jitter. Bluetooth delivers audio in bursts; the radio path is paced by
  the ADC and needs no ring.

The ring has one writer (the Bluetooth task, in `pushBt()`) and one reader (the audio task, in
`popBt()`). It needs no lock. `pushBt()` publishes the new head with a release store; `popBt()`
reads the head with an acquire load and publishes the new tail with a release store. The chip is an
in-order CPU and the ring sits in uncached RAM, so the missing barriers had never caused harm. The
code adds them anyway: "correct by luck is not correct" (2026-09-01, added the tail's
release). `pushBt()` must never block the Bluetooth task, so a full ring drops the newest samples
instead of waiting.

### 10.2.6 One block through the audio task

Each pass of `audioTask()` does four things.

**1. The source-change handshake.** If a new source is pending and `muteOnChange` is on, the fade
envelope's target goes to 0. When the envelope reaches exactly 0, the task switches to the new
source. The envelope then rises again. So a change is: fade out, switch in silence, fade in. With
`muteOnChange` off the task switches at once, with no fade. With no change pending, the target is 0
if muted and 1.0 otherwise.

**2. Get the samples.**

- **RADIO.** `i2s_read` one block. With `monoSum` on (the default), each frame becomes
  `m = (slot0 + slot1) / 2`, sent to both slots. Both slots carry the same signal, so the sum gains
  about 3 dB of signal-to-noise: the signal adds up, the converter noise does not. Both ADC
  channels carry the same mono radio signal (Bible §6). The result is
  multiplied by the radio gain (`gainRadioQ16`) with saturation. A saturated sample sets the
  `clip` flag. With `monoSum` off the two slots are kept apart.
- **BT.** `popBt()` takes 256 frames from the ring. Missing frames are filled with zeros. Each
  16-bit sample is shifted left 16 bits to full 32-bit scale, then multiplied by the Bluetooth
  gain (`gainBtQ16`). The **underrun** counter counts only a *partial* fill: audio was flowing and
  ran out mid-block, and the zero-fill made a click. An empty ring means nothing is streaming and
  is not counted. (The first version counted empty blocks too and ran at a few hundred per second,
  which made it useless.)
- **AUX.** The block is set to zero.

**3. Emit.** The same code runs for every source.

- **Volume law.** `volQ16 = (volume / 255)^gamma × 65536`, where gamma is `taperX10 / 10`, clamped
  to 1.0–4.0. `powf()` runs only when the volume or gamma changes, once per block, never per
  sample. The new gain takes effect as a **step at a block boundary**; it is not ramped. That is
  harmless for a hand on a knob. The "volume watch" counts these steps (`gainSteps`) and records
  the smallest and largest gain applied, because on 2026-09-10 the volume reading wandered by
  itself, between 17 and 20 (see the pop hunt).
- **Balance.** `balance` runs from −100 to +100; negative moves the sound to the left. The other
  side is scaled by `65536 − |balance| × 655`. At ±100 the far side is about −65 dB, not silent.
- **Envelope.** A Q24 number, stepped **every sample**, and **squared** before use. A linear
  amplitude ramp does not sound like a fade: it rushes up and then sits there. Q24 is needed
  because Q16 cannot express a one-second fade (the step rounds to 1, which gives 1.49 s). Each
  sample becomes `sample × envelope² × volume × balance`.
- **Meters.** The **peak** meter reads the very end of the chain, so it moves with the knob. It is
  read and reset by the 250 ms state message, in tenths of a dBFS. It uses 64-bit arithmetic; an
  earlier 32-bit version could never register the loudest possible sample (negating the most
  negative 32-bit value gives itself back), so it under-read exactly during a full-scale event.
  That fix is what made the meter's evidence in the pop hunt trustworthy (10.2.16). The **RMS** meter is
  measured after the source gain and before the volume, so the knob does not move it. That is what
  let the radio and Bluetooth gains be matched once for every volume (2026-09-23).
  Peaks cannot do that job: FM stations are limited hard and music is not.
- **Diagnostics**, all off by default, applied in this order: the constant-DC word (replaces every
  sample), the low-bit output mask, the zero-data floor, the sign-extended tail. Each is described
  in 10.3.3 and the pop hunt.
- **Zero-data witness.** A copy of the DAC's own zero-data detector: 1024 frames in a row whose top
  24 bits are zero. It counts each time the DAC would enter and leave its analogue mute, on what is
  actually about to go out. The PCM5102A mutes its output by itself after 1024 frames of zero data (Bible §24).
  The witness counts a word as zero when its top 24 bits
  are zero, on the reading that the DAC resolves 24 bits of a 32-bit slot; the datasheet does not
  say whether it keeps the bottom 8.
- **I2S fault latches.** The peripheral's raw interrupt bits that the driver does not service are
  OR-ed into `i2sSticky`, which is never cleared.
- **Stall detector.** If more than 23.22 ms (one zero-data window) passed since the previous write,
  the task counts a stall and records its length and time.

**4. Write.** `i2s_write(outBuf, portMAX_DELAY)`.

### 10.2.7 Volume, balance, fades and mute

**Where the volume comes from.** The front volume knob is a position sensor, not an audio part.
`updateVolumePot()` in `main.cpp` reads it every 50 ms: two throw-away conversions, then the median
of five. `potRotation()` maps the raw reading through a three-point calibration (minimum, centre,
maximum). Each half of the travel is its own straight line, and the centre reading maps to 128 by
construction. The calibration exists because, measured on this set, the knob at its mechanical
centre read 215 of 255 with only the two ends calibrated. A **deadband** of 24 ADC counts (about
0.6 % of travel) stops the volume trembling at rest. The knob is read on GPIO35 over its full
0–3.3 V swing (Bible §7), with the ADC at its widest input range (11 dB attenuation).

Three rules follow from "the knob is a sensor":

- **The first reading after boot always applies.** The knob's position *is* the stored state.
- **The knob does not write volume to NVS.** Nothing needs storing.
- **The portal can also set the volume.** That value holds until the knob moves by more than the
  deadband. The portal action `sys.pot 0` makes the A32 ignore the knob altogether; it is an
  experiment and is not kept across a reboot.

If the stored calibration is implausible (maximum not at least 64 counts above minimum),
`potRotation()` falls back to the compiled defaults 60 and 3990 with no centre point. They are
roughly this set's knob ends, held by the firmware as a starting point until the knob is
calibrated.

**Mute is a function of state.** `applyMute()` sets
`Audio::setMute(protoBroken || !awake || cfgA.muted)`. Every place that can change any of those
three calls it. In the first version the sleep edge set the mute and four other places
(`settingsApply()`, `updateSource()`, `setup()`, `otaGiveUp()`) cleared it without looking at
sleep. Turning the source knob while asleep because the S3 had died then brought the sound back,
and the author's chosen alarm (silence when the S3 is gone) vanished. Since 2026-09-01
mute is computed, never toggled. `protoBroken` joined on 2026-09-24: a build whose
protocol self-test failed stays up, muted and invisible to phones, so that it can be replaced over
the air. Since v.1.0 (2026-09-25) such a build is also never confirmed: one that arrived over
the air is rolled back by the next reset, and one flashed by USB takes a new image (chapter 9).

The one deliberate exception is the start of an update (10.2.13), which forces the mute directly.

**Mute is never stored.** `settingsLoad()` forces `cfgA.muted = 0` on every boot. On 2026-08-28 a
stored mute booted the set silent, with nothing on the front of the radio to say why.

**Mute is always software.** Muting means writing zeros. No A32 pin reaches the DAC's hardware
mute input. The firmware used to drive GPIO16 high, believing it was that input; the write reached
nothing and was removed on 2026-09-23. No A32 pin reaches the DAC's XSMT; the DAC
module holds it un-muted by itself, and GPIO16 is unused (Bible §3, §6).

**Fades.** `Audio::setFades()` turns `fadeInMs` and `fadeOutMs` into per-sample Q24 steps. A value
of 0 ms is treated as 1 ms, so a source change can never wait forever for a silence that never
comes. Defaults: 1000 ms in, 150 ms out. The code records the reasons as "a gentle arrival, not a
jumpscare" and "leaving should be quick - you turned the knob".

**Gains.** The compiled defaults are **+8.0 dB for the radio and −7.0 dB for Bluetooth**
(2026-09-23). They were measured with the new RMS meter. At 0 dB the radio read RMS
−29.9 dBFS with peaks to −14.0 dBFS; +8 dB puts its peaks near −6 dBFS and its RMS near −22. The
old +15 dB clipped at the gain stage, which means at every volume. For Bluetooth, a −14 dBFS RMS
pink-noise reference played from a PC at full volume arrived at about −15 dBFS; −7 dB lands it
within about 1 dB of the radio, so changing source does not change loudness. The gain clamp on the
A32 is −60 to +30 dB. The radio gain suits the tube radio's output as it was on 2026-09-23, with
the tube set's own volume control at about one eighth of its travel (Bible §22). A different
tube-set level needs a different radio gain.

### 10.2.8 Source switching

`updateSource()` runs every 50 ms. `readSource()` reads the selector on GPIO36. It first makes
three throw-away conversions, because GPIO36 reads high for a while after another ADC channel has
been sampled (78 counts of residue, measured 2026-08-31). Then it takes the median of five. A raw
value below 424 is AUX, 2471 or above is RADIO, anything between is BT. The thresholds live in
`pins.h` as `A32_MODE_THRESH_AUX_BT` and `A32_MODE_THRESH_BT_RADIO` (Bible §0, §11).

On a change it does, in order:

1. If the old source was BT: `gracefulDisconnect(0xFF)`, which pauses the phone and then drops it
   (10.2.10).
2. `Audio::setSource(s)`: the audio task ramps across (10.2.6).
3. `applyMute()`, `updateBtState()`, `applyScanMode()`.

The fade exists because a change of source without a ramp is an audible thump through a valve
amplifier. The selector is an ON-OFF-ON switch whose centre position, off, reads 0 V through
22 kΩ, which is AUX (Bible §11, §22). So a turn from RADIO to BT, or back, can pass through AUX on
the way.

At boot `setup()` reads the selector once and sets the source before the first loop pass. The audio
task starts on AUX, at volume 0, muted, until `settingsApply()` and the first knob reading run.

### 10.2.9 Awake and asleep

`serviceWake()` in `main.cpp` runs at the top of every `loop()` pass and acts only on a change:

```
awake = ampOn && peerAlive()
```

`ampOn` is what the S3 last said about the amplifier, in `MSG_SET_SYS`. `peerAlive()` is true when
a frame from the S3 arrived within the last `PROTO_SILENCE_MS` = 2000 ms.

**Falling asleep.** If the reason is S3 silence, the A32 **forgets `ampOn`**, so waking needs a
fresh word from the S3. (Without this, an amp switched off while the S3 was rebooting woke the A32
unmuted into an amp that was off; 2026-09-25.) Then: `gracefulDisconnect(BT_OFF)`,
`applyMute()`, lamp to OFF, `applyScanMode()`.

**Waking.** Cancel any disconnect that the sleep edge armed and that has not fired yet (otherwise
the phone is dropped 250 ms after waking and the lamp goes dark while awake), clear the stray-phone
timer, then `applyMute()`, `updateBtState()`, `applyScanMode()`.

**The I2S clocks keep running while asleep.** Asleep is a mute, not a stop. Nothing calls
`i2s_stop`.

What stays alive while asleep: the link to the S3, the clock chip, and the update receiver.

The intent: when the amplifier is detected as powered down, there is no reason for the A32 to do
anything; it just listens for the S3. And if the S3 is unreachable, that must be known: the radio
then does not work at all, with no audio, no clock, no lights, nothing.

This was chosen over the alternative, "assume the amp is on so a dead helper board never silences
the radio". The consequences were accepted and are recorded in the code so that nobody "fixes" them: an S3
reboot or a long S3 update silences the radio mid-listen, and there is a short dead moment at every
power-up until the S3 first reports.

The rule earned itself on its first real fault, on 2026-09-02, the day after it was written. One
direction of the link between the boards failed (a connector fault, since repaired). The
A32 never learned that the amplifier was on, so it slept, and the whole front of the radio said
something was wrong. The firmware before the rule would have played on with a broken link.

### 10.2.10 Bluetooth: life cycle and visibility

**The policy.** The header of `src/a32/main.cpp` states three rules:

- a phone that wanders into range must not latch on its own;
- a phone that has connected before may reconnect on demand, from the phone;
- the button is for devices that have not connected before.

They have been in the code since the first commit (2026-09-01).

They give exactly one arrangement:

| situation | connectable | discoverable |
|---|---|---|
| not in BT mode, or asleep | no | no |
| in BT mode, idle | yes | no |
| pairing window open (after the button) | yes | yes, until the window closes |
| a phone is connected | no | no |

All of it depends on being awake. Both chips are powered whenever the set is plugged in. Without
the awake gate, a phone could connect at 3 a.m. and quietly send its audio into a radio that is
off. **The radio never calls a phone**: `a2dp.set_auto_reconnect(false)`.

**`applyScanMode(force)`** is the only function that writes the scan mode:

```
connectable  = cfgB.connectable && awake && source == BT && !protoBroken && !btConnected
discoverable = connectable && btState == BT_LINK
```

It remembers what it last wrote and returns early unless the result changed or `force` is set. The
`!btConnected` term stops a second phone from calling in behind the first.

**`PolicyA2dpSink` and `scanDirty`** (2026-09-24). The ESP32-A2DP library, at the
pinned version, writes the scan mode by itself, on the Bluetooth task. It calls
`set_scan_mode_connectable(true)` on every disconnect and once more when the stack comes up, and
`set_scan_mode_connectable(false)` on every connect. Its "true" means connectable *and* generally
discoverable. The stack-up call is only queued by `a2dp.start()`, so it lands after `setup()` has
applied the firmware's own mode. Because `applyScanMode()` cached "invisible" and none of those
library writes changed the policy, the radio sat **connectable and discoverable** in RADIO, in AUX
and asleep after every hang-up, source change or sleep, until something moved the policy.

The fix is a subclass. `PolicyA2dpSink` overrides the library's virtual
`set_scan_mode_connectable(bool)` so that it only sets `scanDirty = true`. `loop()` then calls
`applyScanMode(true)` after `updateBtState()`, which re-derives the mode from the firmware's own
state and writes it past the cache. The flag is cleared before the write, so a change that lands
during the write is caught on the next pass. `scanDirty` starts true, so the boot write happens in
any case. `onConnState()` sets it again after updating `btConnected`, so at least one pass sees the
new connection state. The library also has `set_discoverability()`, which writes the mode directly;
the firmware never calls it.

**Why the library is pinned.** `platformio.ini` pins ESP32-A2DP to commit
`3245602afc494f9e62160a0cfb2af864af45a37f` (library 1.8.11, dated 2026-08-12, the version that had
been running) (2026-09-25). Before the pin, a fresh clone or a wiped `.pio` folder fetched
whatever the library's main branch held that day. `PolicyA2dpSink` depends on the library's
internals: that `set_scan_mode_connectable` is virtual, and that every scan-mode write in the
library goes through it. A new library version could break that silently, and the radio would
become visible again with no error anywhere.

**Any bump of the library must re-test invisibility**: in RADIO, in AUX, asleep, after a hang-up
and after a source change, check with a phone that the radio does not appear in its list of new
devices. The platform itself is pinned too, at `espressif32@7.0.1` (Arduino core 2.0.17,
IDF 4.4.7), because core 3.x retires the legacy I2S driver and ESP32-A2DP then routes through a
different audio layer.

**States.** `btState` is published to the S3 and shown by the lamp. `updateBtState()` in `main.cpp`
runs every loop pass.

| state | lamp | entered when | left when |
|---|---|---|---|
| `BT_OFF` | dark | asleep, or the source is not BT | awake in BT mode → `BT_LOOK` |
| `BT_LOOK` | fast breath | entering BT mode, or a connection ended | after 20 s (compiled) → `BT_STDBY` |
| `BT_STDBY` | slow breath | from `BT_LOOK`, or when the pairing window closes | held; never times out |
| `BT_LINK` | regular blink | pair button, portal `bt.pair`, console `b` | after `lookTimeoutS` (default 90 s) → `BT_STDBY` |
| `BT_FOUND` | double flash | a phone connected | the lamp moves itself to `BT_CON` after 450 ms, and the published state follows it |
| `BT_CON` | steady bright | from `BT_FOUND` | disconnect → `BT_LOOK` |
| `BT_ON` | steady dim | portal hang-up (`bt.disconnect`) | held, like `BT_STDBY` |

`BT_LOOK`, `BT_STDBY` and `BT_ON` have the same scan mode: connectable, not discoverable. Only
`BT_LINK` is discoverable. "Looking" is a message from the lamp; the radio never sends a page to a
phone.

**Graceful disconnect.** The rule: disconnect entirely, but pause first.

`gracefulDisconnect(next)` sends an AVRCP pause (if `pauseOnLeave` is on) and arms a disconnect 250
ms later. `serviceDisconnect()`, called from `loop()`, fires it without blocking and moves to the
requested next state. The pause matters: tearing down A2DP under a phone that thinks it is playing
left its media app "playing into nowhere", a real dead end seen on the hardware. Leaving BT mode
drops the phone entirely; staying connected while the selector was elsewhere gave "the phone
showed connected, play appeared to work, and nothing came out".

**`btDropping`** (2026-09-24). `a2dp.disconnect()` only posts a request;
`btConnected` goes false later, on the Bluetooth task. In between, `updateBtState()` used to see a
phone still connected and promote the requested state straight back to `BT_FOUND`. So the pair
button pressed with a phone connected ended in `BT_LOOK` instead of `BT_LINK`: it did not do the one
thing it is for. Now `serviceDisconnect()` sets `btDropping` **before** the request (the callback
that clears it runs on another task and could otherwise land first). While it is set, the
connected branch holds the requested state. `onConnState()` clears it when the disconnect lands. If
the disconnect has not landed after `BT_DROP_TIMEOUT_MS` = 3 s, `updateBtState()` clears it and
sends "BT: the disconnect did not land - still connected" to the USB console and to the S3 as
`MSG_LOG`. A stale flag is swept at the top of `updateBtState()`.

**Stray phones** (2026-09-24). If a phone is connected while asleep or out of BT mode,
`updateBtState()` sends it away through the same pause-then-drop path, at most once every 3 s
(`strayDropAt`). The rule is "nothing connects at night", not merely "nothing can".

**The pair button.** The button is on GPIO25, active low, with the pin's internal pull-up (Bible
§3, §11). No capacitor is fitted on it, so the debounce is the firmware's: `loop()` counts a press
on **release**, and only after the button was held for more than 30 ms. (A code comment once
claimed a capacitor there; the Bible records none.)

A press, only in BT mode, calls `gracefulDisconnect(BT_LINK)`. It drops whoever is connected
first; the code records this as the author's decision, "Otherwise a second phone can pair behind the
first and it stops being obvious which one is in charge".

**Other controls.** Portal guest actions `bt.play`, `bt.pause`, `bt.next`, `bt.prev`,
`bt.disconnect` and `bt.pair`; admin action `bt.forget`, which removes every stored bond
(`forgetPairings()`). If a phone negotiates a sample rate other than 44,100 Hz, `onSampleRate()`
only warns on the USB console; there is no resampler, so such a stream plays at the wrong pitch.

**Bluetooth transmit power.** `applyBtTxPower()` runs once, after `a2dp.start()` and before the
first scan mode lets the radio transmit, as the IDF requires. It sets the controller's power range
to a floor of 0 dBm and a ceiling of `btTxLevel`, which defaults to 0 dBm. The stock range is
0 to +3 dBm. The floor stays at 0 dBm unless the ceiling is set lower, because a low floor lets the
controller run near its margin and raises retransmissions. The portal action `sys.bttx 0..5`
(−12 to +3 dBm, down only) changes it; `sys.bttx 5` restores stock. It is not stored, so every boot
is at 0 dBm. The controller's own readback is taken once, when the value is set, never from the
4 Hz state message. This was a pop mitigation tried on 2026-09-11, committed 2026-09-12. **Its
effect on the pops was never measured.** It was kept as it is.

### 10.2.11 The Bluetooth lamp

`src/a32/btled.cpp` drives the lamp on LEDC channel 0 at 2 kHz, 8-bit. The pattern for each state
is the author's specification, implemented as written:

| state | pattern |
|---|---|
| OFF | off, and actively driven off |
| ON | steady, 64/255 |
| LOOK | raised-cosine breath 32 → 255 → 32 over 1200 ms |
| STDBY | raised-cosine breath 8 → 120 → 8 over 4000 ms |
| FOUND | 150 ms on, 150 off, 150 on, then CON by itself |
| CON | steady, 180/255 |
| LINK | 300 ms on, 300 ms off, repeating |

The lamp is **active low**, so a brightness B is written as 255 − B. "Off" is driven high rather
than left as an input, because an input pin's leakage lights the LED faintly in a dark room.
The lamp is on GPIO33 with its anode on the supply, so the pin sinks its current (Bible §11).
Breathing uses a raised cosine because a linear ramp reads to the eye as a fast rise and a
long dim tail. Each state has a scale 0–255 (`ledBrightness[7]`, default 255, "exactly as
specified"); OFF ignores its scale. `BtLed::update()` runs every loop pass. `BT_FOUND` moves itself
to `BT_CON` after 450 ms, so "found" always resolves; since 2026-09-24 `updateBtState()` reads
`BtLed::state()` back to publish that change, so the lamp and the published state share one clock.

**Full drive.** At the default scale of 255 the firmware drives the lamp **fully on** (the pin
held low for the whole PWM period) for 300 ms of every 600 ms in LINK, at the top of every LOOK
breath, and for both 150 ms flashes of FOUND. That is deliberate. It was kept as it is, relying on a safety analysis of this lamp's drive (not
documented here). A lower per-state scale in the portal lowers it.

### 10.2.12 Where the settings live

The A32 owns its audio and Bluetooth settings: `cfgA` (a `ProtoAudio`), `cfgB` (a `ProtoBtCfg`)
and the three knob-calibration points. They live in the A32's own NVS namespace `amb`. The S3
keeps a mirror, fetched with `MSG_GET_CFG` at every handshake. Writes are **debounced by 2 s**
after the last change (`settingsFlush()`), because a portal slider dragged across its range would
otherwise write flash at every pixel, and flash traffic was measured to disturb the S3's display.
A stored blob of a different size is rejected and the defaults apply. `MSG_REBOOT` saves first,
skipping the 2 s debounce, so a change made in the last two seconds is not lost to the restart
(2026-09-25).

The knob calibration lives on the A32, not the S3, because the A32 takes the readings: the reading
and the thing being calibrated must not sit on opposite sides of a link.

**Nothing is written while the A32's image is on trial** (v.1.0.4, 2026-09-26). An
image that arrived over the air runs "on trial" until it confirms itself (chapter 9). Until then,
`settingsFlush()` returns without writing, and changes wait in RAM. The reason: an update that
changed the size of `ProtoAudio` or `ProtoBtCfg` would otherwise save its new layout during the
trial, and if the update was then rolled back, the old image would find a blob of the wrong size
and run on defaults. When the image is confirmed, `confirmImage()` writes the held changes at once
and sends "settings changed during the trial are now saved" to the S3 as a `MSG_LOG` line, so the
hold is visible from the portal. A
restart during the trial is a rollback, so a change made during the trial and followed by a restart
is not kept, by design. An image flashed by USB is never on trial and saves normally. This is the
S3's own hold, brought over.

Proven live on 2026-09-26: during the trial of v.1.0.4 a lamp-scale setting was changed from the
portal and answered "ok"; the A32 wrote nothing until the image confirmed a minute later, and then
logged the line above together with "image confirmed (a minute of running with the S3)".

**Only a write that reached flash counts as saved** (2026-09-25). `settingsFlush()`
checks the result of every NVS write. If one fails, the change stays unsaved, the A32 prints and
sends to the S3 "settings NOT saved - the flash write failed; retrying", and it tries again after
the next 2 s. Before, the flag was cleared before the write, so a failed write still said "saved"
and the change was lost at the next reboot.

**The S3's copy is dropped when the A32 goes silent** (2026-09-25). Until the next
handshake fetches the settings again, a portal edit of any audio-board row is refused with HTTP 409,
"the audio board is not answering - its settings cannot be changed now", and a settings download
writes `n/a` for those rows. Before, the edit went nowhere and was overwritten when the A32 came
back.

### 10.2.13 Updates and the audio path

This is only the audio side of the A32's over-the-air update; the rest is in chapter 9.

- **Refused while on trial.** If the running image is still on trial (rollback armed), `OTA_BEGIN`
  is refused before anything is muted, with "still on trial - retry in 1 min"
  (2026-09-25).
- **Mute, then wait 400 ms.** `OTA_BEGIN` remembers the user's mute in `otaMutedWas`, forces
  `cfgA.muted = 1`, calls `Audio::setMute(true)` directly and then `delay(400)` before
  `Update.begin()`. Writing flash freezes code that is not in IRAM, the audio task included. If the
  fade and the DMA chain have not finished by then, `tx_desc_auto_clear` swaps in zeros mid-chain:
  a level step at the DAC, a click. The budget is at most 5.8 ms to see the mute, plus `fadeOutMs`
  (150 ms by default), plus the chain (46.4 ms), about 202 ms (2026-09-12). **The wait
  is fixed. It covers a fade-out of up to about 340 ms. `fadeOutMs` can be set up to 5000 ms, and
  any fade-out above about 340 ms makes every update start with a click.**
- **Giving up restores the user's mute**, not 0 (2026-09-24). A transfer silent for
  20 s gives up (`otaGiveUp()`). So a transfer that dies mid-image leaves the radio muted for up
  to 20 s. That is accepted as harmless.

### 10.2.14 The heap budget

**The A32 has about 24 KB of heap really free.** That figure was measured once, on 2026-09-11:
the A2DP library's own report of `esp_get_free_heap_size()`, printed on the A32's USB console, read
23,620 bytes with that day's build. It has not been re-measured since.

**+32 KB of DMA buffers broke updates and Bluetooth.** On the evening of 2026-09-11,
`dma_buf_count` went from 8 to 16 to ride out audio-task freezes of tens of milliseconds. The
driver allocates the same chain for transmit and receive, so that cost another 32 KB of DMA-capable
internal RAM, more than was free. Every update was then refused: `Update.begin()` returned false
with error code 0, which means its own 4 KB allocation had failed, on a chip seconds out of reset.
Phones and the PC could not connect over Bluetooth, and the chip crashed once. There is no way to
reset or reflash the A32 from the S3, so only a USB cable brought it back: no reset or boot-mode wire
runs from the S3 to the A32 (Bible §2, §3). The chain went back to 8 buffers the same evening, by a
USB flash at 19:15, and an over-the-air update of the very same image, refused six times in a row
before, was then accepted. The change had been shipped because the heap figure then in view read
about 231 KB and "never moved" (231,748 before, 231,740 after); that was the wrong instrument (see
"Which number to trust" below). A warning about the memory cost had also been
skipped. The
code's verdict: "a radio that cannot be updated without opening the cabinet is worse than one that
clicks when the audio task is starved."

The arithmetic, for anyone tempted again. Each buffer is 256 frames × 8 bytes = 2 KB; 8 buffers are
16 KB = 46.4 ms per direction; transmit and receive together are 32 KB. Every millisecond of chain
depth costs about 0.35 KB per direction, **about 0.7 KB per millisecond in total**, whether it is
bought with more buffers or longer ones (the driver caps one buffer at 4092 bytes). Longer buffers
differ only in how the memory fragments, not in how much they take. The 32 KB Bluetooth ring is
static memory and already inside the budget.

**Which number to trust.** The portal's `heap` field is the **S3's** own free heap
(`ESP.getFreeHeap()` in the S3's state builder). It says nothing about the A32; no A32 heap figure
reaches the portal at all. The A32's console `s` line prints `ESP.getFreeHeap()`, the free internal
heap in general, which is not the same as what a plain `malloc()` or a DMA allocation can get.
Before spending RAM on the A32, read `esp_get_free_heap_size()` and
`heap_caps_get_largest_free_block(MALLOC_CAP_DMA)` on the A32's own USB console.

### 10.2.15 Telemetry and diagnostics

Every 250 ms (`PROTO_STATE_MS`) `loop()` sends a `ProtoState` to the S3, which shows it on the
portal. The fields for this area:

| portal key | meaning |
|---|---|
| `under` | Bluetooth underruns (partial fills only) since boot |
| `ring` | ring high-water mark since the last read |
| peaks, `clip` | peak level per channel since the last read, in tenths of a dBFS; clip flag |
| `rmsl`, `rmsr` | RMS after the source gain, before the volume |
| `zarm`, `zrel`, `zsince`, `zlong`, `zmute`, `zfloor` | zero-data witness: entries, releases, time since the last release, longest mute, muted now, floor on |
| `i2sraw` | the I2S fault latches, sticky since boot |
| `stalls`, `stallus`, `stlast`, `stlastus` | audio-task stalls longer than 23.22 ms |
| `praw`, `pmin`, `pmax`, `pjump`, `vsteps`, `vjump`, `vjms`, `gsteps`, `gmin`, `gmax`, `upot` | the volume watch: knob readings, their extremes and largest jump, volume steps, applied gain steps and range, knob in use |
| `dindrv`, `sddly`, `tail`, `clkdrv`, `mclkdrv`, `bckdrv`, `lrckdrv` | pad drives and registers, read back from the hardware; `clkdrv` = 255 means the three clock pads disagree |
| `dctest`, `dcword` | the constant-DC diagnostic |
| `a32ms` | A32 uptime |
| `bttx`, `bttxmin` | Bluetooth transmit power range as the controller reported it |

Every readback comes from the hardware (the pad, the register, the controller), never from a copy
of what the firmware thinks it wrote. That rule paid for itself once: a test script's portal
session had expired, its commands were silently refused, and only the pad readback showed that the
setting had never moved (10.8).

**Reading `i2sraw`.** Most of its bits latch in normal running. On this radio it read `0x092A`
(2346) at every read from 2026-09-11 to 09-15, across boots and through the heaviest popping on
record: bits 1, 3, 5, 8 and 11, which are `tx_put_data`, `rx_rempty`, `tx_rempty`, `in_done` and
`out_done` (decoded from the ESP-IDF register header `soc/esp32/include/soc/i2s_struct.h`), most
likely set at start-up. `rx_wfull` and `rx_rempty` are expected on BT and AUX, where nothing reads
the receive chain and it overflows by construction. So a steady `i2sraw` is not a fault, and it gave
no evidence of a transmit fault when pops happened. The USB console's `s` decodes the bits by name. The USB console alone shows the Bluetooth stack's heap
figure, the `[ZDD]` lines that `loop()` prints when the witness sees a release or a stall, and the
text of update errors.

### 10.2.16 The pop hunt

**What a pop was.** Loud, short clicks through the speakers, at irregular times, on RADIO and on
BT, never on AUX. This is the story of how they were chased, in the order it happened. It is kept
because the next person to hear a click should not start from zero. Every rate below is given with
its window; a bare "zero" is never written.

**2026-09-01, round one.** The first symptom was loud, repeated pops during Bluetooth playback. The
author made four observations: music over Bluetooth popped; pausing the phone stopped the pops;
muting the phone (still streaming, so frames of zeros kept arriving) stopped them; and a phone
volume a hair above zero, music inaudible, brought them back **at full loudness**. The mute test
was the author's own design and a clean one: same link, packet rate, decode load, CPU and radio; only the
sample values changed. The last observation killed every linear operation in the firmware as a
cause, the underrun click included (its size is exactly the last sample before the gap): turn the
phone down 40 dB and every artefact a linear stage can make drops 40 dB too. The underrun theory
explained the pause and the mute perfectly, was the leading theory for an hour, and died on this.

The tests of that day, each a measurement:

| test | result | what it excluded |
|---|---|---|
| S3 powered off completely, A32 alone, playing | no change | WiFi, 2.4 GHz coexistence, the link, the whole second board |
| RADIO source | popped the same way as BT | the whole Bluetooth branch: SBC, the ring, underruns, the library's volume |
| AUX | clean | nothing on its own: AUX writes zeros through the same path, so it only says zeros are safe |
| DAC mute input held high | no change | one of the DAC's mute routes (the pin was later found to reach nothing, 10.6) |
| peak meter through 60 s of continuous popping on RADIO | never above −118.4 dBFS; `clip` never set | the firmware's arithmetic: the words written were clean |
| pop timing | irregular and bursty: under one a second, sometimes 3 to 5 inside a second, sometimes 7 to 8 s apart | anything locked to a firmware period (the 172.3 Hz blocks, the 20 Hz knob reads, the 4 Hz state message) |
| ADC unpowered; then ADC powered with its I2S cable out | clean both times; with the cable out a 60 Hz hum went too | — |
| ADC's master clock cut in firmware, ADC still powered and wired | pops and hum gone; restored, both came back | passive loading: the converter had to be running |

How could words at −118.4 dBFS, about one bit, make a full-scale bang? A shift of a word by k bit
positions multiplies it by 2^k, 6 dB per bit, so reaching full scale needed k of about 16 to 20: the
DAC was losing or gaining bit-clock edges mid-frame. And a shift of zero is zero, which explained
why silence never popped without any content-gated cause.

The constant-DC diagnostic was then given a settable word: every slot carries the same fixed word,
silent as audio, so anything heard is the fault. Six words, each held at least 30 s on RADIO:

| word | level | lowest set bit | result |
|---|---|---|---|
| `0x00100000` | −66.2 dBFS | 20 | silent |
| `0x00FF0000` | −42.2 dBFS | 16 | silent |
| `0x000AAAAA` | −69.7 dBFS | 1 | few pops |
| `0x00155555` | −63.7 dBFS | 0 | many pops |
| `0x001FFFFF` | −60.2 dBFS | 0 | many pops |
| `0x000000FF` | −138.5 dBFS | 0 | many pops |

Level, bit count and edge density (the prediction of the day) all failed to order the results. The
**position of the lowest set bit** did, with no exception: `0x00FF0000` was silent and `0x000000FF`
popped, the same 8 bits, 96 dB apart. I2S sends the most significant bit first, so bit 0 sits right
against the frame-clock edge. The conclusion then was that activity on the data line at the word
boundary made the DAC lose its framing, whose clock at that time it recovered from BCK by itself.
Real audio has a busy lowest bit at every level, which is why the pops had not followed the music's
level. The I2S fault latches were clean in eight one-second windows during popping, so the fault
lay after `i2s_write`. The low-bit output mask was built as a fix; it was never confirmed by ear,
and of its widths only bits 0, 1 and 16 had known results. The ADC-clock toggle was retired and the
sleep rule added (2026-09-01). A hardware change was planned for the DAC's clock.

The lesson written down that day: three theories died, each to a measurement and not an argument,
and each had been stated more confidently than the evidence allowed. A measurement *consistent*
with a hypothesis is not one that *discriminates* between hypotheses.

**2026-09-02.** The author changed the DAC's clock wiring (Bible §6), and the radio played with no
pops at all ("The pops are gone."). Nine quiet days followed.

**2026-09-10: the pops return, and an instrument replaces the ear.** A separate microphone recorder
(a device streaming 48 kHz audio over USB, about a foot from the radio)
became the pop counter (the method is in 10.8). The baseline that night was 3.00 pops/min, stable
over three 10-minute blocks (2.80, 3.20, 3.00). Built and tested that day:

- The **zero-data witness and floor**, on the theory that the DAC's own analogue mute (after 1024
  zero frames) released with an unramped step. **Excluded**: `zrel` did not move through 8 minutes
  of pops.
- The **volume watch**, on the theory that the knob's reading wandered. **Excluded**: with the knob
  ignored and `gsteps` frozen, it still popped.
- The **stall counter**: `stalls` frozen and `under` at 0 during pops.
- Also excluded that day or the next: the needle motor, the FM tuner chip, the 5 V supply (the set
  ran from a bench supply), the tube set, the box fan, the earth, a mains filter, the room and the
  amplifier (AUX clean; the pop follows the amplifier's own volume).
- **Muted with the zero-data floor on** (the DAC's output stage kept alive on a constant word):
  0 pops in 3 minutes against 3.7/min playing. It was read then as "the pop needs changing data".
- **Mains is a contributor, not the cause.** A heat gun on the same circuit made it pop from 5 cm
  or 10 m away; on another circuit it did nothing at any distance, so the route is conducted, not
  radiated. The baseline survived on a quiet circuit, so two rates were adding up. An inlet filter
  changed nothing.
- **The sd_out_delay incident.** A register knob that shifts the data line in time
  (`tx_sd_out_delay`) was added that day and swept live. At position 2 it sent the radio to **full
  volume** through the valve amplifier; 1 and 3 silenced it. The speakers survived. See 10.6.
- **Gain staging.** The stored knob calibration (930, 927, 933) was implausible, so
  `potRotation()` silently fell back to its defaults, which had nothing to do with this knob. That
  pinned the volume near 18 of 255 (about −62 dB, the DAC near −77 dBFS) with the amplifier turned
  up to match, "so every fault arrives 62 dB louder than it should". A safety issue, not a quality
  one: recalibrate the knob before anything else.

**2026-09-11: the data-line story falls, and the clocks become the suspect.**

- The **sign-extended tail** (the last 8 bits of each word set to the sign of the next word, so the
  data line never moves across a word boundary) was tested blind with the heat gun, on-off-on-off.
  No change in rate or size, and the rate did not rebound when the tail went back off. The
  data-line mechanism was dropped. The "muted with the floor on: no pops" run of 09-10 had been
  over-read: its premise, that a 1-bit word keeps the DAC's zero-data mute off, was only the
  firmware's model of the DAC. The better reading is that the DAC slips through its clocks whatever
  the data, and a slip is audible only when the word is not tiny.
- **Volume is not a variable.** A result that "volume 17 brings the pops back" turned out to be one
  heat-gun burst. Timestamp every event before believing a rate.
- The remote path to `sd_out_delay` was removed: a build that still carried it was judged
  unsafe. (The console key that remained went too, in v.1.0.3; see 10.6.)
- Pop sizes fell on a ladder of binary weights (¼, ½, ⅝, ¾, 1, 3/2 of a main size; 79 events over
  three soaks, each rung within about 0.2 to 0.4 dB, no clipping). Polarity was mostly mixed, but the
  3/2 rung came out negative 9 times in 9. That reads as a few of the **top** bits of single words
  coming out wrong with the rest intact: neither a framing slip (it would smear) nor the DAC's own
  mute (one size, one polarity). It was read as a different fault from the one of 09-01, which the
  09-02 clock change had fixed.
- A morning alternation of clock drive 2 against 3 (10:05–11:05, 30 pairs of one-minute windows)
  was **inconclusive, not negative**: the fault was dormant, 3 loud pops in the hour (drive 2: 3,
  drive 3: 0, p = 0.125; at a −45 dBFS threshold 10 against 6, p = 0.22). The rate changes by the
  hour: earlier, four 2-minute quarters had been fooled by one burst (a p = 0.005 that vanished pair
  by pair).
- In the evening **it popped on a constant word** (`0xFF000000`, 5 minutes, Bluetooth source, tube
  set out, A32 volume 7, amplifier at maximum: 6 isolated pops). Two populations: 2 of the 6 matched
  the recorded step of the switch into the DC word (correlation 0.75 and 0.88) and sat inside a
  burst of stalls, so they were stall clicks; the other 4 were short impulses, not steps, the same
  sizes as in music. The pops do not depend on the data changing. A near-zero word had never popped,
  and this −42 dBFS word did, so visibility depends on the word's value: that fits the DAC
  mis-reading bits, not noise added in the analogue path.
- Two **crackle storms** of unidentified origin (dense detections for 35 to 60 s, peaks near
  −3 dBFS) crossed a music-against-DC alternation. They made that run's p-value (9.94e-28) invalid:
  199 of its 218 music events came from the storms, so the events clustered and the test's
  independence did not hold. The author did not recognise the sound.
- An A/B of `0xFF000000` against `0x00FF0000` (17:25–17:52, 12 cycles of one-minute windows,
  Bluetooth with no phone connected, drive 2) was inconclusive: 1 against 0 events, p = 0.489,
  under the rule fixed before the run that fewer than 6 events is inconclusive. The chain was alive:
  all 23 switch thumps were detected within 0.06 s of their stamps.
- The pops seemed to follow Bluetooth activity (1 in about 30.7 minutes idle against 12 in about
  6 minutes after a phone connected, the same constant word on the wire). That was not a designed
  A/B and time confounds it: the phone had simply been reconnected after supper.
- **The DMA incident** (10.2.14). Recovered over USB at 19:15.
- **The clock-pad drive A/B, 19:32–20:04.** One-minute alternation, 30 windows, a 220 Hz tone
  streamed over A2DP, the recorder counting. **Drive 2: 52 isolated pops in 936 s (3.33/min).
  Drive 3: 0 isolated events above −35 dBFS in 932 s. Stratified exact test p = 2.8e-16; all 15
  pairs went the same way.** A first scoring gave 2 events at drive 3 (p = 6.3e-14, 14 of 15
  pairs); both fell 4 s after the run's exit had already restored drive 2, and were credited to the
  last window only because that window closed at the end of the recording. No stalls in the run.
- **Both sizes of pop are pops.** The sizes that evening clustered in three families about 1 dB
  wide (−7.2, −9.2 and −25.2 dBFS at the microphone, 31 events). The quiet family's shape correlates
  only 0.41 to 0.46 with the loud one's, but the author listened to clips of both and called both
  pops, so the detector is calibrated against his ear. A low correlation is not evidence of "not a
  pop".
- **Drive-3 soak, 20:05–20:50**, a 220 Hz tone at −30 dBFS streaming over A2DP, A32 volume 47,
  zero-data floor on: **1 isolated pop in 45.1 minutes (0.022/min)**, about 150 times fewer than at
  drive 2. A shower in the house from 20:31 produced no false events.
- **90-minute soak, 20:55–22:25**, drive 3, constant word `0xFF000000`, A2DP streaming (the
  strongest rate amplifier found that evening): **0 isolated events in 90.0 minutes**, one stall
  (39 ms) with no event near it. The capture ran at 47,997 samples/s and lost 0.011 % of the time,
  which at drive 2's rate would hide 0.03 of a pop. Upper bound about 0.03/min: not proof of a cure.
- **The DAC's clock-error mute was excluded.** Its datasheet says a clock error attenuates and then
  mutes hard for about 104 samples (about 2.4 ms), so the programme must vanish. Through 37 recorded
  pops, the streamed tone's level, measured phase-insensitively (band-limited 180–260 Hz, analytic
  magnitude), changed by a median of −0.3 dB; only 8 of 37 dipped past −10 dB. A first attempt with
  a coherent 6 ms average at exactly 220.0 Hz read +4 dB: it was measuring the pop's own ring-down.
- `setClockDrive(3)` became the boot default, read back as 3 from all three pads after a cold
  power cycle. The Bluetooth transmit-power ceiling was added the same night (10.2.10).
- The state message's 4 Hz query of the controller's transmit power was removed that night, after
  an A32 reboot was blamed on it. The blame was withdrawn on 09-15: the uptime read had been the
  S3's, both boards had restarted within a minute, and the set had been power-cycled. **Why the
  A32 rebooted is unknown**; the removal was a precaution, not a proven fix.
- **Prior art** found on the web that day, generic to this DAC and the ESP32 (a search for this exact
  arrangement, one ESP32 master clock feeding both converters, was tried twice and never finished):
  TI's answer to a "click noise" case on a sibling DAC was to ground its system clock so the internal
  PLL runs from BCK (TI E2E 1052863), which is how this radio was wired before 09-02, when the pops
  were harder; TI publishes no data-to-bit-clock setup and hold figure for this DAC family (TI E2E
  1542019), and at a 354 ns bit period timing is not the issue, edge quality is. Nobody else reported
  pops that survive a constant word, or fixed pops with `gpio_set_drive_capability()`.

**2026-09-12.** Committed: drive 3 and the witnesses in the A32, their portal side, and the A32's message
handlers, the 400 ms update wait, the transmit-power knob. A supposed link between stalls and
Bluetooth was retracted: 13 stalls in about 33 minutes with the Bluetooth radio idle and the source
on RADIO (`stalls` 1 → 14 after a 12:12 boot), the longest 60 ms against the 46.4 ms chain. On
09-11 the stalls had seemed to track Bluetooth (0 in 34 minutes with nothing connected, 8 in the 7
minutes after a phone connected); that was coincidence with when the author happened to stream.
That retraction is about the stalls only, not about the pops' relation to Bluetooth: say "the stall
correlation", never "the Bluetooth correlation". The author fitted decoupling capacitors (see the
Bible).

**2026-09-13: drive 3 as a trade-off.** At drive 3 the radio path was heard as distorted, like
people lightly talking through kazoos; at drive 2 the radio sounded right but the pops came back
hard, like full-on popcorn, more than ever before.

Both were reproduced by changing `sys.clkdrv` and nothing else. It was the first result pointing
at the clock lines shared by both converters rather than at either part: `setClockDrive()` moves
all three pads together, which is why it had looked like one knob.

The record was also corrected: drive 3 had never *eliminated* the pops (they had been heard at drive
3, on BT, with the tube radio in or not), and both sources popped alike. Every measured number was from the
Bluetooth path only because that is what had been streamed: a limit of the evidence, not a property
of the fault. A hardware item suspected that week, on the master-clock line, had
always been fitted (Bible §3, §5), so it could not explain any change. At drive 2, Radio source, A32 volume 8, the
recorder counted **932 pops in 5.1 minutes (183/min)** at 09:52, 55 times the 09-11 rate: none
coincided with a stall, the median size was −14.7 dBFS, and 879 of the 932 fell in one 10 dB band
(−20 to −10 dBFS), unlike the three tight families of 09-11. Five conditions differed from 09-11
and none was isolated. The volume of 8 did not hide the pops; they were plainly audible. Any A32 volume other than 0
produces pops.

`setClockDriveSplit()` and the portal action `sys.clkpair` were written to drive MCLK and the
BCK/LRCK pair apart, predicting that MCLK 2 with BCK/LRCK 3 would give both good halves, because
the DAC latches its data on BCK edges while the ADC's conversion timing derives from MCLK (`32`
tests the opposite assumption). A volume-swing test was also designed: alternate the A32 volume
47 and 94 while a tone streams, and see whether the pops move with the tone (an error proportional
to the sample) or stay put (a fixed wrong code at the DAC). **Neither was ever run.**

**2026-09-15.** Corrections to the record: the drive-3 soak had Bluetooth streaming (not idle), so no
drive-3 measurement with Bluetooth idle exists; the split was never run; the cause of an A32 reboot
on 09-11 was withdrawn; the I2S latches read `0x092A` across boots and through the heaviest popping
(10.2.15); the "sign-bit flip at fixed amplitude" idea was not supported (10.6). Stall counts, each
since its own boot: 25 at 21.6 hours up (09-13) and 21 at 41.9 hours up (09-15). The raw recordings
and analysis scripts behind every number in this section were never copied out of a temporary
folder; only the numbers survive.

**2026-09-20 to 09-22: hardware, not firmware.** The author changed the audio cables (Bible §6,
§12), and reported that even before those changes no pops had been heard since the earlier
addition of ferrites.

No firmware changed for it, and no instrumented pop count exists after it. The split build was
committed on 09-22, still never run.

**The decision.** Drive 3 stays the boot default, and drive 2 will not be tried again: drive 3
works with no apparent consequences, and there is nothing to gain from drive 2. The split
`sys.clkpair` stays in the code, never run.

**What worked, and what it is.** `Audio::begin()` calls `setClockDrive(3)`: the MCLK (GPIO0), BCK
(GPIO18) and LRCK (GPIO17) pads at the strongest of the ESP32's four drive strengths, instead of the
power-on 2. It cut the pops about 150 times in the measured conditions. The story offered for it
is a hypothesis; no oscilloscope has ever seen an edge. GPIO0 drives two converter clock inputs
down a harness stub, the DAC's fallback clock recovery is off while an external SCK is present, and
a stiffer edge stops a marginal sampling instant latching the wrong bit. The data rate leaves
354 ns per bit, so the question is edge quality, not timing. It is **a margin fix, not a root
cause**: it says the DAC was latching wrong bits on a marginal edge, not why the edge was marginal.
Its cost, as reported on 2026-09-13, was the "kazoo" distortion on the radio path; drive 3 has
since been judged to work with no apparent consequences (10.7). `sys.clkdrv` stays
in the firmware so that drive 2 against 3 can be A/B-tested again, or drive 2 put back, at any
time without a reflash. The root cause was never found. `gpio_set_drive_capability()` cannot stop MCLK by
accident: GPIO0 is an RTC-capable pad, and the IDF writes only the pad's drive field; the pad's
function select is a separate bit.

---

## 10.3 Settings and constants

### 10.3.1 Stored settings (A32 NVS `amb`, editable from the portal)

| name (portal key) | default | unit | range | meaning | changed by |
|---|---|---|---|---|---|
| `volume` | 40, replaced by the first knob reading | step | 0–255 | master volume | knob; portal; console `+`/`-` |
| `muted` | 1 in the defaults, **forced to 0 at every boot** | bool | 0/1 | software mute | portal; console `m`; forced by an update |
| `taper` (`taperX10`) | 25 (gamma 2.5: −15 dB at half travel) | gamma × 10 | 1.0–4.0 in 0.1 | volume law `(v/255)^gamma`; 1.0 is linear amplitude, 2.0 is −12 dB at half travel, 3.0 is −18 dB, about 3.3 behaves like a real audio-taper potentiometer | portal; console `t`/`T` |
| `balance` | 0 | step | −100 to +100 | negative is toward the left | portal |
| `gainRadio` | +8.0 | dB | −20 to +30 in 0.5 (A32 clamps −60 to +30) | radio makeup gain before the volume | portal |
| `gainBt` | −7.0 | dB | −20 to +30 in 0.5 | Bluetooth gain before the volume | portal |
| `monoSum` | on | bool | | sum the radio's two slots | portal |
| `muteOnChange` | on | bool | | fade across source changes | portal |
| `fadeInMs` | 1000 | ms | 0–5000 in 50 | fade in (0 acts as 1 ms) | portal |
| `fadeOutMs` | 150 | ms | 0–5000 in 10 | fade out (0 acts as 1 ms); above about 340 ms, updates start with a click (10.2.13) | portal |
| `connectable` | on | bool | | "Let a paired phone reconnect"; off closes Bluetooth entirely, the pairing window included | portal |
| `lookTimeoutS` | 90 | s | 15–600 in 5 | the pairing window (`BT_LINK`) | portal |
| `pauseOnLeave` | on | bool | | AVRCP pause before every disconnect | portal |
| `btLedOff` … `btLedLink` (`ledBrightness[7]`) | 255 each | step | 0–255 | per-state lamp scale; OFF ignores it | portal |
| knob calibration `potMin`, `potMid`, `potMax` | 60, 0 (no centre), 3990 | raw ADC | 0–4095 | three-point knob map | portal `pot.min`/`pot.ctr`/`pot.max`; console `p`/`c`/`P` |

**`autoConnect` is not a setting any more.** The byte stays in `ProtoBtCfg`, defaults to 0 and is
stored with the rest of that structure, but no code reads it: `a2dp.set_auto_reconnect(false)` is
compiled in, and the radio never calls a phone. Its portal row was removed on
2026-09-25, and a settings file that still carries an `autoConnect` line uploads cleanly (the line
is skipped). The byte was kept because removing it would change the structure's size, and the A32
rejects a stored blob of another size (10.2.12).

### 10.3.2 Compiled constants

| name | value | unit | meaning | where |
|---|---|---|---|---|
| `SAMPLE_RATE` | 44,100 | Hz | fixed; no resampler | `audio.cpp` |
| `BLOCK` | 256 | frames | one pass, about 5.8 ms | `audio.cpp` |
| `RING_FRAMES` | 8192 | frames | Bluetooth ring, about 186 ms, 32 KB | `audio.cpp` |
| `dma_buf_count` / `dma_buf_len` | 8 / 256 | buffers / frames | 46.4 ms per direction, 32 KB for both | `Audio::begin()` |
| MCLK | 11.2896 | MHz | 256 × 44,100, from the APLL | `Audio::begin()` |
| clock pad drive at boot | 3 | 0–3 | GPIO0, 18, 17; remote knob clamps to 2–3 | `Audio::begin()` |
| data pad drive | power-on default | 0–3 | never set at boot | — |
| `tx_sd_out_delay` | never written; read back only (`sddly`) | 0–3 | nothing can write it since v.1.0.3; **hazard** | `sdOutDelayIs()` |
| audio task | core 1, priority 6, 4096 B | | | `Audio::begin()` |
| `ENV_ONE` | 2^24 | Q24 | envelope unity | `audio.cpp` |
| taper clamp | 10–40 | gamma × 10 | | `setTaper()` |
| balance step | 655 | Q16 per unit | ±100 leaves about −65 dB | `audioTask()` |
| `ZDD_FRAMES` | 1024 | frames | DAC zero-data window, 23.22 ms | `audio.cpp` |
| `ZDD_LSB` | `0x00000100` | word | one 24-bit step, −138.5 dBFS (the floor) | `audio.cpp` |
| `ZDD_STALL_US` | 23,220 | µs | a write gap above this is a stall | `audio.cpp` |
| `DEVICE_NAME` | "Ambersong" | | Bluetooth name | `main.cpp` |
| `BT_DROP_TIMEOUT_MS` | 3000 | ms | disconnect in flight; stray-drop spacing | `main.cpp` |
| pause-to-disconnect gap | 250 | ms | | `gracefulDisconnect()` |
| LOOK → STDBY | 20,000 | ms | | `updateBtState()` |
| `btTxLevel` | `ESP_PWR_LVL_N0` (0 dBm) | level 0–5 = −12 to +3 dBm | ceiling at boot; floor 0 dBm | `main.cpp` |
| `kDc[7]` | `0xFF000000`, `0x00FF0000`, `0xFFFF0000`, `0x00FFFFFF`, `0x000000FF`, `0x00100000`, `0x001FFFFF` | words | DC diagnostic words, all at or below −42 dBFS | `main.cpp` |
| DC word at boot | `0xFF000000` | | the proven popper | `audio.cpp` |
| update mute wait | 400 | ms | before `Update.begin()` | `onMessage()` |
| update give-up | 20,000 | ms | no frame from the S3 | `loop()` |
| source and knob poll | 50 / 50 | ms | | `loop()` |
| `POT_DEADBAND` | 24 | ADC counts | about 0.6 % of travel | `main.cpp` |
| selector thresholds | 424 / 2471 | raw ADC | AUX / BT / RADIO | `pins.h` |
| `PROTO_STATE_MS` | 250 | ms | state message to the S3 | `proto.h` |
| `PROTO_SILENCE_MS` | 2000 | ms | S3 considered gone, A32 sleeps | `proto.h` |
| settings debounce | 2000 | ms | NVS write delay | `settingsFlush()` |
| pair button | release after > 30 ms | | only in BT mode | `loop()` |
| lamp PWM | LEDC channel 0, 2 kHz, 8-bit | | | `btled.cpp` |

### 10.3.3 Experiments: portal actions and console keys

Experiments are **actions, not settings**: none survives a reboot. The code gives the reason: "an
experiment that survives a reboot is one whose result gets attributed to something else a week
later." Every one is read back from the hardware.

| portal action (message) | effect on the A32 | range | notes |
|---|---|---|---|
| `sys.clkdrv` (`MSG_CLK_DRIVE`) | drive of GPIO0, 18 and 17 together | clamped to 2–3 | the pop knob; boot value 3 |
| `sys.clkpair` (`MSG_CLK_PAIR`) | MCLK drive, then BCK/LRCK drive | 22, 23, 32 or 33 only; others refused | never run on the radio |
| `sys.dindrv` (`MSG_DIN_DRIVE`) | data-line pad drive | 0–3 | 0 and 1 may mis-decode for long stretches: someone present, amplifier down |
| `sys.dctest` (`MSG_TEST_DC`) | replace the output with the DC word | 0/1 | silent as audio; anything heard is the fault |
| `sys.dcword` (`MSG_DC_WORD`) | pick `kDc[index]` | 0–6; others refused | an index, never a raw word, so nothing louder than −42 dBFS |
| `sys.zfloor` (`MSG_ZERO_FLOOR`) | zero-data floor | 0/1 | |
| `sys.tail` (`MSG_SIGN_TAIL`) | sign-extended tail | 0/1 | ruled out 2026-09-11 |
| `sys.pot` (`MSG_USE_POT`) | ignore the front knob | 0/1 | |
| `sys.bttx` (`MSG_BT_TX`) | Bluetooth transmit ceiling | 0–5; others refused | down only |
| retired `MSG_ADC_CLOCK` | ignored | | retired 2026-09-01 |
| retired `MSG_SD_DELAY` | ignored | | retired 2026-09-11 |

USB console on the A32's own cable, 115,200 baud: `+`/`-` volume ±10 (`+` also un-mutes), `m`
mute, `b` pair, `v` knob on/off, `p`/`c`/`P` knob minimum/centre/maximum, `d` cycle the DC word,
`k` cycle the output mask (0, 1, 2, 4, 6, 8, 12, 16 bits), `g` cycle the data-line drive 0–3,
`t`/`T` taper −/+0.1, `r` raw knob stream at 10 Hz, `s` status (the knob reading and its
calibration, the I2S latches decoded by name, the link and awake state, every pad and register
read back from the hardware, peaks, ring, underruns and `ESP.getFreeHeap()`; it drains the
read-and-reset values, so prefer the portal for those, §12.5.4). The first `d` press selects
`kDc[1]` (`0x00FF0000`), because the boot word is `kDc[0]`. The boot help line does not list
`k` or `g`. The key `y`, which cycled `tx_sd_out_delay`, was removed in v.1.0.3 (10.6).

---

## 10.4 Decisions

**Decision. I2S is installed once, full duplex, and never reconfigured; the A2DP library never
reaches I2S.**
Why. The two paths need incompatible drivers, and reinstalling under Bluedroid risks hard faults.
The library leaked into I2S through `set_sample_rate()` and into the samples through its volume
control, so both doors are shut by empty objects rather than flags.
Rejected. Letting A2DP own I2S; reinstalling the driver at each source change.
When and evidence. `NullA2dpOutput` dated 2026-08-28 in the code; `A2DPNoVolumeControl` on
2026-09-01. `NullA2dpOutput` and `setup()` in `src/a32/main.cpp`.

**Decision. APLL on, a fixed 256 × fs MCLK on GPIO0; GPIO0 is never detached from I2S.**
Why. Both converters take their system clock from GPIO0, and only the APLL makes 11.2896 MHz
exactly.
Rejected. The integer divider from the 160 MHz clock (cannot make 44,100 exactly).
When and evidence. Comment and configuration in `Audio::begin()`; `setAdcClock()` retired on
2026-09-01.
The retirement of the ADC-clock toggle was the author's call.

**Decision. Mute is software only, computed from state, never stored.**
Why. No pin reaches the DAC's mute input. A mute toggled at events was undone by other code paths.
A stored mute booted the set silent on 2026-08-28.
Rejected. Driving GPIO16 as a hardware mute (it reached nothing); storing mute.
When and evidence. `applyMute()` (2026-09-01); `settingsLoad()`; GPIO16 write removed
(2026-09-23).

**Decision. The radio's two channels are summed to mono.**
Why. Both channels carry the same signal; the sum gains about 3 dB of signal-to-noise.
Rejected. None recorded.
When and evidence. `audioTask()`; setting `monoSum`, default on.

**Decision. Fades are asymmetric, 150 ms out and 1000 ms in, on a squared per-sample Q24 envelope.**
Why. Leaving a source is a deliberate act; arriving should be gentle. A linear ramp does not sound
like a fade.
Rejected. The first Q16 ramp, which advanced per sample and finished in 0.18 ms, a hard cut heard
as a jump-scare.
When and evidence. `settingsDefaults()`, `ENV_ONE` comment in `audio.cpp`.
The code records "author's request: a gentle arrival, not a jumpscare" and `proto.h` records
"Author asked for about 1 s in".

**Decision. The volume curve is a portal setting, gamma 2.5; the knob is calibrated at three
points.**
Why. The right curve depends on the amplifier and can only be settled by ear. The knob's raw
reading is not proportional to rotation, so no curve applied afterwards could fix it; a centre
calibration point does.
Rejected. A fixed logarithmic law.
When and evidence. `potRotation()`, `setTaper()`; 2026-08-31.

**Decision. The knob is a sensor: the first reading applies, volume is not stored.**
Why. The knob's position is the stored state; coming up at a stored volume the knob disagrees with
repeats the silent-boot failure.
Rejected. Storing volume in NVS.
When and evidence. `updateVolumePot()`.

**Decision. Radio gain +8 dB, Bluetooth gain −7 dB.**
Why. Measured with the RMS meter so that both sources are equally loud at every knob position and
the radio does not clip before the volume.
Rejected. +15 dB (clipped at every volume); matching by peaks (FM is limited hard, music is not).
When and evidence. 2026-09-23.
The author set the amplifier's ceiling by ear with the A32 at full volume, and kept these values.

**Decision. Bluetooth is invisible unless awake and in BT mode; the radio never initiates; only
`applyScanMode()` writes the scan mode.**
Why. The three rules (10.2.10) and "nothing connects at night".
Rejected. Letting the library write the scan mode.
When and evidence. 2026-09-24; `PolicyA2dpSink`, `applyScanMode()`.

**Decision. Pin ESP32-A2DP to commit `3245602`.**
Why. The visibility rule depends on the library's internals; an unpinned library changes under the
build without warning.
Rejected. Following the library's main branch.
When and evidence. 2026-09-25; `platformio.ini`.

**Decision. Pause the phone, then disconnect, whenever the radio lets a phone go.**
Why. A phone torn down mid-play leaves its app playing into nowhere.
Rejected. Staying connected outside BT mode.
When and evidence. `gracefulDisconnect()`, `updateSource()`.

**Decision. The pair button drops the connected phone first.**
Why. Otherwise a second phone can pair behind the first and it is no longer obvious which one is in
charge.
Rejected. None recorded.
When and evidence. `loop()`.
Recorded in the code as the author's decision since the first commit (2026-09-01).

**Decision. The A32 sleeps when the amplifier is off, and S3 silence counts as amplifier off.**
Why. Nothing to do with the amplifier off; and a dead S3 must be noticed.
Rejected. "Assume the amp is on so a dead helper board never silences the radio."
When and evidence. 2026-09-01; `serviceWake()`.
The intent is stated in 10.2.9.

**Decision. Forget `ampOn` when sleeping because the S3 went silent.**
Why. A stale "amp on" woke the A32 unmuted into an amplifier that was off.
When and evidence. 2026-09-25.

**Decision. The A32 writes no settings while its image is on trial; held changes are written at
confirmation.**
Why. A rolled-back image must never find a settings blob saved by the image it replaced.
Rejected. Saving during the trial, as before v.1.0.4.
When and evidence. v.1.0.4, 2026-09-26; `settingsFlush()`, `confirmImage()`; proven live
the same day (10.2.12).

**Decision. A failed update restores the user's mute, not "unmuted".**
When and evidence. 2026-09-24; `otaMutedWas`.

**Decision. Clock pads at drive 3 from boot.**
Why. About 150 times fewer pops in the measured conditions (10.2.16).
Rejected. Drive 2 (the power-on default); the untested split `sys.clkpair 23`. The remote knob
`sys.clkdrv` was kept so that the choice can be A/B-tested again, or reversed, without a reflash.
When and evidence. Measured 2026-09-11; committed 2026-09-12; `Audio::begin()`.
Drive 3 works with no apparent consequences, and there is nothing to gain from drive 2.

**Decision. The DMA chain stays at 8 buffers.**
Why. 16 buffers took 32 KB the chip did not have; updates and Bluetooth failed.
Rejected. 16 buffers. Also named in the code but never tried: longer buffers (up to the driver's
4092 bytes each), or a count raised only as far as the largest free DMA block allows; both cost the
same RAM per millisecond.
When and evidence. 2026-09-11; comment in `Audio::begin()`.

**Decision. Bluetooth transmit ceiling of 0 dBm at boot.**
Why. A pop mitigation: transmit bursts are the largest current step the board makes.
When and evidence. 2026-09-12. Never measured.

**Decision. The right channel goes in slot 0.**
Why. On this machine that order gave correct stereo. It is an adjustment for this machine; another
build must check it (10.2.3).
Rejected. The usual order (left in slot 0). A portal swap switch is a possible improvement (§12.8.1).
When and evidence. `audioTask()` and `Audio::begin()` (`I2S_CHANNEL_FMT_RIGHT_LEFT`). A
left/right test track over Bluetooth is still to be played.

**Decision. The blue lamp is driven fully on in LINK, at the top of LOOK and in FOUND.**
Why. That is the author's specification at the default scale of 255 (10.2.11).
Rejected. Capping the drive below full.
When and evidence. `BtLed::update()`; kept, relying on a safety analysis of the lamp's drive (not
documented here).

**Decision. Experiments are actions, never stored; the remote DC word is chosen by index only.**
Why. A stored experiment gets its result blamed on something else later; nothing louder than
−42 dBFS can be sent remotely.
When and evidence. 2026-09-01 to 09-12.

**Decision. Nothing in the firmware writes `tx_sd_out_delay`.**
Why. At position 2 it once sent the radio to full volume through the valve amplifier (10.2.16).
Its remote path went on 2026-09-11; the A32 console key `y` and the setter `setSdOutDelay()` went in
v.1.0.3. Only the readback (`sdOutDelayIs()`, published as `sddly`) remains.
Rejected. Keeping it on the USB console as a hazard with a warning.
When and evidence. 2026-09-12 for the remote path; v.1.0.3 (2026-09-26) for the key
and the setter.

**Decision. The `autoConnect` field stays in the protocol structure, unread; its portal row is
gone.**
Why. It did nothing: the radio never calls a phone. Removing the byte would change the size of the
stored structure (10.3.1).
When and evidence. 2026-09-25.

---

## 10.5 Failures and recovery

| what fails | what the firmware sees | what it does | how to recover | ladder class |
|---|---|---|---|---|
| RAM spent past the ~24 KB free (for example 16 DMA buffers) | nothing; `Update.begin()` fails with error 0, Bluetooth refuses connections | refuses every update | USB reflash only | **BLOCKER** (USB-only) |
| a future change writes `tx_sd_out_delay` (or the BCK/LRCK out-delays) with audio live; nothing in v.1.0.3 can | nothing | 1 or 3: silence; **2: full volume** through the amplifier | write 0; the speakers are at risk | **BLOCKER** (harm) |
| data-line drive set to 0 or 1 | nothing | the line may mis-decode for long stretches | set 2 or 3, or reboot | DEFECT |
| I2S install fails at boot | `[FAIL]` on the USB console | no audio task; silence | reboot; if it persists, reflash | DEFECT |
| audio task blocked forever (no known cause since 2026-09-01) | `loop()` keeps running; the loop watchdog does not watch the audio task | silence | portal reboot of the A32 | DEFECT |
| audio task starved for more than 46.4 ms | stall counted | the chain drains; zeros go out: a click | none needed | NOTE |
| phone negotiates a rate other than 44.1 kHz | `onSampleRate()` | USB warning only; plays at the wrong pitch | none | NOTE |
| phone clock drifts against the local 44.1 kHz | ring fill | ring slowly fills (drops) or empties (underruns) | none needed | NOTE |
| S3 silent for more than 2 s | `peerAlive()` false | sleeps: mute, Bluetooth closed, `ampOn` forgotten | automatic when the S3 speaks and says the amp is on | NOTE (by design) |
| stored knob calibration implausible | `hi ≤ lo + 64` | falls back to 60 / 3990, no centre | recalibrate with `pot.min`/`ctr`/`max` | DEFECT |
| stored knob calibration plausible but wrong | nothing | the knob maps wrongly | recalibrate | DEFECT |
| a disconnect request never lands | `btDropping` older than 3 s | warns on USB and to the S3 (`MSG_LOG`); lamp shows connected | none needed | NOTE |
| a phone connected while asleep or out of BT mode | `btConnected` | pauses and drops it, retried every 3 s | none needed | NOTE |
| protocol self-test fails at boot | `protoBroken` | muted, Bluetooth closed, link and update receiver kept up; says so every 10 s; never confirmed | image that arrived over the air: portal reboot of the A32, which rolls back (an update is refused while it is on trial); image flashed by USB: update to a good image (chapter 9) | DEFECT |
| an update stops mid-transfer | no frame for 20 s | gives up, restores the user's mute | none needed | NOTE |
| `fadeOutMs` set above about 340 ms | nothing | every update starts with a click | set it lower | NOTE |
| `connectable` off | policy | Bluetooth fully closed; the pair button blinks the lamp but no phone can find the radio | turn it on | NOTE (by design) |
| stored settings blob of another size | length check | defaults apply | set again from the portal | NOTE |
| a settings write to flash fails | a short NVS write | keeps the change unsaved, warns on USB and to the S3, retries 2 s later | none needed | NOTE |
| a settings change made while the A32's image is on trial, then a restart before it confirms | `gImgOnTrial` | holds the change in RAM; the restart rolls the image back and the change is not kept | set it again | NOTE (by design) |
| a portal edit of an audio row while the A32 is silent | the S3 has no settings copy | HTTP 409, "the audio board is not answering - its settings cannot be changed now" | wait for the handshake, then edit again | NOTE |
| the pops | nothing the firmware can see | none | they are events | NOTE; root cause open (10.7) |

---

## 10.6 Graveyard

Each entry is what was tried, what happened **on this radio**, and why it was dropped **here**.

These are dead ends for this particular setup, not necessarily for someone else's. The same goes
for the decisions made here: they were not necessarily the best ones.

| approach | what it was | what happened here | code today |
|---|---|---|---|
| Underrun theory | the pop is the ring's zero-fill click | pops stayed full size at a tiny phone volume; a linear stage cannot do that | underrun counter kept (partial fills only) |
| WiFi / coexistence theory | the S3's WiFi disturbs Bluetooth | S3 powered off: no change | — |
| Drain RX on every source | read the idle receive half on BT and AUX so it cannot disturb transmit (the leading candidate for an evening, while RADIO was believed not to pop) | RADIO, which reads it, popped the same | not in code; RX free-runs on BT and AUX by design |
| ADC master-clock toggle (`setAdcClock()`) | detach GPIO0 from I2S to silence the ADC | a hazard once the DAC also took its clock from GPIO0 (a clock error at the DAC); it also had two bugs, a null pin table that selected the built-in DAC pins (so the toggle worked exactly once: the first off was clean, and the peripheral was then left where the next off could not undo it) and a floating clock input | removed 2026-09-01; message id ignored. The code's advice: silence the ADC by muting its samples |
| Data-line framing (the 2026-09-01 "solved" reading) | activity at the word boundary makes the DAC lose framing | the DAC's fallback clock recovery that it blamed was switched off by the 09-02 clock change; the pops later came back on constant words | superseded; see below |
| Output low-bit mask | zero the bottom bits so the data line is still at the boundary | never confirmed by ear (the code records that a `k` sweep fixed nothing on RADIO; widths 2 to 15 bits were never tested one by one); it cannot touch the next word's sign bit that opens every slot | `setOutMaskBits()`, off, console `k` only |
| XSMT held high on GPIO16 | stop a floating mute input | no change; the pin was later found to reach nothing | removed 2026-09-23 |
| Weaker data-line drive | slow the "aggressor" edge | no result recorded that it helped; 0 and 1 risk mis-decoding | `setDinDrive()`, `sys.dindrv` |
| `tx_sd_out_delay` (and the BCK/LRCK out-delays) | walk the data edge off the frame clock; once written up as a "free" knob | position 2 sent the radio to full volume; 1 and 3 silenced it; only 0 plays correctly. Every I2S transmit out-delay mis-frames the DAC with nothing the firmware can detect | remote path removed 2026-09-11; the console key `y` and the setter removed in v.1.0.3 (2026-09-26); only the readback `sddly` remains |
| Zero-data mute release | the DAC's own analogue mute releases with a step | `zrel` did not move through 8 minutes of pops | witness kept as telemetry; floor off, `sys.zfloor` |
| Volume-step theory | block-boundary gain steps from a jittery knob reading | knob ignored, `gsteps` frozen, still popped | volume watch kept |
| Sign-extended tail | the data line never moves across a word boundary | blind heat-gun on-off-on-off: no change in rate or size | `setSignTail()`, off, `sys.tail` |
| DAC clock-error mute | pops are the DAC muting on a clock error | a streamed tone passed through 37 pops almost unchanged (median −0.3 dB) | — |
| One fixed-size sign-bit flip | every pop is the same flip | 20 of 31 pops had the opposite polarity (p ≈ 0.15), sizes spread over 19.1 dB | — |
| Stall–Bluetooth link | stalls are caused by Bluetooth | 13 stalls in 33 minutes with Bluetooth idle | stall counter kept |
| Deeper DMA chain (16 buffers) | ride out audio-task stalls | broke updates and Bluetooth (10.2.14) | back to 8 |
| DAC system clock grounded, internal PLL from BCK (TI's advice for clicks on a sibling DAC) | let the DAC make its own clock from BCK | that was this radio's state before 2026-09-02, when the pops were harder; not re-tried (Bible §6) | — |
| Clock drive split (`sys.clkpair 23`) | MCLK at 2, BCK/LRCK at 3, to keep both good halves | never run; drive 3 was kept, with nothing to gain from drive 2 | `setClockDriveSplit()`, `sys.clkpair` |

The constant-DC harness is kept on purpose: `0xFF000000` is the proven popper and `0x00FF0000` its
matched silent control from round one.

---

## 10.7 Limits and firmware notes

**Open.**

- **The root cause of the pops is not known.** Drive 3 is a margin fix. At drive 3 the measured
  residue was 1 pop in 45.1 minutes (tone over A2DP) and 0 in 90.0 minutes (constant word, A2DP
  streaming), both on 2026-09-11. No instrumented count exists after the cable changes of
  2026-09-20 to 09-22.
- **The 183/min of 2026-09-13 is unexplained.** Five conditions differed from 09-11.
- **The drive-3 "kazoo" distortion of the radio path** was reported on 2026-09-13, judged by ear
  only (no recording of it exists, so it and the pops were never put on one scale). No measurement
  of it after the cable changes of 2026-09-20 to 09-22 exists. Drive 3 is judged to work with no
  apparent consequences.
- **Why the A32 rebooted on 2026-09-11** is not known (10.2.16).
- The decoupling capacitors fitted on 2026-09-12 (Bible) may be part of the cause of the 183/min.
  Untested.
- **The raw evidence is gone.** The pop-hunt recordings and analysis scripts were never kept; only
  the numbers in 10.2.16 survive. The test tools were not kept either;
  anyone who needs such tools again will have to remake them.
- **The stall click.** The audio task is sometimes frozen for 23 to 111 ms. The cause was never
  measured; a flash write with the cache off is the leading guess. It is not fixed, because a
  deeper chain costs RAM the chip does not have (§12.5.2).
- **No waveform has ever been seen.** There was no oscilloscope. Distortion was judged by ear.
- **The firmware cannot see a DAC-side clock error.** No telemetry exists for it.

**Never run on the real radio.**

- **The channel order from the A32 and the DAC**: a left/right test track over Bluetooth is still
  to be played (10.2.3, §12.8.1).
- **Bluetooth invisibility in RADIO mode with a phone**: a check with a phone is still pending.
  It must also be run after any library bump (10.2.10, §12.5.5).
- An A32 settings write that fails, and its retry (10.2.12, §12.3.16).
- The portal's refusal of an audio-board edit while the A32 is silent (10.2.12, §12.3.16).
- `sys.clkpair` (committed, never run) (§12.5.5).
- The Bluetooth transmit-power A/B: the 0 dBm boot ceiling is unproven (§12.5.5).
- A drive-3 measurement with Bluetooth idle (§12.5.5).
- The volume-swing test (A32 volume 47 against 94 with a tone streaming), designed on 2026-09-13
  to tell a fixed wrong code at the DAC from an error proportional to the sample (10.2.16).
- A phone negotiating 48 kHz (§12.5.5).
- `connectable` off (§12.5.5).
- A long session measuring clock drift between a phone and the local 44.1 kHz (no rate adaptation;
  the 186 ms ring absorbs only jitter) (§12.5.5).

**Limits by design.**

- No resampler; no equaliser (§12.8.2). The path is bit-exact apart from gain, volume, balance and
  the fades. A digital equaliser on the A32 could only ever cover RADIO and BT, because AUX never
  passes through the A32; the one point that catches all three sources is analogue, after the
  amplifier's summing.
- The audio task is not watched by the A32's loop watchdog. A wedged audio task leaves the loop,
  the link and the portal alive, so a portal reboot fixes it (§12.5.3).
- The I2S clocks run even while asleep. They may be the source of a fixed comb of spurs the FM
  tuner chip sees at multiples of 2.8224 MHz (harmonics 28 to 34 match within one sweep step); that
is a hypothesis, and the one-variable test (stop
  I2S for about 90 s during a sweep) needs a small A32 hook that was never written (§12.1.7).
- No A32 heap figure reaches the portal. The only reliable number is on the A32's USB console.
- `sys.bttx` resets at every boot, like every experiment, so the boot value is always 0 dBm.
- The `autoConnect` byte is kept in `ProtoBtCfg` and read by nothing (10.3.1, §12.4.3).
- A reboot or an update of the A32 makes one pop of its own: the I2S clocks stop, the DAC goes to
  standby, and restarts.
- The blue lamp is driven fully on for 300 ms of every 600 ms in LINK, at the top of every LOOK
  breath and in both FOUND flashes, by the author's choice (10.2.11).

**Small known issues, left as they are.**

- The `OTA_BEGIN` wait is a fixed 400 ms; a fade-out above about 340 ms makes every update start
  with a click (§12.5.1).
- The portal action `bt.pair` sends a message named `MSG_BT_LOOK`, but it opens the pairing window
  (`BT_LINK`) (§12.4.4).
- Console `s` reads and resets the peak meter, the ring high-water mark and the I2S latches, so the
  portal misses those values for one period (§12.5.4).

---

## 10.8 Changing this area

**Invariants.**

- Never reinstall, reconfigure, `i2s_set_clk`, `i2s_stop` or re-pin I2S after `Audio::begin()`.
- Keep **both** `set_output(nullOut)` and `set_volume_control(&noVol)`, and the stream-reader flag.
- Never detach GPIO0 from the I2S peripheral. Never turn the APLL off.
- The audio task never delays, never prints, and blocks only in the I2S calls. `pushBt()` never
  blocks.
- Mute only through `applyMute()`. The start of an update is the one deliberate exception.
- The scan mode is written only by `applyScanMode()`. Never call the library's
  `set_discoverability()`.
- **Do not change the ESP32-A2DP pin without re-testing invisibility** with a phone: in RADIO, in
  AUX, asleep, after a portal hang-up and after a source change. Check first that every library
  path that writes the scan mode still goes through the virtual `set_scan_mode_connectable()`.
- Do not update the platform (`espressif32@7.0.1`).
- Slot 0 carries the right channel on this machine. On another build, play a left/right test
  track before trusting it (10.2.3).
- New `ProtoState` fields go at the **end**, so a version mismatch truncates rather than shifts.
  `PROTO_VERSION` is deliberately not raised for such an append, because a raised version loses the
  handshake, and with it the update relay to the A32 (chapter 9). New message ids are backward
  compatible.
- Readbacks come from the hardware, never from a copy.
- Read-and-reset counters (`getPeaks()`, `getRms()`, `btRingFill()`, `i2sFaults()`) have one reader:
  the 250 ms state message. `zeroWatch()`, `gainWatch()` and `i2sSticky` are monotonic and safe to
  read anywhere.
- `i2sFaults()` clears only the bits the driver does not service (masked against `int_ena`). An
  earlier version cleared everything and could lose the driver's buffer-done events, blocking the
  audio task forever (2026-09-01).

**The heap.** Before spending any RAM, read `esp_get_free_heap_size()` and
`heap_caps_get_largest_free_block(MALLOC_CAP_DMA)` on the A32's USB console. Budget about 0.7 KB of
DMA RAM per millisecond of chain depth. A +32 KB request was fatal once and only USB recovered it.

**The update wait.** If you raise `fadeOutMs` above about 340 ms, or deepen the DMA chain, raise the
400 ms wait in `OTA_BEGIN` to match.

**Hazards.**

- Never write `tx_sd_out_delay`, `tx_ws_out_delay` or `tx_bck_out_delay` with audio live. Since
  v.1.0.3 nothing in the firmware writes any of them. If one is ever needed again: amplifier down,
  mute, set, then un-mute at a known low level.
- Data-line drive 0 or 1, or any clock drive below 2, can mis-clock for long stretches. The clock
  setter clamps to 2–3; the data-line setter does not.
- Verify a flash by something only the new build can say (its version string, the A32's uptime
  resetting), not by a value the previous build already published.

**Traps in the code.**

- The volume gain steps at block boundaries; it is not ramped.
- `muteOnChange` off switches sources with no ramp: a thump.
- `setFades(0, …)` becomes 1 ms, on purpose.
- Balance ±100 is about −65 dB, not silence.
- The zero-data witness counts the DAC's idea of zero (the top 24 bits), not the firmware's.
- With the sign-extended tail on, a word of 1 to 255 followed by a non-negative word becomes exact
  zero. It matters only if the tail is ever turned on.

**Bluetooth timing races, already handled; do not undo.** `btDropping` is set before
`a2dp.disconnect()`; a stale flag is swept first thing in `updateBtState()`; `scanDirty` is cleared
before the write; `onConnState()` sets it again after `btConnected`; the wake edge cancels a pending
sleep disconnect and the stray-phone timer.

**How to count pops, if they come back.** The method that worked here:

- A microphone recorder about a foot from the radio, streaming continuous 48 kHz audio, with its own
  clock logged beside its sample count (a dropped chunk is a splice, and a splice looks like a
  click). Read it in bulk with a large driver buffer (1 MB here): the first attempt ran at 38 kHz
  with 18 lost chunks. A good capture runs at 47,99x of 48,000 samples/s. Turn the microphone gain
  down far enough not to clip a foot from a speaker.
- A detector on the 1 ms RMS of a high-passed signal (a 10-tap filter), with an absolute threshold
  of −35 dBFS (−45 as a secondary), 50 ms dead time, and an isolation score (event level minus the
  median over ±200 ms). Real pops scored 25–58 dB; crackle, speech and handling scored below 15.
  Have a listener's ear confirm clips of each size of event before trusting the detector. Do not use
  a shape filter ("falls 10 dB in 5 ms"); pops ring, and it kept only 29–61 % of real ones. Keep a
  heat gun, if one is used as a trigger, away from the microphone.
- **One-minute alternation, 15 pairs or more, both arms in the same recording, a source actually
  streaming**, and a stratified per-pair exact test. Never compare totals or one long block per arm:
  the rate changes by the hour (whole mornings of nothing, then bursts). A silent link sends exact
  zeros and hides pops, so use `sys.dctest 1` to make them visible whatever plays. Decide the rule
  before the run (fewer than 6 events is inconclusive). Close the last window at the end of the run,
  not the end of the recording. If events cluster (a crackle storm), the per-pair test is invalid:
  do not cite its p-value.
- Check the stall counter and timestamps to tell a stall click (step-shaped; it correlates with a
  recorded switch step) from a pop (an impulse).
- Traps that cost real time here. Portal sessions expire after 5 idle minutes: a long script must
  log in before every portal call, or its commands are silently refused (a whole 30-minute A-B-A
  soak was lost that way, and only the pad readback showed it). A script stamps a switch after its
  command and readback, so the real switch lands about 1.2 s before the stamp. Never edit a shell
  script while a copy of it runs: the shell re-reads the file and the running copy dies. A
  build-and-flash chain must test the build's own exit code (`pio run | grep` returns grep's status
  and would flash a failed build). A cleanup trap that restores a setting must also exit, or the
  script carries on toggling with no recorder. Two readers of a read-and-reset counter corrupt each
  other: a 40-minute "dropout" log was entirely the console `s` draining the peak meter. A reboot or
  an update pops by itself (10.7), so check the flash log before counting a pop "on its own".
- Rates to beat, each with its window: drive 2, tone over A2DP, 2026-09-11: 52 in 936 s
  (3.33/min); drive 3, same run: 0 in 932 s; drive 3, tone over A2DP: 1 in 45.1 minutes; drive 3,
  constant word, A2DP streaming: 0 in 90.0 minutes; drive 2, Radio, A32 volume 8, 2026-09-13: 932
  in 5.1 minutes.
- Say the window and the bound, never the bare word "zero".

---

# 11. The test tools and a short history

The only tools that ship in `firmware/` are the two build scripts in `firmware/scripts/`:
`page.py`, which checks the portal page's script, compresses the page and embeds it in the S3
build (8.2.8), and `version.py`, which stamps every build with its version and its commit
(chapter 3). The bench programs and test rigs used while the firmware was developed are not
published. Section 11.2 records what they proved, because several of the firmware's design
choices rest on those results. Section 11.1 is a short history of how the firmware came to be
what it is.

The dead ends and the decisions recorded in this Gospel were right for this radio, in this house.
They are not necessarily dead ends for someone else's setup, and the decisions were not
necessarily the best possible ones, even though they are still in the code: your mileage may vary.

## 11.1 History

The firmware's git history runs from 2026-09-01 to 2026-09-27 (tag
`v.1.0.5`); the release tag `v.1.0` is on the 2026-09-25 release. The repository started in the
middle of the work, so the first era below comes from the project's notes, not from git.

### Era 0: bring-up, before the repository (2026-08-26 to 2026-08-31)

Both microcontrollers arrived blank. On 2026-08-26 the toolchain was chosen and pinned
(chapter 3), and a bring-up test suite was written, one small program per test (11.2). Phases A
to G of those tests passed between 2026-08-26 and 2026-08-28. On 2026-08-30 a motion test rig
settled the needle's motion profile (11.2), and the needle was redesigned around a single index
sensor, with no end-stop switches (Bible §11).

On 2026-08-30 and 31 the firmware project itself was written, in three steps: the inter-board
link (framed, CRC-checked, at protocol version 2, with a self-test that corrupts a byte on
purpose), the A32's audio engine (built around the finding that the Bluetooth library's
sample-rate call reconfigured the I2S driver, which is why the library is given an output with
no route to I2S, §10.2.4), and the S3's core, with its versioned, migrated settings. Then, on 2026-08-31, the
web portal: one compressed page, settings generated from one table, user roles, salted password
hashes, sessions held in RAM, a back-off after failed logins, the rescue access point, NTP time
disciplining the battery clock, and over-the-air updates for both boards. The A32's relayed update was proven the same
night; it was the last thing the cabinet had to be opened for. The rule "the needle homes once"
came from that day.

Lessons from this era that still shape the code: check what a configuration call *returns*
(a PWM set-up call had silently refused 11 bits); never starve the idle task (the S3 boot-
looped); give every homing phase its own step budget; keep flash writes away from motion,
because a flash write stalls everything that is not in IRAM. Two more bugs of those days were
found on the hardware: the display showed `88:88` at brightness 255, and the battery clock's
"oscillator stopped" flag was sticky (it stays set until the time is written, which is how the
firmware uses it today, §9.2.14).

### Era 1: the repository is born inside the first pop hunt (2026-09-01)

The machine made audible pops, and the repository was created to track the investigation.

| Date | What and why |
|---|---|
| 2026-09-01 | First commit, "as it stands during the pop investigation". |
| 2026-09-01 | A diagnostic that isolated data activity at the audio word boundary. |
| 2026-09-01 | An ADC clock toggle retired; the A32 sleeps when the amplifier is off (the "awake" rule). |
| 2026-09-01 | Seven defects fixed, three of them regressions from the awake rule. |
| 2026-09-01 | A bounded portal jog; no reboot on top of unsaved settings. |

The update lessons came from this day too: the A32's BEGIN wait went from 5 s to 20 s, an
upload watchdog was added, and "an update that reboots cannot report its own success" became
a rule (chapter 3).

### Era 2: the needle's root cause, and the calibration procedure (2026-09-02 to 09-03)

| Commit | What and why |
|---|---|
| 2026-09-02 | Two-point dial calibration. |
| 2026-09-02 | The high soft limit could not be set, and a bus stumble had eaten a revolution. |
| 2026-09-02 | **The root cause:** the band calibration was measured and then thrown away, while its frame shift was kept, so every soft limit was about 640 half-steps wrong. |
| 2026-09-02 | Two of the previous fixes were destructive together. |
| 2026-09-03 | Homing budgets became constants based on the measured band; corrupt values are purged at load; provisional limits. |
| 2026-09-03 | A jog that landed on the index invented a slip. |
| 2026-09-03 | "Nine items toward v.1": the unguarded end-stop calibration removed (a hazard behind a button); sampling lag at index crossings no longer counted as slip; the author's password taken out of the image, replaced by a placeholder account with a forced change; the OTA settings save fixed; six misleading comments corrected. |

The full calibration procedure was run on the machine the same evening.

### Era 3: the dial calibrates itself (2026-09-04 to 09-07)

The firmware knows the tuning shaft's angle from a magnetic encoder, but the relation between
that angle and the frequency the tube set receives is not a straight line. This era taught the firmware to measure it, using an
RDA5807M FM receiver chip that listens to the tube set's own local oscillator.

| Commit | What and why |
|---|---|
| 2026-09-04 | The dial is a curve, not a line: the straight line was 1.4 MHz wrong at the bottom. The same day, bring-up Phase H proved that the tube set's oscillator can be heard and read (11.2). |
| 2026-09-05 | RDA phase 1. |
| 2026-09-07 | The radio measures its own dial; the curve becomes quadratic. The WiFi transmitter was held at 2.0 dBm, because at the time nothing could hear it at any higher power (measured 2026-09-07); the ceiling rose to 15 dBm on 2026-09-24 (Era 8). |
| 2026-09-07 | The RDA's results reach the portal. |
| 2026-09-07 | The curve predicts a position it never measured (98.5 MHz), which shows the fit is not circular. |
| 2026-09-07 | AFC off during measurements, and a stereo-pilot rejector. |
| 2026-09-07 | A 50 kHz fine pass. |
| 2026-09-07 | Phase 2: the radio keeps calibrating while it is being listened to. |

### Era 4: the needle recovers and watches itself (2026-09-08 to 09-10)

| Commit | What and why |
|---|---|
| 2026-09-08 | A self-recovery ladder: small errors are absorbed at each index crossing, a larger one re-indexes once tuning stops, three failures give FAULT. |
| 2026-09-10 | The needle had been chasing a rounding boundary while its telemetry said "stationary". |
| 2026-09-10 | A hunting detector. |

The stance behind this era: a needle that knows it has slipped must not run on, wildly
inaccurate, and do nothing about it. The hunting bug was found by watching the stepper driver's
LEDs, not by reading the code. It gave the firmware a rule it keeps: every failure mode must be
visible in the machine's own telemetry, at a resolution that would show it (the status line could
not show the hunting, because both positions rounded to the same published value).

### Era 5: the second pop hunt, and the rescue hatch (2026-09-10 to 09-12)

The pops came back on 2026-09-10. On 2026-09-11 an objective counter (a microphone box on USB,
one-minute A/B windows, 30 windows) showed that driving the A32's clock pads at full strength
("drive 3") took the pop rate from 3.33 per minute to none in the 932 seconds measured with
it (p = 2.8e-16). The same measurement excluded one suspected mechanism, the DAC's
clock-error mute. The root cause is still open (10.2.16).

| Commit | What and why |
|---|---|
| 2026-09-12 | Pop-hunt telemetry reaches the portal. A diagnostic message was retired after it sent the radio to full volume. |
| 2026-09-12 | Clock pads at full drive become the boot default. The audio DMA buffers stay at 8, because 32 KB more would exceed the roughly 24 KB of heap really free on the A32. |
| 2026-09-12 | The A32 answers the new settings; its fade wait before an update goes from 250 to 400 ms. |
| 2026-09-12 | The rescue access point becomes reachable and checkable, a required item before v.1. |

Drive 3 was not taken as free at the time: on 2026-09-13 it was heard to distort the radio path,
which is why Era 7 lets the master clock and the frame clocks be driven apart; that
split has never been run. Both sources, radio and Bluetooth, had popped. At v.1 drive 3 was kept
for this machine: it works with no apparent consequences, and there is nothing to gain from
drive 2.

### Era 6: a hardware freeze (2026-09-13 to 09-21)

Hardware and firmware changes were paused while the hardware record (the Hardware Bible) was
completed. The one firmware change of substance in this window is the 2026-09-20 change, which
brought `pins.h` comments and unused defines in line with the Bible without changing behaviour.
Among the defines it deleted were `S3_LIMIT_LEFT` and `S3_LIMIT_RIGHT`, used by nothing in the
firmware.

### Era 7: the IF as a setting, the console in the portal, index debounce (2026-09-22 to 09-23)

| Commit | What and why |
|---|---|
| 2026-09-22 | The audio master clock and the frame clocks can be driven at different strengths, for the next pop test (written 2026-09-13, when drive 3 on all three pads was heard to distort the radio path; Era 5). Never run on the machine. |
| 2026-09-22 | The tube set's intermediate frequency becomes a setting instead of a compiled assumption: 10.6 MHz (Bible §22). The RDA readings of that day, about 10.60 MHz over six stations, agree with it. |
| 2026-09-23 | Four stale things removed, each of which told the reader something untrue. |
| 2026-09-23 | Everything the S3 prints goes into a 16 KB ring that the portal can read and type into, so the console needs no cable between a PC and the machine (for the grounds involved, Bible §30.4). |
| 2026-09-23 | A real index debounce (four polls, back-dated so it adds no position bias). |
| 2026-09-23 | The zero handed to the stepper cannot be lost, and is applied before anything uses it. |
| 2026-09-23 | Hand marks belong to the user alone; the A32 measures RMS level. |
| 2026-09-23 | The compiled audio gains are the measured ones. |

### Era 8: the transmitter and the needle's speed (2026-09-24)

Six fixes from what was observed on the machine.

| Commit | What and why |
|---|---|
| 2026-09-24 | A forced access point lives long enough to be seen. |
| 2026-09-24 | A transmit power loaded from a settings file reaches the radio. |
| 2026-09-24 | The transmit ceiling becomes 15 dBm: measured that day, nothing could see the board at 20 dBm and everything could at 15. The ceiling is a firmware choice (8.2.4). The rescue access point was witnessed for the first time, a PC joining it and logging in. |
| 2026-09-24 | The needle cannot outrun itself through the index: the up leg drops to 1100 half-steps/s. Measured: at 1500 the needle stalled every time moving up through the index, at 1200 to 1400 often, at 1100 never; the stalls had left power-up sweeps landing about 400 half-steps low. |
| 2026-09-24 | The band check's margin grows with speed. |
| 2026-09-24 | The needle follows a filtered encoder reading, so encoder noise no longer moves it. |

### v.1.0 (2026-09-25)

The release, after a full review of the firmware (2026-09-24 to 09-25): one git tag, and the
first real rollbacks on either board.

| Time | What and why |
|---|---|
| 19:56 | "v.1: the S3 rolls back a bad update and restarts on a hang; builds name their commit." MAJOR becomes 1 and means only the author's release; `FW_COMMIT` (the short git hash, `-dirty` for uncommitted changes) is shown beside the version in the console banner, the portal, the settings file and the A32's boot report (chapter 3). The S3 gets the A32's recovery treatment: an image sent over the air boots on trial and is confirmed after about a minute of running with its network and portal up; an S3 upload is refused with HTTP 409 while the image is on trial; a failed link self-test no longer halts the S3; a 15 s task watchdog watches `loop()` and the needle supervisor (§4.2.15). |
| 20:03 | "An A32 build whose proto self-test failed is never confirmed, as on the S3." `confirmTick()` on the A32 returns early for such a build, so the next reset takes the previous image back. |

**The tag.** `v.1.0` is an annotated tag. The builds on the radio at the release
were S3 `v.1.20260925T200355` and A32 `v.1.20260925T200411`, both built from `v.1.0`, both
flashed over the air.

**What was proven live on the radio, 2026-09-25**, every update over the air, with the USB
cables unplugged:

| Build | What was seen |
|---|---|
| Pre-release (19:56), S3 | Booted `ON TRIAL (rollback armed), watchdog on`. A second S3 upload sent during the trial got HTTP 409, "this firmware is still on trial - retry in a minute". Then `image confirmed (a minute of running, network and portal up).`, seen by an uptime of 90 s. |
| Pre-release (19:56), A32 | Boot report `image ON TRIAL (rollback armed), watchdog on`, then `image confirmed (a minute of running with the S3)`. |
| `v.1.0`, both boards | Both booted on trial and were confirmed. |
| Rollback test, S3 | A deliberately hanging image (`loop()` spinning for ever; stamped `<hash>-dirty`, never committed) was sent by OTA. The task watchdog reset the S3 and the bootloader returned to `v.1.0`: `image : valid, watchdog on - an earlier update was ROLLED BACK`, `last reset : *** TASK WATCHDOG ***`, and the needle `remembered at -1154 ... homing from there`, then found the index. |
| Rollback test, A32 | The same kind of hanging image, relayed through the S3. The A32 came back on `v.1.0`: `boot: commit <hash>, reset reason 6, ... image valid, watchdog on - an earlier update was ROLLED BACK` (reset reason 6 is the task watchdog). |

These were the first real rollbacks on either board. Until that evening the rollback had been
seen only as far as the trial state, the refusal and the confirmation.

### v.1.0.1 (2026-09-25, evening)

One commit, S3 only; the A32 stayed on its v.1.0 build.

| Time | What and why |
|---|---|
| 21:13 | "v.1.0.1: a needle with no zero is homed, not re-indexed; nothing is saved while an S3 image is on trial." A re-index asked of a needle in FAULT after a failed home now runs a plain home instead (`startReindex()` in `src/s3/needle.cpp`). A failed re-index hands back the frame the needle had, and a needle that never homed has none, so it used to track in an unmeasured frame, soft limits and all. Second, `settingsWrite()` writes nothing while an S3 image is on trial; changes wait in RAM and are written once the image is confirmed, so a rolled-back image never finds a settings layout newer than it knows. Save says why it is held, and a portal reboot during the trial goes through at once (chapter 3, 3.2.9). |

**Proven live the same evening.** v.1.0.1 was sent to the S3 over the air and booted on trial.
A portal reboot during the trial answered "rebooting - this firmware was still on trial, so
the previous firmware comes back", and the S3 came back on `v.1.0` with "ROLLED BACK" in its
image line; no settings had been written during the trial. v.1.0.1 was then sent again and
confirmed after a minute. Tag `v.1.0.1`; S3 build `v.1.20260925T211316`.

**mDNS after the rescue access point, the same evening.** The question was whether the
machine's name, `ambersong.local`, still answers after the S3 has left the home network for
its rescue access point and come back, or whether the mDNS responder needs a restart. The
rescue access point was forced from the portal at 21:56:03. The S3 left the home network at
21:56:22 and came home by itself at 22:06:07, 10.1 minutes later. Raw mDNS queries, bypassing
the PC's cache, got no answer at that instant, then an answer to every query from 22:06:25,
18 seconds after coming home, to 22:08:56 (11 of 11). No restart is needed, and nothing was
changed.

### v.1.0.2 (2026-09-25, night)

Three commits: a set of fixes decided that evening, then a correction of every comment and of
every false console or portal message.

| Time | What and why |
|---|---|
| 22:02 | "v.1.0.2 (part 1)." The spur list's size guard, which the comments promised, now exists (a `static_assert`). The A32 counts a settings save only when the write reached flash, and retries a failure. When the A32 goes silent the S3 forgets its copy of the A32's settings: portal edits of audio-board rows are refused with HTTP 409, "the audio board is not answering - its settings cannot be changed now", and a settings download writes "n/a" for them. Boards running different protocol versions show it (a `wrong-version` count in the console's `s`, a portal pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both", the state field `linkver`) instead of looking like a dead wire. The RDA keeps the coarse sweep's points after a refinement and keeps the fine pass's 11 points (console `a`, `/api/rda` field `fine`). `W` claims to have set the battery clock only when the A32 answers. The battery clock's health reaches the portal (state field `rtc`: 0 fine, 1 answered with no valid time, 2 not answering; pills "BATTERY CLOCK LOST ITS TIME - check its battery" and "BATTERY CLOCK NOT ANSWERING"). The hand correction of the dial is never erased automatically; a new portal button, "Reset the dial correction to 0" (action `tune.nudgeZero`), is the only way. The RADIO gate is worded as the listening policy it is: "the dial calibrates only while you listen to the radio - switch the amp on and select RADIO". Removed: the `autoConnect` portal row, which did nothing (the field stays in the protocol structure, unread); the console keys `,` and `.` (`.` had no ceiling); the learned transmit power from the settings file (the console keeps its keys). Old files that still carry those keys upload cleanly. |
| 22:20 | "v.1.0.2 (part 2): every comment and console message says what the code does." Every comment in the sources, the portal page, the build scripts and `platformio.ini` was checked against the code and corrected, shortened or removed; hardware stated as fact in comments was replaced by pointers to the Hardware Bible. For 23 files the change was proven to touch comments only: each file, with its comments stripped, was compared with the same file at the previous commit (22:02). About twenty false console and portal messages were fixed, among them the sweep durations, a refused sweep called "already running", the reason given for a refused save, the downgrade lock's need for a complete file, the transmit-power key starting above the 15 dBm cap, and the drop-sample prompt asking for a frequency. Console `D` now prints the IF. |
| 22:21 | The embedded portal page carries the corrected drop-sample prompt. Tag `v.1.0.2`. |

Two rules came with them: a comment that is false serves no purpose; and a hand correction of the
dial is never erased automatically, only by hand.

Both boards were updated over the air, booted on trial and were confirmed. The S3's `s` showed
the link as "rx 489 crc 0 wrong-version 0"; the A32's boot report read "commit <hash> ... ON
TRIAL", then "image confirmed". `/api/state` carried `rtc` 0 and `linkver` 0, and `/api/rda`
carried `fine`. Builds: S3 `v.1.20260925T222141`, A32 `v.1.20260925T222159`, both `v.1.0.2`.

### v.1.0.3 (2026-09-26)

| Time | What and why |
|---|---|
| 10:34 | "v.1.0.3: a full sample table keeps the samples spread; the full-volume key is gone." When every automatic sample slot is full, the most crowded automatic sample (the one closest to any neighbour, counting the new point and the hand marks) makes room for a new dial position. If the new point would itself be the most crowded, nothing is stored ("would add the least coverage"). Hand marks are never replaced. Before this, a full table refused every new dial position until someone dropped a sample by hand, so the automatic sampler could not follow any drift once its nine slots were full. Second, the A32's console key `y` and its setter for the data-output delay (`Audio::setSdOutDelay`) are removed, because one of its positions sent the radio to full volume (Era 5); the readback stays. Tag `v.1.0.3`. |

Both boards were updated over the air on 2026-09-26, booted on trial and were confirmed: S3
`v.1.20260926T103447` and A32 `v.1.20260926T103503`, both `v.1.0.3`. The S3 ran that build
until v.1.0.5.

### v.1.0.4 (2026-09-26)

One commit, A32 only; the S3 stayed on its v.1.0.3 build.

| Time | What and why |
|---|---|
| 11:33 | "v.1.0.4: the A32 saves nothing while its image is on trial." The S3's v.1.0.1 hold, brought to the A32. `settingsFlush()` in `src/a32/main.cpp` writes nothing while the A32's image is on trial and not yet confirmed; changes wait in RAM, and `confirmImage()` writes them the moment the image is confirmed and sends `settings changed during the trial are now saved` to the S3's console. Before it, an A32 update that changed `ProtoAudio` or `ProtoBtCfg` would have saved its layout during the trial, and a rollback would have found a structure of the wrong size and run it on defaults; the only defence was to export a settings file first. Tag `v.1.0.4` (chapter 9, 9.2.3). |

**Proven live the same morning.** A32 build `v.1.20260926T113302` was sent over the
air and booted on trial; the test ran from 11:34 to 11:36. During the trial the Bluetooth lamp's level `btLedOff` was
changed from 255 to 254 in the portal, which answered ok. The A32 wrote nothing until 11:35:40,
when it logged `settings changed during the trial are now saved` together with `image confirmed
(a minute of running with the S3)`. `btLedOff` was put back to 255 at 11:36:36.

The pair then running was S3 `v.1.20260926T103447` (tag `v.1.0.3`) and A32
`v.1.20260926T113302` (tag `v.1.0.4`).

**Not yet exercised on the radio at v.1.0.4:** an RDA measurement showing both the coarse and
the fine lists; the A32-silent refusal (HTTP 409) of audio-board edits; the battery-clock pills
in a real fault; an A32 save failure; the replacement in a full sample table; the re-index of
a needle that never homed; a reset during the A32's trial with a setting held.

### v.1.0.5 (2026-09-27): the open rescue access point

One commit, S3 only; the A32 stays on its v.1.0.4 build.

| Time | What and why |
|---|---|
| 10:13 | "v.1.0.5: the rescue access point is open, and it opens the portal by itself." The rescue access point `Ambersong` no longer has a WiFi password: `AP_PASS` is gone and `startAp()` raises an open network. And while the access point is up, a DNS server answers every name with the access point's address and the portal's catch-all redirect names that address, so a phone that joins opens the sign-in page by itself (a captive portal). The DNS server stops with the access point. Tag `v.1.0.5` (chapter 8, 8.2.3, 8.4). |

The reason: a person trying to make the radio work again should not have to find and type a
passphrase. The cost (on the open access point nothing is encrypted) is stated in 8.2.6.

**Proven live the same morning,** from the S3's side. S3 build `v.1.20260927T101323` was
sent over the air, ran on trial and was confirmed. The access point was forced at 10:15:57; the S3
left the house network at 10:16:16; the console read "open, no password" and "captive portal:
every name now points at this radio.", and probe requests were heard. The S3 came home by itself at
10:26:01 (10.1 minutes), and `ambersong.local` answered from 10:26:19 on. Not yet seen: a phone
joining and opening the sign-in page by itself.

The running pair is therefore S3 `v.1.20260927T101323` (tag `v.1.0.5`) and A32
`v.1.20260926T113302` (tag `v.1.0.4`).

## 11.2 What the bring-up tests proved

The bench programs and rigs below are not published and are not needed to build or run the
firmware. This section records what each one proved, not how to rebuild it. Where a result is
about hardware, the Bible section is named.

### The bring-up suite (2026-08-26 to 2026-09-04)

One PlatformIO environment per test, one source file per environment, so that a failure is
isolated by construction. The phases ran in order, and a phase did not start until the one
before had fully passed.

| Phase and test | Proved | Result |
|---|---|---|
| A (meter only) | the 5 V side before any firmware | passed 2026-08-26 (Bible) |
| B1, S3 identity | the S3 build's memory settings are right | 16 MB flash, 8,386,295 B (8.00 MB) PSRAM seen by the firmware, stable, heap flat (Bible §1) |
| B1, A32 identity | the A32 has no PSRAM, which frees the pins the DAC needs | PSRAM 0 bytes (Bible §1, §3) |
| B2, real-time clock | the clock's I2C bus works | passed after a pin-map correction found by measuring; the clock's "oscillator stopped" flag read as set, and was reported, never cleared |
| B3, encoder | the encoder's I2C bus and magnet | passed; repeatability ±1 count, 0.08°, both on a full sweep out and back and on returning to a marked position |
| B4, inter-board UART | both directions at once | 14 of 14 round trips, 0 lost, 1 to 2 ms each |
| C5 to C7 | panel inputs and the amplifier sense | passed 2026-08-27, after wiring corrections (Bible) |
| D | the clock display | passed (Bible §8, §9) |
| E, stepper | the needle drive end to end | passed 2026-08-28. The first run moved the needle the opposite way to "forward". The fix inverted the phase advance so that there is one definition of forward, shared by the motion, the checks and the messages. The safety pattern carried into the firmware: check before every step, refuse a direction whose limit is reached, release the coils when idle. |
| F, audio | the whole audio path | F1 DAC playback, F4 ADC capture, F5 radio end to end: passed. F2: muting works in software by writing digital zeros, which the firmware has done ever since. F3: the A32 receives Bluetooth audio (A2DP) and plays it, which justified the two-board design (the S3 has no Bluetooth Classic) and the toolchain pin. F5 measured the radio at -53.2 dBFS RMS at the author's reduced volume, about 33 dB below plan: the origin of the gain-staging work. |
| G, display timing under WiFi | whether a web portal can share the S3 with the multiplexed display | a timer interrupt in IRAM drove the digits and timed every slot. WiFi plus four concurrent multi-megabyte HTTP streams: imperceptible. The same plus stepper motion: imperceptible. Deliberate flash-bus thrashing: the only visible disturbance, still bearable. Verdict: cleared. Consequences kept in the firmware: display and stepper on core 1, WiFi and the web server on core 0; the display interrupt stays in IRAM; settings writes are debounced; the display is parked during an S3 update. |
| H, the tube set's local oscillator | whether the dial can be read off the oscillator | see below |

**Phase H, 2026-09-04.** An RDA5807M on a loose ESP32 dev board, a few centimetres from the
running tube set, answered three questions.

- *Is the oscillator detectable?* Yes. A bare carrier (no stereo pilot, no RDS) moved
  one-for-one with the dial: dial 92.5 MHz gave a carrier at 81.8 MHz, dial 98.5 MHz one at
  87.8 MHz. The dial moved 6.0 MHz and the carrier moved 6.0 MHz.
- *Which side is the injection?* Low side: the oscillator runs below the station, at station
  minus IF (Bible §14, §22). The first sweeps had assumed high side and
  looked in the wrong window. Low side puts the whole dial's
  oscillator range inside the chip's tuning range.
- *Can the chip read the dial?* A closed-loop read gave 98.60 MHz against a verified 98.5 MHz,
  one 100 kHz step. Then two blind reads, the author moving the dial without saying where: both
  correct, 107.3 MHz (margin +10 over the local noise floor) and 88.5 MHz (margin +6).

Three lessons from Phase H are built into the firmware's self-calibration. Identify the
oscillator by what moves between two sweeps, never by amplitude: a fixed spur at 79.0 MHz was
louder than the oscillator at every one of five dial positions. Judge a rise against the local
noise floor, not a fixed threshold: a fixed +7 threshold missed the real +6 rise at the bottom
of the dial. And a 40 ms sweep dwell was a defect, because the signal strength is read before
it settles; 300 ms works. Coarse 100 kHz steps to find the oscillator, then fine 50 kHz steps
to pin it down, is what the firmware ships.

One more lesson came from the I2C diagnostics written for that rig: the whole evening's "nothing
on the bus" fault of 2026-09-04 was one flaky jumper wire, and the diagnostics said the pin map and
the chip were fine.

### The motion rig (settled 2026-08-30)

A bare stepper motor and driver module on a spare ESP32, with no needle and no load, used to
find how fast and how smoothly the drive could run, and which motion reads as "analogue" to the
eye, before any of it went into `needle.cpp`. These values were settled on the rig:

| Value | Setting |
|---|---|
| Profile up | second-order (a damped spring: slight overshoot, then settle) |
| Profile down (the fall) | decay |
| Micro-stepping | 1/16 while moving, 1/32 at rest |
| Up leg | 1700 half-steps/s, 18,000 half-steps/s² |
| Fall leg | 1700 half-steps/s, 18,000 half-steps/s², independent of the up leg |
| `wn`, `zeta` | 9, 0.50: about 5° of overshoot, a ring-down tail of about 300 ms |
| Dwell before the sweep | 100 ms |
| Hesitation before the fall | 1000 ms |
| Fall envelope, rise and fall | 100 ms, 1000 ms |
| Wobble | not viable on that gearbox |
| Steps per output revolution | 4096 half-steps, measured on the rig's own motor |

Why micro-stepping depends on activity: the step emitter has a 20 µs floor, so 50,000 steps
per second is a hard ceiling. At 1700 half-steps/s, 1/16 needs 27,200 steps/s and delivered
100 %; 1/32 needs 54,400 and delivered 92 %. But 1/32 is smoother where smoothness shows, in
slow motion. Measurements run at the moving division, because at 1/32 every rate reads about
8 % low.

The rig also found four motion bugs that had the same shape in the firmware: a step deadline
never recomputed as the profile accelerated (an s-curve emitted one step and sat still); linear
profiles braking after the point of no return (braking must begin before
`vmax² / (2·accel)`); a drive-mode lattice that made a target unreachable, so the motor buzzed
in place; and an arrival threshold that must scale with the step size. It also taught the
idle-task starvation trap that the S3 later hit with a busy step emitter.

The firmware's needle started from these numbers. The rig is retired, and its table is a
record of what was tried, not a standard the firmware is held to: the firmware's own defaults,
in chapter 5 and §7.3, are the rule. Where they differ from the rig, the machine decided. The
clearest case is the up leg, 1100 half-steps/s instead of 1700 since 2026-09-24,
because faster than that the needle stalled moving up through the index.

---

# 12. Firmware notes

This chapter collects the known issues in the firmware that were left as they are. None of them
stops the radio from working. Each one is written down so that if it ever starts to matter, whoever
picks it up does not have to rediscover it.

Every note has the same parts:

- **What happens:** what a person would see or hear, and when.
- **Why it was left:** the reason it was not fixed.
- **Where:** the function and the file.
- **Starting point:** where a fix would begin. This is a suggestion, not a design. Nothing here has
  been tried.
- **Class:** the recovery ladder of §0.5. **NOTE** means it recovers by itself, or it is only
  cosmetic. **DEFECT** means it recovers with a portal reboot, a console reset or the front switch.
  No note in this chapter is a BLOCKER.

Everything here was checked against the code at tag `v.1.0.3` for the S3 and
tag `v.1.0.4` for the A32, and brought up to `v.1.0.5` for the
S3. Anything those commits or earlier ones already fixed has been left out,
except 12.4.9, which v.1.0.4 fixed, and 12.6.2, which v.1.0.5 resolved; both are kept, marked, so
that the notes after them keep their numbers. Each chapter's section N.7 lists the
limits of its area. The ones that are simply how the machine works stay there. The ones that are
worth a starting point are here, and each N.7 item points to its note. Most groups end with a note
that gathers the paths never exercised on the radio, with a starting point for testing them.

---

## 12.1 Tuning

### 12.1.1 The top of the dial cannot be measured

**What happens.** The dial is printed 87.9 to 107.9 MHz (§5.9). The tuner chip (the RDA5807M, an
FM receiver the firmware uses as a measuring instrument, chapter 6) finds the dial's position by
listening for the tube set's local oscillator. The firmware only ever sweeps the oscillator between
77.2 and 97.2 MHz. With the IF setting at its default of 10.60 MHz (Bible §22), the oscillator for
107.9 MHz sits at 97.3 MHz, one step outside that range. So a measurement at the very top of the
dial either finds nothing or finds only the edge of the peak. Every station up to 107.8 MHz is
inside the range.

**Why it was left.** The highest station receivable where this radio lives is 107.3 MHz, and the
dial reads correctly up to there.

**Where.** The range is the pair of constants `Rda::LO_WINDOW_LO10` and `Rda::LO_WINDOW_HI10` in
src/s3/rda.h. `requestSweep()` in src/s3/rda.cpp clamps every sweep to them. `requestRefine()` in
the same file checks only that the centre of a refinement is inside them; the refinement's eleven
points then reach 0.25 MHz either side of it. `sampleStart()` in src/s3/main.cpp refuses a dial
position whose oscillator is well outside the range ("the dial is outside the range the oscillator
can be measured over").

**Starting point.** The range was set from the old nominal IF of 10.7 MHz and was not moved when the
IF became a setting (`ifOffset20`). Derive the range from the dial's printed ends minus the IF
setting instead of compiling it. Before widening it, check that it does not let in the fixed
features that the clamp now keeps out for free: those just above 97.2 MHz (98.7, 101.6 and
104.4 MHz, chapter 6) would then need to be learned as fixed features (`cfg.spur`) like the others.
At boot, `purgeCorruptSpurs()` in src/s3/main.cpp clears the whole stored fixed-feature list if any
entry lies outside the range, so that check must follow the new range too.

**Class.** NOTE.

### 12.1.2 Tuning readings shared between tasks without a lock

**What happens.** Three pieces of the tuner's state are written by one task and read by another
without any synchronisation:

- The status text (`gState`), which the RDA task rewrites as a measurement runs, and which the
  console and the portal print. A reader that lands in the middle of a rewrite can print a line
  that mixes the old and the new text.
- The sweep's results (the bins), which a console dump or a `GET /api/rda` can read while a sweep
  is filling them. The dump can show a mix of the previous sweep and the current one.
- The list of learned fixed features (`gSpur[]` and its count), which a settings change replaces
  while the RDA task may be reading it. The count is published in an order that keeps the reader
  inside valid entries, but neither the list nor the count is `volatile` and there is no memory
  barrier.

The first two are cosmetic: a garbled line on a screen. The third has never been seen to do
anything. At worst one sweep would judge one frequency against an old or new list.

**Why it was left.** Found during the comment cleanup of 2026-09-25. The effects are cosmetic or
theoretical, and a lock would add a new way for the RDA task and the portal to wait on each other.

**Where.** `gState`, the bin arrays and `spurSet()` / `isSpur()` / `isSpur20()` in
src/s3/rda.cpp. The readers are `portalRdaJson()` and the console `a` and `s` paths in
src/s3/main.cpp.

**Starting point.** Give the RDA task one small mutex, or copy on write: the task builds the status
text and the bins in a private buffer and publishes them with a single index or pointer swap. For
the fixed-feature list, mark the count and array `volatile` and publish with an atomic store, the
way `pushBt()` in src/a32/audio.cpp publishes its ring head.

**Class.** NOTE.

### 12.1.3 Two unused pieces in the tuner driver

**What happens.** Nothing visible. Two things in the tuner driver are never used:

- `tuneTo()` takes a `fine` flag that every caller sets to false. Setting it to true would be wrong:
  the function takes tenths of a MHz, so doubling the channel only addresses the same 100 kHz points
  again. The real 50 kHz path is `tuneTo20()`.
- `Rda::IF_OFFSET20_NOMINAL` (the nominal 10.7 MHz, in 50 kHz units) is read by nothing. It is kept
  as documentation.

**Why it was left.** Neither affects behaviour.

**Where.** `tuneTo()` in src/s3/rda.cpp; `IF_OFFSET20_NOMINAL` in src/s3/rda.h.

**Starting point.** Remove the `fine` parameter, so nobody can pass true. Remove the constant, or
keep it and let `LO_WINDOW_*` be derived from it (12.1.1).

**Class.** NOTE.

### 12.1.4 The encoder-gap warning always names the tuner's ends

**What happens.** When the tuning encoder (the AS5600, which reads the tuner shaft) stops answering
for a while and comes back, the console prints "AS5600 gap - angle re-seated on the turn inside the
tuner's ends". That is true only when both tuner ends have been measured. Without them, the firmware
picks the turn nearest the last count instead, but the message is the same.

**Why it was left.** A wording issue in a console line, found during the comment cleanup.

**Where.** `readAs5600()` and `seatTurn()` in src/s3/needle.cpp.

**Starting point.** Have `seatTurn()` report which rule it used, and print "on the nearest turn"
when the ends are not measured (or when the shaft lies outside them).

**Class.** NOTE.

### 12.1.5 A sample is stored to 100 kHz, so half the fine pass is rounded away

**What happens.** The fine pass measures the oscillator in 50 kHz steps, but a sample is stored in
tenths of a MHz (`tuneF`). The station frequency in twentieths is halved with rounding up, so an
odd-twentieth result lands about 25 kHz high: an oscillator at 95.05 or at 95.10 MHz both store
105.7 MHz with the default IF.

**Why it was left.** The error is a quarter of a channel step, and the dial reads true. It is noted
here with its fix.

**Where.** `sampleStore()` in src/s3/main.cpp (the `station10` line).

**Starting point.** Store `tuneF` in twentieths and fit the curve on those. That is a settings
version bump with a migration that doubles every stored mark (chapter 7).

**Class.** NOTE.

### 12.1.6 The console's fine-pass list does not mark a broadcast

**What happens.** The console `a` prints the fine pass's eleven points under the legend
"fine pass (50 kHz; 0 = skipped)". A point inside a fixed feature, or one whose read failed, does
read 0. A point rejected as a broadcast (stereo, above the floor) keeps the reading it took, and
nothing in the list says it was rejected. A reader can take it for a candidate that lost to the
chosen point. The portal's `fine` list in `/api/rda` has the same gap. Found 2026-09-26.

**Why it was left.** Not yet addressed. The choice itself is right; only the printout is
incomplete.

**Where.** The fine-pass loop in the RDA task, `gFinePtR[]` and `finePointRssi()` in
src/s3/rda.cpp; the `a` case of the console handler and `portalRdaJson()` in src/s3/main.cpp.

**Starting point.** Keep a stereo flag per fine point, as the coarse bins do (`binStereo()`), and
print "(STEREO - a broadcast)" after such a point, as the coarse list already does. Or complete the
legend.

**Class.** NOTE.

### 12.1.7 The comb's suspected source has never been tested

**What happens.** The tuner chip sees a fixed comb of features at multiples of 2.8224 MHz, which the
firmware learns and notches out (chapter 6). Their source is not known. One suspect is the A32's
I2S clocks, which run even while the A32 sleeps (chapter 10). The one-variable test, stopping the
I2S clocks for about 90 s while the S3 sweeps, needs a small hook on the A32 that was never
written.

**Why it was left.** The notches work, so the comb costs only part of the window (chapter 6).

**Where.** Nothing exists yet. The I2S driver is set up in `Audio::begin()` and fed by
`audioTask()` in src/a32/audio.cpp.

**Starting point.** Add an experiment action (reset at boot, like the others) that stops the I2S
peripheral for a set time and restarts it, then run a sweep inside that time and compare the
console `a` list with one taken with the clocks running.

**Class.** NOTE.

### 12.1.8 Tuning paths never exercised on the radio

**What happens.** Nothing has gone wrong. These paths are in the code and have never run on the
radio: the discard of a measurement after a bus error on the tuner chip, the "lost" path, a
re-probe after a real loss, the full-table replacement of v.1.0.3 (the table has never been full),
and a measurement that shows both the coarse and the fine lists.

**Why it was left.** Each one needs a fault or a state the radio has not had.

**Where.** `pointFailed()` and `markLost()` in src/s3/rda.cpp; `sampleStore()` in src/s3/main.cpp.

**Starting point.** On the bench, break the tuner chip's bus during a sweep and watch the console
for the discard and the "ABANDONED" line, then restore it and watch for the re-probe. Fill the
table to twelve samples and take one more. Run `a` straight after a measurement.

**Class.** NOTE. The class of each path if it fails is in chapter 6, section 6.5.

---

## 12.2 Needle

### 12.2.1 Direct writes of the needle position can race a pending correction

**What happens.** Nothing has ever been seen. This comes from reading the code, and it is a
hypothesis.

The needle's position (`gPos`) belongs to the step task (`stepTask()`, the task that emits the
motor's steps, on core 0). Other code hands it corrections through `gCorrectHs`, and the step task
folds them in with one atomic exchange at the top of every pass. It does this every pass, even in
test mode. A few functions still write `gPos` directly, with a plain read-modify-write:
`calFinishBand()` and `calStartBand()` (the band calibration; `calStartBand()` can run on the portal
task), `useMicro()`, `coilRelease()`, `jogRaw()`, `assumeAtLowStop()` and `bootPosition()`. They all
assume the motor is stopped. If a correction were pending at the instant one of them wrote, the step
task could apply it in the middle of the write and one of the two changes would be lost. The needle
would then run in a frame that is off by that correction, by a few to a few hundred half-steps.

It needs a non-zero correction at that exact moment. Corrections come from homing (`homingTick()`)
and from the index crossing check in `needleTask()`. The next index crossing, the idle check or a
re-index would find the error and correct it.

**Why it was left.** Rare, never seen, and it corrects itself.

**Where.** `stepTask()` and the functions above, all in src/s3/needle.cpp.

**Starting point.** Route every direct write through the same hand-off the corrections use, so that
the step task stays the only writer of `gPos`. For example, add a "set position to" request that
`stepTask()` applies at the top of its pass, next to the fold. A simpler step is to have each writer
wait until `gCorrectHs` reads 0 before it writes, as `homingTick()` already does before it marks,
measures or plans anything.

**Class.** DEFECT at most. The next index crossing or re-index corrects the frame.

### 12.2.2 The "Measuring pass speed" label is narrower than the setting

**What happens.** The portal row "Measuring pass speed" (`reapHsps`, in half-steps per second,
default 60) sets the speed of the band calibration's slow creep **and** the speed of the portal's
jog. It does not set the homing re-approach, whose speed is not a setting. Someone reading only the
label would not guess that it changes the jog, or might think it changes homing.

**Why it was left.** Judged not worth changing.

**Where.** The row in src/s3/settings_table.h; `Needle::setReapproachSpeed()`, called from
`applySettings()` in src/s3/main.cpp; the portal jog calls `Needle::jogRaw()` with `cfg.reapHsps`.

**Starting point.** Rename the label (for example "Calibration creep and jog speed"). The key
`reapHsps` stays, so settings files keep working.

**Class.** NOTE.

### 12.2.3 The boot gain check reads the compiled defaults

**What happens.** At boot, `Needle::begin()` warns on the console when the needle's tracking gain
(`wn`) is above the ceiling at which it overshoots and hunts. The ceiling is worked out from `zeta`,
`upAccel` and `upVmax`. The check runs before the stored settings are applied, so it only ever checks
the compiled defaults, which pass. A stored or portal value that breaks the ceiling is never warned
about, at boot or when it is changed.

**Why it was left.** Judged not needed.

**Where.** `wnCeiling()` and `Needle::begin()` in src/s3/needle.cpp.

**Starting point.** Call the same check from `applySettings()` in src/s3/main.cpp after the needle
settings are pushed down. Print the warning on the console, and return it as a `warn` to the portal
the way other settings warnings are returned.

**Class.** NOTE.

### 12.2.4 Portal needle commands run on the portal task

**What happens.** The portal's needle actions (home, track, sweep, band calibration, re-index, jog,
nudge, the limit captures) run on the portal task, on core 0, and write the needle's state
directly while the needle task runs on core 1. Only stop and abort are handed over as requests that
the needle task carries out itself. Nothing has been seen to go wrong. The jog also holds the
portal task for as long as it runs, up to about 20 s at the lowest `reapHsps`, and the portal does
not answer anyone in that time.

**Why it was left.** A known cross-core class, deferred. The actions are
used by one person at a time, with the needle at rest.

**Where.** The `needle.*` actions in src/s3/settings_table.h; `Needle::startHoming()`,
`Needle::track()`, `Needle::sweepRange()`, `Needle::calStartBand()`, `Needle::startReindex()` and
`Needle::jogRaw()` in src/s3/needle.cpp.

**Starting point.** Turn each action into a request that the needle task picks up at the top of its
pass, the way `Needle::stop()` already works, and answer the portal from the request's result. The
jog becomes a request that returns at once. See also 12.2.1.

**Class.** DEFECT at most. The jog's hold on the portal is a NOTE: it ends by itself.

### 12.2.5 Re-index: the portal's answer and refusal can name the wrong thing

**What happens.** On a needle that has no zero (a FAULT from a failed home), the portal's Re-index
runs a plain home, and the portal still answers "re-indexing". When Re-index is refused, the
refusal always says "a calibration or a bring-up tool has the needle", also when the real reason is
that the needle is homing or sweeping, or the reason `startHoming()` gave.

**Why it was left.** Not yet addressed. The console says what really happened.

**Where.** The `needle.reindex` action in src/s3/settings_table.h; `Needle::startReindex()` in
src/s3/needle.cpp.

**Starting point.** Have `startReindex()` return a reason string, as `startHoming()` does, and a
flag saying it homed instead, and answer "homing - there is no zero to re-index from" in that case.

**Class.** NOTE.

### 12.2.6 The `upVmax` range reaches speeds that stall the needle

**What happens.** The portal accepts `upVmax` (the top speed of every second-order move: tracking,
sweeps, homing) up to 4000 half-steps per second. The needle was measured to stall
through the index far below that (§5.9, 7.3.3). A value in the upper part of the range makes the
needle lose steps.

**Why it was left.** Not yet addressed. The default, 1100, is the measured limit of this mechanism.

**Where.** The `upVmax` row in src/s3/settings_table.h.

**Starting point.** Lower the row's maximum to the measured stall speed of this machine (§5.9), and
say in chapter 5 that another mechanism needs its own ceiling.

**Class.** DEFECT at most. The index crossing check finds the lost steps and re-indexes.

### 12.2.7 `HALFSTEPS_PER_REV` is unused

**What happens.** Nothing visible. `HALFSTEPS_PER_REV` is defined in src/s3/drive.h and nothing
reads it.

**Why it was left.** It does not affect behaviour.

**Where.** src/s3/drive.h.

**Starting point.** Remove it, or keep it as documentation with a comment that nothing uses it.

**Class.** NOTE.

### 12.2.8 Needle paths never exercised on the radio

**What happens.** Nothing has gone wrong. These paths are in the code and have never run on the
radio: a slip of more than 40 half-steps found by an index crossing (rather than by the band check)
driving the re-index ladder; the three-failure FAULT; any homing FAULT since the budget fixes; the
Re-index button on a needle with no zero (v.1.0.1); the RTC record after a crash, and the detection
of a torn record; the turn choice by the measured tuner ends after an encoder outage of more than
half a turn; a portal stop or abort landing in the middle of a band calibration.

**Why it was left.** Each one needs a fault the radio has not had.

**Where.** `needleTask()`, `homingTick()`, `startReindex()`, `bootPosition()`, `seatTurn()` and
`calAbort()` in src/s3/needle.cpp.

**Starting point.** Two can be reached from the portal: Re-index after a failed home, and a stop
during a band calibration. The slip and FAULT paths need lost steps, which a test build can fake by
shifting the counted position. The crash record needs a deliberate crash in a test build, sent and
rolled back over the air as the rollback tests were (chapter 3).

**Class.** NOTE. The class of each path if it fails is in chapter 5, section 5.5.

---

## 12.3 Settings and portal

### 12.3.1 The WiFi driver report calls normal power levels "LOW"

**What happens.** The console's `i` report prints the WiFi transmit power that the driver actually
holds. It adds "*** LOW - the transmitter is turned down ***" to any value under 40 (10 dBm). The
firmware's own transmit ladder has rungs below that (34, 28, 20 and 8, in quarter-dBm), and a board
that has learned one of them prints the warning every time, although nothing is wrong.

**Why it was left.** It is a bench line on the USB and web console. The comment next to it already
says to check the value against the rung.

**Where.** `Net::driverReport()` in src/s3/net.cpp.

**Starting point.** Compare the driver's value with the rung the firmware asked for, and flag only a
difference (or a zero), not a low value.

**Class.** NOTE.

### 12.3.2 Console `B` starts from its own place in its list

**What happens.** The console key `B` steps the WiFi transmit power through a fixed list (60, 44,
34, 28, 20, 8 in quarter-dBm, that is 15 dBm down to 2 dBm) and saves the result. It keeps its own
place in that list from boot, starting at 60. So the first press after a boot always sets 44
(11 dBm), whatever power the radio was running at, and saves it.

**Why it was left.** `B` is a bench key for a transmit-power test. The printout shows the new value.

**Where.** The `B` case of the console handler in src/s3/main.cpp.

**Starting point.** Start from the rung in force (`cfg.wifiTxQ`) and step to the next lower entry.

**Class.** NOTE. The value is saved, but a later press, or the transmit-power ladder that learns from failed joins (chapter 8),
changes it again.

### 12.3.3 The portal is starved while the needle moves

**What happens.** The step task (`nstep`, priority 19) emits the needle's steps on core 0 and does
not yield while the needle moves. The portal task (priority 3) shares that core, so the portal
answers slowly or not at all during a move. An A32 update is relayed by the portal task, and this is
the likely cause of the four failed A32 updates of 2026-09-01.

**Why it was left.** Accepted: the step timing comes first, and after v.1 the portal is not
needed while tuning. It is a quality item, not a blocker.

**Where.** `stepTask()` in src/s3/needle.cpp; `serverTask()` in src/s3/portal.cpp.

**Starting point.** Keep the needle at rest during an A32 update (the workaround in use). For a
fix: emit steps from a hardware timer or the RMT peripheral so the step task can sleep between
steps, then measure the step jitter as the 2026-08-28 work did (chapter 4) before and after.

**Class.** NOTE. It recovers by itself when the needle stops.

### 12.3.4 A settings write can land while the needle moves

**What happens.** Settings are written to flash only when `writeSafe()` says the needle is still,
because a flash write stalls the step task. `writeSafe()` judges that by the needle's speed. The
speed reads 0 during the pause before a move and during a portal jog, so a write can land then and
make the jog stutter. `settingsForceFlush()` also retries a write up to 40 times without asking
`writeSafe()` again, so a retry could start as the needle starts moving. The window is tiny.

**Why it was left.** Not yet addressed. The effect is a stutter, and the step task catches up.

**Where.** `writeSafe()`, `settingsFlush()` and `settingsForceFlush()` in src/s3/main.cpp;
`Needle::velHsps()` in src/s3/needle.cpp.

**Starting point.** Judge "still" by the needle's own state (no move in progress, no dwell, no
jog), not by its speed, and call `writeSafe()` again before each retry.

**Class.** NOTE.

### 12.3.5 No lock around the settings across the two cores

**What happens.** The settings structure (`cfg`) is written from `loop()` on core 1 and from the
portal task on core 0, with no lock. The known races are each a matter of microseconds: a hand mark
and an automatic sample stored in the same instant; a settings-file upload half applied when a
background save copies the structure, followed by a power cut within about 2 s; a soft-limit edit
crossing an apply. None has been seen.

**Why it was left.** The risk was accepted rather than
refactor the settings into a locked design.

**Where.** `cfg`, `settingsWrite()` and `applySettings()` in src/s3/main.cpp; the setters and
`settingsFromText()` in src/s3/settings_table.h.

**Starting point.** One mutex held by every writer of `cfg` and by the copy in `settingsWrite()`.
Or post every portal change to `loop()` as a request, so that only one core ever writes `cfg`.

**Class.** NOTE.

### 12.3.6 The page shows no "unsaved" state

**What happens.** The firmware knows when settings are changed but not yet on flash
(`settingsDirty()`), for example during the trial minute after an update, when nothing is saved.
The state JSON does not carry it, so the page cannot show it. It shows only the lock.

**Why it was left.** Not yet addressed.

**Where.** `settingsDirty()` in src/s3/main.cpp; the state JSON built in the same file; the pills in
data/portal.html.

**Starting point.** Add an `unsaved` field to the state JSON and a pill ("CHANGES NOT SAVED YET")
that names the reason the firmware already keeps for the Save button.

**Class.** NOTE.

### 12.3.7 A settings upload with a warning hides the count

**What happens.** After a settings file is uploaded, the page shows "N settings applied". When the
answer also carries a `warn`, the page shows only the warning, not the count.

**Why it was left.** Not yet addressed.

**Where.** The settings upload handler in data/portal.html.

**Starting point.** Show both: the count, then the warning.

**Class.** NOTE.

### 12.3.8 An unreadable `tuneUsed` line drops the marks silently

**What happens.** In a settings file, the `tuneUsed` line announces which sample slots follow. If
that line cannot be read, it is skipped without a note, and every mark line after it is then
skipped too. The file still uploads, and the stored marks are kept, but the page does not say that
the file's marks were ignored.

**Why it was left.** Not yet addressed. Nothing stored is lost.

**Where.** The `tuneUsed` branch of `settingsFromText()` in src/s3/settings_table.h.

**Starting point.** Return a `warn` ("the tuneUsed line could not be read - the file's samples were
not applied"), as the fixed-feature lines already do through `spBad`.

**Class.** NOTE.

### 12.3.9 The console dump `D` is only partly a settings file

**What happens.** The console `D` prints the settings in the file's `key=value` form, but its first
lines carry several pairs each, which the importer refuses. Only the tuning, IF (`ifOffset=`) and
fixed-feature lines can be pasted into a file as they are. Its `wifiTxQ` line is ignored by the
importer.

**Why it was left.** `D` is a bench view. The portal's download is the settings file.

**Where.** `settingsDump()` in src/s3/main.cpp.

**Starting point.** Print one pair per line, or print the same text as the download.

**Class.** NOTE.

### 12.3.10 The "Reboot the main board" toast shows only "rebooting"

**What happens.** When the portal's "Reboot the main board" succeeds, the page shows "rebooting",
whatever the radio answered. On trial the radio answers "rebooting - this firmware was still on
trial, so the previous firmware comes back", and that sentence never reaches the page. A refusal
is shown in full. Found 2026-09-26.

**Why it was left.** Not yet addressed.

**Where.** The `#rb` handler in data/portal.html; `hReboot()` in src/s3/portal.cpp.

**Starting point.** Show the answer's message: `toast(r.m || 'rebooting')`.

**Class.** NOTE.

### 12.3.11 Two S3 update messages are wrong or missing

**What happens.** An S3 upload refused because the running image is on trial also prints the
updater's "No Error" line on the console. An S3 upload refused because the image is for the wrong
chip gives its reason on the console only; the page shows "flash".

**Why it was left.** Not yet addressed. Both refusals work; only the words are wrong.

**Where.** `hOtaUpload()` and `hOtaEnd()` in src/s3/portal.cpp.

**Starting point.** Skip `Update.end()` when the upload was refused or never began. Keep a reason
string for the S3's own image, as the A32 relay does with `a32OtaSetError()`, and send it from
`hOtaEnd()`.

**Class.** NOTE.

### 12.3.12 Saving the Network card always rejoins

**What happens.** The Network card always sends the network name with the rest, so saving a
change to the NTP server or the time zone alone also makes the radio leave and rejoin its network
3 s later.

**Why it was left.** Not yet addressed. The rejoin takes a few seconds.

**Where.** The `#nsave` handler in data/portal.html; `hNetSet()` in src/s3/portal.cpp;
`Net::setWifi()` in src/s3/net.cpp.

**Starting point.** Have `Net::setWifi()` skip the rejoin when the name and passphrase are
unchanged, or have the page send them only when they changed.

**Class.** NOTE.

### 12.3.13 A portal-raised access point is confirmed on the console only

**What happens.** When the rescue access point is raised from the portal (`net.forceAp`), the line
that confirms the driver really raised it appears on the console only.

**Why it was left.** Not yet addressed. The console is readable in the portal's Console tab.

**Where.** `Net::forceAp()` and `startAp()` in src/s3/net.cpp; the `net.forceAp` action in
src/s3/settings_table.h.

**Starting point.** Carry the access point's state in the state JSON, so a browser that joins it
sees a pill.

**Class.** NOTE.

### 12.3.14 A blank board's rescue access point transmits at 2 dBm

**What happens.** The rescue access point transmits at the rung the transmit-power ladder has
learned. The ladder moves only on failed joins, so on a board with no network configured it never
moves, and the access point runs at the bottom rung, 2.0 dBm. On this machine on 2026-09-24 an
access point at 2.0 dBm "was visible at 44 % but could not be joined". On another board
it may be fine at close range; that is untested.

**Why it was left.** Not yet addressed. On this machine the rung was learned long ago.

**Where.** `gTxQ` and `applyTxPower()` in src/s3/net.cpp; the `wifiTxQ` default in src/s3/main.cpp.

**Starting point.** Raise the access point at a fixed rung known to be joinable (15 dBm on this
machine), or step the ladder while the access point is up and nobody has joined.

**Class.** NOTE. A blank board has just been flashed by USB, and the console `y` sets the network
over the same cable.

### 12.3.15 Console `y` echoes the WiFi passphrase

**What happens.** The console `y` prompt echoes every character typed, the passphrase included,
onto USB and into the console ring, which any administrator with the portal's Console tab open can
read.

**Why it was left.** Accepted for this machine. A builder may see it differently.

**Where.** `setWifiInteractive()` in src/s3/main.cpp.

**Starting point.** Echo the network name, then print `*` for each character after the space.

**Class.** NOTE.

### 12.3.16 Settings and portal paths never exercised on the radio

**What happens.** Nothing has gone wrong. These paths are in the code and have never run on the
radio:

- Settings: the `n/a` rows of an export and the 409 refusal of an audio-board edit, both while the
  A32 is silent; the downgrade lock (a settings version this firmware does not know), with the
  complete-file unlock, the "NOT SENT" note and the refused download while locked; a failed A32
  settings save and its retry.
- Network: the rescue access point coming back after a failed join on the current retry path
  (2026-09-25); the first-login placeholder gate and the whole first-time path of 8.2.2; the lockout
  beyond its first steps, the 507 "full" user path, and session eviction with more than four
  browsers.
- Captive portal (v.1.0.5): a phone joining the open rescue access point and opening the sign-in
  page by itself. On 2026-09-27 the S3's side was seen (the open network raised, "captive portal:
  every name now points at this radio.", probe requests heard); no phone had joined yet. The
  check with a phone is pending.

**Why it was left.** Each one needs the A32 silent, a downgrade, a flash fault, a network that
refuses the radio, or a fresh board; the captive portal needs a person with a phone at the radio.
The author does not downgrade.

**Where.** `settingsLoad()` and `settingsForceFlush()` in src/s3/main.cpp; the settings file's
export and `settingsFromText()` in src/s3/settings_table.h; `settingsFlush()` in src/a32/main.cpp;
src/s3/net.cpp and src/s3/portal.cpp for the network paths.

**Starting point.** A test build of the A32 that stops sending for a minute gives the silent case.
A test build of the S3 with a higher settings version, left to confirm itself and save, then
replaced by the release image over the air, gives the lock (a rollback cannot: nothing is saved on
trial). A spare board with the release image gives the first-time path. For the captive portal: raise the
access point from the portal, join `Ambersong` from a phone, and expect the sign-in page to
open by itself.

**Class.** NOTE. The class of each path if it fails is in chapters 7 and 8, sections 7.5 and 8.5.

### 12.3.17 "Drop one sample" also drops hand marks

**What happens.** The portal button "Drop one sample (slot 0-11)" and the `tune.drop` action accept
slots 0 to 2 too, which are the hand marks A, B and C. Dropping one removes that mark, where
the button's wording suggests only the automatic samples.

**Why it was left.** Not yet addressed. It is the only way to remove one hand mark without clearing
all of them, so it may be wanted; the label is what is loose.

**Where.** `tuneDropAction()` in src/s3/main.cpp; the button list in data/portal.html.

**Starting point.** Either relabel the button ("Drop a mark or sample, slot 0-11") or refuse slots
0-2 there and add a separate "Clear mark A/B/C" button.

**Class.** NOTE.

### 12.3.18 `sys.quiet` casts its argument unchecked

**What happens.** Nothing in practice. `doAction()` passes `sys.quiet`'s number through
`(uint16_t)arg`. For values of 65536 and above that conversion is undefined in C++; on this compiler
65536 becomes 0, which `Net::quiet()` then treats as its 1 s minimum. `Net::quiet()` caps every value
at 600 s, so nothing longer can happen.

**Why it was left.** Harmless.

**Where.** `doAction()` in src/s3/settings_table.h; `quiet()` in src/s3/net.cpp.

**Starting point.** Clamp `arg` to 0..600 before the cast.

**Class.** NOTE.

---

## 12.4 Link and A32

### 12.4.1 An A32 image built with the wrong protocol version confirms itself

**What happens.** An A32 image that arrives by an update runs "on trial" and is rolled back by the
next reset unless it confirms itself (chapter 9). It confirms after a minute of talking to the S3,
or after five minutes if the S3 never spoke. Frames carrying another `PROTO_VERSION` (the protocol
version number both boards compile in) are dropped and counted only as `badVer`, so to the A32 an
S3 speaking another version looks like an S3 that never spoke. An A32 built with the wrong
`PROTO_VERSION` by mistake therefore confirms itself after five minutes and becomes permanent. The
link stays down until the S3 is built with the same version.

**Why it was left.** It is by design. It is what lets a protocol change be rolled out one board at a
time (chapter 9). Since v.1.0.2 it is no longer silent: the S3's console `s` prints
"wrong-version N" with "the boards run different protocol versions: flash both", and the portal
shows the pill "BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both" (state field `linkver`). The
A32 counts the same frames (`badVer()`) but prints the count nowhere; its own console `s` shows
only `link SILENT rx 0`.

**Where.** `confirmTick()` in src/a32/main.cpp; the counters in include/link.h; the console `s`
report and the `linkver` field in src/s3/main.cpp.

**Starting point.** If it ever bites: count `badVer` on the A32 too, and do not take the five-minute
path while wrong-version frames are arriving. The A32 would then stay on trial and roll back on the
next reset. This removes the one-board-at-a-time path, so the protocol procedure in chapter 9 would
need another way.

**Class.** DEFECT. Recovery is an update of the S3 built with the A32's version, then of both boards
together. No cable is needed.

### 12.4.2 Without the A32 at boot, the S3 never takes NTP time

**What happens.** The S3 sets its clock from the A32 (which reads the battery clock, the DS3231),
from the console `W` prompt, or from NTP (network time). The NTP path writes the time through to
the DS3231 over the link, and it runs only after the boards have exchanged their handshake. If the
A32 is absent or silent from boot, the S3 never takes the NTP time either, and the clock display
stays blank even with a working network.

**Why it was left.** Accepted: with the A32 down the radio has no sound either, and the clock comes
back with the link.

**Where.** The NTP block of `loop()` in src/s3/main.cpp, gated on `peerHello`.

**Starting point.** Let NTP seat the S3's own software clock (`epochAtSync`, `millisAtSync`,
`timeValid`) without the handshake, and send the write-through to the DS3231 only when the A32
answers.

**Class.** NOTE. It recovers by itself when the A32 answers.

### 12.4.3 `autoConnect` is still in the protocol, unread

**What happens.** Nothing visible. The Bluetooth settings structure (`ProtoBtCfg`) still carries an
`autoConnect` byte. The A32 stores it and nothing reads it: the A32 always calls
`set_auto_reconnect(false)`, so the radio never starts a Bluetooth connection. Its portal row was
removed in v.1.0.2, and old settings files that still carry the key upload cleanly.

**Why it was left.** Removing the byte changes the structure's size. By proto.h's own rule a size
change needs no new `PROTO_VERSION`: until both boards carry it, each side refuses the other's
Bluetooth-settings messages (`MSG_SET_BT`, and the Bluetooth part of `MSG_CFG`) and everything else
keeps working. The real cost is that the A32 resets its stored Bluetooth settings to defaults
(below), and both boards need the update.

**Where.** `ProtoBtCfg` in include/proto.h; `settingsDefaults()` and `settingsLoad()` in
src/a32/main.cpp.

**Starting point.** Remove it the next time `ProtoBtCfg` has to change anyway, so the reset is paid
once. The A32 keeps no settings migration: `settingsLoad()` accepts a stored structure only if
its size matches exactly, so any size change puts that structure back to its defaults (chapter 7).

**Class.** NOTE.

### 12.4.4 `MSG_BT_LOOK` opens the pairing window

**What happens.** Nothing visible. The portal's "Pair a new device" (`bt.pair`) sends a message named
`MSG_BT_LOOK`, and the A32 answers it by opening the pairing window, the state named `BT_LINK`, not
`BT_LOOK`. The behaviour is right; only the names disagree, which misleads anyone reading the code.

**Why it was left.** Renaming a message touches both boards and the protocol header.

**Where.** `MSG_BT_LOOK` in include/proto.h; the `bt.pair` action in src/s3/settings_table.h;
`onMessage()` in src/a32/main.cpp.

**Starting point.** Rename the message (for example `MSG_BT_PAIR`). The value stays the same, so the
protocol does not change and `PROTO_VERSION` does not need a bump.

**Class.** NOTE.

### 12.4.5 The A32's console help omits two keys

**What happens.** On the A32's USB console, the key list printed at boot leaves out `k` (the output
bit mask, a pop-hunt experiment) and `g` (the data-line pad drive). Both keys work.

**Why it was left.** Both are bench keys, reachable only with a USB cable on the A32.

**Where.** The `keys:` line in `setup()` and the key handler in `loop()`, src/a32/main.cpp.

**Starting point.** Add both keys to the line.

**Class.** NOTE.

### 12.4.6 The A32's version reaches no portal field

**What happens.** The A32's firmware version is printed at each handshake on the S3's console
("[PASS] A32 up: ..."), which the portal's Console tab shows, and in the A32's boot report. No field
of the portal's JSON carries it, so the page cannot show it.

**Why it was left.** Not yet addressed.

**Where.** `gLink.peerVersion`, printed in the `MSG_HELLO_ACK` case of `onMessage()` in
src/s3/main.cpp; the state JSON built in the same file.

**Starting point.** Add the A32's version string (and its commit, from the boot report) to the state
JSON and show it in the System tab next to the S3's.

**Class.** NOTE.

### 12.4.7 An A32 update that fails mid-image sends no abort

**What happens.** When the S3 relays an A32 image and a frame fails in the middle, the S3 stops
sending but does not tell the A32. The A32 stays muted until its own 20 s give-up, then restores
its sound and keeps its old firmware. A wrong chip, a dropped connection, a stalled upload and a
missing final answer do send the abort.

**Why it was left.** Judged not a problem.

**Where.** `hOtaA32Upload()` in src/s3/portal.cpp; `a32OtaChunk()` and `a32OtaAbort()` in
src/s3/main.cpp; the give-up in `loop()` of src/a32/main.cpp (`otaGiveUp()`).

**Starting point.** Call `a32OtaAbort()` in `hOtaA32Upload()` the moment `a32OtaChunk()` fails.

**Class.** NOTE. It recovers by itself after 20 s.

### 12.4.8 An audio-board edit just before the A32 goes silent can be lost

**What happens.** Since v.1.0.2 the portal refuses an edit of an audio-board row while the A32 is
silent (409, "the audio board is not answering"). The refusal starts only once 2 s of silence have
cleared the S3's copy (`haveCfg`). An edit made inside those 2 s is accepted, sent to nobody, and
replaced by the A32's own settings when it comes back.

**Why it was left.** Not yet addressed. The window is short and needs the A32 to fail at that
moment.

**Where.** `haveCfg`, cleared in `loop()` of src/s3/main.cpp when `gLink.peerAlive()` turns false;
the audio-board setters in src/s3/settings_table.h.

**Starting point.** Have the A32 acknowledge each settings message, and report the edit as not
applied when no acknowledgement arrives.

**Class.** NOTE. Nothing is corrupted; the edit is simply not there.

### 12.4.9 The A32 saved settings while its image was on trial (fixed in v.1.0.4)

This note is closed; it is kept so that the notes after it keep their numbers. Until v.1.0.4 the
A32 had no trial hold: an A32 update that changed `ProtoAudio` or `ProtoBtCfg` and saved during
its trial, then was rolled back, would have left the previous image running that structure on
its defaults. Since v.1.0.4 (2026-09-26) `settingsFlush()` in src/a32/main.cpp
writes nothing while the image is on trial, and `confirmImage()` writes what was held at
confirmation, logging "settings changed during the trial are now saved". Proven live on
2026-09-26. Chapter 9 (9.2.3) describes it.

### 12.4.10 The A32's save before a commanded restart is not retried

**What happens.** When the S3 asks the A32 to restart, the A32 forces a save first, skipping its
2 s debounce. If that one write fails, the A32 restarts anyway and the last change is lost.

**Why it was left.** Not yet addressed. A failed write has never been seen.

**Where.** The `MSG_REBOOT` case of `onMessage()` and `settingsFlush()` in src/a32/main.cpp.

**Starting point.** Retry the forced save a few times, and send a `MSG_LOG` line before restarting
if it still fails.

**Class.** NOTE.

### 12.4.11 Console changes on the A32 do not refresh the S3's copy

**What happens.** The taper (`t`) and the pot calibration (`p`) set on the A32's USB console are
saved on the A32, but the S3's copy of the audio settings is not refreshed. The next portal edit of
any audio row sends the whole structure from that copy, and the older taper with it. "Re-read the
A32" (`sys.getcfg`) refreshes the copy.

**Why it was left.** Both keys are bench keys, reachable only with a USB cable on the A32.

**Where.** The `t` and `p` cases of the key handler in `loop()` of src/a32/main.cpp; the S3's copy in
src/s3/main.cpp.

**Starting point.** Have the A32 send its settings to the S3 (as it answers `MSG_GET_CFG`) after
any console change.

**Class.** NOTE.

### 12.4.12 A broken-protocol A32 image on trial says "awaiting OTA"

**What happens.** An A32 image that fails its protocol self-test stays up, muted, and announces
"proto self-test FAILED - muted, BT closed, awaiting OTA". Such an image is never confirmed. If it
arrived by an update, it is on trial, so it refuses any new update, and only a reset (which rolls
it back) gets out. The announcement is true only for an image flashed by USB, which is not on
trial.

**Why it was left.** The behaviour (never confirmed, rolled back by the next reset) is accepted.
Only the words are wrong.

**Where.** The `protoBroken` block in `loop()` and `confirmTick()` in src/a32/main.cpp.

**Starting point.** When the image is on trial, announce "reboot the audio board to go back to the
previous firmware" instead.

**Class.** NOTE.

### 12.4.13 The S3's "PROTOCOL MISMATCH" warning cannot fire

**What happens.** Nothing visible. On the handshake the S3 compares the A32's protocol version with
its own and would print "[WARN] PROTOCOL MISMATCH". The framer drops every frame of another version
before it reaches that code, so the line can never print. The wrong-version counter (12.4.1) does
that job.

**Why it was left.** Harmless dead code, kept as a guard.

**Where.** The `MSG_HELLO_ACK` case of `onMessage()` in src/s3/main.cpp.

**Starting point.** Remove it, or keep it and say in the code that it is a guard for a framer that
one day accepts other versions.

**Class.** NOTE.

### 12.4.14 The battery clock's temperature is sent and not used

**What happens.** Nothing visible. Every `MSG_TIME` carries the DS3231's temperature (`tempC4`). The
A32 prints it in its boot report; the S3 never reads it.

**Why it was left.** Not yet addressed.

**Where.** `ProtoTime` in include/proto.h; `Rtc::read()` in src/a32/rtc.h; the `MSG_TIME` case of
`onMessage()` in src/s3/main.cpp.

**Starting point.** Show it in the portal, or drop the field at the next protocol change that has
to happen anyway (12.4.3).

**Class.** NOTE.

### 12.4.15 The battery clock's century is ignored

**What happens.** The A32 reads and writes the DS3231's year as two digits from 2000 and ignores
its century bit. The clock will be wrong from the year 2100.

**Why it was left.** Not yet addressed.

**Where.** `Rtc::read()` and `Rtc::write()` in src/a32/rtc.h.

**Starting point.** Read and write the DS3231's century bit (see its datasheet), or accept the
limit and say so.

**Class.** NOTE.

### 12.4.16 "ROLLED BACK" stays in every boot report

**What happens.** After a rollback, both boards say "an earlier update was ROLLED BACK" in their
image line at every boot, until the next update replaces the invalid image. A reader can take an
old rollback for a new one.

**Why it was left.** Not yet addressed.

**Where.** `imageReport()` in src/s3/main.cpp; `bootReport()` in src/a32/main.cpp; both use
`esp_ota_get_last_invalid_partition()`.

**Starting point.** Remember in flash that a rollback has been reported, and print it at the first
boot after it only; or print the invalid image's version so the reader can tell.

**Class.** NOTE.

### 12.4.17 Link and A32 paths never exercised on the radio

**What happens.** Nothing has gone wrong. These paths are in the code and have never run on the
radio: the A32's five-minute "S3 never spoke" confirmation; the degraded `protoBroken` mode on
either board, and with it the rule that such a build is never confirmed; the wrong-version counter
and pill with a real mismatch (on the radio they have only read 0); the two battery-clock pills in a
real fault (`rtc` 1 or 2).

**Why it was left.** Each one needs a fault or a mismatched build that the radio has not had.

**Where.** `confirmTick()` in src/a32/main.cpp; the self-test in `setup()` of both boards; the
console `s` report, the `linkver` and `rtc` fields in src/s3/main.cpp.

**Starting point.** Test builds sent over the air and rolled back, as the rollback tests were
(chapter 3): one with another `PROTO_VERSION` on the A32 only, one with a self-test forced to fail.
For the clock pills, a test build of the A32 that reports an invalid time, or does not answer
`MSG_GET_TIME`.

**Class.** NOTE. The class of each path if it fails is in chapter 9, section 9.5.

---

## 12.5 Audio

### 12.5.1 An A32 update can click if the fade-out is long

**What happens.** When an A32 update begins, the A32 mutes and then waits a fixed 400 ms before it
starts writing flash. Writing flash stalls the audio task. The wait covers the fade-out, plus up to
one audio block for the mute to take effect, plus draining the output buffers (46.4 ms). That works
for a fade-out up to about 340 ms. The default fade-out is 150 ms, but the portal allows up to
5000 ms. With a fade-out longer than about 340 ms, the flash write starts while sound is still
fading. The output buffers still holding sound are replaced by exact zeros, and a click is heard at the
start of the update.

**Why it was left.** The default is well inside the wait, and an update is a rare event.

**Where.** The `MSG_OTA_BEGIN` case of `onMessage()` in src/a32/main.cpp (`delay(400)`);
`fadeOutMs` in src/s3/settings_table.h.

**Starting point.** Make the wait follow the setting: the fade-out plus about 60 ms. It must stay well
under the 20 s that the S3 waits for the A32's answer to the start of an update. Alternatively, cap `fadeOutMs`
at 340 ms in the settings table.

**Class.** NOTE.

### 12.5.2 The stall click

**What happens.** The audio task is sometimes frozen for 23 to 111 ms, and a click can be heard (a
stall click, told apart from a pop by its step shape, chapter 10). The cause was never measured. The
leading guess is a flash write, which turns the flash cache off and stalls every task that runs
from flash.

**Why it was left.** A deeper output chain would ride through the stall, but it costs RAM the A32
does not have (chapter 10).

**Where.** `audioTask()` in src/a32/audio.cpp; the output buffers set up in `Audio::begin()` in the
same file; the A32's flash writers, `settingsFlush()` and the update path in src/a32/main.cpp.

**Starting point.** Measure first. The stall detector in `audioTask()` already records each stall's
length and time (`stalls`, `stlast`, chapter 10). Record the time of every flash write on the A32
beside it and see whether they line up. If they do, keep flash writes away from playing moments, as
the S3 keeps them away from the moving needle.

**Class.** NOTE. The sound comes back by itself.

### 12.5.3 The audio task is not watched by the watchdog

**What happens.** The A32's task watchdog watches only `loop()`. If the audio task wedged, the loop,
the link and the portal would stay alive with no sound, and nothing would restart the A32 by itself.

**Why it was left.** Not yet addressed. It has never happened.

**Where.** `setup()` in src/a32/main.cpp (`esp_task_wdt_init()`, `enableLoopWDT()`); `audioTask()` in
src/a32/audio.cpp.

**Starting point.** Subscribe the audio task to the watchdog (`esp_task_wdt_add()`) and feed it once
per block. The 15 s timeout is far above the longest stall seen (12.5.2).

**Class.** DEFECT. A portal reboot of the audio board fixes it.

### 12.5.4 The A32's console `s` steals the portal's readings

**What happens.** The A32's console `s` reads the peak meter, the ring's high-water mark and the I2S
latches, and reading them resets them. The state frame sent to the S3 reads the same values, so the
portal misses them for one period after each `s`.

**Why it was left.** `s` is a bench key, reachable only with a USB cable on the A32.

**Where.** The `s` case of the key handler in `loop()` of src/a32/main.cpp; `Audio::getPeaks()` and
`Audio::i2sFaults()` in src/a32/audio.cpp.

**Starting point.** Give the console a read that does not reset (a "peek"), and leave the resetting
read to the state frame.

**Class.** NOTE.

### 12.5.5 Audio paths never run on the radio

**What happens.** Nothing has gone wrong. These have never been run on the radio: the check that
Bluetooth stays invisible in RADIO mode, with a phone (to be repeated after any library update);
`sys.clkpair` (committed, never run); the volume-swing test (A32 volume 47 against 94 with a
tone streaming, to see whether pops follow the signal level); the Bluetooth transmit-power A/B that would prove the 0 dBm
boot ceiling; a drive-3 pop count with Bluetooth idle; a phone negotiating 48 kHz; `connectable`
off; a long session measuring the clock drift between a phone and the local 44.1 kHz (there is no
rate adaptation; the 186 ms ring absorbs only jitter). The channel order is 12.8.1.

**Why it was left.** Each one waits for a listening session with the right phone or set-up.

**Where.** The Bluetooth state machine and `onMessage()` in src/a32/main.cpp; `audioTask()` in
src/a32/audio.cpp.

**Starting point.** Chapter 10 describes each test where it describes the path. The phone check is
the one still pending.

**Class.** NOTE. The class of each path if it fails is in chapter 10, section 10.5.

---

## 12.6 Build and release

### 12.6.1 The shipped portal account is public

**What happens.** The firmware carries a placeholder portal account (`DEFAULT_USER` and
`DEFAULT_PASS` in src/s3/portal.cpp). Its name and password are public by design: the console
prints them when it creates the account, and the sign-in page names them. A blank board creates it,
and so does the console `~`. Until the account's name and password are replaced, the portal refuses
everything but the "Set your account" form (8.2.2, 8.2.6).

**Why it was left.** By design: no real credentials are compiled into the image.

**Where.** `DEFAULT_USER`, `DEFAULT_PASS` and `credsAreDefault()` in src/s3/portal.cpp.

**Starting point.** Builders: sign in once with the placeholder and set your own name and password
before anything else. Do not compile your own credentials into the image; the first-login form is
the place for them.

**Class.** NOTE.

### 12.6.2 The rescue access point is open; the time zone is the author's

**The rescue access point.** Since v.1.0.5 (2026-09-27) the rescue access point
`Ambersong` is an open network, with no WiFi password, by the author's decision. Until v.1.0.4 it
had a passphrase, a literal (`AP_PASS`) in the source that every builder had to change; `AP_PASS` no
longer exists, and `startAp()` in src/s3/net.cpp prints "open, no password" in its place. The
portal sign-in still guards every action; what the open network costs is set out in 8.2.6 and 8.4.

**Values to set for your own build.** Two values are the author's own and should be set by a
builder in another place: the time zone
and NTP server defaults in `loadPrefs()` (src/s3/net.cpp), and the fixed `EST5EDT` rule that the
console `W` prompt uses in `setClockInteractive()` (src/s3/main.cpp), whatever zone the portal holds.
The prompt also leaves that rule in force, so the display shows eastern North American time until
the zone is saved again from the portal or the S3 restarts. This is noted on purpose and not fixed.

**Class.** NOTE.

### 12.6.3 The display-timing bench tool no longer builds

The bench program behind the display-timing test (bring-up Phase G, 11.2) is not published. It
used `S3_LIMIT_LEFT` and `S3_LIMIT_RIGHT`, which were removed from include/pins.h on 2026-09-20, so it
no longer compiles. This is only recorded. Class: NOTE.

### 12.6.4 The version stamp has no time zone

**What happens.** The version string ("v.1.YYYYMMDDTHHMMSS") is stamped at build time from the
build PC's local clock, with no time zone. Two builds made in different zones, or across a
daylight-saving change, cannot be ordered by their stamps alone.

**Why it was left.** Not yet addressed. Every build so far was made on one PC.

**Where.** scripts/version.py (`datetime.datetime.now()`).

**Starting point.** Stamp in UTC and add a `Z`. The string must still fit the 24-byte version field
of the handshake (chapter 3).

**Class.** NOTE.

### 12.6.5 Code comments that are out of date

**What happens.** Nothing on the radio. Some comments in the v.1.0.3 code say something that is no
longer true, or round a measured value loosely. Found 2026-09-26.

- In `settingsDefaults()` in src/a32/main.cpp, the comment on `cfgB.autoConnect` still says the
  value is "stored and shown". Its portal row was removed in v.1.0.2 (12.4.3), so it is stored and
  not shown.
- The comments on the removal of the A32's console key `y` (`sd_out_delay`) date it 2026-09-25, in
  src/a32/audio.cpp, src/a32/audio.h and the key handler of src/a32/main.cpp. It left the code in
  v.1.0.3, on 2026-09-26.
- Three comments give the tuner's travel as "about 2300 counts" (`seatTurn()`'s banner in
  src/s3/needle.cpp; the `calLow` comment and `SMP_MAX_DRIFT`'s comment in src/s3/main.cpp). The
  measured span is 2197 (2026-09-03), about 2200; this Gospel says about 2200.

**Why it was left.** Found after v.1.0.3 was released and confirmed on both boards. Comments do
not change the image's behaviour, so they wait for the next change to the code.

**Where.** The files and functions above.

**Starting point.** Correct these comments in one commit. A comment-only change can be proved by
stripping the comments and comparing with the previous commit, as was done on 2026-09-25.

**Class.** NOTE.

### 12.6.6 Recovery paths never exercised on the radio

**What happens.** Nothing has gone wrong. The rollback itself has been proven on both boards
(chapter 3), but these recovery paths have never run on the radio: an S3 image that runs but never
earns confirmation (portal or network down); a failed protocol self-test at boot, on either board;
a confirmation write that fails; a reset during either board's trial with a setting changed in it; a
hang of the needle's supervisor task, and a hung portal task.

**Why it was left.** Each one needs a fault the radio has not had, or a test build made to fail.

**Where.** The rollback and confirmation code in `setup()` and `loop()` of src/s3/main.cpp and
`confirmTick()` in src/a32/main.cpp; the task watchdog subscriptions in src/s3/needle.cpp and
src/s3/main.cpp.

**Starting point.** The method of the 2026-09-25 tests (chapter 3): a test build made to fail in one
chosen way, never committed, sent over the air with the USB cables unplugged, and watched through
its rollback. One build per path.

**Class.** NOTE. The class of each path if it fails is in chapters 3 and 4, sections 3.5 and 4.5.

---

### 12.6.7 Two build flags nothing reads

**What happens.** Nothing. platformio.ini defines `AMB_S3=1` for the S3 build and `AMB_A32=1` for
the A32 build, and no source file tests either one: each board is selected by its own source folder
(`build_src_filter`), not by these flags.

**Why it was left.** Harmless.

**Where.** `[env:s3]` and `[env:a32]` in platformio.ini.

**Starting point.** Remove them, or use them if shared code ever needs to tell the boards apart.

**Class.** NOTE.

---

## 12.7 S3 core, clock and display

### 12.7.1 The S3 waits about 3.3 s at boot for a USB host

**What happens.** At every boot the S3 waits up to 3 s for its USB console port to open, then
300 ms more, before it prints its banner and starts anything else. In the radio no USB host is
attached, so every boot pays the whole 3.3 s before the display, the needle and the link start.

**Why it was left.** Not yet addressed. It keeps the boot report whole when a cable is attached.

**Where.** The console start at the top of `setup()` in src/s3/main.cpp.

**Starting point.** Shorten the wait, or start the display and the link first and print the banner
when the port opens. The ring buffer of the web console already keeps lines for a late reader
(chapter 8).

**Class.** NOTE.

### 12.7.2 The tuning readout in mode 1 does not check the encoder

**What happens.** With the readout set to mode 1 ("Tuning always"), the Leditron shows the frequency
of the last count from the tuning encoder even when the encoder has stopped answering. The number
looks valid and is stale. Mode 1 is deliberately not gated on the amp and the source (it is the
instrument for calibrating the shaft); the encoder is a separate matter.

**Why it was left.** Not yet addressed.

**Where.** `updateDisplay()` in src/s3/main.cpp; `Needle::tuneFreq10f()` and `Needle::i2cOk()` in
src/s3/needle.cpp.

**Starting point.** When `Needle::i2cOk()` is false, show the placeholder that the clock shows when
it has no valid time.

**Class.** NOTE.

### 12.7.3 The frequency readout shows a leading zero

**What happens.** Below 100 MHz the tuning readout shows a leading zero ("0879" for 87.9 MHz). The
clock follows the `blankLeadZero` setting; the readout does not.

**Why it was left.** Not yet addressed.

**Where.** The `Display::showNumber()` call in `updateDisplay()`, src/s3/main.cpp.

**Starting point.** Pass `true` for the leading-zero blank, or follow `blankLeadZero`.

**Class.** NOTE.

### 12.7.4 The console brightness keys bypass the fade

**What happens.** The console keys `-` and `=` (or `+`) change `brightOn` by 15 and write it straight
to the display. They skip the fade that every other brightness change goes through, and they set
the "radio on" level even while the amp is off. The clock can then sit at the wrong level until the
next amp change, or step back briefly before it eases. The keys also do not go through the settings
table's bounds (`settingBound()`); today their own floor and ceiling (0 and 255) match the row's,
so nothing differs.

**Why it was left.** Both are bench keys. The portal's brightness rows go through the normal path.

**Where.** The `-` and `=` cases of the console handler, `brightnessTarget()` and `brightnessNow()`
in src/s3/main.cpp; `settingBound()` in src/s3/settings_table.h.

**Starting point.** Route both keys through the settings table's setter for `brightOn`, the same
path as the portal, so the bounds and the fade apply.

**Class.** NOTE.

---

## 12.8 Ideas for improvement

These are not faults. They are possible improvements considered after v.1, written down with the
same starting points.

### 12.8.1 Check, and let the portal swap, left and right

**What happens.** The A32 writes the right speaker into the first I2S slot and the left into the
second, the reverse of the usual order, because that is what gave correct stereo on this machine.
The author knows that the amplifier sends each channel to the right speaker. Whether the A32 and the
DAC deliver them the right way round has never been tested. The channel order is a setting of this
machine that needs adjusting for other devices, and a portal switch to swap it on the fly would make
that easy.

**Pending.** A live test: play a left/right test track over Bluetooth and listen to which speaker
speaks. That test settles the Bluetooth path only. The radio path copies the converter's two slots
straight across, and with "mono sum" on (the default) both slots carry the same sound, so the
radio's order matters only with mono sum off.

**Where.** `audioTask()` in src/a32/audio.cpp: the Bluetooth path writes `.r` into slot 0 and `.l`
into slot 1, and the radio path copies the input slots in order. The channel format is set in
`Audio::begin()`. The order is described at the top of src/a32/audio.h.

**Starting point.** Add a swap flag to the audio settings (`ProtoAudio` in include/proto.h), and
swap each frame's two words in `audioTask()` after the volume, balance and meter pass and before
`i2s_write()`. Balance and the level meters then keep meaning the right and left speakers. Adding a field changes `ProtoAudio`'s size. By the protocol rule (§9.8, include/proto.h
"WHEN PROTO_VERSION CHANGES") a size change needs no new `PROTO_VERSION`: `ProtoFramer::as()`
refuses a payload of the wrong length, so a half-updated pair loses only the messages that carry
`ProtoAudio` and the link stays up. Until both boards carry the new structure the A32 refuses every
`MSG_SET_AUDIO` (volume, balance, fades and the rest do not reach it), and the S3 refuses the A32's
`MSG_CFG` (its copy of the A32's settings stays unknown, so the portal refuses audio-board edits).
So flash both boards: the A32 first, through the S3 (updates do not use `ProtoAudio`), then the S3.
The A32's stored audio settings do go back to their defaults once: `settingsLoad()` in
src/a32/main.cpp accepts a stored structure only if its size matches exactly (12.4.3). Download a
settings file first and upload it afterwards. Add the row to src/s3/settings_table.h and it appears
in the portal.

**Class.** Not a fault. For another builder it is an adjustment to check (§10).

### 12.8.2 An equaliser

**What happens.** There is no equaliser. One was considered together with the swap (12.8.1).

**Where.** It would go in the same pass of `audioTask()` in src/a32/audio.cpp that applies volume and
balance. It would apply to the radio and to Bluetooth only. AUX never passes through the A32
(Bible §22, §30).

**Starting point.** A few biquad filters per channel, with fixed-point coefficients worked out on the
S3 (or when the setting changes) and sent as settings. The A32 is short of RAM and its audio task
runs under a tight deadline (chapter 10), so measure the task's time per block and the free heap
before and after. The pop hunt showed how much this chain cares about timing.

**Class.** Not a fault.

