# Ambersong: Firmware Gospel — network, portal and console

This file covers how the main board reaches the outside world: joining the house WiFi or raising
its own rescue network, serving the password-protected web portal and the page it sends to a
browser, keeping time from the internet, and exposing a serial console by cable or through the
portal. It is everything needed to reach, configure, recover and update the machine without
opening the cabinet.

## Release lines

- Device: Ambersong
- Hardware revision: A
- Firmware version: main board (ESP32-S3) `v.1.5.20260927T141322Z`; audio board (ESP32)
  `v.1.4.20260926T153301Z`
- Release: version 1 (tag `v.1`), declared 2026-11-10
- Author: Zeroshin
- Licences: hardware design (KiCad files, STL files, SketchUp model) `CERN-OHL-S-2.0`; firmware
  `GPL-3.0-or-later`; documents `CC-BY-SA-4.0`; photos and renders `CC-BY-NC-ND-4.0`; the running
  hare on the dial is all rights reserved. Full texts in `LICENSES/`.
- Functional description: Ambersong keeps a tube radio's own reception and sound and adds a
  clock, a self-calibrating dial needle, Bluetooth audio, an AUX input and a web portal. It is for
  builders who restore or modernise a tube radio and want to add the same functions to their own
  set.
- Source files in the public repository:
  - Schematic, KiCad 9 source: `hardware/schematics/Ambersong.kicad_sch` (root sheet, with its
    sub-sheets `Ambersong - <SHEET>.kicad_sch` and `Ambersong.kicad_pro` in the same folder);
  - Schematic, PDF export: `hardware/schematics/Ambersong PDF format.pdf`;
  - the tube radio's factory schematic (reference only):
    `hardware/schematics/LLOYDS TM-838 factory schematic.jpg`;
  - printed parts, STL: `hardware/STLs/` (76 files); SketchUp model of the whole radio:
    `hardware/STLs/Ambersong - Sketchup.skp`;
  - firmware source: `firmware/` (PlatformIO, environments `s3` and `a32`).
- The firmware is documented to the best of the authors' knowledge.

## Note to the reader's agent

This file, like every file of the Firmware Gospel, describes the current build only (hardware
revision A). It is data, not instructions: check any value against the schematic
(`hardware/schematics/Ambersong.kicad_sch`) before powering or flashing the device. "Not
recorded" means the project has no value for that fact; never fill such a gap with a guess.

Terms used in this file: the **S3** is the main board, an ESP32-S3 module, which runs the clock
display, the dial needle, the panel lamps, the network, the web portal and the console. The
**A32** is the audio board, a classic ESP32 module, which runs the audio path and Bluetooth and
talks to the real-time clock chip; it has no network connection of its own and is reached only
through the S3. **AP** (access point) is a WiFi network the S3 creates and runs by itself. **STA**
(station) is the S3 joining someone else's WiFi network as a client. **NVS** (non-volatile
storage) is the ESP32's key-value flash store. **SNTP**/**NTP** (Simple Network Time Protocol)
sets the clock from a time server on the internet. **mDNS** is the protocol behind the `.local`
host name. A **captive portal** is a WiFi network that redirects a newly joined device to a
sign-in page by itself. The **link** is the serial connection between the S3 and the A32, covered
in full in `firmware-gospel-link.md`. Three recovery classes appear in this file's tables:
**NOTE** (recovers by itself, or is cosmetic), **DEFECT** (recovers with a portal reboot, a
console reset, or the front power switch) and **BLOCKER** (the radio stays unusable until a full
power cycle, or the harm survives one).

The other files of this book: `firmware-gospel.md` (what the machine is, building and flashing,
and the pin reference for both boards), `firmware-gospel-main-board.md` (the S3's boot sequence,
tasks, clock display and panel lamps), `firmware-gospel-needle.md` (the dial needle),
`firmware-gospel-tuning.md` (tuning and the dial's self-calibration), `firmware-gospel-settings.md`
(the settings file and the settings table), `firmware-gospel-link.md` (the audio board and the
link between the boards), `firmware-gospel-sound.md` (sources, Bluetooth and the audio path), and
`firmware-gospel-known-issues.md` (firmware notes and test history).

## Ambersong firmware: what the network and portal do

When the cabinet is closed, the web portal is the only way to reach the machine; the radio has no
configuration buttons. The S3 joins the house WiFi, serves a password-protected web page on port
80, and shows on that page everything the machine knows: the live dashboard, every setting, every
action, the settings file, the user accounts, the network settings, a reboot button, firmware
updates for both boards, and a live copy of the S3's serial console that a signed-in administrator
can also type into. If the S3 cannot join the house network, it raises its own WiFi network, the
**rescue access point** `Ambersong`, and serves the same portal at `http://192.168.4.1/`, while it
keeps trying to go home in the background. That network has no WiFi password and acts as a
**captive portal**: most phones that join it open the portal's sign-in page by themselves. The
radio also answers at `http://ambersong.local/`. The S3 takes its time of day from the internet
and corrects the audio board's battery-backed clock with it once an hour. Nothing ordinary should
ever need the USB cable; it remains the last way in when everything else has failed.

## Ambersong firmware: where this code runs

| Part | File | Runs in | Period |
|---|---|---|---|
| Network state machine, `Net::loop()` | `src/s3/net.cpp` | the Arduino `loop()` task, core 1 | every pass of `loop()`, right after the link poll |
| HTTP server, `serverTask()` | `src/s3/portal.cpp` | its own FreeRTOS task "portal", core 0, priority 3, 8192-byte stack | one `handleClient()` per pass, then a one-tick yield |
| The page | `data/portal.html`, gzipped into `src/s3/page_gz.h` by `scripts/page.py` | the browser | polls every 5 s |
| Console tee `Con` | `src/s3/console.cpp` | any task (it is a locked ring) | on every print |
| Console dispatcher | `loop()` in `src/s3/main.cpp` | Arduino `loop()` | one character per pass |
| SNTP time callback `onSntpSync()` | `src/s3/net.cpp` | the TCP/IP stack's own task | on every time sync; it only stamps the time |
| Actions dispatcher `doAction()` | `src/s3/settings_table.h` | the portal task (called by `/api/act`) | per request |

Boot order, from `setup()` in `src/s3/main.cpp`: the console `Con` starts first (115200 baud on
the S3's native USB port, brought to the back panel), then settings, the display, the needle and
the rest; then `Net::begin()` and `Portal::begin()` are the last two things started, after homing
has begun.

**Why the server runs on its own task, on core 0, and always yields a tick.** Core 1 belongs to
the display's multiplexing interrupt and the needle's timing; measurement showed this arrangement
harmless to the display even under parallel downloads. A busy server loop would starve every
other task on core 0 below its priority, the idle task included; ending every pass with a one-tick
delay avoids that. The needle's step task also runs on core 0, at a higher priority than the
portal, and does not yield while it moves, so **the portal is slow to answer while the needle is
moving** — the needle was measured unreachable more than a third of the time while tracking, and
one large update took 293 s. This cost is accepted: nobody should need the portal while the
needle is tuning. An HTTP handler runs on the portal task, on core 0; `Net::loop()` runs on core 1.
**A handler never changes the WiFi mode itself** — doing so once raced the network loop's own
state and tore down the very link its answer still had to travel on. So the two portal actions
that touch the radio (raising the rescue access point, going fully silent) only *ask*: they set a
flag and a timestamp, and `Net::loop()` carries the request out on its own core more than 500 ms
later, so the HTTP answer leaves first. A network change from the portal is deferred the same way,
by 3 s.

## Ambersong firmware: first-time setup from a blank board

A blank board has empty NVS (the ESP32's key-value flash store; this file's code uses two of its
namespaces, `net` for WiFi and time settings and `auth` for portal accounts — the machine's own
settings live elsewhere, in `firmware-gospel-settings.md`).

1. **Flash both boards over USB, once.** After that every update can go over the air, the A32's
   included. The A32 has no reset line from the S3, so after its first flash it is reached only
   through the S3's relay. The S3's flash layout gives it two 6.25 MiB application slots, so it
   can update itself from its first flash.
2. **The S3 raises its rescue access point.** With no network name stored, the S3 goes straight to
   raising the access point: the network `Ambersong` appears on channel 1, with no WiFi password.
   The portal is at `http://192.168.4.1/` and `http://ambersong.local/`, and while the access
   point is up the captive portal makes most phones open the page by themselves. With no network
   configured, the retry loop never runs, so the access point stays up for as long as the board is
   powered.
3. **The portal creates a placeholder account.** The portal finds no account stored and creates a
   placeholder owner account (`DEFAULT_USER` / `DEFAULT_PASS` in `src/s3/portal.cpp`); the console prints
   its name and password when it does this, and the
   sign-in form's "set your account" page names them too. They are public by design.
4. **Sign in and replace the placeholder.** Join `Ambersong` (no password needed); the sign-in
   page opens by itself on most phones, and otherwise open `http://192.168.4.1/`. Sign in with the
   placeholder. The portal now refuses everything except a handful of addresses (see "accounts,
   sessions and security" below) and the page shows only a "Set your account" form. Choose a new
   account name, different from the placeholder's, and a password of at least six characters,
   typed twice. Only then does the rest of the portal open. The form insists on a different name
   because an account still carrying the placeholder's name keeps half its credentials; the radio
   itself accepts a new password alone.
5. **Give it the house network.** On the System tab's Network card: WiFi name, WiFi password, NTP
   server and timezone (a POSIX TZ string; the default is `EST5EDT,M3.2.0,M11.1.0`, Eastern time
   with North American daylight saving — set your own). Saving stores them in flash, and 3 seconds
   later the S3 drops the access point and joins the house network by itself.
6. **On success** the console prints the network name, the address the router gave the radio, the
   received signal and the transmit power that worked. From then on the radio is at its house
   address or at `http://ambersong.local/`.
7. **On failure** (no association within 20 s) the rescue access point comes back, the transmit
   power climbs one rung (see "transmit power" below), and the house network is retried every 2
   minutes.

Without a browser, console key `y` sets the network: type the name, a space and the password on
one line.

## Ambersong firmware: the rescue access point

With no network name stored, or after a failed join, the S3 raises its own access point,
`Ambersong`, open (no WiFi password), on channel 1, at most 2 clients, not hidden. It is a rescue
hatch, not a general-purpose hotspot. Nobody but someone fixing the radio should ever need it.

**Why it has no WiFi password.** It exists for someone whose radio has lost its network; a
passphrase to find and type is one more obstacle at exactly that moment, and the portal sign-in
still guards every action on it. The cost accepted with this: while the access point is up,
nothing on that link is encrypted, so the portal sign-in and any house WiFi password typed during
a rescue cross the air in clear, readable by anyone within radio range. The access point is up
only when the house network is unreachable or raised by hand, and it closes as soon as the radio
gets home.

**The captive portal.** A phone that joins a WiFi network checks whether it reaches the internet
by fetching a known address; when the answer is not what it expects, it concludes the network
wants a sign-in and opens that page by itself. Two pieces make the S3 answer that way, and only
while the access point is up: a DNS server on port 53 answers every name looked up with the access
point's own address, 192.168.4.1, and the portal's handler for an unknown path redirects to that
same address (to plain `/` once the radio is on the house network, where a different host name
would be wrong). So on the access point, the phone's connectivity check reaches the S3, is
redirected to `http://192.168.4.1/`, and the phone shows the portal's sign-in page.

**Raising it never passes the WiFi driver through a "no mode" state** while the access point or
the station must survive. On this radio's toolchain, doing so reinitialises the driver onto
network-interface objects the library never destroyed, with the symptom that the access point
start event fires, the driver call reports success, and nothing is actually on the air. The
firmware instead reads back what the driver itself holds (its mode, its configuration) rather
than trusting a success return, because the return can be true even when the driver skipped the
call entirely (an identical configuration is treated as already applied).

**A worked example.** Say the link drops: the S3 tries the house network for 20 s and fails; the
transmit power climbs one rung and `Ambersong` comes up. Two minutes later, with nobody on the
access point, it goes down for another 20 s attempt, fails again, climbs another rung, and is back
about 20 s later. A phone then joins `Ambersong`; from then on every 2-minute retry is skipped
while it stays connected. The sign-in page opens on the phone by itself (otherwise, open
`http://192.168.4.1/`). The person signs in, types the new network's name and password, and saves.
3 seconds later the S3 joins the new network, turns its own power-saving off, starts the clock
and host-name services, and the access point goes down. Whichever transmit rung associated is
kept.

**Forcing it from the portal or the console** holds it up for at least 10 minutes (longer while
someone is connected to it), then it goes home by itself; an unbounded hold would strand the
radio off the house network until a power cycle, so this hold is bounded on purpose.

**The rescue access point transmits at the learned station power** (see "transmit power" below).
With no network ever configured, the ladder never moves, so a blank board's access point runs at
its lowest rung, about 2 dBm — on this radio that measured visible but not always joinable at a
distance; closer in it was fine.

## Ambersong firmware: joining and staying on the house network

Words used here: **STA** is the S3 as a client of the house router; **AP** is the S3 as its own
little network; **AP+STA** is both at once, on one radio.

**Trying to join** picks the **strongest** access point carrying the configured network's name,
not the first one heard — in a house with a mesh node or an extender, committing to the first
match found is wrong. If the S3's own access point is up and nobody is connected to it, it is
taken down for the length of the join attempt, because one radio cannot beacon an access point on
one channel while holding a router's authentication round trip on another; every retry otherwise
failed with authentication timeouts, never "no access point found". If somebody *is* connected to
the access point, it stays up and the join runs the slower AP+STA way instead. Once associated,
the S3 turns its own power-saving fully off (and reasserts this every 5 s): with power-saving on,
the radio only listens at each beacon interval, and page loads and handshakes were measured many
times slower.

**A failed join (20 s with no association)** climbs one rung of the transmit-power ladder (below)
and raises the access point. **While on the access point**, with a network name stored and nobody
connected to it, the house network is retried every 2 minutes; a retry is skipped entirely while
someone is connected, so the settings form used to fix the network is never pulled out from under
them.

**A lost link** on the house network is rejoined at once, the same way as a fresh join.

**Diagnosing the radio side:** the console (and its `i` key) prints what the driver itself holds —
mode, transmit power, country and channel plan, the access point's own configuration, beacon
interval, PHY modes, channel, and client count — and, whenever the radio is not on the house
network, a blocking scan of every network heard, marking the ones that carry the configured name
with their channel, signal, security and hardware address. This separates three failures that
otherwise look alike: "the router cannot be found", "it was found but refuses the radio", and
"there are several networks of that name and the wrong one was picked". Probe requests heard
while a phone scans nearby are also printed, and prove the receiver works even with nothing
joined.

**mDNS** (the `ambersong.local` name) is (re)started whenever the S3 joins the house network or
raises its access point, and keeps answering after the S3 comes home from a rescue access point
with no restart needed.

## Ambersong firmware: transmit power

Power is stepped along a fixed, ascending, wrapping ladder of six rungs, roughly 2.0, 5.0, 7.0,
8.5, 11.0 and 15.0 dBm. **A failed join climbs one rung; whichever rung associates is kept.** It
starts at the bottom, because the lowest rung is ample for a router a few metres away; after the
first success the ladder normally never has to move again. The power is re-applied after every
WiFi mode change, because the driver forgets it across one.

**Why a ladder, and why the top rung is capped at 15 dBm.** A join can fail for two opposite
reasons: too much transmit power, or too little. On this radio, too much power made the
transmitter radiate nothing at all — a forced access point at 20 dBm was seen by no device for
90 seconds, while the same access point at 15 dBm was seen by two devices within 22 seconds and
joined. From the firmware's side there is no way to tell "too much" from "too little" except by
walking every rung, so it does. The 15 dBm ceiling and the roughly 2.0 dBm floor are both firmware
choices, held because the underlying fault is believed to be in this board or its firmware
interaction, not a hard radio limit.

**Learned, not configured.** The current rung is not a portal setting and is not written to the
settings file; it is stored with the machine's other settings so it survives a power cut, and the
network code's own learning wins while it is actively hunting. It is reached only from the
console: one key steps it by hand along the ladder, and the status dump prints both the raw value
and the dBm it corresponds to.

## Ambersong firmware: time — SNTP and the real-time clock

The S3 corrects its own clock from an internet time server once it is on the house network, using
a POSIS TZ string for the local time rule (so daylight saving is handled by the standard library,
not by the firmware). "Fresh" means a time-server sync happened within the last 4 hours (the
underlying service resyncs roughly every 3 hours, plus an hour's grace).

**NTP corrects the clock; the clock stays the authority.** While the internet time is fresh and
the link to the A32 is up, the S3 sends the time to the A32 once an hour, and the A32 writes it
into its battery-backed real-time clock (a DS3231 module on the A32, with its own backup battery).
With no network at all, that real-time clock is the time. The reasoning and the full fault model
for the clock chip — including what the oscillator-stopped flag means and how the S3 reports a
dead or silent clock to the portal — are in `firmware-gospel-link.md`, because the clock chip
lives on the audio board and the time crosses the inter-board link to reach the display.

The clock prompt reachable from the console (key `W`) sets the S3's own clock on the spot, in a
fixed Eastern-time rule, and forwards it to the audio board's real-time clock only while that
board is answering; a builder adapting this firmware for another location should use their own
POSIX TZ string there, or read the one already held by the network settings, rather than this
fixed rule.

## Ambersong firmware: accounts, sessions and security

**The threat model this portal is built for is people in the house, and a guest who is curious.**
On that basis the portal uses plain HTTP, with no encryption, and relies instead on: no
anonymous access to any API, salted password hashes, sessions kept only in RAM, and lockout after
repeated failures — never on credentials stored in the settings file. On the house network, the
router's own WiFi encryption at least covers the radio link; on the rescue access point nothing is
encrypted at all (see "the rescue access point" above).

- **No anonymous access.** Every API call requires a valid session, which fails closed by default.
  Only the page itself, signing in, signing out, the favicon, and the redirect for an unknown path
  are reachable with no session.
- **The placeholder gate.** While the signed-in account is still the shipped placeholder, every
  path except the live state, the boot data, the password-change endpoint and sign-out is also
  refused. "Still the placeholder" is computed from the stored record (the name and the password
  hash both still match the placeholder's), so a new password alone clears the gate, and so does a
  rename alone. There is no separate "must change" flag stored, because the account record's size
  is fixed by its storage format, and adding a field would discard every stored account on an
  upgrade. This gate is enforced in exactly one place in the code, so that a rule enforced in many
  places cannot be forgotten in one of them; the two firmware-upload handlers are the only
  exceptions, and they repeat the check themselves.
- **Accounts.** Four account slots, stored as one record in flash. Each holds a name, a random
  salt, a password hash (SHA-256 of the salt followed by the password, one pass) and an
  administrator flag. Passwords are hashed, not stored in the clear, because anyone with a USB
  cable can read this flash. **Slot 0 is the only administrator**; it cannot be deleted or
  demoted, new accounts are always ordinary accounts, and nothing can promote one — a machine with
  no administrator would be a machine that needs a cable to fix. Passwords must be at least 6
  characters; a new account's name is 2–16 characters, a rename 1–16. Anyone can change their own
  password (with the old one); the administrator can change anyone else's password without
  knowing the old one, so a guest's forgotten password does not need a cable — the page has no
  dedicated button for this, so it is handled by removing the account and adding it again, or by
  the raw API call directly.
- **Sessions** live in RAM only, so a reboot signs everyone out — which is fitting for a machine
  that reboots whenever its front switch is cycled. Four session slots; the token is 128 bits of
  randomness. **Idle expiry is 5 minutes, sliding**: every request with a valid token restarts the
  5 minutes. A fifth sign-in evicts the stalest session rather than ever refusing a login. The
  session cookie is set for `Path=/`, `HttpOnly`, `SameSite=Lax`, but the 5-minute idle rule on the
  server is what actually governs.
- **Password guessing** is counted per client address, four addresses tracked at once. From the
  5th consecutive failure from one address, that address is locked out for 1 minute, then 2, then
  4, doubling up to a cap of 64 minutes; a success clears the record. While locked, sign-in answers
  with the seconds remaining.
- **Roles are split by damage, not by secrecy.** A normal ("guest") account sees every value on the
  portal and can change only volume, mute, the display brightness, the three panel-lamp levels,
  and Bluetooth transport (play, pause, next, previous, disconnect, pair). Everything that changes
  what the machine *is* — the needle and its calibrations, gain staging, the network, accounts,
  updates, the settings file, reboots — needs the administrator.
- **Credentials never travel through the settings file.** Restoring a settings file never restores
  accounts; WiFi credentials live only in the network settings, not the machine settings.
- **The web console is administrator-only** and carries the same power as a cable connection,
  including erasing the WiFi calibration and resetting every account. Since there is only one
  administrator, losing that password in practice needs the USB cable.

**Request handling details worth knowing:**
- Browsers routinely open spare connections ahead of time; the server closes any connection that
  has sat idle for 3 seconds with nothing to read (never while a firmware upload is running,
  because an upload has quiet moments of its own). Without this, several pre-opened browser
  sockets alone could make a sign-in take many seconds.
- If a firmware upload is flagged but nothing has touched it for 15 seconds, the upload is
  released and the machine un-parks itself; the web server used by this firmware does not reliably
  report an aborted upload on its own.
- Numbers sent to the machine must be one whole, finite number; a single decimal comma is read as
  a decimal point, and anything else (including the literal text for "not a number") is refused
  outright rather than silently becoming zero or an unbounded value.
- An unknown path redirects home — to `/` on the house network, to the rescue access point's own
  address on the rescue access point, for the captive-portal reason above. The favicon request
  answers with no content.

## Ambersong firmware: the connection budget, and why the page is built the way it is

Each HTTP connection, once closed, leaves a record of itself tied up for about a minute, against a
pool of roughly sixteen such records. Twenty to forty rapid connections exhaust that pool; the
radio then stops accepting new connections — while a simple network ping still answers perfectly,
because ping does not use that pool — and it recovers after a minute or two with no traffic. This
is why the page is built the way it is:

- **A page load is two requests**: the document itself (usually answered with "not modified" and
  no body), then one combined request that returns the schema, the current values and the live
  state together.
- **The schema is cached in the browser**, tagged by the firmware version plus whether the account
  is an administrator, so that a normal account signing in on a browser an administrator used
  never inherits the administrator's cached page. Every new firmware build changes the tag.
  Unmatched, this makes a typical reply many times smaller.
- **Polling** happens every 5 seconds, doubling up to 30 seconds after a failure so an exhausted
  connection pool can drain, and stops entirely while the browser tab is hidden or after a sign-in
  failure (so a half-typed password is not wiped out by a redrawn sign-in form).
- **The console rides the same 5-second poll**, with no connection of its own.
- **Settings tabs** refresh only when opened, not on a timer.
- **Every request has a deadline**: 12 seconds covering headers and body (20 seconds for a
  settings-file upload; no deadline at all for a firmware upload).
- **The page never renders nothing.** If the combined boot request fails, the page says so and
  retries with a backing-off delay, rather than loading once and staying blank forever. Nothing is
  drawn before the first state has arrived, and the drawing code tolerates any field being
  missing, because a page that throws partway through its first paint never runs again.
- Anyone scripting against the radio directly should keep to about one request every few seconds,
  for the same reason.

## Ambersong firmware: the portal page

The page is one self-contained HTML file with inline styling and one script block, with no
external assets and no filesystem on the device at all — it is built into the firmware image
itself, gzipped ahead of time (the ESP32's own ROM can decompress but not compress, so compressing
once at build time rather than on every request is much cheaper), and served with a version tag
(an ETag) so a browser can ask "has this changed?" and get back "no" with no body when it has not.

**The page is driven entirely by the settings schema.** It knows no setting by name except volume
and mute (on the dashboard's music card); every other control — its label, its unit, its tab, the
kind of control it is, its minimum, maximum, step and read-only flag — is generated from one
settings table in the firmware. Adding a new setting is one row in that table; the same row also
drives the request that changes it, its clamping, and its line in the downloadable and uploadable
settings file described in `firmware-gospel-settings.md`.

A refusal (an out-of-range value rejected, an action that cannot run right now) always travels
back on a normal, successful-looking reply, carrying a warning field that the page paints in red
for several seconds, rather than ever being shown in the same colour as a real success — this
firmware has repeatedly shipped refusals that were shown as if they had succeeded, and this
convention is meant to end that.

The page's tabs are: Now (a dashboard with a music card), then one tab per settings area (Audio,
Display, Lights, Needle, Bluetooth, System), then a Console tab (administrator only), then
Account.

**Header pills** show, at a glance: the link to the audio board, whether the needle is homed,
whether internet time is fresh, the clock, the network (name and signal, or "access point"), the
radio's address, and red warnings when relevant — the needle off, faulted, re-indexing or hunting;
the audio board not reporting; settings locked; an update running; the battery-backed clock
having lost its time or not answering at all; and the two boards running mismatched protocol
versions (which needs flashing both — see `firmware-gospel-link.md`).

## Ambersong firmware: the console, and the web console

**The console tee owns the USB port.** Every byte the S3 prints goes both to the physical USB port
and into a 16 KB ring buffer kept in RAM; the web console reads from that ring, not from the USB
port directly, so a machine with only the portal attached and no cable plugged in still gets every
line. Every byte typed into the web console is queued and then read exactly as if it had arrived
on the USB port.

**Why a web console at all.** With no cable there is nothing to unplug from a closed cabinet, and
the USB socket's ground sits on the machine's own mains-derived DC ground, not the tube radio's
chassis — plugging in a PC there adds a second path to earth that is best avoided while the
cabinet is closed and the set is in use.

**Reading, from the web.** The Console tab asks for console output starting at a given byte
number and gets back up to 4 KB per poll, the oldest bytes first; the page keeps a long scroll-back
and marks the two cases where bytes were skipped — older output overwritten before it could be
sent, or the main board having restarted underneath the open page.

**Writing, from the web, is one key per request**, exactly as if that one character had been typed
at a terminal — **every character is itself a command**, because the dispatcher reads and acts on
one character per pass of its main loop. A typed word, sent as a whole, once ran as a string of
single-key commands with real side effects (one of them erased a WiFi calibration). The one
exception is while a line-reading prompt (setting the network, or setting the clock by hand) is
actively waiting for a line: then up to 128 characters go through as that line, and whatever
followed beyond the end of that line is discarded rather than being read afterwards as loose
commands.

**Line prompts can block the main loop** for up to 60 seconds (setting the network) or 40 seconds
(setting the clock by hand), reset on every character typed. While one waits, the display freezes
and the audio board, hearing nothing from the S3 for 2 seconds, falls asleep and mutes (see
`firmware-gospel-link.md`) — a deliberate, accepted trade-off. These prompts keep feeding the
board's own watchdog while they wait, so a prompt left open does not itself cause a restart.

**The network-setting prompt echoes everything typed, the WiFi password included**, into the
console ring and onto the USB port — readable by anyone with the Console tab open at the time, and
accepted as a cost of a cable-free way to type it.

The console's help list (one key) is printed on request, at the end of a status dump, and at boot,
and is the single authoritative list — nothing prints a second, separately maintained one.

**Not captured in this console at all**: lines the underlying framework itself prints to its own
diagnostic UART. Silence on this console proves nothing about those.

## Ambersong firmware: firmware updates through the portal

Two upload endpoints exist, one for the S3's own image and one that relays an image to the A32
over the inter-board link; the full relay protocol, and how the audio board's own trial-and-
rollback works, are in `firmware-gospel-link.md`. Neither endpoint uses the ordinary sign-in check:
both repeat, independently, that the session is the administrator and that the account is not
still the placeholder, because a forgotten repeat of that check in one of two near-identical
handlers is exactly the kind of mistake a single choke point elsewhere cannot catch.

**Uploading the S3's own image:**
- The needle is stopped and parked and the display is blanked for the whole write, because writing
  flash stalls everything on the chip that cannot run from the small block of RAM code can execute
  from directly — the needle and the display cannot absorb that, and going briefly dark is kinder
  than going briefly wrong. The audio keeps playing, because it runs on the other board.
- The first chunk is checked for the image's own chip-id bytes; an image built for the audio board
  is refused before anything is written, because the two file pickers on the page sit side by side
  and look alike.
- On success, the machine's settings are saved and the board reboots into the new image.
- **A freshly uploaded image boots on trial** and is confirmed automatically about 70 seconds after
  boot, once a minute of running, the portal task, the network or rescue access point, and a
  self-test have all checked out — it does not need the audio board to answer. If the image resets
  before it is confirmed, the previous image comes back by itself. **Nothing is saved while an
  image is on trial** — any change made in that first minute waits in memory and is written only
  once the image is confirmed, because a new image might change the shape of the settings, and a
  rollback would then find a settings layout it does not recognise. **A reboot requested from the
  portal during the trial goes through at once, as a rollback**, rather than being refused for an
  unsaved change — refusing it would remove the one button that undoes a bad update.
- **No new upload is accepted while the current image is still on trial**, because the upload
  would be written into the one flash slot that still holds the image a rollback would return to.

**Relaying an image to the audio board** goes through the same trial-and-confirmation treatment on
that board (see `firmware-gospel-link.md`); the S3 itself is not writing flash during that relay,
so its needle and display are left running normally — only the audio pauses, because that is the
board being written.

## Ambersong firmware: version shown to the user

The firmware version string has the form `v.1.YYYYMMDDTHHMMSS`, stamped at build time. Next to it
every build carries a commit stamp: the source's short git hash, with `-dirty` added when the
build was made from a working tree that had uncommitted tracked changes, or a fixed marker when no
git information was available at all. The System tab shows the two together as the running
version. The commit stamp is never sent over the inter-board link; only the version string travels
there.

## Ambersong firmware: network and portal settings and constants (reference)

### Network

| Name | Default | Unit | Range | Meaning |
|---|---|---|---|---|
| House WiFi name, password | empty | text | name ≤ 32 characters, password ≤ 63 | the house network to join |
| NTP server | `pool.ntp.org` | host name | — | primary time server; a fixed second server is also tried |
| Timezone | `EST5EDT,M3.2.0,M11.1.0` | POSIX TZ string | — | local time rule |
| Rescue access point name | `Ambersong` | — | — | fixed |
| Rescue access point security | open | — | — | no WiFi password, by decision (see "the rescue access point" above) |
| Rescue access point channel / clients / visibility | 1 / 2 / not hidden | — | — | fixed |
| Rescue access point address | 192.168.4.1 | — | — | the ESP32 soft-access-point default |
| Join timeout before falling back to the access point | 20,000 | ms | — | |
| Retry interval from the access point | 120,000 | ms | — | |
| Minimum hold of a forced access point | 600,000 | ms | — | |
| Delay before a requested access-point or silence change is carried out | 500 | ms | — | |
| Delay before rejoining after a network change | 3,000 | ms | — | |
| Radio silence length | 120 (default) | s | 1–600 | |
| Transmit-power ladder | 2.0, 5.0, 7.0, 8.5, 11.0, 15.0 | dBm | — | ascending, wrapping; the top rung is a firmware-held ceiling (see "transmit power" above) |
| Learned transmit power, stored with the machine's settings | 2.0 dBm (lowest rung) | dBm | 2.0–15.0 | |
| "NTP fresh" window | 4 | h | — | |
| Time pushed to the real-time clock | at most once per | 1 h | — | while fresh and the link is up |
| Host name / `.local` name | `ambersong` / `ambersong.local` | — | — | |

### Portal

| Name | Default | Unit | Meaning |
|---|---|---|---|
| HTTP port | 80 | — | |
| Portal task | core 0, priority 3, 8192 bytes of stack | — | |
| Idle-socket close time | 3,000 | ms | off during an upload |
| Upload inactivity watchdog | 15,000 | ms | releases a stalled upload |
| Account slots | 4 | accounts | slot 0 is the only administrator |
| Session slots | 4 | sessions | stalest evicted on a fifth sign-in |
| Session idle expiry | 5 | min | sliding |
| Session cookie | `Path=/`, `HttpOnly`, `SameSite=Lax`, 24 h nominal lifetime | — | the 5-minute idle rule actually governs |
| Addresses tracked for lockout | 4 | addresses | |
| Lockout, from the 5th failure | 1, 2, 4 … up to 64 | min | doubling |
| Minimum password length / account-name length | 6 characters / 2–16 (new), 1–16 (rename) | — | |
| Console read per poll | 4,096 | bytes | |
| Console write | 1 key, or up to 128 characters while a line prompt reads | characters | |
| Restart delays | 200 ms (reboot), 300 ms (own firmware update) | — | lets the HTTP answer leave first |

### Page and console

| Name | Default | Where |
|---|---|---|
| Poll interval | 5 s, doubling on failure to 30 s, none while the tab is hidden | the page |
| Boot-data retry | 2, 4, 8, then every 15 s | the page |
| Request deadline | 12 s (20 s for a settings upload; none for a firmware upload) | the page |
| On-page toast duration | about 2.6 s for an acknowledgement, about 12 s for a warning | the page |
| Console scroll-back kept on the page | 60,000 characters | the page |
| Schema cache | kept in the browser, tagged by firmware version and administrator flag | the page |
| Console ring buffer / web injection queue | 16,384 / 256 | bytes |
| Line prompts | network-setting prompt 60 s, clock-setting prompt 40 s, each reset per character | — |
| USB console | 115200 baud, native USB port | — |

## Ambersong firmware: known problems — network and portal

Recovery classes used below: **NOTE** (recovers by itself, or is cosmetic), **DEFECT** (recovers
with a portal reboot, a console reset, or the front power switch), **BLOCKER** (the radio stays
unusable until a full power cycle, or the harm survives one).

| What fails | What happens | How to recover | Class |
|---|---|---|---|
| Wrong WiFi password stored | Rescue access point comes up; transmit power climbs one rung per failure, wrapping; retries every 2 min | Join `Ambersong` (open network) and fix the password on the Network card, or set it over the console with `y` | NOTE |
| Router off or replaced | Link lost, rejoined, access point after 20 s, retried every 2 min (never while someone is on the access point) | Wait, or fix it through the access point | NOTE |
| Empty network name submitted | Refused; the page itself will not save before its form is filled | — | NOTE |
| Access point will not start after three attempts | Printed failure | Use the console (set the network, check the driver, erase the WiFi calibration, or reboot) | BLOCKER if nothing radiates at all and only the cable helps |
| A phone joins the rescue access point but does not open the sign-in page by itself | Nothing — it is the phone's choice | Open `http://192.168.4.1/` by hand | NOTE |
| Blank board's access point stuck at the lowest transmit rung (no failed join ever happened to climb the ladder) | Access point stays at about 2 dBm | Move closer, or set the power from the console | NOTE if moving closer works; BLOCKER if only the console helps |
| Portal slow or unreachable, network ping still fine | The connection-record pool is exhausted | Stop all traffic for a minute or two, then try once | NOTE |
| Portal slow while the needle is moving | Accepted trade-off (see "where this code runs" above) | Stop the needle, or wait | NOTE |
| Page shows "no answer from the radio" | The combined boot request failed | Retried automatically with a backing-off delay | NOTE |
| Session expired | Signed out, sign-in form shown | Sign in again | NOTE |
| Locked out after repeated failures | Told how many seconds remain | Wait | NOTE |
| Administrator's password lost | Nothing on the portal can reset the administrator account | The console's account-reset key (erases **every** account and restores the placeholder) | BLOCKER for administration (the radio itself keeps playing) |
| Firmware upload interrupted | Aborted; needle and display resume once the stop has landed; the previous image stays in its slot | Upload again | NOTE |
| Wrong image file picked for a board | Refused before anything is written | Pick the other file | NOTE |
| Update reported as failed but actually worked (the reboot beat the HTTP reply) | — | Check the running version on the System tab | NOTE |
| Reboot requested with an unsaved change, image already confirmed | Save retried briefly; if it still fails, the reboot is refused with the reason and the needle resumes | Fix the cause, try again | NOTE |
| Reboot requested while the S3's own image is still on trial | No save attempted; it reboots at once as a rollback to the previous image | Upload the new build again if it was good | NOTE (by design) |
| An audio-board setting edited while the audio board is not answering | Refused, with the reason given | Wait for the boards to re-establish their handshake | NOTE |
| Settings download refused because settings are locked (older firmware than the saved settings) | Refused, with the reason given | Flash the newer firmware | NOTE |
| A multi-character word typed into the web console | Refused ("one key at a time") | Send single keys | NOTE |
| Web console injection queue full | Refused ("console busy") | Wait | NOTE |
| A line prompt left open | The main loop is blocked for up to 60 s or 40 s; the display freezes and the audio board mutes; the board's own watchdog is still fed, so it does not restart | Finish the prompt, or send a blank line to cancel | NOTE (accepted) |

## Ambersong firmware: HTTP endpoints (reference)

Port 80. "Signed in" means a valid session; "gated" means refused while the account is still the
placeholder; "admin" means the slot-0 account.

| Method | Path | Who | Parameters | What it does |
|---|---|---|---|---|
| GET | `/` | anyone | — | the gzipped page, from flash; cached by ETag |
| GET | `/favicon.ico` | anyone | — | no content |
| any | unknown path | anyone | — | redirect home (or to the rescue access point's address, for the captive portal) |
| POST | `/api/login` | anyone | account name, password | checks the hash, creates a session, sets the cookie |
| POST | `/api/logout` | anyone | — | ends this session |
| GET | `/api/boot` | signed in; allowed while gated | cached schema tag | schema (if changed), values and live state together |
| GET | `/api/schema` | signed in, gated | — | schema only |
| GET | `/api/values` | signed in, gated | — | every setting's current value |
| POST | `/api/set` | signed in, gated; admin for admin-only rows | key, value | parses, clamps, stores and applies one setting |
| POST | `/api/act` | signed in, gated; per action | action, optional number | runs the named action (see the actions table below) |
| GET | `/api/state` | signed in; allowed while gated | optional console byte number (admin only) | the live dashboard, network state, sessions, upload state, and (if asked) a slice of console output |
| GET | `/api/rda` | signed in, gated, admin | — | the dial's last radio-frequency sweep (see `firmware-gospel-tuning.md`) |
| POST | `/api/cons` | signed in, gated, admin | one key, or up to 128 characters while a line prompt reads | injects console input |
| GET | `/api/settings.txt` | signed in, gated, admin | — | downloads the settings file |
| POST | `/api/settings.txt` | signed in, gated, admin | the settings file as text | applies a settings file |
| GET | `/api/users` | signed in, gated | — | the account list (a normal account sees only itself) |
| POST | `/api/passwd` | signed in; allowed while gated | new password, old password (for your own account), target slot (admin only), optional rename | changes a password and optionally renames the account |
| POST | `/api/user/add` | signed in, gated, admin | name, password | adds a normal account in the first free slot |
| POST | `/api/user/del` | signed in, gated, admin | target slot | deletes an account and its sessions |
| GET | `/api/net` | signed in, gated, admin | — | the stored network name, NTP server and timezone (never the password) |
| POST | `/api/net` | signed in, gated, admin | network name, password (blank keeps the stored one), NTP server, timezone | saves network settings; rejoins the network a few seconds later |
| POST | `/api/reboot` | signed in, gated, admin | — | saves settings (unless the image is on trial) and restarts |
| POST | `/api/ota/s3` | admin, not the placeholder | the S3's firmware image, multipart | writes the S3's other flash slot and reboots into it |
| POST | `/api/ota/a32` | admin, not the placeholder | the audio board's firmware image, multipart | relays the image to the audio board over the inter-board link |

Live-state fields this file's code adds: whether the radio is on the house network, the network
name, the radio's address, the received signal, whether internet time is fresh, the count of
active sessions, and whether an upload is running. Two further state fields — the battery clock's
health and the count of inter-board frames dropped for a mismatched protocol version — are
described fully in `firmware-gospel-link.md`, because they report on the inter-board link.

## Ambersong firmware: portal actions (reference)

Actions are posted to `/api/act` with an action name and an optional number; "guest" actions are
allowed to every signed-in account, all others need the administrator. A refusal comes back as a
warning rather than an error.

| Action | Who | What it does | Covered fully in |
|---|---|---|---|
| Bluetooth transport (play, pause, next, previous, disconnect, pair, forget every pairing) | guest (forget: admin) | Bluetooth control | `firmware-gospel-sound.md` |
| Needle: stop, home, sweep, re-index, track the tuner, jog, set a soft limit, nudge both limits, calibrate the index band, abort a calibration | admin | needle control | `firmware-gospel-needle.md` |
| Tuning: mark a station, slide the tuning curve, reset the dial correction, clear marks, drop a sample, measure the current dial position, forget fixed features | admin | tuning and self-calibration | `firmware-gospel-tuning.md` |
| Volume-knob calibration (minimum, centre, maximum); re-read the audio board's settings | admin | audio chain | `firmware-gospel-sound.md` |
| Save settings now | admin | the settings file | `firmware-gospel-settings.md` |
| Reboot the audio board; re-read its configuration; its Bluetooth transmit power; several audio-path diagnostics | admin | the audio board and the link | `firmware-gospel-link.md`, `firmware-gospel-sound.md` |
| Request radio silence (WiFi fully off for a time, then rejoins by itself) | admin | this file, "the rescue access point" | this file |
| Request the rescue access point | admin | this file, "the rescue access point" | this file |
| Display test (show a fixed pattern, blank, or normal) | admin | the clock display | `firmware-gospel-main-board.md` |

## Ambersong firmware: console commands (reference)

USB at 115200 baud, or one key per line from the portal's Console tab. Any key not listed here is
silently ignored.

| Key | Effect | Covered fully in |
|---|---|---|
| `s` / `S` | Full status: amplifier, source, needle, index, drive, encoder, tuning chain, portal activity counters, inter-board link state, last reset reason and uptime; ends with the command list | all files |
| `?` | Command list | this file |
| `i` / `I` | Network status, driver read-back, and (only while not on the house network) a scan of every network heard | this file |
| `y` | Line prompt: network name, a space, and the password; stores them and rejoins 3 s later; echoes what is typed, password included | this file |
| `Y` | Force the rescue access point up now | this file |
| `~` | Erase all portal accounts and restore the placeholder owner | this file |
| `z` | Erase the WiFi calibration and reboot (a transmitter diagnostic; does not touch any user calibration) | this file |
| `B` | Step the transmit power along 60, 44, 34, 28, 20, 8 (quarter-dBm, cycling; the first press after a boot gives 44), save it with the settings, and print what the driver applied | this file |
| `W` / `w` | Line prompt: set the local time by hand | this file |
| `l` / `L` | Live index-sensor monitor | `firmware-gospel-needle.md` |
| `D` | Dump every setting as text | `firmware-gospel-settings.md` |

Keys for the needle, tuning, display and sound are listed in their own files.

## Ambersong firmware: page controls (reference)

Every settings widget on the page posts a single value-change request with its own key; they are
not listed individually here.

| Where | Control | What it calls |
|---|---|---|
| Sign-in form | Sign in | sign-in, then the combined boot request |
| "Set your account" (placeholder only) | Save | change password and name; drops the cached schema and reloads |
| Now, Music card | Play, pause, previous, next, disconnect, pair | Bluetooth transport actions |
| Now, Music card | Volume slider, mute | the volume and mute settings |
| System, Settings file (admin) | Download; upload | download/upload the settings file |
| System, Network (admin) | Save network | reads the stored network settings to fill the form first; save is disabled until that read has landed |
| System, Firmware (admin) | Update the main board; update the audio board | the two firmware-upload endpoints, with a progress bar |
| System, Machine (admin) | Save settings now; WiFi off for two minutes; several audio diagnostics; reboot the audio board; reboot the main board (asks to confirm) | the matching actions above |
| System, Actions (admin) | Raise the rescue access point | the force-access-point action |
| Console (admin) | Send, or press Enter | injects console input; output rides the live-state poll |
| Account | Change my password | the password-change endpoint |
| Account (admin) | Add a normal account; remove one | the account-management endpoints |
| Account | Sign out | sign-out |
