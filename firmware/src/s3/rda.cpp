#include "rda.h"
#include <Wire.h>
#include "pins.h"

namespace Rda {

// ---------------------------------------------------------------------------
//  THE PART. RDA5807M, on its own I2C bus, Wire1 (wiring: Bible §11).
//
//  Its I2C addresses are FIXED IN SILICON - 0x11 random access, 0x10
//  sequential, 0x60 TEA5767 emulation. Three modes of one chip, no address
//  select pin and no strap. It is kept off the AS5600's bus because that bus
//  carries the tuning sensor and was set up for it alone (Bible §11); spending
//  its margin to save two spare GPIOs would be a bad trade.
// ---------------------------------------------------------------------------
static const uint8_t ADDR_RANDOM = 0x11;

//  Register 0x02
static const uint16_t R2_DHIZ    = 0x8000;
static const uint16_t R2_DMUTE   = 0x4000;
static const uint16_t R2_MONO    = 0x2000;
static const uint16_t R2_RESET   = 0x0002;
static const uint16_t R2_ENABLE  = 0x0001;
static const uint16_t R2_NEWMETH = 0x0004;

//  0x04 bit 8, AFC DISABLE. Defaults to 0, meaning AFC WORKS; initChip() sets
//  it, because an instrument must sit where it is put - see the note there.
static const uint16_t R4_AFCD    = 0x0100;
//  0x0A bit 14 STC (tune complete) and bit 10 ST (stereo indicator).
static const uint16_t R0A_STC    = 0x4000;
static const uint16_t R0A_ST     = 0x0400;

//  Register 0x03
static const uint16_t R3_TUNE    = 0x0010;
static const uint16_t R3_BAND2   = 0x0008;   // bits [3:2] = 10 -> 76..108 MHz
static const uint16_t R3_SPACE_100K = 0x0000;
static const uint16_t R3_SPACE_50K  = 0x0002;

static TwoWire     &BUS = Wire1;
static bool         gUp        = false;
//  volatile: rdaTask on core 1 CLEARS it when the part stops answering
//  mid-run, and other tasks read it.
static volatile bool gPresent  = false;

//  THE PART CAN BE LOST AFTER BOOT. Until 2026-09-24 gPresent was set only in
//  begin(), so a module that stopped answering later stayed "present" and the
//  sampler blamed a fixed feature for the empty bins.
//
//  So: LOST_AFTER consecutive failed points end the run, clear gPresent and
//  raise gLost. Five, because a single NACK is a transient and has always been
//  tolerated (a failed bin records 0 and the sweep goes on), while five in a
//  row on a bus that answered a moment ago is not a transient - and five is
//  cheap: at worst a few hundred ms of Wire timeouts before the run gives up.
//  gBusFails is the running total since boot, so the sampler's message and the
//  state line can name the cause with a number rather than infer it from silence.
static const uint8_t  LOST_AFTER = 5;
static volatile bool  gLost      = false;
static volatile uint16_t gBusFails = 0;
static uint8_t        gRunFails  = 0;     // consecutive, this run; task-only
//  ALL failed transfers of the LAST run (sweep or refinement), consecutive or
//  not. Read by the sampler once the run has ended: a measurement taken over a
//  bus that failed even once is not stored.
static volatile uint8_t gRunBad = 0;
static uint32_t       gLastProbe = 0;     // millis() of the last re-probe
static const uint32_t PROBE_GAP_MS = 10000;
static uint8_t      gFloor     = 0;

//  Sweep request/result state. Owned by the task; read by everyone else.
static volatile bool  gWant    = false;
static volatile bool  gRunning = false;
static int16_t  gReqLo = 0, gReqHi = 0;
static uint8_t  gReqStep = 1;
static uint16_t gReqDwell = 300;

//  THE FINE PASS rides the same gWant/gRunning pair as the sweep - see the note
//  on requestRefine(). Frequencies here are TWENTIETHS of a MHz (50 kHz units),
//  never tenths, and the names say so.
static volatile bool gReqFine = false;
static int16_t  gFineCentre10 = 0;
static int16_t  gFineF20 = 0;         // 0 = nothing refined yet
static uint8_t  gFineR   = 0;
//  THE FINE PASS'S OWN POINTS, kept apart from the coarse bins so both stay
//  readable after a measurement: the coarse sweep says
//  where the oscillator is, the fine pass says exactly where, and each is the
//  evidence for its half of the answer. Rssi 0 = skipped (inside a fixed
//  feature, or a failed read); a broadcast keeps its reading but is not chosen.
static const uint8_t FINE_POINTS = 11;
static int16_t  gFinePtF20[FINE_POINTS];
static uint8_t  gFinePtR[FINE_POINTS];
static volatile uint8_t gFinePts = 0;

static int16_t  gBinF[MAX_BINS];
static uint8_t  gBinR[MAX_BINS];
//  One bit per bin: the stereo indicator at that frequency. See isStereoBin().
static uint8_t  gBinST[(MAX_BINS + 7) / 8];
//  volatile: written by rdaTask on core 1, read by other tasks while a sweep
//  may be running. The bin ARRAYS are unsynchronised - a console or portal dump
//  taken mid-sweep can read a torn set. The sampler is not exposed: it reads
//  the bins only once sweeping() is false. Recorded rather than fixed, because
//  fixing it properly means a double buffer.
static volatile uint8_t gBins = 0;
static volatile uint8_t gPct = 0;

static int16_t  gSpur[MAX_SPURS];
static uint8_t  gSpurs = 0;

static char     gState[96] = "not started";

// ---------------------------------------------------------------------------
static bool wr(uint8_t reg, uint16_t val) {
  BUS.beginTransmission(ADDR_RANDOM);
  BUS.write(reg);
  BUS.write((uint8_t)(val >> 8));
  BUS.write((uint8_t)(val & 0xFF));
  return BUS.endTransmission() == 0;
}

static bool rd(uint8_t reg, uint16_t &val) {
  BUS.beginTransmission(ADDR_RANDOM);
  BUS.write(reg);
  if (BUS.endTransmission(false) != 0) return false;
  if (BUS.requestFrom((int)ADDR_RANDOM, 2) != 2) return false;
  uint16_t hi = BUS.read(), lo = BUS.read();
  val = (uint16_t)((hi << 8) | lo);
  return true;
}

// ---------------------------------------------------------------------------
//  Start a tune to a frequency in tenths of a MHz, band 2. It does not wait:
//  the caller waits for STC and the dwell.
//  CHANNEL ARITHMETIC: freq = bandStart + space * CHAN, bandStart 76.0 MHz.
//  Every caller passes fine = false (100 kHz spacing). `fine` = true doubles
//  the channel at 50 kHz spacing, which only re-addresses the same 100 kHz
//  points; the 50 kHz path is tuneTo20().
static bool tuneTo(int16_t f10, bool fine) {
  int32_t chan = fine ? (int32_t)(f10 - 760) * 2 : (int32_t)(f10 - 760);
  if (chan < 0 || chan > 1023) return false;
  uint16_t r3 = (uint16_t)((chan << 6) | R3_TUNE | R3_BAND2 |
                           (fine ? R3_SPACE_50K : R3_SPACE_100K));
  return wr(0x03, r3);
}

//  TUNE ON THE 50 kHz GRID. freq = 76.0 + 0.05 * CHAN, so with f20 in
//  twentieths of a MHz the channel is simply f20 - 1520. This is what the
//  `fine` flag on tuneTo() could never express: that function takes TENTHS, so
//  doubling them only re-addresses the same 100 kHz points.
static bool tuneTo20(int16_t f20) {
  int32_t chan = (int32_t)f20 - 1520;
  if (chan < 0 || chan > 1023) return false;
  uint16_t r3 = (uint16_t)((chan << 6) | R3_TUNE | R3_BAND2 | R3_SPACE_50K);
  return wr(0x03, r3);
}

//  THE READ CAN FAIL, AND THAT IS NOT A ZERO. Until 2026-09-24 this returned 0
//  on a failed transfer, indistinguishable from a real reading of 0 and never
//  counted as a bus failure. The caller now decides what a failed read means.
static bool readRssi(uint8_t &r) {
  uint16_t v = 0;
  r = 0;
  if (!rd(0x0B, v)) return false;
  r = (uint8_t)(v >> 9);               // RSSI is bits [15:9]
  return true;
}

//  THE STEREO PILOT, WHICH AN OSCILLATOR CANNOT HAVE. Read after the tune has
//  completed - ST's power-on default is 1, so reading it early reports "stereo"
//  for everything. True is taken as a broadcast only above the floor (see
//  bestCandidate()); false means nothing.
//  RETURNS WHETHER THE READ HAPPENED. A failed read used to come back as
//  "mono", so a broadcast whose ST read failed passed as the oscillator - and
//  was never counted as a bus failure. Same rule as
//  readRssi: a failed read is not a value.
static bool readStereo(bool &st) {
  uint16_t v = 0;
  st = false;
  if (!rd(0x0A, v)) return false;
  st = (v & R0A_ST) != 0;
  return true;
}

//  Wait for the tune to complete rather than guessing. Returns false on
//  timeout, which is itself worth knowing.
static bool waitTuned(uint16_t maxMs) {
  uint32_t t0 = millis();
  while (millis() - t0 < maxMs) {
    uint16_t v = 0;
    if (rd(0x0A, v) && (v & R0A_STC)) return true;
    vTaskDelay(pdMS_TO_TICKS(5));
  }
  return false;
}

//  THE SAME WIDTH isSpur() GIVES A FIXED FEATURE, ON THE 50 kHz GRID. isSpur()
//  blinds +/-2 tenths, which is +/-0.2 MHz - so here +/-4 twentieths. At every
//  100 kHz point (f20 even) this is exactly isSpur(f20 / 2); the half-way
//  points in between get the same 0.2 MHz, not a rounding of it. isSpur() is
//  defined further down; the list and its publishing order are the same.
static bool isSpur20(int16_t f20) {
  for (uint8_t i = 0; i < gSpurs; i++) {
    int16_t d = (int16_t)(f20 - gSpur[i] * 2);
    if (d < 0) d = (int16_t)-d;
    if (d <= 4) return true;
  }
  return false;
}

//  ONE FAILED POINT. Counted, and then the task YIELDS. rdaTask is priority 2
//  on core 1, above loop() at priority 1; until 2026-09-24 nothing on the
//  failure path blocked, and back-to-back failed transfers starved loop() past
//  the A32 link's 2 s silence limit, so the audio muted. Ten milliseconds is
//  nothing to a sweep that dwells 300 ms per bin. Returns true when the run
//  must end (LOST_AFTER in a row).
static bool pointFailed() {
  gBusFails = (uint16_t)(gBusFails + 1);
  gRunFails++;
  if (gRunBad < 255) gRunBad++;
  vTaskDelay(pdMS_TO_TICKS(10));
  return gRunFails >= LOST_AFTER;
}

static void markLost() {
  gPresent = false;
  gLost = true;
  snprintf(gState, sizeof(gState),
           "LOST - the RDA5807M stopped answering (%u failed transfers since boot)",
           (unsigned)gBusFails);
}

// ---------------------------------------------------------------------------
static void rdaTask(void *) {
  for (;;) {
    if (!gWant) { vTaskDelay(pdMS_TO_TICKS(100)); continue; }

    //  RUNNING GOES UP BEFORE WANT COMES DOWN, and the order is the whole
    //  point: between the two assignments a reader on the other core must never
    //  see BOTH clear, or it concludes no sweep is in flight and reads the
    //  PREVIOUS sweep's bins as if they were this one's.
    gRunning = true;
    gWant = false;
    gPct = 0;
    gRunFails = 0;

    //  THE FINE PASS. Eleven points 50 kHz apart centred on the coarse
    //  candidate - +/-0.25 MHz, which is two and a half coarse bins and
    //  comfortably covers a peak the coarse grid put one bin out. Highest
    //  reading wins: the coarse pass already established that this IS the
    //  oscillator, so this only asks where exactly, and a local floor over
    //  eleven adjacent points would be measuring the peak against itself.
    if (gReqFine) {
      int16_t centre20 = (int16_t)(gFineCentre10 * 2);
      int16_t bestF = 0; uint8_t bestR = 0;
      snprintf(gState, sizeof(gState), "refining %d.%d at 50 kHz",
               gFineCentre10 / 10, gFineCentre10 % 10);
      bool lost = false;
      gRunBad = 0;
      gFinePts = 0;
      for (int8_t k = -5; k <= 5 && !lost; k++) {
        //  PROGRESS FIRST, so a point that is skipped still moves the bar. It
        //  used to be the last line of the body, and every `continue` above it
        //  - a failed tune, a broadcast - left the bar where it was.
        gPct = (uint8_t)((k + 5) * 100 / 11);
        int16_t f20 = (int16_t)(centre20 + k);
        uint8_t pt  = (uint8_t)(k + 5);
        gFinePtF20[pt] = f20; gFinePtR[pt] = 0; gFinePts = (uint8_t)(pt + 1);
        //  NOT INSIDE A FIXED FEATURE. The coarse pass never picks a spur bin,
        //  but a candidate three or four bins from one is legal, and this pass
        //  reaches 0.25 MHz either side of it - into the feature's skirt. A
        //  fixed feature can be LOUDER than the oscillator (79.0 reads 63
        //  against the LO's 56-59, rda.h note 3), so highest-reading-wins stored
        //  the skirt as the oscillator, up to 0.25 MHz wrong.
        //  Same width isSpur() gives the coarse pass; see isSpur20().
        if (isSpur20(f20)) continue;
        if (!tuneTo20(f20)) { lost = pointFailed(); continue; }
        waitTuned(60);
        vTaskDelay(pdMS_TO_TICKS(300));
        uint8_t r = 0;
        if (!readRssi(r)) { lost = pointFailed(); continue; }
        bool st = false;
        if (!readStereo(st)) { lost = pointFailed(); continue; }
        gRunFails = 0;
        gFinePtR[pt] = r;
        //  A BROADCAST IS NOT THE OSCILLATOR. The coarse pass throws out stereo
        //  bins (see candidate()); this pass did not, so a louder station 0.1-
        //  0.25 MHz from the LO won the refinement and was stored as the
        //  oscillator - 107.3 on the dial puts the LO at 96.7, next to the 96.9
        //  broadcast. Same rule as the coarse pass: the flag
        //  counts only above the floor, where there is something to decode.
        if (st && r > (uint8_t)(gFloor + 3)) continue;
        if (r > bestR) { bestR = r; bestF = f20; }
      }
      gPct = 100;
      //  A REFINEMENT THAT LOST THE PART FOUND NOTHING. Whatever it read before
      //  the bus went is a partial pass, not a refinement, so it is discarded
      //  and the caller falls back to the coarse answer, which WAS measured.
      if (lost) {
        gFineF20 = 0; gFineR = 0; gFinePts = 0;
        markLost();
      } else {
        gFineF20 = bestF; gFineR = bestR;
        snprintf(gState, sizeof(gState), "refined to %d.%02d MHz, rssi %u",
                 bestF / 20, (bestF % 20) * 5, (unsigned)bestR);
      }
      gReqFine = false;
      gRunning = false;
      continue;
    }

    int16_t lo = gReqLo, hi = gReqHi;
    uint8_t stp = gReqStep ? gReqStep : 1;
    uint16_t dwell = gReqDwell < 300 ? 300 : gReqDwell;   // see the header note

    uint16_t total = (uint16_t)((hi - lo) / stp + 1);
    if (total > MAX_BINS) total = MAX_BINS;

    snprintf(gState, sizeof(gState), "sweeping %d.%d-%d.%d at %u kHz, %u ms",
             lo / 10, lo % 10, hi / 10, hi % 10, (unsigned)(stp * 100), (unsigned)dwell);

    //  EVERY BIN IS WRITTEN, INCLUDING THE ONES THAT FAILED. A failed point used
    //  to skip its assignments, so the slot kept a PREVIOUS sweep's numbers -
    //  possibly a frequency outside this range - and was counted as this one's:
    //  a measurement not taken, reported as if it were. A failed bin records
    //  rssi 0 at its true frequency and is counted, so the operator sees how
    //  much of the sweep actually happened.
    uint16_t failed = 0;
    bool lost = false;
    gRunBad = 0;
    //  ONLY A SWEEP REPLACES THE BINS. They used to be cleared at the top of
    //  every run, the refinement included, so a measurement ended with nothing
    //  to show for its coarse half (2026-09-25).
    gBins = 0;
    gFinePts = 0;               // a new sweep starts a new measurement
    for (uint16_t i = 0; i < total && !lost; i++) {
      int16_t f = lo + (int16_t)(i * stp);
      gBinF[i] = f;
      gBinST[i >> 3] = (uint8_t)(gBinST[i >> 3] & ~(1u << (i & 7)));
      if (!tuneTo(f, false)) { gBinR[i] = 0; failed++; lost = pointFailed(); }
      else {
        //  STC first, then the dwell. The flag says the synthesiser has settled;
        //  the dwell is what rda.h note 2 records as needed for the RSSI reading
        //  itself to settle - 40 ms produced a confident false negative that hid
        //  the oscillator entirely. Both, not either.
        waitTuned(60);
        vTaskDelay(pdMS_TO_TICKS(dwell));
        uint8_t r = 0;
        bool st = false;
        if (!readRssi(r) || !readStereo(st)) { gBinR[i] = 0; failed++; lost = pointFailed(); }
        else {
          gRunFails = 0;
          gBinR[i] = r;
          if (st) gBinST[i >> 3] = (uint8_t)(gBinST[i >> 3] | (1u << (i & 7)));
        }
      }
      gBins = (uint8_t)(i + 1);
      gPct = (uint8_t)((i + 1) * 100 / total);
    }

    //  A SWEEP THAT LOST THE PART IS NOT A SWEEP. Its last LOST_AFTER bins are
    //  zeros that were never measured, and a run of zeros drags every nearby
    //  local-floor median down - so the genuine bins just before the loss would
    //  stand out as a candidate that is an artefact of the failure. Publish
    //  nothing: zero bins, zero floor, and a state that says why.
    if (lost) {
      gBins = 0;
      gFloor = 0;
      markLost();
      gPct = 100;
      gRunning = false;
      continue;
    }

    //  The floor is the MEDIAN, not the mean: one loud spur must not lift it.
    //  RESET FIRST: it used to survive a sweep that produced nothing, so a
    //  failed sweep printed the PREVIOUS sweep's floor beside its own zero bins.
    gFloor = 0;
    if (gBins) {
      uint8_t tmp[MAX_BINS];
      memcpy(tmp, gBinR, gBins);
      for (uint8_t a = 1; a < gBins; a++) {          // insertion sort, tiny n
        uint8_t v = tmp[a]; int b = a - 1;
        while (b >= 0 && tmp[b] > v) { tmp[b + 1] = tmp[b]; b--; }
        tmp[b + 1] = v;
      }
      gFloor = tmp[gBins / 2];
    }

    if (failed)
      snprintf(gState, sizeof(gState), "swept %u bins, floor %u, %u FAILED on the bus",
               (unsigned)gBins, (unsigned)gFloor, (unsigned)failed);
    else
      snprintf(gState, sizeof(gState), "swept %u bins, floor %u", (unsigned)gBins, (unsigned)gFloor);
    gPct = 100;
    gRunning = false;
  }
}

// ---------------------------------------------------------------------------
//  THE REGISTER SET-UP, shared by begin() and reprobe(). A part that came back
//  after being lost may have browned out and reset to its power-on defaults -
//  AFC on among them - so a re-probe that only checked for an answer would put
//  an instrument back in service configured as a radio. Returns true only if
//  every configuring write landed and the part answered the read at the end.
static bool initChip() {
  //  Reset, then enable. NEW_METHOD improves sensitivity on this part and is
  //  what the bench rig ran with.
  wr(0x02, R2_RESET | R2_ENABLE);
  delay(50);
  //  MONO CLEARED, deliberately, and it is not about audio. The stereo
  //  indicator in 0x0A is the MPX decoder's output, and forcing mono is the
  //  most likely reason it would read 0 for everything - the vendor library
  //  forces mono in powerUp() and every one of its examples that shows a stereo
  //  indicator has to call setMono(false) first. ST is the one amplitude-free
  //  judgement this part will give us, and it is worth having.
  //  EVERY CONFIGURING WRITE MUST LAND: when only the read
  //  below decided "present", one NACK on the AFC write left a receiver free to
  //  slide toward a louder neighbour while every measurement counted as clean.
  //  The reset above is not counted: everything it clears is written again here.
  bool ok = wr(0x02, R2_DHIZ | R2_DMUTE | R2_NEWMETH | R2_ENABLE);

  //  AFC OFF. 0x04 bit 8 is "AFC disable" and AFC defaults to ENABLED; until
  //  this write was added every sweep was taken by a receiver free to slide off
  //  the channel it was told to sit on and toward whatever strong broadcast was
  //  nearby. Half this window is inside the real FM band. An instrument that
  //  reports WHERE a carrier is must not be allowed to drift toward a louder
  //  neighbour and then report the neighbour's place.
  ok = wr(0x04, R4_AFCD) && ok;
  delay(50);

  //  Band 2 (76-108), 100 kHz spacing, no tune yet.
  ok = wr(0x03, R3_BAND2 | R3_SPACE_100K) && ok;

  //  LNA: both knobs at their most sensitive. The bench ran with the part's
  //  DEFAULTS - LNA_ICSEL_BIT 0, the LOWEST current (1.8 mA), and LNA_PORT_SEL
  //  2 - and still read the LO at 59 against a floor of 44. Inside the cabinet
  //  that margin may not survive, and these are free.
  //    bits 7:6 LNA_PORT_SEL = 3 (dual port, covers either antenna routing)
  //    bits 5:4 LNA_ICSEL_BIT = 3 (2.7 mA)
  //    bits 3:0 VOLUME = 0 (this is an instrument, not a radio)
  ok = wr(0x05, (uint16_t)((3 << 6) | (3 << 4) | 0)) && ok;

  uint16_t v = 0;
  //  A set-up that did not fully land is reported as absent; reprobe() tries
  //  the whole set-up again within ten seconds.
  return rd(0x0A, v) && ok;
}

void begin() {
  if (gUp) return;
  BUS.begin(S3_RDA_SDA, S3_RDA_SCL, S3_I2C_HZ);
  gUp = true;

  gPresent = initChip();
  gLastProbe = millis();
  snprintf(gState, sizeof(gState), gPresent ? "present, idle" : "NOT FOUND on the bus");

  xTaskCreatePinnedToCore(rdaTask, "rda", 3072, nullptr, 2, nullptr, 1);
}

bool    present()        { return gPresent; }
bool    lost()           { return gLost; }
uint16_t busFailures()   { return gBusFails; }
uint8_t  runFailures()   { return gRunBad; }

//  A TRANSIENT FAULT HEALS. Once the part is lost nothing would ever touch the
//  bus again - requestSweep() and requestRefine() refuse while it is absent, by
//  design - so without this one bad connector-wiggle retired the instrument
//  until the next boot. Cheap: returns at once if the part is present, and
//  otherwise costs one short transfer and, only if that answers, the ~100 ms
//  of initChip(). Rate-limited to one attempt per PROBE_GAP_MS so a caller in
//  loop() cannot turn a dead bus back into the starvation this file removed.
//  Never while the task holds the bus: a run in flight owns it. Two callers on
//  two tasks can still race each other here; Wire serialises the transfers and
//  the worst outcome is initChip() run twice, which is harmless.
bool reprobe() {
  if (gPresent) return true;
  if (!gUp || gRunning || gWant) return false;
  uint32_t now = millis();
  if (now - gLastProbe < PROBE_GAP_MS) return false;
  gLastProbe = now;
  uint16_t v = 0;
  if (!rd(0x0A, v)) return false;
  if (!initChip()) return false;
  bool was = gLost;
  gLost = false;
  gPresent = true;
  snprintf(gState, sizeof(gState), was ? "present again, idle - answered a re-probe after being lost"
                                       : "present, idle - answered a re-probe after boot");
  return true;
}
uint8_t chipRssiFloor()  { return gFloor; }
//  A QUEUED SWEEP IS A SWEEP. This once returned gRunning alone while the
//  request gate used `gRunning || gWant`, so for the up-to-100 ms before the
//  task noticed a request, a caller was told "not sweeping" and read the
//  PREVIOUS sweep's results as the new ones - the sampler did, on its first run.
bool    sweeping()       { return gRunning || gWant; }
uint8_t progressPct()    { return gPct; }
uint8_t binCount()       { return gBins; }
bool    binStereo(uint8_t i) { return i < gBins && (gBinST[i >> 3] & (1u << (i & 7))); }
int16_t binFreq10(uint8_t i) { return i < gBins ? gBinF[i] : 0; }
uint8_t binRssi(uint8_t i)   { return i < gBins ? gBinR[i] : 0; }
const char *state()      { return gState; }

bool requestSweep(int16_t lo10, int16_t hi10, uint8_t step10, uint16_t dwellMs) {
  if (gRunning || gWant) return false;
  //  INERT WITH THE PART ABSENT. A sweep against a dead bus would be a run of
  //  I2C attempts each blocking on Wire's timeout, on a task above loop()'s
  //  priority; refusing here costs nothing. gPresent is false both for a part
  //  absent at boot and for one lost later (a run that fails LOST_AFTER points
  //  in a row clears it), so this gate covers both. A lost part gets one
  //  re-probe here (rate-limited, see reprobe()) so that a transient fault
  //  heals the next time anyone asks for a measurement.
  if (!gPresent && !reprobe()) return false;
  if (lo10 > hi10) { int16_t t = lo10; lo10 = hi10; hi10 = t; }
  //  Clamped to the dial's own LO window: outside it there is nothing to find
  //  and three of the five known fixed features live there.
  if (lo10 < LO_WINDOW_LO10) lo10 = LO_WINDOW_LO10;
  if (hi10 > LO_WINDOW_HI10) hi10 = LO_WINDOW_HI10;
  //  NOTHING LEFT AFTER THE CLAMP: the request lay wholly outside the window.
  //  Inverted bounds used to wrap the bin count and run a whole-window sweep
  //  reported as a narrow one.
  if (lo10 > hi10) return false;
  gReqLo = lo10; gReqHi = hi10; gReqStep = step10 ? step10 : 1;
  gReqDwell = dwellMs;
  gReqFine = false;                 // a sweep is never a refinement
  gWant = true;
  return true;
}

//  PIN DOWN A COARSE CANDIDATE. Deliberately shares gWant/gRunning with
//  requestSweep() rather than owning a second pair of flags: those two have
//  already been the subject of two separate races in one evening - an accessor
//  that disagreed with its own gate, and a stage raised before the thing it
//  announced - and a third set of flags would be a third opportunity. One
//  request slot, one running flag, one thing in flight at a time.
bool requestRefine(int16_t centre10) {
  if (gRunning || gWant) return false;
  if (!gPresent && !reprobe()) return false;      // see requestSweep()
  if (centre10 < LO_WINDOW_LO10 || centre10 > LO_WINDOW_HI10) return false;
  gFineCentre10 = centre10;
  gFineF20 = 0; gFineR = 0;
  gReqFine = true;
  gWant = true;
  return true;
}

//  Twentieths of a MHz - 1758 is 87.90, 1759 is 87.95. Zero if the last
//  refinement found nothing.
int16_t fineF20()  { return gFineF20; }
uint8_t fineRssi() { return gFineR; }
uint8_t fineCount()               { return gFinePts; }
int16_t finePointF20(uint8_t i)   { return i < gFinePts ? gFinePtF20[i] : 0; }
uint8_t finePointRssi(uint8_t i)  { return i < gFinePts ? gFinePtR[i] : 0; }

// ---------------------------------------------------------------------------
//  THE CANDIDATE. Height above the LOCAL floor, not absolute height, and never
//  a spur. A candidate is not an answer: the sampler in main.cpp applies its
//  own margin, notch and distance checks before anything is stored.
int bestCandidate(uint8_t *marginOut) {
  int best = -1; int bestMargin = 0;
  for (uint8_t i = 0; i < gBins; i++) {
    if (isSpur(gBinF[i])) continue;
    //  A STEREO PILOT MEANS A TRANSMITTER - BUT ONLY IF THERE WAS A SIGNAL.
    //
    //  One-way to begin with: this rejects, it never confirms, because a bare
    //  carrier reads mono and so does a weak or mono broadcast. Then the first
    //  sweep that could be inspected showed the flag set on 87.8 at RSSI 36
    //  against a floor of 42 - six BELOW the noise floor. The datasheet says
    //  why: ST's power-on default is 1, and 0x07's soft blend means the decoder
    //  is making a judgement about a signal that is not there. A pilot decoded
    //  out of noise is not evidence of a transmitter.
    //
    //  So the flag is honoured only where there is something to have decoded it
    //  from. A real broadcast is by definition well above the floor, so nothing
    //  worth rejecting is lost - and the oscillator can no longer be thrown away
    //  because a marginal bin's decoder latched.
    if (binStereo(i) && gBinR[i] > (uint8_t)(gFloor + 3)) continue;
    //  Local floor: median of a window either side, so a wide feature does not
    //  hide inside its own skirt.
    uint8_t lo = (i >= 8) ? (uint8_t)(i - 8) : 0;
    uint8_t hi = (uint8_t)((i + 8 < gBins) ? i + 8 : gBins - 1);
    uint8_t win[17]; uint8_t n = 0;
    for (uint8_t k = lo; k <= hi && n < 17; k++) if (k != i) win[n++] = gBinR[k];
    for (uint8_t a = 1; a < n; a++) {
      uint8_t v = win[a]; int b = a - 1;
      while (b >= 0 && win[b] > v) { win[b + 1] = win[b]; b--; }
      win[b + 1] = v;
    }
    int local = n ? win[n / 2] : gFloor;
    int margin = (int)gBinR[i] - local;
    if (margin > bestMargin) { bestMargin = margin; best = i; }
  }
  if (marginOut) *marginOut = (uint8_t)(bestMargin < 0 ? 0 : bestMargin);
  return best;
}

// ---------------------------------------------------------------------------
uint8_t spurCount()            { return gSpurs; }
int16_t spurFreq10(uint8_t i)  { return i < gSpurs ? gSpur[i] : 0; }

//  Replace the whole list in one call - see the note in rda.h on why this
//  module no longer owns it. Publishing order matters: rdaTask reads the list
//  during a refinement (isSpur20()) while applySettings() may be replacing it.
//  The COUNT goes up last when the list grows and first when it shrinks, so a
//  concurrent reader never reads past the entries written; it may see a mix of
//  old and new entries.
void spurSet(const int16_t *f10, uint8_t n) {
  if (n > MAX_SPURS) n = MAX_SPURS;
  if (n < gSpurs) gSpurs = n;                 // shrink: publish the count first
  for (uint8_t i = 0; i < n; i++) gSpur[i] = f10[i];
  gSpurs = n;                                 // grow: publish the count last
}

//  +/- 200 kHz, because a fixed feature has width and the coarse grid is
//  100 kHz - the 88.5 LO was a coherent four-bin peak at 50 kHz spacing.
bool isSpur(int16_t f10) {
  for (uint8_t i = 0; i < gSpurs; i++) {
    int16_t d = (int16_t)(f10 - gSpur[i]);
    if (d < 0) d = (int16_t)-d;
    if (d <= 2) return true;
  }
  return false;
}

//  THE BIN JUST OUTSIDE A NOTCH, and the feature it borders (0 if none).
//
//  isSpur() blinds +/-2 bins, so the nearest bin a search may pick beside a
//  fixed feature is 3 away. When the tube set's oscillator sits INSIDE a notch,
//  that bin is what bestCandidate() lands on - the true peak is forbidden to
//  it - and the refine starts there and stays near it. Measured 2026-09-23:
//  station 100.7 needs LO 90.1, inside the 90.3 notch; the search took 90.0
//  and the sample stored 100.6, twice, with nothing to say it was wrong.
int16_t besideSpur(int16_t f10) {
  for (uint8_t i = 0; i < gSpurs; i++) {
    int16_t d = (int16_t)(f10 - gSpur[i]);
    if (d < 0) d = (int16_t)-d;
    if (d == 3) return gSpur[i];
  }
  return 0;
}

}  // namespace Rda
