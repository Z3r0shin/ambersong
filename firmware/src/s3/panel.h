// ============================================================================
//  AMBERSONG  -  RADIO PANEL LIGHT CONTROL (RPLC)  (S3, GPIO21)
//
//  The lamps behind the dial, one PWM output (S3_PANEL_PWM) switching them
//  together; the lamp circuit is in Bible §11. 1 kHz PWM: far above flicker
//  fusion, and low enough that the switching edges stay a negligible fraction
//  of the period.
//
//  AUTHOR'S RULES (defaults in brackets)
//    amp off                  -> dark
//    RADIO, dial being turned -> the tuning level (255, full)
//    RADIO, dial idle 5 s     -> the idle level (128, 50%)
//    AUX or BT                -> its own level (96)
//  The three levels, the idle delay, the hold and the fade are portal settings.
//  main.cpp also passes "amp off" while an OTA is running, and AUX whenever it
//  holds no current state from the A32.
//
//  TWO THINGS THAT MATTER MORE THAN THEY LOOK
//
//  The "is the dial being turned" test needs a deadband WIDER THAN THE AS5600's
//  OWN JITTER, or the panel flashes to full at random while nobody is touching
//  it. That deadband lives in needle.cpp (TUNE_DEADBAND, 12 counts - a little
//  over a degree), and the jitter's reversals reset its accumulator.
//
//  And levels are RAMPED, not stepped. An instant jump from full to half reads
//  as a glitch; the same change eased over about a second (250 ms hold, then
//  900 ms of travel, by default) reads as intent. Nothing about the hardware
//  requires this - it is the difference between the machine seeming to think
//  and seeming to twitch.
// ============================================================================

#pragma once
#include <stdint.h>

namespace Panel {

void begin();

void setLevels(uint8_t whileTuning, uint8_t radioIdle, uint8_t otherSource);
void setIdleDelayMs(uint32_t ms);
void setFadeMs(uint16_t ms);      // duration of the eased travel
void setDwellMs(uint16_t ms);     // stillness before it starts moving

//  Call often (main.cpp: every loop() pass). The target is derived from these
//  three on every call; only the ramp and the time of the last turn are kept
//  here.
void update(bool ampOn, uint8_t source, bool tuning);

uint8_t level();      // current, after ramping
uint8_t target();     // where it is heading

}  // namespace Panel
