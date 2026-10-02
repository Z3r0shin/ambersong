# Appendix H. The pop hunt in detail

For about two weeks the radio made **pops**: loud, short clicks through the speakers, at irregular times,
on RADIO and on BT, never on AUX. This appendix keeps every test and every measured number, organised by
the question each one answered, so that the next person to hear a click does not start from zero. It
ends with the method for counting pops. Chapter 10, section 10.10, has the summary. You need this
appendix only if the clicks come back, or before you change anything in the audio clocks.

Every rate here is given with its window. A bare "zero" is never written: zero events in a window is a
bound, not a cure.

**The raw evidence no longer exists.** The recordings and the analysis scripts behind every number here
were not kept, and neither were the test tools. Only the numbers
survive. Anyone who needs such tools again will have to remake them; section H.8 says how they worked.

## H.1 What a pop is

**What you hear.** A loud click, in bursts and alone. Any A32 volume other than 0 produces pops; at a
volume of 8 they were plainly audible.

**Sizes.** Pop sizes fell on a **ladder of binary weights**: ¼, ½, ⅝, ¾, 1 and 3/2 of a main size (79
events over three soaks, each rung within about 0.2 to 0.4 dB, no clipping). Polarity was mostly mixed,
but the 3/2 rung came out negative 9 times in 9. That reads as a few of the **top** bits of single words
coming out wrong with the rest intact: neither a framing slip (it would smear) nor the DAC's own mute (one
size, one polarity).

In one evening's recordings the sizes clustered in three families about 1 dB wide: −7.2, −9.2 and
−25.2 dBFS at the microphone (31 events). The quiet family's shape correlated only 0.41 to 0.46 with the loud one's,
but the author listened to clips of both and called both pops, so the detector is calibrated against his
ear. **A low correlation is not evidence of "not a pop".**

**Pops do not need the data to change.** On a constant word (`0xFF000000`, 5 minutes, Bluetooth source,
tube set out, A32 volume 7, amplifier at maximum) the radio made 6 isolated pops. They fell into two
populations. 2 of the 6 matched the recorded step of the switch into the DC word (correlation 0.75 and
0.88) and sat inside a burst of stalls: they were **stall clicks**. The other 4 were short impulses, not
steps, the same sizes as in music. A near-zero word had never popped and this −42 dBFS word did, so
whether a pop is visible depends on the word's value. That fits the DAC mis-reading bits, not noise added
in the analogue path.

**Not every click is a pop.**

| Click | Shape | How to tell |
|---|---|---|
| A pop | an impulse; it rings | no stall near it |
| A stall click | a step | the stall counter moved; it correlates with a recorded switch step |
| A reboot or update click | one click | the A32's clocks stopped; check the flash log |
| A crackle storm | dense detections for 35 to 60 s, peaks near −3 dBFS | unidentified origin; the author did not recognise the sound |

## H.2 What the pops were not

**The first observations, on Bluetooth.** Music over Bluetooth popped. Pausing the phone stopped the
pops. Muting the phone — still streaming, so frames of zeros kept arriving — stopped them. And a phone
volume a hair above zero, music inaudible, brought them back **at full loudness**. The mute test was the
author's own design and a clean one: same link, packet rate, decode load, CPU and radio; only the sample
values changed. The last observation ruled out every linear operation in the firmware, the underrun
click included (its size is exactly the last sample before the gap): turn the phone down 40 dB and every
artefact a linear stage can make drops 40 dB too. The underrun theory explained the pause and the mute
perfectly, led for an hour, and died on this.

**The first round of tests, each a measurement:**

| Test | Result | What it excluded |
|---|---|---|
| S3 powered off completely, A32 alone, playing | no change | WiFi, 2.4 GHz coexistence, the link, the whole second board |
| RADIO source | popped the same way as BT | the whole Bluetooth branch: SBC, the ring, underruns, the library's volume |
| AUX | clean | nothing on its own: AUX writes zeros through the same path, so it only says zeros are safe |
| DAC mute input held high | no change | one of the DAC's mute routes (the pin was later found to reach nothing) |
| Peak meter through 60 s of continuous popping on RADIO | never above −118.4 dBFS; `clip` never set | the firmware's arithmetic: the words written were clean |
| Pop timing | irregular and bursty: under one a second, sometimes 3 to 5 inside a second, sometimes 7 to 8 s apart | anything locked to a firmware period (the 172.3 Hz blocks, the 20 Hz knob reads, the 4 Hz state message) |
| ADC unpowered; then ADC powered with its I2S cable out | clean both times; with the cable out a 60 Hz hum went too | — |
| ADC's master clock cut in firmware, ADC still powered and wired | pops and hum gone; restored, both came back | passive loading: the converter had to be running |

**How could words at −118.4 dBFS, about one bit, make a full-scale bang?** Shifting a word by k bit
positions multiplies it by 2^k, 6 dB per bit, so reaching full scale needed k of about 16 to 20: the DAC
was losing or gaining bit-clock edges mid-frame. And a shift of zero is zero, which explained why silence
never popped, without any cause that looked at the content.

**When the pops came back** after nine quiet days (section H.3), the microphone recorder of section H.8
replaced the ear. The baseline was **3.00 pops/min**, stable over three 10-minute blocks (2.80, 3.20,
3.00). At that time a bad knob calibration had pinned the volume near 18 of 255, with the amplifier
turned up to match, so every fault arrived 62 dB louder than it should (chapter 10, section 10.5.1).

**Later exclusions:**

| Suspect | Test | Result |
|---|---|---|
| The DAC's own zero-data mute releasing with an unramped step | the zero-data witness | `zrel` did not move through 8 minutes of pops |
| The volume reading wandering (it did wander, between 17 and 20) | the volume watch, then the knob ignored | `gsteps` frozen, still popping |
| Audio-task stalls | the stall counter | `stalls` frozen and `under` at 0 during pops |
| The needle motor, the FM tuner chip, the 5 V supply (the set ran from a bench supply), the tube set, the box fan, the earth, a mains filter, the room, the amplifier | — | all excluded; for the amplifier, AUX was clean and the pop follows the amplifier's own volume |
| The DAC's clock-error mute | its datasheet says a clock error attenuates, then mutes hard for about 104 samples (about 2.4 ms), so the programme must vanish | through 37 recorded pops the streamed tone's level, measured phase-insensitively (band-limited 180–260 Hz, analytic magnitude), changed by a median of −0.3 dB; only 8 of 37 dipped past −10 dB |
| One fixed-size sign-bit flip | polarity and size of 31 pops | 20 of 31 had the opposite polarity (p ≈ 0.15); sizes spread over 19.1 dB |

A first attempt at the clock-error test, with a coherent 6 ms average at exactly 220.0 Hz, read +4 dB:
it was measuring the pop's own ring-down.

**Mains is a contributor, not the cause.** A heat gun on the same circuit made the radio pop from 5 cm or
10 m away; on another circuit it did nothing at any distance, so the route is conducted, not radiated. The
baseline survived on a quiet circuit, so two rates were adding up. An inlet filter changed nothing.

**Volume is not a variable.** A result that "volume 17 brings the pops back" turned out to be one
heat-gun burst. Timestamp every event before believing a rate.

**Stalls and Bluetooth.** Stalls once seemed to track Bluetooth: 0 in 34 minutes with nothing connected,
8 in the 7 minutes after a phone connected. That was coincidence with when streaming happened: later, 13
stalls came in about 33 minutes with the Bluetooth radio idle and the source on RADIO (`stalls` from 1 to
14 after a start), the longest 60 ms against the 46.4 ms chain. That retraction is about the **stalls
only**, not about the pops' relation to Bluetooth: say "the stall correlation", never "the Bluetooth
correlation". Stall counts, each since its own start: 25 at 21.6 hours up, and 21 at 41.9 hours up.

**Pops and Bluetooth activity.** The pops seemed to follow Bluetooth activity: 1 in about 30.7 minutes
idle against 12 in about 6 minutes after a phone connected, the same constant word on the wire. That was
not a designed A/B, and time confounds it: the phone had simply been reconnected after supper.

## H.3 Was it the data line?

**The constant-DC words.** The constant-DC diagnostic was given a settable word: every slot carries the
same fixed word, silent as audio, so anything heard is the fault. Six words, each held at least 30 s on
RADIO:

| Word | Level | Lowest set bit | Result |
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
against the frame-clock edge.

**The reading then:** activity on the data line at the word boundary made the DAC lose its framing; at
that time the DAC recovered its clock from BCK by itself. Real audio has a busy lowest bit at every level,
which is why the pops had not followed the music's level. The I2S fault latches were clean in eight
one-second windows during popping, so the fault lay after the firmware's write. The **low-bit output
mask** was built as a fix; it was never confirmed by ear, and of its widths only bits 0, 1 and 16 had
known results. The ADC master-clock toggle was retired, and the sleep rule added.

**The DAC's clock wiring was then changed** so that the DAC takes its master clock from GPIO0 (Hardware
Bible, chapter 6). The radio played with no pops at all, and nine quiet days followed. That change also
switched off the DAC's own clock recovery from BCK, which the data-line reading had blamed.

**When the pops returned**, the data-line story fell:

- **The sign-extended tail** (the last 8 bits of each word set to the sign of the next word, so the data
  line never moves across a word boundary) was tested blind with the heat gun, on-off-on-off. No change
  in rate or size, and the rate did not rebound when the tail went back off. The data-line mechanism was
  dropped.
- **Muted with the zero-data floor on** (the DAC's output stage kept alive on a constant word) the radio
  made 0 pops in 3 minutes, against 3.7/min playing. It was read then as "the pop needs changing data".
  That was over-read: its premise, that a 1-bit word keeps the DAC's zero-data mute off, was only the
  firmware's model of the DAC. The better reading is that the DAC slips through its clocks whatever the
  data, and a slip is audible only when the word is not tiny.
- **Popping on a constant word** (section H.1) settled it: the pops do not depend on the data changing.
- **An A/B of `0xFF000000` against `0x00FF0000`** (12 cycles of one-minute windows, Bluetooth with no
  phone connected, drive 2) was inconclusive: 1 against 0 events, p = 0.489, under the rule fixed before
  the run that fewer than 6 events is inconclusive. The chain was alive: all 23 switch thumps were
  detected within 0.06 s of their stamps.

The pops of the first round, which the clock-wiring change fixed, and the later ones, with sizes on a
ladder, were read as **two different faults**.

**The data-line output delay.** A register that shifts the data line in time (`tx_sd_out_delay`) was
added and swept live. At position 2 it sent the radio to **full volume** through the valve amplifier;
positions 1 and 3 silenced it. The speakers survived. Nothing in the firmware writes it any more; chapter
10, section 10.15.3, carries the warning.

## H.4 Was it the clocks? What cut the pops

**The suspect.** `setClockDrive()` sets the drive strength of the three clock pads together: MCLK
(GPIO0), BCK (GPIO18) and LRCK (GPIO17). The ESP32 has four drive strengths; the power-on value is 2.

**A first alternation was inconclusive, not negative.** One morning, 30 pairs of one-minute windows,
drive 2 against 3: the fault was dormant, 3 loud pops in the hour (drive 2: 3, drive 3: 0, p = 0.125; at
a −45 dBFS threshold, 10 against 6, p = 0.22). The rate changes by the hour: earlier, four 2-minute
quarters had been fooled by one burst, a p = 0.005 that vanished pair by pair.

**The decisive A/B.** One-minute alternation, 30 windows, a 220 Hz tone streamed over A2DP, the recorder
counting, no stalls in the run:

| Drive | Window | Isolated pops | Rate |
|---|---|---|---|
| 2 | 936 s | 52 | 3.33/min |
| 3 | 932 s | 0 above −35 dBFS | — |

Stratified exact test **p = 2.8e-16**; all 15 pairs went the same way. A first scoring gave 2 events at
drive 3 (p = 6.3e-14, 14 of 15 pairs). Both fell 4 s after the run's exit had already restored drive 2,
and were credited to the last window only because that window closed at the end of the recording.

**The soaks at drive 3:**

| Condition | Window | Isolated pops |
|---|---|---|
| 220 Hz tone at −30 dBFS over A2DP, A32 volume 47, zero-data floor on | 45.1 minutes | 1 (0.022/min), about 150 times fewer than at drive 2. A shower in the house during the run produced no false events. |
| Constant word `0xFF000000`, A2DP streaming (the strongest rate amplifier found) | 90.0 minutes | 0, and one stall (39 ms) with no event near it |

The 90-minute capture ran at 47 997 samples/s and lost 0.011 % of the time, which at drive 2's rate would
hide 0.03 of a pop. Its upper bound is about 0.03/min: **not proof of a cure**.

**Drive 3 became the start-up default**, read back as 3 from all three pads after a cold power cycle.

**What it is.** It cut the pops about 150 times in the measured conditions. The story offered for it is a
hypothesis; no oscilloscope has ever seen an edge. It runs like this. GPIO0 drives two converter clock
inputs down a harness stub. The DAC's fallback clock recovery is off while an external SCK is present. A
stiffer edge stops a marginal sampling instant latching the wrong bit. The data rate leaves 354 ns per bit, so the question
is edge quality, not timing. It is **a margin fix, not a root cause**: it says the DAC was latching wrong
bits on a marginal edge, not why the edge was marginal. The root cause was never found.

**It cannot stop MCLK by accident.** GPIO0 is an RTC-capable pad, and the framework writes only the
pad's drive field; the pad's function select is a separate bit.

**Prior art**, found on the web, generic to this DAC and the ESP32 (no complete search was made for this exact
arrangement, one ESP32 master clock feeding both converters):

- TI's answer to a "click noise" case on a sibling DAC was to ground its system clock so the internal
  PLL runs from BCK (TI E2E 1052863). That is how this radio was wired before the clock-wiring change,
  when the pops were harder.
- TI publishes no data-to-bit-clock setup and hold figure for this DAC family (TI E2E 1542019). At a
  354 ns bit period timing is not the issue; edge quality is.
- Nobody else reported pops that survive a constant word, or fixed pops with
  `gpio_set_drive_capability()`.

**Two other changes of the same night**, neither measured as a pop fix:

- The **Bluetooth transmit ceiling** of 0 dBm was added as a mitigation. Its effect on the pops was never
  measured.
- The state message's 4 Hz query of the controller's transmit power was removed after an A32 reboot was
  blamed on it. The blame was later withdrawn: the uptime read had been the S3's, both boards had
  restarted within a minute, and the set had been power-cycled. **Why the A32 rebooted is unknown**; the
  removal was a precaution, not a proven fix.

## H.5 What did drive 3 cost?

**The trade-off as heard.** At drive 3 the radio path was heard as distorted, like people lightly
talking through kazoos. At drive 2 the radio sounded right, but the pops came back hard, like full-on
popcorn, more than ever before. Both were reproduced by changing `sys.clkdrv` and nothing else. It was the
first result pointing at the clock lines shared by both converters rather than at either part: the
setter moves all three pads together, which is why it had looked like one knob.

**Drive 3 never eliminated the pops.** They had been heard at drive 3, on BT,
with the tube radio in or not, and both sources popped alike. Every measured number was from the
Bluetooth path only because that is what had been streamed: a limit of the evidence, not a property of
the fault. The drive-3 soak had Bluetooth streaming, so **no drive-3 measurement with Bluetooth idle
exists**. A hardware item suspected on the master-clock line had always been fitted (Hardware Bible,
chapter 4), so it could not explain any change.

**The burst.** At drive 2, RADIO source, A32 volume 8, the recorder counted **932 pops in 5.1 minutes
(183/min)**, 55 times the earlier drive-2 rate. None coincided with a stall; the median size was
−14.7 dBFS, and 879 of the 932 fell in one 10 dB band (−20 to −10 dBFS), unlike the three tight families
of the earlier evening. Five conditions differed from the earlier runs and none was isolated. The volume
of 8 did not hide the pops; they were plainly audible. Decoupling capacitors had been fitted the day
before (Hardware Bible, chapter 4); they may be part of the cause. Untested.

**Two tests designed and never run:**

- **The split drive** (`sys.clkpair`, through `setClockDriveSplit()`): drive MCLK and the BCK/LRCK pair
  apart. The prediction was that MCLK at 2 with BCK/LRCK at 3 (`23`) would give both good halves, because
  the DAC latches its data on BCK edges while the ADC's conversion timing derives from MCLK; `32` tests
  the opposite assumption. It is in the firmware, never run.
- **The volume swing**: alternate the A32 volume between 47 and 94 while a tone streams, and see whether
  the pops move with the tone (an error proportional to the sample) or stay put (a fixed wrong code at
  the DAC).

## H.6 Where it stands

**The hardware changed, not the firmware.** The audio cables were changed (Hardware Bible, chapters 6
and 10), and the author reported that even before those changes no pops had been heard since ferrite
cores were added. No firmware changed for it, and **no instrumented pop count exists after it**.

**The decision.** Drive 3 stays the start-up default, and drive 2 will not be tried again: drive 3 works
with no apparent consequences, and there is nothing to gain from drive 2. `sys.clkdrv` stays in the
firmware so that drive 2 against 3 can be tested again, or drive 2 put back, without a reflash.
`sys.clkpair` stays in the code, never run.

## H.7 What is still open

- **The root cause** of the pops.
- **The 183/min burst**, and whether the decoupling capacitors played a part.
- **The "kazoo" distortion**, judged by ear only. No recording of it exists, so it and the pops were never
  put on one scale, and it has not been measured since the cable changes.
- **Why the A32 rebooted** during the hunt.
- **A drive-3 measurement with Bluetooth idle**, the split drive, the volume swing, and the Bluetooth
  transmit-power A/B: none was run.
- **No waveform has ever been seen.** There was no oscilloscope; distortion was judged by ear.
- **The firmware cannot see a DAC-side clock error.** No telemetry exists for it.

**The lesson on method.** Three theories died in the first round alone, each to a measurement and not an
argument, and each had been stated more confidently than the evidence allowed. **A measurement
*consistent* with a hypothesis is not one that *discriminates* between hypotheses.**

## H.8 How to count pops

If the pops come back, this is the method that worked here.

**The recorder.**

- A microphone recorder about a foot from the radio, streaming continuous 48 kHz audio over USB, with its
  own clock logged beside its sample count: a dropped chunk is a splice, and a splice looks like a click.
- Read it in bulk with a large driver buffer (1 MB here). The first attempt ran at 38 kHz with 18 lost
  chunks. A good capture runs at 47 99x of 48 000 samples/s.
- Turn the microphone gain down far enough not to clip a foot from a speaker.

**The detector.**

- The 1 ms RMS of a high-passed signal (a 10-tap filter).
- An absolute threshold of −35 dBFS, with −45 dBFS as a secondary.
- 50 ms dead time.
- An isolation score: the event's level minus the median over ±200 ms. Real pops scored 25–58 dB;
  crackle, speech and handling scored below 15.
- Have a listener's ear confirm clips of each size of event before trusting the detector.
- Do not use a shape filter ("falls 10 dB in 5 ms"): pops ring, and such a filter kept only 29–61 % of
  real ones.
- If a heat gun is used as a trigger, keep it away from the microphone.

**The design of a run.**

- **One-minute alternation, 15 pairs or more, both arms in the same recording, a source actually
  streaming**, and a stratified per-pair exact test.
- Never compare totals, or one long block per arm: the rate changes by the hour (whole mornings of
  nothing, then bursts).
- A silent link sends exact zeros and hides pops, so use `sys.dctest 1` to make them visible whatever
  plays.
- Decide the rule before the run (here: fewer than 6 events is inconclusive).
- Close the last window at the end of the run, not at the end of the recording.
- If events cluster (a crackle storm), the per-pair test is invalid: do not cite its p-value. Two storms
  once crossed a music-against-DC alternation and made its p-value (9.94e-28) invalid: 199 of its 218
  music events came from the storms, so the events clustered and the test's independence did not hold.
- Check the stall counter and the timestamps to tell a stall click (step-shaped; it correlates with a
  recorded switch step) from a pop (an impulse).
- A reboot or an update pops by itself, so check the flash log before counting a pop "on its own".

**Traps that cost real time here.**

- **Portal sessions expire after 5 idle minutes.** A long script must log in before every portal call,
  or its commands are silently refused. A whole 30-minute A-B-A soak was lost that way, and only the pad
  readback showed it.
- **A script stamps a switch after its command and readback**, so the real switch lands about 1.2 s
  before the stamp.
- **Never edit a shell script while a copy of it runs**: the shell re-reads the file and the running copy
  dies.
- **A build-and-flash chain must test the build's own exit code.** `pio run | grep` returns grep's status
  and would flash a failed build.
- **A cleanup trap that restores a setting must also exit**, or the script carries on toggling with no
  recorder.
- **Two readers of a read-and-reset counter corrupt each other.** A 40-minute "dropout" log was entirely
  the A32 console `s` draining the peak meter.

**Rates to beat**, each with its window:

| Condition | Window | Pops |
|---|---|---|
| Drive 2, 220 Hz tone over A2DP | 936 s | 52 (3.33/min) |
| Drive 3, same run | 932 s | 0 above −35 dBFS |
| Drive 3, tone over A2DP | 45.1 minutes | 1 |
| Drive 3, constant word, A2DP streaming | 90.0 minutes | 0 |
| Drive 2, RADIO, A32 volume 8 | 5.1 minutes | 932 |

**Say the window and the bound, never the bare word "zero".**
