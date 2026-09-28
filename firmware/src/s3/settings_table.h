// ============================================================================
//  THE TABLE ITSELF.
//
//  Included by main.cpp ONCE, after `cfg` and the A32 mirror exist, because it
//  points straight into them. Nothing else includes this file - portal.cpp
//  works through settings_api.h and never sees a struct.
//
//  ROLES, decided 2026-08-31: `admin` marks the settings that change what the
//  MACHINE is - the needle profile, the gain staging, the calibrations, the
//  network. Everything left unmarked is what a guest can reasonably touch:
//  loudness, mute, and how bright the thing is. The split is deliberately about
//  DAMAGE, not about secrecy - a normal user can see every value, and can only
//  move the ones that are obvious and reversible.
//
//  ADDING A SETTING whose field already exists is one line here, and the
//  portal, the JSON and the settings file follow. A NEW FIELD is more: on the
//  S3 it is appended to the stored struct with a settings-version bump, a
//  migration and its use in applySettings(); on the A32 it is a change to the
//  wire structs in proto.h (Firmware Gospel §7).
// ============================================================================
#pragma once

const char *const kTabName[TAB_COUNT] = {
  "Audio", "Display", "Lights", "Needle", "Bluetooth", "System"
};

//  Shorthands, so a row stays readable on one line.
#define S_(k,l,u,tab,st,kd,lo,hi,sp,sc,own,adm,p) \
  { k, l, u, tab, st, kd, lo, hi, sp, sc, nullptr, (void*)(p), own, adm }
#define E_(k,l,tab,st,ch,own,adm,p) \
  { k, l, "", tab, st, K_ENUM, 0, 0, 1, 1.0f, ch, (void*)(p), own, adm }

const SettingDesc gSet[] = {
  // ---- AUDIO -------------------------------------------------------------
  S_("volume",      "Volume",              "",   TAB_AUDIO, SU8,  K_INT,     0,   255,  1,  1.0f, OWN_A32_AUDIO, 0, &a32cfg.audio.volume),
  S_("muted",       "Mute",                "",   TAB_AUDIO, SU8,  K_BOOL,    0,     1,  1,  1.0f, OWN_A32_AUDIO, 0, &a32cfg.audio.muted),
  S_("taper",       "Volume law (gamma)",  "",   TAB_AUDIO, SU8,  K_FLOAT, 1.0f, 4.0f, 0.1f, 0.1f, OWN_A32_AUDIO, 1, &a32cfg.audio.taperX10),
  S_("balance",     "Balance",             "",   TAB_AUDIO, SI16, K_INT,  -100,   100,  1,  1.0f, OWN_A32_AUDIO, 1, &a32cfg.audio.balance),
  S_("gainRadio",   "Radio makeup gain",   "dB", TAB_AUDIO, SI16, K_FLOAT, -20,    30, 0.5f, 0.1f, OWN_A32_AUDIO, 1, &a32cfg.audio.gainRadio),
  S_("gainBt",      "Bluetooth gain",      "dB", TAB_AUDIO, SI16, K_FLOAT, -20,    30, 0.5f, 0.1f, OWN_A32_AUDIO, 1, &a32cfg.audio.gainBt),
  S_("monoSum",     "Sum L+R (radio)",     "",   TAB_AUDIO, SU8,  K_BOOL,    0,     1,  1,  1.0f, OWN_A32_AUDIO, 1, &a32cfg.audio.monoSum),
  S_("muteOnChange","Ramp across source changes","",TAB_AUDIO,SU8,K_BOOL,    0,     1,  1,  1.0f, OWN_A32_AUDIO, 1, &a32cfg.audio.muteOnChange),
  S_("fadeInMs",    "Fade in",             "ms", TAB_AUDIO, SU16, K_INT,     0,  5000, 50, 1.0f, OWN_A32_AUDIO, 1, &a32cfg.audio.fadeInMs),
  S_("fadeOutMs",   "Fade out",            "ms", TAB_AUDIO, SU16, K_INT,     0,  5000, 10, 1.0f, OWN_A32_AUDIO, 1, &a32cfg.audio.fadeOutMs),

  // ---- DISPLAY -----------------------------------------------------------
  S_("brightOn",    "Brightness, radio on",  "", TAB_DISPLAY, SU8,  K_INT,   0,  255,  1, 1.0f, OWN_S3, 0, &cfg.brightOn),
  S_("brightOff",   "Brightness, radio off", "", TAB_DISPLAY, SU8,  K_INT,   0,  255,  1, 1.0f, OWN_S3, 0, &cfg.brightOff),
  S_("hour12",      "12-hour clock",         "", TAB_DISPLAY, SU8,  K_BOOL,  0,    1,  1, 1.0f, OWN_S3, 1, &cfg.hour12),
  S_("blankLeadZero","Blank the leading zero","", TAB_DISPLAY, SU8, K_BOOL,  0,    1,  1, 1.0f, OWN_S3, 1, &cfg.blankLeadZero),
  S_("dispDwellMs", "Brightness dwell",    "ms", TAB_DISPLAY, SU16, K_INT,   0, 5000, 50, 1.0f, OWN_S3, 1, &cfg.dispDwellMs),
  S_("dispFadeMs",  "Brightness fade",     "ms", TAB_DISPLAY, SU16, K_INT,   0,10000, 50, 1.0f, OWN_S3, 1, &cfg.dispFadeMs),
  E_("showTuning",  "Clock or tuning readout",        TAB_DISPLAY, SU8,
     "Clock always,Tuning always,Tuning while tuning", OWN_S3, 1, &cfg.showTuning),
  S_("tuneHoldMs",  "Readout stays for",   "ms", TAB_DISPLAY, SU16, K_INT, 500,20000,250,1.0f, OWN_S3, 1, &cfg.tuneHoldMs),

  // ---- LIGHTS ------------------------------------------------------------
  S_("panelTuning", "Panel, while tuning",   "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_S3, 0, &cfg.panelTuning),
  S_("panelIdle",   "Panel, radio idle",     "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_S3, 0, &cfg.panelIdle),
  S_("panelOther",  "Panel, AUX or Bluetooth","",TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_S3, 0, &cfg.panelOther),
  S_("panelIdleMs", "Idle after",          "ms", TAB_LIGHTS, SU16, K_INT,    0,60000,250,1.0f, OWN_S3, 1, &cfg.panelIdleMs),
  S_("panelFadeMs", "Panel fade",          "ms", TAB_LIGHTS, SU16, K_INT,    0, 5000, 50, 1.0f, OWN_S3, 1, &cfg.panelFadeMs),
  S_("panelDwellMs","Panel dwell",         "ms", TAB_LIGHTS, SU16, K_INT,    0, 5000, 50, 1.0f, OWN_S3, 1, &cfg.panelDwellMs),
  S_("btLedOff",    "BT LED, off",           "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[0]),
  S_("btLedOn",     "BT LED, ready",         "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[1]),
  S_("btLedLook",   "BT LED, looking",       "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[2]),
  S_("btLedStdby",  "BT LED, standby",       "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[3]),
  S_("btLedFound",  "BT LED, found",         "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[4]),
  S_("btLedCon",    "BT LED, connected",     "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[5]),
  S_("btLedLink",   "BT LED, pairing",       "", TAB_LIGHTS, SU8,  K_INT,    0,  255,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.ledBrightness[6]),

  // ---- NEEDLE ------------------------------------------------------------
  //  The needle's motion profile. Its defaults are validated settings
  //  (Firmware Gospel §5); the portal can move them, but read that chapter
  //  before you do.
  S_("upVmax",   "Sweep up, top speed",   "hs/s",  TAB_NEEDLE, SU16, K_INT,  100, 4000, 50, 1.0f, OWN_S3, 1, &cfg.upVmax),
  S_("upAccel",  "Sweep up, acceleration","hs/s2", TAB_NEEDLE, SU16, K_INT, 1000,60000,500,1.0f, OWN_S3, 1, &cfg.upAccel),
  S_("dnVmax",   "Fall, top speed",       "hs/s",  TAB_NEEDLE, SU16, K_INT,  100, 4000, 50, 1.0f, OWN_S3, 1, &cfg.dnVmax),
  S_("dnAccel",  "Fall, acceleration",    "hs/s2", TAB_NEEDLE, SU16, K_INT, 1000,60000,500,1.0f, OWN_S3, 1, &cfg.dnAccel),
  S_("wn",       "Ring-down (wn)",        "rad/s", TAB_NEEDLE, SF32, K_FLOAT, 1, 21, 0.5f, 1.0f, OWN_S3, 1, &cfg.wn),
  S_("zeta",     "Damping (zeta)",        "",      TAB_NEEDLE, SF32, K_FLOAT, 0.1f, 2.0f, 0.05f, 1.0f, OWN_S3, 1, &cfg.zeta),
  S_("riseMs",   "Fall envelope, rise",   "ms",    TAB_NEEDLE, SU16, K_INT,   10, 3000, 10, 1.0f, OWN_S3, 1, &cfg.riseMs),
  S_("fallMs",   "Fall envelope, fall",   "ms",    TAB_NEEDLE, SU16, K_INT,   10, 5000, 10, 1.0f, OWN_S3, 1, &cfg.fallMs),
  S_("dwellUpMs","Hesitation before rising","ms",  TAB_NEEDLE, SU16, K_INT,    0, 3000, 25, 1.0f, OWN_S3, 1, &cfg.dwellUpMs),
  S_("dwellDnMs","Hesitation before falling","ms", TAB_NEEDLE, SU16, K_INT,    0, 3000, 25, 1.0f, OWN_S3, 1, &cfg.dwellDnMs),
  S_("microFast","Microstep while moving","1/n",   TAB_NEEDLE, SU8,  K_INT,    1,   32,  1, 1.0f, OWN_S3, 1, &cfg.microFast),
  S_("microSlow","Microstep at rest",     "1/n",   TAB_NEEDLE, SU8,  K_INT,    1,   32,  1, 1.0f, OWN_S3, 1, &cfg.microSlow),
  //  reapHsps drives the band calibration's creep and the portal jog. The
  //  homing re-approach does not use it: that is +10 half-step profiled hops.
  S_("reapHsps", "Measuring pass speed",  "hs/s",  TAB_NEEDLE, SU16, K_INT,   10,  500, 10, 1.0f, OWN_S3, 1, &cfg.reapHsps),
  //  BOTH LIMITS SPAN THE WHOLE TRAVEL because the index is NOT centred on it
  //  (Firmware Gospel §5). Homing puts zero at the index, so posMin is
  //  negative and posMax positive - setGeometry() ENFORCES that, refusing any
  //  pair that does not bracket the index at zero. The wide range exists
  //  because settingBound() CLAMPS rather than rejects: with the old ranges
  //  (-3000..0 and 0..3000, which assumed a centred index) an out-of-range
  //  value stored the bound, and the high limit was silently unsettable while
  //  it was negative. Validity is setGeometry()'s job, not these numbers'.
  S_("posMin",   "Soft limit, low",  "half-steps", TAB_NEEDLE, SI32, K_INT,-3000, 3000,  1, 1.0f, OWN_S3, 1, &cfg.posMin),
  S_("posMax",   "Soft limit, high", "half-steps", TAB_NEEDLE, SI32, K_INT,-3000, 3000,  1, 1.0f, OWN_S3, 1, &cfg.posMax),
  //  THE INDEX BAND, measured by the band calibration (`k`). Rows since
  //  2026-09-24 so the settings FILE carries them: before, a restore onto blank
  //  flash answered "N settings applied" and left all four at zero - and with
  //  the band uncalibrated every crossing read as a slip. They are measurements,
  //  not preferences; normally only the band calibration (console `k`, or
  //  needle.calBand) writes them.
  //  idxOnFwd IS NOT A ROW. It is the homing edge, zero by definition - the
  //  band calibration always yields 0 - and homing puts the index AT it, so an
  //  edited value moved both soft limits physically, into a stop.
  S_("idxOffFwd","Index band, off going up",    "half-steps", TAB_NEEDLE, SI32, K_INT, -500, 500, 1, 1.0f, OWN_S3, 1, &cfg.idxOffFwd),
  S_("idxOnRev", "Index band, on going down",   "half-steps", TAB_NEEDLE, SI32, K_INT, -500, 500, 1, 1.0f, OWN_S3, 1, &cfg.idxOnRev),
  S_("idxOffRev","Index band, off going down",  "half-steps", TAB_NEEDLE, SI32, K_INT, -500, 500, 1, 1.0f, OWN_S3, 1, &cfg.idxOffRev),
  //  MEANING CHANGED 2026-08-31. Homing is not tied to this: the needle is
  //  homed at boot (that home always sweeps), and on an amp-on edge only if it
  //  has no zero. This is purely theatre: a full end-to-end sweep on each
  //  amp-on edge, on top of the journey to the station that happens anyway.
  //  Stored 0 still means "no", so nothing needed migrating.
  S_("sweepOn",  "Full sweep when the radio comes on", "", TAB_NEEDLE, SU8, K_BOOL, 0, 1, 1, 1.0f, OWN_S3, 1, &cfg.sweepOn),
  //  TWO DIFFERENT MHz SCALES, and the old labels made them indistinguishable.
  //  band* is where the TUNING CAPACITOR reaches; dial* is what is PRINTED at
  //  the needle stops. The two are not the same range, which is exactly the
  //  confusion to avoid.
  //  bandLow/bandHigh ARE NO LONGER SETTINGS. They are the pre-mark fallback
  //  line and nothing reads them once two marks exist - so leaving them on the
  //  Needle tab, editable, labelled "Tuner reaches", meant the author would read
  //  a frozen number that the curve had long since left behind and type a
  //  correction into a field that applySettings() discards on the next call.
  //  Where the tuner actually reaches is computed from the live curve and
  //  published in the state JSON as reach0/reach1.
  //  What is PRINTED at the needle stops - not how far the capacitor turns.
  S_("dialLow",  "Printed at low stop",  "x0.1MHz",TAB_NEEDLE, SU16, K_INT,  500, 2000,  1, 1.0f, OWN_S3, 1, &cfg.dialLow),
  S_("dialHigh", "Printed at high stop", "x0.1MHz",TAB_NEEDLE, SU16, K_INT,  500, 2000,  1, 1.0f, OWN_S3, 1, &cfg.dialHigh),
  S_("calLow",   "Tuner angle at low end","counts",TAB_NEEDLE, SI32, K_INT,-20000,20000, 1, 1.0f, OWN_S3, 1, &cfg.calLow),
  S_("calHigh",  "Tuner angle at high end","counts",TAB_NEEDLE,SI32, K_INT,-20000,20000, 1, 1.0f, OWN_S3, 1, &cfg.calHigh),
  //  The tube set's IF (Bible §22). Stored in 50 kHz units; shown in MHz. The
  //  default is 10.60 MHz; change it here if the set's IF is ever retuned.
  S_("ifOffset", "Tube set IF",          "MHz", TAB_NEEDLE, SI16, K_FLOAT, 10.0f, 11.5f, 0.05f, 0.05f, OWN_S3, 1, &cfg.ifOffset20),

  // ---- BLUETOOTH ---------------------------------------------------------
  S_("connectable", "Let a paired phone reconnect","",TAB_BT, SU8,  K_BOOL, 0,    1,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.connectable),
  S_("lookTimeoutS","Pairing window",            "s", TAB_BT, SU16, K_INT,  15,  600,  5, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.lookTimeoutS),
  S_("pauseOnLeave","Pause the phone before hanging up","",TAB_BT,SU8,K_BOOL,0,  1,  1, 1.0f, OWN_A32_BT, 1, &a32cfg.bt.pauseOnLeave),
};

const int gSetCount = (int)(sizeof(gSet) / sizeof(gSet[0]));

#undef S_
#undef E_

// ---------------------------------------------------------------------------
//  GET / SET. The only two functions that know what a `store` code means.
// ---------------------------------------------------------------------------
//  THE ONLY PLACE A VALUE IS BOUNDED, used by BOTH the portal and the settings
//  file. It was written twice before, and the second copy
//  went to the file path only, so `POST /api/set k=showTuning v=99` still went
//  straight into the struct. Two copies of a rule is one copy too many.
//
//  NaN is rejected rather than clamped, because `NaN < lo` and `NaN > hi` are
//  BOTH false: every comparison-based clamp passes it through untouched, and
//  String::toFloat() is atof(), which happily parses "nan". posMin is one of
//  the things standing in for end-stop switches the needle does not have
//  (Bible §11: its one sensor is the index); a soft limit of LONG_MIN is not a
//  soft limit.
static bool settingBound(const SettingDesc &d, float &v) {
  if (!isfinite(v)) return false;
  if (d.kind == K_BOOL) { v = (v != 0.0f) ? 1.0f : 0.0f; return true; }
  if (d.kind == K_ENUM) {
    int n = 1;
    for (const char *c = d.choices; c && *c; c++) if (*c == ',') n++;
    if (v < 0) v = 0;
    if (v > n - 1) v = n - 1;
    return true;
  }
  if (v < d.lo) v = d.lo;
  if (v > d.hi) v = d.hi;
  return true;
}

float settingGet(int i) {
  if (i < 0 || i >= gSetCount) return 0;
  const SettingDesc &d = gSet[i];
  //  COPY, DO NOT DEREFERENCE. Half of these point into __attribute__((packed))
  //  structs whose alignment is 1, so a 16- or 32-bit member can and does land
  //  on an odd address - `bt.lookTimeoutS` is at offset 17 of ProtoCfgAll. A
  //  cast-and-dereference there is undefined behaviour that this toolchain
  //  happens to absorb today; memcpy is defined everywhere and costs nothing
  //  at these widths.
  float raw = 0;
  switch (d.store) {
    case SU8:  { uint8_t  v; memcpy(&v, d.ptr, sizeof(v)); raw = (float)v; break; }
    case SU16: { uint16_t v; memcpy(&v, d.ptr, sizeof(v)); raw = (float)v; break; }
    case SI16: { int16_t  v; memcpy(&v, d.ptr, sizeof(v)); raw = (float)v; break; }
    case SI32: { int32_t  v; memcpy(&v, d.ptr, sizeof(v)); raw = (float)v; break; }
    case SF32: { float    v; memcpy(&v, d.ptr, sizeof(v)); raw = v;        break; }
  }
  return raw * d.scale;
}

int settingFind(const char *key) {
  for (int i = 0; i < gSetCount; i++)
    if (!strcmp(gSet[i].key, key)) return i;
  return -1;
}

bool settingSet(int i, float shown) {
  if (i < 0 || i >= gSetCount) return false;
  const SettingDesc &d = gSet[i];

  //  CLAMP HERE, not in the browser. The browser is a suggestion; this is the
  //  machine. A posMax of 30000 would drive the needle into its stop.
  if (!settingBound(d, shown)) return false;

  //  DO NOT WRITE A MIRROR WE HAVE NEVER READ.
  //  a32cfg is zero until the A32 answers MSG_GET_CFG. Pushing it in that
  //  window would send an all-zero ProtoAudio - wiping the radio makeup gain,
  //  the fade times, mono sum, every BT LED level - and the A32 would dutifully
  //  persist the lot. A boot race that quietly erases a calibration is exactly
  //  the class of bug the settings versioning was added to prevent.
  if (d.owner != OWN_S3 && !haveCfg) return false;

  float raw = (d.scale != 0.0f) ? (shown / d.scale) : shown;
  bool changed = false;

  switch (d.store) {
    case SU8:  { uint8_t  v = (uint8_t) lroundf(raw), o; memcpy(&o, d.ptr, 1); changed = (o != v); memcpy(d.ptr, &v, 1); break; }
    case SU16: { uint16_t v = (uint16_t)lroundf(raw), o; memcpy(&o, d.ptr, 2); changed = (o != v); memcpy(d.ptr, &v, 2); break; }
    case SI16: { int16_t  v = (int16_t) lroundf(raw), o; memcpy(&o, d.ptr, 2); changed = (o != v); memcpy(d.ptr, &v, 2); break; }
    case SI32: { int32_t  v = (int32_t) lroundf(raw), o; memcpy(&o, d.ptr, 4); changed = (o != v); memcpy(d.ptr, &v, 4); break; }
    case SF32: { float    v = raw, o;                   memcpy(&o, d.ptr, 4); changed = (o != v); memcpy(d.ptr, &v, 4); break; }
  }
  if (!changed) return false;

  //  TYPING THE TUNER ENDS COUNTS AS MEASURING THEM. calLow/calHigh are
  //  editable rows on the Needle tab, and that writer set neither the flag nor
  //  reported the result - so an author who typed his measured ends into the
  //  fields instead of pressing the two buttons had them DISCARDED from the fit
  //  domain, while the card two rows below read "tuner ends NOT measured" next
  //  to the numbers he had just entered.
  //  ONE FIELD, ONE END. Setting the whole flag from either field let a single
  //  typed value declare both ends measured while the other was still the
  //  placeholder - and /api/set has no channel to say what that did to the
  //  curve, so the marks were demoted in silence behind a green answer.
  if (d.ptr == &cfg.calLow)  cfg.tunerEndsSet |= ENDS_LOW;
  if (d.ptr == &cfg.calHigh) cfg.tunerEndsSet |= ENDS_HIGH;

  //  Apply immediately and push to whoever owns it. Nothing here writes flash -
  //  settingsFlush() debounces that, and waits while writeSafe() refuses
  //  (needle moving, calibration or update running).
  switch (d.owner) {
    case OWN_S3:        applySettings(); settingsTouch();            break;
    case OWN_A32_AUDIO: gLink.send(MSG_SET_AUDIO, a32cfg.audio);     break;
    case OWN_A32_BT:    gLink.send(MSG_SET_BT,    a32cfg.bt);        break;
  }
  return true;
}

// ---------------------------------------------------------------------------
//  THE SETTINGS FILE. Plain key=value, one per line, with the version and a
//  timestamp as comments. The author asked for download and upload; keeping it
//  text means it is still readable when neither this firmware nor this browser
//  exists any more.
// ---------------------------------------------------------------------------
void settingsToText(String &out) {
  char line[128];
  out.reserve(4096);
  out += "# Ambersong settings\n";
  snprintf(line, sizeof(line), "# firmware %s (%s)\n", FW_VERSION, FW_COMMIT); out += line;
  snprintf(line, sizeof(line), "# settings version %u\n", (unsigned)cfg.version); out += line;
  uint32_t e = nowEpoch();
  if (e) {
    time_t t = (time_t)e; struct tm tmv; localtime_r(&t, &tmv);
    strftime(line, sizeof(line), "# saved %Y-%m-%dT%H:%M:%S\n", &tmv); out += line;
  }
  //  THE AUDIO BOARD'S ROWS ARE WRITTEN AS n/a UNTIL IT HAS ANSWERED. Its
  //  mirror is all zeros before then, and a file of those zeros, uploaded
  //  later, would have pushed volume 0, gain 0, LEDs off and "not connectable"
  //  to the A32, which saves them. n/a is refused on the way
  //  back in, named, and the A32 keeps what it has.
  if (!haveCfg) out += "# the audio board had not answered - its settings are n/a\n";
  for (int i = 0; i < gSetCount; i++) {
    if (gSet[i].owner != OWN_S3 && !haveCfg) {
      snprintf(line, sizeof(line), "%s=n/a\n", gSet[i].key);
      out += line;
      continue;
    }
    float v = settingGet(i);
    if (gSet[i].store == SF32 || gSet[i].scale != 1.0f)
      snprintf(line, sizeof(line), "%s=%.3f\n", gSet[i].key, v);
    else
      snprintf(line, sizeof(line), "%s=%ld\n", gSet[i].key, (long)lroundf(v));
    out += line;
  }
  //  THE TUNING MARKS ARE NOT gSet[] ROWS, so the loop above cannot see them -
  //  and they are the two evenings of calibration this file exists to protect.
  //  bandLow/bandHigh ride here too. They stopped being gSet rows - they are
  //  only the pre-mark fallback, and an editable "tuner reaches" that the fit
  //  ignored was its own bug - but dropping them from gSet also dropped them
  //  from this file, and with no marks stored they ARE the live mapping.
  //  tuneUsed IS WRITTEN EVEN WHEN IT IS ZERO, and the restore hangs its "wipe
  //  the marks" step on THIS key rather than on the first tuneMark line. A v5
  //  file with no marks would otherwise be byte-identical to a pre-v5 file, so
  //  a backup taken before marking could never be used to undo marking - and a
  //  restore is the author's only undo.
  snprintf(line, sizeof(line), "bandLow=%u\n",  cfg.bandLow);  out += line;
  snprintf(line, sizeof(line), "bandHigh=%u\n", cfg.bandHigh); out += line;
  snprintf(line, sizeof(line), "tuneOffset10=%d\n", (int)cfg.tuneOffset10); out += line;
  snprintf(line, sizeof(line), "tuneUsed=%u\n", (unsigned)cfg.tuneUsed); out += line;
  snprintf(line, sizeof(line), "tunerEndsSet=%u\n", (unsigned)cfg.tunerEndsSet); out += line;
  for (int i = 0; i < TUNE_MARKS; i++) {
    if (!(cfg.tuneUsed & (1u << i))) continue;
    snprintf(line, sizeof(line), "tuneMark%d=%ld,%u\n", i, (long)cfg.tuneP[i], cfg.tuneF[i]);
    out += line;
  }

  //  NO TRANSMIT POWER IN THE FILE (the author: "remove from portal, keep
  //  on console"). wifiTxQ is learned state; the ladder in net.cpp owns it and
  //  the console steps it. An old file's wifiTxQ line is ignored on upload.

  //  THE RDA'S FIXED FEATURES, hand-written for the same reason the marks are:
  //  gSet points at one scalar with one range, and this is a variable-length
  //  list. spurUsed IS WRITTEN EVEN WHEN IT IS ZERO, and the restore hangs its
  //  "wipe the list" step on THIS key rather than on the first spur line - the
  //  tuneUsed argument transplanted exactly. Without it a V6 file taken with an
  //  empty list would be byte-identical to a pre-V6 file, so a backup made
  //  BEFORE the teaching ritual could never undo the ritual, and a restore is
  //  the author's only undo.
  snprintf(line, sizeof(line), "spurUsed=%u\n", (unsigned)cfg.spurUsed); out += line;
  for (uint8_t i = 0; i < cfg.spurUsed && i < CFG_SPURS; i++) {
    snprintf(line, sizeof(line), "spur%u=%d\n", (unsigned)i, (int)cfg.spur[i]);
    out += line;
  }

  //  Not settings, but part of the machine's state and cheap to record.
  if (haveCfg) {
    snprintf(line, sizeof(line), "# potcal %u..%u..%u\n",
             a32cfg.potMin, a32cfg.potMid, a32cfg.potMax); out += line;
  }
}

static char gRestoreNote[512] = "";
const char *settingsRestoreNote() { return gRestoreNote; }

//  gActionRefused / refuse() live in main.cpp, ABOVE captureLimit(), because
//  this header is included long after captureLimit() is written. When they
//  were defined here its refusals could not call refuse() and went out green:
//  the author pressed "Set low limit here" before homing and believed the soft
//  limits were captured. Those limits are what keeps the needle off its
//  mechanical stops, so every caller must be able to reach the one helper.
bool actionRefused() { return gActionRefused; }

//  WHAT THE TUNING CURVE IS DOING, for callers outside this translation unit.
//  Empty when there is nothing to say. /api/set had no way to report that the
//  value just typed had demoted or refused the curve - it answered {"ok":1,"v":
//  ...} and the page stored the number - so a single field edit could throw
//  away three marks behind a green answer.
const char *tuneTroubleNote() {
  static char t[184];
  bool bad = false;
  tuneStateLine(t, sizeof(t), &bad);
  //  Silent only when the machine is running the model its marks entitle it to
  //  AND both ends are known. Refused, demoted, or half-measured all speak.
  if (!bad && !fitDemoted() && cfg.tunerEndsSet == ENDS_BOTH) t[0] = 0;
  return t;
}

//  A number, all of it. One decimal comma is accepted as the point (the author
//  writes French); leading/trailing blanks are allowed; nothing else is.
static bool parseNumber(String s, float &out) {
  s.trim();
  s.replace(',', '.');
  if (!s.length()) return false;
  const char *c = s.c_str();
  char *e = nullptr;
  out = strtof(c, &e);
  return e != c && *e == 0 && isfinite(out);
}

//  A whole integer, or nothing - for the keys parsed by hand. toInt() read
//  "tunerEndsSet=" as 0 and APPLIED it.
static bool parseLong(String s, long &out) {
  s.trim();
  if (!s.length()) return false;
  const char *c = s.c_str();
  char *e = nullptr;
  out = strtol(c, &e, 10);
  return e != c && *e == 0;
}

//  REFUSED AND CLAMPED LINES ARE NAMED. The upload used to drop an unreadable
//  value in silence, and store an out-of-range one as its bound, and answer
//  "N settings applied" in green; the toast is the only channel once the
//  cabinet is shut. Kept in their OWN short lists and appended to gRestoreNote
//  LAST, so the tuning and fixed-feature refusals - the ones that matter - are
//  never crowded out of the 512 bytes by a file full of junk keys.
struct KeyList { char s[96]; uint8_t n; };
static KeyList gRefused, gClamped, gNotValid, gSkippedA32;
static void noteKey(KeyList &l, const char *key) {
  size_t at = strlen(l.s);
  if (l.n < 255) l.n++;
  if (l.n <= 5)       snprintf(l.s + at, sizeof(l.s) - at, "%s%.20s", at ? ", " : "", key);
  else if (l.n == 6)  snprintf(l.s + at, sizeof(l.s) - at, ", ...");
}
static void noteRefusedKey(const char *key) { noteKey(gRefused, key); }
static void appendKeyNotes() {
  const KeyList *ls[4] = { &gRefused, &gClamped, &gNotValid, &gSkippedA32 };
  const char *hd[4] = { "REFUSED (unreadable or unknown): ", "CLAMPED to its range: ",
                        "n/a in the file - kept as they are: ",
                        "NOT SENT - the audio board is not answering: " };
  for (int j = 0; j < 4; j++) {
    if (!ls[j]->n) continue;
    size_t at = strlen(gRestoreNote);
    snprintf(gRestoreNote + at, sizeof(gRestoreNote) - at, "%s%s%s", at ? "; " : "", hd[j], ls[j]->s);
  }
}

int settingsFromText(const String &in) {
  gRestoreNote[0] = 0;
  memset(&gRefused, 0, sizeof(gRefused));
  memset(&gClamped, 0, sizeof(gClamped));
  memset(&gNotValid, 0, sizeof(gNotValid));
  memset(&gSkippedA32, 0, sizeof(gSkippedA32));
  bool rowSeen[gSetCount] = {};      // S3 rows the file carried, for the lock
  int applied = 0, at = 0, n = in.length();
  bool touchedS3 = false, touchedAudio = false, touchedBt = false;
  //  The file's marks, held aside until the whole file has parsed.
  //  THE SPUR LIST, parsed into locals and committed as a unit - never
  //  touching cfg unless the file announced N and exactly slots 0..N-1 arrived.
  int16_t  spF[CFG_SPURS] = {0};
  uint32_t spGot  = 0;                // bit i: slot i parsed
  long     spUsed = 0;
  bool     spSeen = false, spBad = false;
  int32_t  mkP[TUNE_MARKS] = {0};
  uint16_t mkF[TUNE_MARKS] = {0};
  uint16_t mkUsed = 0, mkGot = 0;
  bool     mkSeen = false;
  //  V5 files carry tunerEndsSet explicitly. Pre-V5 files carry calLow/calHigh
  //  and no such key, so the fact has to be inferred - AFTER the parse, from
  //  the values that actually arrived.
  //  ONE FLAG PER END, because the file may carry one and not the other, and a
  //  pre-V5 file carries no tunerEndsSet key at all. Inferring the PAIR from
  //  either value promoted a machine that had measured only its low end to
  //  "both measured" on restore, which fitted over a domain twice the shaft's
  //  travel and threw away every mark.
  bool     endsKeySeen = false, calLowWritten = false, calHighWritten = false;
  int16_t  mkOff = 0;
  bool     mkOffSeen = false, mkOffBad = false;

  while (at < n) {
    int nl = in.indexOf('\n', at);
    if (nl < 0) nl = n;
    String ln = in.substring(at, nl);
    at = nl + 1;
    ln.trim();
    if (ln.length() == 0 || ln[0] == '#') continue;
    int eq = ln.indexOf('=');
    if (eq <= 0) continue;
    String k = ln.substring(0, eq); k.trim();
    String v = ln.substring(eq + 1);  v.trim();
    //  THE HAND-PARSED KEYS, HANDLED BEFORE settingFind() because they are not
    //  gSet[] rows. For the marks, a v5 file always carries tuneUsed, and THAT
    //  key is what commits the set - so a v5 backup with no marks correctly
    //  wipes marks made since, while a pre-v5 file, which has no such key,
    //  leaves them alone.
    //  tunerEndsSet is applied eagerly, like calLow/calHigh themselves, which
    //  are ordinary gSet rows: it says whether THOSE were measured, and is not
    //  held aside with the marks.
    if (k == "tunerEndsSet") {
      long e;
      if (parseLong(v, e) && e >= 0 && e <= ENDS_BOTH) {
        cfg.tunerEndsSet = (uint8_t)e; endsKeySeen = true;
        touchedS3 = true; applied++;
      } else noteRefusedKey(k.c_str());
      continue;
    }
    if (k == "bandLow" || k == "bandHigh") {
      long b;
      if (!parseLong(v, b) || b < 500 || b > 2000) { noteRefusedKey(k.c_str()); continue; }
      if (k == "bandLow") cfg.bandLow = (uint16_t)b; else cfg.bandHigh = (uint16_t)b;
      touchedS3 = true; applied++;
      continue;
    }
    //  HELD ASIDE WITH THE MARKS. Applied eagerly, a truncated file installed
    //  ITS offset over the machine's OWN marks - a correction accumulated
    //  against one curve, slid onto a different one, by up to 2 MHz. The marks,
    //  the mask and the offset are one calibration and commit as one unit.
    if (k == "tuneOffset10") {
      //  THE SAME PARSER AS ITS TWO NEIGHBOURS. This was the one field between
      //  them still on toInt(), under a comment observing that hardening one
      //  field and not the ones beside it is the shape this project keeps
      //  repeating. And a line that is SEEN but unusable now refuses the whole
      //  tuning block: silently skipping it installed the FILE's marks under
      //  the MACHINE's old hand offset - the same mismatched-correction bug the
      //  atomic commit exists to prevent, only inverted.
      char *end = nullptr;
      const char *cs = v.c_str();
      long o = strtol(cs, &end, 10);
      if (!*cs || end == cs || *end || o < -200 || o > 200) { mkOffBad = true; continue; }
      mkOff = (int16_t)o; mkOffSeen = true;
      continue;
    }
    //  PARSED INTO LOCALS, COMMITTED ONCE, AT THE END.
    //
    //  This used to wipe cfg.tune* the instant it saw the tuneUsed line, before
    //  a single replacement had parsed. A body truncated after that line - a
    //  dropped upload, a hand-edited file - therefore ERASED the calibration
    //  and put nothing back, using the one file that was supposed to restore
    //  it. The only backup destroying the only copy is not a recoverable
    //  failure. Now nothing touches cfg unless the whole file parsed and the
    //  marks that arrived match the mask that announced them.
    if (k == "tuneUsed") {
      //  IT MUST PARSE, and this is the one key where that matters most.
      //  String::toInt() answers 0 for anything it cannot read, and 0 is
      //  precisely the destructive value here: a mask of nothing means no mark
      //  line can set a bit, mkGot == mkUsed == 0 satisfies the consistency
      //  gate below, and the commit wipes every stored mark while reporting
      //  success. "tuneUsed=0x7" - the form the console dump used to print,
      //  and what someone reconstructing this file by hand may still write -
      //  would have erased two evenings of calibration. strtol with base 0
      //  accepts both forms and says whether it consumed the whole field.
      char *end = nullptr;
      const char *cs = v.c_str();
      long u = strtol(cs, &end, 0);
      if (!*cs || end == cs || *end) continue;
      if (u < 0 || u > 0xFFFF) continue;
      mkUsed = (uint16_t)u;
      mkSeen = true;
      continue;
    }
    //  RETIRED KEYS, accepted and ignored so an old file uploads cleanly:
    //  wifiTxQ left the file and autoConnect left the table on 2026-09-25.
    if (k == "wifiTxQ" || k == "autoConnect") continue;
    if (k == "spurUsed") {
      //  strtol with full-consumption checking, not String::toInt(), for the
      //  same reason tuneUsed uses it: toInt() answers 0 for anything it cannot
      //  read, and 0 is the destructive value - it means "wipe the list".
      char *end = nullptr;
      const char *cs = v.c_str();
      spUsed = strtol(cs, &end, 0);
      if (!*cs || end == cs || *end || spUsed < 0 || spUsed > CFG_SPURS) { spBad = true; continue; }
      spSeen = true;
      continue;
    }
    if (k.startsWith("spur")) {
      if (!spSeen) { spBad = true; continue; }   // a value with no count
      char *e1 = nullptr;
      const char *is = k.c_str() + 4;
      long slot = strtol(is, &e1, 10);
      if (!*is || e1 == is || *e1 || slot < 0 || slot >= (long)CFG_SPURS) { spBad = true; continue; }
      char *e2 = nullptr;
      const char *fs = v.c_str();
      long f = strtol(fs, &e2, 10);
      //  Range-checked to the LO window: anything outside it is not a feature
      //  this instrument can ever see, so the file is wrong rather than exotic.
      if (!*fs || e2 == fs || *e2 || f < Rda::LO_WINDOW_LO10 || f > Rda::LO_WINDOW_HI10) {
        spBad = true; continue;
      }
      if (spGot & (1u << slot)) { spBad = true; continue; }   // duplicate slot
      spF[slot] = (int16_t)f;
      spGot |= (1u << slot);
      continue;
    }
    if (k.startsWith("tuneMark")) {
      if (!mkSeen) continue;          // a mark with no mask: the file is broken
      //  THE SLOT INDEX TOO - the third field beside the two already hardened,
      //  under the comment explaining why they were. "tuneMarkA=-817,1073" is
      //  what someone writes after reading the console dump, which labels the
      //  slots A/B/C; toInt() answers 0, so mark A would silently receive mark
      //  C's numbers, the mask would still agree, and the toast would be green.
      const char *ks = k.c_str() + 8;
      char *se = nullptr;
      long slotL = strtol(ks, &se, 10);
      if (!*ks || se == ks || *se) continue;
      int slot = (int)slotL;
      int comma = v.indexOf(',');
      if (slot < 0 || slot >= TUNE_MARKS || comma <= 0) continue;
      //  THE SAME PARSER AS THE MASK, for the same reason. toInt() answers 0 for
      //  anything it cannot read, and a silent 0 here is a mark planted at the
      //  shaft's zero - which is inside the travel, so no guard would catch it
      //  and the curve would simply be wrong. Hardening one field and not the
      //  two beside it is the shape this project keeps repeating.
      String sp = v.substring(0, comma), sf = v.substring(comma + 1);
      sp.trim(); sf.trim();
      char *pe = nullptr, *fe = nullptr;
      long mp = strtol(sp.c_str(), &pe, 10);
      long mf = strtol(sf.c_str(), &fe, 10);
      if (!sp.length() || pe == sp.c_str() || *pe) continue;
      if (!sf.length() || fe == sf.c_str() || *fe) continue;
      //  The shaft position is signed; only the frequency has a real range.
      if (mp < -1000000L || mp > 1000000L || mf < 870 || mf > 1085) continue;
      mkP[slot] = (int32_t)mp;
      mkF[slot] = (uint16_t)mf;
      mkGot |= (uint16_t)(1u << slot);
      continue;
    }

    int i = settingFind(k.c_str());
    if (i < 0) { noteRefusedKey(k.c_str()); continue; }
    //  n/a: the file was saved while the audio board was silent (see
    //  settingsToText). Not a value - the setting keeps what it has.
    if (v.equalsIgnoreCase("n/a")) { noteKey(gNotValid, k.c_str()); continue; }
    //  The audio board is not answering NOW: its rows cannot be sent. Named,
    //  not dropped in silence.
    if (gSet[i].owner != OWN_S3 && !haveCfg) { noteKey(gSkippedA32, k.c_str()); continue; }

    //  Set the field WITHOUT sending a message per line: a whole file would be
    //  a hundred UART frames and a hundred NVS touches. Push once at the end.
    const SettingDesc &d = gSet[i];
    //  THE WHOLE VALUE MUST BE A NUMBER. toFloat() read "" and "-" as 0 and
    //  "10,60" as 10, and the clamp then stored the row's low bound and counted
    //  it applied: an IF of 10.00 MHz, a soft limit AT the index.
    //  A decimal comma is read as the point it means; anything
    //  else unreadable is refused, named, and changes nothing.
    float shown;
    if (!parseNumber(v, shown)) { noteRefusedKey(k.c_str()); continue; }
    float asked = shown;
    if (!settingBound(d, shown)) { noteRefusedKey(k.c_str()); continue; }
    if (fabsf(shown - asked) > 0.0005f) noteKey(gClamped, k.c_str());
    float raw = (d.scale != 0.0f) ? (shown / d.scale) : shown;
    switch (d.store) {
      case SU8:  { uint8_t  v = (uint8_t) lroundf(raw); memcpy(d.ptr, &v, 1); break; }
      //  (the same rule as settingSet: see the note there)
      case SU16: { uint16_t v = (uint16_t)lroundf(raw); memcpy(d.ptr, &v, 2); break; }
      case SI16: { int16_t  v = (int16_t) lroundf(raw); memcpy(d.ptr, &v, 2); break; }
      case SI32: { int32_t  v = (int32_t) lroundf(raw); memcpy(d.ptr, &v, 4); break; }
      case SF32: { float    v = raw;                    memcpy(d.ptr, &v, 4); break; }
    }
    if (d.ptr == &cfg.calLow)  calLowWritten  = true;
    if (d.ptr == &cfg.calHigh) calHighWritten = true;
    if (d.owner == OWN_S3)             { touchedS3 = true; rowSeen[i] = true; }
    else if (d.owner == OWN_A32_AUDIO) touchedAudio = true;
    else                               touchedBt = true;
    applied++;
  }

  //  COMMIT THE MARKS, or refuse them and leave what was there alone. A
  //  pre-v5 file carries no tuneUsed at all and correctly changes nothing here.
  if (mkSeen && mkGot == mkUsed && !mkOffBad) {
    for (int j = 0; j < TUNE_MARKS; j++) { cfg.tuneP[j] = mkP[j]; cfg.tuneF[j] = mkF[j]; }
    cfg.tuneUsed = mkUsed;
    if (mkOffSeen) cfg.tuneOffset10 = mkOff;
    touchedS3 = true;
    applied++;
  } else if (mkSeen || mkOffSeen || mkOffBad) {
    //  REFUSE THE WHOLE TUNING BLOCK, and say so where it can be read. The
    //  note goes back to the caller because the portal's upload toast is the
    //  only channel that exists once the cabinet is shut - a refusal printed
    //  to the serial console has not been reported.
    //  APPENDED, like the spur note below: refused keys may already be in it.
    size_t at0 = strlen(gRestoreNote);
    if (mkOffBad)
      snprintf(gRestoreNote + at0, sizeof(gRestoreNote) - at0,
               "%sTUNING CALIBRATION REFUSED - the hand-offset line is unreadable; "
               "the stored calibration was kept", at0 ? "; " : "");
    else
      snprintf(gRestoreNote + at0, sizeof(gRestoreNote) - at0,
               "%sTUNING MARKS REFUSED - the file announced %u and %u parsed; "
               "the stored calibration was kept", at0 ? "; " : "",
               (unsigned)__builtin_popcount(mkUsed), (unsigned)__builtin_popcount(mkGot));
    Con.print(F("  portal: "));
    Con.println(gRestoreNote);
  }

  //  COMMIT THE FIXED FEATURES, or refuse them and keep what is stored. A
  //  pre-V6 file carries no spurUsed at all and correctly changes nothing.
  //  Two entries closer than 3 tenths are the SAME feature counted twice -
  //  isSpur() matches at +/-0.2 MHz - so the file announced more than it
  //  delivered, and that refuses the block like any other inconsistency.
  if (spSeen && !spBad && spGot == (uint32_t)((1u << spUsed) - 1u)) {
    for (long a = 0; a < spUsed && !spBad; a++)
      for (long b = a + 1; b < spUsed; b++) {
        int16_t d = (int16_t)(spF[a] - spF[b]);
        if (d < 0) d = (int16_t)-d;
        if (d < 3) { spBad = true; break; }
      }
  }
  if (spSeen && !spBad && spGot == (uint32_t)((1u << spUsed) - 1u)) {
    for (uint8_t j = 0; j < CFG_SPURS; j++) cfg.spur[j] = (j < spUsed) ? spF[j] : 0;
    cfg.spurUsed = (uint8_t)spUsed;
    touchedS3 = true;
    applied++;
  } else if (spSeen || spBad) {
    //  ONE NOTE CHANNEL, TWO POSSIBLE COMPLAINTS. gRestoreNote is a single
    //  buffer and the tuning refusal may already be in it, so this APPENDS
    //  rather than overwriting - the tuning calibration is the more valuable
    //  of the two and must not be silently displaced by a spur message.
    size_t at = strlen(gRestoreNote);
    snprintf(gRestoreNote + at, sizeof(gRestoreNote) - at,
             "%sFIXED-FEATURE LIST REFUSED - the stored one was kept",
             at ? "; " : "");
    Con.print(F("  portal: "));
    Con.println(gRestoreNote);
  }

  //  SETTLED BEFORE THE FIT READS IT, not after. This block sat BELOW the
  //  applySettings() call, so the curve was built from the flag's OLD value and
  //  the correction did not take effect until the next reboot or the next
  //  unrelated setting edit - which is every failure it was written to prevent,
  //  still happening, with the flag afterwards reading the right answer. It
  //  also marks the settings touched when it changes anything, so the fit
  //  actually reruns rather than merely being allowed to.
  if ((calLowWritten || calHighWritten) && !endsKeySeen) {
    //  PER END, and only for the end the file actually carried. A value equal
    //  to its struct default is the placeholder, so it CLEARS that end's bit
    //  rather than leaving a stale claim standing.
    uint8_t inferred = cfg.tunerEndsSet;
    if (calLowWritten)
      inferred = (cfg.calLow  != 0)    ? (uint8_t)(inferred |  ENDS_LOW)
                                       : (uint8_t)(inferred & ~ENDS_LOW);
    if (calHighWritten)
      inferred = (cfg.calHigh != 6023) ? (uint8_t)(inferred |  ENDS_HIGH)
                                       : (uint8_t)(inferred & ~ENDS_HIGH);
    if (inferred != cfg.tunerEndsSet) { cfg.tunerEndsSet = inferred; touchedS3 = true; }
  }

  //  AN UPLOAD IS THE DELIBERATE ACT that lifts the downgrade lock (see
  //  settingsWrite in main.cpp): the author has said "use these".
  //  ONLY A COMPLETE FILE LIFTS IT: every S3 row, the tuning block and the
  //  fixed-feature list, all accepted. While locked, cfg holds DEFAULTS, so
  //  lifting the lock on one line (volume=120) wrote the defaults for
  //  everything else over the newer blob two seconds later - the very loss the
  //  lock exists to prevent. Keys this firmware does not
  //  know (a newer firmware's file) are refused and named, and do not block.
  if (gSettingsLocked) {
    int want = 0, got = 0;
    for (int r = 0; r < gSetCount; r++)
      if (gSet[r].owner == OWN_S3) { want++; if (rowSeen[r]) got++; }
    bool marksOk = mkSeen && mkGot == mkUsed && !mkOffBad;
    bool spursOk = spSeen && !spBad && spGot == (uint32_t)((1u << spUsed) - 1u);
    if (got == want && marksOk && spursOk) {
      gSettingsLocked = false;
      touchedS3 = true;
      Con.println(F("  complete settings file uploaded - the newer stored settings will now be replaced."));
    } else {
      size_t at = strlen(gRestoreNote);
      snprintf(gRestoreNote + at, sizeof(gRestoreNote) - at,
               "%sSETTINGS STILL LOCKED - a complete file is needed (%d of %d settings%s%s); "
               "nothing was saved", at ? "; " : "", got, want,
               marksOk ? "" : ", no tuning marks", spursOk ? "" : ", no fixed features");
      Con.print(F("  portal: "));
      Con.println(gRestoreNote);
    }
  }
  //  THE SOFT LIMITS THE FILE ASKED FOR, against what setGeometry() accepted.
  //  A pair that does not bracket the index is refused there and the old one
  //  written back - it was still counted as applied, with no word.
  int32_t askedMin = cfg.posMin, askedMax = cfg.posMax;
  if (touchedS3)    { applySettings(); settingsTouch(); }
  if (touchedS3 && (cfg.posMin != askedMin || cfg.posMax != askedMax)) {
    size_t at = strlen(gRestoreNote);
    snprintf(gRestoreNote + at, sizeof(gRestoreNote) - at,
             "%sSOFT LIMITS REFUSED (%ld..%ld does not bracket the index) - kept %ld..%ld",
             at ? "; " : "", (long)askedMin, (long)askedMax, (long)cfg.posMin, (long)cfg.posMax);
    Con.print(F("  portal: ")); Con.println(gRestoreNote);
  }

  //  ...AND THE FIT GETS A SAY. gRestoreNote covered parse-level refusals only,
  //  so a file whose marks all parsed cleanly and were then REFUSED BY THE FIT
  //  - three stations that turn the curve back on itself, or that collapse the
  //  dial onto half a megahertz - committed, fell back to the stored line, and
  //  reported "43 settings applied" in green. The reason existed the whole
  //  time, in tuneModelName(), in this translation unit, and was never asked
  //  for on this path. One channel, both levels of refusal.
  //  NOT GATED ON THE FILE BRINGING MARKS. calLow/calHigh are ordinary gSet
  //  rows and they feed the fit's domain, so a file that brings no tuning
  //  block (a pre-v5 backup) can still move them, refuse the
  //  machine's OWN marks, and say nothing. Ask about the state that came out,
  //  whatever the file happened to contain.
  //  Skipped only when the tuning block itself was refused (that note already
  //  says the stored calibration stands); other notes no longer hide it.
  bool tuneRefused = !(mkSeen && mkGot == mkUsed && !mkOffBad) && (mkSeen || mkOffSeen || mkOffBad);
  if (!tuneRefused && cfg.tuneUsed) {
    bool bad = false;
    char tl[160]; tuneStateLine(tl, sizeof(tl), &bad);
    if (bad) {
      size_t at = strlen(gRestoreNote);
      snprintf(gRestoreNote + at, sizeof(gRestoreNote) - at, "%safter restore: %s", at ? "; " : "", tl);
      Con.print(F("  portal: "));
      Con.println(gRestoreNote);
    }
  }
  appendKeyNotes();
  if (gRefused.n || gClamped.n || gNotValid.n || gSkippedA32.n) { Con.print(F("  portal: ")); Con.println(gRestoreNote); }
  if (touchedAudio) gLink.send(MSG_SET_AUDIO, a32cfg.audio);
  if (touchedBt)    gLink.send(MSG_SET_BT,    a32cfg.bt);
  return applied;
}

// ---------------------------------------------------------------------------
//  ACTIONS. Things that happen, rather than things that are.
// ---------------------------------------------------------------------------
static void sendBtCmd(uint8_t c) { gLink.send(MSG_BT_CMD, &c, 1); }
static void sendCalPot(uint8_t which) { gLink.send(MSG_CAL_POT, &which, 1); }

const char *doAction(const char *a, float arg, bool isAdmin) {
  //  CLEARED ON ENTRY, which is the half of the gRestoreNote pattern that did
  //  not get copied. Without it the first refusal latched for the life of the
  //  boot and every later action - "play", "homing", "stopped" - came back
  //  through the warn channel and was painted as a twelve-second failure. A
  //  flag that fires and cannot be cleared is the same defect as a flag that
  //  never fires; this project has now shipped one of each.
  gActionRefused = false;
  //  The guest set. Music, and nothing that can leave the machine broken.
  if (!strcmp(a, "bt.play"))       { sendBtCmd(BTC_PLAY);       return "play"; }
  if (!strcmp(a, "bt.pause"))      { sendBtCmd(BTC_PAUSE);      return "pause"; }
  if (!strcmp(a, "bt.next"))       { sendBtCmd(BTC_NEXT);       return "next"; }
  if (!strcmp(a, "bt.prev"))       { sendBtCmd(BTC_PREV);       return "previous"; }
  if (!strcmp(a, "bt.disconnect")) { sendBtCmd(BTC_DISCONNECT); return "disconnecting"; }
  if (!strcmp(a, "bt.pair"))       { gLink.send(MSG_BT_LOOK);   return "pairing window open"; }

  if (!isAdmin) return nullptr;

  // --- needle -------------------------------------------------------------
  if (!strcmp(a, "needle.stop"))   { Needle::stop();  return "stopped"; }
  //  WHAT WAS REFUSED SAYS SO: these answered "homing" and
  //  "sweeping" whatever the needle did with the request.
  if (!strcmp(a, "needle.home"))   { const char *why = Needle::startHoming(); return why ? refuse(why) : "homing"; }
  if (!strcmp(a, "needle.sweep"))  { return Needle::sweepRange(arg != 0) ? "sweeping"
                                          : refuse("not homed - home the needle first"); }
  //  MEASURE THIS DIAL POSITION. The same thing console 'V' does, reachable
  //  without a cable - once the cabinet is shut the cable is gone (and see
  //  console.h on why no cable is better).
  if (!strcmp(a, "rda.sample"))    { return rdaSampleAction(); }
  //  THE RESCUE HATCH, REACHABLE FROM THE ONLY INTERFACE THAT SURVIVES A CLOSED
  //  CABINET. forceAp() was bound to console 'Y' and nothing else, so the one
  //  way in when the home network is gone could only be opened with the one
  //  cable that is gone. Author's ruling.
  //  ASKED FOR HERE, DONE BY Net::loop() on the other core:
  //  this handler runs on the portal task, and changing the WiFi mode from it
  //  raced the network loop's own state - and tore down the link this answer
  //  still had to travel on. The console 'i' / Console tab shows the result.
  if (!strcmp(a, "net.forceAp")) {
    Net::requestForceAp();
    return "raising the access point - join \"Ambersong\", then http://192.168.4.1/";
  }
  if (!strcmp(a, "tune.drop"))     { return tuneDropAction((int)lrintf(arg)); }
  //  RE-INDEX ON DEMAND. The automatic one fires when a slip exceeds the
  //  absorb window and the operator has stopped tuning, and once per amp-off
  //  period as a routine check; this is the same thing by hand, and it is how
  //  the mechanism gets exercised without waiting for a real slip. A needle
  //  with no zero is homed instead (Needle::startReindex).
  if (!strcmp(a, "needle.reindex")) {
    return Needle::startReindex() ? "re-indexing"
         : refuse("cannot re-index now - a calibration or a bring-up tool has the needle");
  }
  //  FORGET THE FIXED FEATURES, the console 'O' reachable without a cable.
  //  Clears both copies through the one path and persists the clear: the list
  //  is cleared precisely because it is wrong, and a power cycle must not hand
  //  the wrong list straight back.
  if (!strcmp(a, "rda.spurClear")) {
    cfg.spurUsed = 0;
    for (uint8_t i = 0; i < CFG_SPURS; i++) cfg.spur[i] = 0;
    applySettings(); settingsTouch();
    return "fixed-feature list cleared - re-learn it with the set switched OFF";
  }
  if (!strcmp(a, "needle.track"))  { return Needle::track() ? "tracking the tuner"
                                          : refuse("not tracking - the needle has no zero and nothing is homing it; see the fault, then home it"); }
  if (!strcmp(a, "needle.jog")) {
    //  MICRO mode: the portal's jog is for placing the needle by eye, and
    //  smoothness matters more than speed at that job.
    //  CLAMP IN THE MACHINE. settingSet() has this rule and doAction() did not:
    //  hAct passes any finite number straight through, so a hand-made request
    //  jogged an unbounded number of steps - and jogRaw()
    //  busy-waits on the caller's task, so it freezes the portal for
    //  |steps| / hsps seconds. The UI offers 100; 200 leaves headroom and
    //  bounds the freeze to a few seconds.
    int32_t js = (int32_t)arg, want = js;
    if (js >  200) js =  200;
    if (js < -200) js = -200;
    Needle::jogRaw(js, cfg.reapHsps, true);
    //  SAY SO WHEN THE CLAMP BITES. It returned a flat "jogging" whatever it
    //  actually did, so a request for 500 half-steps moved 200 and reported
    //  success - the quiet version of the same defect as a refusal in the
    //  success colour.
    if (want != js) {
      static char jm[72];
      snprintf(jm, sizeof(jm), "jogged %ld half-steps - %ld was clamped to the %d limit",
               (long)js, (long)want, 200);
      return refuse(jm);
    }
    return "jogging";
  }
  //  ONE COPY OF THE RULE, shared with the console m/M. See captureLimit()
  //  in main.cpp - this used to be a second, separately-guarded copy.
  if (!strcmp(a, "needle.limitLow"))  return captureLimit(true);
  if (!strcmp(a, "needle.limitHigh")) return captureLimit(false);
  //  THE NEEDLE'S VERSION OF tune.nudge, and deliberately the same shape.
  //
  //  tune.nudge slides the FREQUENCY curve by a stored offset, so its shape
  //  cannot change and the dial can only shift. This
  //  slides the NEEDLE line by moving posMin and posMax together, for the same
  //  reason: trackTarget() maps dialLow..dialHigh onto posMin..posMax, so
  //  shifting both ends by the same amount moves where the needle points for
  //  every frequency and changes nothing else about the mapping.
  //
  //  It is the honest correction when the needle is consistently a little off
  //  the mark: it says "the printed marks are really N half-steps from where I
  //  captured them", which is exactly what happened if the capture was taken
  //  with the needle a hair off the line.
  //
  //  NOT A CLEARANCE TOOL. Both ends move, so this cannot buy room at one stop
  //  without spending it at the other. To back the low limit away from its
  //  physical stop, set posMin on its own in the Needle tab - that is a
  //  different operation and it changes where the needle points.
  if (!strcmp(a, "needle.nudge")) {
    int32_t d = (int32_t)lrintf(arg);                 // half-steps
    if (d == 0 || d < -200 || d > 200) return refuse("nudge is in half-steps, e.g. 20 or -20");
    int32_t lo = (int32_t)cfg.posMin + d;
    int32_t hi = (int32_t)cfg.posMax + d;
    //  setGeometry() enforces this too and applySettings() writes back whatever
    //  it accepted, but refusing here means the toast can say WHY instead of
    //  reporting a number that did not take.
    if (lo > 0 || hi < 0)
      return refuse("that would move the index outside the needle's own travel - re-set the limits instead");
    cfg.posMin = lo;
    cfg.posMax = hi;
    applySettings(); settingsTouch();
    if (cfg.posMin != lo || cfg.posMax != hi) return refuse("REFUSED - see the console");
    static char msg[104];
    snprintf(msg, sizeof(msg), "both needle limits moved %s%ld half-steps and saved -> %ld .. %ld",
             (d < 0) ? "" : "+", (long)d, (long)cfg.posMin, (long)cfg.posMax);
    return msg;
  }

  //  THE ONE WAY THE HAND CORRECTION IS ERASED (the author: "hand
  //  corrections should never be erased automatically, only manually").
  if (!strcmp(a, "tune.nudgeZero")) {
    if (!cfg.tuneOffset10) return "the dial correction is already 0";
    cfg.tuneOffset10 = 0;
    applySettings(); settingsTouch();
    return "dial correction reset to 0";
  }
  if (!strcmp(a, "tune.nudge")) {
    int32_t d = (int32_t)lrintf(arg * 10.0f);         // MHz -> tenths
    if (d == 0 || d < -50 || d > 50) return refuse("nudge is in MHz, e.g. 0.1 or -0.1");

    //  SLIDES THE CURVE, DOES NOT TILT IT. This used to add d to bandLow and
    //  bandHigh, which was the right shape while the model was a line and the
    //  wrong one now: the offset is a separate stored quantity applied to
    //  whatever fitTuneCurve() produced, so the shape measured by the marks
    //  survives every correction. The offset stays until it is reset by hand
    //  (tune.nudgeZero) - a re-mark does not clear it.
    int32_t o = (int32_t)cfg.tuneOffset10 + d;
    if (o < -200 || o > 200)
      return refuse("that is more than 20 MHz of hand correction - the marks are wrong, re-mark instead");
    cfg.tuneOffset10 = (int16_t)o;
    applySettings(); settingsTouch();

    //  REPORT THE TUNED FREQUENCY, NOT JUST THE DIAL ENDS. The Leditron snaps
    //  its readout to the odd-tenth FM channel grid, so a 0.1 MHz nudge moves
    //  the panel by 0.2 or by 0.0 and never by 0.1 - judge a one-tenth
    //  correction by the panel and you can walk the whole dial off by a tenth
    //  without the panel ever being able to show it. It happened, 2026-09-04:
    //  six presses were remembered as two. This toast is the unsnapped number
    //  at the station actually tuned, on the same screen as the button.
    int32_t now2 = Needle::tuneFreq10();
    int32_t ao   = (o < 0) ? -o : o;
    static char msg[112];
    snprintf(msg, sizeof(msg), "now tuned %ld.%ld MHz (hand offset %s%ld.%ld total)",
             (long)(now2 / 10), (long)(now2 % 10),
             (o < 0) ? "-" : "+", (long)(ao / 10), (long)(ao % 10));
    return msg;
  }

  //  THREE MARKS, FITTED AS A CURVE. Replaces the two-point version, which was
  //  not merely imprecise but STRUCTURALLY WRONG on this set.
  //
  //  Measured 2026-09-04 from three stations the author verified by ear, the
  //  local slope is 0.251 tenths of a MHz per permille between 91.3 and 98.5
  //  and 0.288 between 98.5 and 107.3. A line fitted to two points is exact at
  //  those two points and half a megahertz out in the middle - which is exactly
  //  what happened to him: Mark A/B at 91.3 and 107.3 looked right, mid-band
  //  read 0.5 high, correcting mid-band broke both ends.
  //
  //  The mark only RECORDS a point - the angle now, the frequency typed. All
  //  the fitting is in fitTuneCurve(), which is also where the RDA5807M's
  //  automatic samples land (slots 3 and up), so there is one fit and one
  //  place for it to be wrong. Slots 0-2 are the manual marks; nothing
  //  automatic may take them.
  if (!strncmp(a, "tune.mark", 9) && a[9] >= 'A' && a[9] <= 'C' && a[10] == 0) {
    int slot = a[9] - 'A';
    int f10  = (int)lrintf(arg * 10.0f);
    if (f10 < 870 || f10 > 1085) return refuse("that is not an FM frequency - type it in MHz, e.g. 107.3");
    //  THE SHAFT ITSELF, not permille. See the note on setCurve in needle.h.
    //  NOT FROM A FROZEN ENCODER. The sampler refuses in this state; a hand
    //  mark taken now would sit at a stale position and bend the fit, saved.
    if (!Needle::i2cOk()) return refuse("the tuning encoder is not answering - the mark would be at a stale position");
    int32_t acc = Needle::accumulated();

    //  REFUSE A DUPLICATE POSITION. Two marks at the same spot make the fit
    //  singular; three make it meaningless. ~60 counts is about 0.7 MHz of dial
    //  on this set - close enough to be a mistake, far enough not to obstruct a
    //  deliberate mark.
    for (int i = 0; i < 3; i++) {
      if (i == slot || !(cfg.tuneUsed & (1u << i))) continue;
      int32_t gap = acc - cfg.tuneP[i];
      if (gap < 0) gap = -gap;
      if (gap < 60)
        return refuse("that is the same dial position as another mark - tune well away first");
    }

    //  AND AN AUTOMATIC SAMPLE UNDER THE NEW MARK GOES. The sampler already
    //  refuses to store beside a hand mark (sampleStore); the reverse was
    //  missing, so an older sample within 60 counts stayed in the fit arguing
    //  with the mark - and could never be refreshed, because the sampler finds
    //  the hand mark first there. The mark is the author's.
    int evicted = 0;
    for (int i = 3; i < TUNE_MARKS; i++) {
      if (!(cfg.tuneUsed & (1u << i))) continue;
      int32_t gap = acc - cfg.tuneP[i];
      if (gap < 0) gap = -gap;
      if (gap < 60) { cfg.tuneUsed &= (uint16_t)~(1u << i); cfg.tuneP[i] = 0; cfg.tuneF[i] = 0; evicted++; }
    }

    cfg.tuneP[slot] = acc;
    cfg.tuneF[slot] = (uint16_t)f10;
    cfg.tuneUsed   |= (uint16_t)(1u << slot);
    //  THE HAND CORRECTION IS NOT TOUCHED. It used to be zeroed here; the author
    //  ruled on 2026-09-25 that hand corrections are never erased automatically,
    //  only by hand (tune.nudgeZero).
    applySettings(); settingsTouch();

    //  SAY WHICH MODEL IS RUNNING, WHATEVER THE COUNT. The two-mark fit can be
    //  refused exactly as the three-mark one can - marks 70 counts apart pass
    //  the duplicate guard and then produce a line whose ends leave the FM
    //  band, so fitTuneCurve falls back and both marks are inert. The old
    //  message, "still a straight line until the third", was true and useless:
    //  it IS a straight line, just not his. The [WARN] naming the reason goes
    //  to the serial console, which is behind a shut cabinet; this toast is the
    //  only thing the author can actually read.
    uint8_t n = tuneSampleCount();
    static char msg[144];
    //  THE REACH AT EVERY COUNT, not just at three. A two-mark fit can collapse
    //  the whole dial onto a fraction of a MHz, and the reach is the one number
    //  that shows it - omitting it from the two-mark message hid the failure at
    //  exactly the press that caused it.
    bool bad = false;
    char tl[160]; tuneStateLine(tl, sizeof(tl), &bad);
    snprintf(msg, sizeof(msg), "mark %c taken at %d.%d%s - %s",
             'A' + slot, f10 / 10, f10 % 10,
             evicted ? " (the automatic sample there was dropped)" : "", tl);
    if (bad) gActionRefused = true;   // a REFUSED fit, not merely unmeasured ends
    return msg;
  }

  if (!strcmp(a, "tune.clear")) {
    //  THE HAND CORRECTION IS NOT TOUCHED. It used to be zeroed here; the author
    //  ruled on 2026-09-25 that hand corrections are never erased automatically,
    //  only by hand (tune.nudgeZero).
    cfg.tuneUsed = 0;
    for (int i = 0; i < TUNE_MARKS; i++) { cfg.tuneP[i] = 0; cfg.tuneF[i] = 0; }
    applySettings(); settingsTouch();
    return cfg.tuneOffset10
         ? "marks cleared - back to the stored straight line. Set A, B and C again. "
           "Your dial correction is kept; reset it by hand if it no longer fits."
         : "marks cleared - back to the stored straight line. Set A, B and C again.";
  }

  if (!strcmp(a, "needle.calBand"))   { const char *why = Needle::calStartBand(arg > 0 ? (uint8_t)arg : 3); return why ? refuse(why) : "characterising the index"; }
  if (!strcmp(a, "needle.calAbort"))  { Needle::calAbort(); return "aborted"; }
  //  These set the tuner's mechanical ends. A mark records the shaft position
  //  itself, so re-measuring the ends no longer redefines what a mark MEANS -
  //  but it does still feed the fit's DOMAIN and the fallback line's slope, so
  //  it is not free either. fitTuneCurve() now unions the domain with every
  //  stored mark, which is what makes this safe; an earlier note here claiming
  //  "nothing to guard any more" was wrong on exactly that point.
  //
  //  The toast reports what the curve is doing afterwards, because that is the
  //  only channel the author has once the cabinet is shut.
  if (!strcmp(a, "needle.tuneLow") || !strcmp(a, "needle.tuneHigh")) {
    const bool low = (a[11] == 'L');
    if (!Needle::i2cOk()) return refuse("the tuning encoder is not answering - the end would be captured at a stale position");
    if (low) { cfg.calLow  = Needle::accumulated(); cfg.tunerEndsSet |= ENDS_LOW;  }
    else     { cfg.calHigh = Needle::accumulated(); cfg.tunerEndsSet |= ENDS_HIGH; }
    applySettings(); settingsTouch();
    bool bad = false;
    char tl[160]; tuneStateLine(tl, sizeof(tl), &bad);
    static char msg[200];
    snprintf(msg, sizeof(msg), "tuner %s end set - %s", low ? "low" : "high", tl);
    if (bad) gActionRefused = true;   // a REFUSED fit, not merely unmeasured ends
    return msg;
  }

  // --- pot ----------------------------------------------------------------
  if (!strcmp(a, "pot.min")) { sendCalPot(0); return "pot minimum recorded"; }
  if (!strcmp(a, "pot.max")) { sendCalPot(1); return "pot maximum recorded"; }
  if (!strcmp(a, "pot.ctr")) { sendCalPot(2); return "pot centre recorded"; }

  // --- system -------------------------------------------------------------
  if (!strcmp(a, "bt.forget"))    { gLink.send(MSG_BT_FORGET); return "pairings dropped"; }
  //  THE FLASH WRITE, and the last refusal that was still going out green.
  //  main.cpp's own note on settingsForceFlush() calls this path a data-loss
  //  path in as many words: "Two false reassurances in a row (this, then
  //  'rebooting')". A refusal the author reads in the success colour for 2.6
  //  seconds is a false reassurance whatever the words say.
  if (!strcmp(a, "sys.save"))     { return settingsForceFlush() ? "saved" : refuse(gSaveWhy); }
  if (!strcmp(a, "sys.rebootA32")){ gLink.send(MSG_REBOOT); return "A32 rebooting"; }
  if (!strcmp(a, "sys.getcfg"))   { gLink.send(MSG_GET_CFG); return "re-reading the A32"; }
  //  BLUETOOTH TX POWER, 0..5 = ESP_PWR_LVL_N12..P3 (-12..+3 dBm). DOWN ONLY:
  //  P3 is the controller's default maximum, so this can never raise the radio
  //  above stock. Out of range is refused, not clamped, like sys.dcword.
  if (!strcmp(a, "sys.bttx")) {
    int32_t n = (int32_t)lrintf(arg);
    if (n < 0 || n > 5) return refuse("level must be 0..5 (-12..+3 dBm)");
    uint8_t v = (uint8_t)n;
    gLink.send(MSG_BT_TX, &v, 1);
    return "BT TX power set";
  }
  //  WHICH CONSTANT WORD the DC diagnostic writes - an INDEX 0..6 into the A32's
  //  fixed kDc[] table, never a raw word, so nothing louder than -42 dBFS can be
  //  selected from here. Out-of-range indices are refused, not clamped.
  if (!strcmp(a, "sys.dcword")) {
    int32_t n = (int32_t)lrintf(arg);             // arg is a float, like tune.drop
    if (n < 0 || n > 6) return refuse("index must be 0..6");
    uint8_t i = (uint8_t)n;
    gLink.send(MSG_DC_WORD, &i, 1);
    return "DC word selected";
  }
  if (!strcmp(a, "sys.dctest")) {
    uint8_t on = (arg != 0) ? 1 : 0;
    gLink.send(MSG_TEST_DC, &on, 1);
    return on ? "constant DC on - anything you hear is the fault" : "constant DC off";
  }
  //  IGNORE THE VOLUME POT. An action, not a setting, and for the same reason
  //  as sys.zfloor: it is an experiment. It also must NOT survive a reboot -
  //  a machine that comes back with its front knob dead, days later, with
  //  nothing on the panel to say why, is the silent-boot failure this project
  //  has already had once.
  if (!strcmp(a, "sys.pot")) {
    uint8_t on = (arg != 0) ? 1 : 0;
    gLink.send(MSG_USE_POT, &on, 1);
    return on ? "volume pot ACTIVE"
              : "volume pot IGNORED - set the volume from this page";
  }
  //  DIN PAD DRIVE - an experiment, so an action and not a setting: it must not
  //  survive a reboot into normal use. 3 (strongest) can only change how OFTEN a
  //  slip happens; 0 and 1 could make the line marginal enough to mis-decode for
  //  long stretches, so sweep those only with the author present and the amp down.
  //
  //  sys.sddly WAS HERE from 2026-09-10 to 09-11 and is gone on purpose. It wrote
  //  tx_sd_out_delay remotely with no guard, and that register sent the radio to
  //  full volume; three of its four positions break the stream. Do not restore it.
  if (!strcmp(a, "sys.dindrv")) {
    uint8_t v = (uint8_t)(arg < 0 ? 0 : (arg > 3 ? 3 : arg));
    gLink.send(MSG_DIN_DRIVE, &v, 1);
    return "DIN drive set";
  }
  //  CLOCK-PIN DRIVE - an experiment, an action not a setting. The A32 accepts only
  //  2 and 3 (3, the stiffer, is its boot default since 2026-09-11); a weaker
  //  clock edge could mis-clock both converters for long stretches, so it is
  //  clamped here as well.
  //  MCLK AND FRAME DRIVE AS A PAIR, two digits: 23 = MCLK 2, BCK/LRCK 3. The
  //  single-knob sys.clkdrv below still works and sets both. Four legal values;
  //  anything else is refused rather than clamped, because a silently clamped
  //  pair would make an A/B measure something other than what was asked for.
  if (!strcmp(a, "sys.clkpair")) {
    int32_t n = (int32_t)lrintf(arg);
    if (n != 22 && n != 23 && n != 32 && n != 33)
      return refuse("use 22, 23, 32 or 33 - MCLK digit then BCK/LRCK digit");
    uint8_t v[2] = { (uint8_t)(n / 10), (uint8_t)(n % 10) };
    gLink.send(MSG_CLK_PAIR, v, 2);
    return "clock drive pair set";
  }
  if (!strcmp(a, "sys.clkdrv")) {
    uint8_t v = (uint8_t)(arg < 2 ? 2 : (arg > 3 ? 3 : arg));
    gLink.send(MSG_CLK_DRIVE, &v, 1);
    return "clock-pin drive set";
  }
  //  THE SIGN-EXTENDED TAIL - see audio.cpp. An action for now, like the other
  //  experiments; if it is the fix it becomes a persisted setting, with a reason.
  if (!strcmp(a, "sys.tail")) {
    uint8_t on = (arg != 0) ? 1 : 0;
    gLink.send(MSG_SIGN_TAIL, &on, 1);
    return on ? "sign-extended tail ON" : "sign-extended tail off";
  }
  //  THE ZERO-DATA FLOOR. Deliberately an ACTION and not a saved setting: it is
  //  an experiment, and an experiment that survives a reboot is one whose result
  //  gets attributed to something else a week later. If it turns out to be the
  //  fix it becomes a setting then, with a reason attached.
  if (!strcmp(a, "sys.zfloor")) {
    uint8_t on = (arg != 0) ? 1 : 0;
    gLink.send(MSG_ZERO_FLOOR, &on, 1);
    return on ? "zero-data floor ON - the DAC can no longer analogue-mute"
              : "zero-data floor off";
  }
  if (!strcmp(a, "sys.quiet")) {
    //  Deliberately last-resort-proof: it always comes back on its own.
    Net::requestQuiet(arg > 0 ? (uint16_t)arg : 120);   // see net.forceAp
    return "wifi off - it will return by itself";
  }
  if (!strcmp(a, "disp.test"))    { dispTest = (uint8_t)arg; return "display test"; }

  return nullptr;
}
