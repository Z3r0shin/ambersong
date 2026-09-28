#include <Arduino.h>
#include "soc/gpio_reg.h"
#include "display.h"
#include "pins.h"

namespace Display {

//  Segments are all below GPIO32 and live in the low output register.
//  Digits are 42, 2, 40, 41 - three are above 31 and need OUT1. Splitting this
//  once, here, keeps the ISR down to a handful of register writes.
static const int SEG_PIN[7]   = { S3_SEG_A, S3_SEG_B, S3_SEG_C, S3_SEG_D,
                                  S3_SEG_E, S3_SEG_F, S3_SEG_G };
static const int DIGIT_PIN[4] = { S3_DIGIT_1, S3_DIGIT_2, S3_DIGIT_3, S3_DIGIT_4 };

static uint32_t segBit[7];
static uint32_t digitLo[4], digitHi[4];
static uint32_t segAllMask = 0;

//  bit0=A bit1=B bit2=C bit3=D bit4=E bit5=F bit6=G, standard seven-segment
//  layout: A top, B top-right, C bottom-right, D bottom, E bottom-left,
//  F top-left, G centre. Bit n drives SEG_PIN[n] (pins.h S3_SEG_A..G).
static const uint8_t FONT[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static const uint32_t TICK_US    = 25;
static const uint32_t DEAD_TICKS = 33;    // 825 us - see display.h. Never shortened.
static const uint32_t SLOT_TICKS = 100;   // 2500 us
static const uint32_t LIT_MAX    = SLOT_TICKS - DEAD_TICKS;   // 67

static hw_timer_t       *timer = nullptr;
static volatile uint32_t frameMask[4] = {0, 0, 0, 0};
//  LIT_MAX only until the first setBrightness() (setup() calls it before any
//  digit is shown): its turn-off tick never arrives, and the unconditional
//  blank at tick 99 ends the slot instead.
static volatile uint32_t litTicks = LIT_MAX;
static volatile uint8_t  bright   = 255;
static volatile uint32_t tickCount = 0;
static volatile int      curDigit  = 0;
static volatile bool     running   = false;

static volatile int64_t  lastSlotUs = 0;
static volatile uint32_t worstErrUs = 0;

// ---------------------------------------------------------------------------
//  THE ISR. IRAM_ATTR, register writes only, one call to esp_timer_get_time()
//  which is itself IRAM. It must never touch flash: bench testing showed that
//  flash bus contention is the ONE load that disturbs this display, and living
//  in IRAM is what protects it.
// ---------------------------------------------------------------------------
static void IRAM_ATTR onTick() {
  if (!running) return;

  uint32_t t = tickCount++;

  if (t == DEAD_TICKS) {
    //  The dead time has elapsed and the outgoing digit is off. Digit first,
    //  then its segments. At brightness 0 (litTicks 0) nothing is lit.
    int64_t now = esp_timer_get_time();
    if (lastSlotUs) {
      int64_t err = (now - lastSlotUs) - (int64_t)(SLOT_TICKS * TICK_US);
      if (err < 0) err = -err;
      if (err > (int64_t)worstErrUs) worstErrUs = (uint32_t)err;
    }
    lastSlotUs = now;

    if (litTicks) {
      REG_WRITE(GPIO_OUT_W1TS_REG,  digitLo[curDigit]);
      REG_WRITE(GPIO_OUT1_W1TS_REG, digitHi[curDigit]);
      REG_WRITE(GPIO_OUT_W1TS_REG,  frameMask[curDigit]);
    }
  } else if (t == DEAD_TICKS + litTicks) {
    //  End of the lit window: segments off FIRST, then the digit - the mirror
    //  of the turn-on order.
    REG_WRITE(GPIO_OUT_W1TC_REG,  segAllMask);
    REG_WRITE(GPIO_OUT_W1TC_REG,  digitLo[curDigit]);
    REG_WRITE(GPIO_OUT1_W1TC_REG, digitHi[curDigit]);
  }

  if (t >= SLOT_TICKS - 1) {
    //  UNCONDITIONAL BLANK AT THE SLOT BOUNDARY.
    //
    //  The fix for a real bug, 2026-08-28: at full brightness litTicks was 67,
    //  the turn-off fell on tick 100, which never arrives because the slot
    //  rolls over at 99 - the segments stayed lit and the display read 88:88.
    //  setBrightness() now also clamps litTicks inside the slot, but this blank
    //  means no future arithmetic can resurrect it: a digit CANNOT be left lit
    //  across a slot boundary.
    REG_WRITE(GPIO_OUT_W1TC_REG,  segAllMask);
    REG_WRITE(GPIO_OUT_W1TC_REG,  digitLo[curDigit]);
    REG_WRITE(GPIO_OUT1_W1TC_REG, digitHi[curDigit]);
    tickCount = 0;
    curDigit  = (curDigit + 1) & 3;
  }
}

static void allOff() {
  REG_WRITE(GPIO_OUT_W1TC_REG, segAllMask);
  for (int d = 0; d < 4; d++) {
    REG_WRITE(GPIO_OUT_W1TC_REG,  digitLo[d]);
    REG_WRITE(GPIO_OUT1_W1TC_REG, digitHi[d]);
  }
}

void begin() {
  for (int s = 0; s < 7; s++) {
    segBit[s] = 1UL << SEG_PIN[s];
    segAllMask |= segBit[s];
    pinMode(SEG_PIN[s], OUTPUT); digitalWrite(SEG_PIN[s], LOW);
  }
  for (int d = 0; d < 4; d++) {
    int p = DIGIT_PIN[d];
    digitLo[d] = (p < 32)  ? (1UL << p) : 0;
    digitHi[d] = (p >= 32) ? (1UL << (p - 32)) : 0;
    pinMode(p, OUTPUT); digitalWrite(p, LOW);
  }
  allOff();

  //  setup() runs on the Arduino loopTask, pinned to core 1, so this interrupt
  //  is allocated on core 1 - away from the WiFi stack on core 0. That split is
  //  part of why the display keeps its timing under WiFi.
  timer = timerBegin(0, 80, true);           // 80 MHz / 80 = 1 us ticks
  timerAttachInterrupt(timer, &onTick, true);
  timerAlarmWrite(timer, TICK_US, true);
  timerAlarmEnable(timer);
  running = true;
}

void setBrightness(uint8_t b) {
  bright = b;
  //  Scale the LIT window only. The slot stays 100 ticks, so the frame rate
  //  never moves and dimming cannot introduce flicker.
  uint32_t l = ((uint32_t)b * LIT_MAX) / 255;
  if (b && l == 0) l = 1;                    // never round a visible level to dark
  //  Must stay strictly inside the slot: a turn-off scheduled ON the boundary
  //  never happens, because the rollover resets the counter first. Costs 25 us
  //  of a 1675 us window - 1.5%, invisible - and makes 255 genuinely full.
  if (l > LIT_MAX - 1) l = LIT_MAX - 1;
  litTicks = l;
}

uint8_t brightness() { return bright; }

void showDigits(const int8_t d[4]) {
  uint32_t m[4];
  for (int i = 0; i < 4; i++) {
    if (d[i] < 0 || d[i] > 9) { m[i] = 0; continue; }
    uint8_t f = FONT[d[i]];
    uint32_t mask = 0;
    for (int s = 0; s < 7; s++) if ((f >> s) & 1) mask |= segBit[s];
    m[i] = mask;
  }
  noInterrupts();
  for (int i = 0; i < 4; i++) frameMask[i] = m[i];
  interrupts();
}

void showTime(uint8_t hh, uint8_t mm, bool hour12, bool blankLeadingZero) {
  if (hour12) { hh = hh % 12; if (hh == 0) hh = 12; }
  int8_t d[4];
  d[0] = mm % 10;          // DIGIT 1 IS THE RIGHTMOST
  d[1] = mm / 10;
  d[2] = hh % 10;
  d[3] = hh / 10;
  if (blankLeadingZero && d[3] == 0) d[3] = -1;
  showDigits(d);
}

void showNumber(uint16_t v, bool blankLeadingZeros) {
  int8_t d[4];
  for (int i = 0; i < 4; i++) { d[i] = v % 10; v /= 10; }
  if (blankLeadingZeros)
    for (int i = 3; i >= 1 && d[i] == 0; i--) d[i] = -1;
  showDigits(d);
}

void blank() {
  int8_t d[4] = { -1, -1, -1, -1 };
  showDigits(d);
}

uint32_t worstSlotErrorUs() { return worstErrUs; }
void     resetSlotStats()   { noInterrupts(); worstErrUs = 0; lastSlotUs = 0; interrupts(); }

}  // namespace Display
