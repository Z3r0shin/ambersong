# Ambersong hardware

The design files of Ambersong. What each part is, how it is wired and with which values is in the Hardware Bible:
the [PDF](../docs/Ambersong%20-%20Hardware%20Bible.pdf), or [`docs/hardware-bible.md`](../docs/hardware-bible.md)
for an LLM. Open the PDF first.

| Path | What is there |
|---|---|
| [`schematics/Ambersong.kicad_pro`](schematics/Ambersong.kicad_pro) | The KiCad 9 project. Open it in KiCad; the main sheet is `Ambersong.kicad_sch` |
| `schematics/Ambersong.kicad_sch` | The main sheet: every board, module and connection as built, one box per circuit |
| `schematics/Ambersong - *.kicad_sch` | One sheet per circuit, drawn in more detail, and the cable sheet (`Ambersong - CABLES.kicad_sch`) |
| [`schematics/Ambersong PDF format.pdf`](schematics/Ambersong%20PDF%20format.pdf) | All the sheets as one PDF, for reading without KiCad |
| [`schematics/LLOYDS TM-838 factory schematic.jpg`](schematics/LLOYDS%20TM-838%20factory%20schematic.jpg) | The radio maker's original schematic, for reference only. The KiCad tube radio sheet supersedes it |
| [`STLs/`](STLs/) | The 76 printed parts, named by where they go: `FRONT`, `BACK`, `IN` (inside the cabinet), `AUDIO` (the passive radiator and the speaker holder), `LEDITRON` (the clock digits) |
| [`STLs/Ambersong - Sketchup.skp`](STLs/Ambersong%20-%20Sketchup.skp) | The SketchUp model of the whole cabinet with every printed part in place |

The sheets show what was built, drawn to be read. They are not a PCB layout, so KiCad's electrical rules check
does not pass on them.

Licence: CERN-OHL-S-2.0 ([`LICENSE-CERN-OHL-S-2.0`](../LICENSE-CERN-OHL-S-2.0)). The running hare on the dial
artwork and in the SketchUp model is not covered by it: see [`NOTICE`](../NOTICE).

Back to the [README](../README.md).
