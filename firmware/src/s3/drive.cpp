#include <Arduino.h>
#include <math.h>
#include "drive.h"
#include "console.h"
#include "pins.h"

namespace {

const int PINS[4] = { S3_STEPPER_I1, S3_STEPPER_I2, S3_STEPPER_I3, S3_STEPPER_I4 };

//  Channels 2..5. Channel 1 is the panel lighting on this MCU and channel 0 is
//  the BT LED on the A32 - see drive.h.
const int CHAN[4] = { 2, 3, 4, 5 };

//  Standard 8-phase half-step sequence, bit 0 = IN1. Indices 0,2,4,6 are the
//  wave positions and 1,3,5,7 the full-step positions, so wave, full and half
//  all read from this one table; the mode only picks the step size and which
//  sub-lattice the position sits on.
const uint8_t HALF8[8] = {
  0b0001, 0b0011, 0b0010, 0b0110,
  0b0100, 0b1100, 0b1000, 0b1001
};

uint16_t SINPOS[FINE_PER_EREV];        // max(sin,0) scaled 0..4096
int32_t  gStepUnit = FINE_PER_HALFSTEP;
int32_t  gOffset   = 0;                // the full-step lattice sits half a step over
uint8_t  gMode     = DRV_HALF;
uint8_t  gMicro    = 1;
uint8_t  gDutyPct  = 100;
bool     gInvert   = true;             // MEASURED on this machine
uint32_t gLastDuty[4] = { 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF, 0xFFFFFFFF };
bool     gEnergised = false;
double   gPwmHz     = 0;     // what ledcSetup ACHIEVED, 0 = it refused
uint8_t  gBits      = DRV_LEDC_BITS;
uint32_t gDutyFull  = DRV_DUTY_FULL;

inline uint32_t dutyLimit() {
  uint16_t pct = gDutyPct;
  if (pct > 100) pct = 100;
  if (pct < 1)   pct = 1;
  return (gDutyFull * pct) / 100;
}

//  Only touch a channel whose duty actually changed. In half-step mode that is
//  one or two channels per step instead of four, and at high microstep rates it
//  is the difference between a few percent of a core and a lot of it.
inline void put(int i, uint32_t duty) {
  if (duty == gLastDuty[i]) return;
  gLastDuty[i] = duty;
  ledcWrite(CHAN[i], duty);
}

}  // namespace

namespace Drive {

void begin() {
  //  NEGOTIATE THE RESOLUTION DOWN rather than demanding one and failing.
  //  11 bits at 20 kHz needs log2(80e6/20000) = 11.97 bits of headroom on an
  //  80 MHz APB clock - it only just fits, and if the peripheral is clocked any
  //  other way it does not fit at all. ledcSetup then returns 0 and leaves four
  //  pins doing nothing, with no other symptom anywhere.
  //
  //  Resolution costs nothing that matters here: at 8 bits a microstep sine
  //  still has 256 levels per coil, far finer than the mechanical resolution of
  //  a 28BYJ-48 through a plastic gearbox.
  gPwmHz = 0;
  for (int bits = DRV_LEDC_BITS; bits >= 8 && gPwmHz == 0; bits--) {
    double got = ledcSetup(CHAN[0], DRV_LEDC_FREQ_HZ, bits);
    if (got > 0) { gPwmHz = got; gBits = (uint8_t)bits; gDutyFull = (1u << bits); }
  }
  if (gPwmHz == 0) {
    Con.println(F("  [FAIL] LEDC would not accept 20 kHz at ANY resolution."));
  } else {
    for (int i = 0; i < 4; i++) ledcSetup(CHAN[i], DRV_LEDC_FREQ_HZ, gBits);
    Con.print(F("  stepper PWM: "));
    Con.print(gPwmHz, 0);
    Con.print(F(" Hz at "));
    Con.print(gBits);
    Con.println(F("-bit"));
  }
  for (int i = 0; i < 4; i++) {
    ledcAttachPin(PINS[i], CHAN[i]);
    ledcWrite(CHAN[i], 0);
    gLastDuty[i] = 0;
  }
  for (int i = 0; i < FINE_PER_EREV; i++) {
    float s = sinf(2.0f * (float)M_PI * (float)i / (float)FINE_PER_EREV);
    SINPOS[i] = (s > 0.0f) ? (uint16_t)(s * 4096.0f + 0.5f) : 0;
  }
  gEnergised = false;
  setMode(gMode, gMicro);
}

void setMode(uint8_t mode, uint8_t micro) {
  if (mode >= DRV_MODE_COUNT) mode = DRV_HALF;
  gMode = mode;
  switch (mode) {
    case DRV_WAVE: gStepUnit = 2 * FINE_PER_HALFSTEP; gOffset = 0; break;
    case DRV_FULL: gStepUnit = 2 * FINE_PER_HALFSTEP; gOffset = FINE_PER_HALFSTEP; break;
    case DRV_HALF: gStepUnit = FINE_PER_HALFSTEP;     gOffset = 0; break;
    case DRV_MICRO: {
      uint8_t m = 1;
      while (m * 2 <= micro && m * 2 <= 64) m *= 2;
      gMicro    = m;
      gStepUnit = FINE_PER_HALFSTEP / m;
      gOffset   = 0;
      break;
    }
  }
}

int32_t stepUnit() { return gStepUnit; }

//  Nearest legal position in the current mode. Called on every mode change so
//  that switching drive mode never leaves the rotor between detents.
int64_t snap(int64_t posFine) {
  int64_t p = posFine - gOffset;
  int64_t q = (p >= 0) ? (p + gStepUnit / 2) / gStepUnit
                       : (p - gStepUnit / 2) / gStepUnit;
  return q * gStepUnit + gOffset;
}

//  The step path: masks, shifts and table lookups. The only multiply is the
//  duty scale.
void apply(int64_t posFine) {
  int64_t a = gInvert ? -posFine : posFine;
  //  Two's complement: masking the low 11 bits of a negative int64 gives the
  //  correct non-negative residue mod 2048, which a C division would not.
  uint32_t e   = (uint32_t)(a & (FINE_PER_EREV - 1));
  uint32_t lim = dutyLimit();

  if (gMode == DRV_MICRO) {
    //  IN1 cos, IN2 sin, IN3 -cos, IN4 -sin. A quarter turn is 512 fine.
    static const uint32_t PHASE[4] = { 512, 0, 1536, 1024 };
    for (int i = 0; i < 4; i++) {
      uint32_t v = SINPOS[(e + PHASE[i]) & (FINE_PER_EREV - 1)];
      put(i, (uint32_t)(((uint64_t)v * lim) >> 12));
    }
  } else {
    uint8_t mask = HALF8[e >> 8];
    for (int i = 0; i < 4; i++) put(i, (mask & (1u << i)) ? lim : 0u);
  }
  gEnergised = true;
}

void release() {
  for (int i = 0; i < 4; i++) put(i, 0);
  gEnergised = false;
}

bool energised()               { return gEnergised; }
void setInvert(bool on)        { gInvert = on; }
void setDutyPct(uint8_t pct)   { gDutyPct = pct; }

//  Retrievable at ANY time, not just at boot. This board is on native USB and
//  does not reset when a monitor attaches, so a banner printed once in setup()
//  is invisible in practice - you always join mid-run.
double   pwmHz()               { return gPwmHz; }
uint8_t  mode()                { return gMode; }
uint8_t  micro()               { return gMicro; }
uint8_t  bits()                { return gBits; }

}  // namespace Drive
