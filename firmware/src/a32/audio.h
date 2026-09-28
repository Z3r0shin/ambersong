// ============================================================================
//  AMBERSONG  -  AUDIO ENGINE  (A32)
//
//  THE ONE ARCHITECTURAL DECISION THAT MAKES THIS WORK
//  A2DP DOES NOT OWN I2S. `set_stream_reader(cb, false)` has ESP32-A2DP decode
//  SBC and hand us PCM, and the NullA2dpOutput it is given (a32/main.cpp)
//  leaves it no way to reach the I2S driver - the flag alone did not.
//
//  That matters because the two audio paths want incompatible drivers:
//    - A2DP alone wants TX only, 16-bit, no MCLK
//    - the radio path needs FULL DUPLEX, 32-bit slots, and MCLK on GPIO0,
//      which feeds both converters (Hardware Bible §3)
//  Reconfiguring or reinstalling the driver at every source change - while
//  Bluedroid holds a reference to it - is the part of this project that could
//  hard-fault in ways that are miserable to debug.
//
//  So we install I2S ONCE, at boot, full duplex, and NEVER touch it again.
//  Source switching becomes a decision about where the next block of samples
//  comes from, which is ordinary code with no driver surface at all.
//
//  SIGNAL FORMAT
//  Everything internal is int32 normalised to full scale at 2^31.
//    radio : the PCM1802's 24 bits arrive left-justified in a 32-bit slot,
//            so the raw word is already correct
//    BT    : 16-bit from the SBC decoder, shifted left 16
//
//  CHANNEL ORDER - INVERTED FROM CONVENTION
//  The firmware writes I2S slot 0 as the RIGHT speaker and slot 1 as the LEFT,
//  the order heard on this machine. Never assume slot 0 is left here. A
//  written frame is therefore { right, left }.
//
//  MUTE IS ALWAYS SOFTWARE
//  There is no hardware mute to use: no A32 pin reaches the DAC's XSMT, which
//  the module pulls up itself (Hardware Bible §6). Writing
//  zeros is complete and safe: AUX only needs the DAC silent because the amp's
//  input panel SUMS its three inputs (Hardware Bible §30).
// ============================================================================

#pragma once
#include <stdint.h>
#include "proto.h"

namespace Audio {

//  Installs I2S and starts the audio task. Call once, from setup().
void begin();

//  Source changes are RAMPED, not switched (when muteOnChange is on): ramp
//  out, switch at silence, ramp in. Without the ramp every change is a hard
//  cut, audible as a thump through the amplifier. Moving between the
//  selector's two ends passes through its centre, which reads AUX (Hardware
//  Bible §11).
void setSource(uint8_t src);
uint8_t source();

void setVolume(uint8_t v);          // 0..255, perceptual taper applied inside

//  The taper itself, gamma x 10 (see ProtoAudio::taperX10). Adjustable rather
//  than baked in, because the right curve depends on the amplifier it feeds and
//  can only be settled by ear.
void setTaper(uint8_t gammaX10);

//  Diagnostic: replace the output with a constant non-zero word. Silent as
//  audio, so anything heard while it is on is the fault itself. See audio.cpp.
void setTestDc(bool on);

//  The I2S peripheral's own latched fault bits, read-and-clear. See audio.cpp.
uint32_t i2sFaults();

//  The constant the DC diagnostic writes. Settable so amplitude and DIN edge
//  density can be varied independently - see the comment on dcWord.
void    setDcWord(int32_t w);
int32_t dcWordIs();

//  Force the lowest `bits` bits of every outgoing sample to zero, 0..24.
//  Up to 8 stays below one 24-bit LSB. Default 0 (off). See outMask.
void    setOutMaskBits(uint8_t bits);
int32_t outMaskIs();

//  Pad drive on DIN only, 0 (weakest) .. 3. The aggressor, not the victims.
void    setDinDrive(uint8_t cap);
uint8_t dinDriveIs();

//  Drive strength on the three CLOCK pins together: MCLK/SCK (GPIO0), BCK (18),
//  LRCK (17). Clamped to 2..3; begin() sets 3 at every boot. Read back from all
//  three pads; 0xFF if they disagree. See audio.cpp for why it cannot stop MCLK.
void    setClockDrive(uint8_t cap);            // both, kept for compatibility
//  MCLK separately from the frame pair - see audio.cpp for the measurements that
//  forced this. Each argument is clamped 2..3.
void    setClockDriveSplit(uint8_t mclkCap, uint8_t frameCap);
uint8_t mclkDriveIs();
uint8_t bckDriveIs();
uint8_t lrckDriveIs();
uint8_t clockDriveIs();

//  I2S0.timing.tx_sd_out_delay, 0..3 - read back only (the setter was removed
//  2026-09-25: only 0 plays correctly, and 2 sent the radio to FULL VOLUME).
uint8_t sdOutDelayIs();
bool testDcOn();
void setMute(bool m);
void setBalance(int16_t bal);       // -100..+100, negative = toward LEFT
void setGains(int16_t radioDb10, int16_t btDb10);   // tenths of a dB
void setMonoSum(bool on);
void setMuteOnChange(bool on);

//  Asymmetric by design: quick out, gentle in. See ProtoAudio in proto.h.
void setFades(uint16_t inMs, uint16_t outMs);

//  Fed from the A2DP callback on the Bluetooth task. Lock-free single producer,
//  single consumer - the audio task is the only reader.
void pushBt(const uint8_t *data, uint32_t len);

//  Peak levels since the last call, in tenths of a dBFS (-354 = -35.4 dBFS),
//  at the very end of the chain - after the volume, so the knob moves it.
void getPeaks(int16_t &ldB10, int16_t &rdB10, bool &clipped);
//  RMS since the last call, in tenths of a dBFS, measured AFTER the source gain
//  and BEFORE the volume - so the knob does not move it. -1200 is silence.
void getRms(int16_t &ldB10, int16_t &rdB10);

//  Diagnostics for the portal. A rising underrun count on BT means the link or
//  the ring buffer, not the amplifier.
uint32_t btUnderruns();
uint32_t btRingFill();

//  THE ZERO-DATA WATCH. The PCM5102A analogue-mutes itself after 1024 LRCKs of
//  zero data (SLAS859C 9.3.2.3) and un-mutes without a documented ramp. This
//  mirrors that detector so its releases could be put beside the pops. The
//  theory is EXCLUDED: rel stayed frozen through 8 minutes of pops
//  (2026-09-10). Kept as telemetry. See audio.cpp.
struct ZeroWatch {
  uint32_t arm;          // times the DAC would have entered analogue mute
  uint32_t rel;          // times it would have come back out
  uint32_t sinceRelMs;   // ms since the last release, 0xFFFFFFFF if never
  uint32_t longestMs;    // longest single mute
  uint32_t stalls;       // audio-task gaps able to arm the detector alone
  uint32_t stallMaxUs;   // the longest such gap
  uint32_t stallLastMs;  // millis() of the latest one, 0 if none
  uint32_t stallLastUs;  // and its gap
  uint16_t i2sSticky;    // I2S0.int_raw, OR-accumulated, never cleared
  bool     muted;        // would be analogue-muted right now
  bool     floorOn;      // the zero-data floor is engaged
};
void zeroWatch(ZeroWatch &w);

//  Built as the zero-data theory's fix, default off: lift every word the DAC
//  would read as zero to one 24-bit LSB, so the detector can never arm.
//  -138.5 dBFS.
void setZeroFloor(bool on);
bool zeroFloorOn();

//  Tested 2026-09-11 and NOT the fix; default off. Bits 7..0 of every word take
//  the sign of the next word on the wire, so DIN never transitions across a
//  frame boundary.
void setSignTail(bool on);
bool signTailOn();

//  THE VOLUME WATCH. The volume law is applied as a hard step at a block
//  boundary while the fade envelope beside it ramps per sample, so a reading
//  that moves on its own is a discontinuity in the audio. These report how far
//  the gain that MULTIPLIES THE SAMPLES has actually travelled since boot:
//  65536 is unity, 0 is silence. All monotonic - see audio.cpp.
void gainWatch(uint32_t &steps, uint32_t &q16min, uint32_t &q16max);

}  // namespace Audio
