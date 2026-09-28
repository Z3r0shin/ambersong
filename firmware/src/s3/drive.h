// ============================================================================
//  AMBERSONG  -  STEPPER DRIVE LAYER  (S3)
//
//  Ported from an earlier test rig (not published), where it was measured.
//  That rig is retired; the motion design is in Firmware Gospel §5.
//
//  Turns a position in FINE UNITS into four PWM duties. Nothing above this file
//  knows a ULN2003 exists (the driver and motor: Bible §11).
//
//  WHY EVERYTHING GOES THROUGH LEDC EVEN AT 100% DUTY
//  It is the only way to microstep a unipolar motor on a dumb Darlington array,
//  and half-step chatter is visible on a needle at low speed. The old drive in
//  this firmware wrote four GPIOs directly and could only half-step.
//
//  THE SINE TABLE IS 2048 ENTRIES FOR A REASON. One electrical revolution is
//  8 half-steps = 2048 fine units, so the low 11 bits of the position ARE the
//  electrical angle. No division, no modulo, no float trig in the step path -
//  a mask and four table lookups.
//
//  COIL MAPPING. The firmware drives IN1/IN3 as the two opposed halves of one
//  phase and IN2/IN4 as the halves of the other (the wiring: Bible §11). A
//  unipolar half-coil conducts one way only, so a signed sinusoid becomes two
//  rectified ones on opposite windings:
//
//      IN1 = max( cos t, 0)      IN3 = max(-cos t, 0)
//      IN2 = max( sin t, 0)      IN4 = max(-sin t, 0)
//
//  At 45 degrees IN1 and IN2 both sit at 0.707 rather than 1.0. That is not a
//  rounding loss - it is a constant-magnitude current vector, and it is exactly
//  why microstepping is smoother than half-stepping, whose two-phase-on
//  positions are 41% stronger than its one-phase-on positions.
//
//  LEDC CHANNELS 2..5 ON THIS BOARD. Channel 1 is the panel lighting; the A32
//  uses channel 0 for its BT LED. Keeping the numbers distinct across both
//  firmwares means a channel number always means the same thing.
// ============================================================================

#pragma once
#include <stdint.h>

//  20 kHz is above hearing and slow enough that the ULN2003's slow-decay
//  recirculation holds near-continuous coil current between chops.
static const int      DRV_LEDC_FREQ_HZ = 20000;
static const int      DRV_LEDC_BITS    = 11;
static const uint32_t DRV_DUTY_FULL    = (1u << DRV_LEDC_BITS);   // 2048

static const int32_t FINE_PER_HALFSTEP = 256;
static const int32_t FINE_PER_EREV     = 8 * FINE_PER_HALFSTEP;   // 2048

//  Half-steps per output revolution for a 64:1 gearbox. NOT USED anywhere in
//  the firmware: the needle counts half-steps from the index and never needs
//  the gear ratio.
static const int32_t HALFSTEPS_PER_REV = 4096;

enum : uint8_t { DRV_WAVE = 0, DRV_FULL, DRV_HALF, DRV_MICRO, DRV_MODE_COUNT };

namespace Drive {

void begin();

//  `micro` is rounded DOWN to a power of two, at most 64, so every microstep
//  lands on an integer fine position; anything else accumulates a fractional
//  error.
void setMode(uint8_t mode, uint8_t micro);

int32_t stepUnit();               // fine units per step in the current mode
int64_t snap(int64_t posFine);    // nearest legal position, for mode changes

void apply(int64_t posFine);
void release();
bool energised();

//  Flips the ELECTRICAL ANGLE, not the position, so "position increases" keeps
//  meaning the same thing to everything above this file and only the shaft
//  turns the other way. needle.cpp turns it on: on this machine advancing the
//  phase drives the needle toward LOW FM.
void setInvert(bool on);

//  Percent of full duty. needle.cpp sets 100 for the motor supply the Bible
//  records (§11). It is a parameter rather than a constant so a higher supply
//  could be derated here; this build never changes it.
void setDutyPct(uint8_t pct);

//  Boot-time facts, retrievable later. On native USB the banner is never seen.
double   pwmHz();     // frequency ledcSetup ACHIEVED. 0 means it refused.
uint8_t  mode();
uint8_t  micro();
uint8_t  bits();      // duty resolution actually negotiated

}  // namespace Drive
