// ============================================================================
//  AMBERSONG  -  THE SETTINGS TABLE
//
//  ONE description of every adjustable value in the machine. It generates the
//  schema the portal page builds its controls from, the JSON the browser reads
//  and writes, the POST handler and the downloadable settings file.
//
//  This exists because the alternative was editing four places for every new
//  knob and watching them drift apart. The author asked for "all the knobs and
//  sliders I can"; a table is what makes that affordable rather than a chore.
//
//  The table lives in settings_table.h, which main.cpp includes next to the
//  structs it points into. Only the ACCESS is declared here, so portal.cpp
//  never sees `cfg` at all.
// ============================================================================
#pragma once
#include <Arduino.h>

//  Tabs are the portal's menus. Order here is the order on screen.
enum : uint8_t {
  TAB_AUDIO = 0, TAB_DISPLAY, TAB_LIGHTS, TAB_NEEDLE, TAB_BT, TAB_SYSTEM,
  TAB_COUNT
};
extern const char *const kTabName[TAB_COUNT];

//  How the value is STORED, and how it is PRESENTED. They are different
//  questions: ifOffset is an int16 in 50 kHz units in the struct and a MHz
//  figure on screen.
enum : uint8_t { SU8 = 0, SU16, SI16, SI32, SF32 };
enum : uint8_t { K_INT = 0, K_BOOL, K_FLOAT, K_ENUM };

//  Which peer owns the value, so a change knows where to be sent.
enum : uint8_t { OWN_S3 = 0, OWN_A32_AUDIO, OWN_A32_BT };

struct SettingDesc {
  const char *key;       // stable, machine-readable; the settings file uses it
  const char *label;     // what a person reads
  const char *unit;      // "ms", "dB", "" ...
  uint8_t     tab;
  uint8_t     store;     // SU8 / SU16 / SI16 / SI32 / SF32
  uint8_t     kind;      // K_INT / K_BOOL / K_FLOAT / K_ENUM
  float       lo, hi;    // slider range, and a HARD clamp on the way in
  float       step;
  //  STORED value x scale = SHOWN value, and lo/hi/step are in SHOWN units.
  //  Gains live on the wire as tenths of a dB and taper as gamma x 10, because
  //  integers survive a struct that gets memcpy'd across a UART. A person
  //  should still see "+8.0 dB" and "2.5", so the conversion belongs here and
  //  nowhere else.
  float       scale;
  const char *choices;   // K_ENUM only: comma-separated, index = value
  void       *ptr;
  uint8_t     owner;     // OWN_*
  uint8_t     admin;     // 1 = admin only. See the note on roles in settings_table.h.
};

extern const SettingDesc gSet[];
extern const int         gSetCount;

float  settingGet(int i);
bool   settingSet(int i, float v);          // clamps, writes, then applies and marks dirty (S3)
                                            // or pushes to the A32; false if refused or unchanged
int    settingFind(const char *key);        // -1 if unknown

//  The plain-text form the author asked for: recoverable by hand, and readable
//  in ten years by something that is not this firmware.
void   settingsToText(String &out);
int    settingsFromText(const String &in);  // returns how many keys were applied
//  Empty unless the last settingsFromText() refused something the caller must
//  tell the user about. The portal is the only interface once the cabinet is
//  shut, so a refusal that only reaches the serial console has not been
//  reported at all.
const char *settingsRestoreNote();
//  Why the last settingsFlushNow() refused, and whether writes are locked (a
//  downgrade found newer stored settings - see settingsWrite in main.cpp).
const char *settingsSaveWhy();
bool        settingsLocked();
//  Has the A32 answered with its settings since it last went silent?
bool        a32CfgKnown();
//  Put the needle back to work after the portal stopped it for nothing (a
//  refused reboot). Carried out by loop(), once the stop has landed.
void        portalNeedleResume();
//  True when the action just executed by doAction() was a refusal rather than
//  an acknowledgement, so the portal can render it as one.
bool        actionRefused();
//  Non-empty when the tuning curve is in a state the author needs to see: the
//  marks refused outright, the quadratic demoted to a line, or a tuner end
//  still unmeasured. All three are checked; an earlier version documented all
//  three and tested only the first.
const char *tuneTroubleNote();

//  What the last RDA measurement did, in one line. The console is a cable and
//  the cabinet gets shut, so a measurement that only printed itself has not
//  reported anything. Empty until the first one runs.
const char *sampleNote();
bool        sampleBusy();

//  Actions are not settings - they DO something rather than hold a value.
//  Same admin split. Returns a short human-readable result, or nullptr if the
//  name is unknown or needs admin and the caller is not.
const char *doAction(const char *name, float arg, bool isAdmin);

//  Write dirty settings to flash NOW, skipping the debounce but never the
//  safety. Returns true only when nothing unsaved is left: false while the
//  settings are locked, while this image is on trial, while writeSafe()
//  refuses (needle moving, calibration or update running), on a flash
//  failure, or when the settings kept changing; settingsSaveWhy() says which.
//  Exposed because portal.cpp restarts the machine and must not do that on
//  top of unsaved settings.
bool settingsFlushNow();

// ---------------------------------------------------------------------------
//  What main.cpp provides to the portal, so portal.cpp never touches a struct.
// ---------------------------------------------------------------------------
void portalStateJson(String &out);   // the live dashboard, as JSON

//  The last RDA sweep, bin by bin, so a measurement can be checked from the
//  portal instead of from a serial cable that is not plugged in.
void portalRdaJson(String &out);

//  Park everything mechanical and optical before a long flash write. An OTA
//  holds the flash cache disabled for seconds at a time, which is exactly the
//  load phase G measured as the one thing this machine cannot absorb: the step
//  emitter stalls and the display tears. So the needle stops and the panel and
//  display go dark for the duration, deliberately, rather than misbehaving.
void portalOtaQuiet(bool on);

//  True while this S3 image is on trial after an OTA (main.cpp, ROLLBACK):
//  an S3 upload is refused until it is confirmed.
bool s3ImageOnTrial();

// ---------------------------------------------------------------------------
//  RELAYING A FIRMWARE IMAGE TO THE A32 OVER THE UART.
//  The S3 cannot reset the A32 (Bible §3: its EN is not wired), so this is
//  the only wireless route to that MCU.
//  Implemented in main.cpp, where the link and its acknowledgements live.
// ---------------------------------------------------------------------------
bool        a32OtaBegin();
bool        a32OtaChunk(const uint8_t *data, size_t n);
bool        a32OtaEnd();
void        a32OtaAbort();
void        a32OtaSetError(const char *why);   //  a refusal decided on the portal side
const char *a32OtaError();
uint32_t    a32OtaSent();
