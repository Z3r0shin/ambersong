# Ambersong: Firmware Gospel — the dial needle

> **Specific to this build — adapt.** The needle, its stepper, gearing, index switch and travel are
> this cabinet's mechanism; another build's will differ. The methods below carry over to a
> different mechanism; the measured numbers do not.

This file covers how the firmware moves and homes the dial needle: the motion profiles, the
homing and re-indexing logic, the soft limits, the needle's memory across a reset, and the
physical values this particular mechanism needs. It does not cover how the tuning knob's angle
becomes a frequency, or how the dial calibrates itself against the tube set's local oscillator —
that is `firmware-gospel-tuning.md`.

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

Terms used in this file: **S3** is the main board, an ESP32-S3 module; the needle's firmware runs
on it. **hs** ("half-step") is the unit of needle position: one half-step of the stepper motor,
counted from the homed zero. **The index** is the one position-reference switch on the needle's
travel — a Hall-effect switch that turns on when a magnet passes it. **Homing** is finding that
switch; **re-indexing** is homing again to repair lost steps without the listener noticing. **A32**
is the audio board; it does not take part in anything in this file. **BLOCKER**, **DEFECT** and
**NOTE**, used in the failure tables below, rank how much a fault costs to recover from: BLOCKER
means the radio is unusable until a full power cycle, or the harm survives one (the needle driven
into a stop, a calibration or the settings corrupted and saved); DEFECT recovers with a portal
reboot, a console reset or the front power switch; NOTE recovers by itself, or is cosmetic.

This file covers the dial needle: its motion, homing, tracking, parking, soft limits, recovery
from lost steps, and its memory across a reset. The other files of this edition:
`firmware-gospel.md` (what the firmware does as a whole, the pin tables for both boards, building,
flashing, updating and recovery), `firmware-gospel-main-board.md` (the main board's boot, tasks,
clock display, panel lamps, time keeping and self-protection), `firmware-gospel-tuning.md` (tuning
and the dial's self-calibration), `firmware-gospel-settings.md` (the settings system),
`firmware-gospel-network.md` (network, web portal and console), `firmware-gospel-link.md` (the
link between the two boards), `firmware-gospel-sound.md` (sound, sources and Bluetooth),
`firmware-gospel-known-issues.md` (known issues).

## Ambersong firmware: what the dial needle does

**How the needle and the tuning are tied together.** The needle follows the firmware, and nothing
else. Nothing mechanical joins the two (`hardware-bible.md`): the tuning knob turns the tube set's
ganged tuning capacitor, the needle is moved by a stepper motor, and the firmware is the only link
between them. A magnetic angle sensor on the capacitor's shaft (an AS5600) is the live measure:
read every 20 ms, it tells the S3 that the tuning changed and where the shaft is, and every
position the needle takes while it follows the tuning comes from that angle through the
calibration curve (`firmware-gospel-tuning.md`). A small FM receiver chip calibrates that curve: it
listens to the tube set's local oscillator, the firmware adds the intermediate frequency to get the
station, and the pair (shaft angle, station) is stored as a sample the curve is fitted through. The
FM receiver chip never stands in for the angle sensor: if the angle sensor stops answering, the
needle holds its last target until it answers again.

The S3 turns the shaft angle into a frequency and drives the stepper so that the dial needle
points at that frequency on the printed glass. What matters most is repeatability: the same knob
position must always put the needle in the same place.

What the listener sees:

1. **The set is plugged in.** The S3 boots and the needle finds its reference point (it "homes").
   Then it makes one "power-up flourish": a run to the low end of the dial, a run to the high end,
   then straight to where it belongs — the station if the amplifier is on and the source is RADIO,
   otherwise the low end.
2. **The front switch goes on with the source on RADIO.** The needle leaves the low end and swings
   to the station like a meter movement: a small overshoot and a short settle. If the decorative
   full-sweep setting is on, a full end-to-end sweep comes first.
3. **Tuning.** The needle follows the knob continuously. It ignores changes smaller than two motor
   half-steps, so it does not chase sensor noise.
4. **The amplifier goes off, or the source is not RADIO.** After one second of stillness the
   needle falls to the low end with a fast start and a long, slowing finish, like a drive dying
   away.
5. **Two minutes after the amplifier goes off.** The needle quietly re-checks its reference point
   and returns to the low end, so the next listening session starts from a verified zero.
6. **Tuned past the printed scale.** The needle stays pinned at that end of the scale.
7. **During a firmware upload to the S3.** The needle stops where it is and the clock display goes
   dark. The radio keeps playing, because audio runs on the other board. After the restart the
   needle remembers where it was and homes from there.

The needle has no end-stop switches (`hardware-bible.md`). One Hall-effect switch (the "index") sits
part-way along the travel and is the only position reference. Everything else is kept by counting
steps, and corrected every time the needle passes the index. Three firmware mechanisms keep the
needle off the mechanical stops in its place: soft limits checked before every step, a step budget
on every homing phase, and a known starting belief for the homing search. On this dial the index
sits near 102.5 MHz (below, "Physical values"), so an evening spent below that frequency never
crosses it — which is why the firmware also checks the index on its own when the set is switched
off.

## Ambersong firmware: the needle's chain, end to end

```
 tuning knob -> capacitor shaft -> AS5600 angle (0..4095 per turn)
                                        |
                           multi-turn running count ("accAngle")
                                        |  low-pass filter (needle only)
                           tuning curve: count -> frequency (tenths of MHz)
                                        |
                           frequency -> needle position (printed face)
                                        |
                           needle supervisor: plan a move (profile, microstep)
                                        |
                           step emitter: one step at a time, soft limits
                                        |
                           drive layer: position -> 4 PWM duties -> Darlington driver
                                        |
                                  stepper -> needle
```

The tuning curve (shaft count to frequency) belongs to `firmware-gospel-tuning.md`. This file only
consumes it. What is wired where — the stepper, its driver, the index switch and the angle sensor
— is in `hardware-bible.md`; the pin names the firmware drives are in `firmware-gospel.md`'s pin
tables.

## Ambersong firmware: files, layers and tasks

| Layer | Job |
|---|---|
| Policy (main board's `main.cpp`) | When to home, track, park or sweep; the recovery and idle-check timers; soft-limit capture; band-calibration result collection; saving the shaft count; pausing and resuming for an upload; console keys. |
| Needle (`needle.cpp`, `needle.h`) | Two tasks: the supervisor and the step emitter; homing, tracking, parking, sweeps, index sensing, calibration, angle-sensor reads, the RTC memory record. |
| Drive (`drive.cpp`, `drive.h`) | Turns a position into four PWM duty cycles. Nothing above this layer knows what driver chip is used. |
| Portal actions | The Needle tab's buttons. |
| Upload hooks | Stop the needle for an S3 upload; resume it if the upload does not end in a restart. |

The needle runs on two FreeRTOS tasks:

| Task | Core | Priority | Stack | Rhythm |
|---|---|---|---|---|
| Step emitter | 0 | 19 | 4096 bytes | 1 kHz control tick. Busy-waits between steps while moving; yields when idle or when the next event is more than 2 ms away. |
| Supervisor | 1 | 4 | 4096 bytes | One pass every 5 ms; 20 ms while a bring-up tool holds the needle. Reads the angle sensor every 20 ms and its status every 25th read (every 500 ms). |

Two other code paths call into the needle: the main board's `loop()` (core 1, priority 1), which
runs the policy and the console, and the web portal task (core 0, priority 3), which runs the
Needle tab's buttons.

**Why the emitter is on core 0.** Core 1 carries the clock display's 25 µs timer interrupt and
`loop()`, which services the link to the audio board. The emitter busy-waits between steps, so on
core 1 it would starve `loop()` for the length of every sweep. On core 0 it shares time with WiFi
and the portal instead, so the portal can be slow while the needle moves.

**Why core 0's idle task is removed from the task watchdog.** The emitter starves core 0's idle
task on purpose while it moves. With the idle task still watched, the task watchdog reset the chip
in a boot loop that looked like a power problem. The step emitter therefore removes core 0's idle
task from the watchdog at startup. The cost is that nothing catches a permanently hung task on core
0; if that ever matters, watch the emitter task itself, and do not put the idle task back.

**The supervisor is watched** by the task watchdog (15-second timeout), and feeds it at the top of
every pass. Every branch of a pass sleeps 5 to 20 ms, so the watchdog trips only when the
supervisor has truly stopped turning. A stopped supervisor would leave the needle wherever it was
and deaf to every request; the watchdog restarts the S3 instead, and the needle memory (below)
carries its place across the restart. The step emitter is still not watched, because it busy-spins
by design; neither is the portal task.

## Ambersong firmware: units and the coordinate frame

- **Half-step (hs).** The unit of every geometry number: soft limits, index band, budgets, drift.
  One half-step of the stepper, counted from the homed zero.
- **Fine unit.** 256 fine units = 1 half-step, and 2048 fine units = one electrical revolution of
  the motor, which is 8 half-steps. The low 11 bits of a fine position are therefore the electrical
  angle, so the step path needs only a mask and table lookups, no division.
- **Speeds** are in half-steps per second (hsps), accelerations in hsps² in the settings.
- **Position and target** are 32-bit signed fine positions, chosen because the CPU has no atomic
  64-bit load and these values are read on one core while written on the other. The whole travel is
  1869 hs, about 478,000 fine units, so 32 bits has room to spare.
- **Zero** is the index switch's **forward ON edge**: the point where it switches on when the
  needle approaches it moving in the + direction.
- **+ is toward high FM.** On this machine, advancing the motor's electrical phase moves the needle
  toward low FM. That inversion lives only in one drive-layer setting, so everything above the
  drive layer can say "position increases toward high FM" and mean it. The direction is a physical
  value of this machine, held by the firmware (below, "Physical values").
- **Encoder counts.** The angle sensor gives 0..4095 per shaft turn. The running count is a
  multi-turn total; the tuner's two mechanical ends are stored as that count.
- **Frequency** is in tenths of a MHz, e.g. 1079 = 107.9 MHz.

The published needle position is in **whole half-steps**. A movement smaller than one half-step
does not show in it.

## Ambersong firmware: the drive layer

The drive layer turns a fine position into four PWM duty cycles, one per input of the Darlington
driver.

- **PWM.** Four LEDC channels at 20 kHz, above hearing, negotiated down from 11-bit resolution one
  bit at a time until the hardware accepts it (11 bits at 20 kHz only just fits the clock the PWM
  runs from). If nothing is accepted, the console reports it and the four outputs do nothing; what
  resolution was actually achieved is kept and shown by the console. The panel lamp channel and the
  audio board's Bluetooth-lamp channel are kept distinct from the stepper's channels across both
  firmwares.
- **Why PWM at all.** It is the only way to microstep a unipolar motor through a plain Darlington
  array. Microstepping places the rotor between the motor's natural step positions by giving the
  coils partial currents. Plain half-stepping chatters visibly at the low speeds a needle uses — its
  two-coils-on positions pull about 41% harder than its one-coil-on positions. At high speed the
  advantage vanishes, because the winding cannot follow the waveform fast enough; the needle never
  needs that speed.
- **The waveform.** A table holds the positive half of a sine wave, scaled to the PWM range. The
  four outputs are the positive parts of cosine, sine, −cosine and −sine; a unipolar half-winding
  conducts one way only, so a signed sinusoid becomes two rectified ones on opposite half-windings.
  At 45 degrees the two active outputs sit at equal, non-zero values: the current vector has
  constant magnitude, which is why microstepping is smoother than half-stepping. Which pair of
  inputs forms one phase, and in what turning order, is a physical value of this machine, held by
  the firmware (below, "Physical values").
- **Modes.** Wave, full-step, half-step and micro-step. In micro-step mode the division is rounded
  **down** to a power of two, at most 1/64, so every microstep is a whole number of fine units.
- **Duty.** Full duty is used. The motor's supply is a physical value of this machine (below): it is
  a 5 V part, so full duty on that supply is safe. The duty is kept as a parameter because it is the
  only thing that would stand between a 5 V motor and a higher supply on another build.
- **Releasing the coils** sets all four duties to zero, leaving the coils slack and unpowered.

## Ambersong firmware: the step emitter

The step emitter is the only code that emits a step in normal operation. Each loop:

1. **Fold in any pending correction** from the supervisor (below, "Stop, abort, and who writes the
   position").
2. **Stand aside for bring-up tools**, if one holds the drive.
3. **Every 1 ms control tick:** if a stop or abort is pending, velocity goes to zero and the move
   ends. Otherwise, if a constant-velocity ("manual") speed is set, use it; otherwise run the motion
   profile (below). When a move has been finished for 400 ms, the coils are released — holding
   costs current and heat, and the needle has no load to hold. That the mechanism stays in place
   with the coils unpowered is a physical value of this machine (below).
4. **While moving:** the step interval is one step unit divided by the speed, clamped to at least 20
   µs (a ceiling of 50,000 steps per second) and at most 1 second, and the deadline is pulled in on
   every pass as the profile accelerates — without that, a profile leaving rest slowly can schedule
   its second step from its slowest velocity and then wait far too long for it.
5. **At the deadline:** if the step is allowed by the soft limits, the position moves one step unit
   and the coils are updated. If not, the needle is at a soft limit and the move stops there. The
   worst lateness is kept and shown on the console; a late emitter never bursts to catch up, it
   re-synchronises.

**The count cannot drift.** Every step moves the position by exactly one whole step unit. There is
no fractional accumulator anywhere in the step path, so a million steps out and a million back land
on the number they started from. Any difference between the count and the needle is therefore the
motor, never the arithmetic.

**Soft limits.** A step is refused in the + direction at or above the high limit and in the −
direction at or below the low limit, but only while the limits are armed. They are disarmed while
homing, because zero is not yet known.

## Ambersong firmware: motion profiles

The motion was settled on a separate bench rig and moved into the firmware; the rig's results
became the firmware's settings (below, "Settings and constants"), not a standard the code is held
to. One of two laws runs for each move, chosen per move by the firmware:

**Second-order.** A damped mass on a spring, driven through an actuator with limits — the equation
of a moving-coil meter: acceleration equals a spring term toward the target minus a damping term on
velocity, clamped to the configured acceleration and speed limits. A damping ratio below 1
overshoots and settles; it is the overshoot knob. The ring-down frequency is the other knob, and
raising it **reduces** overshoot — with the speed already clamped, a stiffer system arrives with
less momentum. The law only brakes once the error is small, and does not know the motor cannot
brake harder than its acceleration limit; braking must start before the stopping distance, which
gives a **gain ceiling**: the ring-down frequency must stay below four times the damping ratio
times the acceleration limit, divided by the speed limit. At the defaults (damping ratio 0.50,
acceleration 18,000 hsps², speed 1100 hsps) the ceiling is 32.7 and the ring-down frequency is 9.
The firmware warns at boot if the setting is over the ceiling. Over the ceiling the failure looks
deceptive — a profile over the ceiling can look like it does not move the motor at all, because a
bare motor asked to reverse hard only buzzes. Recompute the ceiling whenever the speed or
acceleration limit for this law changes.

**Decay.** A planned, feed-forward fall: the drive "dying away". The speed follows a rise, then an
exponential decay, scaled so that its integral is exactly the distance to travel. If the peak speed
would exceed the park's speed limit, the fall time is stretched rather than the peak clipped,
because clipping would leave the needle short. A small position-error term keeps the needle on the
plan. The decay law does not use an acceleration limit directly; the park's acceleration setting
enters only the arrival test below.

**Dwell.** A pause before a commanded move starts: 100 ms by default before a second-order move,
1000 ms by default before a decay move. The pause is most of why a mechanism reads as mechanical.
**No dwell while tracking**: tracking issues a new move for every new target, and a dwell there made
the needle seem to refuse to move until the knob had gone some distance — it looked like a
threshold, but it was a timer restarted by every new target.

**Arrival.** A move is finished when the needle is within **one step unit** of the target **and**
its speed is below a threshold that scales with the square root of the acceleration limit (with a
floor of 20 hsps). Both rules exist because a fixed threshold gave a one-microstep limit cycle (a
move that should settle in 2 seconds instead took over 21), and a half-unit tolerance can never be
met once the position is off the microstep lattice.

**Which profile runs when:**

| Move | Law | Microstep | Dwell |
|---|---|---|---|
| Homing: search, backoff hops, re-approach hops | Second-order | unchanged (whatever was last set) | 100 ms before each hop |
| Sweep legs (both directions) | Second-order | fast (1/16 by default) | 100 ms |
| Tracking (both directions) | Second-order | slow (1/32 by default) | none |
| Park (at the low soft limit) | Decay | fast (1/16 by default) | 1000 ms |
| Band-calibration creep | Constant speed | fast | none |
| Jogs (bring-up tools) | Constant speed, emitter suspended | half-step or fast | none |

So "up" and "down" in the setting names (below) are names of the two laws, not directions. The
second-order speed and acceleration limits govern every profiled move except the park; the decay
limits govern only the park.

**Microstep changes only at rest.** Changing the microstep division while a move is running would
leave a target that whole steps can never reach, so the firmware refuses to change it mid-move.
Each division has its own lattice.

## Ambersong firmware: the needle supervisor, one pass

Every 5 ms, the supervisor:

1. Feeds the task watchdog, then carries out any posted stop or calibration abort.
2. Runs the hunting detector (below).
3. Reads the index switch once, for motion purposes (below).
4. Runs the band check (below).
5. Updates the needle's memory in RTC RAM (below).
6. Every 20 ms, reads the angle sensor; every 25th time, reads its status.
7. If a bring-up tool holds the needle, sleeps 20 ms and starts again.
8. If a band calibration is running, advances it and sleeps 5 ms.
9. By state: homing states advance the homing state machine. SWEEP runs the sweep sequence, or
   abandons it on a slip. TRACKING plans a new move if the target differs from the position by 2 hs
   or more. PARKED re-parks if the position is 2 hs or more from the low soft limit.
10. Runs the edge log (if enabled) and the crossing re-reference (below).
11. Sleeps 5 ms.

## Ambersong firmware: reading the index switch

The index switch is read in exactly one place, once per supervisor pass, before anything branches
on state. Homing, the crossing check and the band calibration all read the debounced result and
never the raw pin. The pin is read as active LOW: the switch output is LOW while the magnet is over
it and pulled up otherwise (`hardware-bible.md`).

**Debounce: 4 agreeing polls.** A new level is believed only when four consecutive 5 ms polls
agree. The number is derived from the mechanism, not picked arbitrarily:

- To reject glitches of 10 ms or more, three poll intervals (the delay before the fourth confirming
  poll) must exceed 10 ms, which needs at least 4 polls.
- To still see a fast crossing, the narrowest ON region of the switch (94 hs, below, "Physical
  values"), crossed at the fastest speed near it (1700 hs/s, the park speed), lasts about 55
  milliseconds, about 11 polls; seeing both edges of it needs about twice the debounce count, so the
  debounce count must stay at 5 or below.

Four polls leaves about three polls of margin.

**Back-dated, not delayed.** When the raw level first disagrees with the confirmed one, the
position, speed and time since the previous poll are recorded at that moment. If the run of
agreeing polls reaches four, the edge is reported at **that first snapshot**, not at the poll that
confirmed it. The debounce therefore costs three polls of delay before the edge is known, but adds
no error to where the edge is recorded; what remains is one poll interval of sampling uncertainty.
The recorded interval is clamped to 20 ms so a scheduling hiccup is not taken for a large lag.

The debounce is seeded with a live read at startup, so a magnet already over the switch at
power-up reads ON from the first poll, with no false edge — homing can start within about 15 ms of
the supervisor starting and must see the truth from its first look.

## Ambersong firmware: homing

**States:** idle, seek-index, backoff, re-approach, sweep, tracking, parked, fault.

```
  boot, console Home key, portal Home, amp-on while unhomed, a track/park request from idle unhomed
          |
     start homing   (refused while a calibration or bring-up tool holds the needle)
          |   already ON the switch? --> record a sighting, go to BACKOFF
          v
   SEEK-INDEX   one second-order move of up to 2336 hs in the chosen direction
          |  switch ON        -> record a sighting -> BACKOFF
          |  budget spent     -> warn, reverse once; spent again -> FAULT
          v
   BACKOFF      while ON: hop -20 hs; once OFF: one -60 hs clearance move
          |  more than 400 hs from the phase start -> FAULT
          v
   RE-APPROACH  +10 hs hops until a NEW confirmed ON edge
          |  hand the new zero to the emitter, wait for it to be folded in,
          |  then: homed, limits armed, index clue forgotten
          |  more than 300 hs from the phase start -> FAULT
          v
   automatic re-index?  -> go to whatever was requested next (no sweep)
   any other home       -> SWEEP: low limit, then high limit -> go to whatever was requested next

   stop request -> idle from anywhere;  a jog or coil test -> idle
   FAULT: nothing driven, coils released, reason recorded and published
```

**Direction.** The search goes toward the index clue if one is on record (below, "Recovery").
Otherwise it goes by the sign of the current position: positive searches down, zero or negative
searches up. At a cold boot the position is assumed at the low stop, which is negative, so the
first search goes up.

**Budgets.** Every phase has its own step budget, because the soft limits are disarmed while zero
is unknown and nothing physical stops the needle:

| Phase | Budget | Derived from |
|---|---|---|
| Search (per direction) | 2336 hs | The measured travel, 1869 hs, plus 25% margin. |
| Backoff | 400 hs | About twice the worst case (band edge, plus 60 hs clearance, plus one 20 hs hop). |
| Re-approach | 300 hs | About 2.4 times the roughly 127 hs normally needed (60 hs clearance plus the direction offset). |

The travel (1869 hs, with the index 1195 hs above the low stop and 674 hs below the high stop) and
the offset between the ON regions seen moving + and moving − (about 79 hs) are physical values of
this machine, held by the firmware (below).

In the normal case the search budget is never spent — SEEK-INDEX stops the moment the switch
confirms ON. When the first direction's budget is spent, the firmware warns that the index was not
found in the expected direction and tries the other way once.

**Backoff.** While the switch is ON, BACKOFF moves −20 hs at a time. Once it reads OFF, it moves a
further −60 hs of clearance. The final approach therefore always comes from below.

**Re-approach, and what the "measuring pass speed" setting really drives.** RE-APPROACH moves
**+10 hs at a time, each hop an ordinary second-order move** under the tracking speed and
acceleration limits, with the usual dwell before each hop, until a newly confirmed ON edge. The
setting labelled "measuring pass speed" in the portal is **not** used here, despite the name. It
drives exactly two things: the speed of the band calibration's four creeps, and the speed of the
portal's jog buttons. A 10 hs hop never approaches the top second-order speed; the profile itself
keeps it slow. The effective speed of the re-approach is not a setting and has not been measured.

**Zero.** On the new ON edge, the shift that makes the back-dated edge position read 0 is computed
and handed to the step emitter. The homing logic then **waits** until the emitter has folded the
shift in before it marks the needle homed, arms the soft limits, forgets the index clue, and plans
the sweep.

**FAULT.** A spent budget is handled by the fault routine. For a hand or boot home it goes straight
to FAULT: nothing is driven, the coils are released, and the reason — "index never found in either
direction", "sensor never released during backoff", "sensor never returned during the
re-approach" — is published for the portal. For an automatic re-index it retries instead (below,
"Recovery").

**Leaving FAULT.** FAULT drives nothing on its own. A person gets the needle out of it: the
portal's Home or Re-index button, the console's home key, or switching the amplifier on. Each of
these starts a fresh home, because a needle in FAULT has no zero.

**Refusals.** Starting a home refuses, and says why, while a band calibration or a bring-up tool
(held coil, jog) holds the needle. Starting a home clears the old fault reason and drift evidence,
but keeps the index clue. It does not check whether a home is already running: a second Home press
or an amp-on edge during a home restarts homing from where the needle is, with a fresh budget.

## Ambersong firmware: the sweep, and where the needle goes afterwards

After any home that is not an automatic re-index, the needle enters SWEEP: a second-order move to
the low soft limit, then to the high soft limit, then on to whatever was requested next. With the
"sweep repeatedly" request it continues back and forth until stopped.

Requests to track or park only name where the needle goes when the current busy phase ends. If
homing, a sweep or a calibration is running, they only record the request and leave the state
alone; if the needle is already homed they switch at once; if it is idle and unhomed they start a
home; otherwise (FAULT, or unhomed and not idle) they refuse and say so.

The main-board policy calls for tracking when the radio is live (amp on **and** source RADIO) and
for parking otherwise. That gate is a listening policy, not a statement about the tube set, which
runs whenever the amplifier does: the needle follows the dial while the radio is what is being
listened to (`firmware-gospel-main-board.md`). It does nothing while an S3 upload runs or while the
needle is unhomed. The policy re-applies the needle's mode when the source changes, when the
amplifier changes, when the homed flag changes, after a band calibration, and after an upload that
did not end in a restart.

On an amplifier-on edge, the policy also starts homing if the needle is not homed, or triggers a
decorative sweep if that setting is on. **The boot home always ends in a sweep; the decorative
setting adds one on each amplifier-on edge.** "Not homed" includes FAULT, so every amplifier-on edge
sends a faulted needle homing again (up to 2336 hs each way). This is kept on purpose.

## Ambersong firmware: tracking and parking

**Where to point.** The target position is mapped from frequency, linearly, between the printed
dial's low and high frequencies onto the low and high soft limits, clamped at both ends, using the
**unrounded** frequency. Three mappings are kept separate on purpose: shaft count to per-mille of
the capacitor's travel, shaft count to frequency (the tuning curve), and frequency to needle
position (the printed face).

Worked example, with the soft limits and dial values recorded on this machine (calibration data,
not firmware defaults): low limit −1156 hs, high limit +414 hs, printed face 87.9–107.9 MHz. The
face spans 1570 hs over 200 tenths of a MHz, 7.85 hs per 0.1 MHz. At 100.0 MHz the target is
−1156 + (1000 − 879) × 1570 / 200 = −1156 + 950 = −206 hs.

**A filtered shaft.** The needle follows a low-pass filter of the shaft count, moved 35% of the way
to each new 20 ms sample. A real turn of the knob arrives in about 100 ms; a one-sample spike is cut
to about a third. Every readout and the tuning-activity detector uses the raw, unfiltered count
instead.

**Deadband.** TRACKING plans a move only when the target differs from the position by 2 hs or more.
A move is a whole profile — accelerate, decelerate, settle — and one per half-step of encoder noise
would be audible. PARKED uses the same 2 hs deadband against the low limit, because a park that
settles one half-step short would otherwise be re-commanded forever.

**Tracking moves** are second-order at the slow microstep division with no dwell. **Parking** is a
decay move to the low limit at the fast microstep division, after a one-second dwell. The low soft
limit is both the low end of the dial and the park position.

## Ambersong firmware: passive re-referencing at every crossing

Every confirmed ON edge seen while homed is a free measurement of where the needle really is. On
each supervisor pass:

1. **Subtract the sampling lag.** The edge happened somewhere in the poll interval before the run's
   first confirming sample, so on average half an interval earlier (about 3 hs at 1100 hs/s and a 5
   ms poll interval).
2. **Score only a directional edge.** The switch turns on at different places moving up and moving
   down (its hysteresis). An edge moving + is compared with the forward reference (0, by
   definition). An edge moving − is compared with the reverse reference, but only once the band
   calibration has measured it. An edge at rest, or moving − with no band measured, is counted but
   nothing is concluded.
3. **Drift = where it was seen minus where it should be.**
4. **|drift| ≤ 40 hs: absorbed.** The frame shifts by the negative of the drift, folded in by the
   step emitter. The slip flag is cleared and the index clue retired — the frame has just been shown
   right.
5. **|drift| > 40 hs: not believed.** The number never enters the frame. It becomes a clue to where
   the index actually is, a slip is raised, and a warning is printed. Recovery follows (below).

Nothing is corrected while a calibration runs, because the calibration is measuring these same
edges. Crossings during a jog are not scored either.

40 hs is about half the roughly 79 hs gap between the forward and reverse ON edges — the one
systematic error that could masquerade as drift, and in fact half a half-step above that figure. So
a crossing read in the wrong direction is absorbed only on top of a drift that size already; with
less, it is treated as a slip. The 79 hs gap is a physical value of this machine, held by the
firmware (below).

## Ambersong firmware: the band check

The crossing check (above) scores ON edges only. It cannot see steps lost **after** a correct ON
edge while the magnet is still over the switch. So on every supervisor pass, while homed,
band-calibrated, not calibrating, not in a bring-up tool and not homing: if the debounced switch
reads ON while the position is outside the measured band (with a margin), the frame is judged
wrong. The margin is 30 hs plus 40 ms of travel at the current speed, because the debounced level
lags the needle — a fixed 30 hs margin alone fired on an ordinary crossing, 34 hs past the true OFF
edge, purely from lag.

It is raised once per excursion: the index clue is set to the current position, a slip is raised, a
warning is printed, and recovery follows (below). It does not run before the band has been
calibrated, because with all four band edges unmeasured (at zero), every ordinary crossing would
read as a slip and re-index in a loop with no exit.

Example: with the band recorded on this machine (forward ON 0, forward OFF 97, reverse ON 79,
reverse OFF −15) and the needle at rest, the switch reading ON anywhere outside −45..127 raises it.
At 1700 hs/s the margin grows to 98 hs.

## Ambersong firmware: recovery — the re-index ladder, the idle check and the index clue

**The ladder:**

| Drift at a scored crossing | What happens |
|---|---|
| ≤ 40 hs | Absorbed silently. |
| > 40 hs, or the band check fires | A slip flag is raised; once the knob has been still for 3 seconds, a re-index starts. |
| A re-index fails | The soft limits are re-armed, the old frame is kept, and it tries again — up to 3 attempts. |
| The third attempt fails | FAULT, with "three re-index attempts failed. Something is mechanically wrong." |

A re-index starts when a slip is outstanding, no upload is running, no recovery is already running,
fewer than 3 attempts have failed, the knob has been still for 3 seconds, and no calibration runs.
It does not require the amplifier to be on. Its body is an ordinary home that ends in whatever was
requested next, with no sweep, and a failure is counted instead of faulting. On a failed attempt the
limits are re-armed and the homed flag and slip flag are restored, so the trigger comes round again.
A wrong frame stays bounded in the meantime: the limits are the right distance apart, only in the
wrong place.

**No zero, no re-index.** A re-index is a recovery of a frame that exists. When the needle has no
zero — it is in FAULT, or it was stopped before a home finished — the re-index routine instead runs
a plain home, which ends in the sweep like any other and goes straight back to FAULT if it fails
too. In practice only the portal's Re-index button on a faulted needle reaches this branch; the
automatic triggers all require a homed needle. The portal still answers "re-indexing" when this
happens.

**A slip during the sweep** does not wait for the 3-second stillness rule. The sweep sees the slip
and abandons itself, starting a re-index at once.

**The routine index check.** Once the amplifier has been known to be off for 2 minutes, once per
off period, with the needle homed and no recovery, calibration, tuning measurement or upload
running, a re-index is triggered. The off period's check counts as done only if the re-index
actually started — a park that starts below the index never crosses it, so without this check an
open-loop needle could not learn of lost steps between listening sessions.

**The index clue.** The last place the index was seen, in the current frame, is recorded whenever a
homing search finds the switch, the band check fires, or a crossing is scored as a slip. It is
**cleared** only when the frame is made or shown true: a re-approach declaring zero, a band
calibration shifting the frame, or an absorbed crossing. Nothing that merely starts or stops a home
touches it, so a re-index cut short by a stop, an upload or a restart still leaves the next home
pointed at the index.

## Ambersong firmware: the band calibration

The four edges of the index's ON region — on and off, moving up and moving down — are measured by
a dedicated state machine, advanced once per supervisor pass rather than run as a blocking routine,
because it is reachable from a portal button and a blocking call would freeze the portal.

**Preconditions.** Homed, no slip outstanding, no homing or sweep running, and the needle sitting ON
the switch. Otherwise it refuses and says why.

**One pass** is four constant-speed creeps at 60 hs/s by default, each with a 600 hs budget:

| Step | Direction | Until | Records |
|---|---|---|---|
| 0 | + | OFF | forward OFF edge |
| 1 | − | ON | reverse ON edge |
| 2 | − | OFF | reverse OFF edge |
| 3 | + | ON | forward ON edge |

**Why constant speed, not a profile.** Profiles are for going somewhere known; a creep moves until
the switch says stop and cannot know the distance in advance. A profiled move would mean a fresh
dwell and acceleration ramp every few steps, so the emitter has a constant-speed mode that overrides
the profile entirely — the jogs use it too. At the default creep speed the sampling lag is about
0.15 hs on average, which is why these edges can serve as the reference that faster crossings are
scored against.

The portal runs 3 passes by default; the console allows 1 to 12. **The soft limits stay armed**
during a creep — one that reaches a limit fails the calibration ("stopped at a soft limit") rather
than pushing through it.

**The result.** The frame is shifted so the mean forward-ON position is 0, and the other three edges
are stored relative to it; the needle is marked homed, the slip evidence and the index clue are
cleared, and a one-shot result is raised for the main-board policy to collect and write into the
stored settings. **Both soft limits move by the same shift**, because they are physical marks; if
the moved pair no longer brackets zero, they fall back to the provisional ±300 hs default. The
collector exists because, before it did, nothing copied the band result into the settings: it lived
only in RAM, and the very next settings-apply call (triggered by the next step of the calibration
procedure) put all four values back to zero while the position kept the shift — every limit captured
afterwards was wrong by that shift.

The last band result recorded on this machine: forward ON 0, forward OFF 97, reverse ON 79, reverse
OFF −15.

## Ambersong firmware: soft limits — capture, nudge, purge

- **The rule.** The low limit must be at or below zero, the high limit at or above zero, and the low
  limit strictly below the high limit. The index is a mid-travel mark, so any real pair brackets
  zero. An invalid pair is refused and the old pair kept.
- **Defaults: ±300 hs.** Deliberately narrow: with no end switches, a default must be a lower bound
  on the travel. It also has a lower bound: the post-home sweep at ±300 hs must cross the index
  band, or limit capture can never be satisfied.
- **Purge at load.** A stored pair that is crossed or does not bracket zero is reset to ±300 hs and
  saved. The band values are kept, because they are relative to the index, not to the limits.
- **Capture.** Storing the current position as the low or high limit refuses unless the needle is
  homed, no calibration runs, the band is calibrated, the index has been crossed **and scored**
  since the home (a jog does not count), and no slip is outstanding.
- **Nudge.** The portal's limit-nudge control moves **both** limits by the same amount (1 to 200 hs).
  It slides where the needle points for every frequency; it does not move the needle and it is not a
  clearance tool. It refuses a result that would not bracket zero. The buttons were explicitly
  labelled "Needle Limit" rather than something that suggests moving the needle itself, after an
  attempt to use them that way.

## Ambersong firmware: the tuning encoder, as the needle sees it

The angle sensor is read at 50 Hz; the difference from the previous reading, wrapped to ±2048, is
added to a running multi-turn count. A separate status read keeps the raw status byte (magnet
detected, too weak, too strong), the automatic-gain-control value and the field magnitude
separately, because "no magnet" collapses three conditions that need different fixes. The sensor
answers on its own I2C bus (`hardware-bible.md`); the 100 kHz bus clock is a firmware choice.

**Bus failure.** On an I2C error the reader backs off to one retry every 2 seconds and counts the
outage. While the bus is down the shaft count is frozen and the needle holds its last target.

**Re-seating after a gap.** After any gap the count is not accumulated across the gap; the fresh
angle is placed on a turn as follows:

- With **both tuner ends measured**, the tuner's whole travel lies inside one 4096-count turn, so
  exactly one turn puts the angle inside a margin around those ends. That turn is taken, whatever
  moved during the outage.
- Otherwise (ends unknown, or the measured window is a whole turn or wider, or the angle falls
  outside it), the turn **nearest the last count** is taken, which is exact only for movement under
  half a turn.

A movement hidden by the outage still counts as tuning activity. That the tuner's travel is less
than one shaft turn (about 2200 counts measured) and that its ends repeat within about 300 counts
are physical values of this machine, held by the firmware (below). The code checks the span at run
time: one too wide for a single turn falls back to the nearest-turn method.

**At boot**, the measured tuner ends are set first, then the shaft is seeded on the right turn from
the last saved count. The live count is saved every 10 seconds when it changed and the encoder is
answering; a placeholder count from before the first good read is never saved.

**Tuning activity** is considered active for 400 ms after the knob moved. Movement is detected by
adding up same-direction changes until they reach 12 counts; a reversal resets the sum, so a slow
turn still counts while jitter does not. It drives the display's tuning readout and the "knob is
still" timers, including the 3-second re-index wait.

## Ambersong firmware: the needle's memory across a software reset

A software reset does not move the needle; a power cut happens with the needle wherever it last
was. The boot-position routine uses this.

**The record.** A small structure in RTC memory that survives a software reset: a magic number, the
position in fine units, the homed flag, the index clue's valid flag, the clue position, and a
checksum.

**Writing.** The record is written by the needle task alone, every pass in which anything changed,
with the magic number cleared first and set last, so a reset in the middle leaves it invalid.
Nothing is recorded while a new zero is in flight, or before the boot frame has been set — the
needle task starts before `setup()` reaches the boot-position routine, and would otherwise overwrite
the record with the cold position of zero.

**Reading.** The boot-position routine reads the record once and invalidates it before doing
anything else, so it is never used twice. It is used only when the reset reason is one that leaves
the needle where it was: a software restart (portal reboot, OTA, console), a panic, an interrupt
watchdog, a task watchdog, or another watchdog reset. It checks the magic number, the checksum, that
both flags are 0 or 1, and that both positions are within about 6000 hs of a sane range. If all
hold, it seeds the position, snapped to the current microstep lattice, restores the index clue, and
reports that the needle is remembered and will home from there.

Otherwise — power-on, brownout, the reset pin, deep sleep, unknown, or a missing or torn record —
the position is assumed at the low stop. That value is **a direction hint, not a position**: it
makes the position negative so the search goes up, and it is wrong by up to the whole travel,
harmless only because nothing moves the needle while it is unhomed. That the needle is at the low
stop at power-on (the firmware parked it when the amplifier went off, and it does not move while
unpowered), and that a software reset leaves the rotor where it was to within a detent, are
physical values of this machine, held by the firmware (below).

**A watchdog reset counts as a software restart.** It therefore ends in a restart that homes from
the remembered position, not the low-stop assumption.

**Homing always runs afterwards.** The record is a better starting belief for the search direction,
not a substitute for finding zero. It is kept in fine units so the drive's electrical phase matches
where the rotor actually sits.

## Ambersong firmware: needle boot order

As far as it concerns the needle, boot runs:

0. The task watchdog is set up first of all, so the supervisor can subscribe to it when the needle
   module starts.
1. Settings are loaded, then corrupt soft limits are purged.
2. The link self-test runs; a failure is reported and the boot carries on, so the needle is started
   and homed as usual — only the image's confirmation is withheld
   (`firmware-gospel-main-board.md`).
3. The display and panel are set up, then the needle module: the index pin as input with its
   debounce seeded from a live read, the drive layer started, direction inverted, full duty, fast
   microstep division; the I2C bus started; the first angle and status reads; the gain-ceiling
   warning (checked against the compiled defaults, since the stored settings are not applied yet);
   the step emitter and supervisor tasks started.
4. The tuning receiver is set up (`firmware-gospel-tuning.md`).
5. The tuner's calibration (both ends) is set, then the shaft accumulator is seeded from the last
   saved angle — ends first, because the seed uses them.
6. Settings are applied (around the start of the inter-board link), pushing every needle setting:
   limits, motion profiles, microstep, geometry (with write-back), index band, tuner ends, tuning
   curve, dial.
7. **The boot position** is set: the RTC record, or the low-stop assumption.
8. **The one homing** starts.
9. Networking and the portal start.

When homing finishes, the main-board policy sees the homed flag change and re-applies the needle's
mode; the sweep that follows homing is not interrupted by this.

## Ambersong firmware: stop, abort, and who writes the position

**Stop and abort are requests.** They can be issued from the console, from portal handlers and from
the upload hook — all of which may run on a different core from the needle's own tasks. They only
set a request flag and zero the speed; the step emitter checks the flag every control tick and
before every step, so stepping stops at once. The supervisor does the rest at the top of its next
pass, within 5 ms: it aborts a calibration, clears any manual speed, ends a recovery, ends a
repeating sweep, sets the target equal to the position, releases the coils, releases a held coil
test, and goes idle. A running jog is not interrupted by a stop.

**Who writes the position.** The step emitter alone owns the position value in normal operation.
The supervisor hands corrections over through a single correction value — one writer, one reader
that only adds it in, folded with an atomic exchange so nothing is lost between a read and a clear. A
new zero must wait for that fold before anything treats the needle as homed. This replaced an
earlier design where a stop issued from another core could race the homing and calibration code: an
abort's zeroed speed could be written straight back while a jog ran on toward a soft limit.

A few known exceptions still write the position directly rather than through a correction — the
end of a band calibration, the microstep-division change at rest, and the boot-position and
low-stop-assumption routines — and they are safe only because each runs with the emitter either
stopped or not yet started (below, "Known problems and limits").

## Ambersong firmware: hunting detection

"Hunting" is a needle that keeps stepping without going anywhere. The detector checks every 4
seconds: if at least 120 steps were emitted and the net movement is under 8 hs, hunting is
declared. Following the knob moves both numbers together, so only travel that cancels itself trips
it. It needs no theory of the cause — a hunt has one signature, whatever causes it — but it also
cannot see a needle that is stuck and not moving, a slow one-way error, or a wrong but steady
reading. The console and the portal both report hunting on the rising and falling edge; nothing is
driven differently, it is a sense, not a fix. The step count is of emitted steps at the current
microstep division, so the same physical oscillation trips it sooner at a finer division. It exists
because a real wobble was once found only by watching the driver board's LEDs directly, while the
published position (in whole half-steps) truthfully reported a stationary needle, because both
positions the needle alternated between rounded to the same half-step.

## Ambersong firmware: firmware uploads and reboots — pause and resume

A flash write disables the flash cache and stalls every task running from flash, including the step
emitter. So for an upload of the S3's own image:

- **Start.** The upload handler puts the needle into quiet mode, which stops it and blanks the
  display. While quiet mode is on, the needle's mode is not re-applied, amp edges do nothing, the
  recovery tick does nothing, and the idle check does nothing.
- **Success.** The image is written and the S3 restarts; the needle memory makes the boot home start
  toward the index.
- **No restart.** A refused image, an aborted upload, a failed write, or 15 seconds with no upload
  activity ends quiet mode and raises a resume flag. The main loop acts on it only once the stop has
  actually landed: it starts homing if the needle is unhomed, not recovering and not faulted,
  otherwise it re-applies the needle's mode.
- **A refused reboot.** The portal's Reboot first saves settings. If the save is refused because the
  needle is moving, the reboot stops the needle and retries for 1.5 seconds. If the save still
  fails, the reboot is refused and the same resume flag is raised.
- **A reboot while the S3 image is on trial** skips the save and the needle stop altogether and
  restarts at once, because nothing is saved during the trial and the reboot is what rolls the image
  back (`firmware-gospel-main-board.md`). The needle memory carries the position across it like any
  software restart.
- Settings writes are refused while the needle's speed is not zero, a calibration runs or an upload
  is active, and also while the image is on trial.

An upload relayed to the audio board does not stop the needle.

**An upload refused because the image is on trial** is decided on the first chunk, before quiet
mode is entered, so the needle is never stopped for it — it keeps tracking or parking as if nothing
had happened, and there is nothing to resume.

**A rollback.** A trial image that crashes or trips the task watchdog restarts with a reset reason
the needle memory accepts, and the bootloader starts the previous image, which reads the same RTC
record. The record is read by whichever image boots, so an image with a different memory layout
must change its magic number.

## Ambersong firmware: the needle on the portal and the console

**Portal state fields**: position, target (both in half-steps), state name, homed flag, drift,
pending-drift flag, drift-measured flag, calibration busy/percent/message, fault reason,
re-indexing flag and attempt count, hunting flag, the index switch (live and raw), step jitter,
I2C health, magnet status.

**Pills** (status-line indicators): a "needle" dot lit when homed; "NEEDLE OFF - re-index pending
(try n/3)" while a slip is outstanding; "NEEDLE FAULT"; "NEEDLE RE-INDEXING"; "NEEDLE HUNTING".

**Needle tab buttons** (admin only):

| Button | Notes |
|---|---|
| Home | Refusal shown if it cannot start. |
| Re-index | Refused during a calibration, bring-up tool, homing or sweep; with no zero it runs a plain home instead. |
| Sweep once / Sweep repeatedly | Refused unless homed. |
| Track the tuner | Refused when dropped. |
| STOP | – |
| ◀ 100, ◀ 10, 10 ▶, 100 ▶ (jog) | Clamped to ±200 hs, and says so if clamped; runs on the portal task and blocks it for the length of the jog. |
| Set low / high limit here | See "Soft limits" above. |
| Calibrate the index | See "The band calibration" above. |
| Abort calibration | – |
| Tuner = low end / high end | Refused if the encoder is not answering. |
| Needle Limit ◀ 20 / 20 ▶ (nudge) | Moves both limits; see "Soft limits" above. |

There is no park button on the portal. The tab also holds the tuning-curve actions
(`firmware-gospel-tuning.md`). The tab's status card shows the position, the state (with the fault
reason), the index switch live, and the calibration's progress and message.

**Console keys.** Every key is one command, case-sensitive unless two cases are listed.

| Key | Does |
|---|---|
| `H` `h` | Home. |
| `T` `t` | Track the tuner (says "NOT tracking" when the needle is dropped). |
| `P` `p` | Park (says "NOT parking" when the needle is dropped). |
| `x` `X` | Stop (also aborts a calibration and ends a recovery). |
| `r` | Repeating full sweep, until stopped. |
| `k` `K` | Band calibration, 5 passes (needle on the switch first). |
| `A` | Abort a calibration. |
| `m` / `M` | Capture the low / high soft limit here. |
| `n` / `N` | Jog −20 / +20 hs at 60 hs/s, microstepped. |
| `j` / `J` | Jog +200 / −200 hs at 200 hs/s, half-step mode. |
| `u` / `U` | Jog +200 / −200 hs at 200 hs/s, microstepped. |
| `1`–`4` / `0` | Hold one driver input at full duty / release. |
| `e` | Index edge log on or off. |
| `l` | Live index monitor, raw pin, up to one minute (feeds the task watchdog while it runs). |
| `c` / `C` | Store the tuner's low / high end. |
| `g` / `G` | Prints that the old end-stop calibration was removed and that `m` / `M` replace it; kept so a reader of old instructions is not met with silence. |
| `Z` | Clear the jitter counters. |
| `s` | Status: position, target, state, drift, crossings, jitter, steps, speed, PWM, and the encoder residue check. |
| `D` | Dump all settings. |

## Ambersong firmware: settings and constants for the needle

### Settings

All are rows on the portal's Needle tab (admin), in the downloadable settings file and in the
console's settings dump, unless marked otherwise. Stored in NVS on a 2-second debounce, never while
the needle moves and never while an S3 image is on trial.

| Name | Default | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `upVmax` | 1100 | hs/s | 100–4000 | Top speed of every second-order move: tracking, sweeps, homing. Portal label "Sweep up, top speed". The default is this mechanism's measured limit (below, "Physical values"). | Portal, settings file |
| `upAccel` | 18000 | hs/s² | 1000–60000 | Acceleration limit of the second-order law. | Portal, settings file |
| `dnVmax` | 1700 | hs/s | 100–4000 | Top speed of the decay (park) move. | Portal, settings file |
| `dnAccel` | 18000 | hs/s² | 1000–60000 | Used only in the park's arrival test. | Portal, settings file |
| `wn` | 9.0 | rad/s | 1–21 | Ring-down frequency; keep below 4 × `zeta` × `upAccel` / `upVmax`. | Portal, settings file |
| `zeta` | 0.50 | – | 0.1–2.0 | Overshoot (about 5° and a 300 ms tail at the defaults). | Portal, settings file |
| `riseMs` | 100 | ms | 10–3000 | Decay envelope rise time. | Portal, settings file |
| `fallMs` | 1000 | ms | 10–5000 | Decay envelope fall time. | Portal, settings file |
| `dwellUpMs` | 100 | ms | 0–3000 | Pause before a second-order move (not while tracking). | Portal, settings file |
| `dwellDnMs` | 1000 | ms | 0–3000 | Pause before the park. | Portal, settings file |
| `microFast` | 16 | 1/n | 1–32 | Microstep for sweeps, park, calibration, jogs; rounded down to a power of two. | Portal, settings file |
| `microSlow` | 32 | 1/n | 1–32 | Microstep while tracking. | Portal, settings file |
| `reapHsps` | 60 | hs/s | 10–500 | Speed of the band calibration's four creeps and of the portal jog buttons — nothing else, despite the portal label "Measuring pass speed". | Portal, settings file |
| `posMin` | −300 | hs | −3000–3000 | Low soft limit = park position = where the dial's low frequency is printed. | Console, portal capture, nudge, band carry, purge, settings file |
| `posMax` | +300 | hs | −3000–3000 | High soft limit = where the dial's high frequency is printed. | Console, portal capture, nudge, band carry, purge, settings file |
| `idxOffFwd` | 0 | hs | −500–500 | Index turns off, moving +. | Band calibration (normally only) |
| `idxOnRev` | 0 | hs | −500–500 | Index turns on, moving −. | Band calibration |
| `idxOffRev` | 0 | hs | −500–500 | Index turns off, moving −. | Band calibration |
| `idxOnFwd` | 0 | hs | not a settings row | Index turns on, moving +; 0 by definition. | Band calibration only |
| `sweepOn` | 0 | on/off | 0–1 | A full sweep on each amplifier-on edge. | Portal, settings file |
| `dialLow` | 879 | 0.1 MHz | 500–2000 | Frequency printed at the low needle stop. | Portal, settings file |
| `dialHigh` | 1079 | 0.1 MHz | 500–2000 | Frequency printed at the high needle stop. | Portal, settings file |
| `calLow` / `calHigh` | 0 / 6023 (placeholders) | counts | −20000–20000 | Shaft count at the tuner's two mechanical ends. | Console, "Tuner = low/high end", portal, settings file |
| `lastAngle` | 0 | counts | not a settings row | Last live shaft count; seeds the turn at boot. | Automatically, every 10 s |

`dialLow`/`dialHigh` defaults describe this set's printed face — 87.9 MHz at the low stop, 107.9 MHz
at the high stop — and are physical values of this machine held by the firmware (below). Another
set's builder types in their own. `posMin`/`posMax` are allowed a wide range because their validity
is checked by the geometry rule above, not by the portal range, and they are signed on purpose: an
earlier version of the portal clamped a negative high limit silently to zero and answered "ok". The
three band settings rows exist so the settings file carries them: a restore onto blank flash once
left all four band values at zero, silently switching off the band check and reverse-crossing
scoring.

### Compile-time constants

| Name | Value | Unit | Meaning |
|---|---|---|---|
| Control tick | 1000 | µs | Profile control tick. |
| Fine units per half-step / per electrical revolution | 256 / 2048 | fine units | Lattice unit; one electrical revolution. |
| Stepper PWM frequency / resolution | 20000 / 11 | Hz / bits | Negotiated down to 8 bits if 11 is refused. |
| Step interval clamp | 20 µs .. 1 s | – | 50,000 steps/s ceiling. |
| Coil release delay | 400 | ms | After a move ends. |
| Index debounce | 4 | polls | – |
| Sample interval clamp | 20000 | µs | Lag-model cap. |
| Largest drift absorbed | 40 | hs | – |
| Band-check margin | 30 | hs | Plus 40 ms of travel at the current speed. |
| Tracking/park deadband | 2 | hs | – |
| Search budget per direction | 2336 | hs | – |
| Backoff budget | 400 | hs | – |
| Re-approach budget | 300 | hs | – |
| Backoff hop / clearance | 20 / 60 | hs | – |
| Re-approach hop | 10 | hs | – |
| Failed automatic re-indexes before FAULT | 3 | – | – |
| Band-calibration creep budget | 600 | hs | Per creep. |
| Band-calibration passes | portal 3, console 5, 1–12 allowed | – | – |
| Hunting window / step threshold / net-move threshold | 4000 ms / 120 steps / 8 hs | – | – |
| RTC-record sanity bound | 6000 | hs | – |
| Angle-sensor poll / status interval | 20 ms / every 25th poll | – | – |
| Bus retry after a failure | 2000 | ms | – |
| Tuning-activity threshold | 12 | counts | – |
| Tuning-activity hold | 400 | ms | – |
| Shaft-filter smoothing | 0.35 | per 20 ms | – |
| Tuner-end seating margin | 300 | counts | – |
| Jog speed clamp | 5–900 | hs/s | – |
| Portal jog clamp | ±200 | hs | – |
| Limit-nudge range | ±1–200 | hs | – |
| Provisional soft-limit default | 300 | hs | Also used by the purge routine. |
| Knob-still time before an automatic re-index | 3000 | ms | – |
| Amplifier-off time before the routine index check | 120000 | ms | – |
| Shaft-position save period | 10000 | ms | – |

## Ambersong firmware: decisions behind the needle

**The needle is an electrical indicator of the tuning shaft, and it follows frequency through the
printed face.** The capacitor is one ganged assembly shared with AM and shortwave and turns wider
than the printed FM scale, so following the capacitor's travel directly compressed and offset every
station. Rejected: following the capacitor's raw travel directly.

**Motion profiles from a bench rig: a second-order law for every move but the park, a decay law for
the park, a dwell before commanded moves, microstep by activity.** A trapezoid profile reads as a
machine; a real meter movement overshoots slightly and settles. The rig's results: a damping ratio
of 0.50 and ring-down frequency of 9 gave about 5° of overshoot and a 300 ms ring-down; the fall law
gave a 1-second hesitation, a peak at about 15% of travel, a long trickle, and landed exactly. Step
throughput at 1700 hs/s: 1/8 and 1/16 microstepping delivered it in full, 1/32 delivered about 92%
(clamped by the step-interval floor). 1/32 is kept only for tracking, where smoothness shows most;
the sweep and park use 1/16. Rejected on the rig: an 850 hs/s trapezoid profile (read as a machine),
an s-curve profile (overshot before it could re-plan in time), an exponential approach (servo-like),
a spring-with-friction fall (could stop short, like a tired mechanism), and a random "wobble" (not
viable on this gearbox).

**`upVmax` is 1100, not the rig's 1700.** Going up through the index faster than this, the needle
stopped following while the firmware's position counter kept going, and every power-up sweep landed
about 400 hs (about 5 MHz) low. Sweeping the setting downward: 1500 failed every time, 1200–1400
often, 1100 never. The band check was added at the same time to catch a recurrence. This speed limit
is a physical value of this mechanism (below).

**No end switches: one mid-travel index, soft limits, step budgets and a starting belief.** This
motor and gearbox do not lose steps at the settled motion configuration, confirmed by a full sweep
returning to the index within about 17 hs and repeated returns agreeing within about 12 hs
(including gear lash); a long edge log showed trip points repeating within ±4 hs over dozens of
crossings, with the one large jump coinciding with a commanded stop. Steps are lost only when the
needle is driven past a physical stop. Rejected: two end-stop switches with homing against them.

**Home once, at power-up; the front switch does not home.** The S3 boots only when the set is
plugged in, so its boot is the first power-up; afterwards the position is known and every crossing
corrects it. The front switch only decides where the needle should point, and the run from the low
stop to a station is itself a sweep. Rejected: a search and full sweep every time the front switch
is turned on. Exception kept: a needle with no zero, including one faulted, homes on the
amplifier-on edge.

**Zero is one edge approached from one direction: the forward ON edge, after a backoff below the
band.** A Hall switch turns off at a weaker field than it turns on, so the ON region shifts with the
direction of travel; approaching one edge one way cancels that, and gearbox backlash the same way.
Rejected: reading the index inside the step emitter for more precision at speed — a gain of single
half-steps, at the cost of moving the sensor read across a task boundary on a mechanism with no end
switches.

**Every crossing re-references the frame, but only a directional edge is scored.** The two
directions trip about 79 hs apart, twice what may be absorbed, so a guessed direction produces a
confident, large, wrong number; the sampling lag is subtracted rather than absorbed as drift.
Rejected: taking the velocity sign at rest as the direction (it reads "forward" after every
completed move, which is wrong half the time); scoring reverse edges before the band is measured;
treating sampling lag as slip.

**Absorb up to 40 hs; above that, re-index — never trust the bigger number. Three failed attempts,
then FAULT. A failed attempt re-arms the soft limits.** Before this ladder existed, a drift over 40
hs only set a flag and printed a message, with no path back inside the window — a one-way ratchet,
measured once at 394 hs out after about 16h40 of ordinary use. Absorbing a large drift trusts a
suspect number and shifts the frame by it; re-indexing uses the number only as a direction hint and
takes the new zero from the switch itself — a better measurement, not a bigger dose of the same one.
An automatic re-index skips the power-up sweep, which should happen quietly between one thing and
the next. Rejected: raising the 40 hs ceiling; requiring several consecutive crossings to agree (the
index is crossed only about twice per power cycle); a separate "full search" rung (the search
already tries the other direction once).

**The band check: the switch reading ON outside the measured band means the frame is wrong.** The
crossing check alone cannot see a loss after a correct ON edge while the magnet is still over the
switch. Gated on a calibrated band, with a margin that grows with speed because the debounced
reading lags the needle.

**A slip during the power-up sweep ends the sweep at once and re-indexes, with no stillness wait.**
The sweep drives to limits in a frame the slip has just shown wrong.

**The automatic re-index waits for the knob to go still for 3 seconds, amplifier on or off.** Keep
tracking while the listener is tuning, and correct the moment they stop — the same signal that
returns the display from the tuning readout to the clock. With the set off, nobody is looking at the
dial, which is the best moment to spend the time putting it right.

**A routine index check two minutes after the amplifier goes off.** Parking never crosses the index
from below, so lost steps could otherwise go unseen until the next session.

**The index clue is kept until a better one replaces it.** A re-index cut short by a stop used to
leave no clue, so the next home fell back to a frame the slip had just proved wrong, with the limits
disarmed and a long search budget that could walk into a stop.

**The starting belief at boot: the low stop after power-on; the remembered position after a
software reset.** "Believe the low stop, search up" after a portal reboot or upload with the needle
above the index used to run away from the index toward the high stop. A brownout is excluded on
purpose — a count kept through a supply sag is not one to steer by. Rejected: a boot position saved
to NVS (never built); a cold position of zero (it searched down into the stop the needle was already
sitting on).

**Homing budgets are properties of the mechanism, not of the calibration.** Budgets derived from the
soft-limit span, with the provisional ±300 hs limits, gave too short a search against a needle
resting about 1195 hs below the index — it ran out going up, tried the other way into the low stop,
and faulted. Every homing phase now has its own budget derived from the measured travel and index
position, large enough that a magnet already over the switch (entering directly at the backoff
phase) is still covered.

**Soft limits must bracket the index; defaults ±300 hs; purged at load; carried with the band
shift.** A stored limit pair that did not bracket the index killed passive re-referencing and drove
the park into a stop. A default with no end switches must be a lower bound on the travel. Rejected:
a wider ±967 hs default, itself an estimate that sat past the measured high stop.

**The band calibration measures from the switch outward, never drives into a stop, and keeps the
soft limits armed; the earlier end-stop calibration was removed.** The end-stop calibration checked
nothing before it drove — not homed, not which side of the index — and disarmed the limits; run the
wrong one, it crept deep into the physical stop and left the frame badly out. Capturing each limit by
hand against a visible position replaces it: the operator places the needle, and the machine
measures back to the index, so nothing is driven into a stop while being calibrated.

**The needle follows the unrounded frequency, with a 2 hs deadband, from a filtered shaft reading;
no dwell while tracking.** Following the rounded tenth of a MHz made the target hop back and forth
across a rounding boundary on ordinary encoder noise — a pointer on a glass dial is an analogue
indicator and should not snap to tenths of a MHz. After the fix, resting-needle samples were
overwhelmingly stable at one position; the low-pass filter on the shaft reading removes the rest of
the noise sensitivity. There is no median filter anywhere in this firmware; the exponential filter
is the rule.

**The shaft's turn is chosen by the measured tuner ends.** Re-seating on the nearest turn after a
long outage was right only for movement under half a turn; beyond that it saved a wrong turn that
every later boot then inherited.

**Stop and abort are requests; the emitter alone writes the position; a new zero waits for the
fold.** An in-place stop issued from the portal's core used to race the homing and calibration code
running on the needle's own core — an abort's zeroed speed could be written straight back while a
jog ran on to a soft limit, and corrections written between a read and a clear were lost.

**The step emitter runs on core 0; core 0's idle task is removed from the task watchdog; no flash
writes while the needle moves.** An NVS write disables the flash cache and stalls the emitter —
measured step jitter of 383 µs during a write fell to 49 µs with the guard in place, and the display's
worst slot-timing error fell from 1257 µs to 8 µs in the same test. Portal slowness while the needle
moves is an accepted cost.

**Jogs are not bounded by the soft limits; their step count is.** The jog is how the needle is
placed to set a limit — bounded by the limits, a needle already sitting at a limit could never
widen it. The request is clamped to ±200 hs where it enters, and crossing a limit is announced.
After a jog the needle stays put: a hand command the machine overrides a moment later is worse than
no command at all.

**FAULT is left to a person.** If the firmware cannot fix it, a power cycle will not either. Nothing
automatic retries a FAULT; every way out is something a person does — Home, Re-index, the console
home key, or switching the amplifier on (kept as one of those ways by design).

**Refusals say why, and failures are visible.** A refusal once shown in a success colour made it
look as if limits had been captured when they had not. Every needle action that can be refused —
homing, calibration, tracking, parking, sweeping, the jog clamp, limit capture — returns a reason,
and hunting, pending slips, re-indexing and faults are all in the published state. The underlying
rule: every failure the machine can have must move some published field, at a resolution that would
actually show it — a position field that only shows whole half-steps technically covered a
sub-half-step oscillation, just not finely enough to be useful, and it took watching the driver
board's LEDs directly to find that.

**`idxOnFwd` is not a settings row.** It is 0 by definition; letting it be edited moved both soft
limits physically, into a stop.

**Re-index with no zero runs a plain home.** A re-index recovers a frame that exists; on a needle
that never had one, a failed re-index used to hand back an unmeasured frame and arm the soft limits
in it.

**`upVmax` is changed only through its settings row; the console speed-step keys for it are gone.**
One of those keys had no ceiling and could push the setting past the speed this mechanism can
actually follow.

**Track and park requests only name where the needle goes next; they never cut a busy phase
short.** They used to set the needle's state directly, which could overwrite the power-up sweep
before it even started. Now they set a "go here next" flag and leave a running home, sweep or
calibration alone, and they say whether the request will actually be carried out.

**An interrupted upload resumes the needle only after the stop has fully landed; a refused reboot
resumes it too.** A stop can take up to 5 ms to be carried out; a resume issued inside that window
would be undone by the stop, leaving the needle idle. A reboot refused because the settings save
failed had been stopping the needle for the save and never restarting it.

As with every decision in this edition, these were choices made for this particular mechanism;
another build's needle may need different ones.

## Ambersong firmware: failures and recovery for the needle

| What fails | What the firmware sees | What it does | How to recover | Class |
|---|---|---|---|---|
| A few steps lost (≤ 40 hs) | Drift at a scored crossing | Absorbed. | Nothing. | NOTE |
| Many steps lost (> 40 hs) | Drift at a scored crossing | Index clue set, slip flag raised, "NEEDLE OFF" pill; re-index after 3 s of a still knob. | Nothing. | NOTE |
| Steps lost with the magnet still over the switch | Band check | Same as above. | Nothing. | NOTE |
| A slip during the power-up sweep | Band check or crossing | Sweep abandoned, re-index at once. | Nothing. | NOTE |
| Steps lost while parked or parking below the index | Nothing until a crossing | Routine check after 2 minutes amplifier-off. | Nothing. | NOTE |
| An automatic re-index fails (1st, 2nd attempt) | A spent budget | Limits re-armed, old frame kept, retry. | Nothing. | NOTE |
| The third automatic re-index fails | – | FAULT, coils released, reason published. | Portal Home or Re-index (both run a plain home), console home key, stop then track, amplifier off then on. | DEFECT (a person's job, by design) |
| A hand or boot home fails ("index never found in either direction", "sensor never released during backoff", "sensor never returned during the re-approach") | A spent budget | FAULT at once. | As above; check the switch, its magnet, its pull-up and the motor. | DEFECT |
| Re-index pressed on a needle with no zero | – | Runs a plain home; a failure returns to FAULT, never to a guessed frame. | As above. | NOTE |
| The first homing search goes the wrong way | The first budget spent | Warning, one reverse pass. | Nothing, if the needle survives it. | BLOCKER class if the needle is driven against a stop (limits are disarmed; up to 2336 hs); made unlikely by the index clue and the RTC memory, not impossible |
| Power cut or brownout with the needle above the index | Power-on or brownout reset | Assumes the low stop, searches up first. | Nothing, if it survives the wrong-way pass. | Same risk as above; a brownout is accepted (above, "Decisions") |
| Torn or missing RTC record after a software reset | Magic number or checksum fails | Assumes the low stop. | Same as above. | NOTE when the needle was low; same risk as above otherwise |
| Hunting | Detector | Warns only. | Find the cause; stop the needle. | NOTE |
| Angle sensor stops answering | I2C error | Count frozen, needle holds, retry every 2 s, re-seats on return. | Nothing when it returns; a wiring fault otherwise (`hardware-bible.md`). | NOTE |
| Shaft turn ambiguous after a long outage or at boot | Re-seat | With both tuner ends measured: the one turn inside them. | Measure both tuner ends. | NOTE with ends measured; without them a wrong turn can be saved and inherited by every boot — BLOCKER class, cured by measuring the ends |
| Crossed or non-bracketing soft limits typed in | – | Refused, old pair kept, console warning. | Enter a valid pair. | NOTE |
| Corrupt soft limits in flash | Purge at load | Reset to ±300 hs and saved. | Home, band-calibrate, capture the limits again. | BLOCKER (guarded: the needle stays off the stops, but the limits must be captured again) |
| A creep in the band calibration reaches a soft limit | – | Calibration fails and says so. | Move the limit, or accept the result. | NOTE |
| An S3 upload | Upload start | Needle stopped, display dark. | Wait. | By design |
| An upload refused, aborted, failed or silent for 15 s | Upload hooks | Resume once the stop has landed. | Nothing. | NOTE |
| Portal reboot refused because the save failed | – | Needle resumed. | Retry later. | NOTE |
| Portal reboot while the S3 image is on trial | – | Restarts at once, no save; the previous image comes back and homes from the remembered position. | Nothing. | NOTE |
| A software reset, crash or watchdog mid-motion | Reset reason + RTC record | Homes from the remembered position. | Nothing. | NOTE |
| LEDC accepts no resolution at 20 kHz | – | Reported; the needle never moves. | A firmware change. | BLOCKER (never seen) |
| The supervisor task hangs | Task watchdog, 15 s | The S3 restarts; the needle homes from its remembered position. | Nothing. | NOTE (never seen) |
| A task on core 0 (step emitter or portal) hangs for good | Nothing (idle task and these tasks are not watched) | Nothing. | Power cycle. | BLOCKER class; accepted cost |

## Ambersong firmware: known problems and limits for the needle

1. **Portal needle actions run on core 0**, writing needle state from the portal task while the
   needle's own tasks run on core 1; only stop and abort are cross-core safe.
2. **The position is still written outside the step emitter** by the end of a band calibration
   (which can race the emitter), by the microstep-division change (at rest), and by the
   boot-position and low-stop-assumption routines (in `setup()`, before the emitter is doing
   anything). Left as a known note rather than fully unified.
3. **FAULT has no local sign.** Only the portal and the console show it, by design.
4. **Homing runs with the soft limits disarmed** under a 2336 hs budget per direction. A wrong
   starting belief — a cold boot with the needle left high, a brownout, a torn record — can drive
   toward the far stop before it reverses. In practice: switch the amplifier off, so the needle
   parks, before unplugging the set.
5. **A second home during a home restarts it** from wherever the needle is, with a fresh budget.
6. **The portal answers "re-indexing" when Re-index runs a plain home**, and its refusal during
   homing or the sweep says "a calibration or a bring-up tool has the needle".
7. **The portal jog blocks the portal task** for the length of the jog, up to about 20 seconds at
   the minimum creep speed.
8. **The gain-ceiling warning checks only the compiled defaults**, once at boot.
9. **The boot home always ends in a full sweep**; the decorative-sweep setting governs only the
   amplifier-on sweep.
10. **The soft-limit margin** (20 hs inside each end on this machine) is an open question: whether
    it is enough over days of amplifier on/off cycles. Before it was widened, the needle drifted
    about 24 hs an hour; after, 2 hs of drift in 3h20, then 2 hs at 1 day 20h with no re-index
    needed. Consistent with the concern being real, not proof — nothing counted the amplifier
    switch-offs in those windows.
11. **The absorbed-drift limit, the debounce count, the band margin, the homing budgets and the
    other values in "Physical values" below belong to this machine's mechanism.** A different
    magnet, switch position, travel or faster profile needs them re-measured and re-derived.
12. **A settings write can land during a move's dwell or a portal jog** (the speed reads 0 then)
    and make the jog stutter.
13. **The re-approach speed is not a setting** and has not been measured directly.
14. **A stop during a home leaves the needle idle and unhomed.** It re-homes on the next home
    request, track/park request, re-index, amplifier-on edge, or upload resume.
15. **The `upVmax` row accepts up to 4000 hs/s**, far past the 1100 this mechanism follows through
    the index.
16. **Not yet exercised on this build:** a slip over 40 hs found by a crossing (rather than the
    band check) driving the ladder; the three-failure FAULT; any homing FAULT since the budget
    fixes; the Re-index button on a needle with no zero; the RTC record after a crash and
    torn-record detection; the measured-ends turn choice after an outage of more than half a turn; a
    portal stop or abort landing in the middle of a band calibration.

## Ambersong firmware: physical values of this machine, held by the firmware

No other build will be exactly like this one, so this mechanism's physical values are kept here,
on the firmware side, rather than in the Hardware Bible. The Hardware Bible records what is wired to
what and how the mechanism is built — the parts, how they are mounted, what drives what. It does
not record how this particular mechanism behaves: which way the motor turns the needle, how far the
needle can travel, where the index magnet sits in that travel, how fast the gearbox will follow. The
firmware depends on all of these, and they are gathered here because another builder will not share
them.

**Compiled** means the value is written into the source and changes only with a new build.
**Setting** means it is a stored setting with a compiled default. **Measured** means the firmware
measures it on the machine at run time and stores it; only its compiled default belongs to this
machine. Positions are in half-steps from the homed zero (the index's forward ON edge); shaft
angles are angle-sensor counts, 4096 per turn.

### The motor and its drive

| Physical value | This machine | Kind | How another builder finds their own |
|---|---|---|---|
| Direction: advancing the motor's electrical phase moves the needle toward **low** FM | Inverted | Compiled | Jog the needle +200 hs: if it moves toward low FM with the inversion off, turn it on, and the other way round. Every position above the drive layer assumes + is toward high FM. |
| Coil pairing: two pairs of inputs, each pair one phase, in a fixed turning order | As built | Compiled | Hold each driver input in turn, then step in both directions. A needle that buzzes, stalls or moves backward on some steps has another pairing; change the table, not the wiring. |
| The motor is a 5 V part, so full PWM duty on its supply is safe | 5 V, 100% duty | Compiled | If your motor's rated voltage is below its supply, lower the duty before anything moves. The duty that gives the same coil current as 5 V continuous through this kind of driver is roughly 100 × 4.8 / (V − 0.2) %, about 41% at 12 V — worked out but not soak-tested on this build; check the temperature before trusting it. |
| The needle holds its place with the coils unpowered | Holds | Compiled | If your needle drifts or falls when released, remove the coil-release delay (and accept the heat), or re-reference more often. |
| The motor and gearbox do not lose steps at the settled motion configuration | No lost steps | Compiled | Log index edges over a long repeating sweep: the trip points must repeat. If they wander, slow the profile before anything else. |
| The speed the mechanism follows going up through the index | 1100 hs/s never lost steps; 1200–1400 often did; 1500 always did | Setting (`upVmax`) | Sweep the top speed downward, watching the drift reading over repeated sweeps, and keep a margin below the first failure. The park speed only needs to be the fastest the needle ever crosses the index; it is not limited by this. |
| Gear ratio, output turns to half-steps | 4096 hs per output turn (about 64:1) | Compiled (`HALFSTEPS_PER_REV`, `drive.h`), unused | Nothing to do — the firmware counts half-steps from the index and never needs the ratio. |

The motion feel itself — the ring-down frequency, damping ratio, acceleration limits, rise and
fall times, dwells, and which microstep division is used for which move — is a matter of taste
settled on a bench rig, not a physical value. It was judged on this motor and needle; a lighter or
heavier pointer may want it re-tuned by eye.

### The travel, the index and the soft limits

| Physical value | This machine | Kind | How another builder finds their own |
|---|---|---|---|
| Travel between the two mechanical stops | 1869 hs (about 164° of the motor's output shaft) | Compiled | Jog from stop to stop and read the position. |
| Where the index sits in the travel | 1195 hs above the low stop, 674 hs below the high stop; near 102.5 MHz on the printed dial, so listening below that never crosses it | Compiled | Read the position at each stop after a home. The index must be mid-travel for anything in this file to work. |
| The index band: the four switching edges | Forward ON 0, forward OFF 97, reverse ON 79, reverse OFF −15 | Measured (band calibration); compiled defaults 0 | Run the band calibration with the needle on the switch. |
| Offset between the ON edges seen moving + and moving − | About 79 hs | Compiled (`CORRECT_MAX_HS` kept at about half this) | Keep the absorbed-drift limit at about half your own offset or below, so a crossing read in the wrong direction is absorbed only on top of a drift that size already. |
| Narrowest ON region (reverse ON to reverse OFF) | 94 hs | Compiled (debounce count derived from it) | Re-derive the debounce count from your own band width and fastest crossing speed near the index. |
| How far outside the band the switch may still read ON | 30 hs plus 40 ms of travel | Compiled | Re-check against your own band and speeds; too small raises false slips, too large misses real ones. |
| Homing hops and budgets | Backoff hops 20 hs, clearance 60 hs, budget 400 hs; re-approach hops 10 hs, budget 300 hs | Compiled | Re-derive from your own band width; each budget must stay shorter than the distance to the stop it faces. |
| Provisional soft limits | ±300 hs | Compiled default | Keep it below the shorter side of your travel and above half your band width. |
| The soft limits themselves | −1156 / +414 hs | Measured (captured by hand) | Capture your own, after a home, a band calibration and one scored crossing. |
| The needle is at the low stop after a power-up | Low stop | Compiled (design assumption) | Only a direction hint: if your set is often unplugged while playing, the first search may go the wrong way first, within its budget. |
| A software reset leaves the rotor where it was, to within a detent | Yes | Compiled | Nothing to do unless your driver moves the rotor at reset. |

### The tuner and the dial

| Physical value | This machine | Kind | How another builder finds their own |
|---|---|---|---|
| The tuner's whole travel lies inside one shaft turn | About 2200 counts of 4096, for about 20 MHz | Compiled; checked at run time | Measure both tuner ends. If the span is a whole turn or more, the firmware falls back to the nearest-turn method, which is right only for movement under half a turn. |
| The tuner's ends repeat | Within 300 counts | Compiled | Keep the measured span plus twice the margin under 4096. |
| The tuner's ends | Measured values; compiled placeholders are wider than a real turn on purpose, so nothing reads them as a measurement | Measured | Measure both ends. |
| What the tuner reaches before any calibration | 88.1 MHz at the low end, 107.9 MHz at the high end | Compiled defaults | Calibrate the tuning curve (`firmware-gospel-tuning.md`). |
| The frequencies printed at the two needle stops | 87.9 MHz low, 107.9 MHz high | Setting | Read off your own dial glass and type it in. |

**What another builder does first**, in order: set the motor direction; check the coil pairing and
the duty; find the stall speed and set the top speed below it; jog to both stops and read the
travel and the index position; re-derive the homing budgets, the absorbed-drift limit, the
debounce count and the provisional soft-limit default if they differ, and rebuild; home; run the
band calibration; capture the soft limits; measure the tuner ends; type in the printed dial; then
calibrate the tuning curve (`firmware-gospel-tuning.md`).

## Ambersong firmware: maintaining the needle code

**Invariants that must hold:**

1. The low soft limit must stay at or below zero and the high at or above it, and the low limit
   strictly below the high; every writer goes through the settings-apply path and its write-back.
2. Only the step emitter writes the position in operation. Everyone else hands a correction
   through the single correction value. A new zero is not used until the emitter has folded it in.
3. The index pin is read for motion decisions only in one place, once per supervisor pass.
4. Every homing phase has a constant budget derived from the mechanism, never from the soft
   limits.
5. The provisional soft-limit default stays above about half the index band's width, so the
   post-home sweep crosses the band.
6. The absorbed-drift limit stays at about half the forward/reverse ON offset or below (40 against
   79/2 = 39.5 on this machine).
7. The debounce must fit: enough poll intervals to reject the glitches you want rejected, and few
   enough to still see both edges of the narrowest ON region at the fastest speed near the index.
   Raise the top speeds, or change the magnet, and recompute it.
8. The gain ceiling: the ring-down frequency must stay below 4 × damping ratio × acceleration limit
   / speed limit.
9. Arrival tolerance is one step unit; the speed threshold scales with the square root of the
   acceleration limit.
10. Never change the microstep division during a move; snap both position and target to the new
    lattice when it does change.
11. Keep the deadline pull-in in the step emitter.
12. At 1/32 microstepping, speeds above about 1560 hs/s are clamped by the 20 µs step-interval
    floor.
13. No flash writes while the needle moves; uploads stop it first.
14. Settings are append-only: bump the settings version and add a migration case for any new field.
15. Change the RTC record's magic number whenever its structure changes.
16. The direction inversion lives only in one drive-layer setting.
17. The stepper's LEDC channels are reserved on both firmwares' numbering.
18. The low-stop assumption is a direction hint; nothing may read its position as true.
19. The index clue changes only through its own set/clear routines, and is cleared only when the
    frame is made or shown true.
20. Stop and abort stay requests; anything that sets a speed checks for a pending request first.
21. A refusal returns a reason; never report success for a request that was dropped.
22. A re-index needs a zero. Anything that can reach the real re-index routine on a needle that has
    none must go through the public re-index entry point, which homes instead.
23. The physical values above are this machine's. Change one only from a measurement, and
    re-derive every constant that depends on it.

**Traps:**

- "Up" and "down" in setting names are the two motion laws, not directions.
- The "measuring pass speed" setting does not set the homing re-approach speed, whatever its name
  suggests; it sets the band calibration's creep and the portal jog.
- The position is published in whole half-steps; a smaller oscillation is invisible in it. Watch
  the hunting flag, the jitter reading and the driver board's LEDs.
- A jog runs with the emitter suspended: its crossings are not scored or logged, and it does not
  satisfy the limit-capture guard.
- After a correct band calibration the forward-ON edge is 0 and the other three are not.
- The S3 console is native USB and does not reset when a monitor attaches: anything printed only
  at boot is never seen. Use the status key instead.
- A stop is carried out up to 5 ms later; reading the state right after issuing a stop may still
  show the old state.
- With the provisional ±300 hs limits, the whole printed face maps onto 600 hs and the needle hugs
  the middle of the dial. That is not a failed calibration; capture the real limits.
- A home ends by sweeping away from the index, so the needle is not on the switch afterwards. To
  run the band calibration, jog it back into the band first (about 40 hs above zero is well inside
  it on this machine) and watch the portal's live index field; the edge log prints nothing during a
  jog.
- A tuner end can be negative, and the "low" end can be the larger count — on this machine the low
  end measured 1024 and the high end −1173, the count running down as frequency rose. The seating
  routine takes the ends in either order.
- A gain over the ceiling looks like a profile that does not move the motor at all.
- Two individually correct changes can be jointly worse than the bug they each fix alone: a limit
  fix once fell back to a wide default on purge, which combined with a newly working sweep would
  have driven the needle deep into a stop on its first power-up. Neither half showed the problem on
  its own.

**How to test a change:**

- **On the radio, with the console open:** the edge log (every index edge with position and
  direction), the live switch monitor, the status key (position, target, state, drift, crossings,
  jitter, steps, speed, PWM, and the encoder residue check, which must stay constant), a repeating
  sweep (to watch drift over many crossings), the jitter-counter reset, and the settings dump.
  On the portal: Re-index to exercise recovery, and the state fields for drift, pending drift,
  drift-measured, re-indexing, retry count, hunting, fault and jitter.
- **Motion and drive changes** are best measured on a bare-motor bench rig before they reach the
  needle: a motor, its driver board and an ESP32, with a paper pointer taped to the shaft and every
  test ending where it began, so any pointer error shows up as lost steps. Run every return leg at a
  rate already known to be safe, so the return can never be what lost the steps. Worthwhile tests: a
  rate ladder (where it starts to buzz), an out-and-back at a fixed rate over many cycles (the real
  pull-out test), an unramped start (pull-in), an acceleration ladder, a many-turn run for the gear
  ratio, a thermal soak, and a backlash measurement (do that one early). Measure rates at the moving
  microstep division: at 1/32 any rate above about 1560 hs/s is clamped by the 20 µs floor (1700
  hs/s reads about 8% low).
- **Build** both firmware environments with the pinned toolchain (`firmware-gospel.md`) and check
  the build's exit code, not its output.
- **Flash** only a board positively identified, and not while someone is listening or watching the
  dial. Uploading through the portal is the normal route.
- **Review** needle changes adversarially — a reviewer briefed to disprove the change, sign-off on
  the final diff, then a live boot with the console open.
