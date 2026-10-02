# Appendix I. Pins as the firmware uses them

This appendix is for anyone who changes the firmware, ports it to other boards, or checks a pin with a
meter or a scope. It lists, for both boards, every pin the firmware drives or reads, the name the firmware
gives it, and what the firmware does with it: input or output, any internal pull-up it switches on, and
the level it treats as active. It says nothing about what a pin is wired to. **For wiring, see the
Hardware Bible, chapter 4** (section 4.2 for the main board, section 4.3 for the audio board), and its
chapter 10 for the cables.

All the names come from one file, `include/pins.h`, compiled into both programs. It is the firmware's pin
authority: a pin moves there and nowhere else. The peripherals behind these pins (timers, PWM channels,
buses) are listed in chapter 4, section 4.12.4.

## I.1 Main board (S3)

| GPIO | Firmware name | Direction, as set | What the firmware does with it | Chapter |
|---|---|---|---|---|
| 1 | `S3_AMP_SENSE` | input, plain (no internal pull) | Read on every pass of the main loop. **Low = amplifier on.** A change counts after 50 ms. | 4, section 4.5 |
| 2 | `S3_DIGIT_2` | output | Digit select for the tens of minutes. **High = digit on.** In the low output register. | 4, section 4.6 |
| 5 | `S3_INDEX_HALL` | input, plain (no internal pull) | The needle's index. **Low = index magnet present.** | 5, section 5.8 |
| 6 | `S3_SEG_A` | output | Segment A. Driven **high** during a digit's lit window when the segment is in that digit's pattern; high lights the segment. In the low output register. | 4, section 4.6 |
| 7 | `S3_SEG_B` | output | Segment B, as segment A. | 4, section 4.6 |
| 8 | `S3_SEG_F` | output | Segment F, as segment A. | 4, section 4.6 |
| 9 | `S3_STEPPER_I1` | output, PWM | Needle motor phase; LEDC channels 2–5 at 20 kHz serve the four phases. | 5, section 5.4 |
| 10 | `S3_STEPPER_I2` | output, PWM | Needle motor phase, as above. | 5, section 5.4 |
| 11 | `S3_STEPPER_I3` | output, PWM | Needle motor phase, as above. | 5, section 5.4 |
| 12 | `S3_STEPPER_I4` | output, PWM | Needle motor phase, as above. | 5, section 5.4 |
| 13 | `S3_UART_TX` | output, serial | The link to the A32: sends, on `Serial2` at 921 600 baud, pin given explicitly. | 9, section 9.5 |
| 14 | `S3_UART_RX` | input, serial | The link to the A32: receives. | 9, section 9.5 |
| 15 | `S3_SEG_C` | output | Segment C, as segment A. | 4, section 4.6 |
| 16 | `S3_SEG_D` | output | Segment D, as segment A. | 4, section 4.6 |
| 17 | `S3_SEG_E` | output | Segment E, as segment A. | 4, section 4.6 |
| 18 | `S3_SEG_G` | output | Segment G, as segment A. | 4, section 4.6 |
| 21 | `S3_PANEL_PWM` | output, PWM | The panel lamps: LEDC channel 1, 1 kHz, 8-bit duty. **Higher duty = brighter.** | 4, section 4.7 |
| 38 | `S3_AS5600_SDA` | I2C data | Bus `Wire` (I2C 0), 100 kHz: the tuning-angle sensor (AS5600) at address 0x36. | 5 |
| 39 | `S3_AS5600_SCL` | I2C clock | As above. | 5 |
| 40 | `S3_DIGIT_3` | output | Digit select for the units of hours. High = digit on. In the high output register. | 4, section 4.6 |
| 41 | `S3_DIGIT_4` | output | Digit select for the tens of hours (the leftmost digit). High = digit on. In the high output register. | 4, section 4.6 |
| 42 | `S3_DIGIT_1` | output | Digit select for the units of minutes (the rightmost digit). High = digit on. In the high output register. | 4, section 4.6 |
| 47 | `S3_RDA_SDA` | I2C data | Bus `Wire1` (I2C 1), 100 kHz: the RDA5807M. Printed by console `R`. | 6, section 6.6 |
| 48 | `S3_RDA_SCL` | I2C clock | As above. | 6, section 6.6 |

**The rule the display depends on.** All seven segment pins stay below GPIO 32, because the display
interrupt clears segments through the low output register in one write; digit pins may be anywhere
(chapter 4, section 4.17).

**Other values in the same file:** `S3_I2C_HZ` = 100 000, the clock of both I2C buses; `AS5600_ADDR` =
0x36. `RDA5807_ADDR` (0x11) is defined but unused: the tuner's driver carries its own address constant
(chapter 6, section 6.6).

**What no S3 pin does.** No S3 pin drives a fan, the display's colon, the blue Bluetooth lamp, or anything
on the A32's side. GPIO 4 is not used; `pins.h` calls it "the only spare".

**Pins the S3 firmware leaves alone**, as `pins.h` lists them. A new function must take a pin outside
these, and outside those already used.

| GPIO | Why |
|---|---|
| 35, 36, 37 | Taken by the octal PSRAM of an R8 module, per the module's datasheet. |
| 0, 3, 45, 46 | Strapping pins, read by the chip at reset. |
| 19, 20 | Native USB: the console. |
| 43, 44 | The chip's first UART, kept free. |

## I.2 Audio board (A32)

| GPIO | Firmware name | Direction, as set | What the firmware does with it | Chapter |
|---|---|---|---|---|
| 0 | `A32_PCM1802_MCLK` | output | The I2S master clock, 11.2896 MHz (256 × 44 100), for both converters. Forced: the classic ESP32 can put the I2S master clock only on GPIO 0, 1 or 3, and 1 and 3 are its console. Never detached from the I2S peripheral. Pad drive 3 at start-up. | 10 |
| 4 | `A32_PCM5102_DIN` | output | I2S data out, to the DAC. Its pad drive can be changed from the portal (`sys.dindrv`, 0 to 3). | 10 |
| 17 | `A32_I2S_LRCK` | output | The shared I2S frame clock (LRCK), 44.1 kHz. Pad drive 3 at start-up. | 10 |
| 18 | `A32_I2S_BCK` | output | The shared I2S bit clock (BCK), 2.8224 MHz. Pad drive 3 at start-up. | 10 |
| 19 | `A32_PCM1802_DOUT` | input | I2S data in, from the ADC. | 10 |
| 21 | `A32_RTC_SCL` | I2C clock | Bus `Wire`, 100 kHz (`A32_I2C_HZ`): the battery clock (DS3231) at address 0x68. Starting the bus switches on the ESP32's internal pull-ups. | 9, section 9.9 |
| 22 | `A32_RTC_SDA` | I2C data | As above. | 9, section 9.9 |
| 25 | `A32_BT_BUTTON` | input, **internal pull-up on** | The Bluetooth pair button. **Low = pressed.** Debounced in firmware: a press counts on release, after being held more than 30 ms. | 10 |
| 26 | `A32_UART_RX` | input, serial | The link to the S3: receives, on `Serial2` at 921 600 baud, pin given explicitly. | 9, section 9.5 |
| 27 | `A32_UART_TX` | output, serial | The link to the S3: sends. | 9, section 9.5 |
| 33 | `A32_BT_LED` | output, PWM | The blue Bluetooth lamp: LEDC channel 0, 2 kHz, 8-bit. Driven **active low**: the firmware pulls the pin low to light the lamp. It drives it fully on for 300 ms of every 600 ms in the LINK state, at the top of every LOOK breath, and in both FOUND flashes (the lamp's states are in chapter 10). | 10 |
| 35 | `A32_VOLUME_POT` | input, analogue | The volume knob, read every 50 ms with the ESP32's own ADC at its widest input range (11 dB attenuation): two throw-away conversions, then the median of five. | 10 |
| 36 | `A32_MODE_ADC` | input, analogue | The source selector, read every 50 ms: three throw-away conversions (this pin reads high for a while after another channel of the ESP32's own ADC has been sampled), then the median of five. Raw below 424 = AUX, 2471 and above = RADIO, anything between = BT. | 10 |

**Other values in the same file:** `A32_I2C_HZ` = 100 000; the selector thresholds
`A32_MODE_THRESH_AUX_BT` = 424 and `A32_MODE_THRESH_BT_RADIO` = 2471, which the S3's console `s` also
prints. `DS3231_ADDR` (0x68) and `AT24C32_ADDR` (0x57) are defined but unused: the battery clock's driver
carries its own address constant.

**Why the link's pins are given explicitly.** The classic ESP32's default pins for `Serial2` are GPIO 16
and 17, and GPIO 17 is the I2S frame clock. A serial port opened without its pins would silently take it
(chapter 9, section 9.14.2).

**What no A32 pin does.** No A32 pin drives the DAC's hardware mute: the mute is software only, and
GPIO 16 is unused (chapter 10).

**Pins the A32 firmware leaves alone or does not use**, as `pins.h` lists them:

| GPIO | Status |
|---|---|
| 2, 5, 12, 15 | Strapping pins: left alone. |
| 6 to 11 | The flash chip: left alone. |
| 34 to 39 | Inputs only, with no internal pull-ups or pull-downs; the volume knob (35) and the selector (36) use two of them. |
| 13, 14, 23, 32 | Unused. |
| 16 | Unused. GPIO 16 and 17 are free only on a module without PSRAM: a PSRAM module takes them, and GPIO 17 is the I2S frame clock. |
