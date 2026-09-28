#include <Arduino.h>
#include <math.h>
#include "btled.h"
#include "pins.h"
#include "proto.h"

namespace BtLed {

static const int LED_CH = 0;          // author's spec: LEDC channel 0
static uint8_t   st     = BT_OFF;
static uint32_t  entered = 0;
static uint8_t   scale[7] = { 255, 255, 255, 255, 255, 255, 255 };

//  Raised cosine between lo and hi over `periodMs`. Phase 0 starts at lo, so a
//  breathe always begins from its dim end rather than jumping to mid-brightness
//  when the state changes.
static inline uint8_t breathe(uint32_t t, uint32_t periodMs, uint8_t lo, uint8_t hi) {
  float ph = (float)(t % periodMs) / (float)periodMs;
  float k  = (1.0f - cosf(ph * 2.0f * (float)M_PI)) * 0.5f;
  return (uint8_t)(lo + (hi - lo) * k);
}

static inline void write(uint8_t b, uint8_t sc) {
  uint16_t v = ((uint16_t)b * sc) / 255;
  ledcWrite(LED_CH, 255 - v);         // ACTIVE LOW
}

void begin() {
  ledcSetup(LED_CH, 2000, 8);         // 2 kHz, 8-bit, per the spec
  ledcAttachPin(A32_BT_LED, LED_CH);
  write(0, 255);                      // dark, and DRIVEN dark
  st = BT_OFF;
  entered = millis();
}

void setState(uint8_t s) {
  if (s > BT_LINK) return;
  if (s == st) return;
  st = s;
  entered = millis();
}

uint8_t state() { return st; }

void setScale(const uint8_t s[7]) { for (int i = 0; i < 7; i++) scale[i] = s[i]; }

void update() {
  uint32_t t  = millis() - entered;
  uint8_t  sc = scale[st];

  switch (st) {
    case BT_OFF:   write(0, 255);                       break;
    case BT_ON:    write(64, sc);                       break;
    case BT_LOOK:  write(breathe(t, 1200, 32, 255), sc); break;
    case BT_STDBY: write(breathe(t, 4000,  8, 120), sc); break;
    case BT_CON:   write(180, sc);                      break;

    case BT_LINK:
      //  Explicit pairing mode: a regular, unmistakable blink.
      write((t % 600) < 300 ? 255 : 0, sc);
      break;

    case BT_FOUND: {
      //  One shot: on / off / on, 150 ms each, then hand over to CON. The
      //  handover happens here rather than at the call site so that "found"
      //  always resolves, even if the caller forgets.
      if      (t < 150) write(255, sc);
      else if (t < 300) write(0,   sc);
      else if (t < 450) write(255, sc);
      else              setState(BT_CON);
      break;
    }
  }
}

}  // namespace BtLed
