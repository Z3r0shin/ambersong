// ============================================================================
//  THE WEB PORTAL
//
//  Synchronous WebServer, running in its own task on CORE 0, because that is
//  the arrangement phase G MEASURED on this hardware: WiFi associated, four
//  parallel HTTP downloads saturating the link, and the display's worst slot
//  error stayed under 100 us. The asynchronous server was not tested and is not
//  worth the risk of finding out.
//
//  SECURITY, and its honest limits. Plain HTTP over the home WiFi, because
//  TLS on an ESP32 costs more than it buys against the actual threat model:
//  people on the network, and a curious guest. What IS taken seriously:
//   - no anonymous access to anything but the page itself, login, logout and
//     the favicon; everything else needs a session
//   - passwords stored SALTED AND HASHED, never in the clear. A USB cable can
//     read this flash, and passwords get reused across places that matter.
//   - sessions live in RAM only, so a reboot logs everyone out
//   - passwords are at least six characters, and failed logins back off per
//     address (after five: 1, 2, 4 ... 64 minutes), so guessing is not free
//   - credentials are NOT in the downloadable settings file
// ============================================================================
#pragma once
#include <Arduino.h>

namespace Portal {

void begin();               // starts the server task on core 0
bool running();
int  sessions();            // how many people are logged in right now
bool otaActive();
//  True while the logged-in account is still the shipped admin/admin.
bool mustChangeCreds();

//  IS THE SERVER TASK ACTUALLY RUNNING?
//  HTTP has twice died while ping stayed at 0% packet loss, which means the
//  radio and the IP stack were fine and something above them was not. A counter
//  that the task increments every pass answers the only question that matters:
//  frozen means it is being starved or blocked - and the step emitter sits on
//  the same core at priority 19 against this task's 3, which phase G never
//  measured because the emitter was not there yet. Climbing means the task runs
//  and the fault is in the accept path instead.
uint32_t loops();
uint32_t served();
//  BYTES, not words. The ESP32 port's own header says so:
//  "@return The smallest amount of free stack space there has been (in bytes
//   not words, unlike vanilla FreeRTOS)". Reported as words it over-read the
//   headroom by four, on the one instrument used to judge whether the A32 OTA
//   relay - which puts ~2.1 kB of frame buffers on this task - has room.
uint32_t stackFreeBytes();

//  For the console ('~'): when the password has been lost, erase EVERY
//  account and put the author back on the placeholder admin/admin, which the
//  portal then makes him change. There is no lockout recovery on the front of
//  the machine, by decision - the USB socket is the recovery path, and this
//  is it.
void resetAdmin();

}  // namespace Portal
