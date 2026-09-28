#include "portal.h"
#include "console.h"
#include "net.h"
#include "settings_api.h"
#include "needle.h"
#include "page_gz.h"

#include <WiFi.h>
#include <WebServer.h>
#include <Preferences.h>
#include <Update.h>
#include <esp_system.h>
#include <mbedtls/sha256.h>

namespace Portal {

static WebServer   server(80);
static Preferences ap;
static bool        gRunning  = false;
static bool        gOta      = false;
static volatile uint32_t gLoops  = 0;
static volatile uint32_t gServed = 0;
static TaskHandle_t      gTask   = nullptr;

//  AN INTERRUPTED UPLOAD MUST NOT LEAVE THE MACHINE DARK.
//  An S3 upload sets gOta and calls portalOtaQuiet(), which stops the needle
//  and blanks the display and panel. gOta is cleared by the END handler - but
//  if the connection simply dies,
//  WebServer does not reliably deliver UPLOAD_FILE_ABORTED, and the set would
//  sit motionless and unlit until someone power-cycled it (nearly happened on
//  an over-the-air update that was reset part-way). So 15 s with no upload
//  activity at all means there is no upload.
//  WHICH image is in flight matters: aborting the S3's Update object and
//  un-parking the needle is right for a local write and wrong for a relay,
//  which instead has to tell the A32 to stop.
static uint32_t gOtaTouched = 0;
static bool     gOtaIsA32   = false;
static bool     gOtaChecked = false;
static void otaAlive(bool a32) { gOtaTouched = millis(); gOtaIsA32 = a32; }

static void otaWatchdog() {
  if (!gOta || !gOtaTouched) return;
  if (millis() - gOtaTouched < 15000) return;
  Con.println(F("  [WARN] an update stopped part-way. Releasing the machine."));
  if (gOtaIsA32) {
    a32OtaAbort();
  } else {
    Update.abort();
    portalOtaQuiet(false);
  }
  gOta = false;
  gOtaTouched = 0;
}

// ---------------------------------------------------------------------------
//  USERS
//
//  Four slots. Slot 0 is the author: it cannot be deleted and cannot be demoted,
//  because a machine with no administrator is a machine that needs a cable.
//  Everyone else is a normal user - see the roles note in settings_table.h for
//  what that means, which is "the music and the lights, not the machine".
// ---------------------------------------------------------------------------
#define MAX_USERS 4
struct User {
  char    name[17];
  uint8_t salt[8];
  uint8_t hash[32];
  uint8_t admin;
  uint8_t used;
};
static User users[MAX_USERS];

static void hashPw(const uint8_t salt[8], const char *pw, uint8_t out[32]) {
  mbedtls_sha256_context c;
  mbedtls_sha256_init(&c);
  mbedtls_sha256_starts_ret(&c, 0);
  mbedtls_sha256_update_ret(&c, salt, 8);
  mbedtls_sha256_update_ret(&c, (const uint8_t *)pw, strlen(pw));
  mbedtls_sha256_finish_ret(&c, out);
  mbedtls_sha256_free(&c);
}

static void setPassword(User &u, const char *pw) {
  for (int i = 0; i < 8; i++) u.salt[i] = (uint8_t)(esp_random() & 0xFF);
  hashPw(u.salt, pw, u.hash);
}

static void usersSave() {
  ap.begin("auth", false);
  ap.putBytes("users", users, sizeof(users));
  ap.end();
}

//  THE SHIPPED CREDENTIALS ARE A PLACEHOLDER, NOT A SECRET.
//  They used to be the author's real name and real password, compiled into the
//  image - which was defensible while the tree lived on his disk and stopped
//  being defensible the moment it became a git repository he intends to push.
//  Now the firmware knows only "admin"/"admin", and the portal refuses
//  everything but the change itself until they have been changed (see
//  requireLogin). What
//  replaces them is salted, hashed and stored in NVS, and never exists in the
//  image at any point.
static const char DEFAULT_USER[] = "admin";
static const char DEFAULT_PASS[] = "admin";

static void usersDefaults() {
  memset(users, 0, sizeof(users));
  strncpy(users[0].name, DEFAULT_USER, sizeof(users[0].name) - 1);
  setPassword(users[0], DEFAULT_PASS);
  users[0].admin = 1;
  users[0].used  = 1;
}

//  IS THIS ACCOUNT STILL THE ONE THAT SHIPPED? Asked of the stored hash rather
//  than of a flag, so it needs no field in User (whose size is the NVS record
//  length - adding one would discard every stored account on upgrade) and it
//  cannot fall out of step with reality. It also re-arms by itself if anyone
//  ever sets the defaults back.
static bool credsAreDefault(int i) {
  if (i < 0 || i >= MAX_USERS || !users[i].used) return false;
  if (strcmp(users[i].name, DEFAULT_USER) != 0)  return false;
  uint8_t h[32];
  hashPw(users[i].salt, DEFAULT_PASS, h);
  return memcmp(h, users[i].hash, 32) == 0;
}

static void usersLoad() {
  ap.begin("auth", true);
  size_t n = ap.getBytesLength("users");
  bool ok = false;
  if (n == sizeof(users)) { ap.getBytes("users", users, sizeof(users)); ok = users[0].used; }
  ap.end();
  if (!ok) {
    usersDefaults(); usersSave();
    Con.println(F("  portal: default account created - admin / admin."));
    Con.println(F("          Change it at the first login; nothing else works until you do."));
  }
}

void resetAdmin() {
  usersDefaults();
  usersSave();
  Con.println(F("  portal: ALL ACCOUNTS ERASED, owner reset to admin / admin."));
  Con.println(F("          The portal will refuse everything until they are changed."));
}

static int findUser(const char *name) {
  for (int i = 0; i < MAX_USERS; i++)
    if (users[i].used && !strcmp(users[i].name, name)) return i;
  return -1;
}

// ---------------------------------------------------------------------------
//  SESSIONS. RAM only - a reboot is a logout, which is the behaviour you want
//  from a thing that reboots when someone flips a switch.
// ---------------------------------------------------------------------------
#define MAX_SESS 4
static const uint32_t IDLE_MS = 5UL * 60UL * 1000UL;   // author's requirement
struct Sess { char tok[33]; uint32_t seen; uint8_t user; uint8_t used; };
static Sess sess[MAX_SESS];

static void newToken(char out[33]) {
  static const char *hex = "0123456789abcdef";
  for (int i = 0; i < 32; i += 8) {
    uint32_t r = esp_random();
    for (int j = 0; j < 8; j++) out[i + j] = hex[(r >> (j * 4)) & 0xF];
  }
  out[32] = 0;
}

static int sessFind(const String &tok) {
  if (tok.length() != 32) return -1;
  uint32_t now = millis();
  for (int i = 0; i < MAX_SESS; i++) {
    if (!sess[i].used) continue;
    if (now - sess[i].seen > IDLE_MS) { sess[i].used = 0; continue; }
    if (tok == sess[i].tok) { sess[i].seen = now; return i; }
  }
  return -1;
}

static int sessCreate(uint8_t user) {
  uint32_t now = millis();
  int slot = -1, oldest = 0;
  for (int i = 0; i < MAX_SESS; i++) {
    if (!sess[i].used || now - sess[i].seen > IDLE_MS) { slot = i; break; }
    //  Subtraction, not comparison: raw millis() ordering breaks at the 49-day
    //  wrap, and everything else in this file already does it correctly.
    if ((int32_t)(sess[i].seen - sess[oldest].seen) < 0) oldest = i;
  }
  if (slot < 0) slot = oldest;       // evict the stalest, never refuse a login
  newToken(sess[slot].tok);
  sess[slot].seen = now;
  sess[slot].user = user;
  sess[slot].used = 1;
  return slot;
}

int sessions() {
  uint32_t now = millis();
  int n = 0;
  for (int i = 0; i < MAX_SESS; i++)
    if (sess[i].used && now - sess[i].seen <= IDLE_MS) n++;
  return n;
}

// ---------------------------------------------------------------------------
//  BRUTE-FORCE BACKOFF, per client address, for up to four addresses at once
//  (when all four slots are in use, a new address takes over slot 0).
//  Not a fortress - it makes guessing even a six-character password (the
//  minimum) cost more than an afternoon, which against this threat model is
//  the whole job.
// ---------------------------------------------------------------------------
#define MAX_BAD 4
struct Bad { uint32_t ip; uint8_t fails; uint32_t until; };
static Bad bad[MAX_BAD];

static Bad *badFor(uint32_t ip, bool create) {
  for (int i = 0; i < MAX_BAD; i++) if (bad[i].ip == ip) return &bad[i];
  if (!create) return nullptr;
  int slot = 0;
  for (int i = 0; i < MAX_BAD; i++) if (bad[i].ip == 0) { slot = i; break; }
  bad[slot].ip = ip; bad[slot].fails = 0; bad[slot].until = 0;
  return &bad[slot];
}

static uint32_t lockRemaining(uint32_t ip) {
  Bad *b = badFor(ip, false);
  if (!b || !b->until) return 0;
  uint32_t now = millis();
  if ((int32_t)(now - b->until) >= 0) { b->until = 0; return 0; }
  return (b->until - now) / 1000 + 1;
}

static void noteFail(uint32_t ip) {
  Bad *b = badFor(ip, true);
  if (++b->fails >= 5) {
    //  From the fifth failure, doubling: 1, 2, 4, 8 ... minutes, capped at 64.
    //  Five wrong guesses is a person who has forgotten; twenty is not.
    uint32_t mins = 1u << ((b->fails - 5) > 6 ? 6 : (b->fails - 5));
    b->until = millis() + mins * 60000UL;
    Con.printf("  portal: locked out for %u min after %u failures\n",
                  (unsigned)mins, b->fails);
  }
}

static void noteOk(uint32_t ip) {
  Bad *b = badFor(ip, false);
  if (b) { b->ip = 0; b->fails = 0; b->until = 0; }
}

// ---------------------------------------------------------------------------
//  REQUEST HELPERS
// ---------------------------------------------------------------------------
static int currentSess() {
  String c = server.header("Cookie");
  int at = c.indexOf("amb=");
  if (at < 0) return -1;
  at += 4;
  int end = c.indexOf(';', at);
  if (end < 0) end = c.length();
  return sessFind(c.substring(at, end));
}

static bool isAdminReq(int s) { return s >= 0 && users[sess[s].user].admin; }

//  Every handler that matters starts here. Returns -1 and answers 401 if the
//  caller is not logged in, so a forgotten check fails CLOSED.
//  The only things reachable while the account is still admin/admin: the boot
//  payload and the live state (so the page can render and say why), the change
//  itself, and the way out. Everything else - every setting, every action, the
//  network, the users, the file, OTA, reboot - is refused.
static bool credsStale(int s) {
  return s >= 0 && credsAreDefault(sess[s].user);
}

static bool uriAllowedWhileStale() {
  String u = server.uri();
  return u == "/api/boot" || u == "/api/state" || u == "/api/passwd" || u == "/api/logout";
}

static int requireLogin() {
  int s = currentSess();
  if (s < 0) { server.send(401, "application/json", "{\"e\":\"login\"}"); return -1; }
  //  GATED HERE AND NOT IN EACH HANDLER, deliberately. This project's own
  //  history is "it was written twice before, and the second copy went to the
  //  file path only" - a rule enforced in fifteen places is a rule with
  //  fourteen chances to be forgotten, and the one forgotten would be the one
  //  that mattered. The two OTA routes (their upload and end handlers) do not
  //  use requireLogin() and carry the check explicitly; they are the only
  //  exceptions among the handlers that need a login.
  if (credsStale(s) && !uriAllowedWhileStale()) {
    server.send(403, "application/json", "{\"e\":\"mustchg\"}");
    return -1;
  }
  return s;
}

static void jsonEsc(String &out, const char *in) {
  for (const char *p = in; *p; p++) {
    if (*p == '"' || *p == '\\') { out += '\\'; out += *p; }
    else if ((uint8_t)*p < 0x20)  out += ' ';
    else out += *p;
  }
}

static void sendJson(const String &s) { server.send(200, "application/json", s); }
static void sendOk(const char *msg)   {
  String s = "{\"ok\":1,\"m\":\""; jsonEsc(s, msg); s += "\"}";
  sendJson(s);
}

// ---------------------------------------------------------------------------
//  HANDLERS
// ---------------------------------------------------------------------------
static void hRoot() {
  //  THE PAGE WAS THE WHOLE OF THE SLOWNESS.
  //  Sent uncompressed (21 kB then) by a server that handles one connection
  //  at a time, it measured 0.7 to 3.6 s per load. It is gzipped at build time
  //  now (about 35 kB of HTML goes out as about 13 kB), and the ETag means a
  //  reload usually transfers NOTHING - the browser asks, the radio says 304.
  char etag[24];
  strncpy_P(etag, PAGE_ETAG, sizeof(etag) - 1);
  etag[sizeof(etag) - 1] = 0;

  String want = server.header("If-None-Match");
  if (want.length() && want.indexOf(etag) >= 0) {
    server.send(304, "text/html", "");
    return;
  }

  server.sendHeader("ETag", String("\"") + etag + "\"");
  //  must-revalidate: always ask, but the answer is usually 304 and empty.
  //  A stale portal after a firmware update would be worse than a round trip.
  server.sendHeader("Cache-Control", "no-cache, must-revalidate");
  server.sendHeader("Content-Encoding", "gzip");
  server.send_P(200, "text/html", (PGM_P)PAGE_GZ, PAGE_GZ_LEN);
}

static void hLogin() {
  uint32_t ip = (uint32_t)server.client().remoteIP();
  uint32_t wait = lockRemaining(ip);
  if (wait) {
    String s = "{\"e\":\"locked\",\"s\":" + String(wait) + "}";
    server.send(429, "application/json", s);
    return;
  }
  String u = server.arg("u"), p = server.arg("p");
  int i = findUser(u.c_str());
  if (i >= 0) {
    uint8_t h[32];
    hashPw(users[i].salt, p.c_str(), h);
    //  Constant-time-ish: compare every byte regardless. Over wifi the timing
    //  channel is noise, but the habit costs nothing.
    uint8_t diff = 0;
    for (int k = 0; k < 32; k++) diff |= h[k] ^ users[i].hash[k];
    if (diff == 0) {
      noteOk(ip);
      int s = sessCreate((uint8_t)i);
      String cookie = String("amb=") + sess[s].tok + "; Path=/; HttpOnly; SameSite=Lax; Max-Age=86400";
      server.sendHeader("Set-Cookie", cookie);
      Con.printf("  portal: %s logged in from %s\n",
                    users[i].name, server.client().remoteIP().toString().c_str());
      sendOk("welcome");
      return;
    }
  }
  noteFail(ip);
  server.send(403, "application/json", "{\"e\":\"bad\"}");
}

static void hLogout() {
  int s = currentSess();
  if (s >= 0) sess[s].used = 0;
  server.sendHeader("Set-Cookie", "amb=; Path=/; Max-Age=0");
  sendOk("bye");
}

//  The schema. This is what makes the page generic: the browser has no
//  knowledge of any particular setting, it just renders what the machine says
//  it has. Adding a knob to the table adds it to the portal with no HTML.
//  THE SCHEMA ONLY CHANGES WHEN THE FIRMWARE DOES.
//  It is the bulk of a page load - every row of the table with its label,
//  range and units - and it is identical on every request until this binary
//  is replaced.
//  So it carries a tag, the browser keeps the last copy it was given, and asks
//  for it again only when the tag no longer matches. Admin is in the tag
//  because a normal user gets a different set of read-only flags.
static void schemaTag(char *out, size_t n, bool adm) {
  snprintf(out, n, "%s-%d", FW_VERSION, adm ? 1 : 0);
}

static void buildSchema(String &o, int s) {
  bool adm = isAdminReq(s);
  o.reserve(o.length() + 8192);
  char tag[40]; schemaTag(tag, sizeof(tag), adm);
  o += "{\"tag\":\""; jsonEsc(o, tag);
  o += "\",\"user\":\""; jsonEsc(o, users[sess[s].user].name);
  o += "\",\"admin\":"; o += adm ? 1 : 0;
  o += ",\"fw\":\""; jsonEsc(o, FW_VERSION " (" FW_COMMIT ")");
  o += "\",\"tabs\":[";
  for (int t = 0; t < TAB_COUNT; t++) {
    if (t) o += ',';
    o += '"'; jsonEsc(o, kTabName[t]); o += '"';
  }
  o += "],\"set\":[";
  for (int i = 0; i < gSetCount; i++) {
    const SettingDesc &d = gSet[i];
    if (i) o += ',';
    o += "{\"k\":\""; jsonEsc(o, d.key);
    o += "\",\"l\":\""; jsonEsc(o, d.label);
    o += "\",\"u\":\""; jsonEsc(o, d.unit);
    o += "\",\"t\":"; o += d.tab;
    o += ",\"kind\":"; o += d.kind;
    o += ",\"lo\":"; o += String(d.lo, 3);
    o += ",\"hi\":"; o += String(d.hi, 3);
    o += ",\"st\":"; o += String(d.step, 3);
    o += ",\"ro\":"; o += (d.admin && !adm) ? 1 : 0;
    if (d.kind == K_ENUM) { o += ",\"c\":\""; jsonEsc(o, d.choices); o += '"'; }
    o += '}';
  }
  o += "]}";
}

static void buildValues(String &o) {
  o.reserve(o.length() + 2048);
  o += '{';
  for (int i = 0; i < gSetCount; i++) {
    if (i) o += ',';
    o += '"'; o += gSet[i].key; o += "\":";
    o += String(settingGet(i), 3);
  }
  o += '}';
}

//  A NUMBER, ALL OF IT, or nothing. String::toFloat() is atof(): "107,3" read
//  as 107 - in range, so a tuning mark was stored 0.3 MHz off and every
//  frequency shown afterwards was wrong without a word. The
//  author writes French, so one decimal comma is read as the point; anything
//  else that is not wholly a finite number is refused. An ABSENT value is the
//  caller's business (actions without one pass 0, as they always have).
static bool argNumber(const String &raw, float &out) {
  String t = raw; t.trim(); t.replace(',', '.');
  if (!t.length()) return false;
  const char *c = t.c_str(); char *e = nullptr;
  out = strtof(c, &e);
  return e != c && *e == 0 && isfinite(out);
}

static void hSet() {
  int s = requireLogin(); if (s < 0) return;
  String k = server.arg("k");
  int i = settingFind(k.c_str());
  if (i < 0) { server.send(404, "application/json", "{\"e\":\"nokey\"}"); return; }
  if (gSet[i].admin && !isAdminReq(s)) {
    server.send(403, "application/json", "{\"e\":\"admin\"}");
    return;
  }
  //  REPORT A CLAMP. settingBound() clamps rather than rejects, and throwing
  //  away settingSet()'s answer made that invisible: typing a value outside a
  //  range stored something else and still said "ok". That is how the high
  //  soft limit became unsettable without a single error anywhere.
  //  AN AUDIO-BOARD SETTING NEEDS THE AUDIO BOARD. Without its answer the edit
  //  would go nowhere and read back as a clamp to 0 (2026-09-25).
  if (gSet[i].owner != OWN_S3 && !a32CfgKnown()) {
    server.send(409, "application/json", "{\"e\":\"the audio board is not answering - its settings cannot be changed now\"}");
    return;
  }
  float asked;
  if (!argNumber(server.arg("v"), asked)) {
    server.send(400, "application/json", "{\"e\":\"not a number\"}");
    return;
  }
  settingSet(i, asked);
  float got   = settingGet(i);
  bool  moved = fabsf(got - asked) > 0.0005f;
  String o = "{\"ok\":1,\"v\":" + String(got, 3);
  if (moved) o += ",\"clamped\":1";
  //  AND REPORT WHAT IT DID TO THE CURVE. Two rows on the Needle tab - the
  //  tuner's measured ends - refit the tuning curve when they are written, and
  //  this handler had no way to say so. The identical operation done from the
  //  buttons three rows below returns the whole state; typing it returned a
  //  number. Only for the rows that actually refit, or every settings edit on a
  //  half-calibrated machine would nag.
  if (k == "calLow" || k == "calHigh") {
    const char *note = tuneTroubleNote();
    if (note && *note) { o += ",\"warn\":\""; jsonEsc(o, note); o += "\""; }
  }
  o += "}";
  sendJson(o);
}

static void hAct() {
  int s = requireLogin(); if (s < 0) return;
  String a = server.arg("a");
  float v = 0.0f;
  if (server.hasArg("v") && server.arg("v").length() && !argNumber(server.arg("v"), v)) {
    String o = "{\"ok\":1,\"warn\":\"REFUSED - not a number: ";
    jsonEsc(o, server.arg("v").c_str()); o += "\"}";
    sendJson(o);
    return;
  }
  const char *r = doAction(a.c_str(), v, isAdminReq(s));
  if (!r) { server.send(403, "application/json", "{\"e\":\"no\"}"); return; }
  Con.printf("  portal: %s -> %s\n", users[sess[s].user].name, a.c_str());
  //  A REFUSAL GETS ITS OWN FIELD, exactly as the settings upload does. Every
  //  action used to answer through sendOk(), so "REFUSED - your three marks are
  //  inert" was painted in the success colour and gone in 2.6 seconds.
  if (actionRefused()) {
    String o = "{\"ok\":1,\"warn\":\""; jsonEsc(o, r); o += "\"}";
    sendJson(o);
  } else {
    sendOk(r);
  }
}

static void buildState(String &o) {
  portalStateJson(o);
  //  Portal-side facts the machine does not know about itself.
  o.remove(o.length() - 1);            // drop the closing brace, append, close
  o += ",\"net\":{\"sta\":"; o += Net::isSta() ? 1 : 0;
  o += ",\"ssid\":\""; jsonEsc(o, Net::ssid().c_str());
  o += "\",\"ip\":\""; jsonEsc(o, Net::ip().c_str());
  o += "\",\"rssi\":"; o += Net::rssi();
  o += ",\"ntp\":"; o += Net::ntpFresh() ? 1 : 0;
  o += "},\"sess\":"; o += sessions();
  o += ",\"ota\":"; o += gOta ? 1 : 0;
  o += '}';
}

// ---------------------------------------------------------------------------
//  ONE REQUEST INSTEAD OF THREE.
//
//  Every connection costs this machine a TCP control block for about a minute
//  against a pool of roughly sixteen (Firmware Gospel §8), so the number of round
//  trips is not a detail, it is the budget. Opening the portal used to take
//  FOUR: the document, then the schema, the values and the state. Now it takes
//  two, and the three are composed here.
// ---------------------------------------------------------------------------
static void hBoot() {
  int s = requireLogin(); if (s < 0) return;
  String o; o.reserve(12288);
  //  If the browser says it already has this exact schema, do not send it
  //  again. On a reload that turns roughly 11 kB into roughly 3 kB.
  char tag[40]; schemaTag(tag, sizeof(tag), isAdminReq(s));
  o += "{\"sc\":";
  if (server.arg("sc") == tag) o += "null";
  else                         buildSchema(o, s);
  o += ",\"val\":"; buildValues(o);
  o += ",\"st\":";  buildState(o);
  o += '}';
  sendJson(o);
}

//  Kept individually as well: they are how the machine is inspected from a
//  console or a script without pulling the whole schema every time.
static void hSchema() { int s = requireLogin(); if (s < 0) return; String o; buildSchema(o, s); sendJson(o); }
static void hValues() { int s = requireLogin(); if (s < 0) return; String o; buildValues(o);  sendJson(o); }
//  THE CONSOLE RIDES THE STATE POLL. This server holds about sixteen TCP
//  control blocks and each connection parks one in TIME_WAIT for a minute (see
//  the polling budget in portal.html) - a console with its own once-a-second
//  poll would exhaust it. So an admin page with the Console tab open asks for
//  the log inside the poll it already makes: ?log=<next byte>. No extra
//  connections at all. Non-admin sessions and pages without ?log are served
//  exactly as before.
static void jsonEscLog(String &out, const char *in, size_t n) {
  for (size_t i = 0; i < n; i++) {
    char c = in[i];
    if (c == '"' || c == '\\') { out += '\\'; out += c; }
    else if (c == '\n')          out += "\\n";
    else if ((uint8_t)c < 0x20)  { /* \r and other controls: dropped */ }
    else out += c;
  }
}

static void hState() {
  int s = requireLogin(); if (s < 0) return;
  String o; buildState(o);
  if (server.hasArg("log") && isAdminReq(s)) {
    //  4 KB per poll: a status dump fits, and the copy holds the console lock
    //  for microseconds, not the tens a bigger slice would.
    static char lb[4096];
    uint32_t from  = strtoul(server.arg("log").c_str(), nullptr, 10);
    uint32_t first = 0;
    uint32_t n     = Con.copySince(from, lb, sizeof(lb), first);
    o.remove(o.length() - 1);
    o.reserve(o.length() + n + 64);
    o += ",\"log\":{\"from\":"; o += first;
    o += ",\"next\":";            o += (first + n);
    o += ",\"t\":\"";            jsonEscLog(o, lb, n);
    o += "\"}}";
  }
  sendJson(o);
}

//  KEYS INTO THE CONSOLE, as if typed at the USB port. Admin only: the console
//  can do anything the cable can, including erase the RF calibration (z) and
//  reset the owner account (~). ONE KEY per request - except while a prompt is
//  reading a line (Con.lineWanted), when up to 128 characters go through.
static void hCons() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "text/plain", "admin only"); return; }
  String k = server.arg("k");
  if (k.length() == 0 || k.length() > 128) {
    server.send(400, "application/json", "{\"e\":\"1 to 128 characters\"}"); return;
  }
  if (k.length() > 1 && !Con.lineWanted) {
    server.send(400, "application/json",
                "{\"e\":\"one key at a time - each character is its own command\"}");
    return;
  }
  k += '\n';
  size_t put = Con.inject(k.c_str(), k.length());
  if (put != k.length()) { server.send(503, "application/json", "{\"e\":\"console busy\"}"); return; }
  sendOk("sent");
}

//  Admin only, like the settings file: it exposes nothing dangerous, but it is
//  a diagnostic and there is no reason for it to be part of the ordinary view.
static void hRda() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "text/plain", "admin only"); return; }
  String o; portalRdaJson(o); sendJson(o);
}

// ---- the settings file ----------------------------------------------------
static void hGetFile() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "text/plain", "admin only"); return; }
  //  NOT WHILE LOCKED. The settings in RAM are then the DEFAULTS (a downgrade
  //  found newer stored settings it cannot read), and a file of them looks
  //  complete: uploading it back would lift the lock and write the defaults
  //  over the real calibration.
  if (settingsLocked()) {
    server.send(409, "text/plain",
                "Settings are locked: this firmware is older than the one that saved them, "
                "so what it holds are defaults. Flash the newer firmware to get your settings back.");
    return;
  }
  String txt; settingsToText(txt);
  server.sendHeader("Content-Disposition", "attachment; filename=\"ambersong.txt\"");
  server.send(200, "text/plain", txt);
}

static void hPutFile() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  int n = settingsFromText(server.arg("plain"));
  //  A PARTIAL APPLY IS NOT A SUCCESS. The count says how many keys landed and
  //  says nothing about what was rejected; the tuning marks can be refused
  //  wholesale and the author would read "41 settings applied" in green.
  //  THE REFUSAL TRAVELS IN ITS OWN FIELD ("warn"), not folded into the
  //  success line: inside "m" it was painted in the SUCCESS colour and wiped
  //  itself after 2.6 s, because the HTTP answer is still a 200.
  const char *note = settingsRestoreNote();
  char msg[64];
  snprintf(msg, sizeof(msg), "%d settings applied", n);
  Con.printf("  portal: settings file uploaded, %d applied\n", n);
  if (note && *note) {
    String s = "{\"ok\":1,\"m\":\""; jsonEsc(s, msg);
    s += "\",\"warn\":\""; jsonEsc(s, note); s += "\"}";
    sendJson(s);
  } else {
    sendOk(msg);
  }
}

// ---- users ----------------------------------------------------------------
static void hUsers() {
  int s = requireLogin(); if (s < 0) return;
  String o = "{\"me\":\""; jsonEsc(o, users[sess[s].user].name);
  o += "\",\"u\":[";
  bool adm = isAdminReq(s);
  for (int i = 0, first = 1; i < MAX_USERS; i++) {
    if (!users[i].used) continue;
    if (!adm && i != sess[s].user) continue;      // you see yourself, or all
    if (!first) o += ',';
    first = 0;
    o += "{\"i\":"; o += i;
    o += ",\"n\":\""; jsonEsc(o, users[i].name);
    o += "\",\"a\":"; o += users[i].admin;
    o += '}';
  }
  o += "],\"max\":"; o += MAX_USERS; o += '}';
  sendJson(o);
}

//  Change a password. Anyone may change their OWN; an admin may change anyone's
//  without knowing the old one, which is what makes "I forgot mine" survivable
//  without a cable.
static void hPasswd() {
  int s = requireLogin(); if (s < 0) return;
  int target = server.hasArg("i") ? server.arg("i").toInt() : sess[s].user;
  if (target < 0 || target >= MAX_USERS || !users[target].used) {
    server.send(404, "application/json", "{\"e\":\"nouser\"}"); return;
  }
  bool adm = isAdminReq(s);
  if (target != (int)sess[s].user && !adm) {
    server.send(403, "application/json", "{\"e\":\"admin\"}"); return;
  }
  if (target == (int)sess[s].user) {
    uint8_t h[32];
    hashPw(users[target].salt, server.arg("old").c_str(), h);
    if (memcmp(h, users[target].hash, 32) != 0) {
      server.send(403, "application/json", "{\"e\":\"oldpw\"}"); return;
    }
  }
  String np = server.arg("new");
  if (np.length() < 6) { server.send(400, "application/json", "{\"e\":\"short\"}"); return; }

  //  OPTIONAL RENAME, in the same call and the same save.
  //
  //  An earlier version of this comment said a password-only change "would not
  //  clear the gate". IT WOULD. credsAreDefault() is an AND: it tests the name
  //  AND the hash, so failing EITHER test clears it - and setPassword() reseeds
  //  the salt and rehashes, after which the stored hash cannot match
  //  H(salt,"admin") whatever the name is. The rename is offered because
  //  leaving the account called "admin" is undesirable, not because the gate
  //  requires it. Nobody can be trapped in the form by declining to rename.
  String nn = server.arg("name");
  if (nn.length()) {
    if (nn.length() > sizeof(users[target].name) - 1) {
      server.send(400, "application/json", "{\"e\":\"longname\"}"); return;
    }
    int clash = findUser(nn.c_str());
    if (clash >= 0 && clash != target) {
      server.send(409, "application/json", "{\"e\":\"taken\"}"); return;
    }
    memset(users[target].name, 0, sizeof(users[target].name));
    strncpy(users[target].name, nn.c_str(), sizeof(users[target].name) - 1);
  }

  setPassword(users[target], np.c_str());
  usersSave();
  Con.printf("  portal: credentials changed for %s\n", users[target].name);
  sendOk(nn.length() ? "credentials changed" : "password changed");
}

static void hUserAdd() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  String n = server.arg("n"), p = server.arg("p");
  if (n.length() < 2 || n.length() > 16) { server.send(400, "application/json", "{\"e\":\"name\"}"); return; }
  if (p.length() < 6) { server.send(400, "application/json", "{\"e\":\"short\"}"); return; }
  if (findUser(n.c_str()) >= 0) { server.send(409, "application/json", "{\"e\":\"exists\"}"); return; }
  for (int i = 1; i < MAX_USERS; i++) {
    if (users[i].used) continue;
    memset(&users[i], 0, sizeof(users[i]));
    strncpy(users[i].name, n.c_str(), sizeof(users[i].name) - 1);
    setPassword(users[i], p.c_str());
    //  New accounts are NORMAL. There is exactly one administrator, and it is
    //  slot 0. Nothing in the portal can promote anyone.
    users[i].admin = 0;
    users[i].used  = 1;
    usersSave();
    Con.printf("  portal: user %s added\n", users[i].name);
    sendOk("user added");
    return;
  }
  server.send(507, "application/json", "{\"e\":\"full\"}");
}

static void hUserDel() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  int i = server.arg("i").toInt();
  //  Slot 0 is the author and is not deletable, ever. Losing it means a cable.
  if (i <= 0 || i >= MAX_USERS || !users[i].used) {
    server.send(400, "application/json", "{\"e\":\"no\"}"); return;
  }
  Con.printf("  portal: user %s removed\n", users[i].name);
  //  Drop any session belonging to them, immediately.
  for (int k = 0; k < MAX_SESS; k++) if (sess[k].used && sess[k].user == i) sess[k].used = 0;
  memset(&users[i], 0, sizeof(users[i]));
  usersSave();
  sendOk("user removed");
}

// ---- network --------------------------------------------------------------
static void hNetGet() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  String ssid, pass, ntp, tz;
  Net::getWifi(ssid, pass);
  Net::getNtp(ntp, tz);
  String o = "{\"ssid\":\""; jsonEsc(o, ssid.c_str());
  o += "\",\"ntp\":\""; jsonEsc(o, ntp.c_str());
  o += "\",\"tz\":\"";  jsonEsc(o, tz.c_str());
  o += "\"}";
  sendJson(o);
}

static void hNetSet() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  if (server.hasArg("ssid")) {
    String ssid = server.arg("ssid"), pass = server.arg("pass");
    //  A BLANK PASSWORD MEANS "KEEP"; A BLANK SSID MEANS A MISTAKE.
    //  The form prefills itself from /api/net, and on this machine a timed-out
    //  request is routine - so an empty field is far more likely to be a
    //  failed prefill than an intention. Storing it drops the home network
    //  and puts the radio on its rescue AP for no reason.
    if (ssid.length() == 0) {
      server.send(400, "application/json", "{\"e\":\"the network name is empty\"}");
      return;
    }
    if (pass.length() == 0) { String o, p; Net::getWifi(o, p); pass = p; }
    Net::setWifi(ssid, pass);
  }
  if (server.hasArg("ntp")) Net::setNtp(server.arg("ntp"), server.arg("tz"));
  sendOk("network saved - the radio joins it in a few seconds");
}

static void hReboot() {
  int s = requireLogin(); if (s < 0) return;
  if (!isAdminReq(s)) { server.send(403, "application/json", "{\"e\":\"admin\"}"); return; }
  //  FLUSH FIRST. Writes are debounced two seconds and refused while the
  //  needle moves, so "change something, press Reboot" lost it silently.
  //  AND DO NOT REBOOT OVER AN UNSAVED CHANGE: the needle is
  //  asked to stop - a fresh mark refits the curve and sets it moving, which is
  //  exactly when a save is refused - and the save is tried again. If it still
  //  will not go, the reboot is refused and the page says why. A lock is not a
  //  reason to refuse: nothing is written while locked, so nothing is lost.
  //  ON TRIAL, A REBOOT IS A ROLLBACK: the bootloader takes the previous image
  //  back, and nothing is saved during the trial by design (main.cpp,
  //  settingsWrite) - so there is nothing to wait for, and refusing would take
  //  away the one button that undoes a bad update.
  if (s3ImageOnTrial()) {
    sendOk("rebooting - this firmware was still on trial, so the previous firmware comes back");
    delay(200);
    ESP.restart();
    return;
  }
  if (!settingsFlushNow() && !settingsLocked()) {
    Needle::stop();
    bool ok = false;
    for (int i = 0; i < 15 && !ok; i++) { delay(100); ok = settingsFlushNow(); }
    if (!ok) {
      portalNeedleResume();
      String o = "{\"e\":\"not rebooting - "; jsonEsc(o, settingsSaveWhy()); o += "\"}";
      server.send(409, "application/json", o);
      return;
    }
  }
  sendOk("rebooting");
  delay(200);
  ESP.restart();
}

// ---- OTA, the S3's own image ---------------------------------------------
//
//  The audio lives on the OTHER MCU, so the radio KEEPS PLAYING through an S3
//  update. What cannot survive it is the needle and the display, because a
//  flash write holds the cache disabled: so the needle stops and the display
//  and panel go dark for the duration (portalOtaQuiet), and come back on the
//  reboot - or at once, if the upload fails. Dark beats torn.
static bool gOtaRefused = false;

static void hOtaEnd() {
  int s = currentSess();
  if (s < 0 || !isAdminReq(s)) { server.send(403, "text/plain", "admin only"); return; }
  //  requireLogin() is not on this path - see the note there.
  if (credsStale(s)) { server.send(403, "text/plain", "change the default password first"); return; }
  if (gOtaRefused) {
    gOtaRefused = false;
    server.send(409, "application/json", "{\"e\":\"this firmware is still on trial - retry in a minute\"}");
    return;
  }
  if (Update.hasError()) {
    gOta = false; portalOtaQuiet(false);
    server.send(500, "application/json", "{\"e\":\"flash\"}");
    return;
  }
  server.sendHeader("Connection", "close");
  //  CLEAR gOta BEFORE FLUSHING, and say so if the flush still fails.
  //  The image is fully written by the time this handler runs, but nothing on
  //  the success path ever cleared gOta - UPLOAD_FILE_END only calls
  //  Update.end(). settingsFlushNow() goes through writeSafe(), which requires
  //  !Portal::otaActive(), so the flush could NEVER succeed here: any setting
  //  changed within the save debounce before an upload was silently lost on the
  //  reboot, and the console blamed the needle for it.
  gOta = false;
  if (!settingsFlushNow())
    Con.println(F("  portal: [WARN] settings could not be saved before the OTA reboot."));
  server.send(200, "application/json", "{\"ok\":1,\"m\":\"rebooting into the new firmware\"}");
  delay(300);
  ESP.restart();
}

static void hOtaUpload() {
  HTTPUpload &up = server.upload();
  otaAlive(false);
  int s = currentSess();
  if (s < 0 || !isAdminReq(s)) return;
  if (credsStale(s)) return;              //  requireLogin() is not on this path

  if (up.status == UPLOAD_FILE_START) {
    //  NOT WHILE THIS IMAGE IS STILL ON TRIAL - it would be written over the
    //  slot a rollback returns to (main.cpp, ROLLBACK). Nothing is quieted;
    //  the bytes still arrive and are dropped, and hOtaEnd says why.
    gOtaRefused = s3ImageOnTrial();
    if (gOtaRefused) {
      Con.println(F("  portal: S3 update refused - this image is still on trial."));
      return;
    }
    Con.printf("  portal: OTA of the S3 starting, %s\n", up.filename.c_str());
    //  CHECK WHICH CHIP THE IMAGE IS FOR. The two file pickers sit next to each
    //  other in the System tab and the builds look identical; picking the wrong
    //  one would write an ESP32 image into the S3's slot. Byte 12 of an esp
    //  image header is the chip id - 0x0000 ESP32, 0x0009 ESP32-S3 - and the
    //  first buffer is already in hand here.
    gOta = true;
    gOtaChecked = false;
    portalOtaQuiet(true);
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) Update.printError(Con);
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!gOtaChecked && up.currentSize >= 14) {
      gOtaChecked = true;
      uint16_t chip = (uint16_t)up.buf[12] | ((uint16_t)up.buf[13] << 8);
      if (up.buf[0] == 0xE9 && chip != 0x0009) {
        Con.printf("  [FAIL] that image is for chip id 0x%04x, not the S3 (0x0009).\n", chip);
        Update.abort();
        gOta = false;
        portalOtaQuiet(false);
        return;
      }
    }
    if (!gOta) return;
    if (Update.write(up.buf, up.currentSize) != up.currentSize) Update.printError(Con);
  } else if (up.status == UPLOAD_FILE_END) {
    if (Update.end(true)) Con.printf("  portal: OTA wrote %u bytes.\n", (unsigned)up.totalSize);
    else                  Update.printError(Con);
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    Update.abort();
    gOta = false;
    portalOtaQuiet(false);
    Con.println(F("  portal: OTA aborted."));
  }
}

bool otaActive() { return gOta; }

//  Deliberately NOT part of buildSchema(). The schema is cached client-side and
//  revalidated by a tag of FW_VERSION plus the admin bit (schemaTag), neither of
//  which moves when a password changes - so a mustchg carried in the schema
//  would be answered from cache and the form would never go away.
bool mustChangeCreds() {
  int s = currentSess();
  return credsStale(s);
}


// ---- OTA, the A32 image relayed over the UART -----------------------------
//
//  The S3 is only a pipe here: nothing is stored, each 1 kB frame is
//  acknowledged by the A32 before the next goes out, and the whole thing is
//  checksummed at the end. The needle and display are NOT parked - this MCU is
//  not the one writing flash. The audio stops, because the other one is.
static bool a32Failed = false;

static void hOtaA32End() {
  int s = currentSess();
  if (s < 0 || !isAdminReq(s)) { server.send(403, "text/plain", "admin only"); return; }
  //  requireLogin() is not on this path - see the note there.
  if (credsStale(s)) { server.send(403, "text/plain", "change the default password first"); return; }
  gOta = false;
  if (a32Failed) {
    String o = "{\"e\":\""; jsonEsc(o, a32OtaError()); o += "\"}";
    server.send(500, "application/json", o);
    return;
  }
  char msg[80];
  snprintf(msg, sizeof(msg), "%lu bytes sent, the A32 is rebooting",
           (unsigned long)a32OtaSent());
  sendOk(msg);
}

static void hOtaA32Upload() {
  HTTPUpload &up = server.upload();
  otaAlive(true);
  int s = currentSess();
  if (s < 0 || !isAdminReq(s)) return;
  if (credsStale(s)) return;              //  requireLogin() is not on this path

  if (up.status == UPLOAD_FILE_START) {
    Con.printf("  portal: OTA of the A32 starting, %s\n", up.filename.c_str());
    gOta = true;
    gOtaChecked = false;
    a32Failed = !a32OtaBegin();
  } else if (up.status == UPLOAD_FILE_WRITE) {
    if (!gOtaChecked && up.currentSize >= 14) {
      gOtaChecked = true;
      uint16_t chip = (uint16_t)up.buf[12] | ((uint16_t)up.buf[13] << 8);
      if (up.buf[0] == 0xE9 && chip != 0x0000) {
        Con.printf("  [FAIL] that image is for chip id 0x%04x, not the A32 (0x0000).\n", chip);
        a32OtaAbort();
        a32Failed = true;
        a32OtaSetError("that image is not for the audio board (wrong chip) - nothing was written");
      }
    }
    if (!a32Failed && !a32OtaChunk(up.buf, up.currentSize)) a32Failed = true;
  } else if (up.status == UPLOAD_FILE_END) {
    if (!a32Failed && !a32OtaEnd()) a32Failed = true;
    if (a32Failed) Con.printf("  [FAIL] A32 OTA: %s\n", a32OtaError());
  } else if (up.status == UPLOAD_FILE_ABORTED) {
    a32OtaAbort();
    a32Failed = true;
    gOta = false;
  }
}

// ---------------------------------------------------------------------------
//  SPECULATIVE CONNECTIONS ARE THE OTHER HALF OF THE LATENCY PROBLEM.
//
//  This server handles ONE connection at a time, and browsers open several in
//  advance of needing them. WebServer accepts such a socket and then waits
//  HTTP_MAX_DATA_WAIT - five seconds, a hard #define in the library - for a
//  request that is never coming. Every real request behind it waits too. Three
//  preconnected sockets is a fifteen-second login, which is exactly what the
//  author measured while curl, which opens exactly one connection, saw none of
//  it.
//
//  The constant cannot be overridden from outside the library, but the client
//  can be reached: WebServer::client() hands back the one it is holding. A
//  connection that has been open this long without saying anything is not a
//  request, so it is dropped and the queue moves on.
//
//  RAISED FROM 750 ms, AND IT NEARLY COST THE BOARD.
//  750 ms is only safe while the link is fast. When power save came back after
//  an over-the-air update, round trips went to a second or more, and this timer
//  started killing REAL requests that had merely not arrived yet - connections
//  reset at six seconds, and the portal became unreachable over the only route
//  left to it. A watchdog that fires on slowness is a watchdog that removes
//  your way back in.
//
//  3 s is still well inside the library's own 5 s HTTP_MAX_DATA_WAIT, so it
//  still does its job on a speculative socket, with room for a bad minute on
//  the network.
static const uint32_t IDLE_SOCKET_MS = 3000;

uint32_t loops()  { return gLoops; }
uint32_t served() { return gServed; }
uint32_t stackFreeBytes() { return gTask ? uxTaskGetStackHighWaterMark(gTask) : 0; }

static void serverTask(void *) {
  uint32_t silentSince = 0;
  for (;;) {
    gLoops++;
    otaWatchdog();
    if (Net::connected()) {
      bool had = server.client().connected();
      server.handleClient();
      if (had) gServed++;

      //  client() returns by value in this core, so stopping the copy would
      //  close nothing. The copy shares the same underlying socket handle,
      //  which is what stop() acts on - verified by the latency going away.
      //  NEVER while an image is being written. An upload has quiet moments,
      //  and dropping the socket in one of them would abort the update.
      WiFiClient c = gOta ? WiFiClient() : server.client();
      if (c.connected() && c.available() == 0) {
        if (!silentSince) silentSince = millis();
        else if (millis() - silentSince > IDLE_SOCKET_MS) {
          c.stop();
          silentSince = 0;
        }
      } else {
        silentSince = 0;
      }
    }
    //  One tick, always. A busy-spin here starves core 0's idle task and the
    //  watchdog reboots the machine - which is exactly how the needle emitter
    //  brought this board down on 2026-08-31.
    vTaskDelay(1);
  }
}

void begin() {
  usersLoad();
  memset(sess, 0, sizeof(sess));
  memset(bad,  0, sizeof(bad));

  const char *hdrs[] = { "Cookie", "If-None-Match" };
  server.collectHeaders(hdrs, 2);

  server.on("/",                  HTTP_GET,  hRoot);
  server.on("/api/login",         HTTP_POST, hLogin);
  server.on("/api/logout",        HTTP_POST, hLogout);
  server.on("/api/boot",          HTTP_GET,  hBoot);
  server.on("/api/schema",        HTTP_GET,  hSchema);
  server.on("/api/values",        HTTP_GET,  hValues);
  server.on("/api/set",           HTTP_POST, hSet);
  server.on("/api/act",           HTTP_POST, hAct);
  server.on("/api/state",         HTTP_GET,  hState);
  server.on("/api/rda",           HTTP_GET,  hRda);
  server.on("/api/cons",          HTTP_POST, hCons);
  server.on("/api/settings.txt",  HTTP_GET,  hGetFile);
  server.on("/api/settings.txt",  HTTP_POST, hPutFile);
  server.on("/api/users",         HTTP_GET,  hUsers);
  server.on("/api/passwd",        HTTP_POST, hPasswd);
  server.on("/api/user/add",      HTTP_POST, hUserAdd);
  server.on("/api/user/del",      HTTP_POST, hUserDel);
  server.on("/api/net",           HTTP_GET,  hNetGet);
  server.on("/api/net",           HTTP_POST, hNetSet);
  server.on("/api/reboot",        HTTP_POST, hReboot);
  server.on("/api/ota/s3",        HTTP_POST, hOtaEnd, hOtaUpload);
  server.on("/api/ota/a32",       HTTP_POST, hOtaA32End, hOtaA32Upload);
  //  A browser asks for this on every page load. Answering 302 sent it back to
  //  fetch the whole page (about 13 kB gzipped) a second time, on a server that handles ONE
  //  connection at a time. 204 costs nothing and is the honest answer.
  server.on("/favicon.ico",       HTTP_GET, []() { server.send(204, "text/plain", ""); });
  //  ANY UNKNOWN PATH GOES TO THE PAGE. On the rescue access point this is also
  //  what makes the captive portal work: a phone's "is there internet?" check
  //  (generate_204, hotspot-detect.html, connecttest.txt...) is answered with a
  //  redirect instead of what it expects, so the phone opens the login page by
  //  itself. There the redirect names the AP's own address, because the check
  //  arrives under someone else's host name (net.cpp, gDns).
  server.onNotFound([]() {
    String to = Net::isSta() ? String("/") : "http://" + WiFi.softAPIP().toString() + "/";
    server.sendHeader("Location", to);
    server.send(302, "text/plain", "");
  });

  server.begin();
  gRunning = true;

  //  CORE 0, with WiFi - and with the needle's step emitter, at priority 19
  //  against this task's 3 (see loops() in portal.h). Core 1 has the display
  //  ISR, the needle task and loop().
  xTaskCreatePinnedToCore(serverTask, "portal", 8192, nullptr, 3, &gTask, 0);
  Con.println(F("  portal: listening on port 80."));
}

bool running() { return gRunning; }

}  // namespace Portal
