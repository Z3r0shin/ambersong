# Appendix B. Network and portal constants, HTTP endpoints, state fields

This appendix is for anyone who changes the network or portal code, or who writes a script that talks to
the radio. It lists every network and portal constant, every HTTP endpoint and the network fields of the
live state. You do not need it to use or repair the radio: chapter 8 explains each value in context.

Everything here runs on the main board (the S3). "Fixed" means the value is set in the source and changes
only with a new build.

## B.1 Network constants

In `src/s3/net.cpp` unless stated.

| Name | Value | Unit | Range | Meaning | Changed by |
|---|---|---|---|---|---|
| `ssid`, `pass` (NVS area `net`) | empty | text | name up to 32, password up to 63 | The house network | Network card; console `y` |
| `ntp` (NVS `net`) | `pool.ntp.org` | host name | — | Primary time server; `time.nist.gov` is the fixed second | Network card |
| `tz` (NVS `net`) | `EST5EDT,M3.2.0,M11.1.0` | POSIX TZ rule | — | Local time rule | Network card |
| `AP_SSID` | `Ambersong` | — | — | Rescue access point's name | fixed |
| Access-point security | open (`nullptr` passphrase in `softAP()`) | — | — | No WiFi password on the rescue access point (chapter 8, section 8.6.7 for the cost) | fixed |
| Access-point channel / devices / hidden | 1 / 2 / no | — | — | "a rescue hatch, not a hotspot" | fixed |
| Captive-portal DNS server (`gDns`) | port 53, every name (`"*"`) to the access point's address | — | — | Runs only while the access point is up | fixed |
| Access-point address | 192.168.4.1 | — | — | The ESP32's soft-AP default | — |
| `softAP()` attempts | 3, 400 ms apart; settle 50 ms + 100 ms | — | — | `startAp()` | fixed |
| `JOIN_MS` | 20 000 | ms | — | Join timeout before the access point and the next power rung | fixed |
| `RETRY_MS` | 120 000 | ms | — | Retry the house network from the access point | fixed |
| `FORCE_HOLD_MS` | 600 000 | ms | — | Minimum life of a forced access point | fixed |
| Request grace | 500 | ms | — | Delay before a portal force-AP or radio-silence request is carried out | fixed |
| `setWifi` grace | 3000 | ms | — | Delay before rejoining after a network change | fixed |
| Radio silence | 120 (portal default) | s | 1–600 | Length of radio silence | `sys.quiet` |
| `TX_LADDER` | 8, 20, 28, 34, 44, 60 | quarter-dBm | 2.0–15.0 dBm | Transmit-power rungs; the 15 dBm top is a firmware choice | fixed |
| `cfg.wifiTxQ` (the settings, not the settings file) | 8 | quarter-dBm | 8–60 | Learned transmit power | the ladder; console `B` |
| Scan / choice | all channels / by signal | — | — | Pick the strongest access point with the name | fixed |
| Power save | `WIFI_PS_NONE` | — | — | Set after connecting, and every 5 s | fixed |
| Host name / mDNS | `ambersong` / `ambersong.local`, `_http._tcp` on 80 | — | — | Name on the network | fixed |
| `NTP_FRESH_MS` | 4 | hours | — | Network time counts as fresh this long after a sync | fixed |
| Network time to the battery clock | 1 | hour | — | At most this often, while fresh (`src/s3/main.cpp`) | fixed |

## B.2 Portal constants

In `src/s3/portal.cpp` unless stated.

| Name | Value | Unit | Meaning |
|---|---|---|---|
| HTTP port | 80 | — | |
| Portal task | core 0, priority 3, 8192 | bytes of stack | |
| `IDLE_SOCKET_MS` | 3000 | ms | Silent-socket reaper; off during uploads |
| Upload watchdog | 15 000 | ms | No upload activity for this long ends the upload |
| `MAX_USERS` | 4 | accounts | Slot 0 is the administrator |
| `MAX_SESS` | 4 | sessions | The stalest is evicted on a fifth sign-in |
| `IDLE_MS` | 5 | minutes | Sliding session idle expiry |
| Cookie | `amb`, `HttpOnly`, `SameSite=Lax`, `Max-Age` 86400 s, `Path=/` | — | The server's 5-minute rule governs |
| Session token | 32 hexadecimal characters (128 bits from `esp_random()`) | — | |
| Account record | `User{ name[17], salt[8], hash[32], admin, used }`, one blob, key `users` in NVS area `auth` | — | Password stored as SHA-256 of (8-byte salt + password), one pass |
| `MAX_BAD` | 4 | addresses | Device addresses tracked for lockout |
| Lockout | from the 5th failure: 1, 2, 4 … 64 | minutes | Doubling |
| Password / name | at least 6 / 2–16 (new user), 1–16 (rename) | characters | |
| Log slice | 4096 | bytes per poll | Web console read |
| Console send | 1 key, or up to 128 characters while a prompt reads a line | characters | Web console write |
| Reboot save retries | 15 × 100 | ms | Before refusing a reboot (none while the image is on trial) |
| Restart delays | 200 (reboot), 300 (S3 update) | ms | Let the answer leave first |
| Placeholder account | `DEFAULT_USER` / `DEFAULT_PASS` | — | Created on a blank board and by console `~`. This book does not print the values: the console prints them when it creates the account, and the sign-in page names them (chapter 8, section 8.2.2). |

## B.3 Page and console constants

| Name | Value | Where |
|---|---|---|
| Poll interval | 5 s, doubling on failure to 30 s; none while the tab is hidden | `data/portal.html` (`POLL_MIN`, `POLL_MAX`) |
| Start-up retry | 2, 4, 8, then every 15 s | `data/portal.html` |
| Request deadline | 12 s (settings upload 20 s, firmware upload none) | `data/portal.html` |
| Toast | 2.6 s acknowledgement (amber) / 12 s refusal or warning (red) | `data/portal.html` |
| Console buffer on the page | 60 000 characters; early poll 900 ms after a send | `data/portal.html` |
| Schema cache | `localStorage['amb.sc']`, tag `FW_VERSION-<0/1>` | the page and `schemaTag()` |
| `LOG_CAP` / `IN_CAP` | 16 384 / 256 bytes | `src/s3/console.cpp` |
| Prompts | `y` 60 s, `W` 40 s (restarted at each character); `drainLine` 50 ms | `src/s3/main.cpp` |
| `y` buffer | 98 bytes: a 32-byte name, a space, a 63-byte password, the terminator | `src/s3/main.cpp` |
| `l` monitor | 60 s at most, a line about every 250 ms | `src/s3/main.cpp` |
| USB console | 115 200 baud, native USB CDC (`ARDUINO_USB_CDC_ON_BOOT=1`) | `platformio.ini`, `setup()` |

## B.4 HTTP endpoints

All on the S3, port 80, registered in `Portal::begin()`.

- **Signed in:** a valid session cookie `amb`.
- **Gated:** refused with 403 `{"e":"mustchg"}` while the account is still the placeholder.
- **Admin:** the slot-0 account.
- Request bodies are form-encoded unless stated. `{"ok":1,"m":"…"}` is the generic acknowledgement.
- An API request that needs a session and has none answers 401 `{"e":"login"}`. The two upload
  endpoints answer 403 instead.

Stay at about one request every few seconds (chapter 8, section 8.7.2).

| Method | Path | Who | Parameters | What it does | Answers |
|---|---|---|---|---|---|
| GET | `/` | anyone | header `If-None-Match` | The gzipped page from flash | 200 (gzip, ETag, `no-cache, must-revalidate`); 304 if the ETag matches |
| GET | `/favicon.ico` | anyone | — | Nothing | 204 |
| any | unknown path | anyone | — | Redirect; on the rescue access point this is the captive portal's redirect | 302 `Location: /` on the house network; 302 `Location: http://192.168.4.1/` otherwise |
| POST | `/api/login` | anyone | `u`, `p` | Check the salted hash, create a session, set the cookie | 200 "welcome" + `Set-Cookie`; 403 `{"e":"bad"}`; 429 `{"e":"locked","s":N}` |
| POST | `/api/logout` | anyone (ends the session if the cookie is valid) | — | End this session, clear the cookie | 200 "bye" |
| GET | `/api/boot` | signed in; allowed while gated | `sc` = cached schema tag | Schema (or `null` if the tag matches), values and state in one reply `{"sc":…,"val":…,"st":…}` | 200; 401 |
| GET | `/api/schema` | signed in, gated | — | Schema only: tag, user, admin, fw, tabs, setting rows | 200; 401; 403 |
| GET | `/api/values` | signed in, gated | — | `{key: value}` for every setting row | 200; 401; 403 |
| POST | `/api/set` | signed in, gated; admin for admin rows | `k` key, `v` value | Parse the whole number (decimal comma accepted), clamp, store, apply | 200 `{"ok":1,"v":stored[,"clamped":1][,"warn":…]}` (`warn` only for `calLow`/`calHigh` when the tuning curve is in trouble); 404 `{"e":"nokey"}`; 403 `{"e":"admin"}`; 409 `{"e":"the audio board is not answering - its settings cannot be changed now"}` for an audio-board row while the A32's settings are not known (chapter 7, section 7.4.5); 400 `{"e":"not a number"}` (also when `v` is missing) |
| POST | `/api/act` | signed in, gated; per action (Appendix C) | `a` action, `v` optional number | Run the action | 200 `{"ok":1,"m":…}`; 200 `{"ok":1,"warn":…}` for a refusal, including an unreadable `v` ("REFUSED - not a number: …"); 403 `{"e":"no"}` for an unknown or forbidden action |
| GET | `/api/state` | signed in; allowed while gated | `log` = next console byte (admin only; ignored otherwise) | Live dashboard plus `net{sta,ssid,ip,rssi,ntp}`, `sess`, `ota`, `mustchg`; with `log`, a `log{from,next,t}` slice of up to 4096 bytes | 200; 401 |
| GET | `/api/rda` | signed in, gated, admin | — | The last RDA sweep, bin by bin with the judgement made about each bin (the coarse bins are kept after a refinement), the fine pass's 11 points (`fine`), and the fixed-feature list (chapter 6). It exists so that the judgement can be checked: without it, a wrong judgement would discard the signal silently. | 200; 403 text "admin only" |
| POST | `/api/cons` | signed in, gated, admin | `k` = one key, or up to 128 characters while a prompt reads a line | Inject into the console as if typed, newline appended | 200 "sent"; 403 text "admin only"; 400 `{"e":"1 to 128 characters"}`; 400 `{"e":"one key at a time - each character is its own command"}`; 503 `{"e":"console busy"}` |
| GET | `/api/settings.txt` | signed in, gated, admin | — | Download the settings file (`ambersong.txt`, `key=value`); audio-board rows read `n/a` while the A32's settings are not known (chapter 7) | 200 `text/plain` attachment; 403 text; 409 text "Settings are locked: …" |
| POST | `/api/settings.txt` | signed in, gated, admin | raw `text/plain` body | Apply a settings file | 200 `{"ok":1,"m":"N settings applied"[,"warn":…]}`; 403 `{"e":"admin"}` |
| GET | `/api/users` | signed in, gated | — | `{"me":…,"u":[{i,n,a}],"max":4}`; a normal user sees only themself | 200 |
| POST | `/api/passwd` | signed in; allowed while gated | `new`; `old` (required for your own account); `i` target slot (admin only); `name` optional rename | Change the password (new salt) and optionally the name | 200 "password changed" / "credentials changed"; 404 `{"e":"nouser"}`; 403 `{"e":"admin"}`; 403 `{"e":"oldpw"}`; 400 `{"e":"short"}`; 400 `{"e":"longname"}`; 409 `{"e":"taken"}` |
| POST | `/api/user/add` | signed in, gated, admin | `n` (2–16), `p` (6 or more) | Add a normal user in the first free slot, 1 to 3 | 200 "user added"; 403; 400 `{"e":"name"}` / `{"e":"short"}`; 409 `{"e":"exists"}`; 507 `{"e":"full"}` |
| POST | `/api/user/del` | signed in, gated, admin | `i` (1–3) | Delete the user and drop their sessions at once | 200 "user removed"; 403; 400 `{"e":"no"}` |
| GET | `/api/net` | signed in, gated, admin | — | `{"ssid":…,"ntp":…,"tz":…}`, never the password | 200; 403 `{"e":"admin"}` |
| POST | `/api/net` | signed in, gated, admin | `ssid`, `pass` (blank = keep), `ntp`, `tz` | If `ssid` is present: refuse it empty, store, rejoin in 3 s. If `ntp` is present: store the server and the time zone, apply the zone, restart network time if on the house network | 200 "network saved - the radio joins it in a few seconds"; 400 `{"e":"the network name is empty"}`; 403 |
| POST | `/api/reboot` | signed in, gated, admin | — | If the S3 image is on trial: restart at once, no save (the previous image comes back). Otherwise save the settings; if refused (and not locked), stop the needle and retry 15 × 100 ms; restart | On trial: 200 "rebooting - this firmware was still on trial, so the previous firmware comes back". Otherwise 200 "rebooting"; 409 `{"e":"not rebooting - <why>"}` (needle resumed). Restart 200 ms after a 200; 403 |
| POST | `/api/ota/s3` | admin session, not the placeholder (checked in both upload callbacks) | multipart file (the page uses field `f`) | Write the S3's other program slot, reboot (chapter 3, section 3.10.2) | 200 "rebooting into the new firmware", then restart; 500 `{"e":"flash"}`; 403 text "admin only" / "change the default password first"; 409 `{"e":"this firmware is still on trial - retry in a minute"}` while the running image is on trial (the bytes are received and dropped; nothing restarts) |
| POST | `/api/ota/a32` | same | multipart file | Relay the image to the A32 over the link (chapter 3, section 3.10.3) | 200 "N bytes sent, the A32 is rebooting"; 500 `{"e":"<reason>"}`; 403 |

**Two answers to know about** (chapter 12): `POST /api/net` always answers "network saved - the radio
joins it in a few seconds", even for a request carrying only the time server and zone (the page always
sends the network name, so only a direct request sees this); and the page shows only "rebooting" for any
successful `POST /api/reboot`, whatever the radio answered.

## B.5 State fields

The fields of `GET /api/state` that belong to chapter 8. The other fields belong to the chapters that
produce them; chapter 4, section 4.10, lists the main board's health fields.

| Field | Meaning | Built by |
|---|---|---|
| `net.sta` | 1 = on the house network | `buildState()`, `src/s3/portal.cpp` |
| `net.ssid` | The house network's name, or `Ambersong` | `buildState()` |
| `net.ip` | The radio's address | `buildState()` |
| `net.rssi` | Received signal in dBm; 0 on the access point | `buildState()` |
| `net.ntp` | 1 = network time fresh | `buildState()` |
| `sess` | Active sessions | `buildState()` |
| `ota` | 1 = an upload is in progress | `buildState()` |
| `mustchg` | Set while the signed-in account is still the placeholder; the page then shows only **Set your account** | `portalStateJson()`, `src/s3/main.cpp` |
| `rtc` | The battery clock's health: 0 fine or not asked yet, 1 answered with no valid time, 2 not answering (chapter 4, section 4.10) | `portalStateJson()` |
| `linkver` | The S3's count of link frames dropped for another protocol version (chapter 3, section 3.13; chapter 9) | `portalStateJson()` |
| `log` | Only with `?log=` from an administrator: `{"from":F,"next":N,"t":"..."}`, up to 4096 console bytes, oldest first | — |
