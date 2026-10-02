# Appendix E. Portal map: tabs, cards, buttons and the calls they make

This is the portal page, tab by tab, with the HTTP call behind every control. Use it to find where a
control lives, or which call to make when you script the radio. Chapter 8, sections 8.7 and 8.8, explain
how the page works; Appendix B lists every endpoint and Appendix C every action.

**Settings widgets are not listed one by one.** The page draws them from the schema the radio sends:
each setting row becomes a checkbox, a drop-down, or a slider with a number box, on the tab its row
names. Every one of them posts `POST /api/set` with its key (`k`) and value (`v`); the value in the reply,
possibly clamped, is written back into every widget with that key. The settings themselves are in
Appendix A.

**Who sees what.** A normal account (guest) sees every tab except **Console**, and every value, but its
changes are refused except for the guest controls (chapter 8, section 8.6.1). In the tables, "(admin)"
marks what needs the administrator.

## E.1 Before the tabs

| Where | Control | Call |
|---|---|---|
| Sign-in form | **Sign in** | `POST /api/login` (`u`, `p`), then `GET /api/boot` |
| **Set your account** (shown only while the account is the placeholder) | **Save** | `POST /api/passwd` (`old` = the placeholder password, `new`, `name`); then drops the cached schema and reloads |
| The page itself | (loading) | `GET /` (usually 304), then `GET /api/boot?sc=<tag>`; then `GET /api/state` every 5 s (chapter 8, section 8.7.2) |

## E.2 The tabs, in order

| Tab | What it holds |
|---|---|
| **Now** | The live dashboard and the **Music** card. |
| **Audio** | Its settings from the schema; an **Actions** card (admin). |
| **Display** | Its settings from the schema; an **Actions** card (admin). |
| **Lights** | Its settings from the schema. |
| **Needle** | Its settings from the schema; an **Actions** card (admin). |
| **Bluetooth** | Its settings from the schema; an **Actions** card. |
| **System** | Its settings from the schema; the **Settings file**, **Network**, **Firmware**, **Machine** and **Actions** cards (admin). |
| **Console** (admin) | The S3's console: its output, and one key per send. |
| **Account** | Change your password; add and remove normal users (admin); sign out. |

The settings tabs fetch `GET /api/values` when you open them, not on a timer.

## E.3 Controls and their calls

| Tab, card | Control | Call |
|---|---|---|
| Now, Music | **Play**, **Pause**, **◀◀**, **▶▶**, **Disconnect**, **Pair** | `/api/act` `bt.play`, `bt.pause`, `bt.prev`, `bt.next`, `bt.disconnect`, `bt.pair` |
| Now, Music | Volume slider, **Mute** | `/api/set` `volume`, `muted`. Both follow the physical knob through the live state, except while the slider has focus. |
| Audio, Actions (admin) | **Pot: minimum / centre / maximum**; **Re-read the A32** | `/api/act` `pot.min`, `pot.ctr`, `pot.max`; `sys.getcfg` |
| Needle, Actions (admin) | **Home**; **Re-index**; **Sweep once / repeatedly**; **Track the tuner**; **STOP** | `/api/act` `needle.home`; `needle.reindex`; `needle.sweep` 0 / 1; `needle.track`; `needle.stop` |
| Needle, Actions (admin) | **◀ 100**, **◀ 10**, **10 ▶**, **100 ▶** | `/api/act` `needle.jog` −100, −10, +10, +100 |
| Needle, Actions (admin) | **Set low / high limit here**; **Needle Limit ◀ 20 / 20 ▶** | `/api/act` `needle.limitLow` / `needle.limitHigh`; `needle.nudge` −20 / +20 |
| Needle, Actions (admin) | **Calibrate the index**; **Abort calibration** | `/api/act` `needle.calBand` 3; `needle.calAbort` |
| Needle, Actions (admin) | **Tuner = low / high end** | `/api/act` `needle.tuneLow` / `needle.tuneHigh` |
| Needle, Actions (admin) | **Mark station A / B / C here** (asks for MHz); **Clear all marks** | `/api/act` `tune.markA` / `tune.markB` / `tune.markC`; `tune.clear` |
| Needle, Actions (admin) | **Dial −0.1 / +0.1 MHz**; **Reset the dial correction to 0** | `/api/act` `tune.nudge` −0.1 / +0.1; `tune.nudgeZero` |
| Needle, Actions (admin) | **Measure this dial position**; **Drop one sample** (asks for a slot); **Forget the fixed features** | `/api/act` `rda.sample`; `tune.drop`; `rda.spurClear` |
| Display, Actions (admin) | **Show 8888** / **Blank** / **Normal** | `/api/act` `disp.test` 1 / 2 / 0 |
| Bluetooth, Actions | **Pair a new device**; **Disconnect**; **Forget every pairing** (admin) | `/api/act` `bt.pair`; `bt.disconnect`; `bt.forget` |
| System, Settings file (admin) | **Download** | `GET /api/settings.txt`. While settings are locked, the page refuses with a toast. |
| System, Settings file (admin) | **Upload what is below** (the text box) | `POST /api/settings.txt` (raw text; 20 s deadline) |
| System, Network (admin) | (the form fills itself) | `GET /api/net` |
| System, Network (admin) | **Save network** (disabled until the form is filled) | `POST /api/net` (`ssid`, `pass`, `ntp`, `tz`) |
| System, Firmware (admin) | **Update the main board**; **Update the audio board** | `POST /api/ota/s3` / `POST /api/ota/a32`, form field `f`, sent with `XMLHttpRequest` and a progress bar |
| System, Machine (admin) | **Save settings now** | `/api/act` `sys.save` |
| System, Machine (admin) | **WiFi off for 2 minutes** | `/api/act` `sys.quiet` 120 |
| System, Machine (admin) | **Constant DC test ON / off**; **Zero-data floor ON / off**; **Sign-extended tail ON / off** | `/api/act` `sys.dctest` 1 / 0; `sys.zfloor` 1 / 0; `sys.tail` 1 / 0 |
| System, Machine (admin) | **Ignore the volume pot** / **Volume pot back on** | `/api/act` `sys.pot` 0 / 1 |
| System, Machine (admin) | **Reboot the audio board** | `/api/act` `sys.rebootA32` |
| System, Machine (admin) | **Reboot the main board** (asks to confirm) | `POST /api/reboot` |
| System, Actions (admin) | **Raise the rescue access point** | `/api/act` `net.forceAp` |
| Console (admin) | **Send**, or Enter | `POST /api/cons` (`k`); the output arrives through `GET /api/state?log=` |
| Account | **Change it** (my password) | `POST /api/passwd` (`old`, `new`) |
| Account (admin) | **Add a normal user**; **Remove** | `POST /api/user/add` (`n`, `p`); `POST /api/user/del` (`i`) |
| Account | **Sign out** | `POST /api/logout` |

## E.4 What has no control on the page

| Call | Reached by |
|---|---|
| `POST /api/passwd` with a target slot `i` (the administrator sets another account's password without the old one) | A direct request, signed in as the administrator. From the page, remove the account and add it again. |
| `/api/act` `sys.bttx`, `sys.dcword`, `sys.dindrv`, `sys.clkpair`, `sys.clkdrv` | A direct request, signed in as the administrator (Appendix C). |
| `/api/act` `needle.jog`, `needle.nudge`, `tune.nudge` with other values than the buttons send | A direct request, within the action's bounds (Appendix C). |
