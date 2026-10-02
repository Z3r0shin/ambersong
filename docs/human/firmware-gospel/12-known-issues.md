# 12. Known issues, ideas and untested paths

No firmware is finished; this one has a list of things that were seen, understood and left as they are.
Only one of them stops the radio from working: an audio-board program built with the wrong protocol
version (section 12.5). Each is written down so that whoever meets it does not have to
rediscover it, and so that whoever wants to fix it knows where to start.

The chapter opens with an index: find what you see, and it sends you to the section that explains it,
in this chapter or another. Then come the issues left as they are, grouped by area, one row each: what
you see, why it was left, and where a fix would start. The "where a fix starts" column names functions
and files; only someone changing the firmware needs it. A starting point is a suggestion, not a design;
none has been tried.

The class in the last column is the recovery ladder of *About this book*:

- **NOTE** heals by itself or is only cosmetic.
- **DEFECT** needs a portal reboot, a console reset or the front switch.
- **BLOCKER** leaves the radio unusable through a power cycle, or reachable only by the USB cable.

Two issues here are BLOCKERs: that program (section 12.5) and, at worst, a blank board's access point,
if nothing but the USB console helps (section 12.4). The chapter ends with ideas for improvement, the
paths never exercised on the radio with a way to test each, and a tidy list for the code.

Everything here was checked against the main board's firmware v.1.0.5 and the audio board's v.1.0.4. The
placeholder portal account, the open rescue access point and the author's time zone are not faults but
things a builder sets: see the builder checklist, chapter 8, section 8.2.1.

## 12.1 Index by symptom

Find what you see; the second column gives the section that explains it (5.19 is chapter 5, section
5.19; Appendix H is the pop hunt).

| What you see | Where |
|---|---|
| **The needle** | |
| **NEEDLE OFF**, **NEEDLE RE-INDEXING**, **NEEDLE FAULT**, **NEEDLE HUNTING** | 1.5, 5.19 |
| The needle holds still while you tune | 5.13, 6.12 |
| The needle pinned at one end; the first search at power-up goes up and away; `*** LEDC REFUSED ***` | 5.19 |
| The needle frozen and the clock dark | 4.8, 5.19 |
| **Re-index** answers "re-indexing" but homes; steps lost after raising `upVmax`; no warning for a gain that will hunt | 12.3 |
| A portal jog stutters | 12.4 |
| **Tuning and the dial** | |
| A measurement REFUSED, DISCARDED, ABANDONED or NOTHING FOUND; QUADRATIC or MARKS REFUSED; "tuner ends NOT measured" | 6.12 |
| Every reading off by the same amount; one sample with a large residual in `s` | 6.12 |
| The top of the dial (107.9 MHz) cannot be measured | 6.15, 12.2 |
| Samples stored up to 50 kHz high; the fine-pass list shows a rejected broadcast; a garbled status line or a mixed sweep dump; the gap message names the tuner's ends | 12.2 |
| **Drop one sample** removed a hand mark | 12.4 |
| **The clock and the display** | |
| The clock blank or `--:--`; a battery-clock pill; the old time back after `W` | 1.10, 4.13, 9.10 |
| The clock blank with a working network, the audio board absent since start-up | 12.5 |
| Eastern North American time after `W` | 4.9, 8.2.1 |
| The wrong brightness after `-` or `=`; `0879`; a frequency shown while the angle sensor is dead; 3.3 s before anything starts | 4.16, 12.7 |
| The battery clock wrong from the year 2100 | 12.5 |
| **Sound and Bluetooth** | |
| Pops | 10.10, Appendix H |
| A click at the start of every audio-board update; a click with `stalls` rising; silence while the link and the blue lamp work | 10.11, 12.6 |
| Full volume or silence after a change to the audio interface | 10.11, 10.15.3 |
| Wrong pitch; rare drops; the volume does not follow the knob; a phone dropped; no phone finds the radio | 10.11 |
| "pairing window open", but nothing opens | 12.5 |
| Updates and phones refused (memory) | 10.8, 10.11 |
| Peak or ring readings missing on the portal after the audio board's `s` | 12.6 |
| One pop at every audio-board restart or update | 10.14 |
| **The network, the portal and the console** | |
| Off the house network, `Ambersong` appears; a phone joins but no page opens | 1.4, 8.11 |
| A blank board's access point cannot be joined; the portal slow while the needle moves | 8.11, 12.4 |
| The portal dead but the radio answers a ping; sign-in refused with a wait; the password lost | 8.11 |
| The portal dead while the radio, clock and needle work | 4.13 |
| The Network card's save rejoins; the access point confirmed only on the console; the reboot toast says only "rebooting"; an upload's count hidden; no "unsaved" sign | 12.4 |
| `one key at a time ...`, `console busy`; the display freezes and the sound mutes while a prompt waits | 8.11 |
| `y` shows the password; `i` says `*** LOW ...`; the first `B` gives 11 dBm; `D` cannot be pasted into a file | 12.4 |
| `i` says `OWN ACCESS POINT` while joining; `B`, `o` or `O` say "saved" too early; `q` or `v` refused with no reason | 12.8 |
| **Updates and versions** | |
| An upload refused (wrong chip, on trial, audio board not answering); the page says it failed, but it worked | 3.15 |
| "flash" on the page for a main-board file for the wrong chip; "No Error" after a refusal | 12.4 |
| `- an earlier update was ROLLED BACK` at every start-up; the sound off 20 s after a relay stopped | 3.15, 12.5 |
| `proto self-test FAILED - muted, BT closed, awaiting OTA`; silence and no handshake after an audio-board update | 9.10, 12.5 |
| The audio board's version on no portal page | 12.5 |
| The Firmware card says the needle "parks"; two builds cannot be ordered by stamp | 12.8, 12.7 |
| **Settings** | |
| **SETTINGS LOCKED**; a setting back at its old value after a power cut; the fixed-feature list empty | 1.11, 7.7 |
| Audio settings back at their defaults after an update | 7.7, 10.11 |
| `the audio board is not answering ...` (HTTP 409); `n/a` in a download | 7.7, 9.10 |
| An audio edit lost just before the audio board fell silent; an audio-board console change undone | 12.5 |
| "TUNING MARKS REFUSED - the file announced 0 and 0 parsed", or no word about the marks | 12.4 |
| **The link between the boards** | |
| `[WARN] A32 silent.`; `link SILENT`; **AUDIO BOARD SENDS NO STATE**; asleep for up to 80 s; reset reason 6 | 1.8, 9.10 |
| `[WARN] A32 silent` and the link pill red, yet the sound plays on | 9.10, 12.5 |
| **BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both** | 9.10, 12.5 |
| The audio board says `crc ok`, `[PASS] clock pads`, or `[ZDD] DAC left analogue mute`; its help line omits `k` and `g` | 12.8, 12.5 |

## 12.2 Tuning

Issues in the dial calibration and the tuner chip.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **The top of the dial cannot be measured.** The firmware sweeps the oscillator only between 77.2 and 97.2 MHz. At the default IF of 10.60 MHz, 107.9 MHz needs it at 97.3 MHz, one step outside, so a measurement there finds nothing or only the edge of the peak. Every station up to 107.8 MHz is inside. | The highest station receivable where this radio lives is 107.3 MHz, and the dial reads true up to there. | The range is compiled (`Rda::LO_WINDOW_LO10`, `LO_WINDOW_HI10` in `src/s3/rda.h`), from the old nominal IF of 10.7 MHz. Derive it from the dial's printed ends minus the IF setting. Then the fixed features just above 97.2 MHz (98.7, 101.6 and 104.4 MHz) must be learned like the others, and the start-up purge of out-of-range fixed features (`purgeCorruptSpurs()`) must follow the new range. `requestSweep()` clamps every sweep to the range; `requestRefine()` checks only the centre; `sampleStart()` refuses a position well outside. | NOTE |
| **Readings shared between tasks without a lock.** A status line read mid-rewrite mixes old and new text; a sweep dump (`a`, `/api/rda`) taken during a sweep mixes two sweeps. The fixed-feature list can be replaced while a sweep reads it: never seen; at worst one sweep judges one frequency against the old or the new list. | Cosmetic or theoretical, and a lock would add a new way for the tuner task and the portal to wait on each other. | One small mutex for the tuner task, or copy on write: build the status text and the bins privately, publish them with one index or pointer swap. For the fixed-feature list, make the count and the array `volatile` and publish with an atomic store, as `pushBt()` in `src/a32/audio.cpp` publishes its ring head. | NOTE |
| **The gap message names the tuner's ends** ("AS5600 gap - angle re-seated on the turn inside the tuner's ends") even when the ends are not measured and the nearest turn was used. | Wording only. | Have `seatTurn()` in `src/s3/needle.cpp` report which rule it used, and print "on the nearest turn" when the ends are not measured, or the shaft lies outside them. | NOTE |
| **Samples are stored to 100 kHz.** The fine pass measures in 50 kHz steps, but a sample is stored in tenths of a MHz, rounded up: an odd-twentieth result is stored 50 kHz high (an oscillator at 95.05 or at 95.10 MHz stores 105.7 MHz either way, with the default IF). About +25 kHz on average. | At most half a channel step, and the dial reads true. | Store `tuneF` in twentieths and fit the curve on those: a settings version bump, with a conversion that doubles every stored mark (chapter 7, section 7.5). The rounding is the `station10` line of `sampleStore()`. | NOTE |
| **The fine-pass list does not mark a broadcast.** A point rejected as a broadcast keeps its reading, under the legend "fine pass (50 kHz; 0 = skipped)", and looks like a candidate that lost. The `fine` list in `/api/rda` has the same gap. | The choice itself is right; only the printout is incomplete. | Keep a stereo flag per fine point, as the coarse bins do (`binStereo()`), and print "(STEREO - a broadcast)" after such a point; or complete the legend. | NOTE |
| **The comb's source is unknown.** The tuner chip sees fixed features at multiples of 2.8224 MHz. One suspect, the audio board's I2S clocks, which run even while it sleeps, has never been tested. | The notches work; the comb costs only part of the window (chapter 6, section 6.15). | Add an experiment action, reset at start-up like the others, that stops the I2S peripheral for a set time (about 90 s) and restarts it. Sweep inside that time and compare the `a` list with one taken with the clocks running. I2S is set up in `Audio::begin()` and fed by `audioTask()`. | NOTE |

## 12.3 Needle

Issues in the needle's motion and its portal buttons.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **A direct write of the position can race a pending correction.** Never seen; a hypothesis from reading the code. The step task owns the position and folds corrections in at the top of every pass, but the index calibration's start and finish, the microstep change, the coil release, the jog, the low-stop assumption and the start-up position still write it directly. A correction landing during such a write would be lost, and the needle would run in a frame off by a few to a few hundred half-steps. | Rare, never seen, and it corrects itself: the next index crossing, the idle check or a re-index finds the error. | Make the step task the only writer: add a "set position to" request that `stepTask()` applies at the top of its pass, next to the fold. Simpler: have each writer wait until `gCorrectHs` reads 0, as `homingTick()` does. The writers are `calFinishBand()`, `calStartBand()`, `useMicro()`, `coilRelease()`, `jogRaw()`, `assumeAtLowStop()` and `bootPosition()`, in `src/s3/needle.cpp`. | DEFECT at most |
| **"Measuring pass speed" says less than it does.** The row (`reapHsps`, default 60 half-steps/s) sets the index calibration's creep **and** the portal's jog. It does not set the homing re-approach, which is not a setting. | Judged not worth changing. | Rename the label, for example "Calibration creep and jog speed". The key stays, so settings files keep working. | NOTE |
| **No warning for a needle gain that will hunt.** The start-up check of `wn` against its ceiling runs before the stored settings are applied, so it only checks the compiled defaults. A stored or portal value above the ceiling is never warned about. | Judged not needed. | Call the same check (`wnCeiling()`) from `applySettings()` after the needle settings are pushed down; print the warning and return it to the portal as a `warn`. | NOTE |
| **The portal's needle buttons run on the portal task** (core 0) and write the needle's state while the needle task runs on core 1; only stop and abort are handed over safely. A jog also holds the portal for its whole length, up to about 20 s at the lowest `reapHsps`; nobody gets an answer meanwhile. | A known cross-core class, deferred: the buttons are used by one person at a time, with the needle at rest. | Turn each action into a request that the needle task picks up at the top of its pass, as `Needle::stop()` already works, and answer the portal from its result. The jog becomes a request that returns at once. | DEFECT at most; the jog's hold is a NOTE |
| **Re-index answers "re-indexing" when it homes.** On a needle with no zero it runs a plain home. Its refusal always says "a calibration or a bring-up tool has the needle", also when the needle is homing or sweeping. | Not yet addressed; the console says what really happened. | Have `startReindex()` return a reason string, as `startHoming()` does, and a flag saying it homed; then answer "homing - there is no zero to re-index from". | NOTE |
| **`upVmax` accepts speeds that stall the needle:** up to 4000 half-steps/s, while this needle stalls through the index far below that. | Not yet addressed. The default, 1100, is the measured limit of this mechanism. | Lower the row's maximum to the measured stall speed (chapter 5, section 5.18), and note that another mechanism needs its own ceiling. | DEFECT at most: the crossing check finds the lost steps and re-indexes |

## 12.4 Settings, portal and console

Issues in the stored settings, the web portal, the network and the main board's console.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **`i` calls normal powers "LOW".** It adds `*** LOW - the transmitter is turned down ***` to any value under 10 dBm, but the ladder's rungs 8, 20, 28 and 34 (quarter-dBm) are normal. | A bench line; its comment already says to check the value against the rung. | In `Net::driverReport()`, compare the driver's value with the rung asked for; flag only a difference, or a zero. | NOTE |
| **The first `B` after a start-up gives 11 dBm.** `B` keeps its own place in its list from start-up, so its first press always sets 44 (11 dBm), whatever the radio was running at, and saves it. | A bench key; the printout shows the new value. | Start from the rung in force (`cfg.wifiTxQ`) and step to the next lower entry. | NOTE: a later press, or the ladder, changes it again |
| **The portal is starved while the needle moves.** The step task runs at high priority on core 0 and does not yield during a move. The portal shares that core. An audio-board update is relayed by the portal task: the likely cause of four failed audio-board updates on one early day. | Accepted: the step timing comes first, and the portal is not needed while tuning. | Keep the needle at rest during an audio-board update (the workaround in use). For a fix, emit steps from a hardware timer or the RMT peripheral so the step task can sleep between steps, and measure the step jitter before and after. | NOTE: it ends when the needle stops |
| **A settings write can land while the needle moves.** "Still" is judged by the needle's speed, which reads 0 in the pause before a move and during a portal jog, so a write can land then and make the jog stutter. The forced save also retries up to 40 times without asking again. | Not yet addressed; the stutter is brief and the step task catches up. | Judge "still" by the needle's state (no move, no dwell, no jog), not its speed, and call `writeSafe()` again before each retry. | NOTE |
| **No lock around the settings across the two cores.** Known races, each of microseconds: a hand mark and an automatic sample at the same instant; an upload half applied when a background save copies the settings, followed by a power cut within about 2 s; a soft-limit edit crossing an apply. None seen. | The risk was accepted rather than rebuild the settings around a lock (chapter 7, section 7.11.2). | One mutex held by every writer of `cfg` and by the copy in `settingsWrite()`; or post every portal change to the main loop as a request, so only one core ever writes. | NOTE |
| **No "unsaved" sign on the page**, for example during the trial minute after an update, when nothing is saved. The page shows only the lock. | Not yet addressed. | Add an `unsaved` field to the state the page polls, and a pill "CHANGES NOT SAVED YET" naming the reason the firmware already keeps for the **Save** button. | NOTE |
| **A settings upload with a warning hides the count:** the page shows the warning, not "N settings applied". | Not yet addressed. | Show both: the count, then the warning. | NOTE |
| **An unreadable `tuneUsed` line.** It is skipped with no note, and so is every mark line after it; the stored marks are kept. In a downloaded file the page then says "TUNING MARKS REFUSED - the file announced 0 and 0 parsed; the stored calibration was kept", which is misleading. In a file with no `tuneOffset10` line, nothing is said. | Not yet addressed; nothing stored is lost. | Remember that a `tuneUsed` line was seen but unreadable, as the hand-offset line already does (`mkOffBad`), and refuse the block in its own words: "the tuneUsed line could not be read - the file's samples were not applied". | NOTE |
| **The dump `D` is only partly a settings file.** Its first lines carry several pairs each, which an upload refuses; only its tuning, IF (`ifOffset=`) and fixed-feature lines paste as they are, and its `wifiTxQ` line is ignored. | A bench view; the portal's **Download** is the settings file. | Print one pair per line, or the same text as the download. | NOTE |
| **Reboot the main board shows only "rebooting".** On trial the radio answers "rebooting - this firmware was still on trial, so the previous firmware comes back", and the page drops that sentence. A refusal is shown in full. | Not yet addressed. | In the page's `#rb` handler: `toast(r.m \|\| 'rebooting')`. | NOTE |
| **Two main-board update messages.** An upload refused during the trial also prints the updater's "No Error" on the console. An upload refused for the wrong chip gives its reason on the console only; the page shows "flash". | Not yet addressed; both refusals work. | Skip `Update.end()` when the upload was refused or never began. Keep a reason string for the main board's own image, as the relay does with `a32OtaSetError()`, and send it from `hOtaEnd()`. | NOTE |
| **Saving the Network card always rejoins**, 3 s later, even for an NTP server or time-zone change alone, because the card always sends the network name. | Not yet addressed; the rejoin takes a few seconds. | Have `Net::setWifi()` skip the rejoin when the name and passphrase are unchanged, or send them only when they changed. | NOTE |
| **A portal-raised access point is confirmed on the console only.** | The console is readable on the **Console** tab. | Carry the access point's state in the page's state, so a browser that joins it sees a pill. | NOTE |
| **A blank board's access point transmits at 2 dBm.** The ladder moves only on failed joins, and a board with no network has none. On this machine an access point at 2.0 dBm was visible at 44 % but could not be joined; on another board it may be fine at close range, untested. | On this machine the rung was learned long ago. | Raise the access point at a fixed rung known to be joinable (15 dBm here), or step the ladder while the access point is up and nobody has joined. | NOTE when moving closer works; BLOCKER only if nothing but the USB console helps. A blank board has just been flashed by USB, and `y` sets the network over the same cable |
| **`y` echoes the WiFi password**, onto USB and into the console ring, which any administrator with the **Console** tab open can read. | Accepted for this machine; a builder may judge otherwise. | In `setWifiInteractive()`, echo the network name, then a `*` for each character after the space. | NOTE |
| **Drop one sample also drops hand marks.** It accepts slots 0 to 2, the hand marks A, B and C. | It is the only way to remove one hand mark without clearing them all, so it may be wanted; the label is what is loose. | Relabel it ("Drop a mark or sample, slot 0-11"), or refuse slots 0 to 2 there and add a separate "Clear mark A/B/C". | NOTE |
| **`sys.quiet` casts its number unchecked.** Nothing in practice: a value of 65 536 or more is undefined in C++; on this compiler 65 536 becomes 0, which turns into the 1 s minimum, and radio silence is capped at 600 s anyway. | Harmless. | Clamp the argument to 0..600 before the cast in `doAction()`. | NOTE |

## 12.5 The link and the audio board

Issues in the link between the boards and in the audio board's own firmware.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **An audio-board image built with the wrong `PROTO_VERSION` confirms itself** after five minutes, because frames of another version count only as wrong-version and the S3 looks absent. It becomes permanent; the link stays down, with no radio or Bluetooth sound, until the main board matches. The main board shows it (`wrong-version` in `s`, the pill, `linkver`); the audio board counts it but prints it nowhere, and its `s` shows only `link SILENT rx 0`. | By design: it lets a protocol change roll out one board at a time (chapter 9, section 9.14.5). | If it ever bites: count wrong-version frames on the audio board too, and refuse the five-minute path while they arrive, so the image stays on trial and rolls back at the next reset. That removes the one-board-at-a-time path, so the protocol procedure would need another way. | BLOCKER: radio and Bluetooth stay silent through a power cycle once the audio board has confirmed itself. Recovery: update the main board built with the audio board's version, then both together; no cable |
| **No network time without the audio board.** If the audio board is absent or silent from start-up, the main board never takes network time either, because that path writes through to the battery clock and waits for the handshake. The clock stays blank even with a working network. | Accepted: with the audio board down there is no radio or Bluetooth sound either, and the clock comes back with the link. | Let network time set the main board's software clock (`epochAtSync`, `millisAtSync`, `timeValid`) without the handshake, and send the write-through only when the audio board answers. The block is in the main loop, gated on `peerHello`. | NOTE |
| **A one-way link failure leaves the audio board on a stale amplifier state.** If only the audio-board-to-main-board direction fails, the main board hears nothing, drops the handshake and from then on sends only HELLO, once a second. The audio board still receives those HELLOs, so for it the main board is alive: it stays awake on the last amplifier state it was told. The sound plays on, and if the amplifier is switched off it never learns it (Bluetooth stays connectable on the Bluetooth source). Everything recovers by itself when the line works again. Never seen on the radio (chapter 9, section 9.10). | Not yet addressed; it needs a broken wire to happen. | Have the audio board count only `SET_SYS` (not HELLO) as "the main board is talking", or have the main board keep sending `SET_SYS` while it tries to handshake. The sending side is the link keep-alive block of the main board's `loop()` (`SET_SYS` and `PING` go only while handshaken); the waking side is the audio board's `serviceWake()` (awake = amplifier on and the main board alive). | NOTE |
| **`autoConnect` is still in the protocol**, stored by the audio board and read by nothing: the radio never starts a Bluetooth connection itself. Old settings files that carry the key still upload. | Removing the byte changes the structure's size. That needs no new `PROTO_VERSION`: until both boards carry it, each refuses the other's Bluetooth-settings messages and everything else works. But the audio board then resets its stored Bluetooth settings to defaults, and both boards need the update. | Remove it the next time `ProtoBtCfg` has to change anyway, so the reset is paid once. The audio board has no conversion: any size change puts that structure back to its defaults (chapter 7, section 7.5.5). | NOTE |
| **"pairing window open", and nothing opens.** **Pair a new device** (`bt.pair`) sends a message named `MSG_BT_LOOK`, which opens the pairing window (the state `BT_LINK`, not `BT_LOOK`), and only on the Bluetooth source. The portal answers "pairing window open" on any source. | Renaming a message touches both boards and the protocol header. | Rename it (for example `MSG_BT_PAIR`); the value stays, so `PROTO_VERSION` need not change. Have `bt.pair` refuse with "switch to Bluetooth first" when the audio board's reported source is not Bluetooth. | NOTE |
| **The audio board's help line omits `k` and `g`.** Both keys work (Appendix D). | Bench keys, reachable only with a USB cable on the audio board. | Add both to the `keys:` line printed in `setup()`. | NOTE |
| **The audio board's version is on no portal page.** It appears on the main board's console at each handshake (`[PASS] A32 up: ...`) and in the boot report. | Not yet addressed. | Add the audio board's version (and its commit, from the boot report) to the page's state, and show it on the System tab beside the main board's. The value is `gLink.peerVersion`. | NOTE |
| **A relay that fails mid-image sends no abort.** The audio board stays muted until its own 20 s give-up, then restores its sound and keeps its old firmware. A wrong chip, a dropped connection, a stalled upload and a missing final answer do send the abort. | Judged not a problem. | Call `a32OtaAbort()` in `hOtaA32Upload()` the moment `a32OtaChunk()` fails. | NOTE |
| **An audio edit made just before the audio board falls silent is lost.** The refusal starts only once 2 s of silence have cleared the main board's copy; an edit in those 2 s is accepted, sent to nobody, and replaced by the audio board's own settings when it returns. | Not yet addressed; the window is short and needs the audio board to fail at that moment. | Have the audio board acknowledge each settings message, and report the edit as not applied when no acknowledgement arrives. | NOTE: nothing is corrupted |
| **The save before a commanded restart is tried once.** When the main board asks the audio board to restart, the audio board forces a save; if that write fails, it restarts anyway and the change is lost. | Not yet addressed; a failed write has never been seen. | Retry the forced save a few times, and send a log line to the main board before restarting if it still fails. | NOTE |
| **Audio-board console changes are undone.** The volume law (`t`, `T`) and the knob calibration (`p`, `c`, `P`) set on the audio board's console are saved there, but the main board's copy is not refreshed, and the next portal edit of any audio row sends the older volume law back. **Re-read the A32** (`sys.getcfg`) refreshes the copy. | Bench keys, reachable only with a USB cable on the audio board. | Have the audio board send its settings to the main board after any console change, as it answers `MSG_GET_CFG`. | NOTE |
| **A broken audio-board image on trial says the wrong thing.** An image that failed its link self-test announces "proto self-test FAILED - muted, BT closed, awaiting OTA", and at start-up "link and OTA kept up so this build can be replaced". On trial, an update is refused; only a reset, which rolls it back, gets out. The words are true only for an image flashed by USB. | The behaviour (never confirmed, rolled back by the next reset) is accepted; only the words are wrong. | When the image is on trial, announce "reboot the audio board to go back to the previous firmware" instead. | NOTE |
| **The battery clock's temperature is sent and not used.** Every time message carries it; the audio board prints it once, in its own USB banner; the main board never reads it. | Not yet addressed. | Show it in the portal, or drop the field at the next protocol change that has to happen anyway (see `autoConnect` above). | NOTE |
| **The battery clock is wrong from the year 2100.** The audio board reads and writes the year as two digits from 2000 and ignores the chip's century bit. | Not yet addressed. | Read and write the century bit in `Rtc::read()` and `Rtc::write()`, or accept the limit and say so. | NOTE |
| **"ROLLED BACK" stays in every start-up report** of both boards until the next update replaces the invalid image, so an old rollback can look like a new one. | Not yet addressed. | Remember in flash that a rollback was reported and print it only at the first start-up after it; or print the invalid image's version. Both boards use `esp_ota_get_last_invalid_partition()`. | NOTE |

## 12.6 Sound

Issues in the sound path.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **A click at the start of an audio-board update** when the fade-out is longer than about 340 ms. The audio board mutes, then waits a fixed 400 ms before it writes flash, which stalls the audio task. The wait covers the fade-out, one audio block and the 46.4 ms output buffers. The default fade-out is 150 ms; the portal allows up to 5000 ms. | The default is well inside the wait, and an update is rare. | Make the wait follow the setting, the fade-out plus about 60 ms, staying well under the 20 s the main board waits for the audio board's answer; or cap `fadeOutMs` at 340 ms. The wait is the `delay(400)` in the `MSG_OTA_BEGIN` case of the audio board's `onMessage()`. | NOTE |
| **The stall click.** The audio task is sometimes frozen for 23 to 111 ms, and a click can be heard. The cause was never measured; the leading guess is a flash write, which turns the flash cache off and stalls every task that runs from flash. | A deeper output chain would ride through it, but costs memory the audio board does not have (chapter 10, section 10.8). | Measure first. The stall detector already records each stall's length and time (`stalls`, `stlast`); record the time of every flash write on the audio board beside it. If they line up, keep flash writes away from playing moments, as the main board keeps them away from the moving needle. | NOTE: the sound comes back by itself |
| **Silence while everything else works:** the audio task is not watched by the watchdog. If it wedged, the main loop, the link and the portal would stay alive with no radio or Bluetooth sound, and nothing would restart the audio board. Never happened. | Not yet addressed. | Subscribe the audio task to the watchdog (`esp_task_wdt_add()`) and feed it once per block. The 15 s timeout is far above the longest stall seen. | DEFECT: **Reboot the audio board** fixes it |
| **The audio board's `s` steals the portal's readings.** It reads the peak meter (with its clip flag) and the ring's high-water mark, and reading resets them, so the portal misses them for one period. It also clears the I2S latches, but the portal reads `i2sSticky`, a copy nothing clears. | A bench key, reachable only with a USB cable on the audio board. | Give the console a read that does not reset (a "peek"), and leave the resetting read to the state message. | NOTE |
| **No live figure for the audio board's free memory.** The one figure, about 24 KB, was measured once. `s` prints only `ESP.getFreeHeap()`, which is not what a plain or a DMA allocation can get; the Bluetooth library prints the right figure only at start-up, on USB; the portal's `heap` is the main board's. | Not yet addressed; no fix chosen. | Print `esp_get_free_heap_size()` and `heap_caps_get_largest_free_block(MALLOC_CAP_DMA)` in `s`. Carrying them in `ProtoState` puts them on the portal; a new field goes at the end of the structure (chapter 10, section 10.15). | NOTE |

## 12.7 Main board, clock, display and build

Issues in the main board's start-up, its clock display and the build.

| What you see | Why it was left | Where a fix starts | Class |
|---|---|---|---|
| **About 3.3 s before anything starts.** At every start-up the main board waits up to 3 s for its USB console port to open, then 300 ms more, before the display, the needle and the link start. In the radio no USB host is attached, so every start-up pays it all. | It keeps the start-up report whole when a cable is attached. | Shorten the wait, or start the display and the link first and print the banner when the port opens. The web console's ring already keeps lines for a late reader. | NOTE |
| **Readout mode 1 shows a stale frequency.** With the readout on "Tuning always", the digits show the frequency of the last angle count even when the sensor has stopped answering. Mode 1 is deliberately not gated on the amplifier and the source; the sensor is a separate matter. | Not yet addressed. | When `Needle::i2cOk()` is false, show the placeholder the clock shows when it has no valid time. | NOTE |
| **The frequency readout shows a leading zero** below 100 MHz (`0879` for 87.9 MHz). The clock follows `blankLeadZero`; the readout does not. | Not yet addressed. | Pass `true` for the leading-zero blank in the `Display::showNumber()` call of `updateDisplay()`, or follow `blankLeadZero`. | NOTE |
| **The console's `-` and `=` (or `+`) bypass the fade.** They change `brightOn` by 15 and write it straight to the display, also while the amplifier is off, so the clock can sit at the wrong level until the next amplifier change, or step back briefly before it eases. They also skip the settings table's bounds, though their own floor and ceiling (0 and 255) match today. | Bench keys; the portal's brightness rows take the normal path. | Route both keys through the settings table's setter for `brightOn`, the portal's path, so the bounds and the fade apply. | NOTE |
| **Version stamps cannot always be ordered.** The stamp comes from the build PC's local clock, with no time zone; two builds made in different zones, or across a daylight-saving change, cannot be ordered by stamp alone. | Every build so far was made on one PC. | Stamp in UTC and add a `Z` in `scripts/version.py`. The string must still fit the 24-byte version field of the handshake (chapter 3, section 3.4). | NOTE |

## 12.8 Texts that say something false

These texts tell a reader something that did not happen. None changes what the radio does. All are NOTE.

| Where you read it | What it says | What is true | Fix |
|---|---|---|---|
| The portal's Firmware card | "The needle parks and the display goes dark while an update writes" | The needle stops where it is; a park is a different move. | "The needle stops and the display goes dark". |
| Console `B`, `o`, `O` | "saved" ("known, saved", "the clear is saved") | They only mark the settings changed. The write follows about 2 s later once the needle is still, and never while an image is on trial or the settings are locked. | "will be saved". |
| Console `i` | `OWN ACCESS POINT` and the access point's name whenever the main board is not joined | Also during a join, or radio silence, when no access point is up. | Print the real state: joining, silent, or access point up. |
| The answer to `POST /api/net` | "network saved - the radio joins it in a few seconds", also for a request with only the NTP server and time zone | Only a direct call to the API sees it, because the page always sends the network name (section 12.4). | Answer by what was set. |
| Console `q`, `v` | "sweep refused - see the RDA state in 's' (running, absent, or lost)" | A request lying wholly outside the oscillator window is refused too, and leaves nothing in `s` to see; `v` reaches it when the dial sits outside the window. | Have `requestSweep()` give its reason. |
| The audio board's console | "[ZDD] DAC left analogue mute" | The code only models that event, from a theory the pop hunt excluded (Appendix H). | "[ZDD] a zero run long enough for the DAC's zero-data mute has ended". |
| The audio board's console | "OTA wrote N bytes, crc ok" | Printed also when the main board sent no checksum and none was checked. | Print "no crc sent" in that case. |
| The audio board's start-up line | "[PASS] clock pads at drive N" | PASS whatever the pads read back, also a value other than 3, or 0xFF when the three pads disagree. | Print FAIL unless it reads 3. |

## 12.9 Ideas for improvement

These are not faults: improvements considered after v.1, with a starting point.

### 12.9.1 Check left and right, and let the portal swap them

**The idea.** The audio board writes the right speaker's sound into the first I2S slot and the left's into
the second, the reverse of the usual order, because that gave correct stereo on this machine. The author
knows that the amplifier sends each channel to the right speaker; whether the audio board and the DAC
deliver them the right way round has never been tested. The channel order is a setting of this machine
that another builder will need to check (chapter 10, section 10.3.4), and a portal switch to swap it on
the fly would make that easy.

**The test still to run.** Play a left/right test track over Bluetooth and listen to which speaker speaks.
That settles the Bluetooth path only. The radio path copies the converter's two slots straight across,
and with mono sum on (the default) both slots carry the same sound, so the radio's order matters only with
mono sum off.

> **For firmware changes**
>
> **Where.** `audioTask()` in `src/a32/audio.cpp`: the Bluetooth path writes `.r` into slot 0 and `.l` into
> slot 1, and the radio path copies the input slots in order. The channel format is set in
> `Audio::begin()`; the order is described at the top of `src/a32/audio.h`.
>
> **How.**
>
> 1. Add a swap flag to the audio settings (`ProtoAudio` in `include/proto.h`).
> 2. In `audioTask()`, swap each frame's two words after the volume, balance and meter pass and before
>    `i2s_write()`. Balance and the level meters then keep meaning the right and left speakers.
> 3. Add the row to `src/s3/settings_table.h`, and it appears in the portal.
>
> **What the new field costs.** It changes `ProtoAudio`'s size. A size change needs no new `PROTO_VERSION`
> (chapter 9, section 9.14.5): a message of the wrong length is refused, so a half-updated pair loses only
> the messages that carry `ProtoAudio`, and the link stays up. Until both boards carry the new structure, the
> audio board refuses every `MSG_SET_AUDIO` (volume, balance, fades and the rest do not reach it), and the
> main board refuses the audio board's `MSG_CFG` (its copy stays unknown, so the portal refuses audio
> edits). So:
>
> - **Flash both boards:** the audio board first, through the main board (updates do not use
>   `ProtoAudio`), then the main board.
> - **Download a settings file first and upload it afterwards.** The audio board accepts a stored structure
>   only if its size matches exactly, so its stored audio settings go back to their defaults once.

### 12.9.2 An equaliser

There is none; one was considered together with the swap.

> **For firmware changes**
>
> It would go in the same pass of `audioTask()`
> that applies volume and balance, and would cover the radio and Bluetooth only: AUX never passes through the
> audio board. A few biquad filters per channel, with fixed-point coefficients worked out on the main board
> (or when the setting changes) and sent as settings. The audio board is short of memory and its audio task
> runs under a tight deadline (chapter 10), so measure the task's time per block and the free memory before
> and after. The pop hunt showed how much this chain cares about timing.

## 12.10 Untested paths

Each of these is in the code and has never run on the radio. Nothing has gone wrong; each needs a fault, a
state or a set-up the radio has not had. The class of each path if it fails is in the *When it goes wrong*
table of its chapter. "Test build" means the method of the rollback tests (chapter 3, section 3.19): a
build made to fail in one chosen way, never committed, sent over the air with the USB cables unplugged, and
watched through its rollback; one build per path.

| Area | Path never run on the radio | How to test it |
|---|---|---|
| Tuning | The discard of a measurement after a bus error on the tuner chip; the "lost" path; a re-probe after a real loss | On the bench, break the tuner chip's bus during a sweep and watch the console for the discard and the ABANDONED line; restore it and watch for the re-probe. |
| Tuning | The replacement of a sample in a full table | Fill the table to twelve samples and take one more. |
| Tuning | A measurement showing both the coarse and the fine lists | Run `a` straight after a measurement. |
| Needle | **Re-index** on a needle with no zero | From the portal: **Re-index** after a failed home. |
| Needle | A portal stop or abort in the middle of an index calibration | From the portal, during **Calibrate the index**. |
| Needle | A slip of more than 40 half-steps found at a crossing, driving the re-index ladder; the three-failure FAULT; any homing FAULT since the step budgets were fixed | They need lost steps, which a test build can fake by shifting the counted position. |
| Needle | The position memory after a crash, and the detection of a torn record | The crash: a test build with a deliberate crash. A torn record: no method recorded. |
| Needle | The turn chosen by the measured tuner ends after an angle-sensor outage of more than half a turn | No method recorded. |
| Settings | The `n/a` rows of a download and the HTTP 409 refusal of audio edits, while the audio board is silent | A test build of the audio board that stops sending for a minute. |
| Settings | The downgrade lock, its complete-file unlock, the NOT SENT note and the refused download | A test build of the main board with a higher settings version, left to confirm itself and save, then replaced by the release image over the air. A rollback cannot do it: nothing is saved on trial. |
| Settings | A failed audio-board save and its retry; a failed write at the audio board's confirmation | Needs a flash fault; no method recorded. |
| Network | The rescue access point coming back after a failed join, on the current retry path | Needs a network that refuses the radio. |
| Network | The placeholder gate and the whole first-time path (chapter 8, section 8.2.2) | A spare board with the release image. |
| Network | The lockout beyond its first steps; the "full" answer to a fifth account; session eviction with more than four browsers | No method recorded. |
| Network | A phone joining the open access point and opening the sign-in page by itself. The main board's side has been seen working. | Raise the access point from the portal, join `Ambersong` from a phone, and expect the sign-in page to open by itself. |
| Link | The audio board's five-minute "the S3 never spoke" confirmation; the wrong-version count and pill with a real mismatch (they have only read 0) | A test build with another `PROTO_VERSION` on the audio board only. |
| Link | The degraded mode after a failed link self-test, on either board, and the rule that such a build is never confirmed | A test build with the self-test forced to fail. |
| Link | The two battery-clock pills in a real fault (`rtc` 1 or 2) | A test build of the audio board that reports an invalid time, or does not answer the time request. |
| Recovery | A main-board image that runs but never earns confirmation (portal or network down); a confirmation write that fails; a reset during either board's trial with a setting changed in it; a hang of the needle supervisor; a hung portal task | One test build per path. |
| Sound | Bluetooth staying invisible in RADIO mode, checked with a phone; to be repeated after any library update. **The check still pending.** | With a phone, on RADIO (chapter 10, section 10.7). |
| Sound | The channel order | A left/right test track over Bluetooth (section 12.9.1). |
| Sound | `sys.clkpair`: the master clock and the frame clocks driven at different strengths | Chapter 10, section 10.10, and Appendix H, section H.7. |
| Sound | The volume-swing test: audio-board volume 47 against 94 with a tone streaming, to see whether pops follow the signal level | Chapter 10, section 10.10. |
| Sound | The Bluetooth transmit-power A/B that would prove the 0 dBm start-up ceiling; a drive-3 pop count with Bluetooth idle | Appendix H, sections H.7 and H.8. |
| Sound | A phone negotiating 48 kHz; `connectable` off | Each waits for a listening session with the right phone or set-up (chapter 10, section 10.14). |
| Sound | A long session measuring the drift between a phone's clock and the local 44.1 kHz. There is no rate adaptation; the 186 ms ring absorbs only jitter. | A long listening session over Bluetooth. |

## 12.11 Code tidy list

> **For firmware changes**
>
> None of these changes what the radio does; they mislead a reader of the code. **Comments:** correct them
> in one comment-only commit, and prove it by stripping the comments from each file and comparing with the
> previous commit. **Unused code:** remove it in one commit; the build then proves nothing used it.
>
> **Comments that say something the code does not do.** The code is right.
>
> | Where | What the comment says | Fix |
> |---|---|---|
> | `src/s3/rda.cpp`, the stereo bits (`gBinST`) | "See isStereoBin()"; the function is `binStereo()`. | Rename it in the comment. |
> | `src/s3/rda.cpp`, the broadcast comment in the fine pass of `rdaTask()` | "see candidate()"; it is `bestCandidate()`. | Rename it in the comment. |
> | `src/s3/rda.cpp`, `requestSweep()` | "three of the five known fixed features" lie outside the window; the window's comment in `src/s3/rda.h` lists four (98.7, 101.6, 104.4 MHz and the 76.2 MHz band edge). | Make the two agree. |
> | `src/s3/needle.cpp`, `readAs5600()` | The gap warning "still says 'nearest turn' either way"; it prints "on the turn inside the tuner's ends" (section 12.2). | Correct it with that fix. |
> | `src/s3/needle.h`, `setMicro()` | `moving` is the division used for homing; no homing path selects a division, so a home started from tracking runs at `microSlow`. The comment above `coilRelease()` already says the division is set only for a sweep, tracking and a park. | Correct the header, or have homing select `moving` if that was the intent. |
> | `src/s3/needle.cpp`, `calFinishBand()` | `cfg.idx*` "is never written by anything"; the main loop now writes them from `calTakeBand()`. | Put it in the past tense. |
> | `src/s3/needle.cpp`, the band snapshot at the top | `applySettings()` runs `setIndexCal(0,0,0,0)`; true only before an index calibration has been saved. | "with the stored values, zero before the first band is saved". |
> | `src/s3/main.cpp`, `updateDisplay()` | "The timezone is set ONCE, by Net::begin()"; it is also set by `Net::setNtp()`, by `startNtp()` in `src/s3/net.cpp`, and by the console `W`. | List the setters. |
> | `src/s3/main.cpp`, the `B` case of the console | Its list has an 80 level "applied as 60"; there is no 80, and after 8 it wraps to 60. | Drop the sentence. |
> | `src/s3/main.cpp`, `settingsDump()` | Its `wifiTxQ` line is "BYTE-FOR-BYTE WHAT settingsToText WRITES"; the settings file no longer carries `wifiTxQ`. | Say the line is for reading only. |
> | `src/s3/main.cpp`, above `settingsDump()` | "one-pair lines from dialLow on"; the `dialLow` line carries two pairs. | "from tuneOffset10 on", or split the line. |
> | `src/s3/net.cpp`, the join-failure comment in `Net::loop()` | "This file opens with 'THE RADIO MUST NEVER BECOME UNREACHABLE'"; that text opens `src/s3/net.h`. | Name `net.h`. |
> | `src/s3/main.cpp` and `src/s3/needle.cpp`: `seatTurn()`'s banner, the `calLow` comment, `SMP_MAX_DRIFT`'s comment | The tuner's travel is "about 2300 counts"; the measured span is 2197, about 2200. | Write about 2200. |
> | `data/portal.html`, the `data-a` click handler | `tune.drop` uses "the same frequency prompt"; it has its own slot-number prompt. | Say so. |
> | `src/a32/audio.cpp`, "THE TWO KNOBS" | The data-line output delay function "remains only for the A32's own USB console"; the setter and its key are gone, only the read-back `sdOutDelayIs()` is left. | Say only the read-back remains. |
> | `src/a32/audio.cpp`, `src/a32/audio.h`, the audio board's key handler | They date the removal of the console key `y` a day earlier than the release that removed it, v.1.0.3. | Give the release, v.1.0.3. |
> | `src/a32/audio.cpp`, above `zeroWatch()` | "Every other counter in this file resets on read"; `btUnderruns()` and `gainWatch()` do not. | Name the ones that do. |
> | `src/a32/main.cpp`, above the `[ZDD]` announcements | Calls "a pop is predicted HERE" in the text below stale; that text is gone. | Drop the reference. |
> | `src/a32/main.cpp`, `settingsDefaults()`, `cfgB.autoConnect` | The value is "stored and shown"; its portal row was removed, so it is stored and not shown. | Correct it. |
>
> **Code defined and never used.**
>
> | Where | What | Fix |
> |---|---|---|
> | `src/s3/rda.cpp` | `R2_MONO`. | Remove it. |
> | `src/s3/rda.h`, `src/s3/rda.cpp` | `Rda::fineRssi()`, so `gFineR` is written and never read. | Remove both, or print it with the fine pass (section 12.2). |
> | `src/s3/rda.cpp` | `tuneTo()`'s `fine` flag, false at every caller. True would be wrong: the function takes tenths of a MHz, so doubling the channel addresses the same 100 kHz points; the real 50 kHz path is `tuneTo20()`. | Remove the parameter, so nobody can pass true. |
> | `src/s3/rda.h` | `Rda::IF_OFFSET20_NOMINAL`, the nominal 10.7 MHz, read by nothing. | Remove it, or keep it and derive the oscillator window from it (section 12.2). |
> | `src/s3/needle.h`, `src/s3/needle.cpp` | `Needle::getCalibration()`, `Needle::getIndexCal()`, `Needle::state()`: no caller. | Remove them. |
> | `src/s3/drive.h` | `HALFSTEPS_PER_REV`. | Remove it, or keep it as documentation with a comment that nothing uses it. |
> | `src/s3/drive.h`, `src/s3/drive.cpp` | `DRV_FULL`: `Drive::setMode()` handles it, nothing selects it. | Remove it, or keep it as documentation. |
> | `src/s3/net.h`, `src/s3/net.cpp` | `Net::apVerified()`, `Net::isQuiet()`, `Net::quietLeft()`, `Net::ntpResync()`. | Remove them. |
> | `src/s3/main.cpp`, the `MSG_HELLO_ACK` case of `onMessage()` | The "[WARN] PROTOCOL MISMATCH" check, which can never fire: the decoder drops every frame of another version first, and the wrong-version count does the job. Its comment already calls it "A GUARD THAT CANNOT FIRE TODAY". | Kept as a guard; only its removal is open. |
> | `src/a32/main.cpp` | `btStreaming`: written by the Bluetooth audio-state callback, never read. | Remove it. |
> | `src/a32/audio.h`, `src/a32/audio.cpp`, `src/a32/rtc.h` | `Audio::source()`, `Rtc::present()`. | Remove them. |
> | `include/link.h` | `txDropped_` (counted, never read), `lastRxMs()`, `peerProto()` with `peerProto_` (the main board reads the handshake's `protoVersion` directly). | Remove them, or print `txDropped_` in the console `s`. |
> | `include/proto.h` | `ProtoFramer::overruns`: counted, never read. | Remove it, or print it in `s` beside the other link counters. |
> | `platformio.ini`, `[env:s3]` and `[env:a32]` | The flags `AMB_S3=1` and `AMB_A32=1`: no source file tests them; each board is selected by its own source folder. | Remove them, or use them if shared code ever needs to tell the boards apart. |
