# Appendix C. Portal actions

Look here when you want to know exactly what a portal button does, who may press it, and which chapter
explains it; or when you script an action that has no button.

Every portal button that *does* something, rather than change a setting, posts `/api/act` with an action
name `a` and an optional number `v` (Appendix B, section B.4). Setting widgets post `/api/set` instead.

- **Who:** "guest" actions are allowed to every signed-in account; all others need the administrator.
- **`v`:** absent means 0.
- **Answers:** 200 `{"ok":1,"m":…}` when the action ran; 200 `{"ok":1,"warn":…}` when it was refused,
  painted red on the page (an unreadable `v` gives `REFUSED - not a number: …`); 403 `{"e":"no"}` for an
  unknown action or one the account may not use.
- **Button:** the label on the page, and its tab. "Script only" means there is no button: post
  `/api/act` directly, signed in as the administrator.
- The actions run in `doAction()`, `src/s3/settings_table.h`, on the portal task.

Most actions belong to other chapters; the last column says which.

## C.1 Bluetooth

| Action | Who | `v` | What it does | Button | Ch. |
|---|---|---|---|---|---|
| `bt.play` | guest | — | Bluetooth play, sent to the audio board | Now: **Play** | 10 |
| `bt.pause` | guest | — | Pause | Now: **Pause** | 10 |
| `bt.next` | guest | — | Next track | Now: **▶▶** | 10 |
| `bt.prev` | guest | — | Previous track | Now: **◀◀** | 10 |
| `bt.disconnect` | guest | — | Drop the Bluetooth source | Now: **Disconnect**; Bluetooth: **Disconnect** | 10 |
| `bt.pair` | guest | — | Open the pairing window. The audio board acts only on the Bluetooth source, but the answer is "pairing window open" on any source (chapter 12). | Now: **Pair**; Bluetooth: **Pair a new device** | 10 |
| `bt.forget` | admin | — | Drop every pairing | Bluetooth: **Forget every pairing** | 10 |

## C.2 Needle

| Action | Who | `v` | What it does | Button (Needle tab) | Ch. |
|---|---|---|---|---|---|
| `needle.stop` | admin | — | Stop the needle; also aborts a running calibration and ends a recovery or a repeating sweep (a running jog is not interrupted) | **STOP** | 5 |
| `needle.home` | admin | — | Home; a refusal says why | **Home** | 5 |
| `needle.sweep` | admin | 0 once, non-zero repeat | Sweep the measured travel | **Sweep once** (0) / **repeatedly** (1) | 5 |
| `needle.reindex` | admin | — | Re-index now | **Re-index** | 5 |
| `needle.track` | admin | — | Follow the tuner | **Track the tuner** | 5 |
| `needle.jog` | admin | half-steps, clamped to ±200 (a clamp is reported) | Jog in micro-step mode | **◀ 100**, **◀ 10**, **10 ▶**, **100 ▶** (−100, −10, +10, +100) | 5 |
| `needle.limitLow`, `needle.limitHigh` | admin | — | Set a soft limit here | **Set low / high limit here** | 5 |
| `needle.nudge` | admin | half-steps, ±1 to 200 | Move both soft limits together | **Needle Limit ◀ 20** / **20 ▶** (−20 / +20) | 5 |
| `needle.calBand` | admin | passes (default 3) | Characterise the index band | **Calibrate the index** (3) | 5 |
| `needle.calAbort` | admin | — | Abort a calibration | **Abort calibration** | 5 |

## C.3 Tuning and the dial

| Action | Who | `v` | What it does | Button (Needle tab) | Ch. |
|---|---|---|---|---|---|
| `needle.tuneLow`, `needle.tuneHigh` | admin | — | Record the tuner's end here | **Tuner = low / high end** | 6 |
| `tune.markA`, `tune.markB`, `tune.markC` | admin | MHz, 87.0 to 108.5 | Hand mark at this dial position | **Mark station A / B / C here**; asks "Frequency of the station you are tuned to now, in MHz" (pre-filled 107.3) | 6 |
| `tune.nudge` | admin | MHz, ±0.1 to ±5.0 | Slide the tuning curve (the hand dial correction) | **Dial −0.1 / +0.1 MHz** | 6 |
| `tune.nudgeZero` | admin | — | Reset the hand dial correction to 0, the only thing that erases it. Answers "dial correction reset to 0", or "the dial correction is already 0". | **Reset the dial correction to 0** | 6 |
| `tune.clear` | admin | — | Clear every mark; the hand dial correction is kept (the answer says so when one is set) | **Clear all marks** | 6 |
| `tune.drop` | admin | slot 0 to 11 | Drop one sample; the hand dial correction is kept. Slots 0 to 2 are the hand marks A, B and C, and dropping one removes that mark (chapter 12). | **Drop one sample**; asks "Which sample slot to drop (0-11)?" | 6 |
| `rda.sample` | admin | — | Measure this dial position with the RDA5807M. Refused outside the listening policy: "the dial calibrates only while you listen to the radio - switch the amp on and select RADIO" | **Measure this dial position** | 6 |
| `rda.spurClear` | admin | — | Forget the fixed features | **Forget the fixed features** | 6 |

A comma typed in a prompt's answer is read as a decimal point.

## C.4 Display

| Action | Who | `v` | What it does | Button (Display tab) | Ch. |
|---|---|---|---|---|---|
| `disp.test` | admin | 0 normal, 1 show 8888, 2 blank | Display test | **Show 8888** (1) / **Blank** (2) / **Normal** (0) | 4 |

## C.5 Audio and the audio board

| Action | Who | `v` | What it does | Button | Ch. |
|---|---|---|---|---|---|
| `pot.min`, `pot.ctr`, `pot.max` | admin | — | Record the volume pot's position | Audio: **Pot: minimum / centre / maximum** | 10 |
| `sys.getcfg` | admin | — | Re-read the audio board's settings | Audio: **Re-read the A32** | 9 |
| `sys.rebootA32` | admin | — | Reboot the audio board | System: **Reboot the audio board** | 9 |
| `sys.pot` | admin | 0 ignore / 1 use | Ignore the volume pot (not saved) | System: **Ignore the volume pot** (0) / **Volume pot back on** (1) | 10 |
| `sys.bttx` | admin | 0 to 5 (−12 to +3 dBm) | Bluetooth transmit power, down only (not saved) | script only | 10 |

## C.6 Audio experiments

None of these is saved. Chapter 10 explains each.

| Action | Who | `v` | What it does | Button | Ch. |
|---|---|---|---|---|---|
| `sys.dctest` | admin | 0 or 1 | Constant-DC diagnostic on or off | System: **Constant DC test ON** (1) / **off** (0) | 10 |
| `sys.dcword` | admin | index 0 to 6 | Pick one of seven fixed DC words for the diagnostic | script only | 10 |
| `sys.zfloor` | admin | 0 or 1 | Zero-data floor experiment | System: **Zero-data floor ON** (1) / **off** (0) | 10 |
| `sys.tail` | admin | 0 or 1 | Sign-extended tail experiment | System: **Sign-extended tail ON** (1) / **off** (0) | 10 |
| `sys.dindrv` | admin | 0 to 3, clamped | Data-pin drive experiment | script only | 10 |
| `sys.clkpair` | admin | 22, 23, 32 or 33 | Clock-pin drive pair | script only | 10 |
| `sys.clkdrv` | admin | 2 to 3, clamped | Clock-pin drive | script only | 10 |

`sys.sddly` is retired: this portal action sent the radio to full volume. Do not restore it (chapter 10,
section 10.15.3).

## C.7 System and network

| Action | Who | `v` | What it does | Button (System tab) | Ch. |
|---|---|---|---|---|---|
| `sys.save` | admin | — | Save the settings now; a refusal says why | **Save settings now** | 7 |
| `sys.quiet` | admin | seconds; absent or 0 or less = 120; capped at 600 | Ask for radio silence: WiFi fully off, then back by itself. Carried out by the network loop 500 ms later. Answer: "wifi off - it will return by itself" | **WiFi off for 2 minutes** (120) | 8 |
| `net.forceAp` | admin | — | Ask for the rescue access point; carried out 500 ms later; holds 10 minutes. Answer: "raising the access point - join "Ambersong", then http://192.168.4.1/". The driver's confirmation is on the console only. | **Raise the rescue access point** | 8 |

Rebooting the main board is not an action: **Reboot the main board** posts `/api/reboot` (Appendix B).
