# 1. Rescue: when the radio misbehaves

This chapter is for the moment something is wrong and you remember nothing about how the machine
works. Find your symptom, and try the steps in order, cheapest first. Stop as soon as the radio
behaves. Each section ends with the chapter that tells the full story. The last section is one table
of every light and portal pill, with its meaning.

## 1.1 First things first

**The machine has two small computers.** The **S3**, the main board, runs the clock display, the dial
needle, the panel lamps and the WiFi web page, called the **portal**. The **A32**, the audio board,
runs the sound, Bluetooth and the battery-backed clock chip. Both are powered whenever the set is
plugged in. The front switch only turns the amplifier on and off, so it restarts neither computer. A
**full power cycle** therefore means pulling the plug.

**The recovery ladder, cheapest first:**

1. **Wait.** Most faults heal by themselves.
2. **Press a portal button.**
3. **Reboot one board** from the portal (System tab).
4. **Switch the front switch** off and on. It restarts neither computer, but switching the amplifier on
   sends a needle that has no reference point (a FAULT included) homing again (section 1.5).
5. **Unplug the set**, then plug it in again.
6. **The USB cable** (section 1.12), last.

> **Caution — park the needle before you unplug.** Switch the amplifier off and let the needle fall to
> the low end of the dial before you pull the plug. A power-up assumes the needle is there (chapter 5).

> **Caution — download the settings file before you change any calibration.** Use **Download** on the
> System tab, Settings file card. It is the only undo (chapter 7).

## 1.2 The console without a cable

The S3 has a text console. It prints what it is doing, and every key you type is a command. You do not
need a cable to reach it:

1. Sign in to the portal as the administrator.
2. Open the **Console** tab.
3. Send **one key at a time**. Every character is its own command, so the portal refuses anything
   longer, except while a prompt is waiting for a whole line.

Lines from the audio board arrive in the same console, prefixed `[A32]`. Three keys are enough to
start:

| Key | What it prints |
|---|---|
| `s` | The status: amplifier, source, needle, index switch, angle sensor, tuning chain, portal counters, and the link to the audio board (`link : alive` or `SILENT`, frames received, checksum errors, frames of the wrong protocol version). Then the last reset reason, the uptime, and the `image` line, which tells whether a new firmware is on trial or was rolled back (section 1.9). It ends with the list of commands. |
| `i` | The network: joined, or `OWN ACCESS POINT`; the network's name, the radio's address and signal; portal sessions; network time fresh or stale; access-point clients; and what the WiFi driver really holds. |
| `?` | The list of every console key. |

Attaching to the console does not restart the S3, so you never see its start-up banner. Press `s`
instead.

*Every console key is in Appendix D. The console itself is explained in chapter 8.*

## 1.3 The portal will not open

1. **Try both addresses:** the radio's address on your house network, and `http://ambersong.local/`.
   A page that says "no answer from the radio." retries by itself after 2, 4 and 8 seconds, then
   every 15 seconds.
2. **Wait for the needle.** The portal is slow, or does not answer at all, while the needle moves.
3. **Stop all traffic.** About twenty to forty quick connections use up the radio's connection records. It
   then stops answering, although it still answers a ping. Close every browser tab open on the radio,
   wait one to two minutes, and try once.
4. **Look for the rescue access point.** When the S3 cannot reach the house network, it raises its own
   WiFi network, named **`Ambersong`**, and serves the same portal at **`http://192.168.4.1/`**. It comes
   up by itself:
   - at once, and for good, on a board with no house network stored;
   - 20 seconds after a failed attempt to join the house network;
   - 20 seconds after the house network connection drops.

   **It asks for no password.** Join it; on most phones the portal's sign-in page then opens by
   itself. If it does not, open `http://192.168.4.1/`. At most two devices can join at once. A blank
   board, with no house network stored, transmits at its lowest power, so stand close.

   > **Caution — the rescue network is open.** What you type on it, passwords included, crosses the
   > air unencrypted until the radio goes back to the house network.

5. **Signing in fails.** A session ends after 5 minutes without use. After five wrong passwords, a
   device is locked out for 1 minute, then 2, then 4, and so on up to 64 minutes. A lost administrator
   password cannot be reset from the portal. The console key `~`, typed over the USB cable, erases
   **all** accounts and restores the shipped placeholder account.
6. **Forcing the access point.** The radio has no configuration buttons. From a portal that still
   answers, press **Raise the rescue access point** (System tab), or send the console key `Y`. The
   access point holds for 10 minutes, longer while someone is connected to it. Then the S3 goes back
   to the house network by itself.
7. **Nothing works, but the radio plays.** Nothing watches the portal or the WiFi software. A hang
   there leaves the radio, the clock and the needle working, and the portal dead. Unplug the set. If no
   access point appears afterwards and the house network is not joined either, you need the USB cable
   (section 1.12).

*Explained in chapter 8.*

## 1.4 Getting back to the house network

- **After a forced access point.** Wait out the 10 minutes, or disconnect your phone from `Ambersong`.
  The S3 retries the house network every 2 minutes, but **never while someone is connected to its
  access point**. After **WiFi off for 2 minutes** (System tab), it comes back by itself.
- **After a new router or a new password.**
  1. Join the rescue access point and open the portal.
  2. On the System tab, Network card, type the WiFi name and password. A blank password keeps the
     stored one.
  3. Press **Save network**. The button stays disabled until the form has been filled from the radio.
  4. Three seconds later, the S3 drops its access point and joins your network. Put your phone back on
     the house network.
- **Without a browser.** Over the USB cable, console key `y`, then the network name, a space and the
  password, on one line.

  > **Caution —** the `y` prompt echoes the password into the console log.

*Explained in chapter 8.*

## 1.5 The needle says FAULT, or points to the wrong place

**First, check that it is really wrong.** Several things the needle does are normal. It rests at the
low end of the dial when the amplifier is off, or when the source is not RADIO. It also rests there
when the link to the audio board is down, because the S3 then takes the source as AUX. It stays pinned at one end when the knob is past the printed scale. It
stops, with the clock dark, while the S3 is being updated. Two minutes after the amplifier goes off, it
quietly re-checks its reference point. All of that is normal.

**The needle's pills,** at the top of the portal:

| Pill | What it means | What you do |
|---|---|---|
| **NEEDLE OFF - re-index pending (try n/3)** | The needle lost steps: its motor missed some, so its count no longer matches where it is. It finds its reference point again by itself once the knob has been still for 3 seconds. | Nothing. |
| **NEEDLE RE-INDEXING** | That repair is running. | Nothing. |
| **NEEDLE FAULT** | Three automatic repairs failed in a row, or a homing started by hand or at power-up failed. The motor is released and the needle stays where it is. The reason is printed on the console. | Clear the FAULT (below). |
| **NEEDLE HUNTING** | The needle keeps stepping back and forth without getting anywhere. A warning only. | **STOP** on the Needle tab, or console `x`. |

**Clearing a FAULT, cheapest first:**

1. **Home** or **Re-index** on the Needle tab.
2. Console `H`.
3. The amplifier off and on again: the front switch starts a homing whenever the needle has no
   reference point.

Homing can fail again with `index never found in either direction`, or with a message that begins
`sensor never`. The console then suggests checking the index switch, its magnet, its pull-up resistor
and the motor. That is hardware: see Hardware Bible, chapter 8.

**Off by the same amount at every station, with no pill.**

1. Press **Re-index**.
2. If it is still off, compare the frequency shown on the portal's **Now** tab with the station you
   hear.
3. **The frequency is right:** the needle's mapping onto the glass is off. **Needle Limit ◀ 20** and
   **20 ▶** (Needle tab) slide where the needle points for every frequency.
4. **The frequency is wrong:** go to section 1.6.

*Explained in chapter 5.*

## 1.6 The frequency on the dial is wrong

The knob's angle becomes megahertz through a curve fitted to up to twelve samples. Three are **hand
marks** that you type. Nine are **RDA samples**: a small receiver chip inside the cabinet (an RDA5807M)
measures them by listening to the tube set's own local oscillator. A wrong sample, once stored, is
saved, so download the settings file first (section 1.1). All the buttons below are on the Needle tab.

1. **Measure.** Switch the amplifier on, select RADIO, hold the knob still, and press **Measure this
   dial position**. The result is stored only if it passes every check. A refusal says why, for
   example "Move the dial 0.3 MHz", "NOTHING FOUND ... nudge it", or "the dial calibrates only while
   you listen to the radio ...". When all nine RDA slots are full, a new sample replaces the most
   crowded one, or is not stored at all if it "would add the least coverage". Hand marks are never
   replaced. The radio also measures by itself, at most once every five minutes, while it plays the
   radio and the knob has been still for 8 seconds.
2. **Mark a station, or slide the whole dial.** Tune to a station you know, press **Mark station A
   here** (or B, or C), and type its frequency. **Dial −0.1 MHz** and **Dial +0.1 MHz** move every
   reading together. That hand correction is never erased by itself: **Reset the dial correction to
   0** is the only way back.
3. **Remove one bad sample.** Console `s` lists every sample with its **residual**, its distance from
   the curve. Residuals mean something from four samples on. Press **Drop one sample** and give its
   slot number.
4. **The tuning line says "tuner ends NOT measured".** Turn the knob fully down and press **Tuner = low
   end**; turn it fully up and press **Tuner = high end**.
5. **Every new sample is off by the same amount since the tube set was realigned.** Its intermediate
   frequency (IF) has changed. Set **Tube set IF**, then drop or re-measure the old samples.

To start over, **Clear all marks** returns to the stored straight line. It keeps the hand correction.

*Explained in chapter 6.*

## 1.7 No sound, the wrong source, or Bluetooth misbehaving

**No sound, cheapest first.** On the portal's **Now** tab:

1. **Is the amplifier on?** The audio board plays nothing while it is off.
2. **Is the source the one you expect?** **AUX never passes through the audio board**, so the audio
   board is silent on AUX by design.
3. **Mute and volume.** A volume set from the portal holds until the front knob moves.
4. **Is the `link` dot green?** The audio board goes quiet within 2 seconds of losing the main board
   (section 1.8).
5. **Is an audio-board update running?** The sound stops for about a minute while it writes.
6. **Were experiment buttons pressed on the System tab?** They are never saved, and **Reboot the audio
   board** undoes them.

**Still silent.** If the console shows `[A32] proto self-test FAILED - muted, BT closed, awaiting OTA`
every 10 seconds, see section 1.9. Otherwise press **Reboot the audio board**: it clears a stuck audio
task.

**The wrong source.** Only the audio board reads the front source selector, every 50 ms. The main board
learns the source from it four times a second, and shows AUX while the link is down.

**Bluetooth is not visible.**

- A phone can connect only on the **BT** source, with the amplifier on and the audio board awake.
- A phone that has paired before must reconnect **from the phone**. The radio never calls a phone.
- A new phone needs the **pairing window**. Open it with the front pair button (on BT only), or with
  **Pair** on the Now tab. It lasts 90 seconds (setting `lookTimeoutS`), with the blue lamp blinking
  regularly. Opening it first drops any phone already connected.
- With `connectable` off (Bluetooth tab), Bluetooth is closed.

**Bluetooth is visible when it should not be.** The radio should appear in a phone's list of new
devices only during the pairing window; anything else is a fault. **Reboot the audio board** rewrites
the visibility at start-up. **Forget every pairing** (Bluetooth tab) removes all stored phones.

*Explained in chapter 10; the audio board's awake and asleep states in chapter 9.*

## 1.8 The console says "A32 silent"

`[WARN] A32 silent. Clock and needle keep running.` means the main board has heard nothing valid from
the audio board for 2 seconds. The clock keeps time and the needle parks. The main board calls the
audio board every second, and prints `[PASS] A32 up: proto vN, firmware ...` when it answers. Most
silences heal by themselves.

1. **Read the link line** of console `s`: `link : SILENT rx N crc N wrong-version N`.
   - A **wrong-version** count above 0, with the pill **BOARDS RUN DIFFERENT PROTOCOL VERSIONS -
     flash both**, means the two programs speak different versions of the link protocol. Until they
     match, the main board cannot pass anything to the audio board, so the order matters. First update
     the **main board**, over its own WiFi, with a build whose protocol version matches the audio
     board's. Only then update the **audio board**, through the main board's portal, if it needs a new
     program too. If the audio board is still on trial after its own update, unplugging the set also
     works: it rolls back to its previous program.
   - A receive count (`rx`) that does not move points at the wiring or the power of the link (Hardware
     Bible, chapter 10).
   - Receive and checksum (`crc`) counts climbing together point at the quality of the signal.
2. **Right after an audio-board update:** a new program that never completes the handshake with the
   main board stays on trial (section 1.9). Unplug the set, and it rolls back to the previous program.
3. **The pill AUDIO BOARD SENDS NO STATE**, or `Flash BOTH MCUs` on the console: the two boards are
   talking, but the audio board's status reports do not fit this build. The sound is not affected. Flash
   both boards.
4. **Reboot the audio board** is sent over the link, so it cannot reach a silent audio board. A frozen
   audio board restarts itself within 15 seconds.

*Explained in chapter 9.*

## 1.9 An update went wrong

**Every new program runs on trial.** After an update over the air, a board confirms its new program
only after about a minute of healthy running:

- **the main board**, 70 to 90 seconds after start-up, once its network and portal are up. It prints
  `image confirmed (a minute of running, network and portal up).`;
- **the audio board**, one minute after its first handshake with the main board.

If the board resets for any reason before that — a crash, or a 15-second hang — it goes back to its
previous program by itself.

| What you see | What it means | What you do |
|---|---|---|
| The page reported a failure | The board's restart can beat its answer to the page. | Read the version shown as running on the System tab (main board), or `[PASS] A32 up: ...` on the console (audio board). |
| `this firmware is still on trial - retry in a minute` (HTTP 409), or `A32: still on trial - retry in 1 min` | That board is in its trial minute. | Wait for `image confirmed`, then upload again. |
| `- an earlier update was ROLLED BACK` at the end of the `image` line of `s`, or of the `[A32] boot:` line | The new program failed its trial. The board runs its previous one. | Read the reset reason, fix the build, upload again. |
| `[FAIL] proto self-test - this image will NOT be confirmed` | A broken main-board build is running. | If it came over the air: **Reboot the main board**, and it rolls back. If it was flashed by USB: upload a good build. |
| The new program misbehaves, still on trial | It is not confirmed yet. | Reboot that board from the portal: it rolls back. During the trial, **Reboot the main board** goes through at once, even over an unsaved change, and the previous program comes back. |
| `NOT saved yet - this firmware is on trial after an update ...` | The main board writes no settings during its trial. | Nothing. Changes apply at once and are written once the program is confirmed, about a minute later. |
| The new program misbehaves after `image confirmed` | It is permanent now. | Upload a good program through the portal. |

**An interrupted upload changes nothing.** The main board gives up after 15 seconds without data, the
audio board after 20 seconds, and the old program stays. The main board saves nothing during its
trial, so a rollback finds the settings exactly as the previous program left them. A change made
during the trial is lost with it. Other upload messages, such as "the A32 is not answering", "did not
confirm" or a wrong chip, are in chapter 3's *When it goes wrong* table.

*Explained in chapter 3.*

## 1.10 The clock shows the wrong time

| What you see | What it means | What you do |
|---|---|---|
| Blank digits | No valid time. Normal for a second or two after the main board starts. | If it lasts, the clock chip has no valid time and there is no network time. Join the house network, or set the time with console `W`: local time, `YYYY-MM-DD HH:MM:SS`. |
| `1017` or `0879` | A frequency (101.7 MHz, 87.9 MHz), not a time. | Choose what the digits show with `showTuning` on the Display tab. |
| `8888`, or dark while playing | A display test. | **Normal** on the Display tab, or console `9`. |
| Off by whole hours | The time zone. It is a POSIX time-zone rule on the Network card. The default is eastern North America, `EST5EDT,M3.2.0,M11.1.0`. | Set your own rule. |
| Off by minutes | The clock chip keeps the time; network time corrects it every hour while the `time` dot is green. | Without a network, set it with `W`. |
| 12-hour or 24-hour, not as wanted | The `hour12` setting. | Change it on the Display tab. |
| Pill **BATTERY CLOCK LOST ITS TIME - check its battery** | The clock chip answered with no valid time. Most likely its backup battery or the module is failing. | Set the time, from the network or with `W`. The pill clears at the next good reading. Then check the battery. |
| Pill **BATTERY CLOCK NOT ANSWERING** | The audio board is up, but the clock chip did not answer. | Hardware: see Hardware Bible, chapter 4. |

> **Caution — the `W` prompt always uses the eastern rule.** It converts what you type with the fixed
> rule `EST5EDT,M3.2.0,M11.1.0` and leaves that rule in force until your own rule is applied again: at the next
> restart, or when you save the Network card.

*Explained in chapter 4.*

## 1.11 Settings look lost, or SETTINGS LOCKED

| What you see | What it means | What you do |
|---|---|---|
| **SETTINGS LOCKED - newer version stored** | This program is older than the one that saved the settings. It runs on defaults, writes nothing, and refuses the download. | Flash the newer program again, or upload a **complete** settings file taken earlier. |
| Audio settings back to defaults after an audio-board update | The update changed their layout. | Upload a settings file exported before the update. |
| Audio rows show `n/a`, or edits are refused with "the audio board is not answering - its settings cannot be changed now" | The main board holds no current copy of the audio board's settings: the audio board has not answered since the main board started, or has gone silent. | Wait for the handshake, or press **Re-read the A32** (Audio tab). |
| Needle limits back at ±300 | The stored pair was corrupt and was cleared. | Home the needle, calibrate the index, and set the limits again (chapter 5). |
| Mute off after a restart | By design. | Nothing. |
| A restore did not bring back the accounts or the WiFi network | They are not in the settings file. A restore never touches them. | Change them on the portal (section 1.4, chapter 8). |

*Explained in chapter 7.*

## 1.12 When you need the USB cable

Few faults need it:

- no rescue access point and no house network;
- a lost administrator password (console key `~`);
- a confirmed audio-board program that hangs at every start, before the link comes up;
- too little free memory on the audio board for an update.

**Where.** Each board has its own USB-C socket on the back panel (Hardware Bible, chapter 4).

> **Danger — read the Hardware Bible's safety chapter first** (chapter 2, and the earth and grounds in
> chapter 5). The USB ground is the machine's DC ground, which sits on mains earth. It is **not** the
> tube radio's chassis, which sits on mains neutral.

**Before you write anything to a board**, identify it by its MAC address, and never let PlatformIO
(the build and upload tool, chapter 3) choose the port by itself. The main board's console is its native USB port, at 115200 baud.

*Explained in chapter 3 (identifying and flashing) and chapter 8 (the console).*

## 1.13 What the lights and pills mean

The portal's **pills** are the small labels in its header. Green dots show what is well; red pills
appear only when something needs attention.

| Where | What you see | What it means | See |
|---|---|---|---|
| Pill `link` | green dot | The main board and the audio board have shaken hands. | 1.8 |
| Pill `needle` | green dot | The needle has found its reference point. | 1.5 |
| Pill `time` | green dot | Network time was received within the last 4 hours. | 1.10 |
| Clock pill | `--:--` | No valid time. | 1.10 |
| Network pill | network name and signal in dBm | Joined to the house network. | 1.4 |
| Network pill | `access point` | Running the rescue access point. | 1.3 |
| Red pill | **NEEDLE OFF - re-index pending (try n/3)** | Steps lost; it repairs itself. | 1.5 |
| Red pill | **NEEDLE RE-INDEXING** | The repair is running. | 1.5 |
| Red pill | **NEEDLE FAULT** | Homing failed; the motor is released. | 1.5 |
| Red pill | **NEEDLE HUNTING** | The needle keeps stepping without getting anywhere. | 1.5 |
| Red pill | **AUDIO BOARD SENDS NO STATE** | The boards are talking, but the audio board's status reports do not fit this build. Flash both boards. If the audio board stops talking altogether, the `link` pill turns red instead. | 1.8 |
| Red pill | **BOARDS RUN DIFFERENT PROTOCOL VERSIONS - flash both** | The two programs do not match. Update the main board first, to the audio board's protocol version; then, if it needs a new program, the audio board through it. | 1.8 |
| Red pill | **SETTINGS LOCKED - newer version stored** | Settings saved by a newer program. | 1.11 |
| Red pill | **UPDATING** | An upload is running. | 1.9 |
| Red pill | **BATTERY CLOCK LOST ITS TIME - check its battery** | The clock chip has no valid time. | 1.10 |
| Red pill | **BATTERY CLOCK NOT ANSWERING** | The clock chip does not answer. | 1.10 |
| Clock digits | blank | No valid time, or a main-board update is writing. | 1.10 |
| Clock digits | `8888` | Display test. | 1.10 |
| Panel lamps | dark | Amplifier off, or a main-board update is writing. | — |
| Panel lamps | full | RADIO, knob moving. | — |
| Panel lamps | half | RADIO, knob still for 5 seconds. | — |
| Panel lamps | low | AUX or BT (or the link is down). | — |
| Blue lamp | dark | Not on BT, or the audio board is asleep. | 1.7 |
| Blue lamp | fast breath for 20 seconds, then slow breath | BT source, waiting for a known phone to reconnect. | 1.7 |
| Blue lamp | regular blink | Pairing window open: visible to new phones. | 1.7 |
| Blue lamp | double flash, then steady bright | A phone has connected. | 1.7 |
| Blue lamp | steady dim | Hung up from the portal. | 1.7 |

*The panel lamps are explained in chapter 4, the blue lamp in chapter 10.*
