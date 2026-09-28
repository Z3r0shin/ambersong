# Ambersong

*An old soul, glowing anew into the modern world.*

A LLOYDS TM-838N tube radio from the early sixties, rebuilt around two ESP32 boards that
add Bluetooth, a clock behind the dial glass, and a needle that follows the tuning knob without a
dial cord.

## How it started

I found this radio in the trash, in a pile of other junk. The enclosure was broken, and the
power cord and the antennas were gone. I tried it anyway. All it gave me was static, but it was
warm static, like a summer wind at night, and I wasn't ready to throw that away.

So I started restoring it. Most of the old components had dried up and had to be replaced. The
tubes, though, still had enough life in them for years of non-stop use, and that's when this
stopped being something to tinker with. If the tubes could last that long, the rest of the radio
had to as well, and that was before I'd even designed the cabinet.

Then the radio I had at home broke, so this one took over. I added Bluetooth and a clock, and I
wanted it to look warm and cozy, with an excuse to show off some woodworking and brass work. The
cabinet is oiled oak with walnut accents, matte black and brass, and I formed and etched the
brass plates myself. That cabinet, wood and brass together, is the part I'm proudest of. When
it's on, it glows amber, and it still feels analog in all the good parts.

From there the goal was to make it last for decades, and to write every part of it down so
someone else could build their own, repair this one, or take it further.

## What it does

The tube set still does the receiving. Its original knob still turns its own tuning capacitor.
There is no cord to the needle anymore: a magnetic angle sensor reads the capacitor's shaft, and
a small stepper motor moves the needle to match. The frequency scale isn't a straight line, so
the radio calibrates itself. While it plays, an RDA5807M FM chip listens for the tube set's local
oscillator, works out which station the set is really tuned to, and corrects the needle's curve.

The tube radio's sound goes through a PCM1802 converter into the audio board, and out through a
PCM5102A DAC to the amplifier and speakers of a Kramer Tavor 5-O pair. A front switch picks the
radio, Bluetooth from a phone, or an AUX input. Four LED-filament digits behind the dial glass
show the time from a battery-backed clock that sets itself over the network. A small web page on
your home network shows and changes every setting, and updates both boards over WiFi.

## Why a Bible and a Gospel

Two documents describe the whole machine: the Hardware Bible for everything physical, and the
Firmware Gospel for the code. Each one is meant to be the book you trust for its half of the
radio. The religious names are iconography, nothing more: I was raised in a mild North American
Catholic setting, but I'm not religious.

- [`docs/HARDWARE-BIBLE.md`](docs/HARDWARE-BIBLE.md): what is wired to what, fitted values,
  the tube radio circuit, the FM antenna, the amplifier, the cabinet and the printed parts.
  Start with its §0.
- [`docs/FIRMWARE-GOSPEL.md`](docs/FIRMWARE-GOSPEL.md): what the firmware does and why,
  every setting, what was tried and dropped, and how to build, flash, update and recover it.
  Chapter 1 is for when something goes wrong, chapter 2 is the map, chapter 3 gets you from a
  clean PC to both boards running.

## Building your own

Your radio, cabinet and amplifier won't be the same as mine, and the documents are written with
that in mind. Parts that only make sense for this build are marked as such. On the firmware side,
expect to change pins, sensor directions, the tuner's travel and the needle's limits; the Gospel
says where each one lives.

The firmware is one PlatformIO project with two environments, `s3` for the main board (ESP32-S3,
N16R8) and `a32` for the audio board (ESP32). The toolchain is pinned to `espressif32@7.0.1`.

## Safety

Many tube radios from this era have no isolation transformer, and this one is no exception. Its
chassis is tied to mains neutral, which makes it live whenever the plug goes in the other way
round. Unplug it before you open it. Hardware Bible §31.1 covers what that means in this build.

## What's in here

```
docs/                    the Hardware Bible and the Firmware Gospel
firmware/                the PlatformIO project for both boards
hardware/schematics/     the KiCad project, and the radio's factory schematic
hardware/STLs/           the 76 printed parts (PLA, plus one TPU membrane)
photos/                  the finished radio
```

The KiCad sheets show what was built, drawn to be read. They aren't a PCB layout, and where a
sheet and the Hardware Bible disagree, the Bible wins.

## Credits

- My parents and friends, for their input on the design.
- Axiris, for the LEDitron: seven-segment digits made from LED filaments. I borrowed the idea
  and the name, nothing else. See the [LEDitron display](https://www.elektormagazine.com/labs/leditron-display-150448)
  on Elektor Labs (2015) and the [LEDitron clock and scoreboard](https://www.elektormagazine.com/labs/160205-clock-scoreboardtimer-with-leditron-modules).
- Phil Schatzmann, for the [ESP32-A2DP](https://github.com/pschatzmann/ESP32-A2DP) library that
  handles Bluetooth audio.
- [Claude](https://www.anthropic.com/claude) and [ChatGPT](https://chatgpt.com), for making this
  possible.

The factory schematic of the TM-838N is included for reference. The documents also cite the SAMS
Photofact for this set, which is copyrighted and not included.

## License

- Firmware: GPL-3.0-or-later
- Hardware design files (KiCad, STL): CERN-OHL-S-2.0
- Documentation and photos: CC BY-SA 4.0

You can use any of it, change it and share it, commercially too, as long as what you share stays
under the same license. Details and exceptions are in [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE).
