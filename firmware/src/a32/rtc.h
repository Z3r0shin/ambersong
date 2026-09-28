// ============================================================================
//  AMBERSONG  -  DS3231 REAL TIME CLOCK  (A32)
//
//  The RTC hangs off the AUDIO MCU (Bible §5) and the display off the MAIN one,
//  so the time crosses the UART. The S3 keeps its own software clock and
//  re-syncs it from this one every minute, so a slow or missing link never
//  makes the display stutter.
//
//  PINS: A32_RTC_SDA / A32_RTC_SCL and the bus speed A32_I2C_HZ are in pins.h;
//  what they are wired to is in the Bible (§3, §5).
//
//  EVERYTHING HERE IS UTC. The S3 applies the timezone, because the S3 is the
//  one with NTP and a portal. Storing local time in an RTC means the hour after
//  a DST change is ambiguous and the hour before it happens twice.
//
//  The oscillator-stopped flag is the whole reason `valid` exists. A DS3231
//  that has lost power reports a perfectly well-formed, completely wrong time,
//  and the only way to know is to ask whether its oscillator ever stopped.
// ============================================================================

#pragma once
#include <Arduino.h>
#include <Wire.h>
#include <time.h>
#include "proto.h"
#include "pins.h"

namespace Rtc {

static const uint8_t ADDR      = 0x68;
static const uint8_t REG_TIME  = 0x00;
static const uint8_t REG_STAT  = 0x0F;   // bit 7 = OSF, oscillator stopped
static const uint8_t REG_TEMP  = 0x11;

static inline uint8_t bcd2bin(uint8_t v) { return (uint8_t)((v >> 4) * 10 + (v & 0x0F)); }
static inline uint8_t bin2bcd(uint8_t v) { return (uint8_t)(((v / 10) << 4) | (v % 10)); }

static inline void begin() {
  Wire.begin(A32_RTC_SDA, A32_RTC_SCL, A32_I2C_HZ);
}

static inline bool present() {
  Wire.beginTransmission(ADDR);
  return Wire.endTransmission() == 0;
}

//  Reads UTC. Returns false only if the time registers could not be read; a
//  failed status read makes `valid` 0, a failed temperature read leaves 0.
static inline bool read(ProtoTime &out) {
  memset(&out, 0, sizeof(out));

  Wire.beginTransmission(ADDR);
  Wire.write(REG_TIME);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)ADDR, 7) != 7) return false;

  uint8_t ss = bcd2bin(Wire.read() & 0x7F);
  uint8_t mi = bcd2bin(Wire.read() & 0x7F);
  uint8_t hr = bcd2bin(Wire.read() & 0x3F);    // forced 24-hour on write
  Wire.read();                                  // day of week, unused
  uint8_t dd = bcd2bin(Wire.read() & 0x3F);
  uint8_t mo = bcd2bin(Wire.read() & 0x1F);
  uint8_t yy = bcd2bin(Wire.read());

  //  Oscillator-stopped flag. Without this check a dead battery yields a
  //  confident, well-formed, wrong answer.
  bool osf = false, statOk = false;
  Wire.beginTransmission(ADDR);
  Wire.write(REG_STAT);
  if (Wire.endTransmission(false) == 0 && Wire.requestFrom((int)ADDR, 1) == 1) {
    osf    = (Wire.read() & 0x80) != 0;
    statOk = true;
  }

  Wire.beginTransmission(ADDR);
  Wire.write(REG_TEMP);
  if (Wire.endTransmission(false) == 0 && Wire.requestFrom((int)ADDR, 2) == 2) {
    int8_t  t  = (int8_t)Wire.read();
    uint8_t fr = Wire.read() >> 6;
    out.tempC4 = (int8_t)(t * 4 + fr);          // quarter-degrees
  }

  struct tm tmv = {};
  tmv.tm_sec  = ss; tmv.tm_min = mi; tmv.tm_hour = hr;
  tmv.tm_mday = dd; tmv.tm_mon = mo - 1; tmv.tm_year = 100 + yy;   // 2000-based

  //  mktime interprets in local time, so the zone is set to UTC first - and
  //  left there: nothing on the A32 uses local time.
  setenv("TZ", "UTC0", 1); tzset();
  time_t e = mktime(&tmv);

  out.unixUtc = (uint32_t)e;

  //  VALID = PLAUSIBLE (after 2020-09-13) AND THE STATUS READ ANSWERED AND OSF
  //  CLEAR - since 2026-09-24. Before, OSF only went into reserved[0], which the
  //  S3 never reads, and a DS3231 that had stopped - it resumes from where it
  //  stopped, a perfectly plausible date - was adopted as the truth.
  //
  //  OSF is sticky, and that is the point: a clock that stopped and was never
  //  set since IS wrong. write() clears it, and the S3 writes the time through
  //  MSG_SET_TIME when NTP is fresh (at most hourly) and when the time is set
  //  by hand; the first such write makes `valid` 1 again.
  //
  //  A status register that did not answer counts as NOT valid: the safe
  //  direction, since the S3 keeps its own clock running and only declines to
  //  resync from this one reading. reserved[0] still carries OSF itself.
  out.valid       = (e > 1600000000L && statOk && !osf) ? 1 : 0;
  out.reserved[0] = osf ? 1 : 0;
  return true;
}

//  Writes UTC and clears the oscillator-stopped flag, which is what makes the
//  time trustworthy again after a power loss.
static inline bool write(uint32_t unixUtc) {
  time_t e = (time_t)unixUtc;
  struct tm tmv;
  setenv("TZ", "UTC0", 1); tzset();
  gmtime_r(&e, &tmv);

  Wire.beginTransmission(ADDR);
  Wire.write(REG_TIME);
  Wire.write(bin2bcd(tmv.tm_sec));
  Wire.write(bin2bcd(tmv.tm_min));
  Wire.write(bin2bcd(tmv.tm_hour));               // bit 6 clear = 24-hour mode
  Wire.write(bin2bcd(tmv.tm_wday + 1));
  Wire.write(bin2bcd(tmv.tm_mday));
  Wire.write(bin2bcd(tmv.tm_mon + 1));
  Wire.write(bin2bcd((uint8_t)(tmv.tm_year - 100)));
  if (Wire.endTransmission() != 0) return false;

  Wire.beginTransmission(ADDR);
  Wire.write(REG_STAT);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom((int)ADDR, 1) != 1) return false;
  uint8_t st = Wire.read() & ~0x80;
  Wire.beginTransmission(ADDR);
  Wire.write(REG_STAT);
  Wire.write(st);
  return Wire.endTransmission() == 0;
}

}  // namespace Rtc
