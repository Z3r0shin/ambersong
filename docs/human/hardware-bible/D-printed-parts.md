# Appendix D. Printed parts

This appendix lists every printed part of the build. You need it when you reprint a broken or lost
part, or when you build a cabinet of your own.

Every printed part is an STL file (the file a 3D printer works from) in the `hardware/STLs/` folder of
the published Ambersong project files, named as in the tables below: 76 files. **All are PLA, except the passive radiator's membrane,
which is TPU** (a flexible filament). They are the author's parts for this cabinet and these boards;
adapt them to yours. In the file names, MCU means a microcontroller board, PSU a power supply, and
LEDITRON the clock display.

Not printed: the source switch and the power switch (fixed in the wooden front panel), the mains inlet
(on the amplifier's aluminium back plate), and the standoffs of the amplifier's ±20 V supply, which are
plastic screws and standoffs.

## D.1 Speakers

These are the passive radiators that replace the speakers' original ports, and the speaker holder
(chapter 12, section 12.1).

| File | What it is |
|---|---|
| `AUDIO - Passive Radiator - Holder.stl` | passive radiator: holder |
| `AUDIO - Passive Radiator - Inside Protector.stl` | passive radiator: inside protector |
| `AUDIO - Passive Radiator - Membrane Weight.stl` | passive radiator: membrane weight |
| `AUDIO - Passive Radiator - Outside Protector.stl` | passive radiator: outside protector |
| `AUDIO - Passive Radiator - TPU Membrane.stl` | passive radiator: membrane, **printed in TPU** |
| `AUDIO - Speakers Holder and Seal.stl` | speaker holder and seal |

## D.2 Back

The files whose names begin with BACK are the back panel's anchors and vents, and the parts for the
USB-C ports and the antenna connector (chapter 3, sections 3.1 and 3.4).

| File | What it is |
|---|---|
| `BACK - Dual USB-C Ports.stl` | the two back-panel USB-C ports (chapter 4, section 4.6) |
| `BACK - Electronics - BNC Antenna Interface.stl` | the antenna connector's interface: the connector (J10) is a BNC on the back panel (chapter 11) |
| `BACK - Fan Exhaust.stl` | the box fan's exhaust |
| `BACK - Fan Intake.stl` | the louvred air intakes |
| `BACK - Panel Anchors Bottom.stl` | back panel anchors, bottom |
| `BACK - Panel Anchors Inner Corners.stl` | back panel anchors, inner corners |
| `BACK - Panel Anchors Outer Corners.stl` | back panel anchors, outer corners |
| `BACK - Panel Anchors.stl` | back panel anchors |

## D.3 Front panel

These are the parts of the front panel, the FM dial and the tuning knob (chapter 3, section 3.2;
chapter 8, section 8.1).

| File | What it is |
|---|---|
| `FRONT - FM Dial - Back.stl` | FM dial back; it is also the stepper motor's mount |
| `FRONT - FM Dial - Cover.stl` | FM dial cover |
| `FRONT - FM Dial - Spacer.stl` | FM dial spacer |
| `FRONT - FM Dial - Stepper Needle and Magnet Holder.stl` | the mount on the stepper motor's shaft that carries the needle and the index magnet |
| `FRONT - Panel Anchors - Insidemost.stl` | front panel anchors, insidemost |
| `FRONT - Panel Anchors - Outsidemost.stl` | front panel anchors, outsidemost |
| `FRONT - Tuner Pot Knob.stl` | the printed front tuning knob. On the printed shaft extension, it turns the radio's original tuning knob, which drives the dial cord and the tuning capacitor (chapter 3, section 3.2). "Pot" is the file's word: tuning is by the tuning capacitor. |
| `FRONT - Tuning Pot Extender PLA Shaft.stl` | the tuning shaft extension |
| `FRONT - Tuning Shaft Bearing Seat.stl` | the tuning shaft's bearing seat |
| `FRONT - Volume Pot Knob.stl` | the front volume knob (chapter 9, section 9.4) |

## D.4 Board cases and holders, inside the cabinet

Each board or module sits in its own printed case, or on a printed carrying plate, screwed to the
cabinet (chapter 3, section 3.1).

| File | What it is |
|---|---|
| `IN - Electronics - 5V BUS case Bottom.stl` | 5 V bus case, bottom (chapter 5, section 5.5) |
| `IN - Electronics - 5V BUS case Top.stl` | 5 V bus case, top |
| `IN - Electronics - 5V PSU Case Bottom.stl` | 5 V power supply case, bottom |
| `IN - Electronics - 5V PSU Case Top.stl` | 5 V power supply case, top |
| `IN - Electronics - 5V+3V3 BUS case Bottom.stl` | 5 V and 3.3 V bus case, bottom |
| `IN - Electronics - 5V+3V3 BUS case Top.stl` | 5 V and 3.3 V bus case, top |
| `IN - Electronics - AC Line Filter Case.stl` | mains line filter case (chapter 5, section 5.4) |
| `IN - Electronics - AS5600 Case - Bottom.stl` | the angle sensor's cage, bottom (chapter 5, section 5.4) |
| `IN - Electronics - AS5600 Case - Connecting Rod.stl` | the printed shaft that carries the angle sensor's magnet (chapter 8, section 8.1) |
| `IN - Electronics - AS5600 Case - Top.stl` | the angle sensor's cage, top |
| `IN - Electronics - Audio Listener Case BOTTOM.stl` | the source-switch board's case, bottom (chapter 9, section 9.3) |
| `IN - Electronics - Audio Listener Case TOP.stl` | the source-switch board's case, top |
| `IN - Electronics - Audio MCU Case BOTTOM.stl` | audio board case, bottom |
| `IN - Electronics - Audio MCU Case TOP.stl` | audio board case, top |
| `IN - Electronics - Backing Panel 1.stl` | a panel that holds several of the cases |
| `IN - Electronics - Bluetooth LED Holder.stl` | Bluetooth LED holder |
| `IN - Electronics - Bluetooth LED Pusher.stl` | Bluetooth LED pusher (the LED is also the button) |
| `IN - Electronics - Box Fan LM7812 Case Bottom.stl` | the box fan's regulator case, bottom (chapter 5, section 5.6) |
| `IN - Electronics - Box Fan LM7812 Case Top.stl` | the box fan's regulator case, top |
| `IN - Electronics - DS3231 RTC Case BOTTOM.stl` | real-time clock module case, bottom |
| `IN - Electronics - DS3231 RTC Case TOP.stl` | real-time clock module case, top |
| `IN - Electronics - Digit Drivers Case BOTTOM.stl` | digit driver case, bottom (chapter 7) |
| `IN - Electronics - Digit Drivers Case TOP.stl` | digit driver case, top |
| `IN - Electronics - Front Panel FM LEDs Case BOTTOM.stl` | FM panel lamp board case, bottom (chapter 9, section 9.1) |
| `IN - Electronics - Front Panel FM LEDs Case TOP.stl` | FM panel lamp board case, top |
| `IN - Electronics - Hall Sensor Stepper Limit Case.stl` | the needle index sensor's case (chapter 8, section 8.3) |
| `IN - Electronics - LEDITRON Back.stl` | clock display back |
| `IN - Electronics - LEDITRON Bus Case Bottom.stl` | clock display bus case, bottom |
| `IN - Electronics - LEDITRON Bus Case TOP.stl` | clock display bus case, top |
| `IN - Electronics - PCM1802 ADC Case BOTTOM.stl` | ADC case, bottom |
| `IN - Electronics - PCM1802 ADC Case TOP.stl` | ADC case, top |
| `IN - Electronics - PCM5102A DAC Case Bottom.stl` | DAC case, bottom |
| `IN - Electronics - PCM5102A DAC Case TOP.stl` | DAC case, top |
| `IN - Electronics - Power Detect Case BOTTOM.stl` | amplifier power sensor case, bottom (chapter 9, section 9.5) |
| `IN - Electronics - Power Detect Case TOP.stl` | amplifier power sensor case, top |
| `IN - Electronics - RDA5807M - Bottom.stl` | FM calibration receiver case, bottom (chapter 8, section 8.5) |
| `IN - Electronics - RDA5807M - Cage Holder.stl` | FM calibration receiver cage holder |
| `IN - Electronics - RDA5807M - Middle Bottom.stl` | FM calibration receiver case, middle bottom |
| `IN - Electronics - RDA5807M - Middle TOP.stl` | FM calibration receiver case, middle top |
| `IN - Electronics - RDA5807M - TOP.stl` | FM calibration receiver case, top |
| `IN - Electronics - S3 Main MCU Backing.stl` | main board backing |
| `IN - Electronics - S3 Main MCU Case BOTTOM.stl` | main board case, bottom |
| `IN - Electronics - S3 Main MCU Case TOP.stl` | main board case, top |
| `IN - Electronics - Stepper Driver ULN2003A Case - Bottom.stl` | needle motor driver case, bottom (chapter 8, section 8.2) |
| `IN - Electronics - Stepper Driver ULN2003A Case - Top.stl` | needle motor driver case, top |

## D.5 The tube radio's Faraday cage

These two parts make the cage over the tube radio (chapter 3, section 3.5; chapter 5, section 5.4).

| File | What it is |
|---|---|
| `IN - Tube Radio - Faraday Cage 1.stl` | the tube radio's Faraday cage, part 1; the cage fan's mount is part of it |
| `IN - Tube Radio - Faraday Cage 2.stl` | the tube radio's Faraday cage, part 2 |

## D.6 Clock display

These are the clock display's housing and digits (chapter 7).

| File | What it is |
|---|---|
| `LEDITRON - Colon - Blanket.stl` | colon blanket |
| `LEDITRON - Colon.stl` | colon |
| `LEDITRON - Digit.stl` | digit |
| `LEDITRON - Front Case.stl` | front case |
| `LEDITRON - Glass Spacer.stl` | glass spacer |
