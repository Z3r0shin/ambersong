# Appendix A. Settings reference

This appendix lists every stored setting of both boards, then the constants of the settings system, then
the experiments, which look like settings but are never saved. How settings are saved, backed up and
restored is chapter 7. Where a chapter explains a setting in detail, the row points to it.

**How to read the tables.**

- **Default** is the compiled value, with its unit. Where the firmware stores a value scaled, the stored
  number follows in brackets.
- **Range** is the hard clamp: a value outside it is stored at the bound, and the answer says CLAMPED
  (chapter 7, section 7.6.2). Exceptions are stated.
- **Changed by:**

  | Word | Meaning |
  |---|---|
  | Portal (all) | On its portal tab, by any signed-in account, guest included. |
  | Portal (admin) | On its portal tab, by the administrator only. |
  | File | A settings-file upload (administrator only). |
  | S3 key `x` | The main board's console key (USB, or the portal's Console tab). |
  | A32 key `x` | The audio board's console key (its own USB cable only). |
  | Knob | The front volume knob. |
  | **Button** | A portal button, named as on the page. |
  | Auto | The firmware itself. |

- **Saved in** names the board's storage namespace and key (chapter 7, section 7.2): `amb3/cfg` is the main
  board's one settings blob; `amb/audio` and `amb/bt` are the audio board's two blobs; `amb/potMin` and its
  neighbours are single audio-board keys; `net` and `auth` are the main board's network and account stores.
- Every portal row is written in the downloaded settings file and read back on upload.

## A.1 Audio and Bluetooth (the audio board)

**Audio tab.** All are the audio board's, saved in `amb/audio`.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `volume` | 40 | 0–255 (a linear fader) | Master volume; the volume law below is applied on the audio board. The knob's first reading after each audio-board start replaces it (chapter 7, section 7.4.3). | Portal (all), File, A32 key `+` / `-` (10 per press; `+` also unmutes), Knob |
| `muted` | 1 in the compiled defaults; **forced to 0 at every start** | 0 / 1 | Software mute: the audio board sends zeros to the converter. Saved, but never restored. Forced on during an audio-board update and restored afterwards. | Portal (all), File, A32 key `m` |
| `taper` | 2.5 (stored 25) | 1.0–4.0, step 0.1 | The volume law: gain = (volume ÷ 255) to the power gamma. 1.0 is linear amplitude; 2.0 puts half travel at −12 dB; 2.5 at −15 dB; 3.0 at −18 dB; about 3.3 behaves like a real audio-taper potentiometer. | Portal (admin), File, A32 key `t` / `T` (0.1 per press) |
| `balance` | 0 | −100 to +100 | Negative leans left. | Portal (admin), File |
| `gainRadio` | +8.0 dB (stored 80, tenths of a dB) | −20 to +30 dB, step 0.5 (the audio board also clamps −60 to +30) | Makeup gain on the tube-radio input, before the volume. | Portal (admin), File |
| `gainBt` | −7.0 dB (stored −70) | −20 to +30 dB, step 0.5 (the audio board also clamps −60 to +30) | Bluetooth gain, before the volume, set to match the radio's loudness within about 1 dB. | Portal (admin), File |
| `monoSum` | 1 (on) | 0 / 1 | Sum left and right on the radio input (about 3 dB less noise). | Portal (admin), File |
| `muteOnChange` | 1 (on) | 0 / 1 | Fade down and up across a change of source. | Portal (admin), File |
| `fadeInMs` | 1000 ms | 0–5000 ms, step 50 | Fade-up time on arriving at a source. 0 acts as 1 ms. | Portal (admin), File |
| `fadeOutMs` | 150 ms | 0–5000 ms, step 10 | Fade-down time on leaving a source. 0 acts as 1 ms. Above about 340 ms, an audio-board update starts with a click (chapter 9). | Portal (admin), File |

**Why these defaults.**

- **`volume` 40 and `balance`** date from an earlier setup of the amplifier's own volume control, which has
  since been replaced. That control sits on the amplifier's board, reached only from the back. The front
  knob is the audio board's, not the amplifier's (Hardware Bible, chapter 6, section 6.8). The defaults
  stay: the knob's first reading overwrites `volume` at start-up anyway, and `balance` is a plain
  left/right trim. It does nothing on AUX, which never passes through the audio board (Hardware Bible,
  chapter 6, section 6.7).
- **`gainRadio` +8.0 dB** was measured with the tube radio's own volume control at about one eighth of its
  travel, its normal position (Hardware Bible, chapter 6, section 6.8). The previous +15 dB clipped at the
  gain stage at every volume. A different tube-set level needs a different radio gain.
- **`gainBt` −7.0 dB** was measured the same way. A pink-noise reference at −14 dBFS RMS (dBFS: decibels
  below digital full scale), a typical streaming loudness, played from a computer at full volume, arrived
  at about −15 dBFS. −7 dB lands it within about 1 dB of the radio, so a change of source does not change
  the loudness.
- **`fadeInMs` 1000 and `fadeOutMs` 150:** arriving at a source should be gentle, about a second; leaving
  should be quick, because the listener has just turned the selector.
- **`muteOnChange`** exists because the source selector is an ON-OFF-ON switch whose centre position reads
  as AUX (Hardware Bible, chapter 9, section 9.3), so a change from RADIO to Bluetooth passes through AUX for
  a moment. The fade covers that passage.

Chapter 10 explains the volume law, the gains and the fades.

**Bluetooth tab.** All are the audio board's, administrator only, saved in `amb/bt`.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `connectable` | 1 (on) | 0 / 1 | Let a paired phone reconnect. Even at 1, the audio board is connectable only while it is awake (amplifier on, main board talking) and the source is Bluetooth, so a phone cannot latch on at night to a radio whose amplifier is off. At 0, Bluetooth is closed entirely, the pairing window included (chapter 10). | Portal (admin), File |
| `lookTimeoutS` | 90 s | 15–600 s, step 5 | The **pairing window**, despite the name: the explicit pairing state LINK falls back to STDBY after this. The state LOOK has its own fixed 20 s, which is not a setting (chapter 10). | Portal (admin), File |
| `pauseOnLeave` | 1 (on) | 0 / 1 | Send a "pause" to the phone before the disconnect that follows leaving Bluetooth. | Portal (admin), File |
| `autoConnect` | 0 | — | **Not a setting.** A field of the Bluetooth structure that nothing reads: the radio never starts a Bluetooth connection itself. It stays so the structure keeps its size (a size change would reset every Bluetooth setting). Not a portal row; an old file's `autoConnect` line is skipped. | Nothing |

**The volume knob's calibration.** The audio board's, saved as three single keys `amb/potMin`,
`amb/potMid`, `amb/potMax`. Not portal rows. Written in the settings file only as the comment
`# potcal min..centre..max`, and never read back from it.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `potMin` | 60 | 0–4095 (raw reading) | The knob's raw reading at its minimum. | **Pot: minimum**, A32 key `p` |
| `potMid` | 0, meaning "not measured" | 0–4095 | The raw reading at the knob's mechanical centre. | **Pot: centre**, A32 key `c` |
| `potMax` | 3990 | 0–4095 | The raw reading at its maximum. | **Pot: maximum**, A32 key `P` |

The three points map the knob to the volume, 0–255. If the maximum is not at least 65 above the minimum,
the defaults are used. The defaults 60 and 3990 are this machine's knob, read at its two ends; they apply
until the knob is calibrated (the knob's wiring is in the Hardware Bible, chapter 9, section 9.4). The
**Pot** buttons are on the Audio tab, administrator only; the audio board measures the knob itself, as the
median of nine readings. Chapter 10 explains the knob.

## A.2 Display and lights

**Display tab.** All are the main board's, saved in `amb3/cfg`. Chapter 4 explains them.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `brightOn` | 255 | 0–255 | Clock brightness while the amplifier is on. | Portal (all), File, S3 key `-` and `+` / `=` (15 per press) |
| `brightOff` | 110 | 0–255 | Clock brightness with the amplifier off. Not lower on purpose: the display's colon is lit by its own fixed circuit and cannot dim with the digits (Hardware Bible, chapter 7, section 7.6). | Portal (all), File |
| `hour12` | 0 (off) | 0 / 1 | 12-hour clock, with no AM/PM indicator. | Portal (admin), File |
| `blankLeadZero` | 1 (on) | 0 / 1 | Blank the hour's leading zero. | Portal (admin), File |
| `dispDwellMs` | 250 ms | 0–5000 ms, step 50 | Stillness before the clock brightness starts to move. | Portal (admin), File |
| `dispFadeMs` | 1500 ms | 0–10000 ms, step 50 | Eased (smoothstep) travel between the two brightnesses. | Portal (admin), File |
| `showTuning` | 0 | a choice, 0–2 | 0: the clock always. 1: the tuning readout always. 2: the tuning readout while tuning, then back to the clock. | Portal (admin), File, S3 key `f` / `F` (cycles) |
| `tuneHoldMs` | 4000 ms | 500–20000 ms, step 250 | In mode 2, how long the readout stays after the dial last moved. | Portal (admin), File |

**Lights tab.** The panel lamps are the main board's, saved in `amb3/cfg`. The Bluetooth lamp's levels are
the audio board's, saved in `amb/bt`; the main board only mirrors and forwards them.

| Key | Default | Range | Meaning | Changed by | Saved in |
|---|---|---|---|---|---|
| `panelTuning` | 255 | 0–255 | Panel lamp level while tuning. | Portal (all), File | `amb3/cfg` |
| `panelIdle` | 128 | 0–255 | Panel lamp level, radio idle. | Portal (all), File | `amb3/cfg` |
| `panelOther` | 96 | 0–255 | Panel lamp level on AUX or Bluetooth. | Portal (all), File | `amb3/cfg` |
| `panelIdleMs` | 5000 ms | 0–60000 ms, step 250 | Time without tuning before "idle". | Portal (admin), File | `amb3/cfg` |
| `panelFadeMs` | 900 ms | 0–5000 ms, step 50 | Eased panel fade. 0 acts as 1 ms. | Portal (admin), File | `amb3/cfg` |
| `panelDwellMs` | 250 ms | 0–5000 ms, step 50 | Stillness before the panel lamps move. | Portal (admin), File | `amb3/cfg` |
| `btLedOff` | 255 | 0–255 | Bluetooth lamp brightness scale in state OFF. State OFF ignores its scale (chapter 10). | Portal (admin), File | `amb/bt` |
| `btLedOn` | 255 | 0–255 | … in state ON, ready. | Portal (admin), File | `amb/bt` |
| `btLedLook` | 255 | 0–255 | … in state LOOK, looking. | Portal (admin), File | `amb/bt` |
| `btLedStdby` | 255 | 0–255 | … in state STDBY, standby. | Portal (admin), File | `amb/bt` |
| `btLedFound` | 255 | 0–255 | … in state FOUND. | Portal (admin), File | `amb/bt` |
| `btLedCon` | 255 | 0–255 | … in state CON, connected. | Portal (admin), File | `amb/bt` |
| `btLedLink` | 255 | 0–255 | … in state LINK, explicit pairing. | Portal (admin), File | `amb/bt` |

## A.3 Needle and tuning

"hs" is a half-step of the needle's stepper motor. "Counts" are the tuning shaft's accumulated angle-sensor
readings (4096 per turn, carried across whole turns). Every key here is the main board's and saved in
`amb3/cfg`; every portal row here is on the **Needle** tab, administrator only. The keys that are not portal
rows say so.

### A.3.1 The needle

The motion defaults are the settings arrived at on a bench test rig; they are this build's settings.
Chapter 5, section 5.17, explains each; section 5.18 holds this machine's measured values.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `upVmax` | 1100 hs/s | 100–4000, step 50 | Top speed of every second-order move, in either direction (portal label "Sweep up, top speed"). | Portal (admin), File |
| `upAccel` | 18000 hs/s² | 1000–60000, step 500 | Acceleration of the same moves. | Portal (admin), File |
| `dnVmax` | 1700 hs/s | 100–4000, step 50 | Top speed of the decay: the fall to the park position. | Portal (admin), File |
| `dnAccel` | 18000 hs/s² | 1000–60000, step 500 | The decay's acceleration figure; the decay law itself has no acceleration limit, so it is used only in the park's arrival test (chapter 5). | Portal (admin), File |
| `wn` | 9.0 rad/s | 1–21, step 0.5 | Natural frequency of the second-order ring-down: how quickly the swing settles. Keep it below 4 × `zeta` × `upAccel` ÷ `upVmax`. | Portal (admin), File |
| `zeta` | 0.50 | 0.1–2.0, step 0.05 | Damping of the ring-down: how much the needle overshoots. | Portal (admin), File |
| `riseMs` | 100 ms | 10–3000 ms, step 10 | The fall's envelope: rise. | Portal (admin), File |
| `fallMs` | 1000 ms | 10–5000 ms, step 10 | The fall's envelope: fall. | Portal (admin), File |
| `dwellUpMs` | 100 ms | 0–3000 ms, step 25 | Pause before a second-order move (not while following the knob). | Portal (admin), File |
| `dwellDnMs` | 1000 ms | 0–3000 ms, step 25 | Pause before the fall to the park position. | Portal (admin), File |
| `microFast` | 16 (1/16 step) | 1–32 | Microstep divisor for sweeps, parks, the index calibration and jogs, and for the needle sitting at the park (portal label "Microstep while moving"). Rounded down to a power of two. | Portal (admin), File |
| `microSlow` | 32 (1/32 step) | 1–32 | Microstep divisor while following the tuning knob (portal label "Microstep at rest"). The two portal labels are misleading: "at rest" here means following the knob, and the parked needle uses `microFast` (chapter 5, section 5.17). | Portal (admin), File |
| `reapHsps` | 60 hs/s | 10–500, step 10 | Creep speed of the index calibration (portal label "Measuring pass speed"); also the portal's jog speed. **Not** the homing re-approach. | Portal (admin), File |
| `sweepOn` | 0 (off) | 0 / 1 | A full end-to-end needle sweep each time the amplifier comes on. The home at the main board's start-up (the set plugged in, or a restart) ends with a full sweep whatever this says (chapter 5). | Portal (admin), File |
| `posMin` | −300 hs from the index | −3000 to 3000 | Low soft limit: the park position, and where `dialLow` is printed. The pair must bracket the index (0) and not cross; any other pair is refused and the old one kept. The range is wide on purpose, so that a wrong value is refused rather than silently stored at a bound. Reset to −300…+300 at start-up if corrupt. On this machine: −1156. | Portal (admin), File, S3 key `m`, **Set low limit here**, **Needle Limit ◀ 20** / **20 ▶** (moves both), Auto (the index calibration shifts it; the start-up clean-up) |
| `posMax` | +300 hs | −3000 to 3000 | High soft limit: where `dialHigh` is printed. Same rules. On this machine: +414. | Portal (admin), File, S3 key `M`, **Set high limit here**, **Needle Limit ◀ 20** / **20 ▶**, Auto |
| `idxOffFwd` | 0 hs | −500 to 500 | The index band: the sensor's OFF edge going up, from the index. A measurement, not a preference. | Auto (the index calibration: S3 key `k`, **Calibrate the index**), Portal (admin), File |
| `idxOnRev` | 0 hs | −500 to 500 | The index band: the ON edge going down. | As `idxOffFwd` |
| `idxOffRev` | 0 hs | −500 to 500 | The index band: the OFF edge going down. | As `idxOffFwd` |
| `idxOnFwd` | 0 | — | **Not a row and not in the file.** The homing edge of the band, zero by definition. An upload line `idxOnFwd=...` is refused as unknown. | The index calibration (always 0) |
| `dialLow` | 879 (87.9 MHz) | 500–2000 (tenths of a MHz) | The frequency printed at the low end of the dial face. The firmware puts it at the low soft limit (`posMin`), not at the mechanical stop. | Portal (admin), File |
| `dialHigh` | 1079 (107.9 MHz) | 500–2000 | The frequency printed at the high end of the dial face. The firmware puts it at the high soft limit (`posMax`). | Portal (admin), File |

**Why these defaults.**

- **`upVmax` 1100, not 1700:** moving up through the index faster than about 1100 hs/s, the needle
  mechanism stops following while the firmware keeps counting (measured: always at 1500, often at 1200 to
  1400, never at 1100). A physical value of this machine; the range still allows up to 4000.
- **`microFast` / `microSlow`:** the firmware makes the microsteps itself, with PWM on the four inputs of
  the motor's driver, which has no microstepping of its own (Hardware Bible, chapter 8). The row caps the
  divisor at 32.
- **`posMin` / `posMax` −300 / +300:** the provisional soft limits. They assume the needle's real travel
  extends more than 300 hs on each side of the index, and that the index band is narrower than about
  600 hs, so the sweep after a home crosses it.
- **`dialLow` / `dialHigh`:** this set's dial face prints 87.9 MHz at its low end and 107.9 MHz at its
  high end, and the firmware maps them onto the soft limits (`posMin`, `posMax`). Another set's builder
  types in their own, and sets the soft limits where the glass prints them.

### A.3.2 The tuning

Chapter 6 explains every row. `calLow`, `calHigh` and `ifOffset` are portal rows; the keys below them are
**hand-written keys**, in the settings file but not on the portal. Their upload rules are in chapter 7,
section 7.6.2.

| Key | Default | Range | Meaning | Changed by |
|---|---|---|---|---|
| `calLow` | 0 (a placeholder: "not measured") | −20000 to 20000 counts | Shaft count at the tuner's low mechanical end. Typing it sets bit 0 of `tunerEndsSet`. | Portal (admin), File, S3 key `c`, **Tuner = low end** |
| `calHigh` | 6023 (a placeholder) | −20000 to 20000 counts | Shaft count at the high end. Typing it sets bit 1. | Portal (admin), File, S3 key `C`, **Tuner = high end** |
| `ifOffset` ("Tube set IF") | 10.60 MHz (stored as `ifOffset20` = 212, in 50 kHz steps) | 10.00–11.50 MHz, step 0.05 | The tube set's IF. Turns the tuning chip's reading of the set's local oscillator into a station frequency. 10.60 MHz is the tube set's fitted FM IF (Hardware Bible, chapter 13). | Portal (admin), File |
| `bandLow` | 881 (88.1 MHz) | 500–2000, else REFUSED | The fallback straight line's low end, in tenths of a MHz. Nothing reads it once two samples exist. | File only |
| `bandHigh` | 1079 (107.9 MHz) | as `bandLow` | The fallback line's high end. | File only |
| `tuneOffset10` | 0 | −200 to 200 (tenths of a MHz, so ±20 MHz) | The hand offset (the dial correction), sliding the whole tuning curve. Commits with the marks. Never erased automatically. | **Dial −0.1 MHz** / **Dial +0.1 MHz** (0.1 to 5 MHz per request, ±20 MHz in total); reset to 0 only by **Reset the dial correction to 0**; File |
| `tuneUsed` | 0 | 0–0xFFFF; decimal or `0x`; always written | The mask: bit i set means slot i holds a sample. | **Mark station A/B/C here**, **Clear all marks**, **Drop one sample (slot 0-11)**, **Measure this dial position**, S3 key `V`, Auto (the automatic sampler), File |
| `tuneMark0` … `tuneMark11` | empty | `<shaft count within ±1 000 000>,<frequency × 10, 870–1085>` | The tuning samples. Slots 0 to 2 are the hand marks A, B, C; slots 3 to 11 are automatic samples. Raw shaft counts, so a sample keeps its meaning when the tuner ends are measured again. | As `tuneUsed` |
| `tunerEndsSet` | 0 | 0–3, else REFUSED | Bit 0: the low tuner end was measured; bit 1: the high end. Only 3 lets the curve use the measured travel. Worked out for files older than version 5 (chapter 7, section 7.6.2). | S3 keys `c` / `C`, **Tuner = low end** / **high end**, typing `calLow` / `calHigh` on the portal, File, the version-4 conversion |
| `spurUsed` | 0 | 0–16; always written | How many fixed features are stored. Empty by default on purpose: the list is only ever earned. | S3 key `o` (add the strongest candidate), S3 key `O` (clear), **Forget the fixed features**, Auto (the start-up clean-up), File |
| `spur0` … `spur15` | empty | 772–972 (oscillator frequency × 10); no two closer than 0.3 MHz | The **fixed features**: peaks the cabinet itself produces, left out when hunting the tube set's local oscillator. | As `spurUsed` |
| `lastAngle` | 0 | — | **Not in the file.** The last shaft count, which picks the right turn at start-up. | Auto: every 10 s when it changed, only while the angle sensor answers |

**Why these defaults.** `calLow` / `calHigh` 0 / 6023 are old compiled placeholders meaning "not
measured", not a claim about the shaft. The `spur*` range is the window the tuning chip searches, 77.2 to
97.2 MHz. That is because fixed features are learned at local-oscillator frequencies. The tube set's FM
oscillator runs below the station (Hardware Bible, chapter 13), and the window is the printed dial minus
the 10.7 MHz design IF (chapter 6).

## A.4 Network, accounts and stored state

These are stored but are neither portal rows nor lines of the settings file. All are the main board's.

| Key | Default | Range | Meaning | Changed by | Saved in |
|---|---|---|---|---|---|
| `wifiTxQ` | 8 (2.0 dBm) | 8–60 (quarter-dBm: 2.0–15.0 dBm) | The learned WiFi transmit power: the rung of the joining ladder that last worked. The ladder tops out at 15 dBm, a firmware choice. Printed by the console dump `D`; an old file's `wifiTxQ` line is skipped. Chapter 8, section 8.4. | Auto (copied from the WiFi driver on every pass of the main loop), S3 key `B` (cycles 60, 44, 34, 28, 20, 8; the first press after a start gives 44) | `amb3/cfg` |
| `ssid`, `pass` | empty | name up to 32 characters, passphrase up to 63 | The house WiFi network. Never written in the file. | Portal **Network** card, S3 key `y` | `net` |
| `ntp` | `pool.ntp.org` | a host name | The primary time server; `time.nist.gov` is a fixed second. | Portal **Network** card | `net` |
| `tz` | `EST5EDT,M3.2.0,M11.1.0` | a POSIX time-zone rule | The local time rule: the author's zone. Change it for your build (chapter 8). | Portal **Network** card | `net` |
| `users` | One administrator account with a compiled default password that the portal forces you to change | up to 4 accounts | The portal accounts, salted and hashed. Never written in the file. | Portal account cards; S3 key `~` erases every account and restores the default administrator (chapter 8) | `auth` |
| `magic`, `version` | `0xA838`, 7 | — | The header of the main board's settings blob. The version is written in the file as a comment. | The firmware | `amb3/cfg` |

The network and account stores are written at once, even while a new firmware is on trial (chapter 3,
section 3.11.5).

## A.5 Constants of the settings system

> **For firmware changes**
>
>
> These change only with a new build. The **Where** column names the source file, function or constant
> that holds each value.
>
> | Name | Value | Where | Meaning |
> |---|---|---|---|
> | `SETTINGS_MAGIC` | `0xA838` | `src/s3/main.cpp` | Marks a blob that has a header (version 2 and later). |
> | `SETTINGS_VERSION` | 7 | `src/s3/main.cpp` | The current layout of the main board's settings. |
> | `sizeof(Settings)` | 220 bytes | `src/s3/main.cpp` | The size of the main board's blob. |
> | Oldest layout | 100 bytes | `settingsLoad()` | A shorter blob is refused and the defaults used. |
> | Audio structure | 15 bytes | `include/proto.h` | `ProtoAudio`, the `amb/audio` blob. |
> | Bluetooth structure | 12 bytes | `include/proto.h` | `ProtoBtCfg`, the `amb/bt` blob. |
> | `TUNE_MARKS` | 12 | `src/s3/main.cpp` | Tuning sample slots: 3 by hand, 9 automatic. |
> | `CFG_SPURS` | 16 | `src/s3/main.cpp` | Fixed-feature slots, frozen into the layout. `Rda::MAX_SPURS` must equal it; a `static_assert` checks. |
> | `PROVISIONAL_LIMIT_HS` | 300 hs | `src/s3/main.cpp` | The default soft limits, and the ones the start-up clean-up restores, ±. |
> | Main-board save wait | 2000 ms | `settingsFlush()` | Quiet time after the last change. |
> | `lastAngle` save period | 10 s | the main loop | |
> | "Save settings now" retries | 40, 25 ms apart while the other core is writing | `settingsForceFlush()` | |
> | Reboot save retries | 15, 100 ms apart | `hReboot()` | None while the firmware is on trial. |
> | Audio-board save wait | 2000 ms | `settingsFlush()` (audio board) | |
> | End of the audio board's trial hold | 60 s after the main board's first greeting, or 5 min if the main board never spoke | `CONFIRM_AFTER_HELLO_MS`, `CONFIRM_AFTER_MS` (audio board) | Held changes are written at confirmation. |
> | Knob reading | every 50 ms, median of five | `updateVolumePot()` (audio board) | |
> | Knob deadband | 24 raw counts (about 0.6 % of travel) | `POT_DEADBAND` (audio board) | Smallest knob movement that changes the volume after the first reading. |
> | Knob calibration reading | median of nine | audio board | Taken by the **Pot** buttons. |
> | Knob calibration fallback | minimum 60, maximum 3990; used when the maximum is not at least 65 above the minimum | audio board | |
> | Volume law clamp on the audio board | 10–40 (gamma × 10) | `Audio::setTaper` | Applied when the setting is applied. |
> | Balance clamp on the audio board | −100 to 100 | `Audio::setBalance` | |
> | Gain clamp on the audio board | −60 to +30 dB | audio board | Applied to both gains when they are converted. Fade times are not clamped there; 0 acts as 1 ms. |
> | Upload timeout | 20 s | `data/portal.html` | |
> | Upload note | 512 bytes; each list names 5 keys of up to 20 characters | `settings_table.h` | |
> | Clamp report threshold | 0.0005 | `hSet()`, `settingsFromText()` | A larger change by the bound is reported as clamped. |
> | Mirror payload limit | 1088 bytes | `PROTO_MAX_PAYLOAD`, `include/proto.h` | Checked by `static_assert` for the audio, Bluetooth and combined structures. |
> | Portal rows | 58: 38 main board, 10 audio, 10 Bluetooth | `gSet[]` | |
>

## A.6 Experiments: actions, never saved

These portal **actions** change behaviour until the next restart, and are **never saved**. The reason:
"an experiment that survives a reboot is one whose result gets attributed to
something else a week later. If it turns out to be the fix it becomes a setting then, with a reason
attached." All are administrator only. Those marked "no button" are reached only by posting the action
directly to the portal (Appendix C).

| Action | Value | What it does | Button | Chapter |
|---|---|---|---|---|
| `sys.pot` | 0 ignore, 1 use | Ignore the front volume knob. | **Ignore the volume pot** / **Volume pot back on** | 10 |
| `sys.clkdrv` | 2–3, clamped | Drive strength of the audio board's three clock pins (GPIO0, 18 and 17) together. Its start-up value is 3. | no button | 10 |
| `sys.clkpair` | 22, 23, 32 or 33 only | Master-clock drive, then bit-clock and word-clock drive. | no button | 10 |
| `sys.dindrv` | 0–3, clamped | Drive strength of the data line. | no button | 10 |
| `sys.tail` | 0 / 1 | Sign-extended tail. | **Sign-extended tail ON** / **off** | 10 |
| `sys.zfloor` | 0 / 1 | Zero-data floor. | **Zero-data floor ON** / **off** | 10 |
| `sys.dctest` | 0 / 1 | Replace the output with a constant DC word: silent as audio, so anything heard is the fault. | **Constant DC test ON** / **off** | 10 |
| `sys.dcword` | index 0–6 | Pick the DC word, from a fixed list, all at or below −42 dBFS. | no button | 10 |
| `sys.bttx` | 0–5 (−12 to +3 dBm) | Bluetooth transmit ceiling, down only. | no button | 10 |
| `sys.quiet` | seconds; absent or 0 or less means 120; at most 600 | WiFi off; it returns by itself. | **WiFi off for 2 minutes** | 8 |
| `disp.test` | 0 normal, 1 show 8888, 2 blank | Display test; console keys `8`, `b`, `9` do the same. | **Show 8888** / **Blank** / **Normal** (Display tab) | 4 |

`sys.sddly`, a former experiment of this kind, sent the radio to full volume and is retired; do not
restore it (chapter 10, section 10.15.3).
