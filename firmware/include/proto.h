// ============================================================================
//  AMBERSONG  -  INTER-MCU PROTOCOL
//
//  Shared by BOTH firmwares. This file is the contract; if it changes, both
//  sides get rebuilt. Nothing else may define these numbers.
//
//  THE LINK
//    The firmware uses S3 GPIO13 (TX) / GPIO14 (RX) and A32 GPIO26 (RX) /
//    GPIO27 (TX) - pins.h. What joins them is the Hardware Bible's (§2, §3).
//  Serial2 on BOTH sides, and on the A32 it MUST be given explicit pins - its
//  defaults are GPIO16/17, and GPIO17 is the firmware's LRCK. Calling
//  Serial2.begin() bare silently breaks the DAC and surfaces much later as
//  "no audio". See link.h.
//
//  WHY A FRAMED, CHECKSUMMED PROTOCOL AND NOT PRINTF
//  Three reasons, all learned rather than assumed:
//   1. The S3 relays the A32's firmware images over this link. A corrupted
//      byte in an image must be detected, not installed.
//   2. Both MCUs print to their own consoles. A framed link cannot be confused
//      by a stray log line arriving mid-message.
//   3. Version skew is real: the S3 can update the A32, so the two WILL run
//      mismatched builds at some point. A frame of another PROTO_VERSION is
//      dropped by the framer and counted (badVer; the S3 publishes it as
//      `linkver` and on its console), so skew shows as a silent link with a
//      named cause instead of as misread structs.
//
//  FRAME LAYOUT
//    The start pattern is two DIFFERENT bytes, 0xA5 0x5A, so no run of one
//    repeated value (an idle line reads 0xFF) can form it.
//
//    +------+------+-----+------+--------+---------+-------+
//    | 0xA5 | 0x5A | ver | type | len_lo | len_hi  | ...   |
//    +------+------+-----+------+--------+---------+-------+
//    | payload (len bytes) | crc_lo | crc_hi |
//    +---------------------+--------+--------+
//
//    ver..payload inclusive are covered by CRC16-CCITT (poly 0x1021,
//    init 0xFFFF). The start bytes are NOT covered - they are resynchronisation
//    markers, not data.
//
//  ENDIANNESS
//    Both ends are little-endian Xtensa with the same compiler, so packed
//    structs are copied verbatim. That is a deliberate, recorded shortcut. If a
//    third device ever joins this bus, everything here becomes byte-oriented.
//
//  LIVENESS
//    The A32 sends STATE unprompted every 250 ms. The S3 treats >2 s of silence
//    as "A32 missing" and keeps working - the display and needle do not
//    depend on the audio MCU. The A32 is the other way round, by the author's
//    awake rule (2026-09-01): it plays and takes Bluetooth only while the amp
//    is on AND the S3 is talking (serviceWake() in a32/main.cpp). When the S3
//    goes quiet - its own OTA reboot included - the A32 mutes, drops the phone
//    and forgets the amp, and wakes again only when the S3 is back and says
//    the amp is on.
// ============================================================================

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

//  2 : ProtoAudio gained fadeInMs / fadeOutMs, and the version was bumped with
//      it. A peer of the other version now has its frames dropped by the
//      framer (badVer) instead of misreading the struct.
//  3, 4 : NOT RECORDED. The value was already 4 in the repository's first
//      commit (2026-09-01), so what changed at 3 and at 4 happened
//      before there was a history to read it from. Written down 2026-09-24 so
//      nobody goes looking for two entries that were never kept.
//  Since then no bump: every struct change has been fields APPENDED to
//  ProtoState - see WHEN PROTO_VERSION CHANGES, under the message types.
#define PROTO_VERSION       4
#define PROTO_SOF0          0xA5
#define PROTO_SOF1          0x5A
#define PROTO_MAX_PAYLOAD   1088     // OTA chunk (1024) + header slack
#define PROTO_BAUD          921600   // line rate only: the acked OTA relay is far
                                     // slower (75.8 s for an A32 image, 2026-08-31)
#define PROTO_STATE_MS      250      // A32 -> S3 unprompted state cadence
#define PROTO_SILENCE_MS    2000     // beyond this, the peer is "missing"

//  Version string format, author's convention: v.X.YYYYMMDDTHHMMSS
#define PROTO_VERSION_LEN   24

// ---------------------------------------------------------------------------
//  MESSAGE TYPES
//  Explicit numbers so a mismatch between builds is diagnosable from a capture
//  rather than from source archaeology. 0x0n handshake, 0x1n S3->A32 control,
//  0x2n A32->S3 reporting, 0x3n OTA relay.
// ---------------------------------------------------------------------------
enum : uint8_t {
  MSG_HELLO         = 0x01,   // S3  -> A32   ProtoHello
  MSG_HELLO_ACK     = 0x02,   // A32 -> S3    ProtoHello
  MSG_PING          = 0x03,   // S3  -> A32   empty       (keeps the A32 awake)
  MSG_PONG          = 0x04,   // A32 -> S3    empty

  MSG_SET_AUDIO     = 0x10,   // S3  -> A32   ProtoAudio
  MSG_SET_BT        = 0x11,   // S3  -> A32   ProtoBtCfg
  MSG_SET_SYS       = 0x12,   // S3  -> A32   ProtoSys
  MSG_SET_TIME      = 0x13,   // S3  -> A32   ProtoTime   (NTP -> DS3231)
  MSG_GET_TIME      = 0x14,   // S3  -> A32   empty
  MSG_BT_FORGET     = 0x15,   // S3  -> A32   empty       (drop all pairings)
  MSG_BT_LOOK       = 0x16,   // S3  -> A32   empty       (open the pairing window,
                              //   BT_LINK - not BT_LOOK - and only on the BT source)
  MSG_REBOOT        = 0x17,   // S3  -> A32   empty
  MSG_CAL_POT       = 0x18,   // S3  -> A32   uint8: 0 = MIN, 1 = MAX, 2 = CENTRE
  MSG_BT_CMD        = 0x19,   // S3  -> A32   uint8: BTC_*  (AVRCP transport)
  MSG_GET_CFG       = 0x1A,   // S3  -> A32   empty       (send me your settings)
  MSG_TEST_DC       = 0x1B,   // S3  -> A32   uint8 0/1   constant-DC diagnostic
  MSG_ADC_CLOCK     = 0x1C,   // RETIRED 2026-09-01. GPIO0's clock feeds the
                              //   DAC as well (Hardware Bible §3), so cutting
                              //   MCLK is a DAC clock error, not a mute. The
                              //   id stays reserved so it is never reused.
  MSG_ZERO_FLOOR    = 0x1D,   // S3  -> A32   uint8 0/1   zero-data floor
  MSG_USE_POT       = 0x1E,   // S3  -> A32   uint8 0/1   pot drives volume

  //  0x4n : S3 -> A32 DIAGNOSTICS, a new block because the 0x1n control block
  //  had only 0x1F left. An earlier version of these put SD_DELAY at 0x20, which is
  //  MSG_STATE. It would have worked - STATE only ever travels A32 -> S3 and
  //  these only ever travel S3 -> A32, so no switch would have seen both - and
  //  that is exactly the kind of "correct by luck" this file exists to refuse.
  //  0x1C stays retired and unreused, as its own comment demands.
  MSG_DIN_DRIVE     = 0x40,   // S3  -> A32   uint8 0..3  DIN pad drive
  //  MSG_SD_DELAY RETIRED 2026-09-11, the day after it was added. It was a
  //  remote, unguarded write to tx_sd_out_delay, and that register sent the radio
  //  to FULL VOLUME: of its four positions only 0 plays correctly. It was ruled
  //  unsafe to ship a build that still carried it.
  //  The id stays reserved and is never reused, like MSG_ADC_CLOCK above.
  MSG_SD_DELAY      = 0x41,   // RETIRED. Nothing sends it; the A32 ignores it.
  MSG_SIGN_TAIL     = 0x42,   // S3  -> A32   uint8 0/1   sign-extended tail
  MSG_CLK_DRIVE     = 0x43,   // S3  -> A32   uint8 2..3  clock-pin drive
  MSG_DC_WORD       = 0x44,   // S3  -> A32   uint8 0..6  INDEX into kDc (a32/main.cpp),
                              //   never a raw word: the remote path can only pick
                              //   one of seven fixed words, all <= -42 dBFS
  MSG_BT_TX         = 0x45,   // S3  -> A32   uint8 0..5  BR/EDR max TX power
                              //   level (ESP_PWR_LVL_N12..P3). DOWN only: P3 is
                              //   the controller's own default maximum.
  MSG_CLK_PAIR      = 0x46,   // S3  -> A32   uint8[2]: MCLK drive, then BCK/LRCK
                              //   drive, each clamped 2..3. Written 2026-09-13,
                              //   when drive 3 on all three pads was heard to
                              //   distort the radio path, to set MCLK and the
                              //   frame pair apart. Boot sets all three to 3
                              //   (audio.cpp).

  //  WHEN PROTO_VERSION CHANGES - one rule, and it is what the repository's
  //  history did (v2, before the first commit, bumped for a size change).
  //   - A new MESSAGE ID: no bump. An older peer ignores an id it does not know.
  //   - A struct that changes SIZE: no bump. ProtoFramer::as() refuses a
  //     payload of the wrong length, so a mismatched peer loses only that
  //     message type, and the link itself stays up. Every struct change since
  //     the first commit has been fields APPENDED to ProtoState this way; a
  //     half-updated pair loses telemetry and keeps its audio. (ProtoOtaData
  //     is read at fixed offsets, not with as() - its layout is pinned by the
  //     static_asserts below the payloads.)
  //   - A change as() CANNOT see - same size, different layout or meaning:
  //     bump. Know the cost first: every frame of the other version is dropped
  //     (badVer), so the A32 sleeps by the awake rule until both boards run the
  //     new version. Update the A32 first, through the S3 while they still
  //     match; then the S3 over its own WiFi.

  MSG_STATE         = 0x20,   // A32 -> S3    ProtoState  (unprompted, 250 ms)
  MSG_TIME          = 0x21,   // A32 -> S3    ProtoTime
  MSG_LOG           = 0x22,   // A32 -> S3    char[]      (S3 console: "[A32] ...")
  MSG_CFG           = 0x23,   // A32 -> S3    ProtoCfgAll (reply to MSG_GET_CFG)

  MSG_OTA_BEGIN     = 0x30,   // S3  -> A32   ProtoOtaBegin
  MSG_OTA_DATA      = 0x31,   // S3  -> A32   ProtoOtaData
  MSG_OTA_END       = 0x32,   // S3  -> A32   uint32 crc32 of the whole image.
                              //   The image is STREAMED - the S3 is relaying an
                              //   HTTP upload and does not know the length or
                              //   the checksum until the last byte has gone by -
                              //   so the CRC arrives at the end, not in BEGIN.
  MSG_OTA_ABORT     = 0x33,   // S3  -> A32   empty
  MSG_OTA_STATUS    = 0x34,   // A32 -> S3    ProtoOtaStatus
};

// ---------------------------------------------------------------------------
//  ENUMS SHARED BY BOTH SIDES
// ---------------------------------------------------------------------------

//  Source selector. The A32 owns the ADC that reads the ladder and is therefore
//  the authority - the S3 learns the source from STATE, never assumes it.
//  The thresholds live in pins.h; which switch position puts which level on
//  the ladder is the Hardware Bible's (§11).
enum : uint8_t {
  SRC_AUX   = 0,
  SRC_BT    = 1,
  SRC_RADIO = 2,
};

//  Bluetooth state machine, author's specification of 2026-08-28, reproduced
//  exactly. LED is ACTIVE LOW: for brightness B, write 255 - B.
//
//  NOTE ON NAMING - LINK IS NOT "LINKED".
//  In this specification LINK means EXPLICIT PAIRING MODE, entered by pressing
//  the pair button. It is not the streaming state. An earlier version of this
//  header guessed "connected and streaming" and was wrong. The visual language
//  the author described is the authority:
//    fast breathing = actively looking      slow breathing = idle
//    double flash   = found                 steady         = connected
//    regular blink  = explicit pairing
enum : uint8_t {
  BT_OFF    = 0,   // not in BT mode: dark
  BT_ON     = 1,   // in BT mode, idle: steady, duty 64
  BT_LOOK   = 2,   // actively looking: breathe 32->255->32 over 1200 ms
  BT_STDBY  = 3,   // idle standby:     breathe 8->120->8 over 4000 ms
  BT_FOUND  = 4,   // one shot: 150 on / 150 off / 150 on, then CON
  BT_CON    = 5,   // connected: steady, duty 180
  BT_LINK   = 6,   // EXPLICIT PAIRING (button): blink 300 on / 300 off
};

// ---------------------------------------------------------------------------
//  PAYLOADS
// ---------------------------------------------------------------------------

struct __attribute__((packed)) ProtoHello {
  uint8_t protoVersion;                    // PROTO_VERSION
  char    fwVersion[PROTO_VERSION_LEN];    // v.X.YYYYMMDDTHHMMSS
};

//  All gains are in TENTHS OF A dB so they survive as integers: 80 = +8.0 dB,
//  the compiled radio default. Volume is 0..255 linear on the fader,
//  mapped to a curve on the A32 side.
struct __attribute__((packed)) ProtoAudio {
  uint8_t  volume;         // 0..255 master
  uint8_t  muted;          // 0/1 - SOFTWARE mute, by writing zeros. No A32 pin
                           // reaches the DAC's XSMT (Hardware Bible §6), so
                           // there is no hardware mute to use.
  int16_t  balance;        // -100..+100, negative = toward LEFT.
                           // Scales the A32's own output only, so it does
                           // nothing for AUX, which never passes through the
                           // A32 (Hardware Bible §22).
  int16_t  gainRadio;      // tenths of a dB, default +80 (measured 2026-09-23)
  int16_t  gainBt;         // tenths of a dB, default -70 (matched to the radio)
  uint8_t  monoSum;        // 0/1 - sum the radio's two ADC slots. Correlated
                           // signal, uncorrelated converter noise, so this buys
                           // about 3 dB of SNR.
  uint8_t  muteOnChange;   // 0/1 - ramp down across a source change, switch at
                           // silence, ramp up. Off, the change is a hard cut.
  uint16_t fadeInMs;       // ramp UP time when arriving at a source
  uint16_t fadeOutMs;      // ramp DOWN time when leaving one
                           // ASYMMETRIC ON PURPOSE. Leaving should be quick -
                           // you turned the knob, you want it gone. Arriving
                           // should be gentle, because the new source may
                           // already be playing at full tilt and a hard cut in
                           // is a jumpscare. Author asked for about 1 s in.
  uint8_t  taperX10;       // VOLUME LAW, gamma x 10. gain = (v/255) ^ gamma.
                           // 10 = linear amplitude, 20 = square (-12 dB at half
                           // travel), 25 = -15 dB, 30 = -18 dB, 33 is about what
                           // a real audio-taper pot does. Clamped 10..40.
                           //
                           // This exists because the pot is a SENSOR: the taper
                           // is not a property of the part any more, it is a
                           // number, and a number belongs in the portal.
};

//  AVRCP transport commands. These exist because the portal gives non-admin
//  users control of the MUSIC without control of the MACHINE - play, pause and
//  hang up are the things a guest legitimately wants and cannot break.
enum : uint8_t {
  BTC_PLAY = 0, BTC_PAUSE = 1, BTC_NEXT = 2, BTC_PREV = 3, BTC_DISCONNECT = 4,
};

struct __attribute__((packed)) ProtoBtCfg {
  uint8_t  connectable;    // 0/1 - may BONDED devices connect at all. Even at 1
                           // the A32 is connectable only while awake (amp on,
                           // S3 talking) and on the BT source (applyScanMode()
                           // in a32/main.cpp), so a phone cannot latch at 3 a.m.
                           // onto a radio whose amp is off.
  uint8_t  autoConnect;    // 0/1 - kept in the struct, but NOTHING READS IT: the
                           // A32 hard-codes set_auto_reconnect(false). The radio
                           // never chases the phone; the phone comes to it.
  uint16_t lookTimeoutS;   // LINK (the pairing window) -> STDBY after this many
                           // seconds, author: 60-120. LOOK -> STDBY is a fixed
                           // 20 s in a32/main.cpp.
  uint8_t  ledBrightness[7];  // indexed by BT_* state: a 0..255 scale on the
                              // specified brightness, 255 = as specified (btled.h)
  uint8_t  pauseOnLeave;   // 0/1 - send AVRCP pause BEFORE the disconnect that
                           // always follows the source leaving BT. Leaving BT
                           // mode drops the phone entirely (author's decision,
                           // 2026-08-28); this flag only controls whether it is
                           // stopped politely first, so its media app does not
                           // stay stuck in a playing state with nowhere to go.
};

struct __attribute__((packed)) ProtoSys {
  uint8_t  ampOn;          // the S3's amp-sense input (S3_AMP_SENSE, pins.h),
                           // which the S3 reads as LOW = amp powered. This
                           // field is already de-inverted: 1 = amp on.
  uint8_t  reserved[3];
};

struct __attribute__((packed)) ProtoTime {
  uint32_t unixUtc;        // seconds; UTC always, never local
  uint8_t  valid;          // 0 = the RTC's oscillator-stopped flag is set, its
                           // status did not answer, or the time is implausible
                           // (a32/rtc.h)
  int8_t   tempC4;         // DS3231 die temperature in quarter-degrees
  uint8_t  reserved[2];    // [0] carries the oscillator-stopped flag itself
                           // (a32/rtc.h); [1] unused
};

//  The A32 OWNS its own settings and persists them; the S3 keeps a mirror so
//  the portal can show what is actually in the machine rather than what it last
//  asked for. Requested once at every handshake - a peer that rebooted has the
//  values from ITS flash, not the ones we remember.
struct __attribute__((packed)) ProtoCfgAll {
  ProtoAudio audio;
  ProtoBtCfg bt;
  uint16_t   potMin, potMid, potMax;   // read-only here: the pot is on the A32
};

struct __attribute__((packed)) ProtoState {
  uint8_t  source;         // SRC_*
  uint8_t  btState;        // BT_*
  uint8_t  volume;         // echoed back so the portal shows truth, not intent
  uint8_t  muted;
  int16_t  peakLdBx10;     // -600 = -60.0 dBFS, for the portal meter; the
                           // peak since the last STATE, after the volume
  int16_t  peakRdBx10;
  uint8_t  avrcpVolume;    // 0..127 absolute volume. The PHONE can overwrite
                           // whatever we set - same value, last writer wins.
  uint8_t  clipped;        // sticky since last STATE
  uint16_t rawLadder;      // raw ADC on GPIO36, for diagnosing the selector
  char     peerName[24];   // connected BT device, empty if none

  //  THE AUDIO ENGINE'S HEALTH, added 2026-09-01 because the author heard loud
  //  pops on Bluetooth and there was no way to tell an underrun from an
  //  analogue problem without a serial cable. An underrun zero-fills the rest
  //  of the block, which is a discontinuity, which is a click - so this is the
  //  first thing to look at when the audio misbehaves.
  uint32_t underruns;      // total since boot; partial fills only (audio.cpp)
  uint16_t ringFill;       // high-water of the Bluetooth jitter ring, in
                           //   frames, since the last STATE

  //  THE ZERO-DATA WATCH, added 2026-09-10. See audio.cpp's ZERO-DATA WATCH
  //  block for the mechanism; this is the wire half of it. The theory it
  //  witnessed is EXCLUDED: zddRel stayed frozen through 8 minutes of pops on
  //  2026-09-10. Kept as telemetry.
  //
  //  WHY THESE FIELDS AND NOT A PROTO_VERSION BUMP: appended, so as() refuses
  //  an old peer's frame on its LENGTH and a half-updated machine loses
  //  TELEMETRY and keeps its audio. A bump would drop every frame and put the
  //  A32 to sleep - see WHEN PROTO_VERSION CHANGES, under the message types.
  uint32_t zddArm;         // times the DAC would have entered its analogue mute
  uint32_t zddRel;         // times it would have come back out
  uint32_t zddSinceRelMs;  // ms since the last release, 0xFFFFFFFF if never
  uint32_t zddLongestMs;   // longest single mute so far
  uint8_t  zddMuted;       // would the DAC be analogue-muted right now
  uint8_t  zddFloor;       // is the zero-data floor engaged (built as that
                           //   theory's fix; default off)
  uint16_t i2sSticky;      // I2S0.int_raw, OR-accumulated, never cleared
  uint32_t stalls;         // audio-task gaps long enough to arm the detector
  uint32_t stallMaxUs;     // the longest such gap

  //  THE VOLUME WATCH, added 2026-09-10 after the zero-data watch above came
  //  back FLAT while the author was hearing pops. What the log did show was the
  //  front knob's reading wandering 17..20 with nobody touching it, and the
  //  volume law is applied as a HARD STEP at a block boundary - so the question
  //  became "how far does that reading ever jump", and nothing in the machine
  //  could answer it. A 2 s portal poll cannot see a 50 ms excursion. That
  //  theory is excluded too: with the pot disabled and gainSteps frozen, it
  //  still popped. Kept as telemetry.
  //
  //  EVERY COUNTER AND EXTREME HERE IS MONOTONIC SINCE BOOT, deliberately. A per-window
  //  min/max would be drained by whichever reader got there first - the trap
  //  getPeaks() and i2sFaults() have both already sprung on this project - and
  //  worse, a rare glitch would land in one 250 ms frame and be gone before the
  //  next poll. Monotonic means a single excursion at 03:00 is still on the
  //  page at 09:00, which is the entire point of leaving it running overnight.
  uint16_t potRaw;         // latest ACCEPTED median reading
  uint16_t potSeenMin;     // extremes of the median reading, every read
  uint16_t potSeenMax;
  uint16_t potJumpMax;     // largest accepted |delta raw| in one 50 ms update
  uint32_t volSteps;       // volume changes since boot
  uint8_t  volJumpMax;     // largest |delta volume| in one update, 0..255
  uint32_t volJumpMs;      // millis() of that largest jump, to place it in time
  uint32_t gainSteps;      // applied-gain recomputes seen by the audio task
  uint32_t volQ16Min;      // the excursion of the gain ACTUALLY APPLIED to
  uint32_t volQ16Max;      //   the samples - 65536 is unity, 0 is silence
  uint8_t  usePot;         // is the pot still driving the volume
  uint8_t  dinDrive;       // READ BACK FROM THE PAD, never a shadow copy
  uint8_t  sdDelay;        //   - a sweep that reports what it asked for lies
  uint8_t  signTail;       // sign-extended tail engaged (tested 09-11: NOT the fix)
  uint8_t  clkDrive;       // clock-pin drive READ BACK from all three pads, 0xFF = disagree
  //  THE DC DIAGNOSTIC, published 2026-09-11 for the music-vs-DC A/B. The word is
  //  what makes the test mean anything: 0xFF000000 (-42 dBFS, the boot default)
  //  is far from any zero-data detector, which is exactly what run D's 0x00000100
  //  was never shown to be.
  uint8_t  testDc;         // constant-DC diagnostic engaged
  uint32_t dcWord;         // the word it writes
  //  STALL TIMESTAMPS, 2026-09-11 evening. The 5-minute DC block caught two pops
  //  inside a stall burst and four outside one, and a 2.7 s portal poll of the
  //  stall COUNT could only bracket them. With the A32's own clock in every frame
  //  a host can place each stall to within one 250 ms frame.
  uint32_t a32Ms;          // A32 millis() when this frame was built
  uint32_t stallLastMs;    // A32 millis() of the latest stall, 0 if none yet
  uint32_t stallLastUs;    // that stall's gap
  uint8_t  btTx;           // BR/EDR max TX power level, READ BACK from the
                           //   controller; 0xFF = it would not say
  uint8_t  btTxMin;        // and the min - the end that decides whether the link
                           //   runs near its margin, so it is published too
  uint8_t  mclkDrive;      // MCLK pad drive, READ BACK from the pad
  uint8_t  bckDrive;       // BCK pad drive
  uint8_t  lrckDrive;      // LRCK pad drive. clkDrive above still reports the
                           //   COMMON value and 0xFF when they disagree, which
                           //   after MSG_CLK_PAIR is a legitimate state, not a
                           //   fault.
  int16_t  rmsLdBx10;      // RMS after the source gain, BEFORE the volume, in
  int16_t  rmsRdBx10;      //   0.1 dBFS; drained each frame. -1200 = silence.
};

struct __attribute__((packed)) ProtoOtaBegin {
  uint32_t size;           // always 0: the image is streamed (see MSG_OTA_END)
  uint32_t crc32;          // always 0: the CRC arrives in MSG_OTA_END
  char     fwVersion[PROTO_VERSION_LEN];  // the S3's OWN FW_VERSION, not the
                                          // image's; the A32 only prints it
};

struct __attribute__((packed)) ProtoOtaData {
  uint32_t offset;         // absolute, so a resend is idempotent
  uint16_t len;
  uint8_t  data[1024];     // only the first `len` bytes are transmitted
};

struct __attribute__((packed)) ProtoOtaStatus {
  uint32_t received;       // bytes committed so far
  uint8_t  state;          // 1 receiving, 3 ok, 4 failed. 0 (idle) and 2
                           // (verifying) are reserved; the A32 never sends them.
  uint8_t  errCode;
  char     detail[32];
};

// ---------------------------------------------------------------------------
//  EVERY PAYLOAD FITS THE FRAME - PROVEN BY THE COMPILER, 2026-09-24.
//
//  Until then the only check was protoSelfTest() at boot, which encodes a
//  ProtoState and fails if it does not fit. That is the wrong moment to find
//  out: the A32's OTA is RELAYED THROUGH THIS LINK, so an image whose
//  ProtoState had grown past PROTO_MAX_PAYLOAD would be installed first and
//  found broken only after it booted (it would then run muted and never be
//  confirmed, and a reset takes the previous image back - a32/main.cpp,
//  ROLLBACK). A struct that does not fit is a fact about the SOURCE, so the
//  build is where it belongs. Such an image now cannot be built.
//
//  Every struct that travels in a frame is listed, not just ProtoState: the
//  one nobody thought to check is the one that grows.
// ---------------------------------------------------------------------------
static_assert(sizeof(ProtoHello)     <= PROTO_MAX_PAYLOAD, "ProtoHello exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoAudio)     <= PROTO_MAX_PAYLOAD, "ProtoAudio exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoBtCfg)     <= PROTO_MAX_PAYLOAD, "ProtoBtCfg exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoSys)       <= PROTO_MAX_PAYLOAD, "ProtoSys exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoTime)      <= PROTO_MAX_PAYLOAD, "ProtoTime exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoCfgAll)    <= PROTO_MAX_PAYLOAD, "ProtoCfgAll exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoState)     <= PROTO_MAX_PAYLOAD, "ProtoState exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoOtaBegin)  <= PROTO_MAX_PAYLOAD, "ProtoOtaBegin exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoOtaData)   <= PROTO_MAX_PAYLOAD, "ProtoOtaData exceeds PROTO_MAX_PAYLOAD");
static_assert(sizeof(ProtoOtaStatus) <= PROTO_MAX_PAYLOAD, "ProtoOtaStatus exceeds PROTO_MAX_PAYLOAD");

//  THE FIXED OFFSETS THE CODE HARD-WIRES. MSG_OTA_DATA is not parsed with as():
//  the A32 reads the offset at byte 0, the length at byte 4 and the data from
//  byte 6, by hand, because only the first `len` bytes of data[] are sent. If
//  ProtoOtaData were ever reordered those three numbers would silently read the
//  wrong bytes, so the layout they assume is pinned here.
static_assert(offsetof(ProtoOtaData, offset) == 0, "OTA data parsing assumes offset at byte 0");
static_assert(offsetof(ProtoOtaData, len)    == 4, "OTA data parsing assumes len at byte 4");
static_assert(offsetof(ProtoOtaData, data)   == 6, "OTA data parsing assumes data at byte 6");

// ---------------------------------------------------------------------------
//  CRC16-CCITT, poly 0x1021, init 0xFFFF.
//  Bitwise rather than table-driven: this runs at most a few thousand times a
//  second on control traffic, and during OTA it is not the bottleneck. A 512-
//  byte table would buy nothing and cost cache the display ISR wants.
// ---------------------------------------------------------------------------
static inline uint16_t protoCrc16(const uint8_t *d, size_t n, uint16_t crc = 0xFFFF) {
  while (n--) {
    crc ^= (uint16_t)(*d++) << 8;
    for (int i = 0; i < 8; i++)
      crc = (crc & 0x8000) ? (uint16_t)((crc << 1) ^ 0x1021) : (uint16_t)(crc << 1);
  }
  return crc;
}

// ---------------------------------------------------------------------------
//  ENCODER
//  Returns total bytes written, or 0 if it would not fit. Never partially
//  writes - a caller that ignores the return value still cannot emit a torn
//  frame.
// ---------------------------------------------------------------------------
static inline size_t protoEncode(uint8_t *out, size_t cap, uint8_t type,
                                 const void *payload, uint16_t len) {
  const size_t total = 6 + (size_t)len + 2;
  if (len > PROTO_MAX_PAYLOAD || cap < total) return 0;
  out[0] = PROTO_SOF0;
  out[1] = PROTO_SOF1;
  out[2] = PROTO_VERSION;
  out[3] = type;
  out[4] = (uint8_t)(len & 0xFF);
  out[5] = (uint8_t)(len >> 8);
  if (len && payload) memcpy(out + 6, payload, len);
  uint16_t crc = protoCrc16(out + 2, 4 + (size_t)len);
  out[6 + len]     = (uint8_t)(crc & 0xFF);
  out[6 + len + 1] = (uint8_t)(crc >> 8);
  return total;
}

// ---------------------------------------------------------------------------
//  DECODER
//  Byte-at-a-time state machine so it can be fed straight from a UART with no
//  intermediate buffering and no assumption that frames arrive whole.
//
//  Any failure drops only the frame at hand and goes back to hunting for a
//  start pattern: a bad version from the byte after it, an over-long length
//  from the byte after that, a bad CRC from the byte after the frame's last
//  CRC byte. It does NOT rescan the inside of a failed frame, so a corrupted
//  length byte can swallow up to PROTO_MAX_PAYLOAD + 2 following bytes as
//  "payload" before it is back in step.
// ---------------------------------------------------------------------------
class ProtoFramer {
 public:
  uint32_t badCrc   = 0;    // a rising count means bytes are being damaged on
                            // the way; the S3 publishes it (linkcrc)
  uint32_t overruns = 0;    // a length over PROTO_MAX_PAYLOAD; not published
  uint32_t badVer   = 0;    // a frame of another PROTO_VERSION; the S3
                            // publishes it (linkver)

  void reset() { st_ = S_SOF0; idx_ = 0; }

  //  Returns true exactly once per complete, CRC-valid frame.
  bool feed(uint8_t b) {
    switch (st_) {
      case S_SOF0:
        if (b == PROTO_SOF0) st_ = S_SOF1;
        return false;
      case S_SOF1:
        //  A second 0xA5 is not a failure - it may be the real start byte of a
        //  frame whose predecessor was truncated. Stay here rather than resync.
        if (b == PROTO_SOF1)      st_ = S_VER;
        else if (b == PROTO_SOF0) st_ = S_SOF1;
        else                      st_ = S_SOF0;
        return false;
      case S_VER:
        if (b != PROTO_VERSION) { badVer++; st_ = S_SOF0; return false; }
        st_ = S_TYPE;
        return false;
      case S_TYPE:  type_ = b;  st_ = S_LEN0; return false;
      case S_LEN0:  len_  = b;  st_ = S_LEN1; return false;
      case S_LEN1:
        len_ |= (uint16_t)b << 8;
        if (len_ > PROTO_MAX_PAYLOAD) { overruns++; st_ = S_SOF0; return false; }
        idx_ = 0;
        st_  = len_ ? S_PAYLOAD : S_CRC0;
        return false;
      case S_PAYLOAD:
        buf_[idx_++] = b;
        if (idx_ >= len_) st_ = S_CRC0;
        return false;
      case S_CRC0:  crc_ = b; st_ = S_CRC1; return false;
      case S_CRC1: {
        crc_ |= (uint16_t)b << 8;
        st_ = S_SOF0;
        uint8_t  hdr[4] = { PROTO_VERSION, type_,
                            (uint8_t)(len_ & 0xFF), (uint8_t)(len_ >> 8) };
        uint16_t c = protoCrc16(hdr, 4);
        c = protoCrc16(buf_, len_, c);
        if (c != crc_) { badCrc++; return false; }
        return true;
      }
    }
    return false;
  }

  uint8_t        type()    const { return type_; }
  uint16_t       length()  const { return len_;  }
  const uint8_t *payload() const { return buf_;  }

  //  Copies the payload into a typed struct only if the size matches exactly.
  //  A mismatch means the two firmwares disagree about this message - the skew
  //  a struct change without a PROTO_VERSION bump produces (see WHEN
  //  PROTO_VERSION CHANGES) - so it refuses rather than memcpy whatever fits.
  template <typename T>
  bool as(T &out) const {
    if (len_ != sizeof(T)) return false;
    memcpy(&out, buf_, sizeof(T));
    return true;
  }

 private:
  enum State : uint8_t { S_SOF0, S_SOF1, S_VER, S_TYPE, S_LEN0, S_LEN1,
                         S_PAYLOAD, S_CRC0, S_CRC1 };
  State    st_   = S_SOF0;
  uint8_t  type_ = 0;
  uint16_t len_  = 0;
  uint16_t idx_  = 0;
  uint16_t crc_  = 0;
  uint8_t  buf_[PROTO_MAX_PAYLOAD];
};
