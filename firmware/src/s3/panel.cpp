#include <Arduino.h>
#include "panel.h"
#include "pins.h"
#include "proto.h"

namespace Panel {

static const int PANEL_CH = 1;    // channel 0 is the A32's BT LED; keep them
                                  // distinct so the two firmwares read alike.
                                  // Channels 2-5 are the stepper's (drive.cpp)

static uint8_t  lvlTuning = 255;
static uint8_t  lvlIdle   = 128;
static uint8_t  lvlOther  = 96;
static uint32_t idleDelay = 5000;
static uint16_t dwellMs   = 250;
static uint16_t travelMs  = 900;

static uint8_t  cur = 0, tgt = 0, from = 0;
static uint32_t moveStart = 0;
static uint32_t lastTurnMs = 0;

// ---------------------------------------------------------------------------
//  EASING
//  The author's brief: "not rapid and instantaneous, but like a capacitor
//  emptying" - hold for a moment, then start slowly, accelerate, and settle.
//
//  smoothstep, p*p*(3-2p), gives exactly that shape: zero slope at both ends
//  and the fastest movement in the middle. A linear ramp starts and stops
//  abruptly and reads as electronic; this reads as something with mass.
//
//  The DWELL before it matters as much as the curve. Real panel lamps do not
//  begin dimming the instant the current changes - the hold is what makes the
//  machine seem to decide rather than to react.
// ---------------------------------------------------------------------------
static inline float smoothstep(float p) {
  if (p <= 0) return 0;
  if (p >= 1) return 1;
  return p * p * (3.0f - 2.0f * p);
}

void begin() {
  ledcSetup(PANEL_CH, 1000, 8);
  ledcAttachPin(S3_PANEL_PWM, PANEL_CH);
  ledcWrite(PANEL_CH, 0);
  cur = tgt = from = 0;
  moveStart = millis();
}

void setLevels(uint8_t t, uint8_t i, uint8_t o) { lvlTuning = t; lvlIdle = i; lvlOther = o; }
void setIdleDelayMs(uint32_t ms)                { idleDelay = ms; }
void setFadeMs(uint16_t ms)                     { travelMs = ms ? ms : 1; }
void setDwellMs(uint16_t ms)                    { dwellMs = ms; }

void update(bool ampOn, uint8_t source, bool tuning) {
  uint32_t now = millis();
  if (tuning) lastTurnMs = now;

  uint8_t want;
  if (!ampOn) {
    want = 0;
  } else if (source == SRC_RADIO) {
    //  Full while the dial is moving and for `idleDelay` after it stops. The
    //  hold is what keeps it from dimming between one nudge of the knob and
    //  the next.
    bool recent = lastTurnMs && (now - lastTurnMs) < idleDelay;
    want = recent ? lvlTuning : lvlIdle;
  } else {
    want = lvlOther;
  }

  if (want != tgt) {
    //  Start a fresh move FROM WHEREVER WE ARE, not from the old target. A
    //  change of mind mid-fade must not make the level jump.
    from      = cur;
    tgt       = want;
    moveStart = now;
  }

  if (cur == tgt) return;

  uint32_t el = now - moveStart;
  if (el < dwellMs) return;                       // the hold
  float p = (float)(el - dwellMs) / (float)travelMs;
  float k = smoothstep(p);
  int   v = (int)from + (int)((float)((int)tgt - (int)from) * k);
  cur = (uint8_t)constrain(v, 0, 255);
  ledcWrite(PANEL_CH, cur);
}

uint8_t level()  { return cur; }
uint8_t target() { return tgt; }

}  // namespace Panel
