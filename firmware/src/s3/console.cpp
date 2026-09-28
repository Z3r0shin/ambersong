#include "console.h"

ConsoleTee Con;

//  16 KB is a few screens of status dumps - enough to scroll back through what
//  just happened, small against the S3's RAM. Power of two so the index is a
//  mask. gSeq counts every byte ever written; it wraps after 4 GB, which at the
//  rate this machine talks is years, and a wrap only costs one resync.
static const uint32_t LOG_CAP = 16384;
static char     gLog[LOG_CAP];
static uint32_t gSeq = 0;

//  Keys from the portal. 256 is several typed lines; the endpoint caps one
//  request at 128.
static const uint16_t IN_CAP = 256;
static char     gIn[IN_CAP];
static uint16_t gInHead = 0, gInTail = 0;     // count = head - tail, wraps safely

//  ONE LOCK FOR BOTH RINGS, HELD ONLY FOR MEMORY COPIES. Writers are on both
//  cores (the portal task prints from core 0), so a spinlock rather than a
//  FreeRTOS mutex. The USB write itself happens OUTSIDE the lock: it can wait
//  on the host, and nothing may wait while holding a spinlock.
static portMUX_TYPE gMux = portMUX_INITIALIZER_UNLOCKED;

void ConsoleTee::begin(unsigned long baud) { Serial.begin(baud); }

size_t ConsoleTee::write(uint8_t c) { return write(&c, 1); }

size_t ConsoleTee::write(const uint8_t *buf, size_t n) {
  const uint8_t *p = buf;
  size_t k = n;
  if (k > LOG_CAP) { p += k - LOG_CAP; k = LOG_CAP; }   // only the tail survives anyway
  portENTER_CRITICAL(&gMux);
  uint32_t at = gSeq & (LOG_CAP - 1);
  uint32_t first = LOG_CAP - at;
  if (first > k) first = k;
  memcpy(gLog + at, p, first);
  memcpy(gLog, p + first, k - first);
  gSeq += n;
  portEXIT_CRITICAL(&gMux);
  //  RETURN n, NOT WHAT THE PORT TOOK. With no host attached the port can
  //  report 0, and Print's callers treat a short count as failure - the log
  //  did take it, and that is what a portal-only machine is relying on.
  Serial.write(buf, n);
  return n;
}

int ConsoleTee::available() {
  portENTER_CRITICAL(&gMux);
  uint16_t q = (uint16_t)(gInHead - gInTail);
  portEXIT_CRITICAL(&gMux);
  return q ? (int)q : Serial.available();
}

int ConsoleTee::read() {
  int c = -1;
  portENTER_CRITICAL(&gMux);
  if (gInHead != gInTail) { c = (uint8_t)gIn[gInTail % IN_CAP]; gInTail++; }
  portEXIT_CRITICAL(&gMux);
  return c >= 0 ? c : Serial.read();
}

int ConsoleTee::peek() {
  int c = -1;
  portENTER_CRITICAL(&gMux);
  if (gInHead != gInTail) c = (uint8_t)gIn[gInTail % IN_CAP];
  portEXIT_CRITICAL(&gMux);
  return c >= 0 ? c : Serial.peek();
}

void ConsoleTee::flush() { Serial.flush(); }

ConsoleTee::operator bool() { return (bool)Serial; }

size_t ConsoleTee::inject(const char *s, size_t n) {
  size_t put = 0;
  portENTER_CRITICAL(&gMux);
  while (put < n && (uint16_t)(gInHead - gInTail) < IN_CAP) {
    gIn[gInHead % IN_CAP] = s[put++];
    gInHead++;
  }
  portEXIT_CRITICAL(&gMux);
  return put;
}

uint32_t ConsoleTee::copySince(uint32_t from, char *dst, size_t cap, uint32_t &first) {
  portENTER_CRITICAL(&gMux);
  uint32_t now    = gSeq;
  uint32_t oldest = now > LOG_CAP ? now - LOG_CAP : 0;
  //  A `from` the ring no longer holds, or one from the future (the S3 rebooted
  //  under an open page), resyncs to the oldest byte still here.
  if (from < oldest || from > now) from = oldest;
  uint32_t n = now - from;
  //  THE OLDEST `cap` BYTES, not the newest: the caller's next poll starts at
  //  from + n and picks up the rest. Taking the newest dropped the head of any
  //  burst over the portal's 4 KB - the first lines of `s` and `D` - and the
  //  page then said the ring had overwritten it, which it had not.
  if (n > cap) n = cap;
  uint32_t at = from & (LOG_CAP - 1);
  uint32_t a  = LOG_CAP - at;
  if (a > n) a = n;
  memcpy(dst, gLog + at, a);
  memcpy(dst + a, gLog, n - a);
  portEXIT_CRITICAL(&gMux);
  first = from;
  return n;
}
