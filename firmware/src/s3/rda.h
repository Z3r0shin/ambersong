#pragma once
#include <Arduino.h>

// ---------------------------------------------------------------------------
//  RDA5807M — reading the tube set's own local oscillator.
//
//  WHAT THIS IS FOR, because it is easy to mistake for a radio.
//  It is not a receiver for the author. It is an instrument that measures where
//  the tube set is tuned, by finding the set's LOCAL OSCILLATOR and adding the
//  intermediate frequency back on. That gives a (shaft angle, frequency) pair
//  at ANY dial position, with no station to identify and nothing to type - the
//  same kind of pair the manual marks produce, so it feeds the same table and
//  the same fit. See Firmware Gospel §6.
//
//  FOUND ON THE BENCH, 2026-09-04:
//    station = LO + IF                LOW-SIDE injection. An earlier design note
//                                     assumed high-side and was wrong. The IF is
//                                     cfg.ifOffset20 (default 10.60 MHz).
//    dial 87.9 - 107.9  ->  LO about 77 - 97 MHz, entirely inside band 2.
//
//  FOUR THINGS THAT COST AN EVENING EACH, all still true:
//   1. BAND 2 (76-108), not band 0 (87-108) - the window starts below band 0.
//   2. DWELL >= 300 ms. At 40 ms the RSSI is read before it settles and every
//      channel collapses into 12..23 instead of 40..63, which hides the LO
//      COMPLETELY and looks like a dead antenna. A confident false negative.
//   3. NEVER "the strongest bare carrier". A fixed spur at 79.0 MHz reads 63,
//      louder than the LO's 56-59, and does not move with the dial. It has been
//      the tallest feature in the window at five different dial positions.
//   4. Threshold against the LOCAL noise floor, never a constant. The 88.5
//      reading rose only +6 where 98.5 and 107.3 rose +15..+18 - and because
//      injection is low-side, the weak end of the window IS the bottom of the
//      dial, so a fixed threshold fails exactly at 87.9-89 and nowhere else.
// ---------------------------------------------------------------------------

namespace Rda {

//  The LO window, in tenths of a MHz: the printed dial 87.9-107.9 minus the
//  nominal 10.7 MHz IF. It was not moved when the IF became a setting, so at
//  the default 10.60 the dial's top (107.9 -> LO 97.3) is one bin outside it.
//  Every sweep is clamped to it, which rejects for free the fixed features seen
//  outside it (98.7, 101.6, 104.4, the 76.2 band edge); the ones inside it are
//  learned (cfg.spur) rather than hard-coded.
static const int16_t LO_WINDOW_LO10 = 772;
static const int16_t LO_WINDOW_HI10 = 972;
//  THE IF IS NOT A CONSTANT ANY MORE - it lives in cfg.ifOffset20, in 50 kHz
//  units, default 212 (10.60 MHz, Bible §22), because it is a property of THIS
//  set's IF strip and not of the design. 214 is the nominal 10.7 in those
//  units. Nothing reads this constant: it is documentation only.
static const int16_t IF_OFFSET20_NOMINAL = 214;   // low-side, nominal only

static const uint8_t MAX_BINS  = 210;        // 77.2..97.2 at 100 kHz + slack
//  RAISED FROM 8 TO 16 ON 2026-09-07. This cabinet's self-interference turned
//  out to be a COMB - 79.0 81.8 84.6 87.4 90.3 93.1 95.9, seven members evenly
//  spaced 2.7-2.8 MHz - so the old limit of eight was one slot from full with a
//  single family of spurs learned, and the eighth press had already been spent
//  on noise at +3 over the local floor. Must equal CFG_SPURS in main.cpp,
//  which is the number frozen into the settings layout; a static_assert there
//  enforces it.
static const uint8_t MAX_SPURS = 16;

void    begin();
bool    present();                  // does the part answer NOW - cleared if it is lost
//  LOST AFTER BOOT (2026-09-24). A sweep or refinement that fails on the bus
//  five points in a row ends early, clears present() and sets lost(); its
//  results are discarded, never published as zeros. lost() stays set until a
//  re-probe succeeds, so a caller can say "the RDA stopped answering" instead
//  of blaming a fixed feature for an empty sweep.
bool    lost();
uint16_t busFailures();             // failed transfers since boot
//  Failed transfers during the LAST sweep or refinement, however scattered.
//  Zero means every point of it was really read. A bin that failed is stored as
//  rssi 0, which drags the local floors down and can make noise look like the
//  oscillator - so the sampler refuses any measurement where this is not zero.
uint8_t runFailures();
//  Try to bring a lost (or never-found) part back: one read, and on an answer
//  the full register set-up again. True if present afterwards. Cheap when
//  present, rate-limited to one bus attempt per 10 s otherwise, refused while
//  a run holds the bus. requestSweep()/requestRefine() call it themselves.
bool    reprobe();
uint8_t chipRssiFloor();            // median of the last sweep, for reference

//  Ask for a sweep. Returns false if one is already running. Non-blocking: the
//  work happens on the module's own task, because a full window is over a
//  minute (201 bins at >= 300 ms each) and nothing on this machine may block
//  for that long. Also false if the part is absent and a re-probe fails, or if
//  nothing of the request is left once it is clamped to the LO window.
bool    requestSweep(int16_t lo10, int16_t hi10, uint8_t step10, uint16_t dwellMs);
bool    sweeping();

//  REFINE A COARSE CANDIDATE TO 50 kHz. The sweep locates the oscillator on a
//  100 kHz grid, which is too coarse to decide which side of a channel boundary
//  the tuning capacitor is actually sitting on. Eleven points, five either side
//  of the centre, about 3.5 s. Shares the sweep's request slot, so sweeping()
//  covers it and only one of the two can be in flight. Refused if the centre is
//  outside the LO window.
bool    requestRefine(int16_t centre10);
int16_t fineF20();      // TWENTIETHS of a MHz: 1758 = 87.90, 1759 = 87.95
uint8_t fineRssi();
//  The fine pass's own points (up to 11, 50 kHz apart), kept beside the coarse
//  bins below. Rssi 0 = skipped: inside a fixed feature, or a failed read. A
//  broadcast point keeps its reading but is never chosen. None after a
//  refinement that lost the part.
uint8_t fineCount();
int16_t finePointF20(uint8_t i);
uint8_t finePointRssi(uint8_t i);
uint8_t progressPct();

//  Results of the last completed sweep. A refinement leaves them in place; a
//  sweep that lost the part publishes none.
uint8_t binCount();
int16_t binFreq10(uint8_t i);
uint8_t binRssi(uint8_t i);
//  Did that bin carry a 19 kHz stereo pilot? True means a real broadcast was
//  there and it is certainly not the tube set's oscillator. False means only
//  that no pilot was decoded - a weak or mono station reads the same as a bare
//  carrier, so this REJECTS and never confirms.
bool    binStereo(uint8_t i);

//  The bin that stands furthest above its LOCAL floor (median of +/-8 bins),
//  skipping learned fixed features and loud stereo bins; -1 if none stands
//  above it. A candidate, not an answer: the sampler in main.cpp decides with
//  its own margin, notch and distance checks. Nothing confirms it by movement.
int      bestCandidate(uint8_t *marginOut);

//  Learned fixed features: anything that does not move when the dial does.
//  The list is cfg.spur, pushed down by spurSet().
uint8_t spurCount();
int16_t spurFreq10(uint8_t i);

//  THE SETTINGS ARE THE ORIGIN; THIS MODULE HOLDS A PROJECTION.
//
//  spurAdd()/spurClear() used to live here and that was the wrong direction.
//  applySettings() runs dozens of times a session - every portal slider move
//  calls it - so with the list owned HERE, any unrelated edit would push cfg's
//  older copy over a spur that had been learned but not yet copied back. Same
//  two-owners shape as the purge that once fired mid-edit and overwrote what
//  had just been typed. main.cpp writes cfg and pushes the whole list down; a
//  restore and a clear then re-arm this module for free.
void    spurSet(const int16_t *f10, uint8_t n);
bool    isSpur(int16_t f10);
int16_t besideSpur(int16_t f10);   // the feature a bin borders, 0 if none

const char *state();                // one line for the console and the portal

}  // namespace Rda
