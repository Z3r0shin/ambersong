#include <Arduino.h>
#include <driver/i2s.h>
#include <driver/gpio.h>
#include <soc/i2s_struct.h>
#include <math.h>
#include "audio.h"
#include "pins.h"

namespace Audio {

static const i2s_port_t PORT        = I2S_NUM_0;
static const uint32_t   SAMPLE_RATE = 44100;
static const size_t     BLOCK       = 256;      // frames per pass, ~5.8 ms

//  ~186 ms of jitter absorption for the Bluetooth path. A2DP delivery is bursty
//  and the radio path is not, so only BT needs a ring.
static const size_t     RING_FRAMES = 8192;

struct Frame16 { int16_t l, r; };
static Frame16          ring[RING_FRAMES];
static volatile size_t  ringHead = 0;     // producer: Bluetooth task
static volatile size_t  ringTail = 0;     // consumer: audio task
static volatile uint32_t underruns = 0;

//  HIGH-WATER, not instantaneous. The state message goes out every 250 ms and
//  a burst of arriving audio between two of them was invisible - the ring can
//  fill and drain entirely inside that window. Reported and reset like the peak
//  meter, so "did any audio arrive at all" is answerable.
static volatile uint32_t ringPeak = 0;

// --- settings, all written from the link task, read by the audio task -------
static volatile uint8_t  curSrc      = SRC_AUX;
static volatile uint8_t  pendSrc     = SRC_AUX;
static volatile uint8_t  volume      = 0;
static volatile bool     muted       = true;
static volatile int16_t  balance     = 0;
static volatile int32_t  gainRadioQ16 = 65536;
static volatile int32_t  gainBtQ16    = 65536;
static volatile bool     monoSum      = true;
static volatile bool     muteOnChange = true;
static volatile uint8_t  taperX10     = 25;    // gamma 2.5, -15 dB at half travel

//  CONSTANT-DC DIAGNOSTIC, 2026-09-01.
//  A fixed non-zero word in every slot, written AFTER the volume and envelope
//  so it is genuinely constant. As audio it is silent - DC has no AC content -
//  so anything audible while this runs is the FAULT and nothing else.
//
//  It answers the one question left: is the pop about the CONTENT of the words
//  or merely about them being non-zero?
//    pops on constant DC  -> content is irrelevant. The corruption is in the
//                            serialiser, the wire or the DAC, and no further
//                            reading of this file will find it. Get a scope.
//    silent on constant DC -> it depends on the values, which kills the
//                            frame-slip story: a slip mangles a constant just
//                            as happily as it mangles music.
//  ANSWERED 2026-09-11: it pops on a constant word (0xFF000000), so the pops do
//  not depend on the content.
static volatile bool     testDc       = false;
//  THE CONSTANT WRITTEN BY THE DC TEST.
//
//  It is a VARIABLE because 0x00100000 confounded the experiment it was built
//  for. That word has exactly one bit set, so DIN carries two edges per frame -
//  and "constant DC was silent" is therefore equally consistent with "the
//  sample values stopped changing" and with "the DATA LINE stopped switching".
//  Those are different causes and the old test could not separate them.
//
//  Sweeping amplitude against edge density separates them. The words on offer
//  are kDc[] in main.cpp.
static volatile int32_t  dcWord       = (int32_t)0xFF000000;  // see kDc[] in main.cpp

//  THE OUTPUT MASK - built 2026-09-01 as a candidate fix. Default off
//  (0xFFFFFFFF), console `k` only, and never confirmed by ear.
//
//  On 2026-09-01 six constant-DC words suggested the pops followed WHERE the
//  last set bit sat in the 32-bit slot:
//
//    0x00FF0000  -42.2 dBFS  8 bits  lowest bit 16  silent
//    0x000000FF -138.5 dBFS  8 bits  lowest bit  0  pops
//
//  Same bit count, 96 dB apart, opposite results. I2S is MSB-first, so bit 0
//  is the last bit before LRCK moves, and the reading then was that activity
//  there cost the PCM5102A its word framing. Zeroing the low bits would leave
//  DIN static across the frame boundary. That mechanism did not survive: on
//  2026-09-11 the sign-extended tail, which removes that edge outright, made no
//  difference, and a constant 0xFF000000 popped.
//
//  Bits 7..0 sit below one 24-bit LSB (SLAS859C does not say whether the part
//  keeps them). Even a 12-bit mask leaves 20 bits, more than the Bluetooth
//  source carries.
static volatile int32_t  outMask      = (int32_t)0xFFFFFFFF;

// ---------------------------------------------------------------------------
//  THE ZERO-DATA WATCH  -  2026-09-10.  THE THEORY IT WITNESSED IS EXCLUDED.
//
//  RESULT FIRST: zddRel stayed frozen through 8 minutes of pops on 2026-09-10,
//  so the DAC's zero-data mute is not the pop. The witness is kept as
//  telemetry; what follows is the reasoning that built it.
//
//  WHAT THE AUTHOR MEASURED, and it is the whole reason this exists: the pop's
//  loudness follows the AMPLIFIER's volume and not the A32's. Every operator in
//  this file sits upstream of ONE linear multiply (the envelope x volume x
//  balance pass below), and a linear operator cannot produce a level-independent
//  artefact - turn the knob down and everything born in here gets quieter with
//  it. So the pop is not born in here. It is born in the DAC.
//
//  AND THE DATASHEET SAYS HOW. SLAS859C 9.3.2.3, Zero Data Detect, verbatim:
//    "When the device detects continuous zero data, it enters a full analog
//     mute condition. The PCM510xA counts zero data over 1024 LRCKs (21ms @
//     48kHz) before setting analog mute."
//  1024 LRCKs is 23.22 ms at our 44.1 kHz. That mute is an ANALOGUE switch: the
//  functional block diagram puts it between the I/V stage and LINE OUT, after
//  everything digital. Its release is not documented as ramped, and TI's own
//  "two-level mute system for pop-free performance" claim covers the ENTRY
//  ("digitally attenuates the data ... and then mutes the analog circuit"),
//  never the exit. So each release is a switching transient whose size is set
//  by the mute circuit and the Directpath output bias - fixed amplitude, no
//  matter what the samples were doing. That matched the reported fault.
//
//  THE AUTHOR'S OTHER TWO OBSERVATIONS FITTED IT TOO, which is why it was
//  worth instrumenting rather than arguing about:
//    - AUX never pops. AUX is the one source where this file writes exact zeros
//      for ever (see the memset below), so the DAC mutes once and never
//      releases. No release, no pop.
//    - Volume at zero stopped them, 2026-09-01. Same mechanism: volQ16 goes to
//      0, every word on the wire is 0, the mute engages and stays engaged.
//
//  WHAT THIS CODE IS. Not a fix - a WITNESS. It mirrors the detector in the
//  part, counting exactly what the part counts, so that "I heard a pop" and
//  "the DAC came out of analogue mute" can be put side by side on the portal
//  instead of being two anecdotes. zddRel did not move while he heard them -
//  see the result above.
//
//  ZERO IS THE DAC'S ZERO, NOT OURS. The witness counts a word as zero when its
//  top 24 bits are, on the reading that the PCM5102A resolves 24 bits of a
//  32-bit slot (SLAS859C does not say whether it keeps bits 7..0). Counting
//  our own == 0 would undercount, and by exactly the margin that matters at
//  low digital volume, where quiet passages land in that bottom byte.
// ---------------------------------------------------------------------------
static const uint32_t ZDD_FRAMES = 1024;          // SLAS859C 9.3.2.3
static const int32_t  ZDD_LSB    = 0x00000100;    // one 24-bit LSB, -138.5 dBFS
//  23.22 ms: one ZDD window. A gap between writes longer than this is counted
//  as a stall - a deliberately wide net: the DMA chain (~46 ms) covers a gap
//  that short, so only a longer one puts zeros on the wire (see the stall
//  detector in the emit block).
static const uint32_t ZDD_STALL_US = 23220;

static volatile uint32_t zddRun     = 0;   // consecutive DAC-zero frames
static volatile uint32_t zddArm     = 0;
static volatile uint32_t zddRel     = 0;
static volatile uint32_t zddArmMs   = 0;
static volatile uint32_t zddRelMs   = 0;
static volatile uint32_t zddLongest = 0;
static volatile bool     zddMuted   = false;
static volatile bool     zeroFloor  = false;
static volatile bool     signTail   = false;   // see THE SIGN-EXTENDED TAIL
static volatile uint32_t zddStalls  = 0;
static volatile uint32_t zddStallUs = 0;
static volatile uint32_t zddStallLastMs = 0;   // when the latest stall ended
static volatile uint32_t zddStallLastUs = 0;   // and how long it was
static volatile uint16_t i2sSticky  = 0;

//  THE VOLUME WATCH - see the block in the emit loop where the gain is cached.
static volatile uint32_t gainSteps  = 0;
static volatile uint32_t volQ16Min  = 0xFFFFFFFFu;
static volatile uint32_t volQ16Max  = 0;

//  Zero as the witness models the part: the top 24 bits of the slot.
static inline bool dacZero(int32_t w) { return ((uint32_t)w & 0xFFFFFF00u) == 0u; }

// --- metering ---------------------------------------------------------------
//  Cache for the volume law. Touched only by the audio task.
static int32_t volCacheQ16 = 0;
static uint8_t volCacheV   = 0xFF;
static uint8_t volCacheT   = 0xFF;

static volatile int32_t  peakL = 0, peakR = 0;
//  RMS AFTER THE SOURCE GAIN, BEFORE THE VOLUME. The peak meter reads the very
//  end of the chain, so it moves with the knob; this one does not, which is
//  what lets the radio and Bluetooth be matched once and stay matched at every
//  volume. Peaks cannot do that job: FM stations are limited hard and music is
//  not, so equal peaks leave the radio sounding louder. Sums are of 16-bit
//  samples squared; a spinlock because the reader is on the other task.
static portMUX_TYPE rmsMux = portMUX_INITIALIZER_UNLOCKED;
static uint64_t rmsSumL = 0, rmsSumR = 0;
static uint32_t rmsN = 0;
static volatile bool     clipFlag = false;

// --- ramping ----------------------------------------------------------------
//  Q24, not Q16, and stepped per SAMPLE.
//
//  The first version of this used Q16 and intended to ramp over ~8 blocks, but
//  advanced the envelope once per sample instead - so it completed in 8 frames,
//  about 0.18 ms. Effectively a hard cut, which is exactly the jumpscare the
//  author reported when switching to an already-playing source.
//
//  Q24 exists because Q16 cannot express a one-second fade: 65536/44100 rounds
//  to 1, and a step of 1 gives 65536 samples, or 1.49 s, with no way to ask for
//  anything shorter. Q24 leaves 380 steps per sample-second of headroom.
static const int32_t ENV_ONE = 1 << 24;
static int32_t envQ24      = 0;
static int32_t envTarget   = 0;
static int32_t stepUpQ24   = ENV_ONE / 44100;        // 1000 ms default
static int32_t stepDownQ24 = ENV_ONE / 6615;         //  150 ms default

static inline int32_t db10ToQ16(int16_t db10) {
  //  Clamped hard on the way up. Makeup gain that wraps around into distortion
  //  is worse than makeup gain that is a little short, and +30 dB is far past
  //  the radio's compiled +8.
  if (db10 >  300) db10 =  300;
  if (db10 < -600) db10 = -600;
  return (int32_t)lrintf(powf(10.0f, db10 / 200.0f) * 65536.0f);
}

static inline int32_t clampMul(int64_t v) {
  if (v >  2147483647LL) { clipFlag = true; return  2147483647; }
  if (v < -2147483648LL) { clipFlag = true; return -2147483648LL; }
  return (int32_t)v;
}

static inline int16_t toDb10(int32_t peak) {
  if (peak <= 0) return -1200;
  float f = (float)peak / 2147483648.0f;
  float d = 20.0f * log10f(f);
  if (d < -120.0f) d = -120.0f;
  return (int16_t)lrintf(d * 10.0f);
}

void pushBt(const uint8_t *data, uint32_t len) {
  const Frame16 *in = (const Frame16 *)data;
  size_t n = len / sizeof(Frame16);
  size_t head = ringHead;
  for (size_t i = 0; i < n; i++) {
    size_t next = (head + 1) % RING_FRAMES;
    if (next == ringTail) break;          // full: drop rather than block the
    ring[head] = in[i];                   // Bluetooth task, which must not stall
    head = next;
  }
  //  RELEASE. The sample stores above are plain writes and `volatile` orders
  //  nothing but other volatiles, so the compiler is entitled to sink them past
  //  this publication. The LX6 is in-order and `ring` is uncached internal
  //  DRAM, so it has never bitten - correct by luck is not correct.
  __atomic_store_n((size_t *)&ringHead, head, __ATOMIC_RELEASE);
  uint32_t fill = (head + RING_FRAMES - ringTail) % RING_FRAMES;
  if (fill > ringPeak) ringPeak = fill;
}

static size_t popBt(Frame16 *out, size_t want) {
  size_t tail = ringTail, got = 0;
  //  ACQUIRE, pairing with the release in pushBt().
  size_t head = __atomic_load_n((size_t *)&ringHead, __ATOMIC_ACQUIRE);
  while (got < want && tail != head) {
    out[got++] = ring[tail];
    tail = (tail + 1) % RING_FRAMES;
  }
  //  RELEASE, mirroring pushBt(). The reasoning that put a release on the
  //  head applies verbatim here and was simply not carried across: `ring` is
  //  not volatile, so the LOADS above may be sunk past a plain store to a
  //  volatile index, and the producer is then free to overwrite slots this
  //  loop has not actually read yet. In-order LX6 hides it today; that is the
  //  "correct by luck" the head comment already refuses to accept.
  __atomic_store_n((size_t *)&ringTail, tail, __ATOMIC_RELEASE);
  if (got < want) {
    //  ONLY A PARTIAL FILL IS A DROPOUT.
    //  An entirely empty ring means nothing is streaming - paused, muted at
    //  the source, or no phone - and counting that made the number useless:
    //  it free-ran at a few hundred a second whenever the selector sat on BT,
    //  so it could not distinguish "idle" from "breaking up". A PARTIAL fill
    //  is the real event: audio was flowing, the buffer ran out mid-block, and
    //  the tail was zero-filled. That zero-fill is the discontinuity, which is
    //  the click.
    if (got > 0) underruns++;
    memset(out + got, 0, (want - got) * sizeof(Frame16));
  }
  return got;
}

// ---------------------------------------------------------------------------
//  THE AUDIO TASK
//  Pinned to core 1. Core 0 belongs to the Bluetooth stack, and sharing a core
//  between SBC decode and a hard-real-time write loop is asking for dropouts.
//
//  Pacing is free: on RADIO, i2s_read blocks until a block has arrived; on
//  BT and AUX, i2s_write blocks when the DMA chain is full. There is no
//  vTaskDelay anywhere in this loop and there must not be one.
// ---------------------------------------------------------------------------
static void audioTask(void *) {
  static int32_t  inBuf[BLOCK * 2];
  static int32_t  outBuf[BLOCK * 2];
  static Frame16  btBuf[BLOCK];
  static uint32_t lastWriteUs = 0;      // see the stall detector in emit
  size_t got = 0;

  for (;;) {
    uint8_t src = curSrc;

    //  A pending source change ramps out, switches at silence, then ramps in.
    if (pendSrc != curSrc && muteOnChange) {
      envTarget = 0;
      if (envQ24 == 0) { curSrc = pendSrc; src = curSrc; }
    } else if (pendSrc != curSrc) {
      curSrc = pendSrc;
      src = curSrc;
    } else {
      envTarget = muted ? 0 : ENV_ONE;
    }

    int32_t lpk = 0, rpk = 0;

    if (src == SRC_RADIO) {
      //  Blocks until a full block has been captured - this is what paces the
      //  whole loop when the radio is the source.
      i2s_read(PORT, inBuf, sizeof(inBuf), &got, portMAX_DELAY);
      size_t frames = got / (2 * sizeof(int32_t));
      int32_t g = gainRadioQ16;
      for (size_t i = 0; i < frames; i++) {
        //  Slot 0 is RIGHT on this machine (audio.h). With the same signal in
        //  both slots, summing to mono buys about 3 dB: the signal is
        //  correlated and the converter noise is not.
        int32_t a = inBuf[i * 2], b = inBuf[i * 2 + 1];
        int32_t m = monoSum ? (int32_t)(((int64_t)a + b) / 2) : a;
        int32_t s = clampMul(((int64_t)m * g) >> 16);
        outBuf[i * 2]     = s;   // right
        outBuf[i * 2 + 1] = monoSum ? s : clampMul(((int64_t)b * g) >> 16);
      }
      if (frames == 0) continue;
      goto emit;
    }

    if (src == SRC_BT) {
      popBt(btBuf, BLOCK);
      int32_t g = gainBtQ16;
      for (size_t i = 0; i < BLOCK; i++) {
        outBuf[i * 2]     = clampMul(((int64_t)((int32_t)btBuf[i].r << 16) * g) >> 16);
        outBuf[i * 2 + 1] = clampMul(((int64_t)((int32_t)btBuf[i].l << 16) * g) >> 16);
      }
      got = BLOCK * 2 * sizeof(int32_t);
      goto emit;
    }

    //  AUX: the analogue path bypasses us entirely and the amp sums its three
    //  inputs (Hardware Bible §22, §30), so our only job is to be silent.
    memset(outBuf, 0, sizeof(outBuf));
    got = BLOCK * 2 * sizeof(int32_t);

  emit: {
      size_t frames = got / (2 * sizeof(int32_t));
      //  READ ONCE. Three separate reads of a flag another task can flip meant
      //  one block could go out with the DC word but not the tail, or the
      //  reverse.
      const bool dcNow = testDc;

      //  Volume, balance and the ramp envelope, all in one pass.
      //  THE VOLUME LAW.
      //
      //  The ear hears ratios, so a fader that steps the AMPLITUDE evenly does
      //  almost nothing over its top half and everything in the last few
      //  degrees. gain = (v/255)^gamma spends the travel where it can be heard.
      //  gamma is a setting, not a constant, because the correct value depends
      //  on the amplifier downstream and is decided by ear.
      //
      //  powf() is affordable here ONLY because this is computed once per BLOCK
      //  and cached until something changes - never per sample.
      uint8_t v = volume, t = taperX10;
      if (v != volCacheV || t != volCacheT) {
        //  THE ONE PLACE THE APPLIED GAIN CHANGES, AND IT DOES NOT RAMP.
        //  The envelope beside it moves per SAMPLE, in Q24, precisely so that a
        //  level change is not a step (see ENV_ONE and the jumpscare note). This
        //  cache does not: the new gain lands whole on the first sample of the
        //  next block. That was harmless while the only thing moving it was a
        //  hand on the knob. It stops being harmless if the READING moves on its
        //  own - and on 2026-09-10 the portal log showed it doing exactly that,
        //  wandering 17..20 with nobody touching anything.
        //
        //  So count the steps and record how far the gain actually travelled.
        //  volQ16Min/Max bound the excursion of the number that multiplies the
        //  audio, which is the only version of this question that matters: a
        //  reading that glitches but never reaches the multiply is not a pop,
        //  and a gain that reaches 65536 while the knob sits near 87 is.
        bool firstEver = (volCacheV == 0xFF);
        volCacheV = v; volCacheT = t;
        float g = t / 10.0f;
        if (g < 1.0f) g = 1.0f;
        if (g > 4.0f) g = 4.0f;
        volCacheQ16 = (int32_t)(powf((float)v / 255.0f, g) * 65536.0f + 0.5f);
        //  The very first computation is not a STEP - there was nothing before
        //  it to step from - so it is excluded from the count but still bounds
        //  the range.
        if (!firstEver) gainSteps++;
        uint32_t q = (uint32_t)volCacheQ16;
        if (q < volQ16Min) volQ16Min = q;
        if (q > volQ16Max) volQ16Max = q;
      }
      int32_t volQ16 = volCacheQ16;
      int32_t balR = 65536, balL = 65536;
      int16_t bal = balance;
      if (bal > 0) balL = 65536 - (int32_t)bal * 655;                // toward right
      if (bal < 0) balR = 65536 + (int32_t)bal * 655;                // toward left

      uint64_t accL = 0, accR = 0;
      for (size_t i = 0; i < frames; i++) {
        if (envQ24 < envTarget)      envQ24 = (envQ24 + stepUpQ24   > envTarget) ? envTarget : envQ24 + stepUpQ24;
        else if (envQ24 > envTarget) envQ24 = (envQ24 - stepDownQ24 < envTarget) ? envTarget : envQ24 - stepDownQ24;

        //  SQUARED. A linear amplitude ramp does not sound like a fade - it
        //  rushes up and then sits there, for the same reason a linear LED ramp
        //  does not look like a breath. Squaring spends the time where the ear
        //  can hear it.
        int32_t envLin = envQ24 >> 8;                       // Q24 -> Q16
        int32_t envSq  = (int32_t)(((int64_t)envLin * envLin) >> 16);
        int64_t e = ((int64_t)envSq * volQ16) >> 16;
        //  Squared here, before the volume and the envelope touch the sample.
        int32_t sr = outBuf[i * 2] >> 16, sl = outBuf[i * 2 + 1] >> 16;
        accR += (uint64_t)((int64_t)sr * sr);
        accL += (uint64_t)((int64_t)sl * sl);
        int32_t r = clampMul((((int64_t)outBuf[i * 2]     * e) >> 16) * balR >> 16);
        int32_t l = clampMul((((int64_t)outBuf[i * 2 + 1] * e) >> 16) * balL >> 16);
        outBuf[i * 2]     = r;
        outBuf[i * 2 + 1] = l;
        //  INT64. `-(int64_t)INT32_MIN` is +2147483648, which truncated back
        //  into an int32_t is INT32_MIN again - negative - so the comparison
        //  below failed and THE LOUDEST POSSIBLE SAMPLE NEVER REGISTERED A
        //  PEAK. The meter under-read precisely during a full-scale event,
        //  which is the one moment it exists for.
        int64_t ar = r < 0 ? -(int64_t)r : (int64_t)r;
        int64_t al = l < 0 ? -(int64_t)l : (int64_t)l;
        if (ar > (int64_t)rpk) rpk = (int32_t)(ar > 2147483647LL ? 2147483647LL : ar);
        if (al > (int64_t)lpk) lpk = (int32_t)(al > 2147483647LL ? 2147483647LL : al);
      }

      if (lpk > peakL) peakL = lpk;
      if (rpk > peakR) peakR = rpk;
      portENTER_CRITICAL(&rmsMux);
      rmsSumL += accL; rmsSumR += accR; rmsN += (uint32_t)frames;
      portEXIT_CRITICAL(&rmsMux);

      size_t wrote = 0;
      //  After everything, so neither the volume nor the envelope can scale it.
      if (dcNow) {
        size_t words = got / sizeof(int32_t);
        for (size_t i = 0; i < words; i++) outBuf[i] = dcWord;
      }

      //  AFTER THE SAMPLES, so it covers RADIO, BT, AUX and the DC diagnostic
      //  alike. NO LONGER THE LAST WRITE: the zero-data floor and the sign-
      //  extended tail both run after it and both set low bits again (bit 8,
      //  and bits 7..0). "The bits that go out are the bits this mask allows"
      //  was true when it was written and is not true with either of those on.
      if (outMask != (int32_t)0xFFFFFFFF) {
        size_t mw = got / sizeof(int32_t);
        for (size_t i = 0; i < mw; i++) outBuf[i] &= outMask;
      }

      //  THE ZERO-DATA FLOOR - built as the zero-data theory's fix, default OFF.
      //  The theory is excluded (see THE ZERO-DATA WATCH); the floor stays as
      //  a switch (sys.zfloor). Every word the DAC would read as zero is
      //  lifted to one 24-bit LSB, so the part's zero-data detector can never
      //  arm and the analogue mute can never release. -138.5 dBFS of DC, which
      //  is 26 dB below the part's own 112 dB dynamic range: inaudible by
      //  construction.
      //
      //  It touches ONLY the words that would otherwise be zero. An OR across
      //  every sample would have dithered the whole programme by an LSB for no
      //  reason; the comparison is against the same 24-bit view dacZero() uses,
      //  so nothing the DAC can actually hear is modified.
      //
      //  AFTER the mask, deliberately: a 12- or 16-bit mask would otherwise
      //  clear the floor again and the experiment would silently do nothing.
      //  NOT while the DC test runs - that test's whole value is that the word
      //  on the wire is exactly the word that was asked for.
      if (zeroFloor && !dcNow) {
        size_t fw = got / sizeof(int32_t);
        for (size_t i = 0; i < fw; i++)
          if ((uint32_t)outBuf[i] < 256u) outBuf[i] = ZDD_LSB;
      }

      //  THE SIGN-EXTENDED TAIL - tested 2026-09-11, and it is NOT the fix.
      //  Default OFF.
      //
      //  RESULT FIRST. A blind heat-gun A-B-A-B with Device 3 counting pops: with
      //  comparable gun firing in every window, tail off and tail on gave the same
      //  pop rate and the same size of pop, and the rate did not rebound when the
      //  tail went back off. Kept, off, as the record of a mechanism ruled out -
      //  removing the DIN edge at the word boundary does not stop the fault.
      //
      //  WHY IT WAS TRIED. The 2026-09-01 constant-DC words pointed at a DIN
      //  transition at the frame boundary: static across it, silent; a
      //  transition one bit-time before it, a few pops; a transition across it,
      //  many. And on 2026-09-10, with Device 3 as a pop counter, the A32 muted
      //  with the zero-data floor on - the DAC's output stage held alive on a
      //  CONSTANT word - gave zero pops in three minutes against 3.7/min
      //  playing. The reading was that the pop needs the data on DIN to change;
      //  the next evening a constant 0xFF000000 popped as well.
      //
      //  WHAT THIS DOES. Bits 7..0 of every 32-bit word are ours. SLAS859C never
      //  says whether the part keeps 24 or 32 bits at BCK = 64 fs, so they are
      //  either discarded or weighted at -138.5 dBFS - under one 24-bit LSB and
      //  26 dB below the part's 112 dB dynamic range. Inaudible either way. Set them
      //  to the sign of the NEXT word on the wire. The final eight bit-times of
      //  every slot then already sit at the level the next slot's MSB will take,
      //  and DIN does not move across the frame boundary whatever the music does.
      //  At low levels the next word's upper bits are all sign-extension as well,
      //  so the static window straddling the edge is far wider than eight.
      //
      //  WHAT IT LEAVES. The tail removes the edge ACROSS the boundary, but the
      //  audio's own LSB (bit 8) against the tail (bit 7) can still flip: eight
      //  bit-times before the end of the slot, seven before the WS edge under
      //  STAND_I2S's one-bit shift.
      //
      //  WHY IT CANNOT DO WHAT sd_out_delay DID. That register moved DIN in TIME
      //  against BCK and the DAC latched the wrong bits - full volume through the
      //  author's amplifier, 2026-09-10. This moves nothing in time and touches no
      //  bit the DAC plays at any audible weight. An all-zero stream stays all-zero
      //  (a zero successor gives a zero tail), so AUX and mute are untouched and
      //  the zero-data witness, which looks only at bits 31..8, reads the same.
      //
      //  ORDER. After the mask, which would otherwise clear it; after the floor,
      //  whose bit 8 it does not touch. And DELIBERATELY UNDER THE DC TEST:
      //  an earlier version excluded it, and testing showed what that cost. The proven
      //  popper 0xFF000000 becomes 0xFF0000FF with the tail on -
      //  the same word with its boundary made static, nothing else changed. That
      //  is the cleanest isolation test there is, and excluding it also made the
      //  published `tail` flag lie about what was actually applied.
      //
      //  THE BLOCK'S LAST WORD has no successor yet - the next block does not
      //  exist - so it takes its own sign. That is wrong exactly when the sign
      //  changes across a block boundary: about 172 boundaries a second, and with
      //  monoSum both slots of a frame are equal, so only at a zero crossing.
      //
      //  KNOWN SIDE EFFECT: a word of 1..255 whose successor is not
      //  negative becomes EXACT zero. Nothing changes in the 24-bit view; if the
      //  part's zero-data detector counts all 32 bits, a one-sided sub-LSB passage
      //  could now arm its analogue mute. Unlikely, and it would ADD pops - so it
      //  can only make this change look worse than it is, never better.
      if (signTail) {
        size_t tw = got / sizeof(int32_t);
        for (size_t i = 0; i < tw; i++) {
          int32_t nxt = (i + 1 < tw) ? outBuf[i + 1] : outBuf[i];
          outBuf[i] = (outBuf[i] & (int32_t)0xFFFFFF00) | ((nxt < 0) ? 0xFF : 0x00);
        }
      }

      //  THE WITNESS. Runs on what is about to go on the wire, after the mask
      //  and after the floor, so it reports the machine's actual output and not
      //  its intent - the standing rule that a comment is evidence about intent
      //  and never about behaviour applies to instruments too.
      {
        uint32_t run = zddRun, ms = millis();
        for (size_t i = 0; i < frames; i++) {
          if (dacZero(outBuf[i * 2]) && dacZero(outBuf[i * 2 + 1])) {
            //  Saturates at the threshold rather than free-running: a counter
            //  that wraps would re-arm every 27 hours of silence and invent an
            //  event out of nothing.
            if (run < ZDD_FRAMES && ++run == ZDD_FRAMES) {
              zddMuted = true; zddArm++; zddArmMs = ms;
            }
          } else {
            if (zddMuted) {
              uint32_t held = ms - zddArmMs;
              if (held > zddLongest) zddLongest = held;
              zddMuted = false; zddRel++; zddRelMs = ms;
            }
            run = 0;
          }
        }
        zddRun = run;
      }

      //  THE PERIPHERAL'S OWN ERROR LATCHES, accumulated and NEVER CLEARED.
      //  Masked against int_ena for the reason i2sFaults() documents at length:
      //  the bits the driver services are not ours to touch. Sticky because the
      //  console 's' path is read-and-reset and therefore destroys the portal's
      //  copy - the same instrument trap that cost a 40-minute session on
      //  2026-09-01 with getPeaks(). This one nobody can drain.
      i2sSticky |= (uint16_t)(I2S0.int_raw.val & ~I2S0.int_ena.val);

      //  A STALL: the audio task kept off the CPU for more than one ZDD window.
      //  tx_desc_auto_clear (see begin()) zeroes each DMA buffer once it has
      //  been transmitted, so a frozen audio task does not repeat stale audio:
      //  once the ~46 ms chain runs dry it emits exact zero, a level step at the
      //  DAC, and past ~46 + 23 ms that silence alone could arm the part's
      //  detector. A flash erase with the cache off freezes both cores and does
      //  not care that this task is priority 6. Counted separately from zddArm
      //  so that "the DAC muted" and "the CPU stopped" are two different answers.
      {
        uint32_t nowUs = micros();
        if (lastWriteUs) {
          uint32_t gap = nowUs - lastWriteUs;
          if (gap > ZDD_STALL_US) {
            zddStalls++;
            if (gap > zddStallUs) zddStallUs = gap;
            zddStallLastUs = gap;
            zddStallLastMs = millis();
          }
        }
        lastWriteUs = nowUs;
      }

      i2s_write(PORT, outBuf, got, &wrote, portMAX_DELAY);
    }
  }
}

void begin() {
  i2s_config_t cfg = {};
  cfg.mode                 = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX | I2S_MODE_RX);
  cfg.sample_rate          = SAMPLE_RATE;
  cfg.bits_per_sample      = I2S_BITS_PER_SAMPLE_32BIT;   // PCM1802 is 24-bit in 32-bit slots
  cfg.channel_format       = I2S_CHANNEL_FMT_RIGHT_LEFT;
  cfg.communication_format = I2S_COMM_FORMAT_STAND_I2S;
  cfg.intr_alloc_flags     = ESP_INTR_FLAG_LEVEL1;
  //  EIGHT BUFFERS: 8 * 256 / 44100 = 46 ms of output.
  //  For one evening, 2026-09-11, this was 16 (93 ms), to ride out the audio-
  //  task stalls the detector in the emit block had measured (23 to 111 ms that
  //  day): a stall that drains the chain lets tx_desc_auto_clear put EXACT ZEROS
  //  on the wire, and the output leaving its level and coming back is a click.
  //  It cost +32 KB of internal DMA-capable RAM (TX and RX each got eight more
  //  buffers) and the A32 then refused EVERY over-the-air update: Update.begin()
  //  failed its malloc(4096) and returned false with error 0 (Updater.cpp), on
  //  a machine seconds out of reset. Bluetooth stopped accepting connections in
  //  the same window. Back to 8 the same evening: a radio that cannot be updated
  //  without opening the cabinet is worse than one that clicks when the audio
  //  task is starved. Depth costs the same DMA RAM per millisecond whether it is
  //  bought with the count or with dma_buf_len, so any increase must first be
  //  shown to fit (heap_caps_get_largest_free_block(MALLOC_CAP_DMA)).
  cfg.dma_buf_count        = 8;
  cfg.dma_buf_len          = 256;
  //  APLL ON, and do NOT "try" turning it off. The MCLK on GPIO0 feeds BOTH
  //  converters' SCK (Hardware Bible §3; the DAC's since 2026-09-02), and the
  //  PCM5102A's internal PLL is disabled once an external SCK is supplied
  //  (SLAS859C 9.3.5.3). The APLL is what makes 11.2896 MHz exactly, and BCK and
  //  LRCK are divided from that same clock, so SCK:LRCK is 256 by construction.
  //  Without it the divider runs off PLL_D2 at 160 MHz and cannot make 44100
  //  exactly - a worse clock for BOTH parts.
  cfg.use_apll             = true;
  cfg.tx_desc_auto_clear   = true;
  //  The master clock both converters take from GPIO0. With use_apll above, the
  //  driver derives it from the APLL - not from the integer divider that an
  //  older version of this comment claimed.
  cfg.fixed_mclk           = SAMPLE_RATE * 256;           // 11.2896 MHz

  //  NOTHING DRIVES XSMT, AND NOTHING CAN. Until 2026-09-23 this block set
  //  GPIO16 HIGH, believing it was the PCM5102A's soft-mute input and that a
  //  floating XSMT caused the pops. No A32 pin reaches XSMT: the DAC module
  //  pulls it up itself and GPIO16 is unused (Hardware Bible §6). That write
  //  never reached the DAC, so it was removed. Mute is
  //  software only - see audio.h.

  if (i2s_driver_install(PORT, &cfg, 0, NULL) != ESP_OK) {
    Serial.println(F("  [FAIL] i2s_driver_install"));
    return;
  }
  i2s_pin_config_t pins = {};
  //  MCLK is FORCED to GPIO0 by the silicon: the classic ESP32 can only emit
  //  the I2S master clock on GPIO0, 1 or 3, and 1/3 are the console. BOTH
  //  converters take it (Hardware Bible §3). Never detach GPIO0 from the I2S
  //  peripheral: that is a clock error at the DAC, not merely a silent ADC.
  //  See setAdcClock()'s tombstone below.
  pins.mck_io_num   = A32_PCM1802_MCLK;
  pins.bck_io_num   = A32_I2S_BCK;
  pins.ws_io_num    = A32_I2S_LRCK;
  pins.data_out_num = A32_PCM5102_DIN;
  pins.data_in_num  = A32_PCM1802_DOUT;
  if (i2s_set_pin(PORT, &pins) != ESP_OK) {
    Serial.println(F("  [FAIL] i2s_set_pin"));
    return;
  }

  //  CLOCK PADS AT THE STRONGEST DRIVE, FROM BOOT - measured 2026-09-11:
  //  one-minute alternation, 30 windows, a steady tone over A2DP and Device 3
  //  counting: 52 isolated pops at drive 2 (3.33/min) against 0 at drive 3,
  //  stratified exact p = 2.8e-16, all 15 pairs one-sided - about 150x fewer.
  //  A margin fix, not a root cause, which is still open. The reading behind
  //  it, never seen on a scope: GPIO0 feeds two converters (Hardware Bible
  //  §3) and the DAC's BCK-recovery PLL is off while SCK is present (SLAS859C
  //  9.3.5.3), so the edge into those loads decides whether the DAC latches
  //  the right bit. The author: "Drive 3 works with no seeming
  //  consequences." setClockDrive() clamps to 2..3 and reads back from all
  //  three pads; the remote sys.clkdrv knob still works, so this can be A/B-ed
  //  again - or put back - at any time without a reflash.
  setClockDrive(3);
  Serial.printf("  [PASS] clock pads at drive %u (2 was the default until 09-11).\n",
                (unsigned)clockDriveIs());
  Serial.println(F("  [PASS] I2S installed once: full duplex, 44.1 kHz, MCLK on GPIO0."));
  Serial.println(F("         It is never reinstalled - source switching does not touch it."));

  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 6, nullptr, 1);
}

void setSource(uint8_t src)          { pendSrc = src; }
uint8_t source()                     { return curSrc; }
void setVolume(uint8_t v)            { volume = v; }
void setTaper(uint8_t g)             { taperX10 = (g < 10) ? 10 : (g > 40 ? 40 : g); }
void setTestDc(bool on)              { testDc = on; }
void setDcWord(int32_t w)            { dcWord = w; }
int32_t dcWordIs()                   { return dcWord; }

//  bits = how many LOW bits to force to zero, 0..24.
void setOutMaskBits(uint8_t bits) {
  if (bits > 24) bits = 24;
  outMask = (bits == 0) ? (int32_t)0xFFFFFFFF
                        : (int32_t)(0xFFFFFFFFu << bits);
}
int32_t outMaskIs() { return outMask; }

// ---------------------------------------------------------------------------
//  THE TWO KNOBS - AND sd_out_delay IS NOT FREE. Measured 2026-09-10 with
//  Device 3 counting pops: of its four positions only 0 plays correctly. 1 and 3
//  silenced the audio outright, and 2 sent the radio to FULL VOLUME through the
//  author's amplifier. It moves DIN in time against BCK and the DAC latches the
//  wrong bits. Its remote path was removed on 2026-09-11; the function remains
//  only for the A32's own USB console. NEVER write it with the audio path live.
//  Both knobs were built for a mechanism since EXCLUDED: that a DIN transition
//  on the LRCK edge tripped the PCM5102A's clock-error protection and that
//  jump was the pop. Through 37 recorded pops (2026-09-11) a streamed tone
//  passed with a median -0.3 dB change, which a clock-error mute cannot do.
//
//  Masking the low bits could not have fixed it either: every slot opens with
//  the SIGN BIT of the next sample, so music supplies a boundary transition on
//  every negative sample no matter what the trailing bits do.
//
//  Both readbacks come from the hardware, never from a shadow copy - reading
//  back what we THINK we wrote is how a sweep lies to you.
// ---------------------------------------------------------------------------

//  WEAKEN THE AGGRESSOR ONLY - the hypothesis it was built for: DIN is what
//  couples; BCK and LRCK are what gets upset, and a victim wants FAST edges.
//  So this touches data_out and nothing else. No result recorded that it
//  helped. 0 is weakest (~5 mA), 3 strongest (~40 mA); 2 is the chip's
//  power-on default, and nothing sets it at boot.
void setDinDrive(uint8_t cap) {
  if (cap > 3) cap = 3;
  gpio_set_drive_capability((gpio_num_t)A32_PCM5102_DIN, (gpio_drive_cap_t)cap);
}
uint8_t dinDriveIs() {
  gpio_drive_cap_t c = GPIO_DRIVE_CAP_DEFAULT;
  gpio_get_drive_capability((gpio_num_t)A32_PCM5102_DIN, &c);
  return (uint8_t)c;
}

// ---------------------------------------------------------------------------
//  THE CLOCK PINS' DRIVE - 2026-09-11.
//
//  WHY. The sign-extended tail (the DATA side) was tested blind with the heat
//  gun and did not move the pop rate. The reading that survived is that the DAC
//  loses its framing through its CLOCKS - GPIO0 feeds MCLK to two loads, the
//  PCM1802 and the DAC's SCK (Hardware Bible §3), and the DAC's BCK-recovery
//  PLL, the fallback, is disabled once SCK is supplied (SLAS859C 9.3.5.3).
//  Stiffer victims - faster, lower-impedance edges - are the cheapest direct
//  test of that.
//
//  WHY IT CANNOT STOP MCLK - checked in the installed IDF 4.4 headers first.
//  GPIO0 and GPIO4 are RTC-capable pads, and soc_caps.h says their drive "must"
//  be set through the RTC register; gpio_set_drive_capability does exactly that,
//  via rtcio_ll_set_drive_capability, which writes only the pad's drv field.
//  The pad's mux (digital vs RTC) is a separate bit set only by
//  rtcio_ll_function_select, so this cannot take GPIO0 away from the I2S clock
//  output. GPIO17/18 are plain IO_MUX pads: gpio_ll_set_drive_capability
//  writes only FUN_DRV.
//
//  2 is the chip's power-on default; begin() sets 3 at every boot. ONLY 2 AND 3
//  ARE ACCEPTED: a weaker clock edge driving two converters could mis-clock
//  both for long stretches. A remote change is not persisted - a reboot is
//  back to 3.
void setClockDrive(uint8_t cap) { setClockDriveSplit(cap, cap); }

//  MCLK AND THE FRAME PAIR MOVE SEPARATELY - 2026-09-13. Drive 3 everywhere cut
//  the pops (52 -> 0 over 30 alternating windows, p = 2.8e-16) and that day was
//  heard to distort the ADC path; drive 2 everywhere did the reverse. Both
//  converters see all three pads - the PCM1802 is a SLAVE and takes BCK and
//  LRCK too - so which pad the ADC objects to is unknown, and guessing it costs
//  a soldering iron. This makes it a ten-minute listening test instead. The
//  author has since ruled drive 3 fine as it is: "Drive 3 works
//  with no seeming consequences."
void setClockDriveSplit(uint8_t mclkCap, uint8_t frameCap) {
  if (mclkCap  < 2) mclkCap  = 2;
  if (mclkCap  > 3) mclkCap  = 3;
  if (frameCap < 2) frameCap = 2;
  if (frameCap > 3) frameCap = 3;
  gpio_set_drive_capability((gpio_num_t)A32_PCM1802_MCLK, (gpio_drive_cap_t)mclkCap);
  gpio_set_drive_capability((gpio_num_t)A32_I2S_BCK,      (gpio_drive_cap_t)frameCap);
  gpio_set_drive_capability((gpio_num_t)A32_I2S_LRCK,     (gpio_drive_cap_t)frameCap);
  Serial.printf("  clock drive: MCLK %u, BCK/LRCK %u (read back %u / %u / %u)\n",
                (unsigned)mclkCap, (unsigned)frameCap,
                (unsigned)mclkDriveIs(), (unsigned)bckDriveIs(), (unsigned)lrckDriveIs());
}

//  EVERY READBACK COMES FROM THE PAD, never a shadow copy - the rule the DIN and
//  clock drives have followed since a sweep lied about what it had written.
static uint8_t padDrive(int pin) {
  gpio_drive_cap_t c = GPIO_DRIVE_CAP_DEFAULT;
  gpio_get_drive_capability((gpio_num_t)pin, &c);
  return (uint8_t)c;
}
uint8_t mclkDriveIs() { return padDrive(A32_PCM1802_MCLK); }
uint8_t bckDriveIs()  { return padDrive(A32_I2S_BCK); }
uint8_t lrckDriveIs() { return padDrive(A32_I2S_LRCK); }

//  READ BACK FROM ALL THREE PADS, and 0xFF if they disagree. Reporting one
//  pin's value while another differs is the lying instrument this project has
//  refused every time it has turned up.
uint8_t clockDriveIs() {
  gpio_drive_cap_t a = GPIO_DRIVE_CAP_DEFAULT, b = a, c = a;
  gpio_get_drive_capability((gpio_num_t)A32_PCM1802_MCLK, &a);
  gpio_get_drive_capability((gpio_num_t)A32_I2S_BCK,      &b);
  gpio_get_drive_capability((gpio_num_t)A32_I2S_LRCK,     &c);
  return (a == b && b == c) ? (uint8_t)a : 0xFF;
}

//  tx_sd_out_delay shifts the serial data output relative to the clocks. ONLY 0
//  PLAYS CORRECTLY - see THE TWO KNOBS above. Read back only: the setter and
//  its console key were removed 2026-09-25 (position 2 meant full volume).
uint8_t sdOutDelayIs() { return (uint8_t)I2S0.timing.tx_sd_out_delay; }

//  setAdcClock() WAS HERE AND MUST NOT COME BACK.
//
//  It detached GPIO0 from the I2S peripheral to stop the PCM1802's master
//  clock. That was a useful diagnostic while GPIO0 fed the ADC alone. It was
//  removed on 2026-09-01, ahead of the DAC's SCK moving to the same pin on
//  2026-09-02: the PCM5102A takes its system clock from GPIO0 too (Hardware
//  Bible §3), so cutting it is no longer "silence the radio", it is "clock
//  error at the DAC".
//
//  If the ADC ever needs silencing again, do it by muting its samples, not
//  by taking away a clock two devices now share.
bool testDcOn()                      { return testDc; }
void setMute(bool m)                 { muted = m; }
void setBalance(int16_t b)           { balance = constrain(b, -100, 100); }
void setMonoSum(bool on)             { monoSum = on; }
void setMuteOnChange(bool on)        { muteOnChange = on; }
void setGains(int16_t r, int16_t b)  { gainRadioQ16 = db10ToQ16(r); gainBtQ16 = db10ToQ16(b); }

void setFades(uint16_t inMs, uint16_t outMs) {
  //  At least one step, or the envelope would never move and the source change
  //  would deadlock waiting for silence that never arrives.
  uint32_t fin  = ((uint32_t)(inMs  ? inMs  : 1) * SAMPLE_RATE) / 1000;
  uint32_t fout = ((uint32_t)(outMs ? outMs : 1) * SAMPLE_RATE) / 1000;
  stepUpQ24   = fin  ? (int32_t)(ENV_ONE / fin)  : ENV_ONE;
  stepDownQ24 = fout ? (int32_t)(ENV_ONE / fout) : ENV_ONE;
  if (stepUpQ24   < 1) stepUpQ24   = 1;
  if (stepDownQ24 < 1) stepDownQ24 = 1;
}

static int16_t rmsDb10(uint64_t sum, uint32_t n) {
  if (!n || !sum) return -1200;
  double d = 10.0 * log10(((double)sum / (double)n) / (32768.0 * 32768.0));
  if (d < -120.0) d = -120.0;
  return (int16_t)lrint(d * 10.0);
}

void getRms(int16_t &l, int16_t &r) {
  portENTER_CRITICAL(&rmsMux);
  uint64_t sl = rmsSumL, sr = rmsSumR; uint32_t n = rmsN;
  rmsSumL = 0; rmsSumR = 0; rmsN = 0;
  portEXIT_CRITICAL(&rmsMux);
  l = rmsDb10(sl, n); r = rmsDb10(sr, n);
}

void getPeaks(int16_t &l, int16_t &r, bool &clipped) {
  l = toDb10(peakL); r = toDb10(peakR); clipped = clipFlag;
  peakL = 0; peakR = 0; clipFlag = false;
}

uint32_t btUnderruns() { return underruns; }
//  THE PERIPHERAL'S OWN ERROR LATCHES, which nothing has ever read.
//  I2S0.int_raw latches tx_hung, tx_rempty, rx_wfull and the descriptor errors.
//  The driver enables and services only EOF and DSCR_ERR, so these bits have
//  been accumulating in the silicon untouched. With no oscilloscope in the
//  building they are the only direct evidence available about what the TX path
//  is actually doing. Read and cleared, so a value is "since you last looked".
uint32_t i2sFaults() {
  uint32_t v = I2S0.int_raw.val;
  //  CLEAR ONLY WHAT THE DRIVER IS NOT USING. The first version wrote
  //  0xFFFFFFFF, which also cleared out_eof / in_suc_eof / the descriptor
  //  errors - the very latches i2s_intr_handler_default() reads and services.
  //  Land that write between the hardware asserting out_eof and the ISR
  //  reading int_st and the ISR sees nothing, so it never returns that DMA
  //  buffer to the free queue. Lose all eight and i2s_write(..., portMAX_DELAY)
  //  blocks forever: the audio task stops with no way back but a reboot.
  //  Reachable from the console 's' key, which is pressed hundreds of times
  //  in a pop-hunting session - it was pressed about 360 times on 2026-09-01
  //  and got away with it.
  //
  //  int_ena is the driver's own declaration of what it services, so masking
  //  against it is correct by construction rather than by a hand-kept list.
  I2S0.int_clr.val = v & ~I2S0.int_ena.val;
  return v;
}

// ---------------------------------------------------------------------------
//  Read-only, and NOT read-and-reset. Every other counter in this file resets
//  on read and every one of them has misled somebody: getPeaks() is drained by
//  two readers at once, and i2sFaults() clears what it reports. These are
//  monotonic since boot, so two readers cannot rob each other and a number can
//  be compared against the same number taken five minutes ago.
// ---------------------------------------------------------------------------
void zeroWatch(ZeroWatch &w) {
  w.arm        = zddArm;
  w.rel        = zddRel;
  w.longestMs  = zddLongest;
  w.stalls     = zddStalls;
  w.stallMaxUs = zddStallUs;
  w.stallLastMs = zddStallLastMs;
  w.stallLastUs = zddStallLastUs;
  w.i2sSticky  = i2sSticky;
  w.muted      = zddMuted;
  w.floorOn    = zeroFloor;
  //  "Never" is 0xFFFFFFFF rather than 0, because 0 means "just now" and the
  //  difference between those two is the entire diagnostic.
  w.sinceRelMs = zddRel ? (millis() - zddRelMs) : 0xFFFFFFFFu;
}

void setZeroFloor(bool on) { zeroFloor = on; }
bool zeroFloorOn()         { return zeroFloor; }
void setSignTail(bool on)  { signTail = on; }
bool signTailOn()          { return signTail; }

void gainWatch(uint32_t &steps, uint32_t &q16min, uint32_t &q16max) {
  steps  = gainSteps;
  //  0xFFFFFFFF means "the gain has never been computed", which cannot happen
  //  once the audio task has run one block - but reporting it as a huge number
  //  would read as a full-scale excursion, which is the opposite of the truth.
  q16min = (volQ16Min == 0xFFFFFFFFu) ? 0 : volQ16Min;
  q16max = volQ16Max;
}

uint32_t btRingFill() {
  //  The high-water mark since the last call, then reset - the same contract
  //  as getPeaks(), and for the same reason.
  uint32_t p = ringPeak;
  ringPeak = (ringHead + RING_FRAMES - ringTail) % RING_FRAMES;
  return p;
}

}  // namespace Audio
