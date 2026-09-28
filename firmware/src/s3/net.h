// ============================================================================
//  WIFI, NTP AND mDNS
//
//  THE RADIO MUST NEVER BECOME UNREACHABLE. If the stored network is gone -
//  router replaced, password changed, moved house - the S3 raises its own
//  access point instead, so the portal is always somewhere. It keeps trying the
//  real network in the background, so it heals itself when the router comes
//  back rather than waiting to be rescued.
//
//  Credentials live in NVS, not in the source. They survive a power cut, a
//  reflash and an OTA, and they are not in the settings file that gets emailed
//  around.
// ============================================================================
#pragma once
#include <Arduino.h>

namespace Net {

void begin();          // non-blocking; joins in the background
void loop();           // call from the main loop

bool  isSta();         // true = on the home network; false = not (AP, joining, silent)
bool  connected();
String ip();
String ssid();
int   rssi();

//  Stored configuration. Setting it schedules a reconnect rather than doing it
//  inline, so an HTTP handler is never the thing that tears down its own socket.
void   getWifi(String &ssid, String &pass);
void   setWifi(const String &ssid, const String &pass);

void   getNtp(String &server, String &tz);
void   setNtp(const String &server, const String &tz);

//  RADIO SILENCE, for a fixed time, then back on by itself.
//
//  The A32's firmware never starts WiFi - it uses Bluetooth only - so the
//  2.4 GHz transmitter sharing the cabinet with it is THIS one. Two independent
//  radios have no arbitration between them at all: within one chip ESP-IDF
//  time-slices Bluetooth and WiFi, but across two chips there is nothing.
//
//  This turns the transmitter off long enough to hear whether that is what is
//  breaking up the audio, and brings it back on a timer so the portal cannot be
//  lost by running the test.
void quiet(uint16_t seconds);
bool isQuiet();
uint16_t quietLeft();

//  Raise the access point NOW, whatever the network is doing, and hold it ten
//  minutes (longer while anyone is on it) before the retry may go home. For
//  testing the rescue hatch, which is otherwise only exercised on the day it is
//  needed.
void forceAp();
//  Is the DRIVER in AP or AP+STA mode with a non-empty AP SSID? Distinct
//  from connected()/isSta(), which report what this file believes. softAP()'s
//  own return value cannot answer it - see the note in net.cpp.
bool   apVerified();
int  apClients();      // -1 when not running an access point

//  Transmit power in QUARTER-dBm, clamped to 8..60 (2.0..15.0 dBm). 15 dBm is
//  this firmware's ceiling: measured 2026-09-24, this board radiated nothing at
//  20 dBm and reliably at 15, and the author keeps it as a firmware choice.
//  Stored in the settings and re-applied after every WiFi mode change, because
//  esp_wifi_set_mode() does not carry it across. A failed join steps it up the
//  ladder in net.cpp (TX_LADDER), and the rung that joins is kept.
void    setTxPower(uint8_t quarterDbm);
uint8_t txPower();

//  Print every access point the radio can hear, marking the ones carrying the
//  configured name, with channel, level and advertised security. BLOCKING for
//  a few seconds. Diagnostic: it answers "cannot find it" versus "found it and
//  it refuses me" versus "there are three of them and I picked the far one",
//  which no disconnect reason code can.
void scanReport();

//  Ask the WiFi driver what it actually holds - mode, transmit power, country
//  and channel plan, and the access-point config read back out of it. Cheap and
//  non-blocking. Exists because "softAP() returned true" was the only evidence
//  we ever had that the rescue AP was transmitting, and it was not enough.
void driverReport();

//  From an HTTP handler: ask, and Net::loop() does it on its own core, half a
//  second later, so the answer gets out first.
void     requestForceAp();
void     requestQuiet(uint16_t seconds);
//  Seconds since the epoch from the system clock, which only NTP sets; 0 if it
//  has never synced. The DS3231 stays the authority for the clock; this is what
//  disciplines it, and only while ntpFresh() (an SNTP sync in the last 4 h).
uint32_t ntpEpoch();
bool     ntpFresh();
void     ntpResync();

}  // namespace Net
