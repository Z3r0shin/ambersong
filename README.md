# Ambersong

*An old soul, glowing anew into the modern world.*

A LLOYD'S TM-838N tube radio from the early sixties, rebuilt around two ESP32 boards that
add Bluetooth, a retro-looking clock and a needle over a brass dial that follows the tuning.

## How it started

I found this radio in the trash, in a pile of other junk. The enclosure was bashed in and both the
power cord and antennas were pulled out. I tried it anyway, curious. Even though all it gave me was 
static, it was warm static, kind of like warm wind at night. I wasn't ready to throw that away.

So, I started restoring it. A couple of the old components had to be replaced. The tubes, though, 
still had enough life in them for a lot more years of use. If the tubes could last that long, I could
make something durable out if it, an heirloom.

I wanted it to look warm and cozy, mostly as an excuse and a challenge, to show off some of my 
woodworking skills and try some brass work. The cabinet is oiled oak and walnut, brass, and matte 
black accents. That cabinet, wood and brass together, is the part I'm proudest of. When it's on, 
it glows amber and still feels analog in all the good parts.

From there, the goal was to make it last for decades and write every part of it down so someone else 
could build their own (or use it as help/guide for something similar), repair one or change something.

## What it does

The tube set still does the receiving. Its original knob still turns its own tuning capacitor.
A magnetic angle sensor reads the capacitor's shaft and a small stepper motor moves the needle to 
match. The frequency scale isn't a straight line, so the radio calibrates itself. While it plays, 
an RDA5807M FM chip listens for the tube set's local oscillator, works out which station the set 
is really tuned to and corrects the needle's curve in firmware.

The tube radio's sound output goes through a PCM1802 converter into the audio board, out through a
PCM5102A DAC to the amplifier and speakers of a repurposed Kramer Tavor 5-O pair. A front DPDT switch 
changes the audio input from radio, Bluetooth or an AUX input. Four LED-filament digits glass
show the time from a battery-backed RTC that sets itself over the network. A small web page on
the home network shows and changes every setting and updates both boards over WiFi.

## Why a Bible and a Gospel

Two documents describe the whole machine: the Hardware Bible for everything physical and the
Firmware Gospel for the code. Each one is meant to be the book you trust for its half of the
radio. The religious names are iconography, nothing more: I'm not religious, I just like the
symbols.

- [`docs/HARDWARE-BIBLE.md`](docs/HARDWARE-BIBLE.md): what is wired to what, fitted values,
  the tube radio circuit, the FM antenna, the amplifier, the cabinet and the printed parts.
- [`docs/FIRMWARE-GOSPEL.md`](docs/FIRMWARE-GOSPEL.md): what the firmware does and why,
  every setting, what was tried and dropped, and how to build, flash, update and recover it.

## Building your own

Your radio, cabinet and amplifier probably won't be the same as mine and the documents are 
written with that in mind. You could source your own parts and make it painful to you too to 
make everything work just enough that passed that pissed off state you can accept the flaws and
embrace the result. Parts that only make sense for this build are marked as such. On the firmware
side, expect to change pins, sensor directions, the tuner's travel and the needle's limits; the 
Gospel should say where each one lives.

The firmware is one PlatformIO project with two environments, `s3` for the main board (ESP32-S3,
N16R8) and `a32` for the audio board (ESP32). The toolchain is pinned to `espressif32@7.0.1`, just
to make sure that at least some stuff will translate correctly to you.

## Safety

Many tube radios from this era have no isolation transformer. If you find/use an old tube radio,
check if the chassis is tied to mains. Mine was, and since in those years the wall plug was reversible,
either live/hot or neutral could go to the whole chassis. Unplug it before you open it/play with such
a tube radio. Hardware Bible §31.1 covers what that means in this build.

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
- My stupidity for making me do this and convincing myself that its a good idea.

The factory schematic of the TM-838N is included for reference. The documents also cite the SAMS
Photofact for this set, which is copyrighted and not included.

## Disclaimer

I have barely coded anything in this. Its all been vibe-coded with Claude Code and Codex. The artwork,
design, and anything other than sound and electrical engineering and the actual coding were done
without AI, by me with the help of my friends and family.

## License

- Firmware: GPL-3.0-or-later
- Hardware design files (KiCad, STL): CERN-OHL-S-2.0
- Documentation and photos: CC BY-SA 4.0

You can use any of it, change it and share it, commercially too, as long as what you share stays
under the same license. Details and exceptions are in [`LICENSE`](LICENSE) and [`NOTICE`](NOTICE).
