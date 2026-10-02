# Appendix D. Console keys

Each board has a text console in which **every key is one command**. This appendix lists every key of
both, with where it is explained. Look here when a chapter or a failure table names a key, or when you
work at the bench with a USB cable. The keys are case-sensitive. A row reads like this:

- keys separated only by a space (`s` `S`) do the same thing;
- keys separated by a slash (`m` / `M`) are a pair of different commands, and the slashes in the
  description give each key's action in the same order (`m` sets the low limit, `M` the high one);
- keys on rows of their own (`y` and `Y`) are different commands.

The last column gives the sections that explain each key: 5.16 is chapter 5, section 5.16.

## D.1 Main board

**How to reach it.** The main board's native USB port, the USB-C socket on the back panel, at 115 200
baud; or, with no cable, the portal's **Console** tab, signed in as the administrator (chapter 8, section
8.9). Lines from the audio board appear there too, prefixed `[A32]`.

**Rules.**

- **One key per send** from the web console. Every character is its own command, so the portal refuses
  anything longer, except while a line prompt (`y`, `W`) is waiting, when up to 128 characters go
  through.
- **Any key not listed is ignored**, silently, including the Enter the portal appends to every send.
- The S3 does not restart when a console attaches, so its start-up banner is never seen: press `s`.
- **The web console has the cable's power**, including `z` and `~` (chapter 8, section 8.6.7).

| Key | What it does | Where |
|---|---|---|
| **Status and help** | | |
| `s` `S` | Status: the amplifier, source, needle, index switch, drive, angle sensor and tuning chain, portal counters, the link (`alive` or `SILENT`, frames received, checksum errors, `wrong-version N`, with "<- the boards run different protocol versions: flash both" when N is not 0), the last reset reason, the uptime and the `image` line (trial and rollback). For the needle: position, target, state, drift, crossings, jitter, steps, speed, PWM, and the angle sensor's residue, which must stay constant. For the tuning: the curve, the hand offset, each sample with its residual, the RDA5807M's state. Ends with the command list. | 1.2, 4.12.2, 5.16, 6.9 |
| `?` | The command list (also printed at the end of every `s`, and at start-up). | 8.9 |
| **Network** | | |
| `i` `I` | Network: joined or `OWN ACCESS POINT`, the network's name, the radio's address and signal; portal up or down, sessions, network time fresh or stale; access-point clients; what the WiFi driver really holds; and, only when not on the house network, a scan of every network heard, which holds the main loop for a few seconds. | 8.3.8, 8.9 |
| `y` | **Line prompt:** the house network's `name <space> password` (gives up 60 s after the last key typed; a blank line cancels). Stores them and rejoins 3 s later. Sets the WiFi name and password only; the NTP server and the time zone change only from the portal's Network card. **Echoes what is typed, the password included.** | 8.9 |
| `Y` | Raise the rescue access point now. It holds 10 minutes, longer while someone is on it, then goes home. | 8.3.4, 8.9 |
| `~` | Erase **all** portal accounts and restore the placeholder account (lost-password recovery). | 8.6.6 |
| `z` | Erase the WiFi radio's RF calibration in flash, and reboot. A transmitter diagnosis; it touches none of the radio's own calibrations. | 8.9 |
| `B` | Step the WiFi transmit power along 60, 44, 34, 28, 20, 8 quarter-dBm (15 down to 2 dBm), cycling, and print what the driver applied. The first press after a start-up always gives 44. The value is kept with the settings and written about 2 s later, though the key says "saved" at once. | 8.4, 12.4 |
| **Clock and display** | | |
| `W` `w` | **Line prompt:** local time, `YYYY-MM-DD HH:MM:SS` (gives up 40 s after the last key typed). Converted with a fixed `EST5EDT` rule, whatever zone the portal holds, which stays in force afterwards. Sets the main board's clock, and the battery clock only while the audio board answers; otherwise it says the battery clock was NOT updated. | 4.9 |
| `8` | Show `8888`: all segments. | 4.12.2 |
| `b` | Blank the display. | 4.12.2 |
| `9` | Back to normal. | 4.12.2 |
| `-` / `=` `+` | Clock brightness with the amplifier on (`brightOn`) down / up by 15, written straight to the display with no fade, and saved. | 4.7, 12.7 |
| `f` `F` | Cycle the readout (`showTuning`): clock always, tuning always, tuning while tuning. Saved. | 4.6.2 |
| `Z` | Zero the step-jitter and display slot-error counters. | 4.12.2, 5.16 |
| **Needle** | | |
| `H` `h` | Home the needle. | 5.9, 5.16 |
| `T` `t` | Track the tuner. Prints "NOT tracking" when the request is dropped. | 5.10, 5.16 |
| `P` `p` | Park. Prints "NOT parking" when the request is dropped. | 5.10, 5.16 |
| `x` `X` | Stop the needle; also aborts a running calibration. It also ends a recovery. | 5.15.1, 5.16 |
| `A` | Abort a running calibration. | 5.15.1, 5.16 |
| `r` | Sweep the full measured travel, repeating, until `x`. | 5.10, 5.16 |
| `k` `K` | Calibrate the index band, 5 passes; put the needle on the switch first. (The portal button uses 3.) | 5.12.1 |
| `m` / `M` | Set the low / high soft limit at the needle's position. | 5.12.2 |
| `n` / `N` | Jog −20 / +20 half-steps at 60 half-steps/s, microstepped. | 5.16 |
| `j` / `J` | Jog +200 / −200 half-steps at 200 half-steps/s, in half-step mode (a bring-up tool). | 5.16 |
| `u` / `U` | Jog +200 / −200 half-steps at 200 half-steps/s, microstepped. | 5.16 |
| `1` … `4` / `0` | Hold one driver input at full duty / release the coils. | 5.16 |
| `e` | Index edge log on or off: every edge, with its back-dated position and direction. | 5.8, 5.16 |
| `l` `L` | Live index monitor: the raw pin about every 250 ms, for up to 60 s. Enter is ignored; any other key stops it. It pings the audio board and feeds the watchdog on every pass. | 5.16, 8.9 |
| `g` `G` | Print that the end-stop calibration was removed and that `m` / `M` replace it. Kept because old instructions still name these keys. | 5.16 |
| **Tuning** | | |
| `c` / `C` | Store the tuner's low / high end: the current angle count. Refused if the angle sensor does not answer. | 6.9, 6.10 |
| `V` | Measure this dial position with the RDA5807M and store it as a sample if it passes every gate. | 6.7, 6.9 |
| `R` | The RDA5807M's state, its bus, the window, and where the local oscillator should be. | 6.9 |
| `q` | A sweep of the whole window (about a minute). Not a measurement: nothing is stored. | 6.9 |
| `v` | A narrow sweep around the prediction (about 10 s). Nothing is stored. | 6.9 |
| `a` | Print the last sweep, bin by bin (the coarse points, kept after a refinement), then the fine pass's points at 50 kHz, with the chosen one marked. | 6.9 |
| `o` | Mark the last sweep's strongest candidate as a fixed feature. Refused if already known, or if the list is full (16). Use it with the tube set switched off: `o` does not check. | 6.9, 6.10 |
| `O` | Forget every fixed feature. | 6.9 |
| **Settings** | | |
| `D` | Dump all settings as text, including the IF (`ifOffset=`) and the learned transmit power (`wifiTxQ`, which the settings file does not carry). Only partly a settings file. | 7.10, 12.4 |

`o`, `O` and `B` print "saved" when the change is only marked for saving (section 12.8). The keys `,` and
`.`, which once stepped the needle's top speed, were removed; they now fall under "ignored".

## D.2 Audio board

**How to reach it.** Only through the audio board's own USB cable, at 115 200 baud. The cabinet is shut in
normal use, so these are bench keys; the portal cannot reach them. The audio board's important lines also
reach the main board's console, prefixed `[A32]`.

| Key | What it does | Where |
|---|---|---|
| `+` / `-` | Volume up / down by 10. `+` also un-mutes. | 10.5, 7.4.2 |
| `m` | Mute. | 10.5.5, 7.4.2 |
| `b` | Pair, on the Bluetooth source only, like the pair button: drops the connected phone (pausing it first) and opens the pairing window. On any other source it does nothing but print `not in BT mode`. | 10.7.2, 10.7.4 |
| `v` | The front knob on or off. | Appendix G, G.2 |
| `p` / `c` / `P` | Store the knob's minimum / centre / maximum reading: the knob calibration. | 10.5.1 |
| `t` / `T` | The volume law (`taper`) down / up by 0.1. | 10.5.2 |
| `r` | A raw knob stream at 10 Hz. | Appendix G, G.2 |
| `d` | Cycle the constant DC word. The first press selects `kDc[1]` (`0x00FF0000`), because the start-up word is `kDc[0]`. | Appendix G, G.2; Appendix H |
| `k` | Cycle the low-bit output mask: 0, 1, 2, 4, 6, 8, 12, 16 bits. | Appendix G, G.2 |
| `g` | Cycle the data-line pad drive, 0 to 3. Drives 0 and 1 may mis-decode for long stretches, as with `sys.dindrv`: only with someone present and the amplifier down. | Appendix G, G.2 |
| `s` | Status: the knob reading and its calibration; the I2S fault latches, decoded by name; the link and the awake state; every pad and register, read back from the hardware; the peaks, the ring, the underruns, and `ESP.getFreeHeap()`. It reads and resets the peaks, the ring's high-water mark and the I2S latches, so prefer the portal for those. | 9.5, 10.9, 12.6 |

**Notes.**

- The help line the audio board prints at start-up does not list `k` or `g`; both work (section 12.5).
- The volume law (`t`, `T`) and the knob calibration (`p`, `c`, `P`) set here are saved on the audio
  board, but the main board's copy is not refreshed: the next portal edit of any audio row sends the
  older volume law back. **Re-read the A32** refreshes the copy (section 12.5).
- The key `y`, which cycled the data-line output delay, has been removed, because one of its positions
  sent the radio to full volume (chapter 10, section 10.15.3).
