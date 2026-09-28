# Ambersong

*An old soul, glowing anew into the modern world.*

A vintage vacuum tube radio, connected with ESP32 audio and control.

---

## What it is

Ambersong is a cabinet radio built around an early-1960s tube radio, a LLOYDS TM-838N, which still
does the receiving. Its original tuning knob still turns the tube set's own tuning capacitor.
Around it, two ESP32 boards add what the old set never had:

- a four-digit amber clock behind the dial glass, built from LED filaments (the **Leditron**);
- a dial needle driven by a stepper motor instead of a cord, that follows the tuning through a
  magnetic angle sensor on the tuning shaft;
- a dial that calibrates itself: a small FM receiver chip listens to the tube set's own local
  oscillator and corrects the needle's frequency curve while you listen;
- panel lamps that follow what the listener does;
- Bluetooth music from a phone, through the same amplifier and speakers;
- a battery-backed clock kept in time over the network;
- a password-protected web page (the **portal**) that shows and changes everything, and over-the-air
  firmware updates for both boards.

The tube radio's sound is digitised (PCM1802 ADC) and played back through a PCM5102A DAC into a
small stereo amplifier with its own speakers; a front switch picks the radio, Bluetooth or an AUX
input.

The author's own unit is named *Ambrelievre*. This repository is the record of that one machine.
It is not a kit and not a step-by-step build guide: your tube set, cabinet, amplifier and parts
will differ. It is here to show how it was done, why, and what went wrong on the way, so that
you can build something similar, repair or recover this one, or take it further.

## Safety

**Tube radios of this kind often have a "hot chassis": no isolation transformer, and the chassis
tied to one side of the mains, so it can be live.** Never work on one plugged in, and treat the
chassis as live. What this build does about mains, and what it does not, is in the Hardware Bible,
§31.1.

## Where to start

Two documents describe the whole machine. Everything else in this repository is either source or a
visual aid.

| Document | What it covers |
|---|---|
| [`docs/HARDWARE-BIBLE.md`](docs/HARDWARE-BIBLE.md) | The hardware: what is connected to what, in the machine and in the tube radio; fitted values; datasheet facts; the FM antenna; the amplifier and speakers; the physical build and the printed parts. Start with its §0. |
| [`docs/FIRMWARE-GOSPEL.md`](docs/FIRMWARE-GOSPEL.md) | The firmware: what it does, how it works, its settings, why each decision was made, what was tried and abandoned, how to build, flash, update and recover it. Chapter 1 is for when the radio misbehaves; chapter 2 is the map; chapter 3 builds it from a clean PC. |

## What is in this repository

```
docs/                    the Hardware Bible and the Firmware Gospel
firmware/                PlatformIO project for both boards (environments s3 and a32)
hardware/schematics/     KiCad project (visual aid) and the tube radio's factory schematic
hardware/STLs/           the 76 printed parts (PLA; one TPU membrane), listed in Bible §31.7
photos/                  the finished machine
LICENSE, LICENSES/       licensing (see below)
NOTICE                   names and credits
```

The KiCad sheets are drawn for a human reader, to show what was built. They are not a PCB design,
and where a sheet and the Hardware Bible disagree, the Bible is right (Bible §0, §25).

## Building the firmware

The firmware is a PlatformIO project with two environments: `s3` for the main board (ESP32-S3,
16 MB flash, 8 MB PSRAM) and `a32` for the audio board (ESP32). The platform is pinned to
`espressif32@7.0.1` (Arduino-ESP32 2.0.17 on ESP-IDF 4.4.7); keep that pin unless you are ready
to re-test everything. The Gospel's chapter 3 covers the rest, from a clean PC to the first boot
and over-the-air updates.

```
cd firmware
pio run -e s3
pio run -e a32
```

On first boot, with no home network stored, the S3 raises an open access point named `Ambersong`;
join it and the portal opens (or browse to `http://192.168.4.1/`). The portal's first account is a
public placeholder: change its password before anything else (Gospel §3.2.8).

### How the published firmware differs from the one in the author's radio

The code is the author's v.1.0.5 (S3) and v.1.0.4 (A32), with these changes made for publication
only:

- the network host name and mDNS name (`ambersong`, `ambersong.local`);
- the rescue access point's name (`Ambersong`);
- the Bluetooth device name (`Ambersong`);
- the portal's page title and heading;
- the settings file's header line, its download name (`ambersong.txt`) and the console's settings
  markers;
- the banner printed at boot and the build script's version line;
- comments: wording only, no code.

The published firmware builds without errors for both boards. **It has not been flashed or run
on a radio.** The author's radio runs the same code with the names above set differently.

## Credits

- The clock display follows the **LEDitron** idea by Axiris (Elektor Labs, 2015; Elektor magazine,
  September 2016). Only the idea and the name are borrowed.
- Bluetooth audio uses Phil Schatzmann's
  [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) library.
- The tube set is a LLOYDS TM-838N. Its factory schematic is included for reference; the SAMS
  Photofact the documents cite is a published, copyrighted service document and is not included.

## Licence

Copyleft, one licence per kind of material. Use any part, change it, share it, sell it, as long
as what you share stays under the same licence:

- firmware: **GPL-3.0-or-later**;
- hardware design files (KiCad, STL): **CERN-OHL-S-2.0**;
- documentation and photos: **CC BY-SA 4.0**.

See [`LICENSE`](LICENSE) for the details and the exceptions, and [`NOTICE`](NOTICE) for the names.

Built by Zeroshin.
