#pragma once
// ============================================================================
//  GARAGE RADIO - PIN MAP, SINGLE SOURCE OF TRUTH FOR THE FIRMWARE'S PINS
//
//  The firmware takes every pin number from this file. If
//  the firmware's use of a pin changes, it changes HERE and nowhere else -
//  duplicating pin numbers across twenty test sketches is how they drift.
//
//  This file says which pin the firmware drives or reads. What each pin is
//  WIRED to is the Hardware Bible's ("docs/HARDWARE-BIBLE.md",
//  §2 for the S3, §3 for the A32). Where a comment here disagrees with the
//  Bible, the Bible is right and the comment is stale.
// ============================================================================


// ####################  MAIN MCU - ESP32-S3 N16R8 (Hardware Bible §1)  #######

// --- Amp power sense -------------------------------------------------------
//  The firmware reads LOW as "amp POWERED" (the PC817 sense: Hardware Bible
//  §7, §22).
#define S3_AMP_SENSE        1

// --- Leditron display: segment sinks (Hardware Bible §2, §9) ---------------
#define S3_SEG_A            6
#define S3_SEG_B            7
#define S3_SEG_C           15
#define S3_SEG_D           16
#define S3_SEG_E           17
#define S3_SEG_F            8      // CORRECTED 2026-08-27, was 18
#define S3_SEG_G           18      // CORRECTED 2026-08-27, was 8
//  F and G were swapped. Author drove each and reported which bar lit:
//  the pin previously named G lights the TOP-LEFT bar (F), and vice versa.

// --- Leditron display: digit select ----------------------------------------
//  Driving a digit pin HIGH turns its digit ON (the driver chain is in the
//  Hardware Bible §9). Only ever one digit at a time, and blank before
//  switching.
//  CORRECTED 2026-08-27. Author observed: driving the pin then named DIGIT_1 lit
//  physical digit 2, and the pin named DIGIT_3 lit physical digit 4, so 1<->2
//  and 3<->4 were swapped. The author has since ruled this order right ("Pins.h
//  is right.", Hardware Bible §0).
#define S3_DIGIT_1         42      // was 2
#define S3_DIGIT_2          2      // was 42, confirmed: this pin lights digit 2
#define S3_DIGIT_3         40      // was 41
#define S3_DIGIT_4         41      // was 40, confirmed: this pin lights digit 4

// --- Stepper driver inputs IN1..IN4 (Hardware Bible §11) -------------------
//  drive.cpp drives these in this order, I1..I4 = 9, 10, 11, 12, through LEDC
//  with its own half-step table. AccelStepper is not used.
#define S3_STEPPER_I1       9
#define S3_STEPPER_I2      10
#define S3_STEPPER_I3      11
#define S3_STEPPER_I4      12

// --- Index sensor ----------------------------------------------------------
//  One Hall switch, read active LOW with no internal pull-up (the sensor and
//  its pull-up: Hardware Bible §11). It is a reference mark, NOT an end-stop.
//  Travel is bounded by soft limits in needle.cpp, and every homing phase runs
//  under a step budget so a dead sensor faults instead of driving on forever.
//  It replaced the two limit switches on 2026-08-30.
#define S3_INDEX_HALL       5
//  S3_LIMIT_LEFT (was 5) and S3_LIMIT_RIGHT (was 4) were deleted on
//  2026-09-20. Neither was used anywhere in src/ or include/. LEFT only
//  aliased the Hall pin above, which is a trap. The firmware does not use
//  GPIO4 (Hardware Bible §2).

// --- Inter-MCU UART (the link: Hardware Bible §2, §3) ----------------------
#define S3_UART_TX         13      // TX; the A32 receives on A32_UART_RX
#define S3_UART_RX         14      // RX; the A32 sends on A32_UART_TX

// --- Panel lighting --------------------------------------------------------
//  One PWM output that switches the panel lamps (Hardware Bible §11). Nothing
//  here computes from the lamp hardware: the panel runs LEDC channel 1 at
//  1 kHz, 8-bit.
#define S3_PANEL_PWM       21

// --- AS5600 tuning position encoder (I2C) ----------------------------------
//  100 kHz, NOT 400. The bus's pull-ups, series resistors and supply are in
//  the Hardware Bible (§2, §11).
#define S3_AS5600_SDA      38
#define S3_AS5600_SCL      39
#define S3_I2C_HZ      100000
#define AS5600_ADDR      0x36

// --- RDA5807M, the tuning auto-calibration instrument (Firmware Gospel §6) --
//  ITS OWN BUS, deliberately. The AS5600 bus above is the tuning sensor and
//  is kept to that one device; the RDA5807M's I2C address is fixed in silicon
//  (0x11 random access, 0x10 sequential, 0x60 TEA5767) with no select pin.
//  rda.cpp runs it at S3_I2C_HZ.
//
//  GPIO47/48 are the author's choice, 2026-09-04. GPIO48 also drives the S3
//  board's RGB LED, which lights now (Hardware Bible §2, §22) - judged
//  cosmetic: an addressable LED's data input is high impedance.
//
//  The module's supply decoupling and its antenna (30CM_ANT) are in the
//  Hardware Bible §11. SAFETY: the tube chassis is MAINS-REFERENCED (Hardware
//  Bible §0: CHASSIS is on mains neutral); the module, its ground and its
//  antenna must touch none of it. The antenna is deliberately inefficient:
//  bad at distant stations and adequate for the tube set's own oscillator
//  close by, and that selectivity is what separates the LO from the
//  broadcast band.
#define S3_RDA_SDA         47
#define S3_RDA_SCL         48
#define RDA5807_ADDR     0x11      // NOT USED: rda.cpp keeps its own addresses

// --- Reserved / unusable on the S3 -----------------------------------------
//  GPIO35, 36, 37 : consumed by the octal SPI PSRAM on an R8 module. Espressif
//                   ESP32-S3-WROOM-1 datasheet, Pin Definitions footnote (b).
//                   Confirmed on real silicon by reading PSRAM size.
//  GPIO0, 3, 45, 46 : strapping pins, leave alone.
//  GPIO19, 20     : native USB D-/D+ (the back-panel USB-C: Hardware Bible §2).
//  GPIO43, 44     : U0TXD / U0RXD console. Keep free.
//  GPIO47, 48     : the RDA5807M's I2C bus, see above.
//  GPIO4          : not used by the firmware - the only spare (what is still
//                   on it: Hardware Bible §2).


// ##########  AUDIO MCU - ESP32-WROOM-32 ("A32", Hardware Bible §1)  #########

// --- PCM1802 ADC (radio capture) -------------------------------------------
//  MCLK is FORCED to GPIO0: the classic ESP32 can only emit the I2S master
//  clock on GPIO0, 1 or 3, and 1/3 are the console. Do not "fix" this. The
//  same clock also reaches the DAC (Hardware Bible §3) - see audio.cpp.
#define A32_PCM1802_MCLK    0
#define A32_PCM1802_DOUT   19

// --- PCM5102A DAC ----------------------------------------------------------
#define A32_PCM5102_DIN     4
//  NO XSMT PIN. The A32 reaches nothing on the DAC's soft-mute input: GPIO16
//  is unused and the DAC module pulls XSMT up itself (Hardware Bible §6, author
//  rulings Q7 and M1). There is no hardware mute; mute is software only,
//  see audio.h. A32_PCM5102_XSMT (was 16) was deleted 2026-09-23 - it drove an
//  unconnected pad.

// --- Shared I2S clocks (both converters: Hardware Bible §3) ----------------
#define A32_I2S_LRCK       17
#define A32_I2S_BCK        18

// --- DS3231 RTC (I2C) ------------------------------------------------------
//  CORRECTED 2026-08-27 -- these were the other way round here. Bench testing
//  read the DS3231 correctly only with SDA = 22 and SCL = 21, and the Hardware
//  Bible (§3, §5) records the same.
#define A32_RTC_SDA        22
#define A32_RTC_SCL        21

//  PULL-UPS: the Hardware Bible (§5) draws none on this bus. Wire.begin()
//  enables the ESP32's internal pull-ups on both pins, and the firmware relies
//  on those plus whatever the RTC module carries.
#define A32_I2C_HZ     100000
#define DS3231_ADDR      0x68      // NOT USED: rtc.h keeps its own address
#define AT24C32_ADDR     0x57      // NOT USED: the module's EEPROM (Hardware
                                   // Bible §22); nothing here talks to it

// --- Inter-MCU UART (the link: Hardware Bible §2, §3) ----------------------
//  MUST be given explicit pins. Serial2's DEFAULT pins on the classic ESP32
//  are GPIO16/17 -- and GPIO17 is LRCK here. Calling Serial2.begin()
//  without pin arguments would silently break the DAC.
#define A32_UART_RX        26      // RX; the S3 sends on S3_UART_TX
#define A32_UART_TX        27      // TX; the S3 receives on S3_UART_RX

// --- Source select ---------------------------------------------------------
//  The front selector's resistor ladder, read on ADC1 (the switch and ladder:
//  Hardware Bible §11). GPIO36 is input-only and on ADC1, which is the correct
//  choice (ADC2 is unusable whenever WiFi/BT is active -- and this MCU runs
//  Bluetooth).
#define A32_MODE_ADC       36

//  FRONT PANEL VOLUME POT, added 2026-08-29. The pot is a SENSOR: the firmware
//  reads its wiper and sets the volume; no audio goes to the front panel. Its
//  wiring is in the Hardware Bible (§3, §7).
//  GPIO35 is ADC1_CH7, input-only, no internal pulls - which is what a pot
//  wants - and not one of the SENSOR_VP/VN pins, so it does not share GPIO36's
//  amplifier settling.
//
//  MOVED 34 -> 35 ON 2026-08-31, because GPIO34, 39 and 36 then read as
//  joined, and GPIO36 is the source-select ladder: the knob alone could select
//  all three sources, light the BT lamp on AUX, and push the ladder off its
//  levels. They are not joined now (Hardware Bible §3, §26); the
//  pot stays on GPIO35.
#define A32_VOLUME_POT     35
//  THRESHOLDS, from levels bench-tested on 2026-08-27: AUX = 0,
//  BT = 848, RADIO = 4095 raw. Which switch position gives which level is in
//  the Hardware Bible §11. Each threshold is the midpoint between neighbouring
//  levels (separation 848 and 3247 counts). Do not reuse the older
//  ">3000 = BT / <500 = AUX" pair: it separates the clusters but calls 4095
//  "BT" when it is RADIO, and 848 "RADIO" when it is BT.
#define A32_MODE_THRESH_AUX_BT     424   // raw below this          -> AUX
#define A32_MODE_THRESH_BT_RADIO  2471   // raw at or above this    -> RADIO
                                         // between the two         -> BT

// --- Bluetooth LED and pair button (Hardware Bible §11) --------------------
//  RESOLVED 2026-08-27 by a bench test that watched both candidate pins under
//  an internal pull-up AND an internal pull-down and saw which one moved on a
//  press, assuming neither the pin assignment nor the polarity. The author
//  later ruled these pins the truth (Hardware Bible §0).
#define A32_BT_BUTTON      25      // ACTIVE LOW, INPUT_PULLUP
#define A32_BT_LED         33      // ACTIVE LOW: drive LOW to light

//  The LED: the Hardware Bible (§11) draws no series resistor. The firmware
//  drives it only through LEDC PWM; its brightnesses are in btled.h.

// --- Do not use on the classic ESP32 ---------------------------------------
//  GPIO2, 5, 12, 15 : strapping.
//  GPIO6-11         : SPI flash.
//  GPIO34-39        : input-only, no output drive, no internal pull-ups.
//  Unused by this firmware : 13, 14, 23, 32.
//  GPIO16/17 are usable ONLY on a module without PSRAM (a WROOM, not a
//  WROVER: Hardware Bible §1). A WROVER's PSRAM takes 16 and 17, which would
//  break LRCK (GPIO17). The build checks for this by reading PSRAM size -- a WROVER
//  reports non-zero and must be rejected.
