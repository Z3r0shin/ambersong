# Appendix F. Link reference: constants, messages and payloads

This appendix is for anyone who changes the link's code or decodes the link with a logic analyser. To
use or repair the radio you need only chapter 9, which explains how these values are used.

It lists everything that travels on the link between the main board (S3) and the audio board (A32): the
frame, the constants, every message and every payload.
None of these values is a portal setting. All are compiled in; changing one means rebuilding, and for
those in `include/proto.h`, rebuilding and updating **both** boards (chapter 9, section 9.14).

## F.1 The frame

| Bytes | Field | Value |
|---|---|---|
| 0–1 | start marker | `0xA5 0x5A` (`PROTO_SOF0`, `PROTO_SOF1`); not covered by the checksum |
| 2 | version | `PROTO_VERSION`, 4 |
| 3 | type | the message id (section F.3) |
| 4–5 | length | payload length, little-endian, 0 to 1088 |
| 6 … 5 + length | payload | the message's structure (section F.4) |
| last 2 | checksum | CRC-16/CCITT-FALSE (polynomial 0x1021, initial value 0xFFFF, no final XOR), computed bit by bit over the version, type, length and payload; sent little-endian. The ASCII string `123456789` gives 0x29B1. |

A frame is 8 bytes plus its payload. The line carries about 92 kB/s at 921 600 baud, 8N1. Worked examples
of whole frames are in chapter 9, section 9.6.1.

| Message | Payload, bytes | Frame, bytes |
|---|---|---|
| HELLO, HELLO_ACK | 25 | 33 |
| STATE | 130 | 138 |
| CFG | 33 | 41 |
| TIME, SET_TIME | 8 | 16 |
| SET_AUDIO | 15 | 23 |
| SET_BT | 12 | 20 |
| SET_SYS | 4 | 12 |
| OTA_STATUS | 38 | 46 |
| OTA_DATA | 6 + up to 1024 | up to 1038 |
| empty messages | 0 | 8 |

## F.2 Constants

| Name | Value | Meaning | Where |
|---|---|---|---|
| `PROTO_VERSION` | 4 (0–255) | The version byte in every frame. Change it only by the procedure in chapter 9, section 9.14.5. Version 2 is when the audio settings gained their fades; what changed at 3 and 4 was not recorded. | `include/proto.h` |
| `PROTO_SOF0`, `PROTO_SOF1` | 0xA5, 0x5A | The start marker. | `include/proto.h` |
| `PROTO_MAX_PAYLOAD` | 1088 bytes | The largest payload, and the size of the decoder's buffer. Must hold every payload structure (checked when compiling). | `include/proto.h` |
| `PROTO_BAUD` | 921 600 baud | Link speed, 8N1. | `include/proto.h` |
| `PROTO_STATE_MS` | 250 ms | The A32's status report period. | `include/proto.h` |
| `PROTO_SILENCE_MS` | 2000 ms | How long without a valid frame before a board counts the other as silent; both boards. | `include/proto.h` |
| `PROTO_VERSION_LEN` | 24 bytes | The version string field in HELLO and OTA_BEGIN. | `include/proto.h` |
| Receive buffer | 4096 bytes | The serial receive buffer, set before the port opens. | `Link::begin()` |
| HELLO / PING period | 1000 ms | HELLO until answered, then PING. | S3 `loop()` |
| SET_SYS period | 2000 ms | The amplifier state to the A32; also sent at once on a change. | S3 `loop()` |
| GET_TIME period | 60 000 ms | Re-setting the S3's clock from the battery clock. | S3 `loop()` |
| Battery-clock answer wait | 5000 ms | A periodic GET_TIME unanswered this long, while handshaken, sets `rtc` = 2. | S3 `loop()` |
| Network time to the battery clock | at most once per 3 600 000 ms | Only while handshaken and network time is fresh. | S3 `loop()` |
| Restart-test slack | 5000 ms | The A32's millisecond count must fall back by more than this; earlier values from 0xF0000000 up are ignored. | S3 `onMessage()` |
| BEGIN answer wait | 20 000 ms | The S3 waits for the A32 to accept an update. | `a32OtaBegin()` |
| DATA answer wait | 3000 ms × 4 tries | Per frame. | `otaSendFrame()` |
| END answer wait | 4000 ms | State 3 or 4 required. | `a32OtaEnd()` |
| Upload silence | 15 000 ms | No upload activity this long: ABORT to the A32. | `otaWatchdog()`, `src/s3/portal.cpp` |
| Update frame | 1024 bytes | Data per `MSG_OTA_DATA`. | S3 `otaBuf` |
| Mute settle before flash | 400 ms | From muting to the first flash write. Must exceed the mute, the fade-out and the output buffers' drain. | A32 `MSG_OTA_BEGIN` handler |
| A32 update give-up | 20 000 ms | Since the last accepted DATA frame. | A32 `loop()` |
| Restart delay after success | 250 ms | Lets the final status go out. | A32 `MSG_OTA_END` handler |
| `WDT_TIMEOUT_S` | 15 s | The A32's task watchdog, with a restart. Must exceed start-up and the longest pass of the main loop. | `src/a32/main.cpp` |
| `CONFIRM_AFTER_HELLO_MS` | 60 000 ms | Trial confirmation, counted from the first HELLO. | `src/a32/main.cpp` |
| `CONFIRM_AFTER_MS` | 300 000 ms | Fallback confirmation when nothing at all was ever received. | `src/a32/main.cpp` |
| Confirmation retry | 10 000 ms | After a failed confirmation. | `confirmTick()` |
| A32 settings save wait | 2000 ms | After the last change; skipped by a forced save (Reboot the audio board, confirmation). | `settingsFlush()` |
| A32 settings writes on trial | none | Changes wait in RAM; written at confirmation. | `settingsFlush()` |
| `protoBroken` announcement | 10 000 ms | On the console and as a LOG line. | A32 `loop()` |
| Battery clock | address 0x68; registers 0x00, 0x0F, 0x11; 100 kHz | Chapter 9, section 9.9. | `src/a32/rtc.h`, `include/pins.h` |
| Battery-clock plausibility floor | 1 600 000 000 s (Unix) | September 2020; used on both boards. | `rtc.h`, S3 `onMessage()` |
| A32 partitions | `min_spiffs.csv`: two 1.875 MB program slots | Two slots are needed for both the relay and the rollback (chapter 3, section 3.5). | `platformio.ini` |
| `FW_VERSION` | `v.1.YYYYMMDDTHHMMSS` | Which binary; fits in 24 bytes; stamped at build time (chapter 3, section 3.4). Sent in HELLO and HELLO_ACK. | build |
| `FW_COMMIT` | short git hash, for example `a1b2c3d`; 7 characters, plus `-dirty`; or `nogit` | Which source; stamped at build time. Never sent in the handshake. | build |

## F.3 Messages

**Numbering.** The ids come in blocks: 0x0n handshake, 0x1n S3-to-A32 control, 0x2n A32-to-S3 reporting,
0x3n update relay, 0x4n S3-to-A32 diagnostics (opened because the 0x1n block is full at 0x1F). The
numbers are explicit "so a mismatch between builds is diagnosable from a capture". A retired id is never
reused. Every handler ignores an id it does not know.

| Id | Name | Direction | Payload | Sent by, when | Receiver does |
|---|---|---|---|---|---|
| 0x01 | `MSG_HELLO` | S3 to A32 | `ProtoHello` (25) | S3, every 1 s while not handshaken | Notes the S3's version; answers HELLO_ACK; sends the boot report (once per start-up); starts the 60 s confirmation clock (once). |
| 0x02 | `MSG_HELLO_ACK` | A32 to S3 | `ProtoHello` (25) | Answer to HELLO | Marks the handshake done; prints `[PASS] A32 up: proto vN, firmware X`; sends GET_TIME and GET_CFG. |
| 0x03 | `MSG_PING` | S3 to A32 | empty | S3, every 1 s while handshaken; the S3's live index monitor on each pass | Answers PONG. |
| 0x04 | `MSG_PONG` | A32 to S3 | empty | Answer to PING | Ignored by the S3. |
| 0x10 | `MSG_SET_AUDIO` | S3 to A32 | `ProtoAudio` (15) | A portal edit of an A32 audio setting; a settings upload (once, at the end) | Applies; saves 2 s later (on trial: at confirmation). |
| 0x11 | `MSG_SET_BT` | S3 to A32 | `ProtoBtCfg` (12) | A portal edit of an A32 Bluetooth setting; a settings upload | Applies; saves 2 s later (on trial: at confirmation). |
| 0x12 | `MSG_SET_SYS` | S3 to A32 | `ProtoSys` (4) | S3, every 2 s while handshaken, and at once when the amplifier state changes | Stores the amplifier state; recomputes visibility. |
| 0x13 | `MSG_SET_TIME` | S3 to A32 | `ProtoTime` (8) | Fresh network time (at most hourly, handshaken); console `W` (handshaken only) | Writes the battery clock; clears OSF. |
| 0x14 | `MSG_GET_TIME` | S3 to A32 | empty | At HELLO_ACK, then every 60 s | Reads the battery clock and answers TIME; sends nothing if the chip is silent (the S3 then sets `rtc` = 2). |
| 0x15 | `MSG_BT_FORGET` | S3 to A32 | empty | Portal action `bt.forget` | Removes every Bluetooth pairing. |
| 0x16 | `MSG_BT_LOOK` | S3 to A32 | empty | Portal action `bt.pair` | If the source is Bluetooth: drops the phone and opens pairing. |
| 0x17 | `MSG_REBOOT` | S3 to A32 | empty | Portal action `sys.rebootA32` | Saves pending settings at once (nothing while on trial), then restarts. |
| 0x18 | `MSG_CAL_POT` | S3 to A32 | uint8: 0 minimum, 1 maximum, 2 centre | Portal actions `pot.min`, `pot.max`, `pot.ctr` | Records that knob calibration point. |
| 0x19 | `MSG_BT_CMD` | S3 to A32 | uint8: 0 play, 1 pause, 2 next, 3 previous, 4 disconnect | Portal actions `bt.play`, `bt.pause`, `bt.next`, `bt.prev`, `bt.disconnect` | Sends the command to the phone (AVRCP); a disconnect is graceful. |
| 0x1A | `MSG_GET_CFG` | S3 to A32 | empty | At HELLO_ACK; portal action `sys.getcfg` | Answers CFG. |
| 0x1B | `MSG_TEST_DC` | S3 to A32 | uint8 0/1 | Portal action `sys.dctest` | Constant-DC output test on or off. |
| 0x1C | `MSG_ADC_CLOCK` | — | — | **Retired**; nothing sends it | No handler. |
| 0x1D | `MSG_ZERO_FLOOR` | S3 to A32 | uint8 0/1 | Portal action `sys.zfloor` | Zero-data floor on or off. |
| 0x1E | `MSG_USE_POT` | S3 to A32 | uint8 0/1 | Portal action `sys.pot` | The knob drives the volume, or is ignored. |
| 0x20 | `MSG_STATE` | A32 to S3 | `ProtoState` (130) | A32, every 250 ms, unprompted, from start-up | Fills the S3's picture of the A32's status; restart test. |
| 0x21 | `MSG_TIME` | A32 to S3 | `ProtoTime` (8) | Answer to GET_TIME | Re-sets the S3's clock if valid. |
| 0x22 | `MSG_LOG` | A32 to S3 | text, no terminating zero | The boot report; the confirmation result; settings held during the trial now saved; the `protoBroken` announcement every 10 s; a failed settings save; a Bluetooth disconnect that did not land | Printed as `[A32] <text>`. |
| 0x23 | `MSG_CFG` | A32 to S3 | `ProtoCfgAll` (33) | Answer to GET_CFG | Fills the settings mirror; the A32's settings are known. |
| 0x30 | `MSG_OTA_BEGIN` | S3 to A32 | `ProtoOtaBegin` (32) | Start of a relayed update | Refuses on trial; otherwise mutes and starts receiving. |
| 0x31 | `MSG_OTA_DATA` | S3 to A32 | 6 + length bytes (`ProtoOtaData`, read at fixed offsets) | Each 1 KB frame of the relay | Commits the bytes if the offset matches; always answers OTA_STATUS. |
| 0x32 | `MSG_OTA_END` | S3 to A32 | uint32: the CRC32 of the image | End of the relay | Compares the CRC32, finishes, answers, restarts. |
| 0x33 | `MSG_OTA_ABORT` | S3 to A32 | empty | A wrong chip; an unanswered END; an aborted upload; 15 s of upload silence | If an update is running: gives up and restores the user's mute. |
| 0x34 | `MSG_OTA_STATUS` | A32 to S3 | `ProtoOtaStatus` (38) | After every BEGIN, DATA, END and give-up | Fills the relay's answer fields. |
| 0x40 | `MSG_DIN_DRIVE` | S3 to A32 | uint8 0..3 | Portal action `sys.dindrv` | Drive strength of the DAC data pad. |
| 0x41 | `MSG_SD_DELAY` | — | — | **Retired**; nothing sends it | No handler. |
| 0x42 | `MSG_SIGN_TAIL` | S3 to A32 | uint8 0/1 | Portal action `sys.tail` | Sign-extended tail on or off. |
| 0x43 | `MSG_CLK_DRIVE` | S3 to A32 | uint8 2..3 | Portal action `sys.clkdrv` | Drive of all three clock pads. |
| 0x44 | `MSG_DC_WORD` | S3 to A32 | uint8 index 0..6 | Portal action `sys.dcword` | Picks one of seven fixed DC words, all at or below −42 dBFS; never a raw word; index 7 and up ignored. |
| 0x45 | `MSG_BT_TX` | S3 to A32 | uint8 0..5 (`ESP_PWR_LVL_N12` .. `P3`) | Portal action `sys.bttx` | Bluetooth transmit power. Values above P3 are ignored, so this path can only turn it down from the default maximum. |
| 0x46 | `MSG_CLK_PAIR` | S3 to A32 | uint8[2]: MCLK drive, then BCK/LRCK drive, each 2..3 | Portal action `sys.clkpair` | Drives the two clock groups separately. |

The diagnostics (0x1B, 0x1D, 0x1E, 0x40 to 0x46) belong to the pop hunt; chapter 10 explains them.

## F.4 Payloads

All payload structures are packed and copied byte for byte (chapter 9, section 9.6.1); they live in
`include/proto.h`. Sizes are in bytes.

| Structure | Size | Fields |
|---|---|---|
| `ProtoHello` | 25 | `protoVersion` (u8), `fwVersion[24]` |
| `ProtoAudio` | 15 | `volume`, `muted` (the software mute only), `balance`, `gainRadio` and `gainBt` (tenths of a dB), `monoSum`, `muteOnChange`, `fadeInMs`, `fadeOutMs`, `taperX10` (chapter 10 explains each) |
| `ProtoBtCfg` | 12 | `connectable`, `autoConnect`, `lookTimeoutS`, `ledBrightness[7]`, `pauseOnLeave` |
| `ProtoSys` | 4 | `ampOn` (1 = on), `reserved[3]` |
| `ProtoTime` | 8 | `unixUtc` (u32, always UTC), `valid` (u8), `tempC4` (i8, quarter degrees), `reserved[2]`; the A32 puts the battery clock's OSF bit in `reserved[0]` |
| `ProtoCfgAll` | 33 | `ProtoAudio`, `ProtoBtCfg`, `potMin`, `potMid`, `potMax` |
| `ProtoState` | 130 | source, Bluetooth state, volume, muted, peaks left and right, AVRCP volume, clipped, the raw selector reading, `peerName[24]`, underruns, ring-buffer fill, the zero-data watch, the volume watch, pad drives read back, the DC test and its word, **`a32Ms`** (the A32's millisecond count when the frame was built, used for the restart test and to place events in time), the last stall, the Bluetooth transmit power read back, the three clock pads' drives, RMS left and right |
| `ProtoOtaBegin` | 32 | `size` and `crc32` (both sent as 0: the image is streamed), `fwVersion[24]` (the S3 fills it with its **own** version) |
| `ProtoOtaData` | 1030 | `offset` (u32, absolute), `len` (u16), `data[1024]`; only `len` bytes of data are sent |
| `ProtoOtaStatus` | 38 | `received` (bytes committed), `state` (1 receiving, 3 ok, 4 failed; 0 idle and 2 verifying are defined but never sent), `errCode` (the updater's error number), `detail[32]` |

The updater's error numbers carried in `errCode` are listed in chapter 3, section 3.10.3.

**Checks at compile time.**

- Every one of the ten structures above is checked to fit in `PROTO_MAX_PAYLOAD`. "A struct that does not
  fit is a fact about the SOURCE, so the build is where it belongs." All ten are listed, because "the one
  nobody thought to check is the one that grows."
- The update data structure's layout is pinned: `offset` at byte 0, `len` at byte 4, `data` at byte 6.
  The A32 reads DATA frames at those offsets by hand.

## F.5 What the link prints

These lines appear on the S3's console (and its **Console** tab on the portal). Lines from the A32 itself
carry the `[A32]` prefix.

| Line | Meaning | Explained in |
|---|---|---|
| `[PASS] protocol v4, state frame 138 bytes, CRC rejects damage` | The protocol self-test passed at start-up. Both boards print it, each on its own console. | chapter 9, section 9.14.4 |
| `[PASS] A32 up: proto vN, firmware <version>` | Handshake done; the A32's version. | chapter 9, section 9.7 |
| `[A32] boot: commit <hash>, reset reason N, setup X ms, image <state>, watchdog on` | The A32's boot report, once per A32 start-up. | chapter 9, section 9.3 |
| `[WARN] A32 silent. Clock and needle keep running.` | Nothing valid from the A32 for 2 s. | chapter 9, sections 9.7 and 9.10 |
| `[WARN] the A32 restarted - handshaking again.` | The A32's millisecond count went backwards. | chapter 9, section 9.7 |
| `[FAIL] HELLO_ACK size - version skew` | A HELLO_ACK of the wrong size. | chapter 9, section 9.7 |
| `[WARN] the A32's STATE frame does not match this build. Flash BOTH MCUs. Audio is unaffected - only telemetry stops.` | A status report of the wrong length. | chapter 9, section 9.7 |
| `link : ... wrong-version N  <- the boards run different protocol versions: flash both` | Frames of another protocol version (console `s`). | chapter 9, section 9.5 |
| `[WARN] the DS3231 has no valid time yet.` | The battery clock answered with no valid time. | chapter 4, section 4.9 |
| `[A32] image confirmed (a minute of running with the S3)` or `(five minutes running, the S3 never spoke)` | The A32's new program is permanent. | chapter 3, section 3.11 |
| `[A32] image confirm FAILED (err N) - retrying` | The confirmation write failed. | chapter 3, section 3.11.2 |
| `[A32] settings changed during the trial are now saved` | Held settings written at confirmation. | chapter 3, section 3.11.5 |
| `[A32] settings NOT saved - the flash write failed; retrying` | An A32 settings write fell short. | chapter 7, section 7.4.2 |
| `[A32] proto self-test FAILED - muted, BT closed, awaiting OTA` | A broken A32 build; every 10 s. | chapter 9, section 9.2.1 |
