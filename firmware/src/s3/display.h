// ============================================================================
//  AMBERSONG  -  LEDITRON DISPLAY  (S3)
//
//  Four multiplexed digits, one lit at a time, 100 Hz frame. The timing was
//  proven under WiFi load on the bench (Firmware Gospel §4).
//
//  THE 825 us DEAD TIME IS NOT OPTIONAL AND IS NOT A GUESS.
//  Blanking the segments is not enough: the outgoing digit's high-side switch
//  (Bible §9) is still conducting when the next digit is enabled, and without
//  the dead time the display ghosts badly. 825 us was found by test on the
//  machine. The author judged the resulting brightness fine and decided against
//  a driver change to recover the lost duty - do not re-raise it unless the
//  display is later called too dim.
//
//  TIMING, and why the numbers are what they are
//    tick        25 us   - the greatest common divisor of 825 and 1675, so both
//                          phases land on exact tick counts
//    0  .. 32    dark    - 825 us, the dead time. NEVER shortened.
//    33 .. 33+L  lit     - L scales with brightness, at most 66 ticks (1650 us)
//    .. 99       idle    - whatever is left, so the SLOT stays 2500 us
//    slot        100 ticks = 2500 us, x4 digits = 100 Hz frame
//
//  BRIGHTNESS SHORTENS THE LIT WINDOW, NEVER THE SLOT. The frame rate is
//  therefore constant and dimming introduces no flicker. Two consequences worth
//  knowing: 255 is the MAXIMUM - there is nowhere up to go - and the COLON is
//  not driven by the firmware (Bible §8), so it does not dim with the digits.
//  The dimmer you set the digits, the more the colon stands out.
//
//  ONLY ONE DIGIT IS EVER ON. Structural, not discipline: the ISR clears the
//  outgoing digit at the end of its lit window and again at the slot boundary,
//  and sets the next one only after the dead time.
// ============================================================================

#pragma once
#include <stdint.h>

namespace Display {

void begin();

//  0..255, scaling the lit window. 255 is the longest window the slot allows
//  (66 of 100 ticks). 0 is dark but still refreshing.
void setBrightness(uint8_t b);
uint8_t brightness();

//  -1 in any position blanks that digit. Index 0 is the RIGHTMOST digit.
void showDigits(const int8_t d[4]);

void showTime(uint8_t hh, uint8_t mm, bool hour12, bool blankLeadingZero);

//  For the tuning readout: 1017 shows as "1017", meaning 101.7 MHz. The
//  firmware drives no decimal point, so the reading is by convention.
void showNumber(uint16_t v, bool blankLeadingZeros);

void blank();

//  Diagnostics: worst observed deviation of a digit slot from its ideal 2500 us.
//  The bench timing test's measurement, kept live so the console and the portal
//  can show that the display still keeps time.
uint32_t worstSlotErrorUs();
void     resetSlotStats();

}  // namespace Display
