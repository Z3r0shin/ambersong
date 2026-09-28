#pragma once
// ============================================================================
//  THE CONSOLE, ONCE, FOR TWO AUDIENCES.
//
//  Everything the S3 says goes through Con: to the USB port as before, and into
//  a ring the portal can read. Everything it is told comes through Con too:
//  keys typed at the portal first, then the USB port. So the dispatcher, the
//  interactive prompts (clock, wifi) and every message work the same with or
//  without a cable - and without a cable no PC ground is joined to the set's
//  (Bible §10 and §21 for how the set is grounded and mains-referenced).
//
//  WHY A NEW OBJECT AND NOT A REDEFINED `Serial`: in this build `Serial` is
//  already the core's object for the USB-JTAG port (HWCDC). Redefining it
//  with a macro depends on include order in every file that reaches it; a
//  plain object that owns the port does not. console.cpp is the ONLY file that touches the
//  real port. Shared code in include/ takes a Stream& and is handed Con.
// ============================================================================
#include <Arduino.h>

class ConsoleTee : public Stream {
 public:
  void   begin(unsigned long baud);
  size_t write(uint8_t c) override;
  size_t write(const uint8_t *buf, size_t n) override;
  int    available() override;
  int    read() override;
  int    peek() override;
  void   flush() override;
  explicit operator bool();
  using Print::write;

  //  Portal side. inject() queues keys as if typed; returns how many fitted.
  size_t   inject(const char *s, size_t n);
  //  Copy everything said since byte number `from` (at most `cap` bytes) into
  //  dst. Returns the count; `first` is the byte number of dst[0], which is
  //  later than `from` when the ring has already overwritten the gap. The
  //  caller's next `from` is first + count.
  uint32_t copySince(uint32_t from, char *dst, size_t cap, uint32_t &first);

  //  TRUE WHILE A PROMPT IS READING A WHOLE LINE (the network and clock
  //  prompts). Only then may the portal send more than one key: the dispatcher
  //  takes one character per pass, so a typed word used to run as a string of
  //  commands - "status" ran t, a, u (an unbounded jog), and any word with a z
  //  in it erased the RF calibration.
  volatile bool lineWanted = false;
};

extern ConsoleTee Con;
