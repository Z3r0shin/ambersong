# 11. The FM antenna

The tube radio receives FM from an outdoor antenna that the author built for this machine: a
three-element Yagi on the roof. Any FM or VHF antenna will work on the radio's antenna input. This
chapter describes this one, as built and installed, so that you can check it, repair it or build your own.
Its dimensions, mounting and aim suit this house and this location. Adapt them to yours.

Sections 11.1 and 11.2 are the author's own figures. Section 11.3 gives further details from the build
notes; those details are not confirmed one by one, so they are weaker than 11.1 and 11.2.

## 11.1 The aerial

A **Yagi** is a directional antenna: one **driven element**, the only part wired to the cable, with
unpowered **parasitic** elements beside it on a common support, the **boom**. The longer **reflector**
sits behind the driven element and the shorter **director** in front, so the antenna listens best in the
direction of the director.

This one is a **three-element Yagi**, mounted horizontally on the roof. The table gives each part's
material and size.

| Part | Material | Size |
|---|---|---|
| **Director** | plain aluminium U-channel, 3.2 mm thick, 12.6 mm high, 25.5 mm wide | **133.8 cm** long |
| **Driven element** | **½ in type L copper tube**, folded (below) | 139 cm per element; **about 1390 mm** tip to tip |
| **Reflector** | the same aluminium U-channel as the director | **150.9 cm** long |
| **Boom** | **3030 aluminium extrusion** | **1170 mm** long |

**The driven element is a folded dipole:** two parallel copper tubes, joined at both ends by soldered
90-degree tubing, with the two conductors **50 mm** apart, centre to centre. Only this element is wired
to the coax.

**Nothing touches the boom electrically.** The director and the reflector touch neither the boom nor
anything else. The driven element reaches the boom only through a non-conductive clamp.

## 11.2 The feedline and its earthing

The **feedline** is the coax cable that carries the signal from the antenna down to the radio. The table
gives how the feedline, its earthing and the mast are installed.

| What | As installed |
|---|---|
| **Feedline** | Coax. At present it is at least 100 ft long. At its final place it is to be about 50 ft in total, from the bulkhead (the F-81 connector at the feedpoint, section 11.3) to the radio. |
| **Grounding block** | Fitted outside, out of the weather. Its exact position does not matter. |
| **Earth wire** | **8 AWG stranded copper, ferruled**, run from the grounding block to the electricity meter box (Hydro-Québec) and landed **on a lug on the meter base**, beside one an electrician made for the external wiring. |
| **Mast** | The same 3030 aluminium extrusion as the boom, **4 ft high**, fixed to the roof with continuous aluminium extrusion. The mast is centred on the boom. |
| **Height and aim** | About **4 ft over its base** and **1 ft over the tip of the roof**, clear of reflections from the roof. The roof's slope faces away from Mount Royal; the aerial points **as near Mount Royal as it can**. |
| **Guys** | Dacron lines, **two from each end of the boom**. As fitted, the antenna cannot move short of a very big accident. |

**The coax shield is earthed at the grounding block.** The grounding block is outside. An 8 AWG
stranded copper run goes from the grounding block to a lug on the meter base.

**The boom is earthed through the structure.** The boom is continuous metal through the mast to the
roof's metal. The metal roof is electrically connected to the electrical mast, and so to earth. The boom
and the coax shield's 8 AWG run therefore reach the same earth electrode system.

**What follows from this.** The elements sit on insulating bushings (section 11.3), so they are isolated
from that earthed structure. The coax shield is earthed by its own 8 AWG run from the grounding block to
the meter-base lug, not by a separate wire from the boom.

**At the radio end.** The coax arrives at the machine's antenna connector, a BNC (J10) on the back
panel. Its centre carries the antenna signal, `ANT`; its shell, and so the coax shield, is on Earth. The
tube radio's own antenna input is another coax connector (J53); no cable joining J10 to J53 is
recorded. Chapter 5, section 5.4, gives the earth path inside the cabinet. Chapter 13, section 13.11,
gives the radio's side, where the shield reaches the chassis only through one safety capacitor (C51).

> **Caution — the shield and the chassis.** The coax shield is on Earth. The radio's chassis is on mains
> neutral and is never joined to Earth. The shield's only path to the radio's chassis is the safety
> capacitor (C51). Chapter 2 explains what the chassis is joined to and why it matters.

## 11.3 Build details from the notes

These details come from the build notes and are not confirmed one by one.

**Element positions.** The table gives where each element sits along the boom, with the reflector at
0 cm.

| Element | Position |
|---|---|
| Reflector | 0 cm |
| Driven element | 55 cm |
| Director | 110 cm |

**Elements.** Both U-channels are mounted opening downwards. The centres of the three elements lie in
one plane, within 5 mm.

**Feed gap.** A **20 mm** gap is cut in one tube of the folded dipole only. The feedpoint is there.

**How each element is mounted.** Each element sits in a stack of parts on the boom, listed here from
the top down:

1. a nut;
2. a Belleville washer (a dished spring washer);
3. a neoprene washer, of the kind used with metal-roofing screws;
4. a G10 washer (glass-epoxy laminate);
5. the element, in an ASA bushing;
6. an ASA plate, continuous with that bushing;
7. the boom, through a 90-degree clamp.

The driven element is mounted the same way. **No static-bleed resistors are fitted.** These would be
1 MΩ resistors from the parasitic elements to the boom, to let static charge drain away.

**Feedpoint.** The feedpoint is where the coax joins the driven element. An **F-81 bulkhead** connector
(an F-type coax connector made to pass through a panel) sits in the ASA clamp. Its shell is on one side
of the 20 mm gap, its centre pin on the other. **Six salvaged HDMI ferrite sleeves** (17.4 mm outside, 9.7 mm inside,
28.5 mm long) sit on the feedpoint as a **common-mode choke**, to keep signal currents off the outside of
the coax shield.

**Feedline and grounding block.** The feedline is **RG-6** coax. The grounding block is a **Perfect
Vision PVGB1HFWS**: UL listed, rated to 3 GHz, F-type, single, with a weather boot. It is a plain block.
It has **no gas discharge tube** (no surge arrester).

## 11.4 What is not measured

- **No performance figure is recorded:** no SWR (standing-wave ratio, a measure of how well the antenna
  matches its cable), no field strength, and no comparison of reception with another antenna.
- **No drawing shows the antenna.** The drawings show only the radio end: the antenna connector and the
  radio's antenna input.
- **The cable from the antenna connector to the radio's antenna input** is not recorded (chapter 10,
  section 10.35). How the RG-6 feedline is fitted to the BNC antenna connector (J10) is not recorded
  either.

## 11.5 When something is wrong

The table lists what you may notice and where to look first.

| You see | Check |
|---|---|
| FM weak or noisy on every station, while AM and SW work | The FM antenna path, which only FM uses: the antenna connector (J10), the coax inside the cabinet, the radio's antenna input (J53) and its coupling capacitor (C3) (chapter 13, section 13.2). Then the coax outside, the grounding block and the feedpoint. |
| The aerial has been moved or re-aimed | Its aim (as near Mount Royal as it can, here) and its height clear of the roof (section 11.2). |
