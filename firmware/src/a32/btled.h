// ============================================================================
//  AMBERSONG  -  BT INDICATOR LED  (A32, GPIO33)
//
//  Author's specification of 2026-08-28, implemented verbatim:
//
//    OFF    off
//    ON     steady, duty 64/255
//    LOOK   smooth breathe  32 -> 255 -> 32   over 1200 ms
//    STDBY  slow breathe     8 -> 120 -> 8    over 4000 ms
//    FOUND  150 on / 150 off / 150 on (450 ms), one shot, then CON
//    CON    steady, duty 180/255
//    LINK   blink 300 on / 300 off, repeating   (EXPLICIT PAIRING, not "linked")
//
//  The visual language: fast breathing = actively looking, slow breathing =
//  idle, double flash = found, steady = connected, regular blink = pairing.
//
//  ACTIVE LOW (the LED's wiring: Hardware Bible §11), so every brightness is
//  written inverted: for brightness B, write 255 - B. LEDC channel 0, 2 kHz,
//  8-bit, exactly as specified.
//
//  BREATHING USES A RAISED COSINE, NOT A LINEAR RAMP. The eye's response is
//  roughly logarithmic, so a linear fade reads as a fast rise with a long dim
//  tail rather than as a breath.
//
//  AND THE PIN IS NEVER LEFT FLOATING. Driving it HIGH is what "off" means.
//  Left as an input its leakage can glow the LED faintly - invisible by day,
//  visible in a dark room - and INPUT_PULLDOWN would sink more still.
// ============================================================================

#pragma once
#include <stdint.h>

namespace BtLed {

void begin();

//  Setting FOUND starts its one shot; it advances to CON by itself.
void setState(uint8_t btState);
//  Since 2026-09-24 main.cpp reads this back to publish that FOUND -> CON
//  handover, so the lamp's timer is the only clock that decides it.
uint8_t state();

//  Per-state master scale, 0..255. 255 means "exactly as specified above",
//  which is the default. This is the portal knob without losing the spec.
void setScale(const uint8_t scale[7]);

//  Call often from loop(). Cheap, and the more often it runs the smoother the
//  breathe - there is no timer task behind this.
void update();

}  // namespace BtLed
