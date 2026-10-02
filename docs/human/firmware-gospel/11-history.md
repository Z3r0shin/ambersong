# 11. History and bench tests

The firmware was written in about a month, and many of its rules come from what went wrong along the
way. This chapter says how it came to be and what each release changed. It also says what the bench tests
proved; most of them ran before the firmware was written. Several design choices in chapters 3 to 10 rest
on those results, so this is where to look when a choice seems strange.

## 11.1 How the firmware came to be

The work falls into nine eras. The firmware's own history starts on 1 September 2026; the first era comes
from the project's notes. The last column is what each era left in the firmware: a rule it still follows,
or the part of the code it produced.

| Era | When (2026) | What happened | What it left in the firmware |
|---|---|---|---|
| **0. Bring-up** | 26 to 31 August | Both microcontrollers arrived blank. The toolchain was chosen and pinned (chapter 3, section 3.3). The bench tests of section 11.3 ran, phases A to G. A motion test rig settled the needle's motion (chapter 5). The needle was redesigned around a single index sensor, with no end-stop switches. Then the firmware was written in three steps: the link between the boards (framed, checksummed, with a self-test that corrupts a byte on purpose); the audio board's sound engine; the main board's core, with versioned, migrated settings. Last came the portal: one compressed page, settings generated from one table, user roles, salted password hashes, sessions held in memory, a back-off after failed sign-ins, the rescue access point, network time setting the battery clock, and updates over the air for both boards. Once the audio board could be updated through the main board, the cabinet no longer had to be opened. Two bugs were found on the hardware: the display showed `88:88` at brightness 255, and the battery clock's "oscillator stopped" flag was sticky. | **Check what a configuration call returns:** a PWM set-up call had silently refused 11 bits. **Never starve the idle task:** the main board boot-looped. **Give every homing phase its own step budget.** **Keep flash writes away from motion:** a flash write stalls everything that is not in IRAM (the internal memory code can run from without the cache). The needle homes once, when the main board starts; after that the front switch homes it again only if it has no zero, as after a FAULT. The Bluetooth library gets an output with no route to the audio interface, because its sample-rate call reconfigured the I2S driver (chapter 10, section 10.3.3). The oscillator-stopped flag stays set until the time is written, which is how the firmware uses it (chapter 9, section 9.9). |
| **1. The first pop hunt** | 1 September | The machine popped, and the repository was created to track the investigation. A diagnostic isolated data activity at the audio word boundary. An ADC clock toggle was retired. The audio board learned to sleep while the amplifier is off. Seven defects were fixed, three of them regressions from that sleep rule. The portal's jog was bounded, and a reboot no longer lands on top of unsaved settings. | The awake rule (chapter 9, section 9.4). The main board's wait for the audio board's answer to the start of an update went from 5 s to 20 s (chapter 3, section 3.17), and an upload watchdog was added. **An update that reboots cannot report its own success** (chapter 3). |
| **2. The needle's root cause** | 2 and 3 September | Two-point dial calibration. The high soft limit could not be set, and a bus stumble had eaten a revolution. **The root cause:** the index calibration was measured and then thrown away while its frame shift was kept, so every soft limit was about 640 half-steps wrong. Two earlier fixes turned out to be destructive together, and a jog that landed on the index invented a slip. | Homing step budgets as constants based on the measured band; corrupt values purged at load; provisional limits. The unguarded end-stop calibration removed, as a hazard behind a button (the keys `g` and `G` now only say so). Sampling lag at an index crossing no longer counted as a slip. No password in the image: a placeholder account with a forced change (chapter 8, section 8.6.3). The settings save at an update fixed. |
| **3. The dial calibrates itself** | 4 to 7 September | The dial is a curve, not a line: a two-point line was exact at its marks and about 0.5 MHz out mid-band, and the straight line over the tuner's travel 1.4 MHz out at the bottom. Bench Phase H showed that the tube set's oscillator can be heard and read (chapter 6). The RDA5807M, an FM receiver chip used as an instrument, then measured the dial; the curve became quadratic, and it predicted a position it was never fitted to (98.5 MHz). AFC was switched off during measurements, a stereo-pilot rejector and a 50 kHz fine pass were added, and the radio learned to keep calibrating while it is listened to. WiFi was held at 2.0 dBm, because at the time nothing could hear the board at any higher power. | The self-calibration of chapter 6. A curve is proved only at a position it was not fitted to (chapter 6, section 6.10). |
| **4. The needle watches itself** | 8 to 10 September | A self-recovery ladder: small errors absorbed at each index crossing, a larger one re-indexed once tuning stops, three failures give FAULT. The needle had been chasing a rounding boundary while its status said "stationary"; this was found by watching the stepper driver's LEDs, not by reading the code. A hunting detector followed. | **A needle that knows it has slipped must not run on, wildly inaccurate, and do nothing about it.** **Every failure must be visible in the machine's own telemetry, at a resolution that would show it:** the status could not show the hunting, because both positions rounded to the same published value. |
| **5. The second pop hunt** | 10 to 12 September | The pops came back. An objective counter (a microphone on USB, one-minute A/B windows, 30 windows) showed that driving the audio board's clock pads at full strength, **drive 3**, took the rate from 3.33 pops a minute to none in the 932 s measured with it. The same measurement excluded the DAC's clock-error mute. The root cause is still open. Pop-hunt telemetry reached the portal; a diagnostic message was retired after it sent the radio to full volume. The audio board's fade wait before an update went from 250 to 400 ms. The rescue access point became reachable and checkable. | Drive 3 as the start-up default (Appendix H, sections H.4 and H.5). The audio buffers stay at 8, because 32 KB more would exceed the roughly 24 KB really free on the audio board (chapter 10, section 10.8). The method for counting pops (Appendix H, section H.8). |
| **6. A hardware freeze** | 13 to 21 September | Hardware and firmware changes paused while the hardware record was completed. The one firmware change of substance brought the pin file's comments and unused definitions in line with the hardware, without changing behaviour. | No behaviour. The unpublished display-timing bench program stopped building, because two pin names it used were removed (section 11.4). |
| **7. The IF as a setting, the console in the portal** | 22 and 23 September | The master clock and the frame clocks can be driven at different strengths, for a pop test that has never been run. The tube set's IF (intermediate frequency) became a setting, 10.6 MHz; the RDA5807M's readings that day, about 10.60 MHz over six stations, agree. Four stale things that told the reader something untrue were removed. | The web console: everything the main board prints goes into a 16 KB ring the portal can read and type into, so no cable and no second ground connection to a PC are needed (chapter 8, section 8.9). A real index debounce: four polls, back-dated so it adds no position bias. The zero handed to the stepper cannot be lost, and is applied before anything uses it. Hand marks belong to the user alone. The audio board measures RMS level. The compiled audio gains are the measured ones. |
| **8. The transmitter and the needle's speed** | 24 September | Six fixes from what was seen on the machine. A forced access point lives long enough to be seen. A transmit power loaded from a settings file reaches the radio. The transmit ceiling became 15 dBm: nothing could see the board at 20 dBm, everything could at 15 (chapter 8, section 8.4). The up leg dropped to 1100 half-steps/s: at 1500 the needle stalled every time it moved up through the index, at 1200 to 1400 often, at 1100 never, and the stalls had left power-up sweeps about 400 half-steps low. The band check's margin grows with speed. The needle follows a filtered angle reading, so sensor noise no longer moves it. | **Where the bench rig and the machine differ, the machine decides** (chapter 5, sections 5.6 and 5.20). |

Release v.1.0 followed on 25 September.

## 11.2 Release notes

All six releases were made between 25 and 27 September 2026. The boards are updated separately, so their
numbers need not match. This book describes the main board at **v.1.0.5** and the audio board at
**v.1.0.4**.

| Release | Boards | What changed |
|---|---|---|
| **v.1.0** | both | The main board gets the audio board's recovery: an image sent over the air boots **on trial** and is confirmed after about a minute of running with its network and portal up; a hang or crash before that rolls it back; a main-board upload sent during the trial is refused (HTTP 409); a 15 s task watchdog watches the main loop and the needle supervisor; a failed link self-test no longer halts the main board. An audio-board build whose link self-test failed is never confirmed. Every build names its commit beside its version, in the console banner, the portal, the settings file and the audio board's boot report (chapter 3, sections 3.4 and 3.11). |
| **v.1.0.1** | main | A needle with no zero is homed, not re-indexed. Nothing is saved while a main-board image is on trial: changes wait in memory until it is confirmed; **Save** says why it is held; a portal reboot during the trial goes through at once and brings the previous firmware back (chapter 3, section 3.11.5). |
| **v.1.0.2** | both | The audio board counts a save only when it reached flash, and retries a failure. When the audio board goes silent the main board forgets its copy of the audio settings: audio edits get HTTP 409 and a settings download writes `n/a` for them. Boards running different protocol versions show it: `wrong-version` in the console's `s`, a portal pill, the state field `linkver`. The battery clock's health reaches the portal (`rtc`, two pills). `W` claims to have set the battery clock only when the audio board answers. The RDA5807M keeps the coarse sweep's points after a refinement and the fine pass's 11 points (console `a`, `/api/rda` field `fine`). The hand correction of the dial is never erased automatically; **Reset the dial correction to 0** is the only way. Removed: the `autoConnect` portal row, which did nothing; the console keys comma (`,`) and full stop (`.`); the learned transmit power from the settings file. Old files that carry those keys still upload. Every comment was checked against the code, and about twenty false console and portal messages were fixed. Console `D` prints the IF. |
| **v.1.0.3** | both | When every automatic sample slot is full, the most crowded automatic sample makes room for a new dial position; if the new point would itself be the most crowded, nothing is stored ("would add the least coverage"). Hand marks are never replaced. Before, a full table refused every new position. The audio board's console key `y` and the setter behind it, for the data-line output delay, are removed, because one of its positions sent the radio to full volume (chapter 10, section 10.15.3); the read-back stays. |
| **v.1.0.4** | audio | The audio board saves nothing while its image is on trial. Changes wait in memory and are written the moment it is confirmed, with the console line `settings changed during the trial are now saved` (chapter 7, section 7.4.2). |
| **v.1.0.5** | main | The rescue access point `Ambersong` is an **open network**, with no WiFi password, so a person trying to make the radio work again need not find and type a passphrase. While it is up, a DNS server answers every name with the radio's address, so a phone that joins opens the sign-in page by itself (a captive portal). The DNS server stops with the access point (chapter 8, sections 8.3.2 and 8.3.3; the cost of an open network is in section 8.6.7). |

Two rules came with v.1.0.2 and still hold: **a comment that is false serves no purpose**, and **a hand
correction of the dial is never erased automatically, only by hand.**

## 11.3 What the bench tests proved

Each part of the machine was tested on its own, with one small program per test and one source file
per program. A failure was therefore isolated by construction. The phases ran in order, and a
phase did not start until the one before had fully passed. Phases A to G ran before the firmware was
written; Phase H ran a few days after it. The programs are not published and are not
needed to build or run the firmware; this section records what each one proved. Where a result is about
the hardware itself, the Hardware Bible holds it.

| Phase | What it tested | Result | What it left in the firmware |
|---|---|---|---|
| **A** | The 5 V side, with a meter, before any firmware | Passed (Hardware Bible, chapter 5). | — |
| **B1**, main board | The build's memory settings | 16 MB of flash, and 8 386 295 bytes (8.00 MB) of PSRAM seen by the firmware; stable; the heap flat (Hardware Bible, chapter 4). | — |
| **B1**, audio board | Its identity | No PSRAM, which frees the pins the DAC needs (Hardware Bible, chapter 4). | — |
| **B2** | The battery clock's I2C bus | Passed, after a pin-map correction found by measuring. The clock's "oscillator stopped" flag read as set, and was reported, never cleared. | — |
| **B3** | The angle sensor's I2C bus and its magnet | Passed. Repeatable to ±1 count (0.08°), both over a full sweep out and back and on returning to a marked position. | — |
| **B4** | The link between the boards, both directions at once | 14 round trips of 14, none lost, 1 to 2 ms each. | — |
| **C5 to C7** | The panel inputs and the amplifier sense | Passed, after wiring corrections (Hardware Bible). | — |
| **D** | The clock display | Passed (Hardware Bible, chapter 7). | — |
| **E** | The needle drive, end to end | Passed. The first run moved the needle the opposite way to "forward"; the fix inverted the phase advance, so there is one definition of forward. | **One definition of forward**, shared by the motion, the checks and the messages. **The safety pattern:** check before every step, refuse a direction whose limit is reached, release the coils when idle (chapter 5). |
| **F** | The whole audio path | F1 DAC playback, F4 ADC capture and F5 radio end to end passed. F2: muting works in software, by writing digital zeros. F3: the audio board receives Bluetooth audio (A2DP) and plays it. F5 measured the radio at −53.2 dBFS RMS at the author's reduced volume, about 33 dB below plan. | **Mute by digital zeros**, ever since. **Two boards**, because the main board's chip has no Bluetooth Classic (chapter 2, section 2.3), and the pinned toolchain (chapter 3, section 3.3). **The gain staging** (chapter 10, section 10.5.6), which began with F5's shortfall. |
| **G** | Whether a web portal can share the main board with the multiplexed display | A timer interrupt in IRAM drove the digits and timed every slot. WiFi plus four concurrent multi-megabyte web downloads: imperceptible. The same plus stepper motion: imperceptible. Deliberate thrashing of the flash bus: the only visible disturbance, still bearable. Cleared. | **Display and stepper on core 1, WiFi and the web server on core 0. The display interrupt stays in IRAM. Settings writes are debounced. The display is parked during a main-board update** (chapter 4, sections 4.3, 4.8 and 4.14). |
| **H** | Whether the dial can be read off the tube set's oscillator | Yes. Chapter 6, section 6.13, has the three answers and the lessons. | See below. |

**Phase H** belongs to the tuning chapter (chapter 6, section 6.13). Its main lesson, in short, follows.
Phase H showed the oscillator cannot be told by loudness: a fixed spur at 79.0 MHz was louder at every dial
position, and only moving the dial told them apart. The firmware uses that lesson by learning what does
not move (the fixed features, with the set off) and masking it; it does not itself check that a peak
moves between two sweeps. It also showed that the injection is low-side (the oscillator runs one IF below the station), which
the Hardware Bible records.

**The motion rig**, a bare stepper motor and driver on a spare ESP32 with no needle, settled the needle's
motion before any of it went into the firmware, and found four motion bugs of the same shape as the
firmware's. Its findings, and how to build a new rig, are in chapter 5 (sections 5.6 and 5.23.4). The rig
is retired. Its numbers were a starting point, not a standard: the firmware's own defaults (Appendix A)
are the rule.

## 11.4 Tools that ship

Only two tools ship with the firmware, both build scripts that run before every compile (chapter 3,
section 3.4). They matter only to someone building or changing the firmware.

> **For firmware changes**
>
> | Script | What it does |
> |---|---|
> | `scripts/page.py` | Checks the portal page's script, compresses the page and embeds it in the main board's build. |
> | `scripts/version.py` | Stamps every build of both boards with its version and its commit. |

**Not published:** the bench programs and test rigs of section 11.3, and the pop hunt's recordings and
analysis scripts, which no longer exist (Appendix H). The display-timing program of Phase G no longer
compiles, because it used two pin names (`S3_LIMIT_LEFT`, `S3_LIMIT_RIGHT`) removed from the firmware's
pin file during the hardware freeze. Anyone who needs such tools again will have to remake them: chapter
5, section 5.23.4, says how to build a motion rig, and Appendix H, section H.8, how to count pops.
