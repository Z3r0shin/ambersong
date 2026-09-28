// ============================================================================
//  AMBERSONG  -  DIAL NEEDLE  (S3)
//
//  REWRITTEN 2026-08-30 around two decisions the author made on the bench:
//
//  1. THE MOTION COMES FROM AN EARLIER TEST RIG (now retired). Its numbers took
//     a session and four firmware bugs to find; the design is in Firmware
//     Gospel §5. DO NOT RE-TUNE BY EYE.
//
//     second-order  every move but the park: wn 9, zeta 0.50, ~5 deg
//                   overshoot, ~300 ms ring; 1100 hsps / 18000 hsps^2
//                   (1100, not the rig's 1700, since 2026-09-24 - see
//                   main.cpp's upVmax); 100 ms hesitation, none while tracking
//     decay         the park: rise 100 ms, fall 1000 ms, 1 s hesitation
//                   first; 1700 hsps / 18000 hsps^2, INDEPENDENT limits
//     micro         1/32 while tracking, 1/16 for everything else
//
//     A trapezoid - what this file used to run, at 850 hsps - is the one
//     profile that reads as MACHINE. A real meter movement is a damped
//     second-order system: it overshoots slightly and settles, and that is the
//     cue the eye reads as "physical object arriving" rather than "counter
//     reaching a number".
//
//  2. NO LIMIT SWITCHES AT ALL. One A3144 as a mid-travel INDEX MARK, not an
//     end-stop. The author measured that this motor does not lose steps at the
//     settled configuration, so absolute position is maintained by counting and
//     re-referenced whenever the index is crossed.
//
//     THAT MEANS NOTHING PHYSICAL STOPS THE NEEDLE ANY MORE. Three things
//     replace the switches, and all three are load-bearing:
//       - a STEP BUDGET on the index search, so a missing or dead sensor stops
//         with a fault instead of grinding into the mechanism
//       - SOFT LIMITS enforced inside the one place a step is emitted
//       - a known starting belief for the search, so a boot knows which way
//         to go rather than guessing: the low stop after a power-up (the set
//         parks there), the position remembered in RTC memory after a
//         software restart (2026-09-25 - this line used to say "saved to NVS",
//         which nothing ever did; see bootPosition())
//
//  INDEX HYSTERESIS. A Hall switch releases at a weaker field than it operates,
//  so the ON region sits asymmetrically depending on travel direction:
//
//      moving + :  ON at -a  ......  OFF at +b
//      moving - :  OFF at -b  ......  ON at +a         (b > a)
//
//  The midpoint therefore shifts by +/-(b-a)/2 with direction. We always home
//  on ONE edge approached from ONE direction, which cancels it completely; the
//  other three measurements are kept so a crossing in the opposite direction
//  can still be used to re-reference.
//
//  DIRECTION IS INVERTED ON THIS MACHINE. Advancing the phase moves the needle
//  toward LOW FM. That inversion lives in Drive::setInvert() and nowhere else,
//  so everything here can say "position increases toward high FM" and mean it.
// ============================================================================

#pragma once
#include <stdint.h>

namespace Needle {

enum State : uint8_t {
  IDLE = 0,
  SEEK_INDEX,   // hunting the Hall mark, under a step budget
  BACKOFF,      // clearing the mark so the final approach is one-directional
  REAPPROACH,   // short forward hops onto the ON edge - the measurement that defines zero
  SWEEP,        // the full-travel flourish after a home, or sweepRange()
  TRACKING,     // following the AS5600
  PARKED,       // resting at the low end
  FAULT,        // a homing phase spent its budget (for an automatic re-index,
                // the third in a row) - NOTHING is driven
};

void  begin();
State state();
const char *stateName();

//  Tell the needle where it must already be, before homing chooses a
//  direction. It parks at the low stop whenever the set is off, so at power-up
//  that is where it is - and searching UP from there finds the index on the
//  first pass instead of grinding into the stop for a full span first.
//  COLD STARTS ONLY since 2026-09-25 - call bootPosition(), which uses this as
//  its fallback.
void assumeAtLowStop();

//  THE BOOT-TIME BELIEF, 2026-09-25. Call once in setup(), after begin() and
//  the geometry are applied, and before startHoming() - in place of
//  assumeAtLowStop(). After a SOFTWARE-type reset (esp_restart, a panic, any
//  watchdog) the needle has not moved, and the needle task keeps a checksummed
//  record of its position in RTC memory that survives such a reset: the frame
//  is seeded from it, together with any index clue that was on record, and
//  homing then searches toward the index instead of away from it. After a
//  power-on or brownout, or when the record is missing or torn, it is exactly
//  assumeAtLowStop(). Homing still runs either way; this only chooses where it
//  starts believing. The record is invalidated as it is read, so it is never
//  used twice, and nothing is recorded until this (or assumeAtLowStop()) runs.
void bootPosition();

//  Find the index, then sweep, then go wherever `track()`/`park()` asked for.
//  Returns nullptr if homing started, otherwise WHY NOT - a static string to
//  show (2026-09-25). It refuses while a band calibration or a bring-up tool
//  (a held coil, a jog) holds the needle, and prints the refusal on the console
//  as well. A new attempt also clears faultReason(): the old fault is no longer
//  the current state once someone is trying again.
const char *startHoming();

//  RECOVER FROM A SLIP TOO LARGE TO ABSORB, without trusting the number that
//  revealed it. Goes to the index and re-establishes zero from the forward
//  re-approach; the suspect measurement is used only as a direction hint, which
//  is better evidence than homing's usual one (the sign of a frame we have just
//  decided not to believe). Returns false if it will not start - a calibration
//  or a bring-up tool holds the needle, homing is already running, or the sweep
//  is running. With no zero at all (not homed, e.g. in FAULT) it runs a plain
//  startHoming() instead and returns whether that started (2026-09-25).
//  A slip found DURING the sweep does not wait for this: the needle task ends
//  the sweep and re-indexes on its own (2026-09-24).
bool startReindex(bool routine = false);   // routine: the idle check, not a repair
//  How many automatic recoveries have failed in a row. Three and it stops
//  trying, because a needle that slips straight back out is a mechanical
//  problem and repeating for ever would grind rather than fix.
uint8_t recoverTries();
bool    recovering();

//  True while the emitter is producing steps that go nowhere - the signature of
//  a hunt, stated WITHOUT a cause, because the cause differs every time. What
//  found the wobble of 2026-09-10 was the author watching the ULN2003's LEDs
//  while the status line reported the needle stationary; this is the machine
//  being able to notice that for itself.
bool    hunting();
bool homed();

//  TRUE if the request will be carried out (2026-09-25): now, after the busy
//  phase that is running (homing, the sweep, a calibration), or after the home
//  it just started. FALSE if it is dropped: unhomed in FAULT or any non-IDLE
//  state, or the home it needed was refused. Behaviour is unchanged; only the
//  answer is new, so the portal can stop claiming success it did not have.
bool track();
bool park();

//  Full measured travel, end to end. `repeat` keeps it going until stop().
//  FALSE if refused - not homed (2026-09-25).
bool sweepRange(bool repeat);

//  Abandon the move where it stands, release the coils, and stay put. Not a
//  request to decelerate. Also aborts a running band calibration and ends an
//  automatic re-index (2026-09-24).
//  Safe from any task on either core (2026-09-24): the motor stops at once, and
//  the rest - calibration abort, state() back to IDLE, coils released - is done
//  by the needle task within one 5 ms tick. Do not expect state() == IDLE on
//  the very next line.
//  A HELD COIL TEST (coilTest 1..4) is released by a stop as well, and the
//  drive handed back (2026-09-25); a running jogRaw() is not interrupted.
void stop();

//  True from a stop() or calAbort() until the needle task has carried it out -
//  at most one 5 ms tick (2026-09-25). For a caller that must not start the
//  needle again while a stop it asked for is still on its way: anything it
//  started now would be undone by that stop a moment later.
bool stopPending();

// --- motion, defaults from the rig's validated configuration (upVmax aside) --
//  "Up" limits govern every second-order move, in either direction; "down"
//  limits govern the decay, which only the park uses. Same split for the
//  dwell (the up dwell is skipped while tracking).
void setUpLimits(uint16_t vmaxHsps, uint16_t accelHsps2);
void setDownLimits(uint16_t vmaxHsps, uint16_t accelHsps2);
void setSecondOrder(float wn, float zeta);
void setDecayEnvelope(uint16_t riseMs, uint16_t fallMs);
void setDwell(uint16_t upMs, uint16_t downMs);
//  `moving` is the division for sweeps, parks, homing, the band calibration
//  and a microstep jog; `resting` is the division while TRACKING.
void setMicro(uint8_t moving, uint8_t resting);
//  Speed of the band-calibration creep (and the portal jog passes it to
//  jogRaw()). Homing's REAPPROACH does not use it - it moves in +10 half-step
//  second-order hops.
void setReapproachSpeed(uint16_t hsps);

// --- geometry, in HALF-STEPS relative to the index --------------------------
//  posMin/posMax are the soft limits and the only thing keeping the needle on
//  the dial. They must bracket 0 (the index); a pair that does not is refused.
void setGeometry(int32_t posMin, int32_t posMax);

//  What is PRINTED at the needle's two stops, in tenths of a MHz.
void setDial(uint16_t dialLow, uint16_t dialHigh);

//  THE ONE DEFINITION of the tuner's angle -> frequency mapping.
//
//  It used to be computed independently in SIX places - trackTarget(), the
//  Leditron readout, the portal's state JSON, the console's own
//  "tune chain" line, and both mark actions - each
//  rebuilding `bandLow + permille * span / 1000` from the same two constants.
//  While the model was a straight line they agreed by construction. It is not
//  a straight line (the author's own station readings: local slope runs 0.251
//  tenths/permille at 91-98 MHz and 0.288 at 98-107, so a line fitted to the
//  ends misses the middle by half a megahertz). A curve computed six times is
//  a curve that will be computed six different ways.
//
//  f(p) = a + b*(p - p0) + c*(p - p0)^2, in tenths of a MHz, plus a constant
//  offset that slides the whole curve without changing its shape.
//  THE CURVE IS A FUNCTION OF THE RAW ACCUMULATOR, not of permille.
//  permille is DERIVED from calLow/calHigh, so anchoring stored calibration
//  points to it meant that re-running the tuner-end calibration silently
//  redefined the coordinate every stored point had been measured in. Guarding
//  that took a warning on four separate writers, a staleness flag and a
//  deferred detector, and it still had a path that reported nothing and a flag
//  with no way to clear it. The accumulator is the shaft itself; nothing else
//  defines it, so there is nothing to invalidate and none of that machinery
//  needs to exist.
//
//  x is clamped to [xlo,xhi] before evaluation - the domain the curve was
//  actually fitted over. That is what stops a quadratic extrapolating into
//  nonsense, which is the one useful thing permille's 0..1000 clamp was doing.
void    setCurve(double x0, float a, float b, float c, int32_t offset10,
                 int32_t xlo, int32_t xhi);
int32_t tuneFreq10();                   // at the shaft's current position
int32_t tuneFreq10At(int32_t acc);      // at any accumulator value
//  UNROUNDED. The Leditron snaps its readout to the odd-tenth FM channel grid,
//  and which channel is nearest cannot be decided from a value that has already
//  been rounded to a tenth - see the snap in updateDisplay().
float   tuneFreq10f();
//  The accumulator range the installed curve is valid over. Every "reaches
//  X - Y MHz" readout must be evaluated HERE and nowhere else: asking at
//  calLow/calHigh instead gives a value the evaluator has silently clamped, and
//  on a set whose accumulator counts down against frequency it prints backwards.
void    curveDomain(int32_t &lo, int32_t &hi);
void getGeometry(int32_t &posMin, int32_t &posMax);

//  The four numbers from the author's calibration: for each side, the position
//  at which the sensor releases and at which it operates. Half-steps, relative
//  to the homed zero. onFwd is the edge homing uses.
void setIndexCal(int32_t onFwd, int32_t offFwd, int32_t onRev, int32_t offRev);
void getIndexCal(int32_t &onFwd, int32_t &offFwd, int32_t &onRev, int32_t &offRev);

//  CALIBRATION - NON-BLOCKING, because these become portal buttons and a
//  blocking calibration would freeze the HTTP handler for its whole duration.
//  Start one, then poll calBusy()/calProgressPct()/calMessage().
//
//  The needle must already be ON the sensor. Measures all four edges
//  N times, reports the SPREAD as well as the means, and shifts the frame so the
//  forward ON edge is zero. Required before a soft limit can be captured
//  (captureLimit refuses until bandCalibrated), and before reverse crossings
//  and the band check are used: without pOnRev a reverse crossing would read
//  the whole direction offset as drift.
//
//  Returns nullptr if it started, otherwise the reason it refused (2026-09-24:
//  it now refuses unless homed, with no slip outstanding, and with no homing or
//  sweep holding the needle). The reason is a static string; show it.
const char *calStartBand(uint8_t passes);

//  Like stop(), a request (2026-09-24): motion stops at once, calBusy() goes
//  false within one needle-task tick.
void        calAbort();
bool        calBusy();
uint8_t     calProgressPct();
const char *calMessage();

//  The BAND result, collected once. Without this the four index constants live
//  only in RAM and the next applySettings() zeroes them - see calFinishBand().
//  shiftOut is how far calFinishBand() moved the position frame. The soft
//  limits are physical marks, so they must be shifted by the same amount or
//  they silently describe the wrong places.
bool calTakeBand(int32_t &onFwd, int32_t &offFwd, int32_t &onRev, int32_t &offRev,
                 int32_t &shiftOut);

int32_t position();        // half-steps, index-relative
int32_t target();
bool    indexNow();        // live sensor reading
int32_t lastDrift();       // expected minus actual at the last index crossing
bool    driftPending();    // ...and was it too big to absorb, so still true now?
bool    driftMeasured();   // has the index been crossed since the last home?
bool    bandCalibrated();  // has the index band calibration ever run?
uint32_t indexCrossings();


//  Why homing gave up, or "" if it has not. The console printed this once and
//  then it was gone; the author cannot sit on a serial cable, so it has to
//  survive somewhere the portal can read it.
const char *faultReason();

//  Announcing every index edge is essential while hunting for the sensor and
//  unusable the rest of the time - it drowns anything else on the console.
void setEdgeLog(bool on);
bool edgeLog();

// --- AS5600 -----------------------------------------------------------------
uint16_t angleRaw();
bool     magnetOk();
uint8_t  agc();
bool     i2cOk();
//  How many AS5600 outages since boot. A sampler compares it at start and at
//  commit: any outage in between means the shaft count may not be the one the
//  measurement started from.
uint32_t encoderGaps();
//  bothMeasured: both ends were really captured (tunerEndsSet == both). Only
//  then are they used to decide which turn the shaft is on (seatTurn).
void     setCalibration(int32_t angleAtLow, int32_t angleAtHigh, bool bothMeasured = false);
void     getCalibration(int32_t &angleAtLow, int32_t &angleAtHigh);
int32_t  accumulated();
void     seedAccumulator(int32_t lastKnown);
bool     tuningActive();

//  RAW diagnostics. "NO MAGNET" is three bits collapsed into one boolean and
//  they need different fixes; MAGNITUDE is the field strength itself. The
//  accumulator and the raw angle are exposed so the invariant
//  (accumulator - rawAngle) & 4095 can be watched - it is constant by
//  construction, so any change is an aliasing event with a timestamp.
uint8_t  statusByte();
uint16_t fieldMagnitude();
int32_t  accumulator();
uint16_t rawAngleNow();
uint16_t tunePermille();

// --- bring-up, for isolating the drive layer --------------------------------
//  These bypass the profile, the homing state machine and the soft limits, and
//  they suspend the step emitter while they run. They exist because "the
//  firmware emitted every step and the motor did not turn" narrows the fault to
//  the electrical output, and nothing above the drive layer can test that.

//  coil 1..4 energises that ULN input alone at full duty and HOLDS it, so it
//  can be metered and the rotor felt. 0 releases everything.
void coilTest(int coil);

//  Blind, unprofiled, constant-rate stepping (5..900 hsps), in plain HALF-step
//  mode - the same thing the previous firmware did with digitalWrite - or, with
//  `microstep`, at the moving division. If HALF moves the motor and MICRO does
//  not, the fault is the sine PWM, not the wiring. Blocks its caller for the
//  whole jog and ignores the soft limits (it only warns); leaves the needle
//  IDLE where it stopped.
void jogRaw(int32_t halfSteps, uint16_t hsps, bool microstep);

// --- diagnostics ------------------------------------------------------------
uint32_t jitterMaxUs();    // worst step-to-step timing error
void     jitterClear();
uint32_t stepsEmitted();
float    velHsps();

}  // namespace Needle
