// ============================================================================
//  AMBERSONG  -  MAIN MCU (ESP32-S3)
//
//  Clock, needle, panel lighting, tuning, amp sensing, the link to the audio
//  MCU, WiFi, NTP and the web portal. The portal only sets values that already
//  exist (the settings table) rather than reaching into behaviour.
//
//  CORE SPLIT (Firmware Gospel §4):
//    core 1  display ISR, needle task, RDA task, this loop
//    core 0  WiFi, the portal task and the needle's step emitter
//
//  TIME LIVES ON THE OTHER MCU. The battery clock is on the A32 (Bible §5) and
//  the display is here, so the clock crosses the UART. The S3 keeps its own
//  software clock disciplined by what the A32 reports (and by NTP when WiFi is
//  up), so a slow or missing link never makes the display stutter - it just
//  stops being corrected.
//
//  WHAT "POWER ON" MEANS FOR THE NEEDLE
//  S3 boot is not necessarily the moment a person turns the radio on. The boot
//  home always ends with a full sweep; `sweepOn` adds another full sweep on
//  each amp-on edge (the front toggle), which is when someone is actually
//  watching. Adjustable, because it is a matter of taste.
// ============================================================================

#include <Arduino.h>
#include <Preferences.h>
#include <time.h>
#include <rom/crc.h>
#include "pins.h"
#include "console.h"
#include "link.h"
#include "display.h"
#include "needle.h"
#include "rda.h"
#include <esp_phy_init.h>
#include <esp_wifi.h>
#include "panel.h"
#include "drive.h"
#include "net.h"
#include "portal.h"
#include "settings_api.h"
#include <esp_ota_ops.h>
#include <esp_task_wdt.h>

#ifndef FW_VERSION
#define FW_VERSION "v.0.unstamped"
#endif
#ifndef FW_COMMIT
#define FW_COMMIT "nogit"
#endif

// ---------------------------------------------------------------------------
//  SETTINGS. One struct, stored whole, with a magic and a version so an older
//  layout is migrated (below) rather than reinterpreted. Written to NVS on a
//  DEBOUNCE: flash traffic is the one load that disturbs the display (measured
//  in phase G), and a portal slider dragged across its range would otherwise
//  write on every pixel.
// ---------------------------------------------------------------------------
//  VERSIONED AND MIGRATED, because the previous scheme silently threw away
//  everything on any layout change: the blob was rejected on a size mismatch
//  and the defaults quietly took over. That is right for a bench rig and wrong
//  for a machine somebody has spent an evening calibrating.
//
//  Rules from here on:
//    - NEVER reorder or remove an existing field. Append only.
//    - Bump SETTINGS_VERSION when you append, and add a migration case.
//    - A blob from an older version is COPIED FIELD BY FIELD into the current
//      struct, so old values survive and new fields take their defaults.
//  The soft-limit pair a machine gets before it has been calibrated, and the
//  pair a corrupt stored one is purged to. One number, three uses.
//
//  IT HAS A LOWER BOUND, AND IT IS NOT ABOUT SAFETY. After a purge the
//  post-home SWEEP runs both legs at +/-this, which traverses zero and
//  therefore the index band. That crossing is what satisfies captureLimit()'s
//  "a crossing since the home" (gMeasAtHome is taken when homing finishes), so
//  capturing the soft limits depends on it. Tune this DOWN below about half
//  the band width and homing still succeeds while no stop can ever be
//  captured. 300 clears the half-band several times over and is still far
//  inside the travel.
static const int32_t PROVISIONAL_LIMIT_HS = 300;

static const uint16_t SETTINGS_MAGIC   = 0xA838;   // settings-file magic number
static const uint16_t SETTINGS_VERSION = 7;

//  HOW MANY (angle, frequency) SAMPLES the tuning curve can hold.
//  Three are set by hand - marks A/B/C, a typed frequency at the dial's
//  current position - and the rest are there for the RDA5807M
//  auto-calibration (Firmware Gospel §6), which reads the tube
//  set's own local oscillator and can therefore produce a sample at ANY dial
//  position without a station to identify. The manual marks take slots 0-2 so
//  an automatic sample can never overwrite something the author set by hand.
static const uint8_t TUNE_MARKS = 12;

//  HOW MANY FIXED FEATURES the settings layout can hold. A SETTINGS constant,
//  deliberately NOT Rda::MAX_SPURS: sizing the struct from a module's tuning
//  knob means somebody raising that knob later changes what a version-6 blob
//  MEANS without changing its version, which is the one thing the whole scheme
//  forbids. Frozen at 16 with room to spare - this cabinet needs seven - so
//  growth needs no migration; if it ever must grow, APPEND a second array at
//  a new version rather than widening this one. The static_assert below is
//  the guard.
static const uint8_t CFG_SPURS = 16;
static_assert(Rda::MAX_SPURS == CFG_SPURS,
              "the RDA's fixed-feature list and the settings' must be the same size, or learned features are dropped");

//  tunerEndsSet: bit 0 = the low end was measured, bit 1 = the high end was.
//  Only BOTH means the travel is known.
static const uint8_t ENDS_LOW  = 1;
static const uint8_t ENDS_HIGH = 2;
static const uint8_t ENDS_BOTH = 3;

//  Version 1 is the layout that was in the machine on 2026-08-31, the one that
//  holds the first real needle calibration. It is kept verbatim so that
//  calibration migrates rather than being lost.
struct SettingsV1 {
  uint8_t  brightOn, brightOff, hour12, blankLeadZero;
  uint16_t dispDwellMs, dispFadeMs;
  uint8_t  panelTuning, panelIdle, panelOther;
  uint16_t panelIdleMs, panelFadeMs, panelDwellMs;
  uint16_t upVmax, upAccel, dnVmax, dnAccel;
  float    wn, zeta;
  uint16_t riseMs, fallMs, dwellUpMs, dwellDnMs;
  uint8_t  microFast, microSlow;
  uint16_t seekHsps, reapHsps;
  int32_t  posMin, posMax, idxOnFwd, idxOffFwd, idxOnRev, idxOffRev;
  uint8_t  sweepOn;
  int32_t  calLow, calHigh, lastAngle;
  uint8_t  showTuning;
  uint16_t bandLow, bandHigh;
};

//  Version 2 is the layout that shipped the web portal. Kept verbatim for the
//  same reason V1 is: the calibration in it is an evening of somebody's work.
struct SettingsV2 {
  uint16_t magic, version;
  uint8_t  brightOn, brightOff, hour12, blankLeadZero;
  uint16_t dispDwellMs, dispFadeMs;
  uint8_t  panelTuning, panelIdle, panelOther;
  uint16_t panelIdleMs, panelFadeMs, panelDwellMs;
  uint16_t upVmax, upAccel, dnVmax, dnAccel;
  float    wn, zeta;
  uint16_t riseMs, fallMs, dwellUpMs, dwellDnMs;
  uint8_t  microFast, microSlow;
  uint16_t reapHsps;
  int32_t  posMin, posMax, idxOnFwd, idxOffFwd, idxOnRev, idxOffRev;
  uint8_t  sweepOn;
  int32_t  calLow, calHigh, lastAngle;
  uint8_t  showTuning;
  uint16_t bandLow, bandHigh;
};

struct Settings {
  uint16_t magic   = SETTINGS_MAGIC;
  uint16_t version = SETTINGS_VERSION;
  uint8_t  brightOn      = 255;   // clock brightness, amp powered
  uint8_t  brightOff     = 110;   // clock brightness, amp off. Not too low:
                                  // the firmware has no colon pin, so the colon
                                  // cannot dim with the digits and stands out
                                  // more the further down these go.
  uint8_t  hour12        = 0;
  uint8_t  blankLeadZero = 1;
  uint16_t dispDwellMs   = 250;   // stillness before the brightness moves
  uint16_t dispFadeMs    = 1500;  // eased travel between the two levels

  uint8_t  panelTuning   = 255;
  uint8_t  panelIdle     = 128;
  uint8_t  panelOther    = 96;
  uint16_t panelIdleMs   = 5000;
  uint16_t panelFadeMs   = 900;   // eased travel, not a step
  uint16_t panelDwellMs  = 250;   // stillness first, so it decides rather than reacts

  //  NEEDLE MOTION - THE VALIDATED CONFIGURATION (Firmware Gospel §5).
  //  These took a whole session and four firmware bugs to find. Do not re-tune
  //  them by eye.
  //  1100, NOT 1700 - MEASURED BY THE AUTHOR, 2026-09-24. Faster than this, moving
  //  UP through the index, the needle stops following while the firmware keeps
  //  counting (1500 failed every time, 1200-1400 often, 1100 never), and every
  //  power-up sweep landed ~400 half-steps low. The band check in needleTask now
  //  catches it if it ever happens again.
  uint16_t upVmax        = 1100;  // half-steps/s, every second-order move (either direction)
  uint16_t upAccel       = 18000; // half-steps/s^2, same moves
  uint16_t dnVmax        = 1700;  // the decay (the fall, the park) gets its own limits on purpose
  uint16_t dnAccel       = 18000; // only enters the park's arrival test
  float    wn            = 9.0f;  // ring-down knob. Ceiling is 4*zeta*a/v = 32.7 at these defaults
  float    zeta          = 0.50f; // overshoot knob. ~5 deg, ~300 ms tail
  uint16_t riseMs        = 100;   // decay envelope, the fall
  uint16_t fallMs        = 1000;
  uint16_t dwellUpMs     = 100;   // hesitation before every commanded second-order move (not tracking)
  uint16_t dwellDnMs     = 1000;  // a full second before the fall
  uint8_t  microFast     = 16;    // sweeps, parks, homing. 1/32 clamps to 92% of commanded rate
  uint8_t  microSlow     = 32;    // while tracking the tuner - smoother where smoothness shows
  uint16_t reapHsps      = 60;    // band-calibration creep and the portal jog (not the homing re-approach)

  //  SOFT LIMITS, half-steps either side of the index. These and the search
  //  budget are all that keep the needle on the dial now.
  //  PROVISIONAL, and deliberately narrow (PROVISIONAL_LIMIT_HS, above). The
  //  old +/-967 default was an arc estimate that ran well past the measured high
  //  limit. A default with no end-stop switches must be a LOWER bound on the
  //  travel: too narrow is useless, too wide grinds. Homing does not depend on
  //  this pair (its budgets are constants).
  int32_t  posMin        = -PROVISIONAL_LIMIT_HS;
  int32_t  posMax        =  PROVISIONAL_LIMIT_HS;
  int32_t  idxOnFwd      = 0;     // the homing edge, by definition zero
  int32_t  idxOffFwd     = 0;
  int32_t  idxOnRev      = 0;
  int32_t  idxOffRev     = 0;

  uint8_t  sweepOn       = 0;     // nonzero = a full sweep on each amp-on edge (the boot home always sweeps)

  //  ACCUMULATED AS5600 counts, not raw (turns are counted across the wrap).
  //  The defaults are placeholders until the tuner's ends are measured - see
  //  tunerEndsSet.
  int32_t  calLow        = 0;
  int32_t  calHigh       = 6023;
  int32_t  lastAngle     = 0;     // so the turn can be recovered at boot

  //  0 = the clock, always. 1 = the tuning readout, always.
  //  2 = the readout WHILE TUNING, then back to the clock - the same idea as
  //      the panel lights coming up under your hand and settling afterwards.
  //      Author's request, 2026-08-31.
  uint8_t  showTuning    = 0;
  uint16_t bandLow       = 881;   // 88.1 MHz, shown as "881"
  uint16_t bandHigh      = 1079;  // 107.9 MHz

  //  How long the readout stays after the last movement of the dial, in mode 2.
  uint16_t tuneHoldMs    = 4000;
  //  APPENDED AT V4. What is PRINTED at the needle's two stops, in tenths of
  //  a MHz. Separate from bandLow/bandHigh, which describe how far the tuning
  //  CAPACITOR travels - on this set that is wider than the printed face.
  uint16_t dialLow       = 879;   // 87.9 MHz printed at the low stop
  uint16_t dialHigh      = 1079;  // 107.9 MHz printed at the high stop

  //  APPENDED AT V5. THE MAPPING IS A CURVE, NOT A LINE.
  //
  //  Measured on this set 2026-09-04 from three stations the author verified by
  //  ear: the local slope is 0.251 tenths of a MHz per permille between 91.3
  //  and 98.5, and 0.288 between 98.5 and 107.3 - a 15% change. A straight
  //  line fitted to the two ends is exact at the ends and half a megahertz out
  //  in the middle, which is precisely what happened: the two-point Mark A/B
  //  looked right, mid-band read high, correcting mid-band broke the ends.
  //
  //  SAMPLES ARE STORED, NOT COEFFICIENTS. They are self-describing, they can
  //  be re-fitted with a better model later without another migration, and no
  //  float ever goes into flash. The fit is redone on every applySettings().
  int16_t  tuneOffset10  = 0;     // slides the whole curve, tenths of a MHz
  uint16_t tuneUsed      = 0;     // bit i: slot i holds a sample
  //  RAW ACCUMULATOR COUNTS, not permille. permille is derived from
  //  calLow/calHigh, so a point stored in it stops meaning the same thing the
  //  moment the tuner ends are re-measured. The accumulator is the shaft.
  int32_t  tuneP[TUNE_MARKS] = {0};   // shaft position
  uint16_t tuneF[TUNE_MARKS] = {0};   // frequency there, tenths of a MHz

  //  HAVE THE TUNER'S MECHANICAL ENDS ACTUALLY BEEN MEASURED?
  //
  //  calLow/calHigh default to 0..6023 - a stale guess of well over a turn,
  //  against this tuner's real travel of about 2300 counts, inside one turn
  //  (see seatTurn()). Without knowing which is a measurement and which
  //  is a placeholder, fitTuneCurve() cannot tell "the shaft travels this far"
  //  from "nobody has said yet", and a fit stretched over the placeholder puts
  //  its own vertex inside the domain and refuses three perfectly good marks.
  //
  //  The previous attempt inferred it - bounding the domain to twice the marks'
  //  span - and that proxy cut INSIDE genuinely measured travel whenever the
  //  marks were clustered, freezing the end of the dial with every guard
  //  passing, because the endpoint and span tests are evaluated on the domain
  //  AFTER it has been cut. A fact this cheap to record should not be guessed.
  //  TWO BITS, one per end - bit 0 low, bit 1 high - because it records TWO
  //  measurements and a single bit let one of them speak for both. Typing the
  //  measured low end into the Needle tab, or pressing only "Tuner = low end",
  //  declared the PAIR measured while the high end was still the 6023
  //  placeholder; the fit then took the measured branch over a 7000-count
  //  domain against a far shorter real travel, and either refused three good marks or
  //  quietly demoted the quadratic to the very straight line this feature
  //  exists to replace.
  uint8_t  tunerEndsSet  = 0;

  //  ---- V6 ------------------------------------------------------------------

  //  WIFI TRANSMIT POWER, quarter-dBm. 8 is 2.0 dBm, 80 is 20.0 dBm.
  //
  //  NOT A SETTING ANYBODY TYPES - it is LEARNED. net.cpp walks a ladder on
  //  every failed join and whichever rung associates is written here, so this
  //  field is a record of what worked rather than an instruction. It exists in
  //  the struct only so the next boot starts where the last one succeeded
  //  instead of re-walking the ladder every power cut. Default is the bottom
  //  rung (2.0 dBm), where the ladder starts; net.cpp caps it at 60 (15 dBm)
  //  and its note on TX_LADDER says why.
  uint8_t  wifiTxQ       = 8;

  //  THE RDA5807M'S FIXED FEATURES - self-interference this cabinet produces,
  //  which must be excluded when hunting the tube set's oscillator. Learned by
  //  sweeping with the set switched OFF, where every peak is a spur by
  //  definition. Seven of them here, a comb spaced 2.7-2.8 MHz.
  //
  //  DEFAULTS EMPTY, and the measured comb is deliberately NOT compiled in as a
  //  table: they belong to this cabinet at this antenna position. Baked in, a
  //  freshly-erased machine would claim to have learned features it never
  //  measured, and isSpur()'s +/-0.2 MHz notch would blind the search at seven
  //  places where a real oscillator might sit. The list is only ever earned.
  uint8_t  spurUsed      = 0;         // how many of spur[] are real
  int16_t  spur[CFG_SPURS] = {0};     // tenths of a MHz, LO frequencies

  //  THE TUBE SET'S INTERMEDIATE FREQUENCY, in TWENTIETHS of a MHz - 50 kHz
  //  units, which is the resolution the RDA's fine pass produces and the reason
  //  tenths will not do: 10.65 is not expressible in tenths and was one of the
  //  six readings. The nominal 10.7 is 214.
  //
  //  WHY IT IS A SETTING AND NOT A CONSTANT. The set's IF is 10.6 MHz, not the
  //  nominal 10.7 (Bible §15, §22). The RDA found it first, 2026-09-22: across
  //  six stations the author named by ear - 88.5, 96.9, 98.5, 100.7, 105.7,
  //  107.3 - the implied IF came back 10.60 five times and 10.65 once, never
  //  10.70, while the broadcasts it received directly all landed on valid
  //  odd-tenth channels, so the RDA's own scale is not the cause. The IF can
  //  move again whenever the set is realigned, and it must be correctable
  //  without a flash.
  int16_t  ifOffset20    = 212;       // 10.60 MHz (Bible §22)
};


static Settings   cfg;

//  The IF in TENTHS, for the places that predict and print in tenths. The fine
//  pass and sampleStore work in cfg.ifOffset20 directly - rounding to tenths
//  before adding the IF throws away exactly the half-bin the fine pass exists
//  to resolve.
static inline int16_t ifOff10() { return (int16_t)((cfg.ifOffset20 + 1) / 2); }
static Preferences prefs;
//  "UNSAVED" IS DERIVED, NOT STORED. Every change bumps
//  gTouchGen; a successful write records the count it copied in gSavedGen. The
//  settings are unsaved exactly while the two differ - so a change that lands
//  during a write, on either core, can never be marked saved by it. A stored
//  flag cleared after the write still lost changes in a window of a few
//  instructions, and wider whenever the writer was preempted there.
static volatile uint32_t gTouchGen = 0, gSavedGen = 0;
static inline bool settingsDirty() {
  return __atomic_load_n(&gTouchGen, __ATOMIC_SEQ_CST) != __atomic_load_n(&gSavedGen, __ATOMIC_SEQ_CST);
}
static uint32_t   dirtyAt = 0;

static Link       gLink;
static ProtoState a32;

//  The A32 owns its own settings and persists them; this is the S3's MIRROR,
//  refreshed at every handshake. The portal edits this copy and pushes it - so
//  a peer that rebooted brings ITS values back, rather than ours overwriting
//  whatever it actually came up with.
static ProtoCfgAll a32cfg;
static bool        haveCfg = false;
bool a32CfgKnown() { return haveCfg; }
static bool       haveState = false;
static bool       peerHello = false;
static bool       ampOn     = false;
static bool       ampKnown  = false;

//  Software clock, disciplined by the A32's DS3231.
static uint32_t   epochAtSync = 0;
static uint32_t   millisAtSync = 0;
static bool       timeValid = false;
//  THE BATTERY CLOCK'S HEALTH, for the portal: a DS3231
//  that lost its time means its battery or the module is failing, and that
//  must be visible without a cable. 0 = fine or not asked yet, 1 = it answered
//  with no valid time (oscillator-stop flag set, status unread, or an
//  implausible date), 2 = it did not answer at all.
static uint8_t    gRtcBad   = 0;
static uint32_t   gTimeAsked = 0;   // millis of an unanswered MSG_GET_TIME, 0 = none

// ---------------------------------------------------------------------------
//  EASED DISPLAY BRIGHTNESS
//  The author's brief: powering down should not snap the clock to its standby
//  level, it should let go like something with a reservoir behind it.
//
//  Same shape as the panel - a dwell, then smoothstep, which has zero slope at
//  both ends. A linear ramp between two brightnesses reads as a setting being
//  changed; this reads as the machine winding down.
//
//  A change of mind mid-fade restarts FROM THE CURRENT LEVEL, so flicking the
//  amp off and straight back on never makes the display jump.
// ---------------------------------------------------------------------------
//  0 normal, 1 all eights, 2 all blank. A display you can command is the
//  difference between "the clock is broken" and "the clock has no time yet".
static uint8_t  dispTest = 0;
static uint8_t  brFrom = 255, brTo = 255, brCur = 255;
static uint32_t brStart = 0;

static void brightnessTarget(uint8_t t) {
  if (t == brTo) return;
  brFrom  = brCur;
  brTo    = t;
  brStart = millis();
}

static void brightnessNow(uint8_t v) {
  brFrom = brTo = brCur = v;
  Display::setBrightness(v);
}

static void updateBrightness() {
  if (brCur == brTo) return;
  uint32_t el = millis() - brStart;
  if (el < cfg.dispDwellMs) return;
  float p = (float)(el - cfg.dispDwellMs) / (float)(cfg.dispFadeMs ? cfg.dispFadeMs : 1);
  float k = (p <= 0) ? 0.0f : (p >= 1.0f) ? 1.0f : p * p * (3.0f - 2.0f * p);
  int v = (int)brFrom + (int)((float)((int)brTo - (int)brFrom) * k);
  brCur = (uint8_t)constrain(v, 0, 255);
  Display::setBrightness(brCur);
}

//  A REFUSAL IS A FLAG, NOT A WORD THE BROWSER HAS TO RECOGNISE.
//  Defined here rather than in settings_table.h because captureLimit(), just
//  below, is written long before that header is included - and its refusals are
//  the ones that matter most, since they guard the soft limits on a mechanism
//  with no end-stop switches.
static bool gActionRefused = false;
static const char *refuse(const char *m) { gActionRefused = true; return m; }

static void settingsTouch();          //  defined below, needed by the purge
static bool gSettingsLocked = false;  //  see settingsWrite()
static void applySettings();          //  and by captureLimit() just under it
//  Defined with the RDA sampler, far below, but doAction() lives in
//  settings_table.h which is included long before that - same reason
//  applySettings() is declared here.
static const char *rdaSampleAction();
static const char *tuneDropAction(int slot);

//  ONE COPY OF THE LIMIT-CAPTURE RULE. It existed twice - the portal action and
//  the console m/M - and only the portal copy was guarded, which is the same
//  shape as the settingBound history: "it was written twice before, and the
//  second copy went to the file path only". The author will be on the console.
//
//  Refuses unless the position means something: homed, not mid-calibration,
//  the band actually measured (without it every REVERSE crossing reports the
//  sensor's direction offset as error), and a crossing since the home.
static const char *captureLimit(bool low) {
  if (!Needle::homed())          return refuse("not homed - press H first, the position means nothing yet");
  if (Needle::calBusy())         return refuse("a calibration is running - let it finish");
  if (!Needle::bandCalibrated()) return refuse("run the index band calibration (k) first - the reverse edge is unknown");
  //  NAME AN ACTION THAT WORKS. A jog does NOT satisfy this: jogRaw() sets
  //  gTestMode and the supervisor returns before the crossing block, so gMeas
  //  never moves however many times the needle is walked over the sensor.
  if (!Needle::driftMeasured())
    return refuse("the index has not been crossed under power since the home - run a sweep (r) or let it track; a jog does not count");
  //  ASK WHETHER THE NEEDLE IS STILL OUT, NOT WHETHER IT EVER WAS.
  //  This tested |lastDrift()| > 12, and lastDrift() is never cleared once a
  //  crossing has been absorbed into gPos - so it refused on a discrepancy the
  //  machine had already corrected, and told the author to re-home, which sends
  //  the needle through the index again and re-measures the same number. No
  //  exit; hit at the bench on 2026-09-03. The 12 also contradicted
  //  CORRECT_MAX_HS = 40: everything from 13 to 40 was silently corrected by
  //  the supervisor and then declared untrustworthy here.
  if (Needle::driftPending()) {
    static char w[112];
    snprintf(w, sizeof(w), "REFUSED: the needle slipped %ld half-steps, too far to absorb - re-home (H)",
             (long)Needle::lastDrift());
    return refuse(w);
  }
  int32_t want = Needle::position();
  if (low) cfg.posMin = want; else cfg.posMax = want;
  applySettings(); settingsTouch();
  //  applySettings() writes back whatever setGeometry() actually accepted, so
  //  a refusal shows up here as cfg not holding what we just put in it.
  if ((low ? cfg.posMin : cfg.posMax) != want)
    return refuse("REFUSED: that pair would not bracket the index at 0 (or is crossed) - see console");
  return low ? "low soft limit set here" : "high soft limit set here";
}

//  Runs ONCE, right after settingsLoad(). It used to live inside
//  applySettings(), which runs dozens of times a session including from
//  settingSet() - so fixing posMax in the portal while posMin was still bad
//  fired the purge mid-edit and overwrote what had just been typed.
//  setGeometry() already refuses a bad pair arriving live; this only has to
//  stop a corrupt pair from being carried OUT OF FLASH.
static void purgeCorruptLimits() {
  //  PURGE A CORRUPT PAIR RATHER THAN CARRY IT. See setGeometry(): the index
  //  defines 0 and lies inside the travel, so limits that do not bracket zero
  //  are proof the frame was shifted when they were captured. Carrying them
  //  forward is how one bad calibration poisoned every later one.
  if (cfg.posMin > 0 || cfg.posMax < 0 || cfg.posMin >= cfg.posMax) {
    Con.printf("  [WARN] stored soft limits %ld..%ld do not bracket the index\n",
                  (long)cfg.posMin, (long)cfg.posMax);
    Con.println(F("         at 0. They cannot be real. PURGED - back to safe"));
    Con.println(F("         defaults. Re-home, run the index band calibration,"));
    Con.println(F("         then set the stops by hand."));
    //  NARROW ON PURPOSE - the same +/-PROVISIONAL_LIMIT_HS the struct
    //  defaults use.
    //
    //  With no end-stop switches a default must be a LOWER bound on the travel,
    //  never an upper one: too narrow is merely useless, too wide grinds the
    //  mechanism, and the post-home sweep drives all the way to posMax.
    //
    //  +/-300 is inside any plausible travel, proves the needle moves, and is
    //  obviously provisional so it does not get mistaken for a calibration.
    cfg.posMin = -PROVISIONAL_LIMIT_HS; cfg.posMax = PROVISIONAL_LIMIT_HS;
    settingsTouch();
    //  The index band constants are NOT purged: calFinishBand() stores them
    //  relative to the forward ON edge (onFwd is 0 by construction), so they
    //  are frame-independent and survive a shifted-limit corruption intact.
    Con.println(F("         (index band constants kept - they are index-relative)"));
  }
}

//  WHICH TIER IS ACTUALLY RUNNING. Reported rather than inferred from the
//  sample count, because those two can disagree: three marks close together fit
//  a parabola that passes the monotonicity test and then fails the endpoint
//  test, and the machine falls back while a count-derived readout still says
//  QUADRATIC.
enum : uint8_t { TUNE_STORED_LINE = 0, TUNE_LS_LINE, TUNE_QUADRATIC };

//  THE REFUSAL TRAVELS WITH THE MODEL NAME, so it reaches every surface that
//  reports the model - the console, the portal card and the mark toast - and
//  cannot be left off one of them.
//
//  It has to, because "reaches X - Y MHz" cannot show a refusal: every refusal
//  exit calls installStoredLine() first, so the reach afterwards is always
//  bandLow..bandHigh, which is the most reassuring pair of numbers on the
//  machine. A two-mark fit collapsing the dial onto 0.7 MHz would have been
//  reported as "reaches 88.1 - 107.9".
static char gTuneModelStr[96] = "stored straight line";
static const char *tuneModelName() { return gTuneModelStr; }

//  WERE THE MARKS REFUSED OUTRIGHT? Recorded, not inferred from the model
//  string. Searching that string for "REFUSED" also matched
//  "least-squares line (QUADRATIC REFUSED: turns back on itself)" - which is a
//  mark that WAS accepted, whose line IS the running mapping, and which the
//  author would have seen painted red for twelve seconds as a rejection. The
//  model string exists to be read by a person; a decision must not be parsed
//  back out of prose.
static bool gFitRefused = false;
static bool fitRefused() { return gFitRefused; }

//  DEMOTED IS NOT REFUSED, and it is not nothing either. A quadratic that gets
//  demoted to a least-squares line still uses the marks - so it must not paint
//  a mark red - but it IS the half-megahertz straight line this whole feature
//  replaces, so it must not pass in silence. It needed its own fact rather than
//  a second meaning bolted onto the refusal flag: gating the /api/set warning
//  on gFitRefused alone switched that warning off for the most likely way to
//  cause a demotion, which is typing a wrong tuner end.
static bool gFitDemoted = false;
static bool fitDemoted() { return gFitDemoted; }

static void setTuneModel(uint8_t m, const char *why) {
  //  Refused means: the marks were thrown away and the stored line installed.
  //  A quadratic demoted to a line is NOT that - the marks are still in use.
  gFitRefused = (m == TUNE_STORED_LINE && why && *why);
  gFitDemoted = (m == TUNE_LS_LINE     && why && *why);
  //  THE ENDS NOTE IS APPENDED HERE, at the one place the model string is
  //  built, because the previous version appended it on fitTuneCurve()'s
  //  SUCCESS path - and the state it warns about is most likely on the four
  //  paths that return early. A machine with no marks and no measured ends took
  //  the n<2 exit, ran a line over the 6023-count placeholder against a much
  //  shorter real travel, clamped the whole lower half of the dial dead, and
  //  reported "reaches 88.1 - 107.9 MHz". Same shape, fourth time: the warning
  //  sat where the path that needed it could not arrive.
  if (m == TUNE_QUADRATIC) {
    strncpy(gTuneModelStr, "QUADRATIC", sizeof(gTuneModelStr) - 1);
  } else if (m == TUNE_LS_LINE) {
    //  A LINE BECAUSE THE QUADRATIC WAS REFUSED is not the same fact as a line
    //  because two marks is all there is, and the difference is the half a
    //  megahertz mid-band this whole feature exists to remove. This branch
    //  used to discard `why` outright, so the one refusal that does NOT end at
    //  installStoredLine() was the one refusal nothing reported.
    if (why && *why)
      snprintf(gTuneModelStr, sizeof(gTuneModelStr),
               "least-squares line (QUADRATIC REFUSED: %s)", why);
    else
      strncpy(gTuneModelStr, "least-squares line", sizeof(gTuneModelStr) - 1);
  } else if (why && *why) {
    snprintf(gTuneModelStr, sizeof(gTuneModelStr), "MARKS REFUSED (%s) - running the stored line", why);
  } else {
    strncpy(gTuneModelStr, "stored straight line", sizeof(gTuneModelStr) - 1);
  }
  if (cfg.tunerEndsSet != ENDS_BOTH)
    strncat(gTuneModelStr,
            cfg.tunerEndsSet == ENDS_LOW  ? " - tuner HIGH end not measured" :
            cfg.tunerEndsSet == ENDS_HIGH ? " - tuner LOW end not measured"  :
                                            " - tuner ends NOT measured",
            sizeof(gTuneModelStr) - strlen(gTuneModelStr) - 1);
  gTuneModelStr[sizeof(gTuneModelStr) - 1] = 0;
}

//  ONE STATEMENT OF THE TUNING STATE, and every surface prints this rather than
//  assembling its own. The same defect kept turning up at a new
//  site each time - a reach computed where the evaluator clamps it, a reason
//  omitted from one toast, a note appended on one path - because five places
//  each built their own sentence out of the same parts. Sets *bad when what it
//  describes is something the author needs to act on.
static uint8_t tuneSampleCount();          // defined with the fit, below

//  The same two numbers the state line uses, for the JSON, which wants them
//  separately rather than as prose.
static int32_t tuneReach0() {
  int32_t xlo, xhi; Needle::curveDomain(xlo, xhi);
  int32_t a = Needle::tuneFreq10At(xlo), b = Needle::tuneFreq10At(xhi);
  return (a < b) ? a : b;
}
static int32_t tuneReach1() {
  int32_t xlo, xhi; Needle::curveDomain(xlo, xhi);
  int32_t a = Needle::tuneFreq10At(xlo), b = Needle::tuneFreq10At(xhi);
  return (a < b) ? b : a;
}

static void tuneStateLine(char *out, size_t n, bool *bad) {
  int32_t xlo, xhi;
  Needle::curveDomain(xlo, xhi);
  //  AT THE DOMAIN ENDS, ordered by FREQUENCY. tuneFreq10At(calLow) clamps
  //  whenever the domain is narrower than the travel, and this accumulator
  //  counts down, so the pair printed in accumulator order reads backwards.
  int32_t a = Needle::tuneFreq10At(xlo), b = Needle::tuneFreq10At(xhi);
  int32_t lo = (a < b) ? a : b, hi = (a < b) ? b : a;
  snprintf(out, n, "%u mark%s, %s, reaches %ld.%ld - %ld.%ld MHz",
           (unsigned)tuneSampleCount(), tuneSampleCount() == 1 ? "" : "s",
           tuneModelName(),
           (long)(lo / 10), (long)(lo % 10), (long)(hi / 10), (long)(hi % 10));
  //  *bad MEANS REFUSED, and nothing else. Unmeasured tuner ends are a real
  //  thing the author must fix, but they are not a rejection of the button he
  //  just pressed - and routing them through the refusal channel meant the
  //  FIRST successful mark on any uncalibrated machine, which is every machine
  //  before the ends are set, came back painted as a failure. The note travels
  //  in the sentence above; only a refusal travels as one.
  if (bad) *bad = fitRefused();
}

//  Solve a small system by Gaussian elimination with partial pivoting. The
//  right-hand side is a SEPARATE argument: it used to be column n of a [3][4],
//  and the two-unknown caller put its RHS in column 2, so the solver read a
//  zero and returned zeros - an advertised least-squares line that silently
//  delivered nothing. Returns false if the system is singular, which is how a
//  degenerate set of samples (all at the same angle, say) is refused rather
//  than producing infinities that would reach the needle.
static bool solveLS(const double a[3][3], const double rhs[3], double out[3], int n) {
  double m[3][4];
  for (int r = 0; r < n; r++) {
    for (int c = 0; c < n; c++) m[r][c] = a[r][c];
    m[r][n] = rhs[r];
  }
  for (int col = 0; col < n; col++) {
    int piv = col;
    for (int r = col + 1; r < n; r++)
      if (fabs(m[r][col]) > fabs(m[piv][col])) piv = r;
    if (fabs(m[piv][col]) < 1e-9) return false;
    if (piv != col) for (int k = 0; k <= n; k++) { double t = m[col][k]; m[col][k] = m[piv][k]; m[piv][k] = t; }
    for (int r = 0; r < n; r++) {
      if (r == col) continue;
      double fct = m[r][col] / m[col][col];
      for (int k = col; k <= n; k++) m[r][k] -= fct * m[col][k];
    }
  }
  for (int i = 0; i < n; i++) out[i] = m[i][n] / m[i][i];
  return true;
}

static uint8_t tuneSampleCount() {
  uint8_t n = 0;
  for (int i = 0; i < TUNE_MARKS; i++) if (cfg.tuneUsed & (1u << i)) n++;
  return n;
}

//  THE FALLBACK, used when there are fewer than two samples and whenever a fit
//  is refused: the straight line
//  from bandLow at the tuner's low end to bandHigh at its high end. Expressed
//  in accumulator units like everything else, so there is one input coordinate
//  in the whole chain.
static void installStoredLine(const char *why = nullptr) {
  setTuneModel(TUNE_STORED_LINE, why);
  int32_t x0 = cfg.calLow, x1 = cfg.calHigh;
  if (x0 == x1) {   // never calibrated: a flat line beats a division by zero
    Needle::setCurve(0.0, (float)cfg.bandLow, 0.0f, 0.0f, cfg.tuneOffset10,
                     x0 - 1, x0 + 1);
    return;
  }
  float slope = ((float)cfg.bandHigh - (float)cfg.bandLow) / (float)(x1 - x0);
  Needle::setCurve((double)x0, (float)cfg.bandLow, slope, 0.0f,
                   cfg.tuneOffset10, x0, x1);
}

//  FIT THE SHAFT-ANGLE -> FREQUENCY CURVE and hand it to the needle module,
//  which is the only thing that evaluates it. Called from applySettings(), so
//  a stored sample and a running curve cannot disagree.
static void fitTuneCurve() {
  double px[TUNE_MARKS], py[TUNE_MARKS];
  int n = 0;
  for (int i = 0; i < TUNE_MARKS && n < TUNE_MARKS; i++)
    if (cfg.tuneUsed & (1u << i)) { px[n] = cfg.tuneP[i]; py[n] = cfg.tuneF[i]; n++; }

  if (n < 2) { installStoredLine(); return; }

  //  THE DOMAIN, and this is the third attempt at it, so the reasoning is
  //  written out rather than assumed.
  //
  //  The mapping must be valid everywhere the shaft can physically go, or
  //  tuneFreq10At()'s clamp freezes part of the dial - silently, because the
  //  endpoint and span guards below are evaluated on whatever domain they are
  //  handed, so a domain that is too SMALL can never trip them.
  //
  //  So: if the tuner's ends have been measured, they ARE the travel and they
  //  go in whole, widened by the marks in case a mark somehow sits outside
  //  them. Nothing is allowed to cut that back.
  //
  //  If they have NOT been measured, calLow/calHigh are a placeholder and using
  //  them stretches the fit far beyond the real travel until its own vertex
  //  falls inside and three good marks get refused. In that case the marks are
  //  the only thing actually known, so the domain is their padded hull and the
  //  machine SAYS the dial is limited to it rather than pretending otherwise.
  double mlo = px[0], mhi = px[0];
  for (int i = 1; i < n; i++) { if (px[i] < mlo) mlo = px[i]; if (px[i] > mhi) mhi = px[i]; }

  int32_t xlo, xhi;
  if (cfg.tunerEndsSet == ENDS_BOTH && cfg.calLow != cfg.calHigh) {
    //  MEASURED: the travel IS the domain, with NO padding. A previous version
    //  initialised the domain to the marks' hull plus 30% and let the travel
    //  only widen it - so on this set the pad (480 counts) dominated the travel
    //  entirely and the curve was evaluated 480 counts beyond each mechanical
    //  end. That is where "reaches 85.96 - 112.43 MHz" came from: 112.43 MHz is
    //  not a frequency any FM tuner reaches, and it was about to be printed
    //  under the label "tuner reaches" as the headline diagnostic. The endpoint
    //  and span guards were being applied out there too, so a fit correct
    //  everywhere the shaft can go could be refused for its behaviour where the
    //  shaft cannot.
    xlo = (cfg.calLow < cfg.calHigh) ? cfg.calLow : cfg.calHigh;
    xhi = (cfg.calLow < cfg.calHigh) ? cfg.calHigh : cfg.calLow;
    //  A mark outside the measured travel still has to be inside the domain,
    //  or the curve is asked to explain a point it is not allowed to evaluate.
    if ((int32_t)lrint(mlo) < xlo) xlo = (int32_t)lrint(mlo);
    if ((int32_t)lrint(mhi) > xhi) xhi = (int32_t)lrint(mhi);
  } else {
    //  NOT MEASURED: the marks are all that is known. Pad, because without the
    //  ends there is nothing else to say how far the shaft goes - and the model
    //  string says so on every surface.
    double mpad = (mhi - mlo) * 0.30;
    if (mpad < 100.0) mpad = 100.0;
    xlo = (int32_t)lrint(mlo - mpad); xhi = (int32_t)lrint(mhi + mpad);
  }

  //  Centre on the mean. Uncentred, x^4 over a few thousand counts conditions
  //  the normal equations badly for no reason.
  double x0 = 0;
  for (int i = 0; i < n; i++) x0 += px[i];
  x0 /= n;

  double S0 = 0, S1 = 0, S2 = 0, S3 = 0, S4 = 0, T0 = 0, T1 = 0, T2 = 0;
  for (int i = 0; i < n; i++) {
    double x = px[i] - x0, x2 = x * x;
    S0 += 1; S1 += x; S2 += x2; S3 += x2 * x; S4 += x2 * x2;
    T0 += py[i]; T1 += x * py[i]; T2 += x2 * py[i];
  }

  double a = 0, b = 0, c = 0;
  bool haveQuad = false;
  const char *quadWhy = nullptr;

  if (n >= 3) {
    const double A[3][3] = { { S0, S1, S2 }, { S1, S2, S3 }, { S2, S3, S4 } };
    const double R[3]    = { T0, T1, T2 };
    double sol[3] = { 0, 0, 0 };
    if (solveLS(A, R, sol, 3)) {
      a = sol[0]; b = sol[1]; c = sol[2];
      //  A TUNER IS MONOTONIC. The accumulator may run either way against
      //  frequency - on this set it counts DOWN as frequency goes up - so the
      //  test is that the slope keeps the SAME SIGN across the domain, not that
      //  it is positive. A parabola that turns round inside the dial is wrong
      //  however well it matches three points.
      double d0 = b + 2.0 * c * ((double)xlo - x0);
      double d1 = b + 2.0 * c * ((double)xhi - x0);
      if ((d0 > 0 && d1 > 0) || (d0 < 0 && d1 < 0)) haveQuad = true;
      else {
        quadWhy = "turns back on itself";
        Con.println(F("  [WARN] tuning: that curve turns back on itself inside the"
                         " dial - falling back to a straight line."));
      }
    }
    else {
      quadWhy = "the marks are degenerate";
      Con.println(F("  [WARN] tuning: those marks are degenerate - falling back to a line."));
    }
  }

  if (!haveQuad) {
    const double A[3][3] = { { S0, S1, 0 }, { S1, S2, 0 }, { 0, 0, 0 } };
    const double R[3]    = { T0, T1, 0 };
    double sol[3] = { 0, 0, 0 };
    if (!solveLS(A, R, sol, 2)) {
      Con.println(F("  [WARN] tuning: the marks do not define a line - using the stored one."));
      installStoredLine("degenerate");
      return;
    }
    a = sol[0]; b = sol[1]; c = 0;
  }

  //  Sanity at the domain ends before this can command anything.
  double e0 = a + b * ((double)xlo - x0) + c * ((double)xlo - x0) * ((double)xlo - x0);
  double e1 = a + b * ((double)xhi - x0) + c * ((double)xhi - x0) * ((double)xhi - x0);
  if (!(e0 > 500 && e0 < 2000 && e1 > 500 && e1 < 2000)) {
    //  BY FREQUENCY, NOT BY ACCUMULATOR. xlo/xhi are the numeric min and max of
    //  the shaft coordinate, and on this set the accumulator counts DOWN as
    //  frequency goes up - so e0 is the HIGH end. Printed in that order this
    //  read "reaches 145.0 - 62.0 MHz", descending, while every other reach
    //  report on the machine ascends.
    double lo10 = (e0 < e1) ? e0 : e1, hi10 = (e0 < e1) ? e1 : e0;
    Con.printf("  [WARN] tuning: that fit reaches %.1f - %.1f MHz, which is not a dial.\n"
                  "         REFUSED - still on the stored straight line. Mark further apart.\n",
                  lo10 / 10.0, hi10 / 10.0);
    { char w[44]; snprintf(w, sizeof(w), "fit reaches %.0f-%.0f MHz", lo10 / 10.0, hi10 / 10.0);
      installStoredLine(w); }
    return;
  }

  //  AND THE DIAL HAS TO SPAN SOMETHING. The two-point calibration this
  //  replaced refused a pair whose ends came out less than 2 MHz apart, and
  //  that test was dropped without a replacement. Position alone does not catch
  //  it: two marks 600 counts apart typed 91.3 and 91.5 - a misidentified
  //  station, or 98.5 mistyped - pass the duplicate guard, produce a line whose
  //  ends are 91.2 and 91.9, pass the endpoint test because both are inside the
  //  FM band, and map the ENTIRE shaft travel onto 0.7 MHz. The needle then
  //  sits on one spot for the whole dial and the portal reports a healthy
  //  "least-squares line", because a fit that is never treated as a fallback
  //  never prints a warning.
  double span = (e1 > e0) ? (e1 - e0) : (e0 - e1);
  if (span < 20) {
    Con.printf("  [WARN] tuning: those marks describe a %.1f MHz dial. Check the\n"
                  "         frequencies you typed. REFUSED - still on the stored line.\n",
                  span / 10.0);
    { char w[40]; snprintf(w, sizeof(w), "fit spans only %.1f MHz", span / 10.0);
      installStoredLine(w); }
    return;
  }

  //  ONLY NOW is the model what the machine is actually running. Setting this
  //  before the guards meant a refused fit still reported its intended tier.
  setTuneModel(haveQuad ? TUNE_QUADRATIC : TUNE_LS_LINE, haveQuad ? nullptr : quadWhy);
  Needle::setCurve(x0, (float)a, (float)b, (float)c, cfg.tuneOffset10, xlo, xhi);
}

//  A FIELD BOUNDS CHECK, WHICH IS NOT THE FORBIDDEN SIZE TEST. It does not
//  decide which version a blob is - the header does that - it only refuses a
//  field value this array cannot hold. It is load-bearing because settingsLoad()
//  reads min(got, sizeof(cfg)) bytes, so a current-version blob written by some
//  other build with a LARGER CFG_SPURS arrives here with spurUsed past the end
//  of ours. It also refuses a feature outside the RDA's LO window.
//  PURGE TO EMPTY rather than clamping: a truncated spur list is not a list, and
//  a half-restored one silently blinds the oscillator search at wrong places.
static void purgeCorruptSpurs() {
  bool bad = (cfg.spurUsed > CFG_SPURS);
  for (uint8_t i = 0; !bad && i < cfg.spurUsed; i++)
    if (cfg.spur[i] < Rda::LO_WINDOW_LO10 || cfg.spur[i] > Rda::LO_WINDOW_HI10) bad = true;
  if (!bad) return;
  Con.printf("  [WARN] the stored fixed-feature list is not usable (%u entries) -\n",
                (unsigned)cfg.spurUsed);
  Con.println(F("         cleared. Sweep with the set OFF and press o to re-learn."));
  cfg.spurUsed = 0;
  for (uint8_t i = 0; i < CFG_SPURS; i++) cfg.spur[i] = 0;
  settingsTouch();
}

static void applySettings() {
  brightnessTarget(ampOn ? cfg.brightOn : cfg.brightOff);
  Panel::setLevels(cfg.panelTuning, cfg.panelIdle, cfg.panelOther);
  Panel::setIdleDelayMs(cfg.panelIdleMs);
  Panel::setFadeMs(cfg.panelFadeMs);
  Panel::setDwellMs(cfg.panelDwellMs);
  Needle::setUpLimits(cfg.upVmax, cfg.upAccel);
  Needle::setDownLimits(cfg.dnVmax, cfg.dnAccel);
  Needle::setSecondOrder(cfg.wn, cfg.zeta);
  Needle::setDecayEnvelope(cfg.riseMs, cfg.fallMs);
  Needle::setDwell(cfg.dwellUpMs, cfg.dwellDnMs);
  Needle::setMicro(cfg.microFast, cfg.microSlow);
  Needle::setReapproachSpeed(cfg.reapHsps);
  Needle::setGeometry(cfg.posMin, cfg.posMax);
  //  WRITE BACK. setGeometry() refuses a crossed or non-bracketing pair, and
  //  without this cfg kept the refused values while the needle used the old
  //  ones - the portal showed one geometry and the machine ran another, with
  //  a console warning as the only evidence. Now cfg IS what the needle uses.
  Needle::getGeometry(cfg.posMin, cfg.posMax);
  Needle::setIndexCal(cfg.idxOnFwd, cfg.idxOffFwd, cfg.idxOnRev, cfg.idxOffRev);
  Needle::setCalibration(cfg.calLow, cfg.calHigh, cfg.tunerEndsSet == ENDS_BOTH);
  fitTuneCurve();
  Needle::setDial(cfg.dialLow, cfg.dialHigh);
  //  cfg IS THE ORIGIN for both of these; the modules hold projections. See the
  //  note in rda.h on why the spur list is pushed rather than owned there.
  Rda::spurSet(cfg.spur, cfg.spurUsed);
  Net::setTxPower(cfg.wifiTxQ);
}

//  Load, and MIGRATE rather than discard. A stored blob is trusted only if the
//  magic matches; the version then decides how to read it.
static void settingsLoad() {
  prefs.begin("amb3", true);
  size_t n = prefs.getBytesLength("cfg");
  if (n == 0) { prefs.end(); Con.println(F("  no stored settings - defaults in use.")); return; }

  //  DISPATCH ON THE HEAD OF THE BLOB, NEVER ON ITS SIZE.
  //
  //  Verified with this project's own compiler on 2026-08-31:
  //      sizeof(SettingsV1) == sizeof(SettingsV2) == sizeof(Settings) == 100
  //  so `n == sizeof(...)` was the SAME TEST three times over, and the V1
  //  branch - which had no magic check at all - was an unguarded catch-all for
  //  any hundred-byte blob.
  //
  //  What that cost: one flipped bit in `magic` and a v3 blob was read on the
  //  V1 layout, where the fields sit four bytes earlier from `dispDwellMs`
  //  onward. `wn` would have been read from offset 28, which in v3 holds
  //  dnAccel and padding - so the ENTIRE needle motion profile is replaced by garbage,
  //  and then marked dirty and written back as a valid v3. The soft limits sit
  //  at offset 52 and are identical in both layouts, so nothing would have hit
  //  a stop; the machine would simply have moved wrongly, for ever, with the
  //  evidence overwritten.
  //
  //  Found by review, not by failure. Do not reintroduce a size test here.
  //  READ WHAT IS THERE, UP TO OUR OWN SIZE.
  //
  //  This gate used to demand an EXACT match with the current struct, and
  //  that quietly defeated the whole migration machinery the moment an
  //  append actually changed the size: a shorter but perfectly migratable
  //  blob was thrown away and the defaults took over - precisely the loss
  //  the versioning exists to prevent. It survived this long only because
  //  V1, V2 and V3 all happened to be the same 100 bytes. V4 is the first
  //  append that grows the struct, and it would have eaten the settings of
  //  a machine somebody had just spent an evening calibrating.
  //
  //  The warning above still stands and is a DIFFERENT rule: never use SIZE
  //  to decide WHICH version a blob is. Version comes from the header; size
  //  only says how much of it we are allowed to read.
  uint8_t blob[sizeof(Settings)];
  memset(blob, 0, sizeof(blob));
  size_t  want = (n < sizeof(blob)) ? n : sizeof(blob);
  size_t  got  = want ? prefs.getBytes("cfg", blob, want) : 0;
  prefs.end();

  if (got < sizeof(SettingsV1)) {
    Con.printf("  [WARN] stored settings are %u bytes, too short to migrate.\n",
                  (unsigned)n);
    return;
  }

  uint16_t magic = 0, ver = 0;
  memcpy(&magic, blob,     sizeof(magic));
  memcpy(&ver,   blob + 2, sizeof(ver));

  //  V1 predates the magic, so its first two bytes are brightOn/brightOff -
  //  arbitrary, and vanishingly unlikely to be 0xA838. That absence IS the
  //  discriminator, and it is a positive test rather than a fallthrough.
  if (magic != SETTINGS_MAGIC) {
    SettingsV1 v1;
    memcpy(&v1, blob, sizeof(v1));
    cfg.brightOn = v1.brightOn;  cfg.brightOff = v1.brightOff;
    cfg.hour12 = v1.hour12;      cfg.blankLeadZero = v1.blankLeadZero;
    cfg.dispDwellMs = v1.dispDwellMs;  cfg.dispFadeMs = v1.dispFadeMs;
    cfg.panelTuning = v1.panelTuning;  cfg.panelIdle = v1.panelIdle;
    cfg.panelOther = v1.panelOther;    cfg.panelIdleMs = v1.panelIdleMs;
    cfg.panelFadeMs = v1.panelFadeMs;  cfg.panelDwellMs = v1.panelDwellMs;
    cfg.upVmax = v1.upVmax;   cfg.upAccel = v1.upAccel;
    cfg.dnVmax = v1.dnVmax;   cfg.dnAccel = v1.dnAccel;
    cfg.wn = v1.wn;           cfg.zeta = v1.zeta;
    cfg.riseMs = v1.riseMs;   cfg.fallMs = v1.fallMs;
    cfg.dwellUpMs = v1.dwellUpMs;      cfg.dwellDnMs = v1.dwellDnMs;
    cfg.microFast = v1.microFast;      cfg.microSlow = v1.microSlow;
    cfg.reapHsps = v1.reapHsps;
    cfg.posMin = v1.posMin;   cfg.posMax = v1.posMax;
    cfg.idxOnFwd = v1.idxOnFwd;   cfg.idxOffFwd = v1.idxOffFwd;
    cfg.idxOnRev = v1.idxOnRev;   cfg.idxOffRev = v1.idxOffRev;
    cfg.sweepOn = v1.sweepOn;
    cfg.calLow = v1.calLow;   cfg.calHigh = v1.calHigh;  cfg.lastAngle = v1.lastAngle;
    cfg.showTuning = v1.showTuning;
    cfg.bandLow = v1.bandLow; cfg.bandHigh = v1.bandHigh;
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    Con.println(F("  settings MIGRATED from the pre-versioning layout."));
    return;
  }

  if (ver == SETTINGS_VERSION) {
    memcpy(&cfg, blob, got < sizeof(cfg) ? got : sizeof(cfg));
    Con.println(F("  settings loaded."));
    return;
  }

  //  V3 -> V4 BY PREFIX, and deliberately NOT by blob length.
  //
  //  Append-only means an older blob is a byte-exact prefix of the current
  //  struct - but only up to the last field the older layout actually HAD.
  //  Copying `len` bytes would reach into whatever trailing PADDING the old
  //  struct carried, and a new field can land in that padding: V2 -> V3 did
  //  exactly that, tuneHoldMs landing in V2's tail with no size change at all.
  //  So the copy stops at the first new field, and the new fields keep their
  //  defaults - which is the whole point of appending.
  //  V4 -> V5. Same prefix rule as V3 -> V4. tuneUsed defaults to 0, so a
  //  machine with no marks keeps the straight line it already had and behaves
  //  identically until the author sets the first mark.
  //  V5 -> V6. Same prefix rule again. There is NOTHING to infer here, unlike
  //  V4 -> V5 where a non-default calLow/calHigh was itself evidence that a
  //  hand measurement had happened: a V5 blob contains no trace of a spur ever
  //  having been learned, and no trace of a transmit rung that worked. So both
  //  take their defaults: an empty fixed-feature list, and wifiTxQ 8, the
  //  bottom rung, from which net.cpp's ladder climbs on a failed join.
  //  V6 -> V7. Same prefix rule. The new field is the tube set's IF, and its
  //  default is 212 (10.60 MHz) rather than the nominal 214, because 212 is
  //  what this machine measured. That IS a behaviour change on the first boot
  //  after the upgrade - every computed station moves down 0.1 MHz - and it is
  //  the correct one: the old 10.7 was an assumption that six measurements
  //  contradict. A machine that disagrees can say so from the portal now.
  if (ver == 6) {
    size_t cut = offsetof(Settings, ifOffset20);
    if (got < cut) cut = got;
    memcpy(&cfg, blob, cut);
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    Con.println(F("  settings MIGRATED from version 6 - the tube set's IF starts at"));
    Con.println(F("  10.60 MHz (Bible), where the firmware used to assume 10.70."));
    return;
  }

  if (ver == 5) {
    size_t cut = offsetof(Settings, wifiTxQ);
    if (got < cut) cut = got;
    memcpy(&cfg, blob, cut);
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    Con.println(F("  settings MIGRATED from version 5 - transmit power starts at"));
    Con.println(F("  the bottom rung (2 dBm) and climbs as needed; the RDA's fixed-feature list starts empty."));
    return;
  }

  if (ver == 4) {
    size_t cut = offsetof(Settings, tuneOffset10);
    if (got < cut) cut = got;
    memcpy(&cfg, blob, cut);
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    //  A V4 machine has no such bit, but it does have the evidence: a pair that
    //  is not the struct default was captured by hand at some point.
    cfg.tunerEndsSet = (cfg.calLow != 0 || cfg.calHigh != 6023) ? ENDS_BOTH : 0;
    Con.println(F("  settings MIGRATED from version 4 - the tuning curve starts"));
    Con.println(F("  as the straight line you already had. Set marks A, B and C to bend it."));
    Con.printf ("  tuner ends %s.\n",
                   cfg.tunerEndsSet ? "were already measured" : "have NEVER been measured");
    return;
  }

  if (ver == 3) {
    size_t cut = offsetof(Settings, dialLow);
    if (got < cut) cut = got;
    memcpy(&cfg, blob, cut);
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    Con.println(F("  settings MIGRATED from version 3 - dial face defaults to 87.9-107.9."));
    return;
  }

  if (ver == 2) {
    SettingsV2 v2;
    memcpy(&v2, blob, sizeof(v2));
    cfg.brightOn = v2.brightOn;  cfg.brightOff = v2.brightOff;
    cfg.hour12 = v2.hour12;      cfg.blankLeadZero = v2.blankLeadZero;
    cfg.dispDwellMs = v2.dispDwellMs;  cfg.dispFadeMs = v2.dispFadeMs;
    cfg.panelTuning = v2.panelTuning;  cfg.panelIdle = v2.panelIdle;
    cfg.panelOther = v2.panelOther;    cfg.panelIdleMs = v2.panelIdleMs;
    cfg.panelFadeMs = v2.panelFadeMs;  cfg.panelDwellMs = v2.panelDwellMs;
    cfg.upVmax = v2.upVmax;   cfg.upAccel = v2.upAccel;
    cfg.dnVmax = v2.dnVmax;   cfg.dnAccel = v2.dnAccel;
    cfg.wn = v2.wn;           cfg.zeta = v2.zeta;
    cfg.riseMs = v2.riseMs;   cfg.fallMs = v2.fallMs;
    cfg.dwellUpMs = v2.dwellUpMs;      cfg.dwellDnMs = v2.dwellDnMs;
    cfg.microFast = v2.microFast;      cfg.microSlow = v2.microSlow;
    cfg.reapHsps = v2.reapHsps;
    cfg.posMin = v2.posMin;   cfg.posMax = v2.posMax;
    cfg.idxOnFwd = v2.idxOnFwd;   cfg.idxOffFwd = v2.idxOffFwd;
    cfg.idxOnRev = v2.idxOnRev;   cfg.idxOffRev = v2.idxOffRev;
    cfg.sweepOn = v2.sweepOn;
    cfg.calLow = v2.calLow;   cfg.calHigh = v2.calHigh;  cfg.lastAngle = v2.lastAngle;
    cfg.showTuning = v2.showTuning;
    cfg.bandLow = v2.bandLow; cfg.bandHigh = v2.bandHigh;
    cfg.magic = SETTINGS_MAGIC; cfg.version = SETTINGS_VERSION;
    gTouchGen++; dirtyAt = millis();      // boot, single-threaded
    Con.println(F("  settings MIGRATED from version 2."));
    return;
  }

  //  Our magic, a version we do not know: almost certainly a DOWNGRADE. Guessing
  //  at a future layout is how a calibration gets silently rewritten, so refuse.
  Con.printf("  [WARN] settings are version %u, this firmware knows %u.\n",
                (unsigned)ver, (unsigned)SETTINGS_VERSION);
  Con.println(F("         Defaults in use. Nothing is written back until you"));
  Con.println(F("         load the newer firmware, or upload a COMPLETE settings file."));
  gSettingsLocked = true;
}

//  Human-readable dump, console `D`. Plain key=value so it is recoverable by
//  hand. The portal's settings file is settingsToText(), not this: the grouped
//  lines at the top (several pairs per line) are refused by the importer; the
//  one-pair lines from dialLow on are written in the file's own format.
static void settingsDump() {
  Con.println(F("--- ambersong settings begin ---"));
  Con.printf("version=%u" "\n", cfg.version);
  Con.printf("brightOn=%u brightOff=%u hour12=%u blankLeadZero=%u" "\n",
                cfg.brightOn, cfg.brightOff, cfg.hour12, cfg.blankLeadZero);
  Con.printf("dispDwellMs=%u dispFadeMs=%u" "\n", cfg.dispDwellMs, cfg.dispFadeMs);
  Con.printf("panelTuning=%u panelIdle=%u panelOther=%u" "\n",
                cfg.panelTuning, cfg.panelIdle, cfg.panelOther);
  Con.printf("panelIdleMs=%u panelFadeMs=%u panelDwellMs=%u" "\n",
                cfg.panelIdleMs, cfg.panelFadeMs, cfg.panelDwellMs);
  Con.printf("upVmax=%u upAccel=%u dnVmax=%u dnAccel=%u" "\n",
                cfg.upVmax, cfg.upAccel, cfg.dnVmax, cfg.dnAccel);
  Con.printf("wn=%.2f zeta=%.2f" "\n", cfg.wn, cfg.zeta);
  Con.printf("riseMs=%u fallMs=%u dwellUpMs=%u dwellDnMs=%u" "\n",
                cfg.riseMs, cfg.fallMs, cfg.dwellUpMs, cfg.dwellDnMs);
  Con.printf("microFast=%u microSlow=%u reapHsps=%u" "\n",
                cfg.microFast, cfg.microSlow, cfg.reapHsps);
  Con.printf("posMin=%ld posMax=%ld" "\n", (long)cfg.posMin, (long)cfg.posMax);
  Con.printf("idxOnFwd=%ld idxOffFwd=%ld idxOnRev=%ld idxOffRev=%ld" "\n",
                (long)cfg.idxOnFwd, (long)cfg.idxOffFwd,
                (long)cfg.idxOnRev, (long)cfg.idxOffRev);
  Con.printf("sweepOn=%u showTuning=%u" "\n", cfg.sweepOn, cfg.showTuning);
  Con.printf("calLow=%ld calHigh=%ld lastAngle=%ld" "\n",
                (long)cfg.calLow, (long)cfg.calHigh, (long)cfg.lastAngle);
  Con.printf("bandLow=%u bandHigh=%u tuneHoldMs=%u" "\n",
                cfg.bandLow, cfg.bandHigh, cfg.tuneHoldMs);
  //  The dial calibration asks the author to CHECK these two, and the dump once
  //  did not print them. A settings dump that omits settings is worse
  //  than no dump: it reads as confirmation.
  Con.printf("dialLow=%u dialHigh=%u" "\n", cfg.dialLow, cfg.dialHigh);
  //  DECIMAL, matching what settingsToText writes, so a line copied from this
  //  dump into a settings file is valid rather than silently parsing as zero.
  Con.printf("tuneOffset10=%d" "\n", (int)cfg.tuneOffset10);
  Con.printf("ifOffset=%.2f" "\n", cfg.ifOffset20 * 0.05f);
  Con.printf("tuneUsed=%u" "\n", (unsigned)cfg.tuneUsed);
  Con.printf("tunerEndsSet=%u" "\n", (unsigned)cfg.tunerEndsSet);
  for (int i = 0; i < TUNE_MARKS; i++) {
    if (!(cfg.tuneUsed & (1u << i))) continue;
    //  MACHINE-READABLE, BYTE-FOR-BYTE WHAT settingsToText WRITES. This used to
    //  print "tuneMark0=486 acc,91.3 MHz  (by hand)" while a comment two lines
    //  up invited copying dump lines into a settings file - where strtol stops
    //  at the space, the mark is skipped, and ONE bad line refuses every other
    //  mark in the file with it. The gloss goes on its own '#' line, because
    //  the parser skips only lines that START with '#'; a trailing comment
    //  would break it exactly the same way.
    Con.printf("# mark %c: %u.%u MHz%s" "\n",
                  i < 3 ? ('A' + i) : ('a' + i - 3),
                  cfg.tuneF[i] / 10, cfg.tuneF[i] % 10,
                  i < 3 ? "  (by hand)" : "  (auto)");
    Con.printf("tuneMark%d=%ld,%u" "\n", i, (long)cfg.tuneP[i], cfg.tuneF[i]);
  }
  //  BYTE-FOR-BYTE WHAT settingsToText WRITES, same rule as the marks above:
  //  a line copied out of this dump into a settings file has to be valid, so
  //  any human gloss goes on its own '#' line where the parser will skip it.
  //  RAW QUARTER-dBm, and settingsToText writes exactly the same thing.
  //  It was briefly a gSet row with scale 0.25, which made the FILE say
  //  "wifiTxQ=2.000" while this dump said "wifiTxQ=8" - paste that dump line
  //  into a settings file and it would have been read as EIGHT dBm, with the
  //  portal reporting success. The tuneMark0 scar, one field over. Both write
  //  raw now.
  Con.printf("wifiTxQ=%u" "\n", (unsigned)cfg.wifiTxQ);
  Con.printf("# transmit power %.1f dBm, learned%s" "\n", cfg.wifiTxQ / 4.0f,
                cfg.wifiTxQ < 60 ? " (below the 15 dBm ceiling)" : " (the 15 dBm ceiling)");
  Con.printf("spurUsed=%u" "\n", (unsigned)cfg.spurUsed);
  for (uint8_t i = 0; i < cfg.spurUsed && i < CFG_SPURS; i++) {
    Con.printf("# fixed feature %u.%u MHz LO" "\n", cfg.spur[i] / 10, cfg.spur[i] % 10);
    Con.printf("spur%u=%d" "\n", (unsigned)i, (int)cfg.spur[i]);
  }
  Con.println(F("--- ambersong settings end ---"));
}
//  THE TOUCH COUNT - see gTouchGen. Settings are changed from two cores: the
//  portal task on core 0 (settings, actions, uploads) and loop() on core 1
//  (samples, the shaft angle). Called AFTER the change, always.
static void settingsTouch() {
  dirtyAt = millis();
  __atomic_add_fetch(&gTouchGen, 1, __ATOMIC_SEQ_CST);
}

//  "Save now" from the portal. Still refuses while the needle is moving (see
//  writeSafe(), and Firmware Gospel §7) - it only stops waiting out the debounce.
static bool settingsForceFlush();
//  NEVER WRITE NVS WHILE THE NEEDLE IS MOVING.
//  An NVS write disables the flash cache, which stalls every piece of code that
//  is not in IRAM - including the step emitter. Phase G measured flash traffic
//  as the one load that disturbs this machine, and the first boot after the
//  settings migration proved it again: 383 us of step jitter and a 1257 us
//  display slot error, both one-off, both while the migration was being written
//  during the homing sweep.
//
//  Deferring costs nothing. The write is already debounced; it can wait for the
//  needle to stop.
//
//  RETURNS WHETHER THE BLOB IS ON FLASH. It used to clear `dirty` first and
//  ignore both prefs.begin() and putBytes(), so a failed write printed "settings
//  saved", the portal said "saved", and nothing retried.
//
//  gSettingsLocked: set by settingsLoad() when the stored blob is a version this
//  firmware does not know - a DOWNGRADE. That branch always promised that
//  nothing would be overwritten, but the first settingsTouch() (the
//  lastAngle save, within ten seconds of boot) wrote the defaults over the newer
//  blob two seconds later. Now nothing writes until the author uploads a COMPLETE
//  settings file, which is the one deliberate act that means "use these".
static bool gWriteBusy = false;       // the last refusal was the other core writing
static bool settingsWrite() {
  if (gSettingsLocked) return false;
  //  NOTHING IS WRITTEN WHILE THIS IMAGE IS ON TRIAL.
  //  A new image that changes the settings layout migrates them at boot and
  //  would save the new layout within seconds - and if it then rolled back,
  //  the previous image would find a blob newer than it knows and lock itself
  //  onto defaults. So changes wait in RAM until confirmTick() confirms the
  //  image; the ordinary debounce then writes them. The cost: a change made in
  //  the trial minute is lost if the power goes in that minute.
  if (s3ImageOnTrial()) return false;
  //  ONE WRITER AT A TIME. The portal's reboot retries its save on core 0
  //  while loop() may be flushing on core 1, and both would open the same
  //  Preferences handle. The loser just reports "not now" and is retried.
  static volatile bool busy = false;
  if (__atomic_exchange_n(&busy, true, __ATOMIC_ACQUIRE)) { gWriteBusy = true; return false; }
  gWriteBusy = false;
  uint32_t gen = __atomic_load_n(&gTouchGen, __ATOMIC_SEQ_CST);   // BEFORE the copy
  static Settings snap;                 // static: 220 bytes, not on a task stack
  memcpy(&snap, &cfg, sizeof(snap));
  bool ok = prefs.begin("amb3", false);
  if (ok) {
    ok = prefs.putBytes("cfg", &snap, sizeof(snap)) == sizeof(snap);
    prefs.end();
  }
  if (!ok) {
    dirtyAt = millis();           // stays dirty: retried after the debounce
    __atomic_store_n(&busy, false, __ATOMIC_RELEASE);
    Con.println(F("  [WARN] settings NOT saved - the flash write failed."));
    return false;
  }
  //  What is on flash is the copy taken at `gen`. Anything touched since keeps
  //  the counts apart, so it is still unsaved and is written again.
  __atomic_store_n(&gSavedGen, gen, __ATOMIC_SEQ_CST);
  __atomic_store_n(&busy, false, __ATOMIC_RELEASE);
  Con.println(F("  settings saved."));
  return true;
}

//  A write is refused while the needle moves, while a calibration runs, and
//  while an image is being flashed. All three are the same reason: an NVS write
//  disables the flash cache, and everything that is not in IRAM stops dead.
//  Measured 2026-08-31 - step jitter 383 us during a write, 49 us after.
static bool writeSafe() {
  return Needle::velHsps() == 0.0f && !Needle::calBusy() && !Portal::otaActive();
}

static void settingsFlush() {
  if (!settingsDirty() || millis() - dirtyAt < 2000) return;
  if (!writeSafe()) return;
  settingsWrite();
}

//  The portal's "save now". It skips the debounce, never the safety.
//  RETURNS WHETHER ANYTHING IS ACTUALLY ON FLASH NOW. It used to return void
//  and print the refusal to the console only, so the portal answered "saved"
//  to a save that writeSafe() had declined - and writeSafe() declines for as
//  long as the needle is moving, which is exactly when someone is adjusting
//  needle settings. Two false reassurances in a row (this, then "rebooting")
//  is a data-loss path, and it is the failure the whole settings-migration
//  machinery exists to prevent, arriving by a route that machinery misses.
bool settingsFlushNow();          //  the cross-unit name, defined below

//  WHY the last save was refused, for the portal - there are several reasons,
//  and "the needle is moving" was once given for all of them.
static const char *gSaveWhy = "";
const char *settingsSaveWhy() { return gSaveWhy; }
bool        settingsLocked()  { return gSettingsLocked; }
static bool settingsForceFlush() {
  if (!settingsDirty()) { Con.println(F("  nothing to save.")); return true; }
  if (gSettingsLocked) {
    gSaveWhy = "NOT saved - the stored settings are from a NEWER firmware; upload a complete settings file first";
    Con.println(F("  save refused - the stored settings are from a NEWER firmware."));
    Con.println(F("  Upload a settings file to replace them, or flash the newer firmware."));
    return false;
  }
  if (s3ImageOnTrial()) {
    gSaveWhy = "NOT saved yet - this firmware is on trial after an update; settings are written once it is confirmed (about a minute)";
    Con.println(F("  save held - this firmware is on trial; settings are written once it is confirmed."));
    return false;
  }
  if (!writeSafe()) {
    gSaveWhy = "NOT saved - the needle is moving or a calibration or update is running; try again when it stops";
    Con.println(F("  save deferred - the needle is moving, or a calibration or an update is running."));
    return false;
  }
  //  UNTIL NOTHING IS LEFT, not once. A write can succeed while a newer change
  //  arrives during it, and the callers of this - Save, Reboot, the end of an
  //  update - act on "true" at once. The other core's own
  //  write in progress is waited out, not reported as a flash failure.
  for (int tries = 0; tries < 40 && settingsDirty(); tries++) {
    if (settingsWrite()) continue;
    if (gWriteBusy) { delay(25); continue; }
    gSaveWhy = "NOT saved - the flash write failed; it will be retried";
    return false;
  }
  if (settingsDirty()) {
    gSaveWhy = "NOT saved - the settings kept changing while being written; try again";
    return false;
  }
  return true;
}

//  The one exported entry point. portal.cpp cannot see the static.
bool settingsFlushNow() { return settingsForceFlush(); }

static uint32_t nowEpoch() {
  if (!timeValid) return 0;
  return epochAtSync + (millis() - millisAtSync) / 1000;
}

// ---------------------------------------------------------------------------
//  THE A32 OTA RELAY.
//  An HTTP upload arrives in whatever sizes the browser felt like; it is
//  re-cut into 1 kB frames, each acknowledged before the next goes out. The
//  ack carries the byte count the A32 has actually committed, so a lost frame
//  is recoverable rather than fatal - we resend from where it says it is.
// ---------------------------------------------------------------------------
static volatile bool     otaAckSeen  = false;
static volatile uint32_t otaAckGot   = 0;
static volatile uint8_t  otaAckState = 0;
//  The A32's Update.getError(), which used to be dropped on the floor. 2026-09-11:
//  "begin refused" with no code sent me guessing at a stale session, then at the
//  partition table. Arduino Update.h:15-27: 1 WRITE, 2 ERASE, 3 READ, 4 SPACE,
//  5 SIZE, 6 STREAM, 7 MD5, 8 MAGIC_BYTE, 9 ACTIVATE, 10 NO_PARTITION,
//  11 BAD_ARGUMENT, 12 ABORT.
static volatile uint8_t  otaAckErr   = 0;
static char              otaAckDetail[32] = {0};

static uint8_t  otaBuf[1024];
static uint16_t otaFill = 0;
static uint32_t otaSent = 0;
static uint32_t otaCrc  = 0;
static char     otaErr[96] = {0};   // 48 cut the messages short

uint32_t    a32OtaSent()  { return otaSent; }
const char *a32OtaError() { return otaErr; }

//  Wait for the A32 to answer. The link is polled from loop() on the OTHER
//  core, so this genuinely does make progress while it waits.
static bool otaWaitAck(uint32_t ms) {
  uint32_t t0 = millis();
  while (!otaAckSeen && millis() - t0 < ms) delay(2);
  return otaAckSeen;
}

static bool otaSendFrame() {
  if (!otaFill) return true;
  uint8_t frame[6 + 1024];
  uint32_t off = otaSent;
  uint16_t len = otaFill;
  memcpy(frame,     &off, 4);
  memcpy(frame + 4, &len, 2);
  memcpy(frame + 6, otaBuf, len);

  for (int attempt = 0; attempt < 4; attempt++) {
    otaAckSeen = false;
    gLink.send(MSG_OTA_DATA, frame, 6 + len);
    if (!otaWaitAck(3000)) continue;
    if (otaAckState == 4) {
      snprintf(otaErr, sizeof(otaErr), "A32: %s (err %u)", otaAckDetail, (unsigned)otaAckErr);
      return false;
    }
    //  THE ACK CARRIES NO FRAME IDENTITY, so the contract has to be stated in
    //  terms of the only thing it does carry - the byte count committed. For a
    //  frame at `off` of length `len` there are exactly three answers:
    //
    //    off + len   accepted. Advance.
    //    off         not taken (a duplicate, or it arrived out of order).
    //                Resend. A LATE ACK FROM THE PREVIOUS ATTEMPT ALSO LANDS
    //                HERE and says the same thing, which is why a stale one is
    //                harmless rather than merely unlikely.
    //    anything    the two ends disagree about where they are. Stop; a
    //                firmware image is not something to guess at.
    if (otaAckGot == off + len) {
      otaSent += len;
      otaCrc   = crc32_le(otaCrc, otaBuf, len);
      otaFill  = 0;
      return true;
    }
    if (otaAckGot != off) {
      snprintf(otaErr, sizeof(otaErr), "A32 is at %lu, we are at %lu",
               (unsigned long)otaAckGot, (unsigned long)off);
      return false;
    }
  }
  snprintf(otaErr, sizeof(otaErr), "no answer at %lu bytes", (unsigned long)otaSent);
  return false;
}

bool a32OtaBegin() {
  otaErr[0] = 0; otaFill = 0; otaSent = 0; otaCrc = 0;
  if (!peerHello) { snprintf(otaErr, sizeof(otaErr), "the A32 is not answering"); return false; }
  ProtoOtaBegin b = {};
  b.size = 0;                            // streamed; the length is not known yet
  strncpy(b.fwVersion, FW_VERSION, PROTO_VERSION_LEN - 1);
  otaAckSeen = false;
  gLink.send(MSG_OTA_BEGIN, b);
  //  TWENTY SECONDS, NOT FIVE.
  //  The A32's BEGIN handler mutes, waits out the fade, and then calls
  //  Update.begin() (which does not erase the slot - erasing happens inside
  //  Update.write()). Five seconds started failing consistently on 2026-09-01 -
  //  the transfer never began, so nothing was written and the audio was never
  //  at risk, but the update simply could not be started. There is no cost to
  //  waiting: this is the one-off handshake, not the per-frame path.
  if (!otaWaitAck(20000)) { snprintf(otaErr, sizeof(otaErr), "no answer to BEGIN"); return false; }
  if (otaAckState == 4) { snprintf(otaErr, sizeof(otaErr), "A32: %s (err %u)", otaAckDetail, (unsigned)otaAckErr); return false; }
  Con.println(F("  relaying a firmware image to the A32."));
  return true;
}

bool a32OtaChunk(const uint8_t *data, size_t n) {
  while (n) {
    size_t room = sizeof(otaBuf) - otaFill;
    size_t take = (n < room) ? n : room;
    memcpy(otaBuf + otaFill, data, take);
    otaFill += take; data += take; n -= take;
    if (otaFill == sizeof(otaBuf) && !otaSendFrame()) return false;
  }
  return true;
}

bool a32OtaEnd() {
  if (otaFill && !otaSendFrame()) return false;
  otaAckSeen = false;
  gLink.send(MSG_OTA_END, &otaCrc, 4);
  //  THE A32 ANSWERS BEFORE IT REBOOTS - state 3 "ok, rebooting", then a 250 ms
  //  pause. So silence IS a failure: this used to ignore the wait and test a
  //  state left over from the last DATA ack, and a lost END was reported as
  //  "image sent" while the A32 gave up 20 s later and kept its old firmware
  //  A late DATA ack is not the answer; keep waiting.
  uint32_t t0 = millis();
  bool done = false;
  while (!done) {
    //  ONE READING of the clock per pass: two could straddle 4000 ms and
    //  the subtraction wrap to ~49 days of waiting.
    uint32_t el = millis() - t0;
    if (el >= 4000) break;
    if (!otaWaitAck(4000 - el)) break;
    if (otaAckState == 3 || otaAckState == 4) done = true;
    else otaAckSeen = false;
  }
  if (!done) {
    //  NOT "it keeps its old firmware": the A32 commits the image BEFORE it
    //  answers, so a lost answer can hide a good update. Say what is known,
    //  and tell it to stop waiting - otherwise it sits muted for its own 20 s
    //  give-up. The handshake after it restarts shows which version it runs.
    a32OtaAbort();
    peerHello = false; haveState = false;
    snprintf(otaErr, sizeof(otaErr), "the A32 did not confirm the update - check its version once it reconnects");
    return false;
  }
  if (otaAckState == 4) { snprintf(otaErr, sizeof(otaErr), "A32: %s (err %u)", otaAckDetail, (unsigned)otaAckErr); return false; }
  Con.printf("  A32 image sent: %lu bytes, crc %08lx\n",
                (unsigned long)otaSent, (unsigned long)otaCrc);
  //  Handshake again, so the version shown is the one it comes back with.
  peerHello = false; haveState = false;
  return true;
}

void a32OtaSetError(const char *why) { snprintf(otaErr, sizeof(otaErr), "%s", why); }

void a32OtaAbort() {
  gLink.send(MSG_OTA_ABORT);
  otaFill = 0;
}

//  Every adjustable value in the machine, in one place. See settings_api.h.
#include "settings_table.h"

static void onMessage(const ProtoFramer &f) {
  switch (f.type()) {
    case MSG_HELLO_ACK: {
      ProtoHello h;
      if (!f.as(h)) { Con.println(F("  [FAIL] HELLO_ACK size - version skew")); break; }
      gLink.notePeerHello(h);
      peerHello = true;
      Con.printf("  [PASS] A32 up: proto v%d, firmware %s\n", h.protoVersion, gLink.peerVersion);
      //  A GUARD THAT CANNOT FIRE TODAY: the framer drops any frame whose
      //  version byte is not PROTO_VERSION (it only counts it, badVer) before
      //  it reaches this handler, so a version skew shows as silence - no
      //  handshake at all - not as this line.
      if (h.protoVersion != PROTO_VERSION)
        Con.printf("  [WARN] PROTOCOL MISMATCH - S3 v%d, A32 v%d. Reflash the A32.\n",
                      PROTO_VERSION, h.protoVersion);
      gLink.send(MSG_GET_TIME);
      gLink.send(MSG_GET_CFG);     // and tell me what settings you came up with
      break;
    }
    case MSG_STATE:
      //  A STATE FRAME THAT WILL NOT PARSE MUST INVALIDATE THE MIRROR.
      //
      //  haveState was set true and never cleared, which was safe only while a
      //  length mismatch was impossible. Appending the zero-data watch to
      //  ProtoState on 2026-09-10 made it possible: flash one MCU and not the
      //  other and as() rejects every frame on length (ProtoFramer::as() in
      //  proto.h). The old code would then have kept showing the last values
      //  it ever parsed - source, volume, Bluetooth, the peak meters - with
      //  nothing on the page to say they had stopped moving: a measurement not
      //  taken, reported as if it were.
      //
      //  CRC has already passed by the time we are here, so a rejection is not
      //  noise on the wire - it is a version skew, and the honest answer is "?".
    {
      //  AN A32 RESTART THE LINK NEVER SAW. A reboot shorter than the two
      //  second silence rule left the handshake standing, so the S3 kept the
      //  OLD firmware version and the settings mirror it had read before - seen
      //  after the A32 OTA of 2026-09-24. The A32's clock going backwards is the
      //  tell; the 49.7-day wrap is excluded by requiring an old value that was
      //  not near the top.
      uint32_t prevMs = haveState ? a32.a32Ms : 0;
      if (f.as(a32)) {
        if (prevMs && prevMs < 0xF0000000u && a32.a32Ms + 5000u < prevMs) {
          Con.println(F("  [WARN] the A32 restarted - handshaking again."));
          peerHello = false;
        }
        haveState = true;
        //  THE POT MOVES THE VOLUME AND NOTHING TELLS US. The A32 never resends
        //  its settings when the knob turns - it just reports the new level in
        //  its state. Without this the portal's volume slider showed whatever
        //  the mirror was seeded with at handshake and drifted further from the
        //  truth with every turn of the knob.
        a32cfg.audio.volume = a32.volume;
        a32cfg.audio.muted  = a32.muted;
      } else {
        haveState = false;
        //  Once, not on every frame: this arrives four times a second and a
        //  console scrolling past at that rate hides everything else.
        static bool said = false;
        if (!said) {
          said = true;
          Con.println(F("  [WARN] the A32's STATE frame does not match this build."));
          Con.println(F("         Flash BOTH MCUs. Audio is unaffected - only telemetry stops."));
        }
      }
    }
      break;
    case MSG_TIME: {
      ProtoTime t;
      if (!f.as(t)) break;
      gTimeAsked = 0;
      if (t.valid && t.unixUtc > 1600000000UL) {
        gRtcBad      = 0;
        epochAtSync  = t.unixUtc;
        millisAtSync = millis();
        timeValid    = true;
      } else {
        gRtcBad = 1;
        Con.println(F("  [WARN] the DS3231 has no valid time yet."));
      }
      break;
    }
    case MSG_CFG:
      if (f.as(a32cfg)) {
        haveCfg = true;
        Con.printf("  A32 settings mirrored: vol %u, taper %.1f, pot %u..%u..%u\n",
                      a32cfg.audio.volume, a32cfg.audio.taperX10 / 10.0,
                      a32cfg.potMin, a32cfg.potMid, a32cfg.potMax);
      }
      break;

    case MSG_OTA_STATUS: {
      ProtoOtaStatus st;
      if (!f.as(st)) break;
      otaAckGot   = st.received;
      otaAckState = st.state;
      otaAckErr   = st.errCode;
      strncpy(otaAckDetail, st.detail, sizeof(otaAckDetail) - 1);
      otaAckDetail[sizeof(otaAckDetail) - 1] = 0;
      otaAckSeen  = true;
      break;
    }

    case MSG_LOG:
      Con.printf("  [A32] %.*s\n", (int)f.length(), (const char *)f.payload());
      break;
    default: break;
  }
}

// ---------------------------------------------------------------------------
//  THE PORTAL'S VIEW OF THE MACHINE. One JSON object, polled once a second.
//  Everything here is already tracked for the console; this only formats it.
//  Nothing in this function may block or write flash.
// ---------------------------------------------------------------------------
static const char *srcName(uint8_t s) {
  return s == SRC_RADIO ? "Radio" : (s == SRC_BT ? "Bluetooth" : "Aux");
}
static const char *btName(uint8_t b) {
  static const char *n[] = { "off", "ready", "looking", "standby",
                             "found", "connected", "pairing" };
  return (b < 7) ? n[b] : "?";
}

//  THE LAST SWEEP, BIN BY BIN, for the portal. Text, not JSON objects per
//  bin: 201 bins of {"f":..,"r":..} is four times the size for no more
//  information.
//
//  Each line is  frequency-in-tenths  rssi  flags , where flags is a character:
//    .  ordinary       ^  more than 5 above the last sweep's median floor
//    S  a stereo pilot was decoded there, so it is a broadcast, never the LO
//    X  a learned fixed feature, excluded from the search
void portalRdaJson(String &out) {
  char buf[64];
  uint8_t n = Rda::binCount();
  snprintf(buf, sizeof(buf), "{\"n\":%u,\"floor\":%u,\"if10\":%d,\"bins\":\"",
           (unsigned)n, (unsigned)Rda::chipRssiFloor(), (int)ifOff10());
  out = buf;
  out.reserve(n * 14 + 96);
  for (uint8_t i = 0; i < n; i++) {
    int16_t f = Rda::binFreq10(i);
    uint8_t r = Rda::binRssi(i);
    char flag = Rda::isSpur(f) ? 'X' : Rda::binStereo(i) ? 'S'
              : (r > Rda::chipRssiFloor() + 5) ? '^' : '.';
    snprintf(buf, sizeof(buf), "%d %u %c;", (int)f, (unsigned)r, flag);
    out += buf;
  }
  out += "\",\"fine\":\"";
  for (uint8_t i = 0; i < Rda::fineCount(); i++) {
    snprintf(buf, sizeof(buf), "%d %u;", (int)Rda::finePointF20(i), (unsigned)Rda::finePointRssi(i));
    out += buf;
  }
  out += "\",\"spurs\":\"";
  for (uint8_t i = 0; i < Rda::spurCount(); i++) {
    snprintf(buf, sizeof(buf), "%d;", (int)Rda::spurFreq10(i));
    out += buf;
  }
  out += "\"}";
}

void portalStateJson(String &out) {
  char buf[400];
  uint32_t e = nowEpoch();
  char clock[8] = "--:--";
  if (e) {
    time_t t = (time_t)e; struct tm tmv; localtime_r(&t, &tmv);
    strftime(clock, sizeof(clock), "%H:%M", &tmv);
  }
  //  The 64-bit microsecond timer: millis() is 32-bit and rolled the uptime
  //  back to 0d every 49.7 days.
  uint32_t up = (uint32_t)(esp_timer_get_time() / 1000000LL);
  char uptime[24];
  snprintf(uptime, sizeof(uptime), "%lud %02lu:%02lu:%02lu",
           (unsigned long)(up / 86400), (unsigned long)((up % 86400) / 3600),
           (unsigned long)((up % 3600) / 60), (unsigned long)(up % 60));

  snprintf(buf, sizeof(buf),
    "{\"src\":\"%s\",\"vol\":%u,\"muted\":%u,\"bt\":\"%s\","
    "\"pkL\":%.1f,\"pkR\":%.1f,\"clip\":%u,"
    "\"clock\":\"%s\",\"up\":\"%s\",\"amp\":%u,\"link\":%u,"
    //  The index sensor live, and where the tuner is sitting. Neither was
    //  reachable from the portal, so "is the sensor being tickled" and
    //  "what am I tuned to" both needed a serial cable to answer.
    "\"idx\":%u,\"tunepm\":%u,\"tuning\":%u,",
    haveState ? srcName(a32.source) : "?",
    haveState ? a32.volume : 0, haveState ? a32.muted : 0,
    haveState ? btName(a32.btState) : "?",
    haveState ? a32.peakLdBx10 / 10.0 : -60.0,
    haveState ? a32.peakRdBx10 / 10.0 : -60.0,
    haveState ? a32.clipped : 0, clock, uptime,
    ampOn ? 1 : 0, peerHello ? 1 : 0,
    Needle::indexNow() ? 1 : 0, Needle::tunePermille(),
    Needle::tuningActive() ? 1 : 0);
  //  APPEND. This used to assign, which was harmless while hState() passed an
  //  empty String and fatal the moment /api/boot passed one that already held
  //  the schema and the values: the whole payload was silently replaced by the
  //  state alone, and the browser - which cannot tell a truncated answer from a
  //  short one - reported "the radio sent no settings list".
  out += buf;

  out += "\"peer\":\"";
  if (haveState)
    for (int i = 0; i < 24 && a32.peerName[i]; i++) {
      char c = a32.peerName[i];
      //  92 is the backslash, written as its code so the escaping of this file
      //  never becomes a question. A device name is somebody else's string.
      out += (c == '"' || c == 92 || (uint8_t)c < 0x20) ? ' ' : c;
    }
  out += "\",";

  snprintf(buf, sizeof(buf),
    "\"pos\":%ld,\"tgt\":%ld,\"nstate\":\"%s\",\"homed\":%u,"
    "\"drift\":%ld,\"driftpend\":%u,\"driftmeas\":%u,"
    "\"calbusy\":%u,\"calpct\":%u,\"ladder\":%u,"
    "\"reach0\":%ld,\"reach1\":%ld,\"tunemodel\":\"%s\",\"tunemarks\":%u,"
    "\"mustchg\":%u,\"jit\":%lu,\"slot\":%lu,\"heap\":%lu,",
    (long)Needle::position(), (long)Needle::target(), Needle::stateName(),
    Needle::homed() ? 1 : 0, (long)Needle::lastDrift(),
    Needle::driftPending() ? 1 : 0, Needle::driftMeasured() ? 1 : 0,
    Needle::calBusy() ? 1 : 0, Needle::calProgressPct(),
    haveState ? a32.rawLadder : 0,
    (long)tuneReach0(), (long)tuneReach1(),
    tuneModelName(), (unsigned)tuneSampleCount(),
    Portal::mustChangeCreds() ? 1 : 0,
    (unsigned long)Needle::jitterMaxUs(), (unsigned long)Display::worstSlotErrorUs(),
    (unsigned long)ESP.getFreeHeap());
  out += buf;

  out += "\"calmsg\":\"";
  out += Needle::calMessage();
  out += "\",";

  //  WHY things are not working, without a serial cable. The link counters say
  //  whether the A32 has ever answered - "rx 0" is a wiring or power problem,
  //  "rx climbing with crc climbing" is a signalling one - and the homing fault
  //  says what the needle gave up on.
  snprintf(buf, sizeof(buf),
    "\"linkrx\":%lu,\"linktx\":%lu,\"linkcrc\":%lu,\"hello\":%u,\"astate\":%u,\"i2c\":%u,\"magnet\":%u,"
    "\"under\":%lu,\"ring\":%u,",
    (unsigned long)gLink.rxCount(), (unsigned long)gLink.txCount(),
    (unsigned long)gLink.badCrc(), peerHello ? 1 : 0, haveState ? 1 : 0,
    Needle::i2cOk() ? 1 : 0, Needle::magnetOk() ? 1 : 0,
    (unsigned long)(haveState ? a32.underruns : 0),
    (unsigned)(haveState ? a32.ringFill : 0));
  out += buf;

  //  THE ZERO-DATA WATCH. "zrel" is the number that matters: every increment is
  //  the DAC leaving analogue mute, which the datasheet does not ramp, which is
  //  a pop the A32's volume cannot scale. Published here rather than left on the
  //  A32's console because the console is a cable and the cabinet gets shut -
  //  the author asked for exactly this on 2026-09-10.
  //
  //  Zeroed rather than omitted when the link is down: a missing key would make
  //  the page read "-" and a stale key would make it read a lie. haveState is
  //  published as "astate" - NOT as "hello", which is the handshake and stays 1
  //  when a STATE frame of the wrong size is refused; the
  //  page blanks the audio readings when astate is 0.
  snprintf(buf, sizeof(buf),
    "\"zarm\":%lu,\"zrel\":%lu,\"zsince\":%ld,\"zlong\":%lu,\"zmute\":%u,"
    "\"zfloor\":%u,\"i2sraw\":%u,\"stalls\":%lu,\"stallus\":%lu,",
    (unsigned long)(haveState ? a32.zddArm : 0),
    (unsigned long)(haveState ? a32.zddRel : 0),
    //  -1 for "never", which JavaScript can test for; 0 means "this instant".
    (long)(haveState && a32.zddSinceRelMs != 0xFFFFFFFFu
             ? (long)a32.zddSinceRelMs : -1L),
    (unsigned long)(haveState ? a32.zddLongestMs : 0),
    (unsigned)(haveState ? a32.zddMuted : 0),
    (unsigned)(haveState ? a32.zddFloor : 0),
    (unsigned)(haveState ? a32.i2sSticky : 0),
    (unsigned long)(haveState ? a32.stalls : 0),
    (unsigned long)(haveState ? a32.stallMaxUs : 0));
  out += buf;

  //  THE VOLUME WATCH. "vjump" is the one to read: the largest single-update
  //  change in the volume the knob asked for, with "vjms" saying when. With
  //  nobody's hand on the knob, anything but a small number there is the ADC
  //  glitching, and "gmax" says whether that glitch reached the multiply.
  snprintf(buf, sizeof(buf),
    "\"praw\":%u,\"pmin\":%u,\"pmax\":%u,\"pjump\":%u,\"vsteps\":%lu,"
    "\"vjump\":%u,\"vjms\":%lu,\"gsteps\":%lu,\"gmin\":%lu,\"gmax\":%lu,\"upot\":%u,",
    (unsigned)(haveState ? a32.potRaw : 0),
    (unsigned)(haveState ? a32.potSeenMin : 0),
    (unsigned)(haveState ? a32.potSeenMax : 0),
    (unsigned)(haveState ? a32.potJumpMax : 0),
    (unsigned long)(haveState ? a32.volSteps : 0),
    (unsigned)(haveState ? a32.volJumpMax : 0),
    (unsigned long)(haveState ? a32.volJumpMs : 0),
    (unsigned long)(haveState ? a32.gainSteps : 0),
    (unsigned long)(haveState ? a32.volQ16Min : 0),
    (unsigned long)(haveState ? a32.volQ16Max : 0),
    (unsigned)(haveState ? a32.usePot : 0));
  out += buf;

  snprintf(buf, sizeof(buf),
    "\"dindrv\":%u,\"sddly\":%u,\"tail\":%u,\"clkdrv\":%u,\"dctest\":%u,\"dcword\":%lu,"
    "\"a32ms\":%lu,\"stlast\":%lu,\"stlastus\":%lu,\"bttx\":%u,\"bttxmin\":%u,"
    "\"mclkdrv\":%u,\"bckdrv\":%u,\"lrckdrv\":%u,"
    "\"rmsl\":%.1f,\"rmsr\":%.1f,",
    (unsigned)(haveState ? a32.dinDrive : 0),
    (unsigned)(haveState ? a32.sdDelay : 0),
    (unsigned)(haveState ? a32.signTail : 0),
    (unsigned)(haveState ? a32.clkDrive : 0),
    (unsigned)(haveState ? a32.testDc : 0),
    (unsigned long)(haveState ? a32.dcWord : 0),
    (unsigned long)(haveState ? a32.a32Ms : 0),
    (unsigned long)(haveState ? a32.stallLastMs : 0),
    (unsigned long)(haveState ? a32.stallLastUs : 0),
    //  255, not 0: level 0 is a REAL setting (-12 dBm), so a zero here could not
    //  be told apart from "the A32 is not talking".
    (unsigned)(haveState ? a32.btTx : 255),
    (unsigned)(haveState ? a32.btTxMin : 255),
    (unsigned)(haveState ? a32.mclkDrive : 255),
    (unsigned)(haveState ? a32.bckDrive : 255),
    (unsigned)(haveState ? a32.lrckDrive : 255),
    haveState ? a32.rmsLdBx10 / 10.0 : -120.0,
    haveState ? a32.rmsRdBx10 / 10.0 : -120.0);
  out += buf;

  out += "\"fault\":\"";
  out += Needle::faultReason();
  out += "\",";
  //  See settingsWrite(): a downgrade found newer settings and writes nothing.
  out += gSettingsLocked ? "\"setlock\":1," : "\"setlock\":0,";
  out += "\"rtc\":"; out += String((int)gRtcBad); out += ",";
  out += "\"linkver\":"; out += String((unsigned long)gLink.badVer()); out += ",";
  //  The author's hand offset on the tuning readout, tenths of a MHz. It now
  //  survives automatic samples, so the page must be able to say it is there.
  out += "\"toff\":"; out += String((int)cfg.tuneOffset10); out += ",";

  //  WHAT THE LAST RDA MEASUREMENT DID, and whether one is running now.
  //  Without these the auto-calibration can only be operated from the serial
  //  cable. It is also what makes an AUTOMATIC sample reportable at
  //  all: one that only printed itself to a port nobody is watching has not
  //  reported anything.
  snprintf(buf, sizeof(buf), "\"smpbusy\":%u,\"smppct\":%u,\"reidx\":%u,\"reidxtry\":%u,\"hunt\":%u,",
           sampleBusy() ? 1 : 0, Rda::sweeping() ? Rda::progressPct() : 0,
           Needle::recovering() ? 1 : 0, (unsigned)Needle::recoverTries(),
           Needle::hunting() ? 1 : 0);
  out += buf;
  out += "\"smp\":\"";
  out += sampleNote();
  out += "\",";

  //  THE TUNING READOUT.
  //
  //  This used to require magnetOk() as well as i2cOk(), and that hid the
  //  frequency on a set where the angle was perfectly usable: the AS5600
  //  answers on the bus and returns an angle the NEEDLE TRACKS CORRECTLY,
  //  but its magnet-detect bit reads clear - too far, or too weak, not
  //  absent. Gating a good reading on a marginal status bit meant the Now
  //  page showed nothing at all rather than showing the number it had.
  //
  //  So the frequency now follows the bus, and "magnet" is reported beside
  //  it so the page can flag a doubtful reading instead of swallowing it.
  out += "\"tune\":\"";
  if (Needle::i2cOk()) {
    int32_t f = Needle::tuneFreq10();          //  ONE definition - see needle.h
    //  THE PORTAL SHOWS THE TRUTH. The Leditron clamps - it draws the FM band
    //  and nothing else - but this is the screen used for diagnosis, and the
    //  author's own complaint was that silently pinning destroys the evidence
    //  that the band line is wrong: "if the tuning goes UP over the max steps
    //  I can do then THE NEEDLE IS WRONG". Clamping here would have answered
    //  that objection by removing the instrument.
    bool over = (f < (int32_t)cfg.dialLow) || (f > (int32_t)cfg.dialHigh);
    snprintf(buf, sizeof(buf), "%ld.%ld MHz%s", (long)(f / 10), (long)(f % 10),
             over ? "  (past the printed face)" : "");
    out += buf;
  }
  out += "\"}";
}

//  See the note in settings_api.h. An OTA is a long flash write, and this
//  machine cannot absorb one while it is moving or lit.
static volatile bool otaQuiet = false;
// ---------------------------------------------------------------------------
//  ROLLBACK AND THE WATCHDOG, THE A32'S TREATMENT.
//
//  ROLLBACK. The bootloader and the core are built with app rollback, but the
//  core's initArduino() marked every new image valid before setup() - so an
//  S3 build that hung or crashed at boot looped on itself and only USB could
//  fix it. Returning true here tells the core "this firmware confirms itself":
//  a freshly OTA'd image boots ON TRIAL, and the bootloader goes back to the
//  previous image on the next reset unless confirmTick() confirmed it first.
//  A USB flash is not on trial and is unaffected.
//
//  WHAT EARNS CONFIRMATION: what the next update needs, and nothing more. A
//  minute of running, the network up (home or rescue AP) and the portal task
//  still turning - the way in for the next image. NOT the A32: a broken audio
//  board must never roll back a good main board. NOT the needle: a homing
//  fault is about the mechanism, not the build. And never a build whose proto
//  self-test failed.
//
//  THE WATCHDOG is what turns a hang into that reset. The core's task watchdog
//  (panic on) watched only core 0's idle task, and the needle's step emitter
//  takes that one off on purpose (stepTask). loop() and the needle supervisor
//  are now watched, 15 s. Not the portal task: a flash write legitimately
//  blocks it, and otaWatchdog() already covers a stalled upload. The console
//  prompts that wait in loop() feed it (wdtFeed) - their waits are bounded.
// ---------------------------------------------------------------------------
extern "C" bool verifyRollbackLater() { return true; }

static const uint32_t WDT_TIMEOUT_S    = 15;
static const uint32_t CONFIRM_AFTER_MS = 60000;
static const char *gImgState     = "?";
static bool        gImgOnTrial   = false;
static bool        gImgConfirmed = false;
static bool        gLoopWdtOn    = false;
static bool        gProtoBroken  = false;

//  The portal refuses an S3 upload while this is true: the update would be
//  written over the other slot - the very image a rollback returns to.
bool s3ImageOnTrial() { return gImgOnTrial && !gImgConfirmed; }

static void wdtFeed() { if (gLoopWdtOn) esp_task_wdt_reset(); }

static void imageReport() {
  Con.printf("  image      : %s, watchdog %s%s\n", gImgState, gLoopWdtOn ? "on" : "OFF",
             esp_ota_get_last_invalid_partition() ? " - an earlier update was ROLLED BACK" : "");
}

static void confirmTick(uint32_t now) {
  if (!s3ImageOnTrial() || gProtoBroken || now < CONFIRM_AFTER_MS) return;
  static uint32_t lastTry = 0, seenLoops = 0;
  if (lastTry && now - lastTry < 10000) return;
  lastTry = now;
  //  The portal is turning if its loop count moved since the last look; the
  //  first look only records it.
  uint32_t l = Portal::loops();
  bool portalAlive = seenLoops && l != seenLoops;
  seenLoops = l;
  if (!portalAlive || !Net::connected()) return;
  esp_err_t e = esp_ota_mark_app_valid_cancel_rollback();
  if (e == ESP_OK) {
    gImgConfirmed = true;
    gImgState = "valid (confirmed)";
    Con.println(F("  image confirmed (a minute of running, network and portal up)."));
  } else {
    //  Retried every ten seconds. Until it lands, a reset rolls back a good
    //  image - and an S3 upload is refused.
    Con.printf("  image confirm FAILED (err %d) - retrying\n", (int)e);
  }
}

//  Raised when an upload ends WITHOUT a reboot (refused, aborted, failed): the
//  needle was stopped for it and nothing else would start it again. loop()
//  picks it up - this runs on the portal task, the other core.
static volatile bool otaResume = false;
//  The same resume for a REFUSED REBOOT: hReboot stopped the needle to get its
//  save through, the save still failed, and nothing else would start the
//  needle again - it sat frozen with the radio playing.
void portalNeedleResume() { otaResume = true; }

void portalOtaQuiet(bool on) {
  if (!on && otaQuiet) otaResume = true;
  otaQuiet = on;
  if (on) {
    Needle::stop();
    Display::blank();
  }
}

// ---------------------------------------------------------------------------
//  WHERE THE NEEDLE SHOULD BE POINTING, given the amp and the source.
//
//  THE POLICY, author's, 2026-08-31:
//    - The sensor is sought and the frame calibrated at S3 boot, which the
//      firmware treats as first power-up; the front switch is not. After that
//      it homes again only to recover (a slip, a failed home) or when asked.
//    - After that the position is always known, because the needle sits at the
//      low stop whenever the radio is off or the source is not RADIO. Turning
//      the set on is therefore a journey from a known place, not a search.
//    - Every crossing of the index corrects the position passively - see
//      needle.cpp - so steps cannot accumulate into an error between power-ups.
//
//  What this replaces: homing on every amp power-up, which meant a search and
//  a full sweep every time the front switch was touched.
// ---------------------------------------------------------------------------
//  ONE DEFINITION OF "THE TUNER IS WHAT IS PLAYING", because two things now
//  depend on it and they must not be able to disagree. The needle has used this
//  predicate since it stopped homing on every amp power-up. The Leditron's
//  tuning readout grew up beside it WITHOUT one, so turning the knob lit a
//  frequency with the amp off, or while the source was Bluetooth - a readout
//  for a source that is not playing. The frequency is
//  shown only when the amp is on AND the source is RADIO.
//  A LISTENING POLICY, NOT A HARDWARE FACT. The selector
//  only tells the A32 which sound to play; the tube set runs whenever the amp
//  does, whatever the selector says (Bible §21, §5). The needle follows and the
//  dial calibrates only while the radio is what you are listening to - by
//  choice.
static bool radioLive() {
  uint8_t src = haveState ? a32.source : SRC_AUX;
  return ampOn && src == SRC_RADIO;
}

static void applyNeedleMode() {
  //  Not during an OTA: portalOtaQuiet() stopped the needle once, and an amp or
  //  source change mid-upload used to start it again during the flash write
  //  The upload ends in a reboot, which homes anyway.
  if (otaQuiet) return;
  if (!Needle::homed()) return;          // homing owns the needle until it is done
  if (radioLive()) Needle::track();
  else             Needle::park();
}


// ---------------------------------------------------------------------------
//  THE RDA SAMPLER
//
//  One measurement: read the tube set's local oscillator at whatever dial
//  position the knob is on, turn it into a station frequency, and store it as
//  a tuning sample. LO = station - IF (cfg.ifOffset20, 10.60 MHz by default),
//  low-side, found on this set on the bench 2026-09-04.
//
//  THE SHAFT IS THE MEASUREMENT, NOT THE NEEDLE. What gets stored is the
//  AS5600 accumulator - the tuning capacitor's own angle - paired with the
//  frequency found there. The needle is an indicator that follows the shaft and
//  is not in this path at all, so an outstanding needle slip does not poison a
//  sample. What WOULD poison one is the knob moving while the sweep runs, and
//  that is checked at the end rather than assumed.
//
//  TWO STAGES, cheap one first. The narrow sweep is +/-1.5 MHz around whatever
//  the current curve predicts and takes about 10 s; if the curve is badly
//  wrong, or the oscillator sits under one of the learned spur notches, that
//  finds nothing and the wide sweep - the whole 20 MHz window, about a minute -
//  is tried once. A found candidate is then refined on the 50 kHz grid.
// ---------------------------------------------------------------------------
enum { SMP_IDLE = 0, SMP_NARROW, SMP_WIDE, SMP_FINE };

//  WHAT THE LAST MEASUREMENT DID, in one line, kept rather than only printed.
//  The console is a cable and the cabinet gets shut; this is what the portal
//  reads. Same rule as gRestoreNote and gActionRefused.
static char     gSmpNote[128] = "";

static uint8_t  gSmpStage  = SMP_IDLE;
static int32_t  gSmpAcc    = 0;        // shaft angle when the sweep started
static uint32_t gSmpGaps   = 0;        // encoder outages counted at that moment
static bool     gSmpRitual = false;    // true: asked for by hand (lower margin, "press again"); false: automatic
//  Carried across the fine pass: the coarse answer, kept so a refinement that
//  finds nothing falls back to it rather than losing a good measurement.
static int16_t  gSmpCoarse10 = 0;
static uint8_t  gSmpMargin   = 0;

//  MARGIN OVER THE LOCAL FLOOR, never absolute amplitude. rda.h note 3: a fixed
//  spur reads louder than the oscillator, and the bench proved a fixed absolute
//  threshold works across most of the dial and silently fails at 87.9-89 where
//  the LO window is at band 2's least sensitive edge. Measured margins on this
//  machine so far: +18, +23, +25. Eight is comfortably clear of noise (+3 was
//  the worst thing the baseline sweep offered with the set switched off).
static const uint8_t SMP_MIN_MARGIN = 8;

//  AND A HIGHER BAR WHEN NOBODY IS WATCHING. An operator who pressed the button
//  has just looked at the dial and would notice an absurd answer; an automatic
//  sample is read by nobody and goes straight into the curve the author's whole
//  calibration rests on. Measured margins on this machine have been +18 to +28,
//  so 14 costs nothing real and refuses everything marginal.
static const uint8_t SMP_AUTO_MARGIN = 14;

//  How far the shaft may drift during a sweep before the pairing is a lie.
//  This tuner's travel is about 2300 counts for about 20 MHz, so 20 counts is
//  under 0.2 MHz - about one bin. Anything more and the frequency measured is not the frequency at the
//  position recorded.
static const int32_t SMP_MAX_DRIFT = 20;

//  HOW FAR A CANDIDATE MAY LAND FROM WHERE THE CURVE SAYS THE DIAL IS.
//
//  The narrow sweep is inherently within 1.5 MHz of the prediction, so this is
//  really a leash on the WIDE sweep, which is free to return anything in the
//  20 MHz window. On 2026-09-07 it returned something 7 MHz out and it was
//  stored: the set was on 98.5 and the stored sample read 91.4. The cause was a
//  stage race in sampleStart() (fixed: the sweep is requested before the stage
//  is raised), not the local-floor theory first blamed; but a wrong answer that
//  far out must be refused whatever produces it.
//
//  40 tenths is deliberately generous. Before any calibration this dial's
//  straight line was wrong by 1.4 MHz at the bottom of the band, and a leash
//  tight enough to be satisfying would refuse the very measurements that fix
//  that. Seven megahertz is not a calibration error, it is a different signal.
static const int32_t SMP_MAX_DISAGREE10 = 40;

//  ...AND IT ONLY APPLIES ONCE THE CURVE HAS EARNED IT. With nothing measured
//  the prediction is a stored straight line that nobody has checked, and
//  leashing a rescue sweep to an unchecked guess is how a machine refuses to
//  learn anything. Two samples in, the curve is evidence rather than a default.
static const uint8_t SMP_TRUST_AFTER = 2;

//  Print it and keep it. Every outcome of a measurement goes through here, so
//  there is exactly one place where "the author was told" can be true or false.
static void smpSay(const char *fmt, ...) {
  va_list ap;
  va_start(ap, fmt);
  vsnprintf(gSmpNote, sizeof(gSmpNote), fmt, ap);
  va_end(ap);
  Con.print(F("  "));
  Con.println(gSmpNote);
}

const char *sampleNote() { return gSmpNote; }
bool        sampleBusy() { return gSmpStage != SMP_IDLE; }

//  Whether a narrow sweep around this LO would touch the RDA's window at all.
static bool loInWindow(int32_t lo10) {
  return lo10 + 15 >= Rda::LO_WINDOW_LO10 && lo10 - 15 <= Rda::LO_WINDOW_HI10;
}

static const char *sampleStart(bool ritual) {
  if (gSmpStage != SMP_IDLE)  return refuse("a measurement is already running");
  if (!Rda::present() && !Rda::reprobe())
    return refuse(Rda::lost() ? "the RDA5807M stopped answering after boot - check its connector"
                              : "the RDA5807M is not answering - nothing can be measured");
  if (Rda::sweeping())        return refuse("the RDA is already sweeping - wait for it");
  //  ONLY WHILE THE RADIO IS PLAYING - the listening policy at radioLive().
  //  With the amp off the set is off too, and a sweep would return a spur.
  if (!radioLive())           return refuse("the dial calibrates only while you listen to the radio - switch the amp on and select RADIO");
  if (!Needle::i2cOk())       return refuse("the tuning encoder is not responding - the dial position would be a guess");

  //  THE SWEEP IS ASKED FOR FIRST, AND THE STAGE IS RAISED ONLY IF IT WAS
  //  ACCEPTED. It used to be the other way round, and that is a race with a
  //  reader on the other core: this function runs on the PORTAL task while
  //  sampleTick() runs in loop(), so between "a measurement is in progress" and
  //  "a sweep is in flight" there was a window where tick saw a live
  //  measurement with nothing sweeping, judged the PREVIOUS sweep's bins,
  //  escalated on them, and took the request slot the narrow sweep was about to
  //  use. Same shape as Rda::sweeping() returning gRunning while requestSweep()
  //  gated on gRunning || gWant: a flag raised before the thing it announces.
  int32_t lo = Needle::tuneFreq10() - ifOff10();
  if (!loInWindow(lo))
    return refuse("the dial is outside the range the oscillator can be measured over");
  if (!Rda::requestSweep((int16_t)(lo - 15), (int16_t)(lo + 15), 1, 300))
    return refuse("the RDA refused the sweep");

  gSmpAcc    = Needle::accumulated();
  gSmpGaps   = Needle::encoderGaps();
  gSmpRitual = ritual;
  gSmpStage  = SMP_NARROW;
  smpSay("measuring: the dial says %ld.%ld, listening around %ld.%ld (about 10 s)",
         (long)(Needle::tuneFreq10() / 10), (long)(Needle::tuneFreq10() % 10),
         (long)(lo / 10), (long)(lo % 10));
  return "measuring - watch the tuning panel";
}

//  Store a completed measurement, or say why not. Returns nothing; it reports.
//  lo20 is TWENTIETHS of a MHz - 50 kHz units - because that is the resolution
//  the fine pass produces and rounding it to tenths before the intermediate
//  frequency is added would throw away exactly the half-bin this exists to
//  resolve. The nominal 10.7 MHz is 214 in these units; the default IF
//  (cfg.ifOffset20, 10.60 MHz) is 212.
static void sampleStore(int16_t lo20, uint8_t margin) {
  //  THE SHAFT HALF OF THE PAIR MUST BE AS CLEAN AS THE RADIO HALF. While the
  //  AS5600 is down its count is frozen, so a knob turned then passes the drift
  //  check and the frequency would be stored against the position the
  //  measurement STARTED at. Any outage since the start, or
  //  one now, and nothing is stored.
  if (!Needle::i2cOk() || Needle::encoderGaps() != gSmpGaps) {
    smpSay("DISCARDED: the tuning encoder dropped out during the measurement - "
           "nothing stored%s", gSmpRitual ? "; press measure again" : "; it will retry by itself");
    return;
  }
  int32_t station20 = (int32_t)lo20 + cfg.ifOffset20;
  //  KNOWN LIMIT, KEPT ON PURPOSE. The
  //  fine pass measures in 50 kHz steps, but a sample is STORED in tenths
  //  (tuneF), so half of that precision is rounded away here: LO 95.05 and
  //  95.10 both store 105.7, a bias of about +25 kHz on odd-twentieth results.
  //  The comment above holds only for an odd IF in twentieths (212 is even).
  //  If the dial ever reads 0.05 MHz off where it should not: store tuneF in
  //  twentieths (a settings-version bump and a migration x2), and fit on those.
  int32_t station10 = (station20 + 1) / 2;      // round to the nearest tenth

  //  DOES IT AGREE WITH WHAT THE DIAL SAYS? See SMP_MAX_DISAGREE10.
  if (__builtin_popcount(cfg.tuneUsed) >= SMP_TRUST_AFTER) {
    int32_t said = Needle::tuneFreq10();
    int32_t off  = station10 - said;
    if (off < 0) off = -off;
    if (off > SMP_MAX_DISAGREE10) {
      smpSay("REFUSED: heard %ld.%ld but the dial says %ld.%ld - %ld.%ld MHz apart, "
             "that is a different signal, not a calibration error",
             (long)(station10 / 10), (long)(station10 % 10),
             (long)(said / 10), (long)(said % 10),
             (long)(off / 10), (long)(off % 10));
      return;
    }
  }
  if (station10 < (int32_t)cfg.dialLow || station10 > (int32_t)cfg.dialHigh) {
    smpSay("REFUSED: that puts the station at %ld.%ld, off the printed dial (%u.%u-%u.%u)",
           (long)(station10 / 10), (long)(station10 % 10),
           cfg.dialLow / 10, cfg.dialLow % 10, cfg.dialHigh / 10, cfg.dialHigh % 10);
    return;
  }

  //  A NEARBY SAMPLE IS REFRESHED, NOT REFUSED.
  //
  //  Two samples at one dial position would make the fit singular, so only one
  //  may exist there - but the answer to a second measurement arriving is to
  //  REPLACE the old one, not to turn it away. Drift is the whole reason this
  //  feature was built; a rule that permanently refuses to re-measure a position
  //  refuses to do its job. Refusing duplicates was right when a sample was a
  //  scarce hand-made thing. Now the instrument makes them, and the same
  //  position arriving again is fresh evidence about a stale answer.
  int  slot    = -1;
  bool refresh = false;
  for (int i = 0; i < TUNE_MARKS; i++) {
    if (!(cfg.tuneUsed & (1u << i))) continue;
    int32_t gap = gSmpAcc - cfg.tuneP[i];
    if (gap < 0) gap = -gap;
    if (gap < 60) { slot = i; refresh = true; break; }
  }
  //  A HAND MARK COVERS THIS SPOT. Slots 0-2 are what the author typed, station
  //  by ear, and only tune.markA/B/C may write them (we can
  //  always re-set those in the web portal, just not automatically). Until
  //  then a sample landing near one REFRESHED it, and parking at the band ends
  //  replaced all three. A sample here is not stored at all: the mark already
  //  describes this position, and a second point 60 counts away would only
  //  argue with it in the fit.
  if (refresh && slot < 3) {
    smpSay("hand mark %c covers this position - measured %ld.%ld, not stored",
           'A' + slot, (long)(station10 / 10), (long)(station10 % 10));
    return;
  }

  //  SLOTS 0-2 ARE THE AUTHOR'S. No sample - automatic or V - claims or
  //  refreshes one; see the hand-mark check above.
  if (slot < 0) {
    int from = 3;                 // 0-2 belong to tune.markA/B/C alone
    for (int i = from; i < TUNE_MARKS; i++)
      if (!(cfg.tuneUsed & (1u << i))) { slot = i; break; }
  }
  //  FULL: KEEP THE SAMPLES SPREAD. A full table
  //  used to refuse every new dial position until someone dropped a sample by
  //  hand. Now the most crowded automatic sample - the one sitting closest to a
  //  neighbour, counting the new point - makes room, so the nine automatic
  //  samples keep covering as much of the dial as they can. If the NEW point is
  //  itself the most crowded, it adds the least and nothing is replaced. Hand
  //  marks are never replaced, but they count as neighbours.
  int32_t replacedF = -1;
  if (slot < 0) {
    auto nearest = [&](int32_t p, int skip, bool withNew) {
      int32_t best = INT32_MAX;
      for (int j = 0; j < TUNE_MARKS; j++) {
        if (j == skip || !(cfg.tuneUsed & (1u << j))) continue;
        int32_t d = p - cfg.tuneP[j]; if (d < 0) d = -d;
        if (d < best) best = d;
      }
      if (withNew) { int32_t d = p - gSmpAcc; if (d < 0) d = -d; if (d < best) best = d; }
      return best;
    };
    int32_t crowd = INT32_MAX;
    for (int i = 3; i < TUNE_MARKS; i++) {
      int32_t d = nearest(cfg.tuneP[i], i, true);
      if (d < crowd) { crowd = d; slot = i; }
    }
    if (slot < 0 || nearest(gSmpAcc, -1, false) < crowd) {
      smpSay("measured %ld.%ld, not stored: every sample slot is full and this position "
             "would add the least coverage", (long)(station10 / 10), (long)(station10 % 10));
      return;
    }
    replacedF = cfg.tuneF[slot];
  }

  //  NOTHING CHANGED MEANS NOTHING WRITTEN. Every stored sample is an NVS write,
  //  and an unattended sampler writing every few minutes for a year would sit at
  //  the flash's endurance limit having learned nothing. Drift is slow.
  if (refresh && cfg.tuneF[slot] == (uint16_t)station10) {
    int32_t moved = gSmpAcc - cfg.tuneP[slot];
    if (moved < 0) moved = -moved;
    if (moved < 8) {
      smpSay("slot %d still reads %ld.%ld - nothing changed, nothing written",
             slot, (long)(station10 / 10), (long)(station10 % 10));
      return;
    }
  }

  cfg.tuneP[slot] = gSmpAcc;
  cfg.tuneF[slot] = (uint16_t)station10;
  cfg.tuneUsed   |= (uint16_t)(1u << slot);
  //  THE HAND CORRECTION IS NOT TOUCHED. It used to be zeroed here; the author
  //  ruled on 2026-09-25 that hand corrections are never erased automatically,
  //  only by hand (tune.nudgeZero).
  applySettings();
  settingsTouch();

  smpSay("%s %ld.%ld MHz (LO %d.%02d, +%u over local) -> slot %d; %u stored; curve: %s",
         refresh ? "RE-MEASURED" : replacedF >= 0 ? "MEASURED (replacing the most crowded sample)" : "MEASURED",
         (long)(station10 / 10), (long)(station10 % 10),
         lo20 / 20, (lo20 % 20) * 5, (unsigned)margin, slot,
         (unsigned)__builtin_popcount(cfg.tuneUsed), tuneModelName());
  if (replacedF >= 0)
    Con.printf("  (it replaced the most crowded sample, %ld.%ld MHz, to keep the samples spread)%c",
               (long)(replacedF / 10), (long)(replacedF % 10), 10);
  if (__builtin_popcount(cfg.tuneUsed) < 3)
    Con.println(F("  three spread-out samples bend it into a quadratic - keep going."));
}

//  THE PORTAL'S WAY IN, so the ritual does not need the cable. Refusals
//  already go through refuse(), so actionRefused() paints it red by itself.
static const char *rdaSampleAction() { return sampleStart(true); }

//  REMOVE ONE SAMPLE. tune.clear wipes every one of them, which makes a single
//  bad measurement cost the whole calibration - and on a machine where a
//  measurement can be wrong, the eraser has to be the size of one measurement.
static const char *tuneDropAction(int slot) {
  if (slot < 0 || slot >= TUNE_MARKS)
    return refuse("that is not a sample slot - they are numbered 0 to 11");
  if (!(cfg.tuneUsed & (1u << slot)))
    return refuse("that slot is already empty");

  static char msg[96];
  snprintf(msg, sizeof(msg), "dropped slot %d (%u.%u MHz); %u left, curve: ",
           slot, cfg.tuneF[slot] / 10, cfg.tuneF[slot] % 10,
           (unsigned)(__builtin_popcount(cfg.tuneUsed) - 1));

  cfg.tuneP[slot]  = 0;
  cfg.tuneF[slot]  = 0;
  cfg.tuneUsed    &= (uint16_t)~(1u << slot);
  //  THE HAND CORRECTION IS NOT TOUCHED. It used to be zeroed here; the author
  //  ruled on 2026-09-25 that hand corrections are never erased automatically,
  //  only by hand (tune.nudgeZero).
  applySettings();
  settingsTouch();

  strncat(msg, tuneModelName(), sizeof(msg) - strlen(msg) - 1);
  return msg;
}

//  THE AUTOMATIC SAMPLER'S TRIGGER (autoSampleTick), and it is deliberately timid.
//
//  Everything here is a reason NOT to measure. The machine is being listened to
//  while this runs, the answer goes straight into the curve the author's dial
//  depends on, and nobody is watching the result - so every one of these gates
//  is cheaper than a wrong sample.
static const uint32_t AUTO_GAP_MS   = 300000;   // five minutes between attempts
static const uint32_t AUTO_STILL_MS = 8000;     // the dial must have settled
static uint32_t gAutoLast  = 0;
static uint32_t gDialStill = 0;

//  RUNG 2'S TRIGGER - when to go and re-index.
//
//  The author's rule, and it is better than correcting promptly:
//  keep tracking while he is actually tuning, and go the moment he stops. The
//  readout returning to the clock IS the machine noticing the hand has left the
//  knob, so the same signal that drives the display drives this.
//
//  It deliberately does NOT require the amp to be on. With the set switched off
//  the needle is parked and nobody is looking at the dial at all, which is the
//  best possible moment to spend a few seconds putting it right.
static const uint32_t REIDX_STILL_MS = 3000;
static uint32_t gReidxStill = 0;

//  THE ROUTINE INDEX CHECK. The needle is open-loop: the firmware only learns
//  that steps were lost when it crosses the index, and parking from below the
//  index never crosses it. So once per amp-off period, after two minutes, the
//  needle re-homes quietly (no sweep) and goes back where it belongs - every
//  listening session then starts from a verified zero.
static const uint32_t IDLE_CHECK_MS = 120000;
static uint32_t gAmpOffSince = 0;
static bool     gIdleChecked = false;

static void idleIndexTick() {
  if (!ampKnown || ampOn) { gAmpOffSince = 0; gIdleChecked = false; return; }
  uint32_t now = millis();
  if (!gAmpOffSince) { gAmpOffSince = now; return; }
  if (gIdleChecked || now - gAmpOffSince < IDLE_CHECK_MS) return;
  if (otaQuiet) return;
  if (!Needle::homed() || Needle::recovering() || Needle::calBusy() || sampleBusy()) return;
  //  Once per off-period - but only once it actually STARTED. A refusal (the
  //  needle busy, a sweep running) used to spend the off-period's check.
  if (Needle::startReindex(true)) gIdleChecked = true;
}

static void needleRecoverTick() {
  if (!Needle::driftPending()) { gReidxStill = 0; return; }
  if (otaQuiet) return;                          // see applyNeedleMode()
  if (Needle::recovering()) return;              // one is already running
  //  It has tried three times and said so. Repeating past that is grinding,
  //  not fixing - the remedy is a person, and the portal is telling them.
  if (Needle::recoverTries() >= 3) return;

  uint32_t now = millis();
  if (Needle::tuningActive()) { gReidxStill = 0; return; }
  if (!gReidxStill) { gReidxStill = now; return; }
  if (now - gReidxStill < REIDX_STILL_MS) return;
  if (Needle::calBusy()) return;

  gReidxStill = 0;
  Needle::startReindex();
}

static void autoSampleTick() {
  uint32_t now = millis();

  //  THE DIAL MUST BE STILL, and it must have BEEN still. A sample pairs one
  //  shaft position with a frequency measured over the following ten seconds
  //  or more; a hand still on the knob makes that pairing a fiction. The sweep
  //  itself re-checks for movement and abandons the measurement, but not
  //  starting is better than abandoning.
  if (Needle::tuningActive()) { gDialStill = 0; return; }
  if (!gDialStill) { gDialStill = now; return; }
  if (now - gDialStill < AUTO_STILL_MS) return;

  if (gSmpStage != SMP_IDLE) return;          // one measurement at a time
  if (gAutoLast && now - gAutoLast < AUTO_GAP_MS) return;
  //  The oscillator only runs when the set does, and the shaft reading has to
  //  be live or the position half of the pair is a guess.
  if (!radioLive()) return;
  if (!Needle::i2cOk()) return;
  if (!Rda::present() && !Rda::reprobe()) return;   // a lost part gets one cheap try
  if (Rda::sweeping()) return;
  //  Past the face, where the capacitor still turns: nothing to measure.
  if (!loInWindow(Needle::tuneFreq10() - ifOff10())) return;

  gAutoLast = now;
  sampleStart(false);                          // false: automatic, the higher margin
}

static void sampleTick() {
  if (gSmpStage == SMP_IDLE) return;
  if (Rda::sweeping()) return;

  //  THE KNOB MUST NOT HAVE MOVED. The frequency was measured over about ten
  //  seconds (narrow) or a minute (wide); the position is read once. If they disagree the sample is a lie
  //  in exactly the way a calibration must never be.
  int32_t drift = Needle::accumulated() - gSmpAcc;
  if (drift < 0) drift = -drift;
  if (drift > SMP_MAX_DRIFT) {
    smpSay("ABANDONED: the dial moved %ld counts while measuring - hold it still",
           (long)drift);
    gSmpStage = SMP_IDLE;
    return;
  }

  //  THE SET MUST STILL BE PLAYING THE RADIO. sampleStart() refuses to begin
  //  outside the listening policy (radioLive()) - but a
  //  measurement lasts from about ten seconds to over a minute, and switching the set off (or the source
  //  away) during one is ordinary use with an auto sample every five minutes.
  //  A sweep whose oscillator died with the set was still judged, and a mono
  //  broadcast or noise could be stored as a sample. Leaving
  //  RADIO does not stop the oscillator, but it is outside the listening policy
  //  and is discarded the same way. Checked at every stage, before anything is
  //  kept.
  if (!radioLive()) {
    smpSay("DISCARDED: the set was switched off or left RADIO during the measurement - nothing stored");
    gSmpStage = SMP_IDLE;
    return;
  }

  //  THE BUS MUST HAVE BEEN CLEAN FOR THE WHOLE RUN, or nothing is stored. A
  //  point that failed on I2C is recorded as rssi 0, which drags the local
  //  floors down until noise scores like the oscillator - and a sample stored
  //  from that is a wrong frequency in the curve, shown as right. Checked
  //  BEFORE anything is committed, for every stage. The
  //  automatic sampler simply tries again at its next turn; the ritual says so.
  //  A lost part has its own message, below.
  if (!Rda::lost() && Rda::runFailures()) {
    smpSay("DISCARDED: %u I2C error%s on the RDA bus during the measurement - "
           "nothing stored%s", (unsigned)Rda::runFailures(),
           Rda::runFailures() == 1 ? "" : "s",
           gSmpRitual ? "; press measure again" : "; it will retry by itself");
    gSmpStage = SMP_IDLE;
    return;
  }

  //  THE FINE PASS HAS COME BACK. Use it if it found anything, and fall back to
  //  the coarse answer if it did not - a refinement that fails must never cost
  //  a measurement that succeeded.
  if (gSmpStage == SMP_FINE && Rda::lost()) {
    //  Lost DURING the refinement: said as that, not as "found nothing".
    smpSay("ABANDONED: the RDA5807M stopped answering during the 50 kHz pass - nothing stored");
    gSmpStage = SMP_IDLE;
    return;
  }
  if (gSmpStage == SMP_FINE) {
    int16_t f20 = Rda::fineF20();
    gSmpStage = SMP_IDLE;
    if (f20 == 0) {
      smpSay("the 50 kHz pass found nothing; keeping the 100 kHz answer");
      sampleStore((int16_t)(gSmpCoarse10 * 2), gSmpMargin);
    } else {
      sampleStore(f20, gSmpMargin);
    }
    return;
  }

  //  THE PART WENT AWAY MID-MEASUREMENT. Said as what it is: the bins of a
  //  sweep that lost the RDA are not a spectrum, and "nothing found - a fixed
  //  feature" was the wrong cause to send him looking for.
  if (Rda::lost()) {
    smpSay("ABANDONED: the RDA5807M stopped answering mid-measurement (%u failed transfers) - "
           "not a fixed feature; check its connector", (unsigned)Rda::busFailures());
    gSmpStage = SMP_IDLE;
    return;
  }

  uint8_t margin = 0;
  int c = Rda::bestCandidate(&margin);
  if (c >= 0 && margin >= (gSmpRitual ? SMP_MIN_MARGIN : SMP_AUTO_MARGIN)) {
    //  LOCATED. Now pin it down: the 100 kHz grid cannot say which side of a
    //  channel boundary the tuning capacitor is on, and that is the difference
    //  between the panel showing 98.5 and 98.7.
    gSmpCoarse10 = Rda::binFreq10((uint8_t)c);
    gSmpMargin   = margin;

    //  REFUSE A CANDIDATE WHOSE REAL PEAK MAY BE HIDDEN IN A NOTCH. isSpur()
    //  blinds +/-2 bins, so when the tube set's oscillator sits inside a notch
    //  the search lands on the first bin outside it and the stored station is up
    //  to 0.1 MHz off, silently - SMP_MAX_DISAGREE10 is 4.0 MHz wide. Measured
    //  2026-09-23: 100.7 needs LO 90.1, inside the 90.3 notch; the search took
    //  90.0 and stored 100.6, twice.
    //
    //  BUT NOT ON DISTANCE ALONE. The first version refused every candidate 3
    //  bins from a feature, and on its first boot refused 98.5 - a hand mark:
    //  its peak straddles 87.8/87.9, the search took 87.8, 3 bins from 87.5.
    //  The sweep DOES measure the notched bins; it only will not choose them.
    //  So look at the neighbour on the notch side: if it is at least as strong
    //  as the candidate, the peak may be in there and we cannot know - refuse.
    //  If it is weaker, the candidate is a genuine local peak outside the notch.
    int16_t sp = Rda::besideSpur(gSmpCoarse10);
    if (sp) {
      int16_t nf = (int16_t)(gSmpCoarse10 + (sp > gSmpCoarse10 ? 1 : -1));
      uint8_t cr = Rda::binRssi((uint8_t)c), nr = 0;
      bool    seen = false;
      for (uint8_t i = 0; i < Rda::binCount(); i++)
        if (Rda::binFreq10(i) == nf) { nr = Rda::binRssi(i); seen = true; break; }
      //  Not swept (the edge of a narrow window): no evidence either way, refuse.
      if (!seen || nr >= cr) {
        smpSay("REFUSED: the oscillator reads %d.%d, beside the fixed feature at %d.%d, and "
               "the bin inside that notch is as strong - the real peak may be hidden there. "
               "Move the dial 0.3 MHz and retry.",
               gSmpCoarse10 / 10, gSmpCoarse10 % 10, sp / 10, sp % 10);
        gSmpStage = SMP_IDLE;
        return;
      }
    }

    if (Rda::requestRefine(gSmpCoarse10)) {
      gSmpStage = SMP_FINE;
      smpSay("found it near %d.%d - pinning it down at 50 kHz (about 4 s)",
             gSmpCoarse10 / 10, gSmpCoarse10 % 10);
    } else {
      gSmpStage = SMP_IDLE;
      sampleStore((int16_t)(gSmpCoarse10 * 2), margin);
    }
    return;
  }

  if (gSmpStage == SMP_NARROW) {
    //  Nothing convincing nearby. Either the curve's prediction is further out
    //  than 1.5 MHz, or the oscillator is sitting under one of the learned spur
    //  notches - seven of them, +/-0.2 MHz each, about a sixth of the window.
    //  The whole band, once, before giving up.
    smpSay("nothing convincing within 1.5 MHz (best +%u) - sweeping the whole window (about a minute)",
           (unsigned)margin);
    //  Ask first, raise the stage second - see the note in sampleStart().
    if (Rda::requestSweep(Rda::LO_WINDOW_LO10, Rda::LO_WINDOW_HI10, 1, 300)) {
      gSmpStage = SMP_WIDE;
    } else {
      smpSay("the RDA refused the wide sweep");
      gSmpStage = SMP_IDLE;
    }
    return;
  }

  smpSay("NOTHING FOUND in the whole window (best +%u, needs +%u) - the dial may be "
         "sitting under a learned fixed feature; nudge it and retry",
         (unsigned)margin, (unsigned)(gSmpRitual ? SMP_MIN_MARGIN : SMP_AUTO_MARGIN));
  gSmpStage = SMP_IDLE;
}

// ---------------------------------------------------------------------------
static void updateDisplay() {
  if (dispTest == 1) { int8_t d[4] = { 8, 8, 8, 8 }; Display::showDigits(d); return; }
  if (dispTest == 2) { Display::blank(); return; }
  //  Mode 2: the readout appears under your hand and goes away again. The dial
  //  sensor is what says "tuning is happening", so with the AS5600 absent this
  //  mode simply never triggers and the clock stays - which is the right
  //  failure, not a wrong reading.
  //  MODE 2 ONLY, and that is the author's call. The AUTOMATIC
  //  readout obeys the amp and the source; mode 1 ("Tuning always") does not,
  //  because it is an explicit operator override and it is the instrument used
  //  to calibrate the shaft against the printed dial - a job done standing at
  //  the set with the amp off. Gating it would have left the portal reading
  //  back "Tuning always" and the console printing "display shows TUNING" while
  //  the panel showed a clock, which is the silent-instrument-removal this file
  //  already argues against where the portal readout refuses to clamp.
  //
  //  AND THE DWELL TIMESTAMP IS DELIBERATELY NOT CLEARED while the radio is not
  //  live. A first version did clear it. Clearing only has an effect for
  //  interruptions SHORTER than tuneHoldMs - anything longer has expired on its
  //  own clock - and the short ones on this machine are a brief source-reading
  //  glitch (the A32's readSource() is an undebounced median of 5) or a ~2 s
  //  link hiccup. Either would wipe a
  //  dwell the author is in the middle of reading, and it would not come back
  //  until he touched the knob again. The case a reset would protect against -
  //  a stale dwell surviving the amp coming back on - cannot happen, because an
  //  amp that has been off has been off far longer than four seconds.
  bool wantTuning = (cfg.showTuning == 1);
  if (cfg.showTuning == 2) {
    static uint32_t lastTurn = 0;
    if (Needle::tuningActive()) lastTurn = millis();
    wantTuning = radioLive() && lastTurn && (millis() - lastTurn < cfg.tuneHoldMs);
  }

  if (wantTuning) {
    //  Tuning readout: the tuner shaft's angle (AS5600) through the tuning
    //  curve, in tenths of a MHz. Shown as an integer - 1017 means 101.7 MHz, since the panel has
    //  no decimal point. This exists mainly to calibrate the shaft against the
    //  dial, which is why it is a setting rather than an automatic mode.
    //  ONE definition of the mapping - see needle.h. This used to rebuild the
    //  straight line from bandLow/bandHigh here, which was one of six copies.
    //
    //  CLAMPED TO THE PRINTED FACE (dialLow..dialHigh), applied to the
    //  unrounded value below - unlike the portal, which shows the true value
    //  and flags it. The tuner runs past both ends of the FM band on this set;
    //  the Leditron shows the band and nothing else, pinning at the ends
    //  rather than displaying a frequency that has no station and no mark on
    //  the dial.
    //  SNAP TO THE FM CHANNEL GRID - author's call: "if I'm at 91.3
    //  and up the knob it should go from that to 91.5, not 91.4."
    //
    //  In the Americas FM carriers sit on ODD tenths of a MHz - 87.9, 88.1, ...
    //  107.9 - 200 kHz apart, so an even tenth is not a station at all. The
    //  needle still moves continuously and the PORTAL still reports the true
    //  unrounded frequency; this is the four-digit panel only.
    //
    //  SNAPPED FROM THE UNROUNDED VALUE, and that is the whole point. The rule
    //  used to be "even tenth -> next channel up", justified because the old
    //  frequency came from truncating integer division: the true value lay in
    //  [f, f+1) and f+1 was always the nearer channel. The tuning curve returns
    //  a value rounded to NEAREST instead, which moves the bucket to
    //  [f-0.5, f+0.5) - and there f-1 is nearer over the whole lower half. A
    //  shaft at a true 91.36 would have shown 91.5, when 91.3 is four times
    //  closer: one full channel wrong, on a quarter of the dial, on the readout
    //  this entire round of work exists to make trustworthy. Snapping the raw
    //  float has no tie-break to get wrong.
    float ft = Needle::tuneFreq10f();
    if (ft < (float)cfg.dialLow)  ft = (float)cfg.dialLow;
    if (ft > (float)cfg.dialHigh) ft = (float)cfg.dialHigh;

    int32_t f;
    if (cfg.showTuning == 2) {
      //  Nearest odd tenth: the grid is 879 + 2k.
      f = 879 + 2 * (int32_t)lroundf((ft - 879.0f) / 2.0f);
      if (f > (int32_t)cfg.dialHigh) f -= 2;   // ...but never off the printed face
      if (f < (int32_t)cfg.dialLow)  f += 2;
    } else {
      //  MODE 1 IS THE INSTRUMENT and is deliberately not snapped: it is what
      //  the shaft is calibrated against, and rounding it would hide the very
      //  tenth of a MHz you are there to measure.
      f = (int32_t)lroundf(ft);
    }
    Display::showNumber((uint16_t)f, false);
    return;
  }

  if (!timeValid) {
    //  Better a visible placeholder than a plausible wrong time.
    int8_t d[4] = { -1, -1, -1, -1 };
    Display::showDigits(d);
    return;
  }

  //  The timezone is set ONCE, by Net::begin(), from the stored POSIX rule.
  //  It used to be forced here on every redraw, which would have quietly
  //  overridden whatever the portal was told to use.
  time_t   t = (time_t)nowEpoch();
  struct tm tmv;
  localtime_r(&t, &tmv);
  Display::showTime(tmv.tm_hour, tmv.tm_min, cfg.hour12, cfg.blankLeadZero);
}

// ---------------------------------------------------------------------------
//  CONSOLE PROMPTS: the network (y) and the clock (W).
// ---------------------------------------------------------------------------
//  Set the network over the cable. This exists for the same reason the '~'
//  account reset does: the portal is the normal way in, and there has to be one
//  route back when the portal itself is the thing that is unreachable.
//  WHAT A PROMPT DID NOT READ IS THROWN AWAY, not left for the dispatcher. A
//  line longer than the prompt's buffer used to leave its tail in the console
//  queue, and every character of it then ran as a one-key command - m and c
//  overwrite calibrations, j jogs without limits. Everything
//  up to the end of the line goes; a key typed after it is left alone.
static void drainLine() {
  uint32_t t0 = millis();
  while (millis() - t0 < 50) {
    if (!Con.available()) { delay(2); continue; }
    char c = Con.read();
    if (c == '\r' || c == '\n') break;
  }
}

static void setWifiInteractive() {
  Con.print(F("  ssid <space> password  (blank line cancels) > "));
  //  98: the longest legal pair is a 32-byte SSID, a space and a 63-byte WPA
  //  passphrase - 96 - and the old 96-byte buffer read only 95 of it.
  char buf[98]; size_t n = 0;
  uint32_t t0 = millis();
  Con.lineWanted = true;                  // see console.h
  while (millis() - t0 < 60000 && n < sizeof(buf) - 1) {
    if (!Con.available()) { gLink.poll(); wdtFeed(); delay(5); continue; }
    char c = Con.read();
    if (c == 13 || c == 10) { if (n) break; else continue; }
    buf[n++] = c; Con.print(c); t0 = millis();
  }
  Con.lineWanted = false;
  drainLine();
  buf[n] = 0;
  Con.println();
  if (!n) { Con.println(F("  cancelled.")); return; }

  char *sp = strchr(buf, ' ');
  if (!sp) { Con.println(F("  need a password after the ssid.")); return; }
  *sp = 0;
  Net::setWifi(String(buf), String(sp + 1));
  Con.printf("  stored. joining %s ...", buf);
  Con.println();
}

//  Manual clock set (W) - the route by hand; NTP sets the clock whenever WiFi
//  reaches the internet. Typed as LOCAL time, because that is what a person
//  reads off a watch. It is converted to UTC here and sent to the A32 as UTC
//  for the battery clock - keeping local time in an RTC makes the hour after a
//  DST change ambiguous and the hour before it happen twice.
static void setClockInteractive() {
  Con.print(F("  local time as YYYY-MM-DD HH:MM:SS > "));
  char buf[32]; size_t n = 0;
  uint32_t t0 = millis();
  Con.lineWanted = true;                  // see console.h
  while (millis() - t0 < 40000 && n < sizeof(buf) - 1) {
    if (!Con.available()) { gLink.poll(); wdtFeed(); delay(5); continue; }
    char c = Con.read();
    if (c == '\r' || c == '\n') { if (n) break; else continue; }
    buf[n++] = c; Con.print(c); t0 = millis();
  }
  Con.lineWanted = false;
  drainLine();
  buf[n] = 0;
  Con.println();

  struct tm tmv = {};
  int y, mo, d, h, mi, s;
  if (sscanf(buf, "%d-%d-%d %d:%d:%d", &y, &mo, &d, &h, &mi, &s) != 6) {
    Con.println(F("  could not parse that. Nothing changed."));
    return;
  }
  tmv.tm_year = y - 1900; tmv.tm_mon = mo - 1; tmv.tm_mday = d;
  tmv.tm_hour = h; tmv.tm_min = mi; tmv.tm_sec = s;
  tmv.tm_isdst = -1;                       // let the zone rules decide

  //  BUILDERS, PERSONALISE THIS. The zone is hard-coded to this radio's home
  //  (Montréal, EST5EDT) and ignores the zone set in the portal - here only;
  //  NTP and the portal clock use the portal's setting (net.cpp). On a set
  //  living in another zone the time typed here is read, and then shown, in
  //  the wrong zone. Put your own POSIX TZ string here, or read Net's zone.
  //  Left as is on purpose.
  setenv("TZ", "EST5EDT,M3.2.0,M11.1.0", 1); tzset();
  time_t e = mktime(&tmv);                 // interprets as LOCAL, yields UTC
  if (e < 1600000000L) { Con.println(F("  that is not a plausible date.")); return; }

  ProtoTime pt = {};
  pt.unixUtc = (uint32_t)e;
  pt.valid   = 1;
  epochAtSync = (uint32_t)e; millisAtSync = millis(); timeValid = true;
  //  ONLY CLAIM THE PUSH WHEN SOMEONE IS THERE TO RECEIVE IT (2026-09-25).
  if (peerHello) {
    gLink.send(MSG_SET_TIME, pt);
    Con.println(F("  clock set, and pushed to the DS3231."));
  } else {
    Con.println(F("  clock set on this board only - the audio board is not answering,"));
    Con.println(F("  so the battery clock was NOT updated. Set it again once the link is back."));
  }
}

// ---------------------------------------------------------------------------
//  LIMIT MONITOR (console `l`). The name is historical: there is one input
//  left, the index sensor (Bible §11). A live view of it, four times a second;
//  the monitor drives nothing itself (the needle keeps doing what it was
//  doing), so the sensor and its magnet can be proven by hand.
//
//  What to look for, in order:
//    1. it reads `off` with no magnet near
//    2. the magnet makes it read *ON*
//    3. one FACE of the magnet works, the other does not -> normal, unipolar
//
//  Point 3 is the one that catches people out. The Hall switch answers to ONE
//  pole only; a magnet that does nothing may simply be the wrong way round.
// ---------------------------------------------------------------------------
//  USABLE FROM THE PORTAL (2026-09-25). The portal sends every key with an
//  Enter after it, and "any key stops" took that Enter as the stop - so `l`
//  from the Console tab ended at once. Enter is ignored now; any other key
//  stops, and so does a minute, because loop() waits here.
static void limitMonitor() {
  Con.println(F("  Live index. Wave the magnet past the A3144. Any key stops (a minute at most)."));
  Con.println(F("  The needle keeps doing what it was doing."));
  uint32_t t0 = millis();
  while (millis() - t0 < 60000) {
    bool stop = false;
    while (Con.available()) {
      char c = Con.read();
      if (c != '\r' && c != '\n') { stop = true; break; }
    }
    if (stop) break;
    Con.print(F("  index  "));
    Con.println(Needle::indexNow() ? F("*ON*") : F("off "));
    //  KEEP THE LINK ALIVE. Since the A32 started treating two seconds of
    //  silence as "amp down" (a32/main.cpp, AWAKE/ASLEEP), any console loop
    //  that stops calling loop() also mutes the radio. Receiving is not
    //  enough - the A32 listens for US - so a ping goes out every pass.
    gLink.send(MSG_PING);
    wdtFeed();
    for (int i = 0; i < 5; i++) { gLink.poll(); delay(50); }
  }
  while (Con.available()) Con.read();
  Con.println();
}

//  Printed at the end of every `s`. This board is on native USB and does not
//  reset when a monitor attaches, so anything shown only at boot is never seen
//  in practice - the command list has to be reachable on demand.
static void commandList() {
  Con.println(F("  ------------------------------------------------"));
  Con.println(F("  DISPLAY   8 show 8888    b blank    9 back to normal"));
  Con.println(F("            - = brightness            f clock/tuning readout"));
  Con.println(F("            W set the clock (local time, YYYY-MM-DD HH:MM:SS)"));
  Con.println(F("  NETWORK   i what the portal is on   y set the wifi   Y force the AP up"));
  Con.println(F("            ~ reset the owner account (forgotten password)"));
  Con.println(F("  NEEDLE    H home   T track   P park   k calibrate index"));
  Con.println(F("            n N nudge -/+20 half-steps, slow"));
  Con.println(F("            m M set soft limit LOW/HIGH to here"));
  Con.println(F("            e index edge logging on/off"));
  Con.println(F("            r sweep full travel (repeats)   x STOP"));
  Con.println(F("  CALIBRATE k band  (needle ON the sensor first)"));
  Con.println(F("            A abort a running calibration"));
  Con.println(F("  SETTINGS  D dump all settings as text"));
  Con.println(F("            l live index monitor"));
  Con.println(F("  DRIVE     1 2 3 4 hold one coil       0 release coils"));
  Con.println(F("            j J 200 half-steps fwd/rev, HALF mode"));
  Con.println(F("            u U 200 half-steps fwd/rev, MICRO mode"));
  Con.println(F("  TUNER     c set cal LOW   C set cal HIGH"));
  Con.println(F("  RDA5807M  R state and where the LO should be"));
  Con.println(F("            q wide sweep (~1 min)  v narrow sweep at the prediction (~10 s)"));
  Con.println(F("            a print the last sweep   o mark the peak as fixed   O forget them"));
  Con.println(F("  Z zero the jitter / slot-error counters"));
  Con.println(F("  z erase the RF calibration and reboot (transmitter diagnosis)"));
  Con.println(F("  B step the transmit power down one level (transmitter diagnosis)"));
  Con.println(F("  V measure this dial position with the RDA and store it as a sample"));
  Con.println(F("  s status    ? this list"));
  Con.println(F("  ------------------------------------------------"));
}

//  There used to be a second list here, help(), printed once at boot. It had
//  drifted from the dispatcher - it advertised 'd' (no handler), 'a A' as
//  acceleration (they print the last sweep and abort a calibration) and ', .'
//  as max speed. Two copies of one list is how that happens, so there is one.

static void status() {
  Con.println(F("  ------------------------------------------------"));
  Con.printf("  amp        : %s\n", ampKnown ? (ampOn ? "ON" : "OFF") : "unknown");
  Con.printf("  source     : %s\n", !haveState ? "?" :
                a32.source == SRC_AUX ? "AUX" : a32.source == SRC_BT ? "BT" : "RADIO");
  Con.printf("  needle     : %-14s pos %5ld -> %5ld  %s\n",
                Needle::stateName(), (long)Needle::position(),
                (long)Needle::target(), Needle::homed() ? "homed" : "NOT HOMED");
  Con.printf("  index      : %s  crossings %lu  last drift %ld half-steps%s\n",
                Needle::indexNow() ? "ON " : "off",
                (unsigned long)Needle::indexCrossings(), (long)Needle::lastDrift(),
                Needle::driftPending() ? "  *** SLIP OUTSTANDING - re-home (H) ***" :
                (Needle::driftMeasured() ? "" : "  (not measured since the home)"));
  Con.printf("  drive      : PWM %.0f Hz %s   mode %u   micro 1/%u\n",
                Drive::pwmHz(),
                Drive::pwmHz() == 0 ? "*** LEDC REFUSED ***" : "ok",
                Drive::mode(), Drive::micro());
  Con.print(F("               duty resolution "));
  Con.print(Drive::bits());
  Con.println(F("-bit"));
  Con.printf("  motion     : jitter %lu us  steps %lu  vel %.0f hsps\n",
                (unsigned long)Needle::jitterMaxUs(),
                (unsigned long)Needle::stepsEmitted(), Needle::velHsps());
  //  THE WHOLE TUNING CHAIN, because every number is downstream of the one
  //  above it and reading them apart is what hid this for two evenings.
  //
  //  AGC: on a 3.3 V part the range is 0-128, NOT 0-255 (AS5600 datasheet
  //  v1-06, AGC register description). So 128 is MAXIMUM gain - the weakest
  //  field the part can still work with - and the target is about 64.
  //  Reading 128 as 'mid-scale' is exactly backwards.
  //
  //  STATUS raw, because 'NO MAGNET' collapses MD/ML/MH into one boolean and
  //  they need different fixes: MD clear is no magnet, ML is too far away.
  //  MAGNITUDE is the field strength itself rather than the gain chasing it.
  //
  //  The residue (accumulator - rawAngle) & 4095 is INVARIANT by
  //  construction: seedAccumulator sets accAngle = raw + k*4096 and every
  //  update preserves it. If it ever changes, the accumulator has swallowed
  //  or invented a revolution and every calibration built on it is worthless.
  Con.printf("  AS5600 raw : status 0x%02X (MD %c ML %c MH %c)  AGC %u/128"
                "  magnitude %u\n",
                Needle::statusByte(),
                (Needle::statusByte() & 0x20) ? 'Y' : 'n',
                (Needle::statusByte() & 0x10) ? 'Y' : 'n',
                (Needle::statusByte() & 0x08) ? 'Y' : 'n',
                Needle::agc(), Needle::fieldMagnitude());
  {
    //  THE SIXTH SITE. needle.h said the mapping was computed in five places;
    //  it was six, and this one - inside the block the console calls THE WHOLE
    //  TUNING CHAIN - was the one left rebuilding the straight line by hand.
    //  It would have printed a megahertz and a half away from the residual
    //  table six lines below it.
    uint16_t pm = Needle::tunePermille();
    int32_t  f  = Needle::tuneFreq10();
    Con.printf("  tune chain : permille %u -> %ld.%ld MHz   acc %ld  raw %u"
                  "  residue %ld\n",
                  pm, (long)(f / 10), (long)(f % 10),
                  (long)Needle::accumulator(), Needle::rawAngleNow(),
                  (long)((Needle::accumulator() - (int32_t)Needle::rawAngleNow()) & 4095));
  }
  Con.printf("  AS5600     : raw %4u  %s  AGC %u  %s  tuning %s\n",
                Needle::angleRaw(), Needle::magnetOk() ? "magnet OK" : "NO MAGNET",
                Needle::agc(), Needle::i2cOk() ? "bus ok" : "BUS FAIL",
                Needle::tuningActive() ? "yes" : "no");
  //  %ld, NOT %u. These are int32_t and the high one is legitimately NEGATIVE
  //  on this set - the accumulator counts DOWN as frequency goes up - so %u
  //  printed -1173 as 4294966123 and the one line that is supposed to explain
  //  the tuner was unreadable exactly when it mattered.
  {
    uint8_t n = tuneSampleCount();
    //  THE RESIDUALS ARE ZERO BY CONSTRUCTION AT EXACTLY THREE SAMPLES - a
    //  quadratic through three points interpolates them. They only start to
    //  mean anything at four or more, which is what the RDA5807M samples provide.
    //  Until then this table proves what was STORED, not that it was right.
    char tl[160]; tuneStateLine(tl, sizeof(tl), nullptr);
    Con.printf("  tune curve : %s", tl);

    if (cfg.tuneOffset10) Con.printf("  offset %+d tenths", (int)cfg.tuneOffset10);
    Con.println();
    for (int i = 0; i < TUNE_MARKS; i++) {
      if (!(cfg.tuneUsed & (1u << i))) continue;
      int32_t got = Needle::tuneFreq10At(cfg.tuneP[i]);
      Con.printf("               mark %c  acc %6ld  said %u.%u  curve %ld.%ld  (%+ld)\n",
                    i < 3 ? ('A' + i) : ('a' + i - 3), (long)cfg.tuneP[i],
                    cfg.tuneF[i] / 10, cfg.tuneF[i] % 10,
                    (long)(got / 10), (long)(got % 10),
                    (long)(got - (int32_t)cfg.tuneF[i]));
    }
  }
  Con.printf("  cal        : low %ld  high %ld  (span %ld)  -> %u permille\n",
                (long)cfg.calLow, (long)cfg.calHigh,
                (long)((int32_t)cfg.calHigh - (int32_t)cfg.calLow),
                Needle::tunePermille());
  {
    int32_t lo, hi;
    Needle::getGeometry(lo, hi);
    Con.printf("  soft limits: %ld .. %ld half-steps (NO physical stops)\n",
                  (long)lo, (long)hi);
  }
  {
    uint8_t m = 0;
    int cand = Rda::bestCandidate(&m);
    Con.printf("  RDA5807M   : %s%s\n", Rda::state(),
                  Rda::present() ? "" : "   *** module not answering ***");
    if (Rda::binCount()) {
      Con.printf("               floor %u   best candidate ", (unsigned)Rda::chipRssiFloor());
      if (cand < 0) Con.println(F("none above the local floor"));
      else {
        int16_t f = Rda::binFreq10((uint8_t)cand);
        Con.printf("LO %d.%d -> station %d.%d MHz  (+%u over local)\n",
                      f / 10, f % 10, (f + ifOff10()) / 10,
                      (f + ifOff10()) % 10, (unsigned)m);
      }
    }
    if (Rda::spurCount()) {
      Con.print(F("               known fixed features:"));
      for (uint8_t i = 0; i < Rda::spurCount(); i++)
        Con.printf(" %d.%d", Rda::spurFreq10(i) / 10, Rda::spurFreq10(i) % 10);
      Con.println();
    }
  }
  Con.printf("  panel      : %u -> %u\n", Panel::level(), Panel::target());
  Con.printf("  display    : brightness %u  worst slot error %lu us\n",
                Display::brightness(), (unsigned long)Display::worstSlotErrorUs());
  Con.printf("  time       : %s", timeValid ? "" : "NOT SET\n");
  if (timeValid) {
    time_t t = (time_t)nowEpoch(); struct tm tmv; localtime_r(&t, &tmv);
    Con.printf("%04d-%02d-%02d %02d:%02d:%02d local\n",
                  tmv.tm_year + 1900, tmv.tm_mon + 1, tmv.tm_mday,
                  tmv.tm_hour, tmv.tm_min, tmv.tm_sec);
  }
  if (haveState)
    Con.printf("  source     : %u   raw ladder %4u   (AUX < %u <= BT < %u <= RADIO)\n",
                  a32.source, a32.rawLadder,
                  (unsigned)A32_MODE_THRESH_AUX_BT, (unsigned)A32_MODE_THRESH_BT_RADIO);
  Con.printf("  portal     : loops %lu  served %lu  stack free %lu bytes\n",
                (unsigned long)Portal::loops(), (unsigned long)Portal::served(),
                (unsigned long)Portal::stackFreeBytes());
  //  WRONG VERSION IS NOT A DEAD WIRE (2026-09-25). The framer drops a frame
  //  of another PROTO_VERSION before anything else sees it, so a half-updated
  //  pair read "SILENT, rx 0" - exactly a broken cable. Counted, now shown.
  Con.printf("  link       : %s  rx %lu  crc %lu  wrong-version %lu%s\n",
                gLink.peerAlive() ? "alive" : "SILENT",
                (unsigned long)gLink.rxCount(), (unsigned long)gLink.badCrc(),
                (unsigned long)gLink.badVer(),
                gLink.badVer() ? "  <- the boards run different protocol versions: flash both" : "");

  //  WHY DID IT LAST RESET. On native USB the boot banner is never seen, so a
  //  reboot loop is invisible unless the reason is retrievable afterwards.
  //  TASK_WDT means a watched task (loop, the needle supervisor) stalled for
  //  15 s or a task starved an idle task; BROWNOUT means the chip's supply
  //  dropped below its brownout threshold; PANIC means a crash. They need
  //  completely different fixes and guessing between them wastes an evening.
  {
    const char *r;
    switch (esp_reset_reason()) {
      case ESP_RST_POWERON:  r = "POWERON (normal cold start)"; break;
      case ESP_RST_EXT:      r = "EXT (reset pin)";             break;
      case ESP_RST_SW:       r = "SW (software restart)";       break;
      case ESP_RST_PANIC:    r = "*** PANIC (crash) ***";       break;
      case ESP_RST_TASK_WDT: r = "*** TASK WATCHDOG ***";       break;
      case ESP_RST_INT_WDT:  r = "*** INTERRUPT WATCHDOG ***";  break;
      case ESP_RST_WDT:      r = "*** OTHER WATCHDOG ***";      break;
      case ESP_RST_BROWNOUT: r = "*** BROWNOUT - the supply voltage dipped ***"; break;
      case ESP_RST_DEEPSLEEP: r = "deep sleep";                 break;
      default:               r = "unknown";                     break;
    }
    Con.printf("  last reset : %s   uptime %lu s\n", r, (unsigned long)(esp_timer_get_time() / 1000000LL));
  }
  imageReport();
  commandList();
}

void setup() {
  //  FIRST, so a hang anywhere below resets - see the ROLLBACK banner.
  esp_task_wdt_init(WDT_TIMEOUT_S, true);
  enableLoopWDT();
  gLoopWdtOn = (esp_task_wdt_status(NULL) == ESP_OK);
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
  Con.begin(115200);
  uint32_t t0 = millis();
  while (!Con && millis() - t0 < 3000) delay(10);
  delay(300);

  Con.println();
  Con.println(F("======================================================"));
  Con.println(F("  AMBERSONG  -  MAIN MCU (ESP32-S3)"));
  Con.printf ("  firmware %s (%s)\n", FW_VERSION, FW_COMMIT);
    Con.println(F("======================================================"));

  pinMode(S3_AMP_SENSE, INPUT);      // loop() reads LOW = amp powered (the detector: Bible §7)

  settingsLoad();
  purgeCorruptLimits();
  purgeCorruptSpurs();

  //  A FAILED SELF-TEST NO LONGER HALTS. Halting here, before WiFi and the
  //  portal, left a board that only USB could reach. Now it runs on, the image
  //  is never confirmed (confirmTick), and the next restart - a portal reboot
  //  will do - takes the previous image back.
  if (!protoSelfTest(Con)) {
    gProtoBroken = true;
    Con.println(F("  [FAIL] proto self-test - this image will NOT be confirmed;"));
    Con.println(F("         reboot to roll back, or upload a good build."));
  }

  Display::begin();
  Panel::begin();
  Needle::begin();
  //  The tuning auto-calibration instrument (Firmware Gospel §6). Harmless if the module
  //  is absent - begin() records that and every command says so.
  Rda::begin();
  //  Recover which TURN the shaft is on. The sensor knows the angle exactly and
  //  nothing about the revolution, so the last saved position is the only clue.
  //  The measured tuner ends FIRST: the seed uses them to pick the turn the
  //  shaft is on (seatTurn in needle.cpp) - applySettings() below is later.
  Needle::setCalibration(cfg.calLow, cfg.calHigh, cfg.tunerEndsSet == ENDS_BOTH);
  Needle::seedAccumulator(cfg.lastAngle);
  applySettings();

  gLink.onMessage(onMessage);
  gLink.begin(Serial2, S3_UART_RX, S3_UART_TX);

  Con.printf("  AS5600 %s, magnet %s, AGC %u\n",
                Needle::i2cOk() ? "responding" : "NOT RESPONDING",
                Needle::magnetOk() ? "ok" : "MISSING OR MISALIGNED", Needle::agc());

  brightnessNow(cfg.brightOff);   // come up at standby, then ease to whatever
  applySettings();                //  the amp turns out to be doing

  //  THE BOOT HOMING, ending in the full sweep. After this the position is kept
  //  honest by the index crossings; homing runs again only to recover (a slip,
  //  a failed home, the idle check) or when asked.
  Con.println(F("  finding the index."));
  //  WHERE TO SEARCH FROM. After a power-up the needle parked at the low stop
  //  when the set was last switched off. After a SOFTWARE restart - a portal
  //  reboot, an OTA, the console reset, a crash - it is wherever it was, and
  //  bootPosition() remembers that from RTC memory; assuming the low stop there
  //  sent the search up into the high stop whenever the radio had been playing
  //  above the index (2026-09-25). Order matters: applySettings() above has
  //  already given the needle its geometry.
  Needle::bootPosition();
  Needle::startHoming();
  Net::begin();
  Portal::begin();

  imageReport();
  commandList();
}

void loop() {
  gLink.poll();
  Net::loop();
  confirmTick(millis());
  //  THE LEARNED RUNG COMES BACK. net.cpp owns the live transmit power while it
  //  is hunting; cfg is the record. Copying it here, every pass, keeps the two
  //  within a millisecond of each other - which matters because applySettings()
  //  pushes cfg INTO net and runs dozens of times a session, so a slow sync
  //  would let an unrelated slider move stamp a stale rung over a working one.
  //  Same two-owners shape as the purge that once fired mid-edit.
  if (Net::txPower() != cfg.wifiTxQ) { cfg.wifiTxQ = Net::txPower(); settingsTouch(); }
  sampleTick();
  autoSampleTick();
  needleRecoverTick();
  idleIndexTick();
  settingsFlush();
  updateBrightness();
  uint32_t now = millis();

  // --- amp sense, and everything that hangs off it -------------------------
  {
    static uint32_t lastSys = 0;
    static uint32_t debounce = 0;
    bool amp = (digitalRead(S3_AMP_SENSE) == LOW);
    if (amp != ampOn || !ampKnown) {
      if (!debounce) debounce = now;
      if (now - debounce > 50) {
        bool wasOn = ampOn, wasKnown = ampKnown;
        ampOn = amp; ampKnown = true; debounce = 0;
        Con.printf("  amp %s\n", ampOn ? "ON" : "OFF");
        brightnessTarget(ampOn ? cfg.brightOn : cfg.brightOff);
        //  NO HOMING HERE ANY MORE. The needle knows where it is: it has been
        //  sitting at the low stop since the set was last switched off, and any
        //  index crossing since has corrected it. The switch only decides where
        //  it should now point - and from the low stop to a station IS the
        //  sweep, so nothing is lost visually.
        //
        //  The exception is a set that never found its zero: if homing failed
        //  at boot, the front switch is the natural place to try again.
        //  NOT DURING AN UPLOAD: the flash write stalls the
        //  step timing, so the needle holds still until it is over. A failed
        //  upload is caught by otaResume, which homes an unhomed needle; a good
        //  one reboots, which homes anyway.
        if (otaQuiet) {
          //  nothing - see above
        } else if (ampOn && wasKnown && !wasOn && !Needle::homed()) {
          Con.println(F("  amp came up and the needle has no zero -> homing."));
          Needle::startHoming();
        } else if (ampOn && wasKnown && !wasOn && cfg.sweepOn) {
          Con.println(F("  amp came up -> full sweep (setting)."));
          Needle::sweepRange(false);
        }
        applyNeedleMode();
        lastSys = 0;      // push the new state immediately
      }
    } else debounce = 0;

    if (peerHello && (now - lastSys >= 2000 || lastSys == 0)) {
      lastSys = now;
      ProtoSys s = {};
      s.ampOn = ampOn ? 1 : 0;
      gLink.send(MSG_SET_SYS, s);
    }
  }

  // --- link keepalive ------------------------------------------------------
  {
    static uint32_t lastHello = 0, lastPing = 0, lastTime = 0;
    if (!peerHello && now - lastHello >= 1000) {
      lastHello = now;
      ProtoHello h = {};
      h.protoVersion = PROTO_VERSION;
      strncpy(h.fwVersion, FW_VERSION, PROTO_VERSION_LEN - 1);
      gLink.send(MSG_HELLO, h);
    }
    if (peerHello && now - lastPing >= 1000) { lastPing = now; gLink.send(MSG_PING); }
    if (peerHello && now - lastTime >= 60000) {
      lastTime = now; gLink.send(MSG_GET_TIME);
      if (!gTimeAsked) gTimeAsked = now ? now : 1;
    }
    //  Asked, the A32 is alive, and no TIME came back: the DS3231 is not
    //  answering on its bus (the A32 sends nothing when its read fails).
    if (gTimeAsked && peerHello && now - gTimeAsked > 5000) { gRtcBad = 2; gTimeAsked = 0; }
    if (!peerHello) gTimeAsked = 0;
    if (peerHello && !gLink.peerAlive()) {
      peerHello = false; haveState = false;
      //  AND ITS SETTINGS ARE NO LONGER KNOWN (2026-09-25). The mirror kept
      //  saying "known" after the A32 went quiet: portal edits went nowhere and
      //  were overwritten when it came back, and a download wrote the stale
      //  copy instead of n/a. The next handshake fetches them again.
      haveCfg = false;
      Con.println(F("  [WARN] A32 silent. Clock and needle keep running."));
    }
  }

  // --- needle mode follows the source and the amp --------------------------
  {
    static uint8_t lastSrc  = 0xFF;
    static bool    lastHomed = false;
    uint8_t src = haveState ? a32.source : SRC_AUX;
    bool    h   = Needle::homed();
    //  Also on the moment homing FINISHES: that is when the needle first has a
    //  place to be, and nothing else would tell it to go there.
    //  Not consumed during an upload: applyNeedleMode() refuses then, and an
    //  edge eaten here would never be acted on.
    if (!otaQuiet && (src != lastSrc || h != lastHomed)) {
      lastSrc = src; lastHomed = h;
      applyNeedleMode();
    }
    //  AN UPLOAD THAT DID NOT END IN A REBOOT: put the needle back to work. If
    //  it was mid re-index when stopped it is unhomed, and only an amp-on edge
    //  would have homed it again - so home it now, unless homing had already
    //  given up (that fault stays for a person, as before).
    //  AND ONLY ONCE THE STOP HAS LANDED. stop() is a request the needle task
    //  carries out within 5 ms; an upload refused on its first chunk used to
    //  get here first, start tracking or homing, and then have the stop set the
    //  needle IDLE on top of it - frozen, with the radio playing.
    if (otaResume && !otaQuiet && !Needle::stopPending()) {
      otaResume = false;
      if (!Needle::homed() && !Needle::recovering() && !Needle::faultReason()[0])
        Needle::startHoming();
      else
        applyNeedleMode();
    }
  }

  //  A calibration now finishes asynchronously, so its result has to be
  //  collected rather than returned. Without this the measurement is taken
  //  correctly and then thrown away.
  //  THE BAND RESULT, WHICH USED TO EVAPORATE. calFinishBand() shifts the
  //  position frame and sets the four index constants in RAM; nothing ever
  //  copied them into cfg, and the next applySettings() called
  //  setIndexCal(cfg.idx*, ...) - all zero - putting the reference back while
  //  the frame kept its shift. Every soft limit captured afterwards was out by
  //  that shift. Collecting it here - producer raises a flag, consumer polls it
  //  once - is the contract; the end-stop collector that used to be its twin
  //  was removed on 2026-09-03.
  {
    int32_t a, b, c, d, shift;
    if (Needle::calTakeBand(a, b, c, d, shift)) {
      cfg.idxOnFwd = a; cfg.idxOffFwd = b; cfg.idxOnRev = c; cfg.idxOffRev = d;
      //  SHIFT THE LIMITS WITH THE FRAME rather than telling him to re-set
      //  them. calFinishBand() moved the position frame by `shift`, and the
      //  soft limits are PHYSICAL marks - a point at P in the old frame is at
      //  P - shift in the new one. Asking for a manual re-set is what made the
      //  band calibration destructive, and every corruption so far came from
      //  that step being missed. This also re-brackets zero automatically,
      //  because the index is 0 in the new frame by construction.
      cfg.posMin -= shift; cfg.posMax -= shift;
      //  If the carried pair no longer brackets the index, the OLD pair was
      //  already wrong by more than the travel - fall back to the narrow
      //  provisional pair rather than let setGeometry() refuse it and leave
      //  old-frame numbers behind.
      if (cfg.posMin > 0 || cfg.posMax < 0) {
        Con.printf("  carried limits do not bracket the index - reset to %ld..%ld, set the stops again.\n",
                      (long)-PROVISIONAL_LIMIT_HS, (long)PROVISIONAL_LIMIT_HS);
        cfg.posMin = -PROVISIONAL_LIMIT_HS; cfg.posMax = PROVISIONAL_LIMIT_HS;
      }
      applySettings();
      settingsTouch();
      Con.printf("  index band SAVED: onFwd %ld offFwd %ld onRev %ld offRev %ld\n",
                    (long)a, (long)b, (long)c, (long)d);
      Con.printf("  frame moved %ld half-steps; soft limits carried to %ld..%ld\n",
                    (long)shift, (long)cfg.posMin, (long)cfg.posMax);
      //  calStop() leaves st = IDLE and only applySettings() runs here, so the
      //  needle sat inert after every calibration stage and read as a hang.
      applyNeedleMode();
    }
  }


  //  Dark and still while a firmware image is being written. Deliberate - see
  //  portalOtaQuiet(). Everything comes back on the reboot that follows.
  if (otaQuiet) Panel::update(false, SRC_AUX, false);
  else          Panel::update(ampOn, haveState ? a32.source : SRC_AUX, Needle::tuningActive());

  { static uint32_t last = 0;
    if (!otaQuiet && now - last >= 200) { last = now; updateDisplay(); } }

  // --- NTP disciplines the DS3231, not the other way round -----------------
  //  The RTC stays the authority when the network is absent, which is most of
  //  the time. When NTP has a fresh answer it is written THROUGH to the
  //  DS3231 (at most hourly), so the correction survives the next power cut on
  //  its own. Gated on the handshake: while the A32 is not answering, NTP does
  //  not set this board's clock either.
  {
    static uint32_t lastNtpPush = 0;
    if (peerHello && Net::ntpFresh() &&
        (lastNtpPush == 0 || now - lastNtpPush >= 3600000UL)) {
      uint32_t e = Net::ntpEpoch();
      if (e) {
        lastNtpPush = now;
        ProtoTime t = {};
        t.unixUtc = e;
        t.valid   = 1;
        gLink.send(MSG_SET_TIME, t);
        epochAtSync = e; millisAtSync = now; timeValid = true;
        Con.println(F("  clock disciplined from NTP."));
      }
    }
  }

  //  Remember where the shaft is, so the turn can be recovered at next boot.
  //  Debounced like every other write - flash traffic is the one load phase G
  //  showed disturbs the display.
  {
    static uint32_t last = 0;
    if (now - last >= 10000) {
      last = now;
      //  Only a LIVE count is saved: before the encoder's first good reading
      //  the count is a placeholder, and saving it would seat the next boot
      //  on the wrong turn.
      int32_t a = Needle::accumulated();
      if (Needle::i2cOk() && a != cfg.lastAngle) { cfg.lastAngle = a; settingsTouch(); }
    }
  }

  // --- console -------------------------------------------------------------
  if (Con.available()) {
    char c = Con.read();
    switch (c) {
      //  Refusals are printed by the needle itself; these say only what was taken.
      case 'H': case 'h': if (!Needle::startHoming()) Con.println(F("  homing.")); break;
      case 'T': case 't': Con.println(Needle::track() ? F("  tracking.")
                                    : F("  NOT tracking - no zero and nothing is homing it (see the fault).")); break;
      case 'P': case 'p': Con.println(Needle::park()  ? F("  parking.")
                                    : F("  NOT parking - no zero and nothing is homing it (see the fault).")); break;
      //  ',' and '.' (up-leg vmax -100/+100) were removed 2026-09-25: '.' had no
      //  ceiling, and the portal's upVmax row does the job inside its bounds.
      //  FINDING THE REAL TRAVEL (r, n/N, m/M). The soft limits ship as the
      //  provisional +/-300 (PROVISIONAL_LIMIT_HS; they were a +/-967 estimate
      //  until 2026-09-03). Set too wide, the sweep drives the needle into a
      //  physical stop and loses steps.
      //
      //  So: nudge to each end by hand (n/N), then declare it (m/M). n/N are
      //  deliberately small and slow; there is nothing but these numbers
      //  protecting the mechanism.
      case 'r': if (Needle::sweepRange(true))
                  Con.println(F("  sweeping the full measured travel. x to stop.")); break;
      case 'x': case 'X': Needle::stop(); break;
      case 'e': Needle::setEdgeLog(!Needle::edgeLog());
                Con.print(F("  index edge logging "));
                Con.println(Needle::edgeLog() ? F("ON") : F("off")); break;
      case 'n': Needle::jogRaw(-20, 60, true); break;
      case 'N': Needle::jogRaw( 20, 60, true); break;
      //  SAME RULE AS THE PORTAL, one copy, see captureLimit().
      case 'm': Con.print(F("  ")); Con.println(captureLimit(true));  break;
      case 'M': Con.print(F("  ")); Con.println(captureLimit(false)); break;
      //  The needle must already be sitting on the sensor.
      case 'k': case 'K': Needle::calStartBand(5); break;

      //  Aborts a running calibration and leaves everything else alone. `x`
      //  aborts one too (stop() calls the same abort), and also stops the needle.
      case 'A': Needle::calAbort(); break;

      //  The removed end-stop calibration. Old instructions still name these,
      //  and `default: break;` would answer a documented instruction with
      //  silence - indistinguishable from a key that worked.
      case 'g': case 'G':
        Con.println(F("  g/G were removed on 2026-09-03 - they drove unguarded."));
        Con.println(F("  Jog to the printed mark and use m / M instead."));
        break;
      case 'D': settingsDump(); break;
      case '-': cfg.brightOn = (cfg.brightOn > 15) ? cfg.brightOn - 15 : 0;
                Display::setBrightness(cfg.brightOn); settingsTouch();
                Con.printf("  brightness %u\n", cfg.brightOn); break;
      case '=': case '+':
                cfg.brightOn = (cfg.brightOn < 240) ? cfg.brightOn + 15 : 255;
                Display::setBrightness(cfg.brightOn); settingsTouch();
                Con.printf("  brightness %u\n", cfg.brightOn); break;
      case 'f': case 'F': {
                //  THREE choices, not two. `!showTuning` collapsed the author's
                //  "tuning while tuning" mode to 0 with no console route back.
                cfg.showTuning = (cfg.showTuning + 1) % 3; settingsTouch();
                static const char *n[] = { "the clock", "TUNING", "tuning while tuning" };
                Con.printf("  display shows %s\n", n[cfg.showTuning]);
                break; }
      //  SIGNED. These run -20000..20000, and %u printed a negative one as
      //  about 4.29 billion - on the very numbers being read back during a
      //  tuner calibration.
      //  applySettings(), NOT setCalibration() - it does setCalibration AND
      //  refits the curve. calLow/calHigh feed the fit's domain and the
      //  fallback line's slope, so setting them without a refit left the
      //  running curve clamped to the OLD ends: on a machine still at the
      //  defaults, pressing c then C reported "reaches 88.1 - 88.1 MHz" and
      //  pinned the whole upper half of the dial. Every other writer of this
      //  pair already refits; this was the one that did not.
      //  Not from a frozen encoder: the capture would be a stale position.
      case 'c': if (!Needle::i2cOk()) { Con.println(F("  REFUSED - the tuning encoder is not answering.")); break; }
                cfg.calLow = Needle::accumulated(); cfg.tunerEndsSet |= ENDS_LOW;
                applySettings(); settingsTouch();
                Con.printf("  cal LOW = %ld\n", (long)cfg.calLow); break;
      case 'C': if (!Needle::i2cOk()) { Con.println(F("  REFUSED - the tuning encoder is not answering.")); break; }
                cfg.calHigh = Needle::accumulated(); cfg.tunerEndsSet |= ENDS_HIGH;
                applySettings(); settingsTouch();
                Con.printf("  cal HIGH = %ld\n", (long)cfg.calHigh); break;
      case 'W': case 'w': setClockInteractive(); break;
      case 'y': setWifiInteractive();  break;
      case 'Y':
        Con.println(F("  forcing the access point up. Scan for it now."));
        Con.println(F("  it holds 10 minutes (longer while someone is on it), then goes home."));
        Net::forceAp();
        break;
      case '8': dispTest = 1; Con.println(F("  display: 8888")); break;
      case '9': dispTest = 0; Con.println(F("  display: normal")); break;
      case 'b': dispTest = 2; Con.println(F("  display: blank")); break;
      case '0': Needle::coilTest(0); break;
      case '1': Needle::coilTest(1); break;
      case '2': Needle::coilTest(2); break;
      case '3': Needle::coilTest(3); break;
      case '4': Needle::coilTest(4); break;
      case 'j': Needle::jogRaw(  200, 200, false); break;   // HALF, forward
      case 'J': Needle::jogRaw( -200, 200, false); break;   // HALF, reverse
      case 'u': Needle::jogRaw(  200, 200, true ); break;   // MICRO, forward
      case 'U': Needle::jogRaw( -200, 200, true ); break;   // MICRO, reverse
      case 'l': case 'L': limitMonitor(); break;
      // ---- RDA5807M (Firmware Gospel §6) -----------------------------
      //  THE INSTRUMENT BY HAND: does the part answer and where should the LO
      //  be (R), a wide or narrow sweep (q, v) and its bins (a), the learned
      //  fixed features (o, O). None of these stores a tuning sample; V does.
      case 'R': {
        Con.printf("  RDA5807M: %s\n", Rda::state());
        Con.printf("  module %s   bus GPIO%d/%d   window %d.%d-%d.%d MHz LO\n",
                      Rda::present() ? "ANSWERS" : "SILENT",
                      S3_RDA_SDA, S3_RDA_SCL,
                      Rda::LO_WINDOW_LO10 / 10, Rda::LO_WINDOW_LO10 % 10,
                      Rda::LO_WINDOW_HI10 / 10, Rda::LO_WINDOW_HI10 % 10);
        int32_t f = Needle::tuneFreq10();
        Con.printf("  the dial says %ld.%ld MHz, so the LO should be near %ld.%ld\n",
                      (long)(f / 10), (long)(f % 10),
                      (long)((f - ifOff10()) / 10),
                      (long)((f - ifOff10()) % 10));
        break;
      }
      case 'q':
        //  ~200 channels x 300 ms is about a minute. The dwell is not
        //  negotiable - see rda.h's note on the 40 ms false negative.
        if (Rda::requestSweep(Rda::LO_WINDOW_LO10, Rda::LO_WINDOW_HI10, 1, 300))
          Con.println(F("  wide sweep started - about a minute. 's' for progress, 'a' to print."));
        else Con.println(F("  sweep refused - see the RDA state in 's' (running, absent, or lost)."));
        break;
      case 'v': {
        //  THE PREDICTION IS THE POINT. The bench rig was blind and had to
        //  sweep the whole window; this one knows the shaft angle, so it can
        //  confirm in about 10 s instead of about a minute.
        int32_t lo = Needle::tuneFreq10() - ifOff10();
        if (Rda::requestSweep((int16_t)(lo - 15), (int16_t)(lo + 15), 1, 300))
          Con.printf("  narrow sweep around %ld.%ld MHz - about 10 s.\n",
                        (long)(lo / 10), (long)(lo % 10));
        else Con.println(F("  sweep refused - see the RDA state in 's' (running, absent, or lost)."));
        break;
      }
      case 'a': {
        uint8_t n = Rda::binCount();
        if (!n) { Con.println(F("  no sweep yet - press q or v.")); break; }
        Con.printf("  %u bins, floor %u. Anything at or under the floor is noise.\n",
                      (unsigned)n, (unsigned)Rda::chipRssiFloor());
        for (uint8_t i = 0; i < n; i++) {
          int16_t f = Rda::binFreq10(i);
          uint8_t r = Rda::binRssi(i);
          Con.printf("   %3d.%d  %3u %s%s\n", f / 10, f % 10, (unsigned)r,
                        r > Rda::chipRssiFloor() + 5 ? "<<<" : "",
                        Rda::isSpur(f) ? "  (known fixed)" :
                        Rda::binStereo(i) ? "  (STEREO - a broadcast)" : "");
        }
        if (Rda::fineCount()) {
          Con.println(F("  fine pass (50 kHz; 0 = skipped):"));
          for (uint8_t i = 0; i < Rda::fineCount(); i++) {
            int16_t f = Rda::finePointF20(i);
            Con.printf("   %3d.%02d  %3u%s\n", f / 20, (f % 20) * 5, (unsigned)Rda::finePointRssi(i),
                          f == Rda::fineF20() ? "  <<< chosen" : "");
          }
        }
        break;
      }
      case 'o': {
        //  SPUR LEARNING: sweep (q) with the set switched OFF, where every peak
        //  is a fixed feature by definition, then press this once per feature -
        //  each press marks the strongest candidate not already known (known
        //  ones are skipped by bestCandidate()). They matter because a fixed
        //  spur can read louder than the LO (rda.h, note 3).
        uint8_t m = 0;
        int c = Rda::bestCandidate(&m);
        if (c < 0) { Con.println(F("  nothing stands above the local floor to blame.")); break; }
        int16_t f = Rda::binFreq10((uint8_t)c);
        //  WRITE cfg, NOT Rda - and REFUSE OUT LOUD. spurAdd() used to return
        //  in silence when the list was full or the frequency already known,
        //  while this handler printed "marked as a fixed feature" regardless:
        //  the ninth press reported success and discarded the value. That is
        //  this project's fourth "refusal painted green", and persistence makes
        //  it worse, because the author stops re-teaching and stops watching.
        if (Rda::isSpur(f)) {
          Con.printf("  %d.%d is already known - nothing to do.\n", f / 10, f % 10);
          break;
        }
        if (cfg.spurUsed >= CFG_SPURS) {
          Con.printf("  REFUSED: the fixed-feature list is full (%u). Clear it with O.\n",
                        (unsigned)cfg.spurUsed);
          break;
        }
        cfg.spur[cfg.spurUsed++] = f;
        applySettings();                       // pushes the list into Rda
        settingsTouch();                       // and it survives the power cut
        Con.printf("  %d.%d marked as a fixed feature (%u known, saved).\n",
                      f / 10, f % 10, (unsigned)cfg.spurUsed);
        break;
      }
      case 'O':
        //  Clears BOTH copies through the one path, and persists the clear.
        //  Zeroing the array is not superfluous - tune.clear does the same - so
        //  no stale number can reach a dump or a settings file. A clear that
        //  only cleared RAM would be worse than no persistence at all: the list
        //  is cleared precisely because it is wrong, and a power cycle would
        //  hand the wrong list straight back.
        cfg.spurUsed = 0;
        for (uint8_t i = 0; i < CFG_SPURS; i++) cfg.spur[i] = 0;
        applySettings();
        settingsTouch();
        Con.println(F("  fixed-feature list cleared, and the clear is saved."));
        break;

      case 'Z': Needle::jitterClear(); Display::resetSlotStats();
                Con.println(F("  jitter and display slot-error counters zeroed.")); break;
      case 'i': case 'I':
        Con.printf("  network  : %s \"%s\"   http://%s/   (%d dBm)\n",
                      Net::isSta() ? "joined" : "OWN ACCESS POINT",
                      Net::ssid().c_str(), Net::ip().c_str(), Net::rssi());
        Con.printf("  portal   : %s, %d signed in, ntp %s\n",
                      Portal::running() ? "up" : "down", Portal::sessions(),
                      Net::ntpFresh() ? "fresh" : "stale");
        if (Net::apClients() >= 0)
          Con.printf("  ap       : %d device(s) joined\n", Net::apClients());
        Con.println(F("             http://ambersong.local/ also works."));
        //  ONLY WHEN IT IS THE QUESTION. On the home network this would stall
        //  loop() for seconds to report what you already know.
        Net::driverReport();
        if (!Net::isSta()) Net::scanReport();
        break;
      //  THE RECOVERY PATH. There is no button on the front of the machine for
      //  a forgotten password, by decision - this cable is it.
      case '~':
        Portal::resetAdmin();
        break;
      //  FOR A TRANSMITTER THAT RADIATES NOTHING WHILE THE RECEIVER WORKS (a
      //  diagnostic). Erase the RF
      //  calibration ESP-IDF keeps in NVS and let it calibrate from scratch on
      //  the next boot. Nothing the author has calibrated lives in that blob -
      //  it is the radio's own trim, in its own namespace, not "amb3" and not
      //  "net". If this is not it, the cost was one reboot.
      case 'z': {
        esp_err_t e = esp_phy_erase_cal_data_in_nvs();
        Con.printf("  RF calibration erased (%s). Rebooting to recalibrate.\n",
                      e == ESP_OK ? "ok" : esp_err_to_name(e));
        Con.flush();
        delay(300);
        ESP.restart();
        break;
      }
      //  STEP THE TRANSMITTER DOWN (a diagnostic; net.cpp's ladder normally
      //  learns the power by itself). Quarter-dBm units, so 80 is 20.0 dBm and
      //  8 is 2.0 dBm. net.cpp caps the power at 60 (15 dBm), so the 80 level is
      //  applied as 60 and loop() then copies 60 back into cfg. Each press drops
      //  a level and prints what was asked and what the driver says it actually
      //  applied - which is not always the same, and that difference is itself
      //  informative.
      case 'B': {
        static const int8_t LEVELS[] = { 60, 44, 34, 28, 20, 8 };
        static uint8_t li = 0;
        li = (uint8_t)((li + 1) % (sizeof(LEVELS) / sizeof(LEVELS[0])));
        int8_t want = LEVELS[li];
        //  PERSIST IT. This started life as a pure diagnostic, and then the
        //  diagnostic turned out to BE the fix (2026-09-07): the value that
        //  makes this board radiate has to survive a power cut.
        cfg.wifiTxQ = (uint8_t)want;
        applySettings();
        settingsTouch();
        int8_t got = -128;
        esp_wifi_get_max_tx_power(&got);
        Con.printf("  tx power %d (%.1f dBm), driver reports %d (%.1f dBm), saved\n",
                      (int)want, want / 4.0f, (int)got, got / 4.0f);
        break;
      }
      //  MEASURE THIS DIAL POSITION AND KEEP IT. Lower-case v sweeps and tells
      //  you what it saw; upper-case V runs the full measurement (sampleStart,
      //  by hand) and stores the answer as a tuning sample in slots 3-11 - the
      //  same as the portal's measure button. Hand marks A/B/C are typed in the
      //  portal and never written here.
      case 'V': {
        const char *r = sampleStart(true);
        if (actionRefused()) Con.printf("  %s\n", r);
        break;
      }
      case 's': case 'S': status(); break;
      case '?': commandList(); break;
      default: break;
    }
  }
}
