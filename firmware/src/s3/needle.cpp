#include <Arduino.h>
#include <Wire.h>
#include <math.h>
#include "needle.h"
#include "console.h"
#include "drive.h"
#include "pins.h"
#include "esp_task_wdt.h"
#include "esp_attr.h"      // RTC_NOINIT_ATTR - the needle's memory across a reset
#include "esp_system.h"    // esp_reset_reason()

// ============================================================================
//  Everything about the MOTION came from an earlier test rig (now retired; see
//  Firmware Gospel §5). Everything about the INDEX is new here - that rig had
//  no homing scheme, only a detector.
// ============================================================================

namespace Needle {

// --- units -------------------------------------------------------------------
//  256 fine = one half-step, 2048 fine = one ELECTRICAL revolution. That second
//  identity is what lets Drive::apply() be a mask and a table lookup with no
//  division. Change the units and every profile below needs rescaling.
static inline float hspsToFine(float h) { return h * (float)FINE_PER_HALFSTEP; }
static inline float fineToHsps(float f) { return f / (float)FINE_PER_HALFSTEP; }
static inline float clampf(float v, float lo, float hi) {
  return v < lo ? lo : (v > hi ? hi : v);
}

static const int64_t CTRL_US = 1000;      // 1 kHz control tick

// --- motion state ------------------------------------------------------------
//  32 BITS, NOT 64, 2026-09-24. gPos is written by stepTask on core 0 and read
//  by needleTask, loop() and the portal on the other core, and the Xtensa has
//  no atomic 64-bit load: a reader could take the low word before a step and
//  the high word after it, and a carry across zero turns -1 fine into roughly
//  +4 billion for one read. One 32-bit word is read whole. The range was never
//  needed - the whole travel fits in +/-1600 half-steps, +/-410 000 fine, and
//  the widest homing search adds 2336 more; int32 holds 5000 times that. Math
//  that multiplies a position by anything casts to int64 LOCALLY, at the site.
//  gTarget goes with it for the same reason: needleTask writes it, stepTask
//  reads it every control tick.
static volatile int32_t gPos    = 0;      // fine
static volatile int32_t gTarget = 0;      // fine
static volatile float   gVel    = 0.0f;   // fine/s
static int64_t  gMoveStartUs = 0;
//  VOLATILE: stepTask on core 0 polls this every loop, and needleTask on core 1
//  sets it - REAPPROACH leans on it to stop the stepper the instant zero is
//  handed over. A static non-volatile flag may legally be cached in a register
//  across a loop whose calls cannot touch it, so the stop could arrive late.
static volatile bool gDone = true;
static bool     gDownLeg = false;
static uint32_t gJitter = 0;
static uint32_t gSteps  = 0;
static volatile bool gTestMode = false;   // bring-up tools own the drive
//  WHICH bring-up tool, 2026-09-25: true only while coilTest(1..4) is HOLDING a
//  coil. gTestMode is also raised by jogRaw(), and jogRaw() clears it itself,
//  on its own task, when its loop ends - so stopNow() must not clear gTestMode
//  blindly (a stop landing mid-jog would hand the drive back to stepTask while
//  jogRaw() is still stepping it). A held coil has no loop that will ever end;
//  this flag is what lets a stop release it. See stopNow().
static volatile bool gCoilHeld = false;
static bool gEdgeLog = false;             // announcing every edge drowns the console

//  Decay plan, computed ONCE per move. vpk is fixed by the distance, not
//  chosen: the integral of the envelope is vpk*td^2/(ta+td). If that would
//  exceed the ceiling we stretch td rather than clip vpk - clipping the peak
//  leaves the needle short of home.
static int64_t gDcStart = 0;
static float   gDcDist = 0, gDcTa = 0.1f, gDcTd = 1.0f, gDcVpk = 0, gDcTpk = 0;

// --- parameters, defaults = the rig's VALIDATED CONFIGURATION ----------------
//  except pUpVmax, 1100 rather than the rig's 1700 since 2026-09-24 (see
//  main.cpp's upVmax). applySettings() overwrites all of them at boot.
static uint16_t pUpVmax   = 1100, pUpAccel = 18000;   // see main.cpp's upVmax
static uint16_t pDnVmax   = 1700, pDnAccel = 18000;
static float    pWn       = 9.0f, pZeta    = 0.50f;
//  The PRINTED face (frequency -> needle travel), tenths of a MHz. See
//  trackTarget(). A placeholder: applySettings() installs cfg.dialLow/High.
static uint16_t pDialLow  = 880,  pDialHigh = 1080;

//  The fitted angle->frequency curve.
//
//  These defaults are an INERT PLACEHOLDER, not a working mapping: b and c are
//  zero, so they answer a flat 88.1 MHz at every shaft position. Nothing runs
//  on them - fitTuneCurve() installs a real curve (or installStoredLine()'s
//  fallback) on the first applySettings(), which happens during setup() before
//  the needle is ever asked to track. An earlier note here claimed they
//  "reproduce the old straight line from 88.1 to 107.9"; they do not, and the
//  path it described cannot be taken.
static double   pCurveX0 = 0.0;
static float    pCurveA  = 881.0f;
static float    pCurveB  = 0.0f;
static float    pCurveC  = 0.0f;
static int32_t  pCurveOff10 = 0;
static int32_t  pCurveXlo = -30000, pCurveXhi = 30000;
static uint16_t pRiseMs   = 100,  pFallMs  = 1000;
static uint16_t pDwellUp  = 100,  pDwellDn = 1000;
static uint8_t  pMicroFast = 16,  pMicroSlow = 32;
static uint16_t pReapHsps = 60;           // band-calibration creep speed (not REAPPROACH)

// --- geometry, half-steps relative to the homed zero -------------------------
static int32_t pPosMin = -300, pPosMax = 300;     // PROVISIONAL - applySettings() overwrites; see main.cpp PROVISIONAL_LIMIT_HS
static int32_t pOnFwd = 0, pOffFwd = 0, pOnRev = 0, pOffRev = 0;
//  How far outside the measured band the sensor may still read ON before the
//  frame is called wrong: debounce lag at speed (a few half-steps) plus room.
static const int32_t BAND_MARGIN_HS = 30;

// --- index -------------------------------------------------------------------
static volatile State st = IDLE;
static bool     isHomed = false;
//  A ZERO HANDED TO stepTask BUT NOT YET APPLIED. REAPPROACH writes the new frame
//  into gCorrectHs and must then wait until stepTask has folded it into gPos:
//  anything planned or measured before that uses the OLD frame. Found on the
//  machine 2026-09-23 - see REAPPROACH.
static bool     gZeroPending = false;
static int32_t  gDrift  = 0;
//  gDrift is the LAST MEASURED discrepancy and stays readable for the status
//  line. It is NOT a statement that the needle is currently out: anything
//  within CORRECT_MAX_HS is absorbed into gPos immediately, after which the
//  frame is right again and gDrift describes history. Nothing cleared it, so
//  captureLimit() refused a limit on a residual the machine had already acted
//  on - and its advice, "re-home", regenerated the number on the post-home
//  sweep. A guard whose remedy recreates its own trigger has no exit, and the
//  author hit exactly that at the bench. THIS is the flag that means "still
//  wrong": set only when a crossing was too far out to absorb.
static bool     gDriftPending = false;

//  WHERE THE INDEX IS, IN THE FRAME WE CURRENTLY BELIEVE, as of the crossing
//  that revealed the slip. gDrift = here - expect, so the needle was physically
//  on the index while believing it was at `expect + gDrift`. That is a DIRECTION
//  HINT of much better quality than homing's usual one, which is the sign of a
//  position we have just decided not to trust.
//
//  THE CLUE IS KEPT UNTIL A BETTER ONE REPLACES IT - the author's
//  rule: never throw the clue away before a new one replaces it. It used to
//  live only as long as gDriftPending did, and gDriftPending is cleared by
//  beginReindex() the moment a re-index STARTS - so a re-index cut short by a
//  stop (the OTA quiet, the console x, the portal button) left no clue at all,
//  and the next startHoming() fell back to the sign of gPos in the very frame
//  the slip had just proved wrong. With the limits disarmed and a 2336
//  half-step budget, that is a search that can walk into a stop. So the belief
//  now has its own validity flag, gIdxEvid, which says only "gIndexBelieved is
//  a real observation in the frame gPos is counting in", and nothing that
//  merely STARTS or STOPS a home touches it - not stop(), not beginReindex(),
//  not startHoming(). It is REPLACED by any newer observation of the sensor
//  (a slip, a band fault, the sensor found ON by a homing search), and
//  CLEARED only when the frame is shown or made true: REAPPROACH declaring
//  zero, calFinishBand() shifting the frame to the band it measured, or a
//  scored crossing that landed inside CORRECT_MAX_HS and was absorbed. All
//  three leave the index at 0 in the frame gPos then counts in - the last one
//  is itself the newest observation, and what it observed is that the frame
//  is right - so from then on the sign of gPos IS the clue, and keeping an
//  older one past it would be keeping the worse of the two.
static int32_t  gIndexBelieved = 0;
static bool     gIdxEvid       = false;

//  One place that records a sighting, so the value and its flag never part.
static inline void believeIndexAt(int32_t hs) { gIndexBelieved = hs; gIdxEvid = true; }
static inline void forgetIndexClue()          { gIndexBelieved = 0;  gIdxEvid = false; }
//  True while a recovery is running, so the closing full-travel SWEEP is
//  skipped. That sweep is power-up theatre; nobody wants it mid-listening.
static bool     gAutoHome = false;
//  Consecutive automatic recoveries. If the needle keeps slipping straight back
//  out of the window, something is mechanically wrong and repeating for ever
//  would grind rather than fix. Three, then stop and say so.
static uint8_t  gAutoTries = 0;
static const uint8_t AUTO_TRIES_MAX = 3;

//  PASSIVE RE-HOMING.
//  The index is a mid-travel mark the needle crosses on almost every journey,
//  and every crossing used to be MEASURED and thrown away. The author's point:
//  if the sensor says where the needle actually is, believe it,
//  and steps can never accumulate into an error. A small error is absorbed at
//  the crossing; only a slip too large to absorb (or the routine idle check,
//  or the portal's button) sends the needle back to the index - see
//  startReindex().
//
//  The correction is handed to the EMITTER rather than applied here. gPos is
//  owned by the step task on the other core; writing it from the supervisor
//  races the emitter's own read-modify-write of it (it was also a torn 64-bit
//  write until 2026-09-24, when gPos became one 32-bit word). One int32 of
//  pending correction is a single word, and the emitter folds it in at a point
//  where it is not mid-calculation. (Not every writer keeps this rule:
//  assumeAtLowStop(), bootPosition(), useMicro(), coilRelease(), jogRaw(),
//  calStartBand() and calFinishBand() write gPos directly, relying on the
//  motor being stopped at that moment.)
static volatile int32_t gCorrectHs = 0;
static bool             bandResultPending = false;
//  SNAPSHOT, not the live pOn*/pOff*. Between calFinishBand() raising the
//  flag and main.cpp polling it, ANY applySettings() runs setIndexCal(cfg.idx*)
//  = setIndexCal(0,0,0,0) and zeroes them - so returning the live values would
//  persist zeros and print 'index band SAVED: 0 0 0 0'. The fix would have
//  contained the exact bug it fixes.
static int32_t          bandOnF = 0, bandOffF = 0, bandOnR = 0, bandOffR = 0;
static int32_t          bandShift = 0;

//  Beyond this, a "drift" is not drift. Gearbox lash and the sensor's own
//  hysteresis are a few half-steps; tens mean something slipped or the profile
//  drove into a stop, and silently absorbing that would hide the fault the
//  measurement exists to reveal.
static const int32_t CORRECT_MAX_HS = 40;

//  How far the tracking target must differ before the needle is asked to move.
//  Not a tolerance on accuracy - it is narrower than the pointer is wide - but
//  the difference between following the dial and hunting the encoder's noise.
static const int32_t TRACK_DEADBAND_HS = 2;

//  HUNTING DETECTION - the sense this machine did not have.
//
//  What found the wobble of 2026-09-10 was the author's eyes and the ULN2003's
//  LEDs. The telemetry said the needle was stationary and was not lying: `pos`
//  is published in half-steps and both values it alternated between rounded to
//  the same one. Nobody had looked at the
//  rounding that caused it either.
//
//  So the guard against the CLASS is not to audit harder for known patterns. A
//  hunt has one signature whatever causes it - STEPS ARE BEING EMITTED AND THE
//  NEEDLE IS NOT GOING ANYWHERE - and that is checkable with no theory about
//  why. It catches this bug and it catches the next one.
//
//  Sized against real motion: following a knob moves both numbers together, so
//  only travel that CANCELS ITSELF trips this. 120 steps EMITTED inside four
//  seconds with a net move under 8 half-steps is not a dial being followed.
//  gSteps counts steps at the current microstep division, so 120 steps is
//  3.75 half-steps of travel at 1/32 (tracking) and 7.5 at 1/16.
static const uint32_t HUNT_WINDOW_MS   = 4000;
static const uint32_t HUNT_STEPS_MIN   = 120;
static const int32_t  HUNT_NETMOVE_MAX = 8;
static uint32_t gHuntAt    = 0;
static uint32_t gHuntSteps = 0;
static int32_t  gHuntPos   = 0;
static bool     gHunting   = false;
//  HOMING BUDGETS ARE PROPERTIES OF THE MECHANISM, NOT OF THE CALIBRATION.
//  All three used to be derived from the soft-limit span, which the purge
//  deliberately makes provisional (+/-300): the search got 750, BACKOFF 300
//  and REAPPROACH 150 - too tight for REAPPROACH, the phase that DEFINES
//  ZERO. Worse, a magnet already over the sensor at boot enters homing at
//  BACKOFF, so a floor on the search alone was never even consulted on that
//  path.
//    search     : measured travel 1869 + 25 %                    -> 2336
static const int32_t HOME_BUDGET_HS            = 2336;
//  BACKOFF and REAPPROACH are STANDALONE, derived from the measured index
//  band and not from the search budget - dividing the search figure by 4
//  and 8 was the same coupling one level removed. Each runs in one fixed
//  direction (backoff always -, reapproach always +) with the far stop
//  1195 / 674 half-steps away, so neither budget can reach a stop.
//    backoff    : top of the band to its release below zero (~135 with the
//                 20 half-step hops) + clearance 60 + hop 20 -> 400, ~2x
//    reapproach : clearance 60 + the release point's distance below the
//                 forward ON edge - under ~130 on the band last measured
//                                                                 -> 300, ~2.4x
static const int32_t HOME_BACKOFF_BUDGET_HS    = 400;
static const int32_t HOME_REAPPROACH_BUDGET_HS = 300;
static uint32_t gCross  = 0;
//  gCross counts EDGES; gMeas counts edges the code actually drew a conclusion
//  from. They stopped being the same thing when the crossing evaluation was put
//  behind a velocity guard, and captureLimit()'s "cross the index once since
//  homing" needs the second one - a crossing at which nothing was measured is
//  no evidence that the position was verified.
static uint32_t gMeas   = 0;

//  The index sensor, S3_INDEX_HALL - the one Hall switch left; there are no
//  end switches. Read as active LOW (the sensor and its pull-up: Bible §11).
static inline bool indexRaw() { return digitalRead(S3_INDEX_HALL) == LOW; }

// ============================================================================
//  INDEX DEBOUNCE - ONE CONFIRMED STATE, SAMPLED ONCE PER SUPERVISOR POLL.
//
//  THIS REPLACES indexStable(), 2026-09-23. "Two agreeing reads microseconds
//  apart" caught a same-poll disagreement and nothing slower: a bounce that
//  settles somewhere between one 5 ms poll and the next reads as a real
//  transition, because nothing remembered the poll before. What follows is a
//  REAL debounce - K consecutive polls have to agree before a new level is
//  believed - fed from exactly ONE place, sampleIndex(), called once per
//  needleTask iteration before anything branches on mode. Homing, the passive
//  crossing measurement and band calibration all read idxState()/idxEdge
//  afterwards; none of them calls indexRaw() again.
//
//  K, WORKED OUT FROM THE MACHINE, NOT PICKED:
//
//    T_POLL = 5 ms - needleTask's own vTaskDelay(pdMS_TO_TICKS(5)), the
//    cadence every consumer already assumed (this is the same "supervisor
//    samples every ~5 ms" the old comment here used for its own lag figure).
//
//    REJECT >= 10 ms OF GLITCH. K agreeing polls span at least (K-1)*T_POLL
//    between the first of them and the last, so a disturbance that reverts
//    before that elapses can never produce K agreeing samples, whatever the
//    phase it lands on. Need (K-1)*5 > 10  ->  K > 3  ->  K >= 4.
//
//    STILL SEE A REAL CROSSING. The narrowest band is the reverse one,
//    offRev..onRev, measured at -15..+79 - 94 half-steps wide - and the
//    fastest the needle ever moves near it is pDnVmax = 1700 hsps, the park's
//    decay (every second-order move, homing's included, is capped lower, at
//    pUpVmax = 1100 by default). Time spent inside that band at 1700:
//    94 / 1700 = 55.3 ms, i.e. 55.3 / 5 = ~11.1 polls. Confirming BOTH edges
//    of a pass-through needs 2*K polls to fit inside it: 2*K <= 11 -> K <= 5.
//
//    4 <= K <= 5. CHOSEN K = 4, the low end of that window, so a crossing at
//    the fastest speed this machine ever runs still has ~3 polls of margin
//    (11 - 2*4 = 3) instead of sitting on the edge of the arithmetic.
//
//  BACK-DATED, NOT DELAYED. A run's gPos/gVel, and the interval since the
//  PREVIOUS poll, are captured at its FIRST poll - the instant the raw level
//  first disagreed with the last CONFIRMED one - and held until (if) the run
//  survives to K. The confirmed edge is reported at THAT snapshot, not at the
//  poll that finally believed it: debouncing costs K-1 polls of CONFIRMATION
//  delay, but because the position is back-dated to the run's start, none of
//  that delay becomes lag or bias in WHERE the edge is recorded. What is left
//  is one poll of sampling uncertainty, between the previous confirmed poll
//  and the run's first one - the same residue indexStable() always had, not a
//  new one; see idxEdge.dtUs and its use in needleTask below.
// ============================================================================
static const uint8_t IDX_DEBOUNCE_K = 4;

struct IdxSample {
  int64_t  pos;    // gPos, fine, AT THIS POLL
  float    vel;    // gVel, fine/s, AT THIS POLL
  uint32_t dtUs;   // time since the PREVIOUS poll, clamped - see sampleIndex()
};

static bool      idxConfirmed  = false;   // the one state every consumer reads
static bool      idxRunLevel   = false;   // raw level the current run agrees on
static uint8_t   idxRunCount   = 0;
static IdxSample idxRunFirst   = {0, 0.0f, 0};
static IdxSample idxEdge       = {0, 0.0f, 0};  // back-dated snapshot of the last CONFIRMED transition
static bool      idxEdgeIsNew  = false;         // one-shot: true only on the poll that confirmed idxEdge
static uint32_t  idxLastPollUs = 0;
static bool      idxPolled     = false;         // false until begin()'s seed read

//  Called once per needleTask iteration, before gTestMode/calBusy/state are
//  even looked at - see the banner above for why that single call site is
//  what makes this a real debounce instead of another indexStable().
static void sampleIndex() {
  uint32_t nowUs = (uint32_t)esp_timer_get_time();
  bool     raw   = indexRaw();
  uint32_t dtUs  = idxPolled ? (nowUs - idxLastPollUs) : 0;
  //  A scheduling hiccup must not turn into a large bogus lag correction: the
  //  loop is nominally 5 ms, so anything past 20 ms is a measurement of the
  //  scheduler, not of the sample interval.
  if (dtUs > 20000) dtUs = 20000;
  idxLastPollUs = nowUs;
  idxPolled     = true;

  idxEdgeIsNew = false;

  if (raw != idxRunLevel || idxRunCount == 0) {
    //  A fresh disagreement starts a new run - INCLUDING one that reverses a
    //  run still short of K, which is exactly how a glitch that outlasts one
    //  poll but not K of them gets rejected: it never reaches K before the
    //  level flips back and restarts the count.
    idxRunLevel = raw;
    idxRunCount = 1;
    idxRunFirst = { gPos, gVel, dtUs };
  } else if (idxRunCount < 0xFF) {
    idxRunCount++;
  }

  if (idxRunLevel != idxConfirmed && idxRunCount >= IDX_DEBOUNCE_K) {
    idxConfirmed = idxRunLevel;
    idxEdge      = idxRunFirst;      // BACK-DATED: the run's first poll, not this one
    idxEdgeIsNew = true;
  }
}

//  What every consumer reads instead of indexStable(). A LEVEL, not an edge -
//  homing wants "is it ON right now", and this stays true for as long as the
//  confirmed run does.
static bool idxState() { return idxConfirmed; }

// --- soft limits -------------------------------------------------------------
//  NOTHING PHYSICAL STOPS THE NEEDLE ANY MORE. This, the homing budgets and the
//  boot-time belief (bootPosition()) are what replace the two limit switches.
//  stepTask, the one place a profiled step is emitted, asks this before every
//  step; jogRaw() only warns, and calCreep() asks it too.
static bool gLimitsArmed = false;

static int32_t trackTarget();          //  defined with the geometry setters

static bool stepAllowed(int dir) {
  if (!gLimitsArmed) return true;         // homing runs before zero is known
  int32_t half = (int32_t)(gPos / FINE_PER_HALFSTEP);
  if (dir > 0 && half >= pPosMax) return false;
  if (dir < 0 && half <= pPosMin) return false;
  return true;
}

// ============================================================================
//  PROFILES  -  lifted from an earlier test rig, which is where they were measured
// ============================================================================
static void planDecay(int64_t from, int64_t to) {
  gDcStart = from;
  gDcDist  = (float)(to - from);
  float D  = fabsf(gDcDist);
  float ta = (pRiseMs ? pRiseMs : 1) * 1e-3f;
  float td = (pFallMs ? pFallMs : 1) * 1e-3f;
  float vlim = hspsToFine((float)pDnVmax);
  if (D < 1.0f || vlim <= 0.0f) { gDcTa = ta; gDcTd = td; gDcVpk = 0; gDcTpk = 0; return; }
  float vpk = D * (ta + td) / (td * td);
  if (vpk > vlim) {
    vpk = vlim;
    td = (D + sqrtf(D * D + 4.0f * vlim * D * ta)) / (2.0f * vlim);
  }
  gDcTa = ta; gDcTd = td; gDcVpk = vpk;
  gDcTpk = ta * logf(1.0f + td / ta);
}

//  THE GAIN CEILING, and it is not optional.
//  Second-order is a LINEAR law driving a SATURATED actuator: it only commands
//  deceleration once the error is small, and has no idea the motor cannot brake
//  harder than `accel`. Braking has to begin before the stopping distance, which
//  gives  wn < 4*zeta*accel/vmax, on the second-order ("up") limits.  At the
//  defaults, 18000/1100, that ceiling is 32.7 and wn is 9, so there is margin -
//  but RECOMPUTE IT if vmax or accel ever change. begin() warns if wn exceeds
//  it, but it runs before applySettings(), so it checks only the compiled
//  defaults, never the stored or portal values. The peer shipped wn 12 against
//  a ceiling of 8.4 and it overshot 11 degrees and hunted; it presents as "this
//  profile does not move the motor", because a bare 28BYJ-48 asked to reverse
//  at 2000 hsps just buzzes.
static float wnCeiling() {
  return (pUpVmax > 0) ? (4.0f * pZeta * (float)pUpAccel / (float)pUpVmax) : 1e9f;
}

static void runProfile(float dt) {
  const float vmax = hspsToFine((float)(gDownLeg ? pDnVmax  : pUpVmax));
  const float amax = hspsToFine((float)(gDownLeg ? pDnAccel : pUpAccel));
  const float unit = (float)Drive::stepUnit();
  const float err  = (float)(gTarget - gPos);
  const float dist = fabsf(err);

  //  A move does not start the instant it is asked for. The pause before motion
  //  is most of why a mechanism reads as mechanical, and it costs one compare.
  //  ...BUT NOT WHILE FOLLOWING THE TUNER.
  //
  //  The dwell is character, and it belongs to a COMMANDED move - a source
  //  change, a sweep, a park - where the pause before motion is most of why the
  //  mechanism reads as mechanical. TRACKING issues a fresh move for every new
  //  target, so it paid the toll on each one: 100 ms tuning up and a full
  //  SECOND tuning down, which reads as the needle refusing to move until the
  //  knob has travelled some distance. The author reported exactly that and
  //  read it as a threshold; it is a timer.
  uint16_t dwell = (st == TRACKING) ? 0 : (gDownLeg ? pDwellDn : pDwellUp);
  if (dwell && (int64_t)(esp_timer_get_time() - gMoveStartUs) < (int64_t)dwell * 1000) {
    gVel = 0.0f;
    return;
  }

  if (gDownLeg) {
    //  DECAY - the drive itself dying away, like a reservoir emptying after
    //  mains is lost. Feedforward, not a follower: a jerk-limited chase of
    //  sqrt(2ad) is a feedback loop with lag and cannot ramp deceleration out
    //  before arrival.
    float t = (float)(esp_timer_get_time() - gMoveStartUs) * 1e-6f - dwell * 1e-3f;
    if (t < 0.0f) t = 0.0f;
    const float ta = gDcTa, td = gDcTd;
    const float env = (1.0f - expf(-t / ta)) * expf(-t / td);
    const float A = td, B = ta * td / (ta + td), c = 1.0f / ta + 1.0f / td;
    float sref = (A * (1.0f - expf(-t / td)) - B * (1.0f - expf(-c * t))) / (A - B);
    if (sref > 1.0f) sref = 1.0f;
    float vff  = gDcVpk * env * (gDcDist >= 0.0f ? 1.0f : -1.0f);
    float perr = (float)(gDcStart - gPos) + gDcDist * sref;
    gVel = vff + clampf(perr * 20.0f, -0.3f * vmax, 0.3f * vmax);
    gVel = clampf(gVel, -vmax, vmax);
  } else {
    //  SECOND ORDER - a damped mass-spring with a saturated actuator, which is
    //  what a real meter movement is. zeta below 1 overshoots and settles, and
    //  that is the most physical cue there is.
    //  Counterintuitive but measured: raising wn REDUCES overshoot here, because
    //  the velocity clamp means a stiffer system arrives with less momentum.
    //  zeta is the overshoot knob; wn is the ring-down knob.
    float a = pWn * pWn * err - 2.0f * pZeta * pWn * gVel;
    a = clampf(a, -amax, amax);
    gVel += a * dt;
    gVel  = clampf(gVel, -vmax, vmax);
  }

  //  ARRIVAL. Two numbers here were each a lost session on the rig:
  //   - the velocity threshold must SCALE WITH STEP SIZE. A fixed 20 hsps gave
  //     a one-microstep limit cycle at fine divisions: a 2 s move took 21.4 s
  //     to settle. Half-stepping never showed it, its unit being 8x larger.
  //   - the position tolerance is ONE FULL STEP UNIT, never half. Half a unit
  //     is unreachable by construction once the position leaves the lattice,
  //     and the move then hangs until the backstop.
  float vArrive = fmaxf(1.5f * sqrtf(2.0f * amax * unit), hspsToFine(20.0f));
  if (dist <= unit && fabsf(gVel) < vArrive) {
    gVel = 0.0f;
    gDone = true;
  }
}

// ---------------------------------------------------------------------------
//  CONSTANT-VELOCITY JOG
//
//  The profiles are for going somewhere. Calibration is different: it moves
//  until a SENSOR says stop, and it cannot know the distance in advance. Doing
//  that with profiled moves means a fresh dwell and acceleration ramp for every
//  few steps, which is both slow and jerky.
//
//  So the emitter gains one more mode: when gManualVel is non-zero it simply
//  runs at that velocity and the supervisor decides when to stop. Only the
//  band calibration's calCreep() uses it; the jogs go through jogRaw().
// ---------------------------------------------------------------------------
static volatile float gManualVel = 0.0f;

//  STOP AND ABORT ARE REQUESTS, SERVICED ON THE NEEDLE TASK, 2026-09-24.
//
//  stop() and calAbort() are called from three contexts: the console on the
//  Arduino loop (core 1), and the portal's HTTP handlers and portalOtaQuiet()
//  on the portal task (core 0). They used to do their work where they were
//  called, and from core 0 that raced the supervisor running on core 1. The
//  one that bites: calAbort() cleared calWhat and zeroed gManualVel while
//  calTick() on the other core was part-way through calCreep(), whose
//  manualRun() then wrote the creep velocity straight back - with calWhat
//  already clear, nothing was left that would ever zero it again, so the jog
//  ran on under the ordinary supervisor until it hit a soft limit, and then
//  sat there asking for more. `st = IDLE` from core 0 raced homingTick() the
//  same way: the homing machine could write its next state over the stop.
//
//  So the caller now only POSTS the request, and needleTask carries it out at
//  the top of its next loop, at most one 5 ms tick later - the only task that
//  runs calTick() and homingTick(), so nothing it does can land between their
//  reads and writes. Two things keep that from being a delay the motor can see:
//  stepTask stops emitting the moment the flag is up (it checks it every
//  control tick and before every step), and manualRun() refuses to set a
//  velocity while one is pending. The bits are cleared only AFTER the work is
//  done, so there is no instant where the flag is down and a creep velocity
//  written before the request is still standing.
enum : uint8_t { REQ_STOP = 1, REQ_CALABORT = 2 };
static uint8_t gReq = 0;
static inline uint8_t reqPending() { return __atomic_load_n(&gReq, __ATOMIC_SEQ_CST); }
//  Exported for main.cpp, 2026-09-25 - see needle.h.
bool stopPending() { return reqPending() != 0; }

static void manualRun(int dir, uint16_t hsps) {
  if (reqPending()) return;          //  a stop is on its way - do not undo it
  gManualVel = hspsToFine((float)hsps) * (dir >= 0 ? 1.0f : -1.0f);
}

static void manualStop() {
  gManualVel = 0.0f;
  gVel       = 0.0f;
  gDone      = true;
  gTarget    = gPos;
}

// ============================================================================
//  THE STEP EMITTER
//
//  PINNED TO CORE 0, not core 1, and that is deliberate.
//  Core 1 carries the display's 25 us timer ISR AND the Arduino loop() that
//  runs the inter-MCU link and the panel. This task busy-spins between steps
//  while moving, so on core 1 it would starve loop() for the length of every
//  sweep - and a high-priority spinner on the core running setup() is exactly
//  how the rig's boot banner stopped printing. On core 0 it also never contends
//  with the display ISR at all, which is strictly better than tolerating it.
//  WiFi and the portal task share this core; a needle stutter during portal
//  use is far more forgivable than a stalled link or a glitched display.
// ============================================================================
static void stepTask(void *) {
  //  THIS TASK BUSY-SPINS BETWEEN STEPS, so core 0's idle task does not run for
  //  the length of a move - and the task watchdog resets the chip when idle
  //  starves. An earlier test rig hit exactly this and removed IDLE from the
  //  watchdog on whichever core its emitter runs on. This firmware moved the
  //  emitter to core 0 and did not move the fix with it; the symptom was a boot
  //  loop that looked like a power problem.
  //
  //  The cost is real and worth stating plainly: core 0's idle task no longer
  //  catches a hung task there (WiFi and the portal live on core 0). A task
  //  that needs watching must be added to the watchdog explicitly, not by
  //  putting idle back - idle cannot run while a step generator is spinning,
  //  by design.
  esp_task_wdt_delete(xTaskGetIdleTaskHandleForCPU(0));

  int64_t nextCtrl = esp_timer_get_time();
  int64_t nextStep = nextCtrl;
  bool    wasStepping = false;
  uint32_t idleSince = millis();

  for (;;) {
    //  Fold in any correction the supervisor left for us. Doing it here, at the
    //  top of the loop and nowhere else, keeps gPos owned by this task.
    //  ONE ATOMIC EXCHANGE, not read-then-clear. A correction written by
    //  needleTask between a plain read and the clear was wiped without ever
    //  being applied. That window always existed; since the index debounce it
    //  carries the whole zero on a home - hundreds of half-steps - not just a
    //  crossing's few, so it is closed rather than argued about.
    int32_t c = __atomic_exchange_n(&gCorrectHs, 0, __ATOMIC_SEQ_CST);
    if (c) gPos += c * FINE_PER_HALFSTEP;    // a few thousand half-steps at most
    int64_t now = esp_timer_get_time();

    if (gTestMode) { vTaskDelay(1); nextCtrl = now; nextStep = now; continue; }

    if (now >= nextCtrl) {
      float dt = (float)CTRL_US / 1e6f;
      //  A manual jog overrides the profile entirely - no dwell, no ramp,
      //  just move until something says stop.
      //  A PENDING STOP OUTRANKS BOTH - see gReq. needleTask will do the rest
      //  of the stop within a tick; the motor stops here, now.
      if (reqPending()) { gVel = 0.0f; gDone = true; }
      else if (gManualVel != 0.0f) { gVel = gManualVel; gDone = false; }
      else if (!gDone) runProfile(dt);

      nextCtrl += CTRL_US;
      if (nextCtrl < now - CTRL_US) nextCtrl = now + CTRL_US;

      if (gDone) {
        //  Release the coils 400 ms after the move ends. Holding costs current
        //  and warms the motor for nothing - the needle has no load to hold
        //  against.
        if (Drive::energised() && millis() - idleSince > 400) Drive::release();
        nextStep = now;
      } else {
        idleSince = millis();
      }
    }

    if (!gDone && fabsf(gVel) > 1.0f && !reqPending()) {
      int32_t unit     = Drive::stepUnit();
      int64_t interval = (int64_t)((float)unit * 1e6f / fabsf(gVel));
      if (interval < 20)      interval = 20;        // 50 k steps/s guard
      if (interval > 1000000) interval = 1000000;

      if (!wasStepping) { wasStepping = true; nextStep = now; }

      //  PULL THE DEADLINE IN AS THE PROFILE ACCELERATES.
      //  THE SINGLE MOST IMPORTANT LINE IN THIS FILE. The deadline used to be
      //  set only when a step was emitted, from the velocity at that instant, so
      //  a profile leaving rest slowly scheduled its second step from its
      //  slowest-ever velocity and never revisited the decision. The rig's
      //  s-curve leaves rest at 10 fine/s - one step per 25 SECONDS - so it
      //  emitted one step, parked the deadline 25 s out, and sat there
      //  reporting 2000 hsps with nothing turning.
      if (nextStep > now + interval) nextStep = now + interval;

      if (now >= nextStep) {
        int dir = (gVel > 0.0f) ? +1 : -1;
        if (stepAllowed(dir)) {
          gPos += dir * unit;
          Drive::apply(gPos);
          gSteps++;
        } else {
          //  Soft limit reached. Stop rather than grind - there is no switch
          //  behind this any more.
          gVel = 0.0f; gDone = true;
        }
        uint32_t late = (uint32_t)(now - nextStep);
        if (late > gJitter) gJitter = late;
        nextStep += interval;
        if (nextStep < now) nextStep = now + interval;   // resync, never burst
      }
    } else {
      wasStepping = false;
    }

    //  IDLE ALWAYS YIELDS. Not an optimisation - a spinner that never gives the
    //  core back starves everything else on it.
    if (gDone) {
      vTaskDelay(1);
    } else {
      int64_t until = (nextStep < nextCtrl) ? nextStep : nextCtrl;
      if (until - esp_timer_get_time() > 2000) vTaskDelay(1);
    }
  }
}

// --- move commands -----------------------------------------------------------
static void moveToFine(int64_t t, bool downLeg) {
  gDownLeg     = downLeg;
  gTarget      = (int32_t)Drive::snap(t);   // snap() is 64-bit; the value is not
  planDecay(gPos, gTarget);
  gMoveStartUs = esp_timer_get_time();
  gDone        = false;
}
static void moveToHalf(int32_t h, bool downLeg) {
  moveToFine((int64_t)h * FINE_PER_HALFSTEP, downLeg);
}
static bool moving() { return !gDone; }

//  useMicro(true) selects pMicroSlow (TRACKING only); useMicro(false) selects
//  pMicroFast (the sweep and the park).
//  Microstep division changes SNAP the position onto a new lattice, which stops
//  motion. Never during a move: each mode has its own lattice, and snapping the
//  target without the position leaves a target that integral steps can never
//  reach - it arrives one step short, steps back, and jiggles forever.
static void useMicro(bool resting) {
  if (moving()) return;
  Drive::setMode(DRV_MICRO, resting ? pMicroSlow : pMicroFast);
  gPos    = (int32_t)Drive::snap(gPos);
  gTarget = gPos;
}

// ============================================================================
//  AS5600
// ============================================================================
static const uint8_t REG_STATUS = 0x0B, REG_RAW_ANGLE = 0x0C, REG_AGC = 0x1A;
static const uint8_t REG_MAGNITUDE = 0x1B;
static volatile uint8_t  statusRaw = 0;
static volatile uint16_t magnitude = 0;
static volatile uint16_t rawAngle = 0;
static volatile bool magOk = false, busOk = false;
static volatile uint8_t agcVal = 0;
static int32_t accAngle = 0, calLow = 0, calHigh = 6023;
static bool accInit = false;
static const uint16_t TUNE_DEADBAND = 12;
static volatile uint32_t lastTurnMs = 0;
static int32_t           turnAccum  = 0;   // see the deadband note in readAngle()

//  A MISSING SENSOR MUST NOT SPAM. Polled at 50 Hz an absent AS5600 produces
//  fifty "i2cWriteReadNonStop returned Error -1" lines a second from the core,
//  which drowns the console exactly when you are trying to read it.
//
//  Two things fix it. We back off to one retry every 2 s, because a device that
//  is not on the bus will not be there a millisecond later either. And while we
//  KNOW it is absent we silence the Arduino HAL logger, restoring it the moment
//  the sensor answers - scoped to the outage rather than switched off for good,
//  so a genuine error later is still visible.
static uint32_t busDownUntil = 0;
static bool     busWarned    = false;

static bool busReady() { return !busDownUntil || millis() >= busDownUntil; }

//  EVERY OUTAGE COUNTED, so a measurement can tell whether the shaft reading it
//  started from is still the one it ends on. While the bus is down the count
//  of the shaft is frozen, and a knob turned then is invisible until the bus
//  answers again.
static volatile uint32_t gGapCount = 0;
static void busFail() {
  if (busOk) gGapCount++;
  busOk = false;
  busDownUntil = millis() + 2000;
  if (!busWarned) {
    busWarned = true;
    esp_log_level_set("ARDUHAL", ESP_LOG_NONE);
    Con.println(F("  [WARN] AS5600 not answering. Retrying every 2 s, quietly."));
  }
}

static void busGood() {
  busOk = true;
  busDownUntil = 0;
  if (busWarned) {
    busWarned = false;
    esp_log_level_set("ARDUHAL", ESP_LOG_ERROR);
    Con.println(F("  AS5600 is answering again."));
  }
}

//  TRUE once we hold a rawAngle we are entitled to subtract from. Cleared by
//  any gap in the sampling, because a delta is only meaningful against the
//  IMMEDIATELY preceding sample.
static bool angleFresh = false;

static int32_t gSeedAnchor  = 0;        // see seedAccumulator()
static bool    gSeedPending = false;
static float gTrackAcc = 0.0f;          // accAngle, low-passed, for the needle only
static bool  gTrackAccInit = false;
static const float TRACK_ACC_ALPHA = 0.35f;   // per 20 ms sample: ~100 ms to follow a turn

//  WHICH TURN A RAW ANGLE IS ON, when the running count cannot say (after an
//  encoder outage, and at boot). The sensor is absolute within one turn only.
//
//  THE MEASURED TUNER ENDS FIRST. The tuner's whole travel
//  is about 2300 counts, well inside one 4096-count turn, so once both ends are
//  measured exactly one turn puts the shaft inside them - whatever happened
//  during the outage, or while the set was unplugged. Nearest-to-the-last-count
//  was right only for movement under half a turn; beyond that it chose the
//  wrong turn, saved it as lastAngle, and every boot after inherited it.
//  The window is the measured span plus a margin each side; it must stay
//  narrower than a turn to be unambiguous, so the placeholder ends (0..6023)
//  and anything else too wide fall back to the nearest turn, as before.
static const int32_t SEAT_MARGIN = 300;
static bool gEndsMeasured = false;          // both tuner ends really measured - see setCalibration
static int32_t seatTurn(uint16_t raw, int32_t ref) {
  int32_t k = (int32_t)lroundf((float)(ref - (int32_t)raw) / 4096.0f);
  int32_t nearest = (int32_t)raw + k * 4096;
  int32_t lo = (calLow < calHigh ? calLow : calHigh) - SEAT_MARGIN;
  int32_t hi = (calLow < calHigh ? calHigh : calLow) + SEAT_MARGIN;
  if (!gEndsMeasured || hi - lo >= 4096) return nearest;   // ends unknown or implausible
  for (int32_t d = -1; d <= 1; d++) {
    int32_t c = nearest + d * 4096;
    if (c >= lo && c <= hi) return c;              // the only turn inside the travel
  }
  return nearest;                                  // outside the measured travel
}

static bool readAs5600() {
  if (!busReady()) { angleFresh = false; return false; }
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(REG_RAW_ANGLE);
  if (Wire.endTransmission(false) != 0) { busFail(); return false; }
  if (Wire.requestFrom((int)AS5600_ADDR, 2) != 2) { busFail(); return false; }
  uint16_t hi = Wire.read(), lo = Wire.read();
  uint16_t a = ((hi << 8) | lo) & 0x0FFF;
  //  busGood() - which makes i2cOk() true again - comes AFTER the re-seat
  //  below, never before: in between, a mark or an end capture on another
  //  task would have been allowed against the still-frozen count.

  //  RE-REFERENCE AFTER ANY GAP - DO NOT ACCUMULATE ACROSS ONE.
  //
  //  busFail() parks the sensor for 2000 ms and the early return above left
  //  rawAngle untouched, so the first read afterwards computed a delta against
  //  a two-second-old reference and then pushed it through a +/-2048 wrap
  //  correction that assumes a 20 ms step. Half a revolution in two seconds is
  //  15 rpm - an ordinary slow turn of the knob - so ONE I2C stumble during a
  //  calibration could silently inject a whole 4096-count revolution.
  //
  //  NOT DROPPED ANY MORE - RE-SEATED. Dropping the
  //  movement left accAngle shifted by it until the next boot, so every sample
  //  stored after one stumble with the knob moving sat in a shifted frame, and
  //  the sampler's drift check could not see it. The sensor is absolute within
  //  a turn, so only the TURN is unknown after a gap: seatTurn() picks it - the
  //  one turn inside the measured tuner ends when both are measured, otherwise
  //  the one nearest the last count (the same rule seedAccumulator() uses at
  //  boot). The movement now shows as movement, which is what the drift check
  //  exists to catch. The console line below still says "nearest turn" either
  //  way.
  if (!angleFresh) {
    rawAngle   = a;
    angleFresh = true;
    if (accInit) {
      //  seatTurn()'s reference is the last TRUE count - normally accAngle,
      //  but if the very first read at boot failed, accAngle is only a
      //  placeholder seeded from a stale rawAngle, and the saved angle
      //  (gSeedAnchor) is the one to aim at.
      int32_t was   = accAngle;
      int32_t ref   = gSeedPending ? gSeedAnchor : was;
      gSeedPending  = false;
      accAngle = seatTurn(a, ref);
      //  A move hidden by the outage IS tuning: say so, so the still-timers
      //  (auto sample, recovery) start over and the readout lights.
      int32_t mv = accAngle - was;
      if (mv >= TUNE_DEADBAND || mv <= -TUNE_DEADBAND) lastTurnMs = millis();
      Con.printf("  [WARN] AS5600 gap - angle re-seated on the turn inside the tuner's ends "
                 "(moved %ld counts during the outage).\n", (long)(accAngle - was));
    }
    busGood();
    return true;
  }
  busGood();

  int32_t d = (int32_t)a - (int32_t)rawAngle;
  if (d >  2048) d -= 4096;
  if (d < -2048) d += 4096;
  if (accInit) {
    accAngle += d;
    //  THE NEEDLE FOLLOWS A FILTERED SHAFT, everything else the raw one. One AS5600
    //  sample of noise used to move the needle target 2-3 half-steps across a
    //  rounding boundary - past TRACK_DEADBAND_HS - and the motor went: pos/tgt
    //  flipping -481/-484 every few minutes at idle, and 227-363-step HUNTING
    //  bursts on the first tracking ticks after a boot (2026-09-23/24). A real
    //  turn pushes the same way sample after sample and arrives within ~100 ms;
    //  a one-sample spike is cut to about a third. tuningActive(), the RDA
    //  sampler and every readout stay on accAngle, untouched.
    if (!gTrackAccInit) { gTrackAcc = (float)accAngle; gTrackAccInit = true; }
    else gTrackAcc += ((float)accAngle - gTrackAcc) * TRACK_ACC_ALPHA;
  }
  //  A SLOW TURN IS STILL A TURN.
  //
  //  This compared ONE POLL's delta against TUNE_DEADBAND, and the sensor is
  //  polled at 50 Hz - so tripping it needed 12 counts of 4096 inside 20 ms,
  //  about 53 degrees per second. Turn the knob slower than that and it never
  //  registered as tuning however far it went, so the Leditron never switched
  //  to the readout. Turn it fast and it did. The author reported exactly that,
  //  and noted the NEEDLE tracked the whole time - which is the proof:
  //  accAngle accumulates on every poll regardless of this test, so the angle
  //  was always right and only the motion DETECTOR was rate-gated.
  //
  //  So accumulate instead. Real tuning is directional and adds up; sensor
  //  jitter dithers about zero and reverses, which resets the accumulator
  //  before it can reach the threshold. Same noise rejection, no rate floor.
  uint32_t mag = (uint32_t)(d < 0 ? -d : d);
  if (d != 0) {
    if ((turnAccum > 0 && d < 0) || (turnAccum < 0 && d > 0)) turnAccum = 0;
    turnAccum += d;
    int32_t acc = turnAccum < 0 ? -turnAccum : turnAccum;
    if (mag >= TUNE_DEADBAND || acc >= (int32_t)TUNE_DEADBAND) {
      lastTurnMs = millis();
      turnAccum  = 0;
    }
  }
  rawAngle = a;
  return true;
}

static void readStatus() {
  if (!busReady()) return;
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(REG_STATUS);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom((int)AS5600_ADDR, 1) != 1) return;
  uint8_t s = Wire.read();
  //  KEEP THE RAW BYTE. "NO MAGNET" collapses three bits into one boolean, and
  //  MD clear (no magnet at all) and ML set (air gap too large) want completely
  //  different fixes. Testing both overflow bits means the answer is the same
  //  whichever of ML/MH sits where, so the expression is safe either way.
  statusRaw = s;
  magOk = (s & 0x20) && !(s & 0x10) && !(s & 0x08);
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(REG_AGC);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom((int)AS5600_ADDR, 1) == 1) agcVal = Wire.read();

  //  MAGNITUDE is the direct field-strength number and settles the argument
  //  that AGC alone cannot: AGC says how hard the part is working, this says
  //  what it is working on.
  Wire.beginTransmission(AS5600_ADDR);
  Wire.write(REG_MAGNITUDE);
  if (Wire.endTransmission(false) != 0) return;
  if (Wire.requestFrom((int)AS5600_ADDR, 2) == 2) {
    uint16_t mh = Wire.read(), ml = Wire.read();
    magnitude = ((mh << 8) | ml) & 0x0FFF;
  }
}

static uint16_t anglePermille() {
  int32_t rng = calHigh - calLow, rel = accAngle - calLow;
  if (rng == 0) return 0;
  if (rng < 0) { rng = -rng; rel = -rel; }
  if (rel <= 0)   return 0;
  if (rel >= rng) return 1000;
  return (uint16_t)(((int64_t)rel * 1000) / rng);
}

// ============================================================================
//  HOMING  -  invented here; the rig has none
//
//  The index is a MID-TRAVEL MARK, so the search has to cope with starting on
//  either side of it and with the sensor being absent entirely.
//
//    SEEK_INDEX   hunt in the likelier direction under a STEP BUDGET; if the
//                 budget runs out, try the other way once; then FAULT.
//    BACKOFF      retreat until the sensor releases, plus a clearance, so the
//                 final approach is always from the same side.
//    REAPPROACH   come back up in +10 half-step second-order hops (not at
//                 pReapHsps) and stop on the ON edge. That edge, approached
//                 this way every time, is zero. Homing on one edge in one
//                 direction cancels the sensor's hysteresis completely.
//
//  Gearbox lash is expected to be the larger term anyway - larger than the
//  Hall's own hysteresis - which is another reason the approach is always
//  one-directional.
// ============================================================================
static uint8_t  gHomePhase   = 0;
static uint8_t  gSweepPhase  = 0;
static bool     gSweepRepeat = false;   // keep going until told to stop
static int32_t  gPhaseStart  = 0;      // half-steps, for the per-phase budgets
static int8_t   gHomeDir     = -1;

//  EVERY homing phase is budgeted, not just the search. There is no switch
//  behind any of this any more, and the soft limits cannot help while zero is
//  still unknown, so a budget is the only thing that can stop a phase that will
//  never satisfy its exit condition.
static char gFaultWhy[64] = {0};
const char *faultReason() { return gFaultWhy; }

//  Declared here rather than below, because homeFault() now returns the
//  needle to whatever it was doing when an automatic recovery fails.
static State    gAfter       = PARKED;

static void homeFault(const __FlashStringHelper *why) {
  //  A FAILED AUTOMATIC RECOVERY IS NOT A FAULT, THE THIRD ONE IS.
  //
  //  There is no cleverer search to escalate to: SEEK_INDEX already tries the
  //  OTHER direction under budget before it gives up, so a re-index that failed
  //  has already looked both ways. What is left is to try again later, and the
  //  needle is no worse off than before it started - same frame, same accuracy,
  //  a little motion spent.
  //
  //  RE-ARM THE LIMITS ON THE WAY OUT. beginReindex() disarmed them, and every
  //  other path that abandons a home (stop(), a jog, a FAULT) leaves them
  //  disarmed - which is how a
  //  wrong frame becomes an UNBOUNDED one. Bounded-and-wrong is enormously
  //  better than unbounded: the limits are still the right distance apart, they
  //  are just in the wrong place, and that costs accuracy rather than gearbox.
  if (gAutoHome) {
    gAutoHome    = false;
    gLimitsArmed = true;
    if (++gAutoTries < AUTO_TRIES_MAX) {
      isHomed       = true;        // back to the frame we had; no worse than before
      gDriftPending = true;        // so the trigger comes round again
      //  THE CLUE IS STILL THERE, 2026-09-25. gIdxEvid was never cleared by
      //  beginReindex(), and gPos is still counting in the frame it was
      //  observed in (only REAPPROACH's success moves the frame) - so the next
      //  attempt steers by it, or by the sighting this attempt made on the way.
      gDone = true; gVel = 0.0f; gTarget = gPos;
      st = gAfter;
      Con.printf("  needle: re-index attempt %u failed - will try again.\n",
                    (unsigned)gAutoTries);
      return;
    }
    Con.println(F("  needle: three re-index attempts failed. Something is"));
    Con.println(F("          mechanically wrong - the dial cannot be trusted."));
  }
  strncpy_P(gFaultWhy, (PGM_P)why, sizeof(gFaultWhy) - 1);
  gFaultWhy[sizeof(gFaultWhy) - 1] = 0;
  st = FAULT;
  gDone = true;
  gVel  = 0;
  Drive::release();
  Con.print(F("  [FAIL] homing gave up: "));
  Con.println(why);
  Con.println(F("         Nothing will be driven. Check the A3144, its magnet"));
  Con.println(F("         polarity, the 3V3 pull-up, and that the motor is plugged in."));
}
static int32_t  gHomeBudget  = 0;
static bool     gTriedOther  = false;


//  THE POWER-UP POSITION IS NOT A MYSTERY ANY MORE.
//  gPos starts at 0 on a cold boot, so startHoming() read "we are at the index"
//  and searched DOWN - straight into the low stop, where the needle was already
//  sitting, for a full span before it gave up and tried the other way. That is
//  what "index never found in either direction" was on 2026-08-31: not a broken
//  sensor, a search that began by pushing against a wall.
//
//  Under the author's policy the needle is ALWAYS at the low stop when the set
//  is off, so say so, and the first pass goes the right way.
//  A DIRECTION HINT, NOT A POSITION. This makes gPos negative so startHoming()
//  picks +1 as the search direction; nothing else may trust it. It is wrong by
//  up to the whole travel (the needle is physically at the low STOP, which may
//  be far below pPosMin when the limits are provisional), and the only thing
//  that keeps that harmless is applyNeedleMode() returning early on !homed().
//  Do not "improve" this into something read more widely.
//
//  ONLY AFTER A COLD START, 2026-09-25. The policy above is about the SET being
//  switched off, and a software restart does not switch it off: the needle
//  stays wherever it was tracking. main.cpp now calls bootPosition(), which
//  falls back to this when the reset was a power-on (or anything else that is
//  not a software restart) or the record did not survive.
static bool gMemArmed = false;           //  see THE NEEDLE'S MEMORY, below

void assumeAtLowStop() {
  gPos    = pPosMin * FINE_PER_HALFSTEP;
  gTarget = gPos;
  gMemArmed = true;                      //  the frame is set; start recording it
}

// ============================================================================
//  THE NEEDLE'S MEMORY ACROSS A SOFTWARE RESET, 2026-09-25.
//
//  setup() used to call assumeAtLowStop() on every boot. That is right after a
//  power cut - the set parks the needle at the low stop whenever it is off - and
//  wrong after every other kind of reset, because a portal reboot, an OTA, a
//  console reset, a crash or a watchdog does not move the needle at all: it
//  stays wherever it was tracking. Boot then believed "low stop", searched UP,
//  and from anywhere above the index that is AWAY from it - into the high stop,
//  674 half-steps above the index, with the limits disarmed and a 2336 budget.
//  Seen on the machine as "index not found in the expected direction" on OTA
//  reboots with the needle at 301.
//
//  So the needle task keeps a small record of where the frame believes the
//  needle is, in RTC memory that a software reset does not clear, and boot
//  reads it back when - and only when - the reset was one that leaves the
//  needle where it was. Homing still runs afterwards: the record is a better
//  STARTING BELIEF for chooseHomeDir(), not a substitute for finding zero. With
//  a true position the search goes the right way and is short.
//
//  WHICH RESETS. ESP_RST_SW (esp_restart: the portal's reboot, the OTA's,
//  the console's), ESP_RST_PANIC (a crash), ESP_RST_INT_WDT, ESP_RST_TASK_WDT
//  and ESP_RST_WDT (the watchdogs). In every one of those the chip restarts
//  under power and the motor, whose coils merely go slack, stays put to within
//  a detent. NOT ESP_RST_POWERON, ESP_RST_BROWNOUT, ESP_RST_EXT,
//  ESP_RST_DEEPSLEEP, ESP_RST_SDIO or ESP_RST_UNKNOWN: a power-on means the set
//  was off and the parked-low policy is the better belief; a brownout means the
//  supply sagged, and a count kept through a supply sag is not one to steer
//  by, whatever the memory kept; the reset pin, deep sleep and "unknown" are not
//  events this machine produces in service, and for anything unexplained the
//  old behaviour is the conservative answer.
//
//  A TORN RECORD IS DETECTED, NOT TRUSTED. A reset can land in the middle of
//  an update. So the record is invalidated first (magic = 0), the fields are
//  written, the checksum over them is written, and the magic is written LAST.
//  Every member is volatile, so the compiler keeps those stores in that order
//  and the Xtensa port puts a memw ahead of each. A reset anywhere inside the
//  sequence leaves magic = 0, and the checksum also catches the random
//  contents RTC memory holds after a power-on and a record left by a firmware
//  whose layout differs (change MEM_MAGIC if this struct ever changes).
//
//  NEVER REUSED TWICE. bootPosition() reads the record and invalidates it
//  before it does anything else, and the needle task does not write a new one
//  until bootPosition() (or assumeAtLowStop()) has set the frame - gMemArmed.
//  That gate matters: needleTask is started by begin(), well before setup()
//  reaches bootPosition(), and without it the first pass would overwrite the
//  record with the cold gPos of 0 before anything read it.
//
//  THE FRAME IN THE RECORD WHEN IT SAYS "NOT HOMED". The record is whatever
//  frame gPos was counting in, homed or not. Mid-homing that is the frame the
//  search started from (assumeAtLowStop()'s, or a remembered one, or the
//  suspect one a slip left behind) PLUS every step since - still the best
//  belief the machine had, and the one any clue was observed in. So it is
//  used the same way: seed gPos from it, and if a clue was on record, restore
//  the clue too, which chooseHomeDir() prefers. The one instant the frame is
//  genuinely in flight - REAPPROACH has handed the new zero to stepTask but it
//  is not yet folded in (gZeroPending) - is simply not recorded: the record
//  keeps the old frame until the new one is complete and homed, then changes
//  from one to the other in a single update.
//
//  FINE UNITS, not half-steps: seeding the exact fine position keeps the drive's
//  electrical phase where the rotor actually sits, where a half-step value
//  would move it by up to one half-step on the first apply().
// ============================================================================
struct NeedleMemory {
  uint32_t magic;       //  MEM_MAGIC while the record is whole, 0 while it is written
  int32_t  posFine;     //  gPos, in the frame the needle believed
  uint8_t  homed;       //  was that frame a homed one
  uint8_t  evidValid;   //  gIdxEvid
  int32_t  evidHs;      //  gIndexBelieved, half-steps, same frame as posFine
  uint32_t check;       //  memSum() of the fields above, written before the magic
};
static const uint32_t MEM_MAGIC = 0x4E4D3031;     //  "NM01"
static RTC_NOINIT_ATTR volatile NeedleMemory gMem;

//  FNV-1a over the fields, never over the struct's bytes: padding is not
//  guaranteed to hold anything in particular.
static uint32_t memSum(uint32_t magic, int32_t pos, uint8_t homed, uint8_t ev, int32_t evHs) {
  const uint32_t w[4] = { magic, (uint32_t)pos,
                          (uint32_t)homed | ((uint32_t)ev << 8), (uint32_t)evHs };
  uint32_t h = 2166136261u;
  for (int i = 0; i < 4; i++)
    for (int b = 0; b < 32; b += 8) { h ^= (w[i] >> b) & 0xFFu; h *= 16777619u; }
  return h;
}

//  What was last written, so an unchanged pass costs four compares and no store.
static int32_t gMemPos = 0, gMemEvHs = 0;
static uint8_t gMemHomed = 0xFF, gMemEv = 0xFF;   //  0xFF: nothing written yet

//  CALLED FROM needleTask ONLY, once per pass. The one writer.
static void memRecord() {
  if (!gMemArmed || gZeroPending) return;
  const int32_t p  = gPos;
  const uint8_t h  = isHomed  ? 1 : 0;
  const uint8_t e  = gIdxEvid ? 1 : 0;
  const int32_t eh = gIdxEvid ? gIndexBelieved : 0;
  if (p == gMemPos && h == gMemHomed && e == gMemEv && eh == gMemEvHs) return;
  gMem.magic     = 0;                          //  INVALID from here...
  gMem.posFine   = p;
  gMem.homed     = h;
  gMem.evidValid = e;
  gMem.evidHs    = eh;
  gMem.check     = memSum(MEM_MAGIC, p, h, e, eh);
  gMem.magic     = MEM_MAGIC;                  //  ...to here.
  gMemPos = p; gMemHomed = h; gMemEv = e; gMemEvHs = eh;
}

static bool resetKeepsNeedle(esp_reset_reason_t r) {
  switch (r) {
    case ESP_RST_SW:
    case ESP_RST_PANIC:
    case ESP_RST_INT_WDT:
    case ESP_RST_TASK_WDT:
    case ESP_RST_WDT:
      return true;
    default:
      return false;
  }
}

//  A sanity bound on a record that passed its checksum: the whole travel is
//  under 1900 half-steps and a search adds at most 2336 to wherever the frame
//  started, so anything past this is not a position this machine can hold.
static const int32_t MEM_POS_SANE_HS = 6000;

void bootPosition() {
  const esp_reset_reason_t why = esp_reset_reason();
  //  READ, THEN INVALIDATE, before anything else can happen.
  const uint32_t mg  = gMem.magic;
  const int32_t  pos = gMem.posFine;
  const uint8_t  h   = gMem.homed;
  const uint8_t  e   = gMem.evidValid;
  const int32_t  eh  = gMem.evidHs;
  const uint32_t ck  = gMem.check;
  gMem.magic = 0;
  gMem.check = 0;

  const int32_t posHs = pos / FINE_PER_HALFSTEP;
  const bool whole = (mg == MEM_MAGIC) && (ck == memSum(mg, pos, h, e, eh)) &&
                     h <= 1 && e <= 1 &&
                     posHs > -MEM_POS_SANE_HS && posHs < MEM_POS_SANE_HS &&
                     eh    > -MEM_POS_SANE_HS && eh    < MEM_POS_SANE_HS;

  if (!resetKeepsNeedle(why)) { assumeAtLowStop(); return; }
  if (!whole) {
    Con.println(F("  needle: software restart, but no whole record of where it was -"));
    Con.println(F("          assuming the low stop, as after a power-up."));
    assumeAtLowStop();
    return;
  }

  gPos    = (int32_t)Drive::snap(pos);     //  onto the current drive lattice
  gTarget = gPos;
  if (e) believeIndexAt(eh);
  Con.printf("  needle: remembered at %ld (software restart) - homing from there.\n",
             (long)posHs);
  if (!h)
    Con.println(F("          (it had not finished homing when it restarted; that frame is the best guess)"));
  if (e)
    Con.printf("          a clue was on record: the index was last seen at %ld.\n", (long)eh);
  gMemArmed = true;
}

//  Snapshotted when homing FINISHES, so the capture guard insists on a measured
//  crossing that happened after it. This used to be taken at the START of
//  homing and compared against gCross - but REAPPROACH does its own gCross++ on
//  the way to declaring zero, so the test was satisfied by homing itself and
//  was unconditionally true the instant H succeeded, with gDrift still 0. The
//  comment in main.cpp claiming the post-home SWEEP is what satisfies it was
//  therefore describing an intent the code did not implement.
static uint32_t gMeasAtHome = 0;

//  ONE DIRECTION CHOICE, not two copies of it. A re-index often has better
//  evidence than the sign of gPos - a sighting that MEASURED where the index
//  is, gIndexBelieved. That is not exclusive to a recovery: if the same
//  measurement is sitting there
//  when a hand-driven H is pressed, it is still better than guessing from a
//  frame we have every reason to doubt. So both callers go through this, and
//  only fall back to the sign of gPos when no crossing has said otherwise.
//
//  GATED ON THE CLUE, NOT ON gDriftPending, 2026-09-25. This used to read
//  gIndexBelieved only while gDriftPending was true, and warned its callers to
//  ask before they cleared it. But gDriftPending means "a recovery is wanted",
//  not "we know where the index is", and it is cleared by the first re-index
//  to START - so a re-index stopped part-way (an OTA's quiet, a stop from the
//  console or the portal) came back through here with the flag down and fell
//  back to the sign of a frame the slip had just proved wrong. gIdxEvid lives
//  until the frame is made true (see its note), so every home - hand-driven,
//  automatic, after a stop, after a software restart that remembered it -
//  steers by the best observation there is. The order of the clears in the
//  callers no longer matters.
static int8_t chooseHomeDir(int64_t posFine, int32_t hereHs) {
  if (gIdxEvid) return (hereHs > gIndexBelieved) ? -1 : +1;
  return (posFine >= 0) ? -1 : +1;
}

//  REFUSES WHILE SOMETHING ELSE HOLDS THE NEEDLE, 2026-09-25 (author: make it
//  resilient). startReindex() always had these two guards and this did not:
//  H pressed during a band calibration disarmed the soft limits and unhomed
//  the frame underneath a creep that was still running, and during a held
//  coil test it set a homing state the needle task would not service - while
//  the console said "homing." Now it refuses, and says WHY, the way
//  calStartBand() does: nullptr when it started, otherwise a static reason
//  string the caller shows. Also printed here, so a caller that ignores the
//  answer (track()/park()) still leaves a visible trace on the console.
//  The boot home and the amp-on home are unaffected in practice: neither a
//  calibration (which needs a homed frame to start) nor a bring-up tool is
//  running at those moments, and if one is, refusing is the right answer.
const char *startHoming() {
  const char *why = nullptr;
  if      (calBusy())  why = "a calibration holds the needle - let it finish or stop it (x) first";
  else if (gTestMode)  why = "a bring-up tool holds the needle - release the coil (0) first";
  if (why) {
    Con.print(F("  homing REFUSED: "));
    Con.println(why);
    return why;
  }
  //  THE DIRECTION, before anything below clears the evidence it may need.
  int32_t here = (int32_t)(gPos / FINE_PER_HALFSTEP);
  gHomeDir = chooseHomeDir(gPos, here);
  //  gDrift is evidence, and evidence from before a re-home is worthless.
  //  Leaving it stale meant the capture guard refused with "re-home first"
  //  immediately AFTER a re-home, using the -380 from before it - a loop with
  //  no exit. And a fresh boot with gDrift still 0 passed the gate on no
  //  evidence at all, which proves only that nothing has contradicted us yet.
  //  NOT gIdxEvid/gIndexBelieved (2026-09-25). gDrift is a verdict on the old
  //  frame and dies with it; WHERE the index was seen is a fact about the
  //  mechanism, still true in the frame gPos keeps counting in until this home
  //  succeeds - and if it is stopped first, the next home needs it.
  gDrift = 0;
  gDriftPending = false;
  isHomed      = false;
  gLimitsArmed = false;
  gTriedOther  = false;
  gHomePhase   = 0;
  gAutoHome    = false;      // a hand-driven home is not a recovery attempt
  gAutoTries   = 0;
  //  A NEW ATTEMPT RETIRES THE OLD FAULT, 2026-09-25. gFaultWhy was cleared only
  //  when REAPPROACH succeeded, so for the whole of a fresh home the portal
  //  kept showing why the LAST one gave up - and main.cpp's otaResume, which
  //  reads faultReason() to mean "homing has given up, leave it for a person",
  //  refused to re-home a needle that was in fact homing. The reason stays
  //  true only until someone tries again; if this attempt fails, homeFault()
  //  writes its own.
  gFaultWhy[0] = 0;
  //  A HAND-DRIVEN HOME ENDS A REPEATING SWEEP, 2026-09-24. gSweepRepeat was
  //  cleared by stop() and beginReindex() but not here, so H pressed during a
  //  sweepRange(true) re-homed and then ran the closing sweep - which reads the
  //  flag - end to end for ever. The home is a fresh start; the sweep it ends
  //  with is the single power-up one.
  gSweepRepeat = false;
  gSweepPhase  = 0;
  gPhaseStart  = here;
  //  If the magnet is already over the sensor, the search is skipped entirely
  //  and homing begins by backing off.
  //  That is also a SIGHTING (2026-09-25): the sensor is ON, so the index is
  //  here, in the frame gPos is counting in. Recorded as the newest clue, so a
  //  home stopped before REAPPROACH still leaves the next one pointed at it.
  gZeroPending = false;
  if (idxState()) { believeIndexAt(here); st = BACKOFF; }
  else            st = SEEK_INDEX;
  return nullptr;
}

//  RE-INDEX: homing that knows which way the index is.
//
//  startHoming() used to pick its search direction from the SIGN OF gPos
//  alone, and mid session that is the one thing we have just established we
//  cannot trust - the frame being wrong is the whole reason we are here. If
//  the frame lies about the sign, SEEK_INDEX drives the full search budget
//  AWAY from the index, and the high stop is only 674 half-steps away. So
//  this always goes through chooseHomeDir(), which prefers the crossing that
//  measured gIndexBelieved. An earlier note here said this function only ever
//  runs with gDriftPending true; it never did - the routine idle check and the
//  portal's needle.reindex button both call it on a healthy frame. With no
//  clue on record (gIdxEvid false) the sign of gPos is the right answer there,
//  because nothing has said the frame is wrong.
//
//  Everything after that is the ordinary homing machine: back off until the
//  sensor releases, then come back up in +10 half-step hops and take the ON
//  edge. Zero is DEFINED by that short-hop approach - a crossing taken at
//  tracking speed carries sampling lag that is compensated but not eliminated,
//  which is exactly why this rung goes and looks rather than believing the
//  number.
//
//  Reached only with a homed frame: startReindex() runs a plain home when
//  there is no zero, and sweepSlipped() only runs during a sweep, which needs
//  one.
//
//  Returns false if it will not start; the caller decides what that means.
//
//  THE BODY IS SEPARATE FROM THE GUARDS, 2026-09-24, because there are now two
//  kinds of caller and they must not share the SWEEP refusal. A caller from
//  OUTSIDE (the idle check, the 3 s recovery tick, the portal button) is
//  refused during the sweep - the sweep owns the needle, and the routine idle
//  check in particular must never cut a healthy sweep short. But a slip found
//  DURING the sweep is the one case where the sweep is the problem: it was
//  raising gDriftPending and then carrying on to pPosMin/pPosMax in the very
//  frame it had just proved wrong - for ever, with sweepRange(true) - because
//  this function refused it and the SWEEP branch never looked. That path now
//  calls beginReindex() directly from inside the needle task; see
//  sweepSlipped().
static void beginReindex(bool routine) {
  gDone = true; gVel = 0.0f; gTarget = gPos;      // stop cleanly first
  gSweepRepeat = false;                           // a sticky sweep must not resume
  gSweepPhase  = 0;
  //  THE DIRECTION. gDriftPending is cleared below - the recovery it asked for
  //  is now running - but the clue it was raised with, gIdxEvid, is not: if
  //  this re-index is stopped before zero is found, the next home still needs
  //  to know which way to go (2026-09-25, see gIdxEvid).
  int32_t here = (int32_t)(gPos / FINE_PER_HALFSTEP);
  gHomeDir = chooseHomeDir(gPos, here);
  gDrift = 0;
  gDriftPending = false;
  isHomed      = false;
  gLimitsArmed = false;
  gTriedOther  = false;
  gHomePhase   = 0;
  gAutoHome    = true;
  gPhaseStart  = here;
  //  A new attempt retires the old fault, as in startHoming() (2026-09-25).
  gFaultWhy[0] = 0;
  //  On the sensor already: that is a sighting, as in startHoming().
  gZeroPending = false;
  if (idxState()) { believeIndexAt(here); st = BACKOFF; }
  else            st = SEEK_INDEX;
  if (routine) Con.println(F("  needle: routine index check - the amp has been off two minutes."));
  else if (gIdxEvid)
    Con.printf("  needle: re-indexing (the index should be %ld half-steps from here).\n",
               (long)(gIndexBelieved - here));
  //  No clue on record - the portal's button on a healthy frame. Printing
  //  gIndexBelieved here would print a number nobody measured.
  else Con.println(F("  needle: re-indexing (no slip on record - searching by the frame)."));
}

bool startReindex(bool routine) {
  if (calBusy() || gTestMode) return false;       // guards startHoming() lacks
  //  SWEEP too: the power-up sweep owns the needle until it ends, and aborting it
  //  here leaves homing to restart from wherever it stopped. Every other busy
  //  check in this file (needleBusy) already included it; this one did not.
  //  A slip found DURING the sweep does not come through here - see above.
  if (st == SEEK_INDEX || st == BACKOFF || st == REAPPROACH || st == SWEEP) return false;
  //  NO ZERO, NO RE-INDEX. A re-index is a recovery
  //  attempt, and a failed one hands back "the frame we had" (homeFault) - but
  //  a needle in FAULT from a failed home never had one, so a failed re-index
  //  declared an unmeasured frame homed and tracked in it, soft limits and
  //  all. With no zero, the button runs a plain home instead: one that fails
  //  goes straight back to FAULT, pretending nothing.
  if (!isHomed) {
    Con.println(F("  needle: no zero to re-index from - homing instead."));
    return startHoming() == nullptr;
  }
  beginReindex(routine);
  return true;
}

//  A SLIP DURING THE SWEEP ENDS THE SWEEP AT ONCE. Author's ruling:
//  no 3 s wait for the dial to be still, no finishing the leg. The sweep drives
//  to the soft limits, and the soft limits are positions in the frame the slip
//  has just shown to be wrong - so every further half-step of it is a half-step
//  toward a stop the firmware can no longer locate. beginReindex() stops the
//  motion, drops the repeat flag and starts the ordinary homing machine with
//  the limits disarmed; gAutoHome makes it return to gAfter with no second
//  sweep once zero is found again.
static void sweepSlipped() {
  Con.println(F("  needle: slip during the sweep - sweep abandoned, re-indexing now."));
  beginReindex(false);
}

static void homingTick() {

  switch (st) {
    case SEEK_INDEX:
      if (idxState()) {
        st = BACKOFF; gDone = true; gVel = 0; gHomePhase = 0;
        gPhaseStart = (int32_t)(gPos / FINE_PER_HALFSTEP);
        //  FOUND IT - WRITE IT DOWN, 2026-09-25. The search's whole product is
        //  this sighting, and it used to be thrown away if the home was stopped
        //  or failed before REAPPROACH turned it into a zero. It is the newest
        //  and best clue there is; see gIdxEvid.
        believeIndexAt(gPhaseStart);
        break;
      }
      if (moving()) break;
      if (gHomePhase == 0) {
        //  THE SEARCH BUDGET IS A PROPERTY OF THE MECHANISM, NOT OF THE
        //  CALIBRATION. It used to be span + span/4 from the SOFT LIMITS - the
        //  same `span` the sweep legs use - and the two want opposite things:
        //  the sweep MUST track the limits (it is a tour of the known travel),
        //  the search must NOT (the limits are exactly what is unknown while
        //  homing runs). With the provisional -300..+300 that coupling gave a
        //  750 half-step search against a needle parked ~1195 below the index:
        //  it ran out upward, tried the other way INTO the low stop, and
        //  faulted. Seen on the first boot after the purge.
        //
        //  HOME_BUDGET_HS is justified on its own terms: it must exceed the
        //  measured travel of 1869 or homing cannot work from a stop,
        //  and 25 % over a measured quantity is an ordinary margin. In the
        //  normal case the budget is never spent at all - SEEK_INDEX breaks
        //  out the moment the sensor asserts, so from the low stop the first
        //  pass travels ~1195 and stops. The budget is only consumed when the
        //  parked-low assumption is false or the sensor is dead.
        gHomeBudget = HOME_BUDGET_HS;
        gHomePhase  = 1;
        moveToHalf((int32_t)(gPos / FINE_PER_HALFSTEP) + gHomeDir * gHomeBudget, false);
      } else {
        //  Budget exhausted without finding it.
        if (!gTriedOther) {
          //  The first direction is not a guess - a clue on record, the
          //  position remembered across a software restart, or assumeAtLowStop()
          //  after a cold boot makes it known (2026-09-25: the message named
          //  only the last, and on an OTA reboot it was blaming a policy that
          //  had not been consulted). Running out means that belief was wrong
          //  or the sensor is dead, and the reverse pass can grind a full
          //  budget into the other stop. Say so before it does.
          Con.println(F("  [WARN] index not found in the expected direction - the needle"));
          Con.println(F("         was not where the frame believed. Trying the other way."));
          gTriedOther = true; gHomeDir = -gHomeDir; gHomePhase = 0;
        }
        else {
          homeFault(F("index never found in either direction"));
        }
      }
      break;

    case BACKOFF:
      if (!idxState() && !moving() && gHomePhase == 1) {
        st = REAPPROACH; gHomePhase = 0;
        gPhaseStart = (int32_t)(gPos / FINE_PER_HALFSTEP);
        break;
      }
      if (moving()) break;
      //  BUDGETED, and this was a real hole. If the sensor is already ON at
      //  boot, homing starts here - and a magnet that never releases (a stuck
      //  sensor, a shorted line, or a disconnected motor) would otherwise
      //  retreat forever, because the soft limits are deliberately disarmed
      //  while zero is still unknown. Every homing phase needs its own budget,
      //  not just the search.
      if (labs((long)((int32_t)(gPos / FINE_PER_HALFSTEP) - gPhaseStart)) > HOME_BACKOFF_BUDGET_HS) {
        homeFault(F("sensor never released during backoff"));
        break;
      }
      //  Retreat opposite the canonical approach, a little at a time, until the
      //  sensor lets go - then a fixed clearance so the creep starts clear.
      if (idxState()) moveToHalf((int32_t)(gPos / FINE_PER_HALFSTEP) - 20, false);
      else { gHomePhase = 1; moveToHalf((int32_t)(gPos / FINE_PER_HALFSTEP) - 60, false); }
      break;

    case REAPPROACH:
      //  idxEdgeIsNew, not just idxState(): REAPPROACH's ON check used to run
      //  every tick once the sensor was ON, harmlessly, because indexStable()
      //  was cheap and idempotent. idxState() is just as idempotent, but the
      //  position we want is idxEdge, which is only fresh on the poll that
      //  confirmed it - gate on that so a second look at an already-confirmed
      //  ON never re-reads (and re-applies) a stale snapshot.
      //  ZERO HANDED OVER, NOT YET APPLIED. Measured on the machine 2026-09-23,
      //  first boot of the debounce: REAPPROACH wrote the new frame into
      //  gCorrectHs and in the SAME tick marked the needle homed and planned the
      //  sweep - before stepTask had folded the shift in. The crossing check then
      //  scored the very edge that had just set zero against the old frame and
      //  reported the whole shift as drift ("-1226 half-steps ... Something
      //  slipped"), and the sweep was planned in the old frame and had the frame
      //  jump under it mid-move: 41566 steps in 4 s for a net of one. So nothing
      //  is marked, measured or planned until gCorrectHs reads 0 - stepTask
      //  folds it at the top of its next loop, microseconds away, and gDone
      //  keeps the motor still meanwhile.
      if (gZeroPending) {
        if (gCorrectHs != 0) break;
        gZeroPending = false;
        gTarget = gPos;                  // the new frame, as stepTask now holds it
        isHomed = true;
        gLimitsArmed = true;
        //  THE FRAME IS NOW TRUE, so the clue has done its job (2026-09-25).
        //  Every sighting on record was taken in the frame just replaced; the
        //  index is at 0 in this one by definition. See gIdxEvid.
        forgetIndexClue();
        gCross++;
        //  The reference is fresh as of THIS instant; nothing measured before
        //  it counts toward the capture guard.
        gMeasAtHome = gMeas;
        gFaultWhy[0] = 0;      // no longer true, and it was still being reported
        Con.println(F("  index found; zero set on the forward ON edge."));
        //  NO CLOSING SWEEP ON AN AUTOMATIC RECOVERY. The full-travel sweep is
        //  power-up theatre and takes seconds; the whole point of this rung is
        //  that it happens quietly between one thing and the next.
        if (gAutoHome) {
          gAutoHome  = false;
          gAutoTries = 0;          // it worked; the count starts again
          st = gAfter;
          break;
        }
        gSweepPhase = 0;
        st = SWEEP;
        break;
      }
      if (idxEdgeIsNew && idxState()) {
        //  THIS IS ZERO. One edge, one direction, every time.
        gDone = true; gVel = 0;
        //  BACK-DATED. idxEdge is where the needle WAS at the FIRST poll of
        //  the run that just got confirmed - not here, IDX_DEBOUNCE_K-1 polls
        //  of confirmation later, during which a fast approach keeps moving.
        //  Shift the whole frame so THAT position reads pOnFwd; whatever the
        //  needle travelled since (gPos now, minus idxEdge.pos) rides along
        //  unchanged, so the debounce adds no bias to where zero lands.
        //
        //  gPos IS THE STEP TASK'S, on the other core (see the note above
        //  gCorrectHs) - so the shift is handed to it exactly the way every
        //  other correction is, never written here directly. The direct write
        //  this replaced (`gPos = pOnFwd * FINE_PER_HALFSTEP`) raced stepTask,
        //  which folds gCorrectHs in UNCONDITIONALLY, every loop, even while
        //  gDone is true (and gPos was 64-bit then, so the write could also
        //  tear). Safe here for the same reason every other gCorrectHs write
        //  is: one int32, one writer (this task), one reader that only ever
        //  adds it in.
        int64_t shiftFine = idxEdge.pos - (int64_t)pOnFwd * FINE_PER_HALFSTEP;
        int32_t shiftHs   = (int32_t)lrintf((float)shiftFine / (float)FINE_PER_HALFSTEP);
        gCorrectHs = -shiftHs;
        gZeroPending = true;             // completed above, once stepTask has it
        break;
      }
      if (moving()) break;
      //  Budgeted for the same reason as BACKOFF: if the sensor never comes
      //  back the creep would walk the needle off the dial.
      if (labs((long)((int32_t)(gPos / FINE_PER_HALFSTEP) - gPhaseStart)) > HOME_REAPPROACH_BUDGET_HS) {
        homeFault(F("sensor never returned during the re-approach"));
        break;
      }
      //  Creep forward in +10 half-step second-order hops (each with the up
      //  dwell) so the sensor is polled densely.
      moveToHalf((int32_t)(gPos / FINE_PER_HALFSTEP) + 10, false);
      break;

    default: break;
  }
}

// ============================================================================
//  The calibration state machine is defined near the bottom of this file, but
//  the supervisor above has to drive it.
static void calTick();

//  Two subtractions every four seconds. See the note on HUNT_WINDOW_MS.
static void huntTick() {
  uint32_t now = millis();
  int32_t  pos = (int32_t)(gPos / FINE_PER_HALFSTEP);
  if (!gHuntAt) { gHuntAt = now; gHuntSteps = gSteps; gHuntPos = pos; return; }
  if (now - gHuntAt < HUNT_WINDOW_MS) return;

  uint32_t moved = gSteps - gHuntSteps;
  int32_t  net   = pos - gHuntPos;
  if (net < 0) net = -net;

  bool hunting = (moved >= HUNT_STEPS_MIN && net < HUNT_NETMOVE_MAX);
  if (hunting && !gHunting)
    Con.printf("  [WARN] the needle is HUNTING: %lu steps in %lus, net %ld half-steps.\n",
                  (unsigned long)moved, (unsigned long)(HUNT_WINDOW_MS / 1000), (long)net);
  else if (!hunting && gHunting)
    Con.println(F("  the needle has settled."));
  gHunting = hunting;

  gHuntAt = now; gHuntSteps = gSteps; gHuntPos = pos;
}

static void serviceRequests();         //  with stop(), below

static void needleTask(void *) {
  uint32_t lastSense = 0;
  uint8_t  n = 0;

  //  WATCHED (2026-09-25): a supervisor that stops turning leaves the needle
  //  wherever it was, deaf to every request - the watchdog restarts the board
  //  and the needle memory (bootPosition) carries its place across. Fed once
  //  a pass; every branch below sleeps 5-20 ms, so 15 s only trips on a hang.
  esp_task_wdt_add(NULL);

  for (;;) {
    esp_task_wdt_reset();
    //  FIRST, before anything below can act on a state a stop is about to
    //  replace - see gReq. Also before the gTestMode branch, which would
    //  otherwise hold a stop for as long as a bring-up tool runs.
    serviceRequests();
    huntTick();
    //  THE SINGLE PLACE THE INDEX IS SAMPLED FOR MOTION PURPOSES - before
    //  anything below branches on gTestMode, calBusy() or state. This is what
    //  used to be the special-cased `lastIdx = indexStable()` resync at the
    //  top of the gTestMode and calBusy branches below: those existed because
    //  skipping a poll let the edge detector freeze mid-jog or mid-calibration
    //  and then misread the landing as a fresh crossing (seen 2026-09-03).
    //  Sampling every iteration,
    //  unconditionally, removes the freeze instead of patching around it -
    //  idxState() is never more than one poll stale, in ANY mode.
    sampleIndex();

    //  THE BAND CHECK. The sensor can only be ON while the needle is on the index
    //  band, so ON while the frame says the needle is well outside it means the
    //  frame is wrong - steps were lost - whatever the cause. The crossing check
    //  (further down this loop) cannot see this: it scores ON edges only, and
    //  the one loss that mattered (2026-09-24) happened AFTER a correct ON
    //  edge, with the magnet still sitting over the sensor while the sweep
    //  counted 400 steps on. Raised
    //  like any large drift: the index is believed to be HERE, and the recovery
    //  tick re-indexes once the dial is still. Once per excursion, not per poll.
    {
      static bool bandFault = false;
      //  ONLY AGAINST A MEASURED BAND, 2026-09-24. Before the band calibration
      //  has ever run, pOn*/pOff* are all zero, so "the band" is the single
      //  point 0 plus the margin - and the sensor is physically ON across ~100
      //  half-steps of it. Every ordinary crossing then read as a slip, the
      //  re-index put the needle back on the same band, and the next crossing
      //  raised it again: a re-index loop with no exit on an uncalibrated set.
      bool active = isHomed && bandCalibrated() && !calBusy() && !gTestMode &&
                    st != SEEK_INDEX && st != BACKOFF && st != REAPPROACH;
      if (active && idxState()) {
        int32_t here = (int32_t)(gPos / FINE_PER_HALFSTEP);
        //  THE MARGIN GROWS WITH SPEED. idxState() is the debounced level, which
        //  lags the needle by K polls (~20 ms) plus poll jitter, so at speed it
        //  still reads ON well past the band edge. With a fixed 30 the first boot
        //  (2026-09-24) fired at 131 going up at 1100 hsps - 34 past an edge at
        //  97, pure lag. 40 ms of travel covers it; a real loss is hundreds.
        float   vh = fineToHsps(gVel); if (vh < 0) vh = -vh;
        int32_t m  = BAND_MARGIN_HS + (int32_t)(vh * 0.040f);
        int32_t lo = (pOffRev < pOnFwd ? pOffRev : pOnFwd) - m;
        int32_t hi = (pOffFwd > pOnRev ? pOffFwd : pOnRev) + m;
        if ((here < lo || here > hi) && !bandFault) {
          bandFault = true;
          believeIndexAt(here);
          gDriftPending = true;
          Con.printf("  [WARN] the index sensor is ON but the needle is supposedly at %ld\n",
                     (long)here);
          Con.println(F("         - outside the index band. Steps were lost; re-indexing when still."));
        }
      } else {
        bandFault = false;
      }
    }
    //  THE NEEDLE'S MEMORY, every pass and before any branch below can
    //  `continue` past it - a bring-up jog moves gPos too, and a restart in
    //  the middle of one must remember where it left the needle. Four compares
    //  when nothing changed. See THE NEEDLE'S MEMORY above bootPosition().
    memRecord();
    uint32_t nowMs = millis();

    if (nowMs - lastSense >= 20) {
      lastSense = nowMs;
      readAs5600();
      if (++n >= 25) { n = 0; readStatus(); }
    }

    //  A BRING-UP TOOL OWNS THE NEEDLE. jogRaw() drives gPos directly and
    //  gVel stays at 0 throughout, so a crossing seen here has no direction to
    //  check itself against - sampleIndex() above still tracks it (idxState()
    //  is accurate the instant test mode ends), it is just not ACTED on here.
    if (gTestMode) { vTaskDelay(pdMS_TO_TICKS(20)); continue; }

    //  A calibration owns the needle while it runs; the ordinary supervisor
    //  behaviour must not fight it. calTick() reads idxState()/idxEdge itself.
    if (calBusy()) { calTick(); vTaskDelay(pdMS_TO_TICKS(5)); continue; }

    if (st == SEEK_INDEX || st == BACKOFF || st == REAPPROACH) {
      homingTick();
    } else if (st == SWEEP) {
      //  THE POWER-UP FLOURISH, author's spec: go to the low end, run to the
      //  high end, then straight to the tuned position. No return trip - a real
      //  dial does not reverse twice before settling.
      //
      //  BUT NOT IN A FRAME KNOWN TO BE WRONG. The band check above raises
      //  gDriftPending on this same poll; the crossing check below raises it
      //  for the next one, 5 ms later. Either way the sweep stops here, mid-leg,
      //  and the re-index starts - see sweepSlipped().
      if (gDriftPending) {
        sweepSlipped();
      } else if (!moving()) {
        if      (gSweepPhase == 0) { gSweepPhase = 1; useMicro(false); moveToHalf(pPosMin, false); }
        else if (gSweepPhase == 1) { gSweepPhase = 2; moveToHalf(pPosMax, false); }
        else if (gSweepRepeat)     { gSweepPhase = 1; moveToHalf(pPosMin, false); }
        else                       { st = gAfter; }
      }
    } else if (st == TRACKING) {
      if (!moving()) {
        int32_t tgt = trackTarget();
        int32_t cur = (int32_t)(gPos / FINE_PER_HALFSTEP);
        //  A DEADBAND, because `tgt != cur` launched a whole move profile -
        //  accelerate, decelerate, settle - for ONE half-step: a motor asked to
        //  chase the last bit of encoder noise. Two half-steps is far narrower
        //  than the needle itself and is invisible on the glass; what it
        //  removes is AUDIBLE. It was first justified by encoder noise from the
        //  AS5600's magnet (the magnet: Bible §11). The reason it stays at 2 -
        //  one audible profile per half-step - is not about the magnet. Change
        //  it only with a listening test.
        int32_t err = tgt - cur;
        if (err < 0) err = -err;
        if (err >= TRACK_DEADBAND_HS) { useMicro(true); moveToHalf(tgt, false); }
      }
    } else if (st == PARKED) {
      if (!moving()) {
        int32_t cur = (int32_t)(gPos / FINE_PER_HALFSTEP);
        //  THE SAME DEADBAND, FOR A DIFFERENT REASON. pPosMin is a constant so
        //  no sensor noise reaches this test - but if the fine accumulator
        //  settles one half-step short of it, `cur != pPosMin` stays true and
        //  re-commands the park for ever. The same shape as the tracking hunt,
        //  one branch over, found by going and looking for it.
        int32_t perr = cur - pPosMin;
        if (perr < 0) perr = -perr;
        if (perr >= TRACK_DEADBAND_HS) { useMicro(false); moveToHalf(pPosMin, true); }
      }
    }

    //  RE-REFERENCE ON EVERY CONFIRMED CROSSING. Free, and it turns "it does
    //  not lose steps" from a belief into a number that can be watched over
    //  weeks.
    //
    //  idxEdgeIsNew is the whole edge-triggered test: sampleIndex() already
    //  did the debounce and the back-dating at the top of this iteration, so
    //  there is no `idx != lastIdx` left to compute here, and no risk of
    //  reacting to the same confirmed edge twice - it is a one-shot flag,
    //  true for exactly the poll that confirmed it.
    if (gEdgeLog && idxEdgeIsNew) {
      //  ANNOUNCE EVERY EDGE, with the (back-dated) position and direction.
      //  Without this the index is invisible: you cannot jog onto it
      //  deliberately, and the calibration has nothing to aim at.
      Con.print(F("  INDEX "));
      Con.print(idxConfirmed ? F("ON ") : F("OFF"));
      Con.print(F("  at position "));
      Con.print((long)(idxEdge.pos / FINE_PER_HALFSTEP));
      Con.print(F("  moving "));
      Con.println(idxEdge.vel > 1.0f ? F("+") : (idxEdge.vel < -1.0f ? F("-") : F("(still)")));
    }

    if (idxEdgeIsNew && idxConfirmed && isHomed) {
      //  SUBTRACT THE SAMPLING LAG. Back-dating already removed the
      //  IDX_DEBOUNCE_K-1 polls of CONFIRMATION delay - idxEdge.pos is the
      //  run's first poll, not this one. What is left is the ORIGINAL
      //  residue: the edge happened somewhere in the ONE poll interval before
      //  that first sample, uniformly, so its expected position is half that
      //  interval back along the direction of travel - idxEdge.dtUs is
      //  exactly that interval (the gap before the run started), not the
      //  several polls it then took to believe it. Removing the mean leaves a
      //  zero-bias residue instead of a one-sided offset that the correction
      //  below would otherwise bake into gPos on every fast crossing.
      float   lagHs = fineToHsps(idxEdge.vel) * ((float)idxEdge.dtUs * 0.5e-6f);
      int32_t here  = (int32_t)(idxEdge.pos / FINE_PER_HALFSTEP) - (int32_t)lrintf(lagHs);
      gCross++;

      //  A DIRECTIONAL EDGE CANNOT BE CHECKED WITHOUT A DIRECTION.
      //  The two directions trip at different points - that is the Hall's
      //  hysteresis, and it is why homing only ever approaches one way. On this
      //  machine they are 79 half-steps apart, which is TWICE what the
      //  correction below is allowed to absorb, so guessing wrong does not
      //  produce a small error, it produces a confident large one.
      //
      //  `gVel >= 0` guessed FORWARD whenever the velocity was zero - which is
      //  true after every completed move and throughout every bring-up tool.
      //  Use the same threshold the edge log prints with, and when the needle
      //  is not demonstrably moving, count the crossing but conclude nothing.
      //  velAt is idxEdge.vel - the run's first-poll velocity, same reasoning
      //  as the position: it is what the needle was doing when the edge
      //  actually happened, not K polls later.
      //
      //  AND A REVERSE EDGE CANNOT BE CHECKED WITHOUT pOnRev, 2026-09-24. The
      //  forward edge is zero by definition - homing puts it there - so it is
      //  always scoreable. The reverse edge sits ~80 half-steps away, and until
      //  the band calibration has measured where, pOnRev is 0: every reverse
      //  crossing then reported the whole direction offset as a slip, twice
      //  CORRECT_MAX_HS, and triggered a re-index that could only end in the
      //  same place. Counted, not concluded, exactly like a crossing at rest.
      bool scoreable = (idxEdge.vel > 1.0f) ||
                       (idxEdge.vel < -1.0f && bandCalibrated());
      if (scoreable) {
        int32_t expect = (idxEdge.vel > 0.0f) ? pOnFwd : pOnRev;
        gDrift = here - expect;
        gMeas++;

        //  BELIEVE THE SENSOR. Not while a calibration is running - that is
        //  measuring these very edges and must not be corrected underneath.
        if (!calBusy()) {
          if (gDrift <= CORRECT_MAX_HS && gDrift >= -CORRECT_MAX_HS) {
            if (gDrift) gCorrectHs = -gDrift;
            //  ABSORBED. gPos is right again, so nothing is outstanding.
            gDriftPending = false;
            //  And this crossing is the newest sighting: it saw the index
            //  where the (now corrected) frame says it is, so an older clue
            //  from before it would only point somewhere worse (2026-09-25).
            forgetIndexClue();
          } else {
            //  TOO BIG TO BELIEVE, SO DO NOT BELIEVE IT - REMEMBER WHERE IT
            //  SAYS THE INDEX IS AND GO AND LOOK. The number never enters the
            //  frame; it only says which way to walk.
            believeIndexAt(expect + gDrift);
            gDriftPending = true;
            Con.print(F("  [WARN] index says the needle is "));
            Con.print((long)gDrift);
            Con.println(F(" half-steps from where it should be - too far to"));
            Con.println(F("         absorb silently. Something slipped - it re-indexes by itself."));
          }
        }
      }
    }

    vTaskDelay(pdMS_TO_TICKS(5));
  }
}

// ============================================================================
void begin() {
  pinMode(S3_INDEX_HALL, INPUT);      // no internal pull-up: the pull-up is external (Bible §11)
  //  SEED THE TRACKER WITH A LIVE READ, not the cold default of "OFF". Without
  //  this, a magnet already over the sensor at power-up (assumeAtLowStop()'s
  //  normal case is fine, but a set left parked ON the index is not
  //  impossible) would read as OFF for the first IDX_DEBOUNCE_K-1 polls after
  //  needleTask starts - up to 15 ms - which is exactly the window
  //  startHoming() might run in. Seeding both idxConfirmed and idxRunLevel to
  //  the same live value means they already agree, so no edge fires from this
  //  - it is an initial condition, not a confirmed transition.
  idxConfirmed = idxRunLevel = indexRaw();
  Drive::begin();
  Drive::setInvert(true);             // MEASURED: advancing the phase goes LOW FM
  Drive::setDutyPct(100);             // full duty for the motor supply in Bible §11
  Drive::setMode(DRV_MICRO, pMicroFast);

  Wire.begin(S3_AS5600_SDA, S3_AS5600_SCL, S3_I2C_HZ);
  readStatus();
  readAs5600();

  //  Compiled defaults only - applySettings() has not run yet. See wnCeiling().
  if (pWn > wnCeiling())
    Con.printf("  [WARN] wn %.1f exceeds the gain ceiling %.1f - expect "
                  "overshoot and hunting.\n", pWn, wnCeiling());

  //  Emitter on core 0, supervisor on core 1. See the stepTask header.
  xTaskCreatePinnedToCore(stepTask,   "nstep",   4096, nullptr, 19, nullptr, 0);
  xTaskCreatePinnedToCore(needleTask, "needle",  4096, nullptr, 4,  nullptr, 1);
}

State state() { return st; }
const char *stateName() {
  switch (st) {
    case IDLE: return "idle";        case SEEK_INDEX: return "seeking index";
    case BACKOFF: return "backoff";  case REAPPROACH: return "reapproach";
    case SWEEP: return "sweep";      case TRACKING: return "tracking";
    case PARKED: return "parked";    case FAULT: return "FAULT";
  }
  return "?";
}
bool homed() { return isHomed; }

//  gAfter IS THE POINT OF THESE. Both used to assign `st` unconditionally when
//  homed, which clobbered SWEEP - and the caller does exactly that: homing sets
//  isHomed and st = SWEEP in the same breath, main.cpp fires applyNeedleMode()
//  on the homed edge, and loop() runs far faster than the supervisor's 5 ms
//  tick, so the sweep was overwritten before it was serviced once. The author
//  saw "one trip to the low end, then straight to the station" and reported it
//  as the sweep stopping short. The SWEEP branch already ends with st = gAfter,
//  so setting gAfter and leaving st alone is all that was ever needed.
//
//  Homing and calibration are guarded for the same reason: neither should be
//  interrupted by a mode change that only wanted to say where to go afterwards.
static bool needleBusy() {
  return st == SWEEP || st == SEEK_INDEX || st == BACKOFF || st == REAPPROACH || calBusy();
}
//
//  THEY SAY WHETHER THEY WERE TAKEN, 2026-09-25. The behaviour is exactly as it
//  was; only the answer is new. The portal's needle.track answered "tracking
//  the tuner" whatever happened - including in FAULT, or unhomed and not IDLE,
//  where this does nothing at all. TRUE when the request will be carried out:
//  the needle is now tracking/parked, or a home or other busy phase is running
//  and gAfter will hand over to it when that ends, or a home was started for
//  it. FALSE when it is simply dropped: unhomed with nothing going to home it
//  (FAULT, or any non-IDLE unhomed state), or the home it needed was refused.
bool track() {
  gAfter = TRACKING;
  if (needleBusy()) return true;
  if (isHomed) { st = TRACKING; return true; }
  if (st == IDLE) return startHoming() == nullptr;
  return false;
}
bool park() {
  gAfter = PARKED;
  if (needleBusy()) return true;
  if (isHomed) { st = PARKED; return true; }
  if (st == IDLE) return startHoming() == nullptr;
  return false;
}

void setUpLimits(uint16_t v, uint16_t a)    { pUpVmax = v; pUpAccel = a; }
void setDownLimits(uint16_t v, uint16_t a)  { pDnVmax = v; pDnAccel = a; }
void setSecondOrder(float wn, float z)      { pWn = wn; pZeta = z; }
void setDecayEnvelope(uint16_t r, uint16_t f) { pRiseMs = r; pFallMs = f; }
void setDwell(uint16_t u, uint16_t d)       { pDwellUp = u; pDwellDn = d; }
void setMicro(uint8_t fast, uint8_t slow)   { pMicroFast = fast; pMicroSlow = slow; }
void setReapproachSpeed(uint16_t h)         { pReapHsps = h; }

//  WHERE THE NEEDLE SHOULD POINT, 2026-09-02.
//
//  This used to be  pPosMin + permille * travel / 1000  - the needle followed
//  the TUNING CAPACITOR'S MECHANICAL TRAVEL directly, and never looked at a
//  frequency at all. That is wrong whenever the two are not the same range,
//  and on this set they are not: the tuner covers more than the printed face
//  (Firmware Gospel §6). Every station therefore landed compressed and offset,
//  and the readout and the needle disagreed because they were computed from
//  different things.
//
//  Three mappings, kept separate on purpose:
//    angle -> permille   calLow/calHigh, the capacitor's travel (readouts
//                        only; the needle does not use it)
//    shaft angle -> MHz   the fitted tuning curve (main.cpp fitTuneCurve)
//    MHz -> needle       dialLow/dialHigh, what is PRINTED at each stop
//
//  Past either end of the printed face the needle parks at the stop rather
//  than running off the scale - the set can tune there, the dial cannot show
//  it, and pinning the pointer is the honest thing to draw.
//  THE UNROUNDED FREQUENCY, and it matters more than it looks.
//
//  This used tuneFreq10(), which is lrintf() of the curve - so the needle's
//  target could only land on whole tenths of a MHz. On this dial that is 1570
//  half-steps over 200 tenths, 7.85 half-steps per tenth. Leave the tuning near
//  a x.x5 boundary and the encoder's own noise flips the rounding back and
//  forth, hopping the needle eight half-steps each way indefinitely: the author
//  saw it wobbling and the ULN2003's LEDs flashing while the firmware reported
//  a stationary position, because both values it alternated between rounded to
//  the same reported half-step.
//
//  A pointer on a glass dial is an analogue indicator. It has no business
//  quantising to a tenth of a megahertz, so it follows the unrounded curve -
//  the same one the display snap is built on (tuneFreq10f()).
static float tuneFreq10Raw(int32_t acc);
static int32_t trackTarget() {
  //  The filtered shaft - see readAs5600(). Before the first sample, the raw one.
  float f       = gTrackAccInit ? tuneFreq10Raw((int32_t)lrintf(gTrackAcc)) : tuneFreq10f();
  int32_t dspan = (int32_t)pDialHigh - (int32_t)pDialLow;
  if (dspan <= 0) return pPosMin;
  if (f <= (float)pDialLow)  return pPosMin;
  if (f >= (float)pDialHigh) return pPosMax;
  return pPosMin + (int32_t)lrintf((f - (float)pDialLow)
                                   * (float)(pPosMax - pPosMin) / (float)dspan);
}

static float tuneFreq10Raw(int32_t acc) {
  //  CLAMP TO THE FITTED DOMAIN FIRST. Past the outermost calibration point a
  //  quadratic leaves the physical world quickly, and the needle would follow
  //  it into a stop.
  if (acc < pCurveXlo) acc = pCurveXlo;
  if (acc > pCurveXhi) acc = pCurveXhi;
  float x = (float)((double)acc - pCurveX0);
  float f = pCurveA + pCurveB * x + pCurveC * x * x;
  //  The fit is validated where it is accepted (main.cpp), so this only has to
  //  stop a NaN from becoming a needle command.
  if (!(f > 100.0f && f < 3000.0f)) return (float)(pCurveOff10 + 881);
  return f + (float)pCurveOff10;
}

int32_t tuneFreq10At(int32_t acc) { return (int32_t)lrintf(tuneFreq10Raw(acc)); }
void    curveDomain(int32_t &lo, int32_t &hi) { lo = pCurveXlo; hi = pCurveXhi; }
int32_t tuneFreq10()              { return tuneFreq10At(accAngle); }
float   tuneFreq10f()             { return tuneFreq10Raw(accAngle); }

void setCurve(double x0, float a, float b, float c, int32_t offset10,
              int32_t xlo, int32_t xhi) {
  if (xlo > xhi) { int32_t t = xlo; xlo = xhi; xhi = t; }
  pCurveX0 = x0; pCurveA = a; pCurveB = b; pCurveC = c; pCurveOff10 = offset10;
  pCurveXlo = xlo; pCurveXhi = xhi;
}

void setDial(uint16_t dialLo, uint16_t dialHi) {
  pDialLow = dialLo; pDialHigh = dialHi;
}

void setGeometry(int32_t lo, int32_t hi) {
  //  CROSSED LIMITS FREEZE THE NEEDLE IN BOTH DIRECTIONS, AND PERSIST.
  //  stepAllowed() refuses dir>0 at or above pPosMax and dir<0 at or below
  //  pPosMin; with the two crossed, both tests fire across the overlap and no
  //  direction is legal. The portal's limitLow/limitHigh buttons and the m/M
  //  console keys write these raw, so nothing upstream enforces an
  //  order, and settingsTouch() then writes the result to flash - it survives
  //  the reboot. Refusing here is the one check that covers every writer.
  if (lo >= hi) {
    Con.printf("  [WARN] soft limits refused: %ld >= %ld, keeping %ld..%ld\n",
                  (long)lo, (long)hi, (long)pPosMin, (long)pPosMax);
    return;
  }
  //  THE LIMITS MUST BRACKET ZERO. Homing DEFINES position 0 at the index, and
  //  the index is a mid-travel mark - measured 1195 half-steps above the low
  //  stop and 674 below the high one. So posMin <= 0 <= posMax
  //  is an invariant of any real machine, and a pair that fails it is proof
  //  the frame was shifted when they were captured.
  //
  //  This is not theoretical: the author's stored pair was -1837..-117, BOTH
  //  negative, which puts the index outside the needle's own range. The
  //  consequences were every symptom he reported - the needle could never
  //  cross the index again, so passive re-referencing was structurally dead;
  //  and every trip to posMin drove it 640 half-steps into a hard stop.
  //  Refusing here is what stops a corrupt frame outliving the calibration.
  if (lo > 0 || hi < 0) {
    Con.printf("  [WARN] soft limits %ld..%ld do not bracket the index at 0.\n",
                  (long)lo, (long)hi);
    Con.println(F("         That pair cannot be real - the index is a mid-travel"));
    Con.println(F("         mark. REFUSED. Re-home and set the stops again."));
    return;
  }
  pPosMin = lo; pPosMax = hi;
}
void getGeometry(int32_t &lo, int32_t &hi)  { lo = pPosMin; hi = pPosMax; }
void setIndexCal(int32_t a, int32_t b, int32_t c, int32_t d) {
  pOnFwd = a; pOffFwd = b; pOnRev = c; pOffRev = d;
}
void getIndexCal(int32_t &a, int32_t &b, int32_t &c, int32_t &d) {
  a = pOnFwd; b = pOffFwd; c = pOnRev; d = pOffRev;
}

int32_t position() { return (int32_t)(gPos / FINE_PER_HALFSTEP); }
int32_t target()   { return (int32_t)(gTarget / FINE_PER_HALFSTEP); }
bool    indexNow() { return indexRaw(); }
int32_t lastDrift(){ return gDrift; }
bool    driftPending(){ return gDriftPending; }
bool    hunting()     { return gHunting; }
uint8_t recoverTries(){ return gAutoTries; }
bool    recovering()  { return gAutoHome; }
//  Has the index actually been measured since the last home?
bool driftMeasured() { return gMeas > gMeasAtHome; }
//  Has the band calibration ever run? All four are offsets from pOnFwd == 0
//  and the band is ~100 half-steps wide, so they cannot all be zero afterwards.
bool bandCalibrated() { return pOffFwd || pOnRev || pOffRev; }
uint32_t indexCrossings() { return gCross; }
void     setEdgeLog(bool on) { gEdgeLog = on; }
bool     edgeLog()           { return gEdgeLog; }

uint16_t angleRaw() { return rawAngle; }
bool     magnetOk() { return magOk; }
uint8_t  statusByte() { return statusRaw; }
uint16_t fieldMagnitude() { return magnitude; }
int32_t  accumulator() { return accAngle; }
uint16_t rawAngleNow() { return rawAngle; }
uint8_t  agc()      { return agcVal; }
bool     i2cOk()    { return busOk; }
uint32_t encoderGaps() { return gGapCount; }
void setCalibration(int32_t lo, int32_t hi, bool bothMeasured) {
  calLow = lo; calHigh = hi; gEndsMeasured = bothMeasured;
}
void getCalibration(int32_t &lo, int32_t &hi) { lo = calLow; hi = calHigh; }
int32_t accumulated() { return accAngle; }
void seedAccumulator(int32_t last) {
  //  No fresh reading yet (the first read at boot failed): remember the saved
  //  angle, so the first real reading seats on the turn nearest IT - not the
  //  turn nearest a placeholder.
  gSeedAnchor  = last;
  gSeedPending = !angleFresh;
  accAngle = seatTurn(rawAngle, last);
  accInit  = true;
  gTrackAccInit = false;           // the filter starts again from this value
}
bool     tuningActive() { return lastTurnMs && (millis() - lastTurnMs) < 400; }
uint16_t tunePermille() { return anglePermille(); }

//  Run the measured travel end to end, repeatedly, so the calibration can be
//  judged by eye and the index watched for drift over many crossings. That
//  drift figure is the thing that says whether one index is enough.
//  TRUE if it started, 2026-09-25 - same behaviour, but the portal's
//  needle.sweep answered "sweeping" to a refusal only the console could see.
bool sweepRange(bool repeat) {
  if (!isHomed) { Con.println(F("  not homed - press H first.")); return false; }
  gSweepRepeat = repeat;
  gSweepPhase  = 0;
  st = SWEEP;
  return true;
}

//  STOP NOW. Not a request to decelerate - the profile is abandoned where it
//  stands, the coils are released, and the supervisor is put back to IDLE so it
//  cannot re-assert a target a moment later.
//
//  IT STOPS A CALIBRATION TOO, 2026-09-24. This used to leave calWhat set and
//  gManualVel running: stepTask gives a non-zero gManualVel precedence over
//  everything, so the creep resumed on the next control tick, and calTick()
//  went on driving because the calibration still owned the needle. "Stop"
//  that does not stop is worse than no button. The band calibration is ended
//  through its own abort path, and the jog velocity is zeroed AFTER that. (This
//  note used to add "so a calTick() mid-creep on the other core cannot leave
//  one behind". It could: the creep rewrote the velocity after both. What
//  closes it now is that this body runs on the same task as calTick() - see
//  gReq.)
//
//  AND IT ENDS A RECOVERY. gAutoHome says "the homing now running is a
//  re-index, go back to gAfter and do not sweep". Left set after a stop, the
//  NEXT hand-driven home would have been mistaken for a recovery - no sweep,
//  and a failure counted against the three automatic tries.
//
//  THE BODY RUNS ON THE NEEDLE TASK, 2026-09-24 - see gReq. stopNow() is the
//  stop exactly as it was; stop() is what callers on any core may call.
static void calAbortNow();

//  ENDS A HELD COIL TEST, from coilTest(0) or from a stop (2026-09-25). The
//  coils go slack, and the drive goes back to the mode begin() set: coilTest()
//  switched it to WAVE, whose step is TWO half-steps, and nothing put it back -
//  useMicro() only runs on the way into a sweep, tracking or a park, so a home
//  started straight after a coil test ran its search and its zero-defining
//  creep at half the resolution. gPos is snapped onto that lattice BEFORE the
//  drive is handed back (gTestMode cleared last), while stepTask is still
//  parked in its gTestMode branch and cannot be stepping it.
static void coilRelease() {
  Drive::release();
  Drive::setMode(DRV_MICRO, pMicroFast);
  gPos      = (int32_t)Drive::snap(gPos);
  gTarget   = gPos;
  gCoilHeld = false;
  gTestMode = false;
}

static void stopNow() {
  if (calBusy()) calAbortNow();
  gManualVel   = 0.0f;
  gAutoHome    = false;
  gSweepRepeat = false;
  gDone   = true;
  gVel    = 0.0f;
  gTarget = gPos;
  Drive::release();
  //  A STOP RELEASES A HELD COIL, 2026-09-25. coilTest(1..4) leaves gTestMode
  //  up for as long as the coil is held, and this never cleared it: after 1-4
  //  then x (or the portal's Stop) the coil was released but the needle was
  //  dead - stepTask and needleTask both skip everything while gTestMode is up
  //  - and T, P and H printed "tracking." / "homing." to a needle that would
  //  never move. Only a HELD COIL is released here, never gTestMode as such:
  //  jogRaw() raises it too, and clears it itself when its loop ends on the
  //  caller's task; dropping it under a running jog would let stepTask drive
  //  the motor at the same time as the jog.
  if (gCoilHeld) coilRelease();
  st = IDLE;
  Con.print(F("  stopped at position "));
  Con.println((long)(gPos / FINE_PER_HALFSTEP));
}

//  CALLED ONLY FROM needleTask. The bits handled are cleared after the work,
//  not before: while any bit is up stepTask emits nothing, so clearing first
//  would open a gap in which a creep velocity set before the request could
//  still move the motor. A request posted again while this runs is the same
//  request, and the work it asks for has just been done.
static void serviceRequests() {
  uint8_t r = reqPending();
  if (!r) return;
  if (r & REQ_STOP)          stopNow();       //  aborts a calibration too
  else if (r & REQ_CALABORT) calAbortNow();
  __atomic_fetch_and(&gReq, (uint8_t)~r, __ATOMIC_SEQ_CST);
}

//  SAFE FROM ANY TASK ON EITHER CORE. The motor stops at once - stepTask sees
//  the flag on its next control tick, and the three writes below are single
//  stores it already accepts from other tasks - and the rest (the calibration
//  abort, st = IDLE, releasing the coils, the console line) follows within one
//  needleTask tick, 5 ms. So st still reads its old value for up to that long:
//  a caller that reads state() straight after stop() may see it. None does.
void stop() {
  __atomic_fetch_or(&gReq, (uint8_t)REQ_STOP, __ATOMIC_SEQ_CST);
  gManualVel = 0.0f;
  gDone      = true;
  gVel       = 0.0f;
}

// ---------------------------------------------------------------------------
//  BRING-UP TOOLS. gTestMode stops the step emitter dead so it cannot fight
//  these or release the coils underneath them.
// ---------------------------------------------------------------------------
//
//  Both take the needle out of whatever homing was running, so both clear
//  gAutoHome (2026-09-24) - see the same note on stop().
void coilTest(int coil) {
  gTestMode = true;
  gAutoHome = false;
  gDone = true; gVel = 0;
  if (coil < 1 || coil > 4) {
    //  A held coil goes back through coilRelease(), which also restores the
    //  drive mode (2026-09-25); with none held this is what it always was.
    if (gCoilHeld) coilRelease();
    else           Drive::release();
    Con.println(F("  coils released."));
    st = IDLE;
    gTestMode = false;
    return;
  }
  gCoilHeld = true;              //  so a stop can release it - see gCoilHeld
  //  In WAVE mode one phase is energised at a time, and the four wave positions
  //  are 0, 512, 1024, 1536 fine. invert is bypassed by construction here since
  //  we only care which PIN goes active, not which way the shaft turns.
  Drive::setMode(DRV_WAVE, 1);
  Drive::apply((int64_t)(coil - 1) * 512);
  Con.print(F("  IN"));
  Con.print(coil);
  Con.println(F(" held at full duty. Meter that ULN output to GND: it should"));
  Con.println(F("  pull LOW (near 0 V). The rotor should also detent."));
  Con.println(F("  Press 0 to release. Do not leave a coil held for minutes."));
}

void jogRaw(int32_t halfSteps, uint16_t hsps, bool microstep) {
  gTestMode = true;
  gAutoHome = false;
  gDone = true; gVel = 0;
  if (hsps < 5)   hsps = 5;
  if (hsps > 900) hsps = 900;

  Drive::setMode(microstep ? DRV_MICRO : DRV_HALF, microstep ? pMicroFast : 1);
  gPos = (int32_t)Drive::snap(gPos);

  int32_t unit  = Drive::stepUnit();
  int32_t sub   = microstep ? (FINE_PER_HALFSTEP / unit) : 1;
  int32_t total = (halfSteps < 0 ? -halfSteps : halfSteps) * sub;
  int     dir   = (halfSteps >= 0) ? +1 : -1;
  uint32_t ivUs = 1000000UL / ((uint32_t)hsps * sub);

  Con.print(F("  jogging "));
  Con.print(halfSteps);
  Con.print(F(" half-steps at "));
  Con.print(hsps);
  Con.print(F(" hsps in "));
  Con.println(microstep ? F("MICRO mode") : F("HALF mode"));

  bool warned = false;
  for (int32_t i = 0; i < total; i++) {
    //  DELIBERATELY NOT BOUNDED BY THE SOFT LIMITS.
    //
    //  I gated this on stepAllowed() to stop "100" grinding into the physical
    //  stop, and that broke the tool: the jog is how you PLACE the needle in
    //  order to SET the limits, so bounding it by the limits makes them
    //  impossible to widen. Sitting at posMax, every forward press was refused
    //  and the machine looked dead. Chicken and egg, and the tool has to win.
    //
    //  The bound that remains is the step COUNT, clamped where the request
    //  enters (settings_table.h for the portal; the console keys jog fixed
    //  counts), which caps a runaway without making the
    //  needle unplaceable. Crossing a limit is announced rather than refused.
    if (gLimitsArmed && !stepAllowed(dir) && !warned) {
      warned = true;
      Con.println(F("  jog is now OUTSIDE the soft limits - watch the stop."));
    }
    gPos += dir * unit;
    Drive::apply(gPos);
    delayMicroseconds(ivUs);
  }
  Drive::release();
  Con.print(F("  done. position now "));
  Con.println((long)(gPos / FINE_PER_HALFSTEP));

  //  LEAVE IT WHERE THE OPERATOR PUT IT.
  //  The supervisor re-asserts its target continuously, so without this any
  //  manual nudge was immediately undone - dragged back to the park position
  //  with the decay profile, or to a dead AS5600's zero on RADIO. A hand
  //  command that the machine overrides a moment later is worse than no
  //  command. Automatic behaviour resumes on H, T, P or a source change.
  st = IDLE;
  gTestMode = false;
}

uint32_t jitterMaxUs() { return gJitter; }
void     jitterClear() { gJitter = 0; }
uint32_t stepsEmitted(){ return gSteps; }
float    velHsps()     { return fineToHsps(gVel); }

// ============================================================================
//  CALIBRATION  -  the author's 4-measurement band calibration, 2026-08-31
//
//  THE PROPERTY THAT MAKES IT SAFE: the operator jogs the needle ONTO the
//  sensor, and the machine only creeps across the index band from there, at
//  pReapHsps, inside the armed soft limits and a 600 half-step budget per
//  creep. Nothing is ever driven into a stop, so nothing can lose steps while
//  being calibrated. Earlier attempts drove outward against a guessed limit
//  and lost about 300 half-steps doing it.
//
//  IT IS A STATE MACHINE, NOT A BLOCKING ROUTINE, and that is not a stylistic
//  choice. These become buttons in the web portal, and a calibration that
//  blocks would freeze the HTTP handler for its entire duration and time the
//  request out. Everything below advances one step per supervisor tick and
//  reports progress while it runs.
//
//  ONE stage now - see the note under this banner for what the second one was
//  and why it is gone.
//
//  BAND - the sensor reads ON across roughly 100 half-steps, and a Hall
//     switch releases at a weaker field than it operates. So the trigger point
//     depends on the direction of travel: searching UP from the low end trips
//     at the "ON going +" edge, searching DOWN from the high end trips at
//     "ON going -" - a DIFFERENT PHYSICAL POINT, ~79 half-steps higher on the
//     last measurement. Subtract the two end measurements without knowing that
//     offset and the span is wrong by it.
//
//     One pass, from ON: + until OFF (offFwd), - until ON (onRev), - until OFF
//     (offRev), + until ON (onFwd).
//
//     So all four edges are measured, several times, and the SPREAD is reported
//     as well as the means. The offset is correctable arithmetic; the scatter is
//     what decides whether one index is enough.
//
//  The offset MATTERS EVEN WITH ONE STAGE: until bandCalibrated(), a REVERSE
//  crossing cannot be scored (it would report the whole ~79 half-step
//  direction offset as drift), so only forward crossings are checked, the band
//  check is off, and main.cpp's captureLimit() refuses.
// ============================================================================

//  ONE calibration, not three. The END-STOP calibration (stage 2 of the
//  author's procedure: CAL_LOW / CAL_HIGH, keys g / G and two portal buttons)
//  was REMOVED on 2026-09-03 because it checked nothing before it drove - not
//  homed, not which side of the index, soft limits disarmed. Started from the
//  wrong end it crept a 4000 half-step budget into the physical stop, then
//  left the frame 4000 half-steps out. m/M sets the limits against a position
//  the operator can see.
enum : uint8_t { CAL_NONE = 0, CAL_BAND };

static uint8_t  calWhat    = CAL_NONE;
static uint8_t  calStep    = 0;
static bool     calEntered = false;      // has this step set its origin yet
static uint8_t  calPass    = 0;
static uint8_t  calPasses  = 5;
static int32_t  calOrigin  = 0;          // half-steps, where this creep began
static char     calMsg[72]     = "";
static bool     calLimitHit = false;     // the last creep failed at a soft limit

static int32_t  calOnF[12], calOffF[12], calOnR[12], calOffR[12];

static inline int32_t halfNow() { return (int32_t)(gPos / FINE_PER_HALFSTEP); }

static void calSay(const char *m) {
  strncpy(calMsg, m, sizeof(calMsg) - 1);
  calMsg[sizeof(calMsg) - 1] = 0;
  Con.print(F("  cal: "));
  Con.println(calMsg);
}

static //  RELEASE THE NEEDLE. track()/park() now set gAfter and return while
//  needleBusy(), which is right - but nothing consumed gAfter when a
//  calibration ended, so the needle sat in IDLE until the next source change.
//  The other places that hand over to gAfter: the end of the SWEEP, a
//  successful automatic re-index (REAPPROACH), and a failed one (homeFault()).
void calStop() {
  manualStop();
  calWhat = CAL_NONE;
  calEntered = false;
  gLimitsArmed = isHomed;
  Drive::release();
  //  Hand the needle back to whatever mode was asked for while we held it.
  if (isHomed && st == IDLE) st = gAfter;
}

//  One creep step. Returns true when the wanted state is reached, and sets
//  `failed` if the budget ran out first. On success `atHalf` is the edge's
//  BACK-DATED position - idxEdge.pos, the same tracker homing reads, not
//  halfNow() at the poll that noticed - so the stored idx* constants stay in
//  the same frame REAPPROACH defines zero in. calCreep runs at pReapHsps (60
//  hsps by default), so the bias this removes is under a half-step here; the
//  point is consistency with homing, not chasing a large error. Non-blocking: it
//  issues velocity and returns immediately, so the supervisor keeps ticking.
static bool calCreep(int dir, bool wantIndex, int32_t budget, bool &failed, int32_t &atHalf) {
  failed = false;
  if (!calEntered) { calOrigin = halfNow(); calEntered = true; }
  if (idxState() == wantIndex) {
    manualStop(); calEntered = false;
    atHalf = (int32_t)(idxEdge.pos / FINE_PER_HALFSTEP);
    return true;
  }
  int32_t moved = halfNow() - calOrigin;
  if (moved < 0) moved = -moved;
  if (moved > budget) {
    manualStop();
    calEntered = false;
    failed = true;
    return true;
  }
  //  THE SOFT LIMIT ENDS THE CALIBRATION, 2026-09-24. The limits now stay armed
  //  while it runs (see calStartBand()), and stepTask obeys them - but it obeys
  //  them by refusing the step and nothing more, while calCreep() asked for the
  //  same velocity again every 5 ms. The creep would sit at the limit asking,
  //  moving nothing, until its budget - measured in half-steps it could no
  //  longer travel - never ran out. A band that needs the needle past a soft
  //  limit to find its edge is not a band this dial can measure; say so and
  //  hand the needle back rather than push.
  if (!stepAllowed(dir)) {
    manualStop();
    calEntered = false;
    failed = true;
    calLimitHit = true;
    return true;
  }
  manualRun(dir, pReapHsps);
  return false;
}

//  One way out for every failed creep. The budget message says which edge was
//  never seen; a soft limit reached first overrides it, because then the edge
//  was never looked for.
static void calFail(const char *budgetWhy) {
  calSay(calLimitHit ? "stopped at a soft limit - the band runs past it; aborted"
                     : budgetWhy);
  calStop();
}

static void calFinishBand() {
  auto stat = [&](int32_t *v, const char *name) {
    int32_t lo = v[0], hi = v[0];
    int64_t sum = 0;
    for (uint8_t i = 0; i < calPass; i++) {
      if (v[i] < lo) lo = v[i];
      if (v[i] > hi) hi = v[i];
      sum += v[i];
    }
    int32_t m = (int32_t)(sum / calPass);
    Con.printf("  %-7s mean %6ld   spread %ld half-steps\n",
                  name, (long)m, (long)(hi - lo));
    return m;
  };
  Con.println(F("  ----"));
  int32_t mOnF  = stat(calOnF,  "onFwd");
  int32_t mOffF = stat(calOffF, "offFwd");
  int32_t mOnR  = stat(calOnR,  "onRev");
  int32_t mOffR = stat(calOffR, "offRev");

  //  Shift the whole frame so the forward ON edge is zero - that is the edge
  //  homing lands on, so zero means the same thing everywhere.
  int32_t shift = mOnF;
  pOnFwd  = 0;
  pOffFwd = mOffF - shift;
  pOnRev  = mOnR  - shift;
  pOffRev = mOffR - shift;
  gPos   -= shift * FINE_PER_HALFSTEP;
  gTarget = gPos;
  isHomed = true;
  //  ANNOUNCE THE RESULT SO IT CAN BE SAVED.
  //
  //  Without this the band measurement lived only in the runtime pOn*/pOff*
  //  and was erased by the very next applySettings() - which calls
  //  setIndexCal(cfg.idx*, ...) and cfg.idx* is never written by anything.
  //  Meanwhile gPos KEPT the shift above, so the frame moved and the reference
  //  that justified the move was thrown away: every limit captured afterwards
  //  was out by `shift`. That is the ~640 half-steps in the author's machine.
  //  A FRAME SHIFT INVALIDATES PRIOR CROSSINGS, exactly as a re-home does.
  //  REAPPROACH and this function both establish a new frame and set isHomed
  //  (homeFault()'s failed-recovery branch also sets it, but hands back the
  //  SAME frame), and only REAPPROACH used to snapshot gMeasAtHome, so
  //  captureLimit() would have accepted evidence measured in the frame this
  //  call just abolished, against index constants that were still zero when
  //  it was taken.
  //  THIS WRITES gPos DIRECTLY (above), not through gCorrectHs - the
  //  exception to the rule noted there; the motor is stopped at this point.
  gMeasAtHome = gMeas;
  //  AND IT INVALIDATES THE SLIP EVIDENCE, 2026-09-24, for the same reason
  //  startHoming() clears it. gDrift, gDriftPending and gIndexBelieved are all
  //  statements in the frame this call has just replaced. Left set, the
  //  recovery tick re-indexed a needle that had been re-zeroed a moment ago,
  //  steering by a gIndexBelieved that was `shift` half-steps out. The index
  //  is at 0 in the new frame by construction, so that is what is believed.
  //  (2026-09-25: startHoming() no longer clears gIndexBelieved - starting a
  //  home does not make the frame true. This and REAPPROACH's success are now
  //  the only places that retire the clue for a NEW frame; an absorbed
  //  crossing retires it for a frame it has just shown to be right.)
  gDrift         = 0;
  gDriftPending  = false;
  forgetIndexClue();              //  gIndexBelieved = 0 and its flag, together
  gAutoTries     = 0;             // a fresh frame, as after any home
  bandOnF = pOnFwd; bandOffF = pOffFwd; bandOnR = pOnRev; bandOffR = pOffRev;
  bandShift = shift;
  bandResultPending = true;

  Con.printf("  band %ld half-steps wide; direction offset (onRev) = %ld\n",
                (long)(pOffFwd - pOffRev), (long)pOnRev);
  Con.println(F("  Zero is the forward ON edge. A spread of a few half-steps is"));
  Con.println(F("  fine; tens means one index is not enough and we rethink."));
  calSay("band done");
  calStop();
}

//  Advanced once per supervisor tick while a calibration is running.
static void calTick() {
  bool    failed = false;
  int32_t atHalf = 0;
  const int32_t BAND_BUDGET = 600;

  switch (calWhat) {

    case CAL_BAND:
      switch (calStep) {
        case 0:
          if (calCreep(+1, false, BAND_BUDGET, failed, atHalf)) {
            if (failed) { calFail("never released going +"); return; }
            calOffF[calPass] = atHalf; calStep = 1;
          }
          break;
        case 1:
          if (calCreep(-1, true, BAND_BUDGET, failed, atHalf)) {
            if (failed) { calFail("never triggered going -"); return; }
            calOnR[calPass] = atHalf; calStep = 2;
          }
          break;
        case 2:
          if (calCreep(-1, false, BAND_BUDGET, failed, atHalf)) {
            if (failed) { calFail("never released going -"); return; }
            calOffR[calPass] = atHalf; calStep = 3;
          }
          break;
        case 3:
          if (calCreep(+1, true, BAND_BUDGET, failed, atHalf)) {
            if (failed) { calFail("never triggered going +"); return; }
            calOnF[calPass] = atHalf;
            Con.printf("  pass %u:  onFwd %5ld  offFwd %5ld  onRev %5ld  offRev %5ld\n",
                          calPass + 1, (long)calOnF[calPass], (long)calOffF[calPass],
                          (long)calOnR[calPass], (long)calOffR[calPass]);
            calPass++;
            calStep = 0;
            if (calPass >= calPasses) calFinishBand();
          }
          break;
      }
      break;

    default: break;
  }
}

// --- public -----------------------------------------------------------------
//
//  REFUSES ANYTHING BUT A SETTLED, TRUSTED FRAME, 2026-09-24. It used to check
//  only "is a calibration running" and "is the sensor ON" - and the sensor is
//  ON part-way through every homing BACKOFF and every sweep that crosses the
//  index. Started there, it set st = IDLE out from under the homing machine
//  (whose re-armed limits then never came back) or the sweep, and measured
//  four edges in a frame that was either not yet defined or already known to
//  be wrong - then calFinishBand() declared that frame homed and handed it to
//  main.cpp to save. So: homed, no slip outstanding, and nothing else holding
//  the needle.
//
//  Returns nullptr when it started, otherwise WHY NOT. Every refusal but the
//  first also goes into calMessage(), which the portal already shows; the
//  first does not, because calMessage() is then the RUNNING calibration's
//  progress line and overwriting it would hide what is actually happening.
const char *calStartBand(uint8_t passes) {
  static const char *const RUNNING = "a calibration is already running";
  if (calWhat != CAL_NONE) { Con.print(F("  cal: ")); Con.println(RUNNING); return RUNNING; }
  const char *why = nullptr;
  if      (!isHomed)
    why = "not homed - home the needle (H) first, then jog onto the sensor";
  else if (gDriftPending)
    why = "a slip is outstanding - let the re-index finish (or press H) first";
  else if (st == SEEK_INDEX || st == BACKOFF || st == REAPPROACH)
    why = "homing is running - wait for it to finish";
  else if (st == SWEEP)
    why = "the sweep is running - wait for it, or stop it, first";
  else if (!idxState())
    why = "needle is NOT on the index - jog it onto the sensor first";
  if (why) { calSay(why); return calMsg; }

  if (passes == 0 || passes > 12) passes = 5;
  //  Stop whatever tracking/parking move is under way before the frame is
  //  touched, and end any recovery bookkeeping (the same gAutoHome note as
  //  stop()): the calibration owns the needle from here.
  gDone = true; gVel = 0.0f; gTarget = gPos;
  gAutoHome = false;
  calPasses = passes;
  calPass   = 0;
  calStep   = 0;
  calEntered = false;
  calLimitHit = false;
  //  THE SOFT LIMITS STAY ARMED, 2026-09-24. This line used to disarm them, from
  //  when a calibration could start with no frame at all and the limits meant
  //  nothing. It cannot any more: the refusals above require a homed frame with
  //  no slip outstanding, so the limits are exactly as true now as they will be
  //  when it ends - and disarmed, the only bound left on a creep was its
  //  600-half-step budget, on a machine with no end switch behind it. A creep
  //  that reaches a limit now fails the calibration (calCreep()). Said
  //  explicitly rather than left to whatever homing set, since the frame is
  //  known to be good here. calStop() re-derives it from isHomed on the way
  //  out, which is still right: true on success, and on any failure or abort
  //  the frame is the same homed one this started in.
  gLimitsArmed = true;
  Drive::setMode(DRV_MICRO, pMicroFast);
  gPos = (int32_t)Drive::snap(gPos);
  gTarget = gPos;
  st = IDLE;
  calWhat = CAL_BAND;
  calSay("measuring the index band");
  return nullptr;
}

static void calAbortNow() {
  if (calWhat == CAL_NONE) return;
  calSay("aborted");
  calStop();
}

//  A REQUEST, like stop(), 2026-09-24 - and for the same race: the portal's
//  calAbort button runs on core 0, and ending the calibration there let
//  calCreep() on core 1 write its velocity back after calStop() had zeroed it.
//  The motor stops at once (stepTask honours any pending request); calBusy()
//  stays true for up to one needleTask tick, until the abort is carried out.
void calAbort() {
  if (calWhat == CAL_NONE) return;
  __atomic_fetch_or(&gReq, (uint8_t)REQ_CALABORT, __ATOMIC_SEQ_CST);
  gManualVel = 0.0f;
  gDone      = true;
  gVel       = 0.0f;
}

bool        calBusy()    { return calWhat != CAL_NONE; }
const char *calMessage() { return calMsg; }

uint8_t calProgressPct() {
  if (calWhat == CAL_NONE) return 100;
  if (calWhat == CAL_BAND) {
    uint16_t done = (uint16_t)calPass * 4 + calStep;
    uint16_t all  = (uint16_t)calPasses * 4;
    return all ? (uint8_t)((done * 100) / all) : 0;
  }
  //  calWhat can only be CAL_NONE or CAL_BAND now; the old `calStep * 33` arm
  //  belonged to the end-stop stage and was unreachable the moment it went.
  return 100;
}

//  COLLECTED EXACTLY ONCE. Raising a flag and having main.cpp poll it is the
//  contract: the producer never writes cfg, and the consumer can never read a
//  half-written result or the same one twice.
bool calTakeBand(int32_t &onF, int32_t &offF, int32_t &onR, int32_t &offR,
                 int32_t &shiftOut) {
  if (!bandResultPending) return false;
  bandResultPending = false;
  onF = bandOnF; offF = bandOffF; onR = bandOnR; offR = bandOffR;
  shiftOut = bandShift;
  return true;
}

}  // namespace Needle
