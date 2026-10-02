# Ambersong firmware

One PlatformIO project for the two boards of Ambersong:

| Environment | Board | What it runs |
|---|---|---|
| `s3` | ESP32-S3 (N16R8: 16 MB flash, 8 MB octal PSRAM) | the clock display, the dial needle and its sensors, the tuning calibration (RDA5807M), the panel lamps, WiFi, the web portal, over-the-air updates for both boards |
| `a32` | ESP32 | audio: the tube radio through the PCM1802 ADC, Bluetooth A2DP, the PCM5102A DAC, the source switch, the volume knob, the Bluetooth button and lamp, the DS3231 clock |

```
src/s3/      main board
src/a32/     audio board
include/     shared by both: pins.h (every pin), proto.h and link.h (the link between the boards)
data/        the portal's web page (gzipped into src/s3/page_gz.h at build time)
scripts/     build scripts: version stamping, page packing
```

## Build

```
pio run -e s3
pio run -e a32
```

The platform is pinned to `espressif32@7.0.1` (Arduino-ESP32 2.0.17, ESP-IDF 4.4.7), and the
ESP32-A2DP library to one commit. Do not update either without re-testing everything.

## Everything else

Flashing, the first boot, the portal, updates over the air, recovery, every setting and the reason
behind each design decision are in the Firmware Gospel: the [Human Readable edition](../docs/Ambersong%20-%20Firmware%20Gospel%20-%20Human%20Readable.pdf)
([chapters](../docs/human/firmware-gospel/)) or the complete [LLM edition](../docs/llm-edition/FIRMWARE-GOSPEL.md).
Start with its chapter 3 to build and flash from a clean PC, and its chapter 1 if the radio
misbehaves. What each pin is wired to is in the Hardware Bible,
the [Hardware Bible](../docs/Ambersong%20-%20Hardware%20Bible%20-%20Human%20Readable.pdf)
([chapters](../docs/human/hardware-bible/), [LLM edition](../docs/llm-edition/HARDWARE-BIBLE.md)).

Pins, sensor directions, the tuner's travel and the needle's limits are those of the author's
radio. Expect to change them for yours: the Gospel says where each one lives.

Licence: GPL-3.0-or-later (see [`../LICENSE`](../LICENSE)).
