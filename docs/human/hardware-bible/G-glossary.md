# Glossary

The terms and names you meet in this book, in alphabetical order. Look a term up here whenever a
chapter uses it without explaining it, or when you open an appendix. Signal names are in `code font`, as
on the drawings. Where a term has its own section, the chapter is given.

**`20V_GND`** — The amplifier supply return: the middle (0 V) output of the amplifier's ±20 V supply.
One of the machine's four grounds. Its two middle black wires go to the amplifier's ground, so it is joined to
Earth and to the DC ground (chapter 5, section 5.3).

**`5VDC`** — The 5 V bus, from the 5 V power supply. `5VDC_S` is the same conductor, named before the
bus (chapter 5, section 5.5).

**A32** — The audio board: the ESP32 development board that handles sound, Bluetooth, the front
controls and the real-time clock (chapter 4).

**`A_IOn`** — A signal on pin GPIOn of the audio board: `A_IO22` is its GPIO22.

**`A_VDC`, `A_3V3`** — Two names for one rail: the audio side's 3.3 V, from the audio board's 3.3 V
output, shared out by the 3.3 V bus.

**Absolute maximum** — A maker's limit that must not be exceeded, even briefly. It is not a working
value (Appendix A).

**AC/DC set** — A tube radio with no mains transformer. Its tube heaters are wired in series across
the mains, and its B+ is rectified straight from the mains, so its chassis is tied to the mains. The
tube radio here is one (chapters 2 and 13).

**ADC** — Analogue-to-digital converter: here a PCM1802 board that turns the tube radio's audio into
digital samples for the audio board (chapter 6).

**AFC** — Automatic frequency control: a voltage from the FM detector that keeps the FM oscillator on
the station. Switched on in the radio's FM-AFC position (chapter 13).

**Alignment** — Adjusting a radio's trimmers, padders and IF transformers so that its tuned circuits
track together across the band. Only the tube radio's FM path was professionally tuned; its AM and SW
path was not retuned (chapter 13, sections 13.1.1 and 13.7).

**AM** — Amplitude modulation; on this radio, the medium-wave band. See also SW and FM.

**`AMP_` signals** — The cable sheet's names for the pins of the amplifier's input panel: `AMP_L+`,
`AMP_L-`, `AMP_R+`, `AMP_R-` and `AMP_G`, the ground (chapters 6 and 12).

**ASA** — A weather-resistant plastic. The antenna's element bushings, their plates and the feedpoint
clamp are ASA (chapter 11, section 11.3).

**Attenuator** — A resistor network that lowers a signal's level. The radio's output passes one on its
way into the ADC, starting with a 4.7 kΩ resistor (chapter 6, section 6.4).

**Audio board** — The ESP32 development board that handles the sound, Bluetooth, the front controls and
the real-time clock. Also written A32 (chapter 4).

**AUX** — The auxiliary analogue input. The AUX jack reaches the amp's input panel, which sums it with
the other inputs; the run is not drawn. It never passes through the audio board (chapter 6, section 6.7).

**AVC** — Automatic volume control: a voltage from the AM detector that lowers the gain of earlier
stages on strong stations, so that strong and weak stations come out at similar levels (chapter 13).

**AWG** — American Wire Gauge: a wire-size scale in which a larger number means a thinner wire.

**B+** — The tube radio's high-voltage DC supply for its tubes, made by rectifying the mains. The
numbers in its point names (`B130`, `B115_A`) are the original service data's nominal figures, not
measured voltages (chapter 2; chapter 13, section 13.10).

**Balanced input** — An input that takes each channel on two wires, a plus and a minus, and amplifies the
difference between them. The DAC reaches the amplifier through one (chapter 12, section 12.3).

**BCK, LRCK, SCK** — The three clocks of the I2S audio bus: the bit clock, the word clock (left and
right samples) and the master or system clock (chapter 6, section 6.2).

**BNC** — A coaxial connector with a bayonet lock. The machine's antenna connector, on the back panel,
is one (chapter 11).

**Boom** — The metal bar that carries an antenna's elements (chapter 11).

**Bus** — Here, a star-point bus: a row of 2-pin power sockets in which every power pin is one wire and
every ground pin is one wire. The machine has a 5 V bus and a 3.3 V bus (chapter 5, section 5.5).

**Bypass capacitor** — A capacitor that gives AC a short path to ground or chassis around a part, while
the part keeps its DC level. Many of the tube radio's stages use them (chapter 13).

**`CABLE S/FTP`** — On a connector pin of the drawings: the cable's shield and foil end there, on ground,
and are left unconnected at the other end unless the cable's page says otherwise (chapter 10,
section 10.1).

**CBB22** — A common type of metallised polypropylene film capacitor. One sits at the ADC's input
(chapter 6, section 6.4).

**`CHASSIS`** — The tube radio's circuit common: its metal chassis and everything joined to it. **It is on
mains neutral** and is joined to none of the other three grounds: Earth, the DC ground and the amplifier
supply return (chapter 2, section 2.3; chapter 5, section 5.3).

**Class A** — An amplifier stage whose tube conducts through the whole signal cycle. Appendix A gives
the 50C5's typical class-A operating points.

**Coax** — Coaxial cable: a centre conductor inside a tubular shield, made for radio signals. The FM
antenna's feedline is RG-6 coax; the radio's FM input takes RG179 coax (chapters 11 and 13).

**Common anode** — A display digit whose segment LEDs share one anode. The digit lights when its anode
is switched to the supply and a segment's cathode is pulled low (chapter 7).

**Common-mode choke** — Here, ferrite cores on a cable or a feedpoint. They resist currents that flow the
same way on all the conductors, such as noise picked up along a shield, while passing the signal.

**Coupling capacitor** — A capacitor that passes a signal from one point to the next while blocking DC.
The radio's FM antenna input reaches its first stage through one (C3) (chapter 13, section 13.2).

**CP210x** — A Silicon Labs USB-to-serial bridge chip. The audio board's USB port uses one (chapter 4).

**CTR** — Current transfer ratio: an optocoupler's output current as a share of its LED current
(Appendix A).

**DAC** — Digital-to-analogue converter: here a PCM5102A board that turns the audio board's digital
audio back into analogue audio for the amplifier (chapter 6).

**Darlington** — Two transistors in tandem that act as one with a very high gain. The ULN2003A holds
seven (Appendix A).

**DC ground (`GND`)** — The common return of everything on 5 V and 3.3 V. It reaches the amplifier's
ground and so is on Earth at almost 0 Ω; by which path it reaches the amplifier's ground is not recorded
(chapter 5, section 5.3).

**DCR** — The DC resistance of a coil, measured with an ohmmeter (chapter 13).

**De-emphasis** — A treble cut that undoes the treble boost added to FM broadcasts before they are
sent. The DAC's de-emphasis is switched off; the tube radio has its own (chapters 6 and 13).

**Decoupling capacitor** — A capacitor from a supply pin to ground, close to a part, that keeps the
supply steady when the part draws current in bursts.

**Detector** — The stage that recovers the audio from the IF signal. The tube radio has an AM detector
and, for FM, a ratio detector (chapter 13, sections 13.5 and 13.6).

**Development board** — A module on a small board with pin headers and a USB port. Both microcontroller
boards are development boards (chapter 4).

**Dielectric** — The insulating material inside a capacitor, which gives the capacitor its type: film,
ceramic, mica or electrolytic. Chapter 13 gives it where it is recorded.

**Director, reflector, driven element** — The elements of a Yagi antenna. Only the driven element is
wired to the cable; the reflector sits behind it and the director in front (chapter 11).

**Download mode** — The boot mode in which an ESP32 waits to receive new firmware instead of running its
program (Appendix A).

**Dual-gang, gang** — A gang is one section of a potentiometer or capacitor; a dual-gang part has two
sections turned by one shaft. The amplifier's volume control is a dual-gang potentiometer (chapter 12,
section 12.2).

**Earth** — The mains protective earth, from the mains inlet's earth pin. The Faraday cages, the antenna
connector's shell, the amplifier's ground, the DC ground and the amplifier supply return are on it
(chapter 5, section 5.3).

**Earth star point** — The earth point on the amplifier's aluminium back plate. The amplifier's ground sits
on it, through a washer on the Bass pot (chapter 5, section 5.3). Not the same as
the DC ground's *star point*.

**eFuse** — A one-time setting burned into a chip. The main board's flash voltage is set by eFuse
(chapter 4, section 4.1).

**EMI filter** — See *mains line filter*.

**ESP32, ESP32-S3** — Espressif's microcontroller families. The main board is built on an ESP32-S3, the
audio board on an ESP32 (chapter 4).

**F-type** — The screw-on coax connector of TV and satellite cables. The antenna's feedpoint connector
(an F-81 bulkhead) and its grounding block are F-type (chapter 11).

**Faraday cage** — An earthed metal enclosure that shields what is inside from electrical noise, and
the outside from it. The machine has three, made of aluminium tape (chapter 5, section 5.4).

**FCC ID** — The US radio-approval code printed on a module. Its first part, the grantee code, names the
company that obtained the approval (chapter 4; Appendix C).

**Feedline, feedpoint** — The feedline is the coax from the antenna to the radio. The feedpoint is where
the feedline joins the driven element, at a 20 mm gap in the folded dipole (chapter 11).

**Ferrite core** — A ring or sleeve of ferrite slipped over a cable to damp high-frequency noise on it
(chapter 10).

**Fitted, original** — In chapter 13, *fitted* is the part in this radio now; *original* is what the
factory drawing and the SAMS service data give.

**Flash** — Memory that keeps the program with the power off. The main board has 16 MB, the audio board
4 MB (chapter 4, section 4.1).

**Flyback diode** — A diode across a coil that absorbs the voltage spike when the coil is switched off.
The ULN2003A has them built in (Appendix A).

**FM, FM-AFC** — Frequency modulation, the VHF broadcast band. FM-AFC is the radio's FM position with
automatic frequency control switched on (chapter 13, section 13.1).

**Folded dipole** — A driven element made of two parallel conductors joined at both ends, fed at a gap
in one of them. The antenna's driven element is one (chapter 11).

**Gas discharge tube (GDT)** — A surge arrester often fitted in coax grounding blocks. The antenna's
grounding block has none (chapter 11).

**GPIO** — General-purpose input/output: a pin of a microcontroller that its program can read or drive.

**Grid leak** — A high-value resistor that sets a tube grid's DC level by letting charge leak away
(chapter 13).

**Grounding block** — A block that joins two lengths of coax. The antenna's is fitted outside, out of the
weather. An 8 AWG stranded copper run goes from the grounding block to a lug on the meter base, earthing
the coax shield (chapter 11, section 11.2).

**Hall-effect switch** — A sensor whose output switches when a magnet comes close enough. The needle's
index sensor, an A3144, is one (chapter 8, section 8.3).

**Header** — A row of pins on a board, onto which a connector plugs.

**Heater string** — The tube radio's seven tube heaters wired in series, one after the other, so one
current flows through all of them. One open heater stops them all (chapter 13, section 13.9).

**I2C** — A two-wire bus, data and clock, that a board uses to talk to several chips by address. The
angle sensor, the FM calibration receiver and the real-time clock use it.

**I2S** — A bus that carries digital audio between chips, with a bit clock, a word clock, a master clock
and a data line per direction (chapter 6, section 6.2).

**IEC C14** — The standard three-pin mains inlet, the same kind as on a computer. The machine's mains
inlet is one (chapter 5).

**IF** — Intermediate frequency: the fixed frequency to which a superheterodyne turns every station. The
tube radio's FM IF is 10.6 MHz, its AM IF 455 kHz (chapter 13).

**IF transformer** — A tuned transformer that passes the IF from one stage to the next, in a shielded
metal can with its own tuning capacitors (chapter 13).

**IN/LISTENER/OUT board** — The drawings' name for the source-switch board: the small board, in its own
printed case, that passes the audio board's 3.3 V and the two source-switch legs on to the front
switch. Its connector is named `LISTENER` (chapter 10, section 10.20).

**Index** — The needle's one fixed reference point: a magnet on the needle mount passing over a
Hall-effect switch (chapter 8, section 8.3).

**Isolation transformer** — A transformer that separates a circuit from the mains. The tube radio has
none, which is why its chassis is on mains neutral.

**JST XH** — A family of small connectors with a 2.5 mm pitch, used for the 5 V and 3.3 V buses
(Appendix A).

**JTAG** — A debug interface for chips. On the main board, GPIO39 to GPIO42 are the chip's JTAG pins;
the machine uses them for other jobs (chapter 4; Appendix A).

**Kapton** — A heat-resistant insulating tape, used to cover the aluminium tape of the Faraday cages.

**Ladder** — Here, the resistor divider that turns the three positions of the source switch into three
voltages for the audio board (chapter 9, section 9.3).

**LED** — Light-emitting diode.

**Leditron** — The name of the clock display: four seven-segment digits and a colon, made from single
LEDs (chapter 7).

**Limiter** — An FM stage that clips the IF signal to a steady level, so that changes in strength do not
reach the detector (chapter 13).

**Listed substitute** — A catalogue replacement part named for one of the tube radio's original parts
in the author's unpublished component list. The list is not in this book; chapter 13 names a substitute
only where it tells something about the part (chapter 13, section 13.1.3).

**Local oscillator** — The oscillator inside a superheterodyne whose signal the mixer combines with the
station. The tube radio's FM local oscillator runs one IF below the station (chapter 13).

**Logic-level MOSFET** — A MOSFET that turns fully on with a gate voltage a logic pin can give. The
panel-lamp switch, an IRL540N, is one (chapter 9, section 9.1).

**Loopstick** — A coil wound on a ferrite rod that serves as an AM antenna. The tube radio's AM input uses
its internal loopstick (chapter 13).

**LVCMOS** — Low-voltage CMOS: a family of logic input and output levels (Appendix A).

**Main board** — The ESP32-S3 development board that runs the clock display, the dial needle and its
sensors, the FM calibration receiver and the panel lamps. Also written S3 (chapter 4).

**Mains line filter** — The EMI (electromagnetic interference) filter between the switched mains and the
tube radio, under its own Faraday cage (chapter 5, section 5.2).

**Microcontroller, MCU** — A small computer on one chip, with its own inputs and outputs. The machine has
two, on the main board and the audio board (chapter 4).

**Mixer** — The stage of a superheterodyne that combines the station with the local oscillator's signal
to make the IF (chapter 13).

**Mode switch** — The tube radio's four-position switch: SW, AM, FM and FM-AFC. The drawing splits it
into seven units, S1A to S1G, that turn together (chapter 13, section 13.1.6).

**Module** — The small shielded board that carries a microcontroller chip with its flash memory. Each
development board carries one (chapter 4).

**MOSFET, P-channel MOSFET** — A transistor switched by the voltage on its gate. A P-channel MOSFET
switches a load to the supply; the clock digits are switched by four of them, FQP27P06 (chapter 7).

**Neon lamp** — A small glow lamp that runs from mains voltage. The tube radio's mode lamps (NE1 to NE3)
are drawn but not fitted (chapter 13, section 13.1.6).

**New old stock** — A part made long ago and never used. Three of the radio's tubes are new old stock
(chapter 13).

**NP0** — A ceramic capacitor dielectric whose value hardly changes with temperature, used where a value
must stay exact (chapter 13).

**Op-amp** — Operational amplifier: a small amplifier chip used for signal stages. The amplifier's input
board holds three (chapter 12, section 12.1).

**Open collector** — An output that can only pull its line low; a pull-up resistor sets the high level.
The index sensor and the optocoupler have one.

**Optocoupler** — A part that passes a signal across by light, an LED lighting a phototransistor, with no
wire between the two sides. The amplifier power sensor uses a PC817 (chapter 9, section 9.5).

**Output transformer** — The transformer between the radio's output tube and its load. In this machine
its secondary feeds a 7.5 Ω resistor and the ADC, not speakers (chapter 6, section 6.4).

**Package (TO-220, DIP-4, QFN56)** — The body and pin layout of a chip or transistor. TO-220 is a power
package with three legs and a metal tab; DIP-4 a four-pin through-hole package; QFN56 a flat chip
package with 56 pads (Appendix A).

**Padder, trimmer** — Small capacitors in a tuned circuit. A trimmer is adjustable and sets the end of a
tuning range; a padder, in series with the oscillator's tuning, keeps the oscillator in step with the
signal circuits across the band (chapter 13).

**Passive radiator** — A speaker cone with no magnet or coil of its own, moved by the air in the
enclosure, used in place of a port (chapter 12, section 12.1).

**Pentagrid converter, pentode, triode, beam power tube** — Tube types. A triode has one grid, a pentode
three; a pentagrid converter has five grids and is mixer and oscillator in one tube; a beam power tube is
built to deliver power to a load (chapter 13).

**Perfboard** — A board with a grid of holes for hand-wired circuits. Several of the machine's small
circuits are built on perfboard.

**PLA, TPU, STL** — PLA is the plastic of the printed parts; TPU is a flexible printing plastic, used
only for the passive radiators' membranes; an STL is the file a printed part is made from (Appendix D).

**Plate, cathode, control grid** — Electrodes of a tube. The heated cathode gives off electrons, the plate
(the anode) collects them, and the control grid between them sets how many get through (chapter 13).

**PLL** — Phase-locked loop: a circuit that makes one clock from another. The DAC can make its own
master clock this way from the bit clock (Appendix A).

**Polarised capacitor** — A capacitor with a + and a − terminal, such as an electrolytic, that must be
fitted the right way round. In the tube radio's polarised capacitors (C1A to C1C and C2), pin 1 is the +
terminal (Appendix B).

**PSRAM** — Extra memory beside a microcontroller's own. The main board's chip carries 8 MB (chapter 4).

**PSU** — Power supply unit. Used in drawing names and printed-part file names.

**Pull-up, pull-down** — A resistor that holds a line high (to the supply) or low (to ground) when
nothing else drives it.

**PWM** — Pulse-width modulation: switching a load on and off fast, with the on-time setting the average
power. The FM panel lamps are dimmed by PWM (chapter 9, section 9.1).

**Ratio detector** — The tube radio's FM detector: two diodes and a transformer that turn frequency
changes into audio and largely ignore changes in strength (chapter 13, section 13.5).

**RCA** — The common phono jack for line-level audio. The amplifier's input panel has an RCA pair;
nothing is recorded on it (chapter 12).

**Rectifier** — A part that turns AC into DC. The tube radio's rectifier (X1) makes its B+ from the mains
(chapter 13).

**RF** — Radio frequency: the station's signal as it arrives, before the mixer turns it into the IF. On
FM, the RF amplifier (V1) raises it first (chapter 13, section 13.2).

**RMS** — Root mean square. A power rating in W RMS is a continuous power, not a peak.

**RTC** — Real-time clock: a chip that keeps the time; a backup cell on its module keeps it running
while the power is off. Here a DS3231 module (chapter 4, section 4.7).

**S3** — The main board, an ESP32-S3 development board (chapter 4).

**`S3_3V3`** — The main board's own 3.3 V output. It is a separate rail, not the 3.3 V bus.

**S/FTP** — See *shield and foil*.

**Sample rate** — The number of audio samples per second on each channel.

**SAMS** — SAMS Photofact, the published service data for the tube radio. With the factory drawing, it is
one of the radio's two original documents.

**Schmitt trigger** — An input with two switching thresholds, one for a rising and one for a falling
signal, so that a slow or noisy signal switches cleanly (Appendix A).

**Screen grid** — The grid of a pentode or beam tube between the control grid and the plate, held at a
steady positive voltage (chapter 13).

**SDA, SCL** — The data and clock lines of an I2C bus.

**Shield and foil** — The braided shield and the foil wrap of a shielded cable. The machine's shielded
cables are S/FTP: a braided shield and a foil around twisted pairs (chapter 10, section 10.1).

**Slave mode** — A converter mode in which the chip takes its clocks from outside instead of making them.
The ADC runs in slave mode, clocked by the audio board (chapter 6).

**Source switch** — The front three-position switch: RADIO, BT and AUX. It only tells the audio board
which source was chosen; it switches no audio (chapter 9, section 9.3).

**SPI, quad SPI, octal SPI** — A serial link between a chip and its memory, with four (quad) or eight
(octal) data lines. On the main board's module variant (N16R8), the maker gives quad SPI flash and
octal SPI PSRAM (Appendix A).

**Standoff** — A spacer post that holds a board or a supply off a surface. The amplifier's ±20 V supply
is held off the back plate by plastic screws and standoffs (chapter 5, section 5.3).

**Star point** — The one place where all the DC ground wires meet: the ground pins of the 5 V and 3.3 V
buses (chapter 5, section 5.5). See also *earth star point*.

**Stepper motor** — A motor that turns in fixed steps, one for each change of its coil currents. The
needle's motor is a 28BYJ-48 (chapter 8, section 8.2).

**Strapping pin** — A microcontroller pin whose level is read at power-up to set how the chip starts
(Appendix A).

**Superheterodyne** — A receiver that turns every station into one fixed intermediate frequency (IF)
before amplifying and detecting it (chapter 13, section 13.1.1).

**Suppressor grid** — The grid of a pentode nearest the plate. It stops electrons knocked off the plate
from reaching the screen grid. In the radio's 12BA6 IF tubes (V4, V5) it is on the chassis (chapter 13,
section 13.4).

**SW** — Short wave, one of the tube radio's bands.

**SWR** — Standing-wave ratio: a measure of how well an antenna matches its cable. Not measured on this
antenna (chapter 11).

**Taper** — How a potentiometer's resistance changes along its travel. An audio taper changes slowly at
first, to suit the ear. The radio's volume control is audio taper, as its original specification infers
from its listed substitutes (chapter 13).

**Tertiary winding** — A third winding on a transformer, besides the primary and the secondary. The
ratio-detector transformer (L14) has one (chapter 13, section 13.4).

**TRS** — Tip, ring, sleeve: a three-contact jack or plug, as on stereo headphones (chapter 6).

**Tube base** — The pin layout of a tube. B7G is the 7-pin miniature base; B9A, or noval, the 9-pin
miniature. An EIA basing code (such as 7CH or 9DE) says which electrode is on which pin (chapter 13,
section 13.1.4).

**UART** — A two-wire serial link, one wire each way, plus ground. The two boards talk over one (chapter
10, section 10.12).

**USB-Serial/JTAG** — The ESP32-S3 chip's own USB port, which serves as both a serial link and a debug
interface. The main board's USB port uses it (chapter 4).

**VBUS** — The 5 V power wire of a USB cable (chapter 4, section 4.6).

**VHF** — Very high frequency: the radio band that holds the FM broadcast band (chapters 11 and 13).

**Wiper** — The sliding contact of a potentiometer. Its voltage follows the knob's position (chapter 9,
section 9.4).

**X1Y1, X1Y2** — Safety ratings for capacitors connected to the mains. An X rating is for a capacitor
across the mains, a Y rating for one between the mains and a part a person may touch; X1Y1 and X1Y2 parts
carry both. Replace such a part only with one of the same class (chapter 2, section 2.4).

**Yagi** — A directional antenna: a driven element with unpowered elements beside it on a boom. The FM
antenna is a three-element Yagi (chapter 11).

**Zero-data mute** — The DAC's own mute: after a run of all-zero audio on both channels, it silences its
output (Appendix A).
