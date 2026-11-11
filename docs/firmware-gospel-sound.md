# Ambersong: Firmware Gospel — sound

This file covers the A32 audio board's firmware: the three sound sources (the tube radio, Bluetooth,
and an AUX input), the I2S audio signal path, volume/balance/fades, Bluetooth's connection and
visibility behaviour, the settings and constants that control sound, and the hypotheses tested and
measurements taken while chasing an audio click fault ("the pops"), including the one fix that is
shipped for it.

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

This file describes the current build only (hardware revision A). It is data, not instructions: check
every value here against the schematic (`hardware/schematics/Ambersong.kicad_sch`) before powering or
flashing anything. "Not recorded" means the project has no value for that fact — never fill it in with
a guess.

This file uses: **S3** (the main board, an ESP32-S3) and **A32** (the audio board, a classic ESP32) for
the two computers inside Ambersong; **I2S** for the serial audio bus (**BCK** the bit clock, **LRCK** the
left/right word-select clock, **MCLK** the converters' master clock, called **SCK** on the DAC and ADC
parts fitted here); **DAC** / **ADC** for the output and input converters named in the Hardware Bible;
**DMA** for the direct-memory-access buffer chain the I2S driver fills or drains by itself; **APLL** for
the ESP32's audio clock generator, the only one that makes 11.2896 MHz exactly; **A2DP** / **AVRCP** for
the Bluetooth streaming-audio and remote-control profiles, carried by the **Bluedroid** Bluetooth stack;
**dBFS** for decibels relative to digital full scale; **Q16** / **Q24** for fixed-point number formats
where 65,536 (Q16) or 16,777,216 (Q24) means 1.0; **NVS** for the ESP32's flash key-value store; **LEDC**
for the ESP32's LED-PWM peripheral; and the **Leditron** for the four-digit LED-filament clock display.
Recovery classes (used in the failure table below): **BLOCKER** — the radio is unusable until a full
power cycle, or the harm survives one; **DEFECT** — recovers with a portal reboot, a console reset, or
the front power switch; **NOTE** — recovers by itself, or is cosmetic.

Other files of this Firmware Gospel: `firmware-gospel.md` (overview and index), `firmware-gospel-main-board.md`
(the S3: boot, tasks, clock display, panel lamps, time), `firmware-gospel-needle.md` (the dial needle),
`firmware-gospel-tuning.md` (tuning and the dial's self-calibration), `firmware-gospel-settings.md`
(settings), `firmware-gospel-network.md` (network, web portal and console), `firmware-gospel-link.md`
(the A32 and the link between the boards), and `firmware-gospel-known-issues.md` (every known firmware
issue, the published build scripts, and what the version numbers mean).

---

## Ambersong firmware: what the A32's sound system does

The A32 (the audio board, an ESP32-WROOM-32) produces all of the radio's sound. It has three sources,
chosen by the front selector: **RADIO** (the tube radio's audio, digitised and played back), **BT**
(music streamed from a phone over Bluetooth) and **AUX** (an analogue input that never passes through
the A32, so the A32 stays silent on it). The A32 runs on a separate chip from the S3 because it needs
Bluetooth *Classic*, which the ESP32-S3 does not have.

Turning the selector fades the old source out over 150 ms and brings the new one in over 1 s. The front
volume knob follows a gamma curve chosen to suit the ear. Radio and Bluetooth are matched to the same
loudness at any given knob position. Bluetooth is invisible to other devices unless the selector is on
BT and the amplifier is on; a phone that paired before can reconnect by itself, a new phone needs the
pair button (which makes the radio visible for 90 s), and the radio never initiates a Bluetooth
connection. A blue lamp on the front shows the Bluetooth state. When the amplifier is off, or the S3
stops responding, the A32 "sleeps": it plays nothing, the lamp goes dark, and Bluetooth closes.

Driving the A32's I2S clock pads at a stronger setting ("clock pad drive 3" instead of the power-on
default of 2) cuts one class of loud audio click ("a pop") by about 150 times under the conditions it
was measured in; see "Ambersong firmware: the pop hunt — hypotheses, tests and measurements" below. The
root cause of the pops is not known.

---

## Ambersong firmware: the audio signal path

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

The amplifier adds its inputs together rather than choosing between them, so on AUX the A32's only job
is to send digital zeros to the DAC. AUX reaches the amplifier without passing through the A32.

**I2S is installed once, at boot, and never touched again.** `Audio::begin()` in `src/a32/audio.cpp`
installs the I2S driver once; it is never reinstalled, reconfigured, stopped or re-pinned. Changing
source only changes where the next block of samples comes from — ordinary code, no driver calls. The
radio path needs a full-duplex, 32-bit driver with an MCLK for the ADC; the Bluetooth library, left to
itself, wants a transmit-only, 16-bit driver with no MCLK. Reinstalling the driver at every source
change while the Bluetooth stack still held a reference to it was the kind of fault that crashes the
chip in ways that are hard to trace, so the firmware installs the superset configuration once and keeps
it.

The configuration, as `Audio::begin()` writes it:

| field | value | why |
|---|---|---|
| port | `I2S_NUM_0` | |
| mode | master, TX and RX | The ESP32 makes the clocks; both converters follow them as I2S slaves. |
| sample rate | 44,100 Hz | Fixed. There is no resampler. |
| bits per sample | 32 | The ADC sends 24-bit samples in standard I2S format. They arrive most-significant-bit first at the top of a 32-bit slot, so the raw word is already at full scale. |
| channel format | `I2S_CHANNEL_FMT_RIGHT_LEFT` | |
| frame format | `I2S_COMM_FORMAT_STAND_I2S` | Standard (Philips) I2S: data starts one bit after the `LRCK` edge. |
| interrupt | level 1 | |
| DMA buffers | 8 buffers of 256 frames | 46.4 ms of audio per direction. See the heap budget below. |
| `use_apll` | true | See below. |
| `tx_desc_auto_clear` | true | If the CPU falls behind, the driver sends zeros instead of replaying old audio. |
| `fixed_mclk` | 44,100 × 256 = 11.2896 MHz | `BCK` is 64 × 44,100 = 2.8224 MHz. |

`Audio::begin()` then sets the pins, sets the clock-pad drive to 3 (see the pop hunt below) and starts
the audio task. Pins: `MCLK` on `GPIO0`, `BCK` on `GPIO18`, `LRCK` on `GPIO17`, data to the DAC on
`GPIO4`, data from the ADC on `GPIO19`.

**Why the APLL stays on.** `MCLK` on `GPIO0` feeds both converters' system clock input (`SCK` on the
parts fitted here); each DAC switches off its own internal clock generator when given an external
`SCK`. The APLL is the only ESP32 clock that makes 11.2896 MHz exactly, and `BCK`/`LRCK` are divided
from that same clock, so the ratio `SCK`:`LRCK` is exactly 256 by construction. Without the APLL, the
divider runs from a 160 MHz clock that cannot make 44,100 Hz exactly — a worse clock for both parts.
`GPIO0` must never be detached from the I2S peripheral: that would stop the clock of both converters at
once. (The classic ESP32 can only put the I2S master clock on `GPIO0`, 1 or 3; 1 and 3 are the USB
console.)

**If installation fails,** `Audio::begin()` prints a `[FAIL]` line on the A32's USB console and returns
without starting the audio task; the radio is then silent, and only the USB console says why.

**Channel order.** The firmware puts the **right** channel in I2S slot 0 and the **left** in slot 1,
the reverse of the usual convention: every frame written is `{right, left}`. This gave correct stereo
*on this machine*; it is an adjustment for this particular build, not a property of the parts, and must
be checked on another build. The amplifier-to-speaker wiring is known correct; the A32-and-DAC side has
never been tested on its own with a left/right test track over Bluetooth. There is no portal control to
swap the channels (a possible improvement — see `firmware-gospel-known-issues.md`). Until then, swapping
them means editing `audioTask()` in `src/a32/audio.cpp`, where the Bluetooth path, the balance and the
meters all treat slot 0 as the right channel.

**Three guards keep the Bluetooth (A2DP) library off the I2S bus.** The library normally owns the I2S
driver; here it must not, and three guards in `setup()` in `src/a32/main.cpp` make that structural:

1. `a2dp.set_stream_reader(a2dpStream, false)` hands decoded samples to `a2dpStream()` and tells the
   library not to write I2S itself.
2. `a2dp.set_output(nullOut)`, where `NullA2dpOutput` is an output class whose every method is empty.
   Guard 1 alone does not stop everything: the library calls the output's `set_sample_rate()`
   regardless, and its default output then calls `i2s_set_clk()`, reconfiguring the driver to 16-bit
   transmit-only the moment a phone starts streaming (full-scale garbled Bluetooth audio ignoring the
   volume, swapped channels, and the radio dead afterwards). An empty output object has no way to reach
   I2S at all.
3. `a2dp.set_volume_control(&noVol)`, the library's no-op `A2DPNoVolumeControl`. The library's AVRCP
   volume control otherwise scales the sample buffer in place one line before the stream reader runs,
   bypassing the stream-reader flag; at a phone slider of 1/127 that is roughly a divide by 2048 with
   truncation. With the no-op control the library cannot touch the samples. The phone's AVRCP volume is
   still read as information (`a2dp.get_volume()`, published as `avrcpVolume`); the firmware's own curve
   is the only volume law in the machine.

The firmware uses the plain `BluetoothA2DPSink` class (through the subclass `PolicyA2dpSink`, see
"Bluetooth connection and visibility" below), not the "queued" variant, so the stream callback runs
directly on the Bluetooth task and no extra I2S task is started by the library.

---

## Ambersong firmware: tasks, cores and buffers for sound

| who | core | priority | what it does |
|---|---|---|---|
| audio task, `audioTask()` in `audio.cpp` | 1 | 6 | Reads or fetches one block, processes it, writes it to I2S. Stack 4096 bytes. |
| Bluetooth task (Bluedroid) | 0 | the stack's own | Decodes SBC and calls `a2dpStream()` → `Audio::pushBt()`. |
| Arduino `loop()` in `main.cpp` | 1 | 1 (Arduino default) | The link to the S3, sleep/wake, source and volume polling every 50 ms, the Bluetooth state machine, the lamp, the 250 ms state message to the S3, the USB console. |

Core 0 belongs to the Bluetooth stack; sharing a core between SBC decoding and a real-time audio loop
invites dropouts, so the audio task sits on core 1.

The audio task never calls `vTaskDelay` and must not: pacing is free. On RADIO, `i2s_read(...,
portMAX_DELAY)` waits until a full block has been captured; on BT and AUX, `i2s_write(...,
portMAX_DELAY)` waits when the DMA chain is full. The audio task also never prints — serial output from
a priority-6 real-time task causes the dropouts it would be trying to report; anything worth saying is
announced later from `loop()`.

**Buffers.** `BLOCK` = 256 frames per pass (about 5.8 ms). `inBuf`/`outBuf`: 256 frames × 2 slots × 4
bytes = 2 KB each, static; `btBuf`: 1 KB. The **Bluetooth ring**: `RING_FRAMES` = 8192 frames of two
16-bit samples, 32 KB, static, absorbing about 186 ms of jitter (Bluetooth delivers audio in bursts; the
radio path is paced by the ADC and needs no ring). The ring has one writer (the Bluetooth task, in
`pushBt()`) and one reader (the audio task, in `popBt()`); it needs no lock. `pushBt()` publishes the
new head with a release store, `popBt()` reads the head with an acquire load and publishes the new
tail with a release store — correct on an in-order CPU with the ring in uncached RAM even without the
barriers, but they are kept anyway. `pushBt()` must never block the Bluetooth task, so a full ring
drops the newest samples instead of waiting.

---

## Ambersong firmware: one block of audio, start to finish

Each pass of `audioTask()` does four things.

**1. Source-change handshake.** If a new source is pending and `muteOnChange` is on, the fade
envelope's target goes to 0; when the envelope reaches exactly 0 the task switches to the new source,
then the envelope rises again (fade out, switch in silence, fade in). With `muteOnChange` off the task
switches at once, with no fade and an audible thump. With no change pending, the target is 0 if muted
and 1.0 otherwise.

**2. Get the samples.**

- **RADIO.** `i2s_read` one block. With `monoSum` on (the default), each frame becomes
  `m = (slot0 + slot1) / 2`, sent to both slots — both ADC channels carry the same mono radio signal,
  so the sum gains about 3 dB of signal-to-noise (the signal adds, the converter noise does not). The
  result is multiplied by the radio gain (`gainRadioQ16`) with saturation; a saturated sample sets the
  `clip` flag. With `monoSum` off the two slots are kept apart.
- **BT.** `popBt()` takes 256 frames from the ring; missing frames are filled with zeros. Each 16-bit
  sample is shifted left 16 bits to full 32-bit scale, then multiplied by the Bluetooth gain
  (`gainBtQ16`). The **underrun** counter counts only a *partial* fill (audio was flowing and ran out
  mid-block, so the zero-fill made a click) — an empty ring (nothing streaming) is not counted.
- **AUX.** The block is set to zero.

**3. Emit** (the same code runs for every source).

- **Volume law.** `volQ16 = (volume / 255)^gamma × 65536`, where gamma is `taperX10 / 10`, clamped to
  1.0–4.0. `powf()` runs only when the volume or gamma changes, once per block, never per sample. The
  new gain takes effect as a step at a block boundary; it is not ramped (harmless for a hand on a
  knob). A "volume watch" counts these steps (`gainSteps`) and records the smallest and largest gain
  applied.
- **Balance.** `balance` runs from −100 to +100; negative moves the sound to the left. The other side
  is scaled by `65536 − |balance| × 655`; at ±100 the far side is about −65 dB, not silent.
- **Envelope.** A Q24 number, stepped **every sample** and **squared** before use (a linear amplitude
  ramp does not sound like a fade — it rushes up and then sits there). Q24 is needed because Q16
  cannot express a one-second fade accurately (the step rounds to 1, giving 1.49 s instead). Each
  sample becomes `sample × envelope² × volume × balance`.
- **Meters.** The **peak** meter reads the very end of the chain, so it moves with the knob; it is
  read and reset by the 250 ms state message, in tenths of a dBFS, using 64-bit arithmetic (an earlier
  32-bit version could never register the loudest possible sample, because negating the most negative
  32-bit value gives itself back, so it under-read exactly during a full-scale event). The **RMS**
  meter is measured after the source gain and before the volume, so the knob does not move it — this
  is what let the radio and Bluetooth gains be matched once for every volume setting; peaks cannot do
  that job, because FM stations are limited hard and music is not.
- **Diagnostics**, all off by default, applied in this order: the constant-DC word (replaces every
  sample), the low-bit output mask, the zero-data floor, the sign-extended tail. Each is described in
  the pop hunt below and in the settings/constants reference.
- **Zero-data witness.** A copy of the DAC's own zero-data detector: 1024 frames in a row whose top 24
  bits are zero. The DAC fitted here mutes its output by itself after 1024 frames of zero data; the
  witness counts each time the DAC would enter and leave that mute, on what is actually about to go
  out. It treats a word as zero when its top 24 bits are zero (the DAC resolves 24 of the 32 bits; the
  datasheet does not say whether it keeps the bottom 8).
- **I2S fault latches.** The peripheral's raw interrupt bits that the driver does not service are
  OR-ed into `i2sSticky`, which is never cleared.
- **Stall detector.** If more than 23.22 ms (one zero-data window) passed since the previous write, the
  task counts a stall and records its length and time.

**4. Write.** `i2s_write(outBuf, portMAX_DELAY)`.

---

## Ambersong firmware: volume, balance, fades and mute

**Where the volume comes from.** The front volume knob is a position sensor, not an audio part.
`updateVolumePot()` in `main.cpp` reads it every 50 ms: two throw-away conversions, then the median of
five. `potRotation()` maps the raw reading through a three-point calibration (minimum, centre, maximum);
each half of the travel is its own straight line, and the centre reading maps to 128 by construction.
The calibration exists because the knob's mechanical centre read 215 of 255 with only the two ends
calibrated. A **deadband** of 24 ADC counts (about 0.6 % of travel) stops the volume trembling at rest.
The knob is read on `GPIO35` over its full 0–3.3 V swing, with the ADC at its widest input range
(11 dB attenuation).

Because the knob is a sensor:

- **The first reading after boot always applies** — the knob's position *is* the stored state.
- **The knob does not write volume to NVS** — nothing needs storing.
- **The portal can also set the volume**, and that value holds until the knob moves by more than the
  deadband. The portal action `sys.pot 0` makes the A32 ignore the knob altogether; it is an experiment
  and is not kept across a reboot.

If the stored calibration is implausible (maximum not at least 64 counts above minimum), `potRotation()`
falls back to the compiled defaults 60 and 3990, with no centre point.

**Mute is a function of state.** `applyMute()` sets `Audio::setMute(protoBroken || !awake ||
cfgA.muted)`; every place that can change any of those three calls it. Mute is computed, never toggled,
because toggling it independently from several places let the sound come back when it should not (for
example, turning the source knob while asleep because the S3 had died). `protoBroken` is included so
that a build whose protocol self-test failed stays up, muted and invisible to phones, so that it can be
replaced over the air; such a build is also never confirmed (see `firmware-gospel-link.md`). The one
deliberate exception is the start of a firmware update, which forces the mute directly.

**Mute is never stored** — `settingsLoad()` forces `cfgA.muted = 0` on every boot, because a stored mute
once booted the set silent with nothing on the front of the radio to say why.

**Mute is always software.** Muting means writing zeros. No A32 pin reaches the DAC's hardware mute
input (`XSMT`); the DAC module holds it un-muted by itself. `GPIO16` is unused.

**Fades.** `Audio::setFades()` turns `fadeInMs` and `fadeOutMs` into per-sample Q24 steps; a value of
0 ms is treated as 1 ms, so a source change can never wait forever for a silence that never comes.
Defaults: 1000 ms in, 150 ms out — leaving a source is a deliberate act and should be quick, arriving at
a new one should be gentle.

**Gains.** The compiled defaults are **+8.0 dB for the radio and −7.0 dB for Bluetooth**, measured with
the RMS meter: at 0 dB the radio read RMS −29.9 dBFS with peaks to −14.0 dBFS; +8 dB puts its peaks near
−6 dBFS and its RMS near −22 dBFS. (An earlier +15 dB clipped the gain stage at every volume.) For
Bluetooth, a −14 dBFS RMS pink-noise reference played from a PC at full volume arrived at about
−15 dBFS; −7 dB lands it within about 1 dB of the radio, so changing source does not change loudness.
The A32's gain clamp is −60 to +30 dB. These radio and Bluetooth gain values suit this tube radio's
output with its own volume control at about one eighth of its travel; a different tube-set level needs a
different radio gain.

---

## Ambersong firmware: switching between sources

`updateSource()` runs every 50 ms. `readSource()` reads the selector on `GPIO36`: three throw-away
conversions first (`GPIO36` reads high for a while after another ADC channel has been sampled, about 78
counts of residue), then the median of five. A raw value below 424 is AUX, 2471 or above is RADIO,
anything between is BT (`A32_MODE_THRESH_AUX_BT` and `A32_MODE_THRESH_BT_RADIO` in `pins.h`).

On a change, in order: (1) if the old source was BT, `gracefulDisconnect(0xFF)` pauses the phone and
then drops it; (2) `Audio::setSource(s)` lets the audio task ramp across; (3) `applyMute()`,
`updateBtState()`, `applyScanMode()` run. The fade exists because a change of source with no ramp is an
audible thump through a valve amplifier. The selector is an ON-OFF-ON switch whose centre position, off,
reads 0 V through 22 kΩ, which is AUX, so a turn from RADIO to BT (or back) can pass through AUX on the
way.

At boot, `setup()` reads the selector once and sets the source before the first `loop()` pass. The audio
task starts on AUX, at volume 0, muted, until `settingsApply()` and the first knob reading run.

---

## Ambersong firmware: awake and asleep on the A32

`serviceWake()` in `main.cpp` runs at the top of every `loop()` pass and acts only on a change:

```
awake = ampOn && peerAlive()
```

`ampOn` is what the S3 last said about the amplifier; `peerAlive()` is true when a frame from the S3
arrived within the last 2000 ms (`PROTO_SILENCE_MS`).

**Falling asleep.** If the reason is S3 silence, the A32 **forgets `ampOn`**, so waking needs a fresh
word from the S3 (otherwise an amplifier switched off while the S3 was rebooting would wake the A32
unmuted into an amplifier that was off). Then: `gracefulDisconnect(BT_OFF)`, `applyMute()`, lamp to OFF,
`applyScanMode()`.

**Waking.** Cancel any disconnect that the sleep edge armed but has not fired yet (otherwise the phone
is dropped 250 ms after waking and the lamp goes dark while awake), clear the stray-phone timer, then
`applyMute()`, `updateBtState()`, `applyScanMode()`.

**The I2S clocks keep running while asleep.** Asleep is a mute, not a stop; nothing calls `i2s_stop`.
What stays alive while asleep: the link to the S3, the clock chip, and the update receiver.

The design intent: when the amplifier is detected as powered down there is no reason for the A32 to do
anything but listen for the S3; and if the S3 is unreachable, that must be obvious — the radio then does
not work at all (no audio, no clock, no lights). The alternative, "assume the amp is on so a dead helper
board never silences the radio," was not chosen. Consequences accepted: an S3 reboot or a long S3 update
silences the radio mid-listen, and there is a short dead moment at every power-up until the S3 first
reports. The rule proved itself the first time one direction of the inter-board link failed (a connector
fault, since repaired): the A32 never learned the amplifier was on, so it slept and the whole front of
the radio showed something was wrong — firmware without the rule would instead have played on through a
broken link.

---

## Ambersong firmware: Bluetooth connection and visibility

**The policy**, stated in the header of `src/a32/main.cpp`: a phone that wanders into range must not
latch on its own; a phone that has connected before may reconnect on demand, from the phone; the pair
button is for devices that have not connected before. This gives exactly one arrangement:

| situation | connectable | discoverable |
|---|---|---|
| not in BT mode, or asleep | no | no |
| in BT mode, idle | yes | no |
| pairing window open (after the button) | yes | yes, until the window closes |
| a phone is connected | no | no |

All of it depends on being awake. **The radio never calls a phone**:
`a2dp.set_auto_reconnect(false)`.

**`applyScanMode(force)`** is the only function that writes the scan mode:

```
connectable  = cfgB.connectable && awake && source == BT && !protoBroken && !btConnected
discoverable = connectable && btState == BT_LINK
```

It remembers what it last wrote and returns early unless the result changed or `force` is set; the
`!btConnected` term stops a second phone calling in behind the first.

**`PolicyA2dpSink` and `scanDirty`.** The pinned ESP32-A2DP library writes the scan mode by itself, on
the Bluetooth task: `set_scan_mode_connectable(true)` on every disconnect and once more when the stack
comes up (whose "true" means connectable *and* generally discoverable), and
`set_scan_mode_connectable(false)` on every connect. Because `applyScanMode()` cached its last answer
and none of the library's own writes changed the firmware's *policy*, left unchecked this made the
radio sit connectable and discoverable in RADIO, in AUX and asleep after every hang-up, source change
or sleep. The fix is a subclass, `PolicyA2dpSink`, that overrides the library's virtual
`set_scan_mode_connectable(bool)` so it only sets `scanDirty = true`; `loop()` then calls
`applyScanMode(true)` after `updateBtState()`, re-deriving the mode from the firmware's own state and
writing it past the cache. The flag is cleared before the write, so a change landing during the write is
caught on the next pass; `scanDirty` starts true, so the boot write always happens. `onConnState()` sets
it again after updating `btConnected`. The library also has `set_discoverability()`, which writes the
mode directly; the firmware never calls it.

**Why the Bluetooth library is pinned.** `platformio.ini` pins ESP32-A2DP to commit
`3245602afc494f9e62160a0cfb2af864af45a37f` (library release 1.8.11). `PolicyA2dpSink` depends on two
internal facts of that exact version: that `set_scan_mode_connectable` is virtual, and that every
scan-mode write in the library goes through it. A newer library version could break either silently,
and the radio would become visible again with no error anywhere. **Any bump of the library must
re-test invisibility**: in RADIO, in AUX, asleep, after a hang-up, and after a source change — check
with a phone that the radio does not appear in its list of new devices. The build platform is pinned
too, at `espressif32@7.0.1` (Arduino core 2.0.17, IDF 4.4.7), because platform 3.x retires the legacy
I2S driver this firmware depends on, and the Bluetooth library would then route through a different
audio layer.

**States.** `btState` is published to the S3 and shown by the lamp; `updateBtState()` in `main.cpp` runs
every loop pass.

| state | lamp | entered when | left when |
|---|---|---|---|
| `BT_OFF` | dark | asleep, or the source is not BT | awake in BT mode → `BT_LOOK` |
| `BT_LOOK` | fast breath | entering BT mode, or a connection ended | after 20 s (compiled) → `BT_STDBY` |
| `BT_STDBY` | slow breath | from `BT_LOOK`, or when the pairing window closes | held; never times out |
| `BT_LINK` | regular blink | pair button, portal `bt.pair`, console `b` | after `lookTimeoutS` (default 90 s) → `BT_STDBY` |
| `BT_FOUND` | double flash | a phone connected | moves itself to `BT_CON` after 450 ms |
| `BT_CON` | steady bright | from `BT_FOUND` | disconnect → `BT_LOOK` |
| `BT_ON` | steady dim | portal hang-up (`bt.disconnect`) | held, like `BT_STDBY` |

`BT_LOOK`, `BT_STDBY` and `BT_ON` share one scan mode: connectable, not discoverable. Only `BT_LINK` is
discoverable. "Looking" is only a message from the lamp; the radio never pages a phone.

**Graceful disconnect.** The rule is: disconnect entirely, but pause first. `gracefulDisconnect(next)`
sends an AVRCP pause (if `pauseOnLeave` is on) and arms a disconnect 250 ms later;
`serviceDisconnect()`, called from `loop()`, fires it without blocking and moves to the requested next
state. The pause matters because tearing down A2DP under a phone that thinks it is playing leaves its
media app "playing into nowhere" — a real dead end seen on the hardware.

**`btDropping`.** `a2dp.disconnect()` only posts a request; `btConnected` goes false later, on the
Bluetooth task. In between, without a flag, `updateBtState()` could see a phone still connected and
promote the requested state straight back to `BT_FOUND` — so the pair button pressed with a phone
connected would end in `BT_LOOK` instead of opening the pairing window. `serviceDisconnect()` now sets
`btDropping` **before** the request (the callback that clears it runs on another task and could
otherwise land first); while it is set, the connected branch holds the requested state.
`onConnState()` clears it when the disconnect lands. If the disconnect has not landed after
`BT_DROP_TIMEOUT_MS` = 3 s, `updateBtState()` clears the flag and logs "BT: the disconnect did not land
- still connected" to the USB console and to the S3. A stale flag is swept at the top of
`updateBtState()`.

**Stray phones.** If a phone is connected while asleep or out of BT mode, `updateBtState()` sends it
away through the same pause-then-drop path, at most once every 3 s (`strayDropAt`) — the rule is
"nothing connects at night," not merely "nothing can."

**The pair button** is on `GPIO25`, active low, with the pin's internal pull-up; no capacitor is
fitted, so the debounce is the firmware's: `loop()` counts a press on **release**, only after it was
held for more than 30 ms. A press, only in BT mode, calls `gracefulDisconnect(BT_LINK)`; it drops
whoever is connected first, otherwise a second phone could pair behind the first and it would no longer
be obvious which one is in charge.

**Other controls.** Portal guest actions `bt.play`, `bt.pause`, `bt.next`, `bt.prev`, `bt.disconnect`
and `bt.pair`; admin action `bt.forget`, which removes every stored bond (`forgetPairings()`). If a
phone negotiates a sample rate other than 44,100 Hz, `onSampleRate()` only warns on the USB console;
there is no resampler, so such a stream plays at the wrong pitch.

**Bluetooth transmit power.** `applyBtTxPower()` runs once, after `a2dp.start()` and before the first
scan mode lets the radio transmit, as the IDF requires. It sets the controller's power range to a floor
of 0 dBm and a ceiling of `btTxLevel`, which defaults to 0 dBm (the stock range is 0 to +3 dBm); the
floor stays at 0 dBm unless the ceiling is set lower, because a low floor lets the controller run near
its margin and raises retransmissions. The portal action `sys.bttx 0..5` (−12 to +3 dBm, down only)
changes it; `sys.bttx 5` restores stock. It is not stored, so every boot is at 0 dBm. The controller's
own readback is taken once, when the value is set, never from the 4 Hz state message. This was tried as
a pop mitigation; **its effect on the pops was never measured**, and it was kept as it is.

---

## Ambersong firmware: the Bluetooth status lamp

`src/a32/btled.cpp` drives the lamp on LEDC channel 0 at 2 kHz, 8-bit.

| state | pattern |
|---|---|
| OFF | off, and actively driven off |
| ON | steady, 64/255 |
| LOOK | raised-cosine breath 32 → 255 → 32 over 1200 ms |
| STDBY | raised-cosine breath 8 → 120 → 8 over 4000 ms |
| FOUND | 150 ms on, 150 off, 150 on, then CON by itself |
| CON | steady, 180/255 |
| LINK | 300 ms on, 300 ms off, repeating |

The lamp is **active low**, so a brightness B is written as 255 − B; "off" is driven high rather than
left as an input, because an input pin's leakage would light the LED faintly in a dark room. The lamp is
on `GPIO33` with its anode on the supply, so the pin sinks its current. Breathing uses a raised cosine
because a linear ramp reads to the eye as a fast rise and a long dim tail. Each state has its own scale
0–255 (`ledBrightness[7]`, default 255 for all); OFF ignores its scale. `BtLed::update()` runs every
loop pass; `BT_FOUND` moves itself to `BT_CON` after 450 ms, and `updateBtState()` reads `BtLed::state()`
back to publish that change, so the lamp and the published state share one clock.

**Full drive.** At the default scale of 255 the lamp is driven **fully on** (the pin held low for the
whole PWM period) for 300 ms of every 600 ms in LINK, at the top of every LOOK breath, and for both
150 ms flashes of FOUND. That is the specified behaviour, kept as it is. A lower per-state scale in the
portal lowers it.

---

## Ambersong firmware: where sound and Bluetooth settings are stored

The A32 owns its audio and Bluetooth settings: `cfgA` (a `ProtoAudio`), `cfgB` (a `ProtoBtCfg`) and the
three knob-calibration points, all in the A32's own NVS namespace `amb`. The S3 keeps a mirror, fetched
with `MSG_GET_CFG` at every handshake. Writes are **debounced by 2 s** after the last change
(`settingsFlush()`), because a portal slider dragged across its range would otherwise write flash at
every pixel, and flash traffic disturbs the S3's display. A stored blob of a different size is
rejected and the defaults apply. `MSG_REBOOT` saves first, skipping the 2 s debounce, so a change made
in the last two seconds is not lost to the restart.

The knob calibration lives on the A32, not the S3, because the A32 takes the readings: the reading and
the thing being calibrated must not sit on opposite sides of a link.

**Nothing is written while the A32's image is on trial.** An image that arrived over the air runs "on
trial" until it confirms itself (see `firmware-gospel-link.md`). Until then, `settingsFlush()` returns
without writing, and changes wait in RAM; the reason is that an update that changed the size of
`ProtoAudio` or `ProtoBtCfg` would otherwise save its new layout during the trial, and if the update was
then rolled back, the old image would find a blob of the wrong size and run on defaults. When the image
is confirmed, `confirmImage()` writes the held changes at once and logs "settings changed during the
trial are now saved" to the S3, so the hold is visible from the portal. A restart during the trial is a
rollback, so a change made during the trial and followed by a restart is not kept, by design. An image
flashed by USB is never on trial and saves normally.

**Only a write that reached flash counts as saved.** `settingsFlush()` checks the result of every NVS
write; if one fails, the change stays unsaved, the A32 logs "settings NOT saved - the flash write
failed; retrying", and it tries again after the next 2 s.

**The S3's copy is dropped when the A32 goes silent.** Until the next handshake fetches the settings
again, a portal edit of any audio-board row is refused with HTTP 409, "the audio board is not answering
- its settings cannot be changed now", and a settings download writes `n/a` for those rows.

---

## Ambersong firmware: firmware updates and the audio path

This is only the audio side of the A32's over-the-air update; the rest is in `firmware-gospel-link.md`.

- **Refused while on trial.** If the running image is still on trial (rollback armed), an update start
  is refused before anything is muted, with "still on trial - retry in 1 min".
- **Mute, then wait 400 ms.** The start of an update remembers the current mute state in
  `otaMutedWas`, forces `cfgA.muted = 1`, calls `Audio::setMute(true)` directly and then waits 400 ms
  before beginning the flash write. Writing flash freezes code that is not in IRAM, the audio task
  included; if the fade and the DMA chain have not finished by then, the auto-clear feature swaps in
  zeros mid-chain — a level step at the DAC, a click. The budget is at most 5.8 ms to see the mute, plus
  `fadeOutMs` (150 ms by default), plus the chain (46.4 ms), about 202 ms. **The wait is fixed and
  covers a fade-out of up to about 340 ms.** `fadeOutMs` can be set up to 5000 ms, and any fade-out
  above about 340 ms makes every update start with a click.
- **Giving up restores the user's mute**, not an unconditional unmute. A transfer silent for 20 s gives
  up; a transfer that dies mid-image leaves the radio muted for up to 20 s, accepted as harmless.

---

## Ambersong firmware: the A32's memory budget

**The A32 has about 24 KB of heap really free**, measured once: the Bluetooth library's own report of
`esp_get_free_heap_size()`, printed on the A32's USB console, read 23,620 bytes with that build. It has
not been re-measured since.

**+32 KB of DMA buffers broke updates and Bluetooth once.** Raising `dma_buf_count` from 8 to 16 (to try
to ride out audio-task freezes) cost another 32 KB of DMA-capable internal RAM, because the driver
allocates the same chain for transmit and receive — more than was free. Every firmware update was then
refused (`Update.begin()` returned false with error code 0, meaning its own 4 KB allocation failed on a
chip seconds out of reset); phones and PCs could not connect over Bluetooth, and the chip crashed once.
There is no way to reset or reflash the A32 from the S3 (no reset or boot-mode wire runs between the
boards), so only a USB cable brought it back. The chain was returned to 8 buffers by a USB flash, after
which an over-the-air update of the very same image — refused six times in a row before — was accepted.

The arithmetic for anyone tempted to repeat this: each buffer is 256 frames × 8 bytes = 2 KB; 8 buffers
are 16 KB = 46.4 ms per direction; transmit and receive together are 32 KB. Every millisecond of chain
depth costs about 0.35 KB per direction, **about 0.7 KB per millisecond in total**, whether bought with
more buffers or longer ones (the driver caps one buffer at 4092 bytes) — longer buffers differ only in
how the memory fragments, not in how much they take. The 32 KB Bluetooth ring is static memory and
already inside the budget.

**Which number to trust.** The portal's `heap` field is the **S3's** own free heap; it says nothing
about the A32, and no A32 heap figure reaches the portal at all. The A32's console `s` line prints
`ESP.getFreeHeap()`, the free internal heap in general, which is not the same as what a plain
`malloc()` or a DMA allocation can get. No console key and no portal field prints the A32's
`esp_get_free_heap_size()` or its largest free DMA block
(`heap_caps_get_largest_free_block(MALLOC_CAP_DMA)`) today — the only reading of the first is the
Bluetooth library's own "Available Heap" lines while Bluetooth starts at boot, and nothing prints the
second. So the ~24 KB figure above is the only one recorded. Before spending RAM on the A32, read both
figures on the A32's own USB console, which for the DMA block needs a build that prints it.

---

## Ambersong firmware: sound telemetry fields

Every 250 ms (`PROTO_STATE_MS`) `loop()` sends a `ProtoState` to the S3, which shows it on the portal.
The fields for sound:

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

Every readback comes from the hardware (the pad, the register, the controller) and never from a copy of
what the firmware thinks it wrote. On the USB console only: the Bluetooth stack's own heap figure, the
`[ZDD]` lines that `loop()` prints when the zero-data witness sees a release or a stall, and the text of
update errors. The console `s` command decodes `i2sraw`'s bits by name.

**Reading `i2sraw`.** Most of its bits latch in normal running. On this radio it has read `0x092A`
(2346) at every read, across boots and through heavy popping: bits 1, 3, 5, 8 and 11
(`tx_put_data`, `rx_rempty`, `tx_rempty`, `in_done`, `out_done`, from the ESP-IDF register header
`soc/esp32/include/soc/i2s_struct.h`), most likely set at start-up. `rx_wfull`/`rx_rempty` are expected
on BT and AUX, where nothing reads the receive chain and it overflows by construction. A steady
`i2sraw` is therefore not itself a fault, and it gave no evidence of a transmit fault during the pop
hunt.

---

## Ambersong firmware: the pop hunt — hypotheses, tests and measurements

**What a pop is.** A loud, short click through the speakers, at irregular times, heard on RADIO and on
BT, never on AUX. The root cause has never been found; the table below records every hypothesis tested
and what the measurement showed, kept as evidence for the next person who hears a click, not as a
story. A rate is always given with its measurement window; a bare "zero" is never written.

| hypothesis | test | result | verdict |
|---|---|---|---|
| Bluetooth ring underrun click | mute the phone while it keeps streaming (zero frames arriving); raise the phone's own volume from near zero | muting stopped the pops; the faintest non-zero phone volume brought them back at **full loudness** | **excluded.** A linear stage cannot do this: turning the phone down 40 dB should drop any artefact it makes by 40 dB too, underrun clicks included (their size is exactly the last sample before the gap) |
| WiFi / 2.4 GHz coexistence with the S3 | power the S3 off completely, A32 alone, playing | no change | **excluded** |
| Draining the idle I2S receive half on BT/AUX, in case it disturbed transmit | implemented and tried | RADIO, which already reads the receive half, popped the same way | **excluded.** Not kept in code; the receive chain free-runs on BT and AUX by design |
| Bluetooth-only cause | switch source to RADIO | popped the same way as on BT | **excluded** as a sole cause: rules out the whole Bluetooth branch (SBC decode, ring, underruns, library volume) acting alone |
| AUX path | play AUX | clean | not evidence of a fix — AUX writes zeros through the same output stage, so this only shows zeros are safe |
| DAC mute input held high on a spare pin | drive it | no change | **excluded** as a fix; the pin was later found (hardware) to reach nothing |
| Firmware arithmetic / clipping | peak meter through 60 s of continuous popping on RADIO | never above −118.4 dBFS; `clip` flag never set | **excluded.** The digital words written were clean |
| A fault locked to a firmware period (the 172.3 Hz audio block, the 20 Hz knob poll, the 4 Hz state message) | measure pop timing | irregular and bursty: under one a second at times, 3–5 inside a second at others, 7–8 s apart at others | **excluded** |
| Passive electrical loading of the ADC | ADC unpowered (clean, and a 60 Hz hum also went away); ADC powered with its I2S cable removed (clean); ADC's master clock cut in firmware while still powered and wired | clean with the clock cut, pops and hum both returned when it resumed | **excluded** as simple passive loading — the converter had to be actually clocked and running; narrows suspicion to the clock |
| Data-line activity at the word boundary makes the DAC lose framing | constant-DC diagnostic: six fixed words at known levels, each held ≥30 s on RADIO — level, bit count and edge density all failed to predict the outcome; only the position of the **lowest set bit** did, with no exception (`0x00FF0000`, bit 16, silent; `0x000000FF`, bit 0, popped — the same 8 bits, 96 dB apart) | read at the time as confirmed; **later superseded** — a hardware clock-wiring change fixed this symptom, but pops later returned on constant and near-silent words, which this theory cannot explain | **superseded.** An output low-bit mask was built as a mitigation but never confirmed by ear, and cannot touch the next word's sign bit that opens every slot |
| Sign-extended tail (stop the data line moving across a word boundary) | blind on/off test with a heat gun as an independent trigger | no change in rate or size; the rate did not rebound when turned back off | **excluded** |
| DAC's own zero-data mute releasing with an unramped step | zero-data witness (`zrel`) through 8 minutes of pops | `zrel` did not move | **excluded** |
| Knob/volume reading jitter | volume watch (`gsteps`) with the knob ignored and frozen | still popped | **excluded** |
| Audio task starvation (a stall) | stall counter and underrun counter read during popping | `stalls` frozen, `under` at 0, during pops; a separate check found 13 stalls in about 33 minutes with Bluetooth idle on RADIO | **excluded** as the direct cause (a suspected stall–Bluetooth link was also withdrawn; stall telemetry is kept for its own sake) |
| Mains / conducted interference | a heat gun on the same mains circuit popped the radio from 5 cm or 10 m away; on a different circuit, no effect at any distance; an inlet filter changed nothing; the baseline rate survived on a quiet circuit | pops still occurred on a quiet circuit, at a lower rate | mains is a **contributor on top of an underlying rate, not the sole cause**; the route is conducted, not radiated |
| Knob calibration / gain staging | the stored knob calibration was implausible and silently fell back to unrelated defaults, pinning the volume near 18/255 (about −62 dB, the DAC near −77 dBFS) with the amplifier turned up to compensate | every fault then arrived about 62 dB louder than it would otherwise | not a cause by itself, but a **safety item**: recalibrate the knob before any further pop measurement |
| Data-line output delay register (`tx_sd_out_delay`) | swept live across its positions | position 2 sent the radio to **full volume** through the valve amplifier; 1 and 3 silenced it; only 0 plays correctly; the speakers survived | **dangerous — removed from every remote and console path.** Only a read-back (`sddly`) remains |
| DAC's documented clock-error hard mute (about 104 samples, ~2.4 ms) | measured the level of a streamed 220 Hz tone through 37 recorded pops (band-limited 180–260 Hz, phase-insensitive) | median change −0.3 dB; only 8 of 37 dipped more than −10 dB | **excluded** as the main mechanism |
| One fixed-size sign-bit flip, same on every pop | compared pop sizes and polarities across many events | sizes spread over about 19 dB; polarity split roughly evenly against the prediction | **not supported** |
| Clock pad drive strength (`MCLK`/`BCK`/`LRCK` pads, 0–3) | one-minute alternation between drive 2 and drive 3, 30 one-minute windows, a 220 Hz tone streamed over Bluetooth, an independent microphone counting | **drive 2: 52 isolated pops in 936 s (3.33/min). Drive 3: 0 isolated events above −35 dBFS in 932 s.** Stratified exact test p = 2.8 × 10⁻¹⁶, all 15 pairs the same way. Follow-up soaks at drive 3: 1 pop in 45.1 minutes with a tone streaming (≈0.022/min); 0 events in 90.0 minutes with a constant word and Bluetooth streaming (upper bound ≈0.03/min, allowing for the capture's own 0.011 % of lost time) | **adopted as the shipped fix** — see below. It raises the margin against mis-latching; it is **not** a root cause |
| Clock pads driven as one knob vs. as separate halves | `setClockDrive()` moves `MCLK`, `BCK` and `LRCK` together, so "drive 2 vs. 3" is one variable covering all three lines; a later measurement on the radio source (not Bluetooth), at drive 2, found 932 pops in 5.1 minutes (183/min) — 55× the earlier drive-2 rate, under several uncontrolled differences from the first measurement; a split control (`sys.clkpair`, driving `MCLK` apart from `BCK`/`LRCK`) was written to test which line matters | the split has **never been run** | open |
| Signal-level dependence (does pop size track the sample value?) | a designed A/B of A32 volume 47 vs. 94 with a tone streaming | **never run** | open |
| Hardware changes (a DAC clock-wiring change, then later cable and ferrite changes — see `hardware-bible.md`) | before/after listening | the clock-wiring change removed the pops entirely for some time, until they returned; no firmware changed for the cable/ferrite work, and **no instrumented pop count exists after it** | inconclusive by instrument; by ear, clean since |
| Bluetooth transmit power ceiling (lowered to 0 dBm at boot) | tried as a mitigation, kept as a setting | **its effect on the pops was never measured** | unproven, kept anyway |

Two readings must not be cited as results: a reading that pops seemed to follow Bluetooth activity was
not a designed A/B and is confounded by time of day (a phone happened to be reconnected around when the
observation was made); and one A/B run's very low p-value was invalidated by two unexplained "crackle
storm" bursts of dense detections that broke the test's independence assumption.

**Prior art.** TI's published fix for an audible click on a sibling DAC is to ground that part's system
clock so its internal PLL runs from `BCK` instead (TI E2E thread 1052863) — that was this radio's own
wiring before the clock-wiring change above, when the pops were worse, and it has not been re-tried
since. TI publishes no data-to-bit-clock setup/hold figure for this DAC family (TI E2E thread 1542019);
at this bit rate (354 ns per bit) that suggests edge quality, not setup/hold timing, is the open
question. No report was found of pops surviving a constant (non-changing) data word, or of a fix using
`gpio_set_drive_capability()`, for this exact arrangement — one ESP32 I2S master clock feeding both an
ADC and a DAC.

### Ambersong firmware: the clock-pad-drive fix and its trade-offs

`Audio::begin()` calls `setClockDrive(3)`: it drives the `MCLK` (`GPIO0`), `BCK` (`GPIO18`) and `LRCK`
(`GPIO17`) pads at the strongest of the ESP32's four drive strengths, instead of the power-on default
of 2. This cut the pops about 150 times in the measured conditions above. The explanation offered —
`GPIO0` drives two converter clock inputs down a harness stub, the DAC's own fallback clock recovery is
off while an external `SCK` is present, and a stiffer edge stops a marginal sampling instant from
latching the wrong bit — is a hypothesis; no oscilloscope has ever examined the edge. It is **a margin
fix, not a root cause**: it explains why the DAC might latch wrong bits on a marginal edge, not why the
edge is marginal in the first place.

Drive 3 was reported once to make the radio path (not Bluetooth) sound distorted, "like people lightly
talking through kazoos," while drive 2 sounded clean on the radio path but popped hard. Both were
reproduced by changing only the clock drive and nothing else. Drive 3 is judged, since, to work with no
apparent consequences on this machine; no recording of the distortion exists, so it and the pops were
never put on one measurement scale, and no measurement of it exists after the hardware cable/ferrite
changes. **Drive 3 stays the boot default, and drive 2 will not be tried again**: drive 3 works with no
apparent consequences, and there is nothing measured to gain from drive 2. The remote control
`sys.clkdrv` stays in the firmware so drive 2 can be A/B-tested again, or put back, without a reflash.
The untested split control `sys.clkpair` also stays in the firmware, never run.

### Ambersong firmware: open questions about the pops

- **The root cause is not known.** Drive 3 is a margin fix, not an explanation.
- **The 183/min reading on the radio source at drive 2 is unexplained**; several conditions differed
  from the earlier, lower-rate drive-2 measurement, and none was isolated.
- **The drive-3 "kazoo" distortion of the radio path** was reported once, by ear only, with no
  recording and no measurement since the hardware cable/ferrite changes.
- **The raw pop-hunt recordings and analysis scripts no longer exist**; only the numbers in the table
  above survive.
- **The stall click's cause was never measured**; a flash write with the cache off is the leading
  guess (see "the A32's memory budget" above). It is not fixed, because a deeper DMA chain costs RAM
  the A32 does not have.
- **No waveform has ever been captured.** There was no oscilloscope; every judgement of sound quality
  was by ear.
- **The firmware cannot detect a DAC-side clock error.** No telemetry exists for it.
- A fixed comb of radio-frequency features seen by the tuning receiver, at multiples of 2.8224 MHz, is
  suspected to come from the A32's I2S clocks (which run even while the A32 sleeps); this has never
  been tested (see `firmware-gospel-tuning.md`).

---

## Ambersong firmware: sound settings reference

### Stored settings (A32 NVS `amb`, editable from the portal)

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
| `fadeOutMs` | 150 | ms | 0–5000 in 10 | fade out (0 acts as 1 ms); above about 340 ms, an update starts with a click | portal |
| `connectable` | on | bool | | "Let a paired phone reconnect"; off closes Bluetooth entirely, the pairing window included | portal |
| `lookTimeoutS` | 90 | s | 15–600 in 5 | the pairing window (`BT_LINK`) | portal |
| `pauseOnLeave` | on | bool | | AVRCP pause before every disconnect | portal |
| `btLedOff` … `btLedLink` (`ledBrightness[7]`) | 255 each | step | 0–255 | per-state lamp scale; OFF ignores it | portal |
| knob calibration `potMin`, `potMid`, `potMax` | 60, 0 (no centre), 3990 | raw ADC | 0–4095 | three-point knob map | portal `pot.min`/`pot.ctr`/`pot.max`; console `p`/`c`/`P` |

**`autoConnect`** is stored but read by nothing: the radio never calls a phone
(`a2dp.set_auto_reconnect(false)` is compiled in). Its byte is kept inside `ProtoBtCfg` only because
removing it would change the stored structure's size, which resets the A32's settings to defaults; its
portal row has been removed, and an old settings file that still carries the key uploads cleanly (the
line is skipped).

### Compiled constants

| name | value | unit | meaning | where |
|---|---|---|---|---|
| `SAMPLE_RATE` | 44,100 | Hz | fixed; no resampler | `audio.cpp` |
| `BLOCK` | 256 | frames | one pass, about 5.8 ms | `audio.cpp` |
| `RING_FRAMES` | 8192 | frames | Bluetooth ring, about 186 ms, 32 KB | `audio.cpp` |
| `dma_buf_count` / `dma_buf_len` | 8 / 256 | buffers / frames | 46.4 ms per direction, 32 KB for both | `Audio::begin()` |
| MCLK | 11.2896 | MHz | 256 × 44,100, from the APLL | `Audio::begin()` |
| clock pad drive at boot | 3 | 0–3 | `GPIO0`, 18, 17; remote control clamps to 2–3 | `Audio::begin()` |
| data pad drive | power-on default | 0–3 | never set at boot | — |
| `tx_sd_out_delay` | never written; read back only (`sddly`) | 0–3 | nothing can write it any more — **hazard if it is ever reinstated** | `sdOutDelayIs()` |
| audio task | core 1, priority 6, 4096 B | | | `Audio::begin()` |
| `ENV_ONE` | 2²⁴ | Q24 | envelope unity | `audio.cpp` |
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
| update mute wait | 400 | ms | before the flash write begins | `onMessage()` |
| update give-up | 20,000 | ms | no frame from the S3 | `loop()` |
| source and knob poll | 50 / 50 | ms | | `loop()` |
| `POT_DEADBAND` | 24 | ADC counts | about 0.6 % of travel | `main.cpp` |
| selector thresholds | 424 / 2471 | raw ADC | AUX / BT / RADIO | `pins.h` |
| `PROTO_STATE_MS` | 250 | ms | state message to the S3 | `proto.h` |
| `PROTO_SILENCE_MS` | 2000 | ms | S3 considered gone, A32 sleeps | `proto.h` |
| settings debounce | 2000 | ms | NVS write delay | `settingsFlush()` |
| pair button | release after > 30 ms | | only in BT mode | `loop()` |
| lamp PWM | LEDC channel 0, 2 kHz, 8-bit | | | `btled.cpp` |

### Sound experiments: portal actions and console keys

Every experiment below is an **action, not a setting**: none survives a reboot, and every one is read
back from the hardware.

| portal action (message) | effect on the A32 | range | notes |
|---|---|---|---|
| `sys.clkdrv` (`MSG_CLK_DRIVE`) | drive of `GPIO0`, 18 and 17 together | clamped to 2–3 | the pop knob; boot value 3 |
| `sys.clkpair` (`MSG_CLK_PAIR`) | `MCLK` drive, then `BCK`/`LRCK` drive | 22, 23, 32 or 33 only; others refused | never run on the radio |
| `sys.dindrv` (`MSG_DIN_DRIVE`) | data-line pad drive | 0–3 | 0 and 1 may mis-decode for long stretches: someone present, amplifier down |
| `sys.dctest` (`MSG_TEST_DC`) | replace the output with the DC word | 0/1 | silent as audio; anything heard is the fault |
| `sys.dcword` (`MSG_DC_WORD`) | pick `kDc[index]` | 0–6; others refused | an index, never a raw word, so nothing louder than −42 dBFS |
| `sys.zfloor` (`MSG_ZERO_FLOOR`) | zero-data floor | 0/1 | |
| `sys.tail` (`MSG_SIGN_TAIL`) | sign-extended tail | 0/1 | ruled out |
| `sys.pot` (`MSG_USE_POT`) | ignore the front knob | 0/1 | |
| `sys.bttx` (`MSG_BT_TX`) | Bluetooth transmit ceiling | 0–5; others refused | down only |
| retired `MSG_ADC_CLOCK` | ignored | | retired |
| retired `MSG_SD_DELAY` | ignored | | retired |

USB console on the A32's own cable, 115,200 baud: `+`/`-` volume ±10 (`+` also un-mutes), `m` mute, `b`
pair, `v` knob on/off, `p`/`c`/`P` knob minimum/centre/maximum, `d` cycle the DC word (the first press
selects `kDc[1]`, `0x00FF0000`, because the boot word is `kDc[0]`), `k` cycle the output mask (0, 1, 2,
4, 6, 8, 12, 16 bits), `g` cycle the data-line drive 0–3, `t`/`T` taper −/+0.1, `r` raw knob stream at
10 Hz, `s` status (the knob reading and its calibration, the I2S latches decoded by name, the link and
awake state, every pad and register read back from the hardware, peaks, ring, underruns and
`ESP.getFreeHeap()` — it drains the read-and-reset values, so prefer the portal for those). The boot
help line does not list `k` or `g`.

---

## Ambersong firmware: sound design decisions

| decision | reason | not chosen | evidence |
|---|---|---|---|
| I2S is installed once, full duplex, and never reconfigured; the Bluetooth library never reaches I2S | the two paths need incompatible drivers, and reinstalling under the Bluetooth stack risks hard faults | letting the library own I2S; reinstalling the driver at each source change | `NullA2dpOutput` and `setup()` in `src/a32/main.cpp` |
| APLL on, a fixed 256×fs MCLK on `GPIO0`; `GPIO0` is never detached from I2S | both converters take their system clock from `GPIO0`, and only the APLL makes 11.2896 MHz exactly | the integer divider from the 160 MHz clock (cannot make 44,100 Hz exactly) | `Audio::begin()` |
| mute is software only, computed from state, never stored | no pin reaches the DAC's mute input; a mute toggled by several code paths was left in the wrong state once; a stored mute booted the set silent once | driving `GPIO16` as a hardware mute (it reaches nothing); storing mute | `applyMute()`, `settingsLoad()` |
| the radio's two channels are summed to mono | both channels carry the same signal; the sum gains about 3 dB of signal-to-noise | — | `audioTask()`, setting `monoSum` (default on) |
| fades are asymmetric: 150 ms out, 1000 ms in, on a squared per-sample Q24 envelope | leaving a source is a deliberate act and should be quick; arriving should be gentle; a linear ramp does not sound like a fade | a linear Q16 ramp (finished in 0.18 ms — a hard cut, heard as a jump) | `settingsDefaults()`, `ENV_ONE` in `audio.cpp` |
| the volume curve is a portal setting (gamma 2.5 default); the knob is calibrated at three points | the right curve depends on the amplifier and can only be settled by ear; the knob's raw reading is not proportional to rotation, so no after-the-fact curve fixes it, but a centre calibration point does | a fixed logarithmic law | `potRotation()`, `setTaper()` |
| the knob is a sensor: the first reading applies, volume is not stored | the knob's position *is* the stored state; a stored volume the knob disagrees with on boot repeats a known silent-boot failure | storing volume in NVS | `updateVolumePot()` |
| radio gain +8 dB, Bluetooth gain −7 dB | measured with the RMS meter so both sources are equally loud at every knob position, and the radio does not clip before the volume | +15 dB (clipped at every volume); matching by peaks (FM is limited hard, music is not) | RMS measurements above |
| Bluetooth is invisible unless awake and in BT mode; the radio never initiates; only `applyScanMode()` writes the scan mode | the three-rule policy above, and "nothing connects at night" | letting the library write the scan mode | `PolicyA2dpSink`, `applyScanMode()` |
| pin the Bluetooth library to one commit | the visibility rule depends on the library's internals; an unpinned library can change under the build without warning | following the library's main branch | `platformio.ini` |
| pause the phone, then disconnect, whenever the radio lets a phone go | a phone torn down mid-play leaves its app playing into nowhere | staying connected outside BT mode | `gracefulDisconnect()`, `updateSource()` |
| the pair button drops the connected phone first | otherwise a second phone can pair behind the first and it is no longer obvious which one is in charge | — | `loop()` |
| the A32 sleeps when the amplifier is off, and S3 silence counts as amplifier off | a dead main board must be noticed, not silently assumed to still have the amp on | "assume the amp is on so a dead helper board never silences the radio" | `serviceWake()` |
| forget `ampOn` when sleeping because the S3 went silent | a stale "amp on" once woke the A32 unmuted into an amplifier that was off | — | `serviceWake()` |
| the A32 writes no settings while its image is on trial; held changes are written at confirmation | a rolled-back image must never find a settings blob saved by the image it replaced | saving during the trial (the earlier behaviour) | `settingsFlush()`, `confirmImage()`; proven live once |
| a failed update restores the previous mute state, not "unmuted" | — | — | `otaMutedWas` |
| clock pads at drive 3 from boot | about 150 times fewer pops in the measured conditions | drive 2 (the power-on default); the untested split `sys.clkpair`. The remote control `sys.clkdrv` is kept so the choice can be A/B-tested again, or reversed, without a reflash | the clock-pad-drive measurement above |
| the DMA chain stays at 8 buffers | 16 buffers took 32 KB the chip did not have; updates and Bluetooth failed | 16 buffers; longer buffers or a count raised only as far as the free DMA block allows (both cost the same RAM per millisecond) | the memory-budget incident above |
| Bluetooth transmit ceiling of 0 dBm at boot | a pop mitigation: transmit bursts are the largest current step the board makes | — | never measured |
| the right channel goes in I2S slot 0 | this order gave correct stereo on this machine; it is an adjustment for this machine, and another build must check it | the usual order (left in slot 0) | `audioTask()`, `Audio::begin()` (`I2S_CHANNEL_FMT_RIGHT_LEFT`) |
| the Bluetooth lamp is driven fully on in LINK, at the top of LOOK and in FOUND | the specified behaviour at the default scale of 255 | capping the drive below full | `BtLed::update()` |
| experiments are actions, never stored; the remote DC word is chosen by index only | a stored experiment gets its result blamed on something else later; nothing louder than −42 dBFS can be sent remotely | — | — |
| nothing in the firmware writes `tx_sd_out_delay` | at position 2 it once sent the radio to full volume through the valve amplifier | keeping it on the USB console as a hazard with a warning | the pop-hunt measurement above |
| the `autoConnect` field stays in the protocol structure, unread; its portal row is gone | it did nothing (the radio never calls a phone); removing the byte would change the size of the stored structure | — | — |

---

## Ambersong firmware: sound failures and recovery

| what fails | what the firmware sees | what it does | how to recover | class |
|---|---|---|---|---|
| RAM spent past the ~24 KB really free (for example 16 DMA buffers) | nothing; the update begin call fails with error 0, Bluetooth refuses connections | refuses every update | USB reflash only | **BLOCKER** (USB-only) |
| `tx_sd_out_delay` (or the `BCK`/`LRCK` out-delays) written with audio live; nothing in the shipped firmware can do this | nothing | 1 or 3: silence; **2: full volume** through the amplifier | write 0; the speakers are at risk | **BLOCKER** (harm) |
| data-line drive set to 0 or 1 | nothing | the line may mis-decode for long stretches | set 2 or 3, or reboot | DEFECT |
| I2S install fails at boot | `[FAIL]` on the USB console | no audio task; silence | reboot; if it persists, reflash | DEFECT |
| audio task blocked forever (no known cause) | `loop()` keeps running; the loop watchdog does not watch the audio task | silence | portal reboot of the A32 | DEFECT |
| audio task starved for more than 46.4 ms | stall counted | the chain drains; zeros go out: a click | none needed | NOTE |
| phone negotiates a rate other than 44.1 kHz | `onSampleRate()` | USB warning only; plays at the wrong pitch | none | NOTE |
| phone clock drifts against the local 44.1 kHz | ring fill | ring slowly fills (drops) or empties (underruns) | none needed | NOTE |
| S3 silent for more than 2 s | `peerAlive()` false | sleeps: mute, Bluetooth closed, `ampOn` forgotten | automatic when the S3 speaks and says the amp is on | NOTE (by design) |
| stored knob calibration implausible | `hi ≤ lo + 64` | falls back to 60 / 3990, no centre | recalibrate with `pot.min`/`ctr`/`max` | BLOCKER (guarded: the knob runs on 60/3990) |
| stored knob calibration plausible but wrong | nothing | the knob maps wrongly | recalibrate | BLOCKER |
| a disconnect request never lands | `btDropping` older than 3 s | warns on USB and to the S3; lamp shows connected | none needed | NOTE |
| a phone connected while asleep or out of BT mode | `btConnected` | pauses and drops it, retried every 3 s | none needed | NOTE |
| protocol self-test fails at boot | `protoBroken` | muted, Bluetooth closed, link and update receiver kept up; never confirmed | image that arrived over the air: portal reboot of the A32 (rolls back); image flashed by USB: update to a good image | DEFECT |
| an update stops mid-transfer | no frame for 20 s | gives up, restores the previous mute | none needed | NOTE |
| `fadeOutMs` set above about 340 ms | nothing | every update starts with a click | set it lower | NOTE |
| `connectable` off | policy | Bluetooth fully closed; the pair button blinks the lamp but no phone can find the radio | turn it on | NOTE (by design) |
| stored settings blob of another size | length check | defaults apply | set again from the portal | BLOCKER by design; avoidable by exporting first |
| a settings write to flash fails | a short NVS write | keeps the change unsaved, warns, retries 2 s later | none needed | NOTE |
| a settings change made while the A32's image is on trial, then a restart before it confirms | `gImgOnTrial` | holds the change in RAM; the restart rolls the image back and the change is not kept | set it again | NOTE (by design) |
| a portal edit of an audio row while the A32 is silent | the S3 has no settings copy | HTTP 409, "the audio board is not answering - its settings cannot be changed now" | wait for the handshake, then edit again | NOTE |
| the pops | nothing the firmware can see | none | they are events; root cause open | NOTE |

---

## Ambersong firmware: changing the sound firmware safely

**Invariants.**

- Never reinstall, reconfigure, `i2s_set_clk`, `i2s_stop` or re-pin I2S after `Audio::begin()`.
- Keep **both** `set_output(nullOut)` and `set_volume_control(&noVol)`, and the stream-reader flag.
- Never detach `GPIO0` from the I2S peripheral. Never turn the APLL off.
- The audio task never delays, never prints, and blocks only in the I2S calls. `pushBt()` never
  blocks.
- Mute only through `applyMute()`. The start of an update is the one deliberate exception.
- The scan mode is written only by `applyScanMode()`. Never call the library's
  `set_discoverability()`.
- **Do not change the Bluetooth (A2DP) library's pinned version without re-testing invisibility** with
  a phone: in RADIO, in AUX, asleep, after a portal hang-up and after a source change. Check first that
  every library path that writes the scan mode still goes through the virtual
  `set_scan_mode_connectable()`.
- Do not update the build platform (`espressif32@7.0.1`).
- Slot 0 carries the right channel on this machine. On another build, play a left/right test track
  before trusting it.
- New `ProtoState` fields go at the **end**, so a version mismatch truncates rather than shifts; the
  link's protocol version is deliberately not raised for such an append, because a raised version loses
  the handshake, and with it the update relay to the A32 (see `firmware-gospel-link.md`). New message
  ids are backward compatible.
- Readbacks come from the hardware, never from a copy.
- Read-and-reset counters (`getPeaks()`, `getRms()`, `btRingFill()`) should have one reader: the 250 ms
  state message. Today the console `s` also reads `getPeaks()` and `btRingFill()`. `i2sFaults()` is
  read and cleared by the console `s` alone; the state message sends `i2sSticky` instead.
  `zeroWatch()`, `gainWatch()` and `i2sSticky` are monotonic and safe to read anywhere.
- `i2sFaults()` clears only the bits the driver does not service (masked against `int_ena`); clearing
  everything could lose the driver's buffer-done events and block the audio task forever.

**The heap.** Before spending any RAM, read `esp_get_free_heap_size()` and
`heap_caps_get_largest_free_block(MALLOC_CAP_DMA)` on the A32's USB console (console `s` prints only
`ESP.getFreeHeap()`; add a print for both figures in the same build). Do not trust the ~24 KB figure
above for a build that has grown since. Budget about 0.7 KB of DMA RAM per millisecond of chain depth.
A +32 KB request was fatal once and only USB recovered it.

**The update wait.** If `fadeOutMs` is raised above about 340 ms, or the DMA chain is deepened, raise
the 400 ms wait before a flash write starts to match.

**Hazards.**

- Never write `tx_sd_out_delay`, `tx_ws_out_delay` or `tx_bck_out_delay` with audio live. Nothing in the
  shipped firmware writes any of them. If one is ever needed again: amplifier down, mute, set, then
  un-mute at a known low level.
- Data-line drive 0 or 1, or any clock drive below 2, can mis-clock for long stretches. The clock
  setter clamps to 2–3; the data-line setter does not.
- Verify a flash by something only the new build can say (its version string, the A32's uptime
  resetting), not by a value the previous build already published.
- `gpio_set_drive_capability()` cannot disable `GPIO0`'s clock output by accident: `GPIO0` is an
  RTC-capable pad, and the IDF call writes only the pad's drive-strength field; the pad's function
  select is a separate bit.

**Traps in the code.**

- The volume gain steps at block boundaries; it is not ramped.
- `muteOnChange` off switches sources with no ramp: a thump.
- `setFades(0, …)` becomes 1 ms, on purpose.
- Balance ±100 is about −65 dB, not silence.
- The zero-data witness counts the DAC's idea of zero (the top 24 bits), not the firmware's own.
- With the sign-extended tail on, a word of 1 to 255 followed by a non-negative word becomes exact
  zero; it matters only if the tail is ever turned on.
- `btDropping` is set before `a2dp.disconnect()`; a stale flag is swept first thing in
  `updateBtState()`; `scanDirty` is cleared before the write and `onConnState()` sets it again after
  `btConnected`; the wake edge cancels a pending sleep disconnect and the stray-phone timer. Do not
  undo any of these orderings.

**How to count pops, if they come back.**

- Use a microphone recorder about a foot from the radio, streaming continuous 48 kHz audio, with its
  own clock logged beside its sample count (a dropped chunk is a splice, and a splice looks like a
  click). Read it with a large driver buffer (1 MB worked; a small one lost samples). A good capture
  runs at 47,99x of 48,000 samples/s. Turn the microphone gain down far enough not to clip a foot from
  a speaker.
- Detect on the 1 ms RMS of a high-passed signal (a 10-tap filter), with an absolute threshold of
  −35 dBFS (−45 as a secondary), 50 ms dead time, and an isolation score (event level minus the median
  over ±200 ms). Real pops scored 25–58 dB; crackle, speech and handling scored below 15. Confirm by
  ear before trusting the detector. Do not use a shape filter ("falls 10 dB in 5 ms") — pops ring, and
  that approach kept only 29–61 % of real ones. Keep a heat gun, if used as a trigger, away from the
  microphone.
- Run a **one-minute alternation, 15 pairs or more, both arms in the same recording, a source actually
  streaming**, and a stratified per-pair exact test. Never compare totals or one long block per arm:
  the rate changes by the hour. A silent link sends exact zeros and hides pops, so use `sys.dctest 1`
  to make them visible whatever plays. Decide the inconclusive-result rule before the run (fewer than
  6 events is inconclusive here). Close the last window at the end of the run, not the end of the
  recording. If events cluster (a crackle storm), the per-pair test is invalid — do not cite its
  p-value.
- Check the stall counter and timestamps to tell a stall click (step-shaped; correlates with a recorded
  switch step) from a pop (an impulse).
- Traps that cost real time before: portal sessions expire after 5 idle minutes, so a long script must
  log in before every portal call or its commands are silently refused; a script should stamp a switch
  after its command and readback, because the real switch lands about 1.2 s before the stamp; never
  edit a shell script while a copy of it is running (the shell re-reads the file and the running copy
  dies); a build-and-flash chain must test the build's own exit code, not a piped command's; a cleanup
  trap that restores a setting must also exit cleanly, or the script keeps toggling with no recorder
  running; two readers of a read-and-reset counter corrupt each other's readings; a reboot or an update
  pops by itself, so check the flash log before counting a pop "on its own".
- Rates to beat, each with its window: drive 2, tone over Bluetooth: 52 in 936 s (3.33/min); drive 3,
  same run: 0 in 932 s; drive 3, tone over Bluetooth: 1 in 45.1 minutes; drive 3, constant word,
  Bluetooth streaming: 0 in 90.0 minutes; drive 2, radio source, A32 volume 8: 932 in 5.1 minutes.
  Always state the window and the bound; never write the bare word "zero".
