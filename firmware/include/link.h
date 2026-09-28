// ============================================================================
//  AMBERSONG  -  INTER-MCU LINK
//
//  Thin wrapper over Serial2 + ProtoFramer, shared by both firmwares so the
//  framing, the liveness rule and the handshake exist in exactly one place.
//
//  THE PIN TRAP THIS FILE EXISTS TO PREVENT
//  On the classic ESP32, Serial2's DEFAULT pins are GPIO16 and GPIO17 - and
//  GPIO17 is the firmware's shared I2S LRCK (pins.h). Calling Serial2.begin(baud)
//  without pins silently reassigns it and breaks the DAC, surfacing much later
//  as "the audio does not work" with no obvious cause. (GPIO16 was called XSMT
//  here until 2026-09-24, a stale name: pins.h deleted A32_PCM5102_XSMT on
//  2026-09-23 and this firmware gives GPIO16 no job.)
//  begin() here REQUIRES explicit pins. There is no overload without them.
//
//  LIVENESS - THE TWO BOARDS ARE NOT SYMMETRIC.
//    - the S3 keeps the clock, needle and panel running if the A32 is silent;
//      for the S3, peerAlive() only decides when to forget the handshake and
//      the A32's settings mirror, and to display the link's state.
//    - the A32 GATES ON IT: by the author's awake rule (2026-09-01) it plays
//      and takes Bluetooth only while the amp is on AND peerAlive() is true
//      (serviceWake() in a32/main.cpp). When the S3 goes quiet - its own OTA
//      reboot included - the A32 mutes and drops the phone until it is back.
// ============================================================================

#pragma once
#include <Arduino.h>
#include "proto.h"

class Link {
 public:
  typedef void (*Handler)(const ProtoFramer &f);

  //  Explicit pins are mandatory - see the trap above.
  void begin(HardwareSerial &port, int rxPin, int txPin, uint32_t baud = PROTO_BAUD) {
    port_ = &port;
    //  BEFORE begin(). setRxBufferSize() is silently ignored once the port is
    //  running - it logs an error and leaves the default 256-byte buffer in
    //  place. That is survivable for small control frames, which is exactly why
    //  the first version of this appeared to work, but it would corrupt the OTA
    //  relay: 1 KB chunks at 921600 baud overflow 256 bytes without trying.
    port_->setRxBufferSize(4096);
    port_->begin(baud, SERIAL_8N1, rxPin, txPin);
    framer_.reset();
    lastRxMs_ = 0;
  }

  void onMessage(Handler h) { handler_ = h; }

  //  Call as often as convenient. Drains whatever has arrived; does not block.
  void poll() {
    if (!port_) return;
    while (port_->available()) {
      if (framer_.feed((uint8_t)port_->read())) {
        lastRxMs_ = millis();
        rxCount_++;
        if (handler_) handler_(framer_);
      }
    }
  }

  bool send(uint8_t type, const void *payload = nullptr, uint16_t len = 0) {
    if (!port_) return false;
    uint8_t out[PROTO_MAX_PAYLOAD + 8];
    size_t n = protoEncode(out, sizeof(out), type, payload, len);
    if (!n) { txDropped_++; return false; }
    port_->write(out, n);
    txCount_++;
    return true;
  }

  template <typename T>
  bool send(uint8_t type, const T &p) { return send(type, &p, (uint16_t)sizeof(T)); }

  //  A frame arrived intact within PROTO_SILENCE_MS. The A32's awake rule
  //  gates audio and Bluetooth on it - see LIVENESS above.
  bool     peerAlive() const { return lastRxMs_ && (millis() - lastRxMs_) < PROTO_SILENCE_MS; }
  uint32_t lastRxMs()  const { return lastRxMs_; }
  uint32_t rxCount()   const { return rxCount_; }
  uint32_t txCount()   const { return txCount_; }
  uint32_t badCrc()    const { return framer_.badCrc; }
  uint32_t badVer()    const { return framer_.badVer; }

  //  Peer identity, filled by the HELLO exchange. An empty string means the
  //  peer has not introduced itself yet.
  char peerVersion[PROTO_VERSION_LEN] = {0};

  void notePeerHello(const ProtoHello &h) {
    memcpy(peerVersion, h.fwVersion, PROTO_VERSION_LEN);
    peerVersion[PROTO_VERSION_LEN - 1] = 0;
    peerProto_ = h.protoVersion;
  }
  uint8_t peerProto() const { return peerProto_; }

 private:
  HardwareSerial *port_    = nullptr;
  ProtoFramer     framer_;
  Handler         handler_ = nullptr;
  uint32_t        lastRxMs_ = 0;
  uint32_t        rxCount_  = 0;
  uint32_t        txCount_  = 0;
  uint32_t        txDropped_ = 0;
  uint8_t         peerProto_ = 0;
};

// ---------------------------------------------------------------------------
//  BOOT SELF-TEST
//  Encodes a frame, feeds it back through the framer byte by byte, and checks
//  it survives. Cheap, runs once, and catches a broken ENCODER or FRAMER on
//  this build.
//
//  WHAT IT CANNOT CATCH, corrected 2026-09-24: a struct that changed on one
//  side only. Both ends of this round trip are the same build compiled from
//  the same proto.h, so they always agree with each other. Skew between the
//  S3 and the A32 shows on the wire instead: the framer drops and counts a
//  frame of another PROTO_VERSION (badVer), and ProtoFramer::as() refuses a
//  payload of the wrong length.
//  And a payload too big for a frame is now refused by the compiler (the
//  static_asserts at the end of proto.h's payload section), not found here.
//
//  Also deliberately corrupts a byte and confirms the CRC rejects it. A
//  checksum that has never been seen to fail is not known to work.
// ---------------------------------------------------------------------------
static inline bool protoSelfTest(Stream &out) {
  uint8_t buf[PROTO_MAX_PAYLOAD + 8];
  ProtoState s = {};
  s.source = SRC_RADIO;
  s.btState = BT_LINK;
  s.volume = 200;
  s.peakLdBx10 = -354;              // any plausible level; -35.4 dBFS
  s.peakRdBx10 = -354;
  s.avrcpVolume = 100;
  s.rawLadder = 4095;
  strncpy(s.peerName, "selftest", sizeof(s.peerName) - 1);

  size_t n = protoEncode(buf, sizeof(buf), MSG_STATE, &s, sizeof(s));
  if (!n) { out.println(F("  [FAIL] proto: encode refused a valid frame")); return false; }

  ProtoFramer f;
  bool got = false;
  for (size_t i = 0; i < n; i++) got = f.feed(buf[i]);
  if (!got) { out.println(F("  [FAIL] proto: valid frame did not decode")); return false; }
  if (f.type() != MSG_STATE) { out.println(F("  [FAIL] proto: wrong type")); return false; }

  ProtoState back;
  if (!f.as(back)) { out.println(F("  [FAIL] proto: size mismatch on decode")); return false; }
  if (memcmp(&s, &back, sizeof(s)) != 0) {
    out.println(F("  [FAIL] proto: payload did not survive the round trip"));
    return false;
  }

  //  Now prove the CRC actually rejects damage.
  buf[8] ^= 0x01;
  ProtoFramer f2;
  bool bad = false;
  for (size_t i = 0; i < n; i++) bad |= f2.feed(buf[i]);
  if (bad || f2.badCrc == 0) {
    out.println(F("  [FAIL] proto: a corrupted frame was ACCEPTED"));
    return false;
  }

  out.printf("  [PASS] protocol v%d, state frame %u bytes, CRC rejects damage\n",
             PROTO_VERSION, (unsigned)n);
  return true;
}
