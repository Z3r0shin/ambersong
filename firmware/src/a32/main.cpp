// ============================================================================
//  AMBERSONG  -  AUDIO MCU (ESP32-WROOM-32)
//
//  THE AUDIO ENGINE: A2DP sink, radio capture through the ADC, output through
//  the DAC (the converters: Bible §6), the source switching between them, the
//  DS3231, and the S3 end of the link, in one firmware.
//
//  What makes it tractable is in audio.h: A2DP is told NOT to touch I2S. We install
//  the driver once, full duplex, and never reconfigure it. Source switching is
//  then a decision about where the next block of samples comes from.
//
//  BLUETOOTH CONNECTION POLICY - the author's rules, and why the mapping is
//  what it is.
//    "a phone that wanders into range must not latch on its own"
//    "a phone that has connected before may reconnect on demand FROM THE PHONE"
//    "the button is for devices that have NOT connected before"
//  Those three give exactly one arrangement:
//    NOT in BT mode      -> non-connectable, non-discoverable. Invisible.
//    in BT mode, idle    -> CONNECTABLE, NOT discoverable. A bonded phone can
//                           come back on its own; an unknown phone cannot even
//                           see the radio.
//    button pressed      -> connectable AND discoverable, for new pairings,
//                           until the pairing window (cfgB.lookTimeoutS) closes.
//    a phone connected   -> NOT connectable, so no second phone pages in behind it.
//  And all of it is additionally gated on being AWAKE - the amp on AND the S3
//  talking (see AWAKE / ASLEEP) - and on the proto self-test having passed. The
//  A32 runs whenever the set is plugged in (Bible §10), so without that gate a
//  phone could connect at 3 a.m. and silently route its audio into a radio that
//  is off. Every rule is applied in one place: applyScanMode().
//
//  The radio NEVER initiates. set_auto_reconnect(false), hard-coded in setup().
//  The phone comes to the radio, not the other way round.
// ============================================================================

#include <Arduino.h>
#include <Preferences.h>
#include <Update.h>
#include <rom/crc.h>
#include <esp_gap_bt_api.h>
#include <esp_bt.h>
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>
#include <esp_system.h>
#include "BluetoothA2DPSink.h"
#include "pins.h"
#include "link.h"
#include "audio.h"
#include "btled.h"
#include "rtc.h"

#ifndef FW_VERSION
#define FW_VERSION "v.0.unstamped"
#endif
#ifndef FW_COMMIT
#define FW_COMMIT "nogit"
#endif

static const char *DEVICE_NAME = "Ambersong";

// ---------------------------------------------------------------------------
//  NULL OUTPUT  -  the fix for a real bug, found on the hardware 2026-08-28.
//
//  `set_stream_reader(cb, false)` is documented as "do not write to I2S", and
//  it does suppress begin(), end() and write(). It does NOT suppress
//  set_sample_rate(): BluetoothA2DPSink.cpp calls `out->set_sample_rate()`
//  UNGATED by is_output, and BluetoothA2DPOutputDefault::set_sample_rate()
//  calls i2s_set_clk().
//
//  So the moment a phone started streaming, the library silently reconfigured
//  OUR driver from 32-bit full duplex to its own 16-bit TX settings. Observed
//  symptoms, all explained by that one call:
//    - Bluetooth played at full scale and garbled, ignoring the volume setting
//      entirely, because our 32-bit samples were being clocked out as 16-bit
//    - the active channel swapped, because the frame alignment moved
//    - and RADIO STAYED DEAD AFTERWARDS, because the capture side never came
//      back from a clock reconfiguration it never asked for
//
//  Relying on the flag was the mistake. This class makes it structural: the
//  library is handed an output object that has no way to reach I2S at all.
//  Same discipline as the rest of this firmware - make it impossible, do not
//  make it a rule to remember.
// ---------------------------------------------------------------------------
class NullA2dpOutput : public BluetoothA2DPOutput {
 public:
  bool   begin() override                            { return true; }
  size_t write(const uint8_t *, size_t len) override { return len; }
  void   end() override                              {}
  void   set_sample_rate(int) override               {}
  void   set_output_active(bool) override            {}
  void   write_silence(size_t) override              {}
};
static NullA2dpOutput nullOut;

//  See set_volume_control() below: this makes "the library does not touch our
//  samples" structural rather than hoped for.
static A2DPNoVolumeControl noVol;

//  The negotiated rate is not ours to choose - the phone picks it. Our I2S is
//  fixed at 44.1 kHz, so anything else plays at the wrong pitch. Say so loudly
//  rather than leaving it to sound mysteriously wrong.
static void onSampleRate(uint16_t rate) {
  Serial.printf("  A2DP negotiated %u Hz\n", rate);
  if (rate != 44100)
    Serial.printf("  [WARN] I2S is fixed at 44100 Hz. Audio will play at the\n"
                  "         wrong pitch (%.2fx). This needs resampling.\n",
                  rate / 44100.0);
}

// ---------------------------------------------------------------------------
//  THE LIBRARY MAY NOT SET THE SCAN MODE - 2026-09-24.
//
//  ESP32-A2DP 1.8.11 writes the scan mode on its own, on the Bluetooth task,
//  behind our back: set_scan_mode_connectable(true) on EVERY disconnect
//  (BluetoothA2DPSink.cpp, the DISCONNECTED case), once more when the stack
//  comes up (BT_APP_EVT_STACK_UP, which a2dp.start() only QUEUES - it lands
//  after start() has returned and after setup() applied our own mode), and
//  set_scan_mode_connectable(false) on every connect. Its "true" is CONNECTABLE
//  AND GENERAL_DISCOVERABLE (BluetoothA2DPCommon.cpp, `discoverability`
//  defaults to GENERAL).
//
//  applyScanMode() caches what it last wrote and returns early when nothing in
//  the POLICY changed, and none of those library writes changed the policy. So
//  after the portal's hang-up, a source change or going to sleep - every one of
//  which ends in a disconnect - the radio sat connectable AND discoverable in
//  RADIO, in AUX and asleep, with our cache still saying invisible, and it
//  stayed that way until something happened to move the policy. At boot the
//  queued STACK_UP did the same thing to a radio that had just set itself
//  invisible. That breaks the author's two rules outright: invisible on
//  Bluetooth unless in BT mode and awake, and nothing connects at night.
//
//  set_scan_mode_connectable() is virtual and every one of those calls goes
//  through it (set_scan_mode_connectable_default() included), so this subclass
//  takes the pen away: the library's request becomes a note that the stack has
//  just changed state, and loop() re-derives the mode from OUR state and writes
//  it, forced past the cache. Only applyScanMode() ever writes a scan mode now.
//  Runs on the Bluetooth task - it sets one flag and nothing else.
//  set_discoverability() also writes the mode directly; we never call it.
// ---------------------------------------------------------------------------
static volatile bool scanDirty = true;     // true: loop() writes it once at boot

class PolicyA2dpSink : public BluetoothA2DPSink {
 protected:
  void set_scan_mode_connectable(bool) override { scanDirty = true; }
};

static Link              gLink;
static PolicyA2dpSink    a2dp;
static Preferences       prefs;

//  POT END-POINT CALIBRATION.
//  A pot rarely reaches its own rails, so the knob's ends (and its centre) are
//  taken with the knob in place - calibratePot(), console p/c/P or the portal -
//  and kept in NVS. The numbers here are only the compiled fallback.
//  Stored on the A32 because the A32 reads the pot (Bible §7) - the reading and
//  the thing being calibrated must not live on opposite sides of a link.
static uint16_t potRawMin = 60;
static uint16_t potRawMid = 0;         // 0 = not measured, two-point mapping
static uint16_t potRawMax = 3990;


// --- settings, mirrored in NVS ---------------------------------------------
static ProtoAudio  cfgA;
static ProtoBtCfg  cfgB;
static bool        settingsDirty = false;
//  THE DC DIAGNOSTIC'S WORDS - the console 'd' key cycles them and sys.dcword
//  (MSG_DC_WORD) picks one BY INDEX. What each entry is for is tabled at the
//  'd' case. Every entry is <= -42 dBFS: the remote path can never put anything
//  louder on the wire, which is why it takes an index and not a word.
static const int32_t kDc[7] = { (int32_t)0xFF000000, 0x00FF0000,
                                (int32_t)0xFFFF0000, 0x00FFFFFF,
                                0x000000FF, 0x00100000, 0x001FFFFF };
static uint32_t    dirtySince    = 0;

// --- runtime ----------------------------------------------------------------
static volatile bool btConnected = false;
static volatile bool btStreaming = false;
//  A DISCONNECT WE ASKED FOR AND HAVE NOT SEEN LAND YET - 2026-09-24.
//  a2dp.disconnect() only POSTS the request; btConnected goes false later, from
//  the Bluetooth task's callback. In between, updateBtState() saw a phone still
//  connected and promoted whatever serviceDisconnect() had just set straight to
//  FOUND - so BTC_DISCONNECT never showed ON, and the pair button pressed with
//  a phone connected ended in LOOK instead of LINK (discoverable), i.e. it did
//  not do the one thing it is for. While this is set, the connected branch
//  HOLDS the requested state. Cleared by onConnState on the disconnect, by the
//  stale-flag sweep in updateBtState(), or by its timeout if the disconnect
//  never lands.
static volatile bool btDropping  = false;
static uint32_t      btDropSince = 0;
static const uint32_t BT_DROP_TIMEOUT_MS = 3000;
//  WHEN THE LAST DISCONNECT WAS POSTED, for any reason - 2026-09-24. The stray-
//  phone drop in updateBtState() waits BT_DROP_TIMEOUT_MS after it before asking
//  again, so a drop already in flight is not requested twice. 0 = none yet.
static uint32_t      strayDropAt = 0;
static bool     ampOn        = false;

// ---------------------------------------------------------------------------
//  AWAKE / ASLEEP  -  author's rule, 2026-09-01.
//
//  "If the AMP is detected as powered down, there is no reason to make the A32
//  do anything. It should just listen for the S3."
//
//  So asleep means: no Bluetooth latching, no blue lamp, no audio. The link,
//  the DS3231 and the OTA path stay up, because those are exactly the things
//  the S3 needs from us and the only reason to be awake at all.
//
//  LINK SILENCE COUNTS AS AMP-OFF, and that is a DELIBERATE choice he made
//  against the alternative. Asked whether an unreachable S3 should mean
//  "assume the amp is on so a dead helper board never silences the radio", he
//  chose the opposite: "if the S3 is unreachable then I want to know about it -
//  the radio doesn't work at all, no audio, no clock, no lights, nothing."
//
//  Consequences he accepted knowingly, written down so nobody 'fixes' them:
//    - an S3 reboot, or a long S3 OTA, silences the radio mid-listen
//    - there is a brief dead moment at every power-up, until the S3 first
//      reports, because ampOn starts false and peerAlive() starts false
//  Both are the alarm working, not a bug.
static bool     awake        = false;

// ---------------------------------------------------------------------------
//  THE ONE PLACE THAT DECIDES WHETHER SOUND COMES OUT.
//
//  serviceWake() asserted the mute on its edge and four other sites quietly
//  un-asserted it: settingsApply(), updateSource(), setup() and otaGiveUp()
//  all wrote cfgA.muted straight through with no regard for `awake`. The
//  audible consequence was the feature defeating itself - asleep BECAUSE the
//  S3 went quiet is the author's chosen alarm, and turning the source knob ran
//  updateSource(), which unmuted, and the alarm vanished while the S3 was
//  still dead. No portal needed, just the front knob.
//
//  Boot was the same fault from the other side: `awake` and the mute both
//  start false, so the edge never fires and the documented dead moment at
//  power-up never actually happened. The invariant was only ever maintained,
//  never established.
//
//  So mute is a FUNCTION OF STATE, not an event. Every site calls this - except
//  OTA begin, which forces cfgA.muted = 1 and so writes the same answer direct.
//
//  protoBroken JOINED THE FUNCTION 2026-09-24 - see the self-test in setup().
//  A build whose own framer failed its self-test used to halt before the link
//  came up, which - before rollback existed (2026-09-25) - made it unreachable
//  by OTA. It now stays up, silent and invisible to phones, so the S3 can
//  replace it; and confirmTick() never confirms it, so if it is an image on
//  trial a restart also takes the previous image back.
static bool     protoBroken  = false;
static void applyMute() { Audio::setMute(protoBroken || !awake || cfgA.muted); }
static uint8_t  source       = SRC_AUX;
static uint8_t  btState      = BT_OFF;
static uint32_t btStateSince = 0;
static uint32_t lastStateTx  = 0;
static uint32_t lastSrcPoll  = 0;
static char     peerName[24] = {0};
static bool     scanConnectable = false, scanDiscoverable = false;

//  BR/EDR TX POWER - the CEILING, as an esp_power_level_t index (N12 = 0 ..
//  P3 = 5, 3 dB steps). The controller picks inside [floor, ceiling]; the floor
//  is N0 unless the ceiling goes below it (applyBtTxPower). Boots at N0 = 0 dBm,
//  wire-settable by MSG_BT_TX, not persisted. Never above P3, which is the
//  part's own default maximum: this knob may only turn DOWN.
static uint8_t  btTxLevel = (uint8_t)ESP_PWR_LVL_N0;
//  READ BACK FROM THE CONTROLLER, never the shadow copy - the same rule the pad
//  drives follow - and ONCE, WHEN SET, never from the 4 Hz state builder: the
//  value only changes when it is set. 0xFF = never read, or the controller
//  would not say. (Polling it from loop() was blamed for an A32 reboot on
//  2026-09-11; withdrawn 2026-09-15 - the author had power-cycled the set.)
static uint8_t  btTxMaxSeen = 0xFF, btTxMinSeen = 0xFF;

static void applyBtTxPower() {
  if (btTxLevel > (uint8_t)ESP_PWR_LVL_P3) btTxLevel = (uint8_t)ESP_PWR_LVL_P3;
  //  THE FLOOR STAYS STOCK (N0) unless the ceiling is deliberately taken below
  //  it. esp_bt.h:324-325 says the controller may
  //  use any power in [min,max], so an N12 floor lets the link run near its
  //  margin - and BR/EDR is ARQ-based, so retransmissions would RISE. More RF
  //  activity is exactly what this change exists to reduce. With the floor kept,
  //  level 5 reproduces the stock range N0..P3 and the knob A/Bs against stock.
  uint8_t lo = (btTxLevel < (uint8_t)ESP_PWR_LVL_N0) ? btTxLevel
                                                    : (uint8_t)ESP_PWR_LVL_N0;
  esp_err_t e = esp_bredr_tx_power_set((esp_power_level_t)lo, (esp_power_level_t)btTxLevel);
  //  The ONE place the controller is asked what it actually has.
  esp_power_level_t rmn = ESP_PWR_LVL_N12, rmx = ESP_PWR_LVL_N12;
  if (esp_bredr_tx_power_get(&rmn, &rmx) == ESP_OK) {
    btTxMinSeen = (uint8_t)rmn; btTxMaxSeen = (uint8_t)rmx;
  } else {
    btTxMinSeen = btTxMaxSeen = 0xFF;
  }
  Serial.printf("  BT TX power: asked %d..%d dBm, controller says %d..%d - %s\n",
                -12 + 3 * (int)lo, -12 + 3 * (int)btTxLevel,
                (btTxMinSeen == 0xFF) ? -99 : -12 + 3 * (int)btTxMinSeen,
                (btTxMaxSeen == 0xFF) ? -99 : -12 + 3 * (int)btTxMaxSeen,
                (e == ESP_OK) ? "set" : "REFUSED by the controller");
}

// ---------------------------------------------------------------------------
//  A2DP -> our ring buffer. This runs on the Bluetooth task and must never
//  block; Audio::pushBt drops rather than waits for exactly that reason.
// ---------------------------------------------------------------------------
static void a2dpStream(const uint8_t *data, uint32_t len) { Audio::pushBt(data, len); }

static void onConnState(esp_a2d_connection_state_t s, void *) {
  bool c = (s == ESP_A2D_CONNECTION_STATE_CONNECTED);
  //  The drop flag is cleared BEFORE btConnected goes false, so loop() never
  //  sees "disconnected" with a drop still "in flight" from this side. The
  //  other side of that race - serviceDisconnect() setting the flag just after
  //  this cleared it - is swept up in updateBtState(); see there.
  if (!c) btDropping = false;              // the requested drop has landed
  btConnected = c;
  if (btConnected) {
    const char *n = a2dp.get_peer_name();
    strncpy(peerName, n ? n : "", sizeof(peerName) - 1);
  } else {
    peerName[0] = 0;
  }
  //  AFTER btConnected, and deliberately a second time: the library called
  //  set_scan_mode_connectable() (see PolicyA2dpSink) BEFORE this callback, so
  //  a loop() pass in between may already have re-applied the mode with the
  //  connection state from before the change. This flag guarantees one more
  //  pass that sees the new one.
  scanDirty = true;
}

static void onAudioState(esp_a2d_audio_state_t s, void *) {
  btStreaming = (s == ESP_A2D_AUDIO_STATE_STARTED);
}

// ---------------------------------------------------------------------------
//  Scan mode is the whole connection policy, so it is set in exactly one place
//  and derived from state rather than poked from event handlers.
// ---------------------------------------------------------------------------
static void applyScanMode(bool force = false) {
  //  !protoBroken: a build that failed its self-test is kept alive for OTA
  //  only, and a phone that latched onto it would be playing into a mute.
  //  !btConnected, 2026-09-24: NOT CONNECTABLE WHILE A PHONE IS ON. This was
  //  the library's own rule (non-connectable on every connect) and it is what
  //  the radio has always done in practice; now that the library no longer
  //  writes the mode (PolicyA2dpSink) it has to be ours, or a second phone
  //  could page in behind the first. The disconnect re-opens it via scanDirty.
  bool connectable  = cfgB.connectable && awake && (source == SRC_BT) && !protoBroken
                      && !btConnected;
  bool discoverable = connectable && (btState == BT_LINK);
  //  force: the controller's mode is not known to match the cache - the stack
  //  just changed state (scanDirty), or this is the first write at boot.
  if (!force && connectable == scanConnectable && discoverable == scanDiscoverable) return;
  scanConnectable  = connectable;
  scanDiscoverable = discoverable;
  esp_bt_gap_set_scan_mode(
      connectable  ? ESP_BT_CONNECTABLE      : ESP_BT_NON_CONNECTABLE,
      discoverable ? ESP_BT_GENERAL_DISCOVERABLE : ESP_BT_NON_DISCOVERABLE);
  Serial.printf("  BT scan mode: %s / %s\n",
                connectable ? "connectable" : "NON-connectable",
                discoverable ? "discoverable" : "not discoverable");
}

// ---------------------------------------------------------------------------
//  GRACEFUL DISCONNECT  -  author's decision, 2026-08-28: "disconnect entirely,
//  but pause before".
//
//  The pause matters. Tearing down A2DP under a phone that still thinks it is
//  playing leaves its media app in a playing state with nowhere to go; an AVRCP
//  pause first stops it cleanly, and the disconnect that follows is then just
//  the link going away. The pause is cfgB.pauseOnLeave (on by default); with
//  it off, the disconnect still waits the same 250 ms.
//
//  The gap between the two is deliberate and must not become a delay() - the
//  audio task is pinned to another core and would survive it, but blocking the
//  loop would stall the link, the LED and the source polling for no reason.
// ---------------------------------------------------------------------------
static void setBtState(uint8_t s);             // defined just below

static uint32_t disconnectAt         = 0;      // 0 = nothing pending
static uint8_t  afterDisconnectState = 0xFF;   // 0xFF = let the normal logic decide

static void gracefulDisconnect(uint8_t nextState) {
  if (!btConnected) {
    if (nextState != 0xFF) setBtState(nextState);
    return;
  }
  if (cfgB.pauseOnLeave) a2dp.pause();
  disconnectAt         = millis() + 250;
  afterDisconnectState = nextState;
  Serial.println(F("  BT: pausing the phone, then disconnecting."));
}

static void serviceDisconnect() {
  if (!disconnectAt || millis() < disconnectAt) return;
  disconnectAt = 0;
  //  Flag BEFORE the request, not after: the callback that clears it runs on
  //  another task and could otherwise land first, leaving the flag stuck on
  //  and a phone that reconnects inside the timeout held out of FOUND/CON.
  //  Only when a next state is pending - with 0xFF the source has left BT and
  //  updateBtState() goes to OFF regardless of the connection.
  if (afterDisconnectState != 0xFF && btConnected) { btDropSince = millis(); btDropping = true; }
  a2dp.disconnect();
  strayDropAt = millis() | 1;
  //  "disconnecting", not "disconnected" - 2026-09-24. disconnect() only posts
  //  the request (see btDropping); the line used to claim a drop that had not
  //  happened, and sometimes never did.
  Serial.println(F("  BT: disconnecting."));
  if (afterDisconnectState != 0xFF) setBtState(afterDisconnectState);
  afterDisconnectState = 0xFF;
}

static void setBtState(uint8_t s) {
  if (s == btState) return;
  btState = s;
  btStateSince = millis();
  BtLed::setState(s);
  applyScanMode();
}

static void forgetPairings() {
  int n = esp_bt_gap_get_bond_device_num();
  if (n <= 0) { Serial.println(F("  no pairings to forget.")); return; }
  esp_bd_addr_t *list = (esp_bd_addr_t *)malloc(sizeof(esp_bd_addr_t) * n);
  if (!list) return;
  if (esp_bt_gap_get_bond_device_list(&n, list) == ESP_OK)
    for (int i = 0; i < n; i++) esp_bt_gap_remove_bond_device(list[i]);
  free(list);
  Serial.printf("  forgot %d pairing(s).\n", n);
}

// ---------------------------------------------------------------------------
//  SETTINGS
//  Writes are DEBOUNCED: saved 2 s after the last change (settingsFlush), so a
//  portal slider dragged across its range writes NVS once, not on every step.
// ---------------------------------------------------------------------------
static void settingsDefaults() {
  memset(&cfgA, 0, sizeof(cfgA));
  cfgA.volume       = 40;      // placeholder: the first knob reading replaces it
  cfgA.muted        = 1;       // cleared by settingsLoad(): mute is never restored
  cfgA.balance      = 0;
  //  MEASURED 2026-09-23 through this firmware, with the tube radio as it was
  //  set up that day (its own volume and the rest: Bible §19, §22). At 0 dB the
  //  radio read RMS -29.9 dBFS with peaks to -14.0 before the volume; the old
  //  +15 clipped at the gain stage, i.e. at EVERY volume. +8 puts peaks at about -6 dBFS, RMS -22.
  //  Bluetooth: a -14 dBFS RMS pink-noise reference (streaming-level loudness)
  //  from a PC at full volume arrived at about -15; -7 lands it within ~1 dB of
  //  the radio, so a source change does not change loudness.
  cfgA.gainRadio    = 80;      // +8.0 dB
  cfgA.gainBt       = -70;     // -7.0 dB
  cfgA.monoSum      = 1;
  cfgA.muteOnChange = 1;
  cfgA.fadeInMs     = 1000;    // author's request: a gentle arrival, not a jumpscare
  cfgA.fadeOutMs    = 150;     // leaving should be quick - you turned the knob
  cfgA.taperX10     = 25;      // gamma 2.5: half travel is -15 dB. Tune by ear.

  memset(&cfgB, 0, sizeof(cfgB));
  cfgB.connectable  = 1;
  cfgB.autoConnect  = 0;       // stored and shown, but nothing reads it: the radio
                               // never chases the phone (set_auto_reconnect(false))
  cfgB.lookTimeoutS = 90;      // the pairing window (BT_LINK); LOOK is a fixed 20 s
  cfgB.pauseOnLeave = 1;
  for (int i = 0; i < 7; i++) cfgB.ledBrightness[i] = 255;   // exactly as specified
}

static void settingsApply() {
  Audio::setVolume(cfgA.volume);
  applyMute();
  Audio::setBalance(cfgA.balance);
  Audio::setGains(cfgA.gainRadio, cfgA.gainBt);
  Audio::setMonoSum(cfgA.monoSum);
  Audio::setMuteOnChange(cfgA.muteOnChange);
  Audio::setFades(cfgA.fadeInMs, cfgA.fadeOutMs);
  Audio::setTaper(cfgA.taperX10);
  BtLed::setScale(cfgB.ledBrightness);
  applyScanMode();
}

static void settingsLoad() {
  settingsDefaults();
  prefs.begin("amb", true);
  potRawMin = prefs.getUShort("potMin", 60);
  potRawMid = prefs.getUShort("potMid", 0);
  potRawMax = prefs.getUShort("potMax", 3990);
  //  A stored blob from an older layout has a different size and is REJECTED
  //  by this check, so defaults apply rather than a struct being reinterpreted.
  size_t n = prefs.getBytesLength("audio");
  if (n == sizeof(cfgA)) prefs.getBytes("audio", &cfgA, sizeof(cfgA));
  n = prefs.getBytesLength("bt");
  if (n == sizeof(cfgB)) prefs.getBytes("bt", &cfgB, sizeof(cfgB));
  prefs.end();

  //  MUTE IS NOT A SETTING, so it is never restored.
  //  It was persisted in an early version, and on 2026-08-28 that cost a real
  //  debugging session: the set booted muted, RADIO produced silence, and there
  //  was nothing on the front of the machine to say why. It only came back
  //  because pressing '+' happens to clear mute.
  //  Volume is loaded with the rest of the blob, but the first knob reading
  //  replaces it (updateVolumePot): the knob position IS the volume.
  cfgA.muted = 0;
}

static void settingsTouch() { settingsDirty = true; dirtySince = millis(); }

static bool     gImgOnTrial   = false;       //  booted as a pending-verify OTA image
static bool     gImgConfirmed = false;       //  see confirmImage(), above setup()

//  force: skip the two-second debounce - for a restart, which would otherwise
//  take a change made in the last two seconds with it.
static void settingsFlush(bool force = false) {
  if (!settingsDirty || (!force && millis() - dirtySince < 2000)) return;
  //  NOTHING IS WRITTEN WHILE THIS IMAGE IS ON TRIAL (the S3's v.1.0.1 hold,
  //  brought over). An update that changes ProtoAudio or
  //  ProtoBtCfg would save its layout during the trial, and a rollback would
  //  then find a structure of the wrong size and run it on defaults. Changes
  //  wait in RAM; confirmImage() writes them the moment the image is confirmed.
  //  A restart during the trial is a rollback, so there is nothing to keep.
  if (gImgOnTrial && !gImgConfirmed) return;
  //  ONLY WHAT REACHED FLASH COUNTS AS SAVED (2026-09-25, the S3's 09-24 fix
  //  brought over). The flag used to drop before the write and every result was
  //  ignored, so a failed write said "saved" and the change was gone at the next
  //  reboot. Now a failure stays unsaved, says so, and is retried after the
  //  debounce.
  bool ok = prefs.begin("amb", false);
  if (ok) {
    ok  = prefs.putBytes("audio", &cfgA, sizeof(cfgA)) == sizeof(cfgA);
    ok &= prefs.putBytes("bt",    &cfgB, sizeof(cfgB)) == sizeof(cfgB);
    ok &= prefs.putUShort("potMin", potRawMin) == sizeof(uint16_t);
    ok &= prefs.putUShort("potMid", potRawMid) == sizeof(uint16_t);
    ok &= prefs.putUShort("potMax", potRawMax) == sizeof(uint16_t);
    prefs.end();
  }
  if (!ok) {
    dirtySince = millis();
    const char *m = "settings NOT saved - the flash write failed; retrying";
    Serial.printf("  [WARN] %s\n", m);
    gLink.send(MSG_LOG, m, (uint16_t)strlen(m));
    return;
  }
  settingsDirty = false;
  Serial.println(F("  settings saved."));
}

// ---------------------------------------------------------------------------
static uint8_t readSource(uint16_t &raw) {
  //  Median of 5. GPIO36 (A32_MODE_ADC) is input-only with no internal pulls.
  //  The switch's three positions and the levels they give are in the Bible
  //  (§11, §22); the centre reads 0 V = AUX. The thresholds and the ordering
  //  AUX < BT < RADIO come from this firmware by the author's DPDT ruling
  //  (Bible §0) and live in pins.h - never infer them from anything else.
  //  SETTLE THE CHANNEL FIRST.
  //  GPIO36 is SENSOR_VP, which shares an internal amplifier path, and it reads
  //  high for a while after a DIFFERENT ADC1 channel has been sampled. This
  //  function interleaves with the volume pot (A32_VOLUME_POT in pins.h) every
  //  50 ms, and the first of the five samples was always the contaminated one -
  //  straight into the median. Seen 2026-08-31: 78 counts of residue.
  //  Three throwaway conversions cost 900 us and remove it.
  for (int i = 0; i < 3; i++) { (void)analogRead(A32_MODE_ADC); delayMicroseconds(300); }

  uint16_t v[5];
  for (int i = 0; i < 5; i++) { v[i] = analogRead(A32_MODE_ADC); delayMicroseconds(200); }
  for (int i = 1; i < 5; i++)
    for (int j = i; j > 0 && v[j] < v[j - 1]; j--) { uint16_t t = v[j]; v[j] = v[j - 1]; v[j - 1] = t; }
  raw = v[2];
  if (raw <  A32_MODE_THRESH_AUX_BT)   return SRC_AUX;
  if (raw >= A32_MODE_THRESH_BT_RADIO) return SRC_RADIO;
  return SRC_BT;
}

// ---------------------------------------------------------------------------
//  FRONT PANEL VOLUME POT
//
//  The pot is a sensor, not an audio component. Consequences worth stating:
//   - one reading drives BOTH channels, so gang matching and channel imbalance
//     stop existing as a category
//   - a gritty track becomes a noisy READING, which a median filter absorbs;
//     in the audio path the same grit is crackle, which nothing absorbs
//   - the taper is a firmware curve, so the pot's own law no longer matters
//   - and a knob move never writes NVS. The knob position IS the stored state:
//     the volume that rides along in the saved audio blob is replaced by the
//     first reading at boot.
// ---------------------------------------------------------------------------
static bool     usePot     = true;
static uint16_t potRaw     = 0;
static uint32_t lastPotMs  = 0;
static bool     potStream  = false;    // 'r': raw ADC at 10 Hz, for curve work
static uint32_t lastStreamMs = 0;

//  Wider than the ADC's own jitter, or the volume would tremble at rest and
//  every tremble would be a ramp. About 0.6% of travel.
static const uint16_t POT_DEADBAND = 24;

//  THE VOLUME WATCH, 2026-09-10. All monotonic since boot - see ProtoState.
static uint16_t potSeenMin = 0xFFFF;
static uint16_t potSeenMax = 0;
static uint16_t potJumpMax = 0;
static uint32_t volSteps   = 0;
static uint8_t  volJumpMax = 0;
static uint32_t volJumpMs  = 0;

//  RAW ADC -> ROTATION, 0..255.
//
//  Two points are not enough. Measured on this set: knob at MIN and MAX
//  calibrated, knob at mechanical CENTRE still read 215 of 255 - so the reading
//  is nowhere near proportional to the rotation, and no volume law applied
//  afterwards can undo that. Whatever the cause (the pot's own taper, the
//  ESP32's ADC curve, or both), the fix is the same: measure a third point in
//  the middle and interpolate between the three.
//
//  Each half of the travel gets its own straight line. The centre reading maps
//  to 128 BY CONSTRUCTION, which is the whole point.
static uint8_t potRotation(uint16_t raw) {
  int32_t lo = potRawMin, mid = potRawMid, hi = potRawMax;
  if (hi <= lo + 64) { lo = 60; hi = 3990; mid = 0; }      // implausible: defaults

  int32_t pct;
  if (mid > lo + 32 && mid < hi - 32) {
    if ((int32_t)raw <= mid) pct =       ((int32_t)raw - lo)  * 128 / (mid - lo);
    else                     pct = 128 + ((int32_t)raw - mid) * 127 / (hi - mid);
  } else {
    pct = ((int32_t)raw - lo) * 255 / (hi - lo);           // no centre yet
  }
  return (uint8_t)constrain(pct, 0, 255);
}

static void updateVolumePot() {
  //  Same settling discipline, for the same reason - the ladder is sampled
  //  between these and leaves its own residue behind.
  for (int i = 0; i < 2; i++) { (void)analogRead(A32_VOLUME_POT); delayMicroseconds(300); }

  uint16_t v[5];
  for (int i = 0; i < 5; i++) { v[i] = analogRead(A32_VOLUME_POT); delayMicroseconds(150); }
  for (int i = 1; i < 5; i++)
    for (int j = i; j > 0 && v[j] < v[j-1]; j--) { uint16_t t=v[j]; v[j]=v[j-1]; v[j-1]=t; }
  uint16_t raw = v[2];

  //  THE FIRST READING ALWAYS APPLIES. The knob position IS the stored state,
  //  so coming up at whatever volume happened to be in NVS - while the knob
  //  says something else - is exactly the silent-boot failure that mute
  //  persistence caused on 2026-08-28. The deadband is for the readings after.
  static bool first = true;

  //  EVERY MEDIAN READING, accepted or not. The deadband below decides what
  //  reaches the volume; this decides nothing and only records. Both extremes
  //  are wanted: a glitch can go either way, and a reading that collapses
  //  toward zero is as much a fault as one that flies to full scale.
  if (raw < potSeenMin) potSeenMin = raw;
  if (raw > potSeenMax) potSeenMax = raw;

  uint16_t d = (raw > potRaw) ? (raw - potRaw) : (potRaw - raw);
  if (!first && d < POT_DEADBAND) return;
  if (!first && d > potJumpMax) potJumpMax = d;
  first  = false;
  potRaw = raw;

  uint8_t vol = potRotation(raw);

  if (vol != cfgA.volume) {
    //  RECORD THE SIZE OF THE STEP, AND WHEN. The deadband suppresses SMALL
    //  jitter and does nothing whatever about a LARGE excursion - a big one
    //  clears 24 counts trivially and is applied in full, for one 50 ms poll,
    //  before the next reading pulls it back. If that is the pop, this is the
    //  number that shows it, and volJumpMs is what places it against the
    //  moment the author heard something.
    uint8_t dv = (vol > cfgA.volume) ? (vol - cfgA.volume) : (cfgA.volume - vol);
    volSteps++;
    if (dv > volJumpMax) { volJumpMax = dv; volJumpMs = millis(); }
    cfgA.volume = vol;
    Audio::setVolume(vol);        // Audio applies the taper curve
    //  Deliberately NO settingsTouch() - see above.
  }
}

//  Take the CURRENT reading as one end of the pot travel. The knob must be at
//  that end when this is called - there is no way for the firmware to know
//  where the knob is except by being told.
//  which: 0 = MIN (fully anticlockwise), 1 = MAX (fully clockwise),
//         2 = CENTRE (mechanical middle).
static void calibratePot(uint8_t which) {
  uint16_t v[9];
  for (int i = 0; i < 9; i++) { v[i] = analogRead(A32_VOLUME_POT); delay(3); }
  for (int i = 1; i < 9; i++)
    for (int j = i; j > 0 && v[j] < v[j-1]; j--) { uint16_t t=v[j]; v[j]=v[j-1]; v[j-1]=t; }
  uint16_t raw = v[4];                       // median of nine, it only happens once
  if      (which == 1) potRawMax = raw;
  else if (which == 2) potRawMid = raw;
  else                 potRawMin = raw;
  settingsTouch();
  Serial.printf("  pot %s = %u   (now %u .. %u .. %u  ->  vol %u)\n",
                which == 1 ? "MAX" : (which == 2 ? "CENTRE" : "MIN"),
                raw, potRawMin, potRawMid, potRawMax, potRotation(raw));
  if (potRawMax <= potRawMin + 32)
    Serial.println(F("  [WARN] that range is too small to be real - check the wiring."));
}

// ---------------------------------------------------------------------------
//  OTA RECEIVER
//
//  The S3 has no reset or boot line to this MCU (Bible §3, §5), so the UART is
//  the only way to update it without opening the cabinet. The S3
//  relays an HTTP upload into 1 kB frames and waits for an acknowledgement on
//  each one; every ack carries the byte count committed so far, which makes a
//  lost frame decidable rather than fatal.
//
//  THE AUDIO STOPS FIRST, and deliberately. A flash write stalls every piece of
//  code that is not in IRAM, and the audio engine is not - it would tear rather
//  than pause. Muting through the existing fade-out means the update sounds
//  like the source being switched, which it already does gracefully.
// ---------------------------------------------------------------------------
static bool     otaOn  = false;
static uint32_t otaGot = 0;
static uint32_t otaCrc = 0;
static uint32_t otaLastMs = 0;   // last frame seen, for the give-up timer
//  THE USER'S MUTE, AS IT WAS BEFORE THE UPDATE TOOK IT - 2026-09-24.
//  OTA begin forces cfgA.muted = 1, and otaGiveUp() used to write 0 back
//  unconditionally, so a failed update on a set the user had muted UNMUTED it.
//  The mute that the update imposed is the only one the update may lift.
static uint8_t  otaMutedWas = 0;

static void otaSay(uint8_t state, uint8_t err, const char *detail) {
  ProtoOtaStatus st = {};
  st.received = otaGot;
  st.state    = state;
  st.errCode  = err;
  strncpy(st.detail, detail, sizeof(st.detail) - 1);
  gLink.send(MSG_OTA_STATUS, st);
}

static void otaGiveUp(const char *why) {
  if (otaOn) Update.abort();
  otaOn = false;
  otaSay(4, Update.getError(), why);
  Serial.printf("  [FAIL] OTA: %s\n", why);
  cfgA.muted = otaMutedWas;        // back to what the user had, not to 0
  applyMute();
}

static void bootReport();                    //  defined above setup()
static uint32_t gHelloAt = 0;                //  millis of the first S3 handshake - see confirmTick()

static void onMessage(const ProtoFramer &f) {
  switch (f.type()) {
    case MSG_HELLO: {
      ProtoHello h;
      if (!f.as(h)) break;
      gLink.notePeerHello(h);
      ProtoHello me = {};
      me.protoVersion = PROTO_VERSION;
      strncpy(me.fwVersion, FW_VERSION, PROTO_VERSION_LEN - 1);
      gLink.send(MSG_HELLO_ACK, me);
      bootReport();
      if (!gHelloAt) gHelloAt = millis() ? millis() : 1;   // confirmTick() takes it from here
      break;
    }
    case MSG_PING: gLink.send(MSG_PONG); break;

    case MSG_SET_AUDIO:
      if (f.as(cfgA)) { settingsApply(); settingsTouch(); }
      break;

    case MSG_SET_BT:
      if (f.as(cfgB)) { settingsApply(); settingsTouch(); }
      break;

    case MSG_SET_SYS: {
      ProtoSys s;
      if (!f.as(s)) break;
      if (s.ampOn != ampOn) {
        ampOn = s.ampOn;
        Serial.printf("  amp is now %s\n", ampOn ? "ON" : "OFF");
        applyScanMode();
      }
      break;
    }

    case MSG_GET_TIME: {
      ProtoTime t;
      if (Rtc::read(t)) gLink.send(MSG_TIME, t);
      else Serial.println(F("  [WARN] DS3231 did not answer."));
      break;
    }

    case MSG_SET_TIME: {
      ProtoTime t;
      if (!f.as(t)) break;
      //  The S3 has NTP and we have the battery, so this is the only direction
      //  that makes sense: network time comes in, the RTC becomes the holdover
      //  for when there is no network.
      if (Rtc::write(t.unixUtc)) {
        Serial.print(F("  RTC set to "));
        Serial.print(t.unixUtc);
        Serial.println(F(" UTC"));
      } else {
        Serial.println(F("  [FAIL] could not write the DS3231."));
      }
      break;
    }

    case MSG_BT_CMD:
      if (f.length() == 1) {
        switch (f.payload()[0]) {
          case BTC_PLAY:       a2dp.play();     break;
          case BTC_PAUSE:      a2dp.pause();    break;
          case BTC_NEXT:       a2dp.next();     break;
          case BTC_PREV:       a2dp.previous(); break;
          case BTC_DISCONNECT: gracefulDisconnect(BT_ON); break;
          default: break;
        }
      }
      break;

    //  MSG_ADC_CLOCK is RETIRED - see the note where setAdcClock() used to
    //  live in audio.cpp. An old S3 that still sends it is ignored, which is
    //  the safe direction: the message can no longer take GPIO0 away from a
    //  DAC that now needs it.

    case MSG_DIN_DRIVE:
      if (f.length() == 1) {
        Audio::setDinDrive(f.payload()[0]);
        Serial.printf("  DIN drive %u\n", Audio::dinDriveIs());
      }
      break;

    //  MSG_SD_DELAY: RETIRED 2026-09-11, see proto.h. Deliberately no case - an
    //  old S3 that still sends it is ignored, which is the safe direction.

    case MSG_CLK_DRIVE:
      if (f.length() == 1) {
        Audio::setClockDrive(f.payload()[0]);
        Serial.print(F("  clock-pin drive now "));
        Serial.println(Audio::clockDriveIs());
      }
      break;

    case MSG_SIGN_TAIL:
      if (f.length() == 1) {
        Audio::setSignTail(f.payload()[0] != 0);
        Serial.println(Audio::signTailOn()
          ? F("  sign-extended tail ON - DIN static across the frame boundary (all but 1 word in 512)")
          : F("  sign-extended tail off"));
      }
      break;

    case MSG_USE_POT:
      //  A ONE-VARIABLE TEST, reachable with the cabinet shut: with the pot
      //  ignored the knob's reading leaves the audio path entirely and the
      //  volume stays at whatever was last set. If the pops stop, the knob's
      //  reading was making them. The console 'v' key does the same over USB.
      //  Not persisted: every boot starts with the pot active.
      if (f.length() == 1) {
        usePot = f.payload()[0] != 0;
        Serial.printf("  volume pot %s\n",
                      usePot ? "ACTIVE" : "IGNORED - volume is whatever was last set");
      }
      break;

    case MSG_ZERO_FLOOR:
      if (f.length() == 1) {
        Audio::setZeroFloor(f.payload()[0] != 0);
        Serial.printf("  zero-data floor %s\n",
                      Audio::zeroFloorOn()
                        ? "ON - the DAC can no longer analogue-mute itself"
                        : "off");
      }
      break;

    case MSG_TEST_DC:
      if (f.length() == 1) {
        Audio::setTestDc(f.payload()[0] != 0);
        Serial.print(F("  constant-DC diagnostic "));
        Serial.println(Audio::testDcOn() ? F("ON - anything you hear now is the fault")
                                         : F("off"));
      }
      break;

    case MSG_CLK_PAIR:
      if (f.length() == 2) {
        Audio::setClockDriveSplit(f.payload()[0], f.payload()[1]);
      }
      break;

    case MSG_BT_TX:
      if (f.length() == 1 && f.payload()[0] <= (uint8_t)ESP_PWR_LVL_P3) {
        btTxLevel = f.payload()[0];
        applyBtTxPower();
      }
      break;

    case MSG_DC_WORD:
      if (f.length() == 1 && f.payload()[0] < 7) {
        Audio::setDcWord(kDc[f.payload()[0]]);
        Serial.printf("  DC word 0x%08lX (index %u)\n",
                      (unsigned long)(uint32_t)Audio::dcWordIs(), (unsigned)f.payload()[0]);
      }
      break;

    case MSG_GET_CFG: {
      ProtoCfgAll c;
      c.audio  = cfgA;
      c.bt     = cfgB;
      c.potMin = potRawMin;
      c.potMid = potRawMid;
      c.potMax = potRawMax;
      gLink.send(MSG_CFG, &c, sizeof(c));
      break;
    }

    case MSG_CAL_POT:
      if (f.length() == 1) calibratePot(f.payload()[0]);
      break;

    case MSG_BT_FORGET: forgetPairings(); break;

    case MSG_BT_LOOK:
      if (source == SRC_BT) gracefulDisconnect(BT_LINK);
      break;

    case MSG_OTA_BEGIN: {
      ProtoOtaBegin b;
      if (!f.as(b)) break;
      //  NOT WHILE THIS IMAGE IS STILL ON TRIAL. The update would be written
      //  over the other slot - the very image a rollback would return to - and
      //  a second bad build would then have nothing to fall back on. Refused
      //  before anything is muted; the S3 shows the reason.
      if (gImgOnTrial && !gImgConfirmed) {
        otaSay(4, 0, "still on trial - retry in 1 min");
        break;
      }
      Serial.printf("  OTA starting, sent by S3 %s\n", b.fwVersion);

      //  Silence, then let the fade actually finish before the flash goes busy.
      //
      //  400 ms, NOT 250, since 2026-09-11. The budget: the mute flag is seen at
      //  the next block boundary (<= 5.8 ms), the fade takes fadeOutMs (default
      //  150 ms), and THEN the 8-buffer DMA chain has to drain (46.4 ms) - about
      //  200 ms. (It was raised while the chain was briefly 16 buffers, 92.9 ms,
      //  which left 250 ms no margin; it stayed at 400 when the chain went back
      //  to 8.) When the wait runs short, Update.write() erases flash with the
      //  cache off, the audio task freezes mid-drain, and tx_desc_auto_clear
      //  substitutes EXACT ZEROS for the buffers still holding audio - a level
      //  step at the DAC, a click at the start of every OTA. fadeOutMs is
      //  wire-settable (proto.h); 400 ms covers a fade up to about 340 ms.
      //
      //  Remember the user's mute first - but only if no update is already
      //  running: a second BEGIN would otherwise record the 1 we forced here as
      //  "what the user had", and a failure would leave the set muted.
      if (!otaOn) otaMutedWas = cfgA.muted;
      cfgA.muted = 1;
      Audio::setMute(true);
      delay(400);

      otaGot = 0;
      otaCrc = 0;
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { otaGiveUp("begin refused"); break; }
      otaOn = true;
      otaLastMs = millis();
      otaSay(1, 0, "receiving");
      break;
    }

    case MSG_OTA_DATA: {
      if (!otaOn) break;
      if (f.length() < 6) break;
      const uint8_t *p = f.payload();
      uint32_t off; uint16_t len;
      memcpy(&off, p,     4);
      memcpy(&len, p + 4, 2);
      //  OFFSET IS ABSOLUTE, so a frame that arrives twice is simply ignored
      //  and one that arrives early is refused - the ack tells the sender where
      //  we actually are and it resends from there.
      if (off == otaGot && len && (size_t)(6 + len) <= f.length()) {
        if (Update.write((uint8_t *)(p + 6), len) != len) { otaGiveUp("write failed"); break; }
        otaCrc  = crc32_le(otaCrc, p + 6, len);
        otaGot += len;
        otaLastMs = millis();
      }
      otaSay(1, 0, "");
      break;
    }

    case MSG_OTA_END: {
      if (!otaOn) break;
      uint32_t want = 0;
      if (f.length() == 4) memcpy(&want, f.payload(), 4);
      if (want && want != otaCrc) { otaGiveUp("crc mismatch"); break; }
      otaOn = false;
      if (!Update.end(true)) { otaGiveUp("end failed"); break; }
      Serial.printf("  OTA wrote %lu bytes, crc ok. Rebooting.\n", (unsigned long)otaGot);
      otaSay(3, 0, "ok, rebooting");
      delay(250);
      ESP.restart();
      break;
    }

    case MSG_OTA_ABORT:
      if (otaOn) otaGiveUp("aborted by the S3");
      break;

    //  SAVE FIRST. "Reboot the audio board" within two seconds of changing one
    //  of its settings used to restart before the debounced write, and the
    //  change came back as the old value.
    case MSG_REBOOT: settingsFlush(true); ESP.restart(); break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
static void updateBtState() {
  uint32_t held = millis() - btStateSince;

  //  A STALE DROP FLAG IS SWEPT FIRST, in every mode - 2026-09-24.
  //  serviceDisconnect() tests btConnected and then sets btDropping; the
  //  disconnect can land on the Bluetooth task in between, and onConnState()
  //  then clears the flag BEFORE it is set. Harmless until the next connection,
  //  which would be held out of FOUND and end in a false "did not land" below.
  //  Only this task ever SETS the flag, so clearing it here while nothing is
  //  connected cannot lose one.
  if (btDropping && !btConnected) btDropping = false;

  //  Asleep is dark, whatever the selector says. BT_OFF is the lamp's own
  //  "not in BT mode" state, so this needs no new LED mode.
  if (!awake || source != SRC_BT) {
    setBtState(BT_OFF);
    //  A PHONE THAT GOT IN ANYWAY IS SENT AWAY - 2026-09-24. Until
    //  PolicyA2dpSink, the library re-opened the radio on every disconnect, so
    //  a bonded phone could connect at night or in RADIO, and nothing here ever
    //  dropped it: the drops only happen on EDGES (leaving BT, falling asleep).
    //  The window is closed now, but the rule is "nothing connects at night",
    //  not "nothing can", so a connection found here is dropped through the
    //  same courteous path as leaving BT mode. One request per
    //  BT_DROP_TIMEOUT_MS: disconnect() only posts, and the link stays up for a
    //  while after it, which must not read as a new stray every pass.
    //  strayDropAt is zeroed on the wake edge, with the pending disconnect.
    if (btConnected && !disconnectAt &&
        (!strayDropAt || millis() - strayDropAt >= BT_DROP_TIMEOUT_MS)) {
      strayDropAt = millis() | 1;              // 0 means "none yet"
      Serial.println(F("  BT: a phone is connected while asleep or out of BT mode - dropping it."));
      gracefulDisconnect(0xFF);
    }
    return;
  }

  //  A DROP WE REQUESTED IS STILL IN FLIGHT: hold the state serviceDisconnect()
  //  set (ON or LINK) rather than read the dying connection as a new "found".
  //  The timeout is the fallback for a disconnect that never lands - the phone
  //  is then genuinely still connected, and the lamp should say so.
  if (btDropping && btConnected) {
    if (millis() - btDropSince < BT_DROP_TIMEOUT_MS) return;
    btDropping = false;
    //  ON THE WIRE TOO, 2026-09-24: with the cabinet shut the console is not
    //  there, and a phone that stayed on after the portal hung up is exactly
    //  what the author would ask about. Same channel and form as kBroken in
    //  loop(): MSG_LOG, a constant string, no buffer.
    static const char kNoLand[] = "BT: the disconnect did not land - still connected";
    Serial.printf("  [WARN] %s.\n", kNoLand);
    gLink.send(MSG_LOG, kNoLand, (uint16_t)(sizeof(kNoLand) - 1));
  }

  if (btConnected) {
    if (btState != BT_CON && btState != BT_FOUND) setBtState(BT_FOUND);
    //  FOUND -> CON IS THE LAMP'S DECISION, and the published state now follows
    //  it - 2026-09-24. BtLed ends the 450 ms double flash by moving ITSELF to
    //  CON (btled.cpp, BT_FOUND case), and nothing ever told btState, so
    //  ProtoState reported FOUND for the whole connection and the portal never
    //  saw CON. Reading the lamp's state back, rather than running a second
    //  450 ms timer here, keeps one clock for the handover: the published state
    //  and the lamp cannot disagree about when "found" became "connected".
    //  BtLed::update() runs earlier in the same loop() pass, so the lag is zero.
    else if (btState == BT_FOUND && BtLed::state() == BT_CON) setBtState(BT_CON);
    return;
  }

  switch (btState) {
    case BT_OFF:
      //  Entering BT mode: LOOK - the lamp's "looking" pattern - for 20 s,
      //  then STDBY. Both are connectable to bonded phones and not
      //  discoverable; the radio itself never pages a phone.
      setBtState(BT_LOOK);
      break;
    case BT_LOOK:
      if (held > 20000) setBtState(BT_STDBY);
      break;
    case BT_ON:
    case BT_STDBY:
      //  HELD, deliberately - stated 2026-09-24, when BT_ON first became
      //  reachable. ON is where the portal's hang-up (BTC_DISCONNECT) leaves the
      //  radio: in BT mode, idle, connectable to bonded phones, not
      //  discoverable. That is the same situation as STDBY, and STDBY has never
      //  timed out - only LOOK and LINK do, because those two are ACTIVE states
      //  with a window that must close. An idle state has nothing to time out
      //  to. Leaving BT mode, sleeping, or a phone connecting all move it on.
      break;
    case BT_LINK:
      //  Explicit pairing window closes on its own so the radio does not sit
      //  discoverable forever. Its length is cfgB.lookTimeoutS, despite the
      //  name - it times LINK, not LOOK.
      if (held > (uint32_t)cfgB.lookTimeoutS * 1000) setBtState(BT_STDBY);
      break;
    case BT_CON:
    case BT_FOUND:
      //  Was connected, now is not.
      setBtState(BT_LOOK);
      break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
//  The only place awake/asleep changes, and it is edge-triggered - running the
//  teardown every pass would re-send a disconnect forever.
// ---------------------------------------------------------------------------
static void serviceWake() {
  bool want = ampOn && gLink.peerAlive();
  if (want == awake) return;
  awake = want;

  if (!awake) {
    Serial.printf("  ASLEEP - %s\n",
                  !ampOn ? "amp is down" : "the S3 has gone quiet");
    //  AND FORGET THE AMP when the S3 is the reason. Its last word may be stale
    //  by the time it speaks again - the amp switched off while the S3 was
    //  rebooting - and waking on the old "amp on" unmuted into an amp that was
    //  off, connectable, until the next SET_SYS (author: clear
    //  it so it re-checks). Now it stays asleep until the S3 says so again.
    if (!gLink.peerAlive()) ampOn = false;
    //  Pause the phone before dropping it, the same courtesy the selector
    //  already gets: an app left thinking it is playing into nowhere was a
    //  real dead end on the hardware.
    gracefulDisconnect(BT_OFF);
    applyMute();
    setBtState(BT_OFF);
    applyScanMode();
  } else {
    Serial.println(F("  AWAKE - amp is up and the S3 is talking."));
    //  CANCEL A DISCONNECT THAT HAS NOT FIRED YET. Going to sleep with a phone
    //  connected arms disconnectAt = now + 250 ms. Wake inside that window
    //  and, without this, serviceDisconnect() still drops the phone 250 ms
    //  later and applies BT_OFF - lamp dark while awake and in BT mode, until updateBtState()
    //  walks it back on a later pass.
    disconnectAt = 0; afterDisconnectState = 0xFF; strayDropAt = 0;
    applyMute();
    updateBtState();
    applyScanMode();
  }
}

static void updateSource() {
  uint16_t raw;
  uint8_t s = readSource(raw);
  if (s == source) return;

  uint8_t was = source;
  source = s;
  //  Always say the volume and mute state alongside the source. Silence has
  //  several possible causes and the console should never leave you guessing
  //  which one you are looking at.
  Serial.printf("  source -> %-5s   volume %u/255 %s\n",
                s == SRC_AUX ? "AUX" : s == SRC_BT ? "BT" : "RADIO",
                cfgA.volume, cfgA.muted ? "  *** MUTED ***" : "");

  //  Leaving BT mode drops the phone entirely, after pausing it. Staying
  //  connected while the selector is elsewhere produced a dead end on the
  //  hardware: the phone showed connected, play appeared to work, and nothing
  //  came out. BT LED dark now means BT is genuinely off, and the phone agrees.
  if (was == SRC_BT) gracefulDisconnect(0xFF);

  Audio::setSource(s);
  //  AUX never passes through the A32 and the amp's input panel sums its inputs
  //  (Bible §22, §30), so on AUX the DAC must carry silence. That silence is
  //  STRUCTURAL - the audio task writes zeros for AUX - so mute here means only
  //  what the user asked for, on top of whether we are awake. Mute is software
  //  only: no A32 pin reaches the DAC's XSMT (Bible §6).
  applyMute();
  updateBtState();
  applyScanMode();
}

// ---------------------------------------------------------------------------
//  AN UPDATE THAT CANNOT BOOT MUST NOT NEED THE USB CABLE.
//
//  ROLLBACK. The bootloader and the core are built with app rollback, but the
//  core's initArduino() marked every new image valid BEFORE setup() - so a
//  relayed image that crashed or hung at boot looped on itself, and only USB
//  could fix it (the S3 has no reset line to this MCU, Bible §3). Returning
//  true here tells the core "this firmware confirms itself": a freshly OTA'd
//  image now boots ON TRIAL, and the bootloader goes back to the previous image
//  on the next reset unless confirmImage() ran. confirmTick() runs it a minute
//  after the first handshake with the S3, or - if nothing at all ever came from
//  the S3 - after five minutes of running, so a good image is never thrown away
//  just because the S3 was absent; never while the proto self-test has failed.
//  A USB flash is not on trial and is unaffected.
//
//  THE WATCHDOG is what turns a HANG into that reset. The core's task watchdog
//  reboots the chip (panic on) but only watched core 0's idle task: a frozen
//  loop() - or a Bluetooth start that never returns - went unnoticed, and
//  "reboot the audio board" is a message a frozen loop cannot read. loop() is
//  now watched from the first line of setup(), with room for the Bluetooth
//  start and for the longest legitimate wait in loop() (an OTA's image check).
// ---------------------------------------------------------------------------
extern "C" bool verifyRollbackLater() { return true; }

static const uint32_t WDT_TIMEOUT_S   = 15;
static const uint32_t CONFIRM_AFTER_MS = 5UL * 60UL * 1000UL;
static uint32_t gSetupMs      = 0;       // how long setup() took, for the report
static const char *gImgState  = "?";
static bool     gLoopWdtOn    = false;

//  WHEN A TRIAL IMAGE IS CONFIRMED. Not at
//  the handshake itself - that lands a second after boot, before the image has
//  woken, opened Bluetooth or played, so a build that crashed THERE was
//  already "good" and boot-looped. A full minute of running after the S3 first
//  said HELLO. And the five-minute fallback only when no good frame and no
//  bad-CRC frame ever arrived (rxCount and badCrc both 0: the S3 is taken to be
//  absent). A build that hears frames failing their CRC but never a handshake
//  stays on trial, so the next reset takes the old image back. NOT caught:
//  frames dropped for another PROTO_VERSION (badVer) or no start pattern at all
//  (wrong baud) leave both counters at 0, so such a build is confirmed after
//  five minutes as if the S3 were absent.
static const uint32_t CONFIRM_AFTER_HELLO_MS = 60000;

static void confirmImage(const char *why) {
  if (!gImgOnTrial || gImgConfirmed) return;
  esp_err_t e = esp_ota_mark_app_valid_cancel_rollback();
  char msg[96];
  if (e == ESP_OK) {
    gImgConfirmed = true;
    gImgState = "valid (confirmed)";
    snprintf(msg, sizeof(msg), "image confirmed (%s)", why);
    //  What the trial held back (settingsFlush), written now - and said on the
    //  link, so the hold is visible from the portal console.
    if (settingsDirty) {
      settingsFlush(true);
      if (!settingsDirty) {
        const char *h = "settings changed during the trial are now saved";
        gLink.send(MSG_LOG, h, (uint16_t)strlen(h));
      }
    }
  } else {
    //  Not given up on: retried every ten seconds from loop(). Until it lands,
    //  a reset would roll back a good image - and an OTA is refused.
    snprintf(msg, sizeof(msg), "image confirm FAILED (err %d) - retrying", (int)e);
  }
  Serial.printf("  %s\n", msg);
  gLink.send(MSG_LOG, msg, (uint16_t)strlen(msg));
}

static void confirmTick(uint32_t now) {
  //  Never a build whose proto self-test failed: it runs on, degraded, only
  //  so that it can be replaced - a restart takes the previous image back.
  if (!gImgOnTrial || gImgConfirmed || protoBroken) return;
  static uint32_t lastTry = 0;
  if (lastTry && now - lastTry < 10000) return;
  if (gHelloAt) {
    if (now - gHelloAt < CONFIRM_AFTER_HELLO_MS) return;
    lastTry = now;
    confirmImage("a minute of running with the S3");
  } else if (now > CONFIRM_AFTER_MS && gLink.rxCount() == 0 && gLink.badCrc() == 0) {
    lastTry = now;
    confirmImage("five minutes running, the S3 never spoke");
  }
}

//  Once per boot, at the first HELLO, to the S3's console: the commit, what
//  reset us, how long setup took, whether this image is on trial, whether the
//  loop watchdog is on, and whether an earlier update was rolled back. That is
//  how a rollback - or a watchdog reset - becomes visible without the USB cable.
static void bootReport() {
  static bool said = false;
  if (said) return;
  said = true;
  char msg[160];
  snprintf(msg, sizeof(msg), "boot: commit %s, reset reason %d, setup %lu ms, image %s, watchdog %s%s",
           FW_COMMIT, (int)esp_reset_reason(), (unsigned long)gSetupMs, gImgState,
           gLoopWdtOn ? "on" : "OFF",
           esp_ota_get_last_invalid_partition() ? " - an earlier update was ROLLED BACK" : "");
  gLink.send(MSG_LOG, msg, (uint16_t)strlen(msg));
}

void setup() {
  //  FIRST, so a hang anywhere below - the Bluetooth start included - resets.
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  enableLoopWDT();
  gLoopWdtOn = (esp_task_wdt_status(NULL) == ESP_OK);   // reported in bootReport()
  {
    const esp_partition_t *run = esp_ota_get_running_partition();
    esp_ota_img_states_t st;
    if (run && esp_ota_get_state_partition(run, &st) == ESP_OK) {
      gImgOnTrial = (st == ESP_OTA_IMG_PENDING_VERIFY);
      gImgState = st == ESP_OTA_IMG_PENDING_VERIFY ? "ON TRIAL (rollback armed)"
                : st == ESP_OTA_IMG_VALID          ? "valid"
                : st == ESP_OTA_IMG_NEW            ? "NEW - this bootloader has no rollback"
                : "not tracked (USB flash)";
    } else {
      gImgState = "not tracked (USB flash)";
    }
  }
  Serial.begin(115200);
  delay(300);
  Serial.println();
  Serial.println(F("======================================================"));
  Serial.println(F("  AMBERSONG  -  AUDIO MCU (ESP32-WROOM-32)"));
  Serial.printf ("  firmware %s (%s)\n", FW_VERSION, FW_COMMIT);
    Serial.println(F("======================================================"));

  pinMode(A32_BT_BUTTON, INPUT_PULLUP);      // ACTIVE LOW (Bible §11)
  analogReadResolution(12);
  analogSetPinAttenuation(A32_MODE_ADC,    ADC_11db);
  analogSetPinAttenuation(A32_VOLUME_POT,  ADC_11db);   // 11 dB: the ADC's widest input range

  BtLed::begin();
  settingsLoad();

  Rtc::begin();
  {
    ProtoTime t;
    bool ok = Rtc::read(t);
    Serial.printf("  DS3231 %s", ok ? "responding" : "NOT RESPONDING");
    if (ok) Serial.printf(", time %s, %.2f C",
                          t.valid ? "valid" : "INVALID (lost power)", t.tempC4 / 4.0);
    Serial.println();
  }

  //  A FAILED SELF-TEST NO LONGER HALTS - 2026-09-24.
  //
  //  This line used to be `for(;;) delay(1000);`, BEFORE gLink.begin(). The A32
  //  is updated only through the link (no reset line from the S3, Bible §3)
  //  and, until 2026-09-25, there was no rollback, so an OTA'd image that failed
  //  here was a brick until the cabinet came open. A halt protects nothing that
  //  a mute does not: the self-test is about the FRAMER, and a framer that is
  //  wrong in some corner may still carry an OTA, whereas a halted MCU
  //  certainly cannot.
  //
  //  So the rest of setup() runs as normal - the link comes up, the OTA
  //  receiver with it - but protoBroken holds the audio muted (applyMute) and
  //  the radio non-connectable (applyScanMode) for as long as this build runs,
  //  and loop() says so to the console and to the S3 every 10 s. confirmTick()
  //  never confirms such an image, so on trial a restart rolls it back. No
  //  buffers are added: one bool.
  //
  //  The cheapest failure it guarded - a payload too big for a frame - is now a
  //  compile error (proto.h), so such an image cannot be built at all.
  if (!protoSelfTest(Serial)) {
    protoBroken = true;
    Serial.println(F("  [FAIL] proto self-test - DEGRADED: audio muted, Bluetooth closed,"));
    Serial.println(F("         link and OTA kept up so this build can be replaced."));
  }

  Audio::begin();
  settingsApply();

  //  THE DECISION THAT MAKES THIS FIRMWARE POSSIBLE: A2DP decodes SBC and hands
  //  us PCM, and never touches the I2S driver. BOTH of these are required -
  //  the flag alone leaks i2s_set_clk() through set_sample_rate(). See the
  //  NullA2dpOutput comment at the top of this file.
  //  AND THE VOLUME OBJECT TOO. Same discipline as NullA2dpOutput, same class
  //  of bug, different object.
  //
  //  set_stream_reader(cb, false) does NOT stop the library touching our PCM.
  //  That flag only gates write_audio(); the volume control runs UNGATED and
  //  IN PLACE on the same buffer, one line earlier, on a different object that
  //  the null output cannot reach. If the phone uses AVRCP absolute volume the
  //  library scales our samples for us - at slider position 1/127 that is a
  //  divide by ~2048, and the integer truncation throws away everything below
  //  2048 counts.
  //
  //  A2DPNoVolumeControl overrides update_audio_data() to an empty body, so the
  //  library becomes incapable of touching the audio. AVRCP volume stays
  //  readable through get_volume() as information, and our taper becomes the
  //  only volume law in the machine - which is what the front knob is for.
  a2dp.set_volume_control(&noVol);
  a2dp.set_output(nullOut);
  a2dp.set_stream_reader(a2dpStream, false);
  a2dp.set_sample_rate_callback(onSampleRate);
  a2dp.set_on_connection_state_changed(onConnState);
  a2dp.set_on_audio_state_changed(onAudioState);
  a2dp.set_auto_reconnect(false);            // the radio never chases the phone
  a2dp.start(DEVICE_NAME);

  //  BEFORE applyScanMode(), which is what first lets the radio transmit.
  //  esp_bt.h: call after the controller is enabled and before any RF TX.
  applyBtTxPower();

  //  Start invisible. applyScanMode() opens up only when the rules allow.
  //  The stack-up that a2dp.start() queued has not necessarily run yet; it
  //  no longer writes a mode (PolicyA2dpSink), and scanDirty starts true, so
  //  loop() writes this once more regardless.
  applyScanMode(true);

  gLink.onMessage(onMessage);
  gLink.begin(Serial2, A32_UART_RX, A32_UART_TX);
  Serial.printf("  link on GPIO%d/%d - explicit pins, because Serial2's defaults\n",
                A32_UART_RX, A32_UART_TX);
  Serial.println(F("  are GPIO16/17, and GPIO17 is I2S LRCK."));
  Serial.println();
  Serial.println(F("  keys: + - vol  m mute  b pair  v pot on/off  p c P pot MIN/CTR/MAX  d DC pattern"
                    "  t T taper  r raw stream  s status"));

  uint16_t raw;
  source = readSource(raw);
  Audio::setSource(source);
  applyMute();          //  ESTABLISHES the invariant; asleep until the S3 speaks
  updateBtState();
  gSetupMs = millis();  //  reported at the first handshake - see bootReport()
}

void loop() {
  gLink.poll();
  serviceWake();          //  before anything that depends on being awake
  BtLed::update();
  settingsFlush();
  serviceDisconnect();

  uint32_t now = millis();

  if (now - lastSrcPoll >= 50) { lastSrcPoll = now; updateSource(); }
  //  THE SAME RULE ON THIS SIDE: an update that stops part-way must not leave
  //  the radio muted forever. The S3 could have rebooted, been unplugged, or
  //  simply lost the link mid-transfer.
  if (otaOn && millis() - otaLastMs > 20000) otaGiveUp("the S3 stopped sending");

  confirmTick(now);

  //  A BUILD THAT FAILED ITS SELF-TEST SAYS SO, every 10 s, on both channels -
  //  see setup(). MSG_LOG is printed by the S3 as "[A32] ...", and a message id
  //  it already handles needs no protocol change. A constant string: no buffer.
  if (protoBroken) {
    static uint32_t lastBrokenSay = 0;
    if (now - lastBrokenSay >= 10000) {
      lastBrokenSay = now;
      static const char kBroken[] = "proto self-test FAILED - muted, BT closed, awaiting OTA";
      Serial.printf("  [FAIL] %s\n", kBroken);
      gLink.send(MSG_LOG, kBroken, (uint16_t)(sizeof(kBroken) - 1));
    }
  }

  if (usePot && now - lastPotMs >= 50) { lastPotMs = now; updateVolumePot(); }
  if (potStream && now - lastStreamMs >= 100) {
    lastStreamMs = now;
    uint16_t r = analogRead(A32_VOLUME_POT);
    Serial.printf("  RAW %4u  vol %3u\n", r, potRotation(r));
  }
  updateBtState();

  //  THE STACK CHANGED STATE - re-derive the scan mode and write it, past the
  //  cache. See PolicyA2dpSink. Cleared BEFORE the write, so a change that lands
  //  during it is caught on the next pass rather than lost. After
  //  updateBtState(), so the write sees this pass's btState.
  if (scanDirty) { scanDirty = false; applyScanMode(true); }

  //  Pair button, active LOW (Bible §11). The debounce is here: a press counts
  //  on RELEASE, and only after being held more than 30 ms. BT mode only.
  static bool wasDown = false;
  static uint32_t downAt = 0;
  bool down = (digitalRead(A32_BT_BUTTON) == LOW);
  if (down && !wasDown) downAt = now;
  if (!down && wasDown && now - downAt > 30 && source == SRC_BT) {
    //  Author's decision: the button is for devices that have NOT connected
    //  before, so it drops whoever is on first. Otherwise a second phone can
    //  pair behind the first and it stops being obvious which one is in charge.
    Serial.println(F("  pair button -> LINK (discoverable for new devices)"));
    gracefulDisconnect(BT_LINK);
  }
  wasDown = down;

  if (now - lastStateTx >= PROTO_STATE_MS) {
    lastStateTx = now;
    ProtoState s = {};
    uint16_t raw = 0;
    readSource(raw);
    //  Via locals: a reference cannot bind to a packed struct field, because
    //  the compiler cannot promise the alignment a reference implies.
    bool clipped = false;
    int16_t pl = 0, pr = 0;
    Audio::getPeaks(pl, pr, clipped);
    s.peakLdBx10  = pl;
    s.peakRdBx10  = pr;
    s.source      = source;
    s.btState     = btState;
    s.volume      = cfgA.volume;
    s.muted       = cfgA.muted;
    s.avrcpVolume = a2dp.get_volume();
    s.clipped     = clipped;
    s.underruns   = Audio::btUnderruns();
    s.ringFill    = (uint16_t)Audio::btRingFill();
    s.rawLadder   = raw;
    {
      Audio::ZeroWatch w;
      Audio::zeroWatch(w);
      s.zddArm       = w.arm;
      s.zddRel       = w.rel;
      s.zddSinceRelMs= w.sinceRelMs;
      s.zddLongestMs = w.longestMs;
      s.zddMuted     = w.muted   ? 1 : 0;
      s.zddFloor     = w.floorOn ? 1 : 0;
      s.i2sSticky    = w.i2sSticky;
      s.stalls       = w.stalls;
      s.stallMaxUs   = w.stallMaxUs;
      s.stallLastMs  = w.stallLastMs;
      s.stallLastUs  = w.stallLastUs;
      uint32_t gs, gmin, gmax;
      Audio::gainWatch(gs, gmin, gmax);
      s.gainSteps    = gs;
      s.volQ16Min    = gmin;
      s.volQ16Max    = gmax;
    }
    s.potRaw       = potRaw;
    s.potSeenMin   = (potSeenMin == 0xFFFF) ? 0 : potSeenMin;
    s.potSeenMax   = potSeenMax;
    s.potJumpMax   = potJumpMax;
    s.volSteps     = volSteps;
    s.volJumpMax   = volJumpMax;
    s.volJumpMs    = volJumpMs;
    s.usePot       = usePot ? 1 : 0;
    s.dinDrive     = Audio::dinDriveIs();
    s.sdDelay      = Audio::sdOutDelayIs();
    s.signTail     = Audio::signTailOn() ? 1 : 0;
    s.clkDrive     = Audio::clockDriveIs();
    s.mclkDrive    = Audio::mclkDriveIs();
    s.bckDrive     = Audio::bckDriveIs();
    s.lrckDrive    = Audio::lrckDriveIs();
    {   //  ProtoState is packed: its fields cannot bind to a reference.
        int16_t rl, rr; Audio::getRms(rl, rr); s.rmsLdBx10 = rl; s.rmsRdBx10 = rr; }
    s.testDc       = Audio::testDcOn() ? 1 : 0;
    s.dcWord       = (uint32_t)Audio::dcWordIs();
    s.a32Ms        = millis();
    s.btTx         = btTxMaxSeen;
    s.btTxMin      = btTxMinSeen;
    strncpy(s.peerName, peerName, sizeof(s.peerName) - 1);
    gLink.send(MSG_STATE, s);
  }

  //  ANNOUNCE THE EVENT AS IT HAPPENS, from loop() and never from the audio
  //  task - Serial from a priority-6 real-time task is how you turn a pop
  //  investigation into a dropout investigation. The 250 ms state frame carries
  //  the same numbers to the portal, which is the channel that matters with the
  //  cabinet shut; this line exists for a bench session with a cable.
  //  A pop-hunt witness, kept. The zero-data-mute theory it was built for was
  //  EXCLUDED 2026-09-10 (the release count stayed frozen through 8 minutes of
  //  pops), so the "a pop is predicted HERE" in the text below is stale: the
  //  line only reports that the output left a run of zeros long enough for the
  //  DAC's zero-data mute, as audio.cpp models it.
  {
    static uint32_t seenRel = 0, seenStalls = 0;
    Audio::ZeroWatch w;
    Audio::zeroWatch(w);
    if (w.rel != seenRel) {
      seenRel = w.rel;
      Serial.printf("  [ZDD] DAC left analogue mute (#%lu, held %lu ms)\n",
                    (unsigned long)w.rel, (unsigned long)w.longestMs);
    }
    if (w.stalls != seenStalls) {
      seenStalls = w.stalls;
      Serial.printf("  [ZDD] audio task stalled %lu us (#%lu) - long enough to arm the detector\n",
                    (unsigned long)w.stallMaxUs, (unsigned long)w.stalls);
    }
  }

  //  Local console, so the A32 remains testable on its own bench cable without
  //  the S3 or the portal being involved.
  if (Serial.available()) {
    char c = Serial.read();
    switch (c) {
      case '+': case '=':
        cfgA.volume = (cfgA.volume > 245) ? 255 : cfgA.volume + 10;
        cfgA.muted = 0; settingsApply(); settingsTouch();
        Serial.printf("  volume %u\n", cfgA.volume); break;
      case '-': case '_':
        cfgA.volume = (cfgA.volume < 10) ? 0 : cfgA.volume - 10;
        settingsApply(); settingsTouch();
        Serial.printf("  volume %u\n", cfgA.volume); break;
      case 'm': case 'M':
        cfgA.muted = !cfgA.muted; settingsApply(); settingsTouch();
        Serial.printf("  %s\n", cfgA.muted ? "MUTED" : "unmuted"); break;
      case 'b': case 'B':
        if (source == SRC_BT) gracefulDisconnect(BT_LINK);
        else Serial.println(F("  not in BT mode")); break;
      //  CYCLE THE CONSTANT-DC WORD - a pop-hunt diagnostic (2026-09), kept.
      //
      //  Each word is a constant, so the DAC output is DC and silent through
      //  AC coupling; what differs is how DIN (A32_PCM5102_DIN) switches on the
      //  wire. The word only goes out while the DC test is on (MSG_TEST_DC).
      //  kDc[] is at file scope; sys.dcword picks the same entries by index.
      //
      //    index  word        dBFS    what it is for
      //    0      0xFF000000  -42.1   sign SET: DIN moves at every slot boundary.
      //                               The default in audio.cpp. Popped.
      //    1      0x00FF0000  -42.2   same magnitude, sign clear: no boundary
      //                               transition. Silent.
      //    2      0xFFFF0000  -90.3   sign set, 48 dB quieter
      //    3      0x00FFFFFF  -42.1   bits 0..23 set: the stimulus for the 'k'
      //                               mask sweep (lowest set bit = mask width)
      //    4      0x000000FF -138.5   bits on the wire below anything the DAC
      //                               resolves: the wire, not the value
      //    5      0x00100000  -66.2   first-round reference, silent
      //    6      0x001FFFFF  -60.2   first-round reference, popped
      //
      //  Popped / silent: what was heard during the pop hunt, before clock-pad
      //  drive 3 became the boot default. The key starts at index 0 (the
      //  default), so the first press selects index 1.
      case 'd': case 'D': {
        //  Index 0 and 1 are the sign-bit A/B, one keypress apart. With a
        //  constant, the slot ends on bit 0 and the next opens on bit 31 of the
        //  same value; real audio opens each slot with the sign bit of the NEXT
        //  sample, which flips constantly - so masking low bits cannot keep DIN
        //  still across the boundary (the 'k' sweep fixed nothing on RADIO).
        //  Caveat for index 4: if the DAC discards the low byte it may see
        //  digital zero and auto-mute - silence for an unrelated reason.
        static uint8_t di = 0;
        di = (uint8_t)((di + 1) % 7);
        Audio::setDcWord(kDc[di]);
        //  Transition count on the wire, both edges, MSB-first within the word.
        uint32_t w = (uint32_t)kDc[di], e = 0, pc = 0;
        for (int b = 31; b > 0; b--) if (((w >> b) ^ (w >> (b - 1))) & 1u) e++;
        for (int b = 0; b < 32; b++) if ((w >> b) & 1u) pc++;
        int low = -1;
        for (int b = 0; b < 32; b++) if ((w >> b) & 1u) { low = b; break; }
        Serial.printf("  DC word 0x%08lX   %.1f dBFS   lowest set bit %d   sign %s"
                      "   %lu edges  %lu bits set%s\n",
                      (unsigned long)w,
                      20.0 * log10(fabs((double)kDc[di]) / 2147483648.0),
                      low, (w & 0x80000000u) ? "SET -> boundary transition"
                                             : "clear",
                      (unsigned long)e, (unsigned long)pc,
                      Audio::testDcOn() ? "   (DC test is ON)"
                                        : "   (DC test is off)");
        break;
      }
      //  SWEEP THE OUTPUT MASK: force the low 0, 1, 2, 4, 6, 8, 12 or 16 bits
      //  of every output word to zero. A pop-hunt experiment, kept; never
      //  persisted, a reboot is back to no mask.
      //
      //  With the DC word at 0x00FFFFFF (index 3) the masked word's lowest set
      //  bit IS the mask width. It was a candidate fix and is not one: masking
      //  cannot remove the slot-boundary transition that the sign bit makes
      //  (see the 'd' case). Every masked bit above the ones the DAC ignores
      //  costs real resolution, and the mask applies to real audio too.
      case 'k': case 'K': {
        static const uint8_t kMask[8] = { 0, 1, 2, 4, 6, 8, 12, 16 };
        static uint8_t mi = 0;
        mi = (uint8_t)((mi + 1) % 8);
        Audio::setOutMaskBits(kMask[mi]);
        uint32_t masked = (uint32_t)Audio::dcWordIs() & (uint32_t)Audio::outMaskIs();
        int low = -1;
        for (int b = 0; b < 32; b++) if ((masked >> b) & 1u) { low = b; break; }
        Serial.printf("  out mask %2u bits  -> 0x%08lX   DC word becomes 0x%08lX"
                      "   lowest set bit %d\n",
                      kMask[mi], (unsigned long)(uint32_t)Audio::outMaskIs(),
                      (unsigned long)masked, low);
        break;
      }
      //  DIN pad drive, 0 weakest .. 3 strongest; the chip's default 2 is what
      //  boot leaves it at. Aggressor only. Not persisted.
      case 'g': case 'G': {
        uint8_t d = (uint8_t)((Audio::dinDriveIs() + 1) & 3);
        Audio::setDinDrive(d);
        Serial.printf("  DIN drive -> %u  (readback %u)   0=~5mA 2=default 3=~40mA\n",
                      d, Audio::dinDriveIs());
        break;
      }
      //  'y' (tx_sd_out_delay) was removed 2026-09-25: position 2 had sent the
      //  radio to FULL VOLUME, and the author ruled the key out. Only the
      //  readback remains, in the state frame and the status line.
      case 'p': calibratePot(0); break;   // knob fully anticlockwise first
      case 'P': calibratePot(1); break;   // knob fully clockwise first
      case 'c': calibratePot(2); break;   // knob at its mechanical centre
      case 't': case 'T':
        cfgA.taperX10 = (c == 'T') ? (cfgA.taperX10 >= 40 ? 40 : cfgA.taperX10 + 1)
                                   : (cfgA.taperX10 <= 10 ? 10 : cfgA.taperX10 - 1);
        settingsApply(); settingsTouch();
        Serial.printf("  taper gamma %.1f   (half travel = %.1f dB)\n",
                      cfgA.taperX10 / 10.0,
                      20.0 * log10(pow(0.5, cfgA.taperX10 / 10.0)));
        break;
      case 'r': case 'R':
        potStream = !potStream;
        Serial.println(potStream ? F("  pot raw stream ON") : F("  pot raw stream off"));
        break;
      case 'v': case 'V':
        usePot = !usePot;
        Serial.print(F("  front volume pot "));
        Serial.println(usePot ? F("ACTIVE") : F("ignored (console controls volume)"));
        break;
      case 's': case 'S': {
        int16_t l, r; bool cl;
        Audio::getPeaks(l, r, cl);
        Serial.printf("  pot raw %4u -> vol %3u   cal %u..%u..%u   taper %.1f\n",
                      potRaw, potRotation(potRaw),
                      potRawMin, potRawMid, potRawMax, cfgA.taperX10 / 10.0);
        //  I2S0.int_raw, DECODED FROM THE REGISTER MAP - not from memory.
        //  soc/esp32/include/soc/i2s_struct.h, the int_raw union, packs from
        //  the LSB:  0 rx_take_data  1 tx_put_data  2 rx_wfull  3 rx_rempty
        //  4 tx_wfull  5 tx_rempty  6 rx_hung  7 tx_hung  8 in_done
        //  9 in_suc_eof  10 in_err_eof  11 out_done  12 out_eof
        //  13 in_dscr_err  14 out_dscr_err  15 in_dscr_empty  16 out_total_eof
        //
        //  The first version of this line claimed bit0 = tx_wfull, bit1 =
        //  tx_rempty, bit12 = tx_hung. All three were wrong, and the effect was
        //  to make the peripheral working normally look like a fault report:
        //  the steady-state 0x90A is tx_put_data | rx_rempty | in_done |
        //  out_done, every one of which latches once per DMA buffer by design.
        //
        //  rx_wfull and rx_rempty are EXPECTED here and are not trouble: on BT
        //  and AUX nothing ever calls i2s_read, so the RX chain free-runs and
        //  overflows by construction. That is understood and harmless.
        {
          static const char *kBit[17] = {
            "rx_take_data", "tx_put_data", "rx_wfull",  "rx_rempty",
            "tx_wfull",     "tx_rempty",   "rx_hung",   "tx_hung",
            "in_done",      "in_suc_eof",  "in_err_eof","out_done",
            "out_eof",      "in_dscr_err", "out_dscr_err",
            "in_dscr_empty","out_total_eof" };
          //  The bits that actually mean something is wrong with the TX path.
          const uint32_t TROUBLE = (1u << 4) | (1u << 5) | (1u << 6) |
                                   (1u << 7) | (1u << 10) | (1u << 13) |
                                   (1u << 14);
          uint32_t f = Audio::i2sFaults();
          Serial.printf("  i2s int_raw 0x%03lX", (unsigned long)f);
          if (!(f & TROUBLE)) {
            Serial.println(F("   no trouble bits"));
          } else {
            Serial.print(F("   TROUBLE:"));
            for (int b = 0; b <= 16; b++)
              if ((f & TROUBLE) & (1u << b)) { Serial.print(' '); Serial.print(kBit[b]); }
            Serial.println();
          }
        }
        {
          //  EVERY EXPERIMENT KNOB, READ BACK - DIN drive and sd delay from the
          //  hardware registers. Cycling a setting just to find out what it is
          //  cost two wasted rounds during the pop hunt, and once nearly turned
          //  a baseline into a false positive.
          uint32_t mk = (uint32_t)Audio::outMaskIs();
          int mb = 0; while (mb < 32 && !((mk >> mb) & 1u)) mb++;
          //  THE LINK, AND WHETHER WE THINK WE ARE AWAKE.
          //  Both became load-bearing when sleep started gating audio and
          //  Bluetooth on peerAlive(), and neither was visible from here - so
          //  a dead link looked exactly like a dead A32 from the console.
          Serial.printf("  link %s  rx %lu  awake %s  (amp %s, peer %s)\n",
                        gLink.peerAlive() ? "UP     " : "SILENT ",
                        (unsigned long)gLink.rxCount(),
                        awake ? "yes" : "NO ",
                        ampOn ? "on" : "off",
                        gLink.peerAlive() ? "alive" : "quiet");
          Serial.printf("  dc 0x%08lX %s   mask %d bits   DIN drive %u"
                        "   sd delay %u\n",
                        (unsigned long)(uint32_t)Audio::dcWordIs(),
                        Audio::testDcOn() ? "ON " : "off",
                        mb, Audio::dinDriveIs(), Audio::sdOutDelayIs());
        }
        Serial.printf("  src %d  bt %d  vol %u %s  peak %.1f/%.1f dBFS  ring %lu  under %lu  heap %u\n",
                      source, btState, cfgA.volume, cfgA.muted ? "MUTE" : "",
                      l / 10.0, r / 10.0,
                      (unsigned long)Audio::btRingFill(),
                      (unsigned long)Audio::btUnderruns(), ESP.getFreeHeap());
        break;
      }
      default: break;
    }
  }
}
