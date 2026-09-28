#include "net.h"
#include "console.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <ESPmDNS.h>
#include <DNSServer.h>
#include <Preferences.h>
#include <time.h>
#include <esp_sntp.h>

namespace Net {

static Preferences np;
static String  gSsid, gPass, gNtp, gTz;
static bool    gSta        = false;
static bool    gApUp       = false;
static uint32_t lastTry    = 0;
static uint32_t joinStart  = 0;
static bool    joining     = false;
static bool    mdnsUp      = false;
static bool    ntpOk       = false;
static uint32_t lastNtpOk  = 0;
static uint32_t pendingJoin = 0;    // 0 = nothing scheduled
//  A FORCED access point holds this long before the retry loop may take it
//  down - longer while anyone is connected, by the rule already in the loop.
//  Bounded on purpose: Y can arrive from the portal, and an unbounded hold would
//  leave the machine off the home network until somebody power-cycles it.
static const uint32_t FORCE_HOLD_MS = 600000;
static bool     gForced    = false;
static uint32_t forcedAt   = 0;
static uint32_t quietUntil  = 0;    // 0 = transmitting normally

//  TRANSMIT POWER, quarter-dBm (80 = 20.0 dBm, the part's maximum). THIS
//  FIRMWARE CAPS IT AT 60 (15 dBm): measured 2026-09-24, nothing could see this
//  board at 20 dBm and everything could at 15 - see TX_LADDER. The author keeps
//  that ceiling as a firmware choice. Default and floor: 8 (2.0 dBm).
//
//  WHY THE FLOOR IS SO LOW. Measured 2026-09-07: the receiver was fine - a
//  -88 dBm network in a scan, probe requests logged at -49 dBm - while three
//  independent receivers heard NOTHING from the transmitter at 20.0, 15.0,
//  11.0, 8.5, 7.0 or 5.0 dBm (same binary, same boot, this value alone
//  changed). At 2.0 dBm: 88 % at one metre, and it associated with the house
//  gateway. Good below a threshold and silent above it pointed at the supply
//  sagging under the transmit burst, not a damaged amplifier; the supply as
//  built is Bible §4. No brownout reached the log: IDF's own messages go to
//  UART0 (GPIO43/44) and this console is the native USB port (Bible §4).
//
//  Re-applied after EVERY mode change: esp_wifi_set_mode() restarts the
//  interfaces and does not carry this with it.
static uint8_t  gTxQ = 8;

//  THE LADDER. Quarter-dBm: 8 is 2.0 dBm, 60 is 15.0 dBm.
//
//  Ascending and wrapping. A join can fail for two OPPOSITE reasons and this
//  code cannot tell them apart from the inside: too much power and nothing is
//  radiated at all (what this board did above 2 dBm on 2026-09-07, see gTxQ),
//  and too little cannot reach a distant access point. Stepping through every
//  rung covers both. Whichever one associates is kept and saved, so the next
//  boot starts where it left off and the walk normally happens once.
//
//  It starts at the BOTTOM because 2.0 dBm is the rung that worked when the
//  higher ones did not, and it is ample for a nearby access point - there is
//  no prize for shouting. A distant router simply makes it climb on its first
//  failure and settle higher.
//  THE TOP RUNG IS 15 dBm, NOT 20. Measured 2026-09-24: the forced access point
//  at 20.0 dBm was seen by NOTHING - not the author's phone, not a PC adapter -
//  for 90 s; one step down, at 15.0 dBm, both saw it within 22 s, at 76 %, and a
//  PC joined it and logged into the portal. The house link at 20 dBm had passed
//  a short ping test, but a link surviving is weak evidence and an AP nobody can
//  see is not. So 20 is gone from the ladder, or a failed join would climb back
//  up into the silence.
static const uint8_t TX_LADDER[] = { 8, 20, 28, 34, 44, 60 };
static const uint8_t TX_RUNGS    = sizeof(TX_LADDER) / sizeof(TX_LADDER[0]);

static uint8_t ladderNext(uint8_t q) {
  //  Nearest rung to what we are on, then the one above it, wrapping.
  uint8_t best = 0, bestD = 255;
  for (uint8_t i = 0; i < TX_RUNGS; i++) {
    uint8_t d = (uint8_t)(TX_LADDER[i] > q ? TX_LADDER[i] - q : q - TX_LADDER[i]);
    if (d < bestD) { bestD = d; best = i; }
  }
  return TX_LADDER[(best + 1) % TX_RUNGS];
}

static void applyTxPower() {
  if (gTxQ < 8)  gTxQ = 8;                    // 2.0 dBm, the floor that works here
  if (gTxQ > 60) gTxQ = 60;                  // 15.0 dBm - see TX_LADDER
  esp_wifi_set_max_tx_power((int8_t)gTxQ);
}

//  The rescue network. OPEN, by the author's decision: "The rescue
//  ap shouldn't have the user search for a passphrase when he's only trying to
//  make his device work again." The portal's own login still guards every
//  action. The cost: nothing on this network is encrypted, so the portal login
//  and the home WiFi password typed during a rescue cross the air in clear -
//  for as long as the AP is up, within radio range. Firmware Gospel §8.
static const char *AP_SSID = "Ambersong";

//  THE CAPTIVE PORTAL. While the AP is up, every name a phone looks up answers
//  with the AP's own address, so the phone's "is there internet?" check lands
//  on this portal and the phone opens the login page by itself. Stopped the
//  moment the AP goes down; it never runs on the home network.
static DNSServer gDns;
static bool      gDnsUp = false;

//  20 s is generous for a join. Beyond that something is actually wrong and the
//  right answer is to put the portal up on our own AP rather than keep waiting.
static const uint32_t JOIN_MS  = 20000;
//  Retry the home network every two minutes while running on the AP.
static const uint32_t RETRY_MS = 120000;

static void loadPrefs() {
  np.begin("net", true);
  gSsid = np.getString("ssid", "");
  gPass = np.getString("pass", "");
  gNtp  = np.getString("ntp",  "pool.ntp.org");
  gTz   = np.getString("tz",   "EST5EDT,M3.2.0,M11.1.0");   // America/Montreal
  np.end();
}

static void savePrefs() {
  np.begin("net", false);
  np.putString("ssid", gSsid);
  np.putString("pass", gPass);
  np.putString("ntp",  gNtp);
  np.putString("tz",   gTz);
  np.end();
}


//  THE AP'S OWN EVENTS, because "0 device(s) joined" cannot tell the difference
//  between "nobody tried" and "somebody tried and it went wrong".
//
//  AP_PROBEREQRECVED is the useful one and it is MASKED BY DEFAULT in IDF - see
//  esp_wifi_set_event_mask() in startAp(). A phone scanning for networks sends
//  probe requests; if these lines appear while somebody scans beside the set,
//  our receiver is hearing them on channel 1 and whatever is wrong is on the
//  transmit side. If nothing appears at all, both directions are failing
//  together and the fault is not in this file.
static void onWifiEvent(WiFiEvent_t ev, WiFiEventInfo_t info) {
  switch (ev) {
    case ARDUINO_EVENT_WIFI_AP_START:
      Con.println(F("  [ap] started"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STOP:
      Con.println(F("  [ap] stopped"));
      break;
    case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
      Con.printf("  [ap] probe request heard, %d dBm\n",
                    (int)info.wifi_ap_probereqrecved.rssi);
      break;
    case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
      Con.println(F("  [ap] a device ASSOCIATED"));
      break;
    case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
      Con.println(F("  [ap] a device left"));
      break;
    default:
      break;
  }
}

static void startMdns() {
  if (mdnsUp) return;
  if (MDNS.begin("ambersong")) {
    MDNS.addService("http", "tcp", 80);
    mdnsUp = true;
    Con.println(F("  mDNS up: http://ambersong.local/"));
  }
}

//  NTP IS FRESH WHEN SNTP SAYS IT SYNCED, not when the clock reads past 2020.
//  Nothing else sets the system clock, so the old test was true forever after
//  the first sync: with the server gone, the S3's free-running clock was still
//  "NTP" and was written into the DS3231 every hour. This
//  callback runs in the lwIP task; it only stamps a time.
static volatile uint32_t gSntpSyncMs = 0;
static volatile bool     gSntpSynced = false;
static void onSntpSync(struct timeval *) { gSntpSyncMs = millis(); gSntpSynced = true; }
//  SNTP re-syncs every CONFIG_LWIP_SNTP_UPDATE_DELAY (3 h in this core); fresh
//  means one within that plus an hour's grace.
static const uint32_t NTP_FRESH_MS = 4UL * 3600UL * 1000UL;

static volatile bool     gWantAp = false;
static volatile uint32_t gWantApAt = 0;
static volatile uint16_t gWantQuiet = 0;
static volatile uint32_t gWantQuietAt = 0;

static void startNtp() {
  sntp_set_time_sync_notification_cb(onSntpSync);
  //  configTzTime sets the timezone AND the servers in one call, and the
  //  timezone is a POSIX TZ string so DST is handled by the C library rather
  //  than by arithmetic somebody has to remember to fix twice a year.
  configTzTime(gTz.c_str(), gNtp.c_str(), "time.nist.gov");
}

//  THE ACCESS POINT DOES NOT COME UP RELIABLY IF YOU JUST ASK FOR IT.
//
//  Three things are wrong with the obvious version, and startAp() handles all
//  three:
//
//   1. softAP() RETURNS A BOOL and it was being ignored, so "the AP is up" was
//      a claim rather than a fact. It is now checked - against the driver,
//      below - and retried.
//   2. The mode change is asynchronous. Calling softAP() in the same breath as
//      mode() races the driver actually finishing the switch.
//   3. The station side is stopped first. Coming here from a FAILED join
//      leaves it still retrying in the background, and it drags the radio off
//      the AP's channel. How it is stopped matters - see startAp().
//
//  The channel and the AP's own MAC are printed, because "I cannot see it" and
//  "it is not there" are different problems and only a scan can tell them apart.
//
//  Did the DRIVER end up holding an access point? softAP()'s return value
//  cannot answer this and never could: WiFiAP.cpp skips esp_wifi_set_config
//  altogether when the new config is byte-identical to the current one and
//  returns true regardless, so a retry loop can report three successes without
//  touching the radio. Ask the driver instead.
static bool apReallyUp() {
  wifi_mode_t m = WIFI_MODE_NULL;
  if (esp_wifi_get_mode(&m) != ESP_OK) return false;
  if (m != WIFI_MODE_AP && m != WIFI_MODE_APSTA) return false;
  wifi_config_t c;
  if (esp_wifi_get_config(WIFI_IF_AP, &c) != ESP_OK) return false;
  return c.ap.ssid[0] != 0;
}

static void startAp() {
  //  NEVER THROUGH WIFI_MODE_NULL.
  //
  //  This opened with WiFi.disconnect(true, false), and from pure WIFI_STA that
  //  chains enableSTA(false) -> mode(WIFI_MODE_NULL) -> espWiFiStop(), which
  //  calls esp_wifi_deinit(). The WiFi.mode(WIFI_AP) on the next line then
  //  re-initialises onto esp_netif objects the library never destroys: it
  //  creates them only when the slot is NULL and never calls
  //  esp_netif_destroy_default_wifi(). That is arduino-esp32 issue #7232, whose
  //  reported symptom is precisely "AP_START fires, softAP() returns true, and
  //  nothing broadcasts" - the symptom this machine showed at the time.
  //
  //  The station side still has to be stopped, or a failed join keeps retrying
  //  in the background and drags the single radio off the AP's channel. So:
  //  disconnect WITHOUT turning the radio off, go to AP_STA rather than AP, and
  //  drop the station afterwards - that last step is the `cm && m` branch in
  //  WiFiGeneric::mode() and does not deinit anything.
  WiFi.disconnect(false, false);        // stop chasing; do NOT collapse the mode
  delay(50);
  WiFi.mode(WIFI_AP_STA);
  delay(100);

  bool ok = false;
  for (int attempt = 1; attempt <= 3 && !ok; attempt++) {
    //  Channel 1, explicit. Two stations, not the default four - this is a
    //  rescue hatch, not a hotspot.
    ok = WiFi.softAP(AP_SSID, nullptr, 1, 0, 2);   // open - see AP_SSID
    //  AND THE RETURN IS NOT BELIEVED ON ITS OWN. See apReallyUp().
    if (ok && !apReallyUp()) {
      Con.println(F("  [WARN] softAP() said yes and the driver holds no AP - retrying."));
      ok = false;
    }
    if (!ok) { Con.printf("  [WARN] softAP attempt %d failed.\n", attempt); delay(400); }
  }

  WiFi.enableSTA(false);                // AP_STA -> AP. No deinit on this path.
  applyTxPower();                       // every mode change resets it
  gApUp = ok; gSta = false;
  if (!ok) {
    Con.println(F("  [FAIL] the access point will not start. Set the network"));
    Con.println(F("         over the cable with 'y', or reboot."));
    return;
  }

  //  UNMASK THE PROBE-REQUEST EVENT. IDF suppresses it by default
  //  (WIFI_EVENT_MASK_AP_PROBEREQRECVED), so without this nobody looking for
  //  the access point is ever reported. Zero means mask nothing. It must be
  //  called after the interface is started.
  esp_wifi_set_event_mask(0);

  //  SAY WHAT THE DRIVER HOLDS, not what we asked for. "is up" was once printed
  //  on the strength of a return value while nothing was on the air.
  Con.printf("  [WARN] no house network. Access point \"%s\" raised%s.\n",
                AP_SSID, apReallyUp() ? " and the driver confirms it"
                                      : " but THE DRIVER DOES NOT CONFIRM IT");
  Con.printf("         open, no password   http://%s/   channel %d   bssid %s\n",
                WiFi.softAPIP().toString().c_str(),
                WiFi.channel(), WiFi.softAPmacAddress().c_str());
  startMdns();
}

//  THE RESCUE AP COMES DOWN FOR THE LENGTH OF THE ATTEMPT.
//
//  Until 2026-09-07 gApUp was cleared ONLY by a SUCCESSFUL join, and a failed
//  join ends in startAp(), which raises it again. So once the AP had appeared,
//  every retry ran in WIFI_AP_STA and a plain station join was structurally
//  impossible. On 2026-09-05 the set sat on its own access point retrying
//  every two minutes for half an hour; every attempt ended in reason 2
//  (AUTH_EXPIRE, two to four times) and then 39 (TIMEOUT), never 201
//  (NO_AP_FOUND). The router was heard; the 802.11 AUTHENTICATION exchange
//  failed, and that is the part of a join that needs a tight round trip on ONE
//  channel. One radio cannot beacon an access point on channel 1 and hold that
//  round trip on the router's channel at the same time: IDF time-slices it,
//  returning to the home channel for ~30 ms at a stretch. (The transmit-power
//  fault found on 2026-09-07, see gTxQ, also fails that exchange; this rule
//  stands on its own reasoning.)
//
//  NOT the channel-lock story, which was checked and is WRONG: in AP+STA the
//  STATION's home channel wins and the soft AP is the interface that migrates,
//  announcing it with a CSA (ESP-IDF 4.4.7 Wi-Fi guide, "Home Channel"), and
//  WiFi.begin(ssid, pass) leaves sta.channel = 0, which means "scan for it".
//
//  So if nobody is using the rescue hatch, take it down and give the station
//  the whole radio. A failed join ends in startAp() within JOIN_MS and the
//  hatch is back. If somebody IS on it, they keep it and we try the slow way -
//  the same rule the retry guard in loop() already applies, for the same
//  reason: the one scenario this AP exists for is being locked out.
static void startJoin() {
  if (gSsid.length() == 0) { startAp(); return; }

  bool alone = gApUp && WiFi.softAPgetStationNum() == 0;
  //  NOT softAPdisconnect(true). In pure AP mode (startAp() turns STA off)
  //  that takes the driver through WIFI_MODE_NULL - esp_wifi_deinit() - the
  //  very path startAp() refuses to take, every retry of a home-network
  //  outage, about every two minutes. Blank the AP config
  //  and let the mode() below switch AP -> STA directly, which the core does
  //  with esp_wifi_set_mode() alone.
  if (alone) { WiFi.softAPdisconnect(false); gApUp = false; }

  //  PICK THE STRONGEST ACCESS POINT CARRYING THE NAME, NOT THE FIRST HEARD.
  //  Arduino's default is WIFI_FAST_SCAN, which commits to the first BSSID
  //  matching the SSID, and it applies no RSSI floor (sta.threshold.rssi is
  //  -127). On a house with more than one access point - a mesh node, an
  //  extender - that can commit the join to a BSS whose beacon carries but
  //  whose association will not, which is what AUTH_EXPIRE looks like from
  //  here. Costs a full-band scan per attempt, inside a 20 s window.
  WiFi.setScanMethod(WIFI_ALL_CHANNEL_SCAN);
  WiFi.setSortMethod(WIFI_CONNECT_AP_BY_SIGNAL);

  WiFi.mode(gApUp ? WIFI_AP_STA : WIFI_STA);
  applyTxPower();                             // the mode change just reset it
  WiFi.begin(gSsid.c_str(), gPass.c_str());
  joining = true; joinStart = millis();
  Con.printf("  joining \"%s\" ...%s\n", gSsid.c_str(),
                gApUp ? "  (sharing the radio with the rescue AP)" : "");
}

//  WHAT THE RADIO ACTUALLY HEARS.
//
//  A join failure is one of three different problems - "I cannot find it", "I
//  found it and it will not talk to me", or "I am talking to the wrong one" -
//  and the disconnect reason code separates the first from the other two and
//  then stops. This separates all three: every access point on the air, which
//  of them carry the configured name, how loud each one is, and what security
//  it advertises. WPA3-only, or a transition mode demanding protected
//  management frames, shows up here and nowhere else.
//
//  BLOCKING for a few seconds, so loop() stalls and the display holds its last
//  frame. Offered only when the machine is NOT on the home network, which is
//  the only time the answer is worth anything and the only time the stall
//  costs nothing anybody is looking at.
static const char *authName(wifi_auth_mode_t m) {
  switch (m) {
    case WIFI_AUTH_OPEN:            return "open";
    case WIFI_AUTH_WEP:             return "WEP";
    case WIFI_AUTH_WPA_PSK:         return "WPA";
    case WIFI_AUTH_WPA2_PSK:        return "WPA2";
    case WIFI_AUTH_WPA_WPA2_PSK:    return "WPA/WPA2";
    case WIFI_AUTH_WPA2_ENTERPRISE: return "WPA2-ENT";
    case WIFI_AUTH_WPA3_PSK:        return "WPA3";
    case WIFI_AUTH_WPA2_WPA3_PSK:   return "WPA2/WPA3";
    case WIFI_AUTH_WAPI_PSK:        return "WAPI";
    default:                        return "?";
  }
}

//  Set from the stored settings by applySettings(), and re-asserted on every
//  mode change. Quarter-dBm, clamped by applyTxPower() to 8..60 (2.0..15.0 dBm).
void setTxPower(uint8_t quarterDbm) {
  gTxQ = quarterDbm;
  applyTxPower();
}
uint8_t txPower() { return gTxQ; }

void scanReport() {
  Con.println(F("  scanning, a few seconds ..."));
  int n = WiFi.scanNetworks(false, true);        // blocking, show hidden
  if (n <= 0) {
    Con.println(F("  NOTHING on the air. Either the scan was refused because a"));
    Con.println(F("  join is in flight, or this radio is deaf - try again in 30 s."));
    WiFi.scanDelete();
    return;
  }
  int mine = 0;
  for (int i = 0; i < n; i++) {
    bool ours = (WiFi.SSID(i) == gSsid);
    if (ours) mine++;
    Con.printf("   %s %-22s ch %2d  %4d dBm  %-9s %s\n",
                  ours ? "->" : "  ",
                  WiFi.SSID(i).length() ? WiFi.SSID(i).c_str() : "(hidden)",
                  WiFi.channel(i), WiFi.RSSI(i),
                  authName(WiFi.encryptionType(i)), WiFi.BSSIDstr(i).c_str());
  }
  Con.printf("  %d network(s) heard, %d of them called \"%s\"\n",
                n, mine, gSsid.c_str());
  if (!mine)
    Con.println(F("  the configured network is NOT on the air from where this sits."));
  WiFi.scanDelete();
}

//  WHAT THE DRIVER ACTUALLY HOLDS, as opposed to what we told it.
//
//  Written while the rescue access point had never been seen by anybody (it was
//  first seen on 2026-09-24): every piece of evidence that it was "up" traced
//  back to softAP() returning true and a BSSID printed afterwards, and nothing
//  asked the driver.
//
//  The pattern it tests for is RECEIVE working and TRANSMIT not: a scan that
//  hears distant networks, while the two things that need the transmitter -
//  answering a router's authentication, and beaconing an access point - both
//  fail. Mode, TX power and the AP config read back from the driver are what
//  separate that from a configuration mistake.
void driverReport() {
  wifi_mode_t mode = WIFI_MODE_NULL;
  esp_err_t   e    = esp_wifi_get_mode(&mode);
  const char *mn   = mode == WIFI_MODE_NULL ? "NULL - the driver holds no mode" :
                     mode == WIFI_MODE_STA  ? "STA"  :
                     mode == WIFI_MODE_AP   ? "AP"   :
                     mode == WIFI_MODE_APSTA? "AP+STA" : "?";
  Con.printf("  driver   : mode %s%s\n", mn,
                e == ESP_OK ? "" : "   *** esp_wifi_get_mode FAILED ***");

  //  TX POWER. Quarter-dBm units; this firmware never asks for more than 60
  //  (15 dBm). The LOW flag fires under 40 (10 dBm), which includes the lower
  //  ladder rungs this firmware sets on purpose - check it against the rung.
  //  A zero here, with the scan still working, is the whole answer.
  int8_t pwr = -128;
  if (esp_wifi_get_max_tx_power(&pwr) == ESP_OK)
    Con.printf("  tx power : %d  (%.1f dBm)%s\n", (int)pwr, pwr / 4.0f,
                  pwr < 40 ? "   *** LOW - the transmitter is turned down ***" : "");
  else
    Con.println(F("  tx power : *** esp_wifi_get_max_tx_power FAILED ***"));

  //  COUNTRY AND CHANNEL PLAN. The soft AP is pinned to channel 1; if the
  //  regulatory table in force does not contain channel 1 the driver will not
  //  beacon on it, and it does not have to tell anybody.
  wifi_country_t c;
  if (esp_wifi_get_country(&c) == ESP_OK)
    Con.printf("  country  : \"%c%c%c\"  channels %u..%u  max tx %d dBm  policy %s\n",
                  c.cc[0] ? c.cc[0] : '?', c.cc[1] ? c.cc[1] : '?',
                  c.cc[2] ? c.cc[2] : ' ',
                  (unsigned)c.schan, (unsigned)(c.schan + c.nchan - 1),
                  (int)c.max_tx_power,
                  c.policy == WIFI_COUNTRY_POLICY_AUTO ? "AUTO" : "MANUAL");
  else
    Con.println(F("  country  : *** esp_wifi_get_country FAILED ***"));

  //  THE AP CONFIG READ BACK OUT OF THE DRIVER, not the copy we handed it.
  wifi_config_t ap;
  if (esp_wifi_get_config(WIFI_IF_AP, &ap) == ESP_OK) {
    Con.printf("  ap cfg   : ssid \"%s\" (len %u)  ch %u  hidden %u  max %u  auth %u\n",
                  (const char *)ap.ap.ssid, (unsigned)ap.ap.ssid_len,
                  (unsigned)ap.ap.channel, (unsigned)ap.ap.ssid_hidden,
                  (unsigned)ap.ap.max_connection, (unsigned)ap.ap.authmode);
    if (ap.ap.ssid[0] == 0)
      Con.println(F("             *** the driver holds NO AP SSID - softAP() did not take ***"));
  } else {
    Con.println(F("  ap cfg   : *** esp_wifi_get_config(AP) FAILED - no AP configured ***"));
  }

  //  BEACON INTERVAL AND PHY MODE. A correct SSID on a correct channel at full
  //  power still transmits NOTHING if the beacon interval is nonsense or the
  //  protocol bitmap is empty, and neither shows up in any of the fields read
  //  above. These were the two remaining ways for every other line here to be
  //  right while the air stays silent.
  {
    wifi_config_t apc;
    if (esp_wifi_get_config(WIFI_IF_AP, &apc) == ESP_OK)
      Con.printf("  beacon   : %u ms%s\n", (unsigned)apc.ap.beacon_interval,
                    (apc.ap.beacon_interval < 100 || apc.ap.beacon_interval > 60000)
                      ? "   *** OUT OF RANGE - it will not beacon ***" : "");
    uint8_t proto = 0;
    if (esp_wifi_get_protocol(WIFI_IF_AP, &proto) == ESP_OK)
      Con.printf("  ap phy   : 0x%02X %s%s%s%s\n", proto,
                    (proto & WIFI_PROTOCOL_11B)  ? "b" : "",
                    (proto & WIFI_PROTOCOL_11G)  ? "g" : "",
                    (proto & WIFI_PROTOCOL_11N)  ? "n" : "",
                    proto ? "" : "  *** NO PHY ENABLED ***");
  }

  //  And the primary channel the radio is actually parked on, which is not
  //  necessarily the one we asked the AP for.
  uint8_t prim = 0; wifi_second_chan_t sec = WIFI_SECOND_CHAN_NONE;
  if (esp_wifi_get_channel(&prim, &sec) == ESP_OK)
    Con.printf("  radio on : channel %u\n", (unsigned)prim);

  Con.printf("  ap ip    : %s   clients %d\n",
                WiFi.softAPIP().toString().c_str(), WiFi.softAPgetStationNum());
}


//  The timezone has to be live even with no network at all - the DS3231 keeps
//  UTC and the display needs local time whether or not NTP has ever answered.
//  So it is applied here, at begin(), and again only by setNtp(); startNtp()'s
//  configTzTime() sets the same string.
static void applyTz() { setenv("TZ", gTz.c_str(), 1); tzset(); }

void begin() {
  loadPrefs();
  applyTz();
  WiFi.onEvent(onWifiEvent);
  WiFi.persistent(false);        // we keep credentials ourselves, in one place
  WiFi.setHostname("ambersong");
  startJoin();
}

void loop() {
  uint32_t now = millis();

  //  THE CAPTIVE PORTAL'S DNS follows the AP: up with it, down with it - see gDns.
  if (gApUp) {
    if (!gDnsUp) {
      gDnsUp = gDns.start(53, "*", WiFi.softAPIP());
      if (gDnsUp) Con.println(F("  captive portal: every name now points at this radio."));
    }
    if (gDnsUp) gDns.processNextRequest();
  } else if (gDnsUp) {
    gDns.stop();
    gDnsUp = false;
  }

  //  Requests from the HTTP handler, carried out here - see requestForceAp().
  if (gWantQuiet && now - gWantQuietAt > 500) {
    uint16_t s = gWantQuiet; gWantQuiet = 0;
    quiet(s);
    return;
  }
  //  Half a second's grace, as for quiet: raising the AP drops the house link,
  //  and the answer to the request that asked for it has to get out first.
  if (gWantAp && (int32_t)(now - gWantApAt) > 500) {
    gWantAp = false;
    forceAp();
    Con.println(apReallyUp() ? F("  access point is up.")
                             : F("  [WARN] the access point did NOT come up - the driver holds no AP."));
  }

  //  Come back on our own. Nothing else can restore the radio once it is off,
  //  which is exactly why it must not depend on being asked.
  if (quietUntil) {
    if ((int32_t)(now - quietUntil) < 0) return;
    quietUntil = 0;
    Con.println(F("  radio silence over - rejoining."));
    startJoin();
    return;
  }

  //  A network change scheduled by the portal, deliberately deferred.
  if (pendingJoin && (int32_t)(now - pendingJoin) >= 0) {
    pendingJoin = 0;
    if (gSta) { gSta = false; WiFi.disconnect(); }
    lastTry = now;
    startJoin();
    return;
  }

  if (joining) {
    if (WiFi.status() == WL_CONNECTED) {
      joining = false; gSta = true;
      if (gApUp) { WiFi.softAPdisconnect(true); WiFi.mode(WIFI_STA); gApUp = false; }

      //  MODEM SLEEP OFF, and it must happen HERE - after the association.
      //  Asking before WiFi.begin() is ignored, because connecting applies the
      //  power-save mode again. Left on, the radio only listens at the DTIM
      //  beacon, and every packet of a request waits for the next one: measured
      //  0.5-1.0 s just to complete a TCP handshake on the same LAN, and 4-19 s
      //  for a page. Staying awake draws more current (ESP32-S3 datasheet: tens
      //  of mA); the machine is mains powered (Bible §10).
      WiFi.setSleep(WIFI_PS_NONE);
      //  SAY WHICH RUNG WORKED. main.cpp copies gTxQ into the settings within a
      //  loop pass, so the rung that associated is what the next boot starts on.
      Con.printf("  on \"%s\" as http://%s/  (%d dBm rx, transmitting at %.1f dBm)\n",
                    gSsid.c_str(), WiFi.localIP().toString().c_str(), WiFi.RSSI(),
                    gTxQ / 4.0f);
      startMdns();
      startNtp();
    } else if (now - joinStart > JOIN_MS) {
      joining = false;
      //  CLIMB A RUNG AND KEEP GOING. This file opens with "THE RADIO MUST
      //  NEVER BECOME UNREACHABLE", and on 2026-09-07 it was, for a whole day,
      //  because 20 dBm radiated nothing at all on this board and nothing in
      //  the firmware could see that. Now a failure moves the power instead of
      //  merely reporting it. The rescue access point still goes up in between,
      //  so the machine is never dark while it hunts.
      uint8_t was = gTxQ;
      gTxQ = ladderNext(gTxQ);
      applyTxPower();
      Con.printf("  join failed at %.1f dBm - trying %.1f dBm next time.\n",
                    was / 4.0f, gTxQ / 4.0f);
      startAp();
      lastTry = now;
    }
    return;
  }

  //  Running on the AP with a network configured: keep trying to come home.
  if (!gSta && gSsid.length() && now - lastTry > RETRY_MS &&
      !(gForced && now - forcedAt < FORCE_HOLD_MS)) {
    gForced = false;
    lastTry = now;
    //  BUT NOT WHILE SOMEONE IS USING THE RESCUE HATCH.
    //  A failed startJoin() ends in startAp(), which tears the access point
    //  down and back up and drops whoever is on it. The one scenario this AP
    //  exists for - home network gone, connect to it to fix the settings -
    //  is exactly the scenario where the retry was firing, so the form died
    //  under you every two minutes. Retrying can wait until they leave.
    if (gApUp && WiFi.softAPgetStationNum() > 0) return;
    startJoin();
    return;
  }

  //  Lost the network we had. Do not sit there dark.
  if (gSta && WiFi.status() != WL_CONNECTED) {
    Con.println(F("  [WARN] wifi dropped."));
    gSta = false; mdnsUp = false;
    startJoin();
    return;
  }

  if (gSta) {
    static uint32_t lastCheck = 0;
    if (now - lastCheck > 5000) {
      lastCheck = now;
      if (gSntpSynced) { ntpOk = true; lastNtpOk = gSntpSyncMs; }

      //  RE-ASSERT IT, do not just set it once on the join.
      //  Power save came back after an over-the-air update: ping latency went
      //  to 300-1000 ms, which is the DTIM beacon interval and is produced
      //  entirely inside the WiFi stack, so no application code can be blamed
      //  for it. Whatever re-enabled it, asserting this every five seconds is
      //  idempotent, costs nothing, and removes the whole question.
      WiFi.setSleep(WIFI_PS_NONE);
    }
  }
}

bool   isSta()     { return gSta; }
bool   connected() { return gSta || gApUp; }
String ip()        { return gSta ? WiFi.localIP().toString() : WiFi.softAPIP().toString(); }
String ssid()      { return gSta ? gSsid : String(AP_SSID); }
int    rssi()      { return gSta ? WiFi.RSSI() : 0; }

void getWifi(String &s, String &p) { s = gSsid; p = gPass; }
void setWifi(const String &s, const String &p) {
  gSsid = s; gPass = p; savePrefs();
  //  Do not tear the connection down from inside the handler that answered on
  //  it: Net::loop() rejoins by itself when pendingJoin comes due.
  //  THREE SECONDS OF GRACE. The request that changed this arrived over the
  //  very connection about to be torn down, and the browser has to receive its
  //  answer first or the user sees a failure for a change that worked.
  pendingJoin = millis() + 3000;
}

void getNtp(String &s, String &tz) { s = gNtp; tz = gTz; }
void setNtp(const String &s, const String &tz) {
  gNtp = s; gTz = tz; savePrefs();
  applyTz();
  if (gSta) startNtp();
}

uint32_t ntpEpoch() {
  time_t t = time(nullptr);
  return (t > 1600000000L) ? (uint32_t)t : 0;
}
bool ntpFresh() { return ntpOk && (millis() - lastNtpOk < NTP_FRESH_MS); }
void requestForceAp() { gWantApAt = millis(); gWantAp = true; }
void requestQuiet(uint16_t seconds) {
  gWantQuietAt = millis();
  gWantQuiet = seconds ? seconds : 1;
}
void ntpResync() { if (gSta) startNtp(); }

void quiet(uint16_t seconds) {
  if (seconds > 600) seconds = 600;          // ten minutes is plenty for a test
  Con.printf("  RADIO SILENCE for %u s. The portal will be unreachable.\n", seconds);
  quietUntil = millis() + (uint32_t)seconds * 1000UL;
  gSta = false; gApUp = false; joining = false; mdnsUp = false;
  WiFi.disconnect(true, false);
  WiFi.mode(WIFI_OFF);
}
bool     isQuiet()   { return quietUntil != 0; }
uint16_t quietLeft() {
  if (!quietUntil) return 0;
  int32_t left = (int32_t)(quietUntil - millis());
  return left > 0 ? (uint16_t)(left / 1000) : 0;
}

//  Force the access point up on demand. The fallback is only worth having if it
//  has been SEEN working, and waiting for the router to die is not a test plan.
//
//  IT STAMPS lastTry. Until 2026-09-24 it did not, and lastTry still held the
//  join at boot - hours earlier - so on the very next loop pass the retry tore
//  the new access point down to rejoin home. It lived about one loop tick and
//  no scanner could see it. The fallback after a failed join was never
//  affected - that path stamps lastTry itself. The hold is FORCE_HOLD_MS.
void forceAp() {
  pendingJoin = 0;
  joining = false;
  gForced = true; forcedAt = millis(); lastTry = forcedAt;
  startAp();
}

int  apClients()  { return gApUp ? WiFi.softAPgetStationNum() : -1; }
bool apVerified() { return apReallyUp(); }

}  // namespace Net
